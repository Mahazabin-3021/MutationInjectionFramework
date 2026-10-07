#include "../mutation_base.h"
#include <vector>
#include <string>

// mSDL — Small Deletion within a Line
// Remove a parameter from a function call or signature that has >= 2 params.
// One mutant per removable parameter per call site.
class mSDL : public MutationBase {
public:
    std::string name()        const override { return "mSDL"; }
    std::string cloneType()   const override { return "Type-3"; }
    std::string description() const override {
        return "Remove a parameter from a function call or signature (>= 2 params).";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i].value != "(" ||
                tokens[i].type  != Token::Type::DELIMITER) continue;

            // Previous non-ws must be identifier/keyword
            int prev = -1;
            for (int j = (int)i - 1; j >= 0; --j) {
                if (tokens[j].type == Token::Type::WHITESPACE) continue;
                prev = j; break;
            }
            if (prev < 0) continue;
            if (tokens[prev].type != Token::Type::IDENTIFIER &&
                tokens[prev].type != Token::Type::KEYWORD) continue;

            // Find matching ')' at depth 1
            int depth = 1;
            std::size_t close = i + 1;
            for (; close < tokens.size() && depth > 0; ++close) {
                if (tokens[close].value == "(") ++depth;
                else if (tokens[close].value == ")") --depth;
            }
            if (depth != 0) continue;
            --close;

            // Collect top-level commas inside (i, close)
            std::vector<std::size_t> commas;
            int d2 = 0;
            for (std::size_t j = i + 1; j < close; ++j) {
                if (tokens[j].value == "(" || tokens[j].value == "[") ++d2;
                else if (tokens[j].value == ")" || tokens[j].value == "]") --d2;
                else if (tokens[j].value == "," && d2 == 0) commas.push_back(j);
            }

            // Need at least one comma (= at least 2 params)
            if (commas.empty()) continue;

            // For each param, produce a mutant removing it.
            // params: [i+1 .. commas[0]-1], [commas[0]+1 .. commas[1]-1], ..., [commas.back()+1 .. close-1]
            std::vector<std::pair<std::size_t,std::size_t>> paramRanges;
            std::size_t pstart = i + 1;
            for (std::size_t ci : commas) {
                paramRanges.push_back({ pstart, ci });
                pstart = ci + 1;
            }
            paramRanges.push_back({ pstart, close });

            for (std::size_t p = 0; p < paramRanges.size(); ++p) {
                auto [pb, pe] = paramRanges[p];
                // Tokens to skip: the param range [pb, pe) plus the adjacent comma
                std::size_t skipFrom, skipTo;
                if (p + 1 < paramRanges.size()) {
                    // remove param + following comma
                    skipFrom = pb; skipTo = commas[p] + 1;
                } else {
                    // last param: remove preceding comma + this param
                    skipFrom = commas[p - (commas.size() - 1 == p ? 1 : 0)];
                    // simpler: remove preceding comma
                    skipFrom = commas.back() + 1;
                    skipTo   = close;
                    // remove the preceding comma too:
                    std::string mutant2;
                    std::size_t skipComma = commas.back();
                    for (std::size_t j = 0; j < tokens.size(); ++j) {
                        if (tokens[j].type == Token::Type::END_OF_FILE) break;
                        if (j >= skipFrom && j < skipTo) continue;
                        if (j == skipComma) continue;
                        mutant2 += tokens[j].value;
                    }
                    results.push_back(std::move(mutant2));
                    continue;
                }

                std::string mutant;
                for (std::size_t j = 0; j < tokens.size(); ++j) {
                    if (tokens[j].type == Token::Type::END_OF_FILE) break;
                    if (j >= skipFrom && j < skipTo) continue;
                    mutant += tokens[j].value;
                }
                results.push_back(std::move(mutant));
            }
        }
        return results;
    }
};
