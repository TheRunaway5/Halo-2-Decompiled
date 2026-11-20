#pragma once
// Native TU1.5 object parent-hierarchy attached predicate: BBD10, called with
// an object handle in ECX across the shadow frame, AI frame, and live-support
// units.
//
//   object_hierarchy_root::is_attached_to_flagged_parent(object_handle) -> bool
//     Walks the object's parent chain (`object->parent_object` at +0x14) in
//     the global object header table ([4ECBD0]):
//       1. Look up the 12-byte object header row (`index = handle & 0xFFFF`,
//          row at `table->data + index * 12`).
//       2. Read the row's flag byte (+2) and object pointer (+8).
//       3. If the header row carries bit 4 (`0x10`), return true (AL = 1).
//       4. If the object's own flag byte (+4) carries bit 0 (`0x01`), return
//          true (AL = 1).
//       5. Advance to `object->parent_object` (+0x14); if it is NONE
//          (`0xFFFFFFFF`), return false (AL = 0), otherwise repeat.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads; every access names its
// role (Access) and the boundary adapter
// (native_tu15_object_hierarchy_root.hpp) maps the role to the original
// instruction. Registers, flags and stack saves belong to the adapter.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_hierarchy_root {
using native_object_update::DataArray;
using native_object_update::ObjectHeader;
using native_object_update::at;
using native_object_update::element;
using native_object_update::handle_index;
using native_object_update::none;
using native_object_update::globals::object_headers;

// ------------------------------------------------------------- game layouts
namespace header_flags {
inline constexpr std::uint8_t parent_flagged_bit=0x10u; // ObjectHeader::flags (+2) bit 4
}
namespace object_flags {
inline constexpr std::uint8_t root_flagged_bit=0x01u;   // Object::flags (+4) bit 0
}
struct Object {
    std::uint32_t definition;       // +0x00
    std::uint8_t flags;             // +0x04: bit 0 (0x01) terminates walk with true
    std::uint8_t unused_here_05[0x0f];
    std::uint32_t parent_object;    // +0x14: parent object handle, or NONE (0xFFFFFFFF)
};
static_assert(offsetof(Object,flags)==0x04 && offsetof(Object,parent_object)==0x14);

// ------------------------------------------------------------ access roles
enum class Access : unsigned {
    table_pointer,table_data,header_flags,header_object,object_flags,parent_handle
};

namespace object_hierarchy_root {
// Frame (the adapter):
//   entry_handle()                  initial object handle in ECX
//   begin_walk(data)                save ESI, set ESI = data, AL = 1
//   step_header(handle, flags, obj) update ECX = header address, EDX = obj, test flags & 0x10
//   finish(result)                  if !result: XOR AL,AL; POP ESI; RET
template<class Memory,class Frame> void check(Memory &memory,Frame &frame) {
    using enum Access;
    const auto table=memory.load32(table_pointer,object_headers);
    const auto data=memory.load32(table_data,at(table,offsetof(DataArray,data)));
    frame.begin_walk(data);
    auto handle=frame.entry_handle();
    for(;;) {
        const auto row=element(data,handle_index(handle),sizeof(ObjectHeader));
        const auto h_flags=memory.load8(Access::header_flags,at(row,offsetof(ObjectHeader,flags)));
        const auto object=memory.load32(header_object,at(row,offsetof(ObjectHeader,object)));
        frame.step_header(row,h_flags,object);
        if(h_flags&header_flags::parent_flagged_bit)return frame.finish(true);
        const auto o_flags=memory.load8(Access::object_flags,at(object,offsetof(Object,flags)));
        if(o_flags&object_flags::root_flagged_bit)return frame.finish(true);
        handle=memory.load32(parent_handle,at(object,offsetof(Object,parent_object)));
        if(handle==none)return frame.finish(false);
    }
}
} // namespace object_hierarchy_root
} // namespace halo2::native_object_hierarchy_root
