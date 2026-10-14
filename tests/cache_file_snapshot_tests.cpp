#include "halo2/cache_file_snapshot.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using namespace halo2;
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
int main() try {
#if !defined(__linux__)
    return 77;
#endif
    const auto path = std::filesystem::temp_directory_path() /
        ("halo2-file-snapshot-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code error; std::filesystem::remove(path,error); } } cleanup{path};
    std::vector<std::byte> expected(128 * 1024);
    for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = std::byte((i * 13 + i / 4096) & 255);
    { std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char *>(expected.data()), expected.size()); }
    auto first = CacheFileSnapshot::try_open(path), second = CacheFileSnapshot::try_open(path);
    require(first && second, "Snapshot backing unavailable");
    require(std::equal(first->bytes().begin(), first->bytes().end(), expected.begin(), expected.end()), "Snapshot bytes differ");
    first->mutable_bytes()[17] ^= std::byte{255};
    require(second->bytes()[17] == expected[17], "Private writes reached another snapshot");
    { std::ifstream file(path, std::ios::binary); file.seekg(17); require(std::byte(file.get()) == expected[17], "Private writes reached source"); }
    // An unread page must remain the original snapshot after source mutation.
    { std::ofstream file(path, std::ios::binary | std::ios::trunc); file.put('X'); }
    require(second->bytes().size() == expected.size() && second->bytes().back() == expected.back(),
            "Source truncation changed snapshot backing");
    std::filesystem::remove(path);
    require(second->bytes()[65537] == expected[65537], "Source unlink invalidated snapshot");
    auto retained = second; second.reset();
    require(retained->bytes()[65538] == expected[65538], "Retained snapshot lost its lifetime");
    require(!CacheFileSnapshot::try_open(path), "Missing source was accepted");
    std::cout << "PASS private cache file snapshot: bytes, private writes, source truncation/unlink, retained lifetime, missing input\n";
    return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
