#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace halo2 {
// Actual220C60 call arguments and its complete68-byte caller location,
// copied before the unchanged original call. arguments[4] is historical
// source backing, never a pointer for a native consumer to dereference.
struct SourceLoopSoundRequestTu15 {
    std::array<std::uint32_t,8> arguments;
    std::array<std::byte,68> location;
    // The attached source-loop record owns this identity. arguments[3]
    // is the synchronization owner and can be NONE for a spatial emitter.
    std::uint32_t emitter_owner=0xffffffffu;
};
}
