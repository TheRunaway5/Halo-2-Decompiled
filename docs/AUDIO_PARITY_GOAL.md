# Original Xbox sound playback goal

User request,2026-10-11: ensure all sounds play exactly where they belong.
The goal is active. It covers sound events, event time, emitter placement,
listener selection, authored variation/gain/pitch, looping/stop behavior and
delivery through local and LAN playback. Existing partial ports and passing
component tests do not establish completion.

Original executable instructions and owned sound/tag assets are the authority.
Unknown owners stay unresolved. Native stereo/device adaptation must be
identified separately from Xbox routing/DSP and physical speaker output.

## Current findings

| Area | Evidence and remaining work |
|---|---|
| Distance attenuation | Corrected a proven operand error in the native source admission/query owner.3,543 dense numeric controls now match both original updates exactly; independent original repeats match. See [distance correction](MP_SOUND_ADMISSION_NATIVE_INTEGRATION.md#distance-gain-correction2026-10-11). |
| Packet pitch and sample gain | Corrected ordinary caller/range pitch tuning and selected-permutation gain in the actual native adapter.97 actual packet pitch controls and42 mixed PCM gain controls fail before and pass after. Both original-image oracles repeat exactly. See [packet corrections](MP_AUDIO_PACKET_PARAMETER_CORRECTIONS.md). This is separate from spatial routing and the complete scheduler. |
| Actual initialized original admission | Six real requests from the original Xbox executable in xemu now match the native CPU owner: wind/canyon detail, SMG posing/fire, cement footsteps and a thrown grenade's metal bounce. Includes genuine rejection, active listener/local bindings, actual RNG,13 semantic stores and27 internal numeric returns. The old distance error fails on the captured grenade. [Capture scope and validation](MP_SOUND_ADMISSION_NATIVE_INTEGRATION.md#genuine-original-game-admission-captures2026-10-11). This does not install native spatial playback or the detail scheduler. |
| Original spatial CPU handoff | The complete game CPU spatial update now matches three genuine initialized Xbox calls, two exact36-byte driver requests,29 ordered retained stores and307,200 captured bytes.133 separate declared ABI cases, normal/ASan/UBSan builds and a deliberately removed-transform red control also pass. Actual driver SDK observations confirm the`x,z,y` handoff and retain environment/filter payloads. [Spatial scope and validation](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md). |
| Driver environment and filter CPU handoff | The complete driver/environment CPU owners and filter helpers now match one genuine initialized original call,186 declared original CPU boundaries and8,992 complete original helper calls. Normal/ASan/UBSan builds pass; wrong-axis and wrong-integer-conversion controls fail. The numerical oracle executes unchanged original32-bit instructions on the host CPU after isolating seven Unicorn invalid-log discrepancies. These are CPU/SDK payload checks; live SDK/HRTF/DSP integration remains open. [Driver scope and validation](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#driver-environment-and-filter-cpu-checkpoint). |
| SDK numerical owners and immutable data | All four numerical owners are implemented and match five genuine original calls plus17,924 declared original CPU cases in normal and sanitizer builds. Four isolated wrong implementations fail, two on genuine captures.20 complete SDK bodies and all61 stereo HRTF filters are paired across the three supported images, with an exact native immutable-data importer and bounded identity resolver. Retained SDK state and live DSP remain unfinished. [Implementation scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#sdk-numerical-implementation-and-immutable-data-checkpoint). |
| Original HRTF application | A separate genuine3812E0 call runs through both kernel lock owners, FIFO polling and all19 audio-device writes to its normal return. Actual backing, selected filter bytes and ordered command words are retained. This is Xbox-in-xemu device-command evidence; native HRTF application, physical DSP response and audible parity remain open. [Capture scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#genuine-original-hrtf-application-capture). |
| HRTF CPU command application | The complete3812E0 CPU branch/store/packing order now matches that genuine19-word command capture plus1,135 declared original CPU cases. Independent original repeats, normal/sanitizer builds, three isolated red controls and six required-service stops pass. Actual device locking, current algorithm dispatch and FIFO/DSP application are mandatory backend services; live native spatial output remains unfinished. [CPU scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#sdk-hrtf-cpu-application-checkpoint). |
| Retained SDK calculation | Complete37CA0A CPU update order and three additional numerical owners match two genuine original calls, including a changed source position, plus1,072 declared states and1,445 complete helper calls. All11,519 ordered stores and full retained buffers agree. Independent original repeats, normal/sanitizer builds, four isolated red controls and eight service/lock failure controls pass. Listener/source state, current algorithm dispatch and a real state lock remain mandatory integration owners. The next voice-parameter/environment owner and live device/DSP application remain open. [Calculation scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#retained-sdk-calculation-checkpoint). |
| Retained voice-data CPU | Complete37AE57 update order and its numerical dependencies now match one genuine no-op capture,2,825 declared original states,3,555 complete helper calls and8,757 ordered voice stores. Includes dense curve, unsigned cone, Doppler and environmental-filter controls across five floating-point contexts. Independent original repeats, normal/sanitizer builds, six isolated red controls and ten service/lock failure controls pass. All28 immutable ranges import exactly from three images. The original tracer's missing x87-store metadata is corrected and prior driver/calculation fixtures remain byte unchanged. Live retained/device ownership and audible delivery remain open. [CPU scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#retained-voice-data-cpu-checkpoint). |
| Gain, pitch and filter CPU commands | Complete381CE4/381D7C/381710 CPU owners and380B97/380DEB helpers match three genuine whole calls,2,526 declared controls,9,226 command words and1,483 ordered stores. Includes parent pitch, original recursive caller fragments and retained reloads between commands. All44 complete bodies pair across three images. Independent original repeats, normal/sanitizer checks, eight isolated red controls, twenty service/lock/count stops and ten integrated audio checks pass. Live SDK/device/DSP ownership and audible placement remain open. [CPU scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#gain-pitch-and-filter-device-command-cpu-checkpoint). |
| Combined spatial commit CPU | Complete381E2A dispatcher and382032 update now compose the actual native calculation, voice-data and gain/pitch/HRTF/filter owners. Two genuine whole original commits match, including31 actual device writes in the changed case. The817 declared cases and two captures cover266 complete CPU pipelines;8,871 command words and7,196 ordered stores agree with independently repeated original execution. Normal/sanitizer checks, seven isolated wrong implementations,28 required-owner/lock stops and11 integrated checks pass. Real retained SDK construction and live device/output integration remain open. [Combined scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#combined-spatial-commit-cpu-checkpoint). |
| SDK setters and driver operation bridge | Complete public/synchronized/retained position, environment and filter setters match three genuine whole Xbox calls and154 declared CPU cases. Position/environment updates remain deferred when apply bit0 is set; filter applies immediately. Both direct and driver operation entry paths match928 ordered stores and66 device words. Independent original repeats, normal/sanitizer runs, eight isolated wrong implementations,17 service/FP/publication stops and12 integrated checks pass.55 complete SDK bodies pair across three images. Real SDK construction and live output remain open. [Setter scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#sdk-setters-and-driver-operation-bridge-checkpoint). |
| World emitter placement | `SoundEvent`, `WorldEffectSound`, effect callback positions and posed foot markers now retain their positions through actual SDL submission. The adapter applies original distance gain with authored class/playback bounds and the nearest current local listener; actual near/far, moving-listener, split-screen and absent-listener PCM regressions pass. Fixed stereo output still lacks Xbox HRTF/DSP, occlusion, PVS and complete admission. Device/weapon/FP effect attachment routes and death-camera listener selection still require audit. |
| Retained SDK construction | Five genuine cold-boot constructors/binding calls now match the native retained owners,143 declared original CPU cases and1,967 ordered stores. Preserves selected-image vtables, actual reference dispatch, device defaults, per-voice slot indices, spatial list insertion and source-pointer reloads. Independent repeat, normal/ASan/UBSan, nine isolated wrong implementations, ten required-owner stops and13 integrated audio checks pass.68 complete SDK bodies and30 immutable ranges pair across three images. Allocation, hardware initialization, Release and live output remain open. [Construction scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#retained-sdk-construction-checkpoint). |
| Voice data initialization | Complete descriptor, source/default, format, mix-bin and buffer/cursor initialization now match two genuine whole cold-boot calls and260 declared original CPU cases:2,661 ordered stores and461 executed instructions. The two genuine cases also pass with real native storage, the source allocation owner and native recursive locks. Normal/ASan/UBSan,11 isolated wrong implementations, ten required-owner stops and14 integrated checks pass.81 complete SDK bodies and36 immutable ranges pair across three images. Real hardware creation, Release, live spatial output and audible/event parity remain open. [Initialization scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#voice-data-initialization-checkpoint). |
| Hardware voice activation CPU | Complete381AA9 activation and reached slot allocation, mapping and bin packing now match two genuine whole cold-boot calls and445 declared cases:4,664 ordered stores and9,304 device words. Gain, pitch and HRTF compose their actual native CPU owners. Both genuine cases also pass with real OS storage and recursive locks. Independent original repeat, normal/ASan/UBSan,11 isolated wrong implementations, eight required-owner stops and15 integrated checks pass.88 complete SDK bodies pair across three images. Actual device/output ownership, remaining creation/Release and live event/placement/timing parity remain open. [Activation scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#hardware-voice-activation-cpu-checkpoint). |
| Voice format and initialization dispatch CPU | Complete380900/38094B/382FE3/382CD4/382F58 now match two genuine whole original initializers and2,649 declared CPU cases:5,210 ordered stores and1,367 device words. Both genuine cases also use real native storage/recursive locks with actual native activation/gain/pitch/HRTF owners. Independent original repeats, normal/ASan/UBSan, ten isolated wrong implementations,16 required-owner stops and16 integrated checks pass.93 complete bodies pair across three images; original virtual format slots are imported exactly. Stop/drain/release/attachment remain required services when reached. Live output and event/placement/timing parity remain open. [Dispatch scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#voice-format-and-initialization-dispatch-cpu-checkpoint). |
| Sample-page ownership CPU | Complete manager construction/initialization, mapping acquisition, best-fit split, cached references, physical-address device commands and final-release merge/unlock now match two genuine whole Xbox acquisitions and228 declared original CPU cases:7,823 ordered stores and356 device words. Independent original repeats, normal/ASan/UBSan, nine isolated wrong implementations,11 required-owner stops and17 integrated checks pass.105 complete SDK bodies pair across three images; manager bindings import from exact constructor operands. Real platform page-pin/physical-address and FIFO/device services remain explicit boundaries. This does not supply live PCM/spatial output or complete sound events/timing. [Page scope](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md#sample-page-ownership-cpu-checkpoint). |
| First-person sounds | Existing tests cover actual source reload clock events, owner/view binding and SDL packet lifetime. Audit all admitted weapon/hand actions, suppression gates and remote event delivery. |
| Footsteps/materials | Existing tests cover authored gait events, posed markers, support material and original material/sweetener selection. Complete listener/PVS, spatial mixing and snapshot-loss behavior remain required. |
| World impacts/grenades/props | Existing rendered tests check actual SDL voice delivery/identity. Extend reference comparisons to every admitted effect event, creation delay, world marker and lifetime. |
| Ambience | Normal tracks and BSP palette/portal selection have reference tests. Weather alternates, detail schedules, start/end transitions and seamless packet boundaries remain unfinished. |
| Menus, announcements, music and scripts | Preserve existing menu/source queue/class-fade tests; inventory and compare every reachable publication/start/stop path. Campaign and currently unported event owners remain explicit work. |
| Platform output | Existing provider covers retained packets, actual completion, flush/error, rate and scalar gain. Xbox spatial routing/DSP, complete source scheduling/global RNG and original waveform comparison remain open. |

## Validation record

Current work and commands are under `analysis/audio-parity-20261011`.
The spatial checkpoint and frozen cache-library build are under
`analysis/audio-spatial-20261011`. Its new CMake-integrated actual-game spatial
test passes; independent normal and sanitizer builds also pass the actual and
declared fixture sets. This source-only spatial owner is not installed as a
live native mixer route. See the spatial document for the selected original
driver capture and precise boundaries.
A fresh driver source snapshot also builds the full cache library and passes
all four selected CMake checks: live admission, live spatial update, driver
parameters and the new driver spatial/environment/filter test. Its independent
normal and sanitizer checks pass, with two isolated red controls caught. This
remains CPU/SDK handoff evidence and does not install a live spatial route.
The SDK numerical implementation also builds through the complete cache library
and passes its integrated test alongside those four selected checks. The expanded
five-capture fixture passes independent normal and ASan/UBSan checks, with all
four isolated red controls caught. The further integrated frozen build includes
the three-image immutable-data importer and passes all six selected checks,
including its complete-byte regression. The SDK checkpoint retains the exact
snapshot and logs. Concurrent object-construction imports changed afterward;
those unrelated changes are outside this selected build. Neither build installs
a live spatial route.
The HRTF application addition builds through the full cache library in its
own frozen source snapshot and passes all seven selected CMake checks. Its
independent normal and ASan/UBSan builds pass the captured command case and
1,135 declared original cases, with all three isolated red controls caught.
The numerical SDK suite and four red controls also pass again. Original CPU
HRTF boundary results repeat exactly for all1,136 cases. Actual kernel locking,
FIFO/DSP processing and sound delivery remain required backend integration.
The retained SDK calculation addition builds through the same complete cache
library in a coherent frozen snapshot. All eight selected integrated audio
checks pass, including the extended three-image cone operand importer. The
original capture comparison, declared state/store controls and complete helper
outputs pass normal and ASan/UBSan builds. All four isolated red controls fail
as intended; two fail on the genuine changed-source capture. These remain CPU
state checks, with no live sound placement or audible parity claim.
The retained voice-data owner and all reached numerical dependencies now pass
their expanded original-backed normal and ASan/UBSan checks. All six isolated
red controls fail. The new coherent snapshot builds the entire cache library
and all nine selected integrated checks pass, including the extended three-image
voice-constant importer. Independent original replay agrees for all expanded
state/helper outputs and8,757 ordered stores. Earlier voice-data write-order
counts missed x87 FSTP stores due to Capstone operand metadata; the corrected
original observer and selected fixtures supersede those counts. The driver and
retained calculation fixtures were re-executed and remain byte identical.
The gain/pitch/filter device-command CPU addition builds the complete cache
library and passes all ten integrated checks in its coherent frozen snapshot.
Three genuine original calls agree at normal returns; declared controls also
cover original routing/packing, parent and recursive pitch, filter stores and
retained reloads. An isolated wrong-routing implementation exposed missing
parameter identities in the first declared test set; the selected v3 original
generator corrects those declared inputs. Genuine inputs and original source
are unchanged. This remains CPU/device-command evidence, not live DSP output.
Independent original replay is byte identical; normal and ASan/UBSan runs pass
the expanded fixture. Eight isolated wrong implementations fail their specific
controls. Twenty service/lock/count failures stop as required. Selected source,
reference evidence, logs and coherent build pins are retained in
`device-checkpoint-final/manifest.json`.
The combined dispatcher/update addition builds the complete cache library and
passes all11 selected integrated checks in its coherent frozen snapshot.
Original CPU replay independently repeats byte for byte. Normal and ASan/UBSan
runs pass, all seven isolated incorrect copies fail on their intended cases,
and28 required-owner/lock failure controls stop. The two whole original
captures execute real kernel locks, FIFO polling and device writes to normal
returns in xemu; host replay and native tests explicitly stop or model those
backend boundaries. Selected evidence, source and build pins are retained in
`commit-checkpoint-final/manifest.json`. No live native spatial route or audible
parity is claimed.
The SDK setter and driver operation bridge addition builds the complete cache
library in a coherent snapshot and passes all12 selected integrated audio
checks. The direct API and driver operation paths both match the original
fixtures. Independent original replay agrees byte for byte; normal and
ASan/UBSan checks pass, eight isolated wrong implementations fail, and17
required-owner/FP/publication failure controls stop. These checks preserve
deferred position/environment behavior and actual immediate filter commands;
they do not install retained SDK objects or native spatial output. Evidence,
source and build pins are in `setters-checkpoint-final/manifest.json`.
The first current baseline passes seven focused tests: caller control, source
lifecycle, menu music, frontend slots, authored multiplayer audio, locomotion
footsteps and material sound selection. These are component/native-provider
checks, not complete audible-game parity.

`tools/verify_audio_playback_baseline.py` freezes the tested runtime binary and
runs actual rendered local1/local4/LAN1/LAN4/slow-LAN delivery scenarios. It
records failures as well as passes. An initial current rendered run fails during
model preparation with `model skin bone outside skeleton`, before audio delivery;
that failure is not counted as an audio pass.

The isolated source build now passes all14 selected audio checks, including the
two packet regressions, the original admission suite and the new actual-game
admission capture regression. The new capture test also passes an independent
ASan/UBSan build, and fails with the prior distance operand error reconstructed
only in an isolated diagnostic copy. The
isolated native game and rendered test build successfully. A retained local1
rendered run fails before gameplay audio with `allocate diffuse memory: Vulkan
result -2`; its binary/map/XBE pins and output are in
`rendered-stable-pitch-local`. A preceding120-second run timed out and is retained
as an unsuccessful attempt. The runner now keeps streaming output and records
timeouts explicitly. No current rendered local/split/LAN audio pass is claimed.
The preceding voice-dispatch checkpoint inspected both launchers selecting
`build-unit-object-covenant-20261011/halo2_native`
(SHA-256`96a6eaf2c57c5b0465c221fe03b20bb035944a95aa7d36d8410bdef1cddf630e`);
this goal has not replaced or validated that selected build. Earlier inspections
selected the social and performance builds, so the
installed runtime must be checked again when delivering the completed audio work.

The voice-data initialization addition builds the complete cache library in a
coherent frozen source snapshot and passes all14 selected integrated audio
checks. Its262-case original fixture repeats byte for byte independently.
Normal and ASan/UBSan checks pass; all11 isolated incorrect copies fail on
their intended cases, and ten required-owner controls stop. Both genuine
cases additionally use actual OS allocations, the source allocator and native
recursive mutexes. The allocator comparison uses explicit raw ABI inputs;
this is component composition, not natural DirectSound startup or device/DSP
execution. Selected source, original evidence and build pins are in
`voice-init-checkpoint-final/manifest.json`. The live adapter still discards
emitter positions and uses fixed stereo routing; audible parity is unfinished.

The hardware voice activation addition builds the complete cache library and
passes all15 selected integrated audio checks. Its447-case fixture and full
original executed listing repeat byte for byte. Normal and ASan/UBSan checks
pass,11 isolated incorrect copies fail and eight required-owner controls stop.
Both genuine cases compose real OS storage/recursive locks and the complete
native gain, pitch and HRTF CPU owners. Declared FIFO and command sinks remain
explicit boundaries; they are not live DSP or audible output. Selected source,
evidence and coherent build pins are in `hardware-voice-checkpoint-final/manifest.json`.
The live multiplayer adapter still discards emitter positions. Creation,
Release, real output, scheduling and all-sound trigger/timing validation remain
required before this goal can be marked complete.

The voice format/initialization dispatch addition builds the complete cache
library and passes all16 selected integrated audio checks. Its2,651-case fixture,
executed-original listing and report repeat byte for byte independently. Normal
and ASan/UBSan checks pass; ten isolated wrong implementations fail and16
required-owner controls stop. Both genuine whole initializers compose actual
native storage, recursive locks and activation/gain/pitch/HRTF CPU owners.
Synthetic lifecycle and attachment responses are explicit ABI inputs, while the
genuine initializers reach none of those modeled boundaries. Those inner owners
remain mandatory services in production. Complete source, evidence and coherent
build pins are in `voice-dispatch-checkpoint-final/manifest.json`. Live sound
placement, actual buffer/device/output ownership, all-sound events/timing and
launcher delivery remain unfinished; the goal remains active.

The sample-page ownership addition builds the complete cache library and
passes all17 selected integrated audio checks. The230-case fixture and full
executed-original listing repeat independently byte for byte. Normal and
ASan/UBSan checks pass, nine isolated wrong implementations fail and11
required-owner controls stop. Both genuine acquisitions additionally compose
real native storage and recursive locks. Actual page pin/translation, FIFO and
MMIO are captured or declared boundaries; these are not native kernel/DSP or
waveform comparisons. Source, evidence, binaries and coherent build pins are in
`sample-pages-checkpoint-final/manifest.json`. Current launcher inspection selects
`build-interaction-prompts-20261011/halo2_native`; this audio goal has not replaced
or audio-validated that concurrently maintained build. Whole voice attachment,
remaining creation/lifecycle, real output and complete event/placement/timing
validation remain unfinished. The goal remains active.

The whole voice attachment addition builds the complete cache library and
passes all18 selected integrated audio checks. Its281-case original fixture,
executed listing and report repeat independently byte for byte. Three genuine
cold-boot calls cover new/cached buffered pages and a stream; native comparison
also composes actual OS storage and recursive locks. Normal and ASan/UBSan
checks pass, ten isolated incorrect copies fail and14 required-owner controls
stop. Source, evidence and coherent build pins are retained in
`voice-attachment-checkpoint-final/manifest.json`. Real page ownership/device
output and declared allocator/free/stop/drain boundaries remain distinct from
original CPU proof. Creation, remaining lifecycle, live routing, scheduling,
all-sound placement/event/timing validation and final launcher delivery remain
unfinished. This audio component checkpoint has not replaced or audio-validated
the concurrently maintained installed runtime. The goal remains active.

Completion requires an original-backed trigger/position/timing inventory,
corrected live routes, focused regressions that fail before each correction,
local/split/LAN runtime validation, original capture comparisons where available,
and rebuilt launchers. All remaining unported or unverified sound paths must
remain visible rather than being treated as completed scope.

The notification queue and pending-voice CPU addition builds the complete
cache library and passes all19 selected integrated audio checks. Two
independent unchanged-original runs repeat the274-case fixture and executed
listing byte for byte. Normal and ASan/UBSan checks pass,12 isolated wrong
implementations fail and12 required-owner controls stop. Every case also
uses real OS storage and recursive locks. Time/timer/DPC and forced-stop
inputs are declared boundaries; no genuine emulator case or native timer,
DSP, PCM or audible timing comparison is selected. The failed capture/replay
attempts and their limitations are retained. Source, evidence and coherent
build pins are in `notifications-checkpoint-final/manifest.json`.

The all-sounds goal remains incomplete. Complete3810BD forced stop and other
creation/lifecycle/device/output owners, live positional routing, scheduling,
all-sound event/placement/timing validation and final launcher delivery remain
required. Current launcher inspection still selects the concurrently maintained
`build-interaction-prompts-20261011/halo2_native`; these CPU component changes
have not replaced or audio-validated that runtime.

Current progress,2026-10-12: actual live world impulses now preserve positions
and apply original authored distance attenuation to PCM. A separate native
LAN regression exposed and corrected duplicated local SMG firing and lost
intermediate accepted weapon effects. The isolated corrected runtime passes
local one/four-player, LAN one/four-player and a deliberately delayed client;
authority firing events match real SDL submissions exactly in those runs.
`MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md` records the red/green evidence and
its limits. Complete retirement CPU owners additionally pass459 unchanged
original cases and their independent repeat, sanitizer, wrong-copy and
required-owner controls. This is real progress, not all-sounds completion.

The coherent `audio-delivery-source-v4` build passed and was installed in both
launchers. A concurrent prompt update then installed
`build-interaction-prompts-verified-20261012/halo2_native`. All 100 sound/module/
observer source files in that newer snapshot match the tested audio build.
The newer installed runtime is therefore retained and independently validated,
preserving its prompt/input and Vista pause fixes. Its actual gameplay and
title-screen routes pass through both installed launchers. Its 88-component
suite, all 14 TU1.5 runtime checks and seven TU1.1 runtime checks pass. Current
and prior selected source/asset pins, the initial failed test observation,
corrected independent sound observer and old-runtime negative controls are
retained in `audio-delivery-final/manifest.json`.
Directional/HRTF/DSP output, complete original admission/arbitration/PVS,
device/weapon attachment paths, sound detail scheduling, all authored loop
transitions and the full original event/timing inventory remain open. These
cannot be declared correct from CPU fixtures or SDL packet counts alone.

See [the installed delivery report](MP_AUDIO_DELIVERY_20261012.md) for current
runtime evidence and limits. Earlier checkpoint entries above describe their
historical launcher selections and CPU-only boundaries; they do not override
this installed delivery or establish completion of the all-sounds goal.
