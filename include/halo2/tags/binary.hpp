#pragma once
#include "halo2/tags/types.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace halo2 {
class Bytes;
struct TagBlockRange { std::size_t count{}, offset{}; };
TagBlock read_tag_block_descriptor(const Bytes&, std::size_t descriptor);

// Borrowed serialized allocation used by group decoders and original binary
// adapters. Source pointers stay 32-bit; offsets address only this allocation.
class TagAddressSpace {
public:
    TagAddressSpace(const Bytes&, std::int64_t address_magic);
    std::size_t offset(std::uint32_t source_address, std::size_t size) const;
    TagBlockRange block(std::size_t descriptor, std::size_t stride,
                        std::size_t maximum_count = 65536) const;
private:
    std::span<const std::byte> bytes_;
    std::int64_t magic_;
};
} // namespace halo2
