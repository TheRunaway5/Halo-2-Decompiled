# Native in-game pause menu

Escape or controller Start opens the pause overlay. Continue, Escape at the
root, or Start resumes play. Keyboard arrows/Enter, the mouse, controller D-pad
and left stick navigate it; B or right click goes back. Quit requires a
confirmation with Cancel selected initially.

The overlay imports the selected Vista installation's
`maps/01a_tutorial.map` gameplay UI bank. Vista `mainmenu.map` does not contain
the gameplay pause widgets. Screen18 supplies the pause artwork, layout and
strings; screen259 supplies Settings, with gameplay controller200,
keyboard260, video261 and audio262. Custom keyboard mapping uses244/254.
Dialog7/8 and controller submenus also use Vista definitions. Each cache retains
its own tag datums, locale, fonts, skin and animation ownership.
The launchers default pause artwork/fonts to the selected Vista installation;
`--pause-ui-assets` remains an explicit override.

Local campaign simulation and its audio cursors freeze while the overlay is
open. Mission objectives come from the selected Xbox scenario's objective
locale and live objective states. Revert uses the simulation checkpoint;
Restart reconstructs the current level from its initial BSP/spawn after
confirmation. Multiplayer simulation continues while local gameplay input is
captured by the menu. Closing input must be released before gameplay polling
resumes, preventing a click on Continue from firing a weapon.

Controller stick/button layouts, look sensitivity, inversion and auto-centering
now reach both native gameplay input paths. The three Vista keyboard presets
come from executed original input tables. Custom movement/combat, pickup,
dual-pickup, flashlight/training and grenade-switch requests use the saved
bindings. Mouse dual-wield inversion changes the two actual multiplayer hand
requests. Input settings persist through the existing `menu.cfg` owner and are
adopted by a retained title-menu state on return.

Volume and supported video settings still apply live. Speaker selections now
reach both native SDL audio adapters, retaining active streams when a replacement
device opens successfully. These adapters currently emit decoded stereo into the
front channels; selecting a multichannel endpoint does not supply original
surround/HRTF processing. Device-open failures keep the existing device and report
an error. Rumble and cinematic subtitle display still lack live engine consumers;
their menu choices explicitly report those missing owners. Unsupported custom
actions also retain their explicit error.

Tutorial HUD commands now retain their original independent state fields, and
input tests, pitch calibration and controller inversion have native handlers.
Checkpoint restore retains training state. Training-text drawing, automatic
training events and the full mission's other script/AI services remain open.
See [pause settings status](PAUSE_SETTINGS_STATUS.md) for the precise scope.

The native multiplayer root currently offers Continue, Settings and Quit
using the Vista pause widget. The original Vista multiplayer pause owner has
additional session-gated rows, which are not reproduced here. Campaign Quit
does not claim durable saving: its label is Quit and its confirmation warns
that unsaved progress will be lost. Offline games launched through the current
workbench exit that workbench on Quit. Online matches use their existing return
lifecycle.

## Evidence and verification

Source inventory, original executable disassembly, focused test logs and
native-window captures are retained in
`analysis/frontend/pause-menu-20261011/`.

Original Vista owners inspected:63E595 (campaign rows),63EA28 (dispatch),
63EBDF (screen18/pause),63E33C (multiplayer list),64E1AA (Settings dispatch).
Xbox originals23216C,23217B,2321AB,2321FC and23223D independently establish
revert/restart/settings/quit and scenario objective ownership.

Build and run the source-asset navigation/input test:

```sh
python3 tools/run_tests.py --target game_pause_menu_tests \
  --target game_pause_window_tests --filter '^native_game_pause_(menu|window)$'
```

`tools/test_game_pause_window_flow.py` drives declared SDL events into real
native Vulkan windows for pause, Settings, confirmation, Resume and campaign
pause captures. It accepts the runtime, preload probe, output directory,
case and optional `--validation`. These checks establish bounded native Linux
behavior; they do not establish retail Windows/Xbox framebuffer parity or a
full campaign playthrough.

The focused source/navigation/input suite passed, as did native Vulkan window
checks for pause, Settings, confirmation, Resume, saved live volume, the
missing-checkpoint error and confirmed level restart. Revert/restart were
exercised in standalone campaign simulation (`--play`); restoring a populated
checkpoint through this GUI was not newly exercised. Native captures use
declared SDL inputs and filter unrelated desktop/controller input.

