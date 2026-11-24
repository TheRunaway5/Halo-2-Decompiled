#pragma once
// Native TU1.5 weapon ranges, entered at 1ADC90 (combat action unit): the
// ranges a unit's weapons reach, for its combat actions.
//
//   weapon_ranges::measure   1ADC90(ECX = unit, ESI = ranges*; word) -> AL
//
// The unit's primary weapon (signed index byte +0x212 among its weapons
// +0x218) must exist; with the word NONE its definition must not have bit 5
// of +0x12C. 106160 answers the weapon's scale (XMM0); the definition's five
// ranges (+0x208 to +0x218) give the record's first four (alternately over
// and times the scale) and its widest (the larger of +0x218 and +0x208, over
// the scale, also kept on the stack); the kind and its range are cleared,
// then a melee weapon (1062A0) of kind 1 (word +0x294) takes kind 0x20, a
// ranged one (106230, through the dispatch) of kind 1 or 2 takes 8 or 0x10,
// each with the second range. The secondary weapon (index byte +0x213), when
// there is one, gives its first two ranges as they are, widens the third
// and fourth (with its own scale) and the widest; without one, those two
// take XMM0 as the children leave it.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); scalar SSE as the original orders it; the
// boundary adapter (native_tu15_weapon_ranges.hpp) maps the role to the
// original instruction and owns the ABI of the children. The values the
// original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_aim_clusters_systems.hpp"
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_weapon_ranges {
using native_object_update::at;
namespace sse=native_aim_clusters::sse;
namespace globals {inline constexpr std::uint32_t object_headers=0x4ecbd0u,tag_instances=0x4f0414u,one=0x46a0d0u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,handle_mask=0xffffu,
    none=0xffffffffu,any=0xffffu;
namespace unit {inline constexpr std::uint32_t primary=0x212u,secondary=0x213u,weapons=0x218u;}
namespace weapon {inline constexpr std::uint32_t flags=0x12cu,ranges=0x208u,widest=0x218u,kind=0x294u;inline constexpr unsigned unusable_bit=5u;}
inline constexpr std::uint32_t range=4u;
namespace record {inline constexpr std::uint32_t ranges=0u,widest=0x10u,kind=0x14u,secondary=0x18u,kind_range=0x20u;}
namespace kinds {inline constexpr std::uint32_t melee=0x20u,single=8u,double_=0x10u;}
namespace local {inline constexpr std::uint32_t widest=0xcu,word=0x14u;}

namespace weapon_ranges {
enum class Access {headers_load,datums_load,unit_load,primary_load,weapon_load,datums_again,weapon_object,tag_load,instances_load,
    definition_load,word_load,flags_load,range0,one_load,range0_store,range1,range1_store,range2,range2_store,range3,range3_store,
    widest_load,widest_against,widest_fallback,widest_store,widest_kept,kind_clear,kind_range_clear,melee_kind,melee_range,melee_store,
    melee_range_store,ranged_kind,single_range,single_store,single_range_store,double_range,double_store,double_range_store,
    headers_again,datums_third,unit_again,secondary_load,secondary_weapon,datums_fourth,secondary_object,secondary_tag,
    secondary_instances,secondary_definition,word_again,secondary_range0,secondary_one,third_now,secondary_range0_store,
    secondary_range1,secondary_range1_store,secondary_range2,third_store,secondary_range3,fourth_now,fourth_store,secondary_widest,
    secondary_widest_against,secondary_widest_fallback,widest_kept_load,final_widest_store,final_widest,final_widest_against,
    final_widest_fallback,none_first_store,none_second_store};
// The original's EBX (the weapon), EBP (the unit's offset), ESI (the record) and EDI (the definition).
struct Kept {std::uint32_t weapon_handle,unit_offset,ranges,definition_address;};
// Frame: see native_tu15_weapon_ranges.hpp.
template<class Memory,class Frame> bool measure(Memory &memory,Frame &frame,std::uint32_t unit_handle,Kept kept) {
    using enum Access;
    frame.framing();
    frame.compared(unit_handle,none);
    if(unit_handle==none){frame.restoring(0u);return false;}
    // The primary weapon and its definition.
    frame.saving_unit(unit_handle);
    const auto headers=memory.load32(headers_load,globals::object_headers);
    const auto datums=memory.load32(datums_load,at(headers,datum_data));
    kept.unit_offset=frame.unit_scaled();
    const auto unit_address=memory.load32(unit_load,datums+kept.unit_offset+header_address);
    const auto primary=memory.load8(primary_load,at(unit_address,unit::primary));
    if(!frame.slot_valid(primary,0u)){frame.restoring(2u);return false;}
    frame.saving_weapon();
    kept.weapon_handle=memory.load32(weapon_load,unit_address+4u*frame.slot(primary)+unit::weapons);
    frame.compared(kept.weapon_handle,none);
    if(kept.weapon_handle==none){frame.restoring(3u);return false;}
    const auto datums_now=memory.load32(datums_again,at(headers,datum_data));
    const auto object_address=memory.load32(weapon_object,datums_now+header_size*frame.indexed(kept.weapon_handle)+header_address);
    const auto tag=memory.load32(tag_load,object_address);
    const auto instances=memory.load32(instances_load,globals::tag_instances);
    kept.definition_address=memory.load32(definition_load,frame.tag_offset(tag)+instances+tag_data);
    const auto word=memory.load32(word_load,frame.local(local::word));
    if(frame.word_compared(word)) {
        if(frame.unusable(memory.load32(flags_load,at(kept.definition_address,weapon::flags)))){frame.restoring(3u);return false;}
    }
    // The primary's ranges over and times its scale.
    const auto scale=frame.scaled(kept);
    const auto r0=memory.load32(range0,at(kept.definition_address,weapon::ranges));
    const auto inverse=sse::divide(memory.load32(one_load,globals::one),scale);
    const auto first=sse::multiply(r0,inverse);
    frame.ranged_value(first,inverse);
    memory.store32(range0_store,at(kept.ranges,record::ranges),first);
    const auto second=sse::multiply(memory.load32(range1,at(kept.definition_address,weapon::ranges+range)),scale);
    frame.ranged_value(second,inverse);
    memory.store32(range1_store,at(kept.ranges,record::ranges+range),second);
    const auto third=sse::multiply(memory.load32(range2,at(kept.definition_address,weapon::ranges+2u*range)),inverse);
    frame.ranged_value(third,inverse);
    memory.store32(range2_store,at(kept.ranges,record::ranges+2u*range),third);
    const auto fourth=sse::multiply(memory.load32(range3,at(kept.definition_address,weapon::ranges+3u*range)),scale);
    frame.ranged_value(fourth,inverse);
    memory.store32(range3_store,at(kept.ranges,record::ranges+3u*range),fourth);
    const auto wider=[&](Access load_role,Access against_role,Access fallback_role,std::uint32_t definition) {
        const auto widest=memory.load32(load_role,at(definition,weapon::widest));
        const auto first_range=memory.load32(against_role,at(definition,weapon::ranges));
        return frame.widened(widest,first_range)?widest:memory.load32(fallback_role,at(definition,weapon::ranges));
    };
    const auto widest=sse::multiply(wider(widest_load,widest_against,widest_fallback,kept.definition_address),inverse);
    frame.widest_value(widest);
    memory.store32(widest_store,at(kept.ranges,record::widest),widest);
    memory.store32(widest_kept,frame.local(local::widest),widest);
    // The kind.
    frame.kind_cleared(kept);
    memory.store32(kind_clear,at(kept.ranges,record::kind),0u);
    memory.store32(kind_range_clear,at(kept.ranges,record::kind_range),0u);
    if(frame.melee(kept)) {
        if(frame.kind_compared(memory.load16(melee_kind,at(kept.definition_address,weapon::kind)),1u)) {
            const auto v=memory.load32(melee_range,at(kept.ranges,record::ranges+range));
            memory.store32(melee_store,at(kept.ranges,record::kind),kinds::melee);
            memory.store32(melee_range_store,at(kept.ranges,record::kind_range),v);
        }
    } else if(frame.ranged(kept)) {
        const auto kind=frame.kind_word(memory.load16(ranged_kind,at(kept.definition_address,weapon::kind)));
        if(frame.kind_compared(kind,1u)) {
            const auto v=memory.load32(single_range,at(kept.ranges,record::ranges+range));
            memory.store32(single_store,at(kept.ranges,record::kind),kinds::single);
            memory.store32(single_range_store,at(kept.ranges,record::kind_range),v);
        } else if(frame.kind_compared(kind,2u)) {
            const auto v=memory.load32(double_range,at(kept.ranges,record::ranges+range));
            memory.store32(double_store,at(kept.ranges,record::kind),kinds::double_);
            memory.store32(double_range_store,at(kept.ranges,record::kind_range),v);
        }
    }
    // The secondary weapon.
    const auto headers_now=memory.load32(headers_again,globals::object_headers);
    const auto datums_last=memory.load32(datums_third,at(headers_now,datum_data));
    const auto unit_now=memory.load32(unit_again,datums_last+kept.unit_offset+header_address);
    const auto secondary=memory.load8(secondary_load,at(unit_now,unit::secondary));
    const auto without=[&] {
        const auto value=frame.leftover();
        memory.store32(none_first_store,at(kept.ranges,record::secondary),value);
        memory.store32(none_second_store,at(kept.ranges,record::secondary+range),value);
        frame.answered();
        frame.restoring(3u);
        return true;
    };
    if(!frame.slot_valid(secondary,1u))return without();
    const auto secondary_handle=memory.load32(secondary_weapon,unit_now+4u*frame.slot(secondary)+unit::weapons);
    frame.compared(secondary_handle,none);
    if(secondary_handle==none)return without();
    const auto datums_fourth_value=memory.load32(datums_fourth,at(headers_now,datum_data));
    const auto secondary_object_address=memory.load32(secondary_object,datums_fourth_value+header_size*frame.indexed_secondary(secondary_handle)+header_address);
    const auto secondary_tag_value=memory.load32(secondary_tag,secondary_object_address);
    const auto secondary_instances_value=memory.load32(secondary_instances,globals::tag_instances);
    kept.definition_address=memory.load32(secondary_definition,frame.tag_offset_secondary(secondary_tag_value)+secondary_instances_value+tag_data);
    memory.load32(word_again,frame.local(local::word));
    const auto secondary_scale=frame.scaled_secondary(kept);
    const auto s0=memory.load32(secondary_range0,at(kept.definition_address,weapon::ranges));
    const auto one_now=memory.load32(secondary_one,globals::one);
    const auto third_now_value=memory.load32(third_now,at(kept.ranges,record::ranges+2u*range));
    memory.store32(secondary_range0_store,at(kept.ranges,record::secondary),s0);
    const auto s1=memory.load32(secondary_range1,at(kept.definition_address,weapon::ranges+range));
    const auto secondary_inverse=sse::divide(one_now,secondary_scale);
    frame.secondary_inverse(secondary_inverse);
    memory.store32(secondary_range1_store,at(kept.ranges,record::secondary+range),s1);
    auto third_widened=sse::multiply(memory.load32(secondary_range2,at(kept.definition_address,weapon::ranges+2u*range)),secondary_inverse);
    if(frame.kept_larger(third_now_value,third_widened))third_widened=third_now_value;
    memory.store32(third_store,at(kept.ranges,record::ranges+2u*range),third_widened);
    auto fourth_widened=sse::multiply(memory.load32(secondary_range3,at(kept.definition_address,weapon::ranges+3u*range)),secondary_scale);
    const auto fourth_now_value=memory.load32(fourth_now,at(kept.ranges,record::ranges+3u*range));
    if(frame.kept_larger(fourth_now_value,fourth_widened))fourth_widened=fourth_now_value;
    memory.store32(fourth_store,at(kept.ranges,record::ranges+3u*range),fourth_widened);
    const auto secondary_widest_value=wider(secondary_widest,secondary_widest_against,secondary_widest_fallback,kept.definition_address);
    const auto kept_widest=memory.load32(widest_kept_load,frame.local(local::widest));
    const auto secondary_widened=sse::multiply(secondary_widest_value,secondary_inverse);
    if(frame.kept_widest(kept_widest,secondary_widened)) {
        frame.restoring_final(0u);
        memory.store32(final_widest_store,at(kept.ranges,record::widest),kept_widest);
        frame.answered_final(0u);
        return true;
    }
    const auto last=memory.load32(final_widest,at(kept.definition_address,weapon::widest));
    const auto against=memory.load32(final_widest_against,at(kept.definition_address,weapon::ranges));
    const bool wide=frame.widened(last,against);
    const auto chosen=wide?last:memory.load32(final_widest_fallback,at(kept.definition_address,weapon::ranges));
    frame.restoring_final(wide?1u:2u);
    memory.store32(final_widest_store,at(kept.ranges,record::widest),frame.final_value(sse::multiply(chosen,secondary_inverse)));
    frame.answered_final(wide?1u:2u);
    return true;
}
} // namespace weapon_ranges
} // namespace halo2::native_weapon_ranges
