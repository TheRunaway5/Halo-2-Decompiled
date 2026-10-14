# World impact and grenade presentation

Native terrain contacts now deliver their original global material, contact
plane, projectile identity and game tick to a separate world effect owner.
That owner selects the authored primary, sweetener and secondary effects,
creates clipped surface decals, and plays the original particle materials,
textures, emission, birth and movement definitions. Actual frag and plasma
detonation callbacks use this same owner. Particle simulation advances once
per 30 Hz game tick; their per-view particle draw preparation consumes no random
values. Weather and effect creation share the application's auxiliary RNG, so
different aspect ratios and local-view counts can change their shared history.

This is partial source parity. Explosion lights now submit their authored BSP
geometry passes. Dynamic object receivers, complete original effect admission
and a matched original/native explosion gameplay frame remain open.
The screenshots below are native gameplay captures, not original comparisons.

The visual authority is **OG Xbox TU1.5**, never Vista. Earlier base-disc Xbox
fixtures remain diagnostic evidence of their named routines only. They do not
meet the current TU1.5 matched-frame acceptance gate. The unchanged TU1.5 image
is authenticated by SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The [current TU1.5 impact work](TU15_IMPACT_PRESENTATION_20261013.md) fixes
pre-selection noise/falloff, initialized default responses and admitted prop
damage-material/vitality inputs. Its executable fixtures, current runtime
captures, all-map admission counts and remaining full-goal work order supersede
historical status statements below.

## Organization

| Owner | Responsibility |
| --- | --- |
| `bullet_impact.hpp`, `Simulation::trace`, `projectile_material_response.cpp` | Actual terrain/posed-prop contacts, original response/noise/scale consumers and retained native presentation events |
| `PropSandbox::damage_contact` | Admitted parentless prop damage material and post-hit vitality scale B; distinct region/material selection remains open |
| `material_effects.cpp` | Original material parent fallback and visual sweetener selection |
| `world_effects.cpp` | Nonlooping events, locations, particle banks, clipped static surface decals and retirement |
| `effect_part_control.cpp` | Original part flags selecting the retained effect A/B light control |
| `effect_light_lifetime.cpp` | Salted350-slot standalone light pool, birth, activation, updates and retirement |
| `effect_light_registration.cpp`, `source_light_spawn_contacts.cpp`, `source_sphere_position.cpp` | Persistent light bounds, actual terrain correction, BSP location, spatial membership and removal |
| `source_draw_unit.cpp`, `source_bsp_draw_collector.cpp` | Shared original draw/override/section allocation and BSP requests; live dynamic collector production remains separate |
| `source_object_draw_collector.cpp` | Complete133F70 model/section requests into that shared allocator, with explicit actual context/body/weapon inputs; upstream live4B5D0 remains separate |
| `source_object_presentation.cpp` | Whole133AB0 effect copies, shared section counters, parent mutations and child/weapon ordering, joined to native133F70 in reference tests; actual4B5D0 cache/node production remains explicit |
| `source_model_resources.cpp` | Whole3D4F0/137650 model/node inputs and4C190/4C2B0 section/LOD selection, with native shared-cache allocation/rebind and colors; live model-build and resource publication remain separate |
| `source_model_build.cpp`, `source_model_node_bank.cpp`, `source_model_lighting.cpp` | Whole4B5D0 reference join with native raw nodes/palettes and3DB00/3DFE0 cache chronology; deep pose/query/conversion and live collector integration remain separate |
| `source_object_render_entry.cpp` | Whole133790/1332F0 entry routing, native cached-sphere coverage/distance/fade, alternate/first-person flags and typed native4B5D0 binding; live query/resource owners remain separate |
| `source_effect_light_geometry_frame.cpp` | Joined native secondary BSP collection, culling, command reset/generation and layer12 sorting; current main prefix and dynamic/object owners remain explicit |
| `source_geometry_resource.cpp` | Whole223C70 array/byte/vertex-buffer fixups over actual owned ready allocations; also used by weather after its type0 validation; geometry allocation/IO/cache lifetime remain separate |
| `source_draw_resources.cpp` | Original44940/449E0/2D000 metadata, payload and lighting-classification owners over current banks; actual12DE70 publication remains a platform input |
| `effect_light_draw.cpp`, `ModelView::raw_effect_lights` | Raw source geometry/light packet and Vulkan handoff, with explicit real texture target and per-view constants |
| `particle_birth.cpp`, `particle_creation.cpp`, `particle_emitter.cpp` | Original numerical leaves shared with existing particle presentation |
| `particle_collision.cpp` | Collider arithmetic over an explicit native query adapter |
| `particle_sprite.cpp` | Shared packed-color vertex adapter used by world and muzzle particles |
| `particle_renderer.cpp`, original generated vertex programs | Vulkan particle material/texture/alpha-test submission |
| `multiplayer_network.cpp`, `multiplayer_runtime.cpp` | Native event transport, once-only delivery and per-view drawing |

