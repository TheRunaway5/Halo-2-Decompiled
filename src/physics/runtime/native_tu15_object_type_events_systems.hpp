#pragma once
// Native TU1.5 object type events, entered at 10E340 (death cycle and
// physics live support units): every type in an object's type chain is
// told of an event.
//
//   object_type_events::run   10E340(EDI = object handle, EBX = value;
//                             first, second)
//
// As 10E1F0 does with +0x74: the object's type (the signed byte +0xAA)
// names its type definition (474B90); for each type definition in that
// definition's type chain (+0x84, null-terminated), in order, the callback
// +0x68, when there is one, is called with (object handle, first, second,
// value).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_object_type_events.hpp) maps the role to the original
// instruction and owns the ABI of the callbacks. The chain position the
// original keeps in ESI across them is a Kept, refreshed from the registers
// each callback returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_type_events {
using native_object_update::ObjectTypeDefinition;
using native_object_update::at;
using native_object_update::object_of;
namespace globals {using native_object_update::globals::object_type_definitions;}
inline constexpr std::uint32_t object_type=0xaau,type_event=0x68u;

namespace object_type_events {
enum class Local {first,second};
enum class Access {object_table,object_datums,object_address,type,definition,chain_first,second_argument,chain_entry,
    callback,first_argument,chain_next};
// The chain slot the original keeps in ESI.
struct Kept {std::uint32_t slot;};
// Frame:
//   local(Local)            an argument's address now
//   call(callback, kept)    the callback(EDI the handle, first, second,
//                           EBX); refreshes kept
template<class Memory,class Frame> void run(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto local=[&](Local l){return frame.local(l);};
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto type_value=std::uint32_t(std::int32_t(std::int8_t(memory.load8(type,at(object,object_type)))));
    const auto definition_address=memory.load32(definition,globals::object_type_definitions+4u*type_value);
    Kept kept{at(definition_address,offsetof(ObjectTypeDefinition,type_chain))};
    const auto first_entry=memory.load32(chain_first,kept.slot);
    (void)memory.load32(second_argument,local(Local::second));
    if(first_entry==0)return;
    for(;;) {
        const auto chained=memory.load32(chain_entry,kept.slot);
        const auto callback_value=memory.load32(callback,at(chained,type_event));
        if(callback_value!=0) {
            (void)memory.load32(first_argument,local(Local::first));
            frame.call(callback_value,kept);
        }
        const auto next=memory.load32(chain_next,at(kept.slot,4u));
        kept.slot+=4u;
        if(next==0)return;
    }
}
} // namespace object_type_events
} // namespace halo2::native_object_type_events
