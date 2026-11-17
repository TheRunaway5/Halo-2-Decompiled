# Track `nodebuild` (BF050 node rebuild)

This track replicates TU1.5 BF050 (the object node-matrix rebuild) natively,
together with the routines it executes, over the live world memory. The
research spec is [BF050_node_rebuild.md](../native_replacement/BF050_node_rebuild.md).

| File | Contents |
|---|---|
| `src/gameplay/objects/native_object_factory_nodebuild_tu15.cpp` | owners |
| `include/halo2/native_object_factory_nodebuild_tu15.hpp` | API |
| `tests/native_object_factory_nodebuild_kernels.cpp` | harness kernels |
| `tools/native_object_factory_cases_nodebuild.py` | oracle cases and the composite runner |

## Conventions

- Each owner reproduces the original's world stores exactly: address,
  width, value and order. MOVLPS/MOVHPS stores are single 8-byte stores.
- Floating-point steps run the original instruction under the caller's
  MXCSR, through `campaign/scripting/native_campaign_script_fp_tu15.hpp`.
  Operand order follows the original's destination and source, which keeps
  NaN payloads exact.
- RCPSS (144CF0) runs the host instruction under the same MXCSR.
- The 1E1720 renormalization runs x87 FSQRT, FDIVR and FMUL under the live
  control word. The 80-bit reciprocal root is carried exactly between asm
  blocks with FSTP/FLD TBYTE.
- Stack locals that the original passes by pointer live in a private native
  extent (`Frame`, base 0xFFFE0000). Pointer aliasing and store order are
  therefore the original's, and only world stores reach `memory.write()`.
- BAC00 and C15D0 come from the `nodes` track, BFF20 calls the placement
  track's BA7E0/BA630, and the orientation hooks call tracks weapon
  (1CE720, 1D36C0, E8D50), deviceanim (1E3F30) and units (113C70, 1E14F0).
  Placement and units in turn need havok, unitactions and objtypes, so
  build with
  `-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="nodes;nodebuild;placement;core;weapon;deviceanim;units;havok;unitactions;objtypes"`
  (an empty list, the default, builds every track).
- Callees of other tracks that take a memory and work on the original's
  stack locals (channel records) receive a `FrameMemory`: the Frame's
  private extent reaches its locals, every other address the world, so the
  world stores keep their order and width.
- The API takes `NodeRebuildFp` (`NativeTu15PhysicsFrameFp`, shared with the
  placement track). Leaves that only use SSE take the MXCSR alone. `Frame`
  holds the caller's `NodeRebuildFp` by reference, so control and status
  changes made by another track's callee carry over.

## Ported owners

"Cases" are physical-original cases. Coverage counts executed original
instructions out of the owner's total.

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| C1F40 | `node_mask_empty` | 120 | 32/32 | 1/1 rejected |
| C1EB0 | `node_mask_invert` | 60 | 28/28 | 1/1 |
| C1F00 | `node_mask_limit` | 200 | 25/25 | 2/2 |
| 141010 | `node_mask_last` | 300 | 52/52 | 2/2 |
| C1E50 | `flags_assign_bit` | 100 | 12/12 | 1/1 |
| C1CC0 | `object_animation_frozen` | 100 | 13/13 | 1/1 |
| B87B0 | `animation_manager_valid` | 100 | 12/12 | 1/1 |
| 1E3150 (+1E5C80, 1E3270, 1E5AF0) | `animation_definition` | 400 | 52/52; 74/76, 17/17, 17/19 (a) | 2/2 |
| B8780 | `animation_manager_definition` | 300 | 12/12 | 0/1 (b) |
| E3BC0 (+1F4920) | `biped_physics_height` | 300 | 14/14, 23/23 | 3/3 (c) |
| 1707A0 | `model_default_orientations` | 100 | 46/46 | 2/2 |
| 144CF0 | `orientation_matrix` | 600 | 85/85 | 4/5 (d) |
| 145560 | `matrix_product` | 600 | 69/69 | 3/3 (c) |
| BFC70 | `object_root_matrix` | 800 | 149/149 | 4/4 (c) |
| 10E180 | `object_type_node_hooks` | 400 | 35/35 | 1/1 |
| 1154A0 (+BCDF0) | `unit_node_matrix_hook` | 200 | 29/29, 35/35 | 2/2 |
| 10F440 | `scenery_node_matrix_hook` | 200 | 27/27 | (with 1154A0) |
| 1E1620 (+110FC0, 1E1920) | `animation_interpolation_fraction` | 400 | 49/49, 15/15, 20/20 | 4/4 |
| 1E1720 | `animation_crossfade` | 800 | 135/135 | 5/5 (c) |
| 1D61B0 | `animation_blend_fraction` | 300 | 19/19 | 0/1 (b) |
| 1E32A0 | `animation_codec_entry` | 400 | 54/54 | 2/2 |
| BFB90 (+1D5840, 1CF210, BF9A0, 10E100) | `object_sample_animation` | 800 (e) | 82/82, 39/39 (10E100); see (f) | 7/7 |
| BFF20 | `object_update_bounds` | 1,200 (e) | 199/202 (f) | 6/7 (b) |
| BF0C0 | `object_update_node_matrices` | 1,200 (e) | 367/394 (f) | 11/12 (b) |
| BF050 | `object_rebuild_nodes` | 600 (e) | 35/35 | 2/2 |
| 10C0B0 | `device_orientation_hook` | 400 (e) | 63/63 | 2/2 |
| F34B0 (+F2CC0, F2C40, 1E5160) | `vehicle_orientation_hook` | 816 (e) | 15/15, 522/522, 44/44, 43/43 | 9/9 (c) |
| 1CF3B0 (+283B50, 1E31E0) | `animation_aim_channel` | 600 (e) | 82/82, 170/170, 57/57 | 5/5 |
| 1CF4B0 (+283D70) | `animation_fraction_channel` | 416 (e) | 84/86 (f), 57/57 | 2/2 (c) |
| 28B080 / 28EAF0 (in 283570) | second-frame loops (kinds 2/3, mode 0, unmasked) | (with 283570) | 307/307, 336/336 | 4/4 |
| 114750 (+BBFF0, 1208F0, 111500, CB4A0, 111F70, 1D4400) | `unit_orientation_hook` | 1,016 (e) | 849/849; 102/102, 21/21, 31/31, 38/38, 26/26, 25/25 | 16/16 (c) |
| 10E100 | (in BFB90) | (with BFB90) | 39/39 | — |

Notes:

- (a) The only uncovered instructions are the 126200 calls (resource
  admission). They never execute in the six campaign censuses and are
  refused by name.
- (b) The survivor is equivalent:
  - B8780: 1E3150 itself returns 0 when the high word is 0xFFFF.
  - 1D61B0: any x above the 0.9999 limit becomes at least 1.0001 after the
    x1.0002 step, so the final clamp returns 1.0 either way.
  - BFF20: an unconditional `s*offset` with s == 1.0 differs only under FTZ
    without DAZ, which no live control uses. DAZ zeroes a denormal offset in
    the next multiply anyway, and an sNaN is quieted identically.
  - BF0C0: the 0x4EC4EC4F divisor and divisor+1 differ only for
    sign-extended negative node blocks, which overflow the 256-node mask and
    are refused.
- (c) Includes mutants that first survived and were then rejected after
  fixture strengthening: ramp rounding with random tick seconds, NaN-payload
  scales, partial output/B aliasing, mirror dwords whose low byte is not
  0/1, a previous bank 4 bytes below the current one (1E1720), and
  NaN-payload translations on both sides of the kind-2 layering. For the
  orientation hooks, three mutants (F2CC0's aim value clamp to 1.0,
  283D70's float clamp before CVTTSS2SI, 114750's centered test without the
  yaw) survived the random kernels and are rejected by the probes
  `nodebuild_vehicle_aim_limit`, `nodebuild_fraction_overflow` and
  `nodebuild_unit_yaw`; dropping 1E5160's index store makes the native loop
  forever (rejected by the hang). Over this table and the codec owners from
  296AD0 on (next section), 144 of 149 mutants are rejected; the 5
  survivors are the equivalent ones in (b) and (d).
- The orientation hooks' 38 mutants were screened out of tree: each mutant
  is a scratch copy of the source compiled with the kept build's flags and
  linked into a scratch binary, so the in-tree source is never rewritten
  (its SHA-256 is checked before and after). The two track-decoder survivors are in (g). BFF20's three binding mutants (BA7E0 or BA630 dropped,
  BA630 before the sphere stores) are rejected by
  `nodebuild_bounds_connected`.
- (d) The survivor `add(I1, -W2)` versus `sub(I1, W2)` is equivalent. W2 can
  be NaN only when s is NaN, so every lane is NaN and both forms return the
  destination NaN.
- (e) Composite owners use the track runner (next section), not the shared
  validator.
- (f) The uncovered instructions all follow a refused callee: 1DBBD0 or
  B3AE0/B3B70 for BF0C0; 1DC7A0 for BFF20; 284090 or 1D63C0 for 1CF210,
  1D5840 and 1CF4B0; the overlay loop for BF9A0.
