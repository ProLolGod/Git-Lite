#include "write_tree.h"
#include "add.h"
#include <algorithm>
#include <fstream>
#include <vector>
#include <set>
#include <map>

struct TreeEntry {
    std::string mode;
    std::string name;
    std::string sha_bin;
    bool is_dir;
    TreeEntry(std::string m, std::string n, std::string s, bool d)
        : mode(m), name(n), sha_bin(s), is_dir(d) {}
};

static std::string hex_to_bin(const std::string& hex) {
    std::string bin;
    bin.reserve(20);
    for (size_t i = 0; i < 40; i += 2)
        bin += static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16));
    return bin;
}

static std::string write_tree_staged(ObjectStore& store, std::vector<IndexEntry> entries) {
    std::map<std::string, std::vector<IndexEntry>> dirs;
    std::vector<IndexEntry> root_files;

    for (const auto& e : entries) {
        fs::path p = fs::path(e.path).lexically_normal();

        if (p.parent_path().empty() || p.parent_path() == ".") {
            // Root level file
            IndexEntry normalized = e;
            normalized.path = p.string();
            root_files.push_back(normalized);
        } else {
            // Subdirectory file - strip top component
            std::string top = p.begin()->string();
            IndexEntry stripped = e;
            stripped.path = p.lexically_relative(top).string();
            dirs[top].push_back(stripped);
        }
    }

    std::vector<TreeEntry> tree_entries;

    // Root level files
    for (const auto& e : root_files) {
        std::string name = fs::path(e.path).filename().string();
        tree_entries.push_back({e.mode, name, hex_to_bin(e.sha), false});
    }

    // Recursively build subtrees
    for (auto& [dirname, subentries] : dirs) {
        std::string subtree_sha = write_tree_staged(store, subentries);
        tree_entries.push_back({"40000", dirname, hex_to_bin(subtree_sha), true});
    }

    // Sort - directories sort with trailing slash
    std::sort(tree_entries.begin(), tree_entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        std::string ka = a.is_dir ? a.name + "/" : a.name;
        std::string kb = b.is_dir ? b.name + "/" : b.name;
        return ka < kb;
    });

    std::string body;
    for (const auto& e : tree_entries)
        body += e.mode + " " + e.name + '\0' + e.sha_bin;

    return store.store("tree", body);
}

std::string write_tree(ObjectStore& store, const fs::path& dir, bool staged) {
    
    if (staged) {
        auto index_entries = readIndex();
        if (index_entries.empty()) throw std::runtime_error("Nothing staged. Use add first.");
        return write_tree_staged(store, index_entries);
    }
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
            std::string sha = write_tree(store, entry.path(),false);
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