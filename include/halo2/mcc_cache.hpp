#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace halo2 {
// Reclaimer's 896-byte v13 header and signed compressed chunk table. This
// produces an owned logical file, retaining original VA/resource offsets.
std::vector<std::byte> decode_mcc_cache(std::span<const std::byte>);
std::vector<std::byte> read_mcc_cache(const std::filesystem::path&);
// Bitmap partitions always address textures.dat, independently of the two
// cache-selector bits. Positive segments are zlib; negative segments are raw.
std::vector<std::byte> read_mcc_texture_partition(const std::filesystem::path&,
    std::uint32_t pointer,std::uint32_t stored_size);
}
