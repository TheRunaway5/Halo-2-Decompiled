#pragma once

#include "halo2/physics_contact_record.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>

namespace halo2 {

// Both Havok query consumers own their destination and read the rows before
// releasing the source arrays. Leave ordered scalar access to the caller
// whenever the full extent cannot be borrowed as ordinary source memory.
inline bool try_copy_native_cull_query_rows(const SourcePhysicsContactMemory& memory,
    std::uint32_t address, std::span<std::uint32_t> rows) {
    if (rows.empty()) return true;
    if constexpr (std::endian::native != std::endian::little) return false;
    static const bool reference = [] {
        const auto* flag = std::getenv("HALO2_REFERENCE_SCALAR_CULL_ROWS");
        return flag && *flag;
    }();
    if (reference || rows.size_bytes() > (std::uint64_t{1} << 32) - address) return false;
    const auto source = memory.try_read_view(address, rows.size_bytes());
    if (!source || source->size() != rows.size_bytes()) return false;
    std::memcpy(rows.data(), source->data(), rows.size_bytes());
    return true;
}

} // namespace halo2
