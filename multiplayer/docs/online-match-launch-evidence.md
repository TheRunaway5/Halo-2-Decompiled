# Native match launch: clan layout and Xbox voice evidence

Research date: 2026-10-10. This note records original asset/executable evidence
for the requested ONLINE work. It does not certify implemented Internet game
admission, physical microphone operation, or retail protocol compatibility.
Only this note was written during the research.

## Authorities and method

- Original Xbox `assets/disc/default.xbe`, SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
  Functions `0x57C20`, `0x57D60`, and `0x589E0` were read directly from the
  XBE section bytes with `objdump`, including the mute correction below.
- Original Xbox `assets/disc/maps/mainmenu.map`, SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Original Vista cache
  `analysis/frontend/vista-asset-inventory-20261009/extracted/maps/mainmenu.map`,
  SHA-256 `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`.
- Original `Halo 2 Vista installer/halo2.exe`, SHA-256
  `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`.
  Raster initialization `0x65E191` and `0x664979`, and their float constants,
  were read directly from this PE image.
- [Bounded definition inventory](../../analysis/frontend/hd-ui-followup-20261009/vista-menu-extension-inventory.json)
  and its [reader](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py).
  Additional skin measurements below were read from each original cache using
  these same bounded readers. They are not measurements of invented artwork.
- [Recovered voice source](../../tools/reference/halo2-decompiled/src/network_voice.cpp),
  [observer/radio source](../../tools/reference/halo2-decompiled/src/unknown_0574e0.cpp),
  [session manager](../../tools/reference/halo2-decompiled/src/unknown_058ee0.cpp),
  [session variant/browser fields](../../tools/reference/halo2-decompiled/src/unknown_054fe0.cpp),
  [configuration defaults](../../tools/reference/halo2-decompiled/src/unknown_0662e0.cpp),
  and [directional gains](../../tools/reference/halo2-decompiled/src/unknown_192b90.cpp).

## Vista clan definitions retain the original small canvas

**The supplied Vista cache does not contain a coherently rebuilt, high
resolution clan screen.** Screen 26's pane geometry and its principal clan
bitmap extents are inherited from Xbox. Some shared resources, including
`live_icons` and the default list skin, were enlarged elsewhere in the PC
cache. Importing the entire PC skin catalogue and applying the normal PC
1200-high transform to this legacy screen would not fix its layout. These
are literal decoded asset facts; whether PC retail ever constructed this
legacy screen has not been established. [Definition inventory, screen 26;
original `skin` and `bitm` blocks.]

Both caches call screen 26
`ui\screens\game_shell\online_y_menu\online_y_menu`. Both have flags **4**
(no independent global header), three panes, and button-key enum 0. Pane 1 is
CLAN. All three use list skin **6**, **8** visible rows, origin **(-226,101)**.
The list and pane layout are numerically identical across the two caches;
locale string IDs and source-local resource datums differ. [Inventory.]

All rectangles in this section use source order **[top,left,bottom,right]**,
with positive Y pointing upward from the UI center.

| Screen 26 pane text | Authored bounds | Current native binding / useful space |
| --- | --- | --- |
| 0 | `[193,-262,173,-145]` | CLAN tab label; preserve |
| 1 | `[-186,-272,-210,203]` | Additional Options / Select / Close footer; preserve |
| 2 | `[163,-219,143,0]` | Clan name, width219; keep the 15-character name here |
| 3 | `[218,-272,198,-145]` | Friends/tab-switch prompt; preserve navigation meaning |
| 4 | `[163,0,143,50]` | Original Level heading; spare for a bounded native count |
| 5 | `[163,50,143,100]` | Original Msg heading; spare |
| 6 | `[163,100,143,150]` | Original Voice heading; spare for a compact real voice status |
| 7 | `[163,150,143,205]` | Original Status heading; spare |

These are authored spare regions, not a suggestion to invent player ranks or
presence. If more width is needed, joining the adjacent heading rectangles
4–7 is a clearly identified native text binding inside the original header
band. A full service sentence will require measured text fitting. Screen 26
has **no text8**; the current `online_clan_text(...,8)` is a native service
message, painted by a separate synthetic widget. [Inventory;
`src/frontend_clans.cpp`, `online_clan_text`; `src/frontend_painter.cpp`,
the clan status block.]

