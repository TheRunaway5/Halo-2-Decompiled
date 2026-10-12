# ONLINE Vista bitmap correspondence

Read-only evidence, 2026-10-10. No production code or source asset changed.
Page numbers below are zero-based bitmap image ordinals, not invented aliases.

Sources:

- Actual Xbox `assets/disc/maps/mainmenu.map`, SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Actual extracted Vista `mainmenu.map`, SHA-256
  `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`.
- Bounded [Xbox/Vista authored definition inventory](../../analysis/frontend/hd-ui-followup-20261009/vista-menu-extension-inventory.json)
  and [reader](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py).
- [Vista resource manifest](../../analysis/frontend/vista-menu-ui-20261009/manifest.json)
  and its exact DDS exports. Xbox compressed pages were inspected using the
  existing [hash-verified Xemu decoder](../../tools/oracle_frontend_bc2.py),
  `verify_decoder` and `source_payload`, without a native renderer substitute.
- Actual local `Halo 2 Vista installer/halo2.exe`, SHA-256
  `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`.
  PC function addresses below were read directly with `objdump -d -M intel`.
  The PC interface reference supplies function names/relative offsets, not
  authority to replace original bytes with mod behavior.

## ONLINE action and news bank

Full bitmap name:
`ui\screens\game_shell\xbox_live\xbox_live_main_menu\xbox_live_menu`.
Both banks have **no sequences**; there is no sprite indirection that makes
Vista page 7 valid.

| Xbox pages | Vista pages |
| --- | --- |
| 0–3: 256×256; 4–6: 4×4 placeholders; 7: 512×256 | 0–4: 512×512; 5: 1024×512 |

Xbox screen 186 has four panes. In every pane bitmap index 1 uses page 7 at
`(-23,5)` for the combined Bungie.net/news panel; bitmap index 3 uses the pane
ordinal at `(108,194)` for its action illustration. Actual Vista screen 186
retains those references, including **invalid page 7**. Its presence in the PC
cache therefore does not establish a correct PC runtime presentation.

| Xbox 186 pane | Xbox action page | Authored action | Vista evidence |
| --- | --- | --- | --- |
| 0 | 0 | Quickmatch | Vista page 0 depicts the same player selection/group diagram at twice the extent. |
| 1 | 1 | Optimatch | Vista page 1 depicts the same player/filter selection diagram at twice the extent. |
| 2 | 2 | Create Party | Vista page 2 depicts the same player/list diagram at twice the extent, although the actual PC Create Party screen uses different art, page 3. |
| 3 | 3 | Content Download | Xbox page 3 depicts an envelope/download arrow. Vista page 3 depicts Spartans grouped below a player/rank diagram. **No matching Vista download page established.** |

Pages 0→0, 1→1 and 2→2 are supported visual correspondences for preserving
Xbox authored art and geometry. They are not byte-identical images. The
fourth action cannot use ordinal 3→3 as a claim of equivalence.

The actual PC ONLINE screen is **247**, `ui\screens\game_shell\bungie_news\bungie_news`.
The PC [four-way sign-in reference](../../tools/reference/cartographer/xlive/Blam/Engine/interface/screens/screen_4way_signin.cpp)
explicitly replaces screen 186 with Bungie News; its loader is identified in
[screen_bungie_news.cpp](../../tools/reference/cartographer/xlive/Blam/Engine/interface/screens/screen_bungie_news.cpp).
The authored 247 action bitmap is index 2 at `(250,500)`:

| PC 247 pane | Authored action description | Vista action page |
| --- | --- | --- |
| 0 | Quick entry into a game | 0 |
| 1 | Specify game type / search options | 2 |
| 2 | Look for a game in the browser | 1 |
| 3 | Create Party | 3 |

Vista page 4 contains the Bungie.net logo and a lower/left grid panel. Page 5
contains an upper/right grid border. Xbox page 7 combines the logo and a
differently arranged grid panel. Neither Vista page is a complete exact
replacement for Xbox page 7; no authored screen usage of Vista pages 4/5 was
found. Do not approve 7→5 merely because the physical extent doubles.

For the native Discord/service information panel, hiding the unused original
news decoration at screen 186 bitmap 1 avoids an Xbox fallback without
fabricating page correspondence. Retaining the selected action art 0–2 is
supported; using the PC Create Party illustration for a new/native action is
an explicit presentation choice, not a global Xbox page alias.

## Virtual keyboard: third page is hover, not established press art

The five exact paths are
`ui\screens\game_shell\virtual_keyboard\button_1`, `button_2`, `button_4`,
`button_5`, `button_6`. Each Xbox bank has two pages and each Vista bank three.
There are no sequences. Their per-page physical extents are:

| Button | Xbox | Vista |
| --- | --- | --- |
| 1 | 35×30 | 70×60 |
| 2 | 70×30 | 140×60 |
| 4 | 140×30 | 280×60 |
| 5 | 170×30 | 340×60 |
| 6 | 210×30 | 420×60 |

The actual PC executable's base button getter is VA **`0x63FF21`** (RVA
`0x23FF21`, image base `0x400000`). It resolves the bitmap bank, obtains its
image count, then performs this decision in order:

```text
if widget belongs to focus slot 0 and image_count > 1: return 1
if widget belongs to focus slot 1 and image_count > 2: return 2
return 0
```

The membership helper at VA `0x612152` calls focus lookup `0x609EFD`, compares
the returned widget and its ancestor chain with the requested widget, and
returns the result. The source
[channel layout](../../tools/reference/cartographer/xlive/Blam/Engine/interface/user_interface_channel.h)
names slot 0 **selected**, slot 1 **hovered**, with the latter absent in Xbox.
The getter calls and their bank-count guards are directly present in the
executable, not inferred from how brightly a page looks.

