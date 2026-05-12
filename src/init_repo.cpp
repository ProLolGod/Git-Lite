#include <string>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

void init_repo(const std::string& path) {
    fs::create_directories(path + "/.git/objects");
    fs::create_directories(path + "/.git/refs/heads"); // create_directory function will not work if .git doesnt exist but directorIES function will create all the parent directories if they dont exist
    std::ofstream head(path + "/.git/HEAD");
    head << "ref: refs/heads/master\n";
}