#pragma once
// Native TU1.5 object type callbacks, entered at 10E1F0 (physics live
// support unit): every type in an object's type chain is told.
//
//   object_type_callbacks::run   10E1F0(EBX = object handle; first, second)
//
// The object's type (the signed byte +0xAA) names its type definition
// (474B90); for each type definition in that definition's type chain
// (+0x84, null-terminated), in order, the callback +0x74, when there is one,
// is called with (object handle, first, second).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_type_callbacks.hpp) maps the role to the original
// instruction and owns the ABI of the callbacks. The definition and the
// chain position the original keeps in callee-saved registers across them
// are a Kept, refreshed from the registers each callback returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_type_callbacks {
using native_object_update::ObjectTypeDefinition;
using native_object_update::at;
using native_object_update::object_of;
using native_object_update::widen16;
namespace globals {using native_object_update::globals::object_type_definitions;}
inline constexpr std::uint32_t object_type=0xaau,type_callback=0x74u;

namespace object_type_callbacks {
enum class Local {first,second};
enum class Access {
    object_table,object_datums,object_address,type,second_argument,definition,chain_first,chain_entry,callback,
    first_argument,chain_next
};
// The values the original keeps in callee-saved registers.
struct Kept {
    std::uint32_t definition;   // EDI: the object's type definition
    std::uint32_t position;     // ESI: the position in its type chain
};
// Frame:
//   local(Local)               an argument's address now
//   call(callback, kept)       the callback(EBX the handle, first, second);
//                              refreshes kept
template<class Memory,class Frame> void run(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto type_value=std::uint32_t(std::int32_t(std::int8_t(memory.load8(type,at(object,object_type)))));
    (void)memory.load32(second_argument,local(Local::second));
    Kept kept{memory.load32(definition,globals::object_type_definitions+4u*type_value),0u};
    const auto chain=[&]{return at(kept.definition,offsetof(ObjectTypeDefinition,type_chain)+4u*widen16(kept.position));};
    auto entry=memory.load32(chain_first,chain());
    while(entry!=0) {
        const auto chained=memory.load32(chain_entry,chain());
        const auto callback_value=memory.load32(callback,at(chained,type_callback));
        if(callback_value!=0) {
            (void)memory.load32(first_argument,local(Local::first));
            frame.call(callback_value,kept);
        }
        ++kept.position;
        entry=memory.load32(chain_next,chain());
    }
}
} // namespace object_type_callbacks
} // namespace halo2::native_object_type_callbacks
