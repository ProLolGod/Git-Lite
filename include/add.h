#ifndef ADD_H
#define ADD_H

#include <string>
#include <vector>

struct IndexEntry {
    std::string mode;
    std::string sha;
    std::string path;
};

// Read current index from .git/index
std::vector<IndexEntry> readIndex();

// Write entries back to .git/index (atomic)
void writeIndex(const std::vector<IndexEntry>& entries);


// Main add command - pass "." for everything
void add(const std::string& filepath);

#endif // ADD_H