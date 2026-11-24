#pragma once
// Native TU1.5 animation change, entered at 111B10 (physics live support and
// AI actor units): a unit's animation changed on request.
//
//   animation_change::request   111B10(EAX = flags; unit, mode, a, b, name, c, keep) -> AL, RET 0x1C
//
// The unit's animation block (at the int16 offset +0x12A) has a current
// name (+0x7C) and a field (+0x70, kept in the frame); its second block is
// at the int16 offset +0x33E. With a current name, the request is flagged
// 0x20 when the name is the current one or unset (7000101), else 0x40 when
// the name's partner (the four pairs at 474BE4) is the current one; a unit
// with a player (+0x13C) outside a cutscene (byte [51DAF8] clear) is
// flagged 0x200. An unset mode with an unset name is taken as it is (the
// answer 1); otherwise 1D5320 (EAX the name, ECX the block; the mode, b, a,
// the flags and 0x3F pushed) answers, and a refusal ends it. A new mode (not
// the block's field, or unset) or keep sets the request's switch (in the
// mode's argument slot), and a new name (or unset) or keep the change:
// with the switch 115E20 and 1D4350 (EAX the unit) decide bit 1 of the
// second block's byte; then 115E90 (EAX the name, ECX the current name, DL
// the switch; 0 and c pushed) gives the blend in XMM0 (kept in keep's slot):
// above 0 and with bit 4 of the block's +0x6C clear BC380 (ESI the unit)
// takes it; 115AF0 then takes the unit.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the block's field
// in the slot of the ECX it pushes, the switch and blend in argument slots);
// the boundary adapter (native_tu15_animation_change.hpp) maps the role to
// the original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept, refreshed
// from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_animation_change {
using native_object_update::at;
using native_object_update::none;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,partners=0x474be4u,cutscene=0x51daf8u,zero=0x46a0e8u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,partner_count=4u,partner_size=8u,request_mask=0x3fu,
    unset=0x7000101u;
namespace unit {inline constexpr std::uint32_t block=0x12au,player=0x13cu,second_block=0x33eu;}
namespace block {inline constexpr std::uint32_t field=0x70u,name=0x7cu,switches=0x6cu;inline constexpr std::uint32_t still_bit=4u;}
namespace flags {inline constexpr std::uint32_t same=0x20u,partner=0x40u,player=0x200u;inline constexpr std::uint32_t switched=2u;}
namespace argument {inline constexpr std::uint32_t unit=0u,mode=1u,a=2u,b=3u,name=4u,c=5u,keep=6u;}
// The frame's slot, by offset from the return address: the block's field, in the pushed ECX.
namespace local {inline constexpr std::int32_t field=-4;}

