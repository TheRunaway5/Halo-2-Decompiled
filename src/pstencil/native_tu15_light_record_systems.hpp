#pragma once
// Native TU1.5 light record, entered at C3820 (light portal and map world
// support units): a light's record filled, and whether it lights anything.
//
//   light_record::prepare   C3820(EAX = record, EDX = light handle; split) -> AL, RET 4
//
// A light (0x110 bytes of the lights at [4ECBDC]) is let in unless the
// split argument is set, its definition (through the tag instances at
// 4F0414) lacks the always bit (bit 8 of its flags), the session's variant
// record exists ([56DFA0] by [4F63B8] +0xC14) and the session's flags carry
// bit 1. A light let in, while the game globals' byte +8 ([51DAE8]) is set
// and its own flags (byte +2) carry bit 1, has its shape filled (C3A20, EAX
// the handle, EDI the record) and its record completed (C3AB0, ECX the
// light's +0x84, EDX the record, the definition pushed). It lights when a
// record of kind 0 (word +0) has its intensity (+0x44) above the epsilon at
// 46A0EC; or one of any other kind has its intensity (+0x64) above it and
// its vector (+0x68, +0x6C) longer than the threshold at 46A610 (squared).
//
// The single-precision arithmetic is the original's scalar SSE, in its
// order, on the live MXCSR. This is the game logic only. The game's memory is
// the retained TU1.5 bank, reached through a Memory object with typed loads
// and stores; every access names its role (Access), and the boundary adapter
// (native_tu15_light_record.hpp) maps the role to the original instruction
// and owns the ABI of the children. The light, the record and the definition
// the original keeps in ESI, EDI and EBX across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_light_record {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {
inline constexpr std::uint32_t lights=0x4ecbdcu,tag_instances=0x4f0414u,session=0x4f63b8u,variants=0x56dfa0u,game=0x51dae8u;
inline constexpr std::uint32_t epsilon=0x46a0ecu,threshold=0x46a610u;   // floats
}
inline constexpr std::uint32_t datum_data=0x44u,light_size=0x110u,tag_size=16u,tag_data=8u;
namespace light {inline constexpr std::uint32_t flags=2u,tag=4u,shape_source=0x84u;inline constexpr std::uint32_t lit=2u;}
namespace definition {inline constexpr std::uint32_t always=0x100u;}
namespace session {inline constexpr std::uint32_t variant=0xc14u;inline constexpr std::uint32_t split_flag=2u;}
namespace game {inline constexpr std::uint32_t enabled=8u;}
namespace record {inline constexpr std::uint32_t kind=0u,intensity=0x44u,cone_intensity=0x64u,cone_x=0x68u,cone_y=0x6cu;}

namespace light_record {
enum class Access {lights_header,datums,instances,light_tag,definition_address,definition_flags,split_argument,session_address,
    variant_index,variant_entry,session_flags,game_address,game_enabled,light_flags,kind,intensity,epsilon,cone_intensity,
    cone_epsilon,cone_x,cone_y,threshold};
enum class Outcome {split_off,game_off,light_off,dim,lit,cone_dim,cone_short,cone_lit};
// The original's ESI (the light), EDI (the record) and EBX (the definition) across the children.
struct Kept {std::uint32_t light,record,definition;};
// Frame:
//   argument()                            the split argument's slot now
//   admitted(flags), session_flags(f, allowed), allowed_test(allowed)   the let-in register (CL) and its tests
//   saving(), restoring()                 EBP around the session's test
//   shape(handle, kept)                   C3A20(EAX = handle, EDI = the record), refreshes kept
//   complete(source, kept)                C3AB0(ECX = source, EDX = the record; the definition), refreshes kept
//   measured(y_squared, length)           the cone's products and sum in the original's registers
template<class Memory,class Frame> Outcome prepare(Memory &memory,Frame &frame,std::uint32_t record_address,std::uint32_t handle) {
    using enum Access;
    Kept kept{};
    kept.record=record_address;
    const auto datums_address=memory.load32(datums,at(memory.load32(lights_header,globals::lights),datum_data));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    kept.light=datums_address+light_size*(handle&0xffffu);
    const auto tag=memory.load32(light_tag,at(kept.light,light::tag));
    kept.definition=memory.load32(definition_address,tag_size*(tag&0xffffu)+instances_address+tag_data);
    const auto flags=memory.load32(definition_flags,kept.definition);
    bool allowed=true;
    frame.admitted(flags);
    if(!(flags&definition::always) && memory.load8(split_argument,frame.argument())) {
        frame.saving();
        const auto session_value=memory.load32(session_address,globals::session);
        const auto variant=memory.load32(variant_index,at(session_value,session::variant));
        if(memory.load32(variant_entry,globals::variants+4u*variant)!=0u) {
            const auto session_bits=memory.load32(session_flags,session_value);
            allowed=!(session_bits&session::split_flag);
            frame.session_flags(session_bits,allowed);
        }
        frame.restoring();
    }
    const auto game_value=memory.load32(game_address,globals::game);
    if(!memory.load8(game_enabled,at(game_value,game::enabled)))return Outcome::game_off;
    frame.allowed_test(allowed);
    if(!allowed)return Outcome::split_off;
    if(!(memory.load8(light_flags,at(kept.light,light::flags))&light::lit))return Outcome::light_off;
    frame.shape(handle,kept);
    frame.complete(at(kept.light,light::shape_source),kept);
    if(!memory.load16(kind,at(kept.record,record::kind))) {
        const auto value=memory.load32(intensity,at(kept.record,record::intensity));
        return sse::not_above(value,memory.load32(Access::epsilon,globals::epsilon))?Outcome::dim:Outcome::lit;
    }
    const auto cone=memory.load32(cone_intensity,at(kept.record,record::cone_intensity));
    if(sse::not_above(cone,memory.load32(cone_epsilon,globals::epsilon)))return Outcome::cone_dim;
    const auto x=memory.load32(cone_x,at(kept.record,record::cone_x));
    const auto y=memory.load32(cone_y,at(kept.record,record::cone_y));
    const auto x_squared=sse::multiply(x,x);
    const auto y_squared=sse::multiply(y,y);
    const auto length=sse::add(x_squared,y_squared);
    frame.measured(y_squared,length);
    return sse::not_above(length,memory.load32(Access::threshold,globals::threshold))?Outcome::cone_short:Outcome::cone_lit;
}
} // namespace light_record
} // namespace halo2::native_light_record
