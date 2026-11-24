#pragma once
// Native TU1.5 body matrix, entered at 1D8B90 (scene device constructor
// unit): a physics body's transform as a real_matrix4x3.
//
//   body_matrix::convert   1D8B90(EAX = body index, ECX = model, ESI = matrix*)
//
// The model's bodies (+0x70, 0x60-byte entries) name the body (+0x40), whose
// motion (+0x3C) holds the transform: four 16-byte rows at +0x80 (three
// axes, then the position). The rows are copied to an aligned matrix on the
// stack; with the model's offset bit (bit 6 of +4) 2E4650 composes it in
// place with the model's offset transform (+0x30). The output takes scale
// one ([46A0D0]) and each row's first three components.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores (the rows as
// the original's 16-byte moves); every access names its role (Access); the
// boundary adapter (native_tu15_body_matrix.hpp) maps the role to the
// original instruction and owns the ABI of 2E4650. The values the original
// keeps in callee-saved registers across it are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <xmmintrin.h>

namespace halo2::native_body_matrix {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u;}
namespace model {inline constexpr std::uint32_t flags=4u,offset=0x30u,bodies=0x70u;inline constexpr unsigned offset_bit=6u;}
namespace body {inline constexpr std::uint32_t size=0x60u,entry=0x40u,motion=0x3cu,transform=0x80u;}
// The transform's rows and the real_matrix4x3 (the scale, then three components of each row).
inline constexpr std::uint32_t row_size=0x10u,rows=4u,columns=3u,component=4u;
namespace matrix {inline constexpr std::uint32_t scale=0u,first=4u;}

namespace body_matrix {
enum class Access {bodies,body_entry,motion,row_load,flags,row_store,one,scale_store,element_load,element_store};
enum class Outcome {plain,offset};
// The original's ESI (the output) across 2E4650.
struct Kept {std::uint32_t matrix;};
// Frame: framing() the aligned stack matrix; local(offset) on it; indexed(index), rows(motion), offset_tested(flags),
// composed(offset, kept) [2E4650], restoring().
template<class Memory,class Frame> Outcome convert(Memory &memory,Frame &frame,std::uint32_t index,std::uint32_t model_address,Kept kept) {
    using enum Access;
    frame.framing();
    const auto bodies_address=memory.load32(bodies,at(model_address,model::bodies));
    const auto body_address=memory.load32(body_entry,frame.indexed(index)+bodies_address+body::entry);
    const auto motion_address=memory.load32(motion,at(body_address,body::motion));
    // The transform's four rows, copied to the stack.
    const auto first_row=memory.load128(row_load,motion_address+body::transform);
    const auto flags_value=memory.load32(flags,at(model_address,model::flags));
    const auto rows_address=frame.rows(motion_address);
    memory.store128(row_store,frame.local(0u),first_row);
    const auto second_row=memory.load128(row_load,rows_address+row_size);
    memory.store128(row_store,frame.local(row_size),second_row);
    const auto third_row=memory.load128(row_load,rows_address+2u*row_size);
    const bool offset=frame.offset_tested(flags_value)&1u;
    memory.store128(row_store,frame.local(2u*row_size),third_row);
    const auto fourth_row=memory.load128(row_load,rows_address+3u*row_size);
    memory.store128(row_store,frame.local(3u*row_size),fourth_row);
    if(offset)frame.composed(at(model_address,model::offset),kept);
    // Scale one, then each row's first three components.
    const auto one_value=memory.load32(one,globals::one);
    memory.store32(scale_store,at(kept.matrix,matrix::scale),one_value);
    for(std::uint32_t row=0;row<rows;++row)for(std::uint32_t column=0;column<columns;++column) {
        const auto value=memory.load32(element_load,frame.local(row_size*row+component*column));
        memory.store32(element_store,at(kept.matrix,matrix::first+component*(columns*row+column)),value);
    }
    frame.restoring();
    return offset?Outcome::offset:Outcome::plain;
}
} // namespace body_matrix
} // namespace halo2::native_body_matrix
