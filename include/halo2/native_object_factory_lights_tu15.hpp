#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>

// The per-tick light update of TU1.5 (track "lights"): every live light of
// the light data array is refreshed, moved with its owner object and retired
// when its lifetime ends. See docs/native_object_factory_tracks/lights.md.
//
// The owners work on the live world memory and keep the original store order,
// widths and values. Floating-point steps run under the caller's live x87
// control word and MXCSR (`fp`). Callees that are not native yet are named
// refusals (std::runtime_error), never a call into original code.
namespace halo2::native_object_factory_tu15 {

// C5290 (stack: light, byte `update_owner_flag`; RET 8): delete a light.
// A connected light is unlinked from the light partition first. With
// `update_owner_flag`, the light's parent object (+58) keeps its body +4
// bit 6 ("has lights") only while another live light names it; the light's
// datum is then freed.
void light_delete(SourcePhysicsContactMemory&,std::uint32_t light,bool update_owner_flag);

// C3430 (no arguments): the light tick. For every live light, in index
// order: the light's intensity and colours are recomputed (C2CB0,
// attachments' `light_update`); a connected light whose owner moved this
// tick is unlinked and reconnected at its new place (C4D30, placement's
// `light_reconnect`); a timed light whose lifetime has passed is deleted
// (C5290).
void lights_update(SourcePhysicsContactMemory&,NativeTu15PhysicsFrameFp&);
}