- The hooks' callees from other tracks are not duplicated: 1CE720, 1D36C0
  (through 1D3740) and E8D50 are track weapon's, 1E3F30 track deviceanim's,
  113C70 and 1E14F0 track units', BC200 is `native_tu15_object_velocities`
  (src/physics), and 1E1620 is this track's `interpolation_fraction`.
  1D3C80 and 1D4450 are track units' exported `manager_overlay_trial` and
  `manager_aim_handle` (since 2026-11-12; F2CC0 passes both keys as
  0x7000101, the manager's +74/+78).
- x87: 114750's angles and 1208F0 run FPATAN/FSQRT as the original's
  register sequences under the live control word; 283B50's pitch rows run
  FILD/FADD/FST/FSTP QWORD and the CRT floor/ceil FRNDINT under 0x173F /
  0x1B3F. The original runs on the same host CPU, so FPATAN matches
  bit for bit.

Combined results (seed 0x5C41, `HALO2_ORACLE_STRICT_WRITES=1`):

- Shared validator, all 35 leaf kernels (21 node/animation owners and 14
  track decoders), run together so that the 10E180 hook targets are
  authenticated:
  `PASS 12480 physical original evaluator tails, 24960 native comparisons, 86919 ordered original stores`.
  The command is
  `HALO2_FACTORY_TRACKS=nodebuild python3 tools/run_test.py -- .tools/python/bin/python tools/validate_native_campaign_script_builtins_tu15.py --native <harness> --report <json> --only nodebuild_...`.
- Composite runner, all composite kernels including the three review probes
  (`nodebuild_vehicle_aim_limit`, `nodebuild_fraction_overflow`,
  `nodebuild_unit_yaw`, 16 cases each, on a format-1 animation so their
  forced decoders are native and they reach the clamps) (2026-11-12, after
  the orientation hooks):
  `PASS 10248 physical composite cases, 727342 ordered original stores`.
  The command is
  `HALO2_ORACLE_STRICT_WRITES=1 HALO2_FACTORY_TRACKS=nodebuild python3 tools/run_test.py -- .tools/python/bin/python tools/native_object_factory_cases_nodebuild.py --native <harness> --report <json> --coverage <entries>`.

  | Kernel | Completed | Stopped at a boundary |
  |---|---|---|
  | `nodebuild_sample` | 452 | 148 284090, 55 BF9A0, 33 1D63C0, 20 1236A0, 16 2CEBE0, 14 297550, 12 283E10, ... |
  | `nodebuild_device_hook` | 329 | 37 284090, 7 1236A0, 6 285370, 5 each 2CEBE0 and 125FB0, ... |
  | `nodebuild_vehicle_hook` | 349 | 59 284360, 55 297550, 49 each 283E10 and 1236A0, 37 125FB0, 36 2CEBE0, unported decoders |
  | `nodebuild_unit_orientation_hook` | 398 | 316 284090, 61 284140, 41 1236A0, 37 284360, 26 297550, ... |
  | `nodebuild_aim_channel` | 498 | 41 284090, 30 284140, 6 28E0B0, 5 1236A0, masked second-frame loops, ... |
  | `nodebuild_fraction_channel` | 214 | 98 284090, 46 284140, 13 1236A0, 6 125FB0, ... |
  | `nodebuild_bounds` | 588 | 12 1DC7A0 |
  | `nodebuild_bounds_connected` | 600 | none |
  | `nodebuild_node_matrices` | 1,064 | 101 1DBBD0, 17 B3AE0, 5 each BF9A0 and 1D63C0, 3 1236A0, ... |
  | `nodebuild_rebuild` | 483 | 81 1DBBD0, 16 B3AE0, 7 284090, 3 each 2CEBE0, 284140 and 1D63C0, 1 1DC7A0, ... |
  | `nodebuild_codec_frames` | 619 | refused frame loops and track decoders (111 284140, 106 284290, 55 284360, ...) |
  | `nodebuild_codec_setup` | 583 | 101 284140, 19 each 2849F0 and 284360, 10 2843F0, ... |
  | `nodebuild_resource_data` | 375 | 17 1236A0, 8 125FB0 |
  | `nodebuild_codec` | 558 | 115 284140, 25 1236A0, 20 125FB0, 19 297550, ... |

## Orientation-hook qualification (2026-11-12 continuation)

The unit, vehicle and device orientation implementations left by the previous
session now pass together against the unchanged physical original. The resumed
cohort contains **10,200 composite cases and 722,446 ordered stores**. It uses a
fresh all-tracks harness, built through `tools/run_tests.py` inside an owned
`tools/run_test.py` coordinator. The report is
`build/test-results/resume-nodebuild-20261112/composite.json`.

| Owner | Role | Original instruction coverage |
|---|---|---|
| 114750 | Unit aim, look, moving and channel overlays | 849/849 |
| 10C0B0 | Device position and overlay channels | 63/63 |
| F34B0 / F2CC0 / F2C40 | Vehicle overlay selection and sampling | 15/15, 522/522, 44/44 |
| 1CF3B0 / 283B50 | Aim-channel and yaw/pitch sampling | 82/82, 170/170 |
| 1CF4B0 / 283D70 | Fraction-channel and fraction sampling | 84/86, 57/57 |
| 28B080 / 28EAF0 | Unmasked second-frame loops | 307/307, 336/336 |
| 1E31E0 / 1E5160 / 1D4400 | Aim lookup, graph iterator and channel setup | 57/57, 43/43, 25/25 |
| BBFF0 / 1208F0 / 111500 / CB4A0 / 111F70 | Unit aiming helpers | 102/102, 21/21, 31/31, 38/38, 26/26 |
| 10E100 | Per-type orientation dispatch | 39/39 |

The two unexecuted 1CF4B0 instructions, 1CF525 and 1CF52A, follow its
refused 284090 mask-intersection call. Other recorded refusals remain part of
the tested prefix contract; these results do not establish complete animation
streaming, active-Havok node rebuild, campaign playability or launcher delivery.

Two oracle issues were repaired before qualification:

- The direct frame-decoder fixture enabled the newly reachable second-frame
  loops without initializing their frame indices. Case 56 faulted the original
  at 2963F5 because state +54 held 0xAA3D2FA8. The fixture now supplies valid
  second-frame indices/fractions, as 2835B0 does. That exact case completes;
  the focused 1,200-case codec cohort passes with 10,984 ordered stores.
- A codec-only selection previously authenticated unrelated type hooks,
  bringing in CRT/placement code without those owners' declared boundaries.
  Metadata now adds indirect targets only when their owning routine is reached.
  The focused codec command passes independently.

The targeted `nodebuild_vehicle_aim_limit`, `nodebuild_fraction_overflow`
and `nodebuild_unit_yaw` cases close three gaps in the randomized mutation
cohort. They use a live aim overlay with a non-unit yaw scale, positive
infinity before float-to-integer conversion, and a horizontal turn with zero
pitch but nonzero yaw. Each additional 16-case baseline passes against the
original, without supplying callee outcomes.

All **38 orientation mutation controls are rejected** after that strengthening:
37 produce a parity failure, and omitting 1E5160's iterator-index store makes
the native loop indefinitely. The latter is reproduced on one case with a
passing baseline and rejected by a 20-second bound. Mutants are compiled and
linked from private copies; the production source hash is unchanged. Receipts
are under `build/test-results/resume-nodebuild-20261112/` (the initial screen,
its bounded continuation, and the three targeted regressions). No production
kernel change was required during this continuation.

The combined results above supersede this cohort's counts: they were rerun
on the final fixtures (its second-frame fixture kept, probes included).

The abandoned `halo2-test-tio8pn31` build from the earlier nodebuild session
was retired after the replacement proof and an ownership scan: 334,035,684
logical bytes, 430,010,368 allocated bytes. Its small notes and reference
listings remain intact. New probes, mutation binaries and original banks are
owned by `tools/run_test.py` and removed after each run.

## Animation codec (283A90)

283A90 is native: `animation_decode` decodes animation `id` of a graph at
a frame, with a weight, into the orientations. Only the resource cache
miss is refused (see Refusals).

- **1E5DF0/1E5A70/1262F0 (resource record):** +0 codec data (the cached
  block, `(record +8 << cache +34) + [0x4F0420]`, plus animation +28; or
  animation +38 without +16 bit 4), +4 header (animation +3C), +8 node count,
  +9 animation +11, +A frame count. A cached row (+C handle) stamps the
  cache record's age (+14) and finishes a pending load (1262B0); 126190's
  request scan is native.
- **2835B0 (state block 0x510F30):** +00 kind, +04 mode, +08 force byte, +0C
  node count, +10 orientation count, +14/+18 frame and next frame, +1C
  fraction, +20 weight, +24 animation, +28 inherited entry, +2C node mask,
  +30 codec data, +34/+38/+3C rotation, translation and scale decoders (the
  40-byte formats at 0x48C33C, +12 when forced), +40/+44/+48 flag streams
  (one byte per 8 nodes), +4C orientations, +50 second-frame flag, +54..+60
  second frame, +64/+68/+6C track counters, +70 decoder output. It clears
  0x56E000..0x56E05F first. Kind 0 replaces; kind 2 (additive, full
  weight) and kind 3 (additive, partial weight) decode into the orientation
  0x510F10 and layer it. Mode 0 indexes nodes directly; modes 1 and 2 go
  through the inherited entry's remap (+0C, mask +14), mode 2 for node 0's
  translation and scale only. Forced interpolation keeps the +12 decoder
  set unless the fraction snaps to a key.
- **Frame loops (283570):** 2847D0/284D30 (kind 0, mode 0), 285A70 (mode 1),
  2861A0/2868A0 (mode 2); 28ADA0/28B5F0 (kind 2); 28E730/28F110, 290710,
  2912A0/291E30 (kind 3). Each loop calls the track decoders indirectly
  through +34/+38/+3C, so `codec_track` dispatches on their addresses.
  The quaternion layering is the original's SHUFPS sequence, run as packed
  MULPS/ADDPS/SUBPS under the live MXCSR.
- **SIMD steps:** MULPS/ADDPS/SUBPS/RSQRTPS each run as one packed asm block
  under the live MXCSR. Quantized keys are dequantized exactly
  (MOVQ/PUNPCK*WD/PSRAD/CVTPI2PS on int16 lanes). MOVAPS outputs are single
  16-byte stores.

| Owner | Kind | Cases | Coverage | Mutations |
|---|---|---|---|---|
| 296180 | rotation, one quantized key | 300 | 31/31 | 1/1 |
| 296680 | rotation, quantized key at the frame | 300 | 34/34 | 1/1 |
| 296450 | rotation, quantized keys blended | 300 | 59/59 | 1/1 |
| 296240 | rotation, float keys blended | 300 | 40/40 | 1/1 |
| 2963D0 | rotation, float key copied | 300 | 18/18 | 1/1 |
| 2961F0 | translation, one key | 300 | 15/15 | 1/1 |
| 296410 | translation, key at the frame | 300 | 17/17 | 1/1 |
| 296220 | scale, one key | 300 | 8/8 | 1/1 |
| 2CEC60 | scale, key at the frame | 300 | 10/10 | 1/1 |
| 296C60 | translation, sparse keys (binary search + RCPSS interpolation) | 600 | 97/97 | 2/3 (g) |
| 296DB0 | scale, sparse keys | 600 | 71/71 | 2/3 (g) |
| 296AD0 | rotation, sparse byte keys (frame-distance blend) | 600 | 124/124 | 3/3 |
| 297270 | rotation, sparse word keys | 600 | 123/123 | 1/1 |
| 297400 | translation, sparse word keys | 600 | 96/96 | 1/1 |
| 283570 (+2840D0, 2841B0, 284220, 284300, 284390) | `animation_codec_frames` dispatch | 1,200 (e) | 17/17; 33/33, 33/33, 32/33, 11/12, 12/12 (h) | 2/2 |
| 2847D0 / 284D30 | kind 0, mode 0 (masked / not) | (with 283570) | 162/162, 141/141 | 3/3 |
| 285A70 / 2861A0 / 2868A0 | kind 0, modes 1 and 2 | (with 283570) | 194/194, 214/214, 198/198 | 4/4 |
| 28ADA0 / 28B5F0 | kind 2, mode 0 | (with 283570) | 204/204, 178/178 | 4/4 (c) |
| 28E730 / 28F110 / 290710 / 2912A0 / 291E30 | kind 3 | (with 283570) | 235/235, 209/209, 267/267, 293/295 (i), 271/271 | 3/3 |
| 2835B0 (+283E10) | `animation_codec_setup` | 800 (e) | 296/301 (j); 21/196 (k) | 10/10 |
| 1E5A70 (+1262F0, 126190, 1262B0) | `animation_resource_data`, `animation_resource_load` | 400 (e) | 44/44; 43/61, 46/47, 13/24 (l) | 6/6 |
| 283A90 (+1E5DF0) | `animation_decode` | 800 (e) | 68/68, 29/29 | 2/2 |

| 296540 / 2962F0 | translation, float keys blended (keys staged in the .bss statics 0x56E688 / 0x56E6A0, current at +C) | (composite, 2026-11-14) | 45/45, 45/45 | 7/8 (n) |
| 296620 | scale, keys blended | (composite, 2026-11-14) | 21/21 | (with 296540) |

(n) The survivor reads the keys from their source rows instead of the
statics: equivalent, since the statics are .bss words no output or key row
can alias. Composite proof (strict, seed 0x5C41, `nodebuild_codec_frames`,
`nodebuild_codec`, `nodebuild_codec_second_masked`): `PASS 2300 physical
composite cases, 71468 ordered original stores`, the same under the
ASan/UBSan harness; the statics 0x56E688..0x56E6B7 are declared with
144CF0's 0x56E680 block (`codec_banks`).

(g) Two survivors:
- The 296C60 weight operand order is equivalent: there are no NaN inputs,
  and RCPSS(0) times the frame offset is +-inf in either order.
- 296DB0's re-read of the just-stored output, versus the value row, differs
  only when the output aliases the next value row. The fixtures do not
  exercise that aliasing.

(h) The missing sites are the jumps to refused second-frame loops
(28F460, 285CF0) that no case selected.
(i) The `n+8 <= count` arm of the translation and scale passes: mode 2
clamps the count to 1 first, so it is unreachable.
(j) The two clamped next-frame stores for a negative index (unreachable:
the index is clamped to 0 first) and the kind-4 store after 283E10, which
is refused.
(k) 283E10 is native up to its event check (animation +64 <= 0); the rest
is refused at 0x283E35.
(l) The cache miss (125FB0 onward), the pending-request release (16E8A0),
126190's matching-request return and 1262B0's load wait (1236A0) are
refused.

## Composite runner

The static closures of BFF20, BF0C0, BF050 and BFB90 reach BA630, then
B9B70, then the whole factory, plus CRT and codec jump forms. The shared
validator cannot bound these closures, so
`python3 tools/run_test.py -- .tools/python/bin/python tools/native_object_factory_cases_nodebuild.py --native <harness> --report <json> [--only K] [--coverage entries] [--gold-cache <json>]`
proves them instead. It reuses the validator's `World`, reference build
and ptrace observer:

- The closure stops at declared boundary entries (and cut entries reachable
  only through them).
- The original stops before entering a boundary.
- A case that reaches a boundary must make the native refuse with that
  owner's name, after the identical ordered store prefix and with identical
  final banks.
- Every other case must complete with identical stores, result (the full
  EAX for `long` kernels) and banks.
- Indirect targets are authenticated as extra closure entries: the 10E180
  type hooks (1154A0/10F440), the 10E100 hooks (114750, F34B0, 10C0B0),
  F34B0's channel callback F2C40 (from F2CC0), the native track decoders
  (from every frame loop, the second-frame loops included) and placement's
  machine callback 118120 (from 10DDE0). The unported track decoders are
  boundaries.
- The CRT floor/ceil error reporters (32EBC3, 32EA95, 32EAE8) are cut: the
  original reaches them only for a NaN pitch row, after it has changed the
  x87 control word, so no boundary can stop there (the observer requires
  the entry control word at a stop). The fixtures keep pitch rows numeric.
- The closure stops are the union over the selected kernels, so a shared
  routine that one kernel executes (16E8A0, run by placement's datum
  deletes) is bounded at another kernel's call site instead (0x126307 in
  1262F0).
- `--gold-cache` records the original outcomes and reuses them for
  identical cases (mutation screens); `--coverage` always reruns them.
- `World.banks` packs at most 8 argument words, so the runner packs only
  the first 8; longer lists (2835B0, 283A90) reach the original through
  registers and stack words.

Kernels: `nodebuild_sample` (BFB90), `nodebuild_device_hook` (10C0B0),
`nodebuild_vehicle_hook` (F34B0), `nodebuild_unit_orientation_hook`
(114750), `nodebuild_aim_channel` (1CF3B0), `nodebuild_fraction_channel`
(1CF4B0), `nodebuild_bounds` and `nodebuild_bounds_connected` (BFF20),
`nodebuild_node_matrices` (BF0C0), `nodebuild_rebuild` (BF050),
`nodebuild_codec_frames` (283570 over a direct state block, second frame
included), `nodebuild_codec_setup` (2835B0), `nodebuild_resource_data`
(1E5A70) and `nodebuild_codec` (283A90).

Scene fixture: eleven objects with bodies in the TRACK bank (stride 0xA00)
in an acyclic parent/child forest. Each body has 9 node matrices,
orientations, an animation manager with three channels and an interpolation
state, and previous orientations. Every body also carries the hooks' state:
device channels +18C/+1AC, a unit aim block (+33E, overlapping the previous
orientations: flags, fades +80..+93, channels +9C/+BC/+DC, handles
+FC..+120, the aim/look handles unset), seat, weapon slots, targets, and the
vehicle overlay cache (+400..+424) and values. Unit and vehicle types only
go to bodies whose definition has a render model (114750 and F2CC0 read it
unchecked). The scene also includes:

- object definitions (0x400 bytes each, in the FACTORY bank: the hooks read
  +BC, +1CC seats, +1F4/+1F8/+20C), models and render models (tag rows
  16..25);
- graph hook tables: aim records (+28 rows of 0x1C; animation +12 index), a
  one-path mode table (+34, a leaf with an overlay table of the hooks'
  names for 1E3F30) and the vehicle +40 rows (count +3C);
- physics components for BC200 (+B4 datums 5 and 6: root slot, rigid body,
  motion velocities) and the zero vector [0x474D04]; 114750's default
  angles [0x474D24];
- three jmad graphs (rows 26..28) whose animations carry codec fields
  (+10 kind, +13 nodes, +14 frames, +16 resource bit, +24 resource row, +28
  data offset, +38 direct data, header +3C, events +64); inheritance
  entries admitted by 1E32A0 carry remap (+0C) and mask (+14) tables;
- the resource cache at the TRACK tail (0x4F0418/1C/20/24 globals): three
  cached blocks and a fourth blob (for +28 offsets), each blob a static
  codec block (+0), an animated block (+800) and flag streams (+1000) in
  formats 1-8, occasionally a miss, a pending request or an unfinished
  load;
- the statics 0x56E680/0x56E684, 0x56EB88..0x56EBAF, 0x56E060, 0x4E8BC0
  (255 orientations), 0x474D38, 0x47C1B4 and 0x51DF20, the codec banks
  0x510EF0..0x510FA3 and 0x56E000..0x56E05F, and the literal tables
  0x473E48..0x474BC4 (type definitions) and 0x48C33C (codec formats);
- the physics pool pointer.

Scene objects are never connected (+4 bit 8) unless their cluster sphere is
unchanged: BA7E0/BA630 need the placement track's world.
`nodebuild_bounds_connected` therefore builds placement's reconnect world
(`placement_reconnect_lights`: map, clusters, partitions, lights and light
attachments) and prepares one object for BFF20 (node 0 at body +380,
definition bounds, scale); 85% of them are connected. Its closure stops
where placement's kernels stop (Havok, unported light paths, other
tracks).

The observer silently ignores stores to image memory that is not declared
as a bank, so every static an owner writes must be declared. Banks must lie
in the image or the fixed validator sections (DATA*, AI, FACTORY, TRACK).

## Havok nodes, crate bounds, cache misses and overlays (2026-11-14)