| Clan/Y-menu bank | Xbox extent | Vista extent |
| --- | --- | --- |
| `y_menu_bkd` pages0/1 | 512×512 /16×512 | 512×512 /16×512 |
| `tabs` page0 | 529×38 | 529×38 |
| `y_menu_compact` page0 | 529×60 | 529×60 |
| row `hilite` page0 | 472×42 | 472×42 |
| row `item_background` page0 | 423×41 | 423×41 |
| Y-menu arrows pages0/1 | 27×17 | 27×17 |
| shared `live_icons` page0 | 28×28 | 56×56 |
| shared `rank_icons` page0 | 28×26 | 28×26 |

These extents establish that using genuine Vista-sourced pages does not mean
every clan page has more pixels than Xbox. They do not assert that every
same-sized image is byte-identical. The current texture pack admits same-size
donors, which is appropriate for these retained original-size assets.
[Original image descriptor blocks; current `FrontendPainter::Impl` upload.]

The actual outer art is also unchanged: body page0 is placed at **(-319,166)**,
right strip page1 at **(193,166)**, tabs at **(-320,204)**, compact top art at
**(-320,226)**. The visible panel spans source logical **x0..529** after adding
the centered-640 origin. The 512-high body extends beyond the 480-high viewport;
its clipping is part of this serialized layout. It is not a standalone 529×512
screen that should be squeezed into an invented rectangle. [Inventory.]

### Rows and the demonstrated overlap

Skin6 name text is `[41,4,21,235]`, font4; second line is `[24,4,4,340]`, font1.
The nested player skin's name is `[40,4,20,235]`. Its row backing is423×41 at
(-2,0); its emblem has definition flag1 and is excluded from layout bounds.
The 472×42 highlight has flags3 and is also excluded. The original layout
routine `0x24BC12` calls group construction `0x2BACBC`, then `0x2BAFA4` to
obtain each row union, and advances by **top−bottom**. [Original cache;
recovered `unknown_24c177.cpp` and `unknown_2afd97.cpp`.]

Applying that same rule gives:

- Xbox/legacy skin6 union **[41,-2,0,421]**, row pitch **41**.
- Vista raw skin6 union **[63,-2,0,441]**, pitch **63**, because shared
  live-icons pages became56×56 at inherited positions (287,7), (335,7),
  (385,7). Those definition children participate even when the application
  later hides the unsupported indicators.
- Xbox default skin0 union **[28,10,6,210]**, pitch22.
- Vista default skin0 union **[38,40,-8,440]**, pitch46; its item backing is
  400×42 at(40,-4), versus Xbox200×21 at(10,6).

Therefore a raw Vista skin6 import would push eight rows far outside the
authored clan footer. Legacy clan-options screen217 still places a5-row
default list at(-106,13), and screen31 places6 rows at(-247,29); blindly using
the enlarged PC default skin is similarly incoherent. Screen236 also retains
its four-row list at(-265,-17) and original text bounds. The inconsistency is
in retained PC data; it is not evidence for a new native scale factor.
[Original skin/image fields and inventory screens31/217/236.]

The current native screen retains the correct legacy41 pitch. Its added
status widget, however, clones footer text1 and adds40 to top, producing
**[-146,-272,-176,203]**. Row8 has origin101−7×41=**−186**; its name is
Y[−145,−165] and second line[−162,−182]. The service/voice sentence therefore
overlaps row8. This is an arithmetic reproduction of the current source,
not an inference from a sparse two-person screenshot. Keep status in the
reserved header band above the list, or use a separately measured native
footer composition without consuming the eight-row list area. Keep the clan
name and member count separate so a maximal15-character name need not share
the219-wide name field with `(100/100)`. [Current painter and clan bindings;
authored bounds above.]

### Practical widescreen and adoption seam

The native foreground transform in
[frontend_canvas.hpp](../../include/halo2/frontend_canvas.hpp) is uniform:

```text
s = min(window_width/640, window_height/480)
left = (window_width - 640*s)/2
top  = (window_height - 480*s)/2
pixel_x = left + logical_x*s
pixel_y = top  + logical_y*s
```

Thus source panel x0..529 has these measured target bounds; no horizontal
stretch is required:

| Window | Scale | Foreground origin | Panel X range | Footer Y range |
| --- | --- | --- | --- | --- |
| 640×480 | 1 | (0,0) | 0..529 | 426..450 |
| 960×540 | 1.125 | (120,0) | 120..715.125 | 479.25..506.25 |
| 1280×720 | 1.5 | (160,0) | 160..953.5 | 639..675 |
| 1280×800 | 1.666667 | (106.666667,0) | 106.666667..988.333333 | 710..750 |
| 1920×1080 | 2.25 | (240,0) | 240..1430.25 | 958.5..1012.5 |
| 2560×1080 | 2.25 | (560,0) | 560..1750.25 | 958.5..1012.5 |

