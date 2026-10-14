# TU1.5 model and map light receivers

The authority is **OG Xbox TU1.5** and the original Xbox cache assets. This
checkpoint extends native secondary light commands and indexed ranges to
ordinary model sections. Full visual parity remains unfinished: normal gameplay integration, matched
visible model-light frames, and strict pixel equality are still required. Vista is not a gameplay visual reference.

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
consumer is exercised by the retained GPU frame fixture. The later [local vertex checkpoint](TU15_SKINNED_LIGHT_VERTEX_20261013.md)
adds raw rigid-boned/skinned class2..6 binding, original byte influence fetches
and GPU-verified palette addressing. The later
[live builder checkpoint](TU15_LIVE_MODEL_BUILDER_20261013.md) supplies native
pose/cache/resource production, four exact original CPU view comparisons and
337 exact object-light receiver queries. Its joined live collection produces
eight draw units from two actual native bipeds, now feeding the shared
BSP/model command, opaque selection and range owner. The controlled native
query yields10 raw GPU packets /1,896 indices. Original complete40E80/3BAA0
and eight4D820/15280 palette uploads match, including58,994,774 retained bytes.
The original comparison also verifies the geometry-header offset correction
(section+4) needed to select skinned shader classes. A960x720 native diagnostic now shows visible character illumination with
original depth EQUAL retained and no validation errors (`live-light-gpu-v3`).
This is a controlled native capture. Normal gameplay integration and visible
matched original frames remain open.

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

## Live posed-bound culling, 2026-10-14

The native linear cull adapter now resolves owner WORLD nodes and inverse-bind
data only when an unmasked subpart actually consumes a bounded model node.
The original sentinel-bound, already-masked and accelerator branches do not
read that pose. `source_draw_unit_cull_tests` checks all 1,258 original fixtures
and 41,015 words through both supplied and deferred pose banks; throwing
providers also verify that skipped branches never fetch an unused pose.

`tools/audit_model_light_cull_assets.py` inventories the original raw subpart
and bound resources without executing or replacing the renderer. The retained
`model-light-cull-assets-all-v1.json` covers 37 scenario maps, 35,295 sections,
and 243 sections with a possible linear posed-bound branch across 27 distinct
models. Unresolved bound indices remain explicit. Lockout's 1,029 sections
contain no bound records, so its passing nine-case live collection comparison
(`live-collection-v7`) cannot establish this branch.

`tools/capture_live_model_cull_tu15.cpp` creates the selected owner from its
actual scenario placement, palette and original constructor, then exports its
native memory, admission, draw units, masks and cull calls. The camera and three
light spheres are declared diagnostic inputs. Three further cases explicitly
request subpart culling and are labeled separately from actual admission.

`live-model-zanzibar-cull-v1` (big wheel) and
`live-model-containment-cull-v1` (blue base gate) each pass all six comparisons
against unchanged original `44710`/`45550`. Both produce five consumed pose
resolutions. Zanzibar executes five original WORLD-node reads, compositions and
inverse-point transforms; Containment executes fifteen. Units, masks, calls
and the retained native world banks compare exactly. These prove the bounded
live posed-cull path; they do not prove matched pixels or complete gameplay.

The Cairo tram originally stopped in its real `C1120` constructor at the missing
`202AB0` owner, then at `2184A0`'s unpublished `505E78` bank. Complete original
helpers and the real `1CFAB4 -> 1E7D90` storage startup now supply it. The fresh
`live-model-cairo-tram-cull-v3` passes all six original cull comparisons across
three draw units, with fifteen posed-bound calls and forty-five WORLD-node
reads/compositions/inverse-point transforms. The 400-case scene-device fixture
also passes, including all 73 instructions of the two newly reached helpers.
See `tests/fixtures/tu15_scene_device_constructor.md` for its validation boundary.

