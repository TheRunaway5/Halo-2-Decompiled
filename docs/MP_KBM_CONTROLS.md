# Multiplayer keyboard and mouse controls

Default bindings now follow the locally installed MCC shipped control definitions
(`Data/settings/default.xml` and `Data/BuiltInData/Halo2Controller.json`).
The October28 request supersedes the October8 host mapping. New profiles
and Keyboard Restore Defaults use:

| Input | Action |
| --- | --- |
| WASD / mouse motion | Move / look |
| Space | Jump |
| Left Ctrl | Crouch |
| Left mouse | Fire / primary dual-wield trigger |
| Right mouse | Zoom / offhand dual-wield trigger |
| 1 | Switch weapons |
| E | Action / pick up or exchange weapon |
| C | Pick up a second weapon for dual wield |
| Q | Melee |
| F | Throw grenade |
| R | Reload |
| 2 / 3 | Next / previous grenade type |
| 4 | Flashlight / camouflage |
| Hold Tab | Full multiplayer scoreboard |
| Esc | Pause menu and settings |

The two grenade directions share the native switch action because Halo 2 has
two grenade types. Other alternate keyboard slots start unbound. Mouse wheel
and other supported inputs remain available through Custom bindings.
Saved explicit bindings remain selected until the player chooses Keyboard Restore Defaults.
The keyboard hub identifies the matching bindings as **MCC Default**. The three
authenticated legacy keyboard presets remain available in the preset menu.

Controller defaults match MCC's named **Halo 2 Default** preset: A jumps,
left-stick click crouches, RT fires, LT throws grenades or fires the offhand,
X reloads/interacts, Y switches weapons or picks up a dual weapon, B melees,
right-stick click zooms, LB operates flashlight/camouflage, and RB or
D-pad Left changes grenade type. Start/Pause opens the pause menu; Select/Back holds the scoreboard.
The separate MCC Universal Default preset has different bumper assignments
and is not the named Halo 2 Default used here.

Manual scores follow the OG Xbox TU1.5 held-counter/fade owner (`15E6BC`,
Back input `15E70D`). Scores remain requested while Tab or Select/Back is held;
release lets the original fade retire the scoreboard and restore the HUD.
The input stays local to the owning split-screen view. The original automatic
end-of-game and death-scoreboard gates retain their own source state.

The small reference fixture
`tests/fixtures/mcc_default_controls.json` records the shipped source hashes
and mappings for the native actions. This establishes control assignments,
not MCC gameplay behavior or physical Xbox framebuffer/controller equivalence.

`MultiplayerKbm` receives SDL events for its own window. Held keyboard movement
and mouse buttons persist until release. Jump/fire/reload taps remain queued
until a simulation command is delivered, even when down/up arrive during one
render frame. One-shot requests ignore keyboard repeat. Focus loss, background
and quit clear held state and queued presses. Focus gain requires fresh input.

The runtime adds controller movement to keyboard movement with the existing
axis limits and combines controller buttons with keyboard/mouse buttons. An
idle attached controller no longer replaces player one's keyboard input.
Mouse capture/look and Escape are filtered to the actual game window.

## Banshee evasion

The original Halo 2 maneuvers use the jump control with movement while piloting:

| Maneuver | Keyboard | Xbox controller, default sticks |
| --- | --- | --- |
| Roll left | Space + A | A button + left stick left |
| Roll right | Space + D | A button + left stick right |
| Backflip | Space + S | A button + left stick back |
| Forward flight | W; Space + W also stays forward flight | Left stick forward; A + forward also stays forward flight |

Controller stick presets and keyboard rebindings retain their movement/jump
actions. Boost inhibits evasion through the original vehicle gate. Original
direction thresholds, maneuver motion and cooldown own admission and repeats.
A quick overlapping keyboard chord retains its direction for one seated tick;
pause, focus loss and input consumption clear that queued direction.

## Verification

