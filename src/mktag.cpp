#include "mktag.h"
#include <ctime>
#include <sstream>
#include <stdexcept>

std::string mktag(ObjectStore& store, const std::string& object_sha,
                  const std::string& tag_name, const std::string& message) {
    if (object_sha.size() != 40)
        throw std::runtime_error("Invalid object SHA: " + object_sha);

    std::time_t now = std::time(nullptr);
    std::ostringstream body;
    body << "object " << object_sha << "\n";
    body << "type commit\n";
    body << "tag " << tag_name << "\n";
    body << "tagger git-lite <git-lite@local> " << now << " +0000\n";
    body << "\n";
    body << message << "\n";

    return store.store("tag", body.str());
}