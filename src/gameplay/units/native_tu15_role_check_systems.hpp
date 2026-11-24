#pragma once
// Native TU1.5 role check, entered at 17E100 (death cycle unit): whether a
// record's role is out of place for its owner.
//
//   role_check::check   17E100(EDX = owner*, ECX = record*, ESI = side) -> AL
//
// The owner's bit 5 of byte +2 asks for role 1 (set) or 2 (clear); the
// result is whether the record's role word (+0x10) differs. With exactly one
// local player (the word +8 of [4F54F0]) a differing role is also measured
// against the record's second role word (+0x12): role 2 is refused for a
// nonzero side, role 1 is accepted only for side 1, and any other is
// accepted.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_role_check.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_role_check {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t players=0x4f54f0u;}   // +8: the local player count (word)
inline constexpr std::uint32_t owner_flags=2u,first_role_bit=5u,local_count=8u,role=0x10u,second_role=0x12u;
inline constexpr std::uint32_t role_one=1u,role_two=2u;

namespace role_check {
enum class Access {owner_flags,role,players,local_count,second_role};
enum class Outcome {several,matching,refused_two,accepted,accepted_one,refused_one};
struct Result {Outcome outcome;std::uint32_t value;};
// Frame: compared(value, wanted, bytes) a CMP; noted(differs) the result
// taken; side_tested(side) TEST ESI,ESI.
template<class Memory,class Frame> Result check(Memory &memory,Frame &frame,std::uint32_t owner,std::uint32_t record,
                                                std::uint32_t side) {
    using enum Access;
    const auto flags_value=memory.load8(Access::owner_flags,at(owner,native_role_check::owner_flags))&0xffu;
    const auto current=memory.load16(Access::role,at(record,native_role_check::role))&0xffffu;
    const auto wanted=((flags_value>>first_role_bit)&1u)?role_one:role_two;
    frame.compared(current,wanted,2);
    const std::uint32_t differs=current!=wanted;
    const auto locals=memory.load32(players,globals::players);
    frame.noted(differs);
    const auto count=memory.load16(Access::local_count,at(locals,native_role_check::local_count))&0xffffu;
    frame.compared(count,1u,2);
    if(count!=1u)return {Outcome::several,differs};
    if(!differs)return {Outcome::matching,0u};
    const auto second=memory.load16(Access::second_role,at(record,native_role_check::second_role))&0xffffu;
    frame.compared(second,role_two,2);
    if(second==role_two) {
        frame.side_tested(side);
        if(side!=0)return {Outcome::refused_two,0u};
    }
    frame.compared(second,role_one,2);
    if(second!=role_one)return {Outcome::accepted,1u};
    frame.compared(side,1u,4);
    return side==1u?Result{Outcome::accepted_one,1u}:Result{Outcome::refused_one,0u};
}
} // namespace role_check
} // namespace halo2::native_role_check
