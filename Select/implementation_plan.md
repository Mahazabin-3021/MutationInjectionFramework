# Select Module — Implementation Plan

Randomly picks **N complete functions** (user-specified count) from a large
multi-file codebase and writes each one to a separate output file, ready to
be fed into the **Mutate** pipeline.

Uses **tree-sitter** (C API) for accurate, language-aware function extraction.
**`main` functions are always excluded.**

---

## Goal

```
Select/
└── select          ← compiled C++17 binary

Input : a root directory of source files  +  count N
Output: N files: func001.txt, func002.txt, ..., func<N>.txt
        each containing exactly one distinct function
```

CLI usage:
```bash
./select --root /path/to/codebase --lang cpp --count 50 --output-dir ./selected/
./select --root /path/to/codebase --lang java --count 100 --output-dir ./selected/ [--seed 42]
```

---

## Implementation Language

**C++17** — tree-sitter C API (`tree_sitter/api.h`).
Grammar shared libraries: `libtree-sitter-c`, `libtree-sitter-cpp`,
`libtree-sitter-java`, `libtree-sitter-c-sharp`.

---

## Project Structure

```
Select/
│
├── implementation_plan.md        ← this file
│
├── Makefile
│
├── select.cpp                    ← main entry point (CLI)
│
├── lang_registry.h / .cpp        ← lang string → TSLanguage* + node types + extensions
│
├── file_walker.h / .cpp          ← recursive file enumerator (lazy iterator)
│
├── function_extractor.h / .cpp   ← tree-sitter parse + function node extraction
│                                    + main() filter
│
├── selector.h / .cpp             ← reservoir sampling for k-of-N selection
│
└── tests/
    ├── fixtures/                 ← tiny .c / .cpp / .java / .cs test files
    └── test_select.cpp           ← unit + integration tests
```

---

## Dependencies

| Library | Purpose |
|---------|---------|
| `libtree-sitter` | Core parser engine |
| `libtree-sitter-c` | C grammar |
| `libtree-sitter-cpp` | C++ grammar |
| `libtree-sitter-java` | Java grammar |
| `libtree-sitter-c-sharp` | C# grammar |

Install:
```bash
sudo apt install libtree-sitter-dev
# + grammar packages or build from source (see Makefile notes)
```

---

## Key Changes from Single-Function Design

| Aspect | Old (k=1) | New (k=N) |
|--------|-----------|-----------|
| Selection count | Always 1 | User-specified `--count N` |
| Algorithm | Algorithm R (k=1) | Algorithm R (k=N) — proper k-item reservoir |
| `main` filter | Not specified | Always excluded |
| Output | One file | N files: `func001.txt` … `func<N>.txt` |
| Duplicate guarantee | N/A | All N selected functions are **distinct** |

---

## Step-by-Step Implementation Plan

### Step 1 — `lang_registry.h / .cpp`

Same as before — maps lang string to grammar + node types + extensions.

```cpp
struct LangConfig {
    const TSLanguage*        grammar;
    std::vector<std::string> functionNodeTypes;
    std::vector<std::string> extensions;
};

LangConfig getLangConfig(const std::string& lang);
```

Node type table:

| Language | Grammar call | Function node types |
|----------|-------------|---------------------|
| C        | `tree_sitter_c()` | `"function_definition"` |
| C++      | `tree_sitter_cpp()` | `"function_definition"` |
| Java     | `tree_sitter_java()` | `"method_declaration"`, `"constructor_declaration"` |
| C#       | `tree_sitter_c_sharp()` | `"method_declaration"`, `"constructor_declaration"` |

---

### Step 2 — `file_walker.h / .cpp`

No change from single-function design.

```cpp
class FileWalker {
public:
    FileWalker(const std::filesystem::path& root,
               const std::vector<std::string>& extensions);
    std::filesystem::path next();
    bool done() const;
private:
    // uses std::filesystem::recursive_directory_iterator
    // prunes: .git .svn .hg node_modules __pycache__ build out dist bin obj
};
```

