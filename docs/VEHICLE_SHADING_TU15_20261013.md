# Vehicle shading: OG Xbox TU1.5, 2026-10-13

The visual authority is the owned original Xbox assets and unchanged TU1.5
executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Vista is not a gameplay visual reference. This work advances vehicle material
and ordinary lighting integration; full vehicle appearance parity remains open.

## Original asset census

`tools/audit_vehicle_shaders.py` independently inventories all `vehi` tags and
their model, shader LOD, pass, vertex program and bitmap dependencies across
15 campaign and 21 multiplayer maps, including the nine map-pack maps.
The census at `analysis/vehicles/shading-tu15-20261013/original-census-v2/`
retains each cache hash and report hash. Counts are per-map records, not globally
distinct vehicles: 522 vehicle definitions and 450 model records.

The native admission report `native-admission-v3.json` in that same analysis
directory accepts 3,578 material/LOD cases and prepares models for 512 vehicle
definition records (several definitions share a render-model tag). Its
15 remaining failures are five material cases and ten model cases, all for the
Scarab upper/rear turrets' `scarab_big_turrets` shader in `03a_oldmombasa`,
`03b_newmombasa`, `08a_deltacliffs`, `08b_deltacontrol` and `turf`.
Those passes require `prt_simple` lighting data. Successful ordinary dynamic
material admission does not supply that PRT bank or prove final pixels.

## Exact shader aliases

The original cache directories contain zero-sized shader aliases whose group
and address exactly equal an owned 84-byte shader payload. Rejecting these
aliases discarded real Ghost/Spectre damage materials. `Cache::tag_payload`
now resolves only a positive extent with the same group and address, and
rejects missing or conflicting owners. The alias retains its own datum/name.
The metadata store accepts overlapping aliases only for identical extents.

The opaque-selector regression now checks same-address aliases against their
owned original bytes and still rejects aliases without a payload owner; the old
blanket rejection of every zero-sized alias contradicted the cache directory.
The broader metadata regression passes all 23 maps, 2,642 complete tag records,
3,229,316 exact payload bytes, 2,498 node banks and 250 cross-tag banks. Its
retained fixture has nine historical paths that no longer exist; the separate
relocated fixture changes only those paths to the current owned map-pack caches.
Expected counts and all original byte records remain unchanged.

`original_vehicle_shader_aliases` checks these original directory pairs,
payloads, identities and prepared pass programs/constants:

| Cache | Alias | Payload owner |
| --- | --- | --- |
| 08a_deltacliffs | F4681235 | F30010CD |
| 08b_deltacontrol | F7EC1676 | EC830B0E |
| 08b_deltacontrol | F93117BB | EC830B0E |
| containment | F0EC0F65 | F04F0EC8 |

## Native rendering and ownership

The multiplayer vehicle path prepares resident authored dynamic passes before
Vulkan resources are created, selects the authored per-part shader LOD for each
view, poses the vehicle's current WORLD nodes and passes independent lighting
and color constants to each `ModelObjectDraw`. Shared materials/images do not
share a vehicle's draw constants. Initial colors use the actual placement and
selected variant.

For retained source vehicles, snapshots carry the ultimate parent datum and
definition, its actual lighting sphere center, original linear velocity,
runtime flags and lighting freeze state. The cache is keyed by the complete
salted root handle. Map/sky queries and cache updates use original object type
1. The original velocity routine is TU1.5 `BC200`, corresponding to base
`BA1D0` across all 107 normalized instructions; the correspondence and owned
executable hashes are retained in `velocity-correspondence-v1.json`. No motion
is estimated from rendered pose differences.

Native transport protocol 57 introduced the lighting fields; the isolated
checkpoint uses protocol 61 with subsequent gameplay fields and each model's
own geometry flags/control byte. The shared checkout has additional protocol
revisions from concurrent work and is a separate build boundary. The snapshot checker
rejects independent mutations of the lighting fields and both geometry fields. Real vehicle UDP
transport verification belongs to the live capture tool and must be reported
separately from that checker's sensitivity test.

