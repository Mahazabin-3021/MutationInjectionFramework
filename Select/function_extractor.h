#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <tree_sitter/api.h>

struct FunctionRecord {
    std::string              text;
    std::filesystem::path    sourceFile;
    std::string              funcName;
    int                      startLine;
    int                      endLine;
    int                      numLines;
};

std::vector<FunctionRecord> extractFunctions(
    const std::filesystem::path& filePath,
    TSParser*                    parser,
    const std::vector<std::string>& functionNodeTypes,
    const std::string&           langStr,
    int minLines = 3,
    int minChars = 50
);
