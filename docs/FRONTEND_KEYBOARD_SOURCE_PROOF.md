# Original keyboard input field and generated profile name

`tools/oracle_frontend_keyboard.py` executes the unchanged original bitmap
constructor, widget comparator, default-name collision loop and keyboard
string setup. Bitmap resource loading, standard string routines and platform
save existence are the explicitly bounded seams listed in the result.

Pinned inputs:

- `assets/disc/default.xbe` SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- `assets/disc/maps/mainmenu.map` SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.

Reproduce with `.tools/python/bin/python tools/oracle_frontend_keyboard.py`.
The structured result is
`analysis/frontend/profile-regression/keyboard-original.json` (SHA-256
`970b98736ba10ae8442599c9d74389e057e16a7eb179f84308c0454d8dce171f`).
Two runs produced identical report bytes, checked by
`keyboard-original.sha256`.

## Authored input box layer

The original keyboard pane has the following serialized bitmap definitions:

| Index | Bitmap | Position | Dimensions | Layer |
| --- | --- | --- | --- | ---: |
| 0 | `ui\screens\game_shell\virtual_keyboard\field_2` | (-257,79) | 520×53 | 1 |
| 1 | `ui\global_bitmaps\fs_dialog` | (-270,200) | 1024×512 | 0 |

Original bitmap constructor `0x2B01EB` copies definition `+0x20` to widget
`+0x6A`. The oracle executes this constructor, including original base
constructor `0x22E27B` and flag helper `0x2B0A68`, against the actual copied
definitions. Only resource loading `0x23625D` is intercepted.

Original render collection `0x23566A` gathers visible widgets and their own
animation depth, layer, and type. Original comparator `0x2356D8` draws larger
depth first; when the absolute difference is at most `0.001`, it compares
layers in ascending order, then widget type. Both keyboard bitmap widgets
have type `8` and zero steady animation depth. Executing the original
comparator returns `1` for field/dialog and `-1` for dialog/field: the dialog
must draw first, then the input field.

The previous native loader discarded the bitmap layer field, and the painter drew
the serialized field/dialog order, letting the dialog cover the input box.
The correction loads the original layer and sorts a pane's bitmap widgets
by layer only when their entire depth range is within the original `0.001`
tolerance. Outside that same-depth group it preserves the previous order;
this bounded fix does not claim a complete original widget-sort port.

The integration regression examines the real keyboard foreground after its
popup boundary and requires the authored dialog quad to precede the authored
field quad. It failed before the correction with
`keyboard dialog covers the authored layer1 input-field bitmap`.
Feedback is recorded in `keyboard-field-red-tests.log` and
`keyboard-field-green-tests.log` under
`analysis/frontend/profile-regression/`.

The subsequent actual selected-name capture exposed a second ordering gap:
native text/button metadata also discarded the layer. Original `0x253765`
copies signed text definition `+0x28` to widget `+0x6A`; original `0x253CFF`
copies signed button definition `+0x34`. The keyboard's input/description
texts have layer `5`, and all keys have layer `2`.

A read-only snapshot of the owned original keyboard session records all 54
widgets, including their type, layer, own animation depth, and alpha in
`original-keyboard-runtime-assets.json`. Every widget has depth `0` and alpha
`1`. `tools/oracle_frontend_keyboard_order.py` executes unchanged `0x2356D8`
against their actual captured headers. The resulting distinct groups are:

| Draw group | Layer | Widget type |
| --- | ---: | ---: |
| Dialog artwork | 0 | 8 |
| Screen selection/caret | 0 | 0 |
| Input-field artwork | 1 | 8 |
| Key artwork/labels | 2 | 3 |
| Input/description text | 5 | 6 |
| Header/footer text | 32767 | 6 |

The native keyboard previously drew the opaque-white selection after the
field artwork and input text. The correction preserves complete widget draw
chunks and applies the proven layer/type order when the entire keyboard
depth range is within `0.001`. It leaves the retained lower title widgets and
popup boundary intact. Unequal-depth transition sorting, original qsort
equal-group tie order, and widget Z projection are outside this bounded fix.
The real-painter regression checks the authored layers and requires dialog
before selection/caret, then field artwork, keys, and input text. The
reproduced failure is recorded in `keyboard-layers-red-tests.log`; the three
passing frontend/font checks are in `keyboard-layers-green-tests.log`.

