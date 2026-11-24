#pragma once
// Native TU1.5 seat sum, entered at 1059C0 (physics live support unit): one
// indexed word summed over an object and the occupants of its vehicle that
// share its definition.
//
//   seat_sum::sum   1059C0(EAX = index; object handle, skip byte) -> EAX, RET 8
//
// An object's word comes from its definition when 10B0F0 answers for the
// object: the word +0xC of the definition's 0x5C-byte entry at the index
// (+0x2C4, their count +0x2C0; an index past the count, as signed, gives no
// word); otherwise the object's own word at +0x22A + 16 * index. The
// object's word starts the sum (with no word, the sum is the register 10B0F0
// returns, zero going in). Unless the skip byte is set, the occupants of the
// object's vehicle (+0x154, NONE for none) in its four seats (+0x218..+0x224)
// add theirs: every occupant other than the object whose definition tag (+0)
// is the object's. The handle, the index scaled by 16 and the vehicle are
// kept in stack slots the original reads back each time (the vehicle over
// the skip byte).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_seat_sum.hpp) maps the role to the original instruction and
// owns the ABI of 10B0F0; the object, the index, the header table and the
// sum the original keeps in EDI, ESI, EBP and EBX across it are a Kept,
// refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>
#include <optional>

namespace halo2::native_seat_sum {
using native_object_update::at;
using native_object_update::none;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;   // DataArray* of 12-byte object headers
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_address=8u;
namespace definition {inline constexpr std::uint32_t count=0x2c0u,entries=0x2c4u,entry_size=0x5cu,entry_word=0xcu;}
namespace object {inline constexpr std::uint32_t words=0x22au,word_stride_shift=4u,vehicle=0x154u,seats=0x218u,seat_count=4u;}

namespace seat_sum {
enum class Access {
    handle,headers,header_datums,object_address,local_store,tag,tags,definition,count,entries,entry_word,local,own_word,
    vehicle,skip,vehicle_datums,vehicle_address,vehicle_store,vehicle_reload,seat,seat_handle,occupant,occupant_tag,object_tag
};
struct Kept {std::uint32_t object,index,table,sum;};   // EDI, ESI, EBP, EBX
// Frame:
//   handle_slot(), skip_slot(), local_slot()  the stack slots
//   linked(kept, handle)   10B0F0; refreshes kept; returns AL
//   seat(k)                before the k-th seat
//   added(kept, word)      the sum grown by an occupant's word
template<class Memory,class Frame> std::uint32_t sum(Memory &memory,Frame &frame,std::uint32_t index) {
    using enum Access;
    const auto object_handle=memory.load32(handle,frame.handle_slot());
    const auto table=memory.load32(headers,globals::object_headers);
    const auto datums=memory.load32(header_datums,at(table,datum_data));
    Kept kept{memory.load32(object_address,datums+header_size*(object_handle&0xffffu)+header_address),index,table,0u};
    memory.store32(local_store,frame.local_slot(),kept.index<<object::word_stride_shift);
    const bool linked=frame.linked(kept,object_handle)!=0;
    // The word of an object whose definition tag is known.
    const auto word=[&](std::uint32_t tag_handle,std::uint32_t address)->std::optional<std::uint32_t> {
        if(linked) {
            const auto instances=memory.load32(tags,globals::tag_instances);
            const auto owner=memory.load32(Access::definition,tag_size*(tag_handle&0xffffu)+instances+tag_address);
            if(signed_value(memory.load32(count,at(owner,definition::count)))<=signed_value(kept.index))return std::nullopt;
            const auto list=memory.load32(entries,at(owner,definition::entries));
            return widen16(memory.load16(entry_word,definition::entry_size*kept.index+list+definition::entry_word));
        }
        return widen16(memory.load16(own_word,memory.load32(local,frame.local_slot())+address+object::words));
    };
    if(const auto own=word(linked?memory.load32(tag,kept.object):0u,kept.object))kept.sum=*own;
    const auto vehicle_handle=memory.load32(vehicle,at(kept.object,object::vehicle));
    if(vehicle_handle==none)return kept.sum;
    if(memory.load8(skip,frame.skip_slot())&0xffu)return kept.sum;
    const auto object_datums=memory.load32(vehicle_datums,at(kept.table,datum_data));
    const auto vehicle_record=memory.load32(vehicle_address,object_datums+header_size*(vehicle_handle&0xffffu)+header_address);
    memory.store32(vehicle_store,frame.skip_slot(),vehicle_record);
    for(std::uint32_t k=0;k<object::seat_count;++k) {
        frame.seat(k);
        const auto seats=k==0?vehicle_record:memory.load32(vehicle_reload,frame.skip_slot());
        const auto occupant_handle=memory.load32(seat,at(seats,object::seats+4u*k));
        if(occupant_handle==none)continue;
        if(occupant_handle==memory.load32(seat_handle,frame.handle_slot()))continue;
        const auto occupant_record=memory.load32(occupant,object_datums+header_size*(occupant_handle&0xffffu)+header_address);
        const auto occupant_tag_handle=memory.load32(occupant_tag,occupant_record);
        if(occupant_tag_handle!=memory.load32(object_tag,kept.object))continue;
        if(const auto theirs=word(occupant_tag_handle,occupant_record)) {
            kept.sum+=*theirs;
            frame.added(kept,*theirs);
        }
    }
    return kept.sum;
}
} // namespace seat_sum
} // namespace halo2::native_seat_sum