The panel is intentionally left-biased inside the authored640-wide canvas;
centering the529-wide group separately would be a native presentation change,
not an original Vista transform. Text, controls, hit regions and scrolling
must share whichever transform is selected. Extend only identified background
tracks/frame edges into widescreen margins. The renderer already uses the
foreground canvas for ordinary UI and a separate marked background viewport.
[Current `src/renderer.cpp`, foreground/background UI viewport setup.]

The original PC executable's raster initialization at **0x65E191** computes
height×**1/1200** (constant VA0x7DBD6C, bytes`0e745a3a`), multiplies that by
**1920** (VA0x7DBD68, bytes`0000f044`), subtracts the window width and halves
it (VA0x79DE3C=0.5), then calls **0x664979**. The native Vista painter's
**480/1200=0.4** bridge preserves that PC raster scale for actual1200-high
PC screens such as shell5. It does not prove that the retained480-coordinate
screen26 should be shrunk by0.4. [Direct PE disassembly/constants;
`frontend_painter.cpp::screen`.]

Safe current seam: retain the legacy clan control geometry and41-pixel row
union, sample genuine Vista pages/fonts at their actual source density, and
bind native count/voice text within authored header bounds. A future
source-qualified legacy-Vista catalogue could retain screen26's own locale,
bank identities and480-canvas semantics, with each incompatible resource
handled explicitly. **Do not** add26 to `load_vista_settings` and assume the
existing `vista_menu()` dispatcher plus global0.4 transform supplies a proved
PC clan screen. Original PC construction/navigation of these retained clan
definitions is still unknown. [Current `frontend_vista.cpp`, selected set;
`frontend_painter.cpp` source-qualified dispatcher.]

## Original Xbox match voice policy

### Named lifecycle stages

The recovered session-manager initializer **0x58EE0** explicitly associates
the states with numbers. `voice_update_mode` **0x544E0** then chooses these
audiences; `0x56C60` implements the passive masks. Variant data byte0x48 bit0
is independently used as the **teams** flag by original browser-summary
function **0x64D70**. [Recovered manager/voice/variant sources.]

| Lifecycle | Number | Voice mode / audience |
| --- | --- | --- |
| None | 0 | Disabled0 |
| Pregame | 1 | Passive2: session player mask |
| Start game | 2 | Passive2 |
| In game | 3 | Gameplay1 when game-active138800 and simulation-ready68250; otherwise passive2 |
| Postgame | 4 | Passive2 |
| Joining | 5 | Passive2 |
| Matchmaking | 6 | Passive2 |
| Start match | 7 | Team-session3 when variant teams flag is set; otherwise passive2 |
| In match | 8 | Gameplay1 if active+ready; otherwise same team/all choice as7 |
| Post match | 9 | Passive2 |

Team-session3 includes only other players with equal valid(non-NONE) teams.
Passive2 uses the session mask. This table describes the selected session,
not a separate Discord clan or an automatic all-party override. No active
session kind or disabled voice service still produces no voice. `138800`
checks game globals flag1120. `68250` additionally requires its simulation
global, game-active flag, valid game index, no g_4CF772 inhibit, and simulation
world substate4. [Original recovered functions; `unknown_138800_2.cpp`,
`unknown_067e10.cpp`.]

### Alive/dead and radio/proximity audience

Use the source eligibility predicate **A(p)** rather than just a UI alive
boolean. Original **0x589E0** returns true for a valid simulation player with
a current unit, **or no previous unit**, or a previous unit whose death age
is **≤1.0 seconds**. Beyond that grace it returns false. Missing/invalid
simulation-player records are false. The inclusive1.0 boundary is confirmed
in the original XBE at0x58A3C–0x58A3F. [Original binary and recovered source.]

Define radio audience **T(a,b)** as:

```text
if not(team_game or Juggernaut): true
else if Juggernaut: neither a nor b is the Juggernaut
else: both teams are valid and equal
```

`0x57270` applies that rule from a local player to valid remote session
players. `0x53D90` detects game-type7(Juggernaut); `0x53D40` is team-game
enabled **or** Juggernaut. This avoids accidentally giving the Juggernaut an
all-map radio audience. [Recovered source.]

