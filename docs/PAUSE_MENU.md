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
cmake --build build-multiplayer --target halo2_native game_pause_menu_tests game_pause_window_probe -j6
SDL_AUDIODRIVER=dummy build-multiplayer/game_pause_menu_tests \
  assets/disc/maps 'Halo 2 Vista installer'
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