`live-cairo-tram-light-gpu-v3/comparison-crop.png` shows that actual created tram
in native Vulkan. The diagnostic reuses the existing section-specific PRT
renderer with authored SH and actual WORLD nodes. Two light packets change
25,469 RGB channels (maximum 81) through the original depth EQUAL, with zero
validation errors. The camera/light query are explicitly declared diagnostic
inputs; the light sphere scales with the owner solely for this diagnostic.
This is not a matched original tram frame or ordinary-gameplay integration.

`live-light-gpu-v4/comparison-crop.png` is a fresh native Vulkan model-light
diagnostic after this fix. Five raw light packets across four sections change
33,022 RGB channels while retaining the original depth-equality test; Vulkan
validation reports zero errors. It remains native-only appearance evidence.

## Pixel gate and correction of the no-op result

### Visible original model capture, 2026-10-14

The current visible reference is
`analysis/shader-parity-20261013/live-model-original-visible-frag-v2`.
Normal controller input moved the original player into the lower Lockout room;
`live-model-original-session-v3` retains the inputs and the
`tu15-visible-module` snapshot. The complete live TU1.5 text is authenticated,
with no guest instruction or data patches. This is emulator evidence.

`live-model-visible-ranges-v1` uses the **actual captured** secondary command
bank at `4A3C7`. The unchanged original sorter/range selector and native test
agree on seven commands, eleven indexed draws and 1,222 indices. Three draws
belong to models. Original `4D820/15280` is observed at the first `401410` c50
upload to retain the actual palettes without executing or substituting a GPU
service. A separately replayed collector still supplies the bounded command
builder test; it must not be confused with the actual captured frame schedule.

`live-model-visible-gpu-v1` retains the matched original framebuffer, depth,
textures, mips and sampler state. Draw 0 visibly lights the plasma rifle:
1,244 RGB pixels / 3,720 RGB channels change. Draw 9 visibly lights the power
module: 6,470 RGB pixels / 19,407 RGB channels change. The old no-op model
reference is therefore no longer the only available model fixture.

The native Vulkan comparisons remain strict failures:

| Artifact under `analysis/shader-parity-20261013` | Differing pixels / channels |
| --- | --- |
| `live-model-visible-native-rifle-v1` | 41 / 82 |
| `live-model-visible-native-module-v1` | 2,617 / 5,123 |
| `live-model-visible-native-full-v1` | 24,716 / 59,044 |

All three retain exact source color/depth/stencil seeds, consumed vertex
constants and combiner constants, and zero Vulkan validation errors. The module
capture contains real visible illumination, but does not establish pixel parity
or ordinary gameplay integration. Its matched image is `comparison.png` in the
module artifact. Original model normal-map anisotropy and native sampler state
both specify 8x. The substantial RGB differences need further isolation.

### Normal-map sampling isolation, 2026-10-14

The current power-module mismatch is not an incorrect uploaded texture or
light constant. `live-model-visible-normal-upload-v1/report.json` verifies all
nine mips of original resource 51512 byte-for-byte against Vulkan resource 295.
`live-model-visible-gl-sampler-v1/chunks.json` independently retains the actual
GL calls: LINEAR_MIPMAP_LINEAR, LINEAR, anisotropy 8, bias 0, base level 0 and
maximum level 8. RenderDoc's generic anisotropic descriptor alone hides the
underlying mip-filter choice.

At raw framebuffer pixel (323,130), matched four-pixel inputs in
`live-model-visible-quad-v1` agree within float rounding. Original GL API
inspection addresses that raw row at y=349; Vulkan addresses it at y=130.
`live-model-visible-pixel-v1` gives original RGBA [219,180,99,73] and native
[34,28,20,73], from the same input [27,22,17,45]. The sampled normal differs:
original [0.453910112,0.830014467,0.852719903,1], native
[0.409033328,0.721995890,0.890913248,1]. Cube and volume samples agree.

