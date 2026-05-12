#pragma once
#include "ObjectStore.h"
#include <string>

std::string mktag(ObjectStore& store, const std::string& object_sha,
                  const std::string& tag_name, const std::string& message);