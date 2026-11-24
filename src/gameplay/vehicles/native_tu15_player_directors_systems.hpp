#pragma once
// Native TU1.5 player directors, entered at 157FE0 (vehicle director unit):
// each local player's director runs for the frame.
//
//   player_directors::update   157FE0(seconds)
//
// The directors are four 0x140-byte records at 4F5518, one per local player
// slot; the frame's seconds are kept at 4F5508. For each slot with a player
// (the handles at [4F54F0]+0xC): a request (slot, seconds; 0x2C bytes) and
// a state (0xAC bytes) are prepared on the stack; the director mode (4F550C)
// selects through the original jump table (158220) what the mode change
// (the byte 4F5510, cleared afterwards) does: 158790 or 1588B0 are told, or
// the director is set to the default callback 245400 with its fade at 1.
// The director's callback (+8; 16F6A0 only for the first local player)
// fills the state from the request. When the state is active (bit 0 of its
// first byte) the director's timer (+0) runs down by the seconds, never
// below zero: a timer under 46A16C ends at once for the default callback
// (the state's two fades ending too), otherwise the state's longest timer
// (+0x88) keeps up with it; the state is then the director's (+0x58).
// Otherwise the director's own state is made inactive. 171FF0 then updates
// the slot.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_player_directors.hpp) maps the role to the original
// instruction and owns the ABI of the children (158790, 1588B0, the
// callbacks, 171FF0). The director and the slot the original keeps in EBX
// and ESI are a Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include <cstdint>

