#include "../mutation_base.h"
#include <vector>
#include <string>

// mCW_R — Remove one redundant whitespace token (space/tab run).
// Produces one mutant per WHITESPACE token that is not the sole separator
// between two code tokens that REQUIRE whitespace (e.g., keyword + identifier).
class mCW_R : public MutationBase {
public:
    std::string name()        const override { return "mCW_R"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Remove a redundant whitespace token.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        // Helper: find the previous non-whitespace / non-newline token index
        auto prevCode = [&](std::size_t i) -> int {
            for (int j = (int)i - 1; j >= 0; --j) {
                auto t = tokens[j].type;
                if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE)
                    return j;
            }
            return -1;
        };
        auto nextCode = [&](std::size_t i) -> int {
            for (std::size_t j = i + 1; j < tokens.size(); ++j) {
                auto t = tokens[j].type;
                if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE &&
                    t != Token::Type::END_OF_FILE)
                    return (int)j;
            }
            return -1;
        };

        // Two code-token types that REQUIRE a space separator when adjacent
        auto needsSep = [](Token::Type a, Token::Type b) -> bool {
            using T = Token::Type;
            // keyword/identifier/number next to keyword/identifier/number needs sep
            auto isWord = [](T t) {
                return t == T::KEYWORD || t == T::IDENTIFIER || t == T::NUMBER;
            };
            return isWord(a) && isWord(b);
        };

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type != Token::Type::WHITESPACE) continue;

            int prev = prevCode(i);
            int next = nextCode(i);
            if (prev < 0 || next < 0) continue;

            // Don't remove if the space is the mandatory separator
            if (needsSep(tokens[prev].type, tokens[next].type)) continue;

            // Build mutant: skip token i
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == i) continue; // omit this whitespace token
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
