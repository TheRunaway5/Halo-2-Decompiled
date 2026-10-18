#pragma once
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
struct SourceSoundCacheIoTu15Services {
    std::function<std::vector<std::byte>(std::uint32_t encoded_offset,std::uint32_t bytes)> read;
    std::function<std::uint32_t(std::uint32_t descriptor,std::uint32_t destination,std::uint32_t bytes)> retain_completed_read;
};
// Exact21F4BD ->1A3B70 sound-cache request, after original allocation/binding.
// This uses the world's synchronous native resource policy: actual bound file
// bytes and an actual retained native receipt precede the caller's done byte.
// It supplies no Xbox queue/kernel result, source worker timing or audio output.
// Other callers remain unhandled. A malformed frame or missing/failing owner
// throws before completion; original partial memory writes are not rolled back.
bool source_try_sound_cache_io_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRuntimeCall &,SourcePhysicsWorldRuntimeAbi &,
    const SourceSoundCacheIoTu15Services &);
}
