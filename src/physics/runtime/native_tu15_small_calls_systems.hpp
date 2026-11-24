#pragma once
// Native TU1.5 small queries with one child:
//
//   header_unanswered::test   17C330(EAX = object handle; argument) -> AL
//       (physics live support, death cycle) the object's header active (+2
//       bit 0) and BBD10 (EAX the header's tripled index, ECX the argument)
//       answering 0
//   special_animation::test   C9590(EAX = object handle) -> AL
//       (physics live support, AI frame) C9550 (ECX the handle) answers, or
//       the object's animation (its block at the signed offset +0x12A, +0x7C)
//       is one of 080000C4, 0400004A, 050000C3
//   entry_refresh::refresh    1E5D60(EDI = record) (physics live support)
//       each of the record's entries (20 bytes at +0xB0, the count at +0xAC)
//       whose byte +0x10 is set goes to 126200 (EAX the entry, CL 0)
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapters map the role to the
// original instruction and own the ABI of the children; the values the
// original keeps in callee-saved registers across them are Kept, refreshed
// from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_small_calls {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u;}   // DataArray* of 12-byte object headers

namespace header_unanswered {
enum class Access {headers,header_datums,header_flags,argument};
// Frame: argument() the argument's slot; ask(argument) BBD10; returns AL.
template<class Memory,class Frame> bool test(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,0x44u));
    if(!(memory.load8(header_flags,datums+12u*(handle&0xffffu)+2u)&1u))return false;
    return frame.ask(memory.load32(argument,frame.argument()))==0;
}
} // namespace header_unanswered

namespace special_animation {
enum class Access {headers,header_datums,object_address,block_offset,animation};
inline constexpr std::uint32_t special[]{0x080000c4u,0x0400004au,0x050000c3u};
struct Kept {std::uint32_t handle;};   // ESI
enum class Answer {child,special,other};   // C9550 answered; one of the codes; neither
// Frame: ask(kept) C9550; refreshes kept; returns AL. compared(animation, code) CMP.
template<class Memory,class Frame> Answer test(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    Kept kept{handle};
    if(frame.ask(kept))return Answer::child;
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,0x44u));
    const auto address=memory.load32(object_address,datums+12u*(kept.handle&0xffffu)+8u);
    const auto offset=widen16(memory.load16(block_offset,at(address,0x12au)));
    const auto animation_value=memory.load32(animation,offset+address+0x7cu);
    for(const auto code:special) {
        frame.compared(animation_value,code);
        if(animation_value==code)return Answer::special;
    }
    return Answer::other;
}
} // namespace special_animation

namespace entry_refresh {
enum class Access {count,entries,marked,count_again};
inline constexpr std::uint32_t entry_size=0x14u,entries_offset=0xb0u,count_offset=0xacu,mark=0x10u;
struct Kept {std::uint32_t record,index,offset;};   // EDI, EBX, ESI
// Frame: refresh(kept, entry) 126200; refreshes kept. stepped(kept, limit) INC EBX; ADD ESI,14; CMP.
template<class Memory,class Frame> void refresh(Memory &memory,Frame &frame,std::uint32_t record) {
    using enum Access;
    Kept kept{record,0u,0u};
    if(signed_value(memory.load32(count,at(kept.record,count_offset)))<=0)return;
    frame.walking();
    std::uint32_t limit;
    do {
        const auto base=memory.load32(entries,at(kept.record,entries_offset));
        const auto entry=base+kept.offset;
        if(memory.load8(marked,at(entry,mark))&0xffu)frame.refresh(kept,entry);
        limit=memory.load32(count_again,at(kept.record,count_offset));
        ++kept.index;kept.offset+=entry_size;
        frame.stepped(kept,limit);
    } while(signed_value(kept.index)<signed_value(limit));
}
} // namespace entry_refresh
} // namespace halo2::native_small_calls
