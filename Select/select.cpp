#include <iostream>
#include <string>
#include <filesystem>
#include <optional>
#include <vector>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <random>
#include <tree_sitter/api.h>

#include "lang_registry.h"
#include "file_walker.h"
#include "function_extractor.h"
#include "selector.h"

void printUsage() {
    std::cerr << "Usage:\n"
              << "  ./select --root <dir>\n"
              << "           --lang <c|cpp|java|csharp>\n"
              << "           --count <N>\n"
              << "           --output-dir <dir>\n"
              << "           [--seed <uint64>]\n"
              << "           [--min-lines <int>]    (default 3)\n"
              << "           [--min-chars <int>]    (default 50)\n"
              << "           [--help]\n";
}

int main(int argc, char** argv) {
    std::string rootDir;
    std::string langStr;
    int count = 0;
    std::string outputDir;
    
    std::random_device rd;
    uint64_t seed = rd();
    int minLines = 3;
    int minChars = 50;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage();
            return 0;
        } else if (arg == "--root" && i + 1 < argc) {
            rootDir = argv[++i];
        } else if (arg == "--lang" && i + 1 < argc) {
            langStr = argv[++i];
        } else if (arg == "--count" && i + 1 < argc) {
            count = std::stoi(argv[++i]);
        } else if (arg == "--output-dir" && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stoull(argv[++i]);
        } else if (arg == "--min-lines" && i + 1 < argc) {
            minLines = std::stoi(argv[++i]);
        } else if (arg == "--min-chars" && i + 1 < argc) {
            minChars = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            printUsage();
            return 1;
        }
    }

    if (rootDir.empty() || langStr.empty() || count <= 0 || outputDir.empty()) {
        std::cerr << "Error: Missing required arguments or invalid count.\n";
        printUsage();
        return 1;
    }

    if (!std::filesystem::is_directory(rootDir)) {
        std::cerr << "Error: --root is not a valid directory.\n";
        return 1;
    }

    LangConfig config;
    try {
        config = getLangConfig(langStr);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, config.grammar);

    FileWalker walker(rootDir, config.extensions);

    auto resultOpt = reservoirSample(walker, parser, config.functionNodeTypes, langStr, count, minLines, minChars, seed);

    if (!resultOpt) {
        std::cerr << "Error: No qualifying functions found.\n";
        ts_parser_delete(parser);
        return 1;
    }

    auto& result = *resultOpt;
    if (result.chosen.size() < (size_t)count) {
        std::cerr << "Warning: only " << result.chosen.size() << " qualifying functions "
                  << "found (requested " << count << "). Writing " << result.chosen.size() << " files.\n";
    }

    std::filesystem::create_directories(outputDir);
    int actualCount = result.chosen.size();
    int width = std::to_string(count).size();

    std::vector<std::string> outputFiles;

    for (int i = 0; i < actualCount; ++i) {
        std::ostringstream filename;
        filename << "func" << std::setfill('0') << std::setw(width) << (i + 1) << ".txt";
        std::filesystem::path outPath = std::filesystem::path(outputDir) / filename.str();

        std::ofstream out(outPath);
        if (out) {
            const auto& f = result.chosen[i];
            
            auto relPath = std::filesystem::proximate(f.sourceFile, rootDir);
            
            out << "// SOURCE: " << relPath.string() << "\n";
            out << "// LINES:  " << f.startLine << "-" << f.endLine << "\n";
            out << "// LENGTH: " << f.numLines << " lines\n";
            out << "// LANG:   " << langStr << "\n";
            out << "// FUNC:   " << f.funcName << "\n";
            out << f.text << "\n";
            
            outputFiles.push_back(outPath.string());
        }
    }

    std::cerr << "[SELECT] Scanned " << result.totalFiles << " files, found " << result.totalFunctions << " qualifying functions.\n";
    std::cerr << "[SELECT] Randomly selected " << actualCount << " functions (seed: " << seed << ").\n";
    if (!outputFiles.empty()) {
        std::cerr << "[SELECT] Written to: " << outputFiles.front() << " ... " << outputFiles.back() << "\n";
    }

    ts_parser_delete(parser);
    return 0;
}
