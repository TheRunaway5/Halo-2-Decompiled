#pragma once
// Native TU1.5 unit animation update, entered at 1157D0 (physics live
// support unit).
//
//   unit_animation_update::run   1157D0(unit, a, b) -> RET C
//
// Units whose definition (through the tag instances at 4F0414) carries bit
// 11 of its flags (+0xBC) are left. So is a unit whose animation state (the
// block at its int16 +0x12A) has no animation (+0x68 NONE), no graph (+0
// NONE) or no mode (word +6 0xFFFF). Otherwise: 113B70 answers whether the
// first blend is the constant at 450170 (else 46AA4C); 1139D0 answers the
// same for the second, unless CFB40 refuses the unit. Both blends go to
// 1E14F0 for the unit's animation channels (the block at its int16 +0x33E,
// +0x84 then +0x80; ECX the channel, the blend pushed). 116340 then updates
// the unit (EBX the unit; its +0x14, a, b). A unit with an animation index
// (int8 +0x212 not -1) whose state has both of its +0x74 and +0x78 is
// finished: CE230 (EAX the unit, CX the index; a, b) answers what 116280
// takes (with a, b), and the channels' first byte loses bit 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame and the argument
// slot it reuses; the boundary adapter (native_tu15_unit_animation_update.hpp)
// maps the role to the original instruction and owns the ABI of the children.
// The values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_animation_update {
using native_object_update::at;
using native_object_update::widen16;
using native_object_update::widen8;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u;
inline constexpr std::uint32_t first_blend=0x450170u,second_blend=0x46aa4cu;   // floats
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u;
namespace unit {inline constexpr std::uint32_t tag=0u,parent=0x14u,state=0x12au,index=0x212u,channels=0x33eu;}
namespace definition {inline constexpr std::uint32_t flags=0xbcu;inline constexpr std::uint32_t left_bit=11u;}
namespace state {inline constexpr std::uint32_t graph=0u,mode=6u,animation=0x68u,first=0x74u,second=0x78u;}
namespace channels {inline constexpr std::uint32_t second=0x80u,first=0x84u;inline constexpr std::uint8_t active=1u;}
// The frame's locals and the argument slot the original reuses, by offset from the return address.
namespace local {inline constexpr std::int32_t blend=-0xc,state=-8,index=-4,slot=4,a=8,b=0xc;}

