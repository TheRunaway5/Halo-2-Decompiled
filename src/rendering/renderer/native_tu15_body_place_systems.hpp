#pragma once
// Native TU1.5 body place, entered at 1D9190 (scene device constructor
// unit): a physics body placed at a real_matrix4x3 (the inverse of the body
// matrix, 1D8B90).
//
//   body_place::place   1D9190(EAX = matrix*, ESI = model, EDI = body index)
//
// The matrix's forward, left and up axes and its position become the four
// rows of a transform on an aligned stack frame (each row's fourth
// component 0); the time globals' rate (the signed word +2 of [51DF54]) is
// kept as a float. With the model's offset bit (bit 6 of +4), 2E4470 (through
// the dispatch) writes the inverse of the model's offset transform (+0x30)
// and 2E4650 composes the rows with it. 2E4350 (through the dispatch) turns
// the rows into a quaternion; 2E58F0 places the body (the model's bodies
// +0x70, 0x60-byte entries, the body at +0x40) at the position and
// quaternion with the rate.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores (the position
// row as the original's 16-byte move); every access names its role
// (Access), the components in the original's order of moves; the boundary
// adapter (native_tu15_body_place.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_body_place {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t time=0x51df54u;}
namespace time_globals {inline constexpr std::uint32_t rate=2u;}
namespace model {inline constexpr std::uint32_t flags=4u,offset=0x30u,bodies=0x70u;inline constexpr unsigned offset_bit=6u;}
namespace body {inline constexpr std::uint32_t size=0x60u,entry=0x40u;}
// real_matrix4x3: the scale, then forward, left, up and position (three components each).
namespace matrix {inline constexpr std::uint32_t forward=4u,left=0x10u,up=0x1cu,position=0x28u;}
inline constexpr std::uint32_t component=4u,last=0xcu;
// The aligned stack frame: the rate, the staged position (then the
// quaternion), the transform's rows and the offset's inverse.
namespace local {inline constexpr std::uint32_t rate=0xcu,staged=0x10u,quaternion=0x10u,forward=0x20u,left=0x30u,up=0x40u,position=0x50u,
    inverse=0x60u,size=0xa0u;}

namespace body_place {
enum class Access {component_load,time_address,rate_word,component_store,rate_store,flags,position_load,position_store,bodies,body_entry,
    rate_load};
enum class Outcome {plain,offset};
// The original's ESI (the model) and EDI (the body index) across the children.
struct Kept {std::uint32_t model,index;};
// Frame: framing() the aligned stack frame; local(offset) on it; rate(word) CVTSI2SS; offset_tested(flags);
// offset(kept) [2E4470, 2E4650]; rotation(kept) [2E4350]; indexed(index); placed(body, rate, kept) [2E58F0]; restoring().
template<class Memory,class Frame> Outcome place(Memory &memory,Frame &frame,std::uint32_t matrix_address,Kept kept) {
    using enum Access;
    frame.framing();
    const auto component_of=[&](std::uint32_t offset){return memory.load32(component_load,matrix_address+offset);};
    const auto put=[&](std::uint32_t offset,std::uint32_t value){memory.store32(component_store,frame.local(offset),value);};
    // The rows, in the original's order of moves.
    const auto left_x=component_of(matrix::left);
    const auto time_value=memory.load32(time_address,globals::time);
    const auto rate_value=memory.load16(rate_word,time_value+time_globals::rate);
    put(local::left,left_x);
    put(local::left+component,component_of(matrix::left+component));
    put(local::left+2u*component,component_of(matrix::left+2u*component));
    put(local::up,component_of(matrix::up));
    put(local::up+component,component_of(matrix::up+component));
    const auto up_z=component_of(matrix::up+2u*component);
    memory.store32(rate_store,frame.local(local::rate),frame.rate(rate_value));
    const auto forward_x=component_of(matrix::forward);
    put(local::up+2u*component,up_z);
    const auto position_x=component_of(matrix::position);
    put(local::forward,forward_x);
    const auto forward_y=component_of(matrix::forward+component);
    put(local::staged,position_x);
    const auto position_y=component_of(matrix::position+component);
    put(local::forward+component,forward_y);
    const auto forward_z=component_of(matrix::forward+2u*component);
    put(local::staged+component,position_y);
    const auto position_z=component_of(matrix::position+2u*component);
    const auto flags_value=memory.load32(flags,at(kept.model,model::flags));
    put(local::forward+2u*component,forward_z);
    const bool offset=frame.offset_tested(flags_value)&1u;
    put(local::forward+last,0u);
    put(local::left+last,0u);
    put(local::up+last,0u);
    put(local::staged+2u*component,position_z);
    put(local::staged+last,0u);
    const auto position_row=memory.load128(position_load,frame.local(local::staged));
    memory.store128(position_store,frame.local(local::position),position_row);
    // The offset's inverse composed in; the rows as a quaternion; the body placed.
    if(offset)frame.offset(at(kept.model,model::offset),kept);
    frame.rotation(kept);
    const auto bodies_address=memory.load32(bodies,at(kept.model,model::bodies));
    const auto body_address=memory.load32(body_entry,frame.indexed(kept.index)+bodies_address+body::entry);
    const auto rate_now=memory.load32(rate_load,frame.local(local::rate));
    frame.placed(body_address,rate_now,kept);
    frame.restoring();
    return offset?Outcome::offset:Outcome::plain;
}
} // namespace body_place
} // namespace halo2::native_body_place
