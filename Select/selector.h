#pragma once

#include "file_walker.h"
#include "function_extractor.h"
#include <vector>
#include <optional>
#include <tree_sitter/api.h>
#include <cstdint>

struct SelectResult {
    std::vector<FunctionRecord> chosen;
    long long                   totalFunctions;
    long long                   totalFiles;
};

std::optional<SelectResult> reservoirSample(
    FileWalker&                      walker,
    TSParser*                        parser,
    const std::vector<std::string>&  functionNodeTypes,
    const std::string&               langStr,
    int                              k,
    int                              minLines,
    int                              minChars,
    uint64_t                         seed
);
