# Multiplayer teleporter TU1.5 parity goal

Wire authored teleporters into ordinary multiplayer matches using the original
English Xbox TU1.5 behavior. MCC is the runtime asset source. Original executable
execution and original maps are offline references, never supplied gameplay
outcomes or replacement collision results.

Authority: TU1.5 SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The existing paired instruction audit identifies TU1.5 `1A65F0` initialization,
`1A6830` update, `1A6A40` traversal and `1A66B0` occupied-exit query. The isolated
controller in `source_teleporter.cpp` does not itself provide those live world
dependencies.

| Acceptance area | Required evidence | Status |
| --- | --- | --- |
| Original control | Channel pairing, exact admission/ties, flags, cooldown, heading and callback order against unchanged TU1.5 | Existing 96 initialization/768 control fixture passes; whole original compiled update now owns live decisions |
| Normal match scheduling | Original engine phase, once per accepted authoritative tick, no advancement in display replicas | Whole1A6830 connected before shared player callbacks; first ordinary MCC tick passes |
| Occupied exits | Actual source collection/contact query, player flags and entity publication; blocked notification and retry | Original flag4 stores, blocked/walk-clear/remote retry pass on eight ordinary maps and on Relic after its actual switch opens the shrouds. Authored blocked caption reaches its local/UDP recipient and native split HUD; actual two-view capture inspected |
| Traversal | Real original pose/registration updates on the retained source body; authored entrance to exit | All 20 authored directions across nine MCC maps pass declared-birth coverage. Walking coverage uses 18 ordinary approaches plus both Relic directions after the switch |
| Local view | Original local heading and native camera/interpolation discontinuity | All four local indices and retained heading pass; short-distance/lost-snapshot body and weapon cuts pass. Rendered Beaver Creek walk traverses at tick19, with 21 submitted exit-camera views |
| Presentation | Original event22 selection and activation sound/effect handling; no invented full-screen flash | Actual original submission selects MCC teleporter_activate on all directions. Rendered walk observes one source request and one actual SDL audio submission |
| LAN | Host authority, remote/local traversal, completed pose/view discontinuity, repeat/loss behavior | Actual UDP client join, blocked exit, movement release, repeated input and delayed snapshots pass, including Relic. Second-machine rendered/audio acceptance remains open |
| Persistence | Teleporter timers/flags remain coupled to source authority continuation | Portable encode/decode and coupled checkpoint/restore while blocked pass; 17 malformed MCC cache continuations reject atomically |
| Live MCC acceptance | Real matches on authored teleporter maps, clear/occupied exits, local views and LAN | Source-world/input/render/audio evidence within the stated cases; physical Xbox visual/audio comparison remains open |

First regression: admit an ordinary player through the real source constructor
at a declared authored entrance in prepared MCC Beaver Creek, advance normal
`MultiplayerMatch::step`, and require traversal toward its actual paired exit.
No test writes an expected traversal pose, query result or contact into gameplay.
Later ordinary-input tests must additionally approach and leave the entrance.

Focused CMake checks use `tools/run_tests.py`. One-off original execution and
probes use `tools/run_test.py`; generated banks, binaries and captures stay in
`HALO2_TEST_SCRATCH` and are removed after validation. Keep bounded commands,
source hashes, comparison receipts and failures. Existing runtime selection,
original assets, reference evidence and unrelated changes remain owned inputs.

The wiring goal includes bounded evidence and explicit unresolved parity gaps.
A compiled controller or a declared-input pose test alone is not full
multiplayer parity. Passing the checks below does not certify every aspect of
TU1.5 presentation or gameplay.

## Current implementation evidence

`source_teleporter_tu15_generated.cpp` compiles eleven missing original bodies
(1,035 authenticated instructions), including the original locale loader,
reusing the existing live query, object,
player, event and camera owners with their full ABI. `1A65F0` initialization
already runs from the original new-map callback and is not reset per tick.
The live hook retains the registered-engine and raw-mode exclusions in15EBA0.
Generation evidence is in
`analysis/multiplayer/integration/teleporter-tu15-20261020/generation-with-locale.json`.
The earlier `generation.json` records the initial ten-body closure.

