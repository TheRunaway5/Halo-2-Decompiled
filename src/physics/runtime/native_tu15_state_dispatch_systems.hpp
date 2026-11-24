#pragma once
// Native TU1.5 state dispatch, entered at 1EDDA0 (physics live support
// unit): a physical state handed to its kind's owner.
//
//   state_dispatch::run   1EDDA0(EDX = state, ECX = owner; argument) -> RET 4
//
// The state's kind (byte +0) less one selects, through the original's jump
// table (1EDDE0, without a bound), what happens: 1F9720 (the owner pushed,
// EDX the state's +0x10), nothing, 202CE0 (the state's +0x10 pushed), or
// 1F5230 (ESI the owner, EDI the argument).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the jump table's entry; the boundary
// adapter (native_tu15_state_dispatch.hpp) maps the role to the original
// instruction and owns the ABI of the children.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_state_dispatch {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t kind_table=0x1edde0u;}   // the original's jump table, by kind - 1
inline constexpr std::uint32_t state_body=0x10u;
// The table's targets, as the unit's canonical_branch names them.
namespace cases {inline constexpr std::uint32_t first=0x1eddb1u,nothing=0x1eddbau,second=0x1eddbfu,third=0x1eddcdu;}

namespace state_dispatch {
enum class Access {argument,kind,kind_case};
enum class Outcome {first,nothing,second,third};
// Frame:
//   argument()                     the argument's slot now
//   first(owner, body)             1F9720(EDX the body; the owner pushed)
//   second(body)                   202CE0(the body pushed)
//   third(owner, argument)         1F5230(ESI the owner, EDI the argument)
//   unknown_case()                 an entry that is no case (throws)
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame,std::uint32_t state,std::uint32_t owner) {
    using enum Access;
    const auto argument_value=memory.load32(argument,frame.argument());
    const auto kind_value=memory.load8(kind,state)&0xffu;
    switch(memory.load32(kind_case,globals::kind_table+4u*(kind_value-1u))) {
    case cases::first:frame.first(owner,at(state,state_body));return Outcome::first;
    case cases::nothing:return Outcome::nothing;
    case cases::second:frame.second(at(state,state_body));return Outcome::second;
    case cases::third:frame.third(owner,argument_value);return Outcome::third;
    }
    frame.unknown_case();
    return Outcome::nothing;
}
} // namespace state_dispatch
} // namespace halo2::native_state_dispatch