Sender masks are computed before local and reverse mute/capability filtering:

- **A(sender)**: proximity mask from0x57080 **OR** armed radio mask from
 0x57360. Radio eligibility uses T(sender,recipient).
- **¬A(sender)**: mask0x577D0 contains only recipients with **¬A(recipient)**
 and T(sender,recipient). An active recipient does not receive a dead-beyond-
 grace sender's speech. [0x56BD0,0x57080,0x57360,0x577D0,0x55990.]

At playback, original **0x57C20(listener,talker,packet_mode)** requests:

| State | Requested route |
| --- | --- |
| Both beyond death grace | 2: nonspatial speaker speech |
| Talker eligible, packet radio mode1, and T(listener,talker) | 1: radio/headset request |
| Talker eligible, otherwise, and talker is in listener proximity mask | 3: positional speaker speech |
| Dead-beyond-grace talker with eligible listener, or no eligible route | 0: silent |

The both-dead receiver branch itself does not recheck teams; the sender mask
already restricts it. Proximity is still evaluated for an enemy receiving
someone's team-radio packet, so speaking by radio does not make the speaker
inaudible to nearby enemies. The0/1 packet mode is explicit in the original
incoming channel branch; this is not a proposed Opus packet format.
[Original XBE0x57C20; recovered0x55E10,0x56DF0.]

### Exact proximity and attenuation

`0x57080` uses each player's current unit with previous-unit fallback and
three-dimensional Euclidean distance. A candidate passes when **d≤R+1.5**,
where R is runtime `motion_sensor_range` or fallback **8.0**. Thus the fallback
cutoff is inclusively **9.5 game units**; it is not a hardcoded30-foot value.
The computation performs no line-of-sight, wall, floor or acoustic obstruction
test. [Recovered0x57080 and `voice_proximity_object`.]

At output, **0x586F0** transforms the current talker-unit position into the
listener camera, computes its distance and normalized direction, and uses
default distance volume **D=−80**, directional volume **B=−1000**, and linear
directional gains. Its distance contribution is:

```text
v = D*d/(R+1.5)
if d>R: v += (D/R)*(d-R)*150
mix_bin_volume = clamp(int((1-gain)*B + v), -10000, 0)
```

For fallbackR8 the distance-only values are approximately0 atd0,−33.684 atd4,
−67.368 atd8,−1575.789 atd9, and−2330 atd9.5. There is still a separate hard
candidate cutoff above9.5. Per-bin truncation follows the original integer
casts. Do not substitute a guessed linear0..1 volume ramp. [Observer source;
defaults0x56AE0.]

`0x192D40` obtains five gains from `0x192B90`. In its default linear mode:

```text
gain_i = 1 - clamp(length_squared((direction-speaker_i)*0.5), 0, 1)
speaker directions =
 ( q, q,0), ( q,-q,0), (-q, q,0), (-q,-q,0), ( q,0,0)
q = 0.7071067690849304
```

These correspond to front-left, front-right, rear-left, rear-right, center.
The original omits the smallest gain(first minimum wins ties) and sends four
bins with indices0,1,4,5,2. Its fallback when no usable local camera exists is
one center bin at0 attenuation. Shared speaker playback chooses the nearest
eligible local listener; it does not stack one copy per split-screen listener.
Any nonspatial route takes precedence over spatial routes on that shared
speaker output. [0x586F0,0x57EB0,0x58590,0x192D40.]

Microsoft documents DirectSound volume units as hundredths of a decibel, with
0 unattenuated and−10000 effectively silent. A PCM implementation can derive
amplitude `10^(volume/2000)` from that unit, but a native stereo downmix of
the original four/five speaker routing remains an explicit adapter rather
than measured Xbox hardware parity.
[Microsoft DirectSound volume documentation](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418150%28v%3Dvs.85%29).

### Mutes, output choice, and an original-byte correction

Original **0x57D60** checks **per-player mute masks at0x4C9968**, not the
capability flags at0x4C9988. At0x57D80 and0x57DA8 the literal instruction
reads `WORD[player*2+0x4C9968]`; shared-output0x57E71 uses the same base.
`g_4C9878+0xF0=0x4C9968`, so these are `unknownF0` mute masks. The recovered
helper named `voice_test_unknown110` is incorrect at this call site. Port
flags `unknown110` separately carry capability/disabled/disallowed bits.
Follow the original XBE for this behavior. [Direct XBE disassembly;
`network_voice.h` fields; original0x57D60.]

