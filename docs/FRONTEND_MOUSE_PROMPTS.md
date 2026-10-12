# Native mouse access to menu prompts

Visible prompts now accept left clicks on their icons and labels. The SDL
platform cursor changes to a hand over interactive rows, keyboard keys and
prompts. Hovering a prompt preserves the focused row, so Select/Edit applies to
that row. Right-click Back remains available. Dialogs submit only their own
interactive targets, and old targets are rejected after a menu-state revision.

This is a requested native mouse extension. It reuses original assets and
existing menu commands; it does not claim that Xbox had mouse input.

## Actions and layout

The original `button_keys` strings, pane texts and decoded source fonts define
the prompt inventory. Their controller glyphs map to existing commands:

| Source glyph | Native click command |
| --- | --- |
| E100 / A | Accept the selected item or Continue |
| E101 / B | Back, Cancel, Close or Done |
| E102 / X | The menu's secondary action, including Toggle Icon |
| E103 / Y | Friends/players, or keyboard Space |
| E10C / Start | Commit keyboard text, otherwise Accept |
| E10D / Back | Existing Back command |

The keyboard's drawn keys already carry row-specific actions, including Delete,
Space, cursor movement and Start/Done. The plain title caption is identified by
its original `0E001811` string ID: `PRESS START TO CONTINUE`.

Prompt targets accumulate the actual visible glyph ink during the same layout
that performs font selection, kerning, alignment, wrapping, animation and source
clipping. Icon/label unions exclude the spaces between adjacent prompts and
receive two source pixels of vertical padding within the authored text box.
Vista targets use the same 480/1200 transform as Vista vertices. SDL pointer
coordinates use logical window units and the centered 640x480 canvas, including
widescreen margins and HiDPI windows. Invisible/disabled keyboard buttons and
binding section headings do not submit targets.

The native binding editor's Enter, X, Backspace and Esc captions are explicit
clickable spans. Enter/Edit captures a primary binding, X captures a secondary
binding, and Backspace/Delete clears it. The original Vista DELETE footer clears
the selected primary binding. During binding capture, clicking Cancel/Done or
Clear consumes the command; clicking elsewhere can bind the mouse button.

## Validation

`frontend_mouse_prompt_tests` first failed on the original B/DONE icon at
(545,432), established independently by the original subtitle traversal test.
The completed regression checks the icon and label, inert prompt separation,
stale-target rejection, dialog isolation, transparent animation timing, actual profile/keyboard/error/Vista
commands, primary and secondary binding editing, and six aspect ratios. It also
checks 3,161 rendered controller glyphs across 1,488 menu/pane/selection/context
draws against independently decoded font UVs. Online-only routes retain the
existing sign-in gate. The font-sampling regression verifies that optional HD
fonts preserve prompt targets as well as row targets.

`tools/verify_frontend_mouse_prompts.py` submits real SDL left-button events to
the native window through `--menu-action click:X:Y`. The capture helper uses
ordinary disposable menu routes for setup; only the final clicks are deferred
to the SDL event loop. The coordinates are SDL window coordinates, and queued
clicks retain their order. It checks actions, resulting screens, binding-capture
state, output size and Vulkan validation errors. Cases cover all six aspect
ratios above, title Start, error Continue and covered lobby controls, icon
Toggle/Cancel, keyboard Done/Cancel, and Vista Select/Back/Edit/Delete/captions.

```bash
ctest --test-dir build-sandbox-release-20261010 \
  -R 'retail_frontend_(mouse_prompts|font_sampling_painter)$' --output-on-failure
python3 tools/verify_frontend_mouse_prompts.py \
  --launcher build-sandbox-release-20261010/Halo2-Title-Launcher \
  --output analysis/frontend/mouse-prompts-20261010/captures
```

Reports are under `analysis/frontend/mouse-prompts-20261010/`. The 19-route
SDL bank is in `captures-final/`; `captures-delivery/` checks the installed
launchers after the final transparent-text hit-admission fix. Validation covers
Linux SDL/Vulkan, the current native menu inventory and imported stock/Vista
assets. It does not establish physical mouse-device behavior on Windows/macOS,
new future/localized layouts, or unavailable online/game services.

## Vista special-button bounds

Original PC special-button vtable slot14 reaches63FFF8. It copies the widget's
rectangle and subtracts the signed authored button+54 value from the hit top.
The drawing owner retains the authored rectangle. The importer now preserves
that signed field and the complete button+56 flag word, and the PC painter
applies the hit-only adjustment. Xbox button behavior is unchanged.

The actual PC14 START GAME record has top480 and offset-12, so its hit top is
492 while its draw top remains480. The focused regression reads that real
60-byte record, checks the extra clickable strip, and verifies identical draw
positions before/after the hit adjustment. A separate controlled source copy
checks signed offsets and all32 flag bits through the real importer. It does
not admit the unfinished PC14 lobby route.

Original geometry evidence is retained in
`analysis/frontend/vista-session-flow-audit/pc14-layout-ownership-original/geometry-original.json`.
The unchanged original raster, projection, inverse mouse, special-button and
grid instructions execute across eight viewport profiles through32:9. The
current centered native formula differs by at most0.0001220703125 pixels in
this bounded binary32 corpus; it is not a float-identical or framebuffer claim.
