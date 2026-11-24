#pragma once
// Native TU1.5 round timer, entered at 164EF0 (multiplayer engines unit):
// the round's remaining time shown, and the time-up event raised once.
//
//   round_timer::update   164EF0(BL = announce) -> EAX the time shown
//
// The remaining ticks are the variant's time limit (+0x190, seconds) times
// the time globals' rate (the signed word +2 of [51DF54]) less the ticks
// since the session's round start (+0x70; the tick +8), neither below 0.
// The session's player's display ([56DFA0], by +0xC14) shows them through
// its virtual +0x64 (through the dispatch: the remaining ticks, EBX and 1)
// and answers the time it shows. When announcing, a display that shows the
// remaining time clears the announced byte [56E224]; otherwise, once, a
// time-up event (type 0x25, its handles NONE) is raised (B1E80 and 1A8740,
// through the dispatch, unless the variant's mode +0xC is 4) and the byte set.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_round_timer.hpp) maps the role to the original instruction
// and owns the ABI of the children. The values the original keeps in
// callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_round_timer {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t variant=0x4f3218u,session=0x4f63b8u,time=0x51df54u,displays=0x56dfa0u,announced=0x56e224u;}
namespace variant {inline constexpr std::uint32_t mode=0xcu,time_limit=0x190u;inline constexpr std::uint32_t untimed_mode=4u;}
namespace session {inline constexpr std::uint32_t start=0x70u,player=0xc14u;}
namespace time {inline constexpr std::uint32_t rate=2u,tick=8u;}
namespace display {inline constexpr std::uint32_t vtable=0u,show=0x64u;}
// The time-up event on the stack: zero, its type, five NONE handles, zero, a NONE word.
namespace event {inline constexpr std::uint32_t first=0u,type=4u,handles=8u,last=0x1cu,word=0x20u;inline constexpr std::uint32_t time_up=0x25u,handle_count=5u;}
namespace local {inline constexpr std::uint32_t event=0xcu,size=0x24u;}
inline constexpr std::uint32_t none=0xffffffffu;

namespace round_timer {
enum class Access {variant_load,session_load,start_load,limit_load,time_load,tick_load,player_load,rate_load,display_load,vtable_load,
    show_slot,announced_clear,announced_load,handle_store,word_store,variant_again,first_store,last_store,mode_load,type_store,
    announced_set};
// The original's EBX (the announce flag), ESI (the time shown) and EDI (the remaining ticks) across the children.
struct Kept {std::uint32_t announce,shown,remaining;};
// Frame: framing(); saving(n); local(offset) below the saved registers; elapsed(tick, start); pass(v); clamped(v); remaining(rate,
// limit, elapsed); shown_by(slot, kept) [the display]; compared(a, b); byte_compared(v); sent(kept) [B1E80]; posted(kept) [1A8740];
// restoring(n).
template<class Memory,class Frame> std::uint32_t update(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    // The remaining ticks.
    const auto variant_address=memory.load32(variant_load,globals::variant);
    frame.framing();
    const auto session_address=memory.load32(session_load,globals::session);
    const auto start=memory.load32(start_load,at(session_address,session::start));
    frame.saving(0u);
    const auto limit=memory.load32(limit_load,at(variant_address,variant::time_limit));
    frame.saving(1u);
    const auto time_address=memory.load32(time_load,globals::time);
    const auto tick=memory.load32(tick_load,at(time_address,time::tick));
    const auto elapsed=frame.elapsed(tick,start);
    const auto player=memory.load32(player_load,at(session_address,session::player));
    frame.pass(1u);
    frame.pass(kept.announce);
    const auto counted=frame.clamped(elapsed);
    const auto rate=memory.load16(rate_load,at(time_address,time::rate));
    kept.remaining=frame.remaining(rate,limit,counted);
    // The display shows them.
    const auto display_address=memory.load32(display_load,globals::displays+4u*player);
    const auto table=memory.load32(vtable_load,at(display_address,display::vtable));
    frame.pass(kept.remaining);
    const auto target=memory.load32(show_slot,at(table,display::show));
    kept.shown=frame.shown_by(target,kept);
    if(!(kept.announce&0xffu)){frame.restoring(0u);frame.restoring(2u);return kept.shown;}
    frame.compared(kept.shown,kept.remaining);
    if(kept.shown==kept.remaining) {
        memory.store8(announced_clear,globals::announced,0u);
        frame.restoring(0u);frame.restoring(2u);return kept.shown;
    }
    const auto announced=memory.load8(announced_load,globals::announced);
    frame.byte_compared(announced);
    if(announced){frame.restoring(0u);frame.restoring(2u);return kept.shown;}
    // The time-up event, once.
    for(std::uint32_t k=0;k<event::handle_count;++k)memory.store32(handle_store,frame.local(local::event+event::handles+4u*k),none);
    memory.store16(word_store,frame.local(local::event+event::word),none);
    const auto variant_now=memory.load32(variant_again,globals::variant);
    memory.store32(first_store,frame.local(local::event+event::first),0u);
    memory.store32(last_store,frame.local(local::event+event::last),0u);
    const auto mode=memory.load8(mode_load,at(variant_now,variant::mode));
    frame.mode_compared(mode);
    memory.store32(type_store,frame.local(local::event+event::type),event::time_up);
    if(mode!=variant::untimed_mode){frame.sent(kept);frame.posted(kept);}
    frame.restoring(1u);
    memory.store8(announced_set,globals::announced,1u);
    frame.restoring(3u);
    return kept.shown;
}
} // namespace round_timer
} // namespace halo2::native_round_timer