Required routing consequences:

- Suppress when listener mutes talker **or** talker mutes listener.
- Outgoing0x55990 removes either-direction mutes and recipients with disabled
  or account-disallowed flags1/2.
- Port state0 keeps mixed routing: requested radio is headset1 if available,
  otherwise nonspatial speakers2; requested proximity remains spatial3.
- State1(Speakers Only) sends nonspatial routes to2 but preserves spatial3.
- State2(Communicator Only) forces1; state3(Disable Voice) forces0.
- Shared speaker routes2/3 are suppressed if **any** local player mutes the
  talker, preserving split-screen privacy. [Original0x57D60; recovered0x55990,
  0x541A0; original profile setting labels described in halo2-voice-evidence.md.]

### Radio timing and control authority

Original **0x57620** starts radio when either input value5 or8 is pressed
and A(player) holds. A press or actual XHV talking status refreshes its last
talk tick. Silence **≥0.75seconds** ends the burst. Total duration
**≥10seconds**, or becoming ineligible, ends its radio effect and prevents
rearming until the triggering input is released. Start/end effects are
indices0/1; observer0x56F60 may play effect2 after a recently heard qualifying
remote player's death. Radio sample selection comes from authored multiplayer
globals rather than a fabricated beep. [Observer source; network defaults
`real16F4=0.75`, `real16F8=10.0`.]

