#pragma once
// Native TU1.5 record walk, entered at 180520 (combat action unit): every
// in-use record of the data array at 4F7220 is visited.
//
//   record_walk::each   180520
//
// When the array is valid (its byte at +0x29), 43980 runs first; then the
// in-use bits (+0x48) are scanned from index 0 below the array's first
// unallocated index (+0x38, read again before each scan), and each in-use
// record is visited by 17FAF0 with its handle (the record's 16-bit salt at
// +0 above its index) and its address (index * datum size + data, 32-bit).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_walk.hpp) maps the role to the original instruction
// and owns the ABI of the children (43980, 17FAF0); the array and the index
// the original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_record_walk {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

namespace globals {inline constexpr std::uint32_t records=0x4f7220u;}   // DataArray*
inline constexpr std::uint32_t array_valid=0x29u;

namespace record_walk {
enum class Access {array,valid,bound,bits,bit_word,datum_size,datum_data,identifier};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t array;   // EDI
    std::uint32_t index;   // ESI: the last visited index (the bound while scanning)
};
enum class Exit {invalid,walked};
// Frame:
//   prepare(kept)                  43980
//   visit(kept, record, handle)    17FAF0 for the record
template<class Memory,class Frame> Exit each(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    kept.array=memory.load32(array,globals::records);
    if(!memory.load8(valid,at(kept.array,array_valid)))return Exit::invalid;
    frame.prepare(kept);
    kept.index=none;
    for(;;) {
        auto index=kept.index+1u;
        if(signed_value(index)<0)return Exit::walked;
        const auto limit=memory.load32(bound,at(kept.array,offsetof(DataArray,first_unallocated)));
        if(!(signed_value(index)<signed_value(limit)))return Exit::walked;
        const auto in_use=memory.load32(bits,at(kept.array,offsetof(DataArray,in_use_bits)));
        for(;;) {
            if(memory.load32(bit_word,in_use+std::uint32_t(signed_value(index)>>5)*4u)&(1u<<(index&31u)))break;
            ++index;
            if(!(signed_value(index)<signed_value(limit)))return Exit::walked;
        }
        if(index==none)return Exit::walked;
        const auto size=memory.load32(datum_size,at(kept.array,offsetof(DataArray,datum_size)));
        const auto data=memory.load32(datum_data,at(kept.array,offsetof(DataArray,data)));
        const auto offset=size*index;
        const auto salt=widen16(memory.load16(identifier,offset+data));
        kept.index=index;
        frame.visit(kept,offset+data,(salt<<16)|index);
    }
}
} // namespace record_walk
} // namespace halo2::native_record_walk
