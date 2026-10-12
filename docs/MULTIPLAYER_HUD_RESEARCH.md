# Original Xbox multiplayer HUD

The first-person multiplayer HUD is authored in `nhdt` tags selected from the
player biped, current weapon, and multiplayer globals. Lockout contains 28
`nhdt`, one `hudg`, and no `chdt` or `wphi`. The latter groups should not be
invented as Halo 2 Xbox selectors. The original shader programs are part of the
HUD content; replacing their output with a uniform tint or rectangular meter
does not reproduce the shipped HUD.

Authority: `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`;
disc caches build `02.09.27.09809`, version 8;
[decompiled reference](../tools/reference/halo2-decompiled) commit
`5f08630655e3ddc9ac45909090f11ae799fa0c27`;
[Assembly](../tools/reference/assembly) commit
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`. Raw original cache values are in
[authored-hud-lockout.json](../analysis/multiplayer/authored-hud-lockout.json).
The 41 combined bitmap/text rows in Spartan, SMG and scoreboard match across
all twelve disc multiplayer maps after omitting cache-specific datum/offset
keys, including their state masks, layouts, selectors and effect bytes.
[Map comparison](../analysis/multiplayer/hud-map-comparison.json) does not compare
bitmap pixels or shader programs between maps.

## Reproduce the inspection

```sh
.tools/python/bin/python tools/inspect_multiplayer_hud.py \
  assets/disc/maps/lockout.map \
  --output analysis/multiplayer/authored-hud-lockout.json
.tools/python/bin/python tools/oracle_multiplayer_hud.py \
  --output analysis/multiplayer/hud-instruction-oracle.json
