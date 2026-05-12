#pragma once
#include <string>

std::string sha1_hex(const std::string& data);
std::string zlib_compress(const std::string& data);
std::string zlib_decompress(const std::string& data);