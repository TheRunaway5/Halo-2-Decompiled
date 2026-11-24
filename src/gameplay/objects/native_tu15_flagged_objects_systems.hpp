#pragma once
// Native TU1.5 flagged object visit, entered at 102910 (physics live support
// unit): every object of one iteration whose flag is set handed to 102980.
//
//   flagged_objects::visit   102910()
//
// An object iterator on the stack (mask 4, kind byte 2, word 0, current
// handle NONE, signature 86868686) is stepped by BCEE0 (its address
// pushed), each object it answers kept in a slot below it; an object with
// bit 0 of byte +0x16C has the iterator's current handle passed to 102980.
// Both are reached through the unit's dispatch.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_flagged_objects.hpp) maps the role to the original
// instruction and owns the ABI of the children.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_flagged_objects {
using native_object_update::at;
using native_object_update::none;
// The stack record below the return address: the current object, then the
// iterator.
namespace local {inline constexpr std::uint32_t object=0u,mask=4u,kind=8u,word=0xau,handle=0xcu,signature=0x10u,size=0x14u;}
inline constexpr std::uint32_t iterator_mask=4u,iterator_kind=2u,signature=0x86868686u;
inline constexpr std::uint32_t flag_offset=0x16cu,flag_bit=1u<<0;

namespace flagged_objects {
enum class Access {signature,mask,kind,word,handle,object_store,flag,handle_read};
// Frame: local(offset) a slot of the record; next() BCEE0, returns EAX;
// visit(handle) 102980.
template<class Memory,class Frame> void visit(Memory &memory,Frame &frame) {
    using enum Access;
    memory.store32(Access::signature,frame.local(local::signature),native_flagged_objects::signature);
    memory.store32(mask,frame.local(local::mask),iterator_mask);
    memory.store8(kind,frame.local(local::kind),iterator_kind);
    memory.store16(word,frame.local(local::word),0u);
    memory.store32(handle,frame.local(local::handle),none);
    auto object=frame.next();
    memory.store32(object_store,frame.local(local::object),object);
    while(object) {
        if(memory.load8(flag,at(object,flag_offset))&flag_bit) {
            frame.visit(memory.load32(handle_read,frame.local(local::handle)));
        }
        object=frame.next();
        memory.store32(object_store,frame.local(local::object),object);
    }
}
} // namespace flagged_objects
} // namespace halo2::native_flagged_objects
