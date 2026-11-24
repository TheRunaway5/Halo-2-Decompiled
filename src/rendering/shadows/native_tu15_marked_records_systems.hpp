#pragma once
// Native TU1.5 marked record pass, entered at 179A40 (shadow frame and lens
// flare units): every in-use record of the data array at 4F720C that is
// enabled and not held back is handed to 17C710 and 17CD10.
//
//   marked_records::each   179A40
//
// The records are 400 bytes. The first in-use one comes from the data array
// iterator 16EA60; after each record the in-use bits (+0x48) are scanned
// from the next index below the first unallocated index (+0x38). A record
// with bit 0 of its byte at +2 set, and bits 2, 3 and 6 of that byte clear
// (read again as a word), goes to 17C710 (its handle and address) and then
// 17CD10 (its address); the array is read again after them. Handles are
// the record's 16-bit salt (+0) above its index.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_marked_records.hpp) maps the role to the original
// instruction and owns the ABI of the children; the values the original
// keeps in callee-saved registers across them are a Kept, refreshed from
// the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_marked_records {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

namespace globals {inline constexpr std::uint32_t records=0x4f720cu;}   // DataArray* of 400-byte records
inline constexpr std::uint32_t record_size=0x190u,record_flags=2u;
namespace flag {inline constexpr std::uint32_t enabled=1u,held_back=(1u<<2)|(1u<<3)|(1u<<6);}

namespace marked_records {
enum class Access {
    array,first_size,first_data,first_salt,data,byte_flags,word_flags,array_reload,bound,bits,bit_word,next_size,
    next_data,next_salt
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t array;    // EBX
    std::uint32_t handle;   // EBP: the current record's handle
    std::uint32_t index;    // EDI: its index (the bound while scanning)
    std::uint32_t record;   // ESI: its address (the found index after a scan)
};
enum class Exit {none_in_use,walked};
// Frame:
//   first(kept)                 16EA60 from index 0; returns EAX
//   handled(kept, handle)       a handle was made (EBP)
//   visit(kept)                 17C710 then 17CD10 for the record
//   next(kept, start)           the next index's start (EAX; ESI NONE)
//   scanned(found)              the scan's end (ESI the found index)
template<class Memory,class Frame> Exit each(Memory &memory,Frame &frame) {
    using enum Access;
    Kept kept{};
    kept.array=memory.load32(array,globals::records);
    const auto first=frame.first(kept);
    std::uint32_t handle=none;
    if(first!=none) {
        const auto offset=memory.load32(first_size,at(kept.array,offsetof(DataArray,datum_size)))*first;
        handle=(widen16(memory.load16(first_salt,offset+memory.load32(first_data,at(kept.array,offsetof(DataArray,data)))))<<16)|first;
    }
    frame.handled(kept,handle);
    if(kept.handle==none)return Exit::none_in_use;
    for(;;) {
        const auto data_now=memory.load32(data,at(kept.array,offsetof(DataArray,data)));
        kept.index=kept.handle&0xffffu;
        const auto offset=kept.index*record_size;
        const auto flags_byte=memory.load8(byte_flags,offset+data_now+record_flags);
        kept.record=offset+data_now;
        if(flags_byte&flag::enabled) {
            if(!(memory.load16(word_flags,at(kept.record,record_flags))&flag::held_back)) {
                frame.visit(kept);
                kept.array=memory.load32(array_reload,globals::records);
            }
        }
        // The next in-use record.
        auto index=kept.handle==none?0u:kept.index+1u;
        frame.next(kept,index);
        std::uint32_t found=none;
        if(signed_value(index)>=0) {
            const auto limit=memory.load32(bound,at(kept.array,offsetof(DataArray,first_unallocated)));
            if(signed_value(index)<signed_value(limit)) {
                const auto in_use=memory.load32(bits,at(kept.array,offsetof(DataArray,in_use_bits)));
                for(;;) {
                    if(memory.load32(bit_word,in_use+std::uint32_t(signed_value(index)>>5)*4u)&(1u<<(index&31u))){found=index;break;}
                    ++index;
                    if(!(signed_value(index)<signed_value(limit)))break;
                }
            }
        }
        frame.scanned(found);
        handle=none;
        if(found!=none) {
            const auto next_offset=memory.load32(next_size,at(kept.array,offsetof(DataArray,datum_size)))*found;
            handle=(widen16(memory.load16(next_salt,next_offset+memory.load32(next_data,at(kept.array,offsetof(DataArray,data)))))<<16)|found;
        }
        frame.handled(kept,handle);
        if(kept.handle==none)return Exit::walked;
    }
}
} // namespace marked_records
} // namespace halo2::native_marked_records
