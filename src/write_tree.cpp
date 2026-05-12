#include "write_tree.h"
#include <algorithm>
#include <fstream>
#include <vector>

struct TreeEntry {
    std::string mode;
    std::string name;
    std::string sha_bin;
    bool is_dir;
};

static std::string hex_to_bin(const std::string& hex) {
    std::string bin;
    bin.reserve(20);
    for (size_t i = 0; i < 40; i += 2)
        bin += static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16));
    return bin;
}

std::string write_tree(ObjectStore& store, const fs::path& dir) {
    std::vector<TreeEntry> entries;

    for (const auto& entry : fs::directory_iterator(dir)) {
        std::string name = entry.path().filename().string();
        if (name == ".git") continue;

        if (entry.is_regular_file()) {
            std::ifstream f(entry.path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>()); // actual git uses heap memory instead of loading the whole file into memory, but for simplicity we can do it this way  
            std::string sha = store.store("blob", content);
            entries.push_back({"100644", name, hex_to_bin(sha), false});
        } else if (entry.is_directory()) {
            std::string sha = write_tree(store, entry.path());
            entries.push_back({"40000", name, hex_to_bin(sha), true});
        }
    }

    // Sort entries to ensure deterministic order so identical directories produce identical tree hashes across systems
    std::sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        std::string ka = a.is_dir ? a.name + "/" : a.name;
        std::string kb = b.is_dir ? b.name + "/" : b.name;
        return ka < kb;
    });

    std::string body;
    for (const auto& e : entries)
        body += e.mode + " " + e.name + '\0' + e.sha_bin;

    return store.store("tree", body);
}