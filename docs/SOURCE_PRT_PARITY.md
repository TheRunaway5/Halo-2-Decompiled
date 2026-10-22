# Original Xbox PRT and initial machine rendering

This document records the recovered Xbox path for precomputed radiance transfer (PRT). The authority is the supplied original version-8 caches, the retail executable, and retained original xemu capture/RAM fixtures. CPU arithmetic and captured shader inputs have separate proofs. These results do not establish complete pixel parity, physical Xbox arithmetic precision, later device scripts, or unseen rendering states.

## Source path and native API

| Original routine | Recovered behavior | Native implementation |
| --- | --- | --- |
| `24490` | Select `min(LOD count-1, 5-source LOD)`, locate the exact section, bind its source PRT stream | `Cache::load_source_prt_vertex_stream`, `source_prt_vertex_stream` |
| `137800` | Resolve the section node map; absent maps use the source section/model fallback count | `Cache::source_prt_section_nodes` |
| `1376C0` | Compose current WORLD node transform with authored inverse bind; emit the scaled render palette | `source_prt_node_lighting` |
| `23540` | Allocate coefficient ranges in selected section order, using at most two nodes per section | `source_prt_section_banks` |
| `23600` | Query the actual current world position of the selected model node | `source_prt_node_lighting` |
| `22CF0`, `22C30` | Find nearest SH record and nearest opposite halfspace record; project and interpolate | `source_prt_sample` |
| `239BA`, `142BF0` | Original determinant and inverse matrix arithmetic | `source_prt_inverse_basis` |
| `143600`, `143250` | Rotate order-3 SH with the source signed-axis ordering | `source_prt_rotate` |
| `23CA0` | Project each channel onto the authored cluster/PCA basis using original SIMD reduction order | `source_prt_project` |
| `23690`, `24040` | Cache object incident plus independent section/node coefficients, then bind `c68` onward | `source_prt_bank`, `source_prt_section_banks` |
| `16B90`, `2BCD0` | Resolve the highest authored LOD/pass implementation; submit layer 5 after 3 and before 8/7 | `prepare_prt_material` |

The headers are `include/halo2/source_prt.hpp`, `include/halo2/source_device.hpp`, and `include/halo2/world_material.hpp`. The respective source modules keep the source metadata, object pose production, and material submission separate. `Cache::apply_source_prt_vertices` joins each original `(section, vertex)` identity to its source stream; it does not reuse incident vectors or lightmap UVs for PRT inputs.

The caller supplies the actual posed WORLD node bank and the source collector's selected section order to `source_prt_section_banks`. The helper samples every selected section/node in that order, builds separate coefficient banks, and uses the final sampled incident for every section of the object. This shared incident is important: `23690` overwrites one object cache field while processing its sections, and `24040` later binds that field for each section. Sharing section coefficient banks between different node palettes is incorrect.

## Cache layout and vertex declaration

The render-model PRT block is at `mode+0x74`, with one `0x58`-byte record in every supplied PRT model. Order, cluster count, and PCA count occupy `+0`, `+2`, and `+4`. The LOD block is at `+0x14`, stride 12; each entry contains cluster-basis offset and a block of `(section, stream offset)` pairs. The cluster basis is the float block at `+0x1C`. The raw resource descriptor starts at `+0x34`; the authored vertex buffer descriptor is retained from raw-header `+8`.

All 13 supplied records use order 3, 10 clusters, 4 PCA vectors, source declaration 57, and a 10-byte vertex stride. Source resource type 2, primary locator 0, secondary `FFFF` supplies the stream. `24490` addresses declaration 57 at `(authored offset / 5) * 10`; it does not interpret that offset as bytes directly.

The captured declaration is:

| Source input | Exact format | Native value |
| --- | --- | --- |
| `v5` | Signed scaled `SHORT1` (`R16_SSCALED`) | Signed integer-valued float cluster |
| `v6` | Signed normalized `SHORT4` (`R16G16B16A16_SNORM`) | `max(-1, short / 32767)` per component |

The two attributes share a buffer and stride 10, with byte offsets differing by 2. Dedicated `Vertex::prt_cluster` and `Vertex::prt_weights` fields feed them only when the pass has an explicit PRT bank. The normalized weights are authored PCA coefficients; they are not skin weights and are not renormalized.

The full metadata/stream test covers 34 section streams and 22,525 source vertices across these models:

