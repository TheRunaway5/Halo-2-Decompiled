#pragma once
// Native TU1.5 first-person unit, entered at 3D2B0 (texture camera producer
// and first-person light pose units): which unit the first-person view
// shows, and its models.
//
//   first_person_unit::choose   3D2B0()
//
// The first-person unit (4CE3F0) starts as NONE and its model count
// (4CE3F4) as 0. An active override (the record at [51DF6C]: byte +0 set,
// word +2 mode 4) names a unit (+0x3C) which, when BCDF0 finds it as a unit
// (kind 3), is the first-person unit. Otherwise a unit set at 4C3AE0 (NONE
// for none) is. Otherwise the rendered view (4C66F8, NONE for none), unless
// 158290 claims it, shows its local player's unit (the player handles at
// [4F54F0]+0xC, the 0x21C-byte players of the array at 4F54F4, the unit at
// +0x2C; NONE without a player), and 168DED gathers up to four of that
// unit's first-person models for the view (at 4CE3F8), answering their
// count. Only the gathering path fills the models.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_first_person_unit.hpp) maps the role to the original
// instruction and owns the ABI of the children (BCDF0, 158290, 168DED). The
// values the original keeps in callee-saved registers across them are a
// Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_first_person_unit {
using native_object_update::at;
inline constexpr std::uint32_t none=0xffffffffu;
namespace globals {
inline constexpr std::uint32_t override_record=0x51df6cu;   // the override's record
inline constexpr std::uint32_t unit=0x4ce3f0u,count=0x4ce3f4u,models=0x4ce3f8u;
inline constexpr std::uint32_t set_unit=0x4c3ae0u;          // NONE for none
inline constexpr std::uint32_t view=0x4c66f8u;              // the rendered view, NONE for none
inline constexpr std::uint32_t players=0x4f54f0u;           // +0xC: the four local player handles
inline constexpr std::uint32_t player_array=0x4f54f4u;      // DataArray* of 0x21C-byte players
}
namespace override_record {inline constexpr std::uint32_t active=0u,mode=2u,unit=0x3cu;}
inline constexpr std::uint32_t override_mode=4u,unit_kind=3u,model_capacity=4u;
inline constexpr std::uint32_t local_players=0xcu,datum_data=0x44u;
namespace player_record {inline constexpr std::uint32_t size=0x21cu,unit=0x2cu;}

namespace first_person_unit {
enum class Access {record,unit_clear,count_clear,active,mode,override_unit,unit_store,set_unit,set_store,view,players,player,
    player_array,player_data,player_unit,gathered_store,count_store};
enum class Outcome {overridden,set,no_view,claimed,gathered};
// The original's ESI: the chosen unit, then the view; EDI: the override's unit.
struct Kept {std::uint32_t unit,candidate,view;};
// Frame:
//   find(unit, kind, kept) -> found    BCDF0(EAX the unit; kind pushed), refreshes kept.unit and kept.candidate
//   claims(view, kept) -> claimed      158290(EAX the view), refreshes kept.view
//   arguments(view, unit, capacity, models)   168DED's arguments pushed
//   gather() -> count                  168DED
template<class Memory,class Frame> Outcome choose(Memory &memory,Frame &frame) {
    using enum Access;
    const auto override_address=memory.load32(record,globals::override_record);
    memory.store32(unit_clear,globals::unit,none);
    memory.store32(count_clear,globals::count,0u);
    Kept kept{none,none,none};
    if((memory.load8(active,at(override_address,override_record::active))&0xffu) &&
       (memory.load16(mode,at(override_address,override_record::mode))&0xffffu)==override_mode) {
        kept.candidate=memory.load32(override_unit,at(override_address,override_record::unit));
        if(frame.find(kept.candidate,unit_kind,kept))kept.unit=kept.candidate;
    }
    memory.store32(unit_store,globals::unit,kept.unit);
    if(kept.unit!=none)return Outcome::overridden;
    const auto set_value=memory.load32(Access::set_unit,globals::set_unit);
    memory.store32(set_store,globals::unit,set_value);
    if(set_value!=none)return Outcome::set;
    kept.view=memory.load32(view,globals::view);
    if(kept.view==none)return Outcome::no_view;
    if(frame.claims(kept.view,kept))return Outcome::claimed;
    const auto player_handle=memory.load32(player,memory.load32(players,globals::players)+4u*kept.view+local_players);
    auto unit_handle=none;
    if(player_handle!=none) {
        const auto data=memory.load32(player_data,at(memory.load32(player_array,globals::player_array),datum_data));
        unit_handle=memory.load32(player_unit,data+player_record::size*(player_handle&0xffffu)+player_record::unit);
    }
    frame.arguments(kept.view,unit_handle,model_capacity,globals::models);
    memory.store32(gathered_store,globals::unit,unit_handle);
    memory.store32(count_store,globals::count,frame.gather());
    return Outcome::gathered;
}
} // namespace first_person_unit
} // namespace halo2::native_first_person_unit
