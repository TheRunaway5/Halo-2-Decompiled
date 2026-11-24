#pragma once
// Native TU1.5 light shape, entered at C3A20 (light shape, death cycle and
// map world support units): a light's shape record.
//
//   light_shape::fill   C3A20(EAX = light, EDI = record)
//
// A light (0x110 bytes of the lights at [4ECBDC]) whose marker (+8) names a
// structure light (0x6C-byte entries +0xEC of the structure at [4ECC20])
// flagged bit 0 (byte +0x3E) takes that entry's shape (C39B0, ESI the
// entry); any other takes its definition's (C3960, EAX the definition
// through the tag instances at 4F0414). Then a record of kind 1 has its +0xC
// zero, and one of kind 3 its +4.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter (native_tu15_light_shape_source.hpp)
// maps the role to the original instruction and owns the ABI of the children.
// The record the original keeps in EDI across them is a Kept, refreshed from
// the register each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_light_shape_source {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t lights=0x4ecbdcu,tag_instances=0x4f0414u,structure=0x4ecc20u;}
inline constexpr std::uint32_t datum_data=0x44u,light_size=0x110u,tag_size=16u,tag_data=8u,entry_size=0x6cu;
namespace light {inline constexpr std::uint32_t tag=4u,marker=8u;}
namespace structure {inline constexpr std::uint32_t lights=0xecu;}
namespace entry {inline constexpr std::uint32_t flags=0x3eu;inline constexpr std::uint32_t own_shape=1u;}
namespace kind {inline constexpr std::uint32_t far_cleared=1u,near_cleared=3u;}
namespace record {inline constexpr std::uint32_t near=4u,far=0xcu;}
inline constexpr std::uint32_t zero_bits=0u;

namespace light_shape {
enum class Access {lights,light_data,light_tag,instances,definition,marker,structure,entries,entry_flags,record_kind,near_store,far_store};
enum class Outcome {definition_kind1,definition_kind3,definition_other,entry_kind1,entry_kind3,entry_other};
// The original's EDI across the children: the record.
struct Kept {std::uint32_t record;};
// Frame:
//   from_definition(definition, kept)   C3960(EAX the definition, EDX the record), refreshes kept
//   from_entry(entry, kept)             C39B0(ESI the entry, EDX the record), refreshes kept
template<class Memory,class Frame> Outcome fill(Memory &memory,Frame &frame,std::uint32_t light_handle,std::uint32_t record_address) {
    using enum Access;
    const auto array=memory.load32(lights,globals::lights);
    const auto offset=light_size*(light_handle&0xffffu);
    const auto data=memory.load32(light_data,at(array,datum_data));
    const auto tag=memory.load32(light_tag,offset+data+light::tag);
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    const auto light_address=offset+data;
    const auto definition_address=memory.load32(definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
    const auto marker_value=memory.load32(marker,at(light_address,light::marker));
    Kept kept{record_address};
    bool own=false;
    std::uint32_t entry_address=0u;
    if(marker_value!=none) {
        const auto entries_address=memory.load32(entries,at(memory.load32(Access::structure,globals::structure),structure::lights));
        entry_address=entry_size*marker_value+entries_address;
        own=(memory.load8(entry_flags,at(entry_address,entry::flags))&entry::own_shape)!=0u;
    }
    if(own)frame.from_entry(entry_address,kept);
    else frame.from_definition(definition_address,kept);
    const auto kind_value=memory.load32(record_kind,kept.record);
    const unsigned base=own?3u:0u;
    if(kind_value==kind::far_cleared) {
        memory.store32(far_store,at(kept.record,record::far),zero_bits);
        return Outcome(base+0u);
    }
    if(kind_value==kind::near_cleared) {
        memory.store32(near_store,at(kept.record,record::near),zero_bits);
        return Outcome(base+1u);
    }
    return Outcome(base+2u);
}
} // namespace light_shape
} // namespace halo2::native_light_shape_source
