#include "../mutation_base.h"
#include <vector>
#include <string>
#include <cstdlib>
#include <cstdint>

// mRL_N — Rename/change a numeric literal value.
// For each NUMBER token we produce one mutant where that value is changed:
//   - Integer  n  →  n+1
//   - Float    f  →  f  with a trailing digit bumped
// One mutant per NUMBER token.
class mRL_N : public MutationBase {
public:
    std::string name()        const override { return "mRL_N"; }
    std::string cloneType()   const override { return "Type-2"; }
    std::string description() const override {
        return "Change the value of a numeric literal (e.g., 5 → 6).";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        std::vector<std::string> results;

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type != Token::Type::NUMBER) continue;

            const std::string& orig = tokens[i].value;
            std::string changed = perturbNumber(orig);
            if (changed == orig) continue; // couldn't perturb

            std::string mutant;
            for (std::size_t j = 0; j < tokens.size(); ++j) {
                if (tokens[j].type == Token::Type::END_OF_FILE) break;
                mutant += (j == i) ? changed : tokens[j].value;
            }
            results.push_back(std::move(mutant));
        }
        return results;
    }

private:
    static std::string perturbNumber(const std::string& s) {
        // Strip any suffix (f, L, u, U, l)
        std::size_t suffixStart = s.size();
        while (suffixStart > 0) {
            char c = s[suffixStart - 1];
            if (c == 'f' || c == 'F' || c == 'l' || c == 'L' ||
                c == 'u' || c == 'U') --suffixStart;
            else break;
        }
        std::string core = s.substr(0, suffixStart);
        std::string suffix = s.substr(suffixStart);

        // Hex literal — bump last hex digit
        if (core.size() >= 2 && core[0] == '0' &&
            (core[1] == 'x' || core[1] == 'X')) {
            std::string digits = core.substr(2);
            if (digits.empty()) return s;
            char last = digits.back();
            int v = std::isdigit((unsigned char)last) ? (last - '0')
                  : (std::tolower(last) - 'a' + 10);
            v = (v + 1) % 16;
            digits.back() = (v < 10) ? ('0' + v) : ('a' + v - 10);
            return core.substr(0, 2) + digits + suffix;
        }

        // Check if it looks like a float (contains '.' or 'e'/'E')
        bool isFloat = false;
        for (char ch : core)
            if (ch == '.' || ch == 'e' || ch == 'E') { isFloat = true; break; }

        if (!isFloat) {
            // Pure integer: parse and increment by 1
            // Handle octal (leading '0') vs decimal
            try {
                long long val = std::stoll(core, nullptr, 0);
                return std::to_string(val + 1) + suffix;
            } catch (...) {}
        }

        // Float: bump the last non-'9' digit to avoid wrap-around confusion
        std::string result = core;
        for (int idx = (int)result.size() - 1; idx >= 0; --idx) {
            char& c = result[(std::size_t)idx];
            if (std::isdigit((unsigned char)c) && c != '9') {
                c = c + 1;
                return result + suffix;
            }
        }
        // All digits are '9' — fall back to prepending '1' (e.g. 9.99 → not touched, use fallback)
        return s; // fallback
    }
};
