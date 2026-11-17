#pragma once
#include <string>
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/physics_contact_record.hpp"
#include <cstdint>
#include <functional>

// Native object node-matrix rebuild owners (track "nodebuild"): BF050 and the
// routines it executes. See docs/native_object_factory_tracks/nodebuild.md.
// Each owner replicates its TU1.5 routine over the live world memory: stores
// keep the original address, width, value and order. Floating-point steps
// run the original SSE operation under the caller's live MXCSR (and x87
// control word where the original uses x87). Callees that are not native yet
// are named refusals (std::runtime_error), never a call into original code.
namespace halo2::native_object_factory_tu15 {
using NodeRebuildFp=NativeTu15PhysicsFrameFp;

// ---- Node masks (256-bit, eight dwords) ----
// C1F40: every dword of the mask is zero.
bool node_mask_empty(const SourcePhysicsContactMemory&,std::uint32_t mask);
// C1EB0: invert dwords 0..7; dword 7 is then stored again masked to its low
// byte (nodes 224..231 only).
void node_mask_invert(SourcePhysicsContactMemory&,std::uint32_t mask);
// C1F00: zero the dwords past `count` nodes and mask the last partial dword
// (an RMW store even when nothing changes).
void node_mask_limit(SourcePhysicsContactMemory&,std::uint32_t mask,std::uint32_t count);
// 141010: highest set node index below `count`, else -1 (0xFFFFFFFF).
std::uint32_t node_mask_last(const SourcePhysicsContactMemory&,std::uint32_t mask,std::uint32_t count);
// C1E50: set or clear bit `bit` of the dword at `flags` (one RMW store).
void flags_assign_bit(SourcePhysicsContactMemory&,std::uint32_t flags,std::uint32_t bit,bool set);

// ---- Object and animation queries ----
// C1CC0: a live (non-NONE) object whose body byte +B3 is set.
bool object_animation_frozen(const SourcePhysicsContactMemory&,std::uint32_t object);
// B87B0: the manager's graph (+68), animation tag (+0) and animation index
// (+6) are all present.
bool animation_manager_valid(const SourcePhysicsContactMemory&,std::uint32_t manager);
// 1E3150: animation definition of packed id `id` (low word inheritance
// index, high word animation) in graph `graph` (tag data), or 0. A resolved
// animation passes through the resource request 1E5C80, whose admission
// (126200) is not native yet.
std::uint32_t animation_definition(const SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t id);
// B8780: the manager's current animation definition (1E3150), 0 without an
// animation index.
std::uint32_t animation_manager_definition(const SourcePhysicsContactMemory&,std::uint32_t manager);
// E3BC0: biped physics node height offset (bits of XMM0): 1F4920's ramp
// times -0.125 for a biped in physics state 1 with +3DC state 3, else +0.
std::uint32_t biped_physics_height(const SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mxcsr);

// ---- Matrices (real_matrix4x3: scale, forward, left, up, position) ----
// 1707A0: the render model's default node orientations into `orientations`
// (0x20-byte rows: quaternion, translation, scale 1.0).
void model_default_orientations(SourcePhysicsContactMemory&,std::uint32_t model,std::uint32_t orientations);
// 144CF0: orientation (quaternion, translation, scale) to matrix; lazily
// publishes the 1e-4 epsilon at [0x56E680] (flag [0x56E684]).
void orientation_matrix(SourcePhysicsContactMemory&,std::uint32_t out,std::uint32_t orientation,std::uint32_t mxcsr);
// 145560: out = a * b; returns `out`.
std::uint32_t matrix_product(SourcePhysicsContactMemory&,std::uint32_t a,std::uint32_t b,std::uint32_t out,
    std::uint32_t mxcsr);
// BFC70: root node transform: parent (optionally rescaled/mirrored copy)
// * translate(position) * [scale *] basis(forward, +-(up x forward), up).
// `mirror` low byte negates the left vector; `parent` 0 means no parent.
void object_root_matrix(SourcePhysicsContactMemory&,std::uint32_t up,std::uint32_t forward,bool parent_mirrored,
    std::uint32_t position,std::uint32_t scale_bits,std::uint32_t mirror,std::uint32_t parent,std::uint32_t out,
    std::uint32_t mxcsr);

// ---- Animation sampling (BFB90 subtree) ----
// 1E1620: eased fraction of an interpolation state (+0 frame, +1 length,
// +2 curve: smoothstep 110FC0, smootherstep 1E1920, quadratic, cubic).
std::uint32_t animation_interpolation_fraction(const SourcePhysicsContactMemory&,std::uint32_t state,std::uint32_t mxcsr);
// 1E1720: blend `count` current orientations toward `previous` (masked
// nodes only when `mask`); quaternions renormalize in x87.
void animation_crossfade(SourcePhysicsContactMemory&,std::uint32_t state,std::uint32_t current,std::uint32_t count,
    std::uint32_t mask,std::uint32_t previous,NodeRebuildFp&);
// 1D61B0: the manager's second-channel blend weight from +80.
std::uint32_t animation_blend_fraction(const SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t mxcsr);
// 1E32A0: the inherited graph entry (+50 rows of 0x20) carrying codec data
// for `id`, or 0.
std::uint32_t animation_codec_entry(const SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t id);
// BFB90: sample the manager's channels into `orientations` (1D5840 ->
// 1CF210 -> the codec 283A90), function overlays (BF9A0), the type
// orientation hooks (10E100: unit 114750, vehicle F34B0, device 10C0B0) and
// the cross-fade (1E1720).
void object_sample_animation(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mode,std::uint32_t manager,
    std::uint32_t orientations,std::uint32_t count,std::uint32_t mask,NodeRebuildFp&);

// Codec per-track decoders (entries of the 0x48C33C format table), keyed by
// their original address: rotation 296180/296680/296450 (quantized) and
// 296240/2963D0 (float), translation 2961F0/296410, scale 296220/2CEC60,
// sparse-key rotation 296AD0 (byte keys) and 297270 (word keys),
// translation 296C60 and 297400 (word keys) and scale 296DB0.
// They read the codec state block 0x510F30 and publish into its output
// orientation (+70). Other decoders are refused by address.
void animation_codec_track(SourcePhysicsContactMemory&,std::uint32_t decoder,std::uint32_t mxcsr);
// 283570: the frame decoder the state block selects (kind +00, mode +04,
// node mask +2C, second-frame flag +50): 2847D0/284D30, 285A70,
// 2861A0/2868A0 (kind 0), 28ADA0/28B5F0 (kind 2), 28E730/28F110, 290710,
// 2912A0/291E30 (kind 3), and the unmasked second-frame loops 28B080
// (kind 2) and 28EAF0 (kind 3), which blend each track toward the second
// frame (+54..+60) before layering. Each runs the rotation, translation and
// scale flag streams through the track decoders (+34/+38/+3C).
void animation_codec_frames(SourcePhysicsContactMemory&,std::uint32_t mxcsr);
// 2835B0 inputs: `block` is 1E5DF0's record (+0 data, +4 header, +8 node
// count); frame, weight and second frame/weight are float bits.
struct AnimationCodecSetup {
  std::uint32_t block=0,frame=0,weight=0,graph=0,animation=0,count=0,entry=0,mask=0,orientations=0,force=0,flag=0,
      second_frame=0,second_weight=0;
};
// 2835B0: fill the codec state block (and clear 0x56E000..0x56E05F), then
// decode the static and animated blocks through 283570.
void animation_codec_setup(SourcePhysicsContactMemory&,const AnimationCodecSetup&,std::uint32_t mxcsr);
// The animation resource read 1260C0 issues at 0x126139 (1A3B70): resource
// `row` (+0 epoch, +4 bytes, +8 file offset) into `destination` (`bytes`
// rounded to 512) at `priority`. The reader publishes the payload and the
// completion byte (`completion`, the entry's +4) like the original I/O
// completion and returns the request job. Production binds the live
// World's synchronous animation cache read for the duration of a call;
// without a bound reader a cache miss is refused at 1A3B70.
struct AnimationResourceRead {std::uint32_t row,destination,bytes,file_offset,priority,completion;};
using AnimationResourceReader=std::function<std::uint32_t(SourcePhysicsContactMemory&,const AnimationResourceRead&)>;
class AnimationResourceReaderScope {
public:
  explicit AnimationResourceReaderScope(const AnimationResourceReader&);
  ~AnimationResourceReaderScope();
  AnimationResourceReaderScope(const AnimationResourceReaderScope&)=delete;
  AnimationResourceReaderScope& operator=(const AnimationResourceReaderScope&)=delete;
private:
  const AnimationResourceReader* previous_;
};
// 1262F0: address of an animation resource row's cached block (+C cache
// handle), stamping the cache record's age. A miss allocates the block
// (125FB0: 13FE50 through the native page allocator with the 1263C0/126400
// callbacks), claims its entry (16E770) and reads it (1260C0 through the
// bound reader, mode 0 waiting through 1262B0). A pending request (126190
// hit, 16E8A0), a load still in flight (1236A0) and the cache age rollover
// (13FD10) are refused by name.
std::uint32_t animation_resource_load(SourcePhysicsContactMemory&,std::uint32_t row,std::uint32_t name);
// 1E5A70: the codec data of animation `id` in graph `graph` (tag data).
std::uint32_t animation_resource_data(SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t id);
// 283A90: decode animation `id` of `graph` at `frame` with `weight` into
// `count` orientations (1E5DF0 resource record, then 2835B0).
void animation_decode(SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t id,std::uint32_t count,
    std::uint32_t frame,std::uint32_t weight,std::uint32_t entry,std::uint32_t mask,std::uint32_t orientations,bool force,
    NodeRebuildFp&);

// ---- Type hooks ----
// 10E180: each level of the object's type chain (0x474B90) with a matrix
// hook (+70) runs it on `nodes`: unit 1154A0, scenery 10F440.
void object_type_node_hooks(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t nodes,std::uint32_t mxcsr);
// 1154A0: a unit attached through its +33E block (+28 object) takes that
// object's node 0 as the parent of `nodes`.
void unit_node_matrix_hook(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t nodes,std::uint32_t mxcsr);
// 10F440: likewise for scenery through body +138.
void scenery_node_matrix_hook(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t nodes,
    std::uint32_t mxcsr);
// 10E100's orientation hooks (+6C), on (mask, count, orientations) as BFB90
// passes them.
// 10C0B0: a device with a render model and an animation manager samples its
// position (+18C) and overlay (+1AC) channels (1CF210) at full weight.
void device_orientation_hook(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp&);
// F34B0 (F2CC0 with the F2C40 channel callback): the vehicle's animation
// overlays, each started on a private channel (1CE720, track weapon) and
// sampled at a value from the vehicle's state: steering +35C, lateral and
// forward velocity (BC200) and speed +354 ramps, +360/+364, the graph's +40
// rows at body +398 bytes and the three aim overlays at +288..+290. The
// overlay handles are cached per graph at +400..+424 (1D3C80/1D4450 over
// track deviceanim's 1E3F30 and track weapon's 1D36C0).
void vehicle_orientation_hook(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp&);
// 114750: the unit's aim block (+33E) overlays and aiming: its overlay
// channels and handles (+9C..+110), the moving overlay (+208), the aim
// overlays (+118..+120), and the aiming and looking blends (aim records of
// the +FC/+100 and +114 handles at the angles to the +168 and +18C targets
// through the +90/+88 and +8C fades, which restart through 1E14F0).
void unit_orientation_hook(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t orientations,NodeRebuildFp&);
// 1CF210 (EBX channel; weight, mask, count, orientations): one channel's
// animation into the orientations (the 283A90 codec).
void animation_sample_channel(SourcePhysicsContactMemory&,std::uint32_t channel,std::uint32_t weight,std::uint32_t mask,
    std::uint32_t count,std::uint32_t orientations,NodeRebuildFp&);
// 1D5840 (manager, -, weight, mask, orientations, -, -; ESI count): the
// manager's channel 2, the channel 0/1 blend, else channel 0 (1CF210).
void animation_sample_channels(SourcePhysicsContactMemory&,std::uint32_t manager,std::uint32_t weight,std::uint32_t mask,
    std::uint32_t orientations,std::uint32_t count,NodeRebuildFp&);
// BF9A0 (object, -, manager, mask, count, orientations): the graph's
// function overlays at the object's function values.
void object_function_overlays(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t manager,std::uint32_t mask,
    std::uint32_t count,std::uint32_t orientations,NodeRebuildFp&);
// 1CF3B0: a channel's aim animation (aim record 1E31E0) at (yaw, pitch)
// with `weight` (283B50: the yaw column; the pitch rows below and above
// blended through the codec's second frame, CRT floor/ceil in x87).
void animation_aim_channel(SourcePhysicsContactMemory&,std::uint32_t channel,std::uint32_t yaw,std::uint32_t pitch,
    std::uint32_t weight,std::uint32_t mask,std::uint32_t count,std::uint32_t orientations,NodeRebuildFp&);
// 1CF4B0: a channel's animation at `fraction` of its length with `weight`
// (283D70, forced interpolation).
void animation_fraction_channel(SourcePhysicsContactMemory&,std::uint32_t channel,std::uint32_t mask,
    std::uint32_t fraction,std::uint32_t weight,std::uint32_t count,std::uint32_t orientations,NodeRebuildFp&);

// ---- Havok-driven nodes and crate bounds ----
// 1DBBD0 (component row at [0x52BE5C] +44 + datum * 0xA0, mask, height):
// unless component +4 bit 9, every row's (+70 rows of 0x60) non-fixed
// physics-model records (motion type != 6) whose node is not yet in the
// 8-dword `mask` and not below the row's first record's node take their
// node matrix from the rigid body's motion (1D8B90, the component's +30
// transform under +4 bit 6), raised by `height` (float bits), and set
// `mask`; 1DBFD0 then rebuilds the render-model nodes these reach and
// publishes delivered orientations (body +112). A non-root record's child
// shape transform (shape vtable +34) is refused by name.
void object_physics_nodes(SourcePhysicsContactMemory&,std::uint32_t component,std::uint32_t mask,std::uint32_t height,
    NodeRebuildFp&);
// 1DC7A0 (component row, center, radius): a crate's bounding sphere from its
// rows' hk3AxisSweep AABBs (broadphase [[0x52BE48] +C4] +20 2EB0C0 -> 2EA050,
// x87 dequantization under the live control word), padded by the records'
// largest +80 radius; false without any store when the component is not
// active (+4 bits 5 and 11), empty, or its definition +1C has bit 1.
bool crate_physics_sphere(SourcePhysicsContactMemory&,std::uint32_t component,std::uint32_t center,
    std::uint32_t radius,NodeRebuildFp&);

// ---- Owners ----
// BFF20: bounding spheres (+30, +50; an active crate's +30 from 1DC7A0) and
// the cluster sphere (+40); returns
// false only for a connected object whose cluster sphere is unchanged. A
// connected object that moved is disconnected (BA7E0) and reconnected
// (BA630) through the placement track.
bool object_update_bounds(SourcePhysicsContactMemory&,std::uint32_t object,NodeRebuildFp&);
// BF0C0: rebuild every node matrix of the object from its orientations
// (physics-driven nodes first, through 1DBBD0).
void object_update_node_matrices(SourcePhysicsContactMemory&,std::uint32_t object,NodeRebuildFp&);
// BF050: BF0C0 then BFF20 for the object, then for each child (machines
// excluded) recursively.
void object_rebuild_nodes(SourcePhysicsContactMemory&,std::uint32_t object,NodeRebuildFp&);

// ---- BF640 deferred node matrices (render presentation) ----
// BF640 (object, selected section bytes, matrices): a deferred (+4 bit 30)
// object whose selected sections touch its model's +C4 mask resamples its
// animation (BFB90 under the model's +A4 mask); a unit with live aim or
// frame overlays (110240/1114B0) layers them (1112E0, 110C10) over copies
// at 0x4E8BC0 (orientations) and 0x4E57E8 (matrices); masked child nodes
// rebuild from the orientations; units then take hand IK (1159D0).
// Returns the matrices used: `matrices`, or 0x4E57E8 after overlays.
std::uint32_t object_deferred_node_matrices(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t selected,
    std::uint32_t matrices,NodeRebuildFp&);
// 1112E0 / 110C10 (unit; manager, render, sample mask, count, orientations,
// flags): the aim block's frame overlay (+5C, 13 frames at +6E weights) and
// aim overlays (+30/+34 by the +40 blend and +2 state), each sampled on a
// private channel; sampled animations OR their node flag streams (2130C0)
// into the 32-byte `flags` mask.
void unit_frame_overlays(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t manager,
    std::uint32_t sample_mask,std::uint32_t count,std::uint32_t orientations,std::uint32_t flags,NodeRebuildFp&);
void unit_aim_overlays(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t manager,
    std::uint32_t sample_mask,std::uint32_t count,std::uint32_t orientations,std::uint32_t flags,NodeRebuildFp&);
// 2130C0 (block, mask): OR a 1E5DF0 record's animated and static node flag
// streams into the byte mask.
void animation_node_flags(SourcePhysicsContactMemory&,std::uint32_t block,std::uint32_t mask);
// 1159D0 (unit; mask, count, matrices): hand IK toward the unit's +1A4 or
// the first local player's camera at the 0x4000095 marker (115500).
void unit_hand_ik(SourcePhysicsContactMemory&,std::uint32_t object,std::uint32_t mask,std::uint32_t count,
    std::uint32_t matrices,NodeRebuildFp&);
// 115500 (marker world matrix, target, node matrix): turn the node toward
// the target, yaw/pitch clamped to pi/6 and pi/12.
void hand_ik(SourcePhysicsContactMemory&,std::uint32_t marker,std::uint32_t target,std::uint32_t node,NodeRebuildFp&);
// 1447E0 (out; a, b, c): the rotation matrix of three angles (x87 FSIN/FCOS).
void euler_matrix(SourcePhysicsContactMemory&,std::uint32_t out,std::uint32_t a,std::uint32_t b,std::uint32_t c,
    NodeRebuildFp&);
// ---- First-person animation (16A4DD, 16AD45, 169863) ----
// [0x4F6498]: 0x20CC-byte views per local user (+0 flags, +4 unit, +8
// character variant, slots +C and +101C of 0x1010 bytes, +202C fade, +2030..
// +2054 movement and aim values, +2094 hand node, +2098 hand matrix);
// [0x4F6494]: 0x1000-byte orientation banks per (user, slot), the previous
// orientations at +800. See docs/native_object_factory_tracks/nodebuild.md.
//
// The owners 16AD45 and 169863 reach outside the object factory: 17A570
// (the effects that follow the first-person weapon: stdcall user, object,
// attach), 171440 (resource prediction of the weapon definition's +2B8
// block, DI 0) and 128E70 (sound stop, EBX handle). Production binds them
// to the live world for the duration of a call; without a binding each
// refuses by its address.
struct FirstPersonResiduals {
  std::function<void(std::uint32_t user,std::uint32_t object,bool attach)> effects;  // 17A570
  std::function<void(std::uint32_t block)> predict;                                  // 171440
  std::function<void(std::uint32_t sound)> sound_stop;                               // 128E70
  // 18BE50 (EBX sound, EAX position, ECX direction; object, -1, 1.0): the
  // sound of a first-person animation event (169BA2); returns its handle.
  std::function<std::uint32_t(std::uint32_t sound,std::uint32_t object,std::uint32_t position,std::uint32_t direction)>
      sound_start;
  // Not a residual: 169C46's 16A479 boundary, just before the native
  // sampler (16A4DD) runs for (user, slot); for the composition observers
  // of the translated 16A4DD's entry.
  std::function<void(std::uint32_t user,std::uint32_t slot)> sampler_entry;
};
class FirstPersonResidualScope {
public:
  explicit FirstPersonResidualScope(const FirstPersonResiduals&);
  ~FirstPersonResidualScope();
  FirstPersonResidualScope(const FirstPersonResidualScope&)=delete;
  FirstPersonResidualScope& operator=(const FirstPersonResidualScope&)=delete;
private:
  const FirstPersonResiduals* previous_;
};
// 16A4DD (user, slot; RET 8): the slot's first-person pose. Rebuilds the
// slot (16AD45) when its node maps no longer match the first-person and
// character models, resets the orientations to the identity, publishes the
// models' default orientations through the node maps, samples the manager
// (1D5840), the function overlays (BF9A0), the fade-weighted +98/+B8
// channels, the aim (+D8), movement (+DC) and ammunition (+E0) overlays,
// crossfades (1D4560), then builds the node matrices (1D4000/1D4030) and
// slot 0's hand matrix.
void first_person_animation(SourcePhysicsContactMemory&,std::uint32_t user,std::uint32_t slot,NodeRebuildFp&);
// 16AD45 (user, slot; RET 8): rebuilds the slot from the unit's weapon:
// the manager (1D3380), node maps (1E5770), mirrored node pair, state
// (169863 0x400000C) and overlay handles.
void first_person_rebuild(SourcePhysicsContactMemory&,std::uint32_t user,std::uint32_t slot,NodeRebuildFp&);
// 169863 (EAX key; user, slot, stop sound; RET 0xC): enters the slot's
// animation state `key` (1D5320) unless the current state excludes it,
// after the fade (169B81/169B0A) the key selects.
void first_person_state(SourcePhysicsContactMemory&,std::uint32_t key,std::uint32_t user,std::uint32_t slot,
    bool stop_sound,NodeRebuildFp&);
// 169C46 (user; RET 4), the first-person update of a local user (168D55,
// every tick): the aim and movement springs (121890) toward the unit's
// aim velocity and the input angle changes (C1F90), then per slot the
// countdown, visibility (16B1BC), the manager advance with its animation
// events (1D39C0; sound events through 169BA2 -> 18BE50, a residual), the
// state end (16961F), the motion (+98) and overlay (+B8) channels, the
// settle spring, the fades (1D5790), the idle fidget (0x600005F), the
// sampler (16A4DD) and the resource prediction countdown (16B2B9).
void first_person_update(SourcePhysicsContactMemory&,std::uint32_t user,NodeRebuildFp&);
// 1E5470 (graph, out, orientations, root) / 1E5540 (+ mirrored node, its
// source node): node matrices in breadth-first order of the graph's node
// tree (+C count, +10 rows of 0x20: +4 sibling, +6 child, +8 parent), each
// the parent's (root for node 0) times the orientation's matrix; 1E5540's
// mirrored node takes its source's matrix times the Y-mirrored local
// matrix. `mirrored` NONE is 1E5470.
void animation_node_hierarchy(SourcePhysicsContactMemory&,std::uint32_t graph,std::uint32_t out,std::uint32_t orientations,
    std::uint32_t root,std::uint32_t mirrored,std::uint32_t source,NodeRebuildFp&);
// Differential diagnostics (comparison only): while enabled, the owners
// append per-call notes (BF9A0 overlay function values) to this thread's
// log; the caller prints it for a differing call and clears it.
struct NodeRebuildDebugLog {bool enabled=false;std::string text;};
NodeRebuildDebugLog& node_rebuild_debug_log();
}
