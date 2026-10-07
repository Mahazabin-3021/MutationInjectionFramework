#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <filesystem>
#include <stdexcept>

#include "tokenizer.h"
#include "mutation_base.h"

// Single-TU build: all mutation implementations included here
#include "mutations/mCW_A.cpp"
#include "mutations/mCW_R.cpp"
#include "mutations/mCC_BT.cpp"
#include "mutations/mCC_EOL.cpp"
#include "mutations/mCF_A.cpp"
#include "mutations/mCF_R.cpp"
#include "mutations/mCF_SL.cpp"
#include "mutations/mSRI.cpp"
#include "mutations/mARI.cpp"
#include "mutations/mRL_N.cpp"
#include "mutations/mRL_S.cpp"
#include "mutations/mSIL.cpp"
#include "mutations/mSDL.cpp"
#include "mutations/mIL.cpp"
#include "mutations/mDL.cpp"
#include "mutations/mML.cpp"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static Language parseLanguage(const std::string& s) {
    if (s == "c")                    return Language::C;
    if (s == "cpp" || s == "c++")   return Language::CPP;
    if (s == "java")                 return Language::JAVA;
    if (s == "csharp" || s == "cs") return Language::CSHARP;
    throw std::invalid_argument("Unknown language: " + s);
}

// Auto-detect language from file extension
static Language detectLanguage(const std::string& path) {
    auto dot = path.rfind('.');
    if (dot == std::string::npos)
        throw std::invalid_argument("Cannot detect language from: " + path);
    std::string ext = path.substr(dot + 1);
    if (ext == "c")                        return Language::C;
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "h" || ext == "hpp")
                                           return Language::CPP;
    if (ext == "java")                     return Language::JAVA;
    if (ext == "cs")                       return Language::CSHARP;
    throw std::invalid_argument("Unrecognised extension: " + ext);
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Comment prefix for a given language
static std::string commentPrefix(Language lang) {
    (void)lang; // all four languages support //
    return "// ";
}

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------
static void usage(const char* prog) {
    std::cerr <<
        "Usage: " << prog << " --input <file> --id <id> --output-dir <dir>\n"
        "                     [--lang <c|cpp|java|csharp>]  (auto-detected from extension)\n"
        "\n"
        "For each mutant produced, writes  <output-dir>/<id>-N.txt  where:\n"
        "  Line 1 : // [<mutator>] (<clone-type>) <description>\n"
        "  Line 2+: the mutated function\n"
        "\n"
        "Mutations (16 total):\n"
        "  Type-1 (formatting) : mCW_A  mCW_R  mCC_BT  mCC_EOL  mCF_A  mCF_R  mCF_SL\n"
        "  Type-2 (rename/lit) : mSRI   mARI   mRL_N   mRL_S\n"
        "  Type-3 (structural) : mSIL   mSDL   mIL     mDL      mML\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    std::string langStr, inputPath, outputDir, id;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--lang"   || arg == "-l") && i + 1 < argc) langStr   = argv[++i];
        else if ((arg == "--input"  || arg == "-i") && i + 1 < argc) inputPath = argv[++i];
        else if ((arg == "--output-dir" || arg == "-o") && i + 1 < argc) outputDir = argv[++i];
        else if ((arg == "--id") && i + 1 < argc)                   id        = argv[++i];
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
    }

    if (inputPath.empty() || id.empty() || outputDir.empty()) {
        usage(argv[0]); return 1;
    }

    // Resolve language
    Language lang;
    try {
        lang = langStr.empty() ? detectLanguage(inputPath)
                               : parseLanguage(langStr);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"; return 1;
    }

    // Read source
    std::string source;
    try { source = readFile(inputPath); }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"; return 1;
    }

    // Tokenize
    Tokenizer tokenizer(source, lang);
    std::vector<Token> tokens = tokenizer.tokenize();

    // Round-trip sanity check
    if (reconstruct(tokens) != source)
        std::cerr << "[WARNING] Tokenizer round-trip mismatch!\n";

    // Register all 16 mutators
    std::vector<std::unique_ptr<MutationBase>> mutators;
    mutators.push_back(std::make_unique<mCW_A>());
    mutators.push_back(std::make_unique<mCW_R>());
    mutators.push_back(std::make_unique<mCC_BT>());
    mutators.push_back(std::make_unique<mCC_EOL>());
    mutators.push_back(std::make_unique<mCF_A>());
    mutators.push_back(std::make_unique<mCF_R>());
    mutators.push_back(std::make_unique<mCF_SL>());
    mutators.push_back(std::make_unique<mSRI>());
    mutators.push_back(std::make_unique<mARI>());
    mutators.push_back(std::make_unique<mRL_N>());
    mutators.push_back(std::make_unique<mRL_S>());
    mutators.push_back(std::make_unique<mSIL>());
    mutators.push_back(std::make_unique<mSDL>());
    mutators.push_back(std::make_unique<mIL>());
    mutators.push_back(std::make_unique<mDL>());
    mutators.push_back(std::make_unique<mML>());

    // Create output directory
    std::filesystem::create_directories(outputDir);

    const std::string cpfx = commentPrefix(lang);

    // Seed a PRNG once — used to randomly select one site per mutator
    std::mt19937 rng{ std::random_device{}() };

    int globalIndex = 1;   // sequential counter across all mutators

    for (const auto& mutator : mutators) {
        auto mutants = mutator->mutate(tokens);
        if (mutants.empty()) continue;   // mutator not applicable — skip

        // Pick ONE mutant at random from all applicable sites
        std::uniform_int_distribution<std::size_t> dist(0, mutants.size() - 1);
        const std::string& body = mutants[dist(rng)];

        // Build filename: <outputDir>/<id>-<N>.txt
        std::string filename = outputDir + "/" + id + "-"
                             + std::to_string(globalIndex) + ".txt";

        // Line 1: comment header
        std::string header = cpfx
            + "[" + mutator->name() + "]"
            + " (" + mutator->cloneType() + ")"
            + " " + mutator->description();

        std::ofstream ofs(filename);
        if (!ofs) {
            std::cerr << "Error: cannot write " << filename << "\n";
            continue;
        }
        ofs << header << "\n" << body << "\n";

        std::cout << "  " << filename
                  << "  (" << mutants.size() << " site(s), picked 1)\n";
        ++globalIndex;
    }

    int total = globalIndex - 1;
    std::cerr << "[INFO] " << total << " mutant file(s) written to "
              << outputDir << "/\n";
    return 0;
}