The first regression failed because the normal tick never called the update;
with the whole original phase connected it passes using the prepared MCC
Beaver Creek cache. Receipts are in
`build/test-results/teleporter-tu15-live-hook/`.
The MCC metadata allocation also needed the real native physical page owner
before original11EF70 could protect its static tag pages. BSP admission now
authenticates readable backing and checks write permission on each actual
destination. The native ownership/protection regression passes.

Native snapshots carry a persistent completed-teleport serial (protocol94,
including owning literal-message rows).
Body, held-weapon, root and heading interpolation cut when it changes,
including nearby portals and skipped snapshots. These fields are owning
native player values in coupled authority continuation; the original pair
timers and player latches stay in the existing retained source bank.

The original occupied-query object-type switch needs its exact24 readonly
bytes at16D27C/16D288. The local blocked-notice path also needs the original
empty UTF16 terminator at45C7CC. Relic's ordinary held switch reaches the
four-entry control dispatch table at11E260. These42 bytes are exported directly
from the pinned original executable and admitted through the existing
immutable-owner validation. They supply no query/contact, blocked state,
caption or success.
See `query-data.json` and `mcc-map-inventory.json` in the same evidence directory.
Loading the language resources also exposes1A7CE0's event token formatter
during Relic's longer route. Its fourteen complete UTF16 constants add432
readonly bytes, for474 total. `query-and-message-data.json` authenticates each
original push, token/format literal and terminator. The actual formatter,
player names and scores remain source execution outputs. The initial delivery
matrix exposed the missing `#cause_player` backing; Relic and immutable-owner
validation pass after admitting the exact constants.

Relic is `dune.map`; Tombstone is `highplains`/`highplains2`. Relic's tele_switch
shares position group0 with both teleporter shrouds, the gate and associated
devices. `relic_teleporter_gate_tu15_tests` declares one initial birth on the
actual floor near the switch. Ordinary look and held use select the real
control, change the original shared group at tick7 and advance its devices
through their intermediate poses. Ordinary walking then traverses both
directions. A real UDP participant walks from the switch to an entrance;
actual original contact queries block it until the local player walks clear.
No runtime pose, private spawn-bank edit or collision answer is supplied.

MCC teleporter conversion compares the original **loaded** flag bank. Unchanged
TU1.5 11F060 deliberately shifts two Beaver Creek exits and lowers Relic's
entrances. MCC already includes those changes. Raw Xbox serialization is not
the correct final target. The authenticated converter corrects only four
remaining Beaver Creek position/facing DWORDs, including original float
rounding; complete donor/target flag-bank hashes guard its atomic writes.
Modified/custom banks are preserved. `teleporter-profile-generation.json`
records all nine original/MCC contexts and the pinned unchanged-execution
receipt used for the two affected maps. The earlier
`relic-netgame-profile.json` is explicitly superseded raw exploration.

Coagulation pair0's entrance facing points away from its walkable approach.
The ordinary-input test declares a south-side initial birth and faces
1.5005064328485211 radians before original construction. It traverses at
tick19. The test does not move the player through the intervening wall.

Bounded final receipts: `build/test-results/teleporter-tu15-complete/` and
`build/test-results/teleporter-tu15-complete-matrix/`. The preceding
`teleporter-tu15-verified/` run passes all nine checks, including rendered
camera and actual audio delivery; the final run also includes Relic's real
switch/gate/LAN route. Earlier failing reports are preserved with their real
scope. Source hashes identify the controlled validation snapshot in this
shared dirty checkout. The later delivery now selects the tested snapshot in
`Halo2-Physics-TU15`, with executable SHA256
`6cde1f1bb094abef6f246003f71d04bdcdd8dbb77ab6dc41eb1d5b724167fda3`.
The immutable runtime, shaders, native assets and exact source archive are in
`analysis/multiplayer/integration/teleporter-tu15-20261020/runtime/`.
The previous runtime remains intact. Concurrent engine-migration source edits
after the qualification snapshot remain separate ongoing work; the delivered
source hashes and archive identify exactly what was built.

