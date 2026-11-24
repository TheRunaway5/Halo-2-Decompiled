#pragma once
// Native TU1.5 mode action, entered at 1582F0 (combat action unit): a unit's
// action chosen by its mode.
//
//   mode_action::act   1582F0(EAX = unit; first, second) -> AX, RET 8
//
// 158340 answers the unit's mode (AX; EBX the unit, the second argument's
// slot given for its word). Mode zero goes to 244F80 (ECX the unit), any
// other to 245D10, each with the first argument and the second as it was
// read. The answer is the mode.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_mode_action.hpp) maps the role to the original instruction
// and owns the ABI of the children; the unit, the second argument and the
// mode, which the original keeps in EDI, EBP and ESI across them, are a
// Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_mode_action {
namespace mode_action {
enum class Access {second,first};
enum class Outcome {still,moving};
struct Kept {std::uint32_t unit,second,mode;};   // EDI, EBP, ESI
// Frame:
//   argument(k)                                   the k-th argument's slot now
//   mode(kept) -> AX                              158340, refreshes kept
//   act(kept, moving, first)                      244F80 (mode zero) or 245D10, refreshes kept
template<class Memory,class Frame> Outcome act(Memory &memory,Frame &frame,Kept &kept) {
    kept.second=memory.load32(Access::second,frame.argument(2u));
    kept.mode=frame.mode(kept)&0xffffu;
    const bool moving=kept.mode!=0u;
    frame.act(kept,moving,memory.load32(Access::first,frame.argument(1u)));
    return moving?Outcome::moving:Outcome::still;
}
} // namespace mode_action
} // namespace halo2::native_mode_action
