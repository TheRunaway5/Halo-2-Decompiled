#pragma once

#include "halo2/cache.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {

struct SourceTextureCacheReadTu15 {
    std::uint32_t descriptor, partition, encoded_offset, destination, bytes;
};

// Complete the actual 1301B0 host boundary after 1300B0 has allocated its
// physical pages and tracking row. The reader owns the bytes. The receipt
// owner publishes the job before the tracking row becomes ready.
template<class Read, class Retain>
bool source_try_texture_cache_io_tu15(SourcePhysicsContactMemory& source, SourcePhysicsContactMemory& backing,
    const SourcePhysicsContactRuntimeCall& call, SourcePhysicsWorldRuntimeAbi& abi, Read&& read, Retain&& retain) {
    if (call.instruction != 0x1301b0u || call.target != 0x1a3b70u || call.tail) return false;

    const auto word = [&](std::uint32_t address) {
        std::array<std::byte, 4> value;
        source.read(address, value);
        return Bytes(value).u32(0);
    };
    const auto sp = abi.gp[4], tracking = abi.gp[6];
    const auto pool = word(0x4f2d24u), rows = word(pool + 0x44u);
    if (word(sp) != 0x1301b5u || tracking < rows || (tracking - rows) % 0x28u ||
        (tracking - rows) / 0x28u >= word(pool + 0x38u) ||
        word(sp + 12u) != 0 || word(sp + 16u) != tracking + 3u || word(sp + 20u) != 3u)
        throw FormatError("TU1.5 texture resource I/O differs from its actual source cache block");

    const SourceTextureCacheReadTu15 request{
        word(tracking + 0x10u), word(tracking + 4u), abi.gp[1], word(sp + 8u), word(sp + 4u),
    };
    if (request.partition >= 3u)
        throw FormatError("TU1.5 texture resource I/O has an invalid partition");
    const auto offset = request.partition * 4u;
    const auto size = word(request.descriptor + 0x34u + offset);
    if (!size || std::uint64_t(request.bytes) != ((std::uint64_t(size) + 511u) & ~std::uint64_t(511u)) ||
        request.encoded_offset != word(request.descriptor + 0x1cu + offset) ||
        request.destination != word(request.descriptor + 0x40u + offset))
        throw FormatError("TU1.5 texture resource I/O differs from its allocated descriptor");

    const auto payload = read(request);
    if (payload.size() != request.bytes)
        throw FormatError("TU1.5 texture resource owner returned an incomplete transfer");
    backing.write(request.destination, payload);
    const auto job = retain(request);
    constexpr std::array done{std::byte{1}};
    backing.write(tracking + 3u, done);
    abi.gp[0] = job;
    abi.gp[4] += 28u;
    return true;
}

}
