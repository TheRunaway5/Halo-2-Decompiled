#pragma once
// Native TU1.5 placement check, entered at 175300 (physics live support
// unit): whether a placement record is sane.
//
//   placement_check::sane   175300(EBX = record) -> AL
//
// No record is not sane; a record without its check flag (bit 0 of +0) is.
// Otherwise every part must pass, in the original's order: the two unit
// vectors (+0x2C, +0x38; B1720), the two points (+4, +0x10; 1751B0 each,
// within the world bounds), the vector at +0x44 (B16C0, finite) and the
// three scalars at +0x24, +0x28 and +0x88 (175280, 175240 and 1752C0, each
// finite and within its own range).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_placement_check.hpp) maps the role to the original
// instruction and owns the ABI of the checks; the record the original keeps
// in EBX across them is a Kept, refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_placement_check {
using native_object_update::at;
namespace record {
inline constexpr std::uint32_t flags=0u,first_point=4u,second_point=0x10u,first_scalar=0x24u,second_scalar=0x28u,
    first_vector=0x2cu,second_vector=0x38u,finite_vector=0x44u,third_scalar=0x88u;
inline constexpr std::uint32_t checked_bit=1u;
}

namespace placement_check {
enum class Access {flags,first_scalar,second_scalar,third_scalar};
enum class Check {unit_vectors,first_point,second_point,finite_vector,first_scalar,second_scalar,third_scalar};
struct Kept {std::uint32_t record;};   // EBX
// Frame:
//   present(record)            TEST EBX,EBX
//   check(kept, Check, part)   the check's child on the part (an address, or
//                              a scalar's bits); refreshes kept; returns AL != 0
template<class Memory,class Frame> bool sane(Memory &memory,Frame &frame,std::uint32_t address) {
    using enum Access;
    Kept kept{address};
    if(!frame.present(kept.record))return false;
    if(!(memory.load8(flags,at(kept.record,record::flags))&record::checked_bit))return true;
    if(!frame.check(kept,Check::unit_vectors,kept.record))return false;
    if(!frame.check(kept,Check::first_point,at(kept.record,record::first_point)))return false;
    if(!frame.check(kept,Check::second_point,at(kept.record,record::second_point)))return false;
    if(!frame.check(kept,Check::finite_vector,at(kept.record,record::finite_vector)))return false;
    if(!frame.check(kept,Check::first_scalar,memory.load32(first_scalar,at(kept.record,record::first_scalar))))return false;
    if(!frame.check(kept,Check::second_scalar,memory.load32(second_scalar,at(kept.record,record::second_scalar))))return false;
    return frame.check(kept,Check::third_scalar,memory.load32(third_scalar,at(kept.record,record::third_scalar)));
}
} // namespace placement_check
} // namespace halo2::native_placement_check
