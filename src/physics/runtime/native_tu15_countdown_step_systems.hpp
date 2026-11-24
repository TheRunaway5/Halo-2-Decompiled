#pragma once
// Native TU1.5 countdown step, entered at 1CEB70 (physics live support
// unit): a record's countdown, or its step once the countdown is over.
//
//   countdown_step::run   1CEB70(EAX = record; a, b, c) -> RET C
//
// The record's two words +0x14 and +0x16 are cleared. A countdown (byte
// +0x10) above zero counts down by one. Otherwise a record whose byte +0x12
// carries bit 0 and whose byte +0x11 lacks bits 0 and 3 steps: 1CE950 gets
// the record, the tick's length (the game time's +4 at [51DF54]) times the
// record's rate (+0x18) times the constant at 46A20C plus its base (+0x1C),
// in single precision on the live MXCSR, and a, b, c.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the arguments it reads; the boundary
// adapter (native_tu15_countdown_step.hpp) maps the role to the original
// instruction and owns the ABI of the child (1CE950).
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_countdown_step {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t game_time=0x51df54u,scale=0x46a20cu;}
inline constexpr std::uint32_t tick_length=4u;
namespace record {inline constexpr std::uint32_t countdown=0x10u,blocking=0x11u,enabled=0x12u,first_word=0x14u,second_word=0x16u,
    rate=0x18u,base=0x1cu;}
inline constexpr std::uint32_t enabled_bit=1u,blocking_bits=9u;
namespace argument {inline constexpr std::uint32_t a=0u,b=1u,c=2u;}

namespace countdown_step {
enum class Access {countdown,first_clear,second_clear,countdown_store,enabled,blocking,c_argument,b_argument,a_argument,game_time,
    tick,rate,scale,base,value_store};
enum class Outcome {counted,disabled,blocked,stepped};
// Frame:
//   argument(k)                 an argument's slot now
//   pushing(value, which)       one of 1CE950's arguments pushed (which: c, b, a, the value's slot)
//   value_slot()                the slot the value is stored into
//   step()                      1CE950(the record pushed)
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame,std::uint32_t record_address) {
    using enum Access;
    const auto count=memory.load8(Access::countdown,at(record_address,record::countdown))&0xffu;
    memory.store16(first_clear,at(record_address,record::first_word),0u);
    memory.store16(second_clear,at(record_address,record::second_word),0u);
    if(count) {
        memory.store8(countdown_store,at(record_address,record::countdown),count-1u);
        return Outcome::counted;
    }
    if(!(memory.load8(Access::enabled,at(record_address,record::enabled))&enabled_bit))return Outcome::disabled;
    if(memory.load8(Access::blocking,at(record_address,record::blocking))&blocking_bits)return Outcome::blocked;
    const auto c_value=memory.load32(c_argument,frame.argument(argument::c));
    const auto b_value=memory.load32(b_argument,frame.argument(argument::b));
    frame.pushing(c_value,0u);
    const auto a_value=memory.load32(a_argument,frame.argument(argument::a));
    frame.pushing(b_value,1u);
    const auto time=memory.load32(Access::game_time,globals::game_time);
    const auto tick_value=memory.load32(tick,at(time,tick_length));
    const auto rate_value=memory.load32(rate,at(record_address,record::rate));
    const auto scaled=sse::multiply(sse::multiply(tick_value,rate_value),memory.load32(Access::scale,globals::scale));
    const auto value=sse::add(scaled,memory.load32(Access::base,at(record_address,record::base)));
    frame.pushing(a_value,2u);
    frame.pushing(a_value,3u);
    memory.store32(value_store,frame.value_slot(),value);
    frame.step(value);
    return Outcome::stepped;
}
} // namespace countdown_step
} // namespace halo2::native_countdown_step
