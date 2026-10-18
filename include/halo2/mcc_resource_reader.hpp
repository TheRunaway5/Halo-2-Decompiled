#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace halo2 {
// An owned range of the logical v13 file, using its existing full decoder.
// Linux retains one immutable decoded container per thread. Each request
// admits an opened file identity; changed/replaced sources cannot reuse it.
// Other platforms preserve the uncached full-decoder behavior.
std::vector<std::byte> read_mcc_resource_range(const std::filesystem::path&,
    std::uint64_t offset,std::uint32_t bytes);
}
