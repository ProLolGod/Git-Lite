#ifndef STATUS_H
#define STATUS_H

#include <string>
#include <map>

// Flatten a tree object recursively into map<path, sha>
std::map<std::string, std::string> flatten_tree(const std::string& tree_sha, const std::string& prefix = "");

// Main status command
void status();

#endif // STATUS_H