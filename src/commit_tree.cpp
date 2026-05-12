#include "commit_tree.h"
#include <ctime>
#include <sstream>
#include <stdexcept>

static std::string format_author_line(const std::string& label) {
    // Hardcoded identity — swap for config lookup if needed
    const std::string name  = "git-lite";
    const std::string email = "git-lite@local";

    std::time_t now = std::time(nullptr);
    // Format: "author Name <email> <unix-timestamp> +0000"
    return label + " " + name + " <" + email + "> "
           + std::to_string(now) + " +0000";
}


std::string commit_tree(
    ObjectStore& store,
    const std::string& tree_sha,
    const std::vector<std::string>& parents,
    const std::string& message)
{
    if (tree_sha.size() != 40)
        throw std::runtime_error("Invalid tree SHA: " + tree_sha);

    std::ostringstream body;
    body << "tree " << tree_sha << "\n";

    for (const auto& p : parents) {
        if (p.size() != 40)
            throw std::runtime_error("Invalid parent SHA: " + p);
        body << "parent " << p << "\n";
    }

    body << format_author_line("author")    << "\n";
    body << format_author_line("committer") << "\n";
    body << "\n";
    body << message << "\n";

    return store.store("commit", body.str());
}