#include "../mutation_base.h"
#include <vector>
#include <string>

// mCC_EOL — Append a "// comment" at end of each non-empty, non-comment line.
// One mutant per applicable line.
class mCC_EOL : public MutationBase {
public:
    std::string name()        const override { return "mCC_EOL"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Append a // end-of-line comment after a random line.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;
        const std::string eol_comment = " // mutated";

        // Find indices of NEWLINE tokens; we inject the comment before each
        // newline that follows at least one code token on the same logical line.
        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type != Token::Type::NEWLINE) continue;

            // Scan backwards to see if there is any code token before this newline
            // on the same line (no earlier newline in between)
            bool hasCode = false;
            for (int j = (int)i - 1; j >= 0; --j) {
                auto t = tokens[j].type;
                if (t == Token::Type::NEWLINE) break;
                if (t != Token::Type::WHITESPACE &&
                    t != Token::Type::COMMENT_LINE &&
                    t != Token::Type::COMMENT_DOC &&
                    t != Token::Type::END_OF_FILE) {
                    hasCode = true; break;
                }
            }
            if (!hasCode) continue;

            // Also skip if the line already ends with a // comment
            bool alreadyComment = false;
            for (int j = (int)i - 1; j >= 0; --j) {
                auto t = tokens[j].type;
                if (t == Token::Type::NEWLINE) break;
                if (t == Token::Type::COMMENT_LINE || t == Token::Type::COMMENT_DOC) {
                    alreadyComment = true; break;
                }
            }
            if (alreadyComment) continue;

            // Build mutant: insert comment before the NEWLINE token at i
            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                if (j == i) mutant += eol_comment;
                mutant += tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