The native 64-contact/detonation retention banks and visual pool capacities
are transport/allocation choices. They do not establish original Xbox pool
capacity, global allocation order, RNG stream ownership or admission parity.

## Source evidence and regressions

Evidence is retained under
[`analysis/multiplayer-shading/world-impacts-20261010`](../analysis/multiplayer-shading/world-impacts-20261010).
Each oracle directory includes pinned executable/map hashes and fixture hashes.

| Evidence | Scope and result |
| --- | --- |
| `material-oracle/report.json` | Unchanged TU1.1 `189E10` and u5 counterpart: 8,736 calls per image; material fallback and visual output slots 1/3/5 match exactly. Distance admission is bypassed through explicit ABI input. |
| `collider-oracle/report.json` | Unchanged TU1.1 `2BF6E7..2BF838` and u5 counterpart, including original collision-frame helper: 2,048 controlled contact responses; complete 64-byte particle output matches. No query or contact admission parity is asserted. |
| `shape5-oracle/report.json` | Unchanged retail `249170` with the actual Lockout hard-terrain emitter: 2,048 births; particle and RNG bytes match. |
| `plasma-shape1-oracle/report.json` | Unchanged retail `249170` with the actual plasma explosion ring emitter: 2,048 births; particle and RNG bytes match. |
| `original_particle_submission` | Vertex attribute 8 matches the byte expansion of the original packed ARGB pushbuffer word. NV2A expands this to BGRA; the original class 7/8 program then selects `.zyx`. The former RGBA adapter reversed red and blue. |
| `native_world_impact_and_grenade_effects` | Actual Lockout SMG contacts, decals, frag callback, LAN serialization, retirement and identical particle bytes when five game ticks are delivered per render update. |
| `native_world_plasma_grenade_effects` | Actual Midship spawn inventory, normal throw, terrain contact and plasma detonation; all eight authored systems admitted, electrical textures present, per-view drawing leaves particle state unchanged. |
| `native_world_effect_multiplayer_maps` | 1,260 actual SMG contacts across all 21 multiplayer maps. Every map creates particles and decals; this covers one real firing trajectory per map, not every surface or shader. |
| `multiplayer_grenade_delivery` | SDL Q pulse through native UDP and real gameplay action/fuse/blast. The close blast uses pitch -1.5 so the corrected material-dependent bounce stays nearby. |
| `native_authored_footsteps` | 154 original sound-selection cases plus actual Spartan/Elite walking in one/four views and real SDL audio submissions. The BSP importer resolves support materials once; presentation consumes the resulting global index directly. |

The red runs for the reversed color and render cadence bugs were retained in
the work logs. The first `resolution-captures` directory predates the cadence
fix and is failed evidence. Use `resolution-captures-v2` for the verified runs.

The earlier scoped delivery passed23 focused tests. At that delivery, the normal
and title-first launchers selected `build-online-20261011/halo2_native`. This is
historical delivery evidence; it does not describe current launcher selection
or delivery of the later light-owner changes. That executable/shaders,
contemporaneous source snapshot and diagnostic logs are retained in
[`final-evidence/manifest.json`](../analysis/multiplayer-shading/world-impacts-20261010/final-evidence/manifest.json).

## Resolution captures

All three input sequences were captured at 640x480, 1280x720, 1920x1080 and
1280x720 with four local views. Each uses genuine look/fire/Q inputs and fixed
render interpolation zero. No player pose, contact, grenade or particle was
injected into the gameplay state.

