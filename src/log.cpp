#include "log.h"
#include <iostream>
#include <vector>
#include <stdexcept>

// static std::string parse_field(const std::string& body, const std::string& field) {
//     std::string prefix = field + " ";
//     size_t pos = body.find(prefix);
//     if (pos == std::string::npos) return "";
//     size_t end = body.find('\n', pos);
//     return body.substr(pos + prefix.size(), end - pos - prefix.size());
// }

static std::string parse_field(const std::string& body, const std::string& field){
    std::string prefix = field + " ";
    size_t pos = body.find(prefix);
    if (pos == std::string::npos) return "";
    size_t end = body.find('\n', pos);
    return body.substr(pos + prefix.size(), end-pos-prefix.size());
}

static std::vector<std::string> parse_parents(const std::string& body) {
    std::vector<std::string> parents;
    size_t pos = 0;
    while (body.find(pos = body.find("parent ", pos)) != std::string::npos) {
        size_t end = body.find('\n', pos);
        parents.push_back(body.substr(pos + 7, end - pos - 7));
        pos = end;
    }
    return parents;
}

static std::string parse_message(const std::string& body) {
    size_t blank = body.find("\n\n");
    if (blank == std::string::npos) return "";
    return body.substr(blank + 2);
}

void log_commits(ObjectStore& store, const std::string& start_sha) {
    std::string sha = start_sha;
    while (!sha.empty()) {
        std::string type, content;
        if (!store.load(sha, type, content))
            throw std::runtime_error("Object not found: " + sha);
        if (type != "commit")
            throw std::runtime_error("Not a commit: " + sha);

        std::cout << "commit " << sha << "\n";
        std::cout << "Author: " << parse_field(content, "author") << "\n";
        std::cout << "\n    " << parse_message(content) << "\n";

        auto parents = parse_parents(content);
        sha = parents.empty() ? "" : parents[0];
    }
}