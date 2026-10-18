#pragma once
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>
namespace halo2_test {
inline std::vector<std::byte> read_cairo_fixture(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream)throw std::runtime_error("Missing raw MCC Cairo fixture: "+path.string());
    const auto size=stream.tellg();
    if(size<0 || size>4*1024*1024)throw std::runtime_error("Invalid raw Cairo fixture extent");
    std::vector<std::byte> result(static_cast<std::size_t>(size));stream.seekg(0);
    if(!result.empty() && !stream.read(reinterpret_cast<char*>(result.data()),result.size()))
        throw std::runtime_error("Short raw Cairo fixture read");
    return result;
}
} // namespace halo2_test