The explicit **diagnostic shader replacements** in `live-model-visible-lod-v1`
report mip choices 0 and 0.3359375. `live-model-visible-fixed-lod-v1` then fixes
both UV and both gradients to identical literal inputs in both APIs: GL and
Vulkan still produce different filtered RGB, while explicit mip-zero samples
agree. The raw framebuffer controls confirm this independently of debugger
stepping. These edited probe shaders are not original evidence or production
fixes; the unchanged baseline comparison remains a strict failure.

Matching GL's default minLOD -1000 in a separately retained test build changes
no pixels (`live-model-visible-native-module-minlod-control-v1`). That diagnostic
was removed from the working test; its exact source remains in
`source-effect-light-frame-minlod-control-v1.cpp`. The seamless-cube control
changes two pixels and does not address the normal-map mismatch. Lavapipe also
fails (6,636 pixels / 9,737 channels), so changing host GPU is not a parity fix.

Capture initialization identifies llvmpipe/Mesa; the exported GL replay and
baseline native comparison use NVIDIA RTX 5080. Both capture and replay backend
must be retained. The
[Khronos anisotropic extension](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_filter_anisotropic.txt)
leaves the filtering scheme implementation-dependent. This API/backend
isolation does not identify which result matches NV2A hardware. Do not add a
negative mip bias, force mip zero, change brightness or relax the equality gate
to fit this emulator replay. Physical NV2A filtering precision remains open.

The new view also exposed two reference-harness errors. Re-running collectors
from the mid-frame snapshot can add model units absent from the captured GPU
frame; `--captured-command-bank` avoids that for frame ranges. The GPU fixture
previously paired the next BSP-only packet with a combined-schedule texture
bank. The original unstable sort can permute equal keys differently when model
records are included. The fixture now resolves every BSP packet by its actual
owner, section, part, range and indices, rejects ambiguous/missing packets and
requires all generated packets to be consumed. The original combined schedule
is an explicit GPU-test input; native ordering has a separate CPU comparison.

The capture tool also now guards the legitimate `EDX=FFFFFFFF` no-light call
before its conditional breakpoint dereferences a light, and retains the real
CPU GDT/segment state. The replay uses those descriptors for original spatial
queries that consume `FS:[0]`, rather than supplying an invented SEH state.

`tu15-complete-light-gpu-nvidia-v2` exports the original complete 28-draw
illumination segment, all consumed uniform words, textures and mips, plus its
actual color/D24/stencil input targets. `tu15-complete-light-native-frame-v3`
replays that segment through native Vulkan. Input target bytes, consumed vertex
constants, and combiner constants match; Vulkan validation reports zero errors.
The strict result remains **357 differing pixels / 478 channels**: 268 RGB
pixels and 95 alpha pixels differ, with overlap. The original segment changes
RGB at 24,284 pixels relative to its seed, so this reference is visibly active.

The subsequent frozen shader checkpoint in
`analysis/shader-parity-20261013/tu15-full-light-frame-v1` reduces this to
**164 pixels /197 channels**, with75 RGB and95 alpha pixels differing. Input
targets/constants remain exact and validation reports zero errors. That build
also includes earlier Xbox texture filtering work; this reduction must not be
attributed solely to the skinned vertex changes. Strict equality still fails.

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

## Completed PRT cache bindings, 2026-10-14

The model service now runs complete TU1.5 `22830/23670` PRT cache initialization,
allocation, SH sampling/rotation and PCA projection. Cairo's actual tram placement
matches unchanged `4B5C0` over all 58,962,006 compared retained bytes for valid,
explicitly empty, and explicitly invalid SH-bank controls. The independent C++
section helper matches 2,277 consumed incident/coefficient words. Empty and invalid
banks require explicit retained incident state; absent state rejects.

