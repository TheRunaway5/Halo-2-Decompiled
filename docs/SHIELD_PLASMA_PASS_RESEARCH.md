# Lockout plasma shield pass binding and posed-input contract

Date: 2026-10-11. This is source research for the active damage/shield goal.
It does not establish a live shield draw, an original framebuffer match, or
completion of that goal. The preceding admission/collector findings are in
[SHIELD_OVERLAY_SOURCE_RESEARCH.md](SHIELD_OVERLAY_SOURCE_RESEARCH.md).

## Authority and retained evidence

The executable authority is the original disc
[`default.xbe`](../assets/disc/default.xbe), SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The asset authority is original
[`lockout.map`](../assets/disc/maps/lockout.map), SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Recovered source and Ghidra exports were navigation aids; routine claims below
were checked against original instructions or actual cache bytes.

The ignored directory
[`overlay-pass`](../analysis/damage-shields-goal-20261011/overlay-pass/) contains
the read-only extractor, original listings, literal seven-class compiled
programs and decoded listings, and two isolated native probes. The detailed
asset report is
[`lockout-plasma-pass.json`](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json).
Rerun the extraction with:

```sh
.tools/python/bin/python analysis/damage-shields-goal-20261011/overlay-pass/extract.py
```

No implementation source was changed for this research. The parent owns the
material binder, original animated-constant differential and production
local-position adapter; their validation is separate from these research
probes. No TU1.1 or TU1.5 correspondence for this overlay/pass chain was
established here. A scalar owner's correspondence or a similar address delta
must not be substituted for complete-body/instruction evidence for these
owners.

## The actual shader selection

These are four distinct `shad` roots. First-person and world shaders must retain
their own constants and animation definitions even though their template/pass
is shared.

| Species/view | `shad` datum | Authored name leaf | Animated object export |
| --- | --- | --- | --- |
| Spartan FP | `E9F4086F` | `materchief_plasma_shield_fp` | `0A0012B4` |
| Spartan world | `E9F70872` | `materchief_plasma_shield` | `0A0012B4` |
| Elite FP | `EA860901` | `elite_plasma_shield_fp` | `12001A0B` |
| Elite world | `EA870902` | `elite_plasma_shield` | `12001A0B` |

All four point to `stem E9F50870`,
`shaders\shader_templates\transparent\plasma_shield`. Its only authored LOD
has available-layer mask `0x8000` and projected-height word zero. Layer 15's
packed range is `0x0200` (first 0, count 1); its one pass is `spas E9F60871`,
`shaders\shader_passes\plasma\plasma_shield`, implementation 0. The pass uses
`vrtx E95307CE`, `shaders\vertex_shaders\transparent_generic_offset`.
These are literal cache relationships, not shader-name inference.
[Asset report](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json)

`4BCC0` chooses damage-info `+80` for first-person and `+88` for world. Its
ordinary shield-overlay admission uses recent-shield-damage object `+F4 > 0`,
while its separately carried vitality is object `+F0`. The collector then
clones accepted original geometry; it does not create a sphere or substitute
different model geometry. The prior report records exact selector/collector
instructions and current matrix ownership.
[Admission report](SHIELD_OVERLAY_SOURCE_RESEARCH.md)

## The override part gate is narrower than general shader substitution

Original `41040` receives part ordinal in ECX, signed render-cache index in DX,
and seven stack arguments: view mode, coverage byte, original material-binding
pointer, override shader datum, force-regular byte, and two callback arguments.
It returns with `RET 1C`. Its relevant instructions are:

1. `41120..41134`: reject raw part type 0 or 5, and reject original material
   shader `NONE`. An override does not rescue a missing original shader.
2. `4113A..41162`: choose override when present and call `1BF50` on that chosen
   shader. Keep its selected LOD ordinal.
3. `411D1..41226`: if overriding, read the **original** shader's template mask
   at that **override-selected LOD ordinal**. This is not an independently
   reselected original-material LOD.
4. `41275..4129D`: only when `(original_mask & 0x8080) != 0` and
   `(original_mask & 0xA) == 0`, inspect the override template's **LOD 0** mask
   and reject if bit `0x200000` is set. It does not inspect the selected override
   LOD in this last test.

