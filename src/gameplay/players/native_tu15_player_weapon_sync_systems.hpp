#pragma once
// Native TU1.5 player weapon sync, entered at 168CA1 (sandbox special unit):
// each local player's weapon record kept in step with the player's unit.
//
//   player_weapon_sync::sync   168CA1()
//
// For each local player (records of 0x20CC bytes at [4F6498], four of them)
// 150990 answers the player (NONE for none). A record whose unit (+4) or
// state (+8) differs from the player's (the players' datums, 0x21C bytes:
// the unit +0x2C, the state's signed byte +0x88) is reattached (16B16A,
// through the dispatch: the player's index and the state). A record with a
// unit then checks both weapon slots (0x1010 bytes from +0x10): CE230
// (through the dispatch: EAX the unit, CX the unit's signed weapon index for
// the slot, +0x212) answers the weapon, and a slot holding another (+0)
// changes (16AD45, through the dispatch: the index and the slot); 169C46
// (the required owner: the index) updates the record.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's EBP frame (the player's
// index, the record's offset, the slot); the boundary adapter
// (native_tu15_player_weapon_sync.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_player_weapon_sync {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t records=0x4f6498u,players=0x4f54f4u,object_headers=0x4ecbd0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,none=0xffffffffu;
inline constexpr std::uint32_t record_size=0x20ccu,records_end=4u*record_size,slots=2u,slot_size=0x1010u;
namespace player {inline constexpr std::uint32_t size=0x21cu,unit=0x2cu,state=0x88u;}
namespace record {inline constexpr std::uint32_t unit=4u,state=8u,slots=0x10u;}
namespace slot {inline constexpr std::uint32_t weapon=0u;}
namespace unit {inline constexpr std::uint32_t weapon_index=0x212u;}
// The EBP frame: the player's index, the record's offset, the slot.
namespace local {inline constexpr std::uint32_t index=4u,offset=8u,slot=0xcu;}

namespace player_weapon_sync {
enum class Access {index_clear_load,index_clear,offset_clear_load,offset_clear,index_load,records_load,offset_load,players_load,
    datums_load,state_load,unit_load,record_unit,record_state,index_attach,record_unit_again,slot_store,slot_unit,headers_load,
    object_datums,unit_address,weapon_index,slot_load,slot_weapon,index_change,slot_advance_load,slot_advance,index_update,
    offset_advance_load,offset_advance,index_advance_load,index_advance,offset_compare};
// The original's EBX (the slot), ESI (the record) and EDI (the handle mask) across the children.
struct Kept {std::uint32_t slot,record,mask;};
// Frame: framing(); local(n) EBP-n; cleared(v); saving(kept); player(index, kept) [150990]; compared(a, b); indexed(handle, mask);
// record(records, offset); state_pushed(state); attached(index, kept) [16B16A]; first_slot(kept); unit_index(handle, mask);
// weapon(index, kept) [CE230]; slot_pushed(slot); changed(index, kept) [16AD45]; next_slot(slot); updated(index, kept) [169C46];
// advanced(v, by); restoring().
template<class Memory,class Frame> void sync(Memory &memory,Frame &frame,Kept kept) {
    using enum Access;
    frame.framing();
    memory.store32(index_clear,frame.local(local::index),frame.cleared(memory.load32(index_clear_load,frame.local(local::index))));
    memory.store32(offset_clear,frame.local(local::offset),frame.cleared(memory.load32(offset_clear_load,frame.local(local::offset))));
    frame.saving(kept);
    for(;;) {
        const auto index=memory.load32(index_load,frame.local(local::index));
        const auto player_handle=frame.player(index,kept);
        frame.compared(player_handle,none);
        if(player_handle!=none) {
            // The record against the player's unit and state.
            const auto records=memory.load32(records_load,globals::records);
            const auto offset=memory.load32(offset_load,frame.local(local::offset));
            const auto player_offset=frame.indexed(player_handle,kept.mask);
            kept.record=frame.record(records,offset);
            const auto players=memory.load32(players_load,globals::players);
            const auto player_address=frame.player_address(player_offset,memory.load32(datums_load,at(players,datum_data)));
            const auto state=memory.load8(state_load,at(player_address,player::state));
            const auto unit_handle=memory.load32(unit_load,at(player_address,player::unit));
            const auto record_unit_value=memory.load32(record_unit,at(kept.record,record::unit));
            frame.compared(record_unit_value,unit_handle);
            bool stale=record_unit_value!=unit_handle;
            if(!stale) {
                const auto record_state_value=memory.load32(record_state,at(kept.record,record::state));
                frame.compared(record_state_value,frame.signed_state(state));
                stale=record_state_value!=frame.signed_state(state);
            }
            if(stale) {
                frame.state_pushed(frame.signed_state(state));
                frame.attached(memory.load32(index_attach,frame.local(local::index)),kept);
            }
            // Each weapon slot against the unit's weapon.
            const auto unit_now=memory.load32(record_unit_again,at(kept.record,record::unit));
            frame.compared(unit_now,none);
            if(unit_now!=none) {
                memory.store32(slot_store,frame.local(local::slot),frame.first_slot(kept));
                for(;;) {
                    const auto handle=memory.load32(slot_unit,at(kept.record,record::unit));
                    const auto headers=memory.load32(headers_load,globals::object_headers);
                    const auto datums=memory.load32(object_datums,at(headers,datum_data));
                    const auto unit_address_value=memory.load32(unit_address,datums+header_size*frame.unit_index(handle,kept.mask)+header_address);
                    const auto weapon_index_value=memory.load8(weapon_index,unit_address_value+kept.slot+unit::weapon_index);
                    const auto weapon=frame.weapon(weapon_index_value,kept);
                    const auto slot_address=memory.load32(slot_load,frame.local(local::slot));
                    const auto held=memory.load32(slot_weapon,at(slot_address,slot::weapon));
                    frame.compared(held,weapon);
                    if(held!=weapon) {
                        frame.slot_pushed(kept.slot);
                        frame.changed(memory.load32(index_change,frame.local(local::index)),kept);
                    }
                    const auto slot_now=memory.load32(slot_advance_load,frame.local(local::slot));
                    memory.store32(slot_advance,frame.local(local::slot),frame.advanced(slot_now,slot_size));
                    kept.slot=frame.next_slot(kept.slot);
                    if(!(std::int32_t(kept.slot)<std::int32_t(slots)))break;
                }
                frame.updated(memory.load32(index_update,frame.local(local::index)),kept);
            }
        }
        // The next player.
        const auto offset_now=memory.load32(offset_advance_load,frame.local(local::offset));
        memory.store32(offset_advance,frame.local(local::offset),frame.advanced(offset_now,record_size));
        const auto index_now=memory.load32(index_advance_load,frame.local(local::index));
        memory.store32(index_advance,frame.local(local::index),frame.incremented(index_now));
        const auto offset_value=memory.load32(offset_compare,frame.local(local::offset));
        frame.compared(offset_value,records_end);
        if(!(std::int32_t(offset_value)<std::int32_t(records_end)))break;
    }
    frame.restoring();
}
} // namespace player_weapon_sync
} // namespace halo2::native_player_weapon_sync
