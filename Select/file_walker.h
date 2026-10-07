#pragma once

#include <filesystem>
#include <vector>
#include <string>

class FileWalker {
public:
    FileWalker(const std::filesystem::path& root,
               const std::vector<std::string>& extensions);
    
    std::filesystem::path next();
    bool done() const;

private:
    std::filesystem::recursive_directory_iterator it;
    std::filesystem::recursive_directory_iterator end;
    std::vector<std::string> targetExtensions;
    
    void advanceToValid();
    bool hasValidExtension(const std::filesystem::path& path) const;
};