| Map | Authored PRT models |
| --- | --- |
| Containment | `base_shield`, `base_gate`, `base_shield_blue`, `base_gate_blue` |
| Triplicate | `ze_train`, `ze_train_west` |
| Turf | `scarab`, `scarab_upper_gun`, `scarab_rear_gun` |
| Zanzibar | `main_crane`, `big_wheel`, `flywheel`, `blades` |

This is a loaded-tag inventory. It is not a claim that all 13 models are placed or drawn in the initial match. The contact inventory retains 10 initial bit-31 placements; Turf's scarab family and Containment's shields are loaded definitions without those scenario placements.

## SH sampling and coefficient binding

Active BSP lightmap-group SH records are the `+0x38` block with stride `0xDC`. Each record contains position, three nine-float SH channels, and incident at `+0xA4`. All 21 multiplayer maps contain 20 such records in total. The initial machine path uses populated valid banks. Empty valid banks select green SH; invalid structure/bitmap banks select white SH. Both require the caller's explicitly retained incident when assembling a complete object bank. The TU1.5 full-builder and coefficient tests now cover these branches; see [fixture provenance](../tests/fixtures/tu15_prt_fallback.md). Live callers without that source state continue to reject.

`c68.xyz` is the retained object incident. `c68.w` contains adjacent cache metadata in the original and is not read by the admitted shaders. `c69` is retained as literal source bits:

```
3F800000 40800000 461F6200 3B80A166
```

`c70..109` contains node 0's 40 vectors; a second node occupies `c110..149`. Each cluster has ambient RGB, one unwritten padding word, and four PCA coefficients per channel. The native bank sets only unconsumed padding to zero; the bit-exact fixtures exclude those original unwritten words and `c68.w`. Original capture18 reads only node-0 ranges, while both ranges are retained in the source binding fixtures.

The renderer uses a separate `VertexProgramBank` beginning at constant 68, with `prt_inputs=true`. That avoids enlarging or overwriting the regular camera/material uniform bank. `prepare_prt_material` preserves the source base accumulation pass and authored textures/raster states, places the PRT child in `environment_pass` with vertex category 9, and preserves independent layer 2, 8, 7, and 17 submissions. The supplied machine family admits 7 PRT material uses: 5 Zanzibar uses and 2 Triplicate trains. The trains use the authored `prt_lightmap` program; Zanzibar uses `prt`.

## Initial machine state and pose

The initial machine decoder is separate from scenery initialization. All 49 supplied type-7 placements match original group initialization fixtures. Thirty-seven have a position overlay, none has a power overlay, and every primary initialized idle selection is NONE. Source `107A30` and `1076E0` resolve authored named/private device groups; no blanket position-0/power-1 assumption is made. The 49 placements contain 38 unnamed and 11 named objects, with 48 origin-BSP-0 records and one origin-BSP-NONE record.

Source `2818E0` combines an overlay with the existing pose: quaternion multiplication, translation addition, and scale multiplication for the selected channel mask. The machine channel frame uses `1C6620`'s source endpoint arithmetic. `1152E0` advances gear position with the original rates and subtracts one period once. Generic `2D000` lighting implementation selection applies to ordinary machine model geometry; scenery lightmapping policy is not reused to select it. The initial material audit admits 97 ordinary implementation-2 uses and the 7 PRT uses, with zero unresolved uses.

The paused Zanzibar live fixture checks six actual machine world-node banks: 18 matrices and 234 components, maximum absolute error below `1.90e-6`. It uses the retained source object transform, scale/mirror state, actual position/power channel values, authored overlay, and full hierarchy composition. It does not prove subsequent power-group scripts or game mechanisms.

## Independent validation

`tests/source_prt_tests.cpp` checks source metadata and the retained full-object bank fixture. `tests/fixtures/retail_prt_object_banks.tsv` contains six independently paused original objects, literal current WORLD node matrices, and eight original section banks. All 1,256 consumed coefficient/incident/scale words match float bits through the complete section-bank helper. The preceding coefficient-only comparison checks 1,200 words.

