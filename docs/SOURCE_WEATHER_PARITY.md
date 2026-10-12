# Original weather commands and retained state

The supplied retail executable is the authority: `assets/disc/default.xbe`, SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. Weather command/state parity is implemented in `source_weather.hpp` / `source_weather.cpp` and the `Simulation` script host. Weather particle geometry, cluster/portal visibility, authored particle shaders, wind, and the listener's weather gain remain separate rendering/audio work.

## Command and state ownership

`36AB0` initializes the global state to running, current intensity 1, target 1, no transition, rate 0, and previous render clock 0. `123D40` supplies a 24-byte game-state record separately from these globals. The commands `2A0AA0` (start), `2A0B20` (stop), and `2A0BA0` (change intensity) clamp duration to float `38D1B717` (0.0001 seconds). Change-intensity clamps its target to [0,1]. Every command writes the record and marker `DEADBEEF`, including a command which does not enter a transition.

`398D0` imports the retained state only when its marker is valid, advances the original start/stop/change state machine with the supplied elapsed float, and writes the record afterward. Start transitions always clear the stopped flag. A decreasing change to target zero sets it. A zero-rate change keeps its transition pending. No replacement easing curve is used.

The simulation checkpoint retains this 24-byte record. Render clock/global state is outside that checkpoint. The next `398D0` imports the restored record; the host does not immediately overwrite the render globals during restore.

## Source render clock

Only source view slot 0 enters `36B50`'s shared clock update and `398D0`. Additional local views leave this clock unchanged. Its elapsed value is the float rounding of `double realtime - double(previous float)`. The initial previous-zero case yields elapsed zero, and a negative delta clamps to zero. Rounding each absolute time before subtracting changes the original result.

The producer is now traced: `2B4A0` and `2B540` reset double `4BA040` to zero; `137ED0` adds the actual frame's allowed elapsed float to that double. `2B5D0` copies it into the render parameter packet and `12A50` copies the packet's double into `4858A0`. The `137ED0` instruction sequence at `137F4D..137F58` is `FLD float; FADD double; FSTP double`; its decompiler's cast of the prior double to float must not be reproduced. `146660` supplies allowed elapsed after pause/speed/network/tick-limit admission. This is not a wall-clock epoch or an independently forced 30 Hz weather update.

`Simulation::advance_weather_view` consequently takes the caller's actual admitted absolute render time and original view slot. The multiplayer loop does not yet activate all original map scripts or supply the complete source time-governor/network context. This API and its command integration do not imply a completed live rain renderer.

`source_game_time.hpp/.cpp` now implement the complete `1465B0` initializer,
`146980` speed quantization, `146860` ramp, `146660` governor, and `137F70`
admitted tick loop. Speed first rounds configured ticks divided by speed with
x87 `FISTP`, retains the low signed 16-bit effective tick rate, and derives
seconds per tick and scale from that rounded integer. The ordinary governor's
tick cap, synchronous one-tick allowance, fractional carry, pause gate and
speed-ramp updates preserve the instruction order. A network budget can
replace the game-time bank because the original reloads `510C54` after `680C0`.
No native LAN mode or network budget is inferred by this module.

`137F70` checks the actual stop service before each tick. An immediate stop
sets admitted elapsed to zero; a later stop subtracts the unconsumed ticks at
the current bank's seconds-per-tick rate and clamps the result to zero.
`source_world_time_accumulate` implements only `137ED0`'s final
`FLD float; FADD double; FSTP double` store. It must be scheduled after that
caller's admitted world-update services. It does not claim those services or
the platform/vblank-derived `12B0E0` elapsed producer.

`tools/differential_source_game_time.py` compares 11,712 original calls and
correlated governor/tick/clock frames, totaling 304,512 bit-exact output words.
The original governor, speed and ramp execute unchanged; network budget
`680C0`, stop query `12BE90` and tick body `137FE0` are explicit test services.
The clock comparison executes the untouched final store, without substituting
for earlier world updates. `retail_source_game_time` retains 456 original cases
and checks governed time feeding a weather fade, pause, immediate tick abort,
view-0-only clock ownership across four local views and fresh-map reset.
The verified environment is Linux x86-64 with x87 `027F` and MXCSR `1F80`.
The module does not change the caller's floating-point environment; other
platforms and floating-point modes have not been established by this corpus.

## Retained validation