The campaign scene path also uses authored dynamic shader selection, per-object
map/sky lighting and variant colors for admitted vehicle models. Its existing
placement/script pose owner does not yet supply original moving-body velocity,
freeze flags or ultimate-parent cache ownership. Unsupported PRT vehicles retain
the explicit diagnostic path and log the missing owner.

## Live verification and remaining gates

`vehicle_shading_capture` declares an authored Warthog placement and a camera
derived from that placement and the original BSP floor. It requests the real
source netgame/vehicle constructor before ticking. This explicit request is
not evidence that automatic vehicle-row scheduling is complete. The tool
requires actual source lighting owners, nonempty lit vehicle draws in every
local view, a saved frame and bit-exact lighting inputs through actual UDP.
Output directories must be fresh. A native screenshot from this tool is not a
matched original framebuffer comparison.

The initial `live-640x480-v1` attempt failed correctly: there were no live
vehicle owners or vehicle draws. Its scene image is not vehicle shading proof.
The coherent frozen build also exposed and corrected the BC200 call ABI:
EAX is the object handle, ECX the linear output and the sole stack argument the
optional angular output. Explicit child placement now records the actual source
parent, preventing a second scene registration of the same salted turret.

The accepted Warthog captures are `live-640x480-v9`, `live-1280x720-v2`,
`live-1920x1080-v2` and `live-1280x720-split4-v2`. Each requires nonempty valid
body/turret lighting, source ownership and exact UDP fields. The tool also checks
vehicle triangle centers against the actual submitted camera/frustum and original
BSP collision, so an occluded or offscreen submission cannot pass as a visible
vehicle. This geometry check supplements manual screenshot inspection; it is not
a GPU contribution comparison or original pixel proof.

`live-banshee-1280x720-v2` was a visible-frame failure despite successful draw
admission: its declared camera was above the base floor while the vehicle was
inside the room. It is superseded by `live-banshee-1280x720-v3`, whose camera uses
an original BSP floor and clear sightline in the room, with 1,386 unoccluded
triangle centers in the final view. Camera selection never moves the vehicle.
`live-ghost-1280x720-v2` passes the same visibility, lighting and UDP gates;
its final view has 1,184 unoccluded triangle centers. Warthog has 3,384 in each
tested view. All accepted capture logs have zero validation error/VUID lines.

Scorpion on Containment and Wraith on Waterworks exposed missing original
machine owners during source scene construction. The complete DEVI `10B5C0`,
MACH `117870`, pose-change `10C0B0` and portal `118120` callbacks are now lowered
to native C++ and joined to the retained live world. Their independent original
fixture also covers reached collision-type getter `A9BE0`: 160 cases,
5,328 executed instructions, full GP/XMM/flags,
untouched body bytes and exact ordered writes. All 161 birth/portal/type CFG
instructions and 46/63 fresh pose-change instructions execute; live animation
channel updates retain the existing external owner and are not claimed by those
cases. See [fixture provenance](../tests/fixtures/tu15_scene_device_constructor.md).

Both maps now pass device post-birth and update dispatch. The original
B87F0 -> 10D990 descriptor chain already constructs the device-group pool;
B8A00 invokes its new-map callback. The missing work was the complete DEVI
10C790 group/channel assignment, MACH 117700 and CTRL 11E0B0 post-birth,
DEVI/MACH update callbacks and reached Havok helpers. The generated module now
retains 6,728 original instructions, including the complete reached collision
math bodies; indirect calls keep strict real-owner dispatch. The expanded
independent original device fixture passes 344 cases, 23,026 instructions and
5,133 ordered writes without replaced instructions or services. Those cases
cover fresh device/group paths; they do not exercise every live animation or
Havok branch in the generated module.