---

### Step 3 — `function_extractor.h / .cpp`

```cpp
struct FunctionRecord {
    int                      startLine;
    int                      endLine;
    int                      numLines;
    std::string              text;        // raw source of the function
    std::filesystem::path    sourceFile;  // which file it came from
    std::string              funcName;    // extracted function/method name
};

std::vector<FunctionRecord> extractFunctions(
    const std::filesystem::path& filePath,
    TSParser*                    parser,
    const std::vector<std::string>& functionNodeTypes,
    int minLines = 3,
    int minChars = 50
);
```

#### Algorithm per file:

1. Read file bytes into `std::string`.
2. `ts_parser_parse_string(...)` → `TSTree*`. On null → log warning, return `{}`.
3. DFS through syntax tree (stack-based, non-recursive to avoid stack overflow
   on very deep trees).
4. For each node matching a `functionNodeType`:
   - Extract `text = src.substr(start_byte, end_byte - start_byte)`.
   - Extract `startLine = ts_node_start_point(node).row + 1` and `endLine = ts_node_end_point(node).row + 1`.
   - Calculate `numLines = endLine - startLine + 1`.
   - **Extract function name** by finding the `declarator` / `identifier` child node
     (exact child type depends on language):

     | Language | Name node child type |
     |----------|---------------------|
     | C/C++ | child named `"declarator"` → descend to `"identifier"` |
     | Java | child named `"name"` (type `"identifier"`) |
     | C# | child named `"name"` (type `"identifier"`) |

   - **`main` filter**: if `funcName == "main"` (C/C++) or `funcName == "Main"` (C#)
     → **skip entirely**.
   - Apply size filter: skip if `numLines < minLines` or `< minChars` chars.
   - Append to results. **Do not recurse into matched node** (avoids double-counting
     nested lambdas/closures).
5. `ts_tree_delete(tree)`.

---

### Step 4 — `selector.h / .cpp`

**Algorithm R for k-of-N reservoir sampling** (Vitter 1985).

```cpp
struct SelectResult {
    std::vector<FunctionRecord> chosen;          // exactly k records
    long long                   totalFunctions;  // total qualifying found
    long long                   totalFiles;      // total files scanned
};

std::optional<SelectResult> reservoirSample(
    FileWalker&                      walker,
    TSParser*                        parser,
    const std::vector<std::string>&  functionNodeTypes,
    int                              k,          // --count N
    int                              minLines,
    int                              minChars,
    uint64_t                         seed
);
```

#### Algorithm R (generalised to k items):

```cpp
std::mt19937_64 rng(seed);

std::vector<FunctionRecord> reservoir;  // size grows up to k, then stays k
reservoir.reserve(k);
long long count = 0;   // total functions seen so far
long long files = 0;

while (!walker.done()) {
    auto file = walker.next();
    ++files;
    auto funcs = extractFunctions(file, parser, nodeTypes, minLines, minChars);

    for (auto& f : funcs) {
        ++count;
        if ((long long)reservoir.size() < k) {
            // Phase 1: fill reservoir until we have k items
            reservoir.push_back(f);
        } else {
            // Phase 2: replace a random existing item with probability k/count
            std::uniform_int_distribution<long long> dist(1, count);
            long long j = dist(rng);
            if (j <= k) {
                reservoir[j - 1] = f;   // replace slot j (1-indexed)
            }
        }
    }
}

if (reservoir.empty()) return std::nullopt;

// If fewer functions exist than requested, return what we have
// (reservoir.size() <= k, may be < k if codebase was small)
return SelectResult{ std::move(reservoir), count, files };
```

**Why this guarantees uniform sampling:**
At the end of the scan, each of the N total functions has exactly k/N
probability of being in the reservoir — a mathematically proven property
of Algorithm R.

**If total functions < k:**
Return all of them (with a warning to stderr). Never crash.

---

### Step 5 — `select.cpp` (CLI entry point)

```
Usage:
  ./select --root <dir>
           --lang <c|cpp|java|csharp>
           --count <N>
           --output-dir <dir>
           [--seed <uint64>]
           [--min-lines <int>]    (default 3)
           [--min-chars <int>]    (default 50)
           [--help]
```

#### Flow:

1. Parse CLI args.
2. Validate `--root` is a directory. Validate `--count > 0`.
3. `getLangConfig(lang)` → `LangConfig`.
4. Create `TSParser*`, set language.
5. Create `FileWalker(root, config.extensions)`.
6. Call `reservoirSample(walker, parser, nodeTypes, N, minLines, minChars, seed)`.
7. If `nullopt` → stderr error, return 1.
8. If `chosen.size() < N` → print warning: "Warning: only M qualifying functions
   found (requested N). Writing M files."
9. `std::filesystem::create_directories(outputDir)`.
10. For each `FunctionRecord` at index `i` (0-based):
    - Filename: `func<ZEROPADDED_INDEX>.txt`
      e.g. for N=50: `func001.txt` … `func050.txt`
      for N=500: `func001.txt` … `func500.txt`
      (padding width = number of digits in N)
    - File contents:
      ```
      // SOURCE: <path relative to root>
      // LINES:  <start_line>-<end_line>
      // LENGTH: <number_of_lines> lines
      // LANG:   cpp
      // FUNC:   <funcName>
      <raw function text>
      ```
11. Print summary to stderr:
    ```
    [SELECT] Scanned 1 247 files, found 8 391 qualifying functions.
    [SELECT] Randomly selected 50 functions (seed: 1234567890).
    [SELECT] Written to: ./selected/func001.txt ... ./selected/func050.txt
    ```
12. `ts_parser_delete(parser)`, return 0.

---

## Output File Format

Each file `func<NNN>.txt`:
```
// SOURCE: src/algo/sort.cpp
// LINES:  12-18
// LENGTH: 7 lines
// LANG:   cpp
// FUNC:   bubbleSort
int bubbleSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j+1])
                std::swap(arr[j], arr[j+1]);
}
```

Directly consumable by the existing Mutate binary:
```bash
./Mutate/mutate --input ./selected/func001.txt --id func001 --output-dir ./mutants/
./Mutate/mutate --input ./selected/func002.txt --id func002 --output-dir ./mutants/
# ...
```

---

## `main` Function Exclusion — Detail

### C / C++
A `function_definition` node is excluded if its extracted `funcName == "main"`.

Tree-sitter C/C++ AST for `int main(int argc, char** argv) { ... }`:
```
function_definition
  type: primitive_type ("int")
  declarator: function_declarator
    declarator: identifier  ← this is "main"  ← CHECK THIS
    parameters: ...
  body: compound_statement
```

### Java
Java has no `main` in the same sense (it's a static method named `main`).
Exclude if `funcName == "main"` AND the parent class context is `main` or
the method has `static` modifier — **or simply always exclude `main` by name**
to be safe (configurable via `--keep-main` flag if user ever needs it).

### C#
Exclude if `funcName == "Main"` (C# convention uses capital M).

---

## Zero-Padded Filename Numbering

```cpp
// Compute padding width from N
int width = std::to_string(N).size();  // e.g. N=50 → width=2, N=500 → width=3

// Format index i (0-based) as 1-based zero-padded string
std::ostringstream ss;
ss << "func" << std::setfill('0') << std::setw(width) << (i + 1) << ".txt";
```

Examples:
- `--count 10`  → `func01.txt` … `func10.txt`
- `--count 50`  → `func001.txt` … `func050.txt`  (wait: digits of 50 = 2, so `func01.txt`…`func50.txt`)
- `--count 100` → `func001.txt` … `func100.txt`
- `--count 500` → `func001.txt` … `func500.txt`

---

## Makefile

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS  = -ltree-sitter -ltree-sitter-c -ltree-sitter-cpp \
           -ltree-sitter-java -ltree-sitter-c-sharp

SRCS = select.cpp lang_registry.cpp file_walker.cpp \
       function_extractor.cpp selector.cpp

TARGET = select

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test: tests/test_select.cpp lang_registry.cpp file_walker.cpp \
      function_extractor.cpp selector.cpp
	$(CXX) $(CXXFLAGS) -o test_select $^ $(LDFLAGS)
	./test_select

clean:
	rm -f $(TARGET) test_select
```

---

## Tests (`tests/test_select.cpp`)

| Test | What it checks |
|------|----------------|
| `test_walker_finds_extensions` | correct extensions returned |
| `test_walker_skips_hidden_dirs` | `.git/` pruned |
| `test_extract_cpp_functions` | correct count from fixture |
| `test_extract_main_excluded` | `main()` never appears in results |
| `test_extract_csharp_Main_excluded` | `Main()` (C#) excluded |
| `test_extract_size_filter` | tiny functions skipped |
| `test_extract_bad_syntax` | malformed file → empty, no crash |
| `test_reservoir_k_equals_total` | k >= N → all N functions returned |
| `test_reservoir_k_less_than_total` | exactly k distinct functions returned |
| `test_reservoir_uniform` | chi-squared uniformity over 10 000 runs on small fixture |
| `test_reservoir_seed_reproducible` | same seed → same selection |
| `test_reservoir_no_functions` | returns nullopt |
| `test_cli_n_files_written` | N output files exist after binary run |
| `test_cli_files_have_header` | each file starts with `// SOURCE:` |
| `test_cli_no_main_in_output` | no output file contains `funcName == "main"` |
| `test_cli_count_exceeds_available` | warning printed, fewer files written, exit 0 |

---

## Execution Order (Implementation Steps)

1. `Makefile`
2. `lang_registry.h / .cpp` — `LangConfig` + `getLangConfig()`
3. `file_walker.h / .cpp` — lazy recursive iterator
4. `function_extractor.h / .cpp` — tree-sitter DFS + name extraction + `main` filter + size filter
5. `selector.h / .cpp` — Algorithm R (k-of-N)
6. `select.cpp` — CLI + zero-padded file writer
7. `tests/fixtures/` — fixture files
8. `tests/test_select.cpp` — test suite

---

## Integration with Mutate Pipeline

```
BigCodebase/
  src/...

+----------+   func001.txt        +--------+   mutants/func001-1.txt
|  Select   |   func002.txt   --> | Mutate | → mutants/func001-2.txt ...
|  binary   |   ...               | binary |   mutants/func002-1.txt ...
|  (k=N)    |   func050.txt       +--------+   mutants/func002-2.txt ...
+----------+
```

Shell pipeline:
```bash
./Select/select --root ./bigcodebase --lang cpp --count 50 --output-dir ./selected/

for f in ./selected/func*.txt; do
    id=$(basename "$f" .txt)
    ./Mutate/mutate --input "$f" --id "$id" --output-dir ./mutants/
done
```

---

## Design Decisions & Rationale

| Decision | Rationale |
|----------|-----------|
| **Algorithm R (k-of-N)** | Uniform sampling without replacement; O(k) memory; single pass |
| **`main` always excluded** | `main` is never a meaningful clone target; it ties everything together |
| **Distinct selection** | Algorithm R guarantees no duplicates by construction |
| **Fewer than k available → warn, don't error** | Graceful degradation on small codebases |
| **Zero-padded filenames** | Keeps `ls` and shell globs in correct order |
| **`// FUNC:` header** | Mutate pipeline can log which function was mutated |
| **`--keep-main` flag** | Escape hatch if user ever wants to include main |
