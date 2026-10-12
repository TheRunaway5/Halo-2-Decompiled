# Multiplayer shading parity

Verification covers all 21 supplied multiplayer caches and uses the original
Xbox shaders, textures and rendering routines. Native captures establish that
these paths run; they do not establish equality with a matched original
framebuffer. Moving-object BSP/instance surface queries and the retained
lighting cache are connected to the live multiplayer renderer. Exact original
dynamic-object contacts, shadows and complete frame comparisons remain open.
The [prior history-fix launcher delivery](../analysis/multiplayer-shading/native-view-history-retention/launcher-delivery/README.md)
records the fresh application and installed-path checks. The
[native history correction](SOURCE_NATIVE_VIEW_HISTORY.md) fixes the confirmed
world/first-person attachment resets. Complete original conditional clear and
owner scheduling remain separate requirements.

The subsequent [fog clear RGB correction](SOURCE_FOG_CLEAR_COLOR.md) adds
the missing original terminal fog selection and preserves the separate sky
model control and authored override. All 2,163 forced-cluster samples across
21 maps now match original clear RGB and final sky-density words bit for bit.
The separate 131ED0 packer
passes 33,984 explicit-control cases; its live caller context remains open.
Native screenshots and completed runtime checks are recorded with the
[coherent build](../analysis/multiplayer-shading/fog-clear-color/README.md).

The later [original clear scheduling work](SOURCE_VIEW_CLEAR_SCHEDULING.md)
records an actual original main-view interval, fixes admission of its x87
control word `023F`, and adds 1,306 differential clear-event cases. A version-4
Zanzibar reference fixture now drives admitted native initialization with
explicit interpolation. Complete live transition/resource scheduling and
all-map matched framebuffers remain open.

The [world impact and grenade goal](WORLD_IMPACT_AND_GRENADE_EFFECTS.md) retains
its full completion gates. Its [light-owner evidence](SOURCE_EFFECT_LIGHT_ADMISSION_PARITY.md)
now covers source lifetime/visibility/selection, small-sphere receivers, retained
mask collection, secondary part commands, opaque selection and indexed ranges.
Three raw class0/1 sphere vertex variants match the independently retained
original semantic emitter across seven resolution/split views; the floating
sphere-scissor owner matches680 unchanged original executions. These source and
isolated graphics results do not activate the live illumination producer.
The shared2CBF0 allocator, full30290 screen rectangles and complete133F70
model/section collector now have independent original comparisons, including
odd split-screen layouts. Live4B5D0 geometry/cache production and133AB0 child
orchestration remain open. Registered lights and receiver queries are reported
separately from actual geometry-light packet counts in gameplay captures.
The latest12 native impact/frag/plasma captures have zero validation errors.
The separate restored-camera Lockout comparison still differs in weather,
weapon/effect state, target history and brightness; complete appearance parity
and a new installed effects delivery remain open.

## Authority and map coverage

