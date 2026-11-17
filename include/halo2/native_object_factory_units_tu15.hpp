#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <cstdint>
#include <functional>

// Native unit, biped and vehicle type callbacks of TU1.5 object_new (track
// "units"): the slots 0x474B90 dispatches for units (+2C new, +38 place, +3C
// new failed, +74 nodes) and bipeds/vehicles (+28 validate, +2C new, +3C new
// failed). See docs/native_object_factory_tracks/units.md. Each owner
// replicates its original routine over the live world memory: stores keep the
// original address, width, value and order. SSE steps run the original
// operation under the caller's MXCSR (x87 steps under its control word).
// Callees that are not native yet are named refusals (std::runtime_error),
// never a call into original code.
namespace halo2::native_object_factory_tu15 {
using UnitFp=NativeTu15PhysicsFrameFp;

// ---- Unit queries ----
// C9550: the unit's animation mode (manager +7C) is one of the five
// [0x473990] entries.
bool unit_animation_mode_listed(const SourcePhysicsContactMemory&,std::uint32_t unit);
// CFB40: both weapon slots +212 and +213 are set (not -1).
bool unit_dual_wielding(const SourcePhysicsContactMemory&,std::uint32_t unit);
// CE230: the unit's weapon in slot `index` (body +218 + 4 * index), or NONE
// for index -1.
std::uint32_t unit_weapon(const SourcePhysicsContactMemory&,std::uint32_t unit,std::int16_t index);
// 113C70: the current animation (manager +0/+4) and the aim block's overlay
// animation (+9C/+A0, when +A2 is set) leave bit `bit` of their +18 flags
// clear. Returns the original AL.
std::uint8_t unit_animation_allows(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t bit);
// 113B70: a seated unit (parent and seat +1FC set) whose mode is not a
// listed driver/passenger mode looks around (113C70 bit 2). Returns AL.
std::uint8_t unit_look_enabled(const SourcePhysicsContactMemory&,std::uint32_t unit);
// 1139D0: the unit's aiming state (aim block +A4) and mode allow aiming
// (113C70 bit 3). Returns AL.
std::uint8_t unit_aim_enabled(const SourcePhysicsContactMemory&,std::uint32_t unit);

// ---- Animation fades (4 bytes: position, length, curve, active bit) ----
// 1E14F0 (ECX fade, BL `active`): retarget the fade to round(ticks per
// second * seconds) - 1 ticks, clamped to 1..126, rescaling the position.
void animation_fade_start(SourcePhysicsContactMemory&,std::uint32_t fade,std::uint8_t active,std::uint32_t seconds_bits,
    const UnitFp&);
// 1E1620 (EAX fade): the fade's weight in [0, 1] through its curve (1
// smoothstep 110FC0, 2 smootherstep 1E1920, 3 ease out, 4 cubic ease out);
// returns the XMM0 bits.
std::uint32_t animation_fade_weight(const SourcePhysicsContactMemory&,std::uint32_t fade,std::uint32_t mxcsr);

// ---- Animation graph lookups ----
// 1E5810: binary search of the tag block at `block` ({count, rows}) for the
// signed `key` at row +0 with `stride`; the row address, or 0.
std::uint32_t tag_block_search(const SourcePhysicsContactMemory&,std::uint32_t block,std::uint32_t key,std::uint32_t stride);
// 1E5360 (EDX graph tag data): the packed id (inheritance index, animation)
// of the animation named `name` in the graph or the first inherited graph
// that has it, NONE when absent. A found animation passes through the
// resource request 1E5C80.
std::uint32_t animation_graph_find(const SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t name);

// 1E5250 (`weapon_channel`) / 1E51D0 (ESI cursor, EBX graph): advance the IK
// point cursor at `cursor` (+0 marker, +4 target marker, +8 word index, +A
// word stage, +C mode, +10 weapon class) to the graph's next point; stores
// the cursor like the original. Production keeps the cursor on the native
// stack instead (unit_ik_points).
bool animation_ik_point_next(SourcePhysicsContactMemory&,std::uint32_t cursor,std::uint32_t graph,bool weapon_channel);

// ---- Unit IK (116060 and its two channels) ----
// 116060: walk the graph's IK points for the unit's modes (1E5250 with
// `weapon_channel`, else 1E51D0); a point with both markers set needs the
// marker IK solve (BAD60, 1E1390), which is a refusal.
void unit_ik_points(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t target,bool weapon_channel,
    std::uint32_t weight_bits,std::uint32_t manager,std::uint32_t mxcsr);
// 116340 (EBX unit): the look fade (+84) IK against the parent.
void unit_look_ik(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t parent,std::uint32_t mxcsr);
// 116280 (EBX unit): the aim fade (+80) IK against the weapon.
void unit_aim_ik(const SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t weapon,std::uint32_t mxcsr);

// ---- Unit new leaves ----
// 1708F0 (ECX model tag, EDI name): index of the model's +7C row (0x5C) named
// `name`, or NONE (also for a NONE model).
std::uint32_t model_node_index(const SourcePhysicsContactMemory&,std::uint32_t model_tag,std::uint32_t name);
// 111720 (EAX unit): reset the aim block's animation state rows (+94..+120).
void unit_animation_state_clear(SourcePhysicsContactMemory&,std::uint32_t unit);
// 111000 (EAX unit): zero aim block +5C..+7B, +6C = -1, +5C = graph animation
// 0x800004D (1E5360).
void unit_overlay_animations_find(SourcePhysicsContactMemory&,std::uint32_t unit);
// 1102A0 (EAX unit): zero aim block +30..+5B; +34/+30 = graph animations
// 0x8000145/0xC00024D; +40 = 0.0, +38 = NONE.
void unit_aim_animations_find(SourcePhysicsContactMemory&,std::uint32_t unit);
// 115F50 (EAX unit): the five aim block fades (+80..+93) stopped, curves set.
void unit_fades_reset(SourcePhysicsContactMemory&,std::uint32_t unit);
// 111940 (stack unit): the aim block's animation state, model marker rows
// (+7C/+7E), manager +74/+78 and the three steps above.
void unit_animation_reset(SourcePhysicsContactMemory&,std::uint32_t unit);
// 1175A0 (EAX unit, stack variant name): the +342 record from the model
// (+90, +A0) and its variant rows (+50/+54: dialogue tag +34, override +2C).
void unit_variant_record(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t variant);
// 115E90 (EAX state, ECX previous mode, DL forced, stack urgent byte and
// minimum seconds): the animation blend time, 0.267 s (0.1335 s for three
// states), at least `minimum`. Returns the XMM0 bits.
std::uint32_t animation_blend_seconds(std::uint32_t state,std::uint32_t previous,std::uint8_t forced,std::uint8_t urgent,
    std::uint32_t minimum_bits,std::uint32_t mxcsr);
// 1E16E0 (ECX fade): the fade's remaining (active) or elapsed seconds;
// XMM0 bits.
std::uint32_t animation_fade_remaining(const SourcePhysicsContactMemory&,std::uint32_t fade,std::uint32_t mxcsr);
// 1D38A0 (EAX node count, stack manager, seconds, destination, source):
// start a node interpolation of `seconds` unless the manager's +64 fade has
// longer to run: copies count * 32 bytes and restarts the fade (1E14F0).
bool animation_interpolation_start(SourcePhysicsContactMemory&,std::uint32_t count,std::uint32_t manager,
    std::uint32_t seconds_bits,std::uint32_t destination,std::uint32_t source,const UnitFp&);
// BC380 (ESI object, stack seconds): 1D38A0 over the object's node
// orientation blocks (+112 to +10E, +110 / 32 nodes), then B9390.
void object_interpolation_start(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t seconds_bits,const UnitFp&);
// 1D34E0 (ESI channel): an unset channel returns 0; a set one is a refusal.
std::uint8_t animation_overlay_restart(const SourcePhysicsContactMemory&,std::uint32_t channel);
// 115E20 (EAX unit): 1D34E0 on the aim block channels +9C, +DC, +BC.
void unit_overlays_restart(const SourcePhysicsContactMemory&,std::uint32_t unit);
// 116A10 (EAX unit): the aim overlay 0x10000551 (1E3F30) started on aim
// block channel +DC (1D36C0, flags 0x803F) clears aim byte bit 7.
void unit_aim_overlay_start(SourcePhysicsContactMemory&,std::uint32_t unit,const UnitFp&);
// 1D37D0 (ESI manager, ECX/EDX keys, EDI name; record, flags): starts the
// overlay `name` (1E3F30 on the manager's graph under the keys, NONE or
// 0x7000101 standing for +74/+78) on `record` (1D36C0, key `name`, bytes
// 0xFF, 0xFF, 1); false without a graph or an overlay.
bool manager_start_named_overlay(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t key1,std::uint32_t key2,
    std::uint32_t name,std::uint32_t record,std::uint32_t flags,const UnitFp&);
// 1D3C80 (EAX manager, EDX key, stack key, EDI name): the overlay handle of
// `name` (1E3F30 on the manager's graph) that a trial start (1D37D0 ->
// 1D36C0) on a private 0x20-byte channel record accepts, else NONE. A key
// NONE or 0x7000101 stands for the manager's +74 (`key1`) / +78 (`key2`).
// Only the record is private; every other access is the world's.
std::uint32_t manager_overlay_trial(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t key1,
    std::uint32_t key2,std::uint32_t name,const UnitFp&);
// 1D4450 (EAX manager, EDX aim channel): the aim overlay handle of channel
// 0..2 (1E3F30 of 0xE000044, 0xE000045, 0xB000046 under the manager's
// +74/+78 keys); NONE for another channel.
std::uint32_t manager_aim_handle(const SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t channel);
// 115AF0 (stack unit): the aim block's mode handles (+FC, +100, +104..+114
// through 1D3D80: an overlay a trial channel start accepts, else 1D46A0's
// state animation), 116A10, the three aim overlays (+118..+120, 1D4450) and
// aim byte bit 1.
void unit_animation_channels_refresh(SourcePhysicsContactMemory&,std::uint32_t unit,const UnitFp&);
// 111B10 (EAX flags, stack unit and six words: mode, two keys, state, minimum
// blend seconds, urgent byte): flags 0x20/0x40 (same or exchangeable state,
// 0x474BE4 pairs) and 0x200 (+13C with [51DAF8] clear); 1D5320 (track
// weapon) unless mode and state are both 0x7000101; then 115E20 and 1D4350
// (aim byte bit 1) for a new mode or an urgent change, 115E90's blend into
// BC380, and 115AF0. Returns 1D5320's result (true when skipped).
bool unit_animation_state_set(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t flags,
    const std::array<std::uint32_t,6>& state,const UnitFp&);
// An override of the 111B10 call in C6440/F10E0 (unit, flags, state words);
// empty means the native unit_animation_state_set.
using UnitAnimationStateService=std::function<bool(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t flags,
    const std::array<std::uint32_t,6>& state)>;

// ---- Biped leaves ----
// 1F86A0/1F4850 via 1ED780 (EDI state at body +3DC, stack mode): set the
// biped physics mode; 1 ground (1F86A0 defaults), 2 (forward vector only), 3
// flying (1F4850). Leaving or entering modes 3..5 rebuilds the physics shape
// through track havok's 1CC310; that owner retains its removal/refresh
// refusals. Leaving 4/5 tears down (204610), which is still refused.
void biped_physics_mode_set(SourcePhysicsContactMemory&,std::uint32_t state,std::uint8_t mode,const UnitFp&);
// E7ED0 (EAX biped): 3 when body +10A bit 2, else 2 when definition +264 bit
// 4, else 1.
std::uint32_t biped_physics_mode(const SourcePhysicsContactMemory&,std::uint32_t biped);
// E7E80 (EAX biped): enter that mode (E7CA0 for 2; E7CE0 for 3 needs E65A0,
// a refusal).
void biped_physics_initialize(SourcePhysicsContactMemory&,std::uint32_t biped,const UnitFp&);
// E0A10 (stack biped): biped body state, +368 = world position (core BBE00),
// then E7E80.
void biped_initialize(SourcePhysicsContactMemory&,std::uint32_t biped,const UnitFp&);

// ---- Vehicle leaves ----
// BBF20 (EAX object, ECX out): the root's Havok rigid body origin, else the
// object's world position (BBE00). Returns `out`.
std::uint32_t object_root_origin(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t out,std::uint32_t mxcsr);
// F0F10 (stack vehicle): vehicle body state; +3D0 = BBF20.
void vehicle_state_initialize(SourcePhysicsContactMemory&,std::uint32_t vehicle,std::uint32_t mxcsr);
// 1CE7B0 (ESI channel): +1C = last frame (frames - 1 + 1e-4, at least 0),
// +11 |= 0xA, +10 = 0.
void animation_channel_restart(SourcePhysicsContactMemory&,std::uint32_t channel,std::uint32_t mxcsr);
// 1D6200 (EDI manager): manager defaults; restarts its two channels.
void vehicle_animation_settle(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t mxcsr);

// ---- Unit place leaves ----
// CF400 (EAX unit): a set weapon slot needs the release D0950: a refusal.
void unit_drop_weapons(SourcePhysicsContactMemory&,std::uint32_t unit);
// CEF90 (EDI unit): each definition +1C0/+1C4 weapon row with a tag needs
// B9960 and object_new (B9B70, the coordinator): a refusal.
void unit_create_starting_weapons(SourcePhysicsContactMemory&,std::uint32_t unit);

// ---- Type callbacks (0x474B90 slots; coordinator shapes plus FP) ----
// C6440 (unit +2C new; stack unit, request, failed byte): unit body state and
// blocks +33C/+340/+344 (BE3B0; `scratch_slot` as in object_block_allocate),
// 111940, 1175A0, then 111B10 and the model marker rows. A block
// failure stores 1 at `failed` and returns false; no animation manager (+12A
// -1) returns false.
bool unit_new(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t request,std::uint32_t failed,
    std::uint32_t scratch_slot,const UnitFp&,const UnitAnimationStateService& animation_state={});
// C68B0 (unit +3C new failed): weapon slot bytes cleared; CF400; a held
// object (+238) needs BA570 (a refusal).
void unit_new_failed(SourcePhysicsContactMemory&,std::uint32_t unit);
// C6850 (unit +38 place): CEF90 starting weapons (B9B70 is a refusal), then
// E8E50 with action 8 (the unitactions track's unit_actions::request: B9390
// wake, EAF00, player history) and CECF0 (unit_actions::seat_drivers_update).
void unit_place(SourcePhysicsContactMemory&,std::uint32_t unit,const UnitFp&);
// DE9E0 (biped +28 validate): definition +264 bit 0 without bit 4 raises the
// request position (+1C) along its up vector (+34) by +270.
void biped_validate(SourcePhysicsContactMemory&,std::uint32_t request,std::uint32_t mxcsr);
// DEA60 (biped +2C new): E0A10; returns true.
bool biped_new(SourcePhysicsContactMemory&,std::uint32_t biped,std::uint32_t request,std::uint32_t failed,const UnitFp&);
// DEA70 (biped +3C new failed): body +34B == 1 decrements [[0x4F3218]+1130].
void biped_new_failed(SourcePhysicsContactMemory&,std::uint32_t biped);
// F10E0 (vehicle +2C new): fails (false) when the model's +24 tag is NONE;
// vehicle state, F0F10, deferred BBBC0 for definition +2AC bit 0, then
// 111B10 and 1D6200.
bool vehicle_new(SourcePhysicsContactMemory&,std::uint32_t vehicle,std::uint32_t request,std::uint32_t failed,
    const UnitFp&,const UnitAnimationStateService& animation_state={});
// 1157D0 (unit +74 nodes, from C0200 through 10E1F0 with the node count and
// matrices of BAC30): update the look and aim fades and their IK; the count
// and matrices reach only the (refused) IK solve.
void unit_nodes_changed(SourcePhysicsContactMemory&,std::uint32_t unit,std::uint32_t count,std::uint32_t matrices,
    const UnitFp&);
}
