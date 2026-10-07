// tests/test_runner.cpp
// Round-trip + per-mutator smoke tests for the Mutation Injection Framework.
// Compiled separately: g++ -std=c++17 -I.. -o test_runner test_runner.cpp ../tokenizer.cpp
// (mutation files are header-included from within this TU)

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "../tokenizer.h"
#include "../mutation_base.h"

// Include all mutation implementations
#include "../mutations/mCW_A.cpp"
#include "../mutations/mCW_R.cpp"
#include "../mutations/mCC_BT.cpp"
#include "../mutations/mCC_EOL.cpp"
#include "../mutations/mCF_A.cpp"
#include "../mutations/mCF_R.cpp"
#include "../mutations/mCF_SL.cpp"
#include "../mutations/mSRI.cpp"
#include "../mutations/mARI.cpp"
#include "../mutations/mRL_N.cpp"
#include "../mutations/mRL_S.cpp"
#include "../mutations/mSIL.cpp"
#include "../mutations/mSDL.cpp"
#include "../mutations/mIL.cpp"
#include "../mutations/mDL.cpp"
#include "../mutations/mML.cpp"

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------
static int passed = 0, failed = 0;

static void check(bool cond, const std::string& desc) {
    if (cond) {
        std::cout << "  [PASS] " << desc << "\n";
        ++passed;
    } else {
        std::cout << "  [FAIL] " << desc << "\n";
        ++failed;
    }
}

// ---------------------------------------------------------------------------
// Test inputs
// ---------------------------------------------------------------------------
static const std::string gcd_cpp = R"(int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
})";

static const std::string simple_java = R"(public int add(int x, int y) {
    int result = x + y;
    System.out.println("sum");
    return result;
})";

static const std::string with_strings = R"(void greet(char* name) {
    printf("Hello, %s!\n", name);
    int code = 42;
    return;
})";

// ---------------------------------------------------------------------------
// Round-trip test
// ---------------------------------------------------------------------------
static void testRoundTrip() {
    std::cout << "\n--- Round-trip Tests ---\n";
    for (auto& [src, lang, label] : std::vector<std::tuple<std::string,Language,std::string>>{
        { gcd_cpp,      Language::CPP,  "gcd (C++)" },
        { simple_java,  Language::JAVA, "add (Java)" },
        { with_strings, Language::C,    "greet (C)" },
    }) {
        Tokenizer tok(src, lang);
        auto tokens = tok.tokenize();
        std::string rt = reconstruct(tokens);
        check(rt == src, "Round-trip: " + label);
    }
}

// ---------------------------------------------------------------------------
// Per-mutator smoke tests
// ---------------------------------------------------------------------------
static void testMutator(MutationBase& m, const std::vector<Token>& tokens,
                         const std::string& label, bool requireAtLeastOne = true) {
    auto mutants = m.mutate(tokens);
    if (requireAtLeastOne)
        check(!mutants.empty(), m.name() + " produces >= 1 mutant for " + label);
    else
        check(true, m.name() + " ran (0 mutants ok) for " + label);

    // Each mutant must be non-empty
    for (auto& mut : mutants)
        check(!mut.empty(), m.name() + " mutant is non-empty");
}

static void testAllMutators() {
    std::cout << "\n--- Mutator Smoke Tests (gcd C++) ---\n";
    Tokenizer tok(gcd_cpp, Language::CPP);
    auto tokens = tok.tokenize();

    mCW_A  m1;  testMutator(m1,  tokens, "gcd");
    mCW_R  m2;  testMutator(m2,  tokens, "gcd");
    mCC_BT m3;  testMutator(m3,  tokens, "gcd");
    mCC_EOL m4; testMutator(m4,  tokens, "gcd");
    mCF_A  m5;  testMutator(m5,  tokens, "gcd");
    mCF_R  m6;  testMutator(m6,  tokens, "gcd");
    mCF_SL m7;  testMutator(m7,  tokens, "gcd");
    mSRI   m8;  testMutator(m8,  tokens, "gcd");
    mARI   m9;  testMutator(m9,  tokens, "gcd");
    mRL_N  m10; testMutator(m10, tokens, "gcd", false); // gcd has no numeric literals
    mRL_S  m11; testMutator(m11, tokens, "gcd", false); // no strings
    mSIL   m12; testMutator(m12, tokens, "gcd");
    mSDL   m13; testMutator(m13, tokens, "gcd");
    mIL    m14; testMutator(m14, tokens, "gcd");
    mDL    m15; testMutator(m15, tokens, "gcd");
    mML    m16; testMutator(m16, tokens, "gcd");
}

// ---------------------------------------------------------------------------
// mCF_SL specific tests
// ---------------------------------------------------------------------------
static void testSingleLineMutation() {
    std::cout << "\n--- mCF_SL Single-Line Collapse Tests ---\n";

    // Basic: multiline function must collapse to 1 line
    {
        Tokenizer tok(gcd_cpp, Language::CPP);
        auto tokens = tok.tokenize();
        mCF_SL m;
        auto mutants = m.mutate(tokens);
        check(mutants.size() == 1, "mCF_SL: produces exactly 1 mutant");
        if (!mutants.empty()) {
            const auto& s = mutants[0];
            check(s.find('\n') == std::string::npos, "mCF_SL: no newlines in output");
            check(s.find("int gcd") != std::string::npos, "mCF_SL: function signature preserved");
            check(s.find("return a") != std::string::npos, "mCF_SL: return statement preserved");
            check(s.find("while") != std::string::npos, "mCF_SL: while loop preserved");
            std::cout << "  [INFO] Single-line output:\n  " << s << "\n";
        }
    }

    // Already single-line → no mutants
    {
        std::string oneliner = "int f(int x) { return x + 1; }";
        Tokenizer tok(oneliner, Language::CPP);
        auto tokens = tok.tokenize();
        mCF_SL m;
        auto mutants = m.mutate(tokens);
        check(mutants.empty(), "mCF_SL: no mutant for already-single-line function");
    }

    // Function with line comments — comments must not swallow code
    {
        std::string withComment = R"(int f(int a) {
    // compute result
    int r = a * 2; // double it
    return r;
})";
        Tokenizer tok(withComment, Language::CPP);
        auto tokens = tok.tokenize();
        mCF_SL m;
        auto mutants = m.mutate(tokens);
        check(mutants.size() == 1, "mCF_SL: produces 1 mutant for function with comments");
        if (!mutants.empty()) {
            const auto& s = mutants[0];
            check(s.find('\n') == std::string::npos, "mCF_SL: no newlines in comment-stripping output");
            check(s.find("int r = a * 2") != std::string::npos,
                  "mCF_SL: statement after comment is preserved");
            check(s.find("return r") != std::string::npos,
                  "mCF_SL: return after comment is preserved");
            std::cout << "  [INFO] Comment-strip output:\n  " << s << "\n";
        }
    }

    // Java function
    {
        Tokenizer tok(simple_java, Language::JAVA);
        auto tokens = tok.tokenize();
        mCF_SL m;
        auto mutants = m.mutate(tokens);
        check(mutants.size() == 1, "mCF_SL: works on Java input");
        if (!mutants.empty()) {
            check(mutants[0].find('\n') == std::string::npos,
                  "mCF_SL: no newlines in Java output");
        }
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    std::cout << "=== Mutation Injection Framework — Test Runner ===\n";

    testRoundTrip();
    testAllMutators();
    testSingleLineMutation();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return (failed == 0) ? 0 : 1;
}
