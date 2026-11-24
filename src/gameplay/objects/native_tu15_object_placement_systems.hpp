#pragma once
// Native TU1.5 object placement, entered at B95D0 (death cycle and physics
// live support units): an object moved to a position, facing and up, each
// optional.
//
//   object_placement::place   B95D0(EAX = position*, ECX = forward*, EDX = up*;
//                                    object, a, b) -> RET C
//
// The vectors given (not NULL) are copied into the frame, in the original's
// order, and 10E340 sees the position and forward copies (NULL for those
// not given). B9460 then places the object: the object, the three copies
// (or NULL), a, then 1, 1, b and 0. When it refuses (AL 0), BA570 is told for
// the object, except in game mode 4 (byte +0xC of the game globals at
// [4F3218]) for an object whose +0xD4 is not NONE. B9390 follows for the
// object either way.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the copies and the arguments in the
// original's frame; the boundary adapter (native_tu15_object_placement.hpp)
// maps the role to the original instruction and owns the ABI of the children.
// The values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_placement {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {inline constexpr std::uint32_t game=0x4f3218u,object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,game_mode=0xcu,kept_mode=4u,link=0xd4u;
// The frame's copies, by offset.
namespace copy {inline constexpr std::uint32_t position=0u,forward=0xcu,up=0x18u;}
namespace argument {inline constexpr std::uint32_t object=0u,a=1u,b=2u;}

namespace object_placement {
enum class Access {object_argument,position_x,position_x_store,position_y,position_z,position_y_store,position_z_store,
    forward_x,forward_x_store,forward_y,forward_z,forward_y_store,forward_z_store,up_x,up_y,up_z,up_x_store,up_y_store,up_z_store,
    b_argument,a_argument,game,mode,headers,datums,object_address,object_link};
enum class Outcome {placed,refused_kept,refused};
// The original's EBP, ESI, EBX and EDI across the children: the position's,
// the forward's and the up's copies (0 for none) and the object.
struct Kept {std::uint32_t position,forward,up,object;};
// Frame:
//   argument(k), copy(offset)          an argument's slot, the copies' address, now
//   checked(kept)                      10E340(the position and forward copies pushed), refreshes kept
//   place(kept, a, b) -> AL            B9460(object, the copies, a, 1, 1, b, 0), refreshes kept
//   refused(kept)                      BA570(the object pushed), refreshes kept
//   finish(kept)                       B9390(EAX the object)
template<class Memory,class Frame> Outcome place(Memory &memory,Frame &frame,std::uint32_t position,std::uint32_t forward,std::uint32_t up) {
    using enum Access;
    Kept kept{0u,0u,0u,0u};
    kept.object=memory.load32(object_argument,frame.argument(argument::object));
    if(position) {
        memory.store32(position_x_store,frame.copy(copy::position),memory.load32(position_x,at(position,0u)));
        const auto y=memory.load32(position_y,at(position,4u));
        const auto z=memory.load32(position_z,at(position,8u));
        memory.store32(position_y_store,frame.copy(copy::position+4u),y);
        memory.store32(position_z_store,frame.copy(copy::position+8u),z);
        kept.position=frame.copy(copy::position);
    }
    if(forward) {
        memory.store32(forward_x_store,frame.copy(copy::forward),memory.load32(forward_x,at(forward,0u)));
        const auto y=memory.load32(forward_y,at(forward,4u));
        const auto z=memory.load32(forward_z,at(forward,8u));
        memory.store32(forward_y_store,frame.copy(copy::forward+4u),y);
        memory.store32(forward_z_store,frame.copy(copy::forward+8u),z);
        kept.forward=frame.copy(copy::forward);
    }
    if(up) {
        const auto x=memory.load32(up_x,at(up,0u));
        const auto y=memory.load32(up_y,at(up,4u));
        const auto z=memory.load32(up_z,at(up,8u));
        memory.store32(up_x_store,frame.copy(copy::up),x);
        memory.store32(up_y_store,frame.copy(copy::up+4u),y);
        memory.store32(up_z_store,frame.copy(copy::up+8u),z);
        kept.up=frame.copy(copy::up);
    }
    frame.checked(kept);
    const auto b_value=memory.load32(b_argument,frame.argument(argument::b));
    const auto a_value=memory.load32(a_argument,frame.argument(argument::a));
    auto outcome=Outcome::placed;
    if(!(frame.place(kept,a_value,b_value)&0xffu)) {
        outcome=Outcome::refused;
        if((memory.load8(mode,at(memory.load32(game,globals::game),game_mode))&0xffu)==kept_mode) {
            const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
            const auto object=memory.load32(object_address,datums_address+header_size*(kept.object&0xffffu)+header_address);
            if(memory.load32(object_link,at(object,link))!=none)outcome=Outcome::refused_kept;
        }
        if(outcome==Outcome::refused)frame.refused(kept);
    }
    frame.finish(kept);
    return outcome;
}
} // namespace object_placement
} // namespace halo2::native_object_placement