`source_live_model_prt_bank_tu15` reads completed `24020` bindings from the actual
full-owner cache and the draw's region ordinal. It retains the source no-upload
result for an absent owner or zero node count and bounds coefficients to the
current arena. Fresh valid/empty/invalid readback cases each match 771 consumed
words against independently projected section banks; each fresh full-builder
comparison also passes without a retained-byte difference. These cases are
`live-cairo-prt-readback-{valid,empty,invalid}-v1` under the shader analysis folder.

`live-cairo-prt-cache-light-v1` now binds these completed native source PRT banks
to a per-owner Vulkan draw before grenade illumination. It produces two packets,
25,469 changed RGB channels and maximum delta81 through the original depth EQUAL
state, with zero validation diagnostics. Its baseline and illuminated images are
byte-identical to the previous independently computed tram-bank diagnostic.
The refreshed player capture `live-light-gpu-v6` likewise retains five packets,
33,022 changed channels, maximum delta166 and identical images to v5.
These are native diagnostics with declared camera/light inputs; strict matched
original model-light pixels and ordinary gameplay integration remain open.

## Remaining order and delivery

1. Integrate the shared live receiver/command/range owner into ordinary gameplay,
   with matching base-pass geometry/pose and current object/palette lifetimes.
   Carry the verified partial-sphere culling into that path and verify nonempty
   attachment paths.
2. Resolve the strict pixel differences in the visible original rifle/module
   cases with actual draw evidence. Retain the no-op and exact pixel gates.
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


## Main-view ownership and ordinary vehicles, 2026-10-14

The collector exports an owned main-view bank before secondary light collection
reuses its arena. Entries retain full salted owners, selected sections/LODs,
original coverage, lighting class, c50 rows, colors and PRT cache banks. Light
submission rejects changed owner salts or palette bytes even when cache tokens
match. Tests also retain base draws without lights and reject unavailable native
consumers. `MultiplayerObjectRuntime::render_model_view` scopes world access.

Ordinary local vehicles now use that main selection. Rigid ordinary vehicles
prepare their variant's reachable authored damage sections at all six LODs;
actual source units choose what is drawn. The legacy path retains its initial
section order, including repeats. The real grenade replay previously selected
unprepared damage geometry at frame26; the resource union gets it through that
transition. Overrides, unsupported lighting classes, client region replication,
and complete live shader-function ownership remain separate completion gates.

The first joined Warthog image exposed a red tint from constructor-default native
lighting. Joined and legacy vehicles now share the existing source-faithful C++
map-light query/cache. Packed source colors retain absent channels. The complete
native original map-light query/cache readiness is still unfinished; its newly
constructed default bank is not treated as a completed query.

### Retained captures and resource ownership

Evidence below is under `analysis/shader-parity-20261013`.

| Artifact | Verified boundary |
| --- | --- |
| `live-main-light-player-v1`, `live-main-light-cairo-v1` | Owned handoff preserves previous native lit/baseline pixels exactly; Cairo retains two actual PRT banks |
| `live-vehicle-main-warthog-v2` | Gray body restored; two owners/ten units, visibility and exact UDP lighting inputs |
| `live-main-scorpion-waterworks-v2` | 1280x720 source-selected rendering; three final owners/21 units |
| `live-main-wraith-waterworks-split4-v1` | 1280x720 four views; three final owners/17 units per view, exact UDP inputs |

These live captures precede the later fog-gate correction. They establish
integration, not original framebuffer equality. The first rendered frames also
have genuine source units: Scorpion13, Wraith9 per view.

Eagerly loading every vehicle LOD filled the original512-record cache. The
Waterworks heavy1 resource-only reproduction reaches Scorpion section130 as
the513th cumulative section, and observes capacity/slot extent512. Scorpion
alone publishes all131 sections/524 resources. The ordinary path now uses the
existing complete4C190/4C2B0 selection and actual130980 request service. It does
not enlarge the original cache or supply residency/publication success. See
`live-waterworks-vehicle-resource-set-v2.log` and the retained earlier failures.