Containment's Scorpion rows 71/72 are CTF-only. The old Slayer capture selected
one of those rows and reported its legitimate source rejection as a misleading
capacity error. The diagnostic now distinguishes source rejection. The capture
selects an engine-admitted authored row and can declare the original Primary
Heavy Vehicle option. Fixed choices 1/2 and None now prepare the source-selected
Scorpion/Wraith definition and variant on the original class 3 rows, including
rows with no default collection. Matg -> mulg references supply definitions;
no vehicle definition is inferred by name. Model children follow the selected
variant. Random remains rejected pending its per-birth RNG/template owner.
Overrides require the retained source world. An independent unchanged 15CC70
oracle checks 36 default/fixed/None cases on four owned maps, including original
position/rotation bits and 22 authored child templates.

Accepted normal native captures now include Wraith on Waterworks
`live-wraith-waterworks-1280x720-v9`, Scorpion on Containment with the original
heavy choice 1 in `live-scorpion-containment-1280x720-v11`, and Scorpion on
Waterworks in `live-scorpion-waterworks-640x480-v1`. They run eight normal source
frames, retain real lighting owners, verify UDP fields and have no Vulkan
validation errors. These normal screenshots alone remain distinct from
original-frame comparisons.

The private `original-session-v3` reaches Waterworks through normal original
menus and controller input. Captures authenticate the complete live TU1.5 text
and retain complete object bodies, original WORLD nodes, view inputs and cached
lighting records. `prepare_vehicle_shading_reference.py` executes unchanged
BC200 over the captured pointer banks to obtain actual velocities with no
service replacements. This avoids accepting the rejected partial bulk-RAM
export from `original-session-v2`.

The offline reference seam compiles only into `vehicle_shading_capture`;
normal `halo2_native` contains no reference lookup or image override. It supplies
captured camera, WORLD nodes, own coverage sphere, original velocity and flags.
Native lighting is computed independently. Other world objects, clocks, first
person and HUD are not restored; the whole image is not a matched simulation.
Original record word 20 is packed metadata `FFFF`, not a lighting float. All
20 actual shader lighting floats match bit for bit across all 16 matched body
and child draws for both vehicles in the latest captures. The capture tool now
rejects a mismatch of either these 20 floats or the captured geometry LOD. A
synthetic one-bit expected-lighting negative control verifies this rejection;
original capture files and gameplay remain unchanged.

| Vehicle | Original TU1.5 frame | Latest native comparison | Source geometry LOD | Strict pixels |
| --- | --- | --- | --- | --- |
| Wraith, Waterworks | `original-session-v3/wraith-view-v4` | `wraith-matched-inputs-native-v6/comparison.png` | 3, independently selected | Open |
| Scorpion, Waterworks / authored heavy option 1 | `original-session-v3/scorpion-view-v3` | `scorpion-matched-inputs-native-v5/comparison.png` | 4, independently selected | Open |

The previous vehicle path always used preview geometry LOD 5. It now prepares
all six literal source banks, with one material bank covering their section
origins, and selects per object and per view through the existing original
threshold, hysteresis and cap functions. Distance uses the own coverage sphere;
object +4 and +10A are transported separately from ultimate-root lighting flags.
Only nonempty admitted geometry updates the retained LOD byte. Four-player
captures on Containment and Waterworks select the source cap 2, with visible
lit tank draws and exact UDP state in every view.

The Scorpion comparison exposed a one-float-step lighting error. TU1.5 `11A360`
retains its horizontal square root through the final component multiplies;
native code rounded too early. The fixed helper passes 1,200 bit-exact unchanged
original cases, after the old code failed 469. See
[direction fixture provenance](../tests/fixtures/tu15_object_light_direction.md).

The Wraith GPU comparison also exposed a sampler conversion error. TU1.5 D3D
`409EE0` treats authored anisotropy 4 as ordinal 4, encoding exponent 3 and
selecting 8 samples. Zero replaces both min and mag filters with point; 1..4
replace both with linear and select 1/2/4/8 samples. The Vulkan conversion and
capability admission now follow that behavior. The independent full original
submission fixture passes 540 cases; the old conversion failed 228. Raw authored
state decoding is unchanged. See
[sampler fixture provenance](../tests/fixtures/tu15_texture_filtering.md).

