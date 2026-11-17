#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>
#include <map>

// Native object type callbacks of object_new (track "objtypes"): the "new"
// (type slot +2C) and "new failed" (+3C) callbacks of the non-unit,
// non-weapon types in the 0x474B90 table, and the owners they run. See
// docs/NATIVE_OBJECT_FACTORY.md and docs/native_object_factory_tracks/objtypes.md.
//
// Every "new" callback has the original's three stack arguments: the object
// datum, the placement request (B9960 layout) and the byte the original sets
// when a block allocation fails. Object handles select their row by the low
// 16 bits only, like the original. Unported callees are named refusals
// (std::runtime_error), never a call into original code.
namespace halo2::native_object_factory_tu15 {
using ObjectTypeFp=NativeTu15PhysicsFrameFp;
// Live controls and optional cross-track owners for this track's callbacks.
// An empty hook is the named refusal of its original routine.
struct ObjectTypeServices {
  ObjectTypeFp fp;
  // BF050(object): node matrix rebuild, the last step of 10F710.
  std::function<void(SourcePhysicsContactMemory&,std::uint32_t object)> node_rebuild;
  // 1CE340(EAX object): Havok AABB phantom wake around a released scenery
  // (10F300 with a scenario slot and a live Havok world).
  std::function<void(SourcePhysicsContactMemory&,std::uint32_t object)> phantom_wake;
};

// ---- "new" callbacks (slot +2C) ----
// 10B5C0 device new: device group handles (+130/+13C), position/power
// interpolation records (+18C..+1C8) and the +14C..+184 state floats.
bool device_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed);
// 117870 machine new: body +4 bit 0x800, then bits 0x1000/0x2000 from the
// definition's +11E bit 2; +1DA = -1, +1DC = 0.
bool machine_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed);
// 20FE50 sound scenery new: body +4 bit 0x10000.
bool sound_scenery_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed);
// 11E320 crate new: +12C = 0, then bit 2 when the request tag's definition
// has +BC bit 0 or its physics model (model +24) has no rigid body whose
// motion word (+1E of the 0x90-byte rows) is other than 1 or 2. Succeeds when
// 212C70 finds the object's physics model and that model has rigid bodies.
bool crate_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed);
// 10F260 scenery new: lightmap object index (10F8D0), +138 = -1, +132 = 3,
// deferred wake (BBBC0 with DL 1), animation start (10F4A0), +12C = 0, and
// the scenario scenery slot's [0x52BE70] entry. An animated scenery whose
// graph has animation 0x400000C starts it through 10F710, which ends in
// BF050 (the `node_rebuild` service, else a refusal).
bool scenery_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed,
    const ObjectTypeServices&);
// 11E270 garbage new: delete schedule (native BD980, enabled, 0 ticks), +4
// bit 0x20000, a primary random draw, +16C = ticks + (draw/65535 + 1) * 10 *
// tick rate (x87 under `fp`), +4 bit 0x10000.
bool garbage_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed,
    const ObjectTypeFp& fp);
// FD200 projectile new: arming/fuse defaults, the request owner's root (+140),
// damage scale +16C (1028B0), random or fixed +18C and rate reciprocals
// (+15C, +164; x87 draws and SSE under `fp`), the 'cont' material row +14C,
// initial velocity along the forward vector (+88..+90, +190), the cluster
// water plane bit (+4 bit 3), spin (100D40: x87 sqrt/sin/cos), detonation
// timer +170, +198 copy of the request's +1C vector and, for a biped root
// owner with a matching +3E8 weapon, its +C0 bits and +B8 team.
bool projectile_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed,
    const ObjectTypeFp& fp);
// 11ABF0 creature new: refused whole. Every successful birth runs its 1ED780
// physics-mode switch (jump tables, 1F86A0/1F4850, Havok 1CC310), then
// 1D5320 and BC380 (scale interpolation). Never executed in the six
// campaign censuses.
bool creature_new(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,std::uint32_t failed);

