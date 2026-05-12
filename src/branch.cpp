#include "branch.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

std::string get_current_branch(){
    std::ifstream f(".git/HEAD");
    if (!f) return "";
    std::string line;
    std::getline(f,line);
    if (line.starts_with("ref: refs/heads/")){
        return line.substr(16);
    }
    return "";
}

std::string read_head_commit(){
    std::ifstream f(".git/HEAD");
    if (!f) throw std::runtime_error("Cannot read HEAD");

    std::string line;
    std::getline(f,line);
    if (line.starts_with("ref: ")){
        std::string ref_path = ".git/" + line.substr(5);
        std::ifstream ref_file(ref_path);
        if (!ref_file) throw std::runtime_error("Cannot read ref: " + ref_path);
        std::string sha;
        std::getline(ref_file,sha);
        return sha; 
    }

    //detached head return sha directly
    return line;
}

void branch_create(const std::string& name){
    fs::path branch_path = ".git/refs/heads/" + name;
    if (fs::exists(branch_path)) throw std::runtime_error("Branch already exists: "+name);

    std::string commit_sha = read_head_commit();
    fs::create_directories(branch_path.parent_path());
    std::ofstream out(branch_path);
    if (!out) throw std::runtime_error("Cannot create branch: " + name);
    out << commit_sha;

    std::cout << "Branch '" << name << "' created at " << commit_sha << std::endl;
}

void branch_list(){
    fs::path refs_dir = ".git/refs/heads";
    if (!fs::exists(refs_dir)){
        std::cout<< "No branches found." << std::endl;
        return;
    }
    std::string current_branch = get_current_branch();
    for (const auto& entry : fs::directory_iterator(refs_dir)){
        std::string branch_name = entry.path().filename().string();
        if (branch_name == current_branch){
            std::cout << "* " << branch_name << std::endl;
        }else{
            std::cout << "  " << branch_name << std::endl;
        }
    }
}

void branch(const std::vector<std::string>& args) {
    if (args.empty()) {
        // List branches
        branch_list();
    } else if (args.size() == 1) {
        // Create branch
        branch_create(args[0]);
    } else {
        throw std::runtime_error("Usage: branch [name]");
    }
}