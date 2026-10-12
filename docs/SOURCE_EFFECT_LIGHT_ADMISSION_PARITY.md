# Spherical effect-light admission and candidate collection

The native admission module now reproduces the original render boundaries using
explicit current light, owner, view, frustum, epoch and queue inputs. It does
not discover light registrations or turn on gameplay illumination.

`effect_light_admission.hpp/cpp` separates the persistent spatial center and
radius at light `+18/+24` from the render position and current shape at `+84`.
The distinction matters: `31910` ranks candidates using the persistent center,
while `C17F0/C1A80` constructs the rendered sphere from the render position.

## Original boundaries

| Native function | Original path | Verified output |
| --- | --- | --- |
| `source_effect_light_live` | `C3140` | Active bit, positive persistent radius and overall fade, and diffuse/specular energy threshold |
| `source_effect_light_render_admission` | Whole `31590`, including `C3140/31520/C17F0/C19F0/C1930/C1A80` | 4,096 calls, 203 admissions, 475 exact 31-word shapes, 890 consumed local-priority calls |
| `source_effect_light_candidates` | `31910` up to its call to `4A7C0` | 192 ordered queues, 25,260 exact candidate words, 31 full 128-candidate cases and 69 empty cases |
| `source_effect_light_visibility_visit` | Whole `132A30`, including `C3140/165010/181800/461C0/163C80/C3110` | 4,096 visits, 937 successful appends/epoch stamps, 111,040 exact retained queue words |
| `source_effect_light_collect_visible` | Whole `132220` with object branches explicitly disabled, including the spatial light-list walk and `C3950` fallback | 256 collections, 286 appends/stamps, 22,220 exact retained queue words and 1,239 owner-priority calls |

The `31590` gate includes attached-local/nonlocal flags, debug suppression,
renderer enable, current sphere radius, view type and missing-cluster rules.
Shapes can be produced even when a later view/cluster gate rejects the light.
Early returns preserve the output shape. Runtime animation shapes remain a
separate service; the bounded module rejects that consumed branch.

The candidate collector retains visible-queue order and duplicates. Original
eligibility still runs after the output reaches 128 records. The owner field
comes from `BAF80`; flag byte2 comes from the immediate owner's header kind,
while local-priority flag byte0 is evaluated on the root. Definition flags
produce bytes1/3. Coverage uses the shared exact `source_shadow_coverage`
arithmetic. Selection then belongs to the separately retained dynamic-light
cache: `source_shadow_select`, budget3, acceleration .01, actual source frame
and cinematic state. This module does not manufacture that history.

The visibility visit reserves the queue's final slot. A rejected visit or a
failed append does not stamp the light's visited epoch. Mode0 uses actual
constructed cluster-entry frustums; mode1 uses the original sphere test and
SSE operation order. Other source modes bypass this cull. A negative signed
context byte also bypasses rejection. All inactive queue columns survive.

The collector walks supplied registration lists in their actual link order,
using each constructed entry's ordinal for frustum culling. Its context pass
mode and bit4 suppress both light branches. The fallback scans occupied light
records in ascending pool order, evaluates `31520` even after reaching four,
and then admits only live, unvisited, missing-cluster records from those four.
Inactive or already registered selections can consume fallback slots. This is
the local attached-light fallback, not an unattached-light scan. Containing cluster
NONE and global local-owner NONE independently suppress it. Thirteen fixture
cases append fallback lights with every spatial list empty.

## Evidence and limits

The admission/collection oracle tools use the retained physical RAM
snapshot under `world-effects-goal-20261011/original-session/frag-light-draw`.
Its SHA-256 is
`68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e`.
Original instructions and callees execute unchanged. Controlled light/tag,
owner/local-player, view and queue inputs are declared by each tool. The
candidate replay stops before `4A7C0`; it is not a whole selection replay.

Frozen fixtures and provenance are `effect_light_admission.original.*`,
`effect_light_candidates.original.*` and `effect_light_visibility.original.*`.
CTest names are `original_effect_light_render_admission`,
`original_effect_light_candidates` and `original_effect_light_visibility_visit`.
The original and native reports, including failed intermediate runs, remain in
`analysis/multiplayer-shading/world-effects-goal-20261011/`.

