#pragma once
// Native TU1.5 channel fades, entered at C8390 (physics live support unit):
// an object's channel weights faded toward on or off, and whether any moved.
//
//   channel_fades::update   C8390(EAX = object) -> AL
//
// An object (through the object headers at [4ECBD0]) whose definition
// (through the tag instances at 4F0414) carries bit 11 of +0xBC answers 0.
// Otherwise D3150 (EBX the object) and D05A0 run; an object with bit 26 of
// +0x134, outside game kind 4 ([4F3218] +0xC), with a parent (+0x14) and a
// seat (word +0x1FC), loses that bit and E8E10 hears of it (ECX 0x1E, the
// object pushed). Then each of the definition's channels (a count +0x1B8,
// 8-byte channels at +0x1BC, a duration float +0) has a weight (+0x25C on):
// channel 0 is on with a target (+0x248 not NONE) or bit 1 of +0x134,
// channel 1 with a second target (+0x24C not NONE) that differs from the
// first, the others off; a held object (bit 2 of byte +0x10A) has all off.
// A weight not already at its end (UCOMISS against 1 or 0) moves by
// 1 / (the ticks per second (int16 +2 of [51DF54]) times the duration) and
// stops at the end (at once with no duration); the answer is whether any
// weight moved.
//
// The single-precision arithmetic is the original's scalar SSE, in its order,
// on the live MXCSR. This is the game logic only. The game's memory is the
// retained TU1.5 bank, reached through a Memory object with typed loads and
// stores; every access names its role (Access), including the original's
// frame (the moved byte, in the slot of the ECX it pushes); the boundary
// adapter (native_tu15_channel_fades.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_channel_fades {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace sse=native_aim_clusters::sse;
namespace scalar=native_record_fade::scalar;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,game=0x4f3218u,clock=0x51df54u,
    one=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,channel_size=8u,
    skipped_kind=4u,notice=0x1eu;
namespace definition {inline constexpr std::uint32_t flags=0xbcu,channel_count=0x1b8u,channels=0x1bcu;inline constexpr std::uint32_t fixed=11u;}
namespace object {inline constexpr std::uint32_t parent=0x14u,flags_byte=0x10au,state=0x134u,seat=0x1fcu,target=0x248u,
    second_target=0x24cu,weights=0x25cu;inline constexpr std::uint32_t held=4u,tracking=1u,seated_bit=26u;}
namespace game {inline constexpr std::uint32_t kind=0xcu;}
namespace clock {inline constexpr std::uint32_t ticks_per_second=2u;}
// The frame's slot, by offset from the return address: the moved byte, in the pushed ECX.
namespace local {inline constexpr std::int32_t moved=-1;}

