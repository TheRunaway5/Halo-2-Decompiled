# Original graphics mode against OG Xbox, and at high resolution

Work of 2026-11-11/12. Authority is the OG Xbox TU1.5 executable and the Xbox
disc caches. The first half (shader audit, bloom, shader level, enabling the
campaign lights) was done from the original code alone. The second half used a
real reference: the original TU1.5 title running in xemu, replayed draw by
draw (see "Reference frames"). That is emulator evidence, not a physical Xbox
capture, and no exact-pixel match is claimed.

## Reference frames

`analysis/campaign/character-shaders-20261013/campaign-original-session-v1`
holds an authenticated original TU1.5 session with the snapshot
`tu15-campaign-opening-neutral` (source tick 761 of `00a_introduction`) and a
lossless window recording. A private copy of that session was resumed, run to
the council-chamber shot of the gold Elite (tick 1430..1462) and captured with
RenderDoc between two debugger-placed markers, so the capture holds complete
game frames (about 970 draws each). Replaying one frame gives the target after
any draw, each draw's translated shader, its combiner constants and its vertex
constants. The port's own frame at the same shot (frame 967 at interval 0.05,
tick 1450) was captured the same way through its Vulkan layer.

The original frame is built in this order: texture accumulation (layer 1)
into its own target, lightmap passes (layer 3), environment maps and
self-illumination, then per light: shadow generation into a 512x512 depth
target, layer 11 into target alpha, for a cinematic object a second
generation and a three-tap layer 11, and finally layer 12. Before the light
passes the original Elite is as dark as the port's was; the key light makes it
gold. The port's layer 12 program and constants were already identical to the
original's, including the colour shift (constants halved, final combiner
doubled). The differences were all in what fed those passes.

## Shader definitions are equal

`tools/audit_original_shader_parity.cpp` loads every `shad` through
`Cache::load_material` from an Xbox disc map and from the selected MCC map and
compares template, shader levels, layers, passes, combiner definition and
assembled program, vertex program bytes and declaration, constants, remaps,
externs, render and texture states and the authored animation.

Across the 26 maps present on both sides, 2,304 unique shaders are common.
Every one is equal except:

- `objects\vehicles\spectre\shaders\spectre_damage` (08a, 08b) and
  `objects\vehicles\ghost\shaders\ghost_damage` (08b): not converted in the
  MCC import, so they carry no Xbox passes.
- Nine shaders whose only difference is the overlay `range` word, a string id
  that the two caches number differently. This is a tool limitation, not a
  shader difference.