These owners close every refusal the live differential met in the Armory
(01a) and, except C9070 (below), on Coagulation (4 players). The final
strict composite run over every nodebuild kernel (new fixtures included):
**PASS 11,648 physical composite cases, 785,444 ordered original stores**
(BF0C0 368/394, missing only the refused B3AE0 prediction tail; BFF20,
BF050, BFB90 complete); the targeted `nodebuild_codec_second_masked`
adds 300 cases and 16,021 stores. The leaf decoders 296700 and 296AD0 pass
1,200 cases through the shared validator. An ASan/UBSan harness replays
the new and changed composites (physics nodes, crate sphere, node
matrices, bounds, sample, codec frames, codec, resource data): PASS 6,400
cases, 234,135 stores, no sanitizer report.

Mutation screen (out of tree: each mutant a scratch copy compiled with the
kept harness flags and linked into a scratch harness; the in-tree source
SHA-256 79e5dc0e... is unchanged afterwards): all 40 non-equivalent
mutants are rejected, covering 1DBBD0 (exit bit, fixed motion, masked and
descendant delivery, height operand order, parent walk, empty render
model), 1D8B90 (transform bit, lanes), 2E44D0 (translation, sum order),
1DBFD0 (translation offset, mask, orientation scale, product order,
physics orientation), 1DC7A0 (active bit, definition bit, ties, zero pad,
min/max order, radius pad), 2EA050 (endpoint index, offset, z
reciprocal), 28A7D0/28C8F0 (restore order, mask, counting, layering
order), 16E770 (bitmap, salt, high-water mark), 1262F0/1260C0 (entry
clears, store order), 125FB0 (budgets, busy callback), 1263C0 and 126400.
Two first-round survivors were resolved: a mutant that reused the
float-stored z reciprocal (equivalent by construction, replaced by one
dividing by the y scale) and the masked kind-2 layering order (rejected
once `nodebuild_codec_second_masked` made both operands NaN).

