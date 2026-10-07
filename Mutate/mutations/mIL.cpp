#include "../mutation_base.h"
#include <vector>
#include <string>

// mIL — Insert a new statement line
// Insert "    int _some_horrible_name = 0;" at each safe position
// (between top-2 and bottom-2 logical lines, exclusive).
class mIL : public MutationBase {
public:
    std::string name()        const override { return "mIL"; }
    std::string cloneType()   const override { return "Type-3"; }
    std::string description() const override {
        return "Insert 'int _some_horrible_name = 0;' at a random safe position.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;
        const std::string inserted = "    int _some_horrible_name = 0;\n";

        // Split into logical lines
        std::vector<std::pair<std::size_t,std::size_t>> lines; // (begin, end) in token indices
        std::size_t start = 0;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type == Token::Type::NEWLINE ||
                tokens[i].type == Token::Type::END_OF_FILE) {
                lines.push_back({ start, i + 1 });
                start = i + 1;
            }
        }
        if (start < tokens.size()) lines.push_back({ start, tokens.size() });

        if (lines.size() < 5) return {}; // not enough lines for safe zone

        // Safe insertion points: after line[1] through line[lines.size()-3] (inclusive)
        // i.e., between top-2 and bottom-2
        for (std::size_t l = 1; l + 2 < lines.size(); ++l) {
            // Insert after line l (i.e., before line l+1)
            std::size_t insertBefore = lines[l + 1].first; // token index

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == insertBefore) mutant += inserted;
                mutant += tokens[j].value;
            }
            // Edge: insertBefore == tokens.size() means append
            if (insertBefore >= tokens.size()) mutant += inserted;
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
