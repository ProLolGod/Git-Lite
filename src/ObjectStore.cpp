#include "ObjectStore.h"
#include "utils.h"
#include <fstream>
#include <stdexcept>


// Initialize members in initializer list (not in body)
// → more efficient (no extra assignment)
// → necessary for const/ref members
ObjectStore::ObjectStore(const fs::path& git_dir) : git_dir_(git_dir) {}

fs::path ObjectStore::object_path(const std::string& sha) {
    return git_dir_ / "objects" / sha.substr(0, 2) / sha.substr(2);
}

bool ObjectStore::exists(const std::string& sha) {
    return fs::exists(object_path(sha)); // IO operation - can be slow, but we need it to avoid unnecessary writes and to check if an object exists when loading
}

std::string ObjectStore::store(const std::string& type, const std::string& content) {
    std::string header = type + " " + std::to_string(content.size()) + '\0'; // "type size\0"
    std::string full = header + content;    //blob 11\0hello world
    // 3 tasks-> identify the type, size for integrity check and begining of the content (metadata + type tag + integrity hint + part of identity)


    // sha is for deduplication and integrity check, not for addressing (we use filesystem paths for that), identity is determined by content, not by location
    std::string sha = sha1_hex(full);
    if (exists(sha)) return sha;

    // fs::path = just a path string wrapper, NOT a file
    fs::path path = object_path(sha);
    fs::create_directories(path.parent_path()); // creating all the parent directories if they dont exist, if they already exist it does nothing, so we can safely call it without checking if the directory exists


    // write → temp → rename  (safe write pattern)
    fs::path tmp = path.parent_path() / ("tmp_" + sha.substr(2));
    std::ofstream out(tmp, std::ios::binary); // Open temp file (creates it if not exists)
    if (!out) throw std::runtime_error("Failed to open temp file for writing");

    std::string compressed = zlib_compress(full);
    out.write(compressed.data(), compressed.size());
    out.close();

    // rename() changes actual file on disk, not variables
    fs::rename(tmp, path); // rename is guaranteed to be atomic on the same filesystem, so we won't end up with a half-written file even if the program crashes in the middle of writing
    // after rename there is no tmp file left only our current updated file remains 
    return sha;
}

bool ObjectStore::load(const std::string& sha, std::string& type, std::string& content) {
    fs::path path = object_path(sha);
    if (!fs::exists(path)) return false;

    std::ifstream in(path, std::ios::binary);
    std::string compressed((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

    std::string full = zlib_decompress(compressed);

    size_t null_pos = full.find('\0'); // returns the header if not present then this will be std::string::npos
    if (null_pos == std::string::npos)
        throw std::runtime_error("Corrupt object: no null byte in header");

    std::string header = full.substr(0, null_pos);
    size_t space_pos = header.find(' ');
    type = header.substr(0, space_pos);
    content = full.substr(null_pos + 1);
    return true;
}