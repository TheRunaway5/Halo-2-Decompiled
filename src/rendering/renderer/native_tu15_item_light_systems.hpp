#pragma once
// Native TU1.5 item light, entered at 1D9080 and 1D9100 (scene device
// constructor unit): a vector handed to an item's light.
//
//   item_light::place   1D9080(ECX = item, EAX = vector; record)   the light's +0x50
//   item_light::aim     1D9100(EAX = item, ECX = vector; record)   the light's +0x54, unless the record's flag bit 1
//
// The record's item (0x60 bytes at the record's +0x70, the light at +0x40)
// takes the vector unless its light is off (byte +0x40). The vector, as
// four floats with a zero w, goes to the light's object (+0x3C) through its
// vtable; before that, 315C90 asks whether the light is settled (a byte of
// the frame), and an unsettled light with a pending refresh (+0x44) is
// refreshed (315CB0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters
// (native_tu15_item_light_place.hpp, native_tu15_item_light_aim.hpp) map
// the role to the original instruction and own the ABI of the children (the
// aligned frame among it); the light, which the original keeps in ESI
// across them, is refreshed from the register each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_item_light {
using native_object_update::at;
namespace record {inline constexpr std::uint32_t flags=4u,items=0x70u;}
inline constexpr std::uint32_t aiming_off=2u,item_size=0x60u,light_slot=0x40u,zero_bits=0u;
namespace light {inline constexpr std::uint32_t object=0x3cu,off=0x40u,pending=0x44u;}
namespace vtable {inline constexpr std::uint32_t place=0x50u,aim=0x54u;}

namespace item_light {
enum class Access {record,flags,items,light,off,x,x_store,y,y_store,z,z_store,w_store,light_again,settled,pending,object,table,function};
enum class Outcome {skipped,placed,refreshed};
// The two entries: which vtable slot takes the vector, and whether the
// record's flag and the light's second read take part.
struct Variant {std::uint32_t slot;bool aim;};
inline constexpr Variant placing{vtable::place,false},aiming{vtable::aim,true};
// Frame:
//   argument(), local(k)               the record's slot and the frame's floats now
//   prepare(variant)                   the settled byte's address pushed
//   settled(light&) -> its byte's address   315C90(ECX the light), refreshes the light
//   refresh(light) -> light            315CB0(ECX the light)
//   handing(object)                   the vector's address pushed
//   hand(light, function, vector)      the light object's function (through the unit's dispatch)
template<class Memory,class Frame> Outcome deliver(Memory &memory,Frame &frame,const Variant &variant,std::uint32_t item,std::uint32_t vector) {
    using enum Access;
    const auto record_address=memory.load32(Access::record,frame.argument());
    if(variant.aim && (memory.load32(flags,at(record_address,record::flags))&aiming_off))return Outcome::skipped;
    const auto slot=memory.load32(items,at(record_address,record::items))+item_size*item+light_slot;
    auto light_address=memory.load32(Access::light,slot);
    if(memory.load8(off,at(light_address,light::off))&0xffu)return Outcome::skipped;
    memory.store32(x_store,frame.local(0u),memory.load32(x,at(vector,0u)));
    memory.store32(y_store,frame.local(1u),memory.load32(y,at(vector,4u)));
    memory.store32(z_store,frame.local(2u),memory.load32(z,at(vector,8u)));
    if(variant.aim) {
        memory.store32(w_store,frame.local(3u),zero_bits);
        light_address=memory.load32(light_again,slot);
    }
    frame.prepare(variant);
    if(!variant.aim)memory.store32(w_store,frame.local(3u),zero_bits);
    const auto settled_byte=frame.settled(light_address);
    auto outcome=Outcome::placed;
    if(!(memory.load8(settled,settled_byte)&0xffu) && memory.load32(pending,at(light_address,light::pending))) {
        light_address=frame.refresh(light_address);
        outcome=Outcome::refreshed;
    }
    const auto object_address=memory.load32(object,at(light_address,light::object));
    const auto table_address=memory.load32(table,object_address);
    frame.handing(object_address);
    frame.hand(light_address,memory.load32(function,at(table_address,variant.slot)),frame.local(0u));
    return outcome;
}
} // namespace item_light
} // namespace halo2::native_item_light
