#pragma once
#include <cstdint>

namespace halo2 {
// Inputs from the actual allocated21F4BD caller. The physical descriptor
// disambiguates resource owners whose encoded file offsets happen to match.
// Its runtime cache handle at+8 remains with the source cache allocator.
struct SourceSoundCacheReadTu15 {
    std::uint32_t descriptor{},encoded_offset{},size_flags{},bytes{};
};
}
