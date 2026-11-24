#pragma once
// Native TU1.5 round countdown, entered at 15E870 (multiplayer engines
// unit): the round's time announced as it runs down.
//
//   round_countdown::announce   15E870()
//
// With a display for the session's player ([56DFA0] by +0xC14), a session in
// state 1 (word +0x6C) and active (+0xC04 is 1), and a variant whose mode
// (signed byte +0xC) is not 4: a timed round (time limit +0x190) has the
// round timer (164EF0, BL 1) show the remaining ticks, an untimed one counts
// the ticks since the round's start (+0x70; at least 0). Either becomes whole
// seconds (CVTSI2SS, times the time globals' seconds per tick [51DF54]+4,
// CVTTSS2SI; the untimed at most 0x7FFF). Seconds other than the session's
// last announced (word +0xE0) are kept there with B1A10 (through the
// dispatch, 0x10 pushed). In a timed round, 1800, 900, 300, 60, 30 and 10
// seconds raise their event (types 0xE, 0xF, 0x10, 0x11, 0x20, 0x21) and no
// ticks left raises the countdown event (0x12; 1A87A0 through the dispatch,
// EAX the event: zero, the type, five NONE handles, zero, a NONE word), then
// 15DD50 and 15DDC0 (the required owner, 0 pushed) end the round.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_round_countdown.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_round_countdown {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t session=0x4f63b8u,displays=0x56dfa0u,variant=0x4f3218u,time=0x51df54u;}
namespace session {inline constexpr std::uint32_t state=0x6cu,start=0x70u,announced=0xe0u,active=0xc04u,player=0xc14u;}
namespace variant {inline constexpr std::uint32_t mode=0xcu,time_limit=0x190u;inline constexpr std::uint32_t untimed_mode=4u;}
namespace time {inline constexpr std::uint32_t seconds_per_tick=4u,tick=8u;}
inline constexpr std::uint32_t running=1u,sound=0x10u,longest=0x7fffu,countdown=0x12u;
// The seconds that raise an event, and their events.
struct Announcement {std::uint32_t seconds,type;};
inline constexpr Announcement announcements[]{{1800u,0xeu},{900u,0xfu},{300u,0x10u},{60u,0x11u},{30u,0x20u},{10u,0x21u}};
namespace event {inline constexpr std::uint32_t first=0u,type=4u,handles=8u,last=0x1cu,word=0x20u;inline constexpr std::uint32_t handle_count=5u;}
// The frame below the saved registers: the seconds' float, the event.
namespace local {inline constexpr std::uint32_t seconds=0xcu,event=0x10u,size=0x28u;}

namespace round_countdown {
enum class Access {session_load,player_load,display_load,state_load,variant_load,mode_load,active_load,limit_load,time_load,rate_load,
    seconds_store,seconds_load,session_again,announced_load,announced_store,type_store,first_store,handle_store,last_store,word_store,
    time_untimed,tick_load,start_load,rate_untimed,untimed_store,untimed_load,announced_untimed,untimed_announced_store};
// The original's EBX (the ticks shown), EBP (zero), ESI (the session, then the seconds) and EDI (NONE).
struct Kept {std::uint32_t shown,zero,held,none;};
// Frame: see native_tu15_round_countdown.hpp.
template<class Memory,class Frame> void announce(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.framing();
    kept.held=memory.load32(session_load,globals::session);
    const auto player=memory.load32(player_load,at(kept.held,session::player));
    const auto display=memory.load32(display_load,globals::displays+4u*player);
    kept.zero=frame.zeroed();
    frame.compared(display,kept.zero);
    if(!display){frame.restoring(0u);return;}
    frame.saving(1u);
    const auto state=memory.load16(state_load,at(kept.held,session::state));
    frame.word_compared(state,running);
    if((state&0xffffu)!=running){frame.restoring(1u);return;}
    const auto variant_address=memory.load32(variant_load,globals::variant);
    const auto mode=frame.mode(memory.load8(mode_load,at(variant_address,variant::mode)));
    if(mode==variant::untimed_mode){frame.restoring(1u);return;}
    const auto active=memory.load32(active_load,at(kept.held,session::active));
    frame.compared(active,running);
    if(active!=running){frame.restoring(1u);return;}
    frame.compared(mode,variant::untimed_mode);
    if(mode==variant::untimed_mode){frame.restoring(1u);return;}
    const auto limit=memory.load32(limit_load,at(variant_address,variant::time_limit));
    frame.compared(limit,kept.zero);
    const auto event_raised=[&](std::uint32_t type,bool type_first) {
        const auto address=frame.event_address(type_first);
        if(type_first)memory.store32(type_store,frame.local(local::event+event::type),type);
        memory.store32(first_store,frame.local(local::event+event::first),kept.zero);
        if(!type_first)memory.store32(type_store,frame.local(local::event+event::type),type);
        for(std::uint32_t k=0;k<event::handle_count;++k)memory.store32(handle_store,frame.local(local::event+event::handles+4u*k),kept.none);
        memory.store32(last_store,frame.local(local::event+event::last),kept.zero);
        memory.store16(word_store,frame.local(local::event+event::word),kept.none);
        frame.raised(address,kept);
    };
    if(limit) {
        // A timed round: the remaining seconds.
        frame.saving(2u);
        frame.timed(kept);
        const auto time_address=memory.load32(time_load,globals::time);
        kept.shown=frame.shown_kept();
        const auto rate=memory.load32(rate_load,at(time_address,time::seconds_per_tick));
        memory.store32(seconds_store,frame.local(local::seconds),frame.scaled(sse::multiply(frame.converted(kept.shown),rate)));
        kept.held=frame.truncated(memory.load32(seconds_load,frame.local(local::seconds)));
        const auto session_now=memory.load32(session_again,globals::session);
        const auto announced=frame.widened(memory.load16(announced_load,at(session_now,session::announced)));
        kept.none=frame.handles_none();
        frame.compared(kept.held,announced);
        if(kept.held!=announced) {
            frame.sound_pushed();
            memory.store16(announced_store,at(session_now,session::announced),kept.held);
            frame.sounded(kept);
            for(const auto &a:announcements) {
                frame.compared(kept.held,a.seconds);
                if(kept.held==a.seconds){event_raised(a.type,true);break;}
            }
        }
        frame.compared(kept.shown,kept.zero);
        if(std::int32_t(kept.shown)<=std::int32_t(kept.zero)) {
            event_raised(countdown,false);
            frame.ended(kept);
        }
        frame.restoring(2u);
        return;
    }
    // An untimed round: the seconds so far.
    const auto time_address=memory.load32(time_untimed,globals::time);
    const auto tick=memory.load32(tick_load,at(time_address,time::tick));
    const auto elapsed=frame.elapsed(tick,memory.load32(start_load,at(kept.held,session::start)));
    const auto rate=memory.load32(rate_untimed,at(time_address,time::seconds_per_tick));
    memory.store32(untimed_store,frame.local(local::seconds),frame.scaled(sse::multiply(frame.converted(elapsed),rate)));
    auto seconds=frame.truncated_untimed(memory.load32(untimed_load,frame.local(local::seconds)));
    seconds=frame.limited(seconds,longest);
    const auto announced=frame.widened_untimed(memory.load16(announced_untimed,at(kept.held,session::announced)));
    frame.compared(seconds,announced);
    if(seconds==announced){frame.restoring(1u);return;}
    frame.sound_pushed_untimed();
    memory.store16(untimed_announced_store,at(kept.held,session::announced),seconds);
    frame.sounded_untimed(kept);
    frame.restoring(3u);
}
} // namespace round_countdown
} // namespace halo2::native_round_countdown
