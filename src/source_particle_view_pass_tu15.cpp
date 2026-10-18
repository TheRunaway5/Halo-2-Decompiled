#include "halo2/source_particle_view_pass_tu15.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
SourceParticleViewPassTu15 source_particle_view_pass_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t mode,std::uint32_t distance_bits) {
    constexpr std::uint32_t counter=0x4c685c;
    std::array<std::byte,4> raw;memory.read(counter,raw);
    const auto serial=Bytes(raw).u32(0)+std::uint32_t{1};
    memory.write(counter,std::as_bytes(std::span(&serial,1)));
    return {{serial,mode,distance_bits}};
}
}
