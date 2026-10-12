# Multiplayer keyboard and mouse controls

The user requested these native host bindings on 2026-10-08. They replace the
earlier F reload and Ctrl crouch bindings. Shift supplies the existing crouch
action. The binding request authorizes this PC mapping; original executable
behavior remains the authority for gameplay handlers and action admission.

| Input | Requested action | Current integration |
| --- | --- | --- |
| WASD | Move | Live multiplayer command delivery |
| Mouse motion | Look | Live relative mouse camera input |
| Left mouse | Fire | Live weapon handler |
| Right mouse | Scope / offhand fire | Scope with one weapon; independent offhand trigger while dual wielding |
| Wheel or Tab | Switch weapons | Native carried inventory selection; drops an equipped offhand |
| E | Pick up/swap | Native map-item transfer and spare ammunition |
| X | Dual wield | Native offhand pickup for authored dual-compatible weapons |
| F1 | Optional debug panel | Captures menu/game input; local session controls only |
| F3 | Scoreboard | Live manual active-Slayer adapter using original roster/font/cells; emblem/quality consumers pending |
| F | Melee | Host press recorded; source action/combat handler pending |
| Q | Grenade | Live grenade action, authored throw animation and projectile; releases offhand |
| R | Reload | Live weapon handler |
| Space | Jump | Live movement handler |
| Either Shift | Crouch/sneak | Live crouch handler |

`MultiplayerKbm` receives SDL events for its own window. Held keyboard movement
and mouse buttons persist until release. Jump/fire/reload taps remain queued
until a simulation command is delivered, even when down/up arrive during one
render frame. One-shot requests ignore keyboard repeat. Focus loss, background
and quit clear held state and queued presses. Focus gain requires fresh input.

The runtime adds controller movement to keyboard movement with the existing
axis limits and combines controller buttons with keyboard/mouse buttons. An
idle attached controller no longer replaces player one's keyboard input.
Mouse capture/look and Escape are filtered to the actual game window.

## Verification

`multiplayer_kbm_tests` passes 172 checks through SDL's actual event queue,
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