// ---- "new failed" callbacks (slot +3C) ----
// 10B830 (tail of 10B730) device: releases +130/+13C device groups flagged 4
// (native 16E8A0 on [0x4ECBF8]), sets both to -1 and zeroes +14C..+184.
void device_new_failed(SourcePhysicsContactMemory&,std::uint32_t object);
// 117900 machine: for each of the +1DC (signed) pathfinding entries from
// +1DA, sets its structure bit in [0x505E7C] and zeroes its [0x4F9B58]+28 word.
void machine_new_failed(SourcePhysicsContactMemory&,std::uint32_t object);
// 10F2F0 (tail of 10F300) scenery: clears byte [0x46E468] (2F30F0); unless
// +12C bit 2 is set, clears the scenario scenery slot's [0x52BE70] entry and
// sets +12C bit 2. With a Havok world ([0x52BE74]) the slot release runs
// 1CE340 (Havok phantom wake; the `phantom_wake` service, else a refusal).
void scenery_new_failed(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectTypeServices&);

// ---- address-keyed callbacks (ObjectFactoryServices construct / construct_failed) ----
// "new" (+2C) callbacks of this track keyed by their 0x474B90 slot address:
// 10B5C0 device, 117870 machine, 20FE50 sound scenery, 11E320 crate, 10F260
// scenery, 11E270 garbage, FD200 projectile, 11ABF0 creature (refusal).
// Garbage and projectile run x87/SSE steps under `fp`, the world's live x87
// control word and MXCSR at the birth.
using ObjectTypeConstruct=std::function<bool(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t request,
    std::uint32_t failed)>;
using ObjectTypeConstructFailed=std::function<void(SourcePhysicsContactMemory&,std::uint32_t object)>;
std::map<std::uint32_t,ObjectTypeConstruct> objtypes_construct_callbacks(const ObjectTypeServices& services);
std::map<std::uint32_t,ObjectTypeConstruct> objtypes_construct_callbacks(const ObjectTypeFp& fp);
// "new failed" (+3C) callbacks: 10B830 device, 117900 machine, 10F2F0 scenery.
std::map<std::uint32_t,ObjectTypeConstructFailed> objtypes_construct_failed_callbacks(const ObjectTypeServices& services);
std::map<std::uint32_t,ObjectTypeConstructFailed> objtypes_construct_failed_callbacks();

// ---- owners the callbacks run ----
// BBBC0: wake an object. With physics (type mask 0x1883, +C0 bit 6, +B4)
// and `deferred` clear, activates its component (1D9F10); otherwise with
// `deferred` set only +C1 bit 0 is set. Not deferred: +C1 bit 0 cleared and
// the object marked for update (native B9390). Item and projectile types
// mark their network entry (+D4) with 0x400 (B7E50).
void object_wake(SourcePhysicsContactMemory&,std::uint32_t object,bool deferred,const ObjectTypeFp&);
// 1D9F10: with component +4 bit 5, activates every Havok body of the
// component (+70 rows of 0x60, body +40; count +74) through 315CB0: the
// havok track's havok_component_activate.
void physics_component_activate(SourcePhysicsContactMemory&,std::uint32_t component,const ObjectTypeFp&);
// B7E50: in network mode 4 ([0x4DC04C]+8), ORs `flags` into the entry's
// update mask when its row flag (+44 bit 2) is set.
void network_entry_mark(SourcePhysicsContactMemory&,std::uint32_t entry,std::uint32_t flags);
// 10F8D0: body +134 = lightmap scenery object row (10F960) of the object's
// identifier (+A4) and its render model's +8, or -1 (no model, definition
// +C0 word 2, or no render model).
void scenery_lightmap_object_index(SourcePhysicsContactMemory&,std::uint32_t object);
// 10F960: row of the 12-byte scenery object info block of [0x4ECC14]'s
// first lightmap group (+84, +58 count, +5C rows) matching the 8-byte object
// identifier at `identifier` (unique id, origin BSP word compared only for
// source 0, type and source bytes) and `checksum` (+8); -1 when none.
std::uint32_t lightmap_scenery_object_row(const SourcePhysicsContactMemory&,std::uint32_t identifier,
    std::uint32_t checksum);
