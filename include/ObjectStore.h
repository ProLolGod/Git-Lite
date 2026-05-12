#pragma once
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class ObjectStore {
public:
    explicit ObjectStore(const fs::path& git_dir);

    std::string store(const std::string& type, const std::string& content);
    bool load(const std::string& sha, std::string& type, std::string& content);
    bool exists(const std::string& sha);

private:
    fs::path object_path(const std::string& sha);
    fs::path git_dir_;
};