| Owner | Native | Proof (strict, seed 0x5C41) | Coverage |
|---|---|---|---|
| 1DBBD0 (+1D8B90, 2E4650/2E44D0, 1DBFD0) | `object_physics_nodes`; BF0C0 calls it on its Frame-local physics mask | `nodebuild_physics_nodes` 800 + every BF0C0/BF050 composite | 1DBBD0 249/281 (the 32 missing are the refused child shape transform), 1DBFD0 186/186, 1D8B90 55/55, 2E4650 20/20, 2E44D0 40/40 |
| 1DC7A0 (+2EB0C0, 2EA050) | `crate_physics_sphere`; BFF20 uses it for active crates | `nodebuild_crate_sphere` 600 + the BFF20/BF050 composites | 1DC7A0 231/232 (1DC999: a null collidable, which faults), 2EB0C0 7/7, 2EA050 72/72 |
| 28A7D0 / 28E0B0 | masked second-frame loops (kinds 2/3) | `nodebuild_codec_second_masked` 300 (NaN-rich, forced kind 2/3, masked, flagged), `nodebuild_codec_frames`, `nodebuild_codec`, `nodebuild_sample` | 331/331, 360/360 |
| 28C8F0 / 28BF00 | kind 2, mode 1 (remapped), output-first layering | same | 240/240, 252/256 (node 0's z scale under a mask) |
| 296700 | forced sparse byte-key rotation (frame + fraction) | `nodebuild_codec_296700` 600 leaf cases | complete |
| 125FB0 (+13FE50, 16E770, 1260C0, 1263C0, 126400) | the 1262F0 cache miss | `nodebuild_resource_data`, `nodebuild_codec`, `nodebuild_sample` (stop at 1A3B70) | 125FB0 50/53 (13FD10 rollover refused), 13FE50 346/388 (priority-only and window branches priority 0 never takes), 16E770 45/54 (failure returns), 1260C0 41/64 (after the 1A3B70 stop), 140310 43/43 |
| BF9A0 (+37DC0A/37DC14) | function overlays: 1E5C80, 1D36C0, 1E3150, BCB70, 1CE810, CRT fmod, 1CF210 | `nodebuild_sample` 800 | BF9A0 143/143, 37DC14 9/9 |

- **1DBBD0.** Unless component +4 bit 9, every row (+70 rows of 0x60: +40
  rigid body, +48 record indices, +4C count) delivers its non-fixed records
  (motion getType, the six literal 31D520..31EDE0 owners, != 6) whose node
  is not already in the mask and not below the row's first record's node:
  the node matrix from the motion's +80 transform (1D8B90; times the
  component's +30 transform under +4 bit 6, the 2E44D0 packed product),
  raised by `height` (E3BC0 for bipeds). The node joins the mask; its
  parent chain joins the 8-dword delivery mask; 1DBFD0 then walks the
  physics model's node tree (+CC rows: +6 parent, +8 sibling, +A child):
  physics nodes publish their local orientation (144090, 145560, 144E60 of
  track deviceanim), reached nodes take the render model's default local
  orientation under the parent's matrix (144910 of track placement). A
  non-root record's child shape transform (shape vtable +34) is refused:
  no census executes it. BF0C0's stack mask is zeroed only for
  (count + 31) >> 5 dwords; a node beyond it is refused, like the
  uninitialized read it would be.
- **1DC7A0.** The union of the rows' hk3AxisSweep AABBs (2EB0C0 -> 2EA050:
  the x87 dequantization runs as one asm block under the live control word,
  the x and y reciprocals extended, z's stored as a float first), each
  padded by the largest record +80 radius unless it is exactly 0.0; center
  0.5 * (min + max) and radius sqrt of the squared half extents + 0.001.
- **Cache miss.** 125FB0 runs 13FE50 through the native page allocator
  (`native_tu15_page_allocate_abi`, src/core) over a private stack in the
  Frame, with native 1263C0 (busy) and 126400 (delete) callbacks; a failed
  allocation retries three times without the busy callback, aging the
  cache. 16E770 claims the entry at the page's index; 1260C0 reads through
  the bound `AnimationResourceReader` (production: the live World's
  synchronous animation cache read, the same descriptor/size checks,
  payload, retained job and completion byte as the 0x126139 interception)
  and mode 0 waits (1262B0). Without a reader (the oracle harness) the miss
  refuses at 1A3B70, where the original stops.
- **BF9A0.** Each overlay row requests its animation, starts a private
  channel (flags 0x7F) and evaluates the object's function: mode 0 sets the
  frame to (frames - 1) * value at full weight; mode 1 takes (tick +
  object) * [time +4] / 30 modulo the frame count (CRT fmod: FPREM for a
  normal dividend, FLDZ for zero/denormal, error methods refused at
  3334F0) at weight `value`.

The fixtures behind these proofs: components (datums 5/6) with up to three
rows, six rigid bodies and motions (aligned +80 transforms, NaN-rich
translations), a physics model per object model (tag rows 29..31: records
with nodes 0..8 and +80 radii, a parent < child node tree), the hkWorld
[0x52BE48] and its broadphase (vtable +20 2EB0C0, six nodes, three axes);
a consistent page-allocator owner for the animation cache (callbacks, total
pages, ticks never newer than the cache, records and entry pools in step)
and the priority/age table 0x474D70; function overlay rows with the
literal names BCB70 answers without object state and events on every
scene animation.

## Event frames, kind 4, mask intersection, forced sparse decoders (2026-11-15)

The 2026-11-15 map sweep (scripted input: fire, grenades, jump, use,
crouch, movement) refused on every multiplayer map at 283E10 and 284090,
and on Cyclotron, Gemini, Triplicate and Waterworks at 296890. These are
now native:

| Owner | Native | What |
|---|---|---|
| 283E10 (+120740, 120570 of track deviceanim) | `codec_previous` | For kind 0, each event row (animation +64 count, +68 rows of 0x1C: +0 node, +2 plane flags, +4 packed quaternion, +C translation, +18 scale) whose node (through the entry's +14 mask and +C remap) is below the count composes the node's ancestry (graph +10 rows of 0x20: +4 sibling, +6 child, +8 parent) from the orientations, inverts it (120740), applies the row and publishes the result to every child below the count: row 0x50EF10 + 32 * child and its bit in the rotation, translation and scale planes 0x56E000/20/40 (set or cleared by the row's flag bits 0..2). Returns true when a child was published: 2835B0 then sets kind 4. |
| 120740 | `orientation_inverse` | Conjugate rotation; translation -(R^-1 t) / scale; scale 1/scale (10000 for a zero scale, 1 kept exactly). |
| 284290 / 2842C3 / 2843C0 (kind 4 selectors) and 293290, 2928E0 (mode 0), 2947B0, 293CB0 (mode 1), 295DA0, 295280 (mode 2) | `frames_direct` / `frames_remapped` with `events` | Kind 0's loops; after each decoded node whose plane bit is set: rotation event * decoded; translation: the event's x, y and z added in turn into the decoded +10 (three stores to +10, the original's); scale event * decoded (decoded * event in 2928E0). Mode 1/2 events are at the remapped node. The second-frame forms 292C90/292280, 2940F0/2935C0 and 2956D0/294B80 are refused by name. |
| 284090 | in `channel_mask` | 1CF210's REP MOVSD of the channel mask into 0x56E060, then the in-place AND with the caller's mask (word 7 read before word 6 is stored, the mask's word 7 after it). |
| 296890 / 2969E0 | `sparse_forced` | Forced sparse byte-key translation / scale: the key at or before the frame is copied; before the last key the next value is blended by (frame + fraction - key) * RCPSS(next key - key). |

Proof (strict writes, seed 0x5C41, the all-tracks harness):

- Composite runner, every nodebuild kernel (new fixtures included):
  `PASS 11948 physical composite cases, 988469 ordered original stores`;
  the new `nodebuild_codec_events` kernel (kind 4 forced, every mode,
  masked or not, planes mostly set): `PASS 400 physical composite cases,
  7153 ordered original stores`.
- Shared validator, the leaf decoders 296890, 2969E0 (and 296C60, 296DB0
  as controls, NaN-rich values in a third of the cases): `PASS 2400
  physical original evaluator tails`.
- Coverage: 283E10 196/196, 120740 104/104, 293290 221/221, 2928E0
  256/256, 2947B0 263/263, 293CB0 291/291, 295DA0 265/267 and 295280
  293/295 (the `n + 8 <= count` arm of the translation and scale passes:
  mode 2 clamps the count to 1 first, unreachable), 284290 33/33, 2842C3
  18/18, 2843C0 12/12, 296890 95/95, 2969E0 69/69, 284090 23/23, 1CF210
  137/137.

The event orientation rows are a 255-row bank 0x50EF10..0x510EEF (row 255
is the codec bank 0x510EF0). The fixtures add event rows to the codec
setup and the scene animations, node forests to the graphs (+C count, +10
rows) and NaN-rich event banks; the codec frame fixture reaches kind 4
through its direct state.

## Codec kind 1 (284140, 2026-11-15)

Kind 1 (non-additive, partial weight) was the most frequent stop of the
randomized composites. Its selectors 284140 (modes 0/1) and 284330 (mode
2) and eight of its twelve loops are native, built on the kind 2/3 loop
structures (`frames_layered`, `frames_layered_remapped`,
`frames_layered_second(_masked)`) with `partial`:

- `layer_partial`: the output moves toward the decoded track by the weight
  (+20). The rotation takes `nlerp` with the output first:
  p = out * decoded, the sign of (p0+p2)+(p1+p3) flips the decoded
  quaternion, r = (decoded - out) * w + out, then r * RSQRTSS of
  (r0r0+r2r2)+(r1r1+r3r3) as one 16-byte store. Translation and scale
  lanes take (decoded - out) * w + out.
- The second-frame forms (mode 0) first blend the frame and second frame by
  +60 (the same `nlerp`, which `blend_second` now shares). Kind 1 stores the
  blended translation lanes in x, y, z order (kinds 2/3: y, x, z). The
  unmasked form restores the next frame before the frame in every pass.

| Loop | Kind 1 form | Coverage |
|---|---|---|
| 287130 / 287A20 | mode 0, masked / unmasked | 223/223, 193/193 |
| 286B30 / 287480 | mode 0, second frame, masked / unmasked | 340/340, 313/313 |
| 2883C0 / 288E70 | mode 1, masked / unmasked (node 0's z scaled by the entry's +18) | 280/280, 253/253 |
| 289920 / 28A3F0 | mode 2, masked / unmasked (count 1 after the rotation pass) | 284/284, 257/257 |
| 284140 / 284330 | selectors | 33/34 (2841AC, the return for mode > 2, which 283570 never passes), 12/12 |

The mode 1/2 second-frame loops 287D00/2887F0 and 289240/289D60 stay
refused by name, like kinds 2/3's.

Proof (strict writes, seed 0x5C41): the new `nodebuild_codec_partial`
kernel (kind 1 forced, every mode, masked or not, the second-frame flag
mostly with mode 0, NaN-rich translations, a separate second-frame
fraction, and NaN rotation lanes of both signs) gives `PASS 600 physical
composite cases, 15397 ordered original stores`, and `PASS 600 ...,
15946 ordered original stores` once NaN rotation lanes were added. The
other kernels' kind-1 cases now run natively too. ASan/UBSan
(`nodebuild_codec_partial`, `nodebuild_codec_frames`, `nodebuild_codec`):
`PASS 2600 physical composite cases, 80155 ordered original stores`, no
report.

Mutation screen (out of tree, in-tree SHA-256 unchanged): 10 mutants. 9 are
rejected: the weight (+60 for +20), lane operand order, the sign flip,
the rotation roles, the second frame's lane order, the restore order, mode
2's node count, the masked second-frame form, and the mask. The survivor
pairs the sign dot as (p0+p1)+(p2+p3). It is not equivalent: the sign can
differ when the four lane products cancel to about zero and round
differently. NaN lanes cannot expose it, because one NaN lane makes the
whole normalized result NaN whatever the sign. A crafted output/decoded
pair would; the kind 2/3 `blend_second` shares the pairing and has the
same open screen.

## The 1159D0 marker name on MCC caches (2026-11-15)

1159D0 passes the TU1.5 literal 0x4000095 to BAD60 in both routes. That
literal is "head": string index 0x95, whose MCC StringID is also
0x04000095 (`src/ui/mcc_frontend_string_ids.inc` joins TU1.5 index 0x0095
to MCC 0x04000095 "head"; the MCC namespace first diverges far above
it, for example "one" 0x5A6 -> 0x5B2). So the hand-IK marker lookup asks
for the same marker in both namespaces. No join is needed, and nothing is
missing in either route.

Neither route reaches the lookup in the runs measured. Translated-route
census (`HALO2_REFERENCE_NODE_REBUILD_TU15=1`, instruction counts):

| Run | 1159D0 | 115940 body | 1159AE (+148 bit 15 or +137 bit 0 test) | 1159B7 (hand nodes) | BAD60 from 1159D0 (115A3A) | 115500 |
|---|---|---|---|---|---|---|
| Armory, 300 frames | 590 | 590 | — | 0 | 0 | 0 |
| Coagulation 4p, 450 ticks, scripted input | 4,374 | 4,374 | 2,867 | 0 | 0 | 0 |

115940 declines every call at its +148 bit 15 / +137 bit 0 test (1,507
Coagulation calls stop earlier, at a manager without a graph, +0 NONE). Hand IK
therefore only runs for a unit with one of those bits, as the oracle's
`nodebuild_unit_hand_ik` fixtures construct; whether such a unit's model
carries a "head" marker is model content, the same in both routes.

## Refusals (dependencies)

| Address | Why | Owner |
|---|---|---|
| 1A3B70 (0x126139) | the animation resource read | native in production through the bound reader; the oracle stops there |
| 13FD10 | animation cache age rollover (tick 0x7FFFFFFF) | not native; rare |
| 16E8A0, 1236A0 | pending resource request release; wait for an unfinished load (also from 126400 evicting an in-flight entry) | not native; rare |
| 287D00, 2887F0, 289240, 289D60 | codec kind 1's mode 1/2 second-frame loops | not native (284140 itself is native since 2026-11-15) |
| 292C90, 292280, 2940F0, 2935C0, 2956D0, 294B80 | kind 4 second-frame loops | not native; not reached by the sweep |
| 2843F0, 2849F0, 284EE0, 285630, 285CF0, 286460, 28B870, 28C2C0, 28F460, 290030, 290B50, 291740 | second-frame loops of modes 1/2 and kind 0 | none executes in the censuses or the differentials |
| 285370, 284360, 28FBA0 | frame loops nothing executes | not native |
| 2CEBE0, 2CEC20, 297550, 296E90, 297030, 297180 | track decoders nothing executes | not native |
| 1DBBD0 child shape (0x1DBE23) | a non-root record's child shape transform (shape vtable +34) | not native; no census executes it |
| 2EB0C0 null handle (0x1DC999) | a rigid body at 0xFFFFFFF4 (the original faults) | refused |
| B3AE0, B3B70 | network position/basis prediction (B3A50, vtable +88) | not native; never executed in the Armory or on Coagulation |
| 1D63C0 | channel 2 post-process | not native; never executed |
| 3334F0 | the CRT fmod error methods (37DC0A of an infinite/NaN time or a zero frame count; 283B50's NaN pitch rows) | refused |
| 32B199, 32B0C6 (CRT floor/ceil from 283B50) | a NaN pitch row: the CRT reports it through 32EBC3/32EA95/32EAE8 after it has set the x87 control word | refused natively; the fixtures keep aim pitch rows numeric |
| 283D70 without an animation | 1CF4B0 on a channel with no animation (the original faults) | refused |
| 126200 | animation resource admission | not native; never executed |

BA7E0/BA630 are bound: BFF20's connected path calls the placement track's
`object_disconnect_from_map` and `object_reconnect_to_map` (proven by
`nodebuild_bounds_connected`).

BF0C0 also refuses inputs whose original behaviour reads uninitialized
stack: a node block of more than 256 nodes, or a model or physics node
beyond the zeroed node mask.

## Production (BE850's BE892 -> BF0C0 and BE941 -> BFF20)

Both call sites are re-emitted as required boundaries
(`s.required(0xbe892u,0xbf0c0u,false)` / `(0xbe941u,0xbff20u)` in
src/physics/physics_live_support_tu15_generated.cpp; the generator rule in
tools/generate_physics_live_support_tu15.py lists them for later full
regenerations). `PhysicsLiveWorldTu15::Impl::required_entry_original`
routes them to `node_rebuild_call`, which runs
`object_update_node_matrices` / `object_update_bounds` over the live bank
with the call's live x87 control word and MXCSR, binds the animation
resource reader, sets AL for BFF20 and returns with RET 4.

- `HALO2_REFERENCE_NODE_REBUILD_TU15=1` keeps the translated route
  (comparison only; the native is the default since the continuation
  below).
- `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1` runs the native first under a
  write journal, rolls it back, runs the translated original and compares
  every byte either wrote below the dead stack, plus AL and ESP; the
  original's effects are kept. `[node-rebuild ...]` lines report
  refusals, differences and totals.

### Live differential and status (2026-11-14)

(Superseded by the continuation below: the native is now the default.)
At this point the default stayed translated: the switch condition (every
executed path of the Armory and a multiplayer map native) held for the
Armory only.

| Run (binary with every owner above) | Calls compared | Matched | Differed | Refused |
|---|---|---|---|---|
| Armory 01a, campaign, 480 frames | 7,473 | 7,473 | 0 | 0 |
| Coagulation, 4 players, 600 frames (before BF9A0/296700) | 45,742 | 45,742 | 0 | 250 at ticks 24-25: 126 x 296700, 124 x BF9A0 |
| Coagulation, 4 players, 600 frames | 45,868 | 45,868 | 0 | 124 at tick 25: BCB70 -> 10DE40 -> C9070 (unit function value callback, track attachments) |

With `HALO2_NATIVE_NODE_REBUILD_TU15=1` the Armory runs natively (5,852
native calls in 300 frames, no refusal):

- Frame parity (deterministic `--campaign-reference-capture`, Original,
  300 frames, Vulkan validation on): translated and native frames are
  identical (two runs each), as are their traces; no validation messages.
- Census (300 frames, instruction callbacks): 220,980,065 translated
  instructions with the translated route, 204,561,736 native (-16.4M,
  -7.4%). Translated entries: BF0C0 2,273 -> 295, BFF20 4,169 -> 295 (both
  only through BF050 births now), BFB90 2,143 -> 752 (BF050 and the render
  path BF640), 1DBBD0 1,589 -> 87, 1DC7A0 612 -> 0, 125FB0 26 -> 22.
- Timing: four interleaved ordinary Armory runs at 1920x1018 under a host
  load of 28-32 (other agents' builds and oracles) measured 10.26/11.56
  fps translated and 10.56/10.25 native: below the noise. The sampled
  production profile puts BE850's subtree at 3.7% of main-thread samples.

### Every node rebuild call site, native by default (2026-11-14, continuation)

The function-overlay refusal (C9070) is closed by the attachments track's
type callbacks (C9070, E0900, F9270, 1046C0; see
[attachments.md](attachments.md#type-level-callbacks-2026-11-14)), and the
codec decoders it then reached (296540, with 2962F0 and 296620) are native
here. Every other BF050 call site and BF640's BFB90 are bound as well:

- **BF050** (RET 4): B959C, B9757, BA295, 10F8B6, BB808, ECB2B, ED006,
  EE80E, F0BCA and 1DC29F (re-emitted as `s.required(site,0xbf050u,false)`
  in src/physics/physics_live_support_tu15_generated.cpp), 11DD5A (the same
  in src/campaign/ai/source_ai_frame_tu15_generated.cpp) and the device
  constructor's 10BDC7 and 118096 (already required boundaries). BF050's
  own child recursion (BF0AA) stays inside the native. The generator rule
  in tools/generate_physics_live_support_tu15.py lists every site for later
  regenerations (the AI frame generator shares it).
- **BFB90** (EAX count, ECX mask, EDX object; stack mode, manager,
  orientations; RET 0xC) at BF640's BF7E5, which the render presentation
  and the native model builder's deferred-node service (BF640 through
  `call_actual`) both reach.

`PhysicsLiveWorldTu15::Impl::node_rebuild_site` classifies the sites
(tick: BE892/BE941; birth: the rest) and `node_rebuild_call` runs the
native owner (`object_rebuild_nodes`, `object_sample_animation`). The
native is the default for both groups. `HALO2_REFERENCE_NODE_REBUILD_TU15=1`
keeps every site translated, `HALO2_REFERENCE_NODE_BIRTH_TU15=1` only the
birth group, and `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1` compares all of
them (`[node-rebuild site]` lines count the calls per site).

| Differential (binary with every owner above) | Compared = matched | Differed | Refused | Per site |
|---|---|---|---|---|
| Coagulation 4p, 600 frames (C9070 native, decoders not yet) | 41,598 | 0 | 122 (296540) | tick sites only |
| Coagulation 4p, 600 frames | 70,802 | 0 | 0 | BE892/BE941 21,652 each, B959C 20,538, BF7E5 6,768, BA295 101, B9757 73, BB808 14, 10F8B6 4 |
| Coagulation 1p, 600 frames | 35,502 | 0 | 0 | |
| Containment 4p, 600 frames | 82,667 | 0 | 0 | B959C 29,649, BE892 22,688, BE941 22,880, BF7E5 7,216, BA295 118, B9757 90, BB808 14, 10BDC7/118096/10F8B6 4 each |
| Armory 01a, 480 frames | 8,721 | 0 | 0 | |

Receipts (reports, differential summaries, mutation lists, timing): `build/test-results/nodebuild-attachments-20261115/`.

Default-flip evidence (same binary, translated route
`HALO2_REFERENCE_NODE_REBUILD_TU15=1` against the native at every site):

- Armory frame parity (deterministic `--campaign-reference-capture`,
  Original, 300 frames, 1280x720, Vulkan validation on): translated and
  native (twice) give frame `5627a51badf47762` and trace `8a9b58b38bef`,
  the established reference; no validation messages; 6,737 native calls,
  no refusal.
- Multiplayer frame parity (`tools/perf_4k60/parity.py`, Coagulation
  3840x2160, cases 4:90:0:0, 4:150:10:110, 1:150:10:110, scripted input
  and fixed seed): PASSED, every candidate capture identical to its
  reference (and to the reference repeat).
- Census (Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`): 136,190,092
  translated instructions with the translated route, 121,194,543 with the
  native (-15.0M, -11.0%).
  Translated entries: BF0C0 2,273 -> 0, BFF20 4,169 -> 0, BF050 295 -> 0,
  BFB90 2,143 -> 0, BF9A0 2,143 -> 0, 1DBBD0 1,589 -> 0, 1DC7A0 612 -> 0,
  283A90 2,027 -> 8, 125FB0 26 -> 1; BF640 stays at 590.
- The default binary itself (no switch): Armory 300 frames, 6,737 native
  calls, frame `5627a51badf47762` / trace `8a9b58b38bef` (the reference
  switch on the same binary gives the same frame with no native call);
  Coagulation 4p, 600 frames, 72,655 native calls, no refusal.
- Timing (ordinary Armory, 1280x720, three interleaved pairs, host load
  13-17 from other agents): 14.93/14.39/15.91 fps translated, 16.11/15.05/
  15.28 native (means 15.08 and 15.48): within the noise.

### Map sweep with scripted input and the default (2026-11-15)

The 2026-11-14 differentials used idle input. The 2026-11-15 sweep ran every
multiplayer map of the selected cache at 1 and 4 players, and the campaign
levels 01a, 01b, 03a, 03b, 05a and 08b as far as they run today.

- Multiplayer runs: 600 ticks with the deterministic tick cadence. The
  scripted input fires, throws grenades, jumps, crouches, holds use to
  board vehicles, turns and moves.
- Campaign runs: 600 frames of the deterministic
  `--campaign-reference-capture` recipe.
- Every run used `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1`, so each call
  runs native, is rolled back and compared with the translated original.
- Receipts: `build/test-results/nodebuild-sweep-20261115/` holds one JSON
  and the `[node-rebuild ...]` lines per run, plus `sweep1-table.md`,
  `sweep2-table.md` and `sweep3-table.md`.

| Sweep | Binary | Result |
|---|---|---|
| 1 | the 2026-11-14 native default | Every MP map refuses 283E10 and 284090. 296890 refuses on Cyclotron, Gemini, Triplicate and Waterworks. C3750 refuses on Ascension 4p, Elongation 4p, Gemini, Lockout, Turf 4p and Zanzibar 1p; 1692E0 on Beavercreek 4p and Dune 4p. **Turf 4p differs in 283 of 34,475 calls**: the MCC StringID namespace ([attachments.md](attachments.md#the-mcc-stringid-namespace-2026-11-15)). The coordinator made native opt-in. |
| 2 | 283E10, kind 4, 284090, 296890/2969E0, C3750, 1692E0 and the namespace join | 0 differences in 56 runs. 145070 refuses in 8 runs, reached through C3750's reconnected owned lights. |
| 3 | plus 145070 | **1,594,436 calls compared, 0 differed, 0 refused** in all 56 runs. |

The runs that stop early do so for reasons outside node rebuild. Every
node-rebuild call they made before the stop matched.

- Needle and Street Sweeper: "object authored shader requires unresolved
  source inputs: active_camouflage".
- 01b: "Native memory pressure requires actual 1CD130 trim owner".
- 03a, 03b and 05a: "Scene-copy resources belong to another renderer
  thread".
- 08b: "Required original runtime backing unavailable at 0x48C33C (B9180)".

The native is the default again (`node_rebuild_mode`). The opt-outs are
unchanged: `HALO2_REFERENCE_NODE_REBUILD_TU15=1` keeps every site
translated, and `HALO2_REFERENCE_NODE_BIRTH_TU15=1` keeps the birth group
translated.

Default-flip evidence (one binary, `HALO2_REFERENCE_NODE_REBUILD_TU15=1`
against the default):

- **Armory frame parity.** The deterministic
  `--campaign-reference-capture` recipe (Original, 300 frames, 1280x720,
  Vulkan validation on) gives frame `5627a51badf47762` and trace
  `8a9b58b38bef` for the translated run and both native runs, with no
  validation messages.
- **Multiplayer frame parity** (`tools/perf_4k60/parity.py`, 3840x2160):
  - Coagulation, the default cases: PASSED.
  - Turf with `--motion`, cases 4:300:10:200 and 1:300:10:200: PASSED.
- **Census** (300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`):

  | Run | Translated instructions: translated route | Native default | Change |
  |---|---|---|---|
  | Armory | 111,967,010 | 96,970,544 | -15.0M, -13.4% |
  | Coagulation 4p | 311,230,851 | 224,491,746 | -86.7M, -27.9% |

  In the Armory, BF0C0, BFF20, BF050, BFB90, BF9A0 and 283E10 entries all
  drop to 0, and 283A90 falls from 2,027 to 8. BF640 stays at 590.
  On Coagulation, BF9A0, 283E10 and 283A90 keep 1,884 translated entries
  each, from a caller outside the bound sites; BF640 has 7,714.
- **Timing.** Ordinary Armory at 1904x1018, three interleaved pairs, host
  load about 3.5: translated 25.82/25.69/26.02 fps (mean 25.84), native
  26.43/26.27/26.28 (mean 26.33), +1.9%.
- **Receipts:** `build/test-results/nodebuild-events-20261115/`.

## BF640 deferred node matrices (2026-11-15)

BF640 is native: `object_deferred_node_matrices` (object, selected section
bytes, matrices) returns the matrices it used. It runs for the native model
builder's deferred-node service and the completed body publication.

| Owner | Native | What |
|---|---|---|
| BF640 | `object_deferred_node_matrices` | Returns early without orientations (+112), when frozen (+B3) or without a model. A deferred object (+4 bit 30) without +4 bit 29, and not a biped with +34B, resamples (BFB90 under the model's +A4 mask, bit 30 cleared) when a selected section byte (not 0xFF) is in the model's +C4 mask. A unit (row +3 types 0/1) with live aim or frame overlays copies its orientations to 0x4E8BC0 and its matrices to 0x4E57E8 and layers 1112E0 and 110C10 under a private copy of the +A4 mask. The masked child nodes then rebuild from their orientations (144CF0, 145560), and units take hand IK (1159D0). |
| 110240 | `unit_aim_overlay_live` | The aim block's +36 handle is set, and either its +40 blend is at least 1e-4 or its +2 state is 1..6. |
| 1114B0 | `unit_frame_overlay_live` | The +5C handle is set and one of the 13 weights +6E..+7A is nonzero. |
| 1112E0 | `unit_frame_overlays` | The +5C overlay starts on a private channel (1CE720, start flags 0x40). Each frame 2n + [aim +40] (n = 0..12) inside the animation with a nonzero +6E byte is sampled (1CE810, 1CF210) at FILD byte * [0x5564DC] (x87, live control word). A sampled overlay ORs its node flags (1E5DF0, 2130C0). |
| 110C10 | `unit_aim_overlays` | With +40 above 1e-4: +30 at frame 0 and weight +40, then +34's 12 rows (frame +44 byte, weight +50 byte * [0x5564D8] * +40). Rows at or past the animation end are skipped, and so are rows below 1e-4 by the x87 FABS/FCOMIP test (NaN passes). With +40 exactly 0 and state 1..6: +34 through the manager (1D3740) at frame 17, at weight state/3 (state < 3) or (7 - state)/4, when the animation has more than 17 frames (1CF630). Any other blend only requests +30/+34 (1E5C80). |
| 2130C0 | `animation_node_flags` | ORs a 1E5DF0 record's flag streams into the byte mask, three byte stores per mask byte: the animated block first, then the static one (static bytes and a nonzero +6). More than 32 bytes would overrun BF640's frame and is refused. |
| 115940, 150900, 172200 | (in 1159D0) | Hand IK is live when: +BC bit 11 is clear; the manager is animated; [0x474C04] is set; +148 bit 15 or +137 bit 0 is set; and a hand node (+1F6/+1F7) exists. The target is the first local player ([0x4F54F0] +C) camera row (0x4F655C + 0x358 n), closed by [0x474C24] == 0xFFFF. |
| 1159D0 | `unit_hand_ik` | Target: the unit's +1A4 (first person) or the camera. With the 0x4000095 marker (BAD60, one entry, through the placement track and the frame), each hand below `count` and in the mask turns (115500 on the marker's world matrix). |
| 115500 | `hand_ik` | Lazily publishes the limits pi/6 and pi/12 ([0x56E8E0]/[0x56E8DC], flags [0x56E8E4]). Computes the target in the basis of the node position and the marker's forward and up (144BF0, track deviceanim), normalizes it (RSQRTSS), and takes x87 FPATAN yaw and pitch, clamped to the limits. Then node = basis * rotation (1447E0, 145560), keeping the node's scale. |
| 1447E0 | `euler_matrix` | x87 FCOS/FSIN of each angle under the live control word. The ca*cb and -(sa*cb) terms are x87, the rest SSE. |
| BACD0 / FC510 | placement `object_marker_owner` | The item parent-mode path (FC510) is native: +154 names the owner. |

Literal 0x4000095: the translated 1159D0 pushes the TU1.5 literal unchanged
(`selected()` maps it to itself), so the native does the same. The MCC
StringID join is not applied here.

`Frame` nesting: placement's BAD60 calls this track's public `matrix_product`
on a `FrameMemory`. The fresh `Frame` this opened used to shadow the
enclosing frame's private extent, so the marker matrices read zeros. A
nested `Frame` now allocates above the enclosing frame's locals, and forwards
lower extent addresses to its memory. Top-level frames are unchanged.

Proof (strict writes, seed 0x5C41, the all-tracks harness):

| Kernel | Entry | Cases | Coverage |
|---|---|---|---|
| `nodebuild_deferred` | BF640 | 1,200 | BF640 268/268, C1E90 11/11, 110240 31/31, 1114B0 28/28, 1112E0 131/131, 110C10 277/277, 1159D0 84/84, 115940 45/45, 1447E0 74/74, 2130C0 97/97 |
| `nodebuild_frame_overlays`, `nodebuild_aim_overlays` | 1112E0, 110C10 | 500 + 800 | 1112E0, 110C10, 2130C0, 1CF630, 1CE5D0, 1D3740 complete |
| `nodebuild_unit_hand_ik`, `nodebuild_hand_ik` | 1159D0, 115500 | 600 + 500 | 1159D0, 115940, 150900, 172200, BACD0 44/44, FC510 15/15; 115500 143/157 (a); BAD60 112/118 (b) |
| `nodebuild_euler`, `nodebuild_node_flags` | 1447E0, 2130C0 | 400 + 300 | complete |
| probes: `nodebuild_hand_ik_camera`, `nodebuild_hand_ik_nan`, `nodebuild_aim_overlays_stepped`, `nodebuild_deferred_frames`, `nodebuild_aim_overlays_requests` | | 300 each | |

- (a) 115500's scaled-basis and zero-basis branches are unreachable: 144BF0
  always stores scale 1.0.
- (b) BAD60's name-0 return: 1159D0 always passes 0x4000095.

Results:

- Combined: `PASS 4300 physical composite cases, 189384 ordered original
  stores`, also under the ASan/UBSan harness with no report. The probes:
  `PASS 1500 physical composite cases`. They and the 16A4DD entry kernels
  also pass under ASan/UBSan: 3,200 cases, no report.
- Every nodebuild kernel at once, the old ones included (after the Frame
  nesting change and the native FC510): `PASS 20148 physical composite
  cases, 1298181 ordered original stores`.
  - BF640, BFB90, BFF20, BF050 and 283E10 are complete.
  - BF0C0 misses only the refused B3AE0 prediction tail.
  - 1DBBD0 misses only the refused child shape transform.
- Placement after FC510 (shared validator: `object_marker_owner`,
  `object_marker_matrices`, `model_marker_matrices`, `light_reconnect`,
  `object_reconnect_lights`, `object_disconnect_from_map`): `PASS 2900`.
- Mutation screen (out of tree, source SHA-256 9159fed2... checked
  afterwards): **70 of 70 rejected**. The randomized cohort rejects 62. The
  other 8 are rejected by the probes:
  - the 13th frame weight;
  - state 3 under a directed MXCSR rounding (3 * 1/3 differs from 1.0);
  - 17-frame animations;
  - the request order with resource-admitted animations;
  - +148 against +149;
  - the player scan;
  - the camera gate;
  - distinct NaN payloads in the IK basis.

Production binding (src/physics/physics_live_world_tu15.cpp,
`deferred_node_matrices`): the model builder's `deferred_nodes` and
`completed_body_world_nodes` run the native in the birth group's mode.

- `HALO2_REFERENCE_NODE_REBUILD_TU15=1` or `HALO2_REFERENCE_NODE_BIRTH_TU15=1`
  keeps the translated entry.
- `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1` compares every byte and the
  result.
- `[node-rebuild site] bf640` counts the calls.
- The translated 13A329 caller is dead: the native render path replaced it,
  and the census shows 0 entries.

| Differential (480 Armory frames / 600 ticks) | Compared | Differed | Refused | BF640 calls |
|---|---|---|---|---|
| Armory 01a | 9,671 | 0 | 0 | 950 |
| Coagulation 4p, scripted input | 53,154 | 0 | 0 | 9,273 |

Default-flip evidence (same binary, `HALO2_REFERENCE_NODE_REBUILD_TU15=1`
against the default):

- **Armory frame parity** (Original, 300 frames, 1280x720, Vulkan validation
  on): the translated run and two native runs give frame `5627a51badf47762`
  and trace `8a9b58b38bef`, with 0 validation messages.
- **Multiplayer parity** (`tools/perf_4k60/parity.py`, Coagulation 3840x2160,
  the default cases): PASSED.
- **Census** (Armory, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`):
  96,970,289 -> 95,237,698 translated instructions (-1.73M). The entries for
  BF640, 110240, 1114B0, 1112E0, 110C10, 1159D0 and 283A90 drop to 0.

## First-person sampler callees (16A4DD, 2026-11-15)

The Coagulation entries in BF9A0, 283E10 and 283A90 come from 16A4DD, the
first-person animation sampler: 1,820 entries per 300 frames, one per
player per tick. The 168DED first-person draw runs 168D55 -> 169C46 ->
16A479 -> 16A4DD, translated in src/gameplay/objects/source_sandbox_special_tu15_generated.cpp.
All of 16A4DD's callees are dispatched, so its calls reach the live world's
`required_entry_original`.

These sites are now node-rebuild birth sites, native by default:

| Site | Callee | Native |
|---|---|---|
| 16A6E5 | 1D5840 (manager, -, weight, mask, orientations, -, -; ESI count; RET 0x1C) | `animation_sample_channels` |
| 16A6FD | BF9A0 (object, -, manager, mask, count, orientations; RET 0x18) | `object_function_overlays` |
| 16A82A | 1CF3B0 (ESI channel; yaw, pitch, weight, mask, count, orientations; RET 0x18) | `animation_aim_channel` |
| 16A796, 16A7D5, 16A8FC, 16A986, 16AA19, 16AAA3, 16AAEC, 16AC26 | 1CF210 (EBX channel; weight, mask, count, orientations; RET 0x10) | `animation_sample_channel` |

Proof: the entry composites `nodebuild_sample_channels`,
`nodebuild_function_overlays` and `nodebuild_sample_channel` give
`PASS 1700 physical composite cases, 59837 ordered original stores`.

- Coverage: BF9A0 143/143 and 1CF210 137/137. 1D5840 is 93/97: the tail
  after the refused 1D63C0 post-process is missing, and no census executes
  it.
- The fixtures keep the manager valid (B87B0) and the channel valid (B8760),
  as 16A4DD tests them before each call.

16A4DD itself (587 instructions, with 16AD45, 169863 and 1D3380) stays
translated.

The Coagulation 4p differential with these sites gives 62,905 calls
compared, 0 differed and 0 refused. Per site: 1D5840/BF9A0 2,400 each,
1CF3B0 1,732, the 1CF210 sites 3,219.

### Map sweep with BF640 and the first-person sites (2026-11-15)

The sweep reused the recipe above: every multiplayer map at 1p and 4p, 600
ticks with scripted input and the deterministic tick cadence, plus campaign
01a, 01b, 03a, 03b, 05a and 08b at 600 frames. Every run used
`HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1`.

- **Result: 56 runs, 2,126,988 calls compared, 0 differed, 0 refused.**
- Calls per site: BF640 253,831; 16A6E5 (1D5840) and 16A6FD (BF9A0)
  66,345 each; 16A82A (1CF3B0) 50,677; the 1CF210 sites 95,354.
- The runs that stop early do so for the same reasons as sweep 3, all
  outside node rebuild: Needle, Street Sweeper, 01b, 03a, 03b, 05a and 08b.
- Receipts: `build/test-results/nodebuild-bf640-20261115/`, which holds
  `sweep-table.md`, the per-run JSON and `[node-rebuild]` lines, the
  composite and ASan reports, the mutation reports, the differentials and
  the parity reports.

Default evidence for the final binary (BF640 and the first-person sites;
`HALO2_REFERENCE_NODE_REBUILD_TU15=1` against the default):

- **Armory frame parity.** The translated run and two native runs give
  `5627a51badf47762` / `8a9b58b38bef`, with 0 validation messages.
- **Multiplayer parity** (`tools/perf_4k60/parity.py`, 3840x2160):
  - Coagulation, the default cases: PASSED.
  - Turf with `--motion`, cases 4:300:10:200 and 1:300:10:200: PASSED.
- **Census** (`HALO2_CENSUS_NATIVE_BRIDGES=1`), the previous default
  binary against this one:

  | Run | Previous default | This binary | Change |
  |---|---|---|---|
  | Armory, 300 frames | 96,970,289 | 95,238,814 | -1.73M |
  | Coagulation 4p, 450 ticks of the sweep's scripted input, `--mp-reference-tick-cadence` | 371,188,449 | 353,151,303 | -18.0M, -4.9% |

  On Coagulation these entries drop to 0: BF640 (7,758), BF9A0 and
  283E10 (1,800 each), 283A90 (4,096), 1D5840 (1,800), 1CF210 (4,144),
  1CF3B0 and 283B50 (1,312 each), and 110240/1114B0/1159D0 (4,374 each).
  Still translated on the first-person path: 16A4DD and 169C46 (1,800
  each), 1D3740 (2,624) and 1CE810 (11,862, all callers).
- **Timing.** Ordinary Armory at 1904x1018, three interleaved pairs, host
  load about 4: the previous default ran at 26.78/26.48/26.34 fps (mean
  26.53), this binary at 27.01/26.80/26.72 (mean 26.84). That is +1.2%,
  faster in every pair.

### Rare refusals closed: sweep, parity, census, timing (2026-11-15)

One binary (`build-perf-4k60/halo2_native`) carried the attachments
callbacks, placement's C2870/C39B0/C48E0 change, codec kind 1 and the
chained MCC names. No default changed: every new owner sits on a path that
is already native by default, and refused before.

- **Composite runner, every nodebuild kernel** (strict writes, seed 0x5C41,
  the all-tracks harness, kind 1 and the new placement natives included):
  `PASS 20748 physical composite cases, 1360843 ordered original stores`
  (BF640 run: 20,148 / 1,298,181). The most frequent remaining stops are
  the unported track decoders 297550 (381) and 2CEBE0 (260), the load
  wait 1236A0 (344), 284360 (254), 1DBBD0's child shape (249), the
  resource read 1A3B70 (200) and 1D63C0 (173).
- **Sweep** (the recipe above: every MP map at 1p/4p with scripted input,
  600 ticks, plus campaign 01a, 01b, 03a, 03b, 05a and 08b at 600 frames,
  `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1`): **56 runs, 2,126,988 calls
  compared, 0 differed, 0 refused**. Every run's count is identical to the
  BF640 sweep. The early stops are unchanged and outside node rebuild
  (Needle, Street Sweeper, 01b, 03a, 03b, 05a, 08b).
- **Armory frame parity** (Original, 300 frames, 1280x720, Vulkan validation
  on): the translated route and two native runs give `5627a51badf47762` /
  `8a9b58b38bef`, with 0 validation messages.
- **Multiplayer parity** (`tools/perf_4k60/parity.py`, Coagulation
  3840x2160, the default cases, `HALO2_REFERENCE_NODE_REBUILD_TU15=1` as
  the reference): PASSED, with the same hashes as the BF640 receipts.
- **Census** (`HALO2_CENSUS_NATIVE_BRIDGES=1`):

  | Run | Translated route | Native default |
  |---|---|---|
  | Armory, 300 frames | 111,969,897 | 95,238,404 (BF640 binary: 95,238,814) |
  | Coagulation 4p, 450 ticks, scripted input | 486,327,203 | 353,152,776 (BF640 binary: 353,151,303) |

  The default path's count is unchanged because none of the new owners runs
  through node rebuild in these runs. The census does show them executed
  translated by other callers:
  - Armory: 10BE20 1,502, C39B0 1,512, C2870 3;
  - Coagulation: 11E490 1,812.

  These come from the translated light and object-function paths outside
  the node-rebuild bindings. The proven natives are ready for those
  callers. Still translated on the first-person path (Coagulation): 16A4DD
  and 169C46 1,800 each, 1D3740 2,624, 1CE810 11,862.
- **Timing** (ordinary Armory at 1904x1018, three interleaved pairs, host
  load about 3): translated route 25.27/26.10/23.55 fps (mean 24.97),
  native default 26.15/26.62/27.68 (mean 26.82). The native is faster in
  every pair; the spread is noise-level.
- Receipts: `build/test-results/nodebuild-rare-20261115/` (`sweep/`,
  `sweep-table.md`, `differential/`, `census/`, `timing.log`, `oracle/`,
  `asan/`, `mutation/`).

## First-person sampler 16A4DD with 16AD45 and 169863 (2026-11-15, continuation)

16A4DD, the first-person animation sampler that 169C46 runs per local
player and weapon slot every tick, is native: `first_person_animation`
(user, slot). Its rebuild 16AD45 (`first_person_rebuild`) and the state
change 169863 (`first_person_state`) are native with it, and so is every
callee below them except three residual owners outside the object factory.

| Owner | Native | What |
|---|---|---|
| 16A4DD | `first_person_animation` | The view (`[0x4F6498]` + user * 0x20CC) and slot (+C + slot * 0x1010). With a weapon in the slot: rebuilds through 16AD45 when the slot's node counts (+F4, +F8) no longer match the first-person model (the weapon definition's +2A8/+2AC row for the variant, 11E060) and the character model (`[[0x4ECC1C]+13C]` row +4); stores the graph's node count (1D3FE0) at +300; fills the bank (`[0x4F6494]` + (slot + 2 user) * 0x1000) with the identity orientation; publishes both models' default orientations through the node maps +FC/+1FC. With a valid manager (B87B0): 1D5840, BF9A0, the fade restart (1E14F0: 0.2 s inactive for a +18 bit 4 animation, else 0.4 s active), and with a fade weight above 1e-4 (1E1620) the +98 channel, the +B8 channel at (+188 + 0.5) * weight, the aim overlay +D8 (1CF3B0 at +2040/+2054), the movement overlay +DC (frames 0/1, 3/2, 4/5, 7/6 by the signs of +2030, +2034, +2040, +2044, the last two only without an aim overlay, and frame 8 for +E4; shorter than 9 frames clears the handles); then the ammunition overlay +E0 (a magazine weapon, +292 == 2, in the reload states 0xC000064/0xB000065 eases the shown rounds +22C toward 105C10's rounds clamped to the magazine's +A, by (span - 0x2C) * 0.2 clamped to 1; any other weapon shows +22C clamped to the animation); the crossfade (1D4560) of a posed view. Then +0 bit 1, the root matrix (1441C0: unit axes at the definition's +308) and the node matrices (1D4000 -> 1E5470, or 1D4030 -> 1E5540 for the mirrored pair +2FC/+2FE); slot 0 copies its hand node (+2094) matrix to +2098 less the definition's +308 offset (+0 bit 2). |
| 16AD45 | `first_person_rebuild` | Detaches the slot (16B1BC, 16B1F5 with NONE), resets the hand matrix and the slot flags, finds the unit's weapon for the slot (CE230 by body +212 + slot), and with a first-person row: clears the +202C fade bytes, sets the manager up (1D3380, modelstate's `animation_manager_setup`), finds the hand node 0xE0000E0 (1D4090, slot 0), maps the character and first-person models to the graph's nodes by name (1D4060 -> 1E5770; slot 1 keeps only +A bit 5 nodes of the body map) and, for slot 1, the mirrored node pair (1E5740 by the +A bits 3/4 of the first node's parent). A complete slot (+0 bits 1 and 2) attaches the weapon (16B1F5), enters state 0x400000C (169863), resets +98/+B8 (1CE6D0) and the overlay handles, starts the +B8 overlay 0x12000068 (units' 1D37D0, now exported) and finds +DC/+E0/+D8 (units' 1D3C80: 0x8000061, 0xA000069, 0xE0000DE). The tail is 16B2B9 (+F0 = 30). |
| 169863 | `first_person_state` | The key exclusions (a repeated 0x5000024 or 0x1000006E, a repeated 0x6000006 of a +12C bit 17 weapon, 0x8000063/0x9000062 outside 0x400000C/0x600005F), the fade seconds by key (0.267, 0.1335 or 0), the fade clear (169B81) and restart (169B0A, which copies the orientations to the previous bank), then 1D5320 (weapon's `manager_set_state`, keys 0x7000101, the 169432 base key, 0x7000001, flags 0x82, start flags 0x3F or 0x103F/0x203F for two weapons) and, when entered with the stop flag, the sound stop. |
| 1E5470 / 1E5540 | `animation_node_hierarchy` | Breadth-first node matrices over the graph's node tree (+C count, +10 rows: +4 sibling, +6 child, +8 parent; the original's 255-entry stack queue, refused beyond it); 1E5540's mirrored node takes its source's matrix times the Y-mirrored local matrix (unit scale, forward and up y negated, left = forward x up, position y negated). |
| 105C10 / 1059C0 / 10B0F0 / 15C7F0 | (in 16A4DD) | A magazine's rounds: loaded +22C, plus +22A (or the definition's +2C4 row +C with shared ammunition: game mode 2, an owned item of a player the engine variant admits) and the owner's other weapons of the same tag. |
| 169432, CFB40, 16B1BC, 16B1F5, 16B2B9, 169B0A, 169B81, 1CE6D0, 1D4060, 1D4090, 1D4580, 1E5740, 1E5770, CE230, 11E060, 1441C0, 1D3FE0, 1D4000, 1D4030, 1D4560 | (in the owners above) | |

The residual owners (`FirstPersonResiduals`, bound by the live world for the
duration of a call; without a binding each refuses by its address):

- **17A570** (stdcall user, object, attach), the effects that follow the
  first-person weapon. Its scan of the effect datums (`[0x4F720C]`) stores
  nothing unless an effect follows the weapon; that scan is native
  (`fp_effects_pending`). Only a following effect runs the original 17A570
  (it then stores +58 and runs 17B1F0, or deletes flagged events).
- **171440** (DI 0, stack block), the resource prediction of the weapon
  definition's +2B8 block. An empty block stores nothing (native); a
  non-empty one runs the original.
- **128E70** (EBX handle), the sound stop of 169863's stop flag.

The live world runs each original through its own call site (16B223/16B237,
16B30A, 169A7D) on a frame below the sampler's dead locals, with the
native's current x87 status and MXCSR. They are part of the effect,
resource-prediction and sound subsystems and remain explicit migration work.

### Oracle

New composite kernels (the AI bank holds the view at 0x1300000, its bank at
0x1302400 and the fixture tables; `[0x4F6498]`/`[0x4F6494]` are offset so
the case's user and slot land there):

| Kernel | Entry | Cases | Notes |
|---|---|---|---|
| `nodebuild_first_person` | 16A4DD | 600 | 25% rebuilds; named node trees rooted at node 0; render models as first-person and character models with named records; leaves with state animations; key remaps |
| `nodebuild_first_person_ammunition` | 16A4DD | 600 | the reload-state ammunition overlay, owners with same-tag weapons, shared ammunition |
| `nodebuild_first_person_rebuild` | 16AD45 | 500 | out-of-range variants, missing units, weapons, rows and models |
| `nodebuild_first_person_state` | 169863 | 500 | every key branch, repeated keys |
| `nodebuild_first_person_state_ties` | 169863 | 300 | a tick length that makes the fade seconds equal 1E16E0's remaining time (169B0A's `>=`) |

The residual stops are 17A570's inner PCs 0x17A5FD (attach match) and
0x17A63F (detach match), 171440's 0x171459 (a non-empty block) and 128E70's
entry; 17B1F0 and 171440's prediction callees are cut. 1D5320's refused
physical-query paths (1E4C39, 1D55AA) are stops too. `composite()` takes a
`through` set: entries a kernel executes stay in the closure when another
selected kernel stops before them (1D3380, which placement's kernels stop
at); the other kernels' rows still stop there.

Fixture facts the original depends on (it faults otherwise): 16A4DD reads
the first-person row (11E060 returns 0 without rows), the variant's
character model and the manager's graph unchecked; 169863's 1D5320 needs a
graph wherever a weapon is held; the aim channel's yaw/pitch stay finite
(CRT floor error path); `[0x4F720C]` lies inside the deferred scene's camera
block 0x4F655C, and the engine variant table 0x56DFA0 is the base world's
absolute block (8 entries), so both are written there.

StringID literals: the translated 16A4DD, 16AD45 and 169863 compare their
keys as raw TU1.5 literals (none of their sites is in
`source_object_function_symbols_tu15.inc`), and the natives do the same.
Their keys are code literals (169863's 0xB0005B1, 0xC0005AE and
0xE000607..0xE00060A among them), not cache names; the names they compare
against cache data (1D4090's 0xE0000E0, 1E5770's model and graph node
names, the overlay names 0x12000068, 0x8000061, 0xA000069, 0xE0000DE) are
below the first MCC divergence or come from the cache on both sides.

### Proof

- **Composite runner** (strict writes, seed 0x5C41, the all-tracks
  harness): `PASS 2500 physical composite cases, 514280 ordered original stores` over the five kernels; outcomes: 2,053 returned, the rest at declared boundaries (the residual stops 17A570 39, 171440 48, 128E70 3; the codec and resource stops of the sampled channels).
  - Coverage: 16A4DD 587/587, 16AD45 271/271, 1E5470 65/65, 1E5540
    133/133, 169B0A 39/39, 1059C0 170/170, 105C10, 10B0F0, 15C7F0, 1E5770,
    1E5740, 1D4090, 169432, CFB40, 16B1BC, 16B1F5, 16B2B9, 169B81, 1CE6D0,
    CE230, 11E060, 1441C0, 1D3FE0, 1D4000, 1D4030, 1D4560, 1D4060, 1D4580
    and 1E16E0 complete; 169863 178/180 (the rest follows the 128E70
    residual stop); 17A570 92/152 (the rest follows its two residual
    stops).
- **Every nodebuild kernel at once** (the first-person kernels included,
  their `through` entry and residual stops in the union):
  `PASS 23248 physical composite cases, 1873887 ordered original stores`
  (previous: 20,748 / 1,360,843).
- **ASan/UBSan** (`-DHALO2_SANITIZERS=ON` harness, `halt_on_error=1` for
  both): `PASS 2500 physical composite cases, 514280 ordered original stores`, no report.
- **Mutation screen** (out of tree, in-tree SHA-256 6d2494fe... checked
  afterwards): 51 mutants. The first round rejected 43; five survivors were
  closed by fixture strengthening (8-frame movement animations, the
  16AD45 overlay names in the mode leaves, the new
  `nodebuild_first_person_state_ties` probe whose tick length makes the
  fade seconds equal 1E16E0's remaining time, the team bit with shared
  ammunition, +2 bit 6 against bit 7 of following effects) and are
  rejected in the second round. Three are equivalent:
  - the ammunition overlay's `shown <= last` against `<` (assigning an
    equal frame changes nothing);
  - the fade weight's `> 1e-4` against `>=`: 1E1620 never returns exactly
    1e-4f. Every output over the reachable fade bytes (position, length,
    curve) under all sixteen MXCSR rounding/FTZ/DAZ modes, 7,311,360 of
    them, was enumerated (`mutation/fade_threshold_enumeration.cpp` in the
    receipts);
  - 1059C0's own-only flag: 16A4DD's only call passes 0.

### Production and live evidence

169C46's 16A479 call is re-emitted as a required boundary
(`tools/regenerate_first_person_sampler_boundary_tu15.py`; a full
regeneration of the sandbox special unit must run it again).
`node_rebuild_site` puts 16A479 in the birth group, so the native is the
default; `HALO2_REFERENCE_NODE_REBUILD_TU15=1` or
`HALO2_REFERENCE_NODE_BIRTH_TU15=1` keeps the translated 16A4DD, and
`HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1` compares it. Two binding details:

- In native mode the binding publishes the call's inputs to the private
  first-person context's composition observer
  (`SourceFirstPersonCompositionObserverTu15::entry`), as the translated
  16A4DD does at its entry; without it the first native tick fails with
  "Native FP active hand lost its original composition input boundary".
- The differential now treats the callee's popped return address and
  arguments as dead once it has returned: the original 16A4DD reuses its
  user argument slot for the shown rounds (`mov word [ebp+8]`), which no
  caller reads after RET 8.

| Evidence | Result |
|---|---|
| Differential, Coagulation 4p / 1p (600 ticks, scripted input) | 65,305 / 35,407 calls compared, 0 differed, 0 refused; 16A479 2,400 / 600 |
| Differential sweep (every MP map 1p/4p with scripted input, 600 ticks; campaign 01a, 01b, 03a, 03b, 05a, 08b at 600 frames) | **56 runs, 2,193,333 calls compared, 0 differed, 0 refused**; 16A479 (whole 16A4DD) 66,345 calls. The early stops are the earlier sweeps' (Needle, Street Sweeper, 01b, 03a, 03b, 05a, 08b), all outside node rebuild |
| Native default, Coagulation 4p / 1p (600 ticks) | 53,142 / 32,061 native calls, no refusal |
| MP frame parity (`tools/perf_4k60/parity.py`, 3840x2160, translated route as the reference) | Coagulation, the default cases 4:90:0:0, 4:150:10:110 and 1:150:10:110: PASSED, with the BF640 and rare-refusal receipts' hashes (9c14a330..., 13fa114e..., 05f31264...). Turf with `--motion`, 4:300:10:200 and 1:300:10:200: PASSED |
| Armory frame parity (Original, 300 frames, 1280x720, Vulkan validation on) | the translated route and two native runs give `5627a51badf47762` / `8a9b58b38bef`, 0 validation messages |

Census (`HALO2_CENSUS_NATIVE_BRIDGES=1`, Coagulation 4p, 450 ticks of the
sweep's scripted input, `--mp-reference-tick-cadence`): the previous default
binary 353,147,717 translated instructions, this binary 339,418,422
(-13.73M, -3.9%). Every change is in the first-person closure: 145560
-5.22M (1E5470's products), 16A4DD -3.77M, 1E5470 -2.85M, 1E1720 -0.65M
(1D4560's crossfade), 1E5C80 -0.24M, 1E3150 -0.18M, 1E5AF0 -0.12M, 1CE720
-0.10M, 1D36C0 -0.09M, 1CEC60, 1CE810, 1E1620, 1E14F0, 1CE5D0, B8780,
1D3740, 1441C0, 110FC0, 11E060, 1D4000, B87B0, 1D3FE0, B8760 and 1D4560.
Entries that drop to 0: 16A4DD (1,800), 1E5470 (1,800), 1D3740 (2,624),
1CE5D0, 1441C0, 1D4000, 1D3FE0, 1D4560. The Armory runs no first-person
weapon in its 300 frames (no 16A479 call); its census is 95,243,927 with
the native default and 111,967,353 with the translated route.

Still translated on the first-person path: 169C46 (1,800 entries, 724k
own instructions), 16AD45 from its other callers (12: 168CA1's 168D40,
155AB0, 1693ED, 16B16A, 16B736), 169863 from 16961F, 169C46's 16A469 and
16B736 (368), 1CE810 9,996 and 145560 12.3M from other callers.

Timing: ordinary Armory at 1904x1018, three interleaved pairs, host load 3.9-5.4 (this session's oracle and suite runs): translated route (`HALO2_REFERENCE_NODE_REBUILD_TU15=1`, every node-rebuild site) 26.38/27.47/26.91 fps (mean 26.92), native default 27.63/28.29/28.34 (mean 28.09), faster in every pair. The Armory reaches no 16A479 call, so this pair measures the node-rebuild natives as a whole; 16A4DD's own saving is the multiplayer census above (-3.9% of Coagulation 4p's translated instructions).

Receipts: `build/test-results/nodebuild-first-person-20261115/`
(`sweep/`, `sweep-table.md`, `differential/`, `oracle/`, `asan/`,
`mutation/`, `census/`, `parity/`, `timing.log`).

## First-person update 169C46 and the sampler's callers (2026-11-15, third continuation)

169C46, the first-person update that 168D55 runs per local user every tick,
is native: `first_person_update` (user). It calls the native sampler
(`first_person_animation`) itself at its 16A479 boundary. It is proven
against the original and compared live. It was opt-in until the Elongation
divergence in the shared 1D5320 owner was fixed (see
[Production](#production-169c46-and-the-callers)). It is **native by default
since 2026-11-16** (see
[MCC first-person query names](#mcc-first-person-query-names-and-the-169c46169863-default-2026-11-16)).

The code follows the native code standard
([REMAINING_TRANSLATED_20261115.md](../native_replacement/REMAINING_TRANSLATED_20261115.md#code-standard-user-2026-11-15)):
named layouts (`GameTime`, `UserInput`, `AnimationChannel`,
`AnimationManager`, `AnimationDefinition`, `FirstPersonView`,
`FirstPersonSlot`, `FirstPersonUnit`) replace raw offsets, the animation
event callback is a typed `EventCallback`/`AnimationEvent` instead of the
original's 169C21/169C33 code addresses and stack record, the 16961F state
table is a `switch`, and the systems are split into `fp_sway`, `fp_motion`,
`fp_overlay` and `fp_fidget`. The x87 steps are small commented helpers
(`x87_spring_*`, `x87_vector_length`, `x87_overlay_seconds`,
`x87_fmod_frame`, `x87_random_*`). The live world's adapter
(`node_rebuild_native`) holds the ABI: stack arguments, the residuals'
registers and call sites.

| Owner | Native | What |
|---|---|---|
| 169C46 | `first_person_update` (`fp_sway`, `fp_motion`, `fp_overlay`, `fp_fidget`) | The aim springs (+2030/+2034 values, +2038/+203C rates) toward the unit's aim velocity (+1B0/+1B4) at 72/15 (or the weapon definition's nonzero +2E0/+2E4), the movement springs (+2040/+2044, +2048/+204C) toward the last angle changes per second clamped to [-1, 1] (the pitch negated) at 27/6 (+2E8/+2EC); then the input angle changes (C1F90 against +2050/+2054, or [0x474D24]'s zero vector when the view was not posed), the new angles and +0 bit 0. Per slot: the +F2 countdown, visibility (16B1BC: a first-person camera, 158290, and a valid manager, B87B0), and with a weapon: the manager advance with animation events (1D39C0, callback 169C21/169C33), the state end (16961F when channel 0 ends, +19 bits 1/3), the motion channel +98 (rate from the aim velocity's length, 24570, above 0.1: [0.5, 1], or 1.0 airborne: CAEF0's +348 bit 4 with 1D4510's 0x60000D8 overlay; nothing while flying, D28F0/E65A0; 1D3860; restart and 0x60000D8/0x6000060 start, 1D37D0, with the 169B0A fade), the overlay channel +B8 in state 0xB000067 (0x12000068 restarted at (rate + 1) * dt * 2 + frame / 30 modulo its length, 37DC0A, times 30), the settle spring +E4/+E8 (9/6), the manager fades (1D5790 -> 1E15D0), the idle fidget (all springs and inputs at rest in 0x400000C: a 259B0/1494F0 wait, then 0x600005F unless the seed draw is below [[0x4ECC1C]+134]+9C), the sampler (16A4DD) for a visible slot, and the +F0 prediction countdown (16B2B9 unless [0x51DF50]+5). Then +0 bit 0 clears without a visible slot, and the view fade +202C ticks. |
| 121890 | `fp_spring` | x87 distance test, FSQRT speed and FST rate (the wrapped form, DL set, is unreached from 169C46 and refuses) |
| 1D39C0, 1D3860, 1D3930 | `fp_manager_advance`, `fp_manager_channel_tick`, `fp_manager_blend_flags` | Ticks the three channels while the manager has a graph; a finished transition channel (+40, +51 bits 1/3) interpolates over 0.267 s (1D38A0) unless its animation's +18 bit 1, resets, clears channel 0's +11 bit 0; 1D3930; under [0x47C1B4] the blend channel follows channel 0's fraction |
| 1CEB70, 1CE950 | `fp_channel_tick`, `fp_channel_advance` | The delay byte, dt * rate * 30 frames, event reporting, the loop wrap (+2E, 1D3660 random variant, +11 bit 2) or the end (+11 bit 3), 1CE810 |
| 1CF700, 1CEE60 | `fp_animation_events`, `fp_channel_graph` | Frame (+4C), sound (+54 over the graph's +18) and effect (+5C over +20) events between two frames, with the 0x14-byte callback record and the row flag gates |
| 169BA2 | `fp_report_event` | A sound event plays on the weapon (18BE50, residual) and the slot keeps the handle (+1008) and the state key word (+100C) |
| 16961F | `fp_state_ended` | The state key's follow-up (0x400000C for most; 0xA000066, 0xB0005B1, 0xB0005B2, 0x11000074/6, 0x140005B3, 0x8000071 after F0E00; 0x8000025 marks +74 bit 0) through 169863 without the sound stop |
| D28F0/E65A0, C1F90, 158290, 1E15D0 | `fp_unit_flying`, `fp_angle_change`, `fp_director_kind`, `fp_fade_tick` | |
| 24570, 259B0, 1494F0, CAEF0, 1D4510, 1D5790, 18A210, 18A830, 13F620 | (in the owners above) | |

The residual owners now include 18BE50 (EBX sound, EAX [0x474D08], ECX
[0x474CE8]; object, -1, 1.0), run by the live world through 169C02 (the
weapon effect dispatch's native `direct_sound`). `sampler_entry` is not a
residual: the binding publishes each native sampler call's inputs to the
first-person composition observer, as the translated 16A4DD does at its
entry (169C46's 16A47E return on a frame below the caller). Unbound, the
18BE50 refusal names the original's EBX (sound) and ESI (slot) at 169C02,
and the composite runner compares them with the original's registers at
the stop.

### Oracle and proof

Kernel `nodebuild_first_person_update` (entry 169C46, stack user; 1,500
cases). The fixture (`first_person_update_case`) extends `fp_scene`: the
other slot holds no weapon; the inputs ([0x4F9B54] rows), the idle globals
([[0x4ECC1C]+134]), the graphs' sound and effect references (+18/+20) and
every animation's event tables live in the AI bank's free tail; [0x5564C4]
(turn, half turn), [0x474D24] (a zero vector), [0x474D08]/[0x474CE8] and
[0x47C1B4] are their own banks; the directors (first person mostly), the
unit's flight fields, the manager's channels on its own graph, the mode
leaves with the motion overlays (0x60000D8/0x6000060) and 16961F's follow-up
states, and a 10% variant whose manager has no graph. 1CF700's indirect
callback targets 169C21/169C33 are declared (`INDIRECT_TARGETS`). Fixture
limits the original imposes (it faults or leaves the closure otherwise): a
blend channel needs a primary animation; the game time stays finite (BF9A0
and the +B8 overlay pass it to 37DC0A, whose NaN/infinite classes jump
through 331ECB's table); in state 0xB000067 the weapon's +188 stays finite.

- **Composite runner** (strict writes, seed 0x5C41, the all-tracks
  harness, the final fixture): `PASS 1500 physical composite cases, 326413
  ordered original stores`; the rest of the cases stop at declared
  boundaries (18BE50 44 with matching EBX/ESI, 171440 67, 17A570 3, 128E70
  2, and the sampler's codec and resource stops).
- **Coverage:** 169C46 577/578, 1D39C0, 1CEB70, 1CE950, 1CF700, 1CEE60,
  1D3930, 16961F, 1D3860, 1D5790, 1E15D0, E65A0, D28F0, CAEF0, C1F90, 1494F0
  and 1D4510 complete; 169BA2 35/38 (after the 18BE50 residual); 121890
  111/139 (its wrapped form, unreached from 169C46 and refused natively);
  259B0 17/18. 16A41A and 259CD (a negative FILD of seed >> 16) are
  unreachable.
- **The legibility rewrite** gives the same comparison hash
  (f3e734c0...) over the same 1,500 cases as the first form (322,477
  stores, before the last fixture variant).
- **ASan/UBSan** (`-DHALO2_SANITIZERS=ON` harness, `halt_on_error=1`):
  `PASS 1500 physical composite cases, 322477 ordered original stores`, no
  report.
- **Mutation screen** (out of tree; the in-tree SHA-256 is checked
  afterwards): round 1, 68 mutants on the first form: 57 rejected. The
  fixture and the 18BE50 refusal detail then closed the survivors; round 2,
  37 mutants on the rewritten source (the ten closable survivors and a
  cross-section of round 1): 35 rejected; round 3 (a 0xA000066 fixture
  variant) rejected the 16961F F0E00 survivor. Two are equivalent: C1F90's
  `>=` against `>` at exactly the half turn (the original maps pi to -pi
  and back to pi, the value the mutant keeps, while the turn is twice the
  half turn), and the +100C key word read from +80 (a store after the
  18BE50 residual: the oracle stops before it; the live differential
  compares it). In-tree SHA-256 after the screens: 1726127a....

### Production: 169C46 and the callers

`tools/regenerate_first_person_sampler_boundary_tu15.py` now re-emits
168D55 -> 169C46 too, and `tools/generate_physics_live_support_tu15.py`
lists the live-support sites 16B1AD, 16B78B, 16940B (16AD45) and 16B7F5
(169863) as required boundaries (the generated unit carries the same four
emissions). `node_rebuild_site` binds 168D55 -> 169C46, the 16AD45 sites
168D40, 16B1AD, 16B78B and 16940B, and 16B7F5 -> 169863. The translated
reference of 16AD45/169863 is their own route (the required entry without
the interception).

- **Default:** the 16AD45 sites (`first_person_rebuild`).
- **Opt-in until 2026-11-16** (`HALO2_NATIVE_FIRST_PERSON_UPDATE_TU15=1`):
  168D55 -> 169C46 and 16B7F5 -> 169863. They are native by default since
  then; `HALO2_REFERENCE_FIRST_PERSON_UPDATE_TU15=1` keeps their translated
  routes. On Elongation 4p (scripted input) the weapon
  track's 1D5320 (`manager_set_state` -> 1D46A0 `choose_animation`) finds
  no animation for the TU1.5 state keys 0xC0005AE and 0x140005B0 (from
  169493, entered at 16B7F5) and their follow-up 0xB0005B1 (169C46's
  16961F) on the first-person graph eff750ea, with mode keys 0x6000086 /
  0x30000D9 / 0x30000D9, and falls back to 0x400000C; the translated
  1D5320 finds animation 11. Reproduce:
  `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1 HALO2_NODE_REBUILD_DIFF_TRACE=1`
  on `mp:elongation:4` (six differing calls, all user 2). Neither
  translated owner applies an MCC StringID join (none of their sites is in
  `source_object_function_symbols_tu15.inc`), so the cause is inside the
  native 1D46A0 path on this graph's data, not yet found.
  **Resolved 2026-11-16** (see
  [MCC first-person query names](#mcc-first-person-query-names-and-the-169c46169863-default-2026-11-16)).
  The translated 1D5320 does join these keys. It does so through the
  weapon animation query scopes
  (`SourceWeaponAnimationQueryScopeTu15`, emitted at 1D5320's entry),
  not through the object-function `.inc`. The keys are the staged reload
  (`reload_enter`, `reload_continue_full`, `reload_exit`), which the MCC
  cache names 0x0C0005BA, 0x140005BC and 0x0B0005BD.

| Evidence (final binary) | Result |
|---|---|
| Differential sweep (every MP map 1p/4p, scripted input, 600 ticks; campaign 01a, 01b, 03a, 03b, 05a, 08b at 600 frames; `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1`) | **56 runs, 2,271,055 calls compared, 6 differed, 0 refused.** All six are Elongation 4p: three 169863 calls at 16B7F5 and three 169C46 calls (the divergence above; both opt-in). 168D55 (169C46) 67,529 calls, 16B7F5 9,691, the default 16AD45 sites 168D40 128, 16B1AD 290, 16B78B 84 (0 differences); 16940B is not reached. The early stops are the earlier sweeps' (Needle, Street Sweeper, 01b, 03a, 03b, 05a, 08b) |
| Native runs, Coagulation 4p (450 ticks): default / opt-in | 42,101 / 42,457 native calls, no refusal |
| MP frame parity (`tools/perf_4k60/parity.py`, Coagulation 3840x2160, translated route as the reference) | default and opt-in: PASSED, hashes 9c14a330..., 13fa114e..., 05f31264... (the earlier receipts') |
| Armory frame parity (Original, 300 frames, 1280x720, Vulkan validation on) | translated route and two native runs: `5627a51badf47762` / `8a9b58b38bef`, 0 validation messages |

Census (Coagulation 4p, 450 ticks of the sweep's scripted input,
`HALO2_CENSUS_NATIVE_BRIDGES=1`): the default binary 296,386,126 translated
instructions, the opt-in 293,942,337 (-2.44M, -0.8%): 169C46 (724k),
121890 (447k), D28F0, 16961F, 169863, 1D3860, CAEF0, C1F90, 18A210/18A830
and 169BA2 drop to 0, and 1D39C0's closure falls to its other callers. The
default change itself only removes 16AD45's 12 entries; the session-start
binary's 339,416,963 also predates concurrent work of other sessions in the
shared build tree.

Timing: not measured. The default change reaches 12 calls per Coagulation
census and none on the Armory (no first-person weapon there), and the
interleaved Armory A/B at 1904x1018 timed out (240 s) under a host load of
10-16 from concurrent sessions' builds and runs. The census delta above is
the measured cost.

Another session rebuilt the shared `build-perf-4k60` binary at 21:24 and
21:55, during the sweep and the parity and census runs; this track's sources
were unchanged throughout (nodebuild SHA-256 1726127a..., the live world
d76e3db2...), so every binary used carries the same native code.

Every nodebuild kernel at once (strict writes, seed 0x5C41, the all-tracks
harness): `PASS 24748 physical composite cases, 2185298 ordered original
stores` (previous: 23,248 / 1,873,887).

Receipts: `build/test-results/nodebuild-first-person-update-20261115/`
(`oracle/`, `asan/`, `mutation/`, `sweep/`, `sweep.log`, `differential/`
with the Elongation trace, `census/`, `parity/`, `timing.log`).

### Remaining on the first-person path

- ~~The 1D5320/1D46A0 divergence above, then 169C46 and 16B7F5 by
  default.~~ Done 2026-11-16 (next section).
- The private first-person context (`source_first_person_context_tu15.cpp`)
  still calls the translated 169C46 and 16A4DD on its own memory
  (`owner.call`); none of the sweep's runs executes it.
- The residual owners 17A570, 171440, 128E70 and 18BE50 (effects, resource
  prediction, sound) remain explicit migration work.

## MCC first-person query names and the 169C46/169863 default (2026-11-16)

### Root cause and fix

The Elongation divergence above came from a missing MCC StringID join. The
details are in
[weapon.md](weapon.md#mcc-first-person-query-names-2026-11-16).

User 2 ran a staged reload (`reload_enter` 0xC0005AE, `reload_continue_full`
0x140005B0, then 16961F's `reload_exit` 0xB0005B1). The MCC cache names
these states 0x0C0005BA, 0x140005BC and 0x0B0005BD.

The translated 1D5320 called from 169863 (returning to 169A56) runs inside a
`SourceWeaponAnimationQueryScopeTu15`. In that scope, 1E3420's
action-record search key (pushed at 1E34A3) and 2A0100's class-record
search key (pushed at 2A019A) use the cache's names. The native subtree
searched TU1.5's IDs, found nothing and retried 0x400000C.

The fix:

- **`weapon_animation`** gains `QueryNamespace`, `QueryOrigin` and
  `QueryScope`. The two searches apply the join under the translated
  route's conditions.
- **`first_person_state`** passes `QueryOrigin::first_person_state`.
- **`first_person_rebuild`** scopes its 1D37D0 and three 1D3C80 lookups as
  `first_person_channel`. These are the dual-class join of the translated
  16B0D1/16B0E5/16B108/16B128 wrappers.
- **The live binding** (`node_rebuild_native`) opens
  `QueryNamespace(cache_owner.is_mcc())` next to the object-function
  namespace.

The 16AD45 part matters beyond Elongation. 16AD45 has been native by
default since 2026-11-15, both inside 16A4DD and at its four translated
callers. With a dual-wielded weapon (169432's base 0x400054B), its overlay
channels searched the TU1.5 dual class. The translated route searches the
cache's dual class, so on an MCC cache the two could diverge. No sweep
dual-wields, so the differential never met it. The new oracle kernels do.

### Oracle and proof

- **Observer.** The observer applies the translated route's joins to the
  unchanged original for rows that carry `query_names` (see weapon.md). It
  reads the name table from the translated route's source.
- **New composite kernels.** These use the existing first-person kernels'
  fixtures, moved into the cache's namespace by `mcc_shape`:
  - class rows are mostly the cache's dual class, sometimes TU1.5's;
  - leaf states are mostly the cache's names, sometimes TU1.5's;
  - some leaves carry a pending resource, so a prefetch that finds them
    reaches the 126200 boundary.

  | Kernel | Entry | Cases |
  |---|---|---|
  | `nodebuild_first_person_state_mcc` | 169863 | 500 |
  | `nodebuild_first_person_rebuild_mcc` | 16AD45 | 500 |
  | `nodebuild_first_person_update_mcc` | 169C46 | 800 |

  In `nodebuild_first_person_update_mcc`, the manager often ends one of
  the states whose 16961F follow-up is a joined name.

| Run | Result |
|---|---|
| MCC kernels (strict writes, seed 0x5C41, the final fixture) | `PASS 1800 physical composite cases, 204448 ordered original stores`, 388 translated-route joins, 83 cases at the 126200 boundary. In these kernels' closures: 2A0100 101/101, 16AD45 271/271, 169863 176/180 |
| Every nodebuild kernel at once (strict writes, seed 0x5C41, the all-tracks harness) | `PASS 26548 physical composite cases, 2380145 ordered original stores`, 389 joins (previous: 24,748 / 2,185,298) |
| ASan/UBSan (`-DHALO2_SANITIZERS=ON` harness, `halt_on_error=1`; the nine first-person kernels, MCC included, first fixture) | `PASS 5800 physical composite cases, 1045944 ordered original stores`, 434 joins, no report |
| Weapon track and its dependants | see weapon.md |
| Mutation screen | see weapon.md |

### Differential and sweep

The differential ran on the fixed binary, a frozen copy of
`build-perf-4k60/halo2_native`, with `HALO2_NODE_REBUILD_DIFFERENTIAL_TU15=1`
and the sweep's scripted input:

| Evidence | Result |
|---|---|
| Elongation 4p (600 ticks) | 48,759 calls compared, **0 differed** (previous: 6), 0 refused; 168D55 2,158, 16B7F5 317 |
| Sweep: every MP map 1p/4p, 600 ticks, plus campaign 01a, 01b, 03a, 03b, 05a and 08b at 600 frames | **56 runs, 2,271,055 calls compared, 0 differed, 0 refused** (previous: 2,271,055 / 6). 168D55 (169C46) 67,529 calls, 16A479 66,345, 16B7F5 9,691, 168D40 128, 16B1AD 290, 16B78B 84; 16940B is not reached |

The early stops are the same as in the earlier sweeps, all outside node
rebuild:

- Needle and Street Sweeper: active_camouflage's shader inputs;
- 01b: the 1CD130 memory-pressure trim owner;
- 03a, 03b and 05a: "Scene-copy resources belong to another renderer
  thread";
- 08b: the B9180 root's 2838C6 codec literal table.

### Production

- **Default.** 168D55 -> 169C46 and 16B7F5 -> 169863 are native by
  default, like the other first-person sites.
- **Reference switches.** `HALO2_REFERENCE_FIRST_PERSON_UPDATE_TU15=1` keeps
  the translated 169C46 and 169863 at these two sites. The existing
  switches still apply: `HALO2_REFERENCE_NODE_REBUILD_TU15=1` keeps every
  node-rebuild site translated, and `HALO2_REFERENCE_NODE_BIRTH_TU15=1`
  keeps the birth group translated. `HALO2_NATIVE_FIRST_PERSON_UPDATE_TU15`
  is accepted and has no effect.

| Evidence (default binary) | Result |
|---|---|
| Native default, Elongation 4p / Coagulation 4p (600 ticks, scripted input) | 35,667 / 53,578 native calls, no refusal. 168D55 runs 2,158 / 2,400 calls natively (the sampler runs inside it, so the 16A479 site is no longer separate) and 16B7F5 runs 317 / 424 |
| Armory frame parity (Original, 300 frames, 1280x720, Vulkan validation on) | The translated route (`HALO2_REFERENCE_NODE_REBUILD_TU15=1`), `HALO2_REFERENCE_FIRST_PERSON_UPDATE_TU15=1` and two native-default runs all give `5627a51badf47762` / `8a9b58b38bef`, with 0 validation messages |
| MP frame parity, Coagulation 3840x2160 (`tools/perf_4k60/parity.py`; reference `HALO2_REFERENCE_NODE_REBUILD_TU15=1`, candidate the default) | The default cases 4:90:0:0, 4:150:10:110 and 1:150:10:110: **PASSED**, with 9c14a330..., 13fa114e... and 05f31264... (the earlier receipts' hashes) |
| MP frame parity, Turf with `--motion` (reference `HALO2_REFERENCE_FIRST_PERSON_UPDATE_TU15=1`) | 4:300:10:200 and 1:300:10:200: **PASSED**, with 270e0cc6... (the earlier receipt's 4p hash) and 8be917eb... |

Elongation 4p stays nondeterministic from run to run (a known issue), so the
in-run differential above is its evidence, not frame parity.

Other sessions rebuilt the shared `build-perf-4k60` tree during this work;
every run used a frozen copy of the binary. The sweep binary had this
track's join; the default binary also had the default switch and another
session's opt-in light-update binding (`HALO2_NATIVE_LIGHT_UPDATE_TU15`,
off by default).

Receipts: `build/test-results/nodebuild-fp-query-names-20261116/`
(`differential/`, `sweep/`, `sweep.log`, `native/`, `oracle/`, `asan/`,
`mutation/`, `parity/`).

## Next steps

0. (2026-11-16) The 1D5320 divergence on Elongation is fixed: the MCC
   first-person query names (weapon track). 169C46 (168D55) and 169863
   (16B7F5) are native by default. Still to do on the first-person path:
   the residual owners (17A570, 171440, 128E70, 18BE50) and the private
   first-person context. If units' 115AF0/111B10 or
   unitactions' 1123B0 are ever bound live, they need the same MCC names
   (weapon.md, "Other native callers").
1. 16A4DD, 16AD45 and 169863 are native (section above), default at
   16A479. Next on the first-person path: 169C46 itself (1,800 entries and
   724k own instructions per Coagulation 4p census; its closure is about
   7,700 instructions in 110 functions, the heaviest 1D39C0's animation
   events 496k, 121890 447k and D28F0 341k), then the remaining translated
   callers of the natives: 16AD45 from 168D40, 15611D, 16940B, 16B1AD and
   16B78B (12 entries), 169863 from 1697A9, 16A469 and 16B7F5 (368), and the
   residual owners 17A570 (effects that follow the weapon), 171440
   (resource prediction) and 128E70 (sound stop) with their subsystems.
   145560 still executes 12.3M translated instructions per Coagulation
   census from other callers.
2. Codec kind 1 is native (2026-11-15) except its four mode 1/2
   second-frame loops (287D00, 2887F0, 289240, 289D60).
3. **1DBBD0's child shape transform** (shape vtable +34): no census or
   differential reaches it.
4. **The 125FB0 retry tail** (13FD10 age rollover) and codec kind 1,
   the remaining second-frame loops (kind 4's included) and track decoders,
   if a census ever executes them.
5. The function-value callbacks (100E70, 10BE20, 11E490/1DB800, F9E40
   kind 2, FA5D0, 11FF40, 24B430) are native since 2026-11-15 (see
   [attachments.md](attachments.md)), and so are placement's C2870 and
   C39B0 ([placement.md](placement.md)). Their remaining translated
   executions (Armory 10BE20 1,502, C39B0 1,512, C2870 3; Coagulation
   11E490 1,812) have no caller of their own to rebind: they run under the
   translated BCB70 (62,649 entries, 3.08M instructions on Coagulation 4p),
   10DE40 (44,100, 2.51M) and BC8F0 (8,365), whose callers are the light
   update and the effects. BCB70's translated call sites: C2CB0's C2DF4
   21,554, 17C370's 17C47C/17C497 10,119, BC8F0's BCA75 3,105, C0700's
   C0756 2,652, 17EAD0's 17EB76 2,563. The light update is the tick's
   `call_actual(0xc3430u)` (physics_live_world_tu15.cpp): C3430 (450
   entries, 1.21M) runs C2CB0 at C34B4 (21,770 entries, 5.28M own) and
   C4D30 at C3513 (25,430, 0.79M); C2CB0 reaches C4D30 at C32C7, C4D30
   C2870 at C4D9C, and C48E0 -> C3A20 -> C39B0. Since 2026-11-16 the
   light update is native: track `lights` ([lights.md](lights.md)) binds
   C3430 and C5290 over attachments' `light_update` and placement's
   `light_reconnect`, with placement's 16D740 (the collision sphere fit),
   attachments' C3910 and 13E6E0 (found by the live differential on
   Coagulation, Highplains and Waterworks) and C3AB0's fallback native.
6. The other rare refusals by size: the kind-4 second-frame loops (292C90,
   292280, 2940F0, 2935C0, 2956D0, 294B80) 2,349 instructions, which need
   kind 0's second-frame loops (2849F0/2843F0) as their base; kind 1's four
   mode 1/2 second-frame loops. (Placement's 16D740 is native since
   2026-11-16.)
7. Track havok's builders 1D73D0 (69 own) and 1D9510 (218 own), for the
   1CC310 binding (see [havok.md](havok.md)). Not started on 2026-11-15.
   1D9510 (`havok_component_remove`) and the 1D73D0 dispatch are native;
   the work left is the shape builders' closures (machine 1DDBF0, crate
   1DD950 and unit 1DEE30, about 5,000 census-executed instructions each),
   an entity-world fixture in the units track, and then the 1CC310
   binding.
8. Kind 4's second-frame loops (they need kind 0's second-frame loops
   first).