The ordering fixture `keyboard-widget-order-original.json` SHA-256 is
`39a064537c39349099f1cca86171a51a401aa04032aece470fdb63568bed61bf`;
its captured runtime input SHA-256 is
`ffc903cb0b3cb51ec047426c0319c6bc04c3c7b034083e525a79b63f0338aa32`.

## Generated default profile name and selection

Original caller `0x148C3E` obtains a default profile name from `0x215F40`,
copies it to the edited profile and opens the keyboard with `0x238C21`.
`0x215F40` uses the executable's literal `Halo%04d` format, starts with
suffix `1`, and advances while the platform reports an existing save name.
The oracle executes the original loop with controlled save-existence
results:

| Existing names | Original attempts | Selected candidate |
| --- | --- | --- |
| None | Halo0001 | Halo0001 |
| Halo0001 | Halo0001, Halo0002 | Halo0002 |
| Halo0001..Halo0003 | Halo0001..Halo0004 | Halo0004 |

Original `0x2381DE` copies the original string, caps the profile-name buffer
to 17 UTF-16 units, puts the cursor at its end and sets `editing=true`.
With `Halo0001` the executed result is cursor `8`, maximum `17`, and the
whole string selected. Original `0x2389C7` clears that selected string on
the first typed key; cursor movement `0x238A63` clears the selection while
retaining the string.

The native correction supplies the first unused generated name for the
new-profile keyboard purpose and retains the existing entry selection and
cursor behavior. The real-menu regression accepts the untouched defaults
Halo0001..Halo0004 through customization, sign-in and sign-out, checks DONE
focus `40`, and verifies first-key replacement and insertion after moving
the end cursor left. Feedback is recorded in
`keyboard-default-red-tests.log` and `keyboard-default-green-tests.log`.

## Selected string and bitmap caret

Original keyboard virtual draw `0x237C04` updates its input text with the plain
string (`0x238CBA`), draws its selection, then draws its bitmap caret. The
previous native display inserted a literal `|` glyph and omitted selection.

The selection reads `matg`'s first interface record `+0x64`, the actual
`rasterizer\default_2d` datum `E4750255`, and uses bitmap descriptor/page `1`
(4×4). It supplies white ARGB `FFFFFFFF`, UV clamp `1`, and blend mode `0`. Original
`0x253AB4 → 0x22D108 → 0x22D13D` uses the projected text top/left and the
measured glyph-ink right/bottom. The independent text oracle
`tools/oracle_frontend_text_style.py` executes `0x13F0E0` and `0x13EA60` with
the actual `conduit-13` records. Its `field_measurement` in
`frontend-text-style-original.json` reports:

| Measurement for Halo0001 | Top | Left | Bottom | Right |
| --- | ---: | ---: | ---: | ---: |
| Glyph ink | 167 | 73 | 189 | 137 |
| Selection output | 167 | 72 | 189 | 137 |
| End-cursor line bounds | 167 | 137 | 187 | 138 |

The caret reads retail `wigl+0x114`, actual datum `E4D702B7`,
`ui\global_bitmaps\global_bitmaps` descriptor/page `0` (6×17).
`0x236235 → 0x137610` resolves frame `0`, sequence `0`; the cache contains no
sequences, so `0x137590` selects descriptor `0`. It stretches that bitmap into
the prefix **line bounds** returned by `0x253A73 → 0x22D0E3`, rather than its
glyph-ink bounds. Moving the end cursor one character left clears selection
and moves its one-pixel line rectangle to `[128,129]` horizontally.

The original `0x237C04 → 0x235E5E → 0x23675A` caller now executes
unchanged in `tools/oracle_frontend_keyboard_draw.py`, using the actual bitmap
headers. It records selection arguments `{destination, FFFFFFFF, 1, 0}` and
caret arguments `{destination, FFFFFFFF, 0, 0}`. In `0x235E5E`, argument
`+0x10` is the UV-clamp flag; argument `+0x14` is the blend mode. Both produced
descriptors contain blend `0` at `+0x94`. The frozen
`keyboard-draw-original.json` SHA-256 is
`252aa130ba3f322ac6ff4fe28c755559cacbe764637dc4f32329c8de0636c7d5`.
Measured bounds and resource readiness are declared input seams; the final GPU
sink is recorded, so this probe establishes caller ABI, geometry, UVs, colors,
and the descriptor, rather than final texture binding or framebuffer pixels.

