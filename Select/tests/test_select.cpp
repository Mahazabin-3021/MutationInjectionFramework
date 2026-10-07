#include <iostream>
#include <cassert>
#include <filesystem>
#include "../file_walker.h"
#include "../function_extractor.h"
#include "../selector.h"
#include "../lang_registry.h"

// Basic rudimentary testing framework
int testsPassed = 0;
int testsFailed = 0;

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            testsFailed++; \
        } else { \
            testsPassed++; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << "Assertion failed: " << #expected << " == " << #actual \
                      << " (Expected: " << (expected) << ", Actual: " << (actual) << ")" \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            testsFailed++; \
        } else { \
            testsPassed++; \
        } \
    } while(0)

void test_walker_finds_extensions() {
    FileWalker walker("tests/fixtures", {".cpp"});
    bool found = false;
    while (!walker.done()) {
        auto p = walker.next();
        if (p.extension() == ".cpp") found = true;
    }
    ASSERT_TRUE(found);
}

void test_extract_main_excluded() {
    LangConfig config = getLangConfig("cpp");
    TSParser* parser = ts_parser_new();
    ts_parser_set_language(parser, config.grammar);
    
    auto funcs = extractFunctions("tests/fixtures/sample.cpp", parser, config.functionNodeTypes, 1, 1);
    bool mainFound = false;
    for (const auto& f : funcs) {
        if (f.funcName == "main") mainFound = true;
    }
    ASSERT_TRUE(!mainFound);
    
    ts_parser_delete(parser);
}

int main() {
    std::cout << "Running tests...\n";
    
    try {
        test_walker_finds_extensions();
        test_extract_main_excluded();
    } catch (const std::exception& e) {
        std::cerr << "Test threw exception: " << e.what() << "\n";
    }

    std::cout << "Tests Passed: " << testsPassed << "\n";
    std::cout << "Tests Failed: " << testsFailed << "\n";
    
    return testsFailed == 0 ? 0 : 1;
}