For all four actual Lockout shield roots, override LOD 0 mask is `0x8000`, so
this compatibility test adds no rejection. The original part type and original
material existence remain mandatory. Do not silently clamp a missing original
LOD to zero: the literal source indexing and valid corpus bounds need explicit
admission evidence.
[Original 41040](../analysis/damage-shields-goal-20261011/overlay/00041040-retail.asm)

`412A3..41381` separates regular command admission from the world transparent
queue. A pure mask `0x8000`, force byte zero, view mode zero reaches `1E2D0`;
`402B0` supplies view 1 and force 1 and builds regular layer commands, but its
caller `404B0` executes layers 11/14/12/conditional13, not layer 15. The main
deferred first-person bank instead reaches `3FDF0` with view 0/force 0 and a
separate transparent suffix. Selection of an FP shield shader does not by
itself identify the `402B0` route. Collection order does not establish final
pixel order. See the linked [scheduling ownership note](SHIELD_OVERLAY_SCHEDULING_RESEARCH.md)
for the complete distinction and native integration gaps.
[41040](../analysis/damage-shields-goal-20261011/overlay/00041040-retail.asm),
[402B0](../analysis/damage-shields-goal-20261011/overlay/000402b0-retail.asm),
[44AC0](../analysis/damage-shields-goal-20261011/overlay/00044ac0-retail.asm)

## Vitality byte +77 is not the authored plasma animation input

`2CBF0` quantizes the separately carried vitality into lighting-cache byte
`+77`: `CVTTSS2SI(scalar * 255)`, clamped to 0..255, when the lighting-cache
pointer and override exist and the other-scalar selector is zero. The other
selector writes `+76`. This is an original cache write and must be preserved.
[Original 2CBF0](../analysis/damage-shields-goal-20261011/overlay/0002cbf0-retail.asm)

The actual pass binding path is different:

- `4D9C0` receives AL coverage, ECX draw context, EDX layer, and five stack
  arguments (render-cache index, selected shader, LOD, pass, implementation).
  `4DA6D..4DA70` loads render-cache record `+10` into EAX and calls `1BD50`.
  `1BD50` installs that context in global `485A58` and marks context-dependent
  banks dirty. `1BBF0` binds the chosen shader's brightness/selectors, then
  calls `16B90`.
- `16B90` resolves shader → template LOD → layer → pass → implementation.
  It applies literal states and texture states, copies the 60-word pixel
  program when there are no fragments, maps authored constants, evaluates
  the extern banks, then applies authored animation.
- The actual plasma pass has no fragments, no permutations, no combiner
  assembly records, and **no extern records**. Its ranges at implementation
  `+11E/+120/+122/+124` are all zero. Its scalar/color/switch masks at
  `+104/+108/+10C/+110/+114` are all zero. None of the shader implementations
  animates render-state or texture-state ranges either.
- The dynamic values instead enter `174D0/17420` through object function
  callback `1B230`. Original `1B259..1B275` calls the context callback with
  object and input ID, then object and range ID. `1B27D..1B2B2` handles clock
  input when ID is zero and calls `13B390` on the authored curve. The biped's
  exported function converts `15000557` recent shield damage into the shader
  export shown above; current shield vitality is not that function's input.

[4D9C0](../analysis/damage-shields-goal-20261011/overlay-pass/0004d9c0-retail.asm),
[1BD50](../analysis/damage-shields-goal-20261011/overlay-pass/0001bd50-retail.asm),
[1BBF0](../analysis/damage-shields-goal-20261011/overlay-pass/0001bbf0-retail.asm),
[16B90](../analysis/damage-shields-goal-20261011/overlay-pass/00016b90-retail.asm),
[1B230](../analysis/damage-shields-goal-20261011/overlay-pass/0001b230-retail.asm),
[raw pass/shader fields](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json),
[biped export ownership](SHIELD_OVERLAY_SOURCE_RESEARCH.md)

Therefore no byte-`+77` → plasma extern/constant dataflow is established; this
particular pass has empty extern ranges and dirty-input masks. Preserve the
cache byte as its own output, but do not use it as a guessed shield opacity or
noise-strength input. This does not prove that every other pass or renderer
service ignores that cache byte. The bounded reader scans are retained in
[`retail-cache-byte-readers.json`](../analysis/damage-shields-goal-20261011/overlay-pass/retail-cache-byte-readers.json)
and [`retail-cache-word-readers.json`](../analysis/damage-shields-goal-20261011/overlay-pass/retail-cache-word-readers.json);
they are not exhaustive alias/dataflow proofs.

