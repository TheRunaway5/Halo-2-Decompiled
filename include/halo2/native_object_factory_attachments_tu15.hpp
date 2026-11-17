#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>

// Native attachment creators of TU1.5 object_new (track "attachments"): the
// owners C0CD0 (object_attachments_create) calls for each definition
// attachment row, and the object function value subtree (BCB70) they share
// with C0910. See docs/native_object_factory_tracks/attachments.md. Each owner
// replicates its original routine's stores (address, width, value, order) and
// result over the live world bank. Floating-point owners take the live x87
// control word and MXCSR. Callees that are not native yet are named refusals
// (std::runtime_error), never a call into original code.
namespace halo2::native_object_factory_tu15 {
// An output pointer of an original routine. A nonzero `address` is world
// memory: every store the original makes through it is replayed there in
// order. Address 0 is the caller's stack frame (the original's stores there
// are never observable outside the call): the native keeps the last stored
// value in `value` only.
struct OriginalOutput {
  std::uint32_t address{};
  std::uint32_t value{};
};

// 18BD80 (ESI tag, EDI object, stack: word index, primary scale): a looping
// sound attachment. NONE for a NONE tag or a full [0x4F9B5C] array (native
// 16E6A0); otherwise the new 0x18-byte row: +2 3, +4 0, +5 |= 1, +C tag; with
// an object, +3 1, +14 scale, +10 object, +6 index; without one, +6 -1, +3 0,
// +10 0, +14 0xFF, +16 -1. C0CD0 passes row +4 (tag), its object, the row
// index and row +10, and ORs body +4 with 0x20 for a non-NONE result.
std::uint32_t object_looping_sound_attachment_create(SourcePhysicsContactMemory&,std::uint32_t tag,
    std::uint32_t object,std::uint32_t index,std::uint32_t primary_scale);

// 10FB70 (EBX object, stack: name, value, slot): a scripted function
// variable override ([0x51DAF4]: 32 rows of 0x14 bytes, objects at +280).
// Stores NONE to `slot`, then for a matching row its interpolated value (game
// time [0x51DF54] +8 against row +C/+10, SSE under `mxcsr`) or target value,
// and the row index. Returns true when a row matches.
bool object_function_variable(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& slot,std::uint32_t mxcsr);

// BEC50 (stack: object, name, value, active): the object level's function
// value callback (type slot +4C). Known names publish their value (clamped to
// [0, 1] where the original clamps) and `active` = value > 0 (1 for the
// names BE9A0 answers); unknown names publish 0 and 0 and return false. Its
// callees are native: C1F90 (the heading, 0x700055C), BEAA0 (0x90006B5),
// BE9A0 (the object's +24..+26 bytes), 15B2D0 (the player scale of
// 0xF0005AC), C3910 (the lights' transition, 0x130006BB) and the console
// clock's BEBE0, BEB00, BEB40 and BEB90 (below).
std::uint8_t object_base_function_value(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);

// The console's local date and time as 13BB10 unpacks GetLocalTime (33BA6B)
// into dwords. BEC50's clock names read it; the native takes the host's
// local time.
struct LocalTime {
  std::uint32_t year,month,day,hour,minute,second;
};
// 13BB70: the holiday of a date: 1 on 12/25, 2 on 1/1, 3 on 10/31, 4 on 7/7,
// 5 on 7/4, 6 on 12/12, else 0.
std::uint32_t local_holiday(const LocalTime&);
// BEBE0 (0x70006BA): a holiday's value (bits): 0.25, 0.5, 0.75 and 1.0 for
// holidays 1..4, else 0.
std::uint32_t holiday_function_value(std::uint32_t holiday);
// BEB00 (0xA0006B6): the hour hand: the hour clamped to [1, 24], modulo 12,
// times 1/12 (CVTSI2SS, MULSS under `mxcsr`).
std::uint32_t clock_hour_value(const LocalTime&,std::uint32_t mxcsr);
// BEB40 / BEB90 (0xC0006B7 / 0xC0006B8): the minute or second hand: the
// value clamped to [0, 59] times 1/60.
std::uint32_t clock_minute_value(std::uint32_t minute,std::uint32_t mxcsr);

// 10DE40 (stack: object, name; EBX value, EDI active): clears both outputs,
// walks the type chain's +4C function value callbacks (0x474B90, object
// first) until one accepts, and clears both again when none does. The
// object (BEC50), unit (C9070), biped (E0900), vehicle (F9270) and weapon
// (1046C0) callbacks are native; the projectile, device and machine/crate
// callbacks (100E70, 10BE20, 11E490) are refusals when reached.
std::uint8_t object_type_function_value(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);

// ---- Type-level function value callbacks (10DE40 chain, 2026-11-14) ----
// Each takes the original's stack outputs as `value` / `active` and returns
// AL. Unknown names return 0 without a store (10DE40 has cleared both).
//
// D3750 (EAX unit): XMM0 bits. 0 unless body +334 > 0; with body +338 == 0
// (ordered, DAZ), +334 / definition +1D4 when +1D4 > +334, else 1; otherwise
// (+334 - +338) / definition +1DC when +334 > +338, else 0.
std::uint32_t unit_function_ratio(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t mxcsr);
// 1CF0E0 (ESI animation channel): XMM0 bits. 0 without an animation (+6
// NONE) or with a frame count (1E3150 +14) <= 0; else channel +1C / frames
// clamped to [0, 1]. A channel whose 1E3150 lookup is 0 faults the original
// and is refused.
std::uint32_t animation_channel_fraction(const SourcePhysicsContactMemory&,std::uint32_t channel,std::uint32_t mxcsr);
// D3950 (EAX unit): XMM0 bits. 1 for NONE; for the unit's animation
// manager (body + word +12A) in state 0x700005C its 1CF0E0 fraction, in
// state 0x700005D 1 - fraction (0 without an animation), else 1.
std::uint32_t unit_animation_fraction(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t mxcsr);
// 107140 (EAX weapon, stack: word trigger index): XMM0 bits. 0 beyond the
// definition's +2C8 triggers or for a trigger state (body +20C + 12 i) other
// than 1 or 2; 1.0 for state 2; for state 1, 1 - word +2 / row +1C (rows of
// 0x40 at definition +2CC) * game time +4.
std::uint32_t weapon_trigger_fraction(const SourcePhysicsContactMemory&,std::uint32_t weapon,std::uint32_t index,
    std::uint32_t mxcsr);
// C9070 (stack: unit, name, value, active): the unit level's callback.
// Answers 15 unit names (body fields, D3750, D3950) and forwards 0xB0005A9
// and 0x130005A8 (as 0xC000569) to the unit's current weapon (body +212
// slot of +218) through 1046C0.
std::uint8_t unit_function_value(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);
// E0900 (stack: biped, name, value, active): the biped level's callback
// (0x6000542 body +3DC state 2, 0x70000C9 body +39D, 0xC0005A4 the x87
// speed |+88..+90| / definition +2DC).
std::uint8_t biped_function_value(SourcePhysicsContactMemory&,std::uint32_t biped,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);
// 1046C0 (stack: weapon, name, value, active): the weapon level's callback
// (heat, age, triggers, magazines, barrels; FC510, 107140, 165590 and
// 1655F0's 24B430 under mode 9).
std::uint8_t weapon_function_value(SourcePhysicsContactMemory&,std::uint32_t weapon,std::uint32_t name,OriginalOutput& value,
    OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);

// 24B430 (EAX weapon): XMM0 bits of 1655F0's mode-9 value (1046C0's bomb
// names): 0 without a +17E player, 1 with the player's [0x52D710] +170 bit 3,
// else game time +4 / [0x4F3218] +244 (1 for 0) * the player's +1A0 word
// (0 when not positive).
std::uint32_t weapon_mode9_value(const SourcePhysicsContactMemory&,std::uint32_t weapon,std::uint32_t mxcsr);

// F9270 (stack: vehicle, name, value, active): the vehicle level's callback
// (throttle, speed, turn and seat ratios over the definition's limits; the
// x87 steps under the live control word, F9E10's guarded DIVSS), with F9E40's
// limits (kinds 0 and 2), FA5D0's +348 bit 7 (the engine names) and 11FF40's
// velocity without its forward component (0x10000594).
std::uint8_t vehicle_function_value(SourcePhysicsContactMemory&,std::uint32_t vehicle,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);

// 100E70 (stack: projectile, name, value, active): the projectile level's
// callback (time_remaining +158, tracer +12C bit 1, projectile_attach bit 3,
// range_remaining +168 / definition +E0, acceleration_range).
std::uint8_t projectile_function_value(SourcePhysicsContactMemory&,std::uint32_t projectile,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint32_t mxcsr);
// 10BE20 (stack: device, name, value, active): the device level's callback
// (power +134, position +140, delay, loop, locked with the machine's +13C
// device group, change_in_position and change_in_power: x87 |x| / limit).
std::uint8_t device_function_value(SourcePhysicsContactMemory&,std::uint32_t device,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint16_t x87_control,std::uint32_t mxcsr);
// 1DB800 (ECX name, ESI component row, EDX value, stack: active): a physics
// component's phantom_on (+4 bit 13) and its +1B / +1A bytes over 255.
std::uint8_t physics_component_function_value(SourcePhysicsContactMemory&,std::uint32_t component,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint32_t mxcsr);
// 11E490 (stack: object, name, value, active): the machine and crate
// levels' callback: 1DB800 on the object's +B4 physics component (0 without).
std::uint8_t machine_function_value(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    OriginalOutput& value,OriginalOutput& active,std::uint32_t mxcsr);

// 13E670 (EAX function reference, stack: x, y): 0 for an empty reference;
// otherwise 13DE70 (native_tu15_evaluate_authored_function_extended) and,
// unless the function's +1 high nibble is set, the result clamped to [0, 1]
// and mapped onto the function's +4..+8 range. Returns the float bits.
std::uint32_t function_reference_value(SourcePhysicsContactMemory&,std::uint32_t reference,std::uint32_t x,
    std::uint32_t y,std::uint16_t x87_control,std::uint32_t mxcsr);

// BC8F0 (EAX definition function row, stack: object, value): one object
// function (definition +68 row of 0x20): the import value (10FB70 override,
// else 10DE40), the periodic or mapped default function (13E670), the
// scale-by function (BCB70), the minimum value and the turn-off function.
// Publishes the clamped value (0 when inactive) and returns the active byte.
std::uint8_t object_function_evaluate(SourcePhysicsContactMemory&,std::uint32_t row,std::uint32_t object,
    OriginalOutput& value,std::uint16_t x87_control,std::uint32_t mxcsr);

// The live MCC cache names object functions in its own StringID namespace
// (src/rendering/effects/source_object_function_symbols_tu15.inc, the same
// authenticated join the translated callbacks use at their compiled
// comparisons). While a scope with `mcc` is active on this thread, BCB70's
// literal names and the 10DE40 type callbacks compare an incoming name in
// TU1.5's namespace: an MCC function name maps to its TU1.5 name, and an MCC
// name equal to a TU1.5 compiled literal it does not denote matches none.
// Outside a scope (the oracle) names are unchanged.
class ObjectFunctionNamespace {
public:
  explicit ObjectFunctionNamespace(bool mcc);
  ~ObjectFunctionNamespace();
  ObjectFunctionNamespace(const ObjectFunctionNamespace&)=delete;
  ObjectFunctionNamespace& operator=(const ObjectFunctionNamespace&)=delete;
private:
  bool previous_;
};
std::uint32_t object_function_tu15_name(std::uint32_t name);

// BCB70 (stack: object, name, value): the object function value: 1 for the
// names 0 and 0x30005A6, 0 for 0x40005A7, else the 10FB70 override (body +C0
// bit 9), the type callbacks (10DE40), the definition's matching functions
// (+64/+68, BC8F0, the last match wins) or, for an object attached with body
// +4 bit 26, its parent's value. Returns the active byte.
std::uint8_t object_function_value(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t name,
    OriginalOutput& value,std::uint16_t x87_control,std::uint32_t mxcsr);

// 17EE10 (EAX contrail row): clears +28; with a profile tag (contrail
// definition +34) advances +1A and, unless the current profile +18 still has
// points left, draws a new profile in [+38, +38 + +3A) from [[0x4F3CD8] + 4].
void contrail_profile_advance(SourcePhysicsContactMemory&,std::uint32_t contrail);

// 17E940 (EBX object, DI attachment index, stack: tag): a contrail
// attachment: NONE for a NONE tag, a contrail definition without +34 or a
// full [0x4F7214] array. Otherwise the new 0x48-byte row (+2, +4 tag, +8
// object, +C index, +10 the attachment's primary scale, +18, 17EE10, +1C, +20,
// four +30/+38 pairs) and its BCB70 scale into +14. An active scale needs
// 17EF70 (contrail reset), which is a refusal after the +2 flag store.
std::uint32_t object_contrail_attachment_create(SourcePhysicsContactMemory&,std::uint32_t tag,std::uint32_t object,
    std::uint32_t index,std::uint16_t x87_control=0x027fu,std::uint32_t mxcsr=0x1f80u);

// 179540 (EDX object, stack: tag, primary scale, secondary scale, word
// marker): an effect attachment. It fills a 0x60-byte effect creation request
// on its stack (flags 7, the tag, object and scales, colors 0xFF808080, scales
// 1.0, the BBE00 position and the 1791C0 location block, kind 2) and returns
// effect_new's handle (178E30). 178E30 (the effect system) is not native yet,
// so this is a refusal; nothing observable precedes it. C0CD0 passes row +4,
// row +10, row +14, row +C and stores the result with kind 2.
std::uint32_t object_effect_attachment_create(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t tag,
    std::uint32_t primary_scale,std::uint32_t secondary_scale,std::uint32_t marker);

// ---- Light attachments (C2260) ----
// 134730 (EAX flags, ECX a, ESI b, EDI out, XMM5 t): out = a * (1 - t) +
// b * t per channel (reads a and b per channel, as the original). Flags bit 0
// (an HSV blend through 1343E0/134510 on stack colors) is a refusal. C0910
// passes the function entry's flags (+4), colors (+8, +14) and the object's
// change color.
void color_interpolate(SourcePhysicsContactMemory&,std::uint32_t flags,std::uint32_t a,std::uint32_t b,
    std::uint32_t out,std::uint32_t t,std::uint32_t mxcsr);
// 134870 (ECX out, stack: a, b; EBX change color, EAX blend flags, XMM7 t):
// `a` and `b` are {weight, r, g, b} values. Stores the 134730 lerp of their
// colors into `out` (an HSV blend, flags bit 0, is a refusal) and, with a
// change color, blends it in by the interpolated weight (when either weight
// exceeds 1e-4) or multiplies it in.
void light_color_blend(SourcePhysicsContactMemory&,std::uint32_t out,const std::array<std::uint32_t,4>& a,
    const std::array<std::uint32_t,4>& b,const std::array<std::uint32_t,3>* change_color,std::uint32_t flags,
    std::uint32_t t,std::uint32_t mxcsr);
// 13E6E0 (ECX function reference, stack: input): a colour function's 32-bit
// ARGB colour (NONE without data): a constant, grey (the input times 255),
// or a blend of two, three or four colour stops (16-bit lanes, the fraction
// rounded by CVTPS2PI under the MXCSR).
std::uint32_t color_function_value(const SourcePhysicsContactMemory&,std::uint32_t reference,std::uint32_t input,
    std::uint16_t x87_control,std::uint32_t mxcsr);
// C2B30 (EAX colors, ECX definition +C0 function block, stack: light,
// intensity, scale, pair): the light's animation phase (game time, or a
// per-light offset period), the scale function (+4/+8, 13DE70) into `scale`,
// the colour function (+C/+10, 13DE70 then 13E6E0) tinting `colors`, and
// the pair functions (+14/+18, 13E670) into `pair`. A zero tick rate
// faults (IDIV) and is refused.
void light_function_scales(SourcePhysicsContactMemory&,std::uint32_t light,std::uint32_t intensity,
    std::uint32_t colors,std::uint32_t functions,std::uint32_t scale,std::uint32_t pair,std::uint16_t x87_control,
    std::uint32_t mxcsr);
// C2CB0 (stack: light): recomputes the light's intensity (+D0: the owner's
// BCB70 function for attached lights, the 17F8A0 fade for timed ones),
// colors (+D4/+E0, 134870 or 134730), scale (+EC), C2B30, the active
// camouflage dimming (+C8), the clamps and +C4; then lights it (+2 bit 1,
// C4D30 when [0x4EABC4] allows) or unlights it (C5250 for bit 3).
void light_update(SourcePhysicsContactMemory&,std::uint32_t light,std::uint16_t x87_control,std::uint32_t mxcsr);
// C2260 (stack: tag, object, index, primary scale, word marker): a light
// attachment. NONE when the definition (+0 bit 0 with +94 NONE) or the game
// mode ([[0x4F3218] + 8] not 4/5 with +A4 == 2) declines, or [0x4ECBDC] is
// full; otherwise the new 0x110-byte row, C2CB0, +C and +10C. C0CD0 passes
// row +4, its object, the row index, row +10 and row +C, and ORs body +4 with
// 0x10 for a non-NONE result.
std::uint32_t object_light_attachment_create(SourcePhysicsContactMemory&,std::uint32_t tag,std::uint32_t object,
    std::uint32_t index,std::uint32_t primary_scale,std::uint32_t marker,std::uint16_t x87_control=0x027fu,
    std::uint32_t mxcsr=0x1f80u);
}
