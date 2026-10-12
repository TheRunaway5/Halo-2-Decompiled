# Weather assets, submission math and shader boundary

The authority is the supplied Xbox executable `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`, and its original
map resources. The new `source_weather_assets` module decodes those assets and
reproduces bounded original visibility, immediate particle input, pixel-program
and viewport arithmetic. Subsequent live consumers render
[snow](SOURCE_WEATHER_LIVE_SNOW.md), [rain](SOURCE_WEATHER_LIVE_RAIN.md) and
[background layers](SOURCE_WEATHER_LIVE_PLATES.md). The isolated proof records
below retain their original limits.

## Authored coverage

`weat+0` contains particle systems, stride `0x8C`. `weat+8` contains background
plates, stride `0x3A8`. Neither block is an ordinary render model. The decoder
retains both full authored records, their source bitmap references, particle
render/state arrays and raw resource descriptors. `223C70`'s actual resource
locators and section offsets determine the arrays; particle data is not generated
from a replacement distribution.

| Map | Authored particle system | Background plates | BSP admission |
| --- | --- | --- | --- |
| Lockout | Type 1 snow, 1,024 particles; `snow_flake` bitmap | One plate, three authored snow bitmaps | Palette 0 on clusters 0, 2, 7, 8, 9; 10 exclusion polyhedra |
| Warlock | Type 2 rain, 70 particles; no sprite bitmap | One plate, three authored bitmaps | Palette 0 on all 34 clusters; no exclusion polyhedra |
| Backwash | Type 2 rain, 150 particles; no sprite bitmap | None | Palette 0 on 12 of 43 clusters; 7 exclusion polyhedra |

The remaining 18 supplied multiplayer maps have no weather definitions in their
BSP weather palettes: Ascension, Beaver Creek, Burial Mounds, Coagulation,
Colossus, Cyclotron, Foundation, Headlong, Midship, Waterworks, Zanzibar,
Containment, Delta Tap, Dune, Elongation, Gemini, Triplicate and Turf. The
independent inventory covers all 21 cache files and retains their SHA-256 values
in `analysis/multiplayer-shading/weather-gpu-assets/inventory.json`.

`Cache::load_source_weather_definition` also retains the actual global `matg`
vertex programs: ordinal 21 (`weather_plate`, class 0, five instructions) and
ordinal 22 (`weather_particle`, class 0, nine instructions). Their payloads are
identical across the three weather maps:

| Program | Payload SHA-256 |
| --- | --- |
| Plate, 84 bytes | `c57b3f2509d664bba750f1b99fd09351efd0ddc4b9c5d2288e7fd2154896799a` |
| Particle, 148 bytes | `3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee` |

## Implemented original contracts

`source_weather_admission` reproduces the palette-selection portion of `36B50`.
Its input is the caller's **actual clipped visible-cluster list**. A conservative
PVS mask is not equivalent. The current cluster contributes distance `-1`; other
visible clusters contribute the nearest matching portal-plane distance from the
current cluster, with the original floating-point operation order. Palette
selection retains the original default distance 32,767 and ignores `NONE` tags.

`source_weather_submission` reproduces `391F0`'s immediate inputs. It clamps the
source view count to 1–4 and partitions each system into contiguous equal-sized
view ranges, discarding the remainder. Only particles with age greater than zero
are submitted. Type 2 emits line-list pairs: start is
`position - (velocity * source_elapsed) * streak_scale`, end is the current
position. The two authored opacity values are multiplied by **256** in extended
precision, truncated, and packed into alpha without a replacement clamp. Other
types emit point-list inputs with `v0.w = authored_size * 16777215`. Rain supplies
`v0.w = 1`. `v1` is the original packed color.

`source_weather_pixel_program` reproduces all 60 `39780` words for rain and
points. It does not substitute an ordinary authored material. Point sprites use
the source stage-3 bitmap binding; rain omits that bitmap bind.

`source_weather_viewport_constants` reproduces `12FA0`'s `c10/c11` for the exact
`391F0` call with resolution-scaling flag zero. For a source target rectangle,
the constants are `(width/2, -height/2, 16777215, 0)` and
`(left + width/2 + 0.5, top + height/2 + 0.5, 0, 0)`. Source render mode 2 uses
the original 128×128 target at origin zero.

