#pragma once
#include "halo2/native_tu15_array_growth.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>

// Native object factory Havok owners (track "havok"). Each binds the existing
// src/physics native owner with services that are themselves native; an owner
// with no native is a named refusal. See docs/native_object_factory_tracks/havok.md.
namespace halo2::native_object_factory_tu15 {
// Live x87 control word and MXCSR (status unused by these owners).
using HavokFp=NativeTu15PhysicsFrameFp;

// ---- Havok allocator [0x48C938] (vtable 0x45F4CC) ----
// Its +10 (149DA0) and +14 (149E00) array methods, routed natively
// (native_tu15_route_array_*) to the pools' +10 233250 / +14 233420 owners
// (native_tu15_pool_*). Refused: a heap whose method is not the pool's, the
// lazy default heap 149930, and pool releases above 0x2000 bytes (import).
NativeTu15ArrayGrowthServices havok_allocator(SourcePhysicsContactMemory&);
// [0x48C938] +10: allocate `bytes` of `memory_class`; returns the block.
std::uint32_t havok_array_allocate(SourcePhysicsContactMemory&,std::uint32_t bytes,std::uint32_t memory_class,HavokFp&);
// [0x48C938] +14: release `data` (null is a no-op).
void havok_array_release(SourcePhysicsContactMemory&,std::uint32_t data,std::uint32_t bytes,std::uint32_t memory_class,
    HavokFp&);
// 2E3250: grow the {data,count,capacity|0x80000000 flag} array at `header`
// (doubles the count, 1 for none) through the allocator; 2E2E30 copies.
void havok_grow_array(SourcePhysicsContactMemory&,std::uint32_t header,std::uint32_t stride,HavokFp&);

// ---- Body and component activation ----
// 315CB0(ECX body): with a world (body +8), drop the body's deferred
// references (3165D0), then activate its inactive island (316D40: active
// array growth, inactive swap, 322660 listeners). Refused: listener vtables
// other than the default 2E4D60 (world +4 slot, entity +8 slot) and the body
// destructor virtual when a deferred reference drops to zero.
void havok_activate_body(SourcePhysicsContactMemory&,std::uint32_t body,HavokFp&);
// 1D9F10(ESI component row): with component +4 bit 5, 315CB0 for every
// body (+70 rows of 0x60, body at +40, count +74 reread each step).
void havok_component_activate(SourcePhysicsContactMemory&,std::uint32_t component,HavokFp&);
// 1CD6A0(EAX object, EBX linear, stack angular/force): deliver optional
// retained 3-word vectors to the object's component. Without force, 1D9EC0
// requires an active body; vehicles use their signed +18 root slot, other
// types visit every row. Fixed bodies and motion kind 6 are skipped. The
// existing native 1D9080/1D9100 setters own activation and aligned stores.
// force also activates the component when +4 bit 5 is set (1D9F10).
// Refused: unknown motion-class/setter virtuals and activation's owners.
void havok_object_component_velocities(SourcePhysicsContactMemory&,std::uint32_t object,
    std::uint32_t linear,std::uint32_t angular,bool force,HavokFp&);
// ---- AABB phantom (1CE340's pieces) ----
// 31C3B0(ECX info): hkAabbPhantom construction info: 31FCD0/31FCA0 base
// fields (+0,+4,+C,+10 zero, +14 0x80000000, +8 byte 1), then the AABB
// min/max (+20, +30) zeroed by MOVAPS. Returns `info`. +20 must be 16-byte
// aligned (the MOVAPS faults otherwise; refused before any store).
std::uint32_t havok_aabb_phantom_info(SourcePhysicsContactMemory&,std::uint32_t info);
// 3154E0(ECX phantom, info): hkAabbPhantom constructor: reference count 1,
// world/collidable/array fields, vtable 0x41D87C, overlap array +80 (empty,
// +88 0x80000000), AABB +60/+70 from info +20/+30 (MOVAPS), +28 from info
// +0. Returns `phantom`. info +20 and phantom +60 must be 16-byte aligned.
std::uint32_t havok_aabb_phantom_construct(SourcePhysicsContactMemory&,std::uint32_t phantom,std::uint32_t info);
// 1CE340(EAX object): wake the bodies around an object through a temporary
// AABB phantom. The AABB is the object's +30 center +- ([0x48B874] *
// [0x46A11C] + its +3C radius) (SSE, live MXCSR); the phantom (0x90 bytes,
// class 0x2C, from [0x48C938] +10) is built by 3154E0, added to the world
// [0x52BE48] (318550), every overlapping entity (collidable +18 == 1) with
// an owner whose +40 byte is clear is activated (315CB0), the phantom is
// removed (318680) and its last reference released (283450, else the
// 315620 destructor chain back into the pool). Shape for
// ObjectTypeServices::phantom_wake.
void havok_phantom_wake(SourcePhysicsContactMemory&,std::uint32_t object,HavokFp&);

// ---- hk3AxisSweep broadphase (vtable 0x41C6C0) and hkWorld phantoms ----
// 2EC4B0 (thiscall broadphase; handle, AABB, pairs): quantize the AABB
// (+10/+20 offsets, +30 scale, clamp to [0, 65532] + 65536, live MXCSR),
// append the node (+40 bank of 16-byte nodes, grown by 2E3250), insert its
// endpoints on the x/y/z axes (+4C/+58/+64, 2EAFB0, reserve 2E31E0), repair
// the other nodes' positions (2E9CF0), then sweep an arena bitset (2EBA20/
// 2EB850; marker cells +70/+74/+78 seed it) and append every overlap: a
// {handle, other} pair, or the node index on a marker's list. The AABB and
// broadphase must be 16-byte aligned (MOVAPS; refused before any store).
void havok_broadphase_add_object(SourcePhysicsContactMemory&,std::uint32_t broadphase,std::uint32_t handle,
    std::uint32_t aabb,std::uint32_t pairs,HavokFp&);
// 2EC9B0 (thiscall broadphase; handle, pairs): the same sweep for the
// handle's node, appending its pairs (or dropping it from marker lists),
// then the endpoint removal (2E9FD0), position repair (2E9DF0), the move
// of the last node into the hole (with its axis/handle/marker references)
// and the count shrink (2E31E0 reserve when the capacity is short).
void havok_broadphase_remove_object(SourcePhysicsContactMemory&,std::uint32_t broadphase,std::uint32_t handle,
    std::uint32_t pairs,HavokFp&);
// 318550 (thiscall world; phantom) hkWorld::addPhantom: phantom +8 = world,
// arena pairs (3177E0, world +234), the AABB (+18: only hkAabbPhantom
// 315170), broadphase addObject (world +C4, only 2EC4B0), the +130 id,
// the new pairs through 37CBF0/37C960 (dispatcher +138, filter +D0 +8: only
// 2E9A00; handlers: only the phantom pair 31FFC0/320040, whose overlap
// owners are 315300/315100 or 2E4D60, listeners only 2E4D60), the
// reference, the +120 phantom array and the 322280 listeners (only 2E4D60).
// Returns `phantom`. Other owners are refused by name before any store.
std::uint32_t havok_world_add_phantom(SourcePhysicsContactMemory&,std::uint32_t world,std::uint32_t phantom,
    HavokFp&);
// 318680 (thiscall world; phantom) hkWorld::removePhantom: broadphase
// removeObject (only 2EC9B0), the removed pairs through 37CBF0/37C960, the
// 3222F0 listeners, the reference (last one: 283450 or the 315620 chain),
// world +8 cleared and the swap-removal from the +120 array.
void havok_world_remove_phantom(SourcePhysicsContactMemory&,std::uint32_t world,std::uint32_t phantom,HavokFp&);

// 1D8470(component row): release the row's body references (+7C records,
// +70 rows with their 0x2001/0x2002 body properties reset to -1 through
// 31C450/31C5E0, +98, +9C), freeing record arrays through the allocator;
// a reference reaching zero runs the body destructor (only the default
// 2E4D60 is native; others are refused).
void havok_component_reset(SourcePhysicsContactMemory&,std::uint32_t component,HavokFp&);
// ---- hkWorld entities and the component's world membership ----
// `sp` is the original ESP at the owner's entry (the return address slot).
// It names only the stack local whose address the original publishes in
// the world: the fixed-body pair collector at dispatcher +24 (3181E0 entry
// ESP - 0x30, 318020's aligned frame - 0x34). 0 (production) keeps that
// local at a private address; all locals stay private either way.
//
// 3198A0 (thiscall world; result byte*, entity) hkWorld::removeEntity: a
// locked world (+12C == 1) defers it (+F0 list, reference); otherwise the
// broadphase removal (3181E0: 2EC9B0, the removed pairs through 321510 or,
// for a fixed body, the 0x41D9A0 island collector and 37C960), the world's
// (322210) and the entity's (31FDE0) listeners (only 2E4D60), the island
// detach (318360: 320150, empty world island unlisted and destroyed by
// 322100, deferred +114 references through 3174A0), the reference.
// Refused: pending world operations (+24 > 0, 319750), entity destructors,
// contact agents of removed entity pairs, other listeners/owners.
void havok_world_remove_entity(SourcePhysicsContactMemory&,std::uint32_t world,std::uint32_t result,std::uint32_t body,
    HavokFp&,std::uint32_t sp=0);
// 319370 (thiscall world; entity) hkWorld::addEntity: lock +12C, the +20
// id (+130 up, fixed +134 down), the island (316950: fixed island +2C, a
// new 3212C0 island, or the first active one), the broadphase (318020:
// shape bounds 2E8D90/2E8120/306760/307E30 under the motion transform,
// x87 margin, 2EC4B0, new pairs through 321510 or the collector), the
// reference and the 3221A0 listeners. New entity pairs are filtered
// (2E9A00) and created through 320EF0 up to deferral (320DA0) or a
// phantom pair; contact manager creation is refused. Returns `body`.
std::uint32_t havok_world_add_entity(SourcePhysicsContactMemory&,std::uint32_t world,std::uint32_t body,HavokFp&,
    std::uint32_t sp=0);
// 1D9510 (ECX component row): per row the gameplay listener [0x52D880]
// leaves the entity (+70; +60 for a dynamic body of an active component),
// [0x48B868] counts down, 3198A0 and 282C10 (native; its 1CC310/1D9510
// recursion bound here, other owners refused); then the +88 records
// release (bit 0), the +98/+9C phantoms leave the world (318680) and +4
// bit 5 clears. Refused: constraints (+80, 316AA0), the +94 attached
// object wake and 22D0A0 model constraints.
void havok_component_remove(SourcePhysicsContactMemory&,std::uint32_t component,HavokFp&,std::uint32_t sp=0);
// 1D97F0 (ECX component row): the +9C/+98 phantoms join the world
// (318550); per row the listeners join (315E50/315E90, +5C cleared),
// 1CC560 (entity limit 1CDD60 refused, count, 319370, the 315CE0/316C90
// queue, 282C10), the object's +88/+94 velocities (1D9080/1D9100), and a
// new gameplay body record (0x70 bytes, class 0x26: 1D9AD0/31E820 with
// shape bounds) replaces the body's +54 one (1D9C80 when it drops). +4
// bit 3 clears, bit 5 sets. Refused: constraints (+80, 317350) and a
// non-unit model scale (31E430/31E560/31E590).
void havok_component_add(SourcePhysicsContactMemory&,std::uint32_t component,HavokFp&,std::uint32_t sp=0);
// 1D73D0 (EAX component datum): the shape refresh dispatch. The object's
// definition type (body +AA) selects, through the byte map 0x1D74BC and the
// table 0x1D74A4, the unit 1DEE30, vehicle 1DD9A0, machine 1DDBF0 (after
// +4 |= 0x80; its result is +1C), crate 1DD950 or creature 1DEF50 shape
// builder, then 1CD6A0 (+88/+94, no force), +4 |= 1 and bit 17 from the
// header's +2 bit 0. Every builder is still a named refusal (at its entry).
void havok_component_refresh(SourcePhysicsContactMemory&,std::uint32_t datum,HavokFp&);
// 1CC310(object, flags): with a Havok component (+B4) the component row
// is removed (1D9510, when +4 bit 5), reset (1D8470, native), given
// `flags`, refreshed (1D73D0) and reactivated (1D97F0, native). 1D9510 and
// 1D73D0 are refused at their entries (an active row before any store, an
// inactive one after the reset and the flag store): composed tracks stop
// there. Their natives are havok_component_remove/havok_component_refresh.
void havok_component_state_flags(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t flags,HavokFp&,
    std::uint32_t sp=0);
}