namespace channel_fades {
enum class Access {headers,datums,instances,object_address,tag,definition_address,definition_flags,moved_clear,state,game_address,
    game_kind,parent,seat,state_load,state_store,channel_count,clock_address,one,channels,second_target,target_again,target,
    state_again,flags_byte,weight,duration,ticks,duration_again,weight_again,weight_store,weight_final,weight_end_store,moved_store,
    count_again,moved_load};
enum class Outcome {fixed,updated};
// The original's EBX (the object, then the clock), EBP (the definition), ESI (the object's address) and EDI (the object,
// then the channel) across the children.
struct Kept {std::uint32_t clock,definition,object,channel;};
// Frame:
//   local(), framing(object), saving_ebx()            PUSH ECX (the slot), EBP, ESI, EDI; PUSH EBX
//   fixed_tested(flags), state_tested(state), kind_tested(kind), parent_tested(parent), seat_tested(seat)
//   prepare(kept)                                     D3150(EBX = object) then D05A0, refresh kept
//   notice(object, kept)                              E8E10(ECX = 1E; the object), refreshes kept
//   counted(count), channel_selected(index, channel, on)   the channel's EDX/AL tests in the original's registers
//   at_end(weight, end, equal), fade(...)              UCOMISS/LAHF; the step's COMISS, CVTSI2SS, MULSS, DIVSS, ADDSS/SUBSS
//   advanced(index, count), restoring()
template<class Memory,class Frame> std::uint32_t update(Memory &memory,Frame &frame,std::uint32_t object_handle,Outcome &outcome) {
    using enum Access;
    Kept kept{};
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    frame.framing(object_handle);
    kept.object=memory.load32(object_address,datums_address+header_size*(object_handle&0xffffu)+header_address);
    const auto tag_value=memory.load32(tag,kept.object);
    kept.definition=memory.load32(definition_address,tag_size*(tag_value&0xffffu)+instances_address+tag_data);
    const auto flags=memory.load32(definition_flags,at(kept.definition,definition::flags));
    frame.fixed_tested(flags);
    memory.store8(moved_clear,frame.local(),0u);
    if((flags>>definition::fixed)&1u){outcome=Outcome::fixed;return 0u;}
    frame.saving_ebx();
    kept.clock=object_handle;
    kept.channel=object_handle;
    frame.prepare(kept);
    const auto state_value=memory.load32(state,at(kept.object,object::state));
    frame.state_tested(state_value);
    if((state_value>>object::seated_bit)&1u) {
        const auto kind=memory.load8(game_kind,at(memory.load32(game_address,globals::game),game::kind));
        frame.kind_tested(kind);
        if(kind!=skipped_kind) {
            const auto parent_value=memory.load32(parent,at(kept.object,object::parent));
            frame.parent_tested(parent_value);
            if(parent_value!=none) {
                const auto seat_value=memory.load16(seat,at(kept.object,object::seat));
                frame.seat_tested(seat_value);
                if(seat_value!=0xffffu) {
                    const auto cleared=memory.load32(state_load,at(kept.object,object::state))&~(1u<<object::seated_bit);
                    frame.state_cleared(cleared);
                    memory.store32(state_store,at(kept.object,object::state),cleared);
                    frame.notice(kept.channel,kept);
                }
            }
        }
    }
    // The channels' weights.
    const auto count=memory.load32(channel_count,at(kept.definition,definition::channel_count));
    frame.counted(count);
    if(signed_value(count)>0) {
        kept.clock=memory.load32(clock_address,globals::clock);
        const auto one_value=memory.load32(one,globals::one);
        frame.constants(one_value);
        std::uint32_t index=0u,count_now;
        do {
            kept.channel=memory.load32(channels,at(kept.definition,definition::channels))+channel_size*index;
            bool on=false;
            if(index==0u) {
                const auto target_value=memory.load32(target,at(kept.object,object::target));
                if(target_value!=none)on=true;
                else on=(memory.load32(state_again,at(kept.object,object::state))>>object::tracking)&1u;
                frame.first_channel(target_value,on);
            } else if(index==1u) {
                const auto second_value=memory.load32(second_target,at(kept.object,object::second_target));
                bool differs=false;
                if(second_value!=none)differs=second_value!=memory.load32(target_again,at(kept.object,object::target));
                on=differs;
                frame.second_channel(second_value,on);
            }
            frame.channel_selected(index,kept.channel,on);
            const auto byte=memory.load8(flags_byte,at(kept.object,object::flags_byte));
            frame.held_tested(byte,on);
            const bool fade_in=!(byte&object::held) && on;
            const auto weight_address=kept.object+object::weights+4u*index;
            const auto end=fade_in?one_value:0u;
            const auto weight_value=memory.load32(weight,weight_address);
            const bool at_end=scalar::ordered_equal(weight_value,end);
            frame.at_end(weight_value,end,at_end);
            if(!at_end) {
                const auto duration_value=memory.load32(duration,kept.channel);
                const bool timed=!sse::not_above(duration_value,0u);
                frame.timed(duration_value,timed);
                if(timed) {
                    const auto ticks_value=scalar::from_integer(widen16(memory.load16(ticks,at(kept.clock,clock::ticks_per_second))));
                    const auto span=sse::multiply(ticks_value,memory.load32(duration_again,kept.channel));
                    const auto step=sse::divide(one_value,span);
                    std::uint32_t moved_weight;
                    if(fade_in)moved_weight=sse::add(step,memory.load32(weight_again,weight_address));
                    else moved_weight=sse::subtract(memory.load32(weight_again,weight_address),step);
                    frame.stepped(ticks_value,span,step,moved_weight,fade_in);
                    memory.store32(weight_store,weight_address,moved_weight);
                } else {
                    memory.store32(weight_store,weight_address,end);
                }
                // The weight stops at its end.
                const auto final_weight=memory.load32(weight_final,weight_address);
                const bool past=fade_in?!sse::not_above(final_weight,one_value):!sse::not_above(0u,final_weight);
                frame.ended(final_weight,past,fade_in);
                if(past)memory.store32(weight_end_store,weight_address,end);
                memory.store8(moved_store,frame.local(),1u);
            }
            count_now=memory.load32(count_again,at(kept.definition,definition::channel_count));
            ++index;
            frame.advanced(index,count_now);
        } while(signed_value(index)<signed_value(count_now));
    }
    const auto moved=memory.load8(moved_load,frame.local());
    frame.restoring();
    outcome=Outcome::updated;
    return moved;
}
} // namespace channel_fades
} // namespace halo2::native_channel_fades