| Evidence | Scope |
| --- | --- |
| `tools/differential_source_weather.py` and `analysis/multiplayer-shading/weather-differential.json` | 1,931 unchanged original command/state/record/first-view cases, 25,103 bit-exact output words; no source service hooks |
| `tests/fixtures/retail_source_weather.tsv` and `retail_source_weather` | 131 frozen original cases covering command clamps, valid/invalid/absent records, transition endpoints, negative clock deltas and additional view slots |
| `retail_backwash_weather_host` | Actual compiled Backwash startup commands through `Simulation`, first-view clock ownership, later authored weather continuation and retained-record checkpoint restoration |
| `docs/SOURCE_MAP_SCRIPT_PARITY.md` | 100,000 original/native Backwash dispatcher ticks and exact primary random/command ordering for five explicit input streams; complete launch/constructor RNG chronology is still a caller boundary |

## Wind and auxiliary random ownership

`source_weather_wind.hpp` / `source_weather_wind.cpp` reproduce the mutable
156-byte `weat+10` bank consumed by `4B3D0`. The original speed, gust direction,
gust magnitude, vector normalization and bend delta remain separate operations.
`36A40` consumes the auxiliary stream at `4E7408+4`; its x87 arithmetic retains
double precision until its stored float and subsequent SSE upper clamp. Rounding
the random sample early changes the wind. `30BF0` similarly computes its vector
norm in x87 before a stored float and SSE division.

Every admitted wind step consumes four auxiliary draws, including zero elapsed.
It leaves the primary stream unchanged. The API receives the caller's actual
auxiliary state and source elapsed scalar. `36B50` calls it only for source view0
and the palettes admitted by actual cluster/portal visibility while weather is
running. That visibility and the initial auxiliary chronology are not replaced
with guessed defaults, and this API is not yet activated in multiplayer rendering.

`tools/differential_source_weather_wind.py` executes all three untouched original
routines for 1,324 cases and compares 52,960 output words exactly. These include
100 consecutive authored-bank steps for each of Lockout, Warlock and Backwash,
followed by clamp, zero elapsed and degenerate normalization cases. The frozen
83-case fixture is checked by `retail_source_weather_wind`. The generated asset
inventory covers all21 multiplayer maps; those three maps are the only ones with
weather tags. `weat+0` holds particle systems and `weat+8` holds background
plates. Lockout and Warlock each have both blocks; Backwash has particle
systems without background plates.

The state and wind differentials are numerical evidence. They do not establish
physical Xbox weather appearance or Vulkan particle/texture/filter parity.

## Particle resource initialization

`source_weather_initialization.hpp/.cpp` implement complete `39A80` over owned
140-byte definitions and the caller's already published render/state arrays.
The definition's signed resource count at `+64` controls admission; the resource
group's signed count controls the loop. Zero or negative group counts still set
the initialized byte when the definition is admitted. Unadmitted definitions
clear `+80/+86`, set `+88` to one, and leave particle arrays and RNG unchanged.

Each particle consumes eleven auxiliary draws in the original order: mass,
three initial velocity components, size, three positions, and three noise
components. The original writes packed color `FFFFFFFF`, age zero, repairs
invalid mass bounds to `.1/.8`, and repairs each small view extent to one. Its
position extent order is `+10`, `+8`, `+C`. Velocity z is stored with `FST`,
retaining its unrounded x87 value for the vector norm; the stored norm is then
used by the SSE reciprocal and normalization. The primary random stream is
untouched.

`tools/differential_source_weather_initialization.py` executes unchanged
`39A80` without replacing any instruction or callee. All 1,548 cases match
201,719 output words exactly. These include complete authored arrays from
Lockout (1,024 particles), Warlock (70), and Backwash (150), each under four
explicit auxiliary seeds, plus signed count, mass repair, extent repair,
degenerate velocity and arbitrary untouched-field controls. All output banks,
return AL and the auxiliary stream are compared. The retained fixture is
registered as `retail_source_weather_initialization`; normal and fully
instrumented address/undefined-behavior/leak checks pass, including three
pre-mutation ownership guards.

Resource publication pointers/counts and initial auxiliary seeds in this
corpus are explicit controls. They do not establish the launch RNG chronology,
original async resource-cache chronology, subsequent `38450` particle simulation, or live
Vulkan weather rendering. The literal resource arrays loaded by
`Cache::load_source_weather_definition` remain pre-initialization data.

`SourceWeatherParticleResource` now owns the complete literal cache allocation
and descriptors in private native token-addressed storage. The loader retains
the actual wrapper and section header as well as decoded arrays. A synchronous
native request validates and applies the weather type0 subset of `223C70`:
the definition's `+64/+68` addresses its owned raw header; its two count/pointer
pairs address the same allocation's actual render/state payloads. A linked
request preserves that simulation storage and updates the owner's last-use
age. `13D230` age rebasing is retained. No decoded-copy pointer, fixed readiness
result or captured particle snapshot is substituted. This native one-block
registry does not reproduce Xbox heap salts, asynchronous I/O, pressure or LRU
eviction among unrelated resources.