The collection fixture is `effect_light_collection.original.*`; its test is
`original_effect_light_visibility_collection`. The oracle supplies declared
spatial heads/links, occupied-pool records, two literal frustums, owner/player
fields and retained queue contents. Context bit6 disables object traversal
and the sky object scan is disabled. No light instruction or callee is replaced.

Separately, `oracle_effect_light_birth.py` executes128 complete `C0350`
standalone births using the snapshot's real350-record light pool and both
2,048-record spatial reference pools. It executes96 actual `C2D00/1CAC60`
registrations and confirms owner `+4C`, object `+58`, and both attachment-node
fields remain NONE for declared standalone calls. World position, orientation
and original control survive unchanged. This original-only evidence corrected
the native world-effect light owner: the contact/projectile transport identity
now lives in `effect_owner` separately. It does not establish native allocation,
registration chronology, effect A/B production or attached part delivery.

`effect_light_spatial.hpp/cpp` now retains original light spatial-list storage:
512 maximum heads and two independent2,048-record pools, both with stride12.
Their names are `cluster light reference` and `light cluster reference`;
using the object registry's stride32 or reverse-pool salt would be incorrect.
Insertion prepends each successful forward/reverse allocation independently.
Removal walks the retained reverse list, so entries orphaned by reverse-pool
exhaustion survive exactly as the original does.

The separate `oracle_effect_light_spatial_pool.py` executes unchanged
`16B7A0/16B840/1CAC60/14A550/1CAE40/16BA40`. Sixteen cold/debug/independent
pressure cases cover1,536 ordered insertions/removals. Every step's allocator
state and heads match, as do complete final record bytes and occupancy banks:
284,800 exact words in total. `original_effect_light_spatial_pool` consumes
`effect_light_spatial_pool.original.*`. These are explicit forced-membership
masks over a declared16-cluster bank. A separate native registration comparison
now covers actual Lockout geometry rather than forced memberships.

`source_light_spawn_contacts.cpp` implements the consumed terrain branch of
`16A440`, including the radius expansion, BSP traversal, ordered surface/edge/
vertex deduplication, convex-edge filtering and independent256-contact limits.
`source_sphere_position.cpp` implements complete `16A8E0` correction over those
banks. The native contact producer matches978,067 owned contact words across512
queries; the complete correction matches2,560 output words, including242 moved
centers. Coverage includes92 queries that reach the256-polygon limit. Padding
and unused polygon UV slots that the original does not write are excluded.

`effect_light_registration.hpp/cpp` implements the ordinary standalone spherical
`C2D00/C3220` path with authored persistent bounds, initial collision correction,
actual BSP location, geometry-derived portal membership and both spatial pools.
All128 original registration/removal pairs match1,598,208 complete record,
location and pool words, including67 nonempty registrations and11 corrected
centers. Both the retained-contact fixture mode and native terrain-production
mode pass. Correction changes the spatial/location center while preserving the
original uncorrected cull center. The source does not erase existing links before
registration; the caller still owns that chronology.

Fixtures are `light_spawn_correction.original.*` and
`effect_light_registration.original.*`. CTest names are
`original_light_spawn_correction`, `original_light_spawn_contacts`,
`original_effect_light_registration` and
`original_effect_light_registration_terrain`. These checks cover source
algorithms on Lockout; GPU submission remains separate.

`effect_light_lifetime.hpp/cpp` now owns the original350-record,272-byte
`lights` pool and the complete ordinary standalone `C0350/C0C80/C1400/C3260`
path. Allocations retain salted identities, independent spatial links,
activation changes, update-before-retirement order and the strict duration
comparison. The whole original replay covers932 complete pool checkpoints,
621 creation attempts, two full-pool rejections, release/reuse, debug fill,
generation wrap, signed tick wrap and untimed authored-color clamps. All33,790,592 reconstructed light/spatial
record and allocator words match. Native registration uses actual Cache-loaded
Lockout collision/location/portal banks; no original callee is replaced.

