#pragma once
// Native TU1.5 unit actions, entered at C9010 (physics live support unit):
// the normal unit action wrapper, the unit's tick callback.
//
//   unit_actions::run   C9010(unit) -> AL, RET 4
//
// The unit's updates run in the original's order: C6E90 (the driver's
// controls), C7DD0 and C85A0 (the unit pushed), then C8390, C8C20, 117340,
// C8E70, C8CF0 and C7580 (EAX the unit). The answer is whether any of the
// first five answered AL set (their AL ORed, in BL).
//
// This is the game logic only: a sequence of calls with no memory of its own
// beyond the argument it reads. The boundary adapter
// (native_tu15_unit_actions.hpp) owns the ABI of the children; the unit and
// the answer, which the original keeps in ESI and BL across them, are a
// Kept, refreshed from the registers each child returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_unit_actions {
namespace unit_actions {
enum class Access {unit_argument};
enum class Outcome {unchanged,changed};
// The updates, in order: those whose AL joins the answer, then the others.
enum class Update {controls,c7dd0,c85a0,c8390,c8c20,u117340,c8e70,camouflage,c7580};
inline constexpr unsigned answering_updates=5u;
inline constexpr Update updates[]{Update::controls,Update::c7dd0,Update::c85a0,Update::c8390,Update::c8c20,Update::u117340,
    Update::c8e70,Update::camouflage,Update::c7580};
// The original's ESI and BL across the updates.
struct Kept {std::uint32_t unit,changed;};
// Frame:
//   argument()                     the unit's slot now
//   update(which, kept) -> AL      the update, refreshes kept
//   answered(al, kept)             the answer gathers AL (MOV BL,AL first, then OR BL,AL)
template<class Memory,class Frame> Outcome run(Memory &memory,Frame &frame) {
    Kept kept{memory.load32(Access::unit_argument,frame.argument()),0u};
    for(unsigned k=0;k<sizeof(updates)/sizeof(updates[0]);++k) {
        const auto al=frame.update(updates[k],kept);
        if(k<answering_updates) {
            kept.changed=(k?kept.changed|al:al)&0xffu;
            frame.answered(k,kept);
        }
    }
    return kept.changed?Outcome::changed:Outcome::unchanged;
}
} // namespace unit_actions
} // namespace halo2::native_unit_actions
