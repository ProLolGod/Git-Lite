#pragma once
#include "ObjectStore.h"
#include <string>
#include <vector>

std::string commit_tree(
    ObjectStore& store,
    const std::string& tree_sha,
    const std::vector<std::string>& parents,
    const std::string& message
);

#pragma once
#include "ObjectStore.h"
#include <string>
#include <vector>

std::string commit_tree(ObjectStore& store, const std::string& tree_sha,
                        const std::vector<std::string>& parents,
                        const std::string& message);