The particle program takes the source camera projection `c0..3`, computes DPH,
uses the source viewport constants, divides by projected W, and writes point
diameter `(size * 16777215) / projected_W`. That diameter is in the source
raster target's pixel units. There is no additional authored viewport-width
multiplier. Display resampling and a modern physical target must preserve their
explicit mapping to this source target. The synthetic tests cover 640×480,
1280×720, 3840×2160 and 1920×1080 with 1, 2 and 4 view layouts; they do not
establish original hardware behavior at these modern display sizes.

The plate program passes position and texture stage 0 coordinates through,
then adds `c21..23.xy` to stages 1–3. `37A60` supplies those scroll constants from
each plate's retained per-view state multiplied by its authored texture scale.
Its rectangle edges include the original half-pixel offset.

## Validation and precision

| Check | Result and limit |
| --- | --- |
| `tools/differential_source_weather_geometry.py` | 754 unchanged-XBE cases, 10,274 bit-exact output words: 384 immediate geometry, 2 pixel definitions, 256 admission and 112 viewport cases |
| `retail_source_weather_assets` | Frozen original math plus independently parsed full metadata/resource/program fingerprints for all 21 maps |
| `source_weather_vertex_cpu` | 448 independently evaluated analytic cases, all output words exact with the actual authored programs |
| `source_weather_vertex_gpu` | The same programs/inputs through the shared Vulkan interpreter on RTX 5080; colors, projected W and plate UVs exact; only reciprocal consumers differ |

The GPU analytic fixture has 59 differing words. Its maximum output differences
are screen X `0.0001220703125` pixels, screen Y `0.00006103515625` pixels,
unnormalized D24 depth `0.001953125`, and point diameter `1.192092896e-7` pixels.
The test admits at most four ULP in particle position XYZ and two ULP in point
diameter; every other output remains exact. CPU words remain strict. These
measured Vulkan reciprocal bounds are **not captured NV2A output** and do not
justify changing production dot/division arithmetic to imitate an emulator.
Fixtures and current binary/program hashes are recorded in
`analysis/multiplayer-shading/weather-vertex-validation.json`.

## Required live services and GPU work

`36B50` calls weather after late world geometry and before first-person rendering.
The existing source command/intensity clock and wind modules are documented in
`SOURCE_WEATHER_PARITY.md`. Live rendering still needs the following source
inputs and services, with their actual producer chronology:

1. Actual containing cluster and clipped portal visibility, source view count
   and view slot. Weather admission cannot use every authored cluster or only a
   conservative PVS upper bound. The live multiplayer loop now supplies its
   actual clipped cluster bank to `SourceWeatherWorld` for every local view.
   Its loaded native particle allocations are published and retained. The
   normal loop now initializes and simulates those resources.
   Lockout snow, Warlock/Backwash rain and Lockout/Warlock background layers
   now have the live GPU consumers linked above. Other sprite profiles remain
   outside the supplied maps' authored weather acceptance.
2. Source admitted absolute render time, intensity state and complete auxiliary
   random stream. Only view 0 advances the shared weather clock and wind.
   `4B3D0` consumes four auxiliary random draws even at zero elapsed.
   The original time governor and tick-abort clock adjustment are now ported
   and compared against 11,712 original cases; platform elapsed admission,
   network budget and upstream world-update services remain live caller work.
   The current native integration explicitly uses the match loop's existing
   elapsed policy and persistent session auxiliary word. It does not claim
   original vblank/governor admission or complete startup random history. See
   [the native producer boundary](SOURCE_WEATHER_NATIVE_OWNER.md).
3. Original cold constructors and per-view simulation. `39A80` overwrites the
   decompressed particle resources and consumes 11 auxiliary draws per particle;
   the decoded arrays are not asserted to be a live simulation snapshot.
   `38450` needs camera boxes/transforms, wind, source elapsed, intensity,
   exclusion polyhedra and portal distance. `36F50` initializes all four plate
   view banks; `37260` advances those banks using the same source state. These
   constructors and per-view producers now run in ordinary native matches,
   using actual current cameras and the calculated xemu arithmetic profile.
4. Original particle and plate GPU submission. The ordinary world shader does
   not expose `oPts` as `gl_PointSize`. A dedicated particle path must retain
   source projection/W-depth, point size, point-sprite coordinates and line-list
   interpolation. `391F0` explicitly sets SRC_ALPHA / ONE_MINUS_SRC_ALPHA blend,
   rounded-byte alpha test GEQUAL zero (method033C=0x206, reference0340=0),
   depth comparison LEQUAL, W-buffer enable and no depth
   writes. Non-rain enables the original point-sprite/point-scale state; rain
   supplies its authored line width. Cleanup writes the source named defaults;
   it does not restore arbitrary prior state.
