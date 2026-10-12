# Elite, HUD, radar and audio delivery — 2026-10-12

Both installed launchers select
`build-interaction-prompts-verified-20261012/halo2_native`. Actual gameplay and title-screen
runs through each installed launcher passed. The runtime SHA-256 is
`4ff5d1142603d9df4cd505e48583c5217cf6da0c3d145b7f6985e96b889e993e`.

The delivered changes include ordinary saved-profile Elite multiplayer entry,
the purple Elite unit HUD, shield fill alignment, motion-sensor contacts and
pulse rendering without the dark square. Both original Xbox and the selected
MCC HUD assets passed the bounded rendering checks, including one through four
views. This does not establish a physical Xbox framebuffer match.

World impulse, impact, effect and posed-footstep positions now reach actual SDL
voices. Original distance attenuation uses the authored sound/class range and
the nearest current local listener. Actual PCM tests cover near/far positions,
missing listeners, multiple local listeners and moving listeners. Local SMG
fire no longer plays both generic and authored sound routes. LAN presentation
consumes each genuinely accepted weapon-effect state instead of losing
intermediate accepted states when a frame receives multiple snapshots. Missing
network history is never reconstructed into invented shots.

## Selected validation

| Evidence | Result |
|---|---|
| Audio, sound-resource, footstep and radar component suite | 88/88 pass; no skips |
| Default TU1.5 runtime | 14 selected checks pass: Xbox/selected HUD, world one/four-view, prop LAN, saved Elite single/mixed/LAN, audio local one/four-view and LAN one/four-view/delayed client, plus the component suite |
| TU1.1 runtime | Seven selected checks pass: world, mixed species, local one/four-view and LAN one/four-view/delayed client |
| Installed launchers | Gameplay and title-screen route through each launcher pass with actual SDL/Vulkan |
| Negative controls | The corrected firing-sound observer rejects the unchanged old runtime in local, LAN and delayed-client runs at the intended audio-count assertions |
| Reproducibility | Current compiled snapshot's 11,779 inventory files and selected HUD/menu/loading assets' 1,675 files match their retained pins; prior audio source and its later audit are retained separately |

The final evidence index is
[the delivery manifest](../analysis/audio-spatial-20261011/audio-delivery-final/manifest.json).
Its current selection is the full 14-case
[TU1.5 report](../analysis/audio-spatial-20261011/prompt-audio-validation-tu15-v1/report.json)
and seven-case
[TU1.1 report](../analysis/audio-spatial-20261011/prompt-audio-validation-tu11-v1/report.json).
The newer installed prompt runtime preserves all 100 sound/module/observer
source files byte identically from the prior validated audio build and passes
its own fresh complete runtime suite. The native binary stayed unchanged while
the missing test targets were built.

The earlier inventory included 141 unused object outputs under a nested
reference project's build directory; those were subsequently removed. They
are not inputs in the earlier native build graph. All 12,589 remaining earlier
files and every selected current inventory file match their recorded bytes.
The original inventory and exact later audit are preserved in the manifest.

Prior audio-build evidence separately selects nine non-audio cases from
[TU1.5 v1](../analysis/audio-spatial-20261011/audio-delivery-validation-tu15-v1/report.json)
and five corrected audio cases from
[TU1.5 v2](../analysis/audio-spatial-20261011/audio-delivery-validation-tu15-v2/report.json).
The complete v1 report remains failed: its LAN harness counted the obsolete
generic firing journal after the runtime correctly suppressed that duplicate.
The v2 harness independently counts actual authority-authored effect cues and
actual SDL submissions. It also catches the old runtime's duplicate sounds.
That prior production runtime is byte identical across those runs; only the test
observer changed. [Prior TU1.1 results](../analysis/audio-spatial-20261011/audio-delivery-validation-tu11-v1/report.json)
and [negative controls](../analysis/audio-spatial-20261011/authority-red-validation-v1/negative-controls-verified.json)
remain separately retained.

For the final TU1.5 audio runs, the actual authored firing-request count equals
the actual SDL-submission count in every ordinary and deliberately delayed
local/LAN case, with no pre-join replay or unobserved tail. Counts follow the
observed source ticks and are not padded to a fixed expected number.
Reload, material footsteps and normal ambient packets also pass their actual
runtime assertions. World checks deliver eight impact packets and one grenade
detonation; the prop LAN case delivers its actual removal/explosion once with
the full owner and posed emitter position.

The runtime tests use SDL dummy output and actual Vulkan/X11/native UDP.
Separate manual-mixer tests inspect real decoded PCM. Neither proves physical
speaker response or equality with an original Xbox waveform. Complete sweeps,
contact visibility, transparent shield padding and undarkened radar corners
are explicitly checked, rather than inferred from a successful screenshot.

## Remaining all-sounds scope

The separate all-sounds goal is incomplete. Fixed stereo output still lacks the
original Xbox HRTF/DSP, directional/cone, occlusion and full PVS paths. The
original CPU admission, SDK and lifecycle modules have substantial independent
reference evidence, but they are not all composed into the live adapter.
Ambient detail scheduling, weather alternates, authored start/end transitions
and seamless packet scheduling remain unfinished. Device/weapon attachment,
death-camera listener selection and grenade flight-loop publication/lifetime
also require original-backed live integration. The full campaign and game-mode
trigger/position/timing inventory and original waveform comparison remain open.

The original detail scheduler cannot be replaced by a guessed periodic sound:
its deadline advances only after actual submission success or original reason
1, and it uses the original audio clock, global RNG and spatial/active-camera
request producer. Retained CPU command words do not establish the physical
Xbox DSP response. These boundaries remain explicit in
[the audio goal](AUDIO_PARITY_GOAL.md) and
[spatial integration evidence](MP_SOUND_SPATIAL_NATIVE_INTEGRATION.md).

## Installed entry points

```sh
./Halo2-Launcher --launcher-info
./Halo2-Title-Launcher --launcher-info
```

Both resolve to the tested runtime above. The selected Xbox game files, Vista
UI files, MCC HUD/menu packs and default TU1.5 executable were preserved.
Pre-install launchers and shortcuts are retained in
`analysis/audio-spatial-20261011/launcher-backup-before-audio-delivery-v1`.
The selected newer build includes the Vista prompt DDS-atlas correction,
authored action-button art, remapped keyboard labels and Vista pause assets.
It also includes the live audio corrections. The older audio build and its
launcher runs remain retained evidence; final verification preserves the newer
installed prompt/input changes instead of reinstalling the older audio build.