Retail: `assets/disc/default.xbe`, build `02.09.27.09809`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Map-pack update: `assets/map-pack/title-update/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Cache provenance: `assets/map-pack/cache-provenance.json`.
Update decompilation provenance: `analysis/map-pack-update/provenance.json`.

| Release | Maps |
| --- | --- |
| Disc | Ascension, Beaver Creek, Burial Mounds, Coagulation, Colossus, Cyclotron, Foundation, Headlong, Lockout, Midship, Waterworks, Zanzibar |
| Map pack | Containment, Warlock, Delta Tap, Turf, Backwash, Dune, Elongation, Gemini, Triplicate |

Missing source behavior is not filled with replacement colors, generic lights
or recreated texture art. An unresolved producer remains a reported gap.

## Module boundaries

| Module | Source behavior |
| --- | --- |
| `lightmap.cpp`, `bsp_lighting.hpp` | Cluster/instance buckets, packed incident vectors, resident images and checksum admission |
| `scenery_lighting.cpp`, `model.cpp` | Original placement/game gates, lighting-policy selection, authored probes, identity/checksum and source vertex attribution |
| `world_material.cpp` | Selected authored shader-LOD layer/pass/implementation selection and texture/raster state |
| `source_shader_lod.cpp`, `source_bsp_shader_scene.cpp` | Original coverage, collector byte, shader bias/threshold selection and per-view BSP part bindings |
| `source_model_shader.cpp`, `source_dynamic_model_shaders.cpp`, `source_object_geometry.cpp`, `source_scenery_scene.cpp` | Owner-sphere coverage, independent per-part model shader selection and resident ordinary/PRT banks |
| `source_model_pose.cpp`, `model_pose.cpp` | Original raw bind quaternion, local Transform and hierarchy arithmetic; actual model-order WORLD bank feeds skinning and bounds |
| `source_model_palette.cpp` | Original inverse-bind palette arithmetic used by production skinning and literal resident region maps; full collector palette/transparent queue integration remains open |
| `source_object_primitive.cpp` | Original 20-byte spatial registration primitive; successful global registration chronology remains open |
| `source_render_cache.cpp` | Original 256-record pool, owner lookup, pressure rebind, per-view LOD and transient geometry reuse controls; live ordered collector integration remains open |
| `appearance.cpp`, `shader_animation.cpp` | Retained packed object-color externs before authored animated parameter masks, with original byte-rounding precision |
| `object_lighting.cpp` | `D37F0`/`D4080` conversion through authored curves and `33980`/`19BE0` extern binding |
| `bsp_surface_lighting.cpp` | `14AC60` surface attribution and `11E800` render-triangle barycentrics |
| `bsp_collision_vector.cpp`, `object_light_query.cpp` | Original BSP/instance vector contacts and ordered object-light query rays |
| `source_object_posed_target.cpp`, `source_vector_contact.cpp`, `source_render_contact.cpp` | Complete ordinary posed-target, ordered vector-contact and render-attribution services over explicit current owner/registration/resource banks |
| `source_object_light_uv.cpp`, `source_object_light_streams.cpp`, `source_object_light_sample.cpp`, `source_object_light_rays.cpp` | Original CPU object UVs, extra radiance/incident streams, complete object sample composition and ordered ray orchestration; current cache/residency activation remains explicit |
| `source_instance_accelerator.cpp` | Authored MOPP bytecode, callback/fraction pruning and exact equal-distance instance ownership; all 382 supplied banks are validated during load |
| `object_sky_lighting.cpp`, `object_light_contacts.cpp` | Original sky radiosity fallback, object-contact metadata and attribution/rejection policy |
| `source_camera.cpp` | Observer angle/rotation, split grids, safe-area frustum and mode-4 reticle/scope anchor |
| `source_euler.cpp`, `model_pose.cpp` | Original authored angle conversion and direct local affine assembly; exact scenery/machine constructor node transforms |
| `source_frustum_camera.cpp`, `source_portal_visibility.cpp`, `source_bsp_admission.cpp` | Original per-view camera, portal-frustum traversal and BSP/instance queues with exact fog admission |
| `source_view_projection.cpp`, `render_view.cpp`, `vertex_program.glsl` | Original render-camera basis, view/projection products, direct world/first-person/reflection c0..6 and explicit complete c0..11 banks for target-independent shadow draws |
| `source_portal_mask.cpp`, `source_machine_scene.cpp` | Persistent Gemini portal ownership/masks and actual synchronized initial device/group inputs |
| `source_object_spatial_pool.cpp`, `source_object_admission.cpp`, `source_object_spatial_scan.cpp` | Original independent spatial pools, explicit lifetime journal and direct raw-pool visibility/contact traversal; live registry production remains open |
| `scope_copy.cpp`, `scope_render.cpp` | Original target16, 64x64 RGB scene copy and first-person scope bindings |
| `first_person_ik.cpp`, `reference_player.cpp` | Source marker attachment/aim counter and bounded retained-original diagnostic inputs |
| `source_shadow.cpp`, `source_shadow_submission.cpp`, `source_shadow_plan.cpp`, `source_shadow_filter.cpp`, `source_shadow_render.cpp` | Original caster arithmetic, explicit owner/candidate inputs, completed draw bindings and fixed-target Vulkan Generate/Filter/Apply lifecycle; production caster/receiver queues remain open |
| `source_device.cpp`, `source_machine_scene.cpp` | Source machine groups, ordered position/power overlays, initial live model submission and private gear ticks |
| `source_prop.cpp`, `source_prop_scene.cpp`, `source_prop_initial_state.cpp` | Original type11 constructors, selected geometry/PRT streams and retained initial lighting query bounds |
| `source_netgame_weapon.cpp`, `source_netgame_weapon_scene.cpp`, `source_weapon_shader_input.cpp`, `source_weapon_magazine.cpp` | Authored timed weapon rows, full salted native owners, literal cold constructors, six-level resident shader union and proved magazine/object-function inputs; grounded or graph-owned placement and later lifetime services remain open |
| `model_selection.cpp`, `source_script.cpp`, `scripts.cpp` | Original initial variant/permutation selection, caller-owned primary random draws and authored script continuations |
| `source_vertex_declaration.cpp`, `source_vertex_inputs.glsl` | Selected original class-0 semantic lists bind decoded world inputs; ordinary, PRT and shadow registers retain their independent declarations |
| `source_model_fade.cpp` | Complete original model/root lookup and both fade bytes over explicit owner, definition, model, metric and view-state banks |
| `source_local_player_priority.cpp` | Original observer classification, parent-root traversal and local-player comparison over explicit per-view input banks |
| `source_shadow_lighting.cpp` | Current/root default-lighting precedence and post-acquisition cache-validity selection; actual native owner/cache lifecycle remains a caller requirement |
| `source_object_render_queue.cpp` | Current-model PRT prefix swaps and first-fade-byte row flags on an explicitly admitted original queue; native global collection remains open |
| `source_cpu_skin_inputs.cpp` | Actual source part classes retain pass-specific post-skin input operations; proved reflection classes 3–5 normalize only their normal input |
| `source_weather.cpp`, `source_weather_wind.cpp`, `source_weather_initialization.cpp`, `source_weather_plate_simulation.cpp`, `source_weather_exclusion.cpp`, `simulation.cpp` | Original weather commands, retained record, first-view clock, explicit auxiliary wind, particle/plate initialization and bounded plate/exclusion services; live particle update/admission remains open |
| `source_weather_point_render.cpp`, `source_weather_point*.glsl/.vert/.frag` | Isolated original VS22 non-rain point pass with explicit current packet, resident sprite, full-surface viewport and scene depth; live game integration remains open |
| `source_weather_plate_render.cpp`, `source_weather_plate_caller_render.cpp`, `source_weather_plate.vert/.frag` | Isolated original VS21/mode3 plate pass with actual four-vertex strip, packed current depth alias and three resident layer textures; typed preparation independently derives the caller profile, full program and quad Z/W before binding; live game integration remains open |
| `source_weather_particle_simulation.cpp`, `source_weather_plate_program.cpp`, `source_weather_plate_caller.cpp` | Source ready-particle update with separately owned pre-gate elapsed store and explicit ISA results, pure mode3 compositor-program producer and original37A whole-caller profile; actual resource admission and live scheduling remain caller requirements |
| `source_weather_depth_alias.cpp`, `source_weather_depth_alias.comp` | Exact integer/stencil packing from an actual caller-owned current D24S8 image with owned nearest/clamp sampling and explicit completion-fence lifetime; live renderer uses integer D24 resources, source stencil/weather producer integration remains open |
| `source_sky_stencil.cpp`, `source_sky_stencil*` | Isolated stencil-only retained strip+literal-GS and exact-GSOut paths match the original three-draw D24/S8 chronology; live authored vertices, source owner selection and clear scheduling remain open |
| `source_prt.cpp`, `vertex_program.glsl` | Original SH sampling/rotation/PCA projection, per-section object banks, authored PRT streams and indexed vertex constants |
| `bsp_atmosphere.cpp`, `bsp_patchy_fog.cpp` | Fog classification, sky selection, visibility bitsets and patchy motion |
| `bsp_postprocess.cpp` | Per-view patchy fog/bloom resources and persistent state across resize |
| `water.cpp`, `water_copy.frag` | Authored water geometry/state and original four-tap target copies |
| `texture.cpp`, `texture_program.glsl` | Original texture/mip decoding, signed channels and dependent lookups |
| `renderer.cpp`, `render_view.glsl`, `world_depth_fragment.glsl` | Vulkan integer D24S8 resources, authored pixel conversion, per-view uniforms and source W-to-D24 encoding; source triangle reconstruction and stencil-owner integration remain separate |
| `multiplayer_runtime.cpp` | Player inputs, skies/water/scenery/machines/props, live weapon bitmap inputs and view submission |

Static layer 3 uses implementation 0 or 1 according to the authored lighting
classification; moving objects use implementation 2. Prepared accumulation,
environment, overlays, self illumination, planar fog and bloom masks retain
their selected authored pass/blend state. Complete live phase/owner ordering
and source clear history remain open as described below. The accumulation image
covers the full render target, with
original viewport constants for each view.

Water targets retain their fixed 512x256 extent and normalized sampling.
Original `3CA90` reads the raw vertical camera angle and canonical outer
rectangle ratio, with its own 1.5 widescreen factor; neither the physical
framebuffer ratio nor the expanded safe-area angle replaces those inputs.
Reflection renders the authored sky with the mirrored camera and original
projection. Patchy noise uses half the active viewport; bloom uses one quarter.
Resize preserves the four motion states and shared random state. First-person
projection/depth scale is the original 0.15. The existing full-surface W-depth
path with atmosphere now preserves initialized world depth/stencil through
accumulation, sky and first person, and loads FP accumulation color. Complete
original caller/conditional-clear scheduling remains open; see
[the bounded correction](SOURCE_NATIVE_VIEW_HISTORY.md). Held
weapons inherit the root player's retained lighting bank. Player queries run
inside each actual view before its bindings are copied. A skipped first view
does not prevent a later view from querying at its own source distance.

Original Slayer creation admits 278 of the 410 authored scenery placements,
including 200 baked and 78 dynamic objects. The dynamic objects use 114
layer-3 implementation-2 material slots and 126 independent transparent
layer-15 implementation-0 slots; unused NONE slots are excluded. Twelve
objects receive their original authored massive-object probe directly; the
remaining 66 use the source object-light query and frozen scenery cache.
The production `SourceSceneryScene` now prepares six literal collectors for
274 resolved models (196 baked and all78 dynamic). It binds each selected
collector's section incident/RGB and maintains a separate previous LOD byte
for each local player. The four Zanzibar pillars at placements19–22 retain
their labeled legacy preview until the original initialization RNG history
is supplied. Their arbitrary region choice is not claimed as source parity.
Constructor bounds are captured before animation and retained by full handle;
an explicitly admitted native animated pose refreshes regular `BDEF0` bounds
while leaving lighting frozen. The 42 multi-frame initialized idles now retain
the original iterative controller phase and loop overshoot and sample it
directly. Native clock admission and original activation scheduling remain
open. `SOURCE_SCENERY_PARITY.md` records the complete caller
contract and the all-map stream and GPU isolation checks.
Those queries use the initialized posed node0 transformed bounding offset
from `BDEF0`, rather than the scenario placement position. Ordinary player
queries likewise follow the actual native body hierarchy; the complete original
body animation/aim controller remains an independent acceptance requirement.
The live query now follows `D1850` admission, including definition/runtime
ray-mode flags, successful-retry rejection and the source bank guard. Initial
versus retained queries carry their separate original collision flags. Sky
fallback uses the selected render-view sky, gated by `14B360`'s model/flag
guard; it uses the original fixed diffuse value and static-BSP radiosity
query. Player eligibility currently consumes the source constructor biped
mode. Later character-physics, death/lunge and script/network mode mutations
remain explicit live-state requirements; grounded state is not substituted.

The live machine scene admits all 49 original placements and 48 render models,
with 37 authored position overlays, 97 ordinary and seven PRT material uses.
The 19 gears advance through the original private-group 30 Hz arithmetic;
30 door/platform records retain their original initial state while later
interaction/network and group producers remain open. PRT uses separate original cluster/PCA vertex
inputs and independently bound section coefficient banks. The complete object
section order determines one retained incident direction shared by its section
banks. No scenery lightmapping policy selects machine materials.
`SOURCE_PRT_PARITY.md` records the original metadata and arithmetic proofs.
All 20 BSP probe records retain their exact 20 lighting floats and final
packed SH/unknown shorts. Definition and variant colors are reapplied after
material selection.

The live prop scene adds all415 source-admitted BLOC placements, including
Zanzibar's main crane. The original first `BDEF0` call precedes `B8890` physics
registration, with no physics datum or admitted override. All415 retained
constructor bounds and actual native placement/node0 producers match exactly.
The runtime retains these values with full generation/definition identity;
changed roots/scales or a physics advance require the later original producer.
This is initial prop rendering, not Havok motion. The crane's captured section0
materials consume157 exact source coefficient words independently of the
ordinary prop-lighting bank. `SOURCE_PROP_PARITY.md` records the source identity,
geometry, material and physics-cache boundaries.

The version3 finite original-player image fixture additionally retains the
identified Zanzibar bigwheel/blades phase and actual root transforms. This
aligns the wheel with the retained source frame for appearance measurement;
the production gear clock continues to use its own source progression. It
also retains the exact owner sphere at object+30/+3C. Ordinary bodies, attached
weapons and first-person models share that collector's coverage; the rendered
child's bounds do not replace it. Legacy fixture versions remain readable,
but a shader-selection capture requires the recorded owner sphere. This does
not restore complete gameplay, visibility or HUD state.

The 49 automatic machine constructors establish an initial inventory, not
the later active scene. The authored syntax inventory for all 21 maps contains
no `device_set_position` or `device_set_power` calls. Triplicate's continuous
script destroys both train/death pairs, sleeps for a primary-RNG delay, creates
one pair, then sleeps again. Colossus and Elongation create and remove physics
crates; Backwash changes weather; Turf has a startup fade. These producers
must run through the actual script/host and object services. A map-name filter
or a fitted animation phase cannot replace them.

Original `209310` dispatch and native `ScriptVm` agree over 11,000 ticks and
113 lifetime/random records across ten explicitly seeded Triplicate/Zanzibar
cases. The native script host now uses the original short-range random helper;
4,822 independent original-instruction cases match both result and next state.
This helper does not initialize a stream. Full original initialization and
object-constructor draw chronology, other startup host services, and live
multiplayer script activation remain separate requirements. Original rendering
can follow zero game ticks, so a forced pre-render script tick is not justified.
Scene generations now retain their actual simulation creation tick, allowing
device reconstruction after several ticks without substituting render time.

Original `171D90` rotates the observer using the authored `matg` control
screen offset. `2F800` produces the render angle from the split grid;
`2B790`/`2F970`/`2FB70` then expand its frustum using the camera safe rectangle.
On the captured Ivory Tower view, raw vertical angle is 0.759203374 radians,
projection X/Y include the 0.85 safe-area scale, and the rendered vertical
angle is approximately 0.877663405 radians. The original mode-4 aim projects
to pixel Y=284. Physical native widescreen coordinates already include the
original display aspect correction and must not apply it twice.

Sky draws restore original `4C690` depth clamping and cull-NONE overrides.
Volume bitmap input retains its 3D image/mips and original PROJECT2D lookup:
UV divided by Q, with Z=0. It is not rewritten to PROJECT3D. Scope target16
stays 64x64 across resize; its source rectangle follows the actual viewport
and original aiming anchor. Separate scope descriptors bind it only during
first-person drawing, preserving the original bitmap fallback elsewhere.

## Source verification

| Evidence | Coverage |
| --- | --- |
| Static shader admission | 657 BSP shaders; 2,275 slots; 566 pixel and 566 vertex layer-3 variants |
| Lower BSP shader banks | All 705 lower LODs; 1,410 lighting variants, 1,656 world passes and 936 planar variants |
| Lower model shader banks | 1,267 lower records; 1,270 lighting contexts and 1,893 prepared source passes across all 21 maps |
| Model shader arithmetic | 6,793 original cases / 33,965 words; copied owner coverage and all 256 first-person collector bytes |
| Owner-cache admission | 27 original wheel/blades/crane cases, including PRT, geometry reuse and same-frame distance changes |
| Cold frozen flag | 1,024 original factory-request/binder cases; native type11 constructor state carries the proven cold flag |
| Model local/hierarchy poses | 1,109 original cases / 191,457 words; all 4,119 bind nodes across 403 models / 21 maps match 32,952 raw bind words and 53,547 original WORLD words |
| Object/first-person root ordering | 741 cold render owners / 1,032 original WORLD nodes / 12,384 inverse-bind words; 643 ordinary first-person cases / 220,020 original transform and palette words |
| Model palettes | 1,891 original cases / 208,917 words and 193 literal model definitions / 328 resident sections; 9,576 original part-center selections |
| Render-cache allocation/reuse | 5,506 original cases / 115,520 exact words; fixed 256-record pool and original partial initialization, eviction and geometry reuse |
| Fixed shadow GPU state | 24 original target/state cases; 24 captured vertex cases / 672 original outputs; 17,924 Vulkan target/sampling/depth/combiner checks |
| Retained full shadow texture | Seven actual indexed generation draws and both original softening passes; all 65,536 final RGBA bytes match the retained original receiver texture |
| Decoded WORLD shadow casters | [392 retained sample words](SOURCE_SHADOW_CASTER_WORLD_PARITY.md) match across seven original generation draws; one corroborated Chief section19 pose covers 672 vertices, with 16,128 nonposition GPU words exact and measured bounds of 0.06251 target pixel / `8e-6` depth; live activation remains separate |
| Authored world vertex declarations | 34 original layouts / 2,176 declaration words; 11,776 CPU input words and 12,288 GPU register words exact; 12,257 authored output words exact and 31 within 2 ULP |
| Extra model incident/radiance streams | 23,459 source vertices / 42 sections across all six detail levels; 217,008 expanded and 217,008 rooted posed vertices checked, including both trains and nonzero Turf radiance |
| Source-class post-skin input operations | 84 first-person model banks / 470,925 expanded class identities; 292 original-program cases / 37,376 GPU input/output words, including scoped Elite reflection normal normalization and unchanged ordinary/rigid controls |
| Complete model fades | 19,612 original executions / 39,224 exact bytes, including parent-root selection, 2,277 authored model records and two corroborated actual shadow-phase calls; live metadata/metric producers remain explicit |
| Fixed shadow view volume/secondary camera | [4,710 original cases / 443,456 exact words](SOURCE_SHADOW_VIEW_PARITY.md), including all 111 directly retained secondary camera words; mode0 and native gameplay activation remain explicit |
| Secondary shadow row classification | [5,181 original cases / 305,627 exact words](SOURCE_SHADOW_SECONDARY_PARITY.md), including five retained Chief count/row words; actual secondary queue production remains explicit |
| Secondary draw-unit sphere culling | [1,258 original cases / 41,015 exact words](SOURCE_DRAW_UNIT_CULL_PARITY.md); linear subpart/pose/mask service and 504 retained post-state unit words; pre-cull resource/collector and Havok/frustum services remain explicit |
| Shadow root lighting selection | [2,090 original cases / 10,450 exact words](SOURCE_SHADOW_LIGHTING_SELECTION_PARITY.md), including 42 same-phase existing-cache cases and independently retained Chief pointer/84 lighting bytes; native cache production remains explicit |
| Local-player shadow priority | 8,545 original cases / 42,725 exact class/AL bytes, including the actual retained Chief priority; native observer/local-user state remains a caller requirement |
| Shadow owner/candidate submission | 360 original cases / 67,965 exact words; actual parent/child/sibling and retained scratch-byte inputs remain explicit |
| Actual original shadow phase | All 135 checked words match for the actual 202-owner, 41-collector Zanzibar phase: 125 downstream words plus seven computed/corroborated fade words and three computed/corroborated priority words; actual producer banks and registration remain caller inputs |
| Shadow draw plans | All 280 retained original receiver output words match through completed draw bindings and recovered root globals |
| Production shadow vertex inputs | 80 actual Apply cases through production mapping: 1,920 nonposition words exact, eight wrapped position words at 1 ULP; poison-UV and wrong-phase controls expose incorrect register routing |
| Complete submitting-view shader bank | 722 original cases / 34,656 exact words, including physical viewport/scale/mode controls and foreground replacement; all 528 actual GPU camera words across eleven retained shadow draws match |
| Spatial registration primitives | 6,630 original cases / 33,150 words, including all 742 cold SCEN/MACH/BLOC owners |
| Spatial pool pressure and journal | 10,187 original cases / 123,929 exact words, including both restored Zanzibar pool states; all 329 frozen visibility queues match through literal pools |
| Restored ordinary object admission | 39 original cases / 8,066 exact words using 202 complete original bodies and both literal 512-head spatial pools; original allocation/registration calls are not replaced |
| Render-processed ordinary queue | 1,025 original cases / 261,562 exact words; the actual 41-owner admission, six PRT full-row swaps and 17 first-byte fade markers reproduce all 190 captured queue/association words |
| Raw-pool object-light contact traversal | 4,096 original scans / 49,786 exact words across eight real allocator banks, including orphan entries after independent allocation failures; contact backend returns are explicit controlled inputs |
| Complete retail posed object targets | [4,756 original cases / 71,296 exact words](SOURCE_OBJECT_POSED_TARGET_PARITY.md) across39 literal collision models and158 current WORLD/active banks, including16 crossed-plane/orientation controls; native live lookup, pose and registration remain explicit |
| Current object PRT contact sampling | [1,227 original cases / 8,589 exact words](SOURCE_PRT_CONTACT_PARITY.md), including eight diagnostic contacts with the literal six-row current PRT cache and 24 actual CPU SHORT points / 120 decoded words; native cache creation remains a caller boundary |
| Object PRT and baked-light combination | [2,128 source cases / 8,512 exact words](SOURCE_OBJECT_LIGHT_MIX_PARITY.md), including eight complete-D1E10 corroborations and adjacent luminance/normalization controls; prior sample services and current cache remain explicit inputs |
| Object-light CPU texture coordinates | [22,692 source cases / 45,384 exact words](SOURCE_OBJECT_LIGHT_UV_PARITY.md) across88 resident/authored/control geometry banks; literal second-descriptor byte offsets, SHORT midpoint conversion, component interpolation order and source clamp bounds; corrected by whole-D1E10 composition |
| Complete object-light sample composition | [3,456 original object branches / 24,192 exact status, output and UV words](SOURCE_OBJECT_LIGHT_SAMPLE_PARITY.md); independent geometry/PRT cache indices, both point formats, baked mixing and original texture fallback; bitmap service and current cache producer remain explicit |
| Complete ordinary render-contact query | [3,758 original calls / 52,612 attribution and 89,064 retry-packet words](SOURCE_RENDER_CONTACT_PARITY.md), plus1,476 target IDs;154 asynchronous resource-loading inputs remain explicitly unresolved |
| Complete ordered object-light rays | [3,351 complete original calls](SOURCE_OBJECT_LIGHT_RAYS_PARITY.md), including26 actual current-PRT count-zero status1 controls;32 separately labeled sample-return controls verify outer status/write handling, with339,630 defined comparison words overall;407 kernel-dependent paths remain unresolved |
| Complete MAP object-light samples | [2,534 unchanged original calls / 34,708 exact words](SOURCE_MAP_LIGHT_SAMPLE_PARITY.md): 2,438 actual ready contacts and 96 explicit special-part controls; correct retained-target writes, diffuse UV ordering and 88 instance CPU incident selections. Actual resident images are explicit inputs; their live selection is separate. |
| Bitmap request and partition state | [Original request/readiness composition](SOURCE_BITMAP_RESIDENCY_PARITY.md) and [partition selection](SOURCE_BITMAP_PARTITION_PARITY.md), tested with explicit handles, clocks and cache banks; no source residency history is inferred from dimensions or capture appearance. |
| Authored shared low-detail images | [All 35,442 records across 21 caches](SOURCE_BITMAP_LOW_DETAIL_PARITY.md): 23,157 present descriptions, 115,785 original header words, 15,720 cube-layout words and 1,200,056 decoded planar/cube bytes. All 147 volume decodes remain explicitly unsupported. |
| CPU bitmap image selection | [1,629 original cases / 16,290 exact words](SOURCE_BITMAP_CPU_SELECTION_PARITY.md), including explicit inline-header construction boundaries; flags0 cannot allocate or pump. Native readiness/history remains a caller input. |
| Bitmap coverage | [7,942 original cases / 23,826 exact words](SOURCE_BITMAP_COVERAGE_PARITY.md); original reciprocal and logarithm producer, with actual layer15 caller/stage-bank evidence. No fitted screen metric or selected mip. |
| Bitmap storage initialization | [4,246 cases / 160,446 exact words](SOURCE_BITMAP_INITIALIZATION_PARITY.md); admitted storage constructors and pre-protection stage reservation. Allocation success and initial whole-title budget are separate. |
| Async queue and read protocol | [6,483 queue cases / 3,623,481 words](SOURCE_ASYNC_QUEUE_PARITY.md) and [8,706 request/read/completion cases / 112,058 words](SOURCE_ASYNC_READ_PARITY.md), with explicit acquired/leased ownership phases. No kernel wait or worker schedule is supplied. |
| Actual native file reads | [1,216 construction cases / 21,888 observed/inspected words](SOURCE_BITMAP_ASYNC_READ_PARITY.md), 54 authored ranges / 1,454,108 actually read bytes and high-offset/EOF/seek controls. Original callback-store inspection is distinguished from executed prefix outputs; done does not prove a complete read. |
| Cache busy/state/deletion callbacks | [2,024 source cases / 131,681,440 exact raw-bank bytes](SOURCE_BITMAP_MANAGER_CALLBACKS.md); actual source fences, done bytes and budgets remain inputs. Deletion/state services do not create readiness. |
| Texture physical reservations | [392 complete original calls / 25,503,520 exact raw-bank bytes](SOURCE_BITMAP_RESERVATION_PARITY.md), including eviction, rollover and full-pool failure; source ranking/age/busy inputs remain explicit, and successful reservations require separate same-salt image binding. |
| Weather particle and plate initialization | [1,548 particle calls / 201,719 words and 1,544 plate calls / 362,840 words](SOURCE_WEATHER_PARITY.md), with authored arrays from all three weather maps. Initial auxiliary chronology, later simulation and GPU admission remain explicit. |
| Complete original snow submission | [Actual391F0 entry/return](SOURCE_WEATHER_DRAW_SUBMISSION_AUDIT.md): 1,728 original instructions, all8,872 source-written bytes, 238 ordered points and a7,680-byte command span; exact current four-level sprite binding. No SDK/callee result is supplied. |
| Isolated native snow pass | [952 cases /60,928 exact CPU words and four GPU target sizes](SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md), with source pixel program, sampler, GEQUAL0 alpha, W-depth and retained-binding controls. Controlled scene response is distinct from original scene appearance. |
| Scene game context | 2,160 original factory cases; explicit Slayer game type2 / engine2 removes 114 excess automatic scenery candidates across 20 maps |
| Scene creation attempts | 13 original controlled constructor cases and ordered native BSP reconstructions; successful source allocation/child order remains open |
| Object colors and animation | 11,597 original cases / 2,180,236 words, including overlapping masks and byte-rounding boundaries |
| Instance stipple | 29,400 physical Xbox pixels; all 17 native GPU levels and exact source layer/frame gates |
| Other world passes | 164 accumulation, 88 layer-7, 11 layer-2, 2 layer-8, 71 layer-15, 10 layer-17 variants |
| Lightmap decoding | 186 buckets, 426,329 incident words, 721 clusters, 1,846 instances |
| Baked vertex lighting | 91,802 source samples; all 200 supplied scenery placements |
| Radiance calculation | 1,280 original instruction fixtures, bit exact |
| Atmosphere/visibility | 2,163 original fog/bloom/visibility fixtures; 2,202 spawn locations |
| Patchy fog | 5,760 sequential original frames across four authored tags |
| Water | 288 original fixtures, 17,568 components; maximum numerical difference zero |
| First-person projection | 48 original fixtures, 768 constants, bit exact |
| Object lighting | 4,116 original fixtures across all 21 maps; 82,320 components; maximum difference 4.47e-8 |
| Light-query render attribution | 2,096 original barycentric, 2,053 packed midpoint and 4,111 unequal-vector interpolation fixtures, bit exact |
| BSP/instance contacts | 5,692 BSP and 3,021 instance original instruction fixtures, bit exact |
| Equal-distance contact admission | 20 original source cases / 606 contact and ownership words; 41 individual queries and 69 endpoint controls reproduce the actual Headlong clipping boundary; 51 map queries resolve all 17 real tied rays through authored accelerator order |
| Authored instance accelerator | [382 banks / 5,348 traces](SOURCE_INSTANCE_ACCELERATOR_PARITY.md), all 35,556 opcode visits exact and all 60 reachable opcode types exercised; 29 separate actual tie traces also match |
| Complete static map contacts | [13,229 original contacts / 195,429 contact and callback words](SOURCE_INSTANCE_ACCELERATOR_CONTACTS.md), including all 2,641 ordered callbacks and 135 duplicates across all 21 maps |
| Authored spawn light queries | 4,404 ordered queries: 4,403 attributed and one source-confirmed Triplicate render-attribution miss |
| Object cache cadence/interpolation | 2,304 original fixtures, 87,552 components; maximum difference 5.96e-8 |
| Object-query admission and initial biped mode | 5,122 `D1850` and 65,536 `E5980` original fixtures, bit exact |
| Sky lighting fallback | 3,160 `D1A30` radiosity fixtures and 2,308 combined original static-BSP queries across all 22 sky references, bit exact |
| Camera/scope helpers | 20 original grids, 408 angle and 576 scope fixtures, bit exact |
| Observer/frustum/aim projection | 864 original instruction fixtures; zero anchor mismatches, maximum float difference 6.01e-8 |
| Volume header/mip dimensions | 864 original fixtures, 3,456 components, bit exact |
| Scenery gates/selectors/probes | 34,188 creation gates, 32 actual lighting-selector chains and 1,944 original probe-match fixtures, bit exact |
| First-person marker/aim/action helpers | 7,232 original instruction fixtures, bit exact; 10,344 world vertices against independent original local inputs, bones, weights and palette within 1.90735e-6 |
| Original model skinning | 36 compiled influence variants; 224 original part/count selectors; all 4,980 hand basis outputs against original GPU outputs within 5.36442e-7 |
| Original packed model inputs | 3,295 independent original vertices; all 14 position/UV/basis words on 10,344 expanded SMG/arm vertices bit exact |
| Held weapon attachment | 252 authored unit/gun/hand combinations; 4,096 unnamed and 4,096 named alignment fixtures plus 4,096 parent-local conversion fixtures, bit exact |
| Initialized scenery controllers | All 278 admitted placements, including 73 original controller initializations and 205 graph-NONE placements |
| Shadow fades | 1,728 consumer fade-byte and 24,000 root eligibility/death/camouflage fixtures, bit exact |
| Shadow candidate history | 3,200 sequential original fixtures; 439,038 retained-record components, bit exact |
| Object bounds and frames | 4,096 transformed-sphere fixtures and 4,096 object-frame fixtures; 16,384 and 53,248 components, bit exact |
| Shadow screen coverage | 4,096 original projected-diameter fixtures, bit exact |
| Retained first-person inputs | 23 original words bit exact; 70 active constant records across seven pass families, including the arms environment pass |

Machine-readable evidence is under `analysis/multiplayer-shading`. Existing
battle-rifle scope/display and pose evidence remains under `analysis/appearance`
and in `FIRST_PERSON_PARITY.md`.
`source-shadow-live-phase.md` records the separately captured original
335E0→3290A phase and its precise producer boundaries. Its version3 checks 135
words, including computed/corroborated fade and priority. Its CPU-owned banks
are distinct from the retained original20 GPU replay.
`SOURCE_SHADOW_CASTER_WORLD_PARITY.md` records the seven retained caster suffixes
and the one fully corroborated native pose slice; its measured GPU position
bound does not claim identical silhouette coverage. `SOURCE_NATIVE_SHADOW_ACTIVATION.md`
identifies the remaining actual owner, cache, history and secondary draw-unit
producers. Multiplayer does not yet submit an optional shadow plan. The
original player-count policy separately disables ordinary object shadows for
two, three and four local players.
`SOURCE_OBJECT_RENDER_QUEUE_PARITY.md` records the separate immediate-admission
and render-processed stages of that retained main queue, including the temporal
corroboration of uncaptured service pages. No native allocation or live collector
activation is implied by the reconstructed original state.

`SOURCE_VERTEX_DECLARATION_PARITY.md` and
`SOURCE_WORLD_VERTEX_DECLARATIONS.md` record the original declaration and extra
stream evidence. Triplicate's train shaders read radiance/PRT cluster/weights
from v5/v6/v7, whereas plain PRT reads cluster/weights from v5/v6. Both trains
also carry entirely black radiance streams; absent-stream white was incorrect.
The literal selected declaration and source bytes now reach the production
shader. `live-train-side-declaration-before-verified/train.png` and
`live-train-side-declaration-after/train.png` retain the same explicit camera
before and after this correction. The camera isolates the train outside the
normal world visibility volume, producing a black background. These are native
inspection images, not matched original gameplay frames. Provenance records the
camera, executable, shader, cache and capture hashes. The verified before
capture routes shader file reads to the copied executable's matching original
shader snapshot; its open trace records those paths. It is byte-identical to
the earlier before inspection image. The executable and shader bytes are
unchanged by that capture-only file routing.

`SOURCE_CPU_SKIN_INPUTS.md` records the independent original section/part class
selection and the scoped Elite reflection correction. Its real Renderer
inspection retains authored passes, textures and constants at 640x480 and
1280x720; fresh repeats are identical and before/after RGB is neutral. This is
an input/arithmetic correction, not evidence of a visible appearance change.
`tools/capture_train_inspection.py` reproduces the isolated train camera with
explicit immutable executable/shader routing and capture provenance.

## Reproduction

```sh
cmake -S . -B build-mp-shading -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-mp-shading
ctest --test-dir build-mp-shading --output-on-failure
python3 tools/verify_multiplayer_shading.py --validation --resolutions
```

The production capture sweep selects original Slayer weapon option 5 (SMG),
giving every map the same authored match configuration. Midship/Waterworks
retain their default energy weapons; unfinished energy combat is not replaced
with ballistic behavior. Reports record configuration, binary/shader hashes,
each capture and validation diagnostics.

## Validation snapshots

The ordinary-declaration/extra-stream production snapshot is preserved at
`analysis/multiplayer-shading/source-declaration-stream-snapshot`. Its executable
SHA256 is `0d85c45777e615bbf63d72c69ea2f26d685fd0f9917f6e4f2b4234d693d1cd3d`.
The matching shaders and file hashes are retained with it. Its all-map report,
`live-source-declaration-stream-all-maps/summary.json`, records 78/78 passing
captures with unchanged binary/shader hashes and no Vulkan/synchronization
diagnostics. Coverage is all 21 maps at one/two/four players and five effect
maps at 480p, 1080p and 4K. This is execution and scaling coverage, not matched
original framebuffer equality. The later first-person source-class audit is
complete; its correction and validation boundaries are recorded in
`SOURCE_CPU_SKIN_INPUTS.md`.

`live-source-declaration-stream-reference/comparison.png` uses the same preserved
binary/shaders and the version-3 retained Zanzibar camera/player/device fixture.
Its world and first-person diagnostic regions retain mean absolute channel
differences of 0.873614583 and 0.667061192 out of 255, respectively (1,438 and
340 channels above eight). The original process is untouched; timer, HUD,
complete simulation/controller/visibility history and physical sampling remain
outside a full-frame equality claim. `tools/capture_multiplayer_reference.py`
records the exact command, source inputs, image regions, shader routing and
artifact hashes for reproducible comparison.

The later source-class/collector snapshot is preserved at
`analysis/multiplayer-shading/source-cpu-skin-collector-snapshot`, executable
SHA256 `9907a4892e553588a947ff637a61f695f7921512a310aae7971d3ea768b9b712`.
Its fresh comparison is `live-source-cpu-skin-collector-reference/comparison.png`;
both measured regions retain exactly the preceding diagnostic differences.
`live-source-cpu-skin-collector-all-maps/summary.json` records 78/78 passing
captures, unchanged binary/shader hashes and no Vulkan/synchronization diagnostics.
Its fresh train inspection and before/after image are retained in
`live-source-cpu-skin-collector-train`; the explicit camera/visibility boundary
is recorded in the capture provenance. The first
245-test attempt passed 242 checks and exposed two missing explicit validation
environments and one unbuilt concurrently added test. The two GPU tests now
enable their required layer/synchronization environment through CTest; all
three corrected and two added collector checks pass together (5/5). The real
Elite Renderer sanitizer inspection passes at both extents; GPU leak freedom
remains outside that address/undefined-behavior check.

The subsequent `bsp_collision_vector.cpp` correction changes only the second
ambiguity probe: it queries the full source segment, then compares the returned
fraction exactly. One-ULP clipping missed another actual Headlong instance at
the same fraction; those owners produce different lighting. The correction
retains the selected contact and marks ownership unresolved, without guessing
the accelerator's first owner. Original callback tests and normal/sanitizer
checks pass; the all-21-map surface/spawn outcomes remain unchanged.
`SOURCE_INSTANCE_CONTACT_TIES.md` records the actual cases and remaining
accelerator-order boundary. The complete rebuilt normal suite including this
correction finished with 246/251 passing checks in
`source-instance-tie-full-suite.log`; its five failures shared the held-plasma
menu's missing source extern37 context. The independently recovered zero
camouflage input is now supplied by that menu caller. A fresh coherent rebuild
passes all five failed checks and the new extern37 regression (6/6), recorded
in `source-menu-repair-tests.log`. This focused repair does not replace the
versioned full-suite record.

The later authored-accelerator implementation replaces membership iteration
and unresolved-tie probing with the original MOPP interpreter and collector
fraction updates. All 13,229 checked contacts and 2,641 callback records match
unchanged source execution. Eight tied-ray ownership results are corrected;
all 13,212 spawn-ray results are preserved. Its frozen production snapshot is
`source-instance-accelerator-snapshot`, executable SHA256
`28b50440232164a8087f5f02fe75c15219b941adc481625354cd4d45b90d1a21`.
Fresh Headlong captures pass at one/two/four players; the fresh retained
Zanzibar comparison preserves the preceding world/first-person diagnostic
measurements exactly. These captures are retained under
`live-source-instance-accelerator-headlong` and
`live-source-instance-accelerator-reference`. The complete fresh
`live-source-instance-accelerator-all-maps/summary.json` records 78/78 passing
captures over all 21 maps, split-screen and the five resolution effect maps,
with unchanged frozen executable/shader hashes and zero graphics or
synchronization diagnostics. Full original-frame equality remains unclaimed.
The six added/migrated source
boundary checks pass together in `source-accelerator-shadow-boundaries-tests.log`.
Independent normal and sanitizer checks also pass for the accelerator,
complete contacts, secondary camera and root-cache selection. Native gameplay
shadow activation remains separate from these explicitly supplied-bank helpers.

The subsequent coherent normal rebuild passes all 261 registered checks in
`source-accelerator-full-suite.log` (568.72 seconds). Its tested executable
SHA256 is `c352b61602d689170731bf5aee8e906ad39279655ff16dc8071aa66db8447dda`.
This includes the corrected menu caller and the authored instance accelerator,
shadow-view, lighting-selection and secondary-classification boundaries. The
later object-vector, draw-unit-culling and current-PRT-contact helpers were
registered after this suite began. Their first coherent focused checks pass
together (3/3) in `prt-contact-focused-tests.log`. A later coherent rebuild
passes all five current PRT, object-light mixing, draw-unit culling,
object-vector and model-attribution checks in
`object-light-services-focused-tests.log`. The current PRT helper checks
1,227 complete contact cases / 8,589 exact output words plus 24 retained
CPU-short vertices / 120 exact words; object-light mixing checks 2,128 cases /
8,512 exact words. Both pass address/undefined-behavior checks with leak
detection in `prt-contact-asan-tests.log` and `object-light-mix-asan-tests.log`.
The original bloom screen-context helper also passes its registered normal
check in `bloom-context-root-tests.log`; its separate full-library normal and
sanitizer runs are recorded in `bloom-context-native-validation.json`.
These later focused results do not extend the version boundary of the earlier
261-check run. Passing this
suite and the separate 78 captures does not establish full original-frame
equality.

Copied executables retain an absolute compiled shader directory. Snapshot
captures therefore use the explicit capture-only file-routing shim in
`analysis/multiplayer-shading/native_shader_snapshot.c`; it forwards file reads
to the recorded shader snapshot without changing rendering calls or bytes.
Reports retain its library hash and source/target directories. The traced
pre-correction train capture verifies the actual opened snapshot paths and
remains byte-identical to the previous inspection image.

Earlier production captures, normal/sanitizer checks and original/native GPU
comparisons are retained in
[MULTIPLAYER_SHADING_VALIDATION_HISTORY.md](MULTIPLAYER_SHADING_VALIDATION_HISTORY.md).
They remain evidence for those recorded versions, rather than validation of
later source changes.

## Retained appearance replay: sky order and player bloom

The immutable original20/older native RDC replay identified two concrete
producer mismatches without changing guest state, images or source textures.
Original sky8097/8102/8110 follows the authored sky_dome/clouds_low/moon
stream, while material-ID sorting reversed that stream. Renderer now preserves
contiguous background runs for ordinary and reflection skies. The actual
Zanzibar asset test verifies the180/96/6-vertex source order and reverse-order
negative control;640x480→1280x720→640x480 fresh/reused captures are byte exact
and report no SDK/synchronization diagnostics.

The sleeve red residual was added after the matched final hands environment
pass. Source13870 calls245C0 in reset mode, supplying the original player
screen context factor.75 to its.9 threshold producer. Native postprocess had
omitted that context. The recovered context now gives source C6=0x20ACACAC,
while context absence retains0x20E5E5E5. Blur radii/taps and final strength
already matched and are unchanged. The complete original producer/oracle is
recorded in `SOURCE_BLOOM_CONTEXT_PARITY.md`.

A fresh matched-camera/player/device native image with both corrections is
`analysis/multiplayer-shading/live-sky-bloom-source-fix/original-native-phone.png`.
Its binary SHA256 is
`13a8ec4f5766f5d079f7d07b276b7b9c0ffa2bc6d65e1391ed7c5bca13a7f440`;
the executable and matching SPVs are preserved in that directory's `snapshot`.
`provenance.json` and `comparison.json` retain exact inputs and raw image hashes.
The world diagnostic MAE changes.873614583→.738937500 RGB bytes, with channels
above8 decreasing1438→827. First-person MAE changes.667061192→.619677939,
with channels above8 decreasing340→32 and maximum error31→17. At(330,468),
original RGB(58,21,16) now compares to native(59,22,16), previously(27,17,14).
These are measured regions of a retained diagnostic, not full-frame parity.

Original cloud scroll c22/c23 phase differs from near-start native render
time. That source clock has not been restored or fitted. The separate purple
source8787 draw is now [identified exactly](ORIGINAL_ENERGY_BLADE_DRAW_PARITY.md)
as the third-person energy-blade section0/part0, with32 strip indices and12
compression words exact. The retained owner palette independently matches
all12 consumed transform words. Native placed/netgame weapon construction,
lifetime and rendering are being completed; that draw remains absent from
the recorded both-fix capture. Complete live histories, physical sampling and full original
frame equality remain open. `APPEARANCE_RESIDUAL_DIAGNOSIS.md` records the
actual traces, source/native version boundaries and remaining limits.

The coherent canonical sky/bloom build is separately frozen in
`sky-bloom-canonical-snapshot`, executable SHA256
`b72073102f00d75cec3c41cbc39b1909d028353c6a8af0cfee5cc1787007723a`.
Its fresh `live-sky-bloom-all-maps/summary.json` records78/78 passing captures
over all21 multiplayer maps, one/two/four players and the five effect maps at
480p/1080p/4K. Executable and all43 shader hashes remain unchanged, with zero
graphics/synchronization diagnostics. The canonical authored-sky GPU check
passes in `sky-order-canonical-gpu-tests.log`; the bloom-context and raw object
light-stream checks pass in `sky-bloom-canonical-focused-tests.log`.
These native captures do not supply matching original views for all maps.

## Source-born levitated weapons and capture history

The [netgame weapon scene](SOURCE_NETGAME_WEAPON_PARITY.md) now admits31
source-proved levitated, graph-NONE stock rows across all21 caches. It uses
the original row phase, authored pose/scale, full salted native owners and
resident geometry/shader-LOD union. Grounded rows, graph-owned alignment,
weighted collections and later pickup/replication remain separate services.
The cold carbine uses its actual18/18 magazine initialization and authored
primary-ammunition identity function, so both display overlays receive1.0.
The source unattached camouflage input and original lower environment variant
are also retained; no colors or programs are substituted.

The coherent executable and all43 SPVs are frozen in
`analysis/multiplayer-shading/netgame-ammo-canonical-snapshot`, executable
SHA256 `3733f0f7cc9c877bb993bae02029c58f6d9a3003e080a8efc414016d295acce0`.
`live-netgame-ammo-all-maps/summary.json` records78/78 passing captures,
all21 maps at one/two/four players and the selected effect maps at480p,
1080p and4K. All jobs capture actual tick31, reaching every admitted row
phase; tick6 only reaches the earlier sword/reference rows. Executable/SPV
hashes remain unchanged and all graphics/synchronization diagnostics are zero.
The seven new sampler/contact/ray/weapon checks pass in
`netgame-ammo-canonical-focused-tests.log`.

The separately retained-camera/player/device tick6 comparison is
`live-netgame-ammo-reference/original-native-phone.png`. Its measured world
region MAE is.722656250 RGB bytes (788 channels above8), and first-person
MAE is.619661836 (32 channels above8). These diagnostic regions do not
establish full-frame equality. Shader phase, resident texture selection and
complete original motion/HUD/startup histories remain explicit boundaries.
Subsequent source edits are not covered by this recorded capture version.

## Current MAP sampler build

The corrected MAP sampler is wired to all six production player/scenery/
machine/prop/netgame lighting callers. The special-part retained branch receives
the actual target sample, and instance CPU incident decoding uses the original
resident geometry stream independently of the GPU lightmap bucket.

The executable and 43 SPVs are frozen in
`analysis/multiplayer-shading/map-sampler-bitmap-canonical-snapshot`, executable
SHA256 `50e19d78ce321b87a470ea9b5db7942b7a769e98cf074b61a0de1288944612cf`.
Its retained-camera/player/device tick6 comparison is
`live-map-sampler-bitmap-reference/original-native-phone.png`: world-region
MAE .720812500 RGB bytes, 781 channels above8; first-person MAE .619677939,
32 channels above8. Neither region establishes full-frame parity or restores
the original animation, HUD, shader clocks, residency or startup history.

The complete 299-test run passed296 and failed3. All494 recorded executable,
shader, fixture and command inputs remained unchanged during the run. The
shadow renderer failed with device-loss result-4 and passed on a same-build
retry. The team-scoring test had compiled before its fixture-column update;
recompilation passes the unchanged fixture. The menu test skipped profile
sign-in; its corrected authored title/profile/lobby route passes all existing
window, player, resize, match and validation checks. The six focused bitmap/
team/menu checks pass in `bitmap-selector-canonical-focused-tests.log`.
The versioned logs are `map-sampler-bitmap-canonical-full-suite.log` and
`map-sampler-bitmap-shadow-retry.log`. All-map tick31 captures for this snapshot
passed78/78 in `live-map-sampler-bitmap-all-maps/summary.json`: all21 maps at
one/two/four players, plus selected effect maps at480p/1080p/4K. Actual tick31,
executable/SPV hashes and zero validation diagnostics are recorded explicitly.
These native captures do not compare original framebuffers for all21 maps.

## Resolution boundaries and frozen follow-up build

[Integer view rectangles](SOURCE_RESOLUTION_RECTANGLES.md) now feed the live
world viewport/scissor, c8..c11, postprocessing, water crop, player projection
and scope input. Odd split-screen dimensions previously combined fractional
viewport sizes with independently truncated effect sizes, omitting the last
row or column. All160 original12FA0 banks match; all80 standard-resolution
layouts retain their prior values. Original render-scale/mode2 services and
fixed128x128 shadow targets remain separate.

`resolution-bitmap-canonical-focused-tests.log` records7/7 passing checks,
including310.86 seconds of fresh-versus-reused GPU resize comparisons across
odd dimensions,720p/4K and one/two/three/four views. Every submitted final
row/column is processed. These are native extension/source-bank consistency
checks, not claims about physical Xbox odd display modes.

The battle-rifle idle-pose GPU fixture retains its authored lens environment
pass and digit textures. `live-bitmap-selector-battle-rifle/provenance.json`
records100 digit pairs,139 actual simulation firing/reload ticks, six count
captures and four target resizes through4K. The restored36 image is byte equal
to the first; all validation diagnostics are zero. No matching original
battle-rifle framebuffer or complete controller history is asserted.

The subsequent coherent source copy is
`analysis/multiplayer-shading/resolution-bitmap-frozen-source/project`:
all4,087 copied source/fixture/tool files were stable during copying and after
compilation. Its native executable SHA256 is
`d2d1a336303e9d8d67889acc9f4d22966d7ececa5ce50aad3477faf27b1a9812`.
This includes the CPU bitmap selector, coverage and initialization helpers;
their original clocks, allocation, I/O and live residency remain explicit
caller services. All309 registered tests pass in875.38 seconds in
`resolution-bitmap-frozen-full-suite.log`. All509 recorded command/shader
inputs and all4,087 copied files remain unchanged after the run, recorded in
`resolution-bitmap-frozen-source/post-suite-hashes.json`.

The fresh retained-camera/player tick6 comparison for this tested executable
is `live-resolution-bitmap-frozen-reference/original-native-phone.png`:
world-region MAE .720020833 RGB bytes,774 channels above8; first-person MAE
.619677939,32 channels above8. The recorded boundary still excludes complete
original animation/HUD/startup/shader clocks and resident selection history.

The same tested executable passes21/21 additional actual tick31 captures at
1281x721 with three players in
`live-resolution-bitmap-frozen-odd-three-player/summary.json`. Executable/SPV,
capture tool and all21 map hashes remain unchanged. This includes the live
player/projection/scope/BSP/fog/postprocess paths at an odd split-screen size;
it is distinct from the controlled fresh-versus-reused renderer regression.

The same executable subsequently passes all21 single-player tick31 captures
at1920x1080 in `live-resolution-bitmap-frozen-one-player-1080/summary.json`.
All executable/SPV, capture-tool and map hashes remain unchanged and there are
no graphics or synchronization diagnostics. These are native appearance
captures; original framebuffer comparison was not performed on those21 views.

The subsequently registered queue/read/file/manager helpers have a separate
frozen source check in `analysis/multiplayer-shading/async-core-frozen`.
All four registered focused tests pass in.57 seconds; all14 copied source and
fixture files remain unchanged. This is recorded in `validation.json` and
`async-core-frozen-tests.log`. These modules were not registered in the earlier
309-test rendering snapshot and do not activate its bitmap residency policy.

The subsequently registered reservation and weather-initialization helpers
pass their separate three-test frozen source build in
`analysis/multiplayer-shading/reservation-weather-frozen` (.11 seconds).
All12 copied source/fixture inputs remain unchanged. Both weather fixtures
require the full recorded corpus; empty, comment-only, truncated-row and
last-complete-row-removed controls reject. Independent reviews find no
implementation mismatch, including90 additional particle and81 plate
floating-point classification controls. These helpers do not activate live
streaming or weather rendering and were not part of the earlier309-test build.

The extended helper build in
`analysis/multiplayer-shading/weather-binding-header-frozen` passes all seven
registered checks (.33 seconds), with all28 copied source/fixture inputs
unchanged. In addition to reservations and initialization, it covers complete
same-salt image binding (787 cases /789 original calls /16,227,940 bank bytes),
pending header construction (19,472 complete calls /97,360 words), plate
simulation (1,810 admitted cases /615,400 words), and weather exclusion
(4,421 complete calls /178,629 immutable input words). This build predates the
completed pending request prefix described below. This separate helper build does
not establish live streaming or weather GPU activation and does not change
the identity of the earlier309-test rendering snapshot.

The pending request composition is now registered and passes the extended
`analysis/multiplayer-shading/weather-request-frozen` build: eight checks
(.24 seconds), all36 copied inputs unchanged. Its complete original title-side
prefix compares248 calls,18,635,712 bank bytes and6,944 result words. It reaches
242 actual free-mutex acquisition boundaries and six completed source failures.
It preserves every preceding reservation, binding, header, eviction and request
counter write. It supplies no acquired job, kernel return, file completion,
ready-image publication or successful pending caller return. The corpus also
passes56 ownership/phase guards and full sanitizer/corpus admission checks.
See [pending composition](SOURCE_BITMAP_PENDING_PARITY.md).

Particle simulation has an explicit numerical boundary: the unchanged
`RSQRTSS` instruction at38B02 produces different words in Unicorn and on the
current native CPU for all eight recorded controls. Neither establishes
physical Xbox results. `weather-rsqrt-numerical-boundary.json` records this;
no fitted reciprocal or native full38450 replacement is activated.

The new actual38450 entry/return and directly captured38B02/38B08 instruction
pair independently corroborate the software-emulator result: operand4012FB04
produces3F28ED69. Complete unchanged replay executes506,927 instructions and
matches all41,120 nonstack source-written bytes and the compared return state.
Only the first reciprocal-square-root pair has its own guest instruction
snapshot; the remaining1,008 pairs come from the complete corroborated replay.
Physical Xbox precision remains unresolved.

The native ready-particle helper requires an explicit ISA producer and preserves
the separate384E5..384FD elapsed store before the resource-ready service. Its
46-call corpus matches33,306 words,2,134 ISA pairs and7,479 auxiliary draws,
with378 rejection checks and full address/undefined/leak sanitizer coverage.
The pure mode3 plate-program builder matches2,440 complete original calls and
153,720 result words, including660 source failures preserving the preceding
program/depth sentinels. It does not generalize the existing graphics module's
one-captured-program admission. Details are in the
[particle simulation record](SOURCE_WEATHER_PARTICLE_SIMULATION_PARITY.md) and
[plate program record](SOURCE_WEATHER_PLATE_PROGRAM_PARITY.md).

`cmake/SourceWeatherSimulation.cmake` registers both explicit-input helpers as
`halo2_weather_simulation`. Its four source-fixture checks pass atRelease/O3 in
the separate `weather-cpu-cmake-integration` project (.02 seconds). That project
compiles the source exclusion helper as its only cache support; it does not
rebuild or activate the live game. This is separate from the earlier309-test
snapshot and six graphics checks.

An additional actual36B50 entry/return captures the complete caller chronology.
Unchanged replay runs552,742 instructions through5,290 distinct title addresses
before the genuine NV2A queue read at3FA886/addressFD800044. No MMIO value or
service return is supplied. All seven named producer banks (system, owning
weather, particle render/state, auxiliary random stream, elapsed clock and
intensity) agree with the independent actual return. The SDK tail, cleanup,
full caller return and GPU consumption are not replay-proven. See
[the caller capture](SOURCE_WEATHER_CALLER_CAPTURE.md).

The current-depth alias producer is now independently verified on Vulkan.
It copies D24 integers and stencil bytes separately, packs through an integer
storage view, and supplies its UNORM sampled view directly to the native plate
pass. All307,200 alias pixels match original event215778 exactly; source depth,
stencil and target alpha remain unchanged. The composed cloud result is
byte-identical to the earlier native plate result, retaining its measured
one-byte color boundary. Odd extents, all byte values, five source layouts,
sampling, current-image changes and fence reuse pass. Normal and sanitizer
address/undefined checks record zero SDK synchronization errors. Full graphics
leak checking remains externally noisy and is reported separately in
[the alias record](SOURCE_WEATHER_DEPTH_ALIAS_PROTOTYPE.md).

The three additive CMake modules now pass a combined14-check private build in
`weather-pipeline-cmake-integration` (4.02 seconds), with323 files bound to their
independent source/reference records. The full application is configured only.
The new alias library also builds with the standard sanitizer option. This
combined record keeps the preceding CPU/graphics validations distinct; it
does not add weather to the running game. The separately registered connected
reference passes two additional checks: current D24 → alias → plate → snow,
with all307,200 depth/stencil/alpha pixels preserved and exact original
plate-to-snow input continuity. Its measured final color/coverage limits remain
explicit in [the connected record](SOURCE_WEATHER_CONNECTED_SCENE_PROTOTYPE.md).

The later full-native resource migration selects actual integer D24S8,
graphics+compute+present capability, transfer-source usage and separate
combined-attachment/depth-only sampled views. Stencil is stored and loaded
through main continuations. A coherent4551-file private source build passes
all21 single-player1080p and all21 three-player1281x721 captures with actual
depth/stencil readbacks and no SDK synchronization warnings/errors. The retained
Zanzibar comparison records actual image diagnostics; its fractional shader
clock remains unfixed, so small RGB changes cannot be attributed solely to
the resource change. First-person regional metrics are identical to its D32
baseline. This preserves existing native zero
initialization, while dedicated sky writes, conditional source clear ordering
and live weather producer admission remain open. Details and the separate
resource/resize boundaries are in
[the native D24 migration](SOURCE_WORLD_D24_NATIVE_MIGRATION.md).

The subsequent isolated GPU proof tests every16,777,216 integer W-depth input.
The former native division stores N+1 for6,291,455 inputs on the RTX5080;
the exact source power-of-two/next-float expression preserves every N. Both
production world and light-volume fragments now use that source helper, with
their existing admission/clamp behavior retained. This corrects a proved
encoder discrepancy without claiming complete source interpolation or original
all-map appearance. See [the exhaustive encoding record](SOURCE_DEPTH_ENCODING_GPU_DIAGNOSTIC.md).
The final coherent encoder/LOAD-dependency build passes another42/42 native
map captures,36 persistent resize cases through4K, and the independent scenery
lighting/isolation check. Its4551 source inputs, binaries, shaders, actual
depth/stencil readbacks and historical stages are frozen in the separate
[native migration record](SOURCE_WORLD_D24_NATIVE_MIGRATION.md).

A fresh4,991-file current-source Release build now delivers these runtime
changes through both canonical launchers. Its separate validation passes
all21 maps at1920x1080 and all21 at three-player1281x721 through the preserved
title/update-data wrapper,36 persistent resize cases through4K, the scenery
lighting/isolation check and six focused CPU checks. Both installed launcher
paths pass new representative captures. Normal intro behavior, the existing
title TU1.5 selection, old package bytes and desktop paths are preserved;
launcher replacement is atomic and reversible. The actual child application,
wrapper/core, shader, map and external update-image identities are recorded
separately from the capture tool's wrapper identity. See
[the delivery record](../analysis/multiplayer-shading/native-shading-launcher-delivery/README.md)
and [the coherent build](../analysis/multiplayer-shading/native-shading-delivery/README.md).
These are native execution/resource/layout checks. They do not establish
all-map original appearance, and the retained base-image Zanzibar comparison
is a separate fixture from the TU1.5 launcher suite. Integer capture ticks
still do not fix fractional shader time.

Typed plate preparation now validates source-derived caller admission, all60
program words and every quad Z/W pair before modifying retained bindings.
Normal/address-UB GPU checks preserve prior native scene output exactly;
19 rejected input controls preserve bindings, and52,832 test-only combiner
samples across1,651 source definitions have zero rounded-byte disagreements.
The latter proves arithmetic under explicit inputs, not original rasterization.
See [the typed preparation record](SOURCE_WEATHER_PLATE_TYPED_PREPARATION.md).
The current weather CMake integration separately passes21/21 normal and21/21
sanitized checks, with identical readbacks and zero SDK synchronization errors.
Its13 CPU and8 GPU checks include caller/typed admission, legacy linking and
connected reference composition. Source95 and artifact382 records are bound
in [the integration report](../analysis/multiplayer-shading/weather-typed-cmake-integration/README.md);
this still does not activate live weather producers.

The retained original20 sky producer now matches all307,200 D24/S8 pixels for
A8097/B8102/moon8110 on both retained VSOut-strip+literal-GS and exact-GSOut
paths, independently and as a single-fence chain. Normal/address-UB outputs
agree,17 raster comparisons/controls and58 ownership/input rejections pass,
and SDK synchronization reports no errors. Exactly462 original max-depth
samples change stencil00→FF; later draws retain them. This bounded endpoint
proof does not establish every sky edge under a different depth field or
authored native vertex/color production. Its original caller/clear plan remains
separate from live activation. See [the stencil-only proof](SOURCE_SKY_STENCIL_PROTOTYPE.md).

The [original view/clear audit](SOURCE_VIEW_CLEAR_POLICY_AUDIT.md) now pins
the ordinary caller's initialization before accumulation, shared depth/stencil
across targets0/1/3/18, dedicated sky after world lighting, and the late/FP
layer14 target-preservation rule. The subsequent
[bounded native correction](SOURCE_NATIVE_VIEW_HISTORY.md) preserves initialized
shared D24S8 in the existing full-surface W-depth path, loads FP accumulation
color, and moves main sky after world/shadows and before fog. Existing native
full-frame/per-view initialization remains; the old Renderer fails the new
depth-reset regression and the corrected Renderer passes four layouts with
1,359,455 protected world-depth samples. Its42 native map captures, focused
resize/scenery checks and six CPU checks pass. Complete original132D0 behavior
still needs actual caller/header, transition/override and owner-admission
inputs together with initialized coverage after allocation/resize;
`full_surface`, atmosphere and a vertex span do not supply those source fields.
This correction activates no original sky stencil or weather writer and does
not establish all-map original appearance equality.

The subsequent original weather reference uses independent copied disks and
EEPROM in `analysis/multiplayer-shading/original-weather-20261009/session1`.
The original title is navigated through its menus and outdoors on Lockout using
ordinary keyboard input. `original-lockout-gl-session.png` is a complete original
game screenshot from xemu0.8.136/OpenGL llvmpipe. This is a software-emulator
reference, with no physical Xbox rasterization claim. All retained source-disk
and EEPROM hashes/mtimes remain unchanged after capture.

`original-lockout-gl-source_capture.rdc` records the original rendering context
through the standard RenderDoc API called by its parent host debugger. Guest
instructions and graphics calls are not replaced. This is a bounded recording
containing multiple frames, not a claimed single frame or the earlier238-point
CPU entry. The separately frozen event215873 point draw has1,015 vertices,
actual first0/count1,015 range, all12 camera vectors and actual scene pre/post
surfaces. Its four sprite mip readbacks match all5,440 decoded bytes of the
unchanged1,408-byte resident BC3 partition. It changes1,255 color pixels while
preserving every alpha and depth byte.

`weather-point-original-reference-event215873/packet.json` holds this distinct
reference. `tools/freeze_source_weather_renderdoc_point_packet.py` requires exact
float-color byte recovery and checks the original range, shader tokens, sampler,
viewport and render state. `tools/freeze_source_weather_depth_reference.py`
retains all307,200 original D24 integers and verifies packed-lane/row mapping
against nine actual depth samples. It supplies no scalar-depth approximation.
The native D24 upload/readback and matched point result are a separate check;
the earlier controlled point readbacks do not establish that comparison.

The optional snow module also has its own private normal and sanitizer builds.
Its source/program/fixture/executable hashes and graphics diagnostics are in
`weather-point-prototype/manifest.json`. Review corrected the actual alpha
contract to rounded8-bit GEQUAL0, added pre-mutation rejection of unsupported
vertex outputs/device point sizes, and restricted recording to the proved full
surface viewport. Its four target banks change rectangle constants with the
captured projection and packet fixed. They do not establish full live weather
resolution scaling, adapted camera state, rain coverage or plate composition.

The actual point scene comparison is now complete as a bounded measurement:
306,471 of307,200 RGBA pixels match exactly,729 differ, and13 source-only
effect pixels remain at point-square edges. Every source D24 integer, stencil
byte and target alpha byte is unchanged. The required upper-left point
coordinate conversion follows the original GL raw-row convention. Unchanged
original-program post-VS replay confirms the edge owners' screen centers and
floor W; a particular hardware point-rounding rule remains unproved.
See [the matched scene record](SOURCE_WEATHER_POINT_SCENE_PARITY.md).

The preceding plate event215778 is frozen independently with the actual
four-vertex strip, scroll constants, depth alias and all seven mips of each of
three layer images. All307,200 alias pixels exactly match the original packed
D24/stencil lanes. Unchanged original3A5C0 mode3 execution matches all60 pixel
definition words and all17 captured packed constants. The isolated Vulkan
plate differs at14,477 pixels, each by at most one RGB byte; every depth,
stencil and alpha byte remains exact. Seven unsupported packet controls and
five viewport/scissor controls reject while retaining the preceding valid
binding. No color correction is applied. Normal and address/undefined sanitizer
runs pass; graphics LeakSanitizer reports the same3,156-byte/29-allocation
external teardown leak in the full draw and empty-context control. CPU leak
detection is clean. See [the plate record](SOURCE_WEATHER_PLATE_GPU_PARITY.md).

`cmake/SourceWeatherGraphics.cmake` registers the explicit-input
`halo2_weather_render` library and its shaders. Optional
`HALO2_WEATHER_REFERENCE_TESTS` builds six CPU/GPU checks using these retained
original captures, with strict CPU floating-point options. The full project
configures successfully; a separate CMake build against the earlier coherent
cache archive passes all six checks in3.05 seconds. Its manifest verifies all113
independently frozen input/reference files and records the compiled artifacts
and validation environment in `weather-graphics-cmake-integration/manifest.json`.
This registration does not activate weather or rebuild the complete game.

## Acceptance requirements still open

- Live dynamic-object registration, contact/cache activation, evolving
  breakable state and later object mode/flag producers. Complete ordinary
  query, posed target, render attribution and object-sample services are
  source-verified over explicit current banks. Ordinary
  BSP/instance surface sampling, retained samples, update cadence and
  interpolation are integrated; no nearest-triangle substitutions are used.
- Complete later door/platform scripts, power-group/network transitions,
  active model permutations and moving-object contacts. The source initial
  machine/prop rendering, private gear progression and PRT lighting are live.
  Later prop physics bounds, motion/controllers and network lifetime require
  their actual producers; initial constructor bounds are not a live backend
  substitute.
- Complete live weather particle updates, cluster/portal admission, admitted
  render time and resident publication. Command/checkpoint/clock, auxiliary wind,
  particle/plate initialization and the isolated non-rain point pass have source
  evidence. The isolated plate pass also has the actual packed depth/layer
  evidence above. Rain graphics and complete live integration remain open.
- Complete dynamic-light and shadow GPU producers, attachment/radius inputs
  and actual per-view visibility/ownership flags. The fixed 128x128 target10
  Vulkan Generate/Filter/Apply module and completed draw-plan bindings are verified;
  original owner/candidate helpers require actual collector order, model fade,
  priority, retained scratch/history and receiver queues. These producers are
  not yet connected into a complete live shadow result.
- Complete the original first-person motion/secondary-transition input chain
  beyond the independently matched SMG idle frame and marker attachment.
- Complete Gemini's later automatic door controls, shared-group transitions
  and BSP/script activation. The source initial owner/group state, tick-driven
  portal mask and per-view BSP/instance portal admission are integrated.
- Complete dynamic-owner portal linkage, partial section/subpart visibility,
  occlusion and object-transparent sorting, including activated
  cached/residual patchy-fog branches.
- Compare matching original/native frames and investigate NV2A sampling and
  reciprocal/transcendental precision. Arithmetic/Vulkan validation alone
  does not certify hardware interpolation or fixed-point rounding.
- Complete the source bitmap allocator, coverage/request producers, readiness
  chronology and hardware/shared-image selection in live rendering. The
  separately authored low-detail images are decoded and the request/partition
  arithmetic is source-verified. Loading those assets does not establish that
  the original would select them at a particular frame.

`MULTIPLAYER_ATMOSPHERE_RESEARCH.md` records atmosphere source addresses and
precise fog, effect and visibility verification boundaries.

The latest effect-owner checkpoint adds whole133AB0 object effect/child
orchestration joined to native133F70/2CBF0/44940/2D000/449E0/30290:90 original
calls,869,429 exact retained words and223 ordered callee boundaries match.
Original4B5D0 outputs/contexts remain explicit inputs to that comparison, so
live cache/node production and publication are still open. The controlled
12-capture gameplay sweep passes Vulkan validation and same-aspect720p/1080p
count/RNG checks, while its separate grenade lighting gate fails: admitted frag
receivers still have zero submitted raw/packed light packets. This remains
partial source parity; the exact evidence and screenshots are recorded in
[the world-effect checkpoint](WORLD_IMPACT_AND_GRENADE_EFFECTS.md#controlled-capture-checkpoint).

The next model-resource checkpoint replaces cache return/write replay with
native allocation/rebind and current-color refresh:259 original cases and
4,255,403 retained words match, including476 ordered platform requests and213
native acquisitions. Successful selections feed the native node-count owner;
whole model/node input resolution separately matches182 cases/4,027 words.
All13 focused source regressions pass. These helpers still require live
model-build, resource-publication and gameplay draw integration; see
[their exact scope](SOURCE_MODEL_BUILD_RESOURCE_PARITY.md).
