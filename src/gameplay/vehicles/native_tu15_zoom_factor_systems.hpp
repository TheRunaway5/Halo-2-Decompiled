#pragma once
// Native TU1.5 zoom factor, entered at CAD60 (vehicle director unit): resolve
// a unit's magnification factor for a zoom level, delegating to 106160 when
// the unit's active weapon owns its zoom (106040) and otherwise interpolating
// the globals camera magnification range ([4ECC1C]+0x134).
//
//   zoom_factor::compute   CAD60(EAX = unit_handle, BX = zoom_level) -> XMM0; RET 0
//
// The unit handle resolves into the object header array ([4ECBD0]+0x44,
// 12-byte entries). If the unit's current weapon slot (signed byte at +0x212)
// is not -1 and its weapon handle (+0x218 + 4*slot) is not -1, 106040 tests
// whether the weapon supplies its own magnification; if so, CAD60 tail-calls
// 106160(EAX = weapon_handle, ECX = EBX). Otherwise CAD60 inspects bit 0 of
// byte +0x12E on the weapon's tag definition ([4F0414] + 16*(tag&0xFFFF) + 8)
// and returns the current XMM0 (1.0f, or what 106040 left) when set. Falling
// through, when BX != -1 it linearly interpolates [camera+0xBC..0xC0] using
// BX / (levels - 1) when levels != 1 (or the live XMM1 when levels == 1).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter (native_tu15_zoom_factor.hpp)
// maps the role to the original instruction, owns the ABI of 106040 and the
// tail call 106160, and leaves through the original's epilogue.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_zoom_factor {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t one=0x46a0d0u,objects=0x4ecbd0u,definitions=0x4f0414u,game=0x4ecc1cu;}
inline constexpr std::uint32_t datum_data=0x44u,handle_mask=0xffffu,header_size=12u,header_body=8u,
    definition_size=16u,definition_body=8u,none=0xffffffffu,no_slot=0xffffu,local_flag_offset=0xbu;
namespace unit_field {inline constexpr std::uint32_t weapon_slot=0x212u,weapons=0x218u;}
namespace weapon_field {inline constexpr std::uint32_t tag=0u,flags=0x12eu;}
namespace camera_field {inline constexpr std::uint32_t table=0x134u,levels=0xb8u,minimum=0xbcu,maximum=0xc0u;}

namespace zoom_factor {
enum class Access {
    one_load,objects_load,objects_data,unit_load,weapon_slot,weapon_handle,
    weapon_objects_data,weapon_object,weapon_tag,definitions_load,weapon_definition,weapon_flags,local_flag_store,
    globals_load,camera_globals,zoom_levels,max_zoom,min_zoom_sub,min_zoom_add};
enum class Exit {returned,tail_weapon};
// The original's EBX (zoom level), ESI (weapon handle) and EDI (objects array)
// across 106040.
struct Kept {std::uint32_t zoom_reg,weapon,objects;};

// Frame: see native_tu15_zoom_factor.hpp.
template<class Memory,class Frame> Exit compute(Memory &memory,Frame &frame,std::uint32_t unit_handle,Kept &kept) {
    using enum Access;
    frame.reserve_slot();
    const auto one_bits=memory.load32(one_load,globals::one);
    frame.set_default_weight(one_bits);
    frame.save_esi();
    const auto unit_index=frame.unit_index(unit_handle);
    frame.save_edi();
    kept.objects=memory.load32(objects_load,globals::objects);
    const auto headers=memory.load32(objects_data,at(kept.objects,datum_data));
    const auto unit_addr=memory.load32(unit_load,at(headers+header_size*unit_index,header_body));
    const auto slot_byte=memory.load8(weapon_slot,at(unit_addr,unit_field::weapon_slot));
    const auto slot_word=frame.check_slot(slot_byte);
    frame.copy_default_result();
    if(slot_word!=no_slot) {
        const auto slot_index=frame.slot_index(slot_word);
        kept.weapon=memory.load32(weapon_handle,at(unit_addr,unit_field::weapons)+4u*slot_index);
        if(frame.has_weapon(kept.weapon)) {
            if(frame.weapon_owns_zoom(kept))return Exit::tail_weapon;
            const auto weapon_headers=memory.load32(weapon_objects_data,at(kept.objects,datum_data));
            const auto weapon_idx=frame.weapon_index(kept.weapon);
            const auto weapon_addr=memory.load32(weapon_object,at(weapon_headers+header_size*weapon_idx,header_body));
            const auto tag_handle=memory.load32(weapon_tag,at(weapon_addr,weapon_field::tag));
            const auto defs=memory.load32(definitions_load,globals::definitions);
            const auto def_idx=frame.tag_index(tag_handle);
            const auto def_addr=memory.load32(weapon_definition,at(defs+definition_size*def_idx,definition_body));
            const auto raw_flag=memory.load8(weapon_flags,at(def_addr,weapon_field::flags));
            const auto bit=frame.mask_weapon_flag(raw_flag);
            memory.store8(local_flag_store,frame.local_flag(),bit);
            if(frame.flag_stops_zoom(bit))return Exit::returned;
        }
    }
    const bool unzoomed=frame.is_unzoomed(kept.zoom_reg);
    const auto game_ptr=memory.load32(globals_load,globals::game);
    const auto camera=memory.load32(camera_globals,at(game_ptr,camera_field::table));
    if(unzoomed)return Exit::returned;
    const auto levels=memory.load32(zoom_levels,at(camera,camera_field::levels));
    if(!frame.single_level(levels))frame.compute_fraction(kept.zoom_reg,levels);
    const auto max_bits=memory.load32(max_zoom,at(camera,camera_field::maximum));
    frame.load_max(max_bits);
    const auto min_sub_bits=memory.load32(min_zoom_sub,at(camera,camera_field::minimum));
    frame.subtract_and_scale(min_sub_bits);
    const auto min_add_bits=memory.load32(min_zoom_add,at(camera,camera_field::minimum));
    frame.add_min(min_add_bits);
    return Exit::returned;
}
} // namespace zoom_factor
} // namespace halo2::native_zoom_factor
