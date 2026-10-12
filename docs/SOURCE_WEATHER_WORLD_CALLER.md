# Retained weather caller and reference arithmetic

This connects the recovered producer phases over `SourceWeatherWorld`'s owned
resources. The normal multiplayer loop now initializes the authored banks and
runs their per-view producers using actual cameras, a persistent auxiliary
random owner and the existing native elapsed policy. See
[the native producer integration](SOURCE_WEATHER_NATIVE_OWNER.md).
[Lockout's live snow checkpoint](SOURCE_WEATHER_LIVE_SNOW.md) now connects the
point consumer. Rain and plate GPU submission, complete original scheduling
and startup random history remain unfinished.

## Original caller order

`36B50` advances the shared intensity/render clock on view 0 before its stopped
gate. After clipped-cluster palette admission, each admitted palette runs:

1. `4B3D0` wind on view 0 only, using the shared auxiliary stream.
2. All `37260` plate updates, then all `38450` particle updates while the update
   chain's result remains true. Particle elapsed is stored before the actual
   resource request, including a not-ready request.
3. A separately reset draw chain: all `37A60` plate draws, then all `391F0`
   particle draws. Each particle draw performs its own resource request.

The owner retains the complete mutable 176-byte weather root, mutable plate
banks and its published 20-byte render/32-byte particle-state rows. It accepts
the existing simulation-owned intensity and elapsed clock, an actual auxiliary
stream, explicit floating controls, an explicit reciprocal-square-root provider
and actual draw services. Missing services are rejected before mutation. Earlier
completed phases are not rolled back if a later external service fails.

`2BA10`, `2FD90` and `132D0` establish the weather-consumed view fields. The native
adapter copies the actual slot, camera position/forward/projection angle and
both current source transforms into an owned 244-byte bank. Other unused fields
are zero; this is not a complete original 664-byte view packet. A fresh frame
invalidates previous view banks. The ordinary native loop now supplies these
fields from the same camera and projection used for its clipped visibility.

`SourceWeatherParticleResource::submission` reads current published rows and
current system metadata for the original `391F0` primitive arithmetic. It does
not submit the decoder's cold particle copies. The ownership check mutates a
retained position twice and observes each corresponding submitted position.

## Original execution comparison

`tools/freeze_source_weather_world_caller.py` restores the actual saved Lockout
`36B50` entry, executes unchanged instructions and stops before the first
`37A60` draw. The actual call's weather, plate, system, render, state, auxiliary
and clock banks equal the independently captured original full return byte for
byte. No original instruction, service or draw result is replaced.

Seven cases execute 1,033,905 original instructions: the actual single-view
call, four independent explicit four-view partitions, empty clipped visibility
and stopped weather. Those partition controls reuse the actual input banks;
they are not a captured consecutive split-screen history. Native composition
matches all 95,473 compared output words, including shared intensity/record/time
and every retained producer bank. Five cases reach the actual first-draw
boundary; two return without drawing. A separate explicit false plate-draw
control checks that the submission chain stops independently of updates. It
does not establish an original GPU return value.

The mandatory fixture is `tests/fixtures/retail_source_weather_world_caller.bin`.
Its producer, snapshot and executable hashes are recorded in
`analysis/map-world-features/weather-world-caller-original.json`.

## Explicit xemu arithmetic profile

The pinned original-reference emulator is xemu 0.8.136, commit
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee`. Its
[actual `helper_rsqrtss` implementation](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/target/i386/ops_sse.h#L856-L867)
performs rounded float32 square root followed by rounded float32 division and
restores the previous SSE exception flags.

`source_weather_rsqrt_xemu_0_8_136` calculates precisely those two operations on
an admitted SSE host. It requires positive normal finite operands and observed
nearest/masked/noDAZ/noFTZ controls, preserving the caller's complete MXCSR. It
has no captured lookup, fitted approximation or native `RSQRTSS` substitution.
The caller must explicitly select this profile; the particle helper keeps its
provider input. This is an emulator profile, not proof of physical Xbox
approximation precision.

The calculated profile matches all 2,018 actual operand/answer pairs from the
seven unchanged original executions. Repeating the complete caller composition
with this calculated provider also matches all retained output words. Invalid
operands and DAZ controls are rejected without changing floating controls.
Source provenance is retained under
`analysis/map-world-features/weather-rsqrt-xemu-source/`.

## Runtime boundary

The native build and three focused CTest checks pass. All 23 supplied maps and
92 independently computed portal-clipped views pass the ownership checks,
including current mutable point/rain packets for the three weather maps.
Address/undefined-behavior/leak checks pass for the edited owner, memory adapter,
asset loader, initialization, wind, plate/particle simulation, calculated
profile and test drivers. Other linked cache-library modules are not instrumented
by these bounded checks.

The earlier admission-only integration passed eight actual native startups for
Lockout, Warlock and Backwash with one and four local views, plus Lockout clients
with one and four views joining real
dedicated UDP hosts. They present three hidden 1280x720 frames on RTX 5080 with
Vulkan validation enabled. All 20 first-frame views admit one palette and
retain their current 244-byte camera bank; all owners remain `initialized=0`.
No validation errors are recorded. The eight scene/view previews were inspected.
These are camera/resource/admission checks, not live weather simulation or draws.

All eight runs use a private byte-identical executable copy with SHA-256
`244216ec97b358d5a6f4179b624336a51f92a79de24f76d9dbed325dfd661240`.
Source observations changed during the shared build and CMake changed during
the runs. The frozen executable identifies the tested runtime; later shared
edits are outside its evidence. Full records are retained in
`analysis/map-world-features/weather-world-caller-verification.json` and
`analysis/map-world-features/weather-world-caller-live/`.

The source comparison imports an actual running state into owned resources for
the test. Its explicit cold constructor control is discarded. It proves no
normal startup seed, previous shared-stream consumption, asynchronous original
resource schedule or completed GPU draw. The separate live snow checkpoint now
supplies current inputs to the point consumer; rain and plates remain open. Launcher delivery remains
pending for the full map-world goal. The separate snow test entry now runs
actual authored particles through the normal renderer.