An earlier interpretation treated the selection's clamp `1` as blend `1`.
That was incorrect. The former direct mode-1 descriptor probe demonstrated an
alternate producer input, without proving it belonged to this caller. Native
selection now uses the ordinary page-1 material. The separate multiply material
and pipeline were removed. `keyboard-selection-red.log` records the actual
native navy-box mismatch and the caller evidence that exposed this error.

The original render-state table `0x142F0` maps the caller's mode `0` to
`SRC_ALPHA` (`302`), `ONE_MINUS_SRC_ALPHA` (`303`), and `ADD` (`8006`). Original
`0x52071` writes color mask `10101` to NV2A method `40358` (RGB).
`tools/oracle_frontend_bitmap_combiner.py` independently executes the actual
`0x235E5E → 0x23675A` wrapper and passes its descriptors through unchanged
ordinary `0x52040` combiner construction. The fixture's
`original_235e5e_23675a_producers` records selection clamp-1/blend-0 and caret
clamp-0/blend-0. Both use texture RGBA multiplied by vertex RGBA with white
stage-0 constants; their combiner program SHA-256 is
`4975670784843c86693f87b1598716d991b2e8e88591781ed9efb5e0f4a287fd`.

The regression checks actual source texture pixels, selection/caret bounds,
and draw order. The earlier missing-overlay red/green results remain in
`keyboard-overlay-red-tests.log` and `keyboard-overlay-green-tests.log`.
The source default-2d page-1 disk payload and native prepared texture are
opaque white. The owned original runtime snapshot independently reads that
page's cached D3DTexture `Data=00A3C000` through its readable kernel alias
`80A3C000` and confirms the same opaque-white texels. The distinct original
widget ordering above places that white rectangle beneath input-field
artwork and text. Native now reproduces that order instead of covering the
completed field with white. No gray color constant was inferred from the
screenshot. Fresh native framebuffer captures still establish the visual
result; these source/runtime proofs alone do not claim complete pixel parity.

## Description wrapping at the authored right edge

The reported profile-description mismatch is reproduced with the actual
localized `0x1D00044A` text and authored `conduit-12` definition. Its authored
rectangle `{149,-254,89,263}` projects to `{91,66,151,583}`, width `517`.
`tools/oracle_frontend_keyboard_wrap.py` executes unchanged
`0x13F0E0 → 0x13EF40`, the original iterator and Unicode break predicate,
then `0x13F700`. Only ready font-cache lookup/pixel readiness and the final
glyph callback are supplied. The actual font header and glyph records remain
unchanged. `keyboard-wrap-original.json` freezes each glyph and these results:

| Width | First line ending | Second line |
| ---: | --- | --- |
| 516 | `be up ` | `to 16 characters.` |
| 517, authored | `be up ` | `to 16 characters.` |
| 518 | `up to ` | `16 characters.` |
| 525 | `up to ` | `16 characters.` |
| 534 | `up to ` | `16 characters.` |

Original `0x13EF40` requires `right > candidate_endpoint`. The space following
`to` reaches exactly `517`; equality fails and the line returns to the prior
allowed word break, after the space preceding `to`. Native previously accepted
equality and recorded the overflowing space as its new break point. The
correction rejects equality and records this ASCII break after the preceding
space, preserving that space in the original first-line glyph traversal.
No authored bounds, font metrics, colors, or visual offsets changed.

The real-painter regression verifies the actual description and every emitted
foreground glyph's atlas UV and pixel position against the original two lines.
`keyboard-wrap-red-tests.log` records the reproduced incorrect traversal;
`keyboard-wrap-green-tests.log` records the passing frontend bounds/retail tests.

## Action-label kerning and subtitle color

The retained production keyboard differed from the original at CAPS LOCK and
the ENTER/CANCEL footer. `keyboard-composition-pixel-check.json` pins that
earlier binary and both screenshots. At pixel `(495,307)` a native background
pixel replaced the original CAPS LOCK ink; at `(471,423)` the footer used
`(172,195,216)` while the original used `(173,173,173)`.

`tools/oracle_frontend_keyboard_labels.py` executes original `22FF53` until its
`2538A6` setup call. With the actual screen-167 definition and shared metadata,
the original selects font 1, right justification, bounds
`{-178,-200,-198,259}`, and RGBA `{1,1,1,1}` from the screen definition at
`+8`. The native footer previously used the shared title color. It now uses
the existing parsed `UiScreen.color`, matching original `22FF53`'s subtitle
color selection. Original normalization and packing produce `FFADADAD` for
ordinary footer letters; private controller glyphs retain white vertex RGB
and the atlas's colors.

