#include "../mutation_base.h"
#include <vector>
#include <string>

// mDL — Delete a randomly selected statement line.
// Safe zone: skip top-2 and bottom-2 logical lines.
// Deletable: lines in mutable region that end with ';' or are bare '}'.
class mDL : public MutationBase {
public:
    std::string name()        const override { return "mDL"; }
    std::string cloneType()   const override { return "Type-3"; }
    std::string description() const override {
        return "Delete a randomly selected mutable statement line.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        // Build logical lines
        std::vector<std::pair<std::size_t,std::size_t>> lines;
        std::size_t start = 0;
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type == Token::Type::NEWLINE ||
                tokens[i].type == Token::Type::END_OF_FILE) {
                lines.push_back({ start, i + 1 });
                start = i + 1;
            }
        }
        if (start < tokens.size()) lines.push_back({ start, tokens.size() });

        if (lines.size() < 5) return {};

        // Check if a logical line ends with ';' or is a bare '}'
        auto isDeletable = [&](std::size_t l) -> bool {
            auto [lb, le] = lines[l];
            std::string lastTok;
            for (int i = (int)le - 1; i >= (int)lb; --i) {
                auto t = tokens[i].type;
                if (t == Token::Type::WHITESPACE || t == Token::Type::NEWLINE ||
                    t == Token::Type::END_OF_FILE) continue;
                lastTok = tokens[i].value;
                break;
            }
            // Count code tokens
            int codeCount = 0;
            for (std::size_t i = lb; i < le; ++i) {
                auto t = tokens[i].type;
                if (t != Token::Type::WHITESPACE && t != Token::Type::NEWLINE &&
                    t != Token::Type::END_OF_FILE) ++codeCount;
            }
            if (codeCount == 0) return false;
            return lastTok == ";" || lastTok == "}";
        };

        // Mutable region: lines[2] .. lines[lines.size()-3]
        for (std::size_t l = 2; l + 2 < lines.size(); ++l) {
            if (!isDeletable(l)) continue;

            auto [lb, le] = lines[l];
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j >= lb && j < le) continue; // skip this line
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
