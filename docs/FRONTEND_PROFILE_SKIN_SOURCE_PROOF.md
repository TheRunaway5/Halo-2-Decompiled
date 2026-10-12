# Original profile-list nested skin

`tools/oracle_frontend_profile_skin.py` executes the unchanged original XBE
placement, rectangle union, item visibility, frame selection, RGB/HSV, row
focus propagation/update, intro focus gating, and button RGB
instructions against copied retail skin definitions. Allocation and child
construction are bounded seams initialized from the documented original
constructor bounds; text-buffer getters/copies are intercepted. The emblem
appearance updater is recorded rather than executed, so its drawing and
appearance-case visibility are explicitly unverified.

Pinned inputs:

- `assets/disc/default.xbe` SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- `assets/disc/maps/mainmenu.map` SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Original outer skin `E50202E2`,
  `ui\list_skins\profile_picker\profile_picker`.
- Original nested skin `E50402E4`, `ui\player_skins\player_skin_long`.

Reproduce with `.tools/python/bin/python tools/oracle_frontend_profile_skin.py`.
The structured result is
`analysis/frontend/profile-regression/profile-skin-original.json` (SHA-256
`ddb08c21859871c367205af1048bccd0dd112df10a2b31eec4bb2e5ddbc67800`).
Two original executions produced the same report bytes, verified by
`profile-skin-original.sha256`.

## Create New Profile visibility

Original `0x24A09B` gives Create New Profile item flags `0x20` and `value5e=1`,
shows the **outer** text, and passes the item through `0x22F042` to the nested
group's `0x2AFEAE` updater. For the nested group the original executed result is:

| Child | Placeholder | Enabled |
| --- | --- | --- |
| Bitmap 0, `ui\player_skins\item_background` | `1400069A` | No |
| Bitmap 1, `ui\list_skins\profile_picker\item_background` | `0E00069B` | Yes |
| Bitmap 2, rank icon | `0900069C` | No |
| Emblem, type 9 | — | No |
| Text 0, profile name | `0B000698` | No |

Bitmap 1 is the authored Create New Profile stripe. Its definition flag `1`
excludes it from the rectangle union; it does **not** prevent `0x2AFEAE` from
enabling its runtime visibility. A painter that unconditionally skips this
definition flag omits the stripe.

For a controlled saved item with only flags `1` and a supplied name, the nested
name is enabled and copied, while the stripe, background, rank and emblem are
disabled. The actual saved-profile caller also supplies appearance when
available (flags `2`), enabling the appearance-colored bitmap background and
delegating the emblem update.

`1400069A` depends on appearance or a supplied **team** (`value5c`, flag `4`).
It does not depend on the **rank** (`value60`, flag `0x40`). A separate rank-only
probe with rank `8` enables bitmap 2, selects its frame `8`, and leaves bitmap 0
disabled.

## Original placement and row extent

Original `0x2BACBC` shifts bitmap and emblem bounds upward by their height,
then adds the supplied point. Text bounds only receive the supplied point.
The grid at `(32,-12)` is placed by original `0x2AFDBC`. Bounds below use the
authored upward-positive UI coordinates, before list placement or animation.

| Child | Top | Left | Bottom | Right | Excluded from union |
| --- | ---: | ---: | ---: | ---: | --- |
| Outer end cap | 28 | 2 | 5 | 12 | Yes, flags `3` |
| Nested bitmap 0 | 26 | 32 | 7 | 272 | No |
| Nested Create New stripe | 26 | 11 | 7 | 272 | Yes, flags `1` |
| Nested rank icon | 24 | -11 | 7 | 6 | No |
| Nested emblem | 26 | 12 | -6 | 44 | Yes, flags `1` |
| Nested name | 28 | 36 | 8 | 273 | No |
| Outer Create New text | 28 | 20 | 8 | 210 | No |

Original `0x2BAFA4` produces nested and outer union
`{top=28,left=-11,bottom=7,right=273}`, hence row height **21**. It examines the
definition flags, independently of runtime visibility. The oracle repeats the
union after each visibility case and gets the same rectangle, including when
the rank icon is hidden. List layout `0x24BC12` subtracts this union height from
the next row's Y coordinate.

