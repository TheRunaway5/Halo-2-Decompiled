#pragma once
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_object_type_notification {
using native_object_update::ObjectTypeDefinition;
using native_object_update::object_of;
using native_object_update::widen8;
using native_object_update::widen16;
inline constexpr std::uint32_t object_type=0xaau,notify_callback=0x58u;
inline constexpr std::uint32_t chain=offsetof(ObjectTypeDefinition,type_chain);

enum class Access {object_table,object_datums,object_address,type,argument,definition,first_type,chain_type,callback,next_type};
struct Chain {
    std::uint32_t handle,argument,definition,index;
};

// Resolve the object's type definition and notify each entry in its
// null-terminated definition chain. The source reloads the active slot
// before reading the callback and resumes at the index the callback leaves.
template<class Memory,class Frame>
void notify(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto object=object_of(memory,handle,object_table,object_datums,object_address);
    const auto type_index=widen8(memory.load8(type,object+object_type));
    const auto value=memory.load32(argument,frame.save_and_locate_argument());
    frame.save_chain_registers();
    const auto definition_address=memory.load32(definition,
        native_object_update::globals::object_type_definitions+4u*type_index);
    Chain current{handle,value,definition_address,0u};
    auto slot=current.definition+chain;
    auto entry=memory.load32(first_type,slot);
    while(entry) {
        const auto type_address=memory.load32(chain_type,slot);
        const auto target=memory.load32(callback,type_address+notify_callback);
        if(target)frame.call(current,target);
        ++current.index;
        slot=current.definition+chain+4u*widen16(current.index&0xffffu);
        entry=memory.load32(next_type,slot);
    }
}
} // namespace halo2::native_object_type_notification