Retained original and native GPU captures independently establish the following
bounded Wraith evidence: all 18 packed fragment constants agree for the sampled
body material; the original bump image matches the corresponding resident native
mip byte for byte; all 1,332 expanded vertices across three paired parts have the
same source screen XY and base/bump UVs. GPU output order differs, so the exported
vertex comparison excludes ambiguous coincident position/UV seams when measuring
lighting/reflection outputs. Those unambiguous outputs still have small floating
point differences. This is not an all-vertex or final-pixel equality claim.

Strict equality remains open. The latest Wraith rectangle `(215,163)-(363,226)`
has 3,376 unequal pixels of 9,324, mean RGB absolute error 0.24328/255 and maximum
channel difference 49. Before correcting geometry it had 5,969 unequal pixels,
mean 0.7833 and maximum 73. The latest Scorpion rectangle
`(43,225)-(345,357)` has 31,524 unequal pixels of 39,864, mean 2.31247/255 and
maximum 246; this rectangle also intersects HUD/reticle pixels and background,
so it is not an isolated tank-surface score. Its smaller retained interior
patches also differ. No threshold has been relaxed to report a pixel pass.

An explicitly labeled offline decoder isolation uses the already pinned Xemu
BC expansion before Vulkan sampling. It changes Wraith rectangle error only to
0.24153/255 and does not justify changing production texture storage. Ordinary
native runtime uploads retain authored compressed images. The original GPU
history and raw target also show that residual differences are not confined to
the display gamma pass. The retained Scorpion histories show a native fog
contribution at sampled tank pixels that needs a separate original compositing
trace; the original final additive draw must not be mislabeled as that fog pass.

The root shared build changed during compilation and produced inconsistent
runtime failures. Verification therefore uses the separately compiled frozen
checkpoint `.local/vehicle-parity-source-20261013-v1` and
`build-vehicle-parity-20261013-v1`. The coherent build passes the native
multiplayer test as well as the shader, opaque selector and light-range checks.
Installed launchers are independently managed by the current combined delivery;
these captures do not claim that delivery was replaced.

The tested isolated runtime is retained at
`analysis/vehicles/shading-tu15-20261013/verified-runtime-v5/` with shaders and a
hash manifest. `source-checkpoint-v6.json` hashes its coherent source checkpoint;
`vehicle-progress-v3.json` records binaries, captures, oracle counts and remaining
gates. All 16 freshly built selected tests pass in `isolated-tests-v10.log`.
The separate earlier full metadata regression passes in `isolated-metadata-v3.log`.
Installed launchers remain owned by the separately managed combined delivery.

The remaining sequence is explicit:

1. Trace the residual Scorpion compositing contribution against the retained
   original GPU draw history; finish unambiguous vertex-input attribution at
   coincident seams and strict vehicle surface comparisons.
2. Close vehicle damage/camouflage/engine/weapon function inputs and Scarab PRT
   data/draw ownership, with original evidence for each admitted shader path.
3. Supply campaign moving-body velocity, freeze state and ultimate-root lighting
   ownership; finish automatic vehicle-row scheduling and Random heavy choice.
4. Extend matched TU1.5 captures to further distances, resolutions, split-screen
   layouts and authored vehicle variants. Current native split captures are
   per-view integration evidence, not original split-frame comparisons.

Grenade lighting remains part of the larger goal. The combined original/native
28-draw light replay still has a strict framebuffer mismatch; exact command
banks and native vehicle map/sky lighting do not close that gate. See
[TU1.5 model light receivers](TU15_MODEL_LIGHT_RECEIVERS_20261013.md) and
[grenade light frames](TU15_GRENADE_LIGHT_FRAMES_20261013.md).
