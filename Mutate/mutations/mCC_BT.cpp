#include "../mutation_base.h"
#include <vector>
#include <string>

// mCC_BT — Insert a /* mutated */ comment between two randomly chosen tokens.
// One mutant per valid inter-token gap.
class mCC_BT : public MutationBase {
public:
    std::string name()        const override { return "mCC_BT"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Insert a /* mutated */ block comment between two tokens.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;
        const std::string comment = "/* mutated */";

        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].type  == Token::Type::END_OF_FILE) break;
            if (tokens[i+1].type == Token::Type::END_OF_FILE) break;

            // Insert only between code tokens (not inside whitespace runs)
            if (tokens[i].type   == Token::Type::WHITESPACE) continue;
            if (tokens[i].type   == Token::Type::NEWLINE)    continue;
            if (tokens[i+1].type == Token::Type::NEWLINE)    continue;
            if (tokens[i+1].type == Token::Type::WHITESPACE) continue;

            // Comments inside strings/chars would be wrong — skip
            if (tokens[i].type   == Token::Type::STRING_LIT) continue;
            if (tokens[i].type   == Token::Type::CHAR_LIT)   continue;

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                mutant += tokens[j].value;
                if (j == i) mutant += comment;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }
};
