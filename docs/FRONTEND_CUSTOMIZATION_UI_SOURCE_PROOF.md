# Player customization UI source proof

The native Appearance pages now use the original authored controls, localized
value strings, profile name group and child-list window behavior. These changes
are based on the retail executable and mainmenu definitions, without fitted
positions, colors or replacement icons.

## Routes and definitions

The normal saved-profile route is title9 → Choose Profile10 → main6 → Settings19
→ Player Profiles193 → profile actions194 → profile settings38 → Appearance47.
New profile creation follows9 →10 → keyboard167 →38 →47. `2C8CFC` routes the seven
Appearance rows to the following child definitions:

| Row | Child screen | Choices | Visible widgets | Original row pitch |
| --- | ---: | ---: | ---: | ---: |
| Player Model | 52 | 2 | 4 | 22 |
| Primary Color | 50, slot0 | 18 | 4 | 36 |
| Secondary Color | 50, slot1 | 18 | 4 | 36 |
| Primary Emblem Color | 50, slot2 | 18 | 4 | 36 |
| Secondary Emblem Color | 50, slot3 | 18 | 4 | 36 |
| Emblem Foreground | 49 | 64 | 4 | 22 |
| Emblem Background | 233 | 32 | 4 | 22 |

The original foreground/background masks come from executed `2CB200/2CB220`.
All64 foreground and all32 background choices are enabled. Screen48 is the
profile deletion confirmation, not an appearance child.

`2BAFA4` computes the bounds union and `24BC12` advances each child by its
height. The original list positions and resulting unions are:

| Screen | Authored list origin | Bounds top,left,bottom,right | Pitch |
| --- | --- | --- | ---: |
| 47 | (-271,-9) | (34,10,6,320) | 28 |
| 49/233 | (-226,-77) | (28,10,6,210) | 22 |
| 50 | (-226,-44) | (42,10,6,250) | 36 |
| 52 | (-226,-87) | (28,10,6,210) | 22 |

The old native default child pitch20 omitted the marker bitmap's contribution
to the union. The correction reads the authored union instead of adding a
screen-specific coordinate shift.

## Labels and name group

Unchanged `2C8BF5` assigns both text children of each Appearance row. The second
child resolves species through `2365E0`, the four color indices through
`14986F`, foreground through `149B5A`, and background through `149DA5`. Native
binding retains the localized private-use color glyphs in those strings.

The screen header always receives text-buffer flag2 through `22FDA6/22CD48`.
Original `13FAE0` converts ASCII lower-case letters, é and ó to their upper-case
equivalents. The painter applies exactly that mapping to headers.

Original `2C8D7C` passes one flags3 profile item, name pointer54E5E0 and the
appearance copied from54E6F0 to `22F042`. Its authored group origin is(-271,38).
The name text placeholder0B000698 uses font4 and bounds(130,166,110,320), giving
its unanimated screen origin(215,72). Name color follows the separately
[proved original HSV path](FRONTEND_APPEARANCE_SOURCE_PROOF.md).

Bitmap placeholder1400069A is shown by `2AFEAE`, selects sequence0 through
`2B01DA/2B0A14`, and receives raw primary palette RGB from `7F790`. Original
`1A06A0` supplies literal white for invalid signed color indices; the native
binding uses the shared source color resolver for that fallback too. Original
`2BACBC` adds the bitmap's19-pixel height before the group origin, yielding the
168×19 namebar at(212,74). At equal depth/layer10, the executed `2356D8`
comparison places the bitmap before the text child.

Original bitmap packing `2B05C8..2B0635` multiplies each channel by255 using
f32 arithmetic, stores that result, then executes x87 FISTP under FPCW027F.
It rounds to nearest with ties even. This differs from the font's truncation;
alpha.5 becomes128. The painter applies this packing only to the newly bound
namebar tint. It uses the original ordinary texture×vertex RGBA program, with
no invented gray or saturation adjustment. Emblem composition and its exact
large/small quads have a [separate source proof](FRONTEND_EMBLEM_RESEARCH.md).

## Focus and scrolling

Original `24C515` chooses an existing focused child when a datum is already in
the window. Otherwise it chooses cell min(datum,last-cell), and `24C461` assigns
the sibling datums around it. `24C1C5` moves focus inside those same widgets
without replacing their datums, until an edge or wrap is crossed.