The original Xbox manual pp.22–23 identifies **White** as a tap to arm team
radio; proximity speech requires no radio-button hold. Bungie's September17,
2004 first-party answer independently says one press arms speech until the
speaker stops and that this is not a team/proximity toggle.
[Original Microsoft Xbox manual](https://download.microsoft.com/download/f/9/9/f99ab8f0-5191-4edd-b312-7a9b9e4784fa/halo2_covenant_mnl_us-en.pdf),
[archived Bungie answer, Drox question](https://halo.bungie.org/halo2updates/search.html?date=20040917).

The original Vista manual has a different policy: normal speech is team-wide;
**N** or the front-left bumper opens global speech until speech stops. Its
keyboard reference labels N global voice. Therefore a native keyboard binding
to **Xbox** team radio must be explicitly described as a native binding; N
does not prove Xbox semantics by itself. Both manuals were downloaded into
memory and read with `pdftotext`; their text was not edited.
[Original Vista manual pp.24–25 and keyboard reference](http://files4.bungie.org/Halo2GameManual_en.pdf).

Still unresolved: exact XHV speech detector/voice-mask DSP, physical microphone
threshold, original codec internals, stereo hardware downmix, retail secure
voice/NAT protocol, input-value5/8 physical decoder mapping, and original PC
construction/navigation of the retained clan definitions. A modern authenticated
Opus service can carry source-backed audience decisions without claiming any
of those unresolved platform mechanisms have been reproduced.

## Original in-game talking HUD: exact authored rows and condition producer

Follow-up research date: 2026-10-11. The following was read from original
`assets/disc/maps/lockout.map`, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`,
using [the bounded HUD reader](../../tools/inspect_multiplayer_hud.py).
This is the Xbox in-game HUD, separate from the Vista frontend clan asset
catalogue. It establishes the source semantics for the native HUD adapter.

The original `ui\hud\scoreboard` tag is datum `f2d1114c`, cache file offset
`0x20B8E9C`. Its only voice-related authored bitmap and text rows are:

| Field | Bitmap row 8 | Text row 4 |
| --- | --- | --- |
| Row name | `talking_player_icon` | `talking_player` |
| File offset | `0x20B91E4` | `0x20B9718` |
| Required conditions `[unit,extra,weapon,game]` | `[0,0,0,0x0800]` | `[0,0,0,0x0800]` |
| Forbidden conditions | `[0x0400,0,0,0]` | `[0x0400,0,0,0]` |
| Anchor | 3, scoreboard | 3, scoreboard |
| Widget flags | 0 | **8**, talking player substitution |
| Bitmap special | **0**, ordinary bitmap | not applicable |
| Fullscreen/halfscreen offsets | `(-20,-86)` | `(-22,-88)` |
| Quarterscreen offset | `(-20,-86)` | `(200,100)` |
| Shader | `ui\hud\shaders\widget_simple_flash_red` | `ui\hud\shaders\text_flash` |

The bitmap datum is `f2dc1157`,
`ui\hud\bitmaps\new_hud\scoreboard\speaker`: its one image is **19×19**,
with bitmap registration `(9,9)`. The widget registration is `(0,0)` in
all three modes; its sequence indices are all 0. The text row has right
justification 2, fonts `[0,0,0]`, scales `[0,0,0]`, and placeholder string ID
`blahblahblah`. The placeholder is deliberately replaced by a player name;
it is not a literal UI label. [Original Lockout rows and referenced bitmap.]

The condition is **game-engine word bit 11, mask `0x0800`**, not unit-word
bit 11. The forbidden unit bit 10, mask `0x0400`, is binocular state. The
unit word's `0x0800` belongs to the motion sensor and must not be repurposed
for voice. The recovered original condition builder
[**0x13A050**](../../tools/reference/halo2-decompiled/src/unknown_139296.cpp)
sets the fourth word's `0x0800` from byte `status+0x224`. The supplied
`nhdt.xml` calls this fourth-word bit “Player Talking,” and text flag bit 3
“Talking Player Hack”; the executable and original cache establish the
meaning independently of the editor labels.

The recovered source omits the main status producer, so the original XBE
was disassembled directly. Function **0x13992A**, on its multiplayer status
path, performs:

```text
0x13A008: EAX = local_user_index
0x13A00B: call 0x162FD0
0x13A010: compare result with NONE
0x13A013: status.player_index at +0x228 = result
0x13A019: AL = result != NONE
0x13A01C: status.has_player at +0x224 = AL
```

Function [**0x22A871**](../../tools/reference/halo2-decompiled/src/unknown_22a648.cpp)
handles **text flags `&8`**: when `has_player` is true and `player_index`
is valid, it resolves that player's record in `g_4E8C24` and formats the
UTF-16 screen name at **player record +0x44**. Otherwise it writes an empty
string. This is the concrete binding seam for the actual audible remote
speaker's user-chosen screen name; Discord IDs do not belong in that text.

### Audible-speaker selection, and the local-radio boundary

[**0x162DE0** and **0x162FD0**](../../tools/reference/halo2-decompiled/src/unknown_1600f0.cpp)
were also checked against the original XBE bytes. For each active local user,
the original maintains a signed-short counter per world-player slot:

1. Exclude the local listener and any player with a local-user index at
   player record `+0x28` other than `NONE`: these are **remote speakers only**.
2. Require initialized voice, actual talking status from **0x53750**, and
   the speaker's bit in the listener's actual speaker/headset playback masks
   at `settings+0xC8` or `settings+0x108`.
3. Increment that speaker's counter on each update meeting those conditions;
   clear it to zero immediately when they cease.
4. **0x162FD0** selects the player with the **smallest counter ≥4**, with a
   strict `<best` comparison. Iterator order breaks equal-count ties. With
   simultaneous sustained speakers this favors the newest speaker that has
   completed four consecutive eligible updates. The literal signed-short
   overflow behavior remains visible in the source; no saturation is present.

Therefore the native renderer can retain its existing special-type exclusions
`1,2,6,7`: the talking icon is ordinary **special 0**. Original scoreboard
rows 2/4 are player emblems with special 1, rows 5/6 rival emblems with
special 2, and row 9 territory meter with special 7. The supplied editor
schema labels unused special 6 as motion sensor; no Lockout bitmap widget
in the inspected catalogue uses special 6. None supplies the talking icon.

For source-faithful wiring, supply the selected audible remote name for text
flags 8, and set **only fourth condition-word bit 11** when that selection
exists. Preserve the authored rows, offsets, shaders, and binocular exclusion.
Binding local radio arming or a generic “microphone active” string to these
fields would change their original meaning. The inspected Lockout `nhdt`
catalogue has no additional row named for radio, voice, or microphone, and
the recovered condition builder exposes no separate local-radio condition.
Local radio has proven start/end audio effects in **0x57620** (above); an
original local-transmit `nhdt` indicator has **not** been established.

A separate player marker routine
[**0x162550**](../../tools/reference/halo2-decompiled/src/unknown_162550.cpp)
marks a player's unit as talking when gameplay voice mode is 1 and actual
talking status is true. That routine does not itself test listener audibility;
it must not replace the explicitly audible-speaker selection used by the
scoreboard name/icon. A new native local-radio status can be clearly labeled
as an adapter, but it is not an original authored HUD flag discovered here.
