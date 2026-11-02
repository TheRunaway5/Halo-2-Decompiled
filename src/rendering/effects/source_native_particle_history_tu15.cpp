#include "halo2/source_native_particle_history_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <limits>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {
    return Bytes(memory.view(address,4)).u32(0);
}
std::uint32_t offset(std::uint32_t address,std::uint32_t bytes) {
    if(!address || address>std::numeric_limits<std::uint32_t>::max()-bytes)
        throw FormatError("Native particle history has an invalid source address");
    return address+bytes;
}
struct Patch {std::uint32_t address;std::vector<std::byte> bytes;};
}
void source_stage_native_particle_history_tu15(const SourceRuntimeMemory &current,
    SourceRuntimeMemory &replacement) {
    if(&current==&replacement)
        throw FormatError("Native particle history needs a distinct prepared bank");
    const auto old_game=word(current,0x4f3218u),new_game=word(replacement,0x4f3218u);
    const auto old_clock=word(current,0x51df54u),new_clock=word(replacement,0x51df54u);
    if(word(current,offset(old_game,8))==1u ||
       word(current,offset(old_game,8))!=word(replacement,offset(new_game,8)) ||
       word(current,offset(old_game,0x18))!=word(replacement,offset(new_game,0x18)) ||
       word(current,offset(old_clock,8))!=word(replacement,offset(new_clock,8)))
        throw FormatError("Native particle history requires the same multiplayer session and gameplay tick");

    std::vector<Patch> patches;
    const auto retain=[&](std::uint32_t address,std::size_t bytes) {
        if(!address || bytes>(std::uint64_t{1}<<32) ||
           std::uint64_t(address)+bytes>(std::uint64_t{1}<<32))
            throw FormatError("Native particle history extent wraps its source address");
        // Validate every complete extent and save genuine executed bytes before
        // modifying the unpublished replacement. Aliased owners are rejected.
        replacement.validate_mutable_view(address,bytes);
        const auto value=current.view(address,bytes);
        for(const auto &patch:patches)
            if(std::uint64_t(address)<std::uint64_t(patch.address)+patch.bytes.size() &&
               std::uint64_t(patch.address)<std::uint64_t(address)+bytes)
                throw FormatError("Native particle history has overlapping storage owners");
        patches.push_back({address,{value.begin(),value.end()}});
    };
    // Constructor-owned TU1.5 effect/part, effect, system, location, emitter
    // and particle datum pools. Preserve native allocator state, salts, holes
    // and uncleared records; admission and retirement remain original calls.
    for(const auto [global,stride]:std::array<std::pair<std::uint32_t,unsigned>,6>{{
        {0x4f7208u,60u},{0x4f720cu,400u},{0x51df74u,84u},
        {0x52d71cu,52u},{0x52d718u,76u},{0x52d714u,64u}}}) {
        const auto pool=word(current,global);
        offset(pool,0x4c);
        const auto capacity=word(current,pool+0x20u);
        if(pool!=word(replacement,global) || !capacity || capacity>65536u ||
           word(current,pool+0x24u)!=stride ||
           !std::ranges::equal(current.view(pool,0x34u),replacement.view(pool,0x34u)) ||
           !std::ranges::equal(current.view(pool+0x42u,10u),replacement.view(pool+0x42u,10u)))
            throw FormatError("Native particle history changed its real pool layout or backing addresses");
        for(const auto field:{0x34u,0x38u,0x3cu})
            if(word(current,pool+field)>capacity || word(replacement,pool+field)>capacity)
                throw FormatError("Native particle history exceeds its allocator cursor/count bounds");
        retain(pool+0x34u,14u);
        retain(word(current,pool+0x44u),std::size_t(capacity)*stride);
        retain(word(current,pool+0x48u),(capacity+31u)/32u*4u);
    }
    const auto random=word(current,0x4f3cd8u);
    if(random!=word(replacement,0x4f3cd8u))
        throw FormatError("Native particle history changed its RNG backing address");
    retain(random,8u);
    for(const auto &patch:patches)replacement.write(patch.address,patch.bytes);
}
}
