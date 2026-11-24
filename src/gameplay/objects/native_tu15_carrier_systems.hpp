#pragma once
// Native TU1.5 carrier, entered at D2B60 (physics live support unit): the
// biped or vehicle an object is carried by, all the way up.
//
//   carrier::of   D2B60(EAX = object handle, EBX = result*)
//
// While an object has a parent (+0x14, NONE for none), is attached to it
// (flag bit 26 of +4) and the parent is a biped or a vehicle (object type
// byte +0xAA 0 or 1), the parent stands in for it. The result is that
// object's handle, twice, and the field +0x10 of the last object read.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_carrier.hpp) maps the role to the original instruction; the
// original keeps a one in a stack local for its type test, which the
// adapter owns.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_carrier {
using native_object_update::at;
using native_object_update::none;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u;}   // DataArray* of 12-byte rows (+8 the object)
inline constexpr std::uint32_t datum_data=0x44u,row_size=12u,row_object=8u;
namespace object {inline constexpr std::uint32_t flags=4u,field=0x10u,parent=0x14u,type=0xaau;}
inline constexpr std::uint32_t attached=1u<<26,carrier_types=3u;   // bipeds and vehicles

namespace carrier {
enum class Access {objects,object_data,object,parent,flags,parent_object,type,first,second,field,field_store};
enum class Outcome {loose,detached,uncarried};
// Frame: climbed() each time the parent stands in.
template<class Memory,class Frame> Outcome of(Memory &memory,Frame &frame,std::uint32_t handle,std::uint32_t result) {
    using enum Access;
    const auto data=memory.load32(object_data,at(memory.load32(objects,globals::objects),datum_data));
    std::uint32_t current;
    Outcome outcome;
    for(;;) {
        current=memory.load32(object,data+row_size*(handle&0xffffu)+row_object);
        const auto parent_handle=memory.load32(parent,at(current,object::parent));
        if(parent_handle==none){outcome=Outcome::loose;break;}
        if(!(memory.load32(flags,at(current,object::flags))&attached)){outcome=Outcome::detached;break;}
        const auto parent_address=memory.load32(parent_object,data+row_size*(parent_handle&0xffffu)+row_object);
        if(!((1u<<(memory.load8(type,at(parent_address,object::type))&31u))&carrier_types)){outcome=Outcome::uncarried;break;}
        handle=parent_handle;
        frame.climbed();
    }
    memory.store32(first,at(result,0u),handle);
    memory.store32(second,at(result,4u),handle);
    const auto field_value=memory.load32(field,at(current,object::field));
    memory.store32(field_store,at(result,8u),field_value);
    return outcome;
}
} // namespace carrier
} // namespace halo2::native_carrier