`SourceWeatherWorld` retains these owners for the match/BSP lifetime. Its
explicit initialization method uses the supplied shared auxiliary stream in
`36AB0/39880` palette order, initializing all plates before particles for each
reference. It does not select a seed or keep a private weather stream. It also
keeps no duplicate intensity, command record or render clock; live admission
reads the existing `Simulation::weather()` state. The ordinary multiplayer
loop now passes its actual containing cluster, clipped portal cluster bank,
eye and local view slot into this owner. It now initializes and simulates the
owned banks using a persistent session auxiliary stream and the existing native
elapsed policy. [Lockout snow now renders in live matches](SOURCE_WEATHER_LIVE_SNOW.md).
Rain/plate GPU submission, complete original startup history and original
time-governor admission remain open; see
[the native producer integration](SOURCE_WEATHER_NATIVE_OWNER.md).

`tools/differential_source_weather_resources.py` executes unchanged original
`12DE70`, `223C70` and `13D230` for 1,020 cases / 49,980 matching output words,
covering actual Lockout, Warlock and Backwash allocation bytes, type0 counts,
linked requests and both age-rebase boundaries. Both paths preserve every
particle payload byte; full system and section-header words are compared.
The registered completed native I/O allocation is an explicit caller input;
original I/O is not emulated or claimed. No original instructions or services
are replaced in this resource comparison.

`source_weather_world` checks publication, stable mutable row lifetime,
pre-publication access rejection, atomic malformed-descriptor rejection,
explicit shared-stream constructor ordering, fresh-map ownership and four
local views. All 21 Xbox maps and both Vista-only maps pass, with 92 independently
computed portal-clipped views. Vista texture donors are configured in a
temporary test directory using the existing adapter, without modifying source
maps. Address/undefined-behavior/leak instrumentation covers the new owner,
runtime memory adapter and initialization module; the linked cache library
itself is not instrumented by that bounded run.

The earlier resource/admission integration build passed after registering the existing Vista
lobby metadata implementation with the cache target. Eight actual startup
runs also passed: Lockout, Warlock and Backwash with one and four local views,
plus Lockout clients with one and four local views joining real dedicated UDP
hosts. Each run presented three hidden 1280x720 frames on RTX 5080 with Vulkan
validation enabled. All 20 first-frame local views admitted their authored
weather palette; every resource was genuinely published. The eight captures
were visually inspected for scene/view submission. No validation errors were
recorded. All weather owners still reported `initialized=0`: these checks do
not show snow, rain, initialized particle motion or weather GPU submission.

The live runs used a private byte-identical executable copy with SHA-256
`feac745d168b2ad9f3ee97949b9a51c3ee4acc8ca6001dfcb3101484eef8bd12`.
Subsequent shared-checkout edits are outside that executable's evidence;
`analysis/map-world-features/weather-world-live/source-drift.json` retains
the observed changes. The current resource owner was rebuilt and its map,
original-code and bounded sanitizer checks repeated after the memory adapter
changed. Full hashes and scope are retained in
`analysis/map-world-features/weather-world-verification.json`.

The retained owner now also composes `36B50`'s palette loop: view-0 wind,
plate then particle updates, and a separately reset plate then particle draw
chain. It retains the full mutable weather root and consumes current owned
particle rows. The native camera adapter supplies the actual weather-consumed
fields and both source transforms. Seven unchanged original producer prefixes
match 95,473 native output words. An explicitly selected pinned xemu arithmetic
provider calculates all 2,018 observed reciprocal-square-root results without
a fixture lookup and preserves the full producer outputs. This profile does
not prove physical Xbox approximation precision. The normal loop now runs cold
initialization and these producers with shared auxiliary ownership. Complete
original time/random chronology and the remaining GPU draw services remain live work.
Lockout's point consumer now runs over these actual owned rows. See
[the caller evidence and boundaries](SOURCE_WEATHER_WORLD_CALLER.md).

`source_weather_initialize_plate` also implements complete `36F50`. It resets
the original fields in all four per-view 184-byte records, preserves all other
words, stores each authored layer velocity's magnitude, and consumes nine
auxiliary samples per view (36 total). The complete 936-byte Lockout and Warlock
plate banks under four explicit seeds and 1,536 additional finite velocity and
untouched-field controls match all 1,544 original calls / 362,840 output words.
The fixture is registered as `retail_source_weather_plate_initialization`.
Normal and address/undefined-behavior/leak checks pass, along with the short-bank
pre-mutation guard. This verifies initialization, with the same initial RNG,
later simulation, admission and GPU boundaries stated above.