The virtual keyboard override at VA **`0x63ADB1`** (RVA `0x23ADB1`) checks
keys `0x29`–`0x2c` against the keyboard latch flags and returns 1 when latched;
otherwise it calls `0x63FF21`. The equivalent Xbox logic is recovered at
`c_keyboard_key_widget::v17`, retail `0x2377f6`, in
[unknown_23760b.cpp](../../tools/reference/halo2-decompiled/src/unknown_23760b.cpp).
The PC reference names both getters in
[screen_virtual_keyboard.cpp](../../tools/reference/cartographer/xlive/Blam/Engine/interface/screens/screen_virtual_keyboard.cpp)
and [user_interface_widget_button.cpp](../../tools/reference/cartographer/xlive/Blam/Engine/interface/user_interface_widget_button.cpp).

Thus **0 = normal, 1 = selected/latched, 2 = mouse hover**. Selection wins
when both focus slots contain the same button. No separate pressed-page
semantics were established. Xbox normal 0 and selected/latched 1 can use
Vista source pages 0 and 1 at density 2; page 2 should only be selected by an
actual separate mouse-hover owner if that PC behavior is deliberately added.
It must not be used as a guessed Xbox selected or pressed page.

## Queue screen 210 waiting-pane decoration

The native queue currently chooses screen 210 pane 1. In the Xbox authored
pane bitmap 0 is
`ui\screens\game_shell\settings_screen\variant_settings\quick_options_display`,
page 0, **512×256**, positioned `(-272,10)`. The decoded page is a blue
rectangular grid panel. It contains no score/rank/name text; those would be
independent authored text widgets. The page is decorative backing, not a
working score or ranking feature.

That bitmap tag is **absent** from the actual Vista cache. The PC authored
screen 210 waiting pane instead places
`ui\screens\game_shell\xbox_live\matchmaking_progress_dialog\matchmaking`,
page **1**, at `(-320,100)`; that page is **1024×512** virtual-couch artwork.
Its pane 0 uses the same bank page 0 (lying/sniper Spartan), and pane 2 uses
page 2 (combat Spartan group). Xbox's bank already contains those three image
subjects in that same order, although Xbox waiting pane 1 uses the separate
grid panel instead.

Therefore `quick_options_display`→`matchmaking/1` is not a pixel-equivalent
texture alias. It changes both illustration and authored placement. For the
native live queue status, hiding unused pane 1 bitmap 0 is supported by its
decorative role and avoids a fallback; deliberately adopting the complete PC
waiting-pane illustration/layout is a different, explicit UI policy.

## Observed native integration policy

The implementation inspected during this audit, in
[frontend_painter.cpp](../../src/frontend_painter.cpp) and
[frontend_vista.cpp](../../src/frontend_vista.cpp), uses the following explicit
policies. These observations describe the native port, not an assertion that
Xbox and PC screen constructors were identical:

- ONLINE draws the actual Vista **screen 5** shell background. Its authored
  definition has 18 bitmap widgets, compared with Xbox's 13. It includes Vista
  `framing_center` at `(-1070,654)` with scale approximately 1.078, and
  `track7a` at `(-1100,-284)` with scale approximately 1.1. Loading that PC
  definition preserves its framing and track placement as a unit.
- The native action menu uses the semantic actions of actual Vista screen
  **247**: Quickmatch page **0**, Optimatch page **2**, Create Party page **3**,
  server browser page **1**. This is a deliberate action mapping. It does not
  assert Xbox screen 186's download page 3 equals Vista page 3. The Discord
  account action has no established PC action icon, so its illustration is
  hidden. Screen 186 bitmap 1, the obsolete combined news decoration using
  absent Vista page 7, is also hidden.
- `fs_dialog` and default-list `arrows` retain actual Vista source pages in
  their authored ordinal order. Both banks have two pages and no sequences.
  Vista `fs_dialog/0` is the dialog body, `fs_dialog/1` the narrow blue strip;
  the authored Xbox and Vista definitions both reference those ordinals.
  Vista arrow page **0** points up and page **1** points down. The source
  manifest records their extents as **2048×1025**, **514×64**, and **52×34**
  for each arrow, respectively. These source extents/padding are retained;
  they are not claimed to be exact integer-density copies of Xbox pixels.
- The pregame lobby uses the actual Vista `lobby_display/0` source page
  (**1024×356**, registration512×178). Both authored caches reference page0
  with no sequences. The PC padding differs from the Xbox bank, so the page
  is retained directly instead of passing the equal-density pack filter.
- Pregame `mp_games2` keeps the11 authored game-type pages in source order.
  Actual Xbox40×40 and Vista80×80 pages were visually compared, including
  the player, crown, lightning, skull, sun, flame, flag, bomb, diamonds,
  three-star pennant and question mark. No page alias or invented icon is used.
- Pregame `settings_framing/0` uses the actual Vista346×4 strip. Pregame
  `button` retains Vista358×64 normal/selected pages0/1, with the same
  base-widget selected/hover getter documented above; page2 is not substituted.
- Keyboard source pages **0/1** retain the normal and selected/latched states
  established by the original executable above. The additional PC hover page
  is not substituted for selection.
- Queue screen 210's unbound `quick_options_display` decorative grid is
  hidden rather than assigned unrelated PC virtual-couch artwork.

The parent implementation is auditing the used ONLINE bitmap/font material
bytes against the Vista source and exported HD donor. This research note does
not itself certify the final strict fallback check or complete runtime render.

Limits: the bitmap observations establish image subjects and authored
references; they do not prove original PC full-frame layout or live screen
construction. No original game was launched or modified for this audit.
