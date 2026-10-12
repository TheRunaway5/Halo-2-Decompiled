# Pause settings runtime status

The Vista pause/settings widgets and native setting owners are distinct. This
work does not establish that every setting or the complete tutorial is finished.

| Setting or flow | Current native effect |
| --- | --- |
| Four stick layouts | Source input-table movement/look mappings in campaign and multiplayer |
| Four button layouts | Source requests; zoom reaches the existing multiplayer scope owner |
| Controller sensitivity and inversion | Source sensitivity rates, live in both input loops |
| Controller/mouse auto-centering | Source admission constants and authored matg delay/scale, with native frame scheduling |
| Controller vibration | Explicit unavailable message; original rumble event producers remain open |
| Three keyboard presets | Executed original English tables for supported native actions; an international-only Southpaw key remains unbound with its mouse alternative retained |
| Custom keyboard bindings | Saved supported action bindings, including pickup hold timing; other source actions report unavailable |
| Mouse sensitivity and inversion | Existing live native mouse owner |
| Mouse dual-wield inversion | Swaps actual multiplayer primary/secondary mouse requests when dual wielding |
| Volume | Live campaign/menu and multiplayer mixers |
| Speakers | Live native endpoint channel selection; decoded stereo stays in front channels, without original surround/HRTF processing |
| Cinematic subtitles | Explicit unavailable message; text/event display owner remains open |
| Video options | Existing native renderer/window consumers, with validation and display-mode rollback |
| Tutorial input scripting | Source action masks, direction restrictions, 15-degree tests and controller profile callbacks |
| Tutorial HUD scripting | Independent enable/text/show/request state, retained by checkpoints |
| Complete tutorial | Still open: HUD text drawing, automatic training events and other mission/AI script owners |

The 300-tick tutorial startup now stops at `ai_dialogue_enable`, after getting
past the previous HUD-training failure. A visible 90-frame tutorial window
renders pause and freezes the simulation at tick 3, but still returns failure
for that script fault. Missing AI services are not accepted as successful no-ops.
Training action requests do not establish missing ordinary melee/flashlight
gameplay effects, and campaign zoom still has no native scope owner.

Original source evidence is retained in
`analysis/frontend/pause-settings-20261012/`. Reproducible controller input tables
are in `tests/fixtures/vista_pause_input_tables.json`, with provenance in its
adjacent Markdown file. The comparison covers all 16 layouts and every hardware
ordinal. The pause test also compares all 57 supported digital bindings across
the three presets against the executed original tables. It does not establish
original analog acceleration, physical controller
behavior, complete campaign execution or retail Windows/Xbox pixel parity.

Validation: all six focused CTest suites pass, with 4,154 source-asset pause
checks and 251 SDL keyboard/mouse checks. Native weapon pickup/dual-fire tests
also pass. SDL dummy and live PipeWire six-channel reconfiguration preserve
retained voice packets and playback position; hardware audibility is unverified.
Visible Vulkan controller, keyboard, Resume and volume checks pass with
validation enabled. Evidence reports retain the precise executable hashes.