### Original callback recovery and real grenade replay

The complete original effect/damage/receiver bodies and480-case evidence are
listed in [helper provenance](../tests/fixtures/death_cycle_helpers_tu15.md).
The frozen comparison passes5,442 ordered stores, retained bytes and GP/SSE
plus the documented arithmetic flag mask. Input contexts are declared component
records. Bounded external-owner cases supply no matrix, allocation or service
return. Generation is `projectile-cleanup-generation-v9`; old failing
comparisons and actual native logs are retained.

The actual scripted grenade gets through source collision callbacks, vehicle
placement ownership, effect-request construction, damage-state selection and
the frame26 geometry change. The combined v9 replay now captures actual source
light at tick61: `live-vehicle-grenade-root-warthog-v9/vehicle.png`. Its30 model
packets retain the full occupied light identity and original authored definition.
Delivery provenance joins the projectile and the actual `airborne_detonation`
effect to `grenade_frag_explosion`. Earlier tick38/46/52 deliveries were authored
reflection effects; timing or the transport name alone is not explosion proof.

The paired `live-vehicle-grenade-root-warthog-no-model-v1` control disables only
dynamic-model light submissions after collection. Original simulation, selected
geometry, source inputs, first-person, particles and9 BSP light packets remain.
The captured tick, all recorded owner/geometry inputs and light provenance match.
`live-vehicle-grenade-model-contribution-v1/comparison.json` reports2,257 changed
RGB pixels /6,381 channels, maximum61. Both captures pass Vulkan validation and
UDP snapshot checks. This closes the visible native grenade-to-vehicle handoff,
not the matched original pixel gate or all actor families.

The same actual grenade replay also passes1920x1080. Its tick, occupied light,
authored effect, source owner inputs, draw counts and declared camera match the
960x540 replay (`live-vehicle-grenade-root-warthog-1080-v1`). Four local views at
1280x720 receive17 model and6 BSP packets each. A pinned combined runtime repeats
the four-view frame byte-for-byte after the capture guard. Its model-only control
changes1,186/863/1,012/746 pixels in views0..3,3,807 total, while every recorded
input and all64 configured shader modules remain unchanged. See
`live-vehicle-grenade-split4-contribution-v1/comparison.json`. Difference images
are explicitly amplified8x. The four-player world differs from the one-player
world; matching these two simulation states or original framebuffers is not claimed.

The refreshed Waterworks Wraith capture is
`live-main-wraith-waterworks-split4-fog-v2/vehicle.png`: all four views have valid
lighting and visible geometry after the original fog correction. It does not
replace either retained strict matched vehicle comparison.


The capture request now updates the runtime-owned options copy; the old observer
only changed its caller's options and incorrectly continued past the desired
frame. Raw draw/light delivery provenance is diagnostic and does not alter GPU
constant banks, effect creation or the simulation clock. The later real cleanup
failure at tick180 reached311522 ->282780; its complete original body is now
lowered and independently checked. The fresh `live-vehicle-grenade-root-cleanup-v10`
replay now reaches tick200 with zero validation errors, captures its real final
frame and passes the UDP snapshot check. Its explicit finite end-tick mode
retains the first model-light provenance without stopping early.
The older frozen composition separately lacks324F80, which the combined runtime
owns. Failed runs remain retained and are not accepted screenshots.

`combined-root-shader-build-v18.log` records successful application/capture
builds. `verified-native-light-runtime-v1` pins both executables and the64
configured shader hashes; the application executable itself is not a complete
new gameplay acceptance run. The ordinary root build path was subsequently
removed by concurrent workspace work; the pinned binary survives. Both launcher
info checks still select `.local/crt-20261014/halo2_native` and their separately
managed MCC asset flow. These shader captures use original Xbox assets/TU1.5
explicitly and do not claim installed-launcher delivery or MCC visual authority.


