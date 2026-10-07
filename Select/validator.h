#pragma once

#include <tree_sitter/api.h>
#include <string>

// Checks if a function node meets the criteria to be mutated by ALL 15 mutators.
bool validateFunctionCandidate(const std::string& funcText, const std::string& langStr);