| Sequence and tick | Contacts | Detonations | Decals | Particles |
| --- | ---: | ---: | ---: | ---: |
| Lockout SMG, 16 | 8 | 0 | 6 | 28 |
| Lockout frag, 101 | 0 | 1 | 0 | 24 |
| Midship plasma, 53 | 0 | 1 | 0 | 78 |

Counts agree across all four layouts. The 12 captures have no Vulkan validation
errors. Existing unused vertex-output performance warnings remain in the logs.
The plasma sequence is deliberately close to the player and can obscure most
of the view; it is not an appearance acceptance reference.

[`resolution-captures-v2/manifest.json`](../analysis/multiplayer-shading/world-impacts-20261010/resolution-captures-v2/manifest.json)
records every command, screenshot/log hash, binary hash and shader hash. Its
`frozen` directory retains the captured executable, shaders and input scripts.
The executable's shader directory is compiled in; restoring a capture requires
the retained shaders at that build path.

Reproduce on a working X11 display with the validation layer available:

```sh
SDL_VIDEODRIVER=x11 DISPLAY=:94 \
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/nvidia_icd.json \
VK_LAYER_PATH="$PWD/.tools/validation/usr/share/vulkan/explicit_layer.d" \
LD_LIBRARY_PATH="$PWD/.tools/validation/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
python tools/capture_world_effects.py \
  --native build-world-effects-20261010/halo2_native \
  --gameplay-xbe /absolute/path/to/owned/TU1.5/default.xbe \
  --output analysis/multiplayer-shading/world-effects-new-captures
```

The private display is a host capture dependency, not a rendering substitute.
These captures used the NVIDIA Vulkan driver. They do not verify the desktop
Wayland resize path or physical Xbox sampling/rasterization.

Script rows are `tick player fire reload [grenade yaw pitch [move forward right]]`.
Movement is bounded to [-1,1] and belongs only to its listed tick. Look direction
persists until the next look row. The optional movement fields allow ordinary
throw-and-retreat captures without assigning player or projectile state.

## Remaining source parity gates

1. Light parts now retain tick-owned definitions and standalone scalar/color
   state. Exact original comparisons cover tick updates, sphere shape/spatial
   bounds, six clipping cameras, view fades, color binding, c31..39, sphere
   clusters and BSP/instance admission with main-view auxiliary queues. All
   149,796 I8 attenuation bytes match an independent xemu unswizzle of the
   authored six-mip volume. The explicit Vulkan diffuse draw seam passes16
   controlled layouts, including odd dimensions, resize restoration and four
   views, with exact additive-blend and per-view constant isolation results.
   This uses the actual frag definition, both textured and texture-mapping
   target variants, layer12 materials, vertex programs, samplers and PROJECT3D
   attenuation lookup on controlled geometry. Every authored grenade/BSP LOD
   binding across all21 maps passes:1,430 present layers,1,294 absent layers
   and4,290 passes across three color shifts, with no unresolved bindings.
   An additional32 Vulkan layouts use the original frag/plasma tick-zero
   colors, brightness and size with both shader variants. Bloom strength is
   explicitly zero for that isolated additive-pass check. Blend and per-view
   isolation errors are zero. The128 captures and executed binary/SPIR-V
   are retained in `light-gpu-authored-colors-v5`; this is controlled receiver
   evidence, not matched original gameplay appearance.
   The whole original sphere accelerator and draw-unit paths independently
   match7,296 and2,282 calls respectively. Original render admission,
   preselection and single visibility visits now have independent comparisons;
   see [their exact scope](SOURCE_EFFECT_LIGHT_ADMISSION_PARITY.md).
   Complete light-list collection and the four-record local fallback now
   match256 original executions over explicit spatial heads and pool records.
   Light spatial storage now preserves original stride12 forward/reverse
   pools, independent exhaustion, orphan entries and removal chronology:
   1,536 source registration/removal operations match284,800 exact words.
   Native terrain contact generation, complete spawn-position correction and
   standalone `C2D00/C3220` registration now match the original on actual
   Lockout geometry:512 correction cases and128 registration/removal pairs.
   Native world light parts now use the original350-record salted pool and
   standalone birth/update/register/unlink/retirement path. Whole original
   replay matches932 complete lifetime checkpoints and33,790,592 words,
   including untimed lights with nonpositive duration and out-of-range colors.
   The2,048-case original A/B part-control prefix also matches and replaces
   hardcoded light control1. Actual frag/plasma and render-cadence integration
   tests pass. The per-view owner now matches256 original checkpoints and
   6,456,064 exact words. Small-sphere receiver preparation matches69 whole
   original queries and58,278 owned/camera words; native production uses the
   actual containing cluster, main auxiliary queues and nested query epochs.
   Shared-mask culling matches1,027 original cases and586,479 words. The joined
   BSP collector/cull replay matches48 cases and784,966 words with native-loaded
   Lockout resources. Live global main/dynamic draw ownership, larger-bound
   portal receivers, ordered geometry submission and lens flares remain open.
   Particle bloom does not establish this illumination path.
