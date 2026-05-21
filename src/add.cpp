#include "add.h"
#include "ObjectStore.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

std::vector<IndexEntry> readIndex(){
    std::vector<IndexEntry> entries;
    std::ifstream f(".git/index");
    if (!f) return entries; // no index file yet

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        IndexEntry entry;
        // Tab-delimited to handle spaces in filenames
        std::getline(iss, entry.mode, '\t');
        std::getline(iss, entry.sha, '\t');
        std::getline(iss, entry.path);
        entries.push_back(entry);
    }
    return entries;
}



void writeIndex(const std::vector<IndexEntry>& entries){
    // Atomic index update:
    // write to temporary file first, then atomically rename over old index.
    // Prevents partial/corrupted index if crash occurs during write.
    fs::path tmp = fs::path(".git") / "index.tmp";
    {
        // Scoped block ensures ofstream is flushed and closed
        // before rename() is executed.
        std::ofstream f(tmp);
        if (!f) throw std::runtime_error("Cannot write index");
        for (const auto& e:entries){
            f << e.mode << "\t" << e.sha << "\t" << e.path << "\n";
        }
    }
    fs::rename(tmp, fs::path(".git") / "index"); // atomic replace
}

static bool is_git_path(const fs::path& path){
    //walk all path components, reject if any is ".git"
    for (const auto& component: path){
        if (component == ".git") return true;
    }
    return false;
}

static void add_single(const std::string& filepath){
    // read file content
    std::ifstream f(filepath, std::ios::binary);
    if (!f) throw std::runtime_error("Cannot read file:"+filepath);
    std::string content((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());

    //hash and store as blob
    ObjectStore store(".git");
    std::string sha = store.store("blob", content);

    //deteremine mode correctly 
    auto perms = fs::status(filepath).permissions();
    std::string mode = (perms & fs::perms::owner_exec) != fs::perms::none
                       ? "100755" : "100644";
    
    //read existing index
    auto entries = readIndex();

    //update if exists, append if new
    bool found = false;
    for (auto& e: entries){
        if (e.path == filepath){
            e.sha = sha;
            e.mode = mode;
            found = true;
            break;
        }
    }
    if (!found){
        entries.push_back({mode, sha, filepath});
    }
    writeIndex(entries);

    std::cout<<"Added: " << filepath << std::endl;
}

void add(const std::string& filepath){
    if (filepath=="."){
        for (const auto&entry: fs::recursive_directory_iterator(".")){
            if (!entry.is_regular_file()) continue;
            if (is_git_path(entry.path())) continue;
            add_single(entry.path().string());
        }
    }else{
        if (!fs::exists(filepath)) throw std::runtime_error("File does not exist: "+filepath);
        if (fs::is_directory(filepath)) throw std::runtime_error("Directories not supported: "+filepath);
        if (is_git_path(filepath)) throw std::runtime_error("Cannot add .git directory: "+filepath);
        add_single(filepath);
    }
}