// 10F4A0: clears +12C bit 0; an object whose animation block (+12A) names a
// graph (+68) starts animation 0x400000C (frame 0, no reference, flag_b)
// through 10F710 and sets +12C bit 0 when it starts.
void scenery_start_animation(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectTypeServices&);
// 10F710(EAX graph tag; object, name, start ticks, reference, flag_a,
// flag_b): starts animation `name` on the object's animation manager (body
// + [+12A]). Fails (false, no store) for a NONE object/graph, name 0, no
// manager or a missing animation. A graph other than the manager's needs
// 1D3380 (modelstate): a refusal. Otherwise: 1D4D90, the frame `start` /
// 30 s (1D4E30), +12C bit 0 cleared, B92C0, +138 = reference (a reference
// other than NONE reaches BBA80, a core refusal), BBBC0 deferred and BF050
// (the `node_rebuild` service, else a refusal).
bool object_set_animation(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t graph_tag,std::uint32_t name,
    std::uint32_t start,std::uint32_t reference,bool flag_a,bool flag_b,const ObjectTypeServices&);
// 1E5360(EDX graph data): the packed animation (low word import index, -1 for
// the graph itself; high word animation index) named `name` (+2C count, +30
// rows of 0x6C) in the graph or its imports (+4C/+50 rows of 0x20, tag +4;
// a NONE import ends the search), after 1E5C80 (weapon_animation) for a match.
// Stores it to `out` and returns it; a NONE graph pointer (0) gives -1.
std::uint32_t animation_graph_find_name(SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t name,
    std::uint32_t out);
// 1D3FC0(ECX manager, EAX name, ESI out): 1E5360 on the manager's graph (+68).
std::uint32_t animation_manager_find(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t name,
    std::uint32_t out);
// 1D4D90(ESI manager; packed, flags): the animation state 1D5320
// (weapon_animation::manager_set_state, key 0xE0000C2, flags 4, start 0x3F),
// then with a graph the channel start (1D36C0) and, when it starts, 1D4F00.
bool animation_manager_start(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t packed,std::uint32_t flags,
    const ObjectTypeFp&);
// 1D4F00(EAX manager): resets the overlay channels (+20..+5C).
void animation_manager_clear_overlays(SourcePhysicsContactMemory&,std::uint32_t manager);
// 1D4E30(EAX manager, XMM0 seconds): with a graph and an animation on the
// first channel, its frame = seconds * 30 (1CE810, weapon_animation).
void animation_manager_set_frame(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t seconds,
    const ObjectTypeFp&);
// B92C0(EDI object): an inactive root object (header +2 bit 0 clear, header
// +4 word set, no parent) becomes active; physics types run 1CC3F0 and a
// scheduled deletion (+4 bit 14) counts in [[0x4EABC4]+4].
void object_header_activate(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectTypeFp&);
// 1CC3F0(ECX object): a Havok component (+B4) without flag bit 17 is
// rebuilt through 1CC310(object, 0): the havok track's
// havok_component_state_flags (it still refuses at 1D9510/1D73D0).
void object_physics_rebuild_check(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectTypeFp&);
// BA870(EAX object): a Havok component (+B4) is deactivated through 1D9510
// (physics work: a refusal); then +C0 bit 6 is cleared.
void object_physics_deactivate(SourcePhysicsContactMemory&,std::uint32_t object);
// 1028B0: projectile damage scale bits for definition tag `tag`: 1.0 unless
// definition +BC bit 10; then 1.5 for a sourced projectile (`source` not -1),
// else the [0x4ECC1C] difficulty table row 10 (1F1950) at the campaign
// difficulty ([0x4F3218] +132 when +8 is 1, else 1).
std::uint32_t projectile_damage_scale(const SourcePhysicsContactMemory&,std::uint32_t tag,std::uint32_t source);
// 1F1950: [[0x4ECC1C]+FC] float bits at row `row`, column `difficulty`
// clamped to 0..3 (signed words); 1.0 without the table or its +F8 flag.
std::uint32_t difficulty_scale(const SourcePhysicsContactMemory&,std::uint16_t difficulty,std::uint16_t row);
// 100D40: projectile spin from angular velocity +94..+9C: when its length
// (x87, stored as float) exceeds 0.0001, +12C bit 0, the unit axis
// +174..+17C and sin/cos of length * seconds per tick (+180/+184); otherwise
// +12C bit 0 cleared, +180 = 0, +184 = 1.
void projectile_spin(SourcePhysicsContactMemory&,std::uint32_t object,const ObjectTypeFp& fp);
}