```

[inspect_multiplayer_hud.py](../tools/inspect_multiplayer_hud.py) reads all
`nhdt` rows, bitmap sequence/image metadata, shader postprocess constants and
overlays, and template/pass programs, texture bindings, render states and
externs. [oracle_multiplayer_hud.py](../tools/oracle_multiplayer_hud.py) executes
original viewport, anchor, visibility, input-selection, scoreboard-transition,
shield-history and shield-special instructions in Unicorn. The center test reports no active player, disabling
the additional first-person projection correction. It stops before graphics
APIs. [Instruction results](../analysis/multiplayer/hud-instruction-oracle.json)
are CPU evidence, not a live Xbox/framebuffer comparison.

## Selection and drawing order

`matg+0x110` is the Interface Tags block, stride `0x98`. Interface reference 6
is the `hudg` datum at first-row `+0x34`. Original `0x13E615` indexes the eight-byte
references; `hud.cpp:63` selects index 6. Lockout resolves to
`e7f50670 ui\hud\default`.
[Interface source](../tools/reference/halo2-decompiled/src/interface.cpp#L63),
[HUD initialization](../tools/reference/halo2-decompiled/src/hud.cpp#L63),
[matg schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml#L475).

The multiplayer Spartan representation remains `matg` player-representation
index 2; see [multiplayer research](MULTIPLAYER_RESEARCH.md). That biped's
`+0x1A0` New HUD Interfaces block has stride 8, datum `+4`. Original
`0x13992A:0x139AEB` tests `options.state==2`, then `0x22BFC0` selects
`min(multiplayer ? 1 : 0, count-1)`. An empty block returns `NONE`. Spartan MP's
two entries both resolve to `e9cd0848 ui\hud\masterchief`. This is the exact
selection path; a cache name lookup alone would skip the mode selector.
[Biped schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/bipd.xml),
[selector source](../tools/reference/halo2-decompiled/src/unknown_22bfc0.cpp#L53),
[original selection instructions](../analysis/multiplayer/hud-13992a.txt).

Weapon HUD datum is `weap+0x2B4`, confirmed by original `0x13AFA9`; SMG resolves
to `e590041b ui\hud\smg`. Scoreboard HUD selection follows `matg+0x16C` datum
to `mulg`, Runtime block `mulg+8`, stride `0x568`, then first-row datum `+0x544`.
Original `0x139E87` reads that field. Lockout resolves to
`f2d1114c ui\hud\scoreboard`.
[Weapon source](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L881),
[weap schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/weap.xml#L449),
[mulg schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/mulg.xml#L1419).

Original `0x1394B9` dispatches HUD definitions in this order: backpack type 3,
vehicle weapon type 4, primary type 1, secondary type 2, backpack type 3 again,
scoreboard type 0, unit type 0, parent type 0. The duplicate backpack dispatch
is present in the instructions. Within each definition `0x13A532` visits
screen-effect rows first, bitmap rows next, text rows last, preserving row
order. It computes state once for that weapon type, tests each row's masks,
computes four selected inputs, and dispatches the corresponding renderer.
[Drawing order](../analysis/multiplayer/hud-1394b9.txt),
[row dispatch](../analysis/multiplayer/hud-13a532.txt).

## Compact layout and state conditions

The [nhdt schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/nhdt.xml)
has root size `0x28`: bitmap block `+8` stride `0x64`, text block `+0x10` stride
`0x54`, low clip/ammo cutoffs `i16+0x18/+0x1A`, low age cutoff `f32+0x1C`,
screen effects block `+0x20` stride `0x50`. The root `+0` tag reference is null
for Spartan, SMG and scoreboard; this research does not infer inheritance for
other tags.

Common widget fields are name StringID `+0`, input selector bytes `+4..7`,
four required masks `u16+8..0xE` and four excluded masks `u16+0x10..0x16`,
age/clip/reserve cutoff bytes `+0x18..0x1A`, anchor `u16+0x1C`, flags `+0x1E`.
The four mask groups are unit, aim-target extras, weapon, game engine.
Original `0x13AC87` requires **any** required bit in any group and **no**
excluded bit in any group. All-zero required masks do not draw. The cutoff
bytes are loaded unsigned in the instructions, despite signed XML labels.
[Original conditions](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L714).

Unit masks include default `1`, grenade none/frag/plasma `2/4/8`, single/dual
wield `0x10/0x20`, unzoomed/zoom levels `0x40/0x80/0x100`, grenades disabled
`0x200`, binoculars `0x400`, motion sensor `0x800`, shield enabled `0x1000`,
Elite `0x2000`. Weapon masks include primary/secondary/backpack `1/2/4`,
age/clip/reserve below cutoff `8/0x10/0x20`, overheated `0x40`, out of ammo
`0x80`, target/locking/locked `0x100/0x200/0x400`. Engine masks include solo/co-op
`1/2`, FFA/team `4/8`, leading/not-leading `0x10/0x20`, timed/untimed
`0x40/0x80`, other-score-valid/invalid `0x100/0x200`, bomb arming `0x400`, talking
`0x800`. Original builder is `0x13A050`.
[Builder source](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L327).

Bitmap-specific fields: bitmap datum `+0x24`, shader datum `+0x2C`, signed full/
half/quarter sequence bytes `+0x30..32`, three signed point16 offsets `+0x34`,
`+0x38`, `+0x3C`, three float2 registration fractions `+0x40/+0x48/+0x50`,
effect block `+0x58`, special type `+0x60`. Negative sequence means absent.
Flags bits 0/1 flip UVs; bits 2/3 mirror scope geometry; bit 4 stretches scope
geometry. Special types 1/2 are player/other emblem, 3/4 player/other score
meter, 5 unit shield, 6 motion sensor, 7 territories.

Text-specific fields: shader datum `+0x24`, text/function StringID `+0x28`,
justification `+0x2C` (left/center/right 0/1/2), font bytes `+0x30..32`, authored
scale floats `+0x34..3C`, offsets `+0x40/+0x44/+0x48`, effect block `+0x4C`.
Number flag bit 0 evaluates the StringID as a function; bits 1/2 clamp/pad to
two/three digits. Bit 3 substitutes the talking player's name. Normal strings
resolve through `hudg` HUD text, then the original parser. The original text
draw routine `0x22AA16` does **not** read the scale floats (these are zero in
the three inspected HUDs); treating zero as hidden is incorrect.
[Text preparation](../tools/reference/halo2-decompiled/src/unknown_22a648.cpp#L68),
[text draw instructions](../analysis/multiplayer/hud-22aa16.txt).

## Anchors, margins and split screen

The XML anchor labels describe categories rather than their actual corner
positions. Original `0x1396C7` reads the current safe rectangle
`g_4B9DD8={top,left,bottom,right}` and returns:

| Anchor | Actual base position |
| --- | --- |
| 0 | `(right, top)` |
| 1 | `(left, top)` |
| 2 | `(left, bottom)` |
| 3 | `(right, bottom)` |
| 4 | integer center, plus the first-person projection correction when available |
| 5 | projected lock target; offscreen fallback uses original negative sentinel |

[Anchor instructions](../analysis/multiplayer/hud-1396c7.txt). The supplied
oracle rectangle `(T24,L32,B456,R608)` returns `(608,24),(32,24),(32,456),
(608,456),(320,240)` for anchors 0..4 without player projection.

Anchor 4 starts from the integer safe-rectangle center. When the current local
player has a unit and `0x155760` returns zero (the first-person path), original
`0x139814..0x1398BC` constructs the world point
`camera.origin + camera.forward - observer[0x48] * camera.up`, transforms it by
`g_4B9E14` (`0x142570`), and projects it with camera `g_4B9DA0` (`0x30710`). A
valid projection replaces **only Y**; X stays at the safe-rectangle center.
It finally rounds each coordinate using `floor(value+0.5)`. The active-player
projection must use that camera/observer value rather than a guessed vertical
percentage. The oracle's center case intentionally does not exercise it.

Layout slot selection `0x13A690` compares current outer viewport width/height
with **640/480**: slot 0 if both meet the thresholds, slot 2 if both are smaller,
slot 1 if exactly one is smaller. For slot 1 only, anchor 3 may be promoted to
slot 2 by original `0x16A90` display flag. The helper can suppress that flag
for options state 3 with valid replay settings. This rule is about the original
logical viewport, not the modern output framebuffer's resolution.
[Layout source](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L543).

Original rasterizer initialization `0x12560` calls `0x16A30` with scale 1 and
margins X=48/Y=36; if both `g_485AC5` and `g_485AC6` are true it uses X=32/Y=24.
Default full outer rectangle is `(T0,L0,B480,R640)` and safe rectangle is
`(36,48,444,592)`. This evidence establishes the flag-conditioned margin rule;
this research does not relabel the two flags without further display-mode
tracing. Grid helper `0x2F6B0` divides the safe rectangle with integer arithmetic,
restores outer edges to the full rectangle at the screen edges, and adds four
pixels to internal left/top safe edges or subtracts four from internal
right/bottom safe edges.
[Initialization](../analysis/multiplayer/hud-12560.txt),
[margin/grid source](../tools/reference/halo2-decompiled/src/unknown_0167a0.cpp#L151),
[grid instructions](../analysis/multiplayer/hud-2f6b0.txt).

| Default grid/cell | Outer `(T,L,B,R)` | Safe `(T,L,B,R)` |
| --- | --- | --- |
| 1×2 top | `(0,0,240,640)` | `(36,48,236,592)` |
| 1×2 bottom | `(240,0,480,640)` | `(244,48,444,592)` |
| 2×2 top left | `(0,0,240,320)` | `(36,48,236,316)` |
| 2×2 top right | `(0,320,240,640)` | `(36,324,236,592)` |
| 2×2 bottom left | `(240,0,480,320)` | `(244,48,444,316)` |
| 2×2 bottom right | `(240,320,480,640)` | `(244,324,444,592)` |

Bitmap `0x22AE21` selects the layout's sequence, then its first sprite/image.
When that sequence has more than one entry, Elite appearance selects entry 1;
Spartan selects entry 0. Sprite UV rectangle comes from the sequence; ordinary
image sequences use UV `[0,1]×[0,1]`. Missing sequences plus selected sequence
0 can use image 0. `0x22AFF1` adds the selected widget offset to the anchor,
constructs the registration point inside the UV rectangle, multiplies UV extents
by actual bitmap pixel dimensions, and rounds the near/far edge terms. It
starts with unit-length screen axes; widget effects apply rotation, offset,
then scale. Original bitmap header registration points are not substituted for
the `nhdt` registration fractions.
[Bitmap selection](../tools/reference/halo2-decompiled/src/unknown_22ae21.cpp#L67),
[geometry instructions](../analysis/multiplayer/hud-22aff1.txt),
[effect order](../tools/reference/halo2-decompiled/src/unknown_22a648.cpp#L209).

## Authored Spartan and SMG widgets

Full/half/quarter arrays below are the actual Lockout data, not reconstructed
positions. All listed widgets have no transform effects and flags 0.

| Widget | Anchor | Offsets full / half / quarter | Sequences full / half / quarter | Bitmap |
| --- | --- | --- | --- | --- |
| SMG background right | 0 | `(-103,0)` all | `0/0/0` | `smg_bkd` |
| SMG magazine meter | 0 | `(-98,21)` all | `0/0/-1` | `smg_meter` |
| SMG reticle | 4 | `(0,0)` all | `0/23/23` | `hud_reticles` |
| Frag selected background | 1 | `(0,0)` all | `0/0/0` | `grenade_bkd` |
| Plasma selected background | 1 | `(0,0)` all | `1/1/1` | `grenade_bkd` |
| Shield meter | 2 | `(-1,-98)/(-1,-76)/(-1,-76)` | `0/2/2` | `shield_meter` |
| Shield mask | 2 | `(-1,-99)/(-1,-76)/(-1,-76)` | `1/3/3` | `shield_meter` |
| Radar background | 2 | `(-1,-84)/(-1,-63)/(-1,-63)` | `0/1/1` | `motion_sensor` |

SMG reticle registration is `(0.5,0.5)` for all slots; all other listed
registration fractions are `(0,0)`. Full SMG reticle image 0 is 55×55;
half/quarter image 23 is 27×27. SMG backgrounds are 103×19, magazine meter
98×20. Full shield image 0 is 128×32; split image 4 is 64×32. Radar images are
83×83 full and 64×64 split. These are shipped pixels, including transparent
padding, not procedural silhouettes. Sequence entry variants account for
Spartan/Elite appearances.

SMG reserve count text uses selector 0x33 in input 2, StringID `hud_input2`,
number flag 1, centered at anchor 0 + `(-82,-1)` in all slots. Frag and plasma
counts use selectors `0x14/0x15` in input 1, StringID `hud_input1`, centered at
anchor 1 + `(65,-1)` / `(12,-1)`. All these text widgets use font type 0.
`0x22A85B` maps type 0 to retail font index **6**, type 1 to index **8**;
[font_table.txt](../assets/disc/maps/fonts/font_table.txt) identifies these as
`conduit-13` and `MSLCD-14`. Authored font glyphs and metrics can use the
existing native font decoder.

Text placement `0x22AA16` uses the selected anchor plus signed offset, applies
the widget effect to that point, then subtracts the current outer viewport's
left/top. It measures the **ink rectangle** through `0x13E9C0/0x13EA60`, with
an initial rectangle `(T0,L0,B1000,R1000)` and scale 1. The glyph-bounds callback
`0x13E8E0` extends each glyph's far edge by one pixel. Thus the placement width
is `ink.right-ink.left`, not simply the sum of advances. With that integer
width `W`, justification 0 uses left `trunc(X)`, 1 uses left
`trunc(X-floor(W/2))`, and 2 uses left `trunc(X-W)`. Its top is `trunc(Y)` and
its bottom is `trunc(Y+inkHeight)`. Centered right is
`trunc(X+W-floor(W/2))`; the other two use `trunc(X+W)`/`trunc(X)`.

The offset therefore specifies the **text rectangle top**, not its baseline.
Original `0x13F0E0:0x13F258..0x13F2BF` sets first-line baseline to
`rect.top + font.ascending_height` at scale 1; subsequent lines add
`ascending+descending+leading+line_gap`. `0x13F700` places a glyph at
`penX+bearingX+kerning, baseline-bearingY` and advances the pen by
`advance+bearingX+kerning`. The native frontend's existing advance-only pen
update is not equivalent for nonzero bearing X. `conduit-13` has ascent 16,
descent 4, leading 0; its digit glyphs have bearing X 1, bearing Y 16, width 7,
height 21 and advance 8. `MSLCD-14` has ascent 15, descent 0, leading 0. Text
rendering still uses scale 1 after effects: effect axes can influence the
offset transformation, but `0x22AA16` does not consume their final rotation/
scale to rotate or resize glyphs.
[Text bounds source](../tools/reference/halo2-decompiled/src/unknown_13e8a0.cpp#L29),
[baseline instructions](../analysis/multiplayer/hud-13f0e0.txt),
[bounds instructions](../analysis/multiplayer/hud-13ea60.txt).

## Inputs and shader bindings

Original `0x13AD48` resolves four selector bytes. Relevant values are shield
`0x10`, body `0x11`, target-autoaimed boolean `0x12`, frag/plasma counts
`0x14/0x15`, time with shield below .2 `0x16`, clip count `0x30`, weapon heat
`0x31`, battery percentage `0x32`, **unloaded reserve** `0x33`, clip fraction
`0x36`, battery fraction `0x38`. Score inputs are **0x40 user score fraction,
0x41 other score fraction, 0x42 user winning amount, 0x43 bomb arming**. Assembly
labels for score inputs are shifted by one and must not override the executable.
Authored scoreboard rows use 64/65/66, corroborating the instructions.
[Input source](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L279).

Input 66 is the user state float at `g_510C4C + user*0x6C + 0x28`, initially
zero after new-map initialization. Original `0x13A838:0x13A874..0x13A8EF`
updates it only while `status[0x130]` is true. Target is 1 when
`status[0x133]` is true (leading), otherwise 0. Each update approaches the
target by **2/ticksPerSecond**, clamped to `[0,1]`; at 30 Hz the step is 1/15
and a complete transition takes half a second. Disabled score HUD leaves the
value unchanged. The oracle executes the untouched update instructions for
rising/falling, both clamps, and disabled state. Authored score-meter
offset effects use this input to move the player meter down 18 pixels and
the other meter up 18 pixels; binary values would remove the shipped motion.
[Transition instructions](../analysis/multiplayer/hud-13a838.txt),
[original oracle](../analysis/multiplayer/hud-instruction-oracle.json).

The renderer copies these four resolved inputs to `g_4E6950+0x30..0x3C`.
Shader-animation callback `0x22ACE7` maps StringIDs `0x0A0005CF..0x0A0005D2`
(`hud_input1..4`) to these four values. `hud_elite` StringID `0x090005CE` returns
appearance selection. **Scalar extern bank 40 is global HUD fade**, fetched
from per-user `g_4E69C0`, with 1 for invalid user. It is not input 1.
[Animation callback](../tools/reference/halo2-decompiled/src/unknown_22ace7.cpp#L47),
[scalar extern source](../tools/reference/halo2-decompiled/src/unknown_0350e0.cpp#L1027).

Vector extern banks 36..39 supply custom colors 1..4 at `g_4E6950+0x40,+0x4C,
+0x58,+0x64`. HUD bitmap passes bind texture extern 16, whereas text binds
font bitmap extern 15. The widget `shad` Parameters block is stripped/empty;
real values are in postprocess constants, overlays, remaps and the referenced
`stem`/`spas` programs. Packed constant words are not necessarily final tint
colors. Ordinary flash/gradient/ammo passes have authored blend values
source `0x302`, destination `0x303`; shield uses source 1, destination `0x302`.
Native rendering must bind the authored combiner program and those states.
[Vector extern source](../tools/reference/halo2-decompiled/src/unknown_033a0b.cpp#L224),
[shad schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/shad.xml),
[raw pass metadata](../analysis/multiplayer/authored-hud-lockout.json).

Score-meter special 3 copies the player's status RGB at `status+0x18C` to
**both custom colors 1 and 2 (externs 36 and 37)**; special 4 copies the other
player's RGB at `status+0x1D8` to the same two banks. This is distinct from
shield special's banks 37/38. The authored scoreboard meter selects inputs
`[64,66,0,0]` for player score and `[65,66,0,0]` for other score. Normal
scoreboard text comes from the HUD string reference at `hudg+0x3F8` (datum
`+0x3FC`), including `scoreboard_playerscore`, `scoreboard_otherplayerscore`,
`scoreboard_variantname`, and `scoreboard_timeleft`. The original parser resolves
the score/name/time substitutions; the string is not a generated HUD label.
[Special-type instructions](../analysis/multiplayer/hud-22aff1.txt).

Shield special type 5 executes the original branch at `0x22B3CB`. For layers
0..4, define `current=clamp(unitShield-layer,0,1)` and
`previous=clamp(user.value00-layer,0,1)`. Input 1 becomes **previous**, input 2
**current**. If previous exceeds current, custom color 1 is white times
`clamp(1-user.value04,0,1)` while that pulse is nonnegative. A negative pulse
sets it to black; a layer with no damage forces pulse -1 and also uses black.
The branch draws only when
either fraction is positive. Spartan layer 0 sets custom color 2 from packed
RGB `0x000055AA`, custom color 3 from `0x000078F0`; Elite uses
`0x0045059A/0x009C46C1`. For layers 1..4 it sets both to red `FF0000`, green
`00FF00`, yellow `FFFF00`, purple `7F00FF`. The shield shader's authored
animation curves then remap the fractions. The instruction oracle verifies
ordinary full shield, damage from 1 to .5 with pulse .25, empty shield, and
multi-layer shield; those cases must not be reduced to clipping a blue quad.
[Shield branch](../analysis/multiplayer/hud-22aff1.txt),
[instruction oracle](../analysis/multiplayer/hud-instruction-oracle.json).

The same `0x13A838` HUD update stores shield history in each user's
`+0x00` previous shield, `+0x04` pulse and `+0x08` last tick. New-map values
are `-1,-1,NONE`. The original `0x13A93D..0x13A9D8` compares previous with
unit runtime shield at `+0xF0`. If previous exceeds current, it resets last tick
to now only when pulse is outside `[0,1]`. It computes
`elapsed=(now-lastTick)*secondsPerTick`; while elapsed is below .5 seconds,
it holds previous, writes pulse 0, and leaves last tick unchanged. At .5 or
later it copies current to previous, adds elapsed to pulse, and stores now as
last tick. If current increases, it immediately copies current to previous,
writes pulse -1, and stores now. With equality it increments a positive pulse
by elapsed and stores now; nonpositive pulse stays unchanged. This is a held
damage interval, not an interpolated shield fraction. The shield-history oracle
verifies first damage, the half-second boundary, recharge, and a further hit
while the old shield amount is held.

Multiplayer overshield drain also calls `0x13A6E8` to subtract the drain from
previous shield directly, preventing that intentional drain from appearing as
the held damage difference. Its only recorded direct caller is per-tick damage
update `0xD5DE0`.
[History instructions](../analysis/multiplayer/hud-13a838.txt),
[drain source](../tools/reference/halo2-decompiled/src/damage.cpp#L3537),
[previous-shield adjustment](../tools/reference/halo2-decompiled/src/unknown_139296.cpp#L63).

## Implementation boundaries

This report establishes selectors, row layouts, state masks, authored image
choices, font selection, logical margins, input banks, draw ordering, and
shield-special inputs and the score transition. It supplies the original pixel programs and animation
curves for the existing native shader/bitmap/font pipeline. CPU-generated
screen quads can reproduce the original geometry arithmetic where stripped
HUD vertex-program metadata is unavailable, while retaining authored pixel
programs. The original projected first-person reticle correction, lock targets,
motion-sensor contacts, kill-feed parsing, voice indicators, HUD sound timing,
shield history/pulse progression, all weapon variants and visible framebuffer
parity need their own implementation and verification. The research tools do
not claim any of those runtime surfaces are complete.

## Split-screen placement instruction oracle, 2026-10-07

The executable, rather than a reconstruction of the native layout, is the
authority for the positioning comparison. The new
[position oracle](../tools/oracle_multiplayer_hud_positions.py) executes the
untouched `default.xbe` with SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It copies real Lockout `nhdt` widget rows, `bitm` headers, image dimensions,
sequence/sprite rows, and effect-function data into the emulator. The only
changes to those copied blocks are pointer relocation. It runs the original
bitmap selector `0x22AE21`, anchor selector `0x1396C7`, layout selector
`0x13A690`, and bitmap geometry `0x22AFF1` through `0x22B299`. Thus the
reported vertices include the original registration and edge rounding. For
`player_arrow`, the authored effect also runs original `0x22BCAF`,
`0x22ACE7`, `0x13B390`, and `0x13BB40` with winning input 0. There is no
stub for bitmap selection, grid construction, geometry, rounding, or effect
evaluation, and no native implementation is imported.

The supplied environment is an ordinary NTSC gameplay session without a
playback override, Spartan appearance, zero widget inputs, and no active local
player for center projection. Hooks supply only the two Xbox video query
results, the Spartan appearance response, and the absence of an active player.
The result is a CPU geometry oracle, not proof of a rendered Xbox frame.

Original `0x123B0` stores `XGetVideoFlags() & 1` in byte `0x485AC2`.
It stores video-flags bit 3 in both `0x485AC5` and `0x485AC6`. The branch
at `0x12578..0x125B3` chooses margins `(48,36)` unless both latter bytes
are set; then it chooses `(32,24)` and calls original `0x16A30`. Widescreen
alone therefore retains the default margins. The oracle executes this branch
with video-flags inputs 0, 1, 8, and 9 and records its resulting rectangles.

Original `0x2234E0..0x223585` chooses grid mode 0 for one view, mode 1
for multiple standard-aspect views, and mode 2 for multiple widescreen views,
in the supplied gameplay environment. Original `0x2F600`, `0x2F640`,
and `0x2F6B0` then produce the cells, including the spanning first view for
three players. For default margins, the original outer rectangles are:

| Players | Standard aspect outer `(T,L,B,R)` in view order | Widescreen outer `(T,L,B,R)` in view order |
| --- | --- | --- |
| 1 | `(0,0,480,640)` | `(0,0,480,640)` |
| 2 | `(0,0,240,640)`, `(240,0,480,640)` | `(0,0,480,320)`, `(0,320,480,640)` |
| 3 | `(0,0,240,640)`, `(240,0,480,320)`, `(240,320,480,640)` | `(0,0,480,320)`, `(0,320,240,640)`, `(240,320,480,640)` |
| 4 | `(0,0,240,320)`, `(0,320,240,640)`, `(240,0,480,320)`, `(240,320,480,640)` | Same four quadrants |

The safe grid partitions the full safe rectangle, restores full outer-screen
edges in the outer rectangle, and uses four logical pixels of inset at internal
boundaries. Individual views therefore do not have identical margins relative
to their outer cell. Original `0x13A690` also promotes anchor 3 from layout
slot 1 to slot 2 in widescreen, independently of the other anchors. These are
separate source-defined causes of different placements and sequence choices
between split-screen views.

The report contains 320 cases: eight named real bitmap rows in every 1-, 2-,
3-, and 4-player cell, for all four video flag inputs. The rows are SMG
`weapon_background_right`, `ammo_meter_right`, and `crosshair`; Spartan
`no_grenades`, `shield_meter`, `shield_mask`, and the first
`motion_tracker_background` row (sensor enabled); and scoreboard
`player_arrow`. The
[JSON](../analysis/multiplayer/hud-position-instruction-oracle.json) records
exact quad vertices, bounds, selected sequence and image, grid/span/position,
safe/outer rectangles, and the supplied environment. The
[TSV](../analysis/multiplayer/hud-position-instruction-oracle.tsv) contains the
160 nonprogressive cases for direct native-test ingestion. A hidden sequence
`-1` has `valid=false` and zero bounds in the JSON, with no quad. `valid`
means bitmap selection succeeded, not that gameplay visibility predicates
show the widget.

For example, the original two-player standard-aspect top view yields SMG
background bounds `(489,36,592,55)`, shield bounds `(47,160,111,192)`, and
radar bounds `(47,173,111,237)` in logical `(L,T,R,B)` coordinates. Its
bottom view yields `(489,244,592,263)`, `(47,368,111,400)`, and
`(47,381,111,445)`. A default four-player upper-left view uses SMG
background `(213,36,316,55)` and hides its SMG ammo meter (`sequence=-1`).
These values come directly from original instructions, including transparent
bitmap padding; they are not estimated from a screenshot.

Reproduce the artifacts with:

```sh
.tools/python/bin/python tools/oracle_multiplayer_hud_positions.py
```

The same command exports a
[retail instruction listing](../analysis/multiplayer/hud-position-retail-instructions.txt)
for the executed routines and their caller boundaries. Explicit exclusions
are gameplay visibility-state evaluation, active-player center projection,
special-type shading, scope expansion, UV flips, font/text placement, final
device scaling, and final framebuffer comparison. Those exclusions prevent
the oracle from being treated as evidence for surfaces it does not execute.

## Numeric HUD text placement instruction oracle, 2026-10-07

The separate
[numeric text oracle](../tools/oracle_multiplayer_hud_text.py) extends the
placement evidence to actual reserve-ammo and empty-grenade counter text. It
does not change the bitmap oracle or its fixtures. It supplies the raw
[conduit-13](../assets/disc/maps/fonts/conduit-13) header and digit glyph
headers in a ready font cache. Only cache lookup `0x140B20` and pixel
readiness `0x140870` are hooked; they supply the real cached glyph records
and readiness, without calculating measurement or positions. Original
`font_get` (`0x1224C0`), kerning (`0x122570`), font-cache access
`0x1407D0/0x140900`, string iteration, measurement, and glyph geometry
still execute from the pinned original XBE.

The actual digit `0/1/2` glyph records each have advance 8, bearing X 1,
bearing Y 16, width 7, and height 21. The real font header has ascending
height 16; its real kerning table has no pairs for these digits. The original
`0x13EA60` calls the full original `0x13F0E0` text layout, `0x13EF40`
measurement, and `0x13F700` glyph placement with original `0x13E8E0`
ink-bounds callbacks. The resulting ink rectangle `(T,L,B,R)` is
`(0,1,22,27)` for `120`, and `(0,1,22,9)` for `0`. Thus measured ink
width is **26** for three digits and **8** for one digit, including the
callback's one-pixel far-edge extension and excluding the initial bearing.

For each real widget row, untouched `0x22AAE3..0x22AC98` then runs the
original anchor, selected offset/effects, subtraction of the current outer
viewport origin, UTF-16 conversion and measurement (`0x13E9C0`), and
justification-rectangle construction. This stops before the graphics draw.
The test invokes original `0x13F0E0` with that rectangle and the same
original ink callback to record final glyph geometry. Text value formatting,
font/shader setup, visibility predicates, glyph pixel decoding, and graphics
remain outside this test; numeric strings `120/0` and already-selected font
index 6 are explicit supplied inputs.

The original centered text rectangle uses `sar 1` on measured width,
subtracts that half-width from the **viewport-local** anchor, and truncates
the resulting left/top to integers. `0x13F0E0` places the first baseline at
rectangle top + ascending height. `0x13F700` adds bearing X to each glyph,
subtracts bearing Y from the baseline, and advances by the sum of advance,
bearing X, and kerning. With the actual measured widths, drawable `120` glyph bounds
relative to the local anchor are `(-12,0,13,21)` and drawable `0` bounds
are `(-3,0,4,21)`, expressed as `(L,T,R,B)`. The callback's extended ink
rectangle is a measurement value, not an extra rendered glyph pixel.

The
[text JSON](../analysis/multiplayer/hud-text-instruction-oracle.json) records
120 cases for video flags 0/1/8/9, all local-player cells, and actual rows
`bullets_right` with text `120`, `frag_hidden` with `0`, and
`plasma_hidden` with `0`. It records local anchors, measured ink, local
text rectangles, per-glyph callback positions, and global logical bounds.
Global bounds only add the original outer viewport's origin to the callback
positions; they do not recompute glyph placement. The
[text TSV](../analysis/multiplayer/hud-text-instruction-oracle.tsv) contains
the 60 default nonprogressive cases for direct native comparison. For example,
two-player standard-aspect reserve glyph bounds are `(498,35,523,56)` for
the top view and `(498,243,523,264)` for the bottom view. The lower view's
original local text rectangle is `(T3,L497,B25,R523)`.

Reproduce the report and its
[original instruction listing](../analysis/multiplayer/hud-text-retail-instructions.txt)
with:

```sh
.tools/python/bin/python tools/oracle_multiplayer_hud_text.py
```

These are original-instruction measurements of supplied numeric text and raw
font metadata. They do not prove glyph pixel decoding, color/shader output,
other strings/justifications/fonts, final device scaling, or visible Xbox
framebuffer parity.

## Shield padding rejection

The shipped `ui\\hud\\shaders\\shield_test` pass has stage-0 alpha-kill
value **4**, while `widget_flash_red_pulse` (the shield mask) has value **0**.
Original `0x189A0` applies the low nibble of the compact texture-state record
to `D3DTSS_ALPHAKILL`, before the pixel combiners. The source is preserved in
[the texture-state setter](../tools/reference/halo2-decompiled/src/unknown_020560.cpp).
Bit 2 enables rejection of fetched zero-alpha texels. It is independent of
render-state alpha testing and must use the texture sample, not the final
combiner alpha.

The actual split-screen shield-meter image is 64x32 A8Y8, with 1,301 zero
RGBA padding texels. For a full shield, the original assembled/animated
combiner outputs `(0,0,0,128/255)` for those texels. With the original
ONE/SRC_ALPHA blend, omitting alpha-kill halves the destination color across
the padded rectangle. The mask's zero-alpha padding already produces final
alpha zero under its ordinary SRC_ALPHA/ONE_MINUS_SRC_ALPHA blend.

The native HUD fragment shader now reads the renderer's authored texture
state and discards zero-alpha stage-0 samples when bit 2 is enabled, before
executing the original combiners. It retains the shipped bitmap and blend
states. HUD geometry, safe margins and text placement are unchanged.

[The framebuffer regression](../tests/hud_render_tests.cpp) uses the real
renderer, authored meter materials and a known backdrop. It checks only
padding whose four bilinear texture taps all have zero alpha, and separately
requires visible shield fill. It reproduced the defect at `(47,160)` in
640x480 output. With the correction, 146,304 padding samples preserve the
backdrop across 4:3/widescreen, all four views, damage, recharge and overshield
states. Logs and captures are in `analysis/multiplayer/hud-framebuffer-before`
and `hud-framebuffer-after`; the three focused CTest checks also keep the
160 bitmap and 60 numeric placement fixtures passing. This checks the
specific padding defect, not complete Xbox framebuffer parity.
