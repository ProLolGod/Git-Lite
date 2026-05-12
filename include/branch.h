#ifndef BRANCH_H
#define BRANCH_H

#include <string>
#include <vector>

// Create a new branch pointing to current HEAD
void branch_create(const std::string& name);

// List all branches, mark current with *
void branch_list();

// Main entry point
void branch(const std::vector<std::string>& args);

// Helper: Get current branch name from HEAD (empty if detached)
std::string get_current_branch();

#endif // BRANCH_H