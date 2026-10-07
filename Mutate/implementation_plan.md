# Mutation Injection Framework — Implementation Plan

Takes a **function string** in **C, C++, Java, or C#**, tokenizes it, and applies 15 defined mutations to produce mutated variants.

---

## Project Structure

```
Mutation-Injection/
│
├── tokenizer.h                   # Token types + Tokenizer class declaration
├── tokenizer.cpp                 # Language-aware tokenizer (C / C++ / Java / C#)
│
├── mutation_base.h               # Abstract base class MutationBase
│
├── mutations/
│   │
│   │── [Type-1: Whitespace / Comment / Formatting]
│   ├── mCW_A.cpp                 # Addition of whitespace
│   ├── mCW_R.cpp                 # Removal of whitespace
│   ├── mCC_BT.cpp                # Insert /**/ comment between tokens
│   ├── mCC_EOL.cpp               # Append // comment at end of random line
│   ├── mCF_A.cpp                 # Add a newline between tokens
│   ├── mCF_R.cpp                 # Remove a newline
│   ├── mCF_SL.cpp                # ★ NEW: Collapse entire function to one line
│   │
│   │── [Type-2: Identifier / Literal]
│   ├── mSRI.cpp                  # Systematic rename: rename identifier + ALL occurrences
│   ├── mARI.cpp                  # Arbitrary rename: rename ONE instance of an identifier
│   ├── mRL_N.cpp                 # Change value of a numeric literal
│   ├── mRL_S.cpp                 # Change value of a string or character literal
│   │
│   │── [Type-3: Structural]
│   ├── mSIL.cpp                  # Small insertion within a line (add param to call/signature)
│   ├── mSDL.cpp                  # Small deletion within a line (remove param)
│   ├── mIL.cpp                   # Insert a new statement line at a random position
│   ├── mDL.cpp                   # Delete a randomly selected statement line
│   └── mML.cpp                   # Modify whole line: wrap statement in if(X==Y)
│
├── main.cpp                      # Driver: parse language, tokenize, apply mutators, output
├── Makefile
└── tests/
    └── test_runner.cpp           # Round-trip + per-mutator smoke tests
```

---

## The 15 Mutators

### Type-1 — Whitespace, Comments, Formatting
These mutate only non-semantic tokens (spaces, tabs, newlines, comments). They produce **Type-1 clones** — identical in logic, different in layout.

| File | Mutator | What it does |
|---|---|---|
| `mCW_A.cpp` | **mCW_A** | Insert a tab/space between two randomly chosen tokens |
| `mCW_R.cpp` | **mCW_R** | Remove one syntactically redundant tab/space token |
| `mCC_BT.cpp` | **mCC_BT** | Insert a `/* */` comment between two randomly chosen tokens |
| `mCC_EOL.cpp` | **mCC_EOL** | Append a `//` comment at the end of a randomly chosen line |
| `mCF_A.cpp` | **mCF_A** | Insert a newline between two randomly chosen tokens |
| `mCF_R.cpp` | **mCF_R** | Remove a randomly chosen newline |
| `mCF_SL.cpp` | **mCF_SL** ★ | Collapse the **entire function to a single line** by removing all newlines (and stripping line comments that would swallow subsequent code) |

---

### Type-2 — Identifier & Literal Renaming
These change names or values while keeping structure intact. They produce **Type-2 clones**.

| File | Mutator | What it does |
|---|---|---|
| `mSRI.cpp` | **mSRI** | Pick a random identifier; rename it **and every other occurrence** to a new name |
| `mARI.cpp` | **mARI** | Pick one random **single instance** of an identifier and rename only that occurrence |
| `mRL_N.cpp` | **mRL_N** | Pick a random numeric literal and change its value (e.g., `5` → `6`) |
| `mRL_S.cpp` | **mRL_S** | Pick a random string or char literal and change its value (e.g., `"hello"` → `"world"`) |

---

### Type-3 — Structural Mutations
These change the code structure — adding/removing lines or parameters. They produce **Type-3 clones**.

| File | Mutator | What it does |
|---|---|---|
| `mSIL.cpp` | **mSIL** | Add a parameter to a randomly chosen function call or signature |
| `mSDL.cpp` | **mSDL** | Delete a parameter from a randomly chosen function call or signature |
| `mIL.cpp` | **mIL** | Insert `int _some_horrible_name = 0;` at a random **safe** position |
| `mDL.cpp` | **mDL** | Delete a randomly selected **safe** statement line |
| `mML.cpp` | **mML** | Wrap a randomly selected **safe** statement in `if(X==Y)` guard |

#### ⚠️ Safe-Zone Rules for Type-3 Mutators

All five Type-3 mutators share a concept of **protected lines** that must never be touched:

**Protected at the TOP (first 2 logical lines):**
- Line 1 — the **function signature / header** (e.g. `int gcd(int a, int b) {`)
- Line 2 — the **first statement** inside the function body

**Protected at the BOTTOM (last 2 logical lines):**
- Second-to-last — the **`return` statement** (if present), otherwise the last statement
- Last — the closing `}` of the function

```
int gcd(int a, int b) {      ← PROTECTED (top-1: header)
    while (b != 0) {          ← PROTECTED (top-2: first statement)
        int t = b;            ← MUTABLE
        b = a % b;            ← MUTABLE
        a = t;                ← MUTABLE
    }                         ← MUTABLE
    return a;                 ← PROTECTED (bottom-2: return)
}                             ← PROTECTED (bottom-1: closing brace)
```

