#include "../mutation_base.h"
#include <vector>
#include <string>

// mCF_R — Remove one newline token.
// One mutant per NEWLINE token that is not the first or last token overall.
class mCF_R : public MutationBase {
public:
    std::string name()        const override { return "mCF_R"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Remove a single newline token.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type != Token::Type::NEWLINE) continue;

            // Build mutant: skip this NEWLINE
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == i) continue;
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