The final delivery matrix passed59/60 before the missing Relic formatter
constants were recovered. The subsequent focused run passed5/5, including
Relic, native cache continuation, immutable ownership and the rendered blocked
caption. The earlier caption/traversal/UDP cohort passed6/6. The flash
import/evaluation/render checks passed in the delivery matrix. Historical red
reports remain intact. After deleting the private source/build, the actual
selected launcher rendered40 frames /32 simulation ticks with two players at
ordinary authored Coagulation starts. Its opened-file audit found no original
XBE, foreign map or deleted build input. See `delivery.json`,
`launcher-startup.json` and `delivery-cleanup.json` in the evidence directory.
The retained runtime/source bundle is about216 MB; the1.82 GB temporary build
was removed, along with the inspected framebuffer and disposable test helpers.

Whole original1A9DF0 now owns language-resource allocation, actual MCC reads,
and the final ready flag. The source-only regression first failed with no
message; after this owner was connected, unchanged1A6530/1A9D90 produced
"Teleporter is blocked" through25AAF1. Native delivery observes the actual
B2CD0 unit event and literal/tick, resolves remote recipients through the same
original lookup with their real NONE local index, and carries owning rows in
UDP snapshots and coupled continuation. It supplies no blocked outcome,
localized text, local-view identity or lookup success. Other literal-message
producers, such as the original round-start caption, are outside this transport.

The native four-row submission/order and fade match268 original inputs,
27 complete queue/sort comparisons and56 exact fade cases, including the
unchanged original pow implementation and readonly text colors. The native
HUD uses the authored fonts, original split limits, offsets and pow fade.
The live regression compares the actual blocked text/tick/flags, checks its
native glyph vertices and expiry, and rejects cross-view leakage. Actual UDP
and portable continuation retain the message. A40-frame two-player capture
shows the caption only in the waiting lower view; one real activation request
reaches the SDL queue once. Receipts are under
`build/test-results/teleporter-delivery-20261020/` and `blocked-rendered.json` /
`caption-inspection.json` in the integration evidence directory.

Remaining parity acceptance is rendered/audio LAN behavior on a second
machine and comparison against physical Xbox output. This focused caption
coverage does not certify every message producer, arbitrary wrapped text,
font raster or visibility mode against the original full HUD renderer.
No teleport full-screen damage flash is invented: original traversal emits
event22 and selects the authored multiplayer activation sound.

The main `Halo2-Launcher` and `Halo2-Title-Launcher` now select this same
verified flash/teleporter runtime through `gameplay-runtime-selection.json`.
The record pins the executable, qualification receipt, resource inventory and
prepared MCC map hashes. Ordinary Play starts Coagulation with two local
players. Explicit multiplayer map/player arguments remain available within
the admitted map set; explicit menu requests retain the separately admitted
frontend. This selection does not grant the broader full-gameplay parity flag
or replace the independent physics-launcher selection.

Launcher-only validation passed all four selected CTest suites, including
tampered runtime/resource/map rejection, explicit menu routing, and the real
Tk Play-button behavior. A controlled main-launcher startup rendered40 frames
with two players and audited the executed native hash and opened inputs after
the private build was removed. The final installed launcher also rendered
four frames on the ordinary desktop with no environment override. Its pinned
Linux display preference is X11, addressing the observed SDL Wayland display
failure; an explicit user video-driver override takes precedence. Small
receipts, final launcher hashes and cleanup results are in
`analysis/multiplayer/integration/teleporter-tu15-20261020/main-launcher-wiring/`.
The existing runtime/source bundle remains required; the launcher build and
one-off scratch outputs were removed. The two original launcher executables
are retained there as about488 KB of rollback input.
