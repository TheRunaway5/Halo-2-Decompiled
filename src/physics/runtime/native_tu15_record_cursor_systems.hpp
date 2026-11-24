#pragma once
// Native TU1.5 record cursor step, entered at 1E5160 (physics live support
// unit): a cursor advanced to the next 40-byte record of a list.
//
//   record_cursor::advance   1E5160(EAX = cursor*, EDX = list*) -> AL
//
// The cursor's index word (+0x2A) moves one on; past the list's count
// (+0x3C, signed) the result is 0. Otherwise the record (the list's +0x40,
// 40 bytes each) is copied into the cursor: its handle (+4 to +0x24), then
// +0x14, +0x10, +8 and +0xC (to +0x10, +0xC, +0 and +8), the index (to
// +0x20), then +0x18, +0x24, +0x20 and +0x1C (to +4, +0x1C, +0x18 and
// +0x14). Unless the cursor's word +0x26 (the handle's high word, read
// after it is stored) is NONE, 1E5C80 gets the handle. The result is 1.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_record_cursor.hpp) maps the role to the original
// instruction and owns the ABI of 1E5C80.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_record_cursor {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace list {inline constexpr std::uint32_t count=0x3cu,records=0x40u,record_size=0x28u;}
namespace cursor {inline constexpr std::uint32_t handle=0x24u,handle_high=0x26u,index=0x20u,index_word=0x2au;}
inline constexpr std::uint32_t record_handle=4u;
struct Copy {std::uint32_t from,to;};
inline constexpr Copy early_copies[]{{0x14u,0x10u},{0x10u,0x0cu},{0x08u,0x00u},{0x0cu,0x08u}};
inline constexpr Copy late_copies[]{{0x18u,0x04u},{0x24u,0x1cu},{0x20u,0x18u},{0x1cu,0x14u}};

namespace record_cursor {
enum class Access {index_word,count,index_store,records,handle,handle_store,handle_high,copy,copy_store,index,handle_again};
// Frame: copied(k) before the k-th copy (0-7); flagged(high) the handle's
// high word tested; notify(handle) 1E5C80.
template<class Memory,class Frame> bool advance(Memory &memory,Frame &frame,std::uint32_t cursor_address,std::uint32_t list_address) {
    using enum Access;
    const auto next=(memory.load16(Access::index_word,at(cursor_address,cursor::index_word))+1u)&0xffffu;
    const auto index_value=widen16(next);
    if(signed_value(index_value)>=signed_value(memory.load32(Access::count,at(list_address,list::count))))return false;
    memory.store16(index_store,at(cursor_address,cursor::index_word),next);
    const auto record=list::record_size*index_value+memory.load32(records,at(list_address,list::records));
    memory.store32(handle_store,at(cursor_address,cursor::handle),memory.load32(Access::handle,at(record,record_handle)));
    const auto high=memory.load16(handle_high,at(cursor_address,cursor::handle_high))&0xffffu;
    frame.flagged(high);
    std::uint32_t k=0;
    for(const auto copy:early_copies){frame.copied(k++);memory.store32(copy_store,at(cursor_address,copy.to),memory.load32(Access::copy,at(record,copy.from)));}
    memory.store32(index,at(cursor_address,cursor::index),index_value);
    for(const auto copy:late_copies){frame.copied(k++);memory.store32(copy_store,at(cursor_address,copy.to),memory.load32(Access::copy,at(record,copy.from)));}
    if(high!=0xffffu)frame.notify(memory.load32(handle_again,at(cursor_address,cursor::handle)));
    return true;
}
} // namespace record_cursor
} // namespace halo2::native_record_cursor
