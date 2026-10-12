# Original camouflage pass resources and scheduling

The original biped effect requires more than scalar extern37 on an ordinary
material. The source copies the current color target to target19, submits
the collected camouflage layer18, and conditionally submits layer19 through
a color surface that aliases the depth buffer. This document recovers those
contracts and the authored input mappings. It does **not** admit a native
camouflage pass binder, framebuffer effect, complete King match, or any
remaining variant. The earlier whole selector/collector evidence remains
separate and immutable.
[Selector/collector and owned-map inventory](MP_CAMOUFLAGE_OVERLAY_SOURCE_RESEARCH.md),
[new original report](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json)

The authority is pinned retail `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and unchanged Lockout metadata, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The report preserves 43 complete indexed original function ranges with their
raw-byte and listing hashes, literal dispatch/layer tables, and the bytes
of all 605 executed instructions. Static owner ranges and executed owners
are distinguished below. No executable instruction or game return was
replaced; no resident target or successful graphics service was supplied.
[Reproducible source tool](../tools/inspect_camouflage_pass_contract.py),
[report and dependency pins](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json)

## Authored resources and their actual inputs

All21 owned caches select the same named global override shader/template
and pass families. In Lockout, shader `E7E90664` has sole LOD mask
`000C0002`: layer1 `texaccum_disabled`, layer18 pass `E7EC0667`
`active_camo_opaque` with four implementations, and layer19 pass
`E7EF066A` `active_camo_opaque_stencil_modulate` with one implementation.
The all-map inventory retains actual datums, compiled NV2A programs,
vertex resources, state records and selected packed ranges. Names alone
are not evidence that resources are resident or that drawing succeeds.
[All-map authored census](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)

Layer18 implementations0/1/2 bind texture externs26/0/17/1 at stages0/1/2/3.
Implementation3 uses bitmap parameter0 at stage0 and extern1 at stages1/2/3.
Layer19 declares no textures. The original `18A90 → 19530` contract is:

| Texture input | Original resolution |
|---|---|
| Extern0 | `18A90` marks the stage datum NONE; it does not dispatch `19530`. |
| Extern1 | `19581` reads matg rasterizer row `+C`, bitmap index0, and calls `143C0`. |
| Extern17 | `19880` calls `14F60` with target19 (`13h`), then uses that target's guarded dimensions. |
| Extern26 | `19A68` reads the original shader at context`+8`, its postprocess pointer`+24`, special-index pointer`+60`, and signed bump index`+10`. An index of-1 repeats resolution through the matg global shader at row`+D0`. The chosen 12-byte bitmap row supplies datum`+0` and short index`+4` to `143C0`. |

The24 resolution-prefix probes execute unchanged `19530` for extern1/26,
four stages, and the actual global shader, Master Chief body shader
`E9080783`, and emblem shader `E914078F`. Their own bump indices are0/1/-1;
the emblem exercises the real fallback. They stop **before** the first
instruction of `143C0`, preserving the actual bitmap datum/index/stage.
The report includes the resolved bitmap names. It supplies no bitmap-cache
success or texture handle. `143C0 → 137550` remains the real cache boundary.
[Original texture bank](../analysis/multiplayer/game-variants/camouflage-pass-20261011/texture_bank-00018a90.asm),
[texture dispatch](../analysis/multiplayer/game-variants/camouflage-pass-20261011/texture_extern-00019530.asm),
[actual prefix inputs/outputs](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json),
[bitmap binder](../analysis/multiplayer/game-variants/camouflage-pass-20261011/bitmap_bind-000143c0.asm),
[cache owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/bitmap_cache-00137550.asm)

The packed extern records have different scalar/color semantics. `1AE70`
resolves the implementation's actual usage masks before `19BE0/19CA0`;
`1B2C0` decodes their destination/component modes. These are the camouflage
mappings, not inferred color-dispatch indices:

| Pass implementation | Packed record | Actual input and destination |
|---|---|---|
| Layer18,0/2 pixel | `80000325` | Scalar37; alpha of pixel constant bank0, slot2. |
| Layer18,0/1/2 vertex | `591C0025` | Scalar37; vertex bank1, register9.z. |
| Layer18,0/2 state | `00020028` | **Color40**; packed RGB of render state75, preserving its alpha. |
| Layer19 pixel | `00F0C225` | Scalar37; blue of pixel constant bank0, slot0. Component`C2` has scalar axis2 and mode0 after the source's low-bit decoding. |
| Layer19 vertex | `981C0029` | Color41; vertex bank1, register8.xyz. |
| Layer19 vertex | `781C0027` | Scalar39; vertex bank1, register8.w. |

Scalar37 dispatches `33850 → 33670` and resolves the first biped in the
actual owner/parent chain, with the established source clamp. Color40
dispatches `33EDD → 33670` and repeats that scalar in RGB. This color40
is **not scalar40**, whose unrelated dispatch reads a local-view array.
Scalar39 is zero without a valid fog interval; otherwise `3385D` computes
`-near/(far-near)`. Color41 at `33EF9` computes
`-camera_forward/(far-near)` when fog datum is non-NONE and far>near;
otherwise it copies the original default RGB through `4686D4+4`.
[Extern bank owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/extern_bank-0001ae70.asm),
[scalar dispatch](../analysis/multiplayer/game-variants/camouflage-pass-20261011/scalar_dispatch-000336f0.asm),
[color dispatch](../analysis/multiplayer/game-variants/camouflage-pass-20261011/color_dispatch-00033980.asm),
[vertex mapping](../analysis/multiplayer/game-variants/camouflage-pass-20261011/vertex_extern-00019be0.asm),
[pixel mapping](../analysis/multiplayer/game-variants/camouflage-pass-20261011/pixel_extern-00019ca0.asm),
[value modes](../analysis/multiplayer/game-variants/camouflage-pass-20261011/extern_value-0001b2c0.asm),
[state mapping](../analysis/multiplayer/game-variants/camouflage-pass-20261011/state_extern-0001a170.asm)

The420 executed compositions cover all five actual implementations,
seven finite primary inputs (-.1,0,.25,.75,1,1.1 and fade residue
`31C00000`), three fog intervals and four local-user inputs. Whole `1AE70`,
the reached original dispatch/resolver owners, whole `19BE0`, and whole
`19CA0` return unchanged. The report preserves actual scalar/color bits,
pixel programs, all vertex constants and dirty masks before/after. The
object/header pool is explicitly backed, with one full datum `12340000`,
and the unit is unchanged. Vertex baselines, view/fog values and local
scalar-setter inputs are declared inputs; this composition is not whole
`16B90`, authored animation, pass preparation, or a draw. Nonfinite input
behavior is outside this corpus.
[Composition inputs, source calls and outputs](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json),
[whole pass binder's actual ordering](../analysis/multiplayer/game-variants/camouflage-pass-20261011/pass_binder-00016b90.asm)

## Original copy, scheduling and target ownership

`420A0` groups collected kind1 records into the transparent queue with
callback`41CC0`. `1E370` invokes the sorted queue callbacks. The complete
`41CC0` instructions first set `485A75=1`, save the active bank mask,
zero it, and call `352E0(NONE,1)`. It then submits each group's original
draw collection and renders layers1/3/8/20/7, then generic layers2/16/18,
conditional19, and finally15. Layer19 requires fog datum`4B9F5C != NONE`,
`4B9F88 == 0`, and `max(4B9F78,4B9F7C)>0`. The collection mask is
`40000h` (18) or `C0000h` (18+19) under that same fog condition. Actual
layer-bank admission still applies; calling the layer selector does not
force a successful draw. The callback restores the saved bank mask and
clears its group flag on return.
[Queue grouping](../analysis/multiplayer/game-variants/camouflage-pass-20261011/group_builder-000420a0.asm),
[sorted queue executor](../analysis/multiplayer/game-variants/camouflage-pass-20261011/transparent_queue-0001e370.asm),
[exact callback and layer sequence](../analysis/multiplayer/game-variants/camouflage-pass-20261011/group_callback-00041cc0.asm)

`352E0` defaults NONE to destination target19. Mode1 binds the current
source target from `4858B8` through `14F60` into all four texture stages,
installs the original four-stage copy pixel program, and calls
`34A90(target19,10,01010101,0,0,1,.5,1,4,1,1)`. `34A90` builds the
destination quad and source camera rectangle, selects its destination
through `14BC0`, sets its copy states, and calls `34420`. These complete
instructions establish a drawn scene copy into19, **not** a source-side
clear followed by a generic host image copy. The destination/source
viewport, dimensions, tap geometry and graphics acceptance remain part
of the original contract; this report does not execute those graphics
services to success.
[Scene-copy owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/scene_copy-000352e0.asm),
[copy quad and source rectangle](../analysis/multiplayer/game-variants/camouflage-pass-20261011/copy_quad-00034a90.asm),
[quad builder](../analysis/multiplayer/game-variants/camouflage-pass-20261011/quad_builder-00034420.asm)

`44370` uses36-byte rows at`43FA1C`; the literal rows are retained in the
report. Layer18's target is-1, resolved to current target`4858B8`; layer19's
target is3. Both clear fields at row`+4` are zero, so neither selects a
`14B60` clear. The callback's `485A75` also suppresses ordinary layer
clears. For both18/19, `44370` calls `15370` with BX=0, which disables
hardware stencil. Thus the authored *stencil_modulate* name must not be
turned into an invented hardware-stencil lifecycle.
[Literal layer rows](../analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json),
[layer admission/target/clear/stencil sequence](../analysis/multiplayer/game-variants/camouflage-pass-20261011/layer_select-00044370.asm),
[actual clear mask converter](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_clear-00014b60.asm),
[stencil owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/stencil_control-00015370.asm)

Target19 uses the ordinary98h-stride target table: row begins`4B56A0`,
texture descriptor`4B56A4`, width/height`4B5720/4B5724`, backing/readiness
word`4B5728`, and unavailable byte`4B5735`. `14F60` dispatches19 through
`15074 → 1DCC0`; the descriptor is non-NULL only with a nonzero backing
word and unavailable byte0. `1DD60` likewise returns dimensions0/0 for
a cold19 row; the640x480 fallback applies only to targets0/3/24.
All four whole extern17 cold-target probes return NULL and0/0 through
the unchanged owners. This failure is retained rather than replaced
with a successful target. Static `1D770` sets up target19 as320x240,
linear=true, one level, using the original `4B5138 + 4B513C` backing range
and advancing the range by`4B000h`. `1DF20` is the descriptor producer;
`1DB10` and the startup path own actual backing acquisition. None of
those allocation/platform results is supplied here.
[Target texture binder](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_texture-00014f60.asm),
[guarded texture descriptor](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_descriptor-0001dcc0.asm),
[guarded dimensions](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_dimensions-0001dd60.asm),
[target19 setup at1D94F..1D974](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_setup-0001d770.asm),
[descriptor production](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_record-0001df20.asm),
[backing acquisition](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_backing-0001db10.asm),
[startup chain](../analysis/multiplayer/game-variants/camouflage-pass-20261011/frame_target_setup-00012560.asm)

Target3 is a distinct surface alias. Original `14600` retains the device's
color surfaces`50935C/509360` and depth surface`509364`. It creates a
24-byte surface at`509370` by cloning`50935C`, then replaces its backing
word`+4` with the depth surface's word`+4`. `14BC0` target3 selects
color`509370` and depth`509364`, then `1CD30` performs the actual surface
bind. Therefore layer19's color writes address the original depth
backing through a color interpretation. This is static alias evidence,
not proof that Vulkan can reuse a D24S8 attachment as an ordinary color
attachment or that a host conversion is equivalent.
[Complete surface/alias owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/surface_aliases-00014600.asm),
[target3 mapping](../analysis/multiplayer/game-variants/camouflage-pass-20261011/target_bind-00014bc0.asm),
[actual bind/cache owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/surface_bind-0001cd30.asm)

The authored states confirm the channel/blend operations. Layer18
implementations0/2 enable blending with ONE and
ONE_MINUS_CONSTANT_COLOR, additive operation, and baseline blend color
`FF000000`; color40 supplies its RGB. Their depth writes are off.
Implementation1/3 disable blending and enable depth writes.
Layer19's literal color-write mask is`00000001`: **blue only** in the
source ARGB byte-mask layout. It uses DEST_COLOR/ZERO additive blending,
depth comparison`202h` and no depth writes. Its pixel scalar37 also
selects blue. These masks must be preserved as bytes; calling them red
would swap the actual source channel. Inherited state and pass baseline
selection remain governed by`1BBF0/16B90/18900`.
[Authored selected state records](../analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json),
[state application](../analysis/multiplayer/game-variants/camouflage-pass-20261011/state_override-00018900.asm),
[source state names corroborated by instruction-led reconstruction](../tools/reference/halo2-decompiled/docs/unknown_03a5c0.md)

Original `4CBF0` and `4D0B0` replace the selected shader with matg row`+D0`
and LOD0 on layers18/19 when shader flags`+16` bit2 is clear. The schema
calls that bit *No Active Camo*. `4CBF0` retains the actual initial
implementation selector and original shader LOD metadata; with a non-NULL
lighting bank, layer18 chooses2 if the original LOD mask has bit15,
otherwise chooses1 when bank`+76 == FF` and0 otherwise. Layer19 has its
additional original fog and bank`+76 != FF` branch that selects0.
The original vertex resource loader and `4CBB0/1CAA0` acceptance still
run; this report supplies no successful return from them. Implementation3
is authored and its externs are exercised here, but this does not claim
that every original biped context naturally selects it.
[Complete implementation owner](../analysis/multiplayer/game-variants/camouflage-pass-20261011/implementation-0004cbf0.asm),
[initial selector](../analysis/multiplayer/game-variants/camouflage-pass-20261011/initial_implementation-00015d70.asm),
[draw callback and acceptance](../analysis/multiplayer/game-variants/camouflage-pass-20261011/draw_callback-0004d0b0.asm),
[supplemental flag label](../tools/reference/assembly/src/Assembly/Plugins/Halo2/shad.xml)

## Existing native consumers and remaining boundary

The native loader already preserves compiled implementations, texture
bindings, vertex/pixel programs, externs and ordinary state records.
`remap_pixel_externs` already decodes layer19's`C2` scalar mapping; it is
not an unknown special pixel operation. The current world binder still
rejects the missing vertex extern forms and texture extern17/26. It also
does not implement the camouflage blend-color update or admit the actual
camouflage layer schedule. Existing optional scalar37 material binding
therefore remains a separate, narrower consumer.
[Native material loader](../src/material.cpp),
[native extern remapping](../src/render_program.cpp),
[world pass admission and scalar binding](../src/world_material.cpp)

The existing water copy kernel's literal pixel-program words match the
mode1 `352E0` program, and the renderer has scene capture/downsample
images, descriptors and copy-pass plumbing. These are concrete reusable
components, subject to the original per-group camera/source rectangle
and target19 lifetime. The water-specific schedule or dimensions do not
establish a camouflage schedule. Target19 is also used by original
`26880` patchy fog; sharing its numeric index is not proof that both
consumers can share a host image's contents or lifetime. Native forward
raster setup preserves color masks and comparisons, but the current
constant-blend, depth-color alias, original group submission and final
refraction/depth-buffer results remain unbound and unverified.
[Matching copy-program consumer](../src/water.cpp),
[native images/copy and forward raster](../src/renderer.cpp),
[separate original patchy-fog target19 use](../analysis/multiplayer/game-variants/camouflage-pass-20261011/patchy_target_use-00026880.asm)

Reproduce without changing assets or earlier fixtures:

```sh
.tools/python/bin/python tools/inspect_camouflage_pass_contract.py
.tools/python/bin/python tools/inspect_camouflage_pass_contract.py --output /tmp/camouflage-pass-reproduced
```

The fresh report and43 listings are compared byte-for-byte in the adjacent
reproduction manifest. This deliverable changes only the research tool,
source evidence and this document. No implementation, CMake, progress
or gamemode admission is changed.
[Reproduction manifest](../analysis/multiplayer/game-variants/camouflage-pass-20261011/reproduction.json)