Prop and SCEN synchronization now validate only their own placement types.
Repeated live vehicle generations sharing a netgame row no longer fail those
unrelated renderers; duplicate owned props still fail. Both complete21-map scene
suites pass. Root builds are additionally checked for interface drift because
concurrent header edits produced a rejected BSP atlas pointer; that failing
capture and the forced caller rebuild are retained.

### TU1.5 fog selection and strict pixel gates

The original25D30 gate selects26500 sky-only for classification2/3 unless its
actual override is nonzero. The renderer now consumes the two native source
control bytes and the selected branch. Both retained Waterworks vehicle headers
have classification3 and zero sky opacity;26500 emits nothing. The previous
unconditional atmospheric composite added fog over tank surfaces. The complete
original selector comparison passes81 cases. See
[fog provenance](../tests/fixtures/tu15_fog_draw_selection.md) and
`fog-draw-matched-frame-evidence-v1.json`.

| Matched artifact | Strict residual |
| --- | --- |
| `scorpion-matched-current-v2` | Declared hull patch:1,675/3,500 unequal pixels, mean RGB error0.30010/255, maximum8; prior mean0.67752 |
| `wraith-matched-current-v1` | Retained rectangle:3,454/9,324 unequal pixels, mean0.25665/255, maximum50 |

Both verify the original20 lighting floats and source LOD independently. Both
still fail strict pixels. Background/HUD/first-person clocks are separate input
owners in these offline vehicle comparisons. The larger Scorpion rectangle
intersects HUD and is not an isolated surface mask. A forced-atmospheric native
control is explicitly excluded from original pixel acceptance; it isolates the
fog contribution without modifying source globals or fitting colors.

The next sequence remains: verify extended cleanup and resolution/split-screen
coverage; resolve the matched original filtering/arithmetic/compositing
residuals; finish remaining live campaign/vehicle/effect owners; repeat full
resolution/split-screen and installed-runtime validation. The full goal remains
open, including the retained strict original grenade GPU failures.


## Ordinary BLOC source receivers, 2026-10-14

The live main/light consumer now includes ordinary BLOC placements, including
Lockout's original `E6F80583` power-module owner / `E6FA0585` render model.
The actual source view owns full identities, section/LOD selection, coverage,
WORLD nodes and packed colors. `SourcePropScene` retains complete eligible
owners before secondary arena reuse and submits their base once. Mixed PRT or
override owners retain the prior path and remain unavailable to this light join.
`selected_section_world` consumes immutable WORLD input without changing the
legacy pose cache. Client rendering, PRT, override/camouflage and additional
actor/weapon families remain completion gates.

`native_tu15_prop_main_view` passes in root and frozen builds with an actual
automatic TU1.5 power-module birth/main selection. It checks stale full owners,
wrong full definitions, atomic mixed-owner rejection, omitted/duplicate base
submission, lifetime across producer arena reset and unchanged legacy poses.
Its declared all-cluster camera and regular-sphere input are component stimuli;
this does not claim original portal traversal or a matched framebuffer. The
complete21-map prop suite and both original BSP light draw suites also pass.

The current gameplay capture `live-prop-lockout-grenade-pinned-v4` renders the
authored grenade at tick69: one actual model-light packet on the selected BLOC
owner plus28 BSP packets. Its direct delivery matches the original
`airborne_detonation` effect / `grenade_frag_explosion` light. The same-runtime
model-only control changes6,892 RGB pixels /18,541 channels, maximum125, within
rectangle `(448,319)-(529,446)`. All recorded source nodes, lighting values,
geometry selections, camera and input data match. See
`live-prop-lockout-contribution-v3/comparison.json`. Both captures have zero
Vulkan validation errors. These are native gameplay contribution checks.