## Authored raster, texture, and program state

The pass's six literal state records, interpreted through original `18900`,
are:

| Index | Value | Effect |
| --- | --- | --- |
| 67 | `0x00010101` | RGB write enabled; alpha write disabled |
| 59 | `1` | blending enabled |
| 62 | `1` | source factor ONE |
| 63 | `1` | destination factor ONE |
| 74 | `0x8006` | additive blend operation |
| 64 | `0` | depth writes disabled |

The pass does not author depth-function/cull overrides. `1BBF0`'s reset path
supplies depth-enable 2, compare `0x203`, cull `0x900`, and alpha-test off.
Those are inherited-state requirements, not extra states encoded in `spas`.
Its cleanup packed range `+11C = 6` has count zero.
[18900](../analysis/damage-shields-goal-20261011/overlay-pass/00018900-retail.asm),
[1BBF0](../analysis/damage-shields-goal-20261011/overlay-pass/0001bbf0-retail.asm),
[native state decoding](../src/world_material.cpp)

Two authored texture bindings use parameters 0 and 1 at stages 0 and 1 with
external selector zero. Both resolve to `bitm E95107CC`,
`rasterizer\plasma_shield_noise`, image 0: **32×32×32 volume**, original type 1,
format 3, five additional authored mip levels. Both literal 24-byte sampler states decode
as wrap U/V/W, linear min/mag/mip, zero bias and zero signed-channel/alpha-kill
flags. Retain the real bitmap and mip chain.
[Asset report](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json),
[189A0](../analysis/damage-shields-goal-20261011/overlay-pass/000189a0-retail.asm),
[texture-state decoder](../src/texture_state.cpp)

The original 60-word pixel definition has texture-mode word 54 `0x42`
(PROJECT3D for stages 0/1, disabled 2/3), and combiner-count word 53 `0x11106`
(six active stages). Keep its literal program and three authored pixel
constant remaps; no replacement Fresnel/noise formula is required. The native
texture and combiner interpreters already represent this family. That is
capability evidence, not proof of physical NV2A precision or final pixels.
[Literal program](../analysis/damage-shields-goal-20261011/overlay-pass/lockout-plasma-pass.json),
[texture interpreter](../shaders/texture_program.glsl),
[combiner interpreter](../shaders/combiner.glsl),
[renderer admission](../src/renderer.cpp)

Word 59 is zero. Original `3FA3D0` stores word54 in device `+790` and
`word59 & 0x100` in device `+78C`. Original `3FFA80` rewrites authored texture
modes only when that latter flag is nonzero; the shield retains its two literal
PROJECT3D modes. It must not inherit the distinct Elite plasma weapon's
PROJECT2D/volume-R-zero convention. The production GLSL mode2 branch uses full
`xyz/Q` for 3D images, while mode1 uses `xy/Q` with R zero. Before the parent's
change, `bind_pass`'s volume guard admitted only mode1 and rejected this actual
shield with `unsupported volume texture addressing`; empty externs and supported
combiner arithmetic alone were insufficient material-admission proof. The
parent has now added explicit mode2 admission. A dedicated production volume
sampling/depth/mip validation remains a separate evidence gate.
[Original 3FA3D0](../analysis/damage-shields-goal-20261011/overlay-pass/003fa3d0-retail.asm),
[original 3FFA80](../analysis/damage-shields-goal-20261011/overlay-pass/003ffa80-retail.asm),
[volume guard](../src/world_material.cpp),
[mode1/mode2 production lowering](../shaders/texture_program.glsl)

## Critical pose contract: local noise coordinates survive skinning

The world class 0 is 612 bytes/38 instructions, SHA-256
`ab8189bc13c48e04f39d35a960f18a2eb20a02c9e1e97d84b8fd377661206086`.
Its literal declaration is `[0,4,5,6]`: position, normal, tangent, binormal.
It consumes position/normal and constant attributes v10/v11. Tangent/binormal
are declared but not consumed by this compiled program.
[Class 0](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class0.txt)

Class 0 instruction 10 displaces position with
`MAD normal, c26.w, position`. Instructions 18 and 20 compute/clamp the view
direction dot raw normal; instruction 36 mixes v10/v11 using that fraction.
Its RSQ normalizes the camera-to-position vector, **not the normal**.
Instructions 30..35 generate texture outputs o9/o10.xyz from the same v0
position using DPH against c18..23.
[Class 0 instructions](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class0.txt)