5. Plate compositor `37A60` and `3A5C0` mode 3: actual retained scroll/opacity
   banks, three texture bindings, depth blend intervals and the current source
   depth target. No substitute fullscreen alpha quad is claimed.

The original Lockout rendering context is now retained in
`analysis/multiplayer-shading/original-weather-20261009/session1/original-lockout-gl-source_capture.rdc`.
This recording contains multiple original frames from xemu/OpenGL llvmpipe;
it is not physical Xbox output. Event215873 retains the actual1,015-point
draw, original camera, four resident sprite mips, and scene color/D24 before
and after drawing. Event215778 separately retains the actual four-vertex plate
strip, all three seven-mip layers and the current packed depth alias. Every
one of its307,200 alias pixels matches the actual D24/stencil byte lanes.

`SourceWeatherPointRender` implements the isolated non-rain pass over explicit
current inputs. Its matched-scene Vulkan check preserves every original depth,
stencil and alpha byte. Of307,200 RGBA pixels,306,471 match exactly;729 differ,
including13 original-only visible effect pixels at point-square edges. The
explicit original-GL raw-row coordinate adapter follows the captured upper-left
point coordinate state. No point position/size or color adjustment is fitted to
these residuals. Controlled target banks retain the captured projection and
packet, so they do not prove full live resolution scaling. Details and normal/
sanitizer manifests are in [the point module record](SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md).

`SourceWeatherPlateRender` implements the isolated actual VS21/mode3 strip.
The complete unchanged3A5C0 output agrees with all60 retained pixel words and
all17 captured packed constants. Its matched-scene Vulkan pass differs at14,477
pixels by at most one RGB byte and preserves every original depth, stencil and
alpha byte. The one-byte bound is a measured emulator/host regression check,
not physical NV2A equality. Unsupported derived vertex outputs and hardware
features reject before mutation; direct record after rejected preparation and
invalid viewport/scissor controls preserve the existing valid bindings. Details
are in [the plate module record](SOURCE_WEATHER_PLATE_GPU_PARITY.md).

The standard build registers both modules in `halo2_weather_render` via
`cmake/SourceWeatherGraphics.cmake`. Retained-capture tests are optional through
`HALO2_WEATHER_REFERENCE_TESTS`; all six registered checks pass in the separate
CMake integration build. This library registration supplies the explicit-input
passes. The later live plate checkpoint connects their current producer chain;
chronology and hardware limits remain separate requirements.

The source ready-particle arithmetic and pure mode3 compositor builder are
separately registered in `halo2_weather_simulation` through
`cmake/SourceWeatherSimulation.cmake`. Four source-fixture checks pass in their
private CMake integration build. The particle helper owns the pre-gate elapsed
store as a separate phase and requires an explicit original ISA result for
each velocity-limiting reciprocal square root. It compares46 calls/33,306
words without inventing resource readiness. The compositor builder compares
2,440 complete original calls/153,720 result words, but the graphics pass still
admits its independently captured program only. These helpers do not supply a
resource-ready service, current visibility or a live scheduling timeline.

Actual complete36B50 entry/return snapshots also retain the caller's admitted
palette, first-view clock/wind order, two ready services and bitmap requests.
Replay reaches the uncaptured GPU queue read at3FA886/FD800044 with all seven
named simulation/timing producer banks equal to the actual return. The caller's
remaining SDK tail is unresolved; see
[the bounded caller evidence](SOURCE_WEATHER_CALLER_CAPTURE.md).

The match/BSP owner now composes those producer phases over its actual retained
weather/plate/particle banks and retains each native view's consumed camera
fields/transforms. Seven unchanged original prefixes match 95,473 output words
through the first actual draw boundary. The independent calculated xemu 0.8.136
profile matches all 2,018 observed ISA pairs and the repeated full outputs; it
does not establish physical Xbox approximation precision. Current-row primitive
packets also read the owner's published mutable rows. The subsequent live
checkpoint calls the graphics consumers through `visit_view`. See
[the retained caller record](SOURCE_WEATHER_WORLD_CALLER.md).