namespace unit_animation_update {
enum class Access {headers,datums,unit_argument,unit_address,unit_tag,instances,channels_offset,definition,definition_flags,
    index_store,state_offset,state_store,animation,graph,mode,second_answer,refused_clear,first_constant,second_constant,
    blend_store,blend_other,blend_load,slot_answer,slot_blend_store,slot_blend_other,slot_blend_load,b_load,a_load,parent,
    index_byte,state_load,first_link,second_link,b_again,a_again,headers_again,datums_again,index_load,unit_again,index_value,
    channels_byte,channels_store};
enum class Outcome {left,no_animation,updated,finished};
// The original's EBX (the first answer), EBP (the channels), ESI (the unit's
// object) and EDI (the unit) across the children, and the two blends it keeps
// in XMM3 and XMM2 across the first 1E14F0.
struct Kept {std::uint32_t first,channels,object,unit,one,other;};
// Frame:
//   local(offset)                      a local or argument slot now
//   saving()                           EBX saved before the children
//   first_blend(kept) -> AL            113B70, refreshes kept (BL takes AL)
//   second_blend(kept) -> AL           1139D0, refreshes kept
//   refuses(kept) -> AL                CFB40(EAX the unit), refreshes kept
//   blend(channel, value, kept)        1E14F0(ECX the channel; the value pushed), refreshes kept
//   update(parent, a, b, kept)         116340(EBX the unit; parent, a, b), refreshes kept
//   finishing(b, a)                    CE230's arguments pushed (116280 takes them too)
//   finish_value(index, kept) -> EAX   CE230(EAX the unit, CX the index), refreshes kept
//   finish(value, kept)                116280(the value pushed), refreshes kept
//   restoring()                        EBX back
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame) {
    using enum Access;
    const auto headers_address=memory.load32(headers,globals::object_headers);
    const auto datums_address=memory.load32(datums,at(headers_address,datum_data));
    Kept kept{};
    kept.unit=memory.load32(unit_argument,frame.local(local::slot));
    const auto index_offset=header_size*(kept.unit&0xffffu);
    kept.object=memory.load32(unit_address,datums_address+index_offset+header_address);
    const auto tag=memory.load32(unit_tag,at(kept.object,unit::tag));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    kept.channels=widen16(memory.load16(channels_offset,at(kept.object,unit::channels)));
    const auto definition_address=memory.load32(definition,tag_size*(tag&0xffffu)+instances_address+tag_data);
    const auto flags=memory.load32(definition_flags,at(definition_address,definition::flags));
    memory.store32(index_store,frame.local(local::index),index_offset);
    const auto state_address=widen16(memory.load16(state_offset,at(kept.object,unit::state)))+kept.object;
    kept.channels+=kept.object;
    memory.store32(state_store,frame.local(local::state),state_address);
    if((flags>>definition::left_bit)&1u)return Outcome::left;
    if(memory.load32(animation,at(state_address,state::animation))==none ||
       memory.load32(graph,at(state_address,state::graph))==none ||
       (memory.load16(mode,at(state_address,state::mode))&0xffffu)==0xffffu)return Outcome::no_animation;
    frame.saving();
    kept.first=frame.first_blend(kept)&0xffu;
    memory.store8(second_answer,frame.local(local::slot),frame.second_blend(kept));
    if(frame.refuses(kept)&0xffu)memory.store8(refused_clear,frame.local(local::slot),0u);
    // The first blend, through the frame's local.
    kept.one=memory.load32(first_constant,globals::first_blend);
    kept.other=memory.load32(second_constant,globals::second_blend);
    memory.store32(blend_store,frame.local(local::blend),kept.one);
    if(!(kept.first&0xffu))memory.store32(blend_other,frame.local(local::blend),kept.other);
    frame.blend(at(kept.channels,channels::first),memory.load32(blend_load,frame.local(local::blend)),kept);
    // The second blend, through the argument's slot.
    const auto second=memory.load8(slot_answer,frame.local(local::slot))&0xffu;
    memory.store32(slot_blend_store,frame.local(local::slot),kept.one);
    if(!second)memory.store32(slot_blend_other,frame.local(local::slot),kept.other);
    frame.blend(at(kept.channels,channels::second),memory.load32(slot_blend_load,frame.local(local::slot)),kept);
    const auto b_value=memory.load32(b_load,frame.local(local::b));
    const auto a_value=memory.load32(a_load,frame.local(local::a));
    frame.update(memory.load32(parent,at(kept.object,unit::parent)),a_value,b_value,kept);
    auto outcome=Outcome::updated;
    if((memory.load8(index_byte,at(kept.object,unit::index))&0xffu)!=0xffu) {
        const auto state_again=memory.load32(state_load,frame.local(local::state));
        if(memory.load32(first_link,at(state_again,state::first))!=none &&
           memory.load32(second_link,at(state_again,state::second))!=none) {
            const auto b_again_value=memory.load32(b_again,frame.local(local::b));
            const auto a_again_value=memory.load32(a_again,frame.local(local::a));
            const auto headers_again_address=memory.load32(headers_again,globals::object_headers);
            frame.finishing(b_again_value,0u);
            const auto datums_again_address=memory.load32(datums_again,at(headers_again_address,datum_data));
            frame.finishing(a_again_value,1u);
            const auto object_again=memory.load32(unit_again,datums_again_address+memory.load32(index_load,frame.local(local::index))+header_address);
            const auto index_value_byte=widen8(memory.load8(index_value,at(object_again,unit::index)));
            frame.finish(frame.finish_value(index_value_byte,kept),kept);
            memory.store8(channels_store,kept.channels,memory.load8(channels_byte,kept.channels)&~std::uint32_t(channels::active));
            outcome=Outcome::finished;
        }
    }
    frame.restoring();
    return outcome;
}
} // namespace unit_animation_update
} // namespace halo2::native_unit_animation_update
