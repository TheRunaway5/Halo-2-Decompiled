# Vista settings menus and native display controls

The MCC frontend loads the existing PC menu definitions from the selected
imported `halo2/h2_maps_win64_dx11/mainmenu.map`. Fonts come from its sibling
`halo2/h2_fonts` and bitmap resources from its MCC `textures.dat` partitions.
No Vista installation, font bank or legacy texture pack is required.
**Settings** uses the existing six-category PC hub: Player, Game Variants,
Video, Audio, Network, and About. Each category retains its authored pane and
illustration, including the monitor, speakers, globe/router, and Halo 2 logo.
**Player Profile > Keyboard/Mouse** opens the PC keyboard settings;
**Settings > Video** opens its video options. The original Xbox menus remain in
their existing resource namespace. PC screen IDs use `0x10000 | source_id`, so
shared IDs such as sensitivity screen 42 cannot displace the Xbox definition.
Compiled PC StringIDs are resolved by an exact unique symbol-name join into
the MCC namespace. Authored MCC widget IDs and localized strings stay intact;
the mapping contains identifiers, not a copied Vista runtime bank. Existing
profile, video, audio, keyboard, split-screen and Online controllers are reused.
Installed launchers select their imported MCC cache through
`assets/mcc-source.tsv`; `--launcher-info` reports the selected runtime and paths.
The older Vista loader remains available for offline reference validation.

Custom keyboard and mouse bindings, secondary bindings, look sensitivity, and
look inversion are saved per native profile and passed to the multiplayer
keyboard/mouse adapter. In **Keyboard Settings > Custom**, Enter or a click
captures a primary binding; X captures a secondary binding. Backspace clears it,
and Escape cancels capture. Escape, F1, and Backspace remain menu/debug controls.
The mouse wheel retains the existing host adapter's single switch pulse in
either direction; actions requiring a held input reject wheel bindings.

Display Mode and Screen Resolution apply to the shared SDL desktop window.
The PC windowed resolution table is supplemented with SDL display modes and
the current window size, plus native windowed options through 7680 x 4320.
Fullscreen uses an exact host mode when available and desktop fullscreen otherwise.
The menu reports the resulting window dimensions after a desktop fallback.
Changes open the PC confirmation dialog; OK saves the setting, while Escape or
the source countdown restores the previous mode. The original countdown starts
at 16 and expires at 16000 ms. Confirmed display settings persist separately
from the profile. Driver errors appear in the existing error dialog.

Internal Resolution independently selects the scene color/depth size; Match Screen
Resolution follows the drawable. The renderer scales the completed image into
the acquired swapchain image and captures at the actual output dimensions.
Native **2x SSAA** and **4x SSAA** multiply both internal dimensions by two or
four, with one or two linear 2:1 downsample stages respectively when internal
and output resolution match. These are supersampling controls, explicitly
labeled separately from Vista's original Direct3D 2/4-sample MSAA. Vulkan image/
framebuffer limits and a native 64-megapixel allocation limit reject excessive
combined resolution/AA requests before applying the setting.

Brightness and Gamma use the scalar tables executed from PC663D0E/663E28 in a
native final-image correction pass. The native composition is
`pow(clamp(color + brightness, 0, 1), gamma)`; complete source shader parity is
not claimed. The existing native baseline starts with brightness 4 (zero bias)
and gamma Disabled. Video Restore Defaults resets these render controls,
internal resolution, AA, LOD, and safe area; desktop mode remains governed by
its separate confirmation transaction.

Level of Detail offers High, Medium, and Low. Medium and Low halve the native
bloom raster dimensions using PC590B26's 480x360 versus 240x180 quality ratio.
Low additionally applies the recovered ordinary geometry cap3, coverage .8,
secondary scale .5, and shader metric .3 to native multiplayer object/BSP shader
selection. Stricter existing split-screen caps remain effective. Real resident
model banks supply geometry; no replacement low-detail meshes are generated.
Safe Area insets multiplayer HUD geometry by five percent of view height per
edge, from PC664A18, including aspect-correct horizontal margins. Scoreboards
retain their authored layout.

Audio includes Game Volume (Mute and 1–10), Speaker Configuration, Subtitles,
and Restore Defaults. Native master volume scales menu and game PCM
without stopping voices. Stereo/System Default use the existing stereo mixer.
Network exposes System Default for the operating system's existing LAN routing.
About shows the native runtime identity and the imported cache build.

The PC menu artwork, strings, fonts, panes, skins, widget coordinates, colors,
and animations come from the cache. The renderer uses the executable's
height/1200 UI scale through its centered native canvas. Source fonts and
bitmaps remain separate from Xbox resources. Twenty-eight PC definitions are
imported, including choice dialogs and the 43-row advanced binding editor.

The PC framing now uses the same native widescreen side extensions as the
other menus. Its authored center quad and foreground coordinates stay centered;
the independent side copies sample the final, scaled canvas boundaries. The
original footer enum table is recovered from PC60BEBB and resolved through
`wigl+144`, retaining the source header/footer bounds and controller icons.
About, Network, and choice dialogs blur/tint their retained lower menu before
drawing the popup, so the Settings list and description do not overlap its text.