Finite captures now carry an explicit shader directory through DesktopSession,
Renderer and both raw geometry/light backend instances. Ordinary callers retain
the configured default. `prop-shader-assets-v1` preserves64 unchanged SPIR-V
modules; actual renderer/raw-backend logs confirm that private path. The first
control was rejected after a shared patchy shader changed; an intermediate
control exposed a remaining raw-backend global lookup. Both are retained and
superseded by the complete-path control. The older frozen gameplay composition
also failed to execute its grenade pulse; it is not a grenade-frame acceptance.
Root is the accepted current gameplay producer. No service outcome, light state,
clock, bias, brightness or original instruction is supplied to make it pass.

The refreshed strict original comparison is
`live-prop-matched-original-module-v3`: captured original640x480 color/depth/
stencil seed and all consumed vertex/combiner constant words agree exactly,
with zero validation errors. Pixels still fail:2,617 unequal pixels /5,123
channels, maximum236. This reproduces the retained baseline failure. Its new
authority check independently hashes the owned TU1.5 executable and compares
all3,611,948 captured live text bytes with the owned image. This is an emulator
reference, not physical Xbox proof. The matched crop is explicitly a2x nearest
pixel enlargement. The full shader goal and strict pixel gate remain open.


### Host filtering controls, 2026-10-14

The corrected `normal-sampling-sweep-v1/replay-v3` selects the isolated native
power-module capture, event14 /681 triangle-list indices. Original event15589
has229 triangle-strip indices. `replay-v1` incorrectly selected a BSP-only
capture and its constant pixel results are rejected; `replay-v2` rejected the
incorrect assumption that both API index counts were229. These failures remain
retained in `normal-sampling-sweep-v1/evidence-status.json`.

The corrected native sampler inventory retains the actual `vkCreateSampler`:
LINEAR min/mag, LINEAR mip, repeat, bias0, anisotropy8, minLOD0, maxLOD1000.
The two APIs have matching authored filtering inputs. Identical literal UV and
gradient probes still differ. Replaying the **same** unchanged Xbox TU1.5 RDC
through Mesa GL also changes these samples relative to NVIDIA GL. This is direct
host-backend evidence, not evidence of which interpolation/filter precision
matches NV2A. No fitted mip bias, custom filter or pixel tolerance was added.

`live-model-visible-gpu-mesa-v2` retains all eleven source draws, both declared
reseeds, shader/uniform banks, color/depth and full texture mip chains from
software GL. Its capture hash and unchanged TU1.5 executable authority are the
same as the NVIDIA reference. `live-model-visible-gpu-mesa-v1` exports segment
endpoints only; the two attempted native v1 replays reject its missing per-draw
seed. It must not be used as a per-draw reference.

Using the unchanged frozen fixture and64 pinned unmodified production SPIR-V
modules, with the Mesa draw9 color/depth seed:

| Native device | Exact differences against Mesa GL | Inputs and validation |
| --- | --- | --- |
| NVIDIA RTX5080 |6,988 RGBA pixels /10,569 channels;4,057 RGB pixels; max237 | Consumed vertex/combiner bank word errors0, Vulkan errors0 |
| Lavapipe / LLVM23.1.1 |3 RGB pixels /3 channels; max1 | Same exact source seed; consumed vertex/combiner bank word errors0, Vulkan errors0 |

Both runs return failure under the unchanged equality gate. The canonical
NVIDIA GL/NVIDIA Vulkan comparison still fails at2,617 pixels /5,123 channels;
the software control does not replace that retained failure. The genuine
software pair is `live-model-native-mesa-reference-lavapipe-v2/comparison-crop.png`.
Its three residual raw coordinates are(311,111), (312,215) and(325,220).

`module-float-probes-v1` uses explicit alpha-byte shader replacements to retain
float bits at those pixels without a shader debugger. Both original and native
alpha controls independently reproduce byte17. Native RGB stays at the same
three-pixel failure in all21 probes, and every probe has exact source banks and
zero validation errors. Normal samples already differ before the combiners:
for example at(311,111), red is0.744472324848175 in Mesa GL versus
0.7444713711738586 in Lavapipe. Final green is0.6568633913993835 versus
0.6568610668182373. These probes locate a difference before framebuffer blending;
they do not yet distinguish interpolation, sampling and arithmetic causes.
They are diagnostic replacements only and have not changed production shaders.

