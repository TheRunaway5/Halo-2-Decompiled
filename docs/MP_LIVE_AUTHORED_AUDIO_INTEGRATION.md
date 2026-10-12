# Live authored multiplayer audio

Reload sounds, BSP ambient loops and walking footsteps now reach the native SDL
playback provider in the playable multiplayer runtime. Actual SDL keyboard
fire/release/R/W input passes in local one-player, local four-player, native-LAN
one-player and native-LAN four-player Lockout sessions. This is a native platform
adaptation using original assets and recovered selectors; it does not claim
complete Xbox sound-scheduler, DSP, PVS or hardware waveform parity.

`MultiplayerAudio` retains and decodes every permutation of each selected sound,
uses the recovered gain/pitch/permutation kernels and selected-image driver
conversions, submits real retained PCM packets, consumes real provider completion,
and releases voices on completion/reset/exit. The output matrix is explicitly
native stereo. The secondary RNG uses the recovered seed recipe with host clocks;
it is separate from the unreconstructed global Xbox sound-consumer order.

The existing local first-person event owner now delivers actual type1 events from
the current primary hand's real source clock and salted weapon/unit binding.
The LAN client consumes events from its received self-contained one-tick clock
pair once, without advancing the authority's clock again. Shot/dry-fire and prop
sound requests also enter this adapter; sounds are not inferred from render
frames or clip names. The new audio observer runs only after actual accepted SDL
submission and reports sound, owner, view, selected permutation, voice and frames.

The BSP importer retains literal palette/cluster/path/PAS or encoded-distance
data. Target selection follows Base `18D730→18AE80`, portal distance `14B240`,
encoded distance `249D60`, and the original `13DE30/13E0E0` sort including ties.
The first actual local camera supplies the native listener position and cluster.
The original source-backed class fade gains feed all active native voices.
This selected native map/camera binding is not a claim that original `11E8E0`
resource activation or Xbox listener/PVS initialization has executed.

An all-map probe initially exposed Backwash and Desolation alternate-only rain
tracks. Rejecting those tracks had incorrectly discarded their ordinary ambient
loops. Unchanged Base `21A250` selection instructions (`21A712..21A80B`, stopping
before creation) now establish the normal lane: initial phase0 and alternate0
select literal track+20, or NONE, and retain desired phase1. The native adapter
keeps the ordinary loops and leaves inactive rain silent. It does not invent a
weather activation. The complete body maps to 1.1 `21C360` and authenticated 1.5
`220C60`; the complete looping caller `18A7B0` maps to `18C1E0` / `18D5A0`.
All three literal seven-word phase tables match across the images. There are
4,838 measured normal/alternate slice controls over every resident `lsnd` track
on 23 owned maps, with identical outputs across all three images and an
independent Base repeat. These are declared raw-ABI controls, not fabricated
guest admission, device or weather state.

Walking presentation uses the original directional classifier, imported
deadzone word (1.1 `4654C0`, 1.5 `46A560`, bits `3C23D70B`), authored weapon-action
bindings, complete variant selection and channel clock. Real native consumed
control axes and collision support are explicit inputs. No retail physical-mode
record is synthesized from `Body::grounded`. Actual timing events2/3/9/10 choose
the recovered sound kind/marker; idle does not create a timer cadence. The body
renderer consumes this same authored gait and fractional clock sample. An actual
grenade primary channel owns its layer while throwing, preventing a second gait
event clock from running over it.

Footstep sound selection executes the original material-parent, material-effects
fallback and sweetener rules. Whole unchanged Base `1883E0`, 1.1 `189E10` and 1.5
`18B1D0` produce identical 154 Lockout controls, with a repeated Base run. The
native comparison checks all three sound outputs. Shared serialized entry arrays
are read from their actual owning tag; they are not relocated into the foot tag.
Actual collision support resolves BSP local material to the authored global
material. Actual current posed foot markers supply emission positions, with the
recovered 10-unit cull adapted to the first native listener. Appended prop
materials have a distinct namespace and are not misread as BSP materials.

