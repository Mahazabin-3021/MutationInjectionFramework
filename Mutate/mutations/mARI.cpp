#include "../mutation_base.h"
#include <vector>
#include <string>

// mARI — Arbitrary Rename Identifier
// Rename exactly ONE occurrence (by token index) of each identifier.
// One mutant per IDENTIFIER token.
class mARI : public MutationBase {
public:
    std::string name()        const override { return "mARI"; }
    std::string cloneType()   const override { return "Type-2"; }
    std::string description() const override {
        return "Rename a single occurrence of an identifier (arbitrary rename).";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type != Token::Type::IDENTIFIER) continue;

            const std::string newName = "_mut_" + tokens[i].value + "_arb";
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == i)
                    mutant += newName;
                else
                    mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