2. The native collision backend uses imported triangles/capsules. Havok,
   original query filters, visual collider child effects and unit sticking remain
   separate. Actual posed prop contacts now emit impact packets; unit contacts,
   region/material routing and moving-prop decal attachment remain open.
   Part, material and collision sound events now reach the once-per-tick audio
   queue independently of view count. Original `2BF380` calls the `2490D0` sound leaf
   after bounce arithmetic and then sets the same first-contact flag. Rejecting
   those systems had dropped rock debris on Triplicate, Turf and Warlock.
3. Effect A/B scaling, alternate/stochastic projectile responses, all looping
   and parent coordinate modes, swarm/wind controllers, original pool/RNG
   order and compound decal variation need their original owners. Decals use
   clipped static triangles; original recent-24 overlap admission is missing.
   Standalone light ownership now follows original `C0350`: attached owner
   NONE, with the delivered projectile/contact identity retained separately
   as `effect_owner`. Original-only128-birth evidence and actual frag/plasma
   native integration tests verify this distinction. World lights now allocate
   and register through the source lifetime owner; attached parts and shared
   chronology with the other original light creators remain separate owners.
4. Plasma detonation-start effects and dynamic illumination need separate
   delivery. Original sound control/pitch-range helpers and actual SDL delivery
   are verified in bounded tests; original spatial/admission, effective driver
   pitch and RNG ownership remain open. The completed detonation callback
   uses projectile `+FC`; plasma `+F4` is a different airborne effect with no
   particle systems and cannot replace the ground effect.
5. Capture matched original/native gameplay frames with the same map, camera,
   timing, material response and effect state. The isolated original shader
   fixtures and native resolution sweep do not close this gate.

Keep the existing frozen original clear-scheduling comparisons unchanged.
Complete the source owners above before describing these effects or all-map
shading as fully 1:1.

Current source/replay evidence is retained separately under
`analysis/multiplayer-shading/world-effects-goal-20261011/`; the older
`final-evidence` delivery above is immutable and predates these additions.
The earlier capture sweep in `raw-light-allocator-gameplay-v1` passes all12 impact/frag/plasma
layouts pass with zero Vulkan validation errors. Current impact counts are
5 decals/20 particles, frag24 particles/one registered light, and plasma78
particles/one registered light. The earlier table records the immutable older
delivery, whose impact counts were6/28. Frag now prepares per-view receivers;
the close plasma light's cluster NONE fails original visibility admission.
The pending light draw path remains unsubmitted. These captures do not close
the matched original-frame appearance gate or establish current runtime delivery.

## Ordered completion gates

1. Retain the original light lifetime, visibility, candidate selection, receiver,
   shared-mask, secondary BSP commands, opaque selection and indexed range owners.
   Their bounded source checks pass. Literal raw class0/1 sphere shader comparisons
   also pass; see [the admission evidence](SOURCE_EFFECT_LIGHT_ADMISSION_PARITY.md).