`effect_part_control.hpp/cpp` reproduces `17A7E4..17A818`: part flags at+30/+34
select retained A/B controls rather than assuming light control1. All2,048
original output words match. This is the part-control prefix; production of the
effect's retained A/B values and its other admission/location branches remains
separate.

`NativeWorldEffects` uses both modules for world light parts. Tick simulation
creates/registers, updates, deactivates/unlinks and retires the actual salted
records. Read-only `lights()` exposes their registration state in ascending
occupied-slot order. The effect's transport identity stays separate from the
standalone attached owner NONE. Actual frag/plasma and cadence integration tests
pass. This pool currently owns world-effect lights; merging attached/muzzle and
other original light creators into the shared chronology remains open.

Fixtures are `effect_light_lifetime.original.*` and
`effect_part_control.original.*`; tests are `original_effect_light_lifetime`
and `original_effect_part_control`.

`effect_light_view.hpp/cpp` now owns occupied-slot `C15A0` view preparation,
`1320F0` query epochs/stamps, actual spatial-list collection, `31910` candidate
production, and the four retained `4A7C0/4AC30` local selection banks. The
original replay matches256 checkpoints,2,182 appends,2,252 selections and
6,456,064 exact words. The render-frame counter advances once before all local
views; simulation tick, view index and query epoch remain distinct. Nested
light receiver queries advance the same query epoch without collecting lights.
New world light births use that current epoch rather than bootstrap0.

`effect_light_receiver.hpp/cpp` joins the actual ordinary small-sphere
`1320F0/132B80/132D60/134390` path. Its69 whole original queries match58,278
owned/camera words,47 nonempty receiver lists and93 complete association masks.
Persistent radius must be below3: the original switches to portal construction
at3, so larger bounds remain an explicit unresolved branch. A containing
cluster of NONE invokes original `11BED0` point-location fallback. The six
`C1D90` cameras, persistent spatial bounds and actual main-view auxiliary queues
remain separate inputs. Context+8 is the containing BSP cluster, not a local
player index; production now supplies `fog.cluster` there.

`source_draw_unit_sphere_cull` now also consumes one actual512-word mask arena.
Distinct starting addresses can overlap after `1332B0` overflows. The original
replay matches1,027 such controlled cases and586,479 exact unit/mask/call words,
including the complete retained arena. Both linear and accelerated writes
observe prior writes in source order. The previous exact-address bank interface
remains available for callers with independent masks.

`source_bsp_draw_collector.hpp/cpp` implements the BSP-owned
`2CB30/133390/133520/2CBF0` corridor with an explicit real main prefix, retained
850-unit/coverage banks, current fog and lighting/residency services. It preserves
zero-alpha queue writes, unused mask words and the original capacity behavior.
A joined48-case replay through unchanged `44720/45560` matches784,966 exact
retained unit, coverage, arena, queue and call words. Native culling loads
Lockout accelerator geometry independently from the map. A separate resource
audit accepts all21 maps:1,265 sections/51,946 subparts, all accelerated with
zero linear bounds. The loader also retains optional type16/channel20 bounds;
these map fixtures do not exercise a nonempty linear BSP bound resource. The actual receiver's dynamic collector adds no
units in this fixture; every variant verifies that boundary. This does not
supply the live multiplayer main/dynamic draw ownership or final draw order.

The next owners now retain the exact secondary BSP part commands, ordinary
layer12 opaque selection and sorted draw ranges. The whole selection replay
covers39 selections,1,999 selected records and239,616 exact words. The range
replay covers182 cases,370 ordered indexed calls and67,449 exact Cache indices,
including retained shared-arena aliasing and failed binding schedules. An
explicit successful1CAA0 result remains a caller input to the range adapter;
metadata presence does not prove resource binding success.

