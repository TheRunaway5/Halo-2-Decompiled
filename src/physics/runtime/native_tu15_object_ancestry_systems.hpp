#pragma once
// Native TU1.5 object ancestry, entered at BCFE0 (physics live support
// unit): whether an object is another object or one of its parents.
//
//   object_ancestry::within   BCFE0(EAX = object, ESI = ancestor) -> EAX
//
// The object's parent chain (+0x14 of each object's data, NONE at the top)
// is walked from the object until it reaches the ancestor or the top; the
// result is 1 when the ancestor is not NONE and was reached, otherwise 0.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_ancestry.hpp) maps the role to the original
// instruction. The header table's datums are read again for every step.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_object_ancestry {
using native_object_update::at;
using native_object_update::none;

namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_data=8u,object_parent=0x14u;

namespace object_ancestry {
enum class Access {headers,rows,object_data,parent};
// The walk's end: the last object visited (the original's ECX), and whether
// the walk stopped at the ancestor (rather than at the top).
struct Walk {std::uint32_t visited;bool stopped,within;};
template<class Memory> Walk within(Memory &memory,std::uint32_t object,std::uint32_t ancestor) {
    using enum Access;
    std::uint32_t visited=none;
    bool stopped=false;
    if(object!=none) {
        const auto table=memory.load32(headers,globals::object_headers);
        for(auto current=object;;) {
            if(visited==ancestor){stopped=true;break;}
            const auto rows_table=memory.load32(rows,at(table,datum_data));
            visited=current;
            current=memory.load32(parent,at(memory.load32(object_data,rows_table+(current&0xffffu)*header_size+header_data),object_parent));
            if(current==none)break;
        }
    }
    return {visited,stopped,ancestor!=none && visited==ancestor};
}
} // namespace object_ancestry
} // namespace halo2::native_object_ancestry
