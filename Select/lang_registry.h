#pragma once

#include <string>
#include <vector>
#include <tree_sitter/api.h>

struct LangConfig {
    const TSLanguage*        grammar;
    std::vector<std::string> functionNodeTypes;
    std::vector<std::string> extensions;
};

LangConfig getLangConfig(const std::string& lang);
