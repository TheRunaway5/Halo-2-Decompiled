# Track `deviceanim`

This track ports the device animation tracks behind the HS builtins
`device_set_overlay_track` (10D5E0) and `device_set_position_track`
(10CF80), with the device channel starters and root sampling. Track
`weapon` (`weapon_animation`) supplies the shared animation-graph routines.

Sources:

- `src/gameplay/objects/native_object_factory_deviceanim_tu15.cpp`
- `include/halo2/native_object_factory_deviceanim_tu15.hpp`
- `tests/native_object_factory_deviceanim_kernels.cpp`
- `tools/native_object_factory_cases_deviceanim.py`

The namespace is `halo2::native_object_factory_tu15::device_animation`.
Build with
`HALO2_NATIVE_OBJECT_FACTORY_TRACKS="deviceanim;weapon;core;nodes;nodebuild;placement;units"`.

## HS-level owners

- `device_set_overlay_track(m, device, name, fp)` (10D5E0, argument order
  `EDI device, stack name`). A device has type mask 0x380 and an animation
  manager. It clears +16C..+184 and starts the overlay channel (+1AC) with
  `name`, falling back to 0x5000081. It then marks the device for update and
  sets or clears +12C bit 4. Returns whether the channel started.
- `device_set_position_track(m, device, name, seconds_bits, fp, services)`
  (10CF80, stack `device, name, seconds`). See the header for its contract.
  The services are:
  - `rebuild_nodes`: BF050, track nodebuild `object_rebuild_nodes`.
  - `set_position`: B95D0, track placement `object_set_position` with
    location 0 and force 0.
  - `codec`: 283A90, the animation codec. It is not native yet, so an absent
    service is the named refusal `283A90`.

  The services receive the frame overlay memory, because their address
  arguments name 10CF80's stack locals.

## Ported owners

Cases are physical-original fixture cases at seed 0x5C41, all run with
`HALO2_ORACLE_STRICT_WRITES=1`. Coverage counts the original instructions
the cases execute. "Muts" counts mutations the oracle rejected.

| Owner | Native | Cases | Coverage | Muts |
|---|---|---|---|---|
| 10D5E0 | `device_set_overlay_track` | 400 | 52/52 | 2 |
| 10D870 | `device_start_overlay_channel` | 300 | 73/73 | 1 |
| 10D790 | `device_start_position_channel` | 300 | 73/73 | 1 |
| 1D3770 | `start_named_animation` | 300 | 34/34 | 1 |
| 1E5360 | `find_named_animation`: a frame-word wrapper of track units' `animation_graph_find` | 300 | 93/93 | units' |
| 1E3F30 | `overlay_lookup` | 300 | 967/1007 | 1 |
| 10CF80 | `device_set_position_track` (composite) | 1,200 | 275/393 | 6 |
| 1D4200 | `sample_root` (composite) | 400 | 39/39 | 1 |
| 1D4280 | `sample_root_velocity` (composite) | 300 | 36/55 | — |
| 1D4170 | `sample_orientation` (composite) | 400 | 46/46 | 1 |
| 1E5BA0 (+1E5B30) | `static_pose` | 300 | 73/73 (+35/37) | 2 |
| 1D4100 | `codec_entry` | 200 | 20/20 | 1 |
| 1D4090 | `graph_node_index` | 150 | 18/18 | 1 |
| 1CF150 | `channel_seconds` | 150 | 15/15 | 1 |
| C1600 | `object_has_node_orientations` | 60 | 10/10 | 1 |
| 30BD0 | `normalize3` | 300 | 38/38 | 1 |
| 144BF0 | `basis_matrix` | 200 | 53/53 | 1 |
| 144090 | `matrix_inverse` | 300 | 82/82 | 2 |
| 144A60 | `rotation_quaternion` | 400 | 111/111 | 2 |
| 144E60 | `matrix_orientation` | 200 | 16/16 | 1 |
| 1201E0 | `quaternion_product` | 200 | 75/75 | 1 |
| 120570 | `orientation_compose` | 300 | 114/114 | 2 |

Reused natives:

- 1E5360 comes from track units.

- 1CE720, 1D36C0, 1E5810, 2A0100, 1E3270, 1E3150 and 1E5C80/1E5AF0 come
  from track weapon.
