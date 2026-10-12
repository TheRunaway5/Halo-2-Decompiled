# Live Lockout snow checkpoint

The latest [background weather checkpoint](SOURCE_WEATHER_LIVE_PLATES.md)
repeats the Lockout snow check: 2,452 changed RGB pixels, byte-identical enabled
repeats, and exact agreement with the preceding rain checkpoint. Lockout's
authored haze layers now render too. All 25 live cases have zero Vulkan errors
and no frozen source/shader drift. `tools/run_lockout_snow_test.sh` selects
that verified runtime. The historical records below remain preserved.

Lockout's snow now renders in ordinary native matches. This connects the
retained cold constructors, wind and per-view particle kernels to the actual
map sprite, original VS22/39780 programs and current source camera bank.
It is the first visible checkpoint for the active all-map world-feature goal.

`SourceWeatherPointDraw` carries current published immediate rows into each
`ModelView`. The renderer uploads the authored four-mip 32-square BC3 sprite,
retains independent point bindings for every local view, and records the pass
after late world effects and before first person. Split views use the full
surface viewport with their own original viewport constants and scissor.
The pass keeps RGB-only source blending, W-depth testing and no depth writes.
Resource requests touch the actual retained native allocation before drawing.

The strict captured-packet entry remains unchanged. The live entry removes
centers rejected by homogeneous W/source W clipping and handles the actual
device point-size range. OpenGL clamps programmable point sizes to its
implementation range; the Vulkan adaptation uses its queried device limits,
without treating the SDK fixed-size maximum as a programmable-point limit.
See the [OpenGL 2.1 specification, section 3.3](https://registry.khronos.org/OpenGL/specs/gl/glspec21.pdf).
These native limits do not establish physical Xbox raster precision.

The frozen test executable passes six live checks with zero Vulkan validation
errors: outdoor snow on/off/repeat, a normal spawn, four local views and four
local views joining a real dedicated UDP host. The exposed-walkway on/off
capture differs at 2,452 RGB pixels. The enabled repeat is byte-identical.
Only the snow draw is disabled by `HALO2_REFERENCE_WEATHER_HIDE`; initialization,
simulation, random draws and other world rendering still run. Those three
controlled captures use explicit finite-capture RNG/cadence inputs. The normal,
split and LAN cases use actual native session time/random inputs.

The earlier sheltered lift-room on/off pair correctly has zero changed pixels;
it is retained separately and does not count as visible-snow acceptance.
The first live attempt also exposed the strict prototype's point-range
rejection, which the live OpenGL-style range handling fixes. The extended GPU
check retains all original output controls and exercises world clipping and
subpixel point admission at four target sizes.

The final executable, commands, source/shader observations, screenshots and
comparison are retained under
`analysis/map-world-features/weather-visible-snow/actual-outdoor/`.
The original six-case executable is
`analysis/map-world-features/weather-visible-snow/halo2_native-tested-range`.
Run `tools/run_lockout_snow_test.sh` for an interactive Lockout test; walk out
of the starting room to see snow. `--local-players 4` selects four local views.
Existing installed launchers have not been redirected by this test checkpoint.

The subsequent nearby-object-query build repeats the visible on/off/repeat
check successfully, with the same 2,452 changed pixels and zero Vulkan
validation errors. That fresh frozen runtime also passes ordinary Zanzibar
startup. Its separate record is
`analysis/map-world-features/control-spatial-query-live/verification.json`;
the original six-case checkpoint remains preserved.

The unit-center kernel build also passes that four-case live regression:
2,452 changed RGB pixels, an identical enabled repeat and zero validation
errors. Its frozen runtime hash is
`42aec4bfe437b8d71ba6f3c37fc2feb8e896b3f8df71acf0a6b60f527f219f18`.
The captures and unchanged source/shader observations are recorded in
`analysis/map-world-features/unit-center-live/verification.json`.
This remains a weather/startup check; the new control unit-center kernel is
not yet called by normal gameplay.

The 2026-10-12 [live computer/gate checkpoint](SOURCE_CONTROL_LIVE_GATE.md)
uses a complete frozen source snapshot to avoid changing shared headers during
builds. Its ordinary native executable also passes fresh snow on/off/repeat
captures: 2,452 changed RGB pixels, a byte-identical enabled repeat and zero
Vulkan validation errors. All frozen source and shader hashes stayed unchanged.
The record is `analysis/map-world-features/control-runtime-snow/verification.json`;
the runtime SHA-256 is
`426068420f9cd4d4837ff319d16a050ea86a6d5f9f8b3b1b328d3a9ce9706c92`.
At that checkpoint `tools/run_lockout_snow_test.sh` selected that exact executable, with its
private frozen build's shader directory. The original six-case normal,
split-screen and LAN observations remain the evidence for those earlier runs;
the latest weather regression repeats the three controlled outdoor captures.

The subsequent [device-event checkpoint](SOURCE_DEVICE_EVENTS.md) repeats the
three captures with the newly built ordinary native executable: 2,452 changed
RGB pixels, an identical enabled repeat, zero validation errors, and unchanged
source/shader hashes. Its record is
`analysis/map-world-features/device-event-snow/verification.json`; the runtime
SHA-256 is
`38d8092c34c277a8370cc4fe3b189900c8cf6e0b17efa4b4fc73b673d59d8db8`.
At that checkpoint the snow test script selected this executable. The separate
`tools/run_zanzibar_world_test.sh` still selects it on Zanzibar.
Both scripts pass syntax and traced-execution checks recorded in
`analysis/map-world-features/device-event-snapshot-20261012/test-launchers.json`.
The ordinary title launchers retain the concurrent interaction-menu delivery;
the test scripts explicitly select this isolated verified checkpoint.

Rain and background plates now have separate live checkpoints linked above.
Other sprite profiles, complete original startup/time scheduling and the
remaining authored map interactions are still active work.
This does not claim full weather or all-map completion. The underlying kernels
and ownership evidence remain documented in
[the native producer record](SOURCE_WEATHER_NATIVE_OWNER.md).