**Rules per mutator:**

| Mutator | Insert allowed in | Delete/Wrap allowed on |
|---|---|---|
| `mIL` | Any line **between** top-2 and bottom-2 (exclusive) | — |
| `mDL` | — | Any **mutable** line that is a complete single statement (ends with `;` or is a `}` closing a block other than the function's own) |
| `mML` | — | Any **mutable** single-statement line ending with `;` |
| `mSIL` | — | Any function call/signature in mutable region |
| `mSDL` | — | Any function call/signature in mutable region that has ≥ 2 parameters |

**Inserted line for `mIL`:** always exactly `int _some_horrible_name = 0;` (language-adapted: Java/C# use the same syntax since `int` is valid in both).

---

## Tokenizer Design — 4-Language Support

The tokenizer is **language-aware**. The caller specifies the source language:

```cpp
enum class Language { C, CPP, JAVA, CSHARP };

class Tokenizer {
public:
    Tokenizer(const std::string& source, Language lang);
    std::vector<Token> tokenize();
private:
    Language lang_;
    // helpers: scanIdentifier, scanNumber, scanString,
    //          scanOperator, scanComment, scanWhitespace
};
```

### Language differences handled:
| Feature | C | C++ | Java | C# |
|---|---|---|---|---|
| Single-line comment | `//` | `//` | `//` | `//` |
| Block comment | `/* */` | `/* */` | `/* */` | `/* */` |
| Doc comment | — | — | `/** */` | `///` |
| String literal | `"..."` | `"..."` | `"..."` | `"..."`, `@"..."` verbatim |
| Char literal | `'a'` | `'a'` | `'a'` | `'a'` |
| Extra keywords | — | `nullptr`, `auto`, `class` | `null`, `import`, `extends` | `null`, `var`, `using`, `namespace` |

The token **types** are the same across all languages:

```cpp
struct Token {
    enum class Type {
        KEYWORD, IDENTIFIER, NUMBER,
        STRING_LIT, CHAR_LIT,
        OPERATOR, DELIMITER,
        COMMENT_BLOCK, COMMENT_LINE, COMMENT_DOC,
        WHITESPACE, NEWLINE,
        END_OF_FILE
    };
    Type        type;
    std::string value;
    int         line, col;   // original position (for diagnostics)
};

// Lossless reconstruction: join all token values in order
std::string reconstruct(const std::vector<Token>& tokens);
```

> The tokenizer is **lossless** — WHITESPACE and NEWLINE tokens are kept, so `reconstruct(tokenize(src)) == src` exactly.

---

## Mutation Base Interface

```cpp
class MutationBase {
public:
    virtual ~MutationBase() = default;
    virtual std::string name()        const = 0;  // e.g. "mCW_A"
    virtual std::string cloneType()   const = 0;  // "Type-1", "Type-2", "Type-3"
    virtual std::string description() const = 0;

    // Returns one mutant per applicable mutation site.
    // tokens = output of Tokenizer::tokenize()
    virtual std::vector<std::string> mutate(
        const std::vector<Token>& tokens) const = 0;
};
```

---

## Driver (`main.cpp`)

**CLI usage:**
```bash
./mutate --lang cpp --input func.cpp
./mutate --lang java --input MyClass.java
./mutate --lang csharp --input Program.cs
./mutate --lang c --input foo.c
```

**Output per mutant:**
```
=== Mutant [mCW_A-1] (Type-1) ===
<mutated function text>
=================================
```

Optional flag `--output-dir ./mutants/` writes each mutant to its own file.

---

## Build System (`Makefile`)

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Imutations

MUTATION_SRCS = mutations/mCW_A.cpp mutations/mCW_R.cpp \
                mutations/mCC_BT.cpp mutations/mCC_EOL.cpp \
                mutations/mCF_A.cpp mutations/mCF_R.cpp \
                mutations/mCF_SL.cpp \
                mutations/mSRI.cpp mutations/mARI.cpp \
                mutations/mRL_N.cpp mutations/mRL_S.cpp \
                mutations/mSIL.cpp mutations/mSDL.cpp \
                mutations/mIL.cpp  mutations/mDL.cpp \
                mutations/mML.cpp

SRCS = tokenizer.cpp main.cpp $(MUTATION_SRCS)
TARGET = mutate

all: $(TARGET)
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^
clean:
	rm -f $(TARGET)
```

---

## Execution Order (Implementation Steps)

1. `tokenizer.h` — Token struct + Language enum + Tokenizer declaration
2. `tokenizer.cpp` — Full tokenizer implementation (all 4 languages)
3. `mutation_base.h` — Abstract interface
4. **Type-1** mutations: `mCW_A`, `mCW_R`, `mCC_BT`, `mCC_EOL`, `mCF_A`, `mCF_R`, `mCF_SL` ★
5. **Type-2** mutations: `mSRI`, `mARI`, `mRL_N`, `mRL_S`
6. **Type-3** mutations: `mSIL`, `mSDL`, `mIL`, `mDL`, `mML`
7. `main.cpp` — Driver with CLI
8. `Makefile`
9. `tests/test_runner.cpp` — Smoke tests + round-trip test

---

## Verification Plan

### Round-trip test
```cpp
assert(reconstruct(tokenizer.tokenize()) == original_source);
```

### Mutator smoke test
For each of the 15 mutators — given a non-trivial input function — assert `mutants.size() >= 1`.

### Manual spot-check input
```cpp
int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}
```
Expected: all 15 mutators produce at least 1 valid mutant each.
