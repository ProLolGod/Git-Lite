#include "commit.h"
#include "commit_tree.h"
#include "status.h"
#include "write_tree.h"
#include "ObjectStore.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

static std::string get_head_commit() {
    std::ifstream f(".git/HEAD");
    if (!f) throw std::runtime_error("Cannot read HEAD");

    std::string line;
    std::getline(f, line);

    if (line.starts_with("ref: ")) {
        std::string ref_path = ".git/" + line.substr(5);
        std::ifstream ref(ref_path);
        if (!ref) return ""; // first commit, no parent yet
        std::string sha;
        std::getline(ref, sha);
        return sha;
    }

    // Detached HEAD
    return line;
}

static std::string get_current_branch() {
    std::ifstream f(".git/HEAD");
    if (!f) throw std::runtime_error("Cannot read HEAD");

    std::string line;
    std::getline(f, line);

    if (line.starts_with("ref: refs/heads/")) {
        return line.substr(16);
    }

    return ""; // detached HEAD
}

static void update_branch_ref(const std::string& branch, const std::string& sha) {
    fs::path ref_path = fs::path(".git/refs/heads") / branch;
    fs::create_directories(ref_path.parent_path());

    // Atomic write
    fs::path tmp = ref_path.parent_path() / (branch + ".tmp");
    {
        std::ofstream f(tmp);
        if (!f) throw std::runtime_error("Cannot write branch ref");
        f << sha;
    }
    fs::rename(tmp, ref_path);
    
}

void commit(const std::string& message) {
    if (message.empty()) throw std::runtime_error("Commit message cannot be empty");

    ObjectStore store(".git");

    // 1. Build tree from working directory
    std::string tree_sha = write_tree(store, ".",true); // true to use staged index, false to read directly from filesystem

    // 2. Get parent commit
    std::string parent = get_head_commit();
    std::vector<std::string> parents;
    if (!parent.empty()) parents.push_back(parent);

    // 3. Create commit object
    std::string commit_sha = commit_tree(store, tree_sha, parents, message);

    // 4. Update branch ref
    std::string branch = get_current_branch();
    if (branch.empty()) throw std::runtime_error("Cannot commit in detached HEAD state");
    update_branch_ref(branch, commit_sha);

    std::cout << "[" << branch << " " << commit_sha.substr(0, 7) << "] " << message << std::endl;
}