Actual retained RAM supplies13 selected raw bindings:10 class0 clusters and3
class1 instances, with three literal program/declaration pairs and actual c50
instance palettes. The raw importer preserves unchanged geometry and c170
compression inputs. The generated catalog admits25 unique programs across the
class0/1 inventory. Comparing its three sphere variants with the independently
retained original Xemu semantic emitter matches70,182 complete GS records and
3,579,282 exact words across seven resolution/split views. Seven changed-c35
controls are detected, with zero Vulkan validation errors. This is an isolated
shader/input comparison; no original gameplay framebuffer is claimed.

`effect_light_scissor.hpp/cpp` reproduces whole1650A0 and4A3D7..4A677 with actual
projection camera, sphere, clipping rectangle and short viewport inputs. All680
original cases match9,520 words, including178 bounded outputs,462 present-light
fallbacks and40 absent lights. The x87 transcendental path executes unchanged
in the original oracle; the native implementation restores the caller's control
word. This verifies floating bounds and depth, not their device/scissor consumer.

All48 source checks pass in `raw-sphere-bridge-source-regressions-v1.log`; the
additional scissor check passes in `light-scissor-ctest-v1.log`. Twelve raw-title
catalog/fragment/raster checks also pass after the projected-volume correction.
Fresh native captures in `raw-sphere-bridge-gameplay-v1` cover all12
impact/frag/plasma layouts with zero Vulkan validation errors. Frag prepares2
BSP clusters and6 instances in its single-player view. The close plasma light
resolves to cluster NONE and is rejected by the source visibility gates. The
captured binary SHA-256 is
`11005c4368cf88154b29a8acc3cba53ada5c8126bfb09bd97fbb6878b1734cc9`.
These captures do not submit the pending geometry illumination path.

`light-camera-comparison-v1` restores the literal original Lockout camera and
projection; all eight adjacent original camera headers agree. Geometry aligns,
but weather, weapon/effect state, target history and brightness differ. That
camera-only comparison is explicitly rejected as complete appearance parity.

Remaining owners include live global main/dynamic draw production, ordered
GPU submission, larger-bound portal receivers, object/first-person illumination
and lens flares. These numerical and service checks do not prove a matched
original gameplay frame or full multiplayer shading parity.

## Raw renderer and shared allocation continuation

`EffectLightRawDraw` now retains the actual class0/1 geometry, instance palette,
selected source pass, complete c[192] bank and per-draw light/material constants.
The renderer submits these packets through `MenuWorldNativeDraw`, with the real
base-color target and projected3D attenuation image. Original vertex output
already includes the player rectangle offset, so raw draws use a full-surface
Vulkan viewport and retain each player's scissor. Applying another player
viewport would scale and offset the geometry twice.

`raw-light-renderer-gpu-v2` passes15 controlled layouts: all three original
sphere variants at640x480,1280x720,1920x1080,853x479 with four players, and a
return to640x480. Additive blending and per-view constant isolation have zero
measured channel error; there are no Vulkan validation diagnostics. The fixture
uses actual Lockout parts, textures and instance transforms with explicitly
controlled base color, camera and light. It is not a gameplay/original frame.
Six existing light/fragment GPU regressions also pass.

The first raw run exposed the original attenuation sampler's address4 gate.
The native sampler already maps it to `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER`.
Fragment admission now accepts the three exact standard ARGB border colors;
other colors require the actually enabled custom-border-color device feature.
Address5 remains rejected. Source Xemu Vulkan sampler code corroborates these
bindings; admitting address4 does not approximate its color or addressing.

`source_draw_unit.hpp/cpp` supplies the shared `2CB30/2CBF0` bank owner for
BSP, instance and dynamic model requests. The BSP collector now uses this same
allocator. It retains850 units and both parallel byte banks,192 override slots,
32 section records and the caller's512-word arena. Failed resource admission
rolls back only the draw count; prior override, section and context writes
survive. Main/secondary reset, capacity, mask overflow, fade256 quantization and
retained bytes follow the source order.

`draw-unit-allocator-original-native-v1` compares75 whole original calls,
714,440 exact words and210 ordered services. Original `44940/2D000/449E0/30290`
execute unchanged; their recorded results are explicit native service inputs.
The fixed section rectangle branches use the original positive/negative maximum
float bounds. This test does not implement those four services or produce live
owner collectors. The durable fixture is `source_draw_unit.original.bin` and
the test is `original_shared_draw_unit_allocator`. All seven joined source
regressions pass in `draw-unit-source-regressions-v1.log`.

