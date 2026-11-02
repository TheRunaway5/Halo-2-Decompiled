#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace halo2 {
// Actual2E33F0/2E3350 over the current2E11E0 registry. This is an owning
// memory query, never a supplied missing-lock or release result. The bounded
// native admission covers constructed power-of-two tables through65536 slots.
inline bool native_tu15_base_lock_registered(const SourcePhysicsContactMemory &memory,
    std::uint32_t object) {
    const auto word=[&](std::uint32_t address) {
        std::array<std::byte,4> raw{};memory.read(address,raw);
        std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    };
    const auto registry=word(0x48c93cu)+8u;
    const auto mask=word(registry+8u),keys=word(registry);
    if(mask>0xffffu || (mask&(mask+1u)))
        throw std::runtime_error("Native TU1.5 base lock requires a qualified registry extent");
    auto slot=((object>>4u)*0x9e3779b1u)&mask;
    for(std::uint32_t count=0;;++count) {
        const auto key=word(keys+slot*4u);
        if(!key){slot=mask+1u;break;}
        if(key==object)break;
        if(count==mask)
            throw std::runtime_error("Native TU1.5 base lock registry has no terminating slot");
        slot=(slot+1u)&mask;
    }
    // Actual2E3350 rereads the signed limit after the lookup, without loading
    // a value or inventing an allocator result for an absent key.
    return std::bit_cast<std::int32_t>(slot)<=std::bit_cast<std::int32_t>(word(registry+8u));
}

// Complete actual2E12A0(flags=0) when its current registry lookup is absent.
// A registered key needs the separately unqualified registry/storage owners.
inline void native_tu15_release_unregistered_base_lock(SourcePhysicsContactMemory &memory,
    std::uint32_t object,const SourcePhysicsContactRecordObserver &observer={}) {
    if(native_tu15_base_lock_registered(memory,object))
        throw std::runtime_error("Native TU1.5 registered base lock requires its registry/storage release owner");
    std::array<std::byte,1> before{};memory.read(object+5u,before);
    const std::array written{before[0]&std::byte{0x7f}};
    memory.write(object+5u,written);
    if(observer)observer({0x2e12c2u,object+5u,{before.begin(),before.end()},
        {written.begin(),written.end()}});
}
} // namespace halo2
