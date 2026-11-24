#pragma once
// Native TU1.5 shape copy, entered at C3960 (light shape, death cycle,
// shadow frame and map world support units): a light's shape into a
// 124-byte record.
//
//   shape_copy::copy   C3960(EAX = source, EDX = record)
//
// The record (31 dwords) is cleared as REP STOSD clears it, in the direction
// the string direction flag gives; then it takes the source's kind (int16
// +4, widened). A kind 0 shape takes the source's +0x18 and +0x1C after it;
// any other the five dwords at the source's +0x20.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter (native_tu15_shape_copy.hpp)
// maps the role to the original instruction and owns the string
// instruction's registers.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_shape_copy {
using native_object_update::at;
using native_object_update::widen16;
inline constexpr std::uint32_t record_words=31u,shape_words=5u;
namespace source {inline constexpr std::uint32_t kind=4u,first=0x18u,second=0x1cu,shape=0x20u;}

namespace shape_copy {
enum class Access {clear,kind,kind_store,first,first_store,second,second_store,shape_read,shape_write};
enum class Outcome {simple,shaped};
// Frame: step() the direction flag's step; clear_started() the REP STOSD's registers;
// leaving() the saved registers back before the last store.
template<class Memory,class Frame> Outcome copy(Memory &memory,Frame &frame,std::uint32_t from,std::uint32_t record) {
    using enum Access;
    frame.clear_started();
    const auto step=frame.step();
    for(std::uint32_t word=0;word<record_words;++word)memory.store32(clear,record+word*step,0u);
    const auto kind_value=widen16(memory.load16(kind,at(from,source::kind)));
    memory.store32(kind_store,record,kind_value);
    if(!(kind_value&0xffffu)) {
        memory.store32(first_store,at(record,4u),memory.load32(first,at(from,source::first)));
        const auto second_value=memory.load32(second,at(from,source::second));
        frame.leaving();
        memory.store32(second_store,at(record,8u),second_value);
        return Outcome::simple;
    }
    for(std::uint32_t word=0;word<shape_words;++word) {
        const auto value=memory.load32(shape_read,at(from,source::shape+4u*word));
        if(word+1u==shape_words)frame.leaving();
        memory.store32(shape_write,at(record,4u+4u*word),value);
    }
    return Outcome::shaped;
}
} // namespace shape_copy
} // namespace halo2::native_shape_copy