The same oracle executes original `13F0E0`, `13EF40`, `13F700`, and `122570`
with actual conduit-12 records. The machine-code callers place the previous
character in EDI and the current character in EBX. `122570` searches the
serialized pair in that order. The native frontend's lookup had reversed
these characters. The authored pair `P,A,-1` therefore incorrectly shortened
`A,P` in CAPS LOCK and failed to shorten `P,A` in BACKSPACE. The measured
width affected centering as well as later character advances. All three
frontend lookup sites now use `{previous,current}`; there is no added pixel
offset.

The frozen `keyboard-labels-original.tsv` has SHA-256
`00bb4769c8b0595a393e8ff4f26046fbef5e2ba5f85ffe838c244719fc9c314c`.
Its 77 glyph records cover all nine visible retail action labels and the
footer, including disabled ACCENTS/SYMBOLS and default focused DONE. CAPS
LOCK's original glyph x positions are
`468,474,482,489,496,500,507,515,522`. The original footer's E begins at
`(471,418)` with packed color `FFADADAD`. These are executed glyph outputs,
not values inferred from screenshot edges.

The production painter regression first failed on BACKSPACE's original
controller glyph at x `300.5`. After the source correction, all 77 original
glyph positions, sizes, UV identities and packed colors match. The existing
description wrapping, header geometry, keyboard bounds, and font color/shadow
tests also pass. Logs are `keyboard-labels-red-tests.log` and
`keyboard-labels-green-tests.log`. Child linkage and localized binding are
supplied for the subtitle setup probe; font-cache readiness and final glyph
submission are supplied for the glyph probe. General text control tokens and
arbitrary string iterator transitions remain outside this single-line
keyboard fixture; this does not establish complete framebuffer parity.
The wrapping fixture SHA-256 is
`ed28b7fb4de1cdf112d455c52abcbf543aa780773b21ef4582dd4aab61507a3e`.
This verifies the supplied English profile description and the measured edge
cases, rather than claiming all Unicode line-breaking behavior is implemented.

## Cumulative UI clock and caret opacity

Original `0x1479C3` receives a stored float frame delta, multiplies it by
`0x45DCCC = 1000.0` in x87 double precision, calls the original truncating
conversion `0x322B0C`, then adds the result to unsigned `g_54D5B8` at
`0x1479D4`. It starts at zero in executable zero-fill and is not reset by UI
initialization `0x1476C7`. `0x12B450` calls it after the original frame-delta
calculation `0x12B0E0`, which clamps the supplied delta to `[0,10]` seconds.
The UI update is skipped entirely while `g_509340` is true. Changing menus
does not reset this clock.

The caret's `0x237D30` corridor converts the unsigned clock to x87, computes
`(cos(clock * 0.003f) + 1) * 0.5 * widget_scale * 255`, stores a float and
converts with nearest-even rounding. The unchanged original instructions
produce alpha bytes `255,221,137,1,0,101,255` at milliseconds
`0,250,500,1000,1047,1500,2094`. Cases also cover quarter opacity and unsigned
clock values above `2^31`. Native helpers are checked against these executed
results, including per-frame truncation and unsigned wrapping. The painter
receives this clock independently of its screen-entry animation time.

Original pane setup `0x22F6CA` sets the screen's animation index to the
pane definition's animation. Its own screen-widget alpha therefore supplies
caret opacity, while `0x22E9C6` adds only the field widget's own animation
offset to the measured text rectangle. The saved live
`original-keyboard-channels.json` screen-167 bytes at `+0x64` confirm steady
alpha `1` for the tested keyboard.

Animation `31`'s 250 ms intro/outro moves widget Z between `1000` and `0`.
Original `0x22D13D` projects bounds through `0x2360C3`; native pane/text/overlay
drawing does not yet port this depth projection. The steady geometry checks
therefore do not establish intro visual parity.

The oracle bounds platform existence to supplied display-name collisions;
it does not prove complete Xbox save-folder encoding or native filesystem
equivalence. These checks do not establish complete keyboard pixel parity,
complete intro animation timing, original hardware clock phase under movie
decode blocking, popup GPU pixels, or aspect-ratio presentation.
