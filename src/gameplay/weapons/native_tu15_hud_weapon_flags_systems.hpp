#pragma once
// Native TU1.5 HUD weapon flags, entered at 13DC44 (weapon HUD unit): a HUD
// record's flags for its unit's weapon.
//
//   hud_weapon_flags::compute   13DC44(EAX = unit, EBX = record*)
//
// The record's flags (+0x4C) clear. A unit (through the object headers at
// [4ECBD0]) with a weapon (its slot +0x1C8 not NONE; BCDF0 through the
// dispatch answers the weapon, with -1 pushed) marks: an object of type 0
// or 1 (byte +0xAA) that 1E7810 (through the dispatch: EAX the unit, SI and
// CX their teams +0x138) finds not allied, flag 1; a magazine (the slot's
// +4, below its definition's count +0x68; 0x1C-byte entries at +0x6C of the
// definition the weapon's +0x38 names, through the tag instances at 4F0414)
// at full charge (the slot's +0x1C not below [46A0D0]) with entry flag 4,
// flag 0x10, and with entry flag 8, flag 4. A record with a target (+0xC not
// NONE and byte +0xAB set, or +0x10 not NONE and byte +0xE3 set) and a slot
// 10B3D0 (EDI the slot) accepts at full charge marks flag 8.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the original's EBP frame (the unit, the
// slot, the definition); the boundary adapter
// (native_tu15_hud_weapon_flags.hpp) maps the role to the original
// instruction and owns the ABI of the children. The values the original
// keeps in callee-saved registers across them are a Kept, refreshed from the
// registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_hud_weapon_flags {
using native_object_update::at;
using native_object_update::none;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,one=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_data=8u,entry_size=0x1cu,
    typed_mask=3u;
namespace unit {inline constexpr std::uint32_t slot=0x1c8u,team=0x138u;}
namespace weapon {inline constexpr std::uint32_t type=0xaau,team=0x138u;}
namespace slot {inline constexpr std::uint32_t magazine=4u,charge=0x1cu;}
namespace definition {inline constexpr std::uint32_t magazines=0x38u,count=0x68u,entries=0x6cu;}
namespace entry {inline constexpr std::uint32_t flags=0x14u;inline constexpr std::uint32_t counted=4u,shown=8u;}
namespace record {inline constexpr std::uint32_t first=0xcu,second=0x10u,flags=0x4cu,first_on=0xabu,second_on=0xe3u;}
namespace flag {inline constexpr std::uint32_t enemy=1u,full=8u,counted=0x10u,shown=4u;}
// The EBP frame's slots: the definition, the unit, the slot.
namespace local {inline constexpr std::int32_t definition=-0xc,unit=-8,slot=-4;}