For example, selecting color10 produces window[7,8,9,10]. Three Up presses
focus9,8,7 while retaining that window. The next Up replaces it with[6,7,8,9].
The old native painter recomputed a last-row window every time, shifting all
rows on every Up press. `MenuFrame::first_visible` now retains the proven
window for screens49/50/52/233. Other list behavior is outside this correction.
Mouse selection inside a visible window retains its datums and pointer hits
continue to resolve absolute choice indices. Arrow visibility uses the actual
first and last displayed datums.

The edited appearance preview callbacks remain active while moving focus.
Accept, cancel snapshots, foreground X toggle and saved appearance commitment
are covered by the [appearance state proof](FRONTEND_APPEARANCE_SOURCE_PROOF.md).

Successful profile rename also refreshes the retained profile picker's name.
Original `238742` ends the edit through `148BFF`, which clears
`g54E5D0.profile_index` at `54E5D4`. The matched `24A01F` list update observes
the transition from its previous non-NONE index to NONE and calls `24A150`.
That matched routine rebuilds via `24A190` and retains/clamps the focused datum
slot. The native rename keeps the same slot and count, so it refreshes copied
saved-profile labels in retained screens10/193 without replacing focus or their
visible window. The appearance regression now follows the actual saved-profile
route193 →194 →38 →167, accepts a pending color, renames, and returns to193.
Its first run failed on the stale old name; the correction passed while retaining
the selected slot, window, saved appearance and focused model appearance.
Logs are `appearance-rename-retained-{red,green}-{build,tests}.log` under
`analysis/frontend/customization`. Network-task refresh gating and input timing
are outside this bounded correction.

## Independent child animations and subtitle binding

The first full33-case native customization matrix exposed a child popup
placement defect on49/50/52/233. Each pane uses animation56, whose steady offset
is(-72,-67). Its title, description and footer independently use animation54,
whose steady offset is(0,0); panel bitmap widgets independently use55 or56.
The old painter added the pane offset to those already animated child widgets.
This shifted the panel left72/down67, clipping text and exposing parent rows.

Retail `22E9C6` copies only the widget bounds at+20/+24 and adds its own
animation offset at+58/+5C. `22E9FF` performs the corresponding float-bounds
operation. `2538F5` reads only that text widget's animation scale at+64 before
calling `22E9C6`. `22FC08` starts each child widget's animation independently;
there is no inherited pane transform in these draw paths. The correction removes
the extra pane transform/alpha from pane text, bitmap and button submissions.
Explicit grid and list placement remains, and scroll arrows retain the list
origin without inheriting pane animation. The authored title projects to(69,218),
description to(69,237), footer right364/top390, and UL bitmap to(45,204).

The real MenuState/painter regression checks these original projected bounds
through52, all four50 color slots,49 and233. Its semantic first run failed at
the shifted popup title. After correction, it reached the next independent
subtitle regression, establishing that the entire popup geometry and composition
test sequence had passed. Parent row submissions remain source-admitted: correct
panel placement supplies their occlusion, rather than a synthetic visibility
gate. The separately proved child backdrop policy is documented by the
composition investigation and tests.

A remaining3-pixel A/SELECT footer difference on47 was an enum binding error,
not a font offset. Original `1496F6` maps subtitle type11 to string180006D8,
whose text has two spaces between SELECT and the B icon. Native code selected
locale range position10, which holds unrelated0F001775 with one space. The
English range order differs from the enum order; type5's110006D2 is absent.
The corrected parser resolves the literal22 source string IDs within the
authored range. An absent ID stays empty, as original `1A02D0` specifies.
Individual child subtitle strings retain their own authored spacing.

`tools/oracle_frontend_subtitle_strings.py` executes unchanged1496F6,
1A0180,1A0350,1A02D0,140440 and3207ED for25 inputs(-1 through23), using the
actual English locale table and unic range with relocated pointers. Only
148350 supplies the relocated wigl; language0/table-loaded1 are explicit inputs.
It separately executes original22FF53 subtitle setup and original font
traversal with real conduit-12 records. Widget linkage/string binding and font
cache readiness/final glyph submission are supplied for that glyph stage.
The resulting footer callbacks place A atx472, SELECT atx491..528, B atx540,
and DONE atx560..585, all aty427. The painter regression checks all12 nonspace
glyphs on the real47 route, as well as the exact type11 text and empty type5.
Its first run failed on enum11 before the parser correction.