## Saved-name color

The original updater runs RGB-to-HSV `0x1318D0`, preserves hue, overrides
saturation with the original float at `45E19C`
(`0.2083333283662796`) and value with `45E198`
(`0.7916666865348816`), then runs HSV-to-RGB `0x131A00`.
An explicit item color (flag `0x200`) bypasses this conversion and copies its
RGB values verbatim.

The oracle executes these color kernels unchanged. The name-only controlled
case starts from original fallback white and yields RGB
`(0.7916666865,0.6267361641,0.6267361641)`.
This is not a claim that ordinary saved profiles are pink: ordinary profiles
also supply appearance colors.

The appearance-color-10 case executes original `0x7F790` and `0x1A06A0` against
the authored main-menu `matg` palette. It yields background RGB
`(0.2549020052,0.4235289991,0.5607839823)` and name RGB
`(0.6267361641,0.7176591754,0.7916666865)`.
Original profile defaults `0x1A0750` set primary appearance index `10`.

These results establish source placement, visibility and color requirements.
They do not establish native full-frame pixel parity, hover animation,
emblem rendering, dialog backdrop, or aspect-ratio presentation.

## Focus opacity and position

Original row focus `0x24AC80` and unfocus `0x24AD0B` call virtual slot 5,
whose retail row vtable points to `0x22E930`. That function replaces the
animation with `0x22E89C`, then recursively gives each descendant the same
animation. Text draw `0x2538F5` reads the descendant's own scale at `+0x64`
and position at `+0x58`. It does not multiply pane/list/row opacity or add
their animation offsets to the descendant's animation offset.

The oracle executes that propagation and the original updates
`0x22E391`/`0x22E3CD`. A deliberately different parent scale `0.25` and
position `(100,200,300)` leave the descendants' focus results unchanged:

| Animation | Elapsed | Descendant opacity | X shift |
| --- | ---: | ---: | ---: |
| Focus | 60 ms | 0.65 | 8.5 |
| Focus | 120 ms | 1 | 17 |
| Unfocus | 100 ms | 0.65 | 8.5 |
| Unfocus | 200 ms | 0.3 | 0 |

All original descendant nodes, including nested name and bitmap widgets,
receive these values. The zero-elapsed update uses the original minimum
one-millisecond step, recorded explicitly in the structured report.

Original `0x22F0FF` rejects focus replacement while intro flag `1` is set.
The oracle separately checks that gate: scale `0.25` and the previous
animation target remain unchanged. This proves the gate and steady focus
values; the native regression below does not implement or certify the
entire intro-to-focus transition.

## Native regression boundary

`tests/frontend_tests.cpp::profile_pointer_geometry` creates two profiles
through the real title Start, Create New Profile, Profile Name, customization,
sign-in and sign-out flow. It then focuses the middle of the three profile
rows to exercise all original end-cap frames (`1,0,2`). The test checks the
exact authored bitmap page pixels and UV corners, and locks successive row
positions to the original 21-pixel union height.

The pointer targets the Create New stripe's right edge, beyond the previous
text-only hit rectangle. The resulting selection is passed to `MenuState`;
the painter must retain the stripe, apply the original steady focus shift of
17 pixels, and change the normalized focus opacity from `0.3` to `1`.
The emitted stripe opacity is `0.3` before focus and `1` after focus. The
previous painter multiplied the list's ended intro opacity `0.25` into each
row, reducing those values incorrectly to `0.075` and `0.25`.
The pointer stays over the row after its focus shift. Accept still opens
Profile Name, and Back restores Choose Profile.

The popup regression separately requires a title snapshot before the
`UiDraw.backdrop` boundary, and verifies that the sharp logo is absent from
the foreground vertices after that boundary. This distinguishes the required
blurred title background from the previously overlapping foreground logo.

Focused feedback is recorded in
`analysis/frontend/profile-regression/profile-alpha-red-tests.log` and
`analysis/frontend/profile-regression/profile-alpha-green-tests.log`.
This is a painter geometry/state regression. It does not establish the
120/200-millisecond focus transition, native mouse-event plumbing, GPU pixels,
or complete submenu layout/flow parity.
