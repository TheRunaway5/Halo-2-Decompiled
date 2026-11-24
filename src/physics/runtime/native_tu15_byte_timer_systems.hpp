#pragma once
// Native TU1.5 byte timer step, entered at 1E15D0 (physics live support
// unit): a four-byte timer advanced one tick.
//
//   byte_timer::step   1E15D0(ECX = timer*) -> AL whether it ran
//
// The timer holds its count (+0), its limit (+1) and flags (+3: bit 0 counts
// up, bit 1 done). A timer with no limit, or done, does not run. Otherwise
// the count moves one step (up or down, as signed bytes); a count up that
// reaches the limit, or a count down that reaches zero, stops there and the
// timer is marked done.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_byte_timer.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_byte_timer {
using native_object_update::at;
struct ByteTimer {std::uint8_t count,limit,unused,flags;};
static_assert(sizeof(ByteTimer)==4 && offsetof(ByteTimer,limit)==1 && offsetof(ByteTimer,flags)==3);
namespace timer_flags {inline constexpr std::uint32_t counts_up=1u<<0,done=1u<<1;}
constexpr std::int32_t signed_byte(std::uint32_t v){return std::int8_t(std::uint8_t(v));}

namespace byte_timer {
enum class Access {limit,flags,up_count,up_store,up_done_count,up_done_flags,down_count,down_store,down_done_count,down_done_flags};
enum class Outcome {idle,finished,up,up_done,down,down_done};
template<class Memory> Outcome step(Memory &memory,std::uint32_t timer) {
    using enum Access;
    const auto limit_value=memory.load8(Access::limit,at(timer,offsetof(ByteTimer,limit)))&0xffu;
    if(!limit_value)return Outcome::idle;
    const auto flags_value=memory.load8(Access::flags,at(timer,offsetof(ByteTimer,flags)))&0xffu;
    if(flags_value&timer_flags::done)return Outcome::finished;
    if(flags_value&timer_flags::counts_up) {
        const auto count=(memory.load8(up_count,at(timer,offsetof(ByteTimer,count)))+1u)&0xffu;
        memory.store8(up_store,at(timer,offsetof(ByteTimer,count)),count);
        if(signed_byte(count)<signed_byte(limit_value))return Outcome::up;
        memory.store8(up_done_count,at(timer,offsetof(ByteTimer,count)),limit_value);
        memory.store8(up_done_flags,at(timer,offsetof(ByteTimer,flags)),flags_value|timer_flags::done);
        return Outcome::up_done;
    }
    const auto count=(memory.load8(down_count,at(timer,offsetof(ByteTimer,count)))-1u)&0xffu;
    memory.store8(down_store,at(timer,offsetof(ByteTimer,count)),count);
    if(signed_byte(count)>0)return Outcome::down;
    memory.store8(down_done_count,at(timer,offsetof(ByteTimer,count)),0u);
    memory.store8(down_done_flags,at(timer,offsetof(ByteTimer,flags)),flags_value|timer_flags::done);
    return Outcome::down_done;
}
} // namespace byte_timer
} // namespace halo2::native_byte_timer