The frozen `analysis/frontend/customization/subtitle-strings-original.json`
SHA-256 is `7540f1ac353b11faebd6eec62dc4cb58a160888bcb4aaa653a35337fc6674bc7`.
This proves selector, localization and glyph placement boundaries, not GPU
sampling or synchronized framebuffer parity. The pre-correction33-case matrix
and explicit failed visual review remain under `final-verification`; fresh
captures must replace their role as delivery evidence, not rewrite those files.

## Direct popup widget layers

After the geometry and subtitle corrections, the retained52 reference still
showed a blue-tinted title and lifted black glyph shadows. This is authored
composition rather than a different caption color. On the four known child
panes49/50/52/233, every steady animation depth is0. Their appearance_detail
bitmap has layer0/type8, caption and description have layer0/type6, dialog1
and dialog2 have layer1/type8, corner glow bitmaps have layer2/type8, and the
footer has layer5/type6. Original2356D8 orders that same-depth group by layer
ascending, then type descending. Native previously submitted every bitmap
before every text, putting the caption/shadow above the blue panel.

`tools/oracle_frontend_popup_widget_order.py` executes307 unchanged original
2356D8 comparisons with those actual serialized layers, types and animation
depths. It freezes all four child metadata sets in
`analysis/frontend/customization/popup-widget-order-original-v2.json`, SHA-256
`23d802dc106b249befbc038ce0415d6445ffac79bd1cb0a56c7c083a54a525ba`.
The v2 reader applies the original253765/2B01EB authored animation minus1
conversion before reading the steady key. The first retained fixture selected
the following animation record; its supplied depths were also0, so comparator
inputs/results were unchanged, but its animation-key provenance was incorrect.
It remains retained as earlier evidence rather than being silently rewritten.
Internal ordering among equal layer/type/depth widgets is deliberately outside
this proof. The original epsilon depth comparator is not used as a general
native sorting predicate.

`tools/audit_frontend_popup_texture_layers.py` independently decodes the actual
BC3 dialog1 page and projects it using its authored(-139,104) and own animation
offset(-131,-73). At the retained reference's title pixel(70,228), texel
RGBA(63,162,255,68) over authored glyph RGB(172,195,216) yields(143,186,226),
exactly the reference. At shadow pixels(71,231) and(72,243), the corresponding
texels over black yield(17,44,69) and(17,43,68), also exactly the reference.
The original corner glow texels at these points have zero alpha. The frozen
`popup-text-overlay-source-audit-v2.json` records those literal asset inputs and
standard source-alpha compositions, without offset or color fitting. It derives
the dialog1 definition and one-based animation conversion directly from52's
serialized pane before checking the projected origin and sampling the page.

The real MenuState/painter regression follows creation into47 and opens each
child. Its semantic first run failed because the panel/glow was below the
caption. The correction reorders only these known panes' contiguous direct
bitmap/text ranges before their list submissions, within the proved depth
group. It reuses the bounded keyboard layer-ordering seam; scenes, their
before_vertex boundaries, list rows, grid groups and equal-group internal
ordering remain outside the new sorting scope. Fresh framebuffer captures are
required to validate the resulting pixels; these source and painter checks
alone do not establish complete customization pixel parity.

The combined retail_frontend run passed in7.33s after the mixed ordering
correction, including all four child-layer regressions, the preceding pane
geometry and subtitle checks, and the independent scenario-filter admission
regression. The earlier33-case `verified-customization` captures and explicit
visual review predate the layer/filter corrections. The fresh
`source-matched-customization` matrix is the corresponding delivery capture
boundary; its visual review must retain any remaining differences rather than
treating the passing route checks as whole-frame parity.

## Pinned Vulkan UI BC3 expansion

The first mixed-layer capture reproduced the retained title and title-shadow
diagnostic pixels exactly, but description-shadow pixel(72,243) remained
(17,44,69) instead of(17,43,68). The pinned Xemu0.8.136 Vulkan backend always
passes compressed2D texture levels through its CPU `s3tc_decompress_2d` before
uploading RGBA8. Native UI preparation retained format16 and uploaded hardware
BC3 instead. The same integer-expansion boundary was already established for
the title's BC1 noise texture; model/general compressed textures are outside
this UI correction.

