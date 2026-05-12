#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "ObjectStore.h"
#include "write_tree.h"
#include "commit_tree.h"
#include "mktag.h"
#include "log.h"
#include <sstream>
#include <iomanip>
#include "checkout.h"
#include "branch.h"

void init_repo(const std::string& path); // Declared so that compiler knows about its existance before main()

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: git-lite <command> [args]\n";
        return 1;
    }

    std::string cmd = argv[1];

    try {
        if (cmd == "init") {
            init_repo(".");
            std::cout << "Initialized empty git-lite repository\n";
        }
        else if (cmd == "hash-object") {
            if (argc < 3) throw std::runtime_error("Usage: hash-object <file>");
            std::ifstream f(argv[2], std::ios::binary); //std::ios::binary to prevent newline translation on windows
            if (!f) throw std::runtime_error("Cannot open file");
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
            ObjectStore store(".git");
            std::cout << store.store("blob", content) << "\n";
        }
        else if (cmd == "cat-file") { //"-p" not implemented
            if (argc < 4) throw std::runtime_error("Usage: cat-file -p <sha>");
            ObjectStore store(".git");
            std::string type, content;
            if (!store.load(argv[3], type, content))
                throw std::runtime_error("Object not found");

            if (type != "tree") {
                std::cout << content;
                return 0;
            }

            // Pretty-print tree entries
            size_t i = 0;
            while (i < content.size()) {
                // Parse mode
                size_t sp = content.find(' ', i);
                std::string mode = content.substr(i, sp - i);

                // Parse name
                size_t nul = content.find('\0', sp + 1);
                std::string name = content.substr(sp + 1, nul - sp - 1);

                // Parse 20-byte binary SHA -> hex
                std::string sha_bin = content.substr(nul + 1, 20);
                std::ostringstream hex;
                for (unsigned char c : sha_bin)
                    hex << std::hex << std::setw(2) << std::setfill('0') << (int)c;

                std::string entry_type = (mode == "40000") ? "tree" : "blob";
                std::cout << mode << " " << entry_type << " " << hex.str() << "    " << name << "\n";

                i = nul + 1 + 20;
            }
        }
        else if (cmd == "write-tree") {
            ObjectStore store(".git");
            std::cout << write_tree(store, ".") << "\n";
//in the current version there is no garbage collection, so we are not maintaining and deleting away dangling trees

        }
        else if (cmd == "commit-tree") {
            if (argc < 5) throw std::runtime_error("Usage: commit-tree <tree> -m <msg> [-p <parent>]");
            std::string tree_sha = argv[2];
            std::vector<std::string> parents;
            std::string message;
            for (int i = 3; i < argc; ++i) {
                std::string flag = argv[i];
                if (flag == "-p") {
                    if (++i >= argc) throw std::runtime_error("-p requires an argument");
                    parents.push_back(argv[i]);
                } else if (flag == "-m") {
                    if (++i >= argc) throw std::runtime_error("-m requires an argument");
                    message = argv[i];
                } else {
                    throw std::runtime_error("Unknown flag: " + flag);
                }
            }
            if (message.empty()) throw std::runtime_error("-m <message> is required");
            ObjectStore store(".git");
            std::cout << commit_tree(store, tree_sha, parents, message) << "\n";
        }
        else if (cmd == "mktag") {
            if (argc < 5) throw std::runtime_error("Usage: mktag <object-sha> <tagname> <message>");
            ObjectStore store(".git");
            std::cout << mktag(store, argv[2], argv[3], argv[4]) << "\n";
        }
        else if (cmd == "log") {
            if (argc < 3) throw std::runtime_error("Usage: log <commit-sha>");
            ObjectStore store(".git");
            log_commits(store, argv[2]);
        }
        else if (cmd == "branch"){
            std::vector<std::string> args(argv+2,argv+argc);
            branch(args);
        }
        else if (cmd == "checkout"){
            if (argc < 3) throw std::runtime_error("Usage: checkout <branch>");
            std::string target = argv[2];
            GitLite::checkout(target);
        }
        else {
            throw std::runtime_error("Unknown command: " + cmd);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}