`analysis/multiplayer-shading/prt-original.py` executes the unmodified retail routines from executable SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`, without routine stubs:

| Original arithmetic | Fixtures | Float components | Result |
| --- | ---: | ---: | --- |
| Determinant/inverse | 512 | 4,608 | Bit exact |
| SH rotation | 512 | 4,608 | Bit exact |
| SH sampling/interpolation | 512 | 15,360 | Bit exact |
| Cluster/PCA projection | 256 | 17,376 | Bit exact |

Reports are `prt-differential.json`, `prt-live-bank-comparison.json`, and `prt-stream-gpu-proof.json` under `analysis/multiplayer-shading`. The stream/GPU proof compares 51,140 bytes across all nine captured18 PRT buffer prefixes to the original authored section streams and offsets.

Sky's independent capture18 export contains 118 original shader-debug input/output cases across nine draws. All 590 cluster/weight/address input components match their original raw SHORT values. The actual shader is authored `vrtx E93707C2`, compiled class 1, 55 instructions, SHA-256 `98429107aa68a684da22d6117e2f6c78a1a4ef4f203c4d76d1cbc1a183525850`, retained in `tests/fixtures/retail_prt_rigid_local.vp`. It decodes local positions with `c170/c171`. The world-space class-0 variant is 41 instructions, SHA-256 `f18351bc6c5ba40a04f931001b7fccd14dc465f6ff418d799f570bcffa76b235`; these are different authored wrappers and must not be labeled as the same captured program.

Address-register floor semantics are supported by the [NVIDIA primary specification](https://developer.download.nvidia.com/opengl/specs/nvOpenGLspecs.pdf), `NV_vertex_program` section `2.14.1.10.1`, and the actual captured address-register trace. The original capture's CPU debugger, xemu post-VS output, native CPU interpreter, and Vulkan execution have distinct precision boundaries. A debugger comparison does not establish physical NV2A reciprocal/RSQ precision or complete screen-image equality.

## Live admission constraints

The source machine CPU decoder and source coefficient/stream path are implemented. Live callers must provide selected sections in original order, actual channel state and posed world nodes, and one independent material bank per section/palette. Admission must retain the original active-bank conditions, actual source LOD, exact PRT resource declaration, and supported authored material program. Missing or invalid metadata throws; authored NONE remains optional. Later scripted channel progression, model/permutation transitions, and source shadow/filter passes are not covered by the initial machine proof. The separate renderer/interpreter GPU tests and live integration must pass before claiming that this CPU/metadata work establishes an on-screen PRT result.

## SourceMachineScene integration seam

`SourceMachineScene` is implemented in `include/halo2/source_machine_scene.hpp` and `src/source_machine_scene.cpp`, registered with `halo2_renderer`. Construct it with the cache, retained scenario object metadata, active BSP index, the material mesh, and existing collision service before renderer allocation. It allocates stable material ranges and separate PRT children keyed by exact LOD/section/source shader. Production `update_pose` receives the simulation tick/render frame and refreshes posed geometry and coefficient banks; `append_view` selects each view's geometry using source camera distance, flags and retained per-player LOD bytes, then evaluates the owner lighting cache at that view's unmodified source distance before part shader selection, including empty resident collectors. The actual last-query-frame guard deduplicates completed queries; a later view can query when an earlier view skipped for cadence. Only nonempty ordinary selections receive a lighting binding. Legacy `update`/`append` remain explicit LOD5 diagnostics.

Only the recovered type-2 gear producer advances automatically, in discrete `1/30`-second steps. All 19 supplied gears have private power/position groups; named gear groups throw until the shared-group write producer is implemented. Door/platform later state progression remains an explicit initial-state-only limitation (30 initial records). Ordinary lighting queries use actual posed node0 bounds, constructor-derived runtime flags, original type7 query admission and the existing source temporal cache. The shared light-query service currently omits live dynamic-object contact ownership; a native unresolved attribution is not promoted to a source success.

PRT shader bindings do not suppress the owner's ordinary lighting cache.
Original `4B5D0` performs `3DD10` / `3DB00` after optional `23690` and before
part shader selection. Its geometry-cache reuse and context gates remain
separate from the material implementation. Complete original tail fixtures
for the retained wheel, blades and crane prove that initial invalid caches
reach `D1850`, even when frozen, while a valid frozen cache, repeated query
frame or reused geometry can skip that query. The 27-case instruction report
is `analysis/multiplayer-shading/prt-object-light-admission-original.json`;
the cache allocator and earlier geometry/PRT services remain explicit caller
boundaries. `ViewDraw::ordinary_lighting` describes ordinary shader uniform
binding only. See [owner cache admission](SOURCE_MODEL_SHADER_PARITY.md#owner-lighting-cache-admission).

The initial variant/permutation and resident section-collector ordering are now independently verified for all 49 MACH placements. The machine scene requires a resolved `Cache::load_initial_model_selection`, retains the actual fixed model-region permutation bytes, and obtains its ordered sections from `source_initial_model_sections` (source `4C2B0`/`137800`, every LOD0..5). Duplicate section entries remain duplicated. Every expanded mesh vertex's source section must belong to that literal collector array; section order is no longer inferred from triangle expansion or sorted IDs. The collector oracle supplies residency=true, so asynchronous cache-residency fallback remains outside this proof. The immutable bank may be shared by multiple materials within the same section; distinct object/LOD/section palettes retain separate banks. The machine union has 89 ordinary and 66 PRT material slots; 78 section banks are calculated from authored metadata, including banks with no material consumer. Captured source bank assertions inspect actually submitted IDs rather than all same-name resident slots. Per-frame shader/bitmap animation remains in the common runtime animation scheduler, after scene pose/bank updates and before uploading material constants.

Machine lifetime synchronization now consumes the simulation's actual `SceneObjects` full generation handles, global content placement indices, visibility and creation ticks before posing. Destroyed machines stop submitting geometry and lighting bindings; fresh private-group generations reset controller/group state and lighting cache, then advance from their actual birth tick. Original `106680` destruction and fresh construction are tested for all 49 records. Named groups preserve current shared state in the original, so native recreation requires that producer rather than resetting them. Triplicate's four scripted machines use private groups. Detailed HS chronology, shared primary random semantics and remaining activation boundaries are recorded in [SOURCE_MAP_SCRIPT_PARITY.md](SOURCE_MAP_SCRIPT_PARITY.md).


## TU1.5 completed-cache Vulkan bindings, 2026-10-14

The source model service includes authenticated complete TU1.5 `22830`, `23670`
and their SH/rotation/projection dependencies. `PhysicsLiveWorldTu15::begin_model_view`
exposes the original `12D19 -> 22830` per-view reset separately from frame entry.
`source_live_model_prt_bank_tu15` reads the completed `24020` cache by full object
handle and region ordinal, copying actual c68/69/coefficient words into an
immutable owner draw bank. It preserves no-upload results and rejects an
out-of-arena coefficient range. Fresh Cairo valid/empty/invalid controls each
match 771 consumed words; unchanged full-builder replay matches every compared
retained byte. The Vulkan tram diagnostic binds two original PRT material banks
and preserves its prior baseline/illuminated images byte for byte.

The material/presentation path still needs original gameplay admission ordering,
per-view lifetime and live cache incident wiring across all requested entities.
The matched-frame and installed-runtime gates remain open.


October20 physics continuation: MCC Flood Zone campaign context32 (vehicle
placement4/BSP0) reaches visible machine placement355 before rendering. Its
empty SH path correctly rejects missing retained caller incident. Offline
`analysis/multiplayer/physics-live-tu15-20261012/native-prt-caller-v1244/`
authenticates the actual English TU1.5 code bytes: full builder4B5C0 calls23670
at4B9E6. The original PRT cache uses up to24 records at4C51D0 with104-byte
stride; incident is the record's+92 field. Calls23983/23BB0 pass that actual
field into22CD0. `source_live_model_prt_bank_tu15` already reads this original
owner record and current coefficient arena for admitted vehicle draws.
MACH is absent from the current native model resource/selection roots and its
resource prepass assembles PRT banks before a retained render caller is bound.
The remaining repair must connect MACH material residency and each actual
view's completed native cache state, including client presentation ownership;
it must preserve checkpoint/material lifetime rules and skip no visible owner.
No zero vector, another object's bank, recorded coefficient replay or relaxed
incident guard has been introduced. This audit is code/ownership evidence only,
not live qualification of that context.

Offline v1276 authenticates actual per-view22830: it resets24 record owners,
allocation tokens, coefficient offsets, node counts and515EC8. It deliberately
leaves each record's+92..103 incident bytes untouched. For an empty/invalid
active SH bank, the initializer exits at22C04 without replacing those retained
incidents. A new per-object zero default would therefore lose the prior view's
slot lifetime. The required live renderer owner must preserve this cross-view
cache state before source23670/22CD0 consume it. Evidence is retained at
`analysis/multiplayer/physics-live-tu15-20261012/native-prt-view-initializer-v1276/`.

October20 continuation v1331 compares original and imported MCC FloodZone SH
metadata directly: BSP0/1/2 each have zero records, BSP3 has seven and BSP4 has25,
with matching structure ownership. Machine355 atBSP0 therefore needs the actual
retained caller even with correct MCC conversion. Native model probe v1341
first reaches a duplicate47524E mount inside the complete writable camera-effect
request type table. This follows the separately authenticated450212 duplicate
inside the complete70-byte startup table. Shared source now borrows both
original owners; no original memory result is supplied. The active v1343 owned
incremental diagnostic will verify those fixes before continuing the actual
native admission/model-builder path. It uses actual MCC assets and a declared
isolated diagnostic view, so its output remains component evidence.

Native v1343 now reaches actual23670 and observes its seven source arguments
without changing instructions, registers, memory outcomes or service results.
A declared isolated whole-builder call produces one genuine cache record,
two region0 node banks and the actual cold incident0/0/0. This does not admit
machine355 into a view: actual source admission remains empty and its spatial
reference head isNONE. The globally instantiated key is at295.7645,-73.7401,790
with radius100; the campaign observer start is nearZ-188. Candidate v1346 fixes
the eager presentation demand by keeping deferred resource errors until draw
submission and testing the actual source-camera frustum first. Any in-frustum
unresolved machine must still reject. Real per-view MACH PRT integration for
admitted owners, later slot history and client ownership remain open. The
v1343 private build/source are removed; required exact probe/source archive and
small positive/negative reports remain with retention metadata.

Candidate v1346's actual-MCC deferred-machine/frustum regression passes,
including strict in-frustum failure and retirement, along with the existing
machine presentation suite. Linked qualification remains negative29/35 because
six checks reach one additional cold ownership regression: selector2 at47C1F8
is inside the complete writable reaction-name table. Original image audit
v1356 independently confirms four words0600008B..0600008E and .data flags7.
The two older aliases now borrow that original owner, including the later
selector0 read; no overlapping remount or readonly conversion remains. Full
linked v1357 and fresh visible v1358/v1359 acceptance are pending. The v1346
private build/source have been verified removed; all failure receipts remain.

The v1357 factory and34 linked checks now pass after borrowing the complete
writable reaction-name owner and adopting the existing shared multiplayer
branch provider. Independent v1361 authenticates its19 readonly intervals
(577 bytes) and newly reached1EF260's unsigned0..4 dispatch bound. Base live
v1359 passes the full9-case physics core with the original submitted vehicle
camera. Actual FloodZone v1358 still rejects machine355, exposing a second
presentation producer mismatch: its recomputed node0 shadow sphere differs
from the actual retained object+30/+3C admission bound. Revision v1357-r1
looks up the latter from the current authoritative WORLD-pose snapshot by full
generation, and uses it for machine frustum and LOD metrics. That revision
passes34 linked checks/export and core v1365 passes9/9, but FloodZone v1364
still fails. Read-only v1367 confirms its real radius100 sphere enters the
ordinary camera frustum during look. Candidate v1369 adds actual full-generation
cluster memberships from both source spatial pools, transported in protocol95,
and requires membership in a current portal frustum before machine submission.
All36 linked checks and immutable export pass; actual visible v1371/v1372
qualification is running. The26-case vehicle camera/seat cohort v1373 is queued. Admitted unresolved PRT owners still fail.
No recorded bound, incident, pose or admission result is supplied to live physics.

Actual FloodZone v1371 now passes on this protocol95 candidate:102 submitted
Phantom frames per view,16.76 units of causal original-controller response and
185 exact same-tick LAN comparisons. v1374 recording review at20/50/85 percent
shows the Phantom clearly in both views through approach, overhead pass and
departure; all3 decoded review PNGs are removed. This resolves context32's
unregistered machine355 presentation rejection. It does not establish admitted
machine per-view PRT ownership or complete physics parity. Core, all26 camera/
seat selections and a one-runtime47-context campaign census remain in progress.

The same production native binary is now wired as v1369-r1 (protocol95).
Qualification retains36 linked checks, core9/9,26 consolidated vehicle/seat
selections (including explicitly declared Banshee takeoff input),54 teleporter/
HUD/flash compatibility checks and3 validation-enabled rendered delivery checks.
Ordinary native and wired-launcher startup pass120 frames with two players and
MCC-only opened files. The earlier render failure was a missing installed
validation-layer search path in the one-off harness; corrected v1390 passes3/3.
Admitted MACH per-view PRT remains open; the full current campaign census is
running and is not inferred from these bounded passes.