`tools/oracle_frontend_ui_bc3.py` compiles and invokes the unchanged retained
`s3tc.c` at commit `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`. Its SHA-256 is
`bf25e6ffa9cf1c3620b974cf638f9838ea298223d0fa7f1c73910fa761fc9c4a`.
Only the allocation/include environment is supplied. The312 original fixtures
cover13 alpha endpoint pairs,6 RGB565 endpoint pairs and4 dimensions, including
both alpha branches, equal/extreme endpoints, all8 alpha/all4 color selectors,
and clipped blocks. The tool also freezes all65536 actual dialog1 RGBA texels;
the unchanged decoder output SHA-256 is
`a7cabea687ba02c9e7734c4ccdc64e0fca8df0981d82b01218a34237e48ccdba`.
Inputs and outputs are retained in `tests/fixtures/frontend_ui_bc3_original.tsv`
and `frontend_ui_dialog1_original.bc3/.rgba`, with provenance in
`analysis/frontend/customization/popup-bc3-audit/original-bc3-fixtures.json`.

At the description diagnostic's source texel(22,34), alpha endpoints70/68 and
selector6 give integer(2*70+5*68)/7=68. Source RGB565 endpoint expansion and
color interpolation also truncate, producing RGBA(62,161,255,68). The earlier
Pillow audit produced(63,162,255,68); its three rounded framebuffer predictions
still matched the reference, but it did not establish the pinned backend's
actual expanded texture bytes. Those earlier reports remain retained, and the
unchanged decoder fixtures supply the stronger texture boundary.

The correction expands only format16 in `prepare_ui_bitmap`, before filtering.
It preserves the exact original four-color DXT5 rule even when color0<=color1,
integer endpoint/alpha arithmetic, clipped block writes and checked mip bounds.
Model/general `prepare_texture` remains compressed. All25 authored mainmenu UI
BC3 pages have resident-mip maximum0, so their current top-only caller does not
discard an authored resident chain. The derived implementation retains the
original MIT attribution and was independently reviewed against the original
decoder. The tests-only run failed in0.02s because UI BC3 was still compressed;
after correction retail_frontend passed in7.20s, including312 original cases,
the entire dialog1 page and actual frontend-loader integration. Fresh delivery
captures validate the GPU result separately; this proves the stated Xemu
Vulkan texture boundary rather than every Xbox GPU's decoding behavior.

The final82f4a540 native capture matches all three unchanged-reference
diagnostic RGB values exactly: title(70,228)=(143,186,226),
title-shadow(71,231)=(17,44,69), and description-shadow(72,243)=(17,43,68).
`customization-delivery/popup-layer-bc3-final-pixel-check.json` retains their
source texels, decoder RGBA bytes, composition, and binary/image hashes. No
alignment, color or offset fitting was performed. Whole-region/background
differences retain their separate, unsynchronized-phase boundary.

## Executed evidence and validation limits

The original XBE SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The mainmenu SHA-256 is
`1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.

- `tools/oracle_frontend_appearance_ui.py` executes row/string assignment,
  availability masks, bounds union, flags3 group binding and title casing.
  `appearance-ui-original.json` SHA-256:
  `5dbb9007239d6a670d45401624ecf91a45f425c4d9c4402d48a22cb86df3b46c`.
- `tools/oracle_frontend_appearance_window.py` executes18 original focus/window
  cases using four authored widgets and18 original contiguous color datums.
  `appearance-window-original.json` SHA-256:
  `a488135e9b2c3020f3bdd7ff9c54ea73fe29ba5544f6d7f656ad5a94b6b56324`.
- `tools/oracle_frontend_appearance_namebar.py` executes60 original cases:
  all18 palette entries and invalid indices-1/18 at alpha1/.5/.3, including actual RGB writes,
  visibility, sequence and ARGB packing, plus the bitmap/text order comparison.
  `appearance-namebar-original.json` SHA-256:
  `5a2f49d20c6380744b9c1d903562c0ba56fa317849ae805dc68f8388d142b68f`.

The oracles declare supplied boundaries: ready widget linkage and definitions,
localized text getters/setters, contiguous datum stepping, focus predicate and
notification/sound, and alpha supplied to the bitmap kernel. Constructors,
input repeat timing, focus transition alpha, GPU sampling and framebuffer
composition are excluded where applicable.

`retail_frontend` follows the real creation/edit route and checks actual painter
header/name/value glyphs, child pitch22, all18 namebar colors, nearest-even
alpha, the source first-visible-row regression and all18 original focus/window
outputs. Initial failures and passing reruns are retained in
`appearance-ui-red-*`, `appearance-window-{red,green}-*`, and
`appearance-namebar-{red,green}-*` logs. The earlier focused frontend, appearance
and emblem run passed3/3. These checks establish the scoped UI bindings and
window behavior; they do not establish complete customization pixel parity,
transition timing, or all model/shader rendering.