- BCDF0 and B9390 are the script builtins' natives.
- C15D0 and BAC00 come from track nodes.
- 1E32A0 and 145560 come from track nodebuild.
- 144910 comes from track placement.
- BC380 comes from track units.

The rows marked composite run through the composite runner,
`tools/native_object_factory_cases_deviceanim.py` as a script. It uses the
validator's World, reference build and observer. The original stops before
entering 283A90, BF050, B95D0 or 126200, and the native must then refuse
with that owner's name after the identical ordered store prefix. All other
rows run through the shared validator.

The 30 mutations, from one to six per owner, are all rejected. 1E5360's
mutations belong to track units. One
proposed 1D4100 mutation dropped the own-graph guard. That mutation is
equivalent, because 1E32A0 returns 0 for such handles itself. The screen
replaces it with a mutation that drops the codec entry. 1D4280 has no
observable effect before its codec boundary, so no mutation of it can be
observed.

Uncovered sites:

- 1E3F30: only the refused 126200 calls.
- 1E5B30 1E5B97: a NONE animation row, whose next read faults.
- 10CF80: everything after a boundary:
  - the speed measurement (10D1D2..10D1DC, 10D4BE..10D57B) needs two
    different root samples, which only the codec produces;
  - the node re-basing after B95D0 (10D391..10D454);
  - the +12C flag stores after BF050 (10D58E..10D5CE).
- 1D4280: everything after its second (codec) sample.

Final runs, all with `HALO2_ORACLE_STRICT_WRITES=1`:

- **Shared validator:** the whole builtins oracle, with the base factory
  owners and this track's regular kernels (`HALO2_FACTORY_TRACKS=deviceanim`),
  passes 31,600 physical cases with 63,200 native comparisons and 249,549
  ordered stores. This track contributes 4,660 of those cases.
- **Composite runner:** 2,300 cases with 8,132 ordered stores. Of the 1,200
  10CF80 cases:
  - 194 reach BF050;
  - 25 reach B95D0;
  - 121 reach the codec;
  - 860 return early (not a device, no model or render model, no animation
    manager).

  Every 1D4280 case reaches the codec.

Track units calls `overlay_lookup` (1E3F30) from 111B10, so that API is
kept stable. Units also builds its animation fixtures on this module's
`Fixture`. That fixture is budget-aware: before building a mode table, it
compares the table's worst-case subtree with the room left in the 64 KiB
track bank (keeping `RESERVE` bytes back), and drops keys until the subtree
fits. No extra random draws are taken, so construction is deterministic and
never overflows. Across 3,000 seeds the peak use was 0xA6B0 bytes.

## Frame locals

10CF80, 1D4200 and 120570 pass stack locals to other owners: matrices,
orientations, forward/up vectors and copies of aliased operands.
`FrameMemory` is a nested write-through overlay. It keeps 4 KiB of locals
private per frame, starting at 0xFFFC0000 so that it sits below track
nodebuild's 0xFFFE0000 frame. All other addresses pass through to the world
memory in order.

## Floating point

- The SSE steps run the original instruction under the live MXCSR.
  COMISS/UCOMISS use `sse_compared` (DAZ).
- The x87 sequences each run as one asm block under the live control word:
  - 30BD0: length, then the epsilon compare on the extended value.
  - 144A60: FADD/FSQRT/FMUL/FST.
  - 1D4170: the frame scale and its epsilon compare.
  - 10CF80: the FMUL tick scaling, FSQRT/FDIV and FSQRT/FMUL speeds, and
    the nine FSUB/FABS/FCOMIP epsilon compares.
- 1E5BA0 normalizes the packed base-pose quaternion with RSQRTPS. RSQRTPS is
  an approximation whose bits are the host CPU's, so the native runs the same
  instruction, as the physical oracle does.

## Refusals

- `283A90`: the animation codec, owned by track nodebuild. Every root sample
  at a nonzero frame reaches it, and so does a frame-zero sample of an
  animation without a base pose.
- `BF050`, `B95D0`: service boundaries. In production these are native in
  tracks nodebuild and placement.
- `126200`: the animation resource request (as in track weapon).
