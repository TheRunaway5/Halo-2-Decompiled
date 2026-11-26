#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>

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

inline void source_sound_location_store_tu15(std::array<std::byte,68>& location,
    std::size_t offset,float value){
    const auto word=std::bit_cast<std::uint32_t>(value);
    for(unsigned byte=0;byte<4;++byte)location[offset+byte]=std::byte(word>>(byte*8));
}

// 224590/224244..224342, for an lsnd attached to a playback-channel parent.
// The caller supplies resolved distance bounds when location flag 0x100 needs
// them. The scheduler owns the real parent identity separately from the ABI.
inline SourceLoopSoundRequestTu15 source_sound_loop_companion_request_tu15(
    std::uint32_t sound,std::uint32_t source_flags,std::uint32_t source_parent,
    std::uint32_t synchronization,std::array<std::byte,68> location,
    std::uint32_t loop_flags,float gain_db,std::uint32_t component_flags,
    unsigned phase,bool interrupted,std::optional<std::array<float,2>> bounds={}){
    if(bounds){
        location[0]|=std::byte{0xc};location[1]|=std::byte{2};
        source_sound_location_store_tu15(location,0x34,(*bounds)[0]);
        source_sound_location_store_tu15(location,0x38,(*bounds)[1]);
    }
    std::uint32_t gain{};
    for(unsigned byte=0;byte<4;++byte)gain|=std::to_integer<std::uint32_t>(location[8+byte])<<(byte*8);
    source_sound_location_store_tu15(location,8,gain_db+std::bit_cast<float>(gain));
    if(loop_flags&8u)source_flags&=~1u;
    if(loop_flags&0x40u)source_flags&=~2u;
    const bool alternate=(component_flags&8u) || (phase==2 && interrupted && (component_flags&0x10u));
    const auto flags=(phase==2?2u:0u)|unsigned(alternate);
    return {{sound,source_flags,source_parent,synchronization,0xffffffffu,phase,flags,0u},location};
}
}