2. Complete the raw Vulkan light handoff, including the real base-color target,
   projected3D attenuation, independent per-view constants and source draw order.
   Then connect actual live main/dynamic production, material visibility/resource
   ownership and shared-arena chronology. The gameplay producer is still open.
   The handoff now passes15 controlled raw-geometry GPU layouts, and the shared
   allocator passes75 whole original calls. These close service boundaries only;
   they do not create actual live collectors or illuminate gameplay geometry.
   Complete30290 also matches2,228 original rectangles; the repeated allocator
   comparison now consumes this native rectangle owner.
   Whole133F70 now also matches73 original calls/695,958 retained words and
   uses the same native allocator/rectangle; upstream live geometry remains
   open. Capture logs now count raw/packed geometry light
   packets separately from registered lights and receiver queries. The optional
   `capture_world_effects.py --require-light-packets` gate rejects an admitted
   BSP receiver view that has no lighting packet; packet presence alone still
   cannot establish a matching original framebuffer.
   The subsequent native resource join matches100 allocator cases/952,504
   words and repeats all73 collector cases using native body/weapon/metadata/
   classification/payload lookups. Actual12DE70 publication and live node/cache
   ownership remain open.
   Whole133AB0 effect/child orchestration subsequently matches90 original
   calls/869,429 retained words,183 native collection calls and40 explicit
   upstream model-build boundaries. It joins native133F70, allocator, resources
   and rectangle owners; the model-build output/context boundary is still
   explicit and does not establish native4B5D0 production.
   The model-resource/cache join now passes259 cases/4,255,403 words,476
   ordered resource requests and213 native cache acquisitions, including
   allocation/rebind and current-color refresh. Whole3D4F0/137650 additionally
   passes182 cases/4,027 words. Successful selections feed native137650.
   Cache-result/write-patch boundaries are removed;12DE70 publication,
   live4B5D0/node production and gameplay submission remain open. See
   [the exact model-build scope](SOURCE_MODEL_BUILD_RESOURCE_PARITY.md).
   Whole4BCC0 admission also passes367 cases/6,068,710 words, including
   four PRT resource calls,267 native prior-cache acquisitions and30 scalar
   calls. It preserves first-person/world selection, visibility, threshold
   loads, overlays and region sort order. Publication/scalar services and
   live4B5D0/node production are still separate. All17 focused regressions pass.
   The subsequent whole4B5D0 join passes63 calls/3,557,610 words and672 ordered
   owner events, including native admission/cache/resources/nodes/allocation
   and lighting-cache chronology. It removes420 replayed lighting-cache write
   words; query/conversion results remain explicit leaves. Standalone raw node
   construction passes170 calls/525,130 words; lighting routing/cache chronology
   passes160 calls/2,632,960 words. These do not close live collector integration,
   raw class2/3 consumers or the zero-packet gameplay illumination gate.
   The next join implements the separate main/secondary query arena resets,
   persistent visibility initialization and40E90 bank reset. Their40-case
   comparison retains646,780 words. Whole secondary40E90 adds79 cases/1,869,039
   words. The combined native BSP collect/cull/command/layer12 sort path passes
   39 cases/1,846,744 words without replaying intermediate output snapshots.
   Empty dynamic/object queues and the retained31-unit main prefix remain
   explicit source inputs. This is not live geometry-light packet production.
   Whole133790 entry routing adds110 cases/13,530 retained words with native
   coverage/distance/fade/1332F0 and explicit4B5D0/133AB0 record boundaries.
   The typed build adapter independently repeats the63 whole4B5D0 calls.
   Full223C70 fixups add114 cases/1,607,057 resource words, including16 actual
   ready allocations. Its native shared implementation replaces weather's
   duplicate type0 stores. Resource allocation/IO/cache completion and live
   main/dynamic production are still required before raw packets can be drawn.
3. Complete the remaining contact, attachment, A/B, audio, RNG and child-effect
   owners listed above. Close larger-bound/object/first-person light consumers.
4. Capture matching original/native effect states and compare final frames at
   the required resolutions/views. `light-camera-comparison-v1` restores only
   camera/projection and shows remaining visual differences; it is not acceptance.
5. Freeze the validated complete runtime and install both launcher routes. Later
   source/service checks have not yet established a new effects delivery.

The active goal retains all five gates. Passing a numerical or isolated graphics
check does not close the corresponding live gameplay or matched-frame gate.

## Controlled capture checkpoint

`controlled-object-collector-gameplay-v4` retains12 actual gameplay captures,
all with zero Vulkan validation errors, from binary
`06d2f3afd608d3e4912d49557d4ef2cc0f71bd60648d8b14b98e1076a7a561c3`.
Each run explicitly seeds the actual application auxiliary RNG with305419896
and presents one30Hz simulation tick per frame. These controls are accepted
only for finite, isolated local input-script captures; production startup RNG
and normal frame cadence are unchanged. Captures freeze the binary, SPIR-V,
input scripts and validation-runtime hashes. This runtime predates the later
unused133AB0 helper and is not an installed effects delivery.