Local/skin classes differ in a crucial way:

| Class | Bytes / instructions | Position/normal producer | Noise-position instructions |
| --- | --- | --- | --- |
| 1 | 756 / 47 | fixed matrix c50..52 | 31..32; 39..44 |
| 2 | 836 / 52 | indexed one-influence matrix | 36..37; 44..49 |
| 3 | 836 / 52 | aliases class 2 | 36..37; 44..49 |
| 4 | 916 / 57 | two-influence matrix blend | 41..42; 49..54 |
| 5 | 996 / 62 | three-influence matrix blend | 46..47; 54..59 |
| 6 | 1076 / 67 | four-influence matrix blend | 51..52; 59..64 |

Each local program decompresses position using c170/c171, applies its actual
matrix/palette to position and raw normal for displacement/view/projection,
then independently reconstructs the **unposed decompressed local position**
for texture DPH. No local class normalizes the skinned normal. Class 6 has a
distinct four-influence program; it does not share the reflection family's
class-6 alias limitation.
[Class 1](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class1.txt),
[class 2](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class2.txt),
[class 3](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class3.txt),
[class 4](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class4.txt),
[class 5](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class5.txt),
[class 6](../analysis/damage-shields-goal-20261011/overlay-pass/e95307ce-class6.txt)

Thus default `Preserve` in `source_cpu_skin_inputs.cpp` does **not** prove
class-0 equivalence over posed geometry. That helper has a specific reflection
normal policy; it does not recover this offset program's local noise position.
Existing `skin_model` preserves the raw transformed basis, which is the right
normal contract here. Retain local position separately from its current posed
position, and preserve original section/part/class identity.
[CPU skin policy](../src/source_cpu_skin_inputs.cpp),
[pose producer](../src/model_pose.cpp),
[existing class-selection evidence](SOURCE_CPU_SKIN_INPUTS.md)

The isolated probe
[`pose_probe.cpp`](../analysis/damage-shields-goal-20261011/overlay-pass/pose_probe.cpp)
executes all six actual local/skin programs and the actual class-0 program
with controlled current matrices, source weights, nonidentity poses, explicit
camera/constant banks and nonzero displacement. It preserves posed position
and raw posed normal for geometry, and replaces only o9/o10.xyz with the
literal local-position DPH c18..23 results. All 18 cases/1,152 output words
are exact in this CPU interpreter comparison. Plain posed class 0 fails all
18 texture controls; normalizing the posed normal fails all 18 controls.
[Retained results](../analysis/damage-shields-goal-20261011/overlay-pass/pose-probe.txt)

This demonstrates the smallest bounded lowering contract: a separately
retained original decompressed local position for these six texture-coordinate
outputs. The parent has subsequently added an exact-family production adapter
using `Vertex.source_local_position` and six output corrections, with separate
CPU/GPU validation; that implementation is outside this probe. Its contract must
gate it by the exact compiled family/class evidence, transport that input to
the actual GPU vertex path, and compare its production output. Alternatively,
execute each actual selected local/skinned class with original local streams,
c170/c171 and the current source palette. Reusing the class-0 noise calculation
over posed position or deriving a global inverse for skinned geometry would
violate the recovered contract.

## Native capability and minimum remaining owners

The isolated
[`native_probe.cpp`](../analysis/damage-shields-goal-20261011/overlay-pass/native_probe.cpp)
loads all four actual shader roots through `Cache::load_material`, retaining
their one pass, class-0 code/declaration and zero extern counts. In the inspected
production path all four return `forward_pass = false`, and
`prepare_dynamic_material = false`: the existing forward/dynamic guards admit
transparent passes and plasma_alpha, but do not admit plasma_shield. Forcing
only the research copy's forward scheduler state then evaluates every root at
three explicit exported-function values without exceptions. FP/world colors
and displacement differ as authored (for example Spartan maximum c26.w is
0.01 FP versus 0.05 world; Elite FP leaves it zero, world reaches 0.1).
[Probe output](../analysis/damage-shields-goal-20261011/overlay-pass/native-probe.txt),
[loader](../src/material.cpp), [forward guard](../src/frontend_material.cpp),
[dynamic guard/binder](../src/world_material.cpp),
[animation evaluator](../src/shader_animation.cpp)

