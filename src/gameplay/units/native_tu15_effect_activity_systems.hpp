#pragma once
// Native TU1.5 effect activity, entered at 17C5C0 (physics live support and
// death cycle units): an effect's active flag brought up to date; the
// result is whether the effect was retired.
//
//   effect_activity::update   17C5C0(EBX = effect handle) -> AL
//
// An effect (0x190 bytes in [4F720C]) without a location (+0x20 NONE) is
// active. Otherwise it is inactive when 3EB10 accepts its location and it
// has an owner (+0x48), or while the game globals ([51DF50]) hold effects
// (+5); failing both, its position (+0x1C) decides: through 11E7B0 for a
// tag with flag bit 2, 11E780 for any other, the effect being active when
// that test fails. When the activity differs from the effect's flag (bit 3
// of +2) the flag follows; an effect that becomes active with neither bit 1
// nor bit 9 of +2 set is offered to 17A4A0, and when that accepts the effect
// is retired.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_effect_activity.hpp) maps the role to the original
// instruction and owns the ABI of the children; the handle, effect and tag
// the original keeps in callee-saved registers across them are a Kept,
// refreshed from the registers each returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_effect_activity {
using native_object_update::at;
using native_object_update::none;
namespace globals {
inline constexpr std::uint32_t effects=0x4f720cu;          // DataArray* of 0x190-byte effects
inline constexpr std::uint32_t tag_instances=0x4f0414u;    // 16-byte tag instances (+8 the tag's address)
inline constexpr std::uint32_t game=0x51df50u;             // the game globals, or zero
}
inline constexpr std::uint32_t datum_data=0x44u,effect_size=0x190u,tag_instance_shift=4u,instance_address=8u;
namespace effect {
inline constexpr std::uint32_t flags=2u,tag=4u,position=0x1cu,location=0x20u,owner=0x48u;
inline constexpr std::uint32_t active_bit=3u,kept_bit=1u,persistent_bit=9u;   // in the word at +2
}
inline constexpr std::uint32_t tag_flags_offset=0u,tag_position_bit=2u,game_hold=5u;
inline constexpr std::uint16_t no_location=0xffffu;

namespace effect_activity {
enum class Access {
    effects,effects_data,tag_instances,effect_tag,tag_address,location,owner,game,hold,tag_flags,flags_read,
    flags_set,flags_clear,flags_word
};
enum class Test {location,position,flagged_position,retire};
struct Kept {
    std::uint32_t handle;   // EBX
    std::uint32_t effect;   // ESI
    std::uint32_t tag;      // EDI
};
// Frame:
//   test(kept, Test, argument)   the child (3EB10 with the location, 11E7B0
//                                or 11E780 with the position, 17A4A0 with
//                                the handle); refreshes kept; returns AL
//   negated(al)                  NEG AL; SBB AL,AL; INC AL: AL becomes AL == 0
//   held(), owned()              the activity's other sources (XOR AL,AL)
//   compared(active, flag)       CMP AL,CL
//   retired(kept)                XOR ESI,ESI
enum class Result {unchanged,deactivated,activated,kept,retired};
template<class Memory,class Frame> Result update(Memory &memory,Frame &frame,std::uint32_t handle) {
    using enum Access;
    const auto table=memory.load32(effects,globals::effects);
    const auto data=memory.load32(effects_data,at(table,datum_data));
    const auto instances=memory.load32(tag_instances,globals::tag_instances);
    Kept kept{handle,effect_size*(handle&0xffffu)+data,0u};
    const auto tag_handle=memory.load32(effect_tag,at(kept.effect,effect::tag));
    kept.tag=memory.load32(tag_address,((tag_handle&0xffffu)<<tag_instance_shift)+instances+instance_address);
    bool active=true;
    const auto location_index=memory.load16(location,at(kept.effect,effect::location));
    if(location_index!=no_location) {
        const bool located=frame.test(kept,Test::location,location_index)!=0;
        if(located && memory.load32(owner,at(kept.effect,effect::owner))!=none){active=false;frame.owned();}
        else {
            const auto game_address=memory.load32(game,globals::game);
            if(game_address && memory.load8(hold,at(game_address,game_hold))!=0){active=false;frame.held();}
            else {
                const bool flagged=(memory.load32(tag_flags,at(kept.tag,tag_flags_offset))>>tag_position_bit)&1u;
                const auto test=flagged?Test::flagged_position:Test::position;
                active=frame.negated(frame.test(kept,test,at(kept.effect,effect::position)))!=0;
            }
        }
    }
    const bool flag=(memory.load8(flags_read,at(kept.effect,effect::flags))>>effect::active_bit)&1u;
    frame.compared(active,flag);
    if(active==flag)return Result::unchanged;
    if(!active) {
        memory.store8(flags_clear,at(kept.effect,effect::flags),
            memory.load8(flags_clear,at(kept.effect,effect::flags))&~(1u<<effect::active_bit));
        return Result::deactivated;
    }
    memory.store8(flags_set,at(kept.effect,effect::flags),
        memory.load8(flags_set,at(kept.effect,effect::flags))|(1u<<effect::active_bit));
    const auto word=memory.load16(flags_word,at(kept.effect,effect::flags));
    if((word>>effect::kept_bit)&1u || (word>>effect::persistent_bit)&1u)return Result::kept;
    if(!frame.test(kept,Test::retire,kept.handle))return Result::activated;
    frame.retired(kept);
    return Result::retired;
}
} // namespace effect_activity
} // namespace halo2::native_effect_activity
