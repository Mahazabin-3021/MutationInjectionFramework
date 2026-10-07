#pragma once
#include "tokenizer.h"
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Abstract base class for all mutators.
//
// Every mutation is a stateless functor:
//   mutate(tokens) → list of mutated function strings
//
// One entry per independent mutation site; an empty vector means the input
// is not applicable for this mutation.
// ---------------------------------------------------------------------------
class MutationBase {
public:
    virtual ~MutationBase() = default;

    virtual std::string name()        const = 0;  // e.g. "mCW_A"
    virtual std::string cloneType()   const = 0;  // "Type-1", "Type-2", "Type-3"
    virtual std::string description() const = 0;

    // Returns one mutant string per applicable site.
    virtual std::vector<std::string> mutate(
        const std::vector<Token>& tokens) const = 0;
};