The live producer and complete appearance gates above remain open. In
particular, an empty `ModelView::raw_effect_lights` still submits no lighting;
the tested renderer API does not populate that span by itself.

`source_sphere_rectangle.hpp/cpp` now implements complete30290 from the actual
rendering bank, camera position and signed-short outer viewport. The independent
whole-original comparison matches2,228 rectangles/8,912 exact words, including
816 empty,541 full and871 partially bounded outputs across eight viewport
layouts. Changed-center controls differ in874 cases. Maximum-float bounds,
near-depth fallback, asymmetric projection and scale arithmetic are retained.
This is the floating rectangle owner, not its later integer/device consumer.

`draw-unit-rectangle-original-native-v1` repeats all75 allocator cases with
native30290 rather than recorded rectangle results. It matches the same714,440
bank words and210 ordered service boundaries. Metadata, lighting classification
and resource admission remain explicit services. The frozen version1 checkpoint
retains the earlier rectangle-service evidence separately; the current durable
allocator fixture uses version2.

`source_object_draw_collector.hpp/cpp` now implements whole133F70 and calls
the shared native2CBF0 allocator and native30290. It retains all five model
rows, selected section/region banks, model/section order, low-byte owner filter,
source flags and distance bytes. The conditional weapon policy copies the
complete collector and resets only its original section/override/fade fields.
Object bodies, context identities and weapon definition+290 are explicit owned
inputs; missing consumed banks reject rather than admitting a default model.

`object-draw-collector-original-native-v4` compares73 complete calls,
170 ordered source requests,492 metadata/lighting/residency service boundaries
and695,958 retained output words. Five real collector inputs are produced by
unchanged2CBC0/133720/133790/4B5D0 from the retained main context00547F88.
Controlled variants cover all five model slots,16 section slots, NONE geometry,
weapon policy, capacity, filter bytes, retained banks, fade/region values and
eight viewport layouts including the four odd853x479 cells. No original
instruction or callee is replaced. A forced dynamic instance0 was rejected
as invalid source state: these actual models have no authored lightmapped
object reference bank. The failed exploratory logs remain separate.

The durable fixture is `source_object_draw_collector.original.bin`; the CTest
name is `original_object_draw_collector`. Nine joined source checks pass in
`object-draw-collector-source-regressions-v1.log`. Upstream live4B5D0 cache/node
production,133AB0 effect/child orchestration and actual resource publication
remain open; this fixture does not populate gameplay raw light packets.

The next joined resource evidence is `draw-unit-resource-original-native-v6`
and `object-draw-resource-original-native-v2`. Native44940,449E0 and2D000 now
consume independent pre-call source banks through `source_draw_resources`.
They select actual dynamic/BSP/instance metadata, read the payload pointer only
after a requested publication succeeds, retain checksum/bitmap/group gates and
implementBADC0's full-datum fallback. The object collector's resource adapter
also resolves original direct low16 body/weapon lookups; those deliberately
differ fromBADC0's salted validation.

The allocator now matches100 whole calls/952,504 retained words,285 ordered
owner boundaries and28 actual12DE70 request inputs/results. Added cases cover
all lighting classifications, absent/negative/checksum gates, body flags,
stale/empty/out-of-range headers, failed geometry/lightmap requests and explicit
valid controlled dynamic lightmap reference banks. The model collector repeats
all73 calls/695,958 words with native body/weapon and these three native owners,
preserving492 boundaries. Its actual dynamic449E0 calls do not request12DE70;
upstream4B5D0 owns those resources. Both fixture schemas retain their older
service-only evidence in separate immutable checkpoints. Current fixtures use
allocator version3 and object collector version2.

12DE70 resource publication and its cache mutations remain an explicit platform
service. Native address tokens must resolve current owned banks; copying the
reference RAM into gameplay is not an implementation of that ownership. The
live4B5D0/133AB0/main-query producer, class2/3 raw vertex consumers and matched
original frames remain open.

