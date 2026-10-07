#include "../mutation_base.h"
#include <vector>
#include <string>

// mCW_A — Insert one extra space between two randomly chosen tokens.
// Produces one mutant per valid inter-token gap (between non-newline,
// non-whitespace-adjacent tokens).
class mCW_A : public MutationBase {
public:
    std::string name()        const override { return "mCW_A"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Insert an extra space between two tokens.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            const Token& cur  = tokens[i];
            const Token& next = tokens[i + 1];

            // Skip trivial / EOF positions
            if (cur.type  == Token::Type::END_OF_FILE) break;
            if (next.type == Token::Type::END_OF_FILE) break;

            // Only insert between code tokens (not between two whitespace tokens,
            // not after/before newlines where whitespace already leads the line)
            if (cur.type  == Token::Type::WHITESPACE) continue;
            if (cur.type  == Token::Type::NEWLINE)    continue;
            if (next.type == Token::Type::NEWLINE)    continue;
            if (next.type == Token::Type::WHITESPACE) continue;

            // Build mutant: insert " " after token i
            std::string mutant;
            mutant.reserve(128);
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                mutant += tokens[j].value;
                if (j == i) mutant += " ";
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
