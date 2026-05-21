#include "status.h"
#include "branch.h"
#include "add.h"
#include "ObjectStore.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

static std::string bin_to_hex(const std::string& bin) {
    std::ostringstream oss;
    for (unsigned char c : bin)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    return oss.str();
}

static std::string hash_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot read file: " + path);
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    std::string header = "blob " + std::to_string(content.size()) + '\0';
    return sha1_hex(header + content);
}

std::map<std::string, std::string> flatten_tree(const std::string& tree_sha, const std::string& prefix) {
    std::map<std::string, std::string> result;
    ObjectStore store(".git");

    std::string type, content;
    if (!store.load(tree_sha, type, content)) return result;
    if (type != "tree") return result;

    size_t pos = 0;
    while (pos < content.size()) {
        size_t space = content.find(' ', pos);
        if (space == std::string::npos) break;
        std::string mode = content.substr(pos, space - pos);
        pos = space + 1;

        size_t null_pos = content.find('\0', pos);
        if (null_pos == std::string::npos) break;
        std::string name = content.substr(pos, null_pos - pos);
        pos = null_pos + 1;

        if (pos + 20 > content.size()) break;
        std::string sha = bin_to_hex(content.substr(pos, 20));
        pos += 20;

        std::string full_path = prefix.empty() ? name : prefix + "/" + name;

        if (mode == "40000" || mode == "040000") {
            auto sub = flatten_tree(sha, full_path);
            result.insert(sub.begin(), sub.end());
        } else {
            result[full_path] = sha;
        }
    }

    return result;
}

void status() {
    std::string branch = get_current_branch();
    if (branch.empty()) {
        std::cout << "HEAD detached\n";
    } else {
        std::cout << "On branch " << branch << "\n";
    }

    // Get HEAD tree
    std::string head_commit = read_head_commit();
    std::map<std::string, std::string> head_files;

    if (!head_commit.empty()) {
        ObjectStore store(".git");
        std::string type, content;
        if (store.load(head_commit, type, content)) {
            std::istringstream iss(content);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.starts_with("tree ")) {
                    head_files = flatten_tree(line.substr(5));
                    break;
                }
            }
        }
    }

    // Get index
    auto index_entries = readIndex();
    std::map<std::string, std::string> index_files;
    for (const auto& e : index_entries) {
        index_files[e.path] = e.sha;
    }

    // Get working directory files
    std::set<std::string> working_files;
    for (const auto& entry : fs::recursive_directory_iterator(".")) {
        if (!entry.is_regular_file()) continue;
        fs::path p = entry.path().lexically_normal();
        bool is_git = false;
        for (const auto& component : p) {
            if (component == ".git") { is_git = true; break; }
        }
        if (is_git) continue;
        std::string path = p.string();
        if (path.starts_with("./")) path = path.substr(2);
        working_files.insert(path);
    }

    // Staged changes (index vs HEAD)
    std::vector<std::string> staged_new, staged_modified, staged_deleted;
    for (const auto& [path, sha] : index_files) {
        if (head_files.find(path) == head_files.end())
            staged_new.push_back(path);
        else if (head_files[path] != sha)
            staged_modified.push_back(path);
    }
    for (const auto& [path, sha] : head_files) {
        if (index_files.find(path) == index_files.end())
            staged_deleted.push_back(path);
    }

    // Unstaged changes (working dir vs index) - no disk writes
    std::vector<std::string> unstaged_modified, unstaged_deleted;
    for (const auto& [path, sha] : index_files) {
        if (working_files.find(path) == working_files.end()) {
            unstaged_deleted.push_back(path);
        } else {
            std::string working_sha = hash_file(path);
            if (working_sha != sha) {
                unstaged_modified.push_back(path);
            }
        }
    }

    // Untracked files
    std::vector<std::string> untracked;
    for (const auto& path : working_files) {
        if (index_files.find(path) == index_files.end())
            untracked.push_back(path);
    }

    // Print
    std::cout << "\n";
    if (!staged_new.empty() || !staged_modified.empty() || !staged_deleted.empty()) {
        std::cout << "Changes to be committed (staged):\n";
        for (const auto& p : staged_new)      std::cout << "  new file:  " << p << "\n";
        for (const auto& p : staged_modified) std::cout << "  modified:  " << p << "\n";
        for (const auto& p : staged_deleted)  std::cout << "  deleted:   " << p << "\n";
        std::cout << "\n";
    }

    if (!unstaged_modified.empty() || !unstaged_deleted.empty()) {
        std::cout << "Changes not staged for commit:\n";
        for (const auto& p : unstaged_modified) std::cout << "  modified:  " << p << "\n";
        for (const auto& p : unstaged_deleted)  std::cout << "  deleted:   " << p << "\n";
        std::cout << "\n";
    }

    if (!untracked.empty()) {
        std::cout << "Untracked files:\n";
        for (const auto& p : untracked) std::cout << "  " << p << "\n";
        std::cout << "\n";
    }

    if (staged_new.empty() && staged_modified.empty() && staged_deleted.empty() &&
        unstaged_modified.empty() && unstaged_deleted.empty()) {
        std::cout << "nothing to commit, working tree clean\n";
    }
}