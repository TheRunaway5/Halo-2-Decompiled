#pragma once
#include "halo2/native_object_factory_weapon_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>

// Native device animation owners (track "deviceanim"): the HS
// device_set_overlay_track (10D5E0) and device_set_position_track (10CF80)
// owners, the device channel starters 10D870 (overlay, body +1AC) and 10D790
// (position, body +18C) and their animation lookups. Shared animation-graph
// routines come from track "weapon" (weapon_animation). See
// docs/native_object_factory_tracks/deviceanim.md.
namespace halo2::native_object_factory_tu15 {
namespace device_animation {
using M=SourcePhysicsContactMemory;
using Fp=NativeObjectFactoryFp;
using weapon_animation::FrameWord;

// 1E5360 (track units' animation_graph_find) into a frame word: the
// animation named `name` (row +0) in the graph, then in each child graph in
// order (a NONE child ends the search); NONE when absent. The handle's low
// word is the child index (0xFFFF: the graph itself).
void find_named_animation(const M&,std::uint32_t graph,FrameWord& out,std::uint32_t name);
// 1E3F30: 1E3420's search over the mode leaves' +C overlay tables.
void overlay_lookup(const M&,std::uint32_t graph,std::uint32_t key3,std::uint32_t r0,std::uint32_t r1,std::uint32_t r2,
    FrameWord* out0,FrameWord* out1,FrameWord* out2,FrameWord& result);
// 1D3770: starts `record` on the manager's graph with the animation named
// `name` (1E5360, then 1D36C0 with `flags`); false when no graph or no match.
bool start_named_animation(M&,std::uint32_t manager,std::uint32_t record,std::uint32_t flags,std::uint32_t name,const Fp&);
// 10D870 / 10D790: start a device channel (+1AC overlay, +18C position)
// from the animation named `name`, else from the manager's current mode
// (1E3F30 overlay table, 1E3420 is not used here); marks the object.
bool device_start_overlay_channel(M&,std::uint32_t object,std::uint32_t name,const Fp&);
bool device_start_position_channel(M&,std::uint32_t object,std::uint32_t name,const Fp&);
// 10D5E0 (device_set_overlay_track): a device (type mask 0x380) with an
// animation manager clears +16C..+184, starts the overlay `name` (falling
// back to 0x5000081), marks the object and sets or clears +12C bit 4.
bool device_set_overlay_track(M&,std::uint32_t device,std::uint32_t name,const Fp&);

// ---- Matrices (0x34 bytes: +0 scale, +4 forward, +10 left, +1C up, +28
// position) and orientations (0x20 bytes: quaternion, +10 translation, +1C
// scale). Addresses may name original stack locals: callers pass a frame
// overlay memory. ----
// 30BD0: normalize the 3-vector in place (x87 length); returns the length
// bits, or 0 (no stores) when the length is within 1e-4 of zero.
std::uint32_t normalize3(M&,std::uint32_t vector,const Fp&);
// 144BF0: matrix of unit scale from position, forward and up (left = up x forward).
void basis_matrix(M&,std::uint32_t out,std::uint32_t position,std::uint32_t forward,std::uint32_t up,const Fp&);
// 144090: inverse of a scaled rigid matrix (zero for scale 0).
void matrix_inverse(M&,std::uint32_t out,std::uint32_t in,const Fp&);
// 144A60: quaternion (w >= 0) of a 3x3 rotation.
void rotation_quaternion(M&,std::uint32_t out,std::uint32_t rotation,const Fp&);
// 144E60: orientation of a matrix.
void matrix_orientation(M&,std::uint32_t out,std::uint32_t matrix,const Fp&);
// 1201E0: quaternion product a * b.
void quaternion_product(M&,std::uint32_t out,std::uint32_t a,std::uint32_t b,const Fp&);
// 120570: orientation composition a * b (out may alias either).
void orientation_compose(M&,std::uint32_t out,std::uint32_t a,std::uint32_t b,const Fp&);

// ---- Root sampling ----
// 1D4090: index of the node named `name` in the manager's graph (+C/+10
// rows of 0x20), or NONE.
std::uint32_t graph_node_index(const M&,std::uint32_t manager,std::uint32_t name);
// 1D4100: the codec entry (nodebuild 1E32A0) of a child-graph handle; 0 for
// a NONE animation or a handle of the graph itself.
std::uint32_t codec_entry(const M&,std::uint32_t manager,std::uint32_t handle);
// 1E5BA0: the animation's base pose (graph +B8 rows: packed quaternion
// normalized with RSQRTPS, translation, scale) into `out`; without one, the
// identity orientation ([0x474D38]) and false.
bool static_pose(M&,std::uint32_t graph,std::uint32_t handle,std::uint32_t out,const Fp&);
// 283A90 (animation codec, track nodebuild): sample `handle` at `frame_bits`
// into the orientation `out`. Not native yet: an absent service refuses.
using CodecService=std::function<void(M&,std::uint32_t graph,std::uint32_t handle,std::uint32_t frame_bits,
    std::uint32_t entry,std::uint32_t out)>;
// 1D4170: the root orientation of `handle` at `seconds_bits` (base pose at
// frame 0, else the codec).
void sample_orientation(M&,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds_bits,std::uint32_t out,
    const Fp&,const CodecService&);
// 1D4200: the root matrix of `handle` at `seconds_bits` into `out`.
void sample_root(M&,std::uint32_t out,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds_bits,
    const Fp&,const CodecService&);
// 1D4280: root velocity of `handle` at `seconds_bits` (at least one tick):
// the root translation change over the last tick times `rate_bits` * 30.
void sample_root_velocity(M&,std::uint32_t out,std::uint32_t manager,std::uint32_t handle,std::uint32_t seconds_bits,
    std::uint32_t rate_bits,const Fp&,const CodecService&);
// 1CF150: the channel animation's length in seconds (frames / 30); 0
// without an animation.
std::uint32_t channel_seconds(const M&,std::uint32_t record,const Fp&);
// C1600: the object has node orientation blocks (body +112 is not -1).
bool object_has_node_orientations(const M&,std::uint32_t object);

// ---- device_set_position_track (10CF80) ----
struct PositionTrackServices {
  std::function<void(M&,std::uint32_t object)> rebuild_nodes;  // BF050 (track nodebuild)
  // B95D0 (track placement) with position/forward/up addresses, no location.
  std::function<void(M&,std::uint32_t object,std::uint32_t position,std::uint32_t forward,std::uint32_t up)> set_position;
  CodecService codec;  // 283A90
};
// 10CF80: a device with a model and an animation manager starts the
// position channel with `name` (falling back to 0x8000080). A device already
// on a position track (+12C bit 3) first measures its root: an unparented
// device whose root moved is re-placed (B95D0) and its node orientations
// re-based; the track speed (+150) follows the new animation's root travel.
// Interpolates (BC380) for positive `seconds_bits`, rebuilds the nodes
// (BF050) and sets +12C bit 2 and bit 3 (the channel started).
bool device_set_position_track(M&,std::uint32_t device,std::uint32_t name,std::uint32_t seconds_bits,const Fp&,
    const PositionTrackServices&);
// The script host's entry: 10CF80 with BF050 (track nodebuild
// object_rebuild_nodes) and B95D0 (track placement object_set_position,
// rebuilding through BF050) bound; the codec stays a refusal until track
// nodebuild ports 283A90.
bool device_set_position_track(M&,std::uint32_t device,std::uint32_t name,std::uint32_t seconds_bits,const Fp&);
}
}
