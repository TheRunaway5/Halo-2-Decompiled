# Shared dialog composition and layout checks

The pregame lobby's unavailable message previously used a generic black dimmer.
Its translucent panel therefore retained sharp lobby text and controls beneath
the message and Continue prompt. The painter now applies the shared source
tint-and-blur boundary to every ordinary dialog over a retained lower menu,
rather than limiting it to profile selection, keyboard, Game Setup and Map.
The foreground panel, text, controls and pointer coordinates retain their
authored geometry. Definition flag5 still suppresses the backdrop for the
Appearance choice panels, as required by the original source.

## Source evidence

`tools/oracle_frontend_dialog_backdrops.py` executes unchanged Xbox
`22F916`, `1473C9`, `23029A` and `234F8C` using literal mainmenu definitions.
All 53 dialog definitions match the shared flag5 policy. All 85 screen virtual
tables in the inspected UI table range with the common channel/window accessors
also share `1473C9`. Error screens7/8 use their actual `454560`/`4545D0` tables.
The settled submissions are authored tint `D900142C`, blur strength1, then the
foreground widgets. Suppressed definitions submit only their widgets.

The replay uses a supplied foreground channel0/window4 over retained lobby14
on channel5/window4 in mainmenu mode2. It establishes the common policy, not
every original navigation route or a synchronized framebuffer comparison.
Pinned executable/cache hashes and submissions are saved in
`analysis/frontend/gui-overlap-20261010/original-dialog-backdrops.json`.
Existing Vista popup composition also goes through the shared painter boundary
and uses the active Vista definition's tint and animation.

## Regression coverage

- `frontend_error_dialog_tests` creates an Elite profile through normal
  navigation, enters Split Screen, activates Start Game and reproduces the
  reported error through the production map-validator failure callback, so
  expanded biped admission cannot disable the UI regression. It verifies the
  backdrop boundary, authored entry timing,
  body clipping for a long diagnostic through the map-validation failure path,
  and Continue returning to the unobscured lobby.
- `frontend_menu_layout_tests` exercises all available Xbox menus and all 26
  standalone Vista routes, their panes/selections, variant categories and every
  parameter choice: 1,488 draws with stock art and another 1,488 with the setup
  Vista artwork/font library. It checks backdrop isolation/suppression, complete
  widget boundaries, finite coordinates, absence of the generic dimmer and
  text containment in the authored foreground canvas. Vista shell 5 and
  profile pane 38 are composed decorations, rather than standalone PC routes.
- `tools/verify_frontend_dialog_layout.py` captures an unavailable launch at
  4:3, 16:9, 16:10, 21:9 and 2560x1437, plus dismissal, profile selection, keyboard,
  confirmation, sensitivity, Vista About, Video and binding editor pages.
  It checks destinations, image dimensions and Vulkan validation output.
  The initial capture bank reproduces the reported Elite admission error;
  the expanded native biped support now uses the unavailable King engine to
  exercise the same dialog in subsequent runs.
- The existing native menu sweep covers all 150 Xbox panes, variant categories,
  parameter dialogs, both player-model choices and keyboard pages.

Run the focused tests with:

```bash
ctest --test-dir build-sandbox-release-20261010 \
  -R 'retail_frontend_(error_dialog|menu_layout.*)$' --output-on-failure
build-sandbox-release-20261010/frontend_menu_layout_tests assets/disc/maps \
  'Halo 2 Vista installer/maps/mainmenu.map' \
  assets/imported/3c4bc518c80645f9b12caccab6d61f57/vista
.tools/python/bin/python tools/oracle_frontend_dialog_backdrops.py
python3 tools/verify_frontend_dialog_layout.py \
  --launcher build-sandbox-release-20261010/Halo2-Title-Launcher \
  --output analysis/frontend/gui-overlap-20261010/captures
```

Reports and captures are under `analysis/frontend/gui-overlap-20261010/`.
These checks cover the available native menu definitions and the reported
composition failure. They do not establish every possible future/localized
string, every original transition, or complete Xbox/native visual parity.
