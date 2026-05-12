#include "utils.h"
#include <openssl/sha.h>
#include <zlib.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

std::string sha1_hex(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    std::ostringstream oss;
    for (auto byte : hash)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    return oss.str();
}

std::string zlib_compress(const std::string& data) {
    uLongf bound = compressBound(data.size());
    std::string out(bound, '\0');
    if (compress(reinterpret_cast<Bytef*>(out.data()), &bound,
                 reinterpret_cast<const Bytef*>(data.data()), data.size()) != Z_OK)
        throw std::runtime_error("zlib compression failed");
    out.resize(bound);
    return out;
}

std::string zlib_decompress(const std::string& data) {
    std::string out(1024 * 1024, '\0');
    uLongf out_size = out.size();
    if (uncompress(reinterpret_cast<Bytef*>(out.data()), &out_size,
                   reinterpret_cast<const Bytef*>(data.data()), data.size()) != Z_OK)
        throw std::runtime_error("zlib decompression failed");
    out.resize(out_size);
    return out;
}