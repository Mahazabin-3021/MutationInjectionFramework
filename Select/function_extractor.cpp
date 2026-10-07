#include "function_extractor.h"
#include "validator.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

static std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::string getFunctionName(TSNode funcNode, const std::string& sourceText) {
    // We will attempt to find the identifier child for the function node.
    // C/C++: declarator -> identifier
    // Java/C#: name (type identifier)

    std::string name = "";

    // A helper lambda for DFS to find the first node of type "identifier" inside a "declarator" or "name" or directly.
    // Actually, simpler heuristic for tree-sitter: find the first child named "declarator" then "identifier",
    // or direct child "name".
    
    // Check direct children for "declarator" or field "name"
    TSNode declaratorNode = ts_node_child_by_field_name(funcNode, "declarator", 10);
    if (!ts_node_is_null(declaratorNode)) {
        // C/C++ pattern
        TSNode idNode = ts_node_child_by_field_name(declaratorNode, "declarator", 10); // sometimes it's nested
        if (ts_node_is_null(idNode)) {
            // Find "identifier" inside declarator
            uint32_t count = ts_node_named_child_count(declaratorNode);
            for (uint32_t i = 0; i < count; i++) {
                TSNode child = ts_node_named_child(declaratorNode, i);
                if (std::string(ts_node_type(child)) == "identifier") {
                    idNode = child;
                    break;
                }
            }
            if (ts_node_is_null(idNode) && std::string(ts_node_type(declaratorNode)) == "identifier") {
                idNode = declaratorNode;
            }
        }
        
        if (!ts_node_is_null(idNode)) {
            uint32_t start = ts_node_start_byte(idNode);
            uint32_t end = ts_node_end_byte(idNode);
            if (end > start) {
                return sourceText.substr(start, end - start);
            }
        }
    }

    TSNode nameNode = ts_node_child_by_field_name(funcNode, "name", 4);
    if (!ts_node_is_null(nameNode)) {
        // Java/C# pattern
        uint32_t start = ts_node_start_byte(nameNode);
        uint32_t end = ts_node_end_byte(nameNode);
        if (end > start) {
            return sourceText.substr(start, end - start);
        }
    }

    // Fallback: DFS for "identifier"
    std::vector<TSNode> stack;
    stack.push_back(funcNode);
    while (!stack.empty()) {
        TSNode node = stack.back();
        stack.pop_back();

        std::string type = ts_node_type(node);
        if (type == "identifier" && !ts_node_eq(node, funcNode)) {
            uint32_t start = ts_node_start_byte(node);
            uint32_t end = ts_node_end_byte(node);
            return sourceText.substr(start, end - start);
        }

        uint32_t count = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < count; i++) {
            stack.push_back(ts_node_named_child(node, count - 1 - i));
        }
    }

    return name;
}

static bool has_valid_declarator(TSNode node) {
    std::string type = ts_node_type(node);
    if (type == "function_declarator") return true;
    uint32_t count = ts_node_child_count(node);
    for (uint32_t i = 0; i < count; i++) {
        if (has_valid_declarator(ts_node_child(node, i))) return true;
    }
    return false;
}

std::vector<FunctionRecord> extractFunctions(
    const std::filesystem::path& filePath,
    TSParser*                    parser,
    const std::vector<std::string>& functionNodeTypes,
    const std::string&           langStr,
    int minLines,
    int minChars
) {
    std::string sourceText = readFile(filePath);
    if (sourceText.empty()) return {};

    TSTree* tree = ts_parser_parse_string(parser, NULL, sourceText.c_str(), sourceText.length());
    if (!tree) return {};

    std::vector<FunctionRecord> results;
    TSNode root = ts_tree_root_node(tree);

    std::vector<TSNode> stack;
    stack.push_back(root);

    // Get the language name from parser by checking extensions roughly or just checking if functionNodeTypes has "function_definition"
    bool isCppOrC = std::find(functionNodeTypes.begin(), functionNodeTypes.end(), "function_definition") != functionNodeTypes.end();

    while (!stack.empty()) {
        TSNode node = stack.back();
        stack.pop_back();

        std::string type = ts_node_type(node);
        bool isFunction = std::find(functionNodeTypes.begin(), functionNodeTypes.end(), type) != functionNodeTypes.end();

        if (isFunction) {
            // Filter out macro blocks like TEST(...) in C/C++
            if (isCppOrC && !has_valid_declarator(node)) {
                // Not a valid function declarator (likely a macro block)
                continue;
            }

            uint32_t start_byte = ts_node_start_byte(node);
            uint32_t end_byte = ts_node_end_byte(node);
            
            if (end_byte > start_byte) {
                std::string funcText = sourceText.substr(start_byte, end_byte - start_byte);
                std::string funcName = getFunctionName(node, sourceText);

                if (funcName != "main" && funcName != "Main") {
                    // Check if the function name is entirely uppercase (macro like TEST)
                    bool isAllUpper = true;
                    for (char c : funcName) {
                        if (std::isalpha(c) && !std::isupper(c)) {
                            isAllUpper = false;
                            break;
                        }
                    }

                    if (!isAllUpper && funcName.length() > 0) {
                        uint32_t start_point = ts_node_start_point(node).row;
                        uint32_t end_point = ts_node_end_point(node).row;
                        int lines = end_point - start_point + 1;
                        int chars = end_byte - start_byte;

                        if (lines >= minLines && chars >= minChars) {
                            if (validateFunctionCandidate(funcText, langStr)) {
                            results.push_back({
                                funcText,
                                filePath,
                                funcName,
                                (int)start_point + 1,
                                (int)end_point + 1,
                                lines
                            });
                        }
                        }
                    }
                }
            }
            // Do not recurse into this function to avoid nested
            continue;
        }

        uint32_t child_count = ts_node_named_child_count(node);
        for (uint32_t i = 0; i < child_count; i++) {
            stack.push_back(ts_node_named_child(node, child_count - 1 - i));
        }
    }

    ts_tree_delete(tree);
    return results;
}
