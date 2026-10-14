# TU1.5 model and map light receivers

The authority is **OG Xbox TU1.5** and the original Xbox cache assets. This
checkpoint extends native secondary light commands and indexed ranges to
ordinary model sections. Full visual parity remains unfinished: live model
receiver collection, complete gameplay frames, and strict pixel equality are
still required. Vista is not a gameplay visual reference.

## Implemented native path

`source_bsp_append_part_commands` now resolves actual model geometry, materials
and the current cache palette through an explicit model service. It retains the
original secondary command bank, part visibility, shader overrides, LOD
selection, category admission, and enqueue order. Missing model ownership is an
error. Main-view commands and dynamic MTPR/prt3 branches remain separate owners.

`load_source_model_range_geometry` reads the original mode section and resident
part/subpart resources independently of vertex binding support. Ordinary opaque
selection now admits model metadata through the same service as map geometry.
Models without a subpart resource are accepted only when the actual subpart
count is zero.

`SourceModelEffectLights` preloads authored layer 12 / implementation 1 passes
for rigid model sections before renderer allocation. Its draw input requires the
actual full object datum, current palette, selected shader/LOD/pass, source range,
raster state and completed light constants. A dynamic object binding carries
its salted object identity; it is not assigned a scenery placement. This
consumer is exercised by the retained GPU frame fixture. It has **not** been
joined to live object receiver collection and does not provide native palette
production. The later [local vertex checkpoint](TU15_SKINNED_LIGHT_VERTEX_20261013.md)
adds raw rigid-boned/skinned class2..6 binding, original byte influence fetches
and GPU-verified palette addressing. Live palette/cache production and receiver
submission remain open.

## Independent original checks

Evidence root: `analysis/multiplayer-shading/world-effects-goal-20261011`.

The authenticated executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete captured live text matches that executable. The source snapshot is
`xbox-tu15-frag-capture-v7`, RAM SHA-256
`1120cd1b7c4838c7b30fc3504a0c233d537e03147a7b5c368a0a4b4fa7b5ff5c`.

`tools/oracle_tu15_light_model_commands.py` runs original C35D0 and 31C70 to
produce current units, palettes and resources. After the declared secondary
40E80 command-bank reset, it runs whole original 44AB0 with no replaced
instructions or services. The repaired instruction hook is installed before
translation; an empty instruction/call trace is rejected. The accepted output
is `tu15-complete-light-ranges-v3/commands-traced-v2.json`: **263 ordered calls,
980 distinct verified original instructions, six model unit owners**.

The native test independently loads geometry, materials, section selectors and
shader definitions from Cache. Only actual current source inputs, including the
consumed palette center prefix, are fixture inputs. Model palettes retain each
unit's own cache identity instead of aliasing a shared model/section. All three
command bank headers and their storage, the special-unit bank and ordered calls
are compared against original outputs.

`tools/oracle_tu15_effect_light_draw_ranges.py --include-models` retains the
combined source sort and post-material indexed range selection. Only final
device flush 1CF30 and indexed submission 405080 are intercepted. The native
test matches **12 sorted commands, 28 indexed draws and 1,303 indices**,
including the actual 229-index power-module draw between the first 22 and final
five map draws. Range fixture SHA-256:
`32b2a8f5b7de24dbb58d96357ac41f208f80334783a2e54319932873274d1145`.

Run the registered test with:

```sh
cmake --build build --target source_model_effect_light_tests -j 4
ctest --test-dir build --output-on-failure -R '^original_tu15_model_effect_light_ranges$'
```

Fixture provenance is in `tests/fixtures/tu15_full_light.md` and the adjacent
local provenance JSON. The original executable, maps, RAM and GPU captures are
local owned inputs.

## Pixel gate and correction of the no-op result

`tu15-complete-light-gpu-nvidia-v2` exports the original complete 28-draw
illumination segment, all consumed uniform words, textures and mips, plus its
actual color/D24/stencil input targets. `tu15-complete-light-native-frame-v3`
replays that segment through native Vulkan. Input target bytes, consumed vertex
constants, and combiner constants match; Vulkan validation reports zero errors.
The strict result remains **357 differing pixels / 478 channels**: 268 RGB
pixels and 95 alpha pixels differ, with overlap. The original segment changes
RGB at 24,284 pixels relative to its seed, so this reference is visibly active.

The isolated model comparison `tu15-model-native-frame-v2` initially reported
zero differing pixels. The original model draw also changes **zero pixels**
relative to its seed. This is a no-op and does not prove visible model lighting.
The verifier now reports original RGB/alpha contribution separately and rejects
an invisible or alpha-only reference segment. `tu15-model-native-frame-v4`
correctly fails that gate while
retaining exact input banks and zero validation errors.

The matched screenshot is
`tu15-complete-light-native-frame-v3/comparison.png`. It shows an illumination
segment with explicit original input targets, not a complete native gameplay
frame or a physical Xbox framebuffer.

The minimized second-draw probe `tu15-inputs-v3` compares all consumed position,
UV and packed normal/tangent/binormal bytes for 187 selected vertices: zero
differences. The post-vertex window and texture output banks also match exactly.
Post-geometry instrumentation reports small W/slope and light-vector differences.
Those results alone do not establish which arithmetic differs during the actual
draw; compiler/instrumentation effects remain to be separated. The exploratory
CPU dot-order probes do not match the original GPU outputs and are not acceptance
evidence. No shader arithmetic or depth bias was changed from those probes.

## Reference capture reliability

The private capture tool now waits for the configured USB controller topology,
reads Xvfb's complete display number, and optionally advances unchanged original
frame boundaries before input. Declared look/movement inputs use normal host
controller bindings. Their host hold times do not imply deterministic guest
input cadence. Guest code and memory are not patched.

New `xbox-tu15-model-frag-capture-v6` and `v7` retain authenticated TU1.5
RAM/PGRAPH captures. The v6 combined range oracle reproduces the 28-draw sequence.
The downward-look v8 attempt fails at the source breakpoint and remains a failed
diagnostic. A timeout now retains a failure record even if a secondary QMP
diagnostic also times out. None of these captures yet demonstrates a visible
original model light contribution matched by the native live receiver path.

## Remaining order and delivery

1. Join genuine native model receiver collection, current cache palettes and
   object lifetimes to the combined secondary command/sort/range owner. Admit
   required skinned sections from original declarations and shader programs.
2. Resolve the strict pixel differences with actual draw evidence, and obtain a
   visible original model-light case. Retain the no-op and exact pixel gates.
3. Complete the original impact continuation, posed unit contacts, effect/audio
   owners and grenade presentation paths. The bounded impact work is recorded
   in `TU15_IMPACT_PRESENTATION_20261013.md`.
4. Verify complete matched gameplay frames across the requested multiplayer maps,
   campaign enemies/allies, first-person models, resolutions, resize and local
   split-screen layouts. A seeded illumination segment is insufficient.
5. Build and verify a combined installed runtime that includes these changes and
   concurrent delivered features. On this checkpoint both launchers select
   `.local/online-couch-20261013/delivery/halo2_native`, with the unchanged TU1.5
   gameplay executable configured. That selected binary does not establish
   delivery of this model-light work. The older installed hash in the prior
   grenade-frame document is superseded by this current launcher check.

The full world-impact/grenade goal and the broader visual-parity scope remain
unfinished. Passing source fixtures does not close the runtime or appearance
requirements.
