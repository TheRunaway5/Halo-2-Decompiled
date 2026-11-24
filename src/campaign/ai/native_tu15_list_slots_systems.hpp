#pragma once
// Native TU1.5 list slots, entered at 1AFA90 (AI actor and final startup
// storage units): fifty slots filled from a list's entries or defaults.
//
//   list_slots::fill   1AFA90(slots*, list*, defaults*, default count) -> RET 0x10
//
// Each slot in turn takes the next of the list's entries (a count, word +8;
// 24-byte entries at +4) when that entry's kind (word +0) accepts it, which
// the original's jump table (1AFB48, one target for each kind up to 4) says:
// an entry whose id (word +2) is the current id; never; an entry whose id is
// the next default's, which that default goes with; an entry once the
// defaults are spent; always. A taken entry's value (+4) fills the slot.
// Otherwise the slot takes the next of the defaults (20 bytes each; their
// id, word +0, becomes the current id, which starts as [47C1A4]), and when
// those are spent the filling stops. The answer (EAX) is the slots filled.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's frame (the list's count,
// in the slot of the ECX it pushes) and the table's entry; the boundary
// adapter (native_tu15_list_slots.hpp) maps the role to the original
// instruction and owns the ABI.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_list_slots {
using native_object_update::at;
using native_object_update::widen16;
namespace globals {inline constexpr std::uint32_t first_id=0x47c1a4u,kinds=0x1afb48u;}
inline constexpr std::uint32_t slots=50u,last_kind=4u,entry_size=24u,default_size=20u;
namespace list {inline constexpr std::uint32_t entries=4u,count=8u;}
namespace entry {inline constexpr std::uint32_t kind=0u,id=2u,value=4u;}
namespace argument {inline constexpr std::uint32_t slots=0u,list=1u,defaults=2u,default_count=3u;}
// The table's targets, as the unit's canonical_branch names them: what each kind does with an entry.
namespace targets {inline constexpr std::uint32_t same_id=0x1afae2u,never=0x1afae8u,default_id=0x1afb16u,when_spent=0x1afb2au,
    always=0x1afb31u;}
// The frame's slot, by offset from the return address: the list's count, in the pushed ECX.
namespace local {inline constexpr std::int32_t count=-4;}
inline std::int32_t word(std::uint32_t v){return std::int16_t(std::uint16_t(v));}

namespace list_slots {
enum class Access {first_id,defaults_argument,list_argument,count_clear,list_count,count_store,list_again,count_load,entries,
    entry_kind,table_entry,same_id,default_id,default_entry_id,spent_count,default_count,entry_slots,entry_store,defaults_again,
    default_slots,default_store,default_record_id};
// Frame:
//   argument(k), local(offset), saving(n)        PUSH ECX; PUSH EBX, EBP; PUSH ESI, EDI
//   clearing(list)                               XOR EBX, EAX, ESI; CMP EDI,EBX
//   entry_compared(entry, count), record(address), kind_tested(kind)   CMP BX,[ESP+10]; LEA EDX; CMP EDI,4
//   branch(target) -> target                     the table entry through canonical_branch; unknown_case() for none
//   ids_compared(a, b), default_indexed(used), defaults_compared(used, count)
//   entry_slot(slot, record), entry_used(entry)  MOVSX EDI,AX; ADD EDX,4; INC EBX
//   default_slot(slot, used, record), default_used(used), slot_advanced(slot)   INC ESI; INC EAX; CMP AX,32
template<class Memory,class Frame> std::uint32_t fill(Memory &memory,Frame &frame) {
    using enum Access;
    frame.saving(0u);
    auto id=memory.load32(first_id,globals::first_id);
    frame.saving(1u);
    auto defaults=memory.load32(defaults_argument,frame.argument(argument::defaults));
    frame.saving(2u);
    auto list_address=memory.load32(list_argument,frame.argument(argument::list));
    frame.clearing(list_address);
    memory.store32(count_clear,frame.local(local::count),0u);
    if(list_address)memory.store16(count_store,frame.local(local::count),memory.load16(list_count,at(list_address,list::count)));
    std::uint32_t slot=0u,entry=0u,used=0u;
    for(bool first=true;;first=false) {
        if(!first)list_address=memory.load32(list_again,frame.argument(argument::list));
        // The next entry, if one is left, by its kind: taken, or the next default (checked or not).
        enum class Choice {entry,check_default,default_record} choice=Choice::check_default;
        const auto count=memory.load16(count_load,frame.local(local::count));
        frame.entry_compared(entry,count);
        std::uint32_t record=0u;
        if(word(entry)<word(count)) {
            record=memory.load32(entries,at(list_address,list::entries))+entry_size*widen16(entry);
            frame.record(record);
            const auto kind=widen16(memory.load16(entry_kind,at(record,entry::kind)));
            frame.kind_tested(kind);
            if(kind<=last_kind) {
                switch(frame.branch(memory.load32(table_entry,globals::kinds+4u*kind))) {
                case targets::same_id: {
                    const auto entry_id=memory.load16(same_id,at(record,entry::id));
                    frame.ids_compared(id,entry_id);
                    if((id&0xffffu)==entry_id)choice=Choice::entry;
                    break;
                }
                case targets::never:break;
                case targets::default_id: {
                    frame.default_indexed(used);
                    const auto default_value=memory.load16(Access::default_id,defaults+default_size*widen16(used));
                    const auto entry_id=memory.load16(default_entry_id,at(record,entry::id));
                    frame.ids_compared(default_value,entry_id);
                    if(default_value==entry_id){++used;frame.default_used(used);choice=Choice::entry;}
                    break;
                }
                case targets::when_spent: {
                    const auto spent=memory.load16(spent_count,frame.argument(argument::default_count));
                    frame.defaults_compared(used,spent);
                    choice=word(used)<word(spent)?Choice::default_record:Choice::entry;
                    break;
                }
                case targets::always:choice=Choice::entry;break;
                default:frame.unknown_case();
                }
            }
        }
        if(choice==Choice::entry) {
            const auto slots_address=memory.load32(entry_slots,frame.argument(argument::slots));
            frame.entry_slot(slot,record);
            memory.store32(entry_store,slots_address+4u*widen16(slot),record+entry::value);
            defaults=memory.load32(defaults_again,frame.argument(argument::defaults));
            ++entry;
            frame.entry_used(entry);
        } else {
            if(choice==Choice::check_default) {
                const auto remaining=memory.load16(default_count,frame.argument(argument::default_count));
                frame.defaults_compared(used,remaining);
                if(!(word(used)<word(remaining)))break;
            }
            const auto slots_address=memory.load32(default_slots,frame.argument(argument::slots));
            const auto chosen=defaults+default_size*widen16(used);
            frame.default_slot(slot,used,chosen);
            memory.store32(default_store,slots_address+4u*widen16(slot),chosen);
            id=memory.load16(default_record_id,chosen);
            frame.default_id_taken(id);
            ++used;
            frame.default_used(used);
        }
        ++slot;
        frame.slot_advanced(slot);
        if(!(word(slot)<word(slots)))break;
    }
    return slot;
}
} // namespace list_slots
} // namespace halo2::native_list_slots