`mcc_default_controls_tests` checks every existing keyboard action and the
Halo 2 Default controller buttons against the shipped mapping reference. It
also delivers Tab through SDL's event queue, holds it across90 consumed ticks,
injects repeat events, releases it, and verifies that a completed tap does not
become a toggle or weapon switch.

`native_mcc_scoreboard_tab_single` and `native_mcc_scoreboard_tab_split4`
exercise held/released/held Tab through the rendered multiplayer loop.
`native_mcc_scoreboard_controllers` uses four independently assigned SDL virtual
controllers to check Select/Back in every local view. These checks observe
source roster identity, current-player highlight, artwork and normal-HUD return.
`native_mcc_scoreboard_all39_presets` separately checks the authored engine
formats. They require `HALO2_MCC_ONLINE_PRESENTATION_MAP` and a Vulkan display;
they do not establish physical controller or live online network acceptance.

The pause-menu checks also cover restoring MCC defaults, retaining that
label, editing custom controls, and preserving the original legacy presets.

The 2026-10-28 Banshee check enters an actual prepared MCC vehicle and flies it
through the native world. Eight keyboard/controller mapping cases cover both
rolls, backflip, forward no-trick behavior, boost exclusion, the original
46-tick held-input repeat, release beyond cooldown, and a queued keyboard chord.
The missing original `B3350` event producer and right-roll vector binding are
now connected. Its complete producer also passed 24 comparisons against the
original instructions. The controller cases use the mapping's hardware sample
structure; physical controller hardware and rendered/cross-machine play remain
separate checks.

Run the native check with:

```bash
python3 tools/run_tests.py --target banshee_evasion_tu15_tests \
  --filter '^banshee_evasion_tu15$' \
  --cmake-arg=-DHALO2_MCC_BANSHEE_EVASION_MAP=/path/to/prepared/coagulation.map \
  --retain-report banshee-evasion.json
```

The runner removes its build and retains only the small report when requested.

`multiplayer_kbm_tests` exercises SDL's actual event queue,
including every requested binding, pending taps, repeat, foreign windows,
focus changes and a real attached SDL virtual game controller. These checks
establish host requests, not missing gameplay handlers.

`multiplayer_kbm_runtime_tests` executes the actual rendered multiplayer loop
and match. Its diagnostic observer reads delivered commands and published
state after stepping; its only input stimuli are SDL events. It never supplies
controls, units, physics state, magazine state or outcomes. With an idle attached
controller and the imported 1.5 executable selected, it passed actual WASD
movement, Shift crouch, a single Space tap with positive jump velocity, LMB
ammunition expenditure, a single R tap with reload and refill, relative mouse
look, and focus reset across 126 simulation ticks on NVIDIA GeForce RTX 5080.
The original run log is in `analysis/multiplayer/integration/kbm-runtime/lockout-1.5.log`.
Vulkan validation emitted no VUID messages in that run.

The rendered probe needs the user's local map assets and a working Vulkan
display. The CTest entry skips without assets or DISPLAY. Optional diagnostic
environment variables enable validation, a PNG at tick128 and a per-tick
observation file: `HALO2_KBM_VALIDATION`, `HALO2_KBM_CAPTURE`, `HALO2_KBM_TRACE`.
They do not alter gameplay input admission. The production runtime leaves its
`input_observer` unset.

This does not establish full original movement/combat parity, physical keyboard
hardware behavior, LAN packet reliability, or the three pending gameplay
handlers and full original scoreboard session/presentation ownership. The old executable remains authoritative for those owners; native
bindings do not justify substitute implementations.

## Isolated test build

The remaining sections preserve historical qualification records. Their old
key assignments describe those checkpoints, not the current MCC defaults.

The frozen Linux checkpoint is
`build-kbm-20261008-162141/runtime/Halo2-Title-Launcher`. It starts at the title
screen and selects the user's external 1.5 gameplay data. The copied source
manifest pins 1,254 files. Its title run and serial rendered input run passed
with Vulkan validation; the source and output checksums are recorded in that
folder. Original assets are local symbolic-link mounts. Keep the entire folder
in place because its runtime uses the compiled shader directory.

