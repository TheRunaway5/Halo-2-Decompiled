#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace halo2 {
// Owned220C60-shaped native playback input and its complete68-byte location.
// Source observers copy the actual caller before its unchanged original call.
// A peer's ordinary static-loop selector derives presentation intent from a
// completed pre-view emitter and its own real BSP/cameras; this supplies no
// source call, admission return or voice. arguments[4] is historical source
// backing (NONE for peer intent), never a native-consumer pointer.
struct SourceLoopSoundRequestTu15 {
    std::array<std::uint32_t,8> arguments;
    std::array<std::byte,68> location;
    // The attached source-loop record owns this identity. arguments[3]
    // is the synchronization owner and can be NONE for a spatial emitter.
    std::uint32_t emitter_owner=0xffffffffu;
};
}
