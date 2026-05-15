#include "checkout.h"
#include "ObjectStore.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <vector>

namespace fs = std::filesystem;

namespace GitLite {

std::string binaryToHex(const std::string& bin){
    std::ostringstream oss;
    for (unsigned char c : bin){
        oss << std::hex <<std::setw(2) << std::setfill('0') << (int)c;
    }
    return oss.str();
}

std::string readHEAD(){
    std::ifstream f(".git/HEAD");
    if (!f) throw std::runtime_error("Cannot read HEAD");

    std::string line;
    std::getline(f,line);
    if (line.starts_with("ref: ")){
        std::string ref_path = line.substr(5);
        return resolveRef(ref_path);
    }

    return line;
}

std::string resolveRef(const std::string &ref_path){
    std::string full_path = ".git/" + ref_path;
    std::ifstream f(full_path);
    if (!f) throw std::runtime_error("Cannot resolve ref: " + ref_path);

    std::string sha;
    std::getline(f,sha);
    return sha;
}

std::string extractTreeFromCommit(ObjectStore& store, const std::string& commit_sha){
    std::string type,content;
    if (!store.load(commit_sha,type,content)) throw std::runtime_error("Commit not found: " + commit_sha);
    if (type != "commit") throw std::runtime_error("Object is not a commit: " + commit_sha);

    size_t tree_pos = content.find("tree ");
    if (tree_pos == std::string::npos) throw std::runtime_error("Invalid commit data: no tree");

    size_t end = content.find('\n',tree_pos);
    return content.substr(tree_pos + 5,end-tree_pos-5);
}

std::vector<TreeEntry> parseTree(const std::string& tree_data){
    std::vector<TreeEntry> entries;
    size_t pos = 0;
    while(pos<tree_data.size()){
        size_t space = tree_data.find(' ',pos);
        if (space == std::string::npos) break;

        std::string mode = tree_data.substr(pos,space-pos);
        pos = space + 1;

        //read name until null
        size_t nullpos = tree_data.find('\0',pos);
        if (nullpos == std::string::npos) break;

        std::string name = tree_data.substr(pos,nullpos-pos);
        pos = nullpos + 1;
        if (pos+20 > tree_data.size()) break;
        std::string binary_sha = tree_data.substr(pos,20);
        pos+=20;

        std::string hex_sha = binaryToHex(binary_sha);
        entries.push_back({mode,name,hex_sha});
    }
    return entries;
}

void restoreBlob(const std::string& blob_sha, const std::string& filepath){
    ObjectStore store(".git");
    std::string type,content;
    if (!store.load(blob_sha,type,content)) throw std::runtime_error("Blob not found: " + blob_sha);
    if (type != "blob") throw std::runtime_error("Object is not a blob: " + blob_sha);

    fs::path path(filepath);
    if (path.has_parent_path()){
        fs::create_directories(path.parent_path());
    }

    std::ofstream out(filepath,std::ios::binary);
    if (!out) throw std::runtime_error("Cannot write file: " + filepath);
    out.write(content.data(),content.size());
}

void restoreTree(const std::string &tree_sha, const std::string &base_path){
    ObjectStore store(".git");
    std::string type,content;
    if (!store.load(tree_sha,type,content)) throw std::runtime_error("Tree not found: " + tree_sha);
    if (type!="tree") throw std::runtime_error("Object is not a tree!");
    auto entries = parseTree(content);
    std::cout << "Entries count: " << entries.size() << "\n";
    for(const auto& entry : entries){
        fs::path full_path = fs::path(base_path) / entry.name;
        // extra 0 is seen in actual git cause of octal system or something
        if (entry.mode == "40000" || entry.mode == "040000"){
            fs::create_directories(full_path);
            restoreTree(entry.sha, full_path.string());
        }else{
            restoreBlob(entry.sha, full_path.string());
        }
        std::cout << entry.mode << " "
          << entry.name << " "
          << entry.sha << "\n";
    }
}

void updateHEAD(const std::string &target){
    std::string branch_path = ".git/refs/heads/" + target;
    
    std::ofstream f(".git/HEAD");
    if (!f) throw std::runtime_error("Cannot update HEAD");
    
    if (fs::exists(branch_path)){
        f << "ref: refs/heads/" << target << '\n';
    }else{
        f << target << '\n';
    }

// Git commits are reverse-linked:
// commit → parent (no child pointers)
//
// Example:
// A ← B ← C
// C.parent = B
//
// Branches are lightweight refs (files storing latest commit SHA):
// main → C
//
// Normally:
// HEAD → main → C
//
// Detached HEAD:
// HEAD → B (direct commit, not branch)
//
// New commits from detached HEAD:
// A ← B ← C
//     ↓
//     D
//
// main still points to C.
// D exists but becomes dangling/unreachable
// unless a branch/ref points to it.
}

void checkout(const std::string& target){
    ObjectStore store(".git");
    std::string commit_sha;
    std::string branch_path = ".git/refs/heads/" + target;
    if (fs::exists(branch_path)){
        commit_sha = resolveRef("refs/heads/" + target);
    }else{
        commit_sha = target;
    }

    std::string tree_sha = extractTreeFromCommit(store,commit_sha);

    for (const auto& entry : fs::directory_iterator(".")){
        if (entry.path().filename() != ".git") fs::remove_all(entry.path());
    }
    restoreTree(tree_sha, ".");
    updateHEAD(target);

    std::cout << "Checked out " << target << std::endl;

// fs::directory_iterator(".") operates relative to current working directory.
//
// Current checkout implementation deletes/recreates files in "." directly,
// so checkout should be run from repo root unless repo root resolution is implemented.
//
// Real Git resolves the repository root first instead of blindly using current directory.
}

} // namespace GitLite