Fresh RenderDoc capture under Lavapipe is unavailable under its advertised
production shader/layout feature contract. The attempt and a failed software
shader-debugger control are retained, not accepted as new pixel evidence. The
byte probes avoid changing that contract or claiming unsupported debugger data.

The original-frame exporter now records whether every selected draw/reseed was
exported, records its phase/event schedule and replay optimisation, and returns
a nonzero process exit after an exception. A segment-endpoint export can no
longer be mistaken for a complete per-draw export by inspecting its manifest.


### Source constant-W repair, 2026-10-14

The follow-up coordinate byte probes in `module-coordinate-probes-v1` find
bit-identical UV x/y at all three residual pixels. Native W at(325,220) is
0.9999999403953552 while the original is exactly1. Some helper gradients also
differ. The actual original vertex source at module event15589 initializes
`oT2` to(0,0,0,1) and writes only xy. The constant-W diagnostic in
`module-projected-w-control-v1` restores exact equality over the entire640x480
illumination-pass output; it is retained separately from the production repair.

Production now derives a four-bit mask from each admitted original vertex
program's actual output writes. `menu_world_vertex_default_texture_w_mask`
keeps a bit only when W is never written. The Vulkan raw fragment pipeline
specializes those lanes to their original initialized value1 before projected
sampling. Every explicitly written W keeps its ordinary interpolation. There
are no pixel, camera, material-name, resolution, brightness or mip-bias selectors.
Vertex catalog ABI2, the3104/448-byte uniform banks, texture mip data and sampler
state are unchanged.

`default-w-original-shaders-v1/default-w-proof.json` independently scans the
unmodified original active shader sources. The module's mask is14: T0 has an
explicit W write, which must remain interpolated. The rifle's mask is15. The
new native per-draw reports produce exactly those masks. The existing original
semantic-fixture tests additionally compare the native analysis with independent
original GLSL output-write masks for the title and spherical-light programs;
both registered source tests pass.

`default-w-matched-controls-v2` uses one pinned runtime and the same source
color/depth seed, original textures, current palettes, constants and exact gate:

| Case | Exact result |
| --- | --- |
| Original Mesa GL / native Lavapipe, power-module illumination |0 unequal pixels /0 channels; exit0 |
| Same runtime, previous unmodified fragment SPIR-V negative control |3 unequal pixels /3 channels, max1; exit1 |
| Original Mesa GL / native Lavapipe, Battle Rifle illumination |0 unequal pixels /0 channels; exit0 |
| Original NVIDIA GL / native NVIDIA, power-module illumination |2,616 unequal pixels /5,121 channels, max236; exit1 |

All four cases have exact source color/depth seeds, zero consumed vertex or
combiner-bank word errors and zero Vulkan validation errors. The equality gate
has not changed. The exact software passes establish these two illumination
consumers against the authenticated TU1.5 **emulator** capture on the stated
backends. They do not establish a completely native scene, all model families,
NVIDIA filtering parity or physical Xbox GPU precision. The native pass starts
from the original scene's color/depth seed; screenshots label that scope.

The complete64-module shader snapshot is `prop-shader-assets-v3`: the previous
pinned snapshot with only the rebuilt production `menu_world.frag.spv` replaced.
It preserves the other63 modules. `prop-shader-assets-v2` was an incomplete
48-module collection and is explicitly rejected, never used in these controls.
The final GPU fixture runtime is pinned in `native-default-w-runtime-v2`.
The root application/live-capture rebuild and ordinary live validation are
tracked separately; no launcher delivery claim follows from the GPU fixture.