That earlier frozen release contains only the six basic movement/fire/reload/
jump/crouch bindings and mouse look. Scope and F1 were integrated afterwards
and are not part of its source or binaries.

## Scope and F1 integration (later checkpoint)

The actual rendered `multiplayer_scope_runtime_tests` uses only SDL events and
read-only command/camera observations. Stock SMG right clicks advance the
original retained view selector and actual unit CURRENT independently. The
original observer transitions horizontal FOV by its imported magnification
and fifth-order motion; prepared shots and manual reload clear the selectors.
Magazine/action admission prevents scope during reload, including the original
biped-before-child action release order. Native UDP reset acknowledgement
prevents a cached old input from restoring cleared scope. The native protocol
now carries every participant's original zoom/action/HUD fields in bounded,
reassembled snapshots; it is not the Xbox network protocol.

Actual original HUD status/mask tests cover309 cases, including all256 CURRENT
bytes. Original bitmap geometry covers152 cases and312 ordered requests,
including the authentic binocular mask's four stretched/mirrored pieces across
all view grids. The selected original image supplies its stretch/mirror words.
Original shader sampling, framebuffer equivalence and other weapon managers
remain independent validation boundaries.

F1 now submits the original salted roster, sanitizer, default-profile palette,
font selector6, row/rank/score layout, backgrounds and current-row border. Its
native manual held/released branch uses the original imported counter cap and
HUD suppression. Source variant readers prove Slayer display kind2 and native
parameter IDs0/2/16 for rounds/score/lives. Source viewport delivery is reused
with explicit PC output compensation. This adapter does not fabricate original
engine lifecycle, automatic death display or local-profile records. Saved
profiles, emblems, connection quality, original CRT math and complete original
display scheduling remain open. Native participant names are currently ASCII.

The first integrated640x480 one/four-player scope and F1 probes passed with
zero Vulkan VUID messages; actual movement/input regression and native UDP
scope also passed. Evidence is retained in
`analysis/multiplayer/integration/kbm-integrated-20261008-180711`.
That capture predates the reviewed widescreen projection correction. The new
`build-kbm-20261008-184611` retains separate one/four-player widescreen validation.

The following paragraph records the earlier checkpoint; current weapon/throw integration is described in [native weapon gameplay](MP_WEAPON_GAMEPLAY.md).

Wheel/Tab, F and Q were faithfully recorded host requests. Carry selection and
switch/action kernels have original fixtures, but genuine second-item pickup,
retained animation/capability, melee contact/damage and grenade allocation/throw
owners are not yet live. Mapping a key does not establish its missing action.
The project multiplayer goal remains active.

## Higher-refresh test checkpoint

`build-kbm-20261008-184611/runtime/Halo2-Title-Launcher` reads the external supplied
1.5 image and starts at the original title screen. It includes live RMB scope,
manual F1, the basic controls, and fractional presentation of the existing body
idle and registered single-SMG first-person animations. The actual clock input
after action selection and its completed output are sampled without dispatching
events at render frequency. Original gameplay timing is retained; full locomotion
selection and replicated client first-person clock ownership remain unfinished.
[Integration and precise limits](MP_ANIMATION_PRESENTATION_INTEGRATION.md).

Source replay at30/60/120/144/240Hz preserves541 original ticks/2,705 phase states/
four callbacks and516 Match origin/output comparisons. Normal and sanitizer
checks pass. The release's rendered one/four-player animation probes pass with
actual submitted fractional vertices. Its manual F1 probe waits for the actual
SDL keyboard-focus owner before delivering input, rather than fabricating focus.
The restored, previously verified menu module and initial failed current-menu
probe are explicitly recorded in the checkpoint. Its source manifest, validation
logs and release checksums retain the exact delivered scope.