namespace animation_change {
enum class Access {headers,datums,unit_argument,object_address,name_argument,block_offset,current,field,second_offset,field_store,
    partner,partner_name,player,cutscene,mode_argument,b_argument,a_argument,mode_again,name_again,field_load,keep_argument,
    switch_clear,switch_set,keep_again,switch_load,change_unit,second_flags,second_store,name_final,c_argument,switch_byte,zero,
    blend_store,still,blend_load,blend_unit,finish_unit};
enum class Outcome {refused,unchanged,changed};
// The original's EBX (the flags, then the answer in BL), EBP (the current name), ESI (the block) and EDI (the
// second block) across the children.
struct Kept {std::uint32_t flags,current,block,second;};
// Frame: argument(k), local(offset), saving(n) and the hooks below each name the original instructions they stand for.
template<class Memory,class Frame> std::uint32_t request(Memory &memory,Frame &frame,std::uint32_t flags_in,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    frame.saving(0u);
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.saving(1u);
    kept.flags=flags_in;
    const auto unit_handle=memory.load32(unit_argument,frame.argument(argument::unit));
    frame.indexing(unit_handle);
    const auto unit_address=memory.load32(object_address,datums_address+header_size*(unit_handle&0xffffu)+header_address);
    auto name=memory.load32(name_argument,frame.argument(argument::name));
    frame.saving(2u);
    const auto offset=widen16(memory.load16(block_offset,at(unit_address,unit::block)));
    kept.current=memory.load32(current,offset+unit_address+block::name);
    const auto field_value=memory.load32(field,offset+unit_address+block::field);
    frame.saving(3u);
    const auto second_block_offset=widen16(memory.load16(Access::second_offset,at(unit_address,unit::second_block)));
    kept.block=frame.placed(offset,unit_address,0u);
    kept.second=frame.placed(second_block_offset,unit_address,1u);
    frame.compared(kept.current,none);
    memory.store32(field_store,frame.local(local::field),field_value);
    // The request against the current name: the same (or unset), or a partner's.
    if(kept.current!=none) {
        frame.compared(kept.current,name);
        bool same=kept.current==name;
        if(!same){frame.compared(name,unset);same=name==unset;}
        if(same)kept.flags=frame.flagged(kept.flags,flags::same);
        else {
            std::uint32_t partner_value=none;
            frame.clear_ecx();
            for(std::uint32_t k=0;;) {
                const auto candidate=memory.load32(partner,globals::partners+partner_size*k);
                frame.compared(name,candidate);
                if(name==candidate){partner_value=memory.load32(partner_name,globals::partners+4u+partner_size*k);frame.partnered(partner_value);break;}
                ++k;
                frame.counted(k);
                if(!(std::int32_t(k)<std::int32_t(partner_count))){frame.no_partner();break;}
            }
            frame.compared(kept.current,partner_value);
            if(kept.current==partner_value)kept.flags=frame.flagged(kept.flags,flags::partner);
        }
    }
    const auto player_value=memory.load32(player,at(unit_address,unit::player));
    frame.compared(player_value,none);
    if(player_value!=none) {
        const auto in_cutscene=memory.load8(cutscene,globals::cutscene);
        frame.tested8(in_cutscene);
        if(!in_cutscene)kept.flags=frame.flagged(kept.flags,flags::player);
    }
    // The mode: an unset mode with an unset name is taken as it is; otherwise 1D5320 decides.
    auto mode=memory.load32(mode_argument,frame.argument(argument::mode));
    frame.compared(mode,unset);
    bool taken=mode==unset;
    if(taken){frame.compared(name,mode);taken=name==mode;}
    if(taken)kept.flags=frame.answered(kept.flags,1u);
    else {
        const auto b=memory.load32(b_argument,frame.argument(argument::b));
        frame.pushing_request(b,kept);
        const auto a=memory.load32(a_argument,frame.argument(argument::a));
        if(!(frame.decide(a,mode,name,kept)&0xffu)){outcome=Outcome::refused;frame.restoring();return kept.flags;}
        mode=memory.load32(mode_again,frame.argument(argument::mode));
        name=memory.load32(name_again,frame.argument(argument::name));
    }
    // The switch: a new mode or keep; the change: a new name or keep.
    const auto field_now=memory.load32(field_load,frame.local(local::field));
    frame.compared(field_now,mode);
    bool switched=true;
    if(field_now==mode || (frame.compared(mode,unset),mode==unset)) {
        const auto keep=memory.load8(keep_argument,frame.argument(argument::keep));
        frame.tested8(keep);
        memory.store8(switch_clear,frame.argument(argument::mode),0u);
        switched=keep!=0u;
    }
    if(switched)memory.store8(switch_set,frame.argument(argument::mode),1u);
    frame.compared(kept.current,name);
    bool change=true;
    if(kept.current==name || (frame.compared(name,unset),name==unset)) {
        const auto keep=memory.load8(keep_again,frame.argument(argument::keep));
        frame.tested8(keep);
        change=keep!=0u;
    }
    frame.changed(change);
    const auto switch_flag=memory.load8(switch_load,frame.argument(argument::mode));
    frame.tested8(switch_flag);
    if(switch_flag) {
        frame.switching(memory.load32(change_unit,frame.argument(argument::unit)),kept);
        const bool on=frame.switch_on(kept)&0xffu;
        name=memory.load32(name_final,frame.argument(argument::name));
        const auto byte=memory.load8(second_flags,kept.second);
        memory.store8(second_store,kept.second,frame.switched_byte(byte,on));
    } else {
        frame.tested8(change?1u:0u);
        if(!change){outcome=Outcome::unchanged;frame.restoring();return kept.flags;}
    }
    // The blend: kept in keep's slot; above 0 and still-free BC380 takes it; 115AF0 takes the unit.
    const auto c=memory.load32(c_argument,frame.argument(argument::c));
    frame.pushing_blend(c,name);
    const auto switch_byte_value=memory.load8(switch_byte,frame.argument(argument::mode));
    const auto blend=frame.blend(switch_byte_value,kept);
    const auto zero_value=memory.load32(zero,globals::zero);
    frame.compared_float(blend,zero_value);
    memory.store32(blend_store,frame.argument(argument::keep),blend);
    if(!sse::not_above(blend,zero_value)) {
        const auto switches=memory.load8(still,at(kept.block,block::switches));
        if(!frame.still(switches)) {
            const auto stored=memory.load32(blend_load,frame.argument(argument::keep));
            frame.take_blend(memory.load32(blend_unit,frame.argument(argument::unit)),stored,kept);
        }
    }
    frame.finish(memory.load32(finish_unit,frame.argument(argument::unit)),kept);
    outcome=Outcome::changed;
    frame.restoring();
    return kept.flags;
}
} // namespace animation_change
} // namespace halo2::native_animation_change