`SourceWeatherDepthAlias` now produces the exact packed texture from a supplied
actual current D24S8 image. Its integer compute path and owned nearest/clamp
sampler match all307,200 original alias pixels. A single submission connects
that generated binding to the native plate consumer and preserves every depth,
stencil and target-alpha byte; its output equals the preceding native plate
result. The separate alias library and reference tests are registered through
`cmake/SourceWeatherDepthAlias.cmake`. The combined private CMake build passes
all14 registered weather checks with SDK synchronization validation.

The separate connected-reference check now records current D24 → alias →
plate → snow in one submission. Its intermediate plate equals the preceding
native bytes, and every307,200 depth/stencil/target-alpha pixel is preserved.
The original plate output and original snow input match all1,228,800 bytes.
Two additional registered checks pass in the separate connected build; they
do not reattribute or rerun the prior14-check record. The final comparison has
15,159 differing RGBA pixels, mostly within one RGB byte, with the same13
source-only snow coverage pixels and maximum18-byte channel difference.
See [the connected scene record](SOURCE_WEATHER_CONNECTED_SCENE_PROTOTYPE.md).

The native renderer's integer-depth resource migration passes its separate
full-game build and42 native map captures. It selects D24S8 with transfer-source support and a
present graphics+compute queue, retains a depth-only sampled view, and stores
stencil through LOAD continuations. This change preserves existing native
zero initialization; the original conditional clear lifecycle and dedicated
sky stencil writers are separate pending source scheduling work. Current-bank
ownership and weather producer activation remain unresolved. The isolated
module does not reinterpret a float target as packed original depth.
The source integer-depth encoder is now connected to world/light-volume
fragments after an exhaustive actual-GPU16,777,216-input proof. Its final
native map/resize checks are recorded separately from the resource-only stage.
See [the native resource/encoding record](SOURCE_WORLD_D24_NATIVE_MIGRATION.md).

The plate module also accepts a typed original37A caller input. It derives the
AboveNearWDepth profile and exact program/quad outputs before binding; near
depth-disabled and zero-W source cases remain explicitly unsupported. Normal
and address/UB GPU checks match the prior native plate bytes,19 rejected inputs
preserve retained bindings, and52,832 test-only combiner samples match rounded
scalar bytes. This does not manufacture current live weather producer state.
See [the typed preparation proof](SOURCE_WEATHER_PLATE_TYPED_PREPARATION.md).
Its regular CMake integration now passes21/21 normal and21/21 sanitized checks
(13 CPU,8 GPU each), with identical native readbacks and zero SDK synchronization
errors. The separately frozen [current integration](../analysis/multiplayer-shading/weather-typed-cmake-integration/README.md)
preserves the older14/connected2 and standalone typed records.

The native stencil-only sky diagnostic also matches every original D24/S8
pixel across retained A/B/moon geometry and their single-fence sequence.
Both retained VSOut+literal-GS and exact-GSOut paths pass normal/address-UB
comparisons and controls. Its462 far-depth samples bound this endpoint proof;
live sky owner/clear scheduling and authored native vertices are still separate.
See [the sky producer proof](SOURCE_SKY_STENCIL_PROTOTYPE.md).

The source [view/clear audit](SOURCE_VIEW_CLEAR_POLICY_AUDIT.md) now establishes
that ordinary initialization precedes accumulation and that world, dedicated
sky and the late/FP bracket preserve the same depth/stencil attachment. Live
activation still needs the actual caller/transition/override and sky-owner
inputs, plus initialized-region coverage after allocation/resize. The
[bounded native history correction](SOURCE_NATIVE_VIEW_HISTORY.md) now preserves
already initialized world depth/stencil through native accumulation, sky and
first person, and retains FP accumulation color. It keeps existing native
initialization and does not infer the missing original132D0 inputs. The fresh
application is installed through both existing launchers with its separate
42-map native/resize/scenery/CPU validation in
[the delivery record](../analysis/multiplayer-shading/native-view-history-retention/launcher-delivery/README.md);
this does not activate the isolated sky/weather producers.

These captured passes and isolated modules do not establish full weather
simulation, rain raster coverage, physical NV2A limits or final game appearance.
Live activation still requires the actual current time, auxiliary random state,
clipped visibility, resident ownership and plate/particle producers listed
above. The graphics modules do not substitute authored cold arrays for current
simulation output or manufacture those producer inputs.
