#include "file_walker.h"
#include <algorithm>
#include <unordered_set>

static const std::unordered_set<std::string> PRUNE_DIRS = {
    ".git", ".svn", ".hg", "node_modules", "__pycache__",
    "build", "out", "dist", "bin", "obj"
};

FileWalker::FileWalker(const std::filesystem::path& root,
                       const std::vector<std::string>& extensions)
    : targetExtensions(extensions) {
    
    std::error_code ec;
    it = std::filesystem::recursive_directory_iterator(
        root,
        std::filesystem::directory_options::skip_permission_denied,
        ec
    );
    end = std::filesystem::recursive_directory_iterator();
    
    if (ec) {
        it = end; // If error opening root, mark as done
    } else {
        advanceToValid();
    }
}

void FileWalker::advanceToValid() {
    while (it != end) {
        std::error_code ec;
        if (it->is_directory(ec)) {
            if (PRUNE_DIRS.count(it->path().filename().string())) {
                it.disable_recursion_pending();
            }
        } else if (it->is_regular_file(ec)) {
            if (hasValidExtension(it->path())) {
                return; // Found a valid file
            }
        }
        
        it.increment(ec);
        if (ec) {
            it = end;
            break;
        }
    }
}

bool FileWalker::hasValidExtension(const std::filesystem::path& path) const {
    if (targetExtensions.empty()) return true;
    std::string ext = path.extension().string();
    return std::find(targetExtensions.begin(), targetExtensions.end(), ext) != targetExtensions.end();
}

std::filesystem::path FileWalker::next() {
    if (done()) return {};
    
    std::filesystem::path result = it->path();
    std::error_code ec;
    it.increment(ec);
    if (ec) {
        it = end;
    } else {
        advanceToValid();
    }
    
    return result;
}

bool FileWalker::done() const {
    return it == end;
}