namespace halo2::native_player_directors {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
using native_record_fade::scalar::ordered_equal;

namespace globals {
inline constexpr std::uint32_t players=0x4f54f0u;          // +0xC: the four local player handles
inline constexpr std::uint32_t seconds=0x4f5508u;          // float: this frame's seconds
inline constexpr std::uint32_t mode=0x4f550cu;             // the director mode (0..4 dispatch)
inline constexpr std::uint32_t mode_changed=0x4f5510u;     // byte
inline constexpr std::uint32_t directors=0x4f5518u;        // the four 0x140-byte directors
inline constexpr std::uint32_t mode_table=0x158220u;       // the original jump table
inline constexpr std::uint32_t one=0x46a0d0u,quick_end=0x46a16cu;   // floats
}
inline constexpr std::uint32_t slot_count=4u,director_size=0x140u,local_handles=0xcu,request_words=11u,
    state_words=0x2bu;
// The original walks the directors through a pointer 0x52 bytes in, and
// checks a director's address (an offset from -0x140) against null.
inline constexpr std::uint32_t director_cursor=0x52u,null_director=0xfffffec0u;
inline constexpr std::uint32_t default_callback=0x245400u,first_player_callback=0x16f6a0u;
// The mode's cases, by their original addresses.
inline constexpr std::uint32_t tell_158790=0x15807du,no_change=0x158089u,tell_1588b0=0x1580d0u,reset_default=0x1580dfu;
namespace director {
inline constexpr std::uint32_t timer=0u,callback=8u,target=0xcu,target_second=0x10u,flag_a=0x51u,flag_b=0x52u,
    state=0x58u,fade_flag=0x104u,fade=0x108u;
}
namespace state {
inline constexpr std::uint32_t longest=0x88u,first_fade_kind=0x8cu,second_fade_kind=0x8fu,first_fade=0x94u,
    second_fade=0xa0u;
inline constexpr std::uint32_t ending_fade=3u;
}
namespace request {inline constexpr std::uint32_t slot=0u,seconds=8u;}

namespace player_directors {
// The argument, and the locals of the aligned frame.
enum class Local {seconds,director_offset,handle_offset,slot,request,state};
enum class Access {
    seconds_argument,seconds_store,offset_init,callback_call,handle_offset_init,slot_init,director_offset,players,handle_offset,
    player_handle,request_clear,flag_a_clear,flag_b_clear,mode,seconds_global,request_slot,request_seconds,
    mode_case,mode_changed_for_158790,mode_changed_for_1588b0,mode_changed_for_reset,one,target_clear,
    target_second_clear,callback_store,fade_store,fade_flag_clear,mode_changed_clear,state_clear,callback,
    first_players,first_probe,state_flags,timer,quick_end,timer_quick,callback_again,timer_end,first_fade_end,
    first_fade_kind,second_fade_end,second_fade_kind,longest,timer_longest,timer_kept,longest_store,timer_now,
    seconds_again,timer_store,state_word,state_word_store,slot_reload,inactive_read,inactive_store,offset_read,
    handle_offset_read,slot_store,offset_store,handle_offset_store
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t cursor;   // EBX: the director, 0x52 bytes in
    std::uint32_t slot;     // ESI: the local player slot
};

// Frame:
//   local(Local)               an argument's or local's address now
//   tell_158790(changed, kept) 158790(AL changed, ECX the slot); refreshes
//   tell_1588b0(changed, kept) 1588B0(CL changed, EAX the slot); refreshes
//   run_callback(entry, target, kept)  the director's callback (entry)
//                              (target, request, state) through the
//                              unit's route; refreshes
//   unknown_case()             a mode case the table does not name
//   first_slot_found(found)    the first local player search's end
//   timer_left(left)           the timer less the seconds, before the
//                              floor at zero (XMM0, flags)
//   update_slot(kept)          171FF0(ECX the slot); refreshes kept
template<class Memory,class Frame> void update(Memory &memory,Frame &frame) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto seconds=memory.load32(seconds_argument,local(Local::seconds));
    Kept kept{globals::directors+director_cursor,0u};
    const auto director=[&]{return kept.cursor-director_cursor;};
    memory.store32(seconds_store,globals::seconds,seconds);
    memory.store32(slot_init,local(Local::slot),kept.slot);
    memory.store32(handle_offset_init,local(Local::handle_offset),local_handles);
    memory.store32(offset_init,local(Local::director_offset),kept.slot);
    for(;;) {
        if(memory.load32(director_offset,local(Local::director_offset))!=null_director) {
            const auto player_globals=memory.load32(players,globals::players);
            const auto handle=memory.load32(player_handle,memory.load32(handle_offset,local(Local::handle_offset))+player_globals);
            if(handle!=none) {
                // The request and the state start empty.
                for(std::uint32_t word=0;word<request_words;++word)
                    memory.store32(request_clear,local(Local::request)+4u*word,0u);
                memory.store8(flag_a_clear,at(director(),director::flag_a),0u);
                memory.store8(flag_b_clear,at(director(),director::flag_b),0u);
                const auto mode_value=memory.load32(mode,globals::mode);
                const auto frame_seconds=memory.load32(seconds_global,globals::seconds);
                memory.store32(request_slot,local(Local::request)+request::slot,kept.slot);
                memory.store32(request_seconds,local(Local::request)+request::seconds,frame_seconds);
                bool reset=false;
                if(mode_value<=4u) {
                    switch(memory.load32(mode_case,globals::mode_table+4u*mode_value)) {
                    case tell_158790:frame.tell_158790(memory.load8(mode_changed_for_158790,globals::mode_changed),kept);break;
                    case tell_1588b0:frame.tell_1588b0(memory.load8(mode_changed_for_1588b0,globals::mode_changed),kept);break;
                    case reset_default:
                        if(memory.load8(mode_changed_for_reset,globals::mode_changed)) {
                            const auto one_value=memory.load32(one,globals::one);
                            memory.store32(target_clear,at(director(),director::target),0u);
                            memory.store32(target_second_clear,at(director(),director::target_second),0u);
                            memory.store32(callback_store,at(director(),director::callback),default_callback);
                            memory.store32(fade_store,at(director(),director::fade),one_value);
                            memory.store8(fade_flag_clear,at(director(),director::fade_flag),0u);
                            reset=true;
                        }
                        break;
                    case no_change:break;
                    default:frame.unknown_case();
                    }
                }
                frame.cases_done(reset);
                memory.store8(mode_changed_clear,globals::mode_changed,0u);
                for(std::uint32_t word=0;word<state_words;++word)
                    memory.store32(state_clear,local(Local::state)+4u*word,0u);
                // The director's callback fills the state.
                const auto callback_value=memory.load32(callback,at(director(),director::callback));
                if(callback_value!=0) {
                    bool runs=true;
                    if(callback_value==first_player_callback) {
                        const auto first_globals=memory.load32(first_players,globals::players);
                        std::uint32_t first=none;
                        for(std::uint32_t k=0;k<slot_count;++k)
                            if(memory.load32(first_probe,at(first_globals,local_handles+4u*k))!=none){first=k;break;}
                        frame.first_slot_found(first);
                        runs=kept.slot==first;
                    }
                    if(runs)frame.run_callback(memory.load32(callback_call,at(director(),director::callback)),
                                               at(director(),director::target),kept);
                }
                if(memory.load8(state_flags,local(Local::state))&1u) {
                    const auto timer_value=memory.load32(timer,at(director(),director::timer));
                    if(!ordered_equal(timer_value,0u)) {
                        const auto quick=memory.load32(quick_end,globals::quick_end);
                        if(!sse::not_above(quick,memory.load32(timer_quick,at(director(),director::timer))) &&
                           memory.load32(callback_again,at(director(),director::callback))==default_callback) {
                            memory.store32(timer_end,at(director(),director::timer),0u);
                            memory.store32(first_fade_end,local(Local::state)+state::first_fade,0u);
                            memory.store8(first_fade_kind,local(Local::state)+state::first_fade_kind,state::ending_fade);
                            memory.store32(second_fade_end,local(Local::state)+state::second_fade,0u);
                            memory.store8(second_fade_kind,local(Local::state)+state::second_fade_kind,state::ending_fade);
                        } else {
                            const auto longest_value=memory.load32(longest,local(Local::state)+state::longest);
                            if(sse::not_above(longest_value,memory.load32(timer_longest,at(director(),director::timer)))) {
                                memory.store32(longest_store,local(Local::state)+state::longest,
                                               memory.load32(timer_kept,at(director(),director::timer)));
                            }
                        }
                        const auto now=memory.load32(timer_now,at(director(),director::timer));
                        auto left=sse::subtract(now,memory.load32(seconds_again,local(Local::seconds)));
                        frame.timer_left(left);
                        if(!sse::not_above(0u,left))left=0u;
                        memory.store32(timer_store,at(director(),director::timer),left);
                    }
                    for(std::uint32_t word=0;word<state_words;++word)
                        memory.store32(state_word_store,at(director(),director::state+4u*word),
                                       memory.load32(state_word,local(Local::state)+4u*word));
                    kept.slot=memory.load32(slot_reload,local(Local::slot));
                } else {
                    const auto flags=memory.load32(inactive_read,at(director(),director::state));
                    memory.store32(inactive_store,at(director(),director::state),flags&~1u);
                }
                frame.update_slot(kept);
            }
        }
        // The next slot.
        const auto offset=memory.load32(offset_read,local(Local::director_offset));
        const auto handles=memory.load32(handle_offset_read,local(Local::handle_offset));
        ++kept.slot;
        kept.cursor+=director_size;
        memory.store32(slot_store,local(Local::slot),kept.slot);
        memory.store32(offset_store,local(Local::director_offset),offset+director_size);
        memory.store32(handle_offset_store,local(Local::handle_offset),handles+4u);
        if(!(std::int32_t(kept.cursor)<std::int32_t(globals::directors+director_cursor+slot_count*director_size)))break;
    }
}
} // namespace player_directors
} // namespace halo2::native_player_directors