| Case | 640x480, one view | 1280x720 and1920x1080, one view | 1280x720, four views |
| --- | --- | --- | --- |
| Impact | 8 contacts,3 decals,13 particles | 8 contacts,5 decals,19 particles | 8 contacts,6 decals,27 particles |
| Frag | 1 detonation,25 particles,1 registered light | Same counts | Same counts |
| Plasma | 1 detonation,78 particles,1 registered light | Same counts | Same counts |

The720p/1080p pairs match both effect counts and the final primary/auxiliary
RNG words for each effect. Aspect/view changes alter the original weather
coverage and shared auxiliary consumption; their separately retained captures
are not equal-history resolution comparisons. The failed unconstrained and
mixed-aspect comparisons remain in earlier logs; no private production RNG was
restored to make their counts agree. The failed controlledv2 capture also
retains the stale weather point-range rejection from a source change during
the build; the rebuilt current live weather path completes all12 captures.

`controlled-live-light-packet-gate-v1` deliberately fails. Its frag light is
visible/selected, with two cluster and six instance receivers at640x480, but
both raw and packed geometry-light packet counts are zero. All four frag
layouts in the completed sweep likewise lack lighting packets in the selected
view. The particle images therefore do not close the illumination or matched
original-frame gates. Plasma's close light still fails original visibility
admission in this scripted state. No launcher route was changed by this check.

The later `lighting-cache-gameplay-v1` records three completed one-view frag
captures at640x480,1280x720 and1920x1080. Its fourth run was interrupted when
the X server disappeared; it is not counted as complete. The independent
`lighting-cache-four-view-resume-v2` completes1280x720/four views. All four
completed runs have one detonation,25 particles,one registered light and zero
Vulkan validation errors, but every view still has zero raw/packed geometry
light packets. The four-view selected light has three cluster and six instance
receivers. The post-capture verification confirms the resume binary and all52
shader hashes match the earlier frozen runtime. Its binary is
`6d69be75bcbe7488671b8dc661e35655e7ee49dc0fcd15c748cd20c0d13aad5e`.
This runtime predates the subsequent reset/frame/object-entry helpers; no
installed-launcher or matched-original-frame completion is inferred.

The subsequent `geometry-resource-gameplay-v1` completes all four frag layouts
in one sweep from frozen binary
`c82a130262215497935cbbbc5025478367063f4d0f7b0047dee180be61ce87be`.
All runs have one detonation,25 particles,one registered light and zero Vulkan
validation errors.720p/1080p effect counts and both RNG words match. Four-view
and4:3 weather histories remain separately retained. The selected light reaches
two cluster and six instance receivers in each selected view, but raw and
packed geometry-light packets remain zero. This is a fresh native particle
capture after the shared resource fixups, not an illumination or original-frame
parity result. The task-owned Xvfb server was terminated after completion.

`geometry-request-source-regressions-v4.log` records33 passing original-owner,
model, light, resource and weather checks. The complete native request owner
now joins the verified223C70 fixups, while acquisition, promotion, yield and
pump are explicitly recorded lower boundaries in its differential fixture.
The remaining work still includes live resource admission/IO publication,
normal/dynamic geometry production and ordered light draw submission, unresolved
contact/audio/effect owners, original/native final-frame comparisons and
installed runtime delivery. This checkpoint does not close those gates.

## Particle-death sound and debris correction, 2026-10-12

World-effect loading previously rejected any particle system whose death
reference was not `effe`. This discarded the authored `rocks` system, whose
death reference is `snd!` (`rocks_small`), before any particle could be born.
The original248340 →246E60 dispatcher supports both branches; its sound leaf
247000 (TU1.1 2490D0) copies the particle's last position and consumes the same
246EB0/TU1.1 248F80 velocity control already used for collision sounds.

`NativeWorldEffects` now retains these systems and queues `Death` sounds before
unlink, once per simulation tick. Existing effect death dispatch remains in
place. Sound spatial/admission, effective driver behavior and shared sound RNG
remain separate validation boundaries. This fix restores the original dispatch
input; it does not establish complete original audio output.

