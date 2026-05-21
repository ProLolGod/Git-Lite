#ifndef DIFF_H
#define DIFF_H

#include <string>
#include <map>

// Diff two strings line by line, returns unified diff string
std::string diff_strings(const std::string& old_content, const std::string& new_content, const std::string& filename);

// Mode 1: working dir vs index
void diff_working();

// Mode 2: index vs HEAD
void diff_staged();

// Mode 3: commit vs commit
void diff_commits(const std::string& sha1, const std::string& sha2);

// Main entry point
void diff(int argc, char* argv[]);

#endif // DIFF_H