`objects\gear\covenant\military\shaders\damage` fails to load from the MCC
import in ten multiplayer maps ("Source tag alias has no owned same-address
payload"). 81 shaders exist only in the MCC maps and are unconverted.

`tools/audit_shader_layer_passes.cpp` lists the authored pass and vertex
program a shader submits on one source layer, or every layer of every level
for shaders matching a substring (`MAP all SUBSTRING`, `AUDIT_PASS_DETAIL=1`
adds externs, textures, states and the program).

Reports: `build/test-results/original-shader-parity-20261111/audit-disc/`.

## Bloom

### Filter raster

TU1.5 targets 7/8 are quarter-size images of the 640x480 raster and each view
owns its part of them. The port sized its blur targets from the physical view
and only pinned the original tap spacing for a single exact 4:3 view, so at
16:9 or in split screen the glow shrank with resolution (about 6x at 2160
lines).

`BspPostprocessView::source_logical_extent` now carries the view's size in the
original raster for every non-title view. When the physical rectangle is
larger, the blur targets are a quarter of the original raster and the
threshold pass reads the original raster as the exact area average of the
physical texels (`shaders/bloom.frag`, `controls.w` packs width*4096+height).
A 640x480 view takes the unchanged path. Control:
`HALO2_REFERENCE_PHYSICAL_BLOOM`.

### Layer-17 glow mask

TU1.5 `44360` for layer 17 clears target alpha (`14B40` selector `0x10` is
`D3DCLEAR_TARGET_A`) and sets byte `492294`; `13430` clears that byte each
frame. The layer's passes (`bloom`, `transparent_bloom`, `bloom_zero`) write
alpha only. `214D0` then builds different programs when `492294` is set:

| Program | Word | Without mask | With mask |
| --- | --- | --- | --- |
| Threshold | 8 (final ABCD) | `000F000D` | `1C0F000D` |
| Threshold | 9 (final EFG) | `0C110000` | `0C111D00` |
| Composite | 34 (RGB stage 0) | `08011800` | `08011811` |

The final-combiner constant alpha is zero (`21813..21882`), so the threshold's
`A*B` term is zero and only its alpha output changes: the average mask times
0.25. Each blur pass scales alpha by `(0.4+1)*0.5*2 = 1.4`. The composite adds
`2*(blur.rgb*strength + blur.a*C0.a)` with `C0.a` literal 1. The original
bloom fixtures were all captured with the byte clear, which is why the port
only had the mask-less programs.

`menu_bloom(threshold,strength,mask)` now produces both forms; the two new
programs are in `shaders/native-programs/fragments/atmosphere.json`
(`tools/export_native_atmosphere_fragment_programs.py`). The renderer sets the
mask for a view when a visible draw has an enabled layer-17 material, calls
`BspPostprocess::record_bloom_mask_clear`, draws that layer, then
`record_bloom(view,target,true)`. Layer 17 is disabled for two to four players
by the original policy (`2C470`), so split screen has no mask. Control:
`HALO2_REFERENCE_NO_BLOOM_MASK`.

## Shader level of near models

`2CBF0` stores coverage*0.25 in a byte, so coverage saturates at 1020 pixels:
more than two view heights at 480 lines, less than half of one at 2160. The
level thresholds are fractions of the view height, so a tall view capped every
near model below the level the original selects for the same picture.

`source_shader_coverage_camera(bank,physical_height,source_height)` and the
matching `source_shader_lod_view` overload substitute the view's height in the
original 640x480 layout, as the BSP path already did (`source_lod_height`).
Multiplayer model and object views use it. At 3840x2160 on Coagulation the
first-person arm regains its top level; 640x480 output is unchanged. Control:
`HALO2_REFERENCE_PHYSICAL_SHADER_COVERAGE`.

Campaign uses it too. Its native collector takes the level multipliers that
`1BE90` writes at `492348..492360` from the physical camera rectangle, so the
campaign view owner republishes those seven values for the original layout
(`SourceCampaignViewInputTu15::source_shader_height`). `1BE90` runs twice in
that owner: once before BSP collection and again inside
`source_live_shadow_main_inputs_tu15`, immediately before the model draws are
collected. The first version of this republished only before the first run,
so the second run restored the physical values and a hologram body command
in the opening cinematic arrived one level lower than the owner's resident
draw (multiplier 1/2160 against 1/480). It now republishes after both.
Control: `HALO2_REFERENCE_CAMPAIGN_PHYSICAL_SHADER_COVERAGE`.

## Campaign dynamic lights in Original mode

The native campaign light owner (`pstencil::CampaignRenderer`) ran only in
pStencil and Path Tracing. Original mode therefore drew no dynamic lights in
campaign, including the opening cinematic's projected key lights
(`x01_light_arbiter`, `x01_light_prophets`, `x01_light_masterchief`,
`x01_light_master_guns`, `x01_light_cairo_fill`). It now runs in Original mode
too, without stencil casters. A receiver the owner cannot complete drops that
frame's lights and is reported once instead of ending the mission. Control:
`HALO2_REFERENCE_NO_CAMPAIGN_LIGHTS`.

`31150` submits source layer 11 before layer 12 for a light that is shadowed,
specular, or has definition flag `0x20`. Layer 11's implementation comes from
`30D00`; for a specular light without a shadow it is implementation 2, an
unblended alpha-only pass that writes the light's gel into target alpha where
the vertex term passes. The specular light response (layer 12 implementation
0) blends `DST_ALPHA/ONE` and does not sample the gel itself, so without layer
11 it was scaled by whatever alpha the scene left. Original mode now selects
and draws that layer first (`native_projected_light_selection_tu15(...,layer,
shadow)`, `prepare_projected_light_alpha_material`,
`ProjectedLights::draw(...,original_alpha_layer)`).

These passes test depth `EQUAL` against a base surface the ordinary renderer
rasterized separately. The two integer D24 results can differ by a step or
two, which left a pattern of unlit dots on lit models. In Original mode each
such draw is also submitted one and two steps nearer and farther; the stored
depth is a single value, so at most one of the five passes and alpha-tested
holes still reject. On the Prophet close-up at 640x480, one step lit 22,331
more pixels, the second 905 more, and steps three and four only 73 more. This
is a native tolerance, not original behaviour
(`HALO2_REFERENCE_LIGHT_DEPTH_STEPS`, 0..8, default 2).

Result in the opening cinematic at 640x480 and 3840x2160 (frame 2267 at
interval 0.05, tick 3400, the Prophet close-up): the headdress is lit to a
blown-out yellow-white with a wide glow and the face and collar are lit, where
before the lights it was dull tan with a black face. That is the look of the
user's Xbox still of the same shot.

## Object function inputs under an MCC cache

The gold Elite stayed nearly black under its key light. The replay showed why:
the original's texture-accumulation target holds bright yellow armour, the
port's held a dark mottled plate. The change colours were right
(`1.0, 0.749, 0.086`); the detail stage was not. The Elite armour shaders
(`arms`, `torso`, `legs`, `helmet`) animate their detail bitmap's frame from
the object function `elite_burnt` through the curve `1 - x`: frame 0 is a flat
8x8 texture, frame 1 a dark 128x128 plate. The cinematic Elite's biped defines
`one -> elite_burnt`, so the original evaluates 1.0 and binds the flat frame
(confirmed by a breakpoint at `1B288`: input 1.0 for that object, 0.0 for the
other Elites, which do show the plate).

`evaluate_object_function` recognised the builtin inputs `one` and `zero`
only by their Xbox string ids (`0x030005A6`, `0x040005A7`). An MCC cache
numbers them `0x030005B2` and `0x040005B3`, so every authored `one -> x`
function evaluated to zero. `ObjectFunctionBuiltins::from_cache` now supplies
the owning cache's two identities and the campaign scene, machine and
secondary-view callers and the main menu pass it. Multiplayer players and
campaign AI characters already resolved these names per cache
(`DamageFeedbackNames`), so they were not affected.
`audit_shader_layer_passes MAP functions` lists every object function driven
by `one` or `zero`.

The same callers now also answer the object-level vitality inputs
(`alive`, `body_vitality`, `shield_vitality`, recent damage, overshield) from
the live native object for a scene object that has one, as `108D90` does.
The armour's self-illumination scales by `shield_vitality`; without it the
orange gauntlet glow was missing.

## Parts that only have a self-illumination pass

The Elite's green inset lights are separate small sections whose shader
(`inset_lights`, template `illum`) has source layer 7 and nothing else. The
renderer drew a material's own pass with the texture-mapping passes and the
lighting passes afterwards, so the armour's lightmap pass, which tests depth
against the armour beneath, overwrote them. The original submits layer 7
after every lightmap pass. A material whose own pass is layer 7 and which has
no lighting pass is now drawn after the lighting passes of its list
(`emissive_forward_materials`; control
`HALO2_REFERENCE_EARLY_EMISSIVE_PARTS`). Multiplayer 640x480 frames are
unchanged.

## Light shadow buffers (self-shadowing)

`31150` gives a projected light a shadow buffer (feature bit 2) when the light
allows shadows, the global shadow byte is set and its caster bank submitted
layer 10. It then draws, per light:

1. `48ED0`: the projection block, vertex constants c40..48 (reciprocal range
   plane, target size from the light's footprint up to 512x512, the depth
   planes, the gel basis), and target 9.
2. Layer 10 (`shadow_generate`, implementations 1 and 2 from `30CB0`) from the
   light's third command bank (`4CE298`) into that depth target, cleared to
   one, with polygon offset factor 2 and bias 550 (`495D0`). This target is a
   Z buffer, not the W buffer of the main view.
3. Layer 11 on the receivers with implementation 0 (one tap), 1 (three taps)
   in place of 2: target alpha = clip term x shadow comparison x gel.
4. In a cinematic, `30EB0`: for each `render_lights_enable_cinematic_shadow`
   object this light receives, `167F80` projects the sphere around the
   object's marker into the light camera, the result is clamped to the light
   frustum, `494A0` turns it into a scale and offset (c48) that zooms the
   whole 512x512 target onto the object, layer 10 is drawn again, and layer
   11 implementation 6 (three taps, no clip term) is redrawn on that object's
   commands only.

All of that is now drawn in Original mode:

- `ProjectedLights::draw` selects with the full feature mask when the caller
  supplies shadow target slots, emits the caster draws
  (`native_projected_light_selection_tu15(...,10)` on bank 2), the comparison
  draws and the cinematic pair, and carries c40..48 on each
  (`light_shadow_block`, `cinematic_light_shadows` in
  `src/pstencil/projected_lights.cpp`). The marker position comes from the
  original `BAD60`.
- The values were checked against the replay: for the Arbiter light the
  port's block equals the original's c40..c47 and the 512x512 extent, and the
  authored scalar extern 17 (shadow weight) is 1.0 in every original draw.
- `prepare_projected_light_shadow_material` binds the layer 10 and layer 11
  shadow passes; texture extern 13 is the light's depth target.
- The renderer owns up to eight 512x512 depth targets per frame
  (`ensure_light_shadow_targets`), renders every target before the main pass,
  and binds it per draw. `shaders/menu_world.frag` performs the comparison
  (`sourceShadowCompare`): stored D24 depth greater than the reference
  `T.z/T.w`, texel coordinates, the four comparison results filtered as the
  hardware does for the authored LINEAR state, and outside the map lit. The
  emulator filters depth before comparing, so shadow edges differ slightly
  from the replay by design.
- Draws into a shadow target store screen-linear Z
  (`worldZDepth`, `MenuWorldNativeDrawInput::linear_z_depth`). With the main
  view's reconstructed W every caster stored the same depth and everything
  was in shadow.
- Control: `HALO2_REFERENCE_NO_LIGHT_SHADOWS`.

Result on the gold Elite shot at 640x480 and 3840x2160: bright gold armour
with the glow, green inset lights and orange gauntlet glow, and the neck, jaw
and far arm in the helmet's and pauldron's shadow, as in the original frame
(`elite-front-original-before-after.png`).

## Open

- The cinematic shadow zoom (c48) agrees with the replay only as closely as
  two captures a few ticks apart can (the Elite moves); it was not compared
  at an identical tick.
- Shadow buffers are drawn for campaign projected lights only, and were
  only exercised in the opening cinematic. Multiplayer projected lights (the
  flashlight) still select without feature bit 2.
- Sphere lights on models go through `SourceBspEffectLights` and do not have
  the depth tolerance; they were not examined for the same dots.
- The three unconverted damage shaders and the failing `damage` shader above.

## Verification

State of the working tree on 2026-11-12, kept build tree
`.local/test-scratch/halo2-test-2m1lm8cc/build`. Nothing is committed or
published.

- Gold Elite front shot (frame 967 at interval 0.05, tick 1450), 640x480:
  frame hash `2230615b4863`, compared by eye with the original recording and
  replay (`elite-front-original-before-after.png`,
  `elite-front-zoom-original-vs-port.png`). The same shot at 3840x2160
  (`8f6659f1f4cb`, with the original shader level) shows the same lighting,
  glow, inset lights and shadows (`elite-front-4k-downscaled.png`,
  `elite-front-480-vs-4k-detail.png`).
- The port's shadow projection block for the Arbiter light equals the
  original replay's c40..c47 to the five digits printed, with the same
  512x512 extent. The cinematic zoom was `(-2.032, 2.039, 987.5, -408.0)`
  against the original's `(-2.034, 2.039, 978.9, -409.9)` a few ticks apart.
- Gold Elite back shot (frame 2667, tick 4000), 640x480: hash `0afdbd129151`;
  blown-out yellow-white highlights with glow where it was dark
  (`elite-back-before-after.png`). The same frame at 3840x2160
  (`3561c3d5fbb4`) shows the same lighting; reaching it runs the whole
  cinematic to tick 4000 at 2160 lines with the original shader level.
- Prophet close-up (frame 2267, tick 3400): 640x480 `7de2f87c44dc`,
  3840x2160 `7774f1a1a878`. The raised hand and face are lit in both; the
  earlier 480-line difference is gone.
- 640x480 multiplayer frames (Coagulation `eaa41783d03c`, Lockout
  `6dfc533d7906`, tick 120) are byte-identical to the published build
  `17a52de3`.
- `03b_newmombasa` runs 400 frames in Original mode (its opening
  cinematic) at 960x540 (`ce1739864819`, unchanged by the shader-level
  change) and at 3840x2160 (`4a5ca58a9ccf`). The gameplay flashlight, which is also a shadow-casting
  projected light, was **not** exercised: skipping that cinematic ends the
  run at tick 4 with "TU1.5 live world owner missing at 1b0842 -> 1caee0",
  and the published build `17a52de3` fails the same way.
- Main menu: renders with the cache's builtin identities and looks the same
  as the published build. Its capture is not repeatable (the published build
  differs from itself between runs), so there is no byte comparison.
- Tests run from the existing makefiles (CMake regeneration is currently
  broken by an unrelated in-progress change, see below): passing
  `original_tu15_model_effect_light_ranges`,
  `original_effect_light_opaque_selection`,
  `original_effect_light_raw_bindings`, `retail_menu_world_fragment`,
  `retail_menu_world_raster`, `retail_menu_bloom_gpu`,
  `original_cinematic_bloom_tu15`, `original_cinematic_object_functions_tu15`,
  `retail_source_shader_lod`, `retail_source_shader_lod_assets`,
  `retail_shader_animation`, `native_object_function_query_binding`,
  `pstencil_projected_light_selection`; three object-function tests skip.
  Not runnable there: the five `retail_menu_world_fragment_raster*` tests
  (fixture `native-fragment-raster-fixtures/1335/before-color.bin` is
  absent), `original_effect_light_opaque_sort` and
  `retail_menu_bloom_capture_gpu` (binaries not built),
  `original_effect_light_materials` ("Linked or unavailable resource path",
  as before these changes), and every test that links `halo2_systems`
  (link fails on the unrelated change). They were run before the last two
  changes (campaign shader level default, main-menu builtins), which touch
  only code those tests do not link. No full suite or launcher qualification
  was run.

Evidence images are in `analysis/original-mode-xbox-parity-20261112/`
(about 8 MB): start with `elite-front-original-before-after.png` and
`elite-front-stages-original-vs-port-before.png`. `as-run-tools/` there holds
the scripts used for the original session and the replays exactly as they
were run (they name a session scratch directory and need their paths
changed): `vm.py`/`hostcap.py` resume the private xemu session and bound a
RenderDoc capture with a host debugger, `rdlist.py`/`rdhist.py` are
`qrenderdoc --script` replays (per-draw state, targets, constants; pixel
history), and `nhook.c`/`ncap.sh` capture one frame of the port.

During this work `cmake/CampaignNativeAiAreaPerform.cmake` (another session's
untracked file) included `CampaignNativeAiPathArena.cmake` a second time, so
CMake could not regenerate any build tree. The engine was built per target
from the existing makefiles and that change's two new sources were linked by
hand. Those files were not touched.

Deterministic cinematic captures past frame 600 need
`HALO2_REFERENCE_CAMPAIGN_CAPTURE_FRAME_LIMIT` (600..20000) with
`--campaign-reference-capture`; at interval 0.05, frame 967 is the gold Elite
front shot (tick 1450), frame 2267 the Prophet close-up (tick 3400) and frame
2667 the gold Elite from behind (tick 4000).

## Follow-up validation, 2026-11-12

The background run that finished after the renderer session stopped did not
pass: 94 tests passed, 10 skipped and 22 failed out of 126. Its bounded build
log, test outcome log and receipt are preserved in
`build/test-results/original-renderer-resume-20261112/prior-claude/`.
The previously described CMake include problem is resolved in the current
checkout. The following remaining test/build issues were reproduced and fixed:

- `native_tu15_frustum_draw_cull.cpp` now belongs to `halo2_cache`, alongside
  its effect-light geometry-frame caller. Cache-only light collector tests
  previously failed to link because the implementation lived in the higher
  level `halo2_systems` library.
- The raw sphere, packed sphere and pStencil roundtrip fixtures retain their
  authored world vertex/fragment pair. Their old controlled baseline replaced
  only the fragment with a scope-copy program, which is not an admitted world
  program. The constant texture and additive/isolation assertions remain.
- The bitmap-import test uses the current `TexturePixels` constructor. The
  vertex-catalog rejection test uses an unsupported ABI; its old value had
  become a supported ABI. Both pass in a fresh disposable build.
- The frontend model test can locate the retained extracted Vista cache when
  its original import path is absent. It authenticates the complete cache
  against the unchanged fixture SHA-256 before checking the original data.
- Shader animation coverage now feeds the authored Elite object function
  through the evaluator and into all ten armor bitmap cases with the MCC
  builtin identities. The untranslated-identity control reproduces burned
  frame 1; the correctly mapped input selects normal frame 0.

Those repaired GPU fixtures then exposed a production error in packed
`EffectLightDraw` submission: the renderer still required a nonzero interpreted
vertex-instruction count. Native materials deliberately leave that count at
zero, so valid sphere lights failed with "Sphere diffuse draw lacks authored
vertex program". The guard now requires the material's exact compiled native
vertex kernel, matching the existing native shadow admission. Unbound materials
still reject. All four historical-library probes reached this same failure;
the raw-light and pStencil fixtures had already exercised their raw light
draws and relative checks before reaching the packed control. Probe commands,
source/library hashes and bounded logs are in `fixture-probes/` beneath the
follow-up report directory. The production fix is included in the fresh build.

The same retired instruction-count check also prevented global/title material
fog updates. Per-view gameplay banks independently supplied their fog, but
packed global/title draws kept zeros in c12..17. An actual Vulkan descriptor
binding probe at 853x480 observed three affected bindings; expected c12 was
`(-0.00179125, 0.00639476, 0.000585638, 0.093964)`, and the submitted c12 was
all zero. This guard now also checks native-kernel residency. The fog edit
landed after the focused test binaries linked, so its final-source validation
is a separate bounded compiler/Vulkan binding probe using retained dependencies,
not a claim that those earlier binaries include the fog edit. See
`fog-baseline.log`, `fog-compile.json` and `fog-final.json` in the follow-up
report directory for exact source, dependency and command provenance. Rebuilding
the final renderer source and rerunning the identical probe changes the result
from 0/3 to 3/3 bindings with all six expected fog vectors exactly present.
The private archive, object, executable and capture outputs were removed.

The fresh focused run uses `tools/run_tests.py`, which discovers the bundled
Vulkan validation layer, and supplies the authenticated TU1.5 executable
(`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`) to
the shader-LOD oracle. Claude's direct CTest run lacked both settings.
The raw-light GPU test also enables synchronization validation explicitly.
The focused run passes **24/24 tests**, with no skips, including the sphere
light GPU cases, pStencil roundtrip, Chief/Elite color GPU checks, bloom,
shader animation, frontend models and the native object-function binding
proof. The shader-LOD oracle passes 1,393 cases against unchanged original
instructions. The separate menu run passes **2/2**, for **26 focused tests**
in total. Results are in
`build/test-results/original-renderer-resume-20261112/focused/report.json`
and `menu/report.json`. The final global-fog edit is covered by the separate
final-source probe described above.

Two additional probes used the retained renderer build, not a fresh engine:

- The Original opening cinematic reached frame 967/tick 1450 with projected
  lights active and no synchronization-hazard messages. This did not reproduce
  the suspected shadow render-pass dependency problem; no speculative
  production synchronization change was made. The bounded receipt is
  `build/test-results/renderer-resume-20261112-shadow-sync/baseline.json`.
- `native_late_effect_light_creation` passes with the launcher-selected
  imported cache `mcc-82114d7d2e034d48ba762109935f9ca3`. The earlier test used
  `mcc-sky-water-tu15-20261025-v7`, which lacks the required celestial-rank
  artwork. Its report and executable-hash receipt are in `prior-claude/`.

Eight historical menu GPU capture tests still lack original capture inputs;
their evidence was not recreated or replaced with native output. The linked
resource failure in `original_effect_light_materials` is still outside this
focused validation. The visual parity limitations in **Open** remain, and
these checks do not establish whole-game parity or launcher qualification.
The selected launcher runtime remains `17a52de387e14a4d8ca388371353014f`.

Both new CMake build owners and every new compiler/capture probe scratch
directory were verified removed. No new binaries, archives, memory banks or
frames are retained; approximately 9 MB of bounded logs, receipts and small
probe source remain. The pre-existing approximately 17 GiB Claude build at
`.local/test-scratch/halo2-test-2m1lm8cc` and the original/reference evidence
were preserved. `validation.json` in the follow-up directory records this
scope and cleanup, and `source-hashes.json` authenticates the final relevant
sources; no subsequent drift was found.

## Council lighting and pStencil follow-up, 2026-11-12/13

The later Claude session titled **pStencil graphics implementation** added a
campaign model-lighting correction and suppressed cinematic helper geometry.
The correction uses the existing native D1850/D1E10 lightmap query and D4080
conversion when the world's lighting-query stamp advances. It writes the
completed lighting/sample bank consumed by the model builder. This is not a
hardcoded councillor color: the retained original-memory reading for
`e315009e` has primary RGB `.104/.096/.113`; the old port reading was
`.447/.404/.456`. The corrected reading agrees with the original at the
diagnostic's three decimal places. The original was sampled at tick1248 and
the port at tick1240, so this is not an identical-tick or bitwise parity claim.
The retained evidence is summarized in
`analysis/launcher-engine-build/pstencil-campaign-resume-20261112/prior-council-check.json`.

Campaign model selection now rejects authored part types0/5, matching native
41030. The actual cinematic anchor has18 authored vertices and submits none;
the visible council Elite remains a positive control with2484 vertices in
both CPU and deferred selection. A private negative control without the part
filter fails. Level restart also clears the old World identity before replacing
Simulation, so allocator address reuse cannot suppress callback installation.

The lighting service still has documented boundaries: rigid-body velocity,
biped sky-fallback state and dynamic-contact attribution are not established
as full original-game parity by this correction.

The campaign pause owner previously required a separate HUD asset directory,
and its MCC constructor lacked the admitted gameplay-map argument. The default
campaign route therefore had no pause menu; supplying a HUD directory instead
failed the MCC menu admission. The caller now admits the MCC route and supplies
its gameplay map. A real-main input regression covers pause, saving Graphics
Mode and restart; a component menu test alone would not exercise this caller.

pStencil's volume vertex shader computed W with an unordered dot product even
after computing the source program's ordered clip W. Its fragment shader then
used hardware varying interpolation, while the visible surface reconstructed
integer W from the ordered, quantized screen triangle. The previous GPU fixture
used the same shortcuts and could not detect their disagreement. The fixture
now executes the retained surface program, preserves its screen triangle and
uses production `worldWDepth`. The unchanged volume shader produced18–23 false
self-shadow pixels on the captured Prophet hand as the camera moved. Reusing
clip W removed those but left one false pixel on the analytic convex cap.

The volume pipeline now preserves the ordered screen triangle in a geometry
stage and reconstructs W with the same function as the surface. All18 moving
captured cases have zero false self-shadow pixels; all72 analytic W/Z,
inside-volume, near-plane, stipple, split-view and resize cases pass with exact
Original restoration and no Vulkan diagnostics. See
`build/test-results/pstencil-resume-20261112-depth-final/` for source hashes,
bounded evidence and scratch-cleanup confirmation. This proves the tested
depth defect; live packaged-runtime verification is tracked separately in
`analysis/launcher-engine-build/pstencil-campaign-final-20261113-r3/`.

Review then exposed a second depth mismatch when temporal upscaling translates
the raster viewport. The surface subtracts that offset before reconstructing
W; the volume initially did not. Eight jittered captured triangles produced
7,984 false self-shadow pixels in total, despite all18 unjittered cases passing.
The volume recorder now transports the viewport translation in the otherwise
unused source-camera depth-row XY slots, preserving the128-byte push range and
the conventional camera's original depth row. Both point-light and lightmap
recording paths, including the owner-exclusion draw, use that correction.
The final GPU run passes all18 moving cases,12 jitter/origin cases through both
paths, and72 analytic cases, with zero false self-shadow pixels and no Vulkan
diagnostics. Nonzero viewport origins and nonbinary fractional offsets are
included. Red/green receipts are in
`build/test-results/pstencil-resume-20261112-depth-jitter-{red,green}/`.

The separate fresh component run passed anchor selection, lighting-cache,
interpolation-gate and TU1.5 admission tests. Its broader retail lighting test
stopped at a linked DLC fixture after passing12 disc maps. The test now resolves
its explicitly named imported fixture and borrows the real disc shared-map
directory, without changing production resource admission. A preflight using
the preserved Claude dependencies passes all21 maps,2,874 dynamic shader
passes and1,182 accumulation variants. Final fresh-engine qualification includes
this test; the preflight alone is not final-source qualification.

A separate live diagnostic found a huge cinematic scenery volume entering the
sun-shadow path: `hc_introcine_love/hc_cin_volume/hc_cin_volume`, owner
`e2a40035`, with852 authored vertices. Its volume darkened almost the entire
council view. Suppressing only that sun-volume draw reduced affected pixels
from approximately230,000 to18,748 while preserving other shadows. Repeated
renders matched exactly, so this was an invalid caster, rather than evidence
of stale stencil history. The bounded causal receipt is
`analysis/launcher-engine-build/pstencil-campaign-resume-20261112/shadow-council-without-sun-volume.json`.

The sun path now shares the native lightmap-shadow type0/1 eligibility gate
with its incident-light sample, both before expensive app-side queries and at
the campaign module boundary. The source reference is May2003 `001c60d0.c:81`
under `analysis/pstencil/2003-shadow-lighting/functions/`. This family gate does
not apply to point-light shadows. The real-MCC regression rejects the cinematic
volume while preserving Mercy, Regret's hologram and Elite casters. Removing
the gate fails that regression. Authored manifold compatibility and the separate
shadow-opacity owner remain outside this family-gate correction; it is not a
claim of complete May2003 shadow parity.

## Delivery qualification follow-up, 2026-11-13

The `pstencil-campaign-final-20261113-r3` attempt passed145 of147 tests but
timed out in both loading GPU cases. It did not install its candidate. A fresh
`tools/run_tests.py` build of the unchanged loading tests passed both on the
default Wayland backend (53.63s and55.12s). The same exact executable passed
both X11 controls (21.485s and38.277s), with no Vulkan validation errors in
those control logs. A debugger sample observed a GPU timeline wait during
network-transition drawing and the main thread waiting for its loading
handoff. That sample does not establish a deadlock: the default CTests
completed, and the debugger child was deliberately interrupted. The historical
120-second timeouts were not reproduced, and no production change or timeout
increase was made. See
`build/test-results/pstencil-loading-resume-20261113/diagnosis.json`.

The moving-triangle/jitter fixture now runs through main CMake as
`pstencil_volume_depth_gpu_tests` / `pstencil_volume_depth_gpu`, using the
production pStencil shader outputs. Engine qualification requires it, and
freezes/hashes `tests/pstencil` with the engine so concurrent fixture edits
cannot change the captured surface program. The focused integration passes
18 moving cases,12 jitter/origin cases and72 analytic cases, with zero false
self-shadow pixels, exact Original restoration and no Vulkan diagnostics.
Its19 recorded implementation/test inputs match the new frozen engine source
cohort. Receipts are in
`build/test-results/pstencil-volume-depth-qualification-20261113-host/` and
`analysis/launcher-engine-build/pstencil-delivery-20261113/depth-source-cohort.json`.

Live native capture acceptance now requires exact Original restoration for
every capture set. Council capture checks can also admit a prepared candidate
through the complete runtime selection reader before launcher publication.
Final package qualification, live checks and publication are recorded under
`analysis/launcher-engine-build/pstencil-delivery-20261113/`; component success
alone does not establish installation or complete-game visual parity.

That first prepared candidate passed148 CTests,27 Original and4 pStencil
packaged frame checks, and frontend startup, but its real MCC campaign pause
check failed before presenting a frame. It was not published. The new campaign
caller exposed an existing UI-loader assumption: solo introduction and Armory
caches retain a `mulg` identity without an owned payload, while the UI loader
eagerly requested multiplayer team colors and names. The allocation reader
correctly rejected that request. Previous pause coverage used Vista campaign
assets or MCC multiplayer maps and did not cover this caller/asset combination.

The UI loader now uses the authored scenario type before requesting those
multiplayer-only banks. Strict allocation validation remains unchanged. A
real-cache loader regression fails on both solo maps before the change and
passes afterward; Lockout and mainmenu retain all9 team colors and names.
This causal probe reused explicitly hashed reference archives with a newly
compiled loader/test, so it is separate from fresh package qualification.
Both probe owners were removed. The normal regression and actual-main MCC
pause/graphics/restart/HUD check are now mandatory launcher qualification,
with150 required CTests. Replacement qualification and final delivery evidence
are under `analysis/launcher-engine-build/pstencil-delivery-20261113-r2/`.

During that replacement build, a concurrent Original introduction capture
exposed `VUID-vkCmdDrawIndexed-renderPass-02684`: projected-light shadow passes
declared zero dependencies while the reused scene pipelines declared two.
The frozen renderer source matched the failing source hash exactly. The
corrected shared renderer gives both passes the same dependency declarations,
including completed depth writes before sampling. Its separate frame450 probe
reports zero Vulkan errors but failed another capture-diagnostics assertion,
so that probe is only evidence for this specific validation repair. This
delivery declared a single-file refresh in its still-active owned source/build
pair, with fresh timestamps and before/after hashes, then restarted all150
required checks. See `renderpass-cause.json`, `renderpass-refresh.json` and
`engine-refreshed/` in the replacement evidence directory.

The refreshed actual-game check passed pause (90 frames), saving Original
graphics mode (90 frames), and restart (120 frames), without Vulkan errors.
Its fourth case incorrectly expected an explicitly supplied empty HUD pack to
be ignored. Explicit HUD overrides remain supported and require a valid
manifest; the real runtime correctly rejected the missing `hud.tsv` before
presenting a frame. The case is now named `invalid_hud` and requires that exact
failure, zero frames/observations, no timeout and no Vulkan errors. The three
positive cases are unchanged. A focused check of this corrected negative
control passed; `hud-test-contract.json` records the contract and preserved
failure. The r2 handoff ended at a staging cleanup guard, and its supervisors
removed the source/build and coordinator owners. The corrected helper is now
included in fresh qualification under
`analysis/launcher-engine-build/pstencil-delivery-20261113-r3/`, using the
repository's finite command-session runner. Both earlier candidates remain
unpublished; explicit HUD asset validation remains strict.

## Published Original/pStencil follow-up, 2026-11-13

The fresh r3 package `24e55c67483a41b8b00df94a01ed21c4` is now published with
runtime SHA-256 `eb8538ef5057c20731c6785f6ff523a3fd6ac8899811611aa24323f64034e7b0`.
All 150 required CTests, 27 Original and 4 pStencil packaged smoke cases, frontend
startup and five exact-package live checks passed. The normal launcher,
pStencil launcher and campaign launcher each report that runtime. Publication
used the unchanged-root guard for both launcher binaries and both runtime
selections. Evidence and retained artifact sizes are summarized in
[r3 results](../analysis/launcher-engine-build/pstencil-delivery-20261113-r3/RESULTS.md).

The Original council control at tick 1240 had 91 ROI pixels above channel 128;
the corrected image has 0 while retaining the councillor (maximum channel 104).
The anchor ROI has no bright speck. The 640×480 comparison and 4K preview were
reviewed. Actual campaign input passed pause (90 frames), Graphics (90 frames) saving
Original, and Restart (120 frames); the fourth case correctly rejected an explicit
invalid HUD pack before presenting any frame.

Native pStencil review passed 4 complete council samples at 1239–1288 (camera 7)
and 10 Prophet samples at 3400–3494 (camera 15), including exact repeated renders,
exact Original restoration, localized remaining shadows, both actual Regret
hologram/throne layer 21 owners and exclusion of the cinematic volume caster.
The separate FSR3.1 Quality review covered 39 consecutive natural frames at
3400–3500, with 640×360 input and 960×540 output and no diagnostic repeat renders.
No runtime or Vulkan errors were reported. Fine stippled shading remains
visible; these bounded camera windows do not establish complete Xbox or
May 2003 parity, or absence of every subpixel shimmer.

The r3 retry archive retains the narrow council-window and early frame-cap
failures. Widening capture bounds within camera 7 and increasing the finite
frame cap supplied sufficient observations without changing production code,
the natural clock or acceptance gates. Subsequent concurrent Path Tracing and
native AI changes remain outside the frozen qualified cohort. Source/build,
verification/capture, publication and launcher-info scratch removal is verified.
The explicitly approved cleanup removed the failed unselected b88 package and
obsolete Claude build: 22.97 GB of allocated files were retired, with bounded
metadata retained. Eight superseded R1/R2 helpers were retired with their
commands and hashes preserved. The installed package remains retained at
5,006,866,384 bytes counting each inode once; original assets, existing
reference evidence and unrelated work remain preserved. See the
[cleanup receipt](../analysis/launcher-engine-build/pstencil-delivery-20261113-r3/cleanup-receipt.json)
and [helper receipt](../analysis/launcher-engine-build/pstencil-delivery-20261113-r3/retired-helpers.json).
