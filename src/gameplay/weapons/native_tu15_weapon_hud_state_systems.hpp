#pragma once
// Native TU1.5 weapon HUD state, entered at 13DA89 (weapon HUD unit): one
// weapon's 0x38-byte HUD state (SourceWeaponHudState in
// include/halo2/weapon_hud.hpp), filled from the weapon's status.
//
//   weapon_hud_state::fill   13DA89(EAX = state, ECX = weapon; output*) -> RET 4
//
// 1055F0 writes the weapon's status into the frame (0x38 bytes); the state
// takes from it, in the original's order: the overheated byte, the loaded,
// capacity, reserve and maximum reserve rounds, the magazine, input,
// charging, target, locking and locked bytes, the heat, age, charge and lock
// progress, the target position (copied as REP MOVSD copies it, in the
// direction the string direction flag gives), and the weapon's datum, with
// the present byte 1. The output gets the weapon definition's +0x2B4 (the
// weapon object's tag through the tag instances at 4F0414).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), including the frame's status and the object the
// original spills there; the boundary adapter (native_tu15_weapon_hud_state.hpp)
// maps the role to the original instruction and owns the ABI of the child
// (1055F0). The values the original keeps in callee-saved registers across it
// are a Kept, refreshed from the registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_weapon_hud_state {
using native_object_update::at;
namespace globals {inline constexpr std::uint32_t objects=0x4ecbd0u,tag_instances=0x4f0414u;}
inline constexpr std::uint32_t datum_data=0x44u,header_size=12u,header_address=8u,tag_size=16u,tag_data=8u,handle_mask=0xffffu;
// The HUD state (SourceWeaponHudState).
namespace state {
inline constexpr std::uint32_t datum=0u,present=4u,loaded=6u,reserve=8u,capacity=0xau,maximum_reserve=0xcu,age=0x10u,heat=0x14u,
    charge=0x18u,overheated=0x20u,magazine_working=0x21u,input_held=0x22u,charging=0x23u,target=0x24u,locking=0x25u,locked=0x26u,
    lock_progress=0x28u,target_position=0x2cu;
}
// 1055F0's status, in the frame.
namespace status {
inline constexpr std::uint32_t heat=0u,age=4u,overheated=8u,input_held=9u,charge=0xcu,charging=0x10u,target=0x11u,locking=0x12u,
    locked=0x13u,lock_progress=0x14u,target_position=0x18u,magazine_working=0x26u,loaded=0x28u,capacity=0x2au,reserve=0x2cu,
    maximum_reserve=0x2eu;
}
inline constexpr std::uint32_t position_words=3u;
namespace definition {inline constexpr std::uint32_t output=0x2b4u;}

namespace weapon_hud_state {
enum class Access {objects,object_data,object_address,object_spill,
    overheated,overheated_store,loaded,loaded_store,capacity,capacity_store,reserve,heat,reserve_store,maximum,maximum_store,
    magazine,magazine_store,input,input_store,charging,charging_store,target,target_store,locking,heat_store,age,locking_store,
    locked,instances,age_store,charge,charge_store,progress,datum_store,locked_store,object_reload,present_store,progress_store,
    position_read,position_write,object_tag,weapon_definition,output_value,output_address,output_store};
// The original's ESI, EDI and EBX across 1055F0: the state, the weapon and
// the handle mask.
struct Kept {std::uint32_t state,weapon,mask;};
// Frame:
//   status(offset)                    the frame's status field (or the spilled object) now
//   argument()                        the output's slot now
//   query(weapon, status, kept)       1055F0(the weapon, the status's address), refreshes kept
//   copy_step(), copy_started()       the REP MOVSD's direction and registers
//   leaving()                         the saved EDI and ESI back before the output's store
inline constexpr std::uint32_t spilled_object=0x3cu;   // the frame's slot past the status
template<class Memory,class Frame> void fill(Memory &memory,Frame &frame,std::uint32_t state_address,std::uint32_t weapon) {
    using enum Access;
    const auto data=memory.load32(object_data,at(memory.load32(Access::objects,globals::objects),datum_data));
    memory.store32(object_spill,frame.status(spilled_object),
        memory.load32(object_address,data+header_size*(weapon&handle_mask)+header_address));
    Kept kept{state_address,weapon,handle_mask};
    frame.query(weapon,frame.status(0u),kept);
    const auto s=[&](std::uint32_t offset){return frame.status(offset);};
    const auto d=[&](std::uint32_t offset){return at(kept.state,offset);};
    memory.store8(overheated_store,d(state::overheated),memory.load8(overheated,s(status::overheated)));
    memory.store16(loaded_store,d(state::loaded),memory.load16(Access::loaded,s(status::loaded)));
    memory.store16(capacity_store,d(state::capacity),memory.load16(Access::capacity,s(status::capacity)));
    const auto reserve_value=memory.load16(Access::reserve,s(status::reserve));
    const auto heat_value=memory.load32(Access::heat,s(status::heat));
    memory.store16(reserve_store,d(state::reserve),reserve_value);
    memory.store16(maximum_store,d(state::maximum_reserve),memory.load16(maximum,s(status::maximum_reserve)));
    memory.store8(magazine_store,d(state::magazine_working),memory.load8(magazine,s(status::magazine_working)));
    memory.store8(input_store,d(state::input_held),memory.load8(input,s(status::input_held)));
    memory.store8(charging_store,d(state::charging),memory.load8(Access::charging,s(status::charging)));
    memory.store8(target_store,d(state::target),memory.load8(Access::target,s(status::target)));
    const auto locking_value=memory.load8(Access::locking,s(status::locking));
    memory.store32(heat_store,d(state::heat),heat_value);
    const auto age_value=memory.load32(Access::age,s(status::age));
    memory.store8(locking_store,d(state::locking),locking_value);
    const auto locked_value=memory.load8(Access::locked,s(status::locked));
    const auto instances_address=memory.load32(instances,globals::tag_instances);
    memory.store32(age_store,d(state::age),age_value);
    memory.store32(charge_store,d(state::charge),memory.load32(Access::charge,s(status::charge)));
    const auto progress_value=memory.load32(progress,s(status::lock_progress));
    memory.store32(datum_store,d(state::datum),kept.weapon);
    memory.store8(locked_store,d(state::locked),locked_value);
    const auto object=memory.load32(object_reload,frame.status(spilled_object));
    memory.store8(present_store,d(state::present),1u);
    memory.store32(progress_store,d(state::lock_progress),progress_value);
    frame.copy_started();
    const auto step=frame.copy_step();
    for(std::uint32_t word=0;word<position_words;++word)
        memory.store32(position_write,d(state::target_position)+word*step,memory.load32(position_read,s(status::target_position)+word*step));
    const auto tag=memory.load32(object_tag,object);
    const auto definition_address=memory.load32(weapon_definition,tag_size*(tag&kept.mask)+instances_address+tag_data);
    const auto value=memory.load32(output_value,at(definition_address,definition::output));
    const auto output=memory.load32(output_address,frame.argument());
    frame.leaving();
    memory.store32(output_store,output,value);
}
} // namespace weapon_hud_state
} // namespace halo2::native_weapon_hud_state
