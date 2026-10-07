#include "../mutation_base.h"
#include <vector>
#include <string>
#include <cctype>

// ---------------------------------------------------------------------------
// mCF_SL — Single-Line Collapse
//
// Collapses the entire function to a single line by:
//   1. Removing ALL NEWLINE tokens.
//   2. Collapsing any run of WHITESPACE tokens adjacent to a removed newline
//      down to a single space (so tokens that had their separator provided by
//      the newline still remain syntactically separated).
//   3. Stripping leading/trailing whitespace from the result.
//
// This produces exactly ONE mutant (the whole-function single-line form).
// It is a Type-1 clone: formatting-only change, semantics unchanged.
//
// Example input:
//   int gcd(int a, int b) {
//       while (b != 0) {
//           int t = b;
//           b = a % b;
//           a = t;
//       }
//       return a;
//   }
//
// Example output:
//   int gcd(int a, int b) { while (b != 0) { int t = b; b = a % b; a = t; } return a; }
// ---------------------------------------------------------------------------
class mCF_SL : public MutationBase {
public:
    std::string name()        const override { return "mCF_SL"; }
    std::string cloneType()   const override { return "Type-1"; }
    std::string description() const override {
        return "Collapse the entire function to a single line by removing all newlines.";
    }

    std::vector<std::string> mutate(const std::vector<Token>& tokens) const override {
        // Guard: if there are no newlines at all, the function is already
        // single-line — this mutation is not applicable.
        bool hasNewline = false;
        for (const auto& t : tokens)
            if (t.type == Token::Type::NEWLINE) { hasNewline = true; break; }
        if (!hasNewline) return {};

        // Build a collapsed token stream:
        //   - Drop all NEWLINE tokens.
        //   - Wherever a NEWLINE was, ensure exactly one space separates its
        //     neighbours (merge with any adjacent WHITESPACE tokens).
        //
        // Strategy: emit tokens in order, but when we encounter a NEWLINE
        //   we set a flag "pendingSpace". Any WHITESPACE tokens that
        //   immediately precede or follow the NEWLINE are also absorbed.
        //   When the next real code token arrives, we emit a single " " if
        //   pendingSpace is set AND the last emitted character is not already
        //   a space.

        std::string result;
        result.reserve(256);

        bool pendingSpace = false;

        // Helper: is the last char of `result` a space?
        auto endsWithSpace = [&]() {
            return !result.empty() && result.back() == ' ';
        };

        for (std::size_t i = 0; i < tokens.size(); ++i) {
            const Token& tok = tokens[i];

            if (tok.type == Token::Type::END_OF_FILE) break;

            if (tok.type == Token::Type::NEWLINE) {
                // Mark that a space may be needed here
                pendingSpace = true;
                continue;
            }

            if (tok.type == Token::Type::WHITESPACE) {
                if (pendingSpace) {
                    // Absorb: the whitespace was indentation after a newline
                    // — we'll emit our own single space later.
                    continue;
                }
                // Normal whitespace (within a line) — preserve as-is,
                // but collapse multiple spaces to one.
                if (!endsWithSpace()) result += ' ';
                continue;
            }

            // Skip line comments: they run to EOL, so including them on a
            // single line would comment out everything that follows.
            if (tok.type == Token::Type::COMMENT_LINE ||
                tok.type == Token::Type::COMMENT_DOC) {
                // Replace the comment with a space separator
                pendingSpace = true;
                continue;
            }

            // Real code token — emit pending space if needed
            if (pendingSpace && !endsWithSpace() && !result.empty()) {
                result += ' ';
            }
            pendingSpace = false;

            result += tok.value;
        }

        // Strip trailing whitespace
        while (!result.empty() && result.back() == ' ')
            result.pop_back();

        if (result.empty()) return {};

        return { result };
    }
};