The earlier tutorial window check rendered the pause menu and froze its
simulation clock, but failed on `hud_enable_training`. That builtin now has a
state handler; this alone does not establish a working tutorial mission. The three full frontend inventory tests
also remain failing on unbound Vista lobby/menu routes, including screen14.
Their logs are retained separately from the passing pause suite.

## Offline pause policy (2026-10-20)

TU1.5 keeps campaign and multiplayer pause separate. Solo screen18's
constructor `238AC2` sets `*51DF54+1` and calls audio transition `1285A0(0)`.
The window-clear owner `14AE01` clears that byte and calls `1285A0(1)` only
after `14BD64` reports no UI windows. Multiplayer screen195's constructor
`14A0D9` does not pause either owner, including in local offline matches.
The native overlay exposes this distinction through `pauses_game_time()`;
settings and confirmations keep the campaign pause active.

`Simulation::advance_frame_time` publishes the campaign pause byte to its
retained source clock as well as gating native tick admission. Raw incoming
camera time remains a separate original caller input; it is not replaced with
accepted game time. Resume drains the remainder of its SDL event batch before
the released-device poll admits gameplay input, including mouse motion and
controller events queued behind the dismissal.

The unchanged original CPU oracle is reproducible with the owned English
TU1.5 executable (SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`):

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_game_pause_tu15.py \
  --xbe '/path/to/u5English/default.xbe' \
  --output analysis/pause-offline-20261020/original-policy.json
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_game_pause_tu15.py \
  --xbe assets/disc/default.xbe \
  --output analysis/pause-offline-20261020/disc-policy.json
```

Its eight cases per image execute original constructors, clock predicate, pause stores,
window-clear resume and sound transition body without instruction patches.
UI construction/window occupancy and sound hardware are declared controls;
this is not physical Xbox UI or audio output evidence. The new window test
submits actual Vista pause/settings/confirmation geometry to SDL/Vulkan and
checks mouse ownership, Resume and a held virtual controller. The campaign
camera tests exercise both original-backed and native-cold retained clock
owners, pause, Resume and checkpoint restoration. These bounded checks do not
establish a complete campaign playthrough, full multiplayer menu/session
parity, or retail framebuffer/audio parity.

The native-cold camera prerequisite also shares the authenticated immutable
word at `46A75C` between the absent-unit camera and vehicle closure; their
previous duplicate mounts stopped world construction before any pause input.
The existing byte/access checks still reject conflicting values or mutable
owners. The rumble test ages its inactive timers before testing two distinct
active slots, matching the original oldest-timer selection. Its SDL virtual
motor assertions use the actual callback packets, including zero output while
paused. The virtual callback success convention is calibrated first for the
local SDL2 compatibility provider, with successful API receipts still required.

Verification: all seven focused CTests pass, including the actual SDL/Vulkan
window with validation, campaign audio/rumble/text, source frame-clock fixture,
checkpoints and both original-backed/native-cold camera modes. The native
executable builds successfully. Both unchanged original executables pass eight
policy cases each. The [receipt](../analysis/pause-offline-20261020/validation.json)
and [test results](../analysis/pause-offline-20261020/final-tests.log) retain the
precise scope. Owned scratch/build trees were removed; no binary, frame or
capture artifacts were retained.

Video Settings begins with Graphics Mode. Enter, controller A, or a row click
cycles Original and pStencil in place, applies the live renderer preference,
and saves the selection. The F1 debug menu and its cheat controls are removed.

Fullscreen Mode follows Graphics Mode and offers Windowed, Fullscreen, and
Borderless Fullscreen. Fullscreen uses SDL's exclusive display mode; Borderless
Fullscreen uses the desktop display mode. Changes retain the existing preview
confirmation and timeout rollback, and save the exact selected mode.

Video Settings also exposes Upscaling, replacing the former SSAA choices. The
shared submenu offers Off, FSR 3.1 and DLSS 4.5, with Native AA / DLAA through
Ultra Performance quality modes. The title, campaign pause and multiplayer
pause menus share the same live adapter and saved preferences. See
[UPSCALING.md](UPSCALING.md) for the native runtime details.
