#include "../mutation_base.h"
#include <vector>
#include <string>

// mCF_A — Insert a newline between two randomly chosen tokens.
// One mutant per valid inter-token gap (not already adjacent to a newline).
class mCF_A : public MutationBase {
public:
    std::string name()        const override { return "mCF_A"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Insert an extra newline between two tokens.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].type   == Token::Type::END_OF_FILE) break;
            if (tokens[i+1].type == Token::Type::END_OF_FILE) break;

            // Skip positions already adjacent to a newline
            if (tokens[i].type   == Token::Type::NEWLINE) continue;
            if (tokens[i+1].type == Token::Type::NEWLINE) continue;
            // Skip inside strings/comments
            if (tokens[i].type   == Token::Type::STRING_LIT)    continue;
            if (tokens[i].type   == Token::Type::COMMENT_BLOCK) continue;

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                mutant += tokens[j].value;
                if (j == i) mutant += "\n";
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