`particle-death-sound-original-v1` executes the unchanged retail dispatcher,
sound wrapper and gain body for2,048 explicit position/velocity inputs. Every
sound identity, copied position and gain matches the existing original gain
fixture. Spatial producers188DD0/11BED0 and sound admission1895F0 are explicitly
recorded boundaries. `particle-death-authored-all-maps-v1.json` records45 `snd!`
and46 `effe` death references across the21 actual multiplayer maps.

The revision10 runtime passes the actual Lockout impact/grenade, render-cadence,
retirement and SDL delivery checks. The all-map test covers1,260 contacts and
1,696 SDL requests, including222 death sounds. These checks also assert that
a sound-only death child cannot suppress its visual particle system. The
combined resource/effects selection passes45 checks with no skips. Original
collider response and gain fixtures match2,048 cases each in both updates.

`particle-death-gameplay-v1` retains12 new validation captures from binary
`18646d09819250e71b4d274921263fdf6ca1c36ee1d094067e4c3edb70e2f404`.
Compared with the preceding capture, restored births also change subsequent
shared auxiliary RNG consumption and decal variation. This is expected from
restoring a previously omitted source system, and is not an original-frame
parity claim.720p/1080p counts and both RNG words match for each effect.

| Case | 640x480, one view | 720p/1080p, one view | 720p, four views |
| --- | --- | --- | --- |
| Impact | 8 hits,5 decals,26 particles | 8 hits,4 decals,21 particles | 8 hits,5 decals,23 particles |
| Frag | 1 detonation,25 particles,1 registered light | Same counts | Same counts |
| Plasma | 1 detonation,78 particles,1 registered light | Same counts | Same counts |

All12 captures have zero Vulkan validation errors.4:3 and four-view weather
histories remain separate comparisons. Grenade geometry-light packets still
remain zero; the cache-owner proofs in
[SOURCE_GEOMETRY_RESOURCE_PARITY.md](SOURCE_GEOMETRY_RESOURCE_PARITY.md) do not
close that live rendering gate. Full shading, unresolved contact/effect/audio
owners and matched original explosion appearance remain incomplete.

## Installed particle-death delivery

`build-particle-death-delivery-20261012` builds this correction over the current
interaction-prompt runtime. Its archive audit compares all387 ordered systems
members: only `world_effects.cpp.o` changes. The original native link inputs and
compiled shader directory remain pinned by its manifest. This preserves the
newer prompt implementation while delivering the tested debris correction.

`particle-death-delivery-gameplay-v1` repeats all12 impact/frag/plasma layouts
with identical counts to the revision10 capture above and zero Vulkan
validation diagnostics. Its720p/1080p primary and auxiliary RNG words match.
`particle-death-delivery-original-compare-v1` restores the retained Zanzibar
camera/player/device/clear inputs against actual `original_capture_20.png`.
The retained world crop has0.73935 mean absolute channel difference; the
first-person crop has0.61311. Simulation, HUD and shader clocks, target history
and effect state are not restored, so this is neither an explosion comparison
nor complete framebuffer parity.

The installed runtime is now
`build-interaction-prompts-verified-20261012/halo2_native`, SHA-256
`904a8296538789366de88a8a7618177b2cdce59a8bf4d5acc0c70f359d348eec`.
Both launcher-info routes select that file. Installation first checked the
unchanged base hash and all100 private/compiled shader-file copies, backed up
the previous binary, then replaced it atomically. The exact record and backup
are in `world-effects-goal-20261011/particle-death-installed-v1`.

`particle-death-installed-gameplay-v1` then runs the installed file directly
for the720p impact and frag cases. Both complete with zero validation
diagnostics and the exact previously captured input/count/RNG records. The
installed binary and compiled shaders remain unchanged afterward. The frag
record still has one selected light, three cluster and six instance receivers,
and zero raw/packed geometry-light packets. Its task-owned Xvfb server was
terminated after the two captures.

This installation completes the particle-death correction only. The full
effects goal remained incomplete at that historical delivery. Authored BSP
grenade illumination has since been connected; model receivers, remaining
contact/effect/audio owners and matched original explosion frames remain open.
