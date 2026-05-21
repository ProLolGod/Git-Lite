#include "diff.h"
#include "add.h"
#include "status.h"
#include "branch.h"
#include "ObjectStore.h"
#include "utils.h"
#include "dtl/dtl.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <map>

namespace fs = std::filesystem;

static std::vector<std::string> split_lines(const std::string& content){
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while(std::getline(iss,line)){
        lines.push_back(line);
    }
    return lines;
}

std::string diff_strings(const std::string& old_content, const std::string& new_content, const std::string& filename){
    std::vector<std::string> old_lines = split_lines(old_content);
    std::vector<std::string> new_lines = split_lines(new_content);

    dtl::Diff<std::string> diff(old_lines,new_lines);
    diff.compose();
    diff.composeUnifiedHunks();

    std::ostringstream out; // we use osstringstream to build the diff string 

    out << "--- a/" << filename << "\n";
    out << "+++ b/" << filename << "\n";
    for (const auto& hunk : diff.getUniHunks()){
        out << "@@ -" << hunk.a << "," << hunk.b << " +" << hunk.c << "," << hunk.d << " @@\n";

        for (const auto& element: hunk.common[0]){
            out << " " <<element.first << "\n"; //for getting common lines we use space as prefix
        }
        for (const auto& element: hunk.change){
            if (element.second.type == dtl::SES_DELETE) out <<"-"<< element.first << "\n"; 
            else if (element.second.type == dtl::SES_ADD) out << "+" << element.first << "\n"; 
            else if (element.second.type == dtl::SES_COMMON) out << " " << element.first << "\n"; 
        }
        for (const auto& element : hunk.common[1]){
            out <<" "<<element.first <<"\n";
        }

    }
    return out.str();

}

static std::string read_blob(const std::string& sha) {
    ObjectStore store(".git");
    std::string type, content;
    if (!store.load(sha, type, content)) throw std::runtime_error("Object not found: " + sha);
    if (type != "blob") throw std::runtime_error("Not a blob: " + sha);
    return content;
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot read file: " + path);
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

void diff_working() {
    auto index_entries = readIndex();
    std::map<std::string, std::string> index_map;
    for (const auto& e : index_entries) {
        index_map[e.path] = e.sha;
    }

    bool any_diff = false;

    for (const auto& [path, sha] : index_map) {
        if (!fs::exists(path)) {
            std::cout << "deleted: " << path << "\n";
            any_diff = true;
            continue;
        }

        std::string working_content = read_file(path);
        std::string header = "blob " + std::to_string(working_content.size()) + '\0';
        std::string working_sha = sha1_hex(header + working_content);

        if (working_sha != sha) {
            std::string blob_content = read_blob(sha);
            std::cout << diff_strings(blob_content, working_content, path);
            any_diff = true;
        }
    }

    if (!any_diff) std::cout << "No differences\n";
}


void diff_staged() {
    auto index_entries = readIndex();
    std::map<std::string, std::string> index_map;
    for (const auto& e : index_entries) {
        index_map[e.path] = e.sha;
    }

    std::string head_commit = read_head_commit();
    std::map<std::string, std::string> head_map;

    if (!head_commit.empty()) {
        ObjectStore store(".git");
        std::string type, content;
        if (store.load(head_commit, type, content)) {
            std::istringstream iss(content);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.starts_with("tree ")) {
                    head_map = flatten_tree(line.substr(5));
                    break;
                }
            }
        }
    }

    bool any_diff = false;

    // New or modified files
    for (const auto& [path, sha] : index_map) {
        if (head_map.find(path) == head_map.end()) {
            std::string blob_content = read_blob(sha);
            std::cout << diff_strings("", blob_content, path);
            any_diff = true;
        } else if (head_map[path] != sha) {
            std::string old_content = read_blob(head_map[path]);
            std::string new_content = read_blob(sha);
            std::cout << diff_strings(old_content, new_content, path);
            any_diff = true;
        }
    }

    // Deleted files
    for (const auto& [path, sha] : head_map) {
        if (index_map.find(path) == index_map.end()) {
            std::string old_content = read_blob(sha);
            std::cout << diff_strings(old_content, "", path);
            any_diff = true;
        }
    }

    if (!any_diff) std::cout << "No differences\n";
}

void diff_commits(const std::string& sha1, const std::string& sha2) {
    ObjectStore store(".git");

    // Get tree from commit 1
    std::map<std::string, std::string> map1;
    {
        std::string type, content;
        if (!store.load(sha1, type, content)) throw std::runtime_error("Commit not found: " + sha1);
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.starts_with("tree ")) {
                map1 = flatten_tree(line.substr(5));
                break;
            }
        }
    }

    // Get tree from commit 2
    std::map<std::string, std::string> map2;
    {
        std::string type, content;
        if (!store.load(sha2, type, content)) throw std::runtime_error("Commit not found: " + sha2);
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.starts_with("tree ")) {
                map2 = flatten_tree(line.substr(5));
                break;
            }
        }
    }

    bool any_diff = false;

    // New or modified in sha2
    for (const auto& [path, sha] : map2) {
        if (map1.find(path) == map1.end()) {
            std::string new_content = read_blob(sha);
            std::cout << diff_strings("", new_content, path);
            any_diff = true;
        } else if (map1[path] != sha) {
            std::string old_content = read_blob(map1[path]);
            std::string new_content = read_blob(sha);
            std::cout << diff_strings(old_content, new_content, path);
            any_diff = true;
        }
    }

    // Deleted in sha2
    for (const auto& [path, sha] : map1) {
        if (map2.find(path) == map2.end()) {
            std::string old_content = read_blob(sha);
            std::cout << diff_strings(old_content, "", path);
            any_diff = true;
        }
    }

    if (!any_diff) std::cout << "No differences\n";
}

void diff(int argc, char* argv[]) {
    if (argc == 2) {
        diff_working();
    } 
    else if (argc == 3 && std::string(argv[2]) == "--staged") {
        diff_staged();
    } else if (argc == 4) {
        diff_commits(argv[2], argv[3]);
    } 
    else {
        throw std::runtime_error("Usage: diff [--staged] [<sha1> <sha2>]");
    }
}