The subsequent `source_object_presentation.hpp/cpp` implements whole133AB0.
It preserves effect-copy order128,2,1,32; the actual shared section counter;
depth SUBSS/MULSS/ADDSS order; parent flag changes; suppression of the ordinary
draw; low-byte child/skip/build arguments; direct body child/sibling links;
conditional weapon reset; and4B5D0 child build arguments. Copy identities and
section counters belong to the caller; native code does not adopt reference
stack addresses or manufacture live object/cache banks. Invalid cyclic native
child graphs reject instead of recursing indefinitely.

`object-presentation-joined-original-native-v5` matches90 whole original calls,
869,429 exact parent/counter/shared-draw/mutable-context words and223 ordered
callee boundaries:183 native133F70 collections and40 explicit4B5D0 builds.
The native collector calls2CBF0/44940/2D000/449E0/30290 directly, using
independent pre-call resource banks. The original executes3,184 unchanged
instructions with zero interception. Cases cover all effect combinations,
existing/initial/wrapped section counters, signed empty model counts, low-byte
arguments, child suppression, real retained weapon policy and controlled links
to an actual retained weapon. Those declared links are test inputs, not an
assertion about the saved live attachment graph. Temporary collector pointer
identities are omitted from comparison; all121 raw record words remain exact.

Original4B5D0 output records and produced render contexts remain explicit
native test boundaries. They must be replaced by actual native cache/node/LOD
and resource publication owners before gameplay integration is claimed.

The subsequent model-resource owner removes the3DDD0 cache-result/write-patch
boundary: native4C190/4C2B0/3D430/4C640 join shared allocation/rebind and whole
3DD10/BAD50 current colors. The259-case comparison retains4,255,403 exact words,
476 ordered platform requests and213 native acquisitions, with successful
selections consumed by native137650. Whole3D4F0/137650 separately matches182
calls/4,027 words. Actual12DE70 publication, live4B5D0/133790 node/query ownership
and gameplay lighting remain open; see
[the model-resource evidence](SOURCE_MODEL_BUILD_RESOURCE_PARITY.md).
The durable fixture is `source_object_presentation.original.bin`, schema2;
CTest name `original_object_effect_child_presentation`. Earlier schema1 proof
retains isolated orchestration comparison only. None of these checks populates
live `ModelView::raw_effect_lights` or supplies matched original/native frames.
All10 joined owner regressions pass in
`object-presentation-joined-source-regressions-v1.log`. The executed native
test hash is `5a435e35683e5146ad7ae443848b222666ffb5c440889d00bcca2b028382011d`;
fixture hash is `d77749d0e9aacf497dd051b3d3a659d2d615d8da042766f3ec72aa0e4706f7c7`.

## Query resets and joined secondary geometry frame

`source_bsp_material_visibility_initialize`, `source_bsp_query_begin` and
`source_part_commands_begin` implement183EC0,1348E0 and40E90's bank reset.
The persistent material allocation is exactly0x4204 bytes: activation writes
only the first0x4201 bytes toFF and retains the last three. It is not cleared
per view. Each query owns its own arena; main547F88 and secondary54AA68 must
not share it.1348E0 clears only the previous used prefix, zeroes the cursor,
association count, marked-cluster words and four queue counts, and sets the
512 cluster-entry bytes toFF. Other header bytes and the arena tail survive.
2CB30 itself does not clear that arena.

40E90 resets command counts/cursors without replacing capacities, pointers,
word+10 or command storage. Mode0 resets bank0 and the two main object counts;
secondary modes reset banks1 and2. Selector0 usesFFFFFFFF, selector2 uses10,
and every other nonzero selector uses20. Its44AC0 group is the literal31,
independent of the incoming command context.

`bsp-frame-reset-original-native-v2` matches40 cases/646,780 exact retained
words with76 unchanged instruction addresses. This fixture stops40E90 at its
41C80 service entry; it does not execute the entire command frame.
`secondary-command-frame-original-native-v3` separately executes whole40E90,
including unchanged41C80 and44AC0, and matches79 cases/1,869,039 words with955
unchanged instruction addresses. Its object queue is verified empty, with
zero41C20 calls in every case. Native code requires an explicit object owner;
an absent callback is never interpreted as an empty queue. Both comparisons
use zero instruction interception.

