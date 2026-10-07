#include "../mutation_base.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

// mSRI — Systematic Rename Identifier
// Pick each distinct identifier; rename it AND every occurrence to a new name.
// One mutant per distinct identifier.
class mSRI : public MutationBase {
public:
    std::string name()        const override { return "mSRI"; }
    std::string cloneType()   const override { return "Type-2"; }
    std::string description() const override {
        return "Rename an identifier and all its occurrences systematically.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        // Collect all distinct identifier names
        std::unordered_set<std::string> seen;
        for (const auto& t : tokens)
            if (t.type == Token::Type::IDENTIFIER) seen.insert(t.value);

        for (const auto& oldName : seen) {
            std::string newName = "_mut_" + oldName + "_renamed";
            std::string mutant;
            for (const auto& t : tokens) {
                if (t.type == Token::Type::END_OF_FILE) break;
                if (t.type == Token::Type::IDENTIFIER && t.value == oldName)
                    mutant += newName;
                else
                    mutant += t.value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