All 21 hub icon annotations and the Keyboard/Mouse icon annotation retain
their authored typing flag. PC6303B9 advances one character per admitted draw
after 50 ms, displays the old prefix first, and uses the original block cursor.
Changing the selected pane recreates its typing state; a retained popup does
not recreate its parent. Ordinary descriptions keep their authored text flags.

Outside binding capture, mouse-wheel up/down emits one menu navigation step,
independent of magnitude, through the same list movement as the source's
events10/11 and arrow keys (PC614CA9). This includes wraparound, follows long
lists, and skips the native controls editor's section headings. SDL's flipped
wheel direction is normalized. Wheel input counts as activity and respects
debug-panel capture; binding capture continues to receive its separate pulse.
Precise wheel events are admitted when the host compatibility layer leaves
the integer delta empty, including the tested SDL2-compat host.

## Current boundaries

- Native AA, final color correction, and the mapped detail policy are native
  adapters. Original Vista MSAA, every quality-dependent engine subsystem,
  source postprocessing parity, and complete campaign HUD effects are unproven.
- Cinematic subtitle playback and surround speaker routing remain unavailable.
  The corresponding authored menus remain present and explain the limitation.
  Network adapter-specific binding remains unavailable; System Default is the
  currently supported route.
- Default, Skirmish, and Southpaw preset application remains unavailable pending
  recovery and admission of the original input binding tables. Custom edits and
  keyboard Restore Defaults use the previously requested native host defaults
  documented in [MP_KBM_CONTROLS.md](MP_KBM_CONTROLS.md).
- Auto Look Centering, Dual-Wield Inversion, and advanced actions without a native
  input consumer report unavailable. Showing an original action does not add its
  gameplay implementation. The supported binding actions are movement, jump,
  crouch, fire, grenade, weapon switch, melee, reload, zoom, and scoreboard.
- Sensitivity scales the existing native mouse gain by `sensitivity / 3`.
  This does not establish the original Vista mouse response formula.
- The game-shell main menu remains the original Xbox frontend. This is a bounded addition
  of Vista settings, not replacement of the entire frontend or complete Vista
  rendering/input parity.

## Reproducible evidence and checks

`tools/audit_vista_presentation.py` executes the unchanged PC prompt-table
construction, draw-time typing prefix, and list event dispatch. Its explicit
services are the clock getter and navigation leaf; this does not execute the
complete original Windows input/rendering system. The regression test covers
the reported About route, frame side copies, actual glyph prefixes, footer
bounds, and wheel navigation. `tools/capture_vista_presentation.py` captures
all six categories, aspect ratios, popup stacking, Keyboard/Mouse, video rows,
and wheel events delivered through the normal SDL adapter.

```bash
.tools/python/bin/python tools/audit_vista_presentation.py \
  --output analysis/frontend/vista-presentation-20261010/source
ctest --test-dir build-vista-settings --output-on-failure \
  -R '^(retail_frontend_vista_presentation|retail_frontend_vista_settings|retail_frontend_typewriter_painter|retail_frontend|frontend_frame_sampling)$'
python3 tools/capture_vista_presentation.py \
  --binary build-vista-settings/halo2_native \
  --output analysis/frontend/vista-presentation-20261010/fresh-captures
```

`tools/audit_vista_settings.py` pins the supplied executable and cache SHA-256
hashes. It executes the original x86 scalar label routines in bounded Unicorn
memory and reads the video jump table, resolution table, UI scale, and timer
instructions. It does not launch or execute the complete Windows game.

```bash
.tools/python/bin/python tools/audit_vista_settings.py \
  --output analysis/frontend/vista-settings-20261010/source
cmake --build build-frontend --target halo2_native frontend_vista_settings_tests \
  multiplayer_kbm_tests frontend_extension_catalog_tests frontend_tests -j2
ctest --test-dir build-frontend --output-on-failure \
  -R '^(retail_frontend_vista_settings|retail_frontend_extension_catalog|retail_frontend|multiplayer_kbm_bindings)$'
./build-frontend/halo2_native --menu-vista-settings \
  'Halo 2 Vista installer/maps/mainmenu.map' --menu-screen 65792
# Live SDL fullscreen/rollback and Vulkan internal-resolution/SSAA checks:
./build-frontend/frontend_vista_settings_tests assets/disc/maps \
  'Halo 2 Vista installer/maps/mainmenu.map' \
  analysis/frontend/vista-settings-20261010/source/source.tsv \
  build-frontend/vista-settings-test.cfg --live-window
```

The source fixture checks six hub labels, four audio labels, eight video labels,
43 editor rows, and 31 choice labels against the imported and native menus. The
two AA labels are intentionally native SSAA labels; source strings remain
intact. The native Internal Resolution row follows Screen Resolution, keeping
the relative order of all eight source rows. The integration test imports the real
cache, checks resource isolation and parent navigation, edits and reloads both
binding slots, exercises the multiplayer input adapter, verifies display
commit/cancel/timeout/error handling, and builds draw vertices for the imported
menus. The optional live test renders and captures all three native AA modes,
checks actual internal/output extents, recreates the direct path, and rejects
an oversized combination. Authored audio tests verify mute/unmute output while
voices continue. The object-LOD test selects the real Headlong bus bank under
the recovered PC Low cap. SDL queue tests cover the existing keyboard/mouse adapter. Vulkan
captures validate the native rendered layouts. These checks do not establish
pixel parity with a running Vista installation or physical fullscreen switching
on every host display.
