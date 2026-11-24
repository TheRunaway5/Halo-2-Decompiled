#pragma once
// Native TU1.5 unit chain step, entered at D2B00 (physics live support unit):
// the next unit in a chain of objects.
//
//   unit_chain::next   D2B00(ESI = chain cursor) -> EAX the unit, or 0
//
// The cursor holds the found unit's handle (+4), a word of it (+8) and the
// next handle in the chain (+0xC, NONE at its end). Each step takes the
// object the next handle names, advances the cursor to that object's own
// next (+0xC), and stops at the first unit: an object whose type (+0xAA) is
// a biped or a vehicle (1 << type within the low two bits). Then its handle
// and its word +0x1FC are stored in the cursor and the object is the result.
// The cursor is read again for every step.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_unit_chain.hpp) maps the role to the original instruction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_chain {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u;
namespace cursor {inline constexpr std::uint32_t handle=4u,word=8u,next=0xcu;}
namespace object {inline constexpr std::uint32_t next=0xcu,type=0xaau,word=0x1fcu;}
inline constexpr std::uint32_t unit_types=(1u<<0)|(1u<<1);   // biped, vehicle

namespace unit_chain {
enum class Access {first_next,headers,next_handle,header_datums,object_address,object_next,next_store,object_type,
    next_check,handle_store,unit_word,word_store};
struct Result {bool found;std::uint32_t unit;};
template<class Memory> Result next(Memory &memory,std::uint32_t chain) {
    using enum Access;
    if(memory.load32(first_next,at(chain,cursor::next))==none)return {false,0u};
    const auto table=memory.load32(headers,globals::object_headers);
    for(;;) {
        const auto handle=memory.load32(next_handle,at(chain,cursor::next));
        const auto datums=memory.load32(header_datums,at(table,datum_data));
        const auto address=memory.load32(object_address,datums+header_size*(handle&0xffffu)+header_address);
        memory.store32(next_store,at(chain,cursor::next),memory.load32(object_next,at(address,object::next)));
        const auto type=memory.load8(object_type,at(address,object::type))&0xffu;
        if((1u<<(type&31u))&unit_types) {
            memory.store32(handle_store,at(chain,cursor::handle),handle);
            memory.store16(word_store,at(chain,cursor::word),memory.load16(unit_word,at(address,object::word)));
            return {true,address};
        }
        if(memory.load32(next_check,at(chain,cursor::next))==none)return {false,0u};
    }
}
} // namespace unit_chain
} // namespace halo2::native_unit_chain