Native snapshots now retain consumed movement axes, crouch, actual grounded
state and actual BSP support material. Clients cut gait continuity at a missing
snapshot interval rather than invent intervening control/animation states. A
retained authoritative primary-channel/event replica remains necessary for
complete loss-tolerant remote gait and footstep continuity.

## Measured validation

Evidence is under `analysis/multiplayer/integration/live-authored-audio-20261010`.
The source authority is the original disc SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`, disc 1.1
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and owned
authenticated 1.5 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

| Actual X11 rendered scenario | Render frames / simulation ticks | Reload / ambient / shot / footstep packets |
|---|---:|---:|
| Local1 |386 /171|1 /2 /8 /7|
| Local4 |148 /172|1 /3 /8 /7|
| Native LAN1 |313 /172|1 /2 /8 /7|
| Native LAN4 |196 /173|1 /2 /8 /7|

These runs use the pinned `runtime-w-probe.bin`, actual Vulkan rendering, isolated
X11 SDL input and actual SDL dummy-device callbacks. They predate the narrow
inactive-alternate correction and subsequent shared grenade protocol23 changes;
the exact pinned binary is retained and is not described as the current full
source tree. That correction does not change Lockout's normal tracks.

The final production CPU/native-provider tests run both one/four-player reload
clocks and normal ambient PCM/lifetime on all 23 current owned maps with the
authenticated 1.5 provider. Every map passes, including Backwash and Desolation.
The focused walking test runs actual one/four-player Match movement, six authored
timing events and six real support contacts per case, actual SDL footstep packets,
and stopped-idle checks. Final normal and GCC ASan/UBSan runs pass all 23 maps and both focused tests.
The final audio tests also check that actual native mute/unmute preserves ongoing
voice ownership while silencing/restoring PCM. Exact commands are retained with
the freeze. Earlier interrupted/failed reports remain separate.

Reproduce the focused tests with `ctest --test-dir
build-animation-metadata-store-20261008 -R
'^(multiplayer_authored_audio|native_authored_footsteps)$' -V`. Run
`tools/probe_live_authored_audio_maps.py --binary <multiplayer_audio_tests>
--output <new-directory>` for the 23-map native-provider scenario. Original
selection tools are `oracle_native_footstep_selection.py`,
`oracle_native_ambient_track_selection.py`, and
`research_native_ambient_owners.py`.

Remaining boundaries include source global scheduling/RNG order, positional
attenuation and full PVS, Xbox mix bins/DSP, weather/alternate activation,
ambient detail scheduling, start/end transitions, multiple pitch-range admission,
and exact physical walking admission. Loop continuation currently follows actual
native packet completion on the main update and can have a boundary gap.
Physical speakers, subjective mixing and original Xbox waveforms were not tested.
The tests establish audible nonzero native PCM and real SDL submission, not 100%
audio or sandbox parity.


## Frozen checkpoint and provenance

`analysis/multiplayer/integration/live-authored-audio-20261010/frozen/manifest.json`
pins the exact tested binaries, observed production archives/build flags, source
snapshots, original reports/fixtures and runtime logs. `audit-final.json` and its
independent repeat verify those pins and the declared source comparisons.
`REPRODUCE.md` records direct binary and original-instruction reproduction.

The two final CPU binaries were linked after the inactive-alternate correction
and after concurrent master-volume changes. Production archives are retained as
observed artifacts; individual unrelated archive members may predate the source
snapshot. No reproducible complete-tree or full archive freshness claim is made.
The rendered runtime binary remains the exact earlier tested artifact described
above. No new launcher was published by this checkpoint.

The footstep oracle's historical help text names the Base entry. Its explicit
image entry table, original image SHA256 and captured stdout identify the actual
1.1 and 1.5 executions; those reports are preserved without rewriting them.
