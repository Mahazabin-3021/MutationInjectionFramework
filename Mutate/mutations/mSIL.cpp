#include "../mutation_base.h"
#include <vector>
#include <string>

// mSIL — Small Insertion within a Line
// Add an extra parameter to a randomly chosen function call or signature.

class mSIL : public MutationBase {
public:
    std::string name()        const override { return "mSIL"; }
    std::string cloneType()   const override { return "Type-3"; }
    std::string description() const override {
        return "Add a parameter to a function call or signature.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        // Find every '(' that is preceded by an IDENTIFIER (function name).
        // Then find the matching ')'. If there's already at least one param, insert
        // after the last param. Otherwise insert "0" as sole param.
        for (std::size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i].value != "(") continue;
            if (tokens[i].type != Token::Type::DELIMITER) continue;
            // Previous non-ws token must be IDENTIFIER or KEYWORD
            int prev = -1;
            for (int j = (int)i - 1; j >= 0; --j) {
                if (tokens[j].type == Token::Type::WHITESPACE) continue;
                prev = j; break;
            }
            if (prev < 0) continue;
            if (tokens[prev].type != Token::Type::IDENTIFIER &&
                tokens[prev].type != Token::Type::KEYWORD) continue;

            // Skip control-flow keywords — they are not function calls
            static const std::vector<std::string> controlFlow = {
                "for", "while", "if", "switch", "do", "else", "catch"
            };
            bool isControlFlow = false;
            for (const auto& kw : controlFlow)
                if (tokens[prev].value == kw) { isControlFlow = true; break; }
            if (isControlFlow) continue;

            // Find matching ')'
            int depth = 1;
            std::size_t close = i + 1;
            for (; close < tokens.size() && depth > 0; ++close) {
                if (tokens[close].value == "(") ++depth;
                else if (tokens[close].value == ")") --depth;
            }
            if (depth != 0) continue;
            --close; // now tokens[close].value == ")"

            // Determine whether the parameter list is empty (no real tokens between '(' and ')')
            bool hasParams = false;
            for (std::size_t k = i + 1; k < close; ++k) {
                auto t = tokens[k].type;
                if (t != Token::Type::WHITESPACE &&
                    t != Token::Type::NEWLINE &&
                    t != Token::Type::END_OF_FILE) {
                    hasParams = true;
                    break;
                }
            }

            // Insert before the closing ')':
            //   if params exist: ", _extra_param_"
            //   if list is empty: "_extra_param_"  (no leading comma)
            const std::string insertion = hasParams ? ", _extra_param_" : "_extra_param_";

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == close) mutant += insertion;
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
