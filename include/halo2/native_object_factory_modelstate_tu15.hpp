#pragma once
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <span>

// Native model-state owners of TU1.5 object_new (track "modelstate"): change
// colors (C0270), regions and permutations (BC5C0), function colors (C0910).
// See docs/native_object_factory_tracks/modelstate.md. Each owner replicates
// its original routine's stores (address, width, value, order) over the live
// world bank. Floating-point owners take the live x87 control word and MXCSR;
// every SSE/x87 step runs the original instruction under those controls.
// Callees that are not native yet are named refusals (std::runtime_error).
namespace halo2::native_object_factory_tu15 {
// 1343E0: RGB at `rgb` to hue/saturation/value at `hsv` (+8 value, +4
// saturation, then hue in turns, wrapped once into [0, 1)). Returns `hsv`.
std::uint32_t color_rgb_to_hsv(SourcePhysicsContactMemory&,std::uint32_t rgb,std::uint32_t hsv,std::uint32_t mxcsr);
// 134510: hue/saturation/value at `hsv` to RGB at `rgb` (six-sector switch).
// A sector outside 0..5 (hue outside [0, 1) or NaN) is a refusal: the
// original jumps through bytes past its table. Returns `rgb`.
std::uint32_t color_hsv_to_rgb(SourcePhysicsContactMemory&,std::uint32_t rgb,std::uint32_t hsv,std::uint32_t mxcsr);
// 1349E0: packed 0x00RRGGBB from the RGB floats at `rgb` (x87 FMUL by 255
// and FISTP, low byte of each).
std::uint32_t color_pack(const SourcePhysicsContactMemory&,std::uint32_t rgb,std::uint16_t x87_control);
// BCD80: copies change color `channel` (the current half: rows count..2*count
// of body +124/+126) to `out` when 0 <= channel < count.
bool object_color_channel(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t channel,std::uint32_t out);
// 3DD00: publishes the packed current change colors into the object's render
// row ([0x515F14] rows of 0x100 at body +CC: +A8 colors, +B8 count). Skipped
// for a NONE row, and without `force` unless definition +1C bit 0 is set.
void object_color_publish(SourcePhysicsContactMemory&,std::uint32_t object,bool force,std::uint16_t x87_control);
// C0270: initial change colors. Each of the definition's change color rows
// (+B0, 16 bytes) takes the request color (`mask` bit, `colors` + 12 * i) or
// [[0x474C70]], or a position-hashed permutation blend (37DC0A fmod, 1343E0,
// 134510); clamps to [0, 1], copies to the current half and publishes
// (3DD00, forced).
void object_change_colors_initialize(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,
    std::uint32_t colors,std::uint16_t x87_control,std::uint32_t mxcsr);
// C0910: function-scaled change colors when definition +1C bit 0 is set:
// copies each base color to the current half, blends it by each function
// entry's (+24) object function value (BCB70, 134730: the attachments
// track's object_function_value and color_interpolate), multiplies it by
// each entry's scale (+20) value and clamps it. Returns the original AL (0
// when skipped or empty, else 1).
std::uint32_t object_function_colors(SourcePhysicsContactMemory&,std::uint32_t object,std::uint16_t x87_control,
    std::uint32_t mxcsr);

// BC6C0: the object's region bank: the signed region count (body +118 / 10)
// and its three arrays at body + [+11A]: states, cached states (+count) and
// 8-byte region rows (+2 * count). Every output slot is optional except
// `count_slot`; stores follow the original (count slot written twice).
void object_region_bank(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t count_slot,
    std::uint32_t active_slot,std::uint32_t cached_slot,std::uint32_t rows_slot);
// 1700E0: chooses each model region's permutation (model +70 regions of 16
// bytes at +74): variant permutation candidates (170930, seeded random over
// weighted rows) or the region's unflagged permutations, then a weighted
// pick (170B40); regions whose `mask` bit is set keep the reset state.
// Writes states[i] and the 8-byte region rows; advances [[0x4F3CD8]].
void model_region_permutations_select(SourcePhysicsContactMemory&,std::uint32_t model_tag,std::int32_t variant,
    std::uint32_t states,std::uint32_t rows,std::uint32_t mask,std::uint16_t x87_control,std::uint32_t mxcsr);
// 17E060: restarts the object's region-attached effect (the first +11C
// attachment of type 2) when its [0x4F720C] effect datum is live. Restarting
// it (17B1F0, effect locations) is a refusal.
void object_region_effect_restart(SourcePhysicsContactMemory&,std::uint32_t object);
// 1CE0E0: with a Havok component (+B4) whose [0x52BE5C] row has flag bit
// 11, the first region whose permutation's physics byte (+6) changed from
// `previous` runs the physics rebuild 1CC310(object, 0) (the havok track's
// havok_component_state_flags, which still refuses at 1D9510/1D73D0).
// `previous` holds the caller's prior states; reading past it is a refusal.
void object_region_physics_check(SourcePhysicsContactMemory&,std::uint32_t object,
    std::span<const std::uint8_t> previous,std::uint16_t x87_control,std::uint32_t mxcsr);
// BC5C0: initial regions and permutations: BC6C0, the selection (1700E0, or
// reset rows without a model), the cached copy, 17E060 and 1CE0E0.
void object_regions_initialize(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,
    std::uint16_t x87_control,std::uint32_t mxcsr);

// Factory entry points (ObjectFactoryServices shapes).
//
// 1D3380(record ESI, graph tag EDI, model tag EDX, `enable` byte): returns
// false, storing nothing, when a model is given and the graph is NONE or its
// node count (graph +C, signed word) differs from the model's (+78).
// Otherwise initializes the manager record (0x84 bytes) and sets the
// default state (1D5320, the weapon track's weapon_animation::
// manager_set_state, with "any" labels, flags 0x317, channel flags 0x3F);
// returns true. Callers should pass the live controls; the defaults are the
// TU1.5 game thread's (x87 0x027F, MXCSR 0x1F80).
bool animation_manager_setup(SourcePhysicsContactMemory&,std::uint32_t record,std::uint32_t graph_tag,
    std::uint32_t model_tag,bool enable,std::uint16_t x87_control=0x027fu,std::uint32_t mxcsr=0x1f80u);
// D6E40: clears the widget list (body +DC) and creates the definition's
// widgets (+9C count, +A0 rows of 8: group tag, tag). A widget of a known
// type ('ant!' 118480, 'clwd' 119060, 'tdtl' C5B00) with a tag needs its
// creator, which is a refusal (before its [0x4ECBF0] datum is allocated).
void object_widgets_create(SourcePhysicsContactMemory&,std::uint32_t object);
// C0CD0: the definition's attachments (+94 count, +98 rows of 0x18) into the
// body's attachment block (+11E, 8-byte rows: type byte, handle). Lights
// (C2260, then body +4 |= 0x10), looping sounds (18BD80, body +4 |= 0x20)
// and contrails (17E940) are the attachments track's creators; effects
// (179540) still refuse at 178E30. Other kinds (5) and NONE tags store their
// row with a NONE handle.
void object_attachments_create(SourcePhysicsContactMemory&,std::uint32_t object,std::uint16_t x87_control,
    std::uint32_t mxcsr);
// C06C0: type finish. Projectiles (header type byte 5) clear body +DC, other
// types run D6E40; then C0CD0.
void object_type_finish(SourcePhysicsContactMemory&,std::uint32_t object,std::uint16_t x87_control,std::uint32_t mxcsr);
}
