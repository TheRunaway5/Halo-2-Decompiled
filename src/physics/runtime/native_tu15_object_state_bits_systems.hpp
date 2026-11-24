#pragma once
// Native TU1.5 object state bits, entered at 16E290 (physics live support
// unit): an object's state summarized in a small bit set.
//
//   object_state_bits::of   16E290(EAX = object handle) -> EAX
//
//   bit 0  the header's +2 bit 4, or the object's +4 bit 0
//   bit 1  the object's +4 bit 20
//   bit 2  not +4 bit 22
//   bit 3  not +4 bit 31
// and by the object's type (the signed byte +0xAA):
//   type 0   bit 4 the byte +0x10A bit 2, bit 5 the byte +0x348 bit 3
//   type 11  bit 6 the dword +0x12C bit 1
// The original sets and clears each bit with OR and AND masks that also
// clear EAX above bit 15.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_state_bits.hpp) maps the role to the original
// instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_state_bits {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_flags=2u,header_address=8u;
inline constexpr std::uint32_t header_bit=0x10u;
namespace object {inline constexpr std::uint32_t flags=4u,type=0xaau,details=0x10au,state=0x348u,kind_flags=0x12cu;}
inline constexpr std::uint32_t first_type=0u,eleventh_type=11u;

namespace object_state_bits {
enum class Access {headers,header_datums,object_address,header_flags,type,object_byte,object_flags,details,state,kind_flags};
enum class Last {other_type,first_set,first_clear,eleventh_set,eleventh_clear};
struct Result {std::uint32_t bits;Last last;};
// Frame:
//   first(bits)            MOV EAX,1 when bit 0 is set
//   tested(shift)          MOV ECX,ESI; SHR ECX,shift; TEST CL,1 (the flags word)
//   applied(bits)          the OR or AND that leaves the bits
//   type_tested(), eleventh_tested()   TEST EDI,EDI; CMP EDI,0B
template<class Memory,class Frame> Result of(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto set=[](std::uint32_t bits,bool on,std::uint32_t mask){return on?bits|mask:bits&(0xffffu&~mask);};
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    const auto header=datums+header_size*(handle&0xffffu);
    const auto address=memory.load32(object_address,at(header,header_address));
    const auto header_value=memory.load8(Access::header_flags,at(header,native_object_state_bits::header_flags));
    const auto type_value=std::int32_t(std::int8_t(memory.load8(type,at(address,object::type))));
    std::uint32_t bits=0;
    if((header_value&header_bit) || (memory.load8(object_byte,at(address,object::flags))&1u)){bits=1u;frame.first(bits);}
    const auto flags_value=memory.load32(object_flags,at(address,object::flags));
    frame.tested(20u);bits=set(bits,(flags_value>>20)&1u,2u);frame.applied(bits);
    frame.tested(22u);bits=set(bits,!((flags_value>>22)&1u),4u);frame.applied(bits);
    frame.tested(31u);if(!((flags_value>>31)&1u)){bits|=8u;frame.applied(bits);}
    frame.type_tested();
    if(type_value==std::int32_t(first_type)) {
        bits=set(bits,(memory.load8(details,at(address,object::details))>>2)&1u,0x10u);frame.applied(bits);
        const bool on=(memory.load8(state,at(address,object::state))>>3)&1u;
        bits=set(bits,on,0x20u);
        return {bits,on?Last::first_set:Last::first_clear};
    }
    frame.eleventh_tested();
    if(type_value==std::int32_t(eleventh_type)) {
        const bool on=(memory.load32(kind_flags,at(address,object::kind_flags))>>1)&1u;
        bits=set(bits,on,0x40u);
        return {bits,on?Last::eleventh_set:Last::eleventh_clear};
    }
    return {bits,Last::other_type};
}
} // namespace object_state_bits
} // namespace halo2::native_object_state_bits
