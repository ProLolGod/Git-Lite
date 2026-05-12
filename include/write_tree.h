#pragma once
#include "ObjectStore.h"
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string write_tree(ObjectStore& store, const fs::path& dir);