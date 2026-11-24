#pragma once
// Native TU1.5 filtered data array iterators: the next in-use record that
// is not passed over.
//
//   filtered_iterator::next   1A8E50(ESI = iterator) -> AL   (physics live
//                             support and death cycle units): records with
//                             bit 1 of their byte at +2 are passed over
//   linked_iterator::next     1A8F10(ESI = iterator) -> AL   (CTF player
//                             iterator unit): records whose link (+0x2C) is
//                             NONE are passed over
//
// The iterator holds the current record's address (+0), the data array
// (+4), the current handle (+8) and index (+0xC). The next in-use index
// after the current one comes from the data array iterator 16EA60, then the
// record (index * datum size + data, 32-bit), its handle (the 16-bit salt at
// +0 above the index) and index are stored; past the last record the index
// becomes the array's maximum count (+0x20), the handle NONE and the record
// zero. While the current record is passed over, the in-use bits (+0x48)
// are scanned from the next index below the first unallocated index (+0x38)
// the same way. The result is whether a record remains.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_filtered_iterator.hpp, native_tu15_linked_iterator.hpp) maps
// the role to the original instruction and owns the ABI of the child 16EA60; the iterator and the
// array the original keeps in callee-saved registers across it are a Kept,
// refreshed from the registers it returned. The iterator is read again for
// every step.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_filtered_iterator {
using native_object_update::DataArray;
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;

struct Iterator {std::uint32_t record,array,handle,index;};
static_assert(offsetof(Iterator,array)==4 && offsetof(Iterator,handle)==8 && offsetof(Iterator,index)==0xc);
inline constexpr std::uint32_t record_flags=2u,passed_over=2u;
inline constexpr std::uint32_t record_link=0x2cu;

namespace filtered_iterator {
enum class Access {
    index_read,array_read,size,data,index_store,salt,handle_store,maximum,index_end,handle_clear,record_store,
    current,filter,index_reload,array_reload,bound,bits,bit_word,next_size,next_data,next_index_store,next_salt,
    next_handle_store,next_maximum,next_index_end,next_handle_clear,next_record_store,final_record
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t iterator;   // ESI
    std::uint32_t array;      // EDI
};
// Frame:
//   entry()             ESI at entry
//   first(kept, start)  16EA60 from start in the iterator's array; returns EAX
// skipped(record): whether the record is passed over (its access the filter).
template<class Memory,class Frame,class Skipped> bool next(Memory &memory,Frame &frame,Skipped skipped) {
    using enum Access;
    Kept kept=frame.entry();
    const auto start=memory.load32(index_read,at(kept.iterator,offsetof(Iterator,index)))+1u;
    kept.array=memory.load32(array_read,at(kept.iterator,offsetof(Iterator,array)));
    const auto found=frame.first(kept,start);
    std::uint32_t record=0;
    // The record an index names, stored into the iterator (or its end).
    const auto settle=[&](std::uint32_t array,std::uint32_t index,Access size_role,Access data_role,Access index_role,
                          Access salt_role,Access handle_role) {
        const auto size_value=memory.load32(size_role,at(array,offsetof(DataArray,datum_size)));
        const auto address=size_value*index+memory.load32(data_role,at(array,offsetof(DataArray,data)));
        memory.store32(index_role,at(kept.iterator,offsetof(Iterator,index)),index);
        memory.store32(handle_role,at(kept.iterator,offsetof(Iterator,handle)),(widen16(memory.load16(salt_role,address))<<16)|index);
        return address;
    };
    const auto finish=[&](std::uint32_t array,Access maximum_role,Access index_role,Access handle_role) {
        memory.store32(index_role,at(kept.iterator,offsetof(Iterator,index)),
                       memory.load32(maximum_role,at(array,offsetof(DataArray,maximum_count))));
        memory.store32(handle_role,at(kept.iterator,offsetof(Iterator,handle)),none);
        return 0u;
    };
    record=found!=none?settle(kept.array,found,size,data,index_store,salt,handle_store)
                      :finish(kept.array,maximum,index_end,handle_clear);
    memory.store32(record_store,at(kept.iterator,offsetof(Iterator,record)),record);
    if(record) {
        while(skipped(memory.load32(current,at(kept.iterator,offsetof(Iterator,record))))) {
            auto index=memory.load32(index_reload,at(kept.iterator,offsetof(Iterator,index)))+1u;
            const auto array=memory.load32(array_reload,at(kept.iterator,offsetof(Iterator,array)));
            std::uint32_t next_found=none;
            if(signed_value(index)>=0) {
                const auto limit=memory.load32(bound,at(array,offsetof(DataArray,first_unallocated)));
                if(signed_value(index)<signed_value(limit)) {
                    const auto in_use=memory.load32(bits,at(array,offsetof(DataArray,in_use_bits)));
                    for(;;) {
                        if(memory.load32(bit_word,in_use+std::uint32_t(signed_value(index)>>5)*4u)&(1u<<(index&31u))){next_found=index;break;}
                        ++index;
                        if(!(signed_value(index)<signed_value(limit)))break;
                    }
                }
            }
            record=next_found!=none?settle(array,next_found,next_size,next_data,next_index_store,next_salt,next_handle_store)
                                   :finish(array,next_maximum,next_index_end,next_handle_clear);
            memory.store32(next_record_store,at(kept.iterator,offsetof(Iterator,record)),record);
            if(!record)break;
        }
    }
    return memory.load32(final_record,at(kept.iterator,offsetof(Iterator,record)))!=0;
}
// 1A8E50: records with the pass-over flag are skipped.
template<class Memory,class Frame> bool next(Memory &memory,Frame &frame) {
    return next(memory,frame,[&](std::uint32_t record){return (memory.load8(Access::filter,at(record,record_flags))&passed_over)!=0;});
}
} // namespace filtered_iterator

namespace linked_iterator {
using filtered_iterator::Access;
using filtered_iterator::Kept;
// 1A8F10: records without a link are skipped.
template<class Memory,class Frame> bool next(Memory &memory,Frame &frame) {
    return filtered_iterator::next(memory,frame,[&](std::uint32_t record){return memory.load32(Access::filter,at(record,record_link))==none;});
}
} // namespace linked_iterator
} // namespace halo2::native_filtered_iterator
