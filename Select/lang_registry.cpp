#include "lang_registry.h"
#include <stdexcept>

extern "C" const TSLanguage* tree_sitter_c();
extern "C" const TSLanguage* tree_sitter_cpp();
extern "C" const TSLanguage* tree_sitter_java();
extern "C" const TSLanguage* tree_sitter_c_sharp();

LangConfig getLangConfig(const std::string& lang) {
    if (lang == "c") {
        return {
            tree_sitter_c(),
            {"function_definition"},
            {".c", ".h"}
        };
    } else if (lang == "cpp") {
        return {
            tree_sitter_cpp(),
            {"function_definition"},
            {".cpp", ".hpp", ".cc", ".cxx", ".h"}
        };
    } else if (lang == "java") {
        return {
            tree_sitter_java(),
            {"method_declaration", "constructor_declaration"},
            {".java"}
        };
    } else if (lang == "csharp") {
        return {
            tree_sitter_c_sharp(),
            {"method_declaration", "constructor_declaration"},
            {".cs"}
        };
    }
    throw std::invalid_argument("Unsupported language: " + lang);
}
