#include "../mutation_base.h"
#include <vector>
#include <string>

// mRL_S — Change a string or char literal value.
// For STRING_LIT: append "_mutated" inside the quotes.
// For CHAR_LIT:   replace the inner character with 'X'.
// One mutant per string/char literal token.
class mRL_S : public MutationBase {
public:
    std::string name()        const override { return "mRL_S"; }
    std::string cloneType()   const override { return "Type-2"; }
    std::string description() const override {
        return "Change the value of a string or character literal.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            const Token& tok = tokens[i];
            if (tok.type != Token::Type::STRING_LIT &&
                tok.type != Token::Type::CHAR_LIT) continue;

            std::string changed = perturbLiteral(tok);
            if (changed.empty()) continue;

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                mutant += (j == i) ? changed : tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }

private:
    static std::string perturbLiteral(const Token& tok) {
        const std::string& v = tok.value;
        if (tok.type == Token::Type::STRING_LIT) {
            // Find the closing delimiter and insert before it
            // Handle @"" (C# verbatim) and normal "..."
            std::size_t close = v.rfind('"');
            if (close == std::string::npos || close == 0) return "";
            return v.substr(0, close) + "_mutated" + v.substr(close);
        }
        if (tok.type == Token::Type::CHAR_LIT) {
            // Replace inner char with 'X', unless it's already 'X'
            if (v.size() < 3) return "";
            // Simple approach: open quote + X + close quote
            // Preserve prefix if any
            std::size_t open = v.find('\'');
            if (open == std::string::npos) return "";
            std::size_t close = v.rfind('\'');
            if (close == open) return "";
            std::string inner = v.substr(open + 1, close - open - 1);
            if (inner == "X") inner = "Y";
            else inner = "X";
            return v.substr(0, open + 1) + inner + "'";
        }
        return "";
    }
};
