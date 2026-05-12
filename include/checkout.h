#ifndef CHECKOUT_H
#define CHECKOUT_H

#include <string>
#include <vector>

namespace GitLite {
    // Main checkout function - ADD THIS LINE
    void checkout(const std::string& target);
    
    void updateHEAD(const std::string& target);
    std::string readHEAD();
    std::string resolveRef(const std::string &ref_path);
    std::string extractTreeFromCommit(const std::string &commit_data);
    void restoreTree(const std::string &tree_sha, const std::string &base_path = ".");
    void restoreBlob(const std::string &blob_sha, const std::string &filepath);
    
    struct TreeEntry{
        std::string mode;
        std::string name;
        std::string sha;
    };
    
    std::vector<TreeEntry> parseTree(const std::string& tree_data);
    std::string binaryToHex(const std::string& binary);
}

#endif // CHECKOUT_H