The parent has subsequently added dedicated `prepare_shield_overlay_material`
for actual layer15/implementation0 and reported 240 original actual authored
animated-bank comparisons passing. The normal `load_material` and dynamic
guards in this older pinned probe remain distinct from that explicit binder.
The parent also added the production local-position adapter and its validation
tests; no live schedule is inferred from either addition.
[Dedicated binder](../include/halo2/world_material.hpp),
[material test](../tests/source_shield_material_tests.cpp),
[vertex test](../tests/source_shield_vertex_tests.cpp)

The minimum remaining work follows these concrete seams:

1. **Admit this exact authored pass.** Select its actual LOD/layer/pass/impl;
   reuse literal textures, sampler states, program/constants, RGB/additive
   raster state and original shader animation. The parent owns this binder and
   the original animated-bank differential, now reported passing. Close the
   actual PROJECT3D volume sampling gate as described above. No vitality-opacity
   approximation is needed.
2. **Supply the authored object callback and clock.** Use the selected unit's
   actual exported function, recent-damage state and original evaluation
   semantics; preserve species/view shader roots. Existing callback-capable
   shader animation is a consumer, not proof of the live producer connection.
3. **Carry exact part admission/current geometry.** Apply `41040` gates to
   each accepted current model/section/part and retain current pose banks from
   the selector/collector. Do not include rejected type-5 parts or create
   unrelated geometry. Preserve source class and original material identity
   even when the displayed shader is overridden.
4. **Retain the proved local-position pose gate.** Retain unposed decompressed position
   alongside posed geometry for the proved o9/o10 lowering, or execute actual
   local/skinned class input banks. Prove nonidentity rigid and mixed-weight
   skins through the production GPU loader. Normal normalization is wrong for
   this family.
5. **Schedule in the actual view route.** World mask-8000 transparent queue,
   main FP transparent suffix and the specialized regular command path are
   different admission services. Keep original depth,
   cull, target and ordering inputs. Full final-stage context and pixel parity
   remain open.
6. **Render and compare.** Use actual noise/mips, original uniform bank, posed
   Spartan/Elite world and FP geometry, zero/hit/fade/recharge/depletion cases,
   and required Vulkan validation. Then compare against original retained
   frames when available. The present CPU probes do not close that gate.

The probes are reproducible using retained local cache archives; the pose
probe compiles its two directly used pose/palette translation units privately
to avoid a stale archive/header layout during concurrent parent work. Full
source/header, compiled-program, executable/cache, output, executable and
archive SHA-256 values plus exact compiler/commands are pinned in
[`probe-provenance.json`](../analysis/damage-shields-goal-20261011/overlay-pass/probe-provenance.json).
Both archives used by the rerun are retained under `overlay-pass/provenance/`.
The matching probe/helper source and headers were subsequently retained under
`overlay-pass/provenance/source/`; their manifest is
[`probe-source-snapshot.json`](../analysis/damage-shields-goal-20261011/overlay-pass/probe-source-snapshot.json).
Three unrelated headers had changed since the pinned run and were deliberately
not substituted. None is in the direct probes/helpers' dependency closure,
retained in
[`probe-dependencies.txt`](../analysis/damage-shields-goal-20261011/overlay-pass/probe-dependencies.txt).
The retained archives are the authority for their linked object code; this
snapshot does not reconstruct the archive build's full historical source tree.
Compiler: GCC 16.2.1 20260810, `-std=c++20 -Iinclude`, default optimization.
Cache archive SHA-256 is
`7f251cd66c071ad68ec18ec32bdd08605fa57d822c215fc075d50d0d951a8911`;
synchronization archive SHA-256 is
`23b556ee681dc6184ba9ac4ff49f2eae4054f95b2b573fefc81bef99adf543ff`.
The pinning script validates archive copies and reruns both probes:

```sh
.tools/python/bin/python analysis/damage-shields-goal-20261011/overlay-pass/pin_probes.py
```

The completed goal foundations and remaining damage-source scope remain in
[DAMAGE_SHIELD_COVERAGE_GOAL.md](DAMAGE_SHIELD_COVERAGE_GOAL.md). Neither these
bounded probes nor a newly admitted plasma material establish complete damage
coverage, shield effect attachment, FP/world live shield visuals, or original
GPU/framebuffer equivalence.
