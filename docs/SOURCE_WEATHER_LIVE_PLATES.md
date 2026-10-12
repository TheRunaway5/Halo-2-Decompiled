# Live Lockout and Warlock background weather

Ordinary matches now draw both maps' authored background weather layers,
followed by snow or rain. The previous accepted runtime changed zero pixels
when only plates were toggled. The fresh runtime changes 513,004 RGB pixels
on Lockout and 70,190 on Warlock, with byte-identical enabled repeats.
Snow and rain also remain byte-identical to the preceding accepted rain
checkpoint under the same finite inputs.

## Original submission and current inputs

`source_weather_plate_submission` reads the current owned 0x3a8 record,
one of its four view banks, the signed-short source rectangle and separate
selection/projection near/far inputs. It reproduces 37A60's opacity lanes,
tint, c21..23 scroll commands, half-pixel quad and immediate v0..4 registers,
then composes the retained 3A5C0/37DBE program/depth helper. NoDraw retains
scroll output without a quad and still returns source success. It consumes
no random draws.

The unchanged base-disc 37A60 and real SDK callees, entered through the actual
36B50 prefix, match 161 cases and 27,048 output words with zero mismatches.
The comparison verifies 2,886 distinct original instructions. Controls cover
all four slots, ordinary/odd/split rectangles, independent projection inputs,
NoDraw and the classified near/depth-disabled profile. Seventeen rejected
input controls and the fixture also pass ASAN/UBSAN.

Warlock arithmetic controls use its authored values while retaining the
Lockout capture's actual texture datums and controlled copied view rows.
They are not an observed Warlock source frame or draw history. An earlier
experiment inserting Warlock datums into that Lockout bank hit a real
external boundary; its failed log is preserved. The graphics consumer still
rejects the near/depth-disabled profile. Both maps' admitted plate intervals
exceed the current native near plane.

The live match now calls `SourceWeatherWorld::visit_view`, connecting the
source update/draw chain and stopped-state suppression to actual consumers.
Plate draws retain actual VS21, all three authored DXT45 bitmap mip chains
decoded through the existing pinned xemu BC3 integer decoder, and current
owned scroll/opacity banks. The native camera supplies near/far 0.015625/1024
independently to selection and projection. No captured camera, plate state
or stage0 texture enters the live draw.

## Current depth and frame ownership

The existing native world attachment is D24S8 with transfer-source usage.
After late world effects and before first person, the renderer ends the pass,
copies actual depth and stencil through `SourceWeatherDepthAlias`, then
resumes the load pass for weather. The three authored layers use their actual
mips with linear min/mag/mip and repeat. The alias retains exact integer
channel mapping and nearest/clamp sampling. Each local view has independent
consumer bindings and an alias allocation; the actual frame fence covers
all producers and consumers before reuse.

`prepared_binding()` exposes allocated descriptor handles without claiming
readiness. GPU consumption still requires ordered production. If recording
fails before submission, the renderer resets the actual never-submitted
command, discards the pending record, restores prior alias layout history
and signals the otherwise abandoned frame fence. The 17x9 GPU lifetime test
checks fresh and previously completed layouts, exact odd-tail bytes and
rejection guards. A deliberate match recording abort exits with the expected
error, no validation errors and no hang.

## Acceptance and delivery

The frozen source/build is
`analysis/map-world-features/plate-runtime-snapshot-20261012/`: 11,359 source
inputs, fifteen changes over the accepted rain source, a fresh 824-job build,
and all fourteen focused checks passing. The initial ownership failure was
a missing external Vista asset link in the snapshot; the supplied
needle/street_sweeper files and hashes are now recorded separately. The
ownership check covers all 23 supplied multiplayer maps and 92 clipped views.

`analysis/map-world-features/weather-visible-plates/accepted-v1/` retains all
25 live cases, commands, screenshots, repeats and comparisons. Cases include
ordinary one/four-view matches, real four-view UDP joins on Lockout/Warlock,
odd 641x481 four-view rendering, Backwash's no-plate regression and the
deliberate recording abort. Every case has zero Vulkan validation errors.
All frozen source/shader hashes stayed unchanged. Finite comparisons use
explicit cadence and seed inputs; ordinary and UDP cases use native session
inputs.

The first accepted executable SHA-256 was
`f42d2a4a34301fd72afec6cb5088066d7b9ffd02ec14eba51b72745d0da75385`.
The current helper delivery uses the combined MCC menu/HUD and weather build
described in [the launcher acceptance record](SOURCE_WORLD_LAUNCHER_DELIVERY.md).
`tools/run_lockout_snow_test.sh`, `tools/run_rain_test.sh` and
`tools/run_zanzibar_world_test.sh` now select that same verified launcher.
Add `--local-players 4` for split screen; rain accepts `backwash` as its first
argument. The ordinary root launchers are independently updated by concurrent
work; their current runtime is recorded separately in the delivery manifest.

Original-reference GPU checks retain their bounded evidence, including the
one-byte maximum RGB residual in the captured plate scene. Native Vulkan
response is not physical Xbox/NV2A raster proof. Original conditional
depth/stencil writer chronology, async cache admission, complete elapsed/RNG/
network scheduling, resets, scripts, trains, spawners and attached effects
remain unresolved. The full all-map goal remains active.
