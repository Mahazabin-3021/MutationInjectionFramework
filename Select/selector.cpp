#include "selector.h"
#include <random>

std::optional<SelectResult> reservoirSample(
    FileWalker&                      walker,
    TSParser*                        parser,
    const std::vector<std::string>&  functionNodeTypes,
    const std::string&               langStr,
    int                              k,
    int                              minLines,
    int                              minChars,
    uint64_t                         seed
) {
    std::mt19937_64 rng(seed);

    std::vector<FunctionRecord> reservoir;
    reservoir.reserve(k);
    long long count = 0;
    long long files = 0;

    while (!walker.done()) {
        auto file = walker.next();
        if (file.empty()) break;
        ++files;
        auto funcs = extractFunctions(file, parser, functionNodeTypes, langStr, minLines, minChars);

        for (auto& f : funcs) {
            ++count;
            if ((long long)reservoir.size() < k) {
                // Phase 1: fill reservoir until we have k items
                reservoir.push_back(std::move(f));
            } else {
                // Phase 2: replace a random existing item with probability k/count
                std::uniform_int_distribution<long long> dist(1, count);
                long long j = dist(rng);
                if (j <= k) {
                    reservoir[j - 1] = std::move(f);   // replace slot j (1-indexed)
                }
            }
        }
    }

    if (reservoir.empty()) return std::nullopt;

    return SelectResult{ std::move(reservoir), count, files };
}
