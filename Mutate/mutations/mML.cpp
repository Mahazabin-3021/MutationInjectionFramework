#include "../mutation_base.h"
#include <vector>
#include <string>

// mML — Modify whole line: wrap a mutable statement in if(X==Y){ ... }
// Applicable to mutable lines ending with ';'.
class mML : public MutationBase {
public:
    std::string name()        const override { return "mML"; }
    std::string cloneType()   const override { return "Type-3"; }
    std::string description() const override {
        return "Wrap a mutable statement line in if(X==Y){ ... }";
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

        // Find last code token of a logical line
        auto lastTokVal = [&](std::size_t l) -> std::string {
            auto [lb, le] = lines[l];
            for (int i = (int)le - 1; i >= (int)lb; --i) {
                auto t = tokens[i].type;
                if (t == Token::Type::WHITESPACE || t == Token::Type::NEWLINE ||
                    t == Token::Type::END_OF_FILE) continue;
                return tokens[i].value;
            }
            return "";
        };

        auto indent = [&](std::size_t l) -> std::string {
            auto [lb, le] = lines[l];
            if (lb < tokens.size() && tokens[lb].type == Token::Type::WHITESPACE)
                return tokens[lb].value;
            return "";
        };

        // Mutable region: lines[2] .. lines[lines.size()-3]
        for (std::size_t l = 2; l + 2 < lines.size(); ++l) {
            if (lastTokVal(l) != ";") continue;

            auto [lb, le] = lines[l];
            std::string ind = indent(l);

            // Reconstruct the original statement line (without leading indent and newline)
            std::string stmtLine;
            bool skipIndent = true;
            for (std::size_t j = lb; j < le; ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (skipIndent && tokens[j].type == Token::Type::WHITESPACE) {
                    skipIndent = false; continue;
                }
                skipIndent = false;
                if (tokens[j].type == Token::Type::NEWLINE) break;
                stmtLine += tokens[j].value;
            }

            // Wrap: if(X==Y){ <statement> }
            std::string wrapped = ind + "if(X==Y){ " + stmtLine + " }\n";

            // Build mutant: replace line l with wrapped
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == lb) { mutant += wrapped; }
                if (j >= lb && j < le) continue;
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