The new `source_effect_light_geometry_frame` joins secondary2CB30 collection,
the explicit133720 dynamic owner,44720/45560 culling,40E90/44AC0 command
generation and3BAB0/13DA70 layer12 selection/sorting. Every native stage
consumes the preceding native unit, mask, coverage and command outputs.
The read-only geometry and shader inputs come independently from Cache;
intermediate original output snapshots are not copied between stages.

`light-geometry-frame-original-native-v2` matches39 calls and1,846,744 exact
retained words through3,134 unchanged instruction addresses, with zero
interception. It executes133720 and verifies that the observed dynamic queue
leaves the BSP state unchanged.41C80 is likewise verified empty.4BA014=1 is
the declared ordinary-sphere selection input. The actual retained31-unit main
prefix remains an explicit input, not a native main-frame producer. Nonempty
dynamic/object queues, ordered1CAA0 binding/ranges and GPU packet production
remain required subsequent owners.

Durable fixtures are `source_bsp_frame.original.bin`,
`effect_light_command_frame.original.bin` and
`effect_light_geometry_frame.original.bin`, each with its exact provenance.
CTest names are `original_bsp_frame_resets`,
`original_effect_light_command_frame` and `original_effect_light_geometry_frame`.
These CPU comparisons do not populate live `ModelView::raw_effect_lights`.

## Object render entry and model-build binding

`source_object_render_entry` implements whole133790. It consumes the current
queue row and cached object sphere, uses native source coverage/distance and
4BAF0, writes only the original collector bytes, calls its explicit4B5D0 owner,
then applies first-person/alternate-pass flags and133AB0 admission. The first
fade byte owns the queue lowbit0 marker. The separate native1332F0 helper keeps
the query sign and ordered planar-fog equality gates; NaN does not count as
zero. The alternate argument and force-world flag retain lowbyte semantics.

`object-render-entry-original-native-v4` compares110 whole original calls,
13,530 retained collector/queue/return words and326 ordered owner boundaries.
Cases include five actual main entries, all pass/alternate controls, queue
association/flag combinations, signed query flags, planar fog, positive and
negative zero, quiet/signaling NaNs, retained randomized local storage, first
fade rejection and first-person model-count/repeat controls. Coverage inputs
include119.75 and239.5 for odd split-view heights. This is scalar source
evidence, not a resolution framebuffer comparison. Native4BAF0 reads independent
pre-call current/parent/definition/model banks.4B5D0 and133AB0 collector results
are still explicit ordered record boundaries in this entry fixture.

`source_object_build_collector` separately binds the typed collector to whole
native4B5D0 over the same current memory/cache/arena banks. It retains owner+0,
copies only collector+4 onward and reads the actual native result back. No
4B5D0 output records or lighting-cache writes are supplied to this adapter.
`model-build-typed-collector-original-native-v11` repeats all63 whole calls,
3,557,610 retained words and672 ordered deep-owner events through this binding.
Those deep animation/sky/PRT/query/conversion/publication boundaries remain
explicit as documented in the model-build evidence. The separate110-case
entry fixture does not yet join this build service or native133AB0 in one
fixture, and it does not supply a live main/dynamic collector.

The new durable fixture is `source_object_render_entry.original.bin` with
its exact provenance; CTest is `original_source_object_render_entry`.

`raw-light-allocator-gameplay-v1` passes all12 native impact/frag/plasma
resolution/view captures with zero validation errors and equal simulation
counts. Its frozen binary hash is
`fa14314994389b80b5e5cb8afb910bde9a9b5d16ae7a7b881f058a8812f89939`.
It contains the raw renderer/allocator changes, predates the unused standalone
30290 helper, and still submits no live geometry illumination. Current launcher
selection was inspected independently and points to `build-armor-emblem-20261011`;
these captures do not establish delivery through that separate runtime.