namespace hud_weapon_flags {
enum class Access {headers,datums,object_address,flags_load,flags_clear,unit_store,slot_store,weapon_handle,weapon_tag,instances,
    definition,weapon_type,definition_store,unit_load,weapon_team,unit_team,enemy_load,enemy_store,definition_load,magazines,slot_load,
    magazine,charge,one,magazine_instances,magazine_definition,count,entries,counted_flags,counted_load,counted_store,shown_flags,
    shown_load,shown_store,slot_again,first,first_on,second,second_on,slot_charge,one_again,full_load,full_store};
enum class Outcome {unarmed,armed};
// The original's EBX (the record), ESI (the weapon, then the slot) and EDI (0xFFFF, then the slot) across the children.
struct Kept {std::uint32_t record,source,mask;};
// Frame: framing() PUSH EBP; MOV EBP,ESP; SUB ESP,C; local(offset) EBP-relative; the hooks name their instructions.
template<class Memory,class Frame> Outcome compute(Memory &memory,Frame &frame,std::uint32_t unit_handle,Kept kept) {
    using enum Access;
    frame.framing();
    const auto datums_address=memory.load32(datums,at(memory.load32(headers,globals::object_headers),datum_data));
    frame.saving(0u);
    kept.mask=0xffffu;
    frame.indexing(unit_handle,kept.mask);
    const auto unit_address=memory.load32(object_address,datums_address+header_size*(unit_handle&kept.mask)+header_address);
    memory.store32(flags_clear,at(kept.record,record::flags),frame.anded(memory.load32(flags_load,at(kept.record,record::flags)),0u));
    memory.store32(unit_store,frame.local(local::unit),unit_address);
    const auto slot_address=frame.slot(unit_address);
    memory.store32(slot_store,frame.local(local::slot),slot_address);
    const auto weapon_value=memory.load32(weapon_handle,slot_address);
    frame.compared(weapon_value,none);
    if(weapon_value==none){frame.restoring(0u);return Outcome::unarmed;}
    frame.saving(1u);
    kept.source=frame.weapon(weapon_value,kept);
    frame.tested(kept.source);
    const auto set_flag=[&](Access load_role,Access store_role,std::uint32_t bit) {
        memory.store32(store_role,at(kept.record,record::flags),frame.ored(memory.load32(load_role,at(kept.record,record::flags)),bit));
    };
    if(kept.source) {
        const auto tag=memory.load32(weapon_tag,kept.source);
        const auto instances_address=memory.load32(instances,globals::tag_instances);
        const auto definition_value=memory.load32(definition,frame.tag_index(tag,kept.mask,0u)+instances_address+tag_data);
        const auto type=memory.load8(weapon_type,at(kept.source,weapon::type));
        memory.store32(definition_store,frame.local(local::definition),definition_value);
        if(frame.typed(type)&typed_mask) {
            const auto unit_now=memory.load32(unit_load,frame.local(local::unit));
            const auto weapon_team_value=memory.load16(weapon_team,at(kept.source,weapon::team));
            const auto unit_team_value=memory.load16(unit_team,at(unit_now,unit::team));
            if(!(frame.allied(unit_now,weapon_team_value,unit_team_value,kept)&0xffu))set_flag(enemy_load,enemy_store,flag::enemy);
        }
        const auto definition_now=memory.load32(definition_load,frame.local(local::definition));
        const auto magazines_value=memory.load32(magazines,at(definition_now,definition::magazines));
        frame.compared(magazines_value,none);
        kept.source=memory.load32(slot_load,frame.local(local::slot));
        if(magazines_value!=none) {
            const auto magazine_value=memory.load32(magazine,at(kept.source,slot::magazine));
            frame.compared(magazine_value,none);
            if(magazine_value!=none) {
                const auto charge_value=memory.load32(charge,at(kept.source,slot::charge));
                const auto one_value=memory.load32(one,globals::one);
                frame.charged(charge_value,one_value);
                if(!sse::below(charge_value,one_value)) {
                    const auto magazine_instances_value=memory.load32(magazine_instances,globals::tag_instances);
                    const auto magazine_definition_value=memory.load32(magazine_definition,
                        frame.tag_index(magazines_value,kept.mask,1u)+magazine_instances_value+tag_data);
                    const auto count_value=memory.load32(count,at(magazine_definition_value,definition::count));
                    frame.compared(magazine_value,count_value);
                    if(std::int32_t(magazine_value)<std::int32_t(count_value)) {
                        const auto scaled=frame.scaled(magazine_value);
                        const auto entry_address=frame.entry(scaled,memory.load32(entries,at(magazine_definition_value,definition::entries)));
                        const auto counted=memory.load8(counted_flags,at(entry_address,entry::flags));
                        frame.tested8(counted&entry::counted);
                        if(counted&entry::counted)set_flag(counted_load,counted_store,flag::counted);
                        const auto shown=memory.load8(shown_flags,at(entry_address,entry::flags));
                        frame.tested8(shown&entry::shown);
                        if(shown&entry::shown)set_flag(shown_load,shown_store,flag::shown);
                    }
                }
            }
        }
    } else kept.source=memory.load32(slot_again,frame.local(local::slot));
    // A record with a target: the slot's acceptance at full charge.
    bool targeted=false;
    const auto first_value=memory.load32(first,at(kept.record,record::first));
    frame.compared(first_value,none);
    if(first_value!=none) {
        const auto on=memory.load8(first_on,at(kept.record,record::first_on));
        frame.byte_compared(on);
        targeted=on!=0u;
    }
    if(!targeted) {
        const auto second_value=memory.load32(second,at(kept.record,record::second));
        frame.compared(second_value,none);
        if(second_value!=none) {
            const auto on=memory.load8(second_on,at(kept.record,record::second_on));
            frame.byte_compared(on);
            targeted=on!=0u;
        }
    }
    if(targeted && (frame.accepts(kept)&0xffu)) {
        const auto charge_value=memory.load32(slot_charge,at(kept.source,slot::charge));
        const auto one_value=memory.load32(one_again,globals::one);
        frame.charged(charge_value,one_value);
        if(!sse::below(charge_value,one_value))set_flag(full_load,full_store,flag::full);
    }
    frame.restoring(1u);
    return Outcome::armed;
}
} // namespace hud_weapon_flags
} // namespace halo2::native_hud_weapon_flags
