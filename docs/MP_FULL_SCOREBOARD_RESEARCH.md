# Original full in-match scoreboard

The original full scoreboard is an engine-built roster of text/rectangle widgets. It has a place column, player emblem, Name and Score, optional Wins, and a connection-quality field. It does not use Lockout's small `nhdt ui\hud\scoreboard` meter as the roster layout. The narrow positive oracle executes the complete original later-update roster builder, its sort/rank/string/widget callees, and the display tail through the first real graphics request. This establishes a portable layout/text contract; complete graphics, live local input ownership and the preceding world-dependent display controller remain separate boundaries.
[Original-byte oracle](../tools/oracle_mp_full_scoreboard_1_5.py),
[complete outputs and inputs](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json),
[independent verification](../analysis/multiplayer/title-update-1.5/full-scoreboard/independent-freeze-verification.json).

The executable is the authenticated external `u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The comparison original TU1.1 is `default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Actual disc Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The suite preserves the four prior KBM pins, including its 172-check report. No production file, previous oracle/report or original asset is changed.
[Authentication](../analysis/multiplayer/title-update-1.5/dlc-authentication.json),
[source identities, dependency hashes and preserved pins](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

## Owning functions and proof

The suite compares **26 complete TU1.1/later bodies**, preserving every instruction byte, internal branch/switch topology, relocated image-memory operand and mapped direct-call destination. Decompiler labels and prefix matches are not used as behavioral authority. The report distinguishes complete correspondences from the separately supplied API fixtures.
[Complete instruction manifest](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instruction-manifest.json).

| Owner | TU1.1 | Later update |
| --- | --- | --- |
| Full roster builder | `161F80` | `163360` |
| Full renderer | `162AC0` | `163EA0` |
| Player row builder | `161A90` | `162E70` |
| Team row builder | `161790` | `162B70` |
| Sort/list/rank producer | `241500` | `2486F0` |
| Rank/tie result | `241D30` | `248F20` |
| Stat comparison | `241880` | `248A70` |
| Per-player update containing held counter | `15D160` | `15E540` |
| Current-view display controller | `15AEC0` | `15C2A0` |
| Whole per-player scheduler | `15D7C0` | `15EBA0` |
| Normal HUD suppression predicate | `163200` | `1645E0` |
| Death-observer display exclusion | `1642F0` | `1656D0` |
| Player emblem request | `164F00` | `1662E0` |
| Connection bars | `164FD0` | `1663B0` |
| Current-player pulse border | `161620` | `162A00` |
| Engine-table CRT constructor | `37EB40` | `3825D0` |

`163360` takes three stack arguments `(current_player_full_datum, output, binary32_alpha)` and returns with `RET C`. Its output extent is `293C`. The current-player argument resolves its low16 slot for local scroll and compares the complete datum for the current-row highlight. `163EA0` receives EDX=current player datum and one stack binary32 alpha, returns with `RET4`, and calls `163360` before any rendering. Its local output is at renderer-entry ESP minus `293C`.
[Builder ABI](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163360.asm),
[renderer ABI and draw order](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163ea0.asm).

## Portable ordinary Slayer text/layout contract

The source-owned list producer iterates the player pool's occupied bitmap ascending, builds full datums from each record's unsigned16 salt and slot, sorts through its actual comparator and emits rank bytes. `2486F0` receives ESI=list output and stack `(mode, secondary_comparison_enabled)`, `RET8`. The full scoreboard passes mode `Game+1128 !=0`, secondary comparison enabled1. List output has 16 DWORD datums at+00, eight signed16 team identities at+40, 16 rank bytes at+50, eight team rank bytes at+60, signed16 player count+68 and team count+6A. Displayed place is signed rank-byte divided toward zero by2, then1. Equal ranks retain the source tie bit and produce the same displayed place. **The source sort is not stable**; a stable score-only sort would change the frozen four-player order.
[Actual list producer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-002486f0.asm),
[rank result](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00248f20.asm),
[actual sort, including short-list helper](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00140910.asm),
[short-list helper](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00140bc0.asm).

Ordinary mode0 comparison selector bytes at `466180` are `00 02 03 0A 0D 12 05 FF`. Their actual `248A70/248C90` gates handle connected/exhausted/membership/team distinctions before primary Score. Selector5 reads the signed16 score at `engine+304+slot*1C+4` descending. The secondary comparator is not used to manufacture different displayed tie places: `248F20` passes its original secondary flag0. The complete comparator/switch listings and literal selector arrays remain available rather than replacing the other gates with a score-only inference.
[Comparison and widths](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00248a70.asm),
[team/secondary comparator](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00248c90.asm),
[literal selector words](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

| Full output offset | Type / meaning |
| --- | --- |
| `00,01,02` | Byte flags: primary Score available, Wins available, individual Wins available |
| `04` | Title widget, 80 UTF16 units; size`DC` |
| `E0` | Empty place header widget, 20 UTF16 units; size`64` |
| `144` | Name header widget, 20 UTF16 units; size`64` |
| `1A8` | Optional Wins header widget, 20 UTF16 units; size`64` |
| `20C` | Optional Score header widget, 20 UTF16 units; size`64` |
| `270` | Disabled reserved header widget; valid byte`2D0` is cleared |
| `2D4` | Eight team rows, stride`13C`; their count is DWORD`CB4` |
| `CB8` | Sixteen player rows, stride`1C8`; their count is DWORD`2938` |

The primary column is available when `Game+1128` is nonzero or DWORD`Game+18C !=1`. Wins availability is DWORD`Game+188 !=0`; individual Wins additionally requires team bit0 of`Game+184` clear when an engine exists. The ordinary positive fixture has no Wins column. Primary Score reads signed16 `engine+304+slot*1C+4`, or+6 when `Game+1128` is nonzero. Wins reads signed16+12. Player name is UTF16 at player+44. `162E70` copies a maximum255 units to scratch, executes the actual `233CF1` sanitizer and creates a20-unit widget containing at most19 units plus NUL. Decimal fields use original `%d` formatters and6-unit widgets, so their storage is at most5 units plus NUL. `15BB50` selects the original time formatter only for modes3,4,8; ordinary Slayer mode2 uses decimal Score. This source table has no kills/deaths columns.
[Full field readers](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163360.asm),
[row strings and columns](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162e70.asm),
[mode-dependent Score formatter](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015bb50.asm).

| Player-row offset | Meaning |
| --- | --- |
| `00` | Place widget6, size`48` |
| `48` | Emblem widget6, size`48`; full player datum at row`7C`, icon kind signed16 at`80` |
| `90` | Name widget20, size`64` |
| `F4` | Wins widget6, size`48` |
| `13C` | Score widget6, size`48` |
| `184` | Connection widget2, size`40` |
| `1C4` | Byte current-player flag; compared using the full datum |

Each widget's signed16 rectangle is `{top,left,bottom,right}` at+0; the source constructor decrements bottom and right to make the stored rectangle inclusive. Byte+8 is right justification. Text rectangle is four signed16 words at+A: top-1, left+2, stored bottom, stored right-2. Background ARGB binary32 words are+14 and foreground ARGB+24. UTF16 text begins+38. Valid byte is+44 for widget6, +60 for widget20, +3C for widget2 and+D8 for widget80. Inactive widgets have only their valid byte cleared; other bytes are not promised initialized. Native code must test validity rather than draw discarded raw fields.
[Widget6 initializer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00166020.asm),
[widget20 initializer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001660d0.asm),
[widget2 initializer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00166180.asm),
[widget80 initializer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00166230.asm).

Geometry uses source outer rectangle`4C65F0` and inner/safe rectangle`4C65F8`, supplied as rectangle API inputs in this suite. They are **not asserted as recovered default viewport values**. Horizontal origin is signed16 inner width divided toward zero by2, minus outer.left plus inner.left minus134. Vertical origin is inner.top minus outer.top minus truncation of the local scroll float. Content height is20 times(player count+team count)+44, plus21 if any team rows. Scroll is reset to0 when content fits, otherwise clamped to[0, content height-inner height] with original COMISS ordering. Title is20 high; headers start24 below the origin; each team/player row advances20 and the first team-to-player gap is21.
[Exact signed arithmetic and scroll](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163360.asm).

A player row reserves25 pixels for place and20 for emblem. Name width is216 minus54 when Score is visible and minus50 when individual Wins is visible. Wins starts at left+45+name width; Score follows Wins. Name is left justified; place/Wins/Score are right justified. A connection record, when admitted by actual`69E80`, adds its own7-pixel widget. The source reader handles network/quality state rather than deriving bars from native ping. For ordinary connected players, active names retain normal text color; disconnected, exhausted or membership-NONE gates dim the row. Membership-NONE also hides place and numerical columns. The actual player-color caller reads profile bytes at player+84 and source palette helpers; full profile/network replication is not recovered by this suite.
[Row widths/admission](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162e70.asm),
[profile/team palette caller](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-000830c0.asm),
[connection query](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00069e80.asm).

Per-row source background alpha is row_alpha times literal binary32`3E800000` (0.25); RGB is the actual caller color. Foreground alpha is row_alpha. Foreground RGB is color times`3E99999A` then plus`3F333333`; dim rows then multiply by`3ECCCCCD`. These are separate SSE operations, not a fused expression. Header background RGB is binary32`3E000000` (0.125). Title/header/row alpha has the source stagger: for source row index k among N=players+teams+2, clamp((input_alpha - (binary32(k)/binary32(N))*0.5)*2,0,1), retaining each operation and source unordered comparisons. Title uses k0, headers k1, then team/player rows from k2.
[Literal words and original ordered stores](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json),
[row color instructions](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162e70.asm).

## Actual tags, headings and font

Actual Lockout matg `E1740000 globals\globals` is at file offset`1D18800`. Matg+16C identifies mulg`E78C0537 multiplayer\multiplayer_globals`. The **Universal** block's+1C is unic`EB8309FE multiplayer\global_multiplayer_messages`. The original full builder requests SIDs`07000105` (Name), `070000FD` (Wins) and`08000102` (Score). The suite runs original`1A9EE0` against the actual English locale index/string bytes; its three returned UTF8 strings are Name/Wins/Score. They are not guessed labels or substituted layout text.
[Literal tag blocks, locale rows and returned text](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json),
[original locale search](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001a9ee0.asm).

The complete renderer sets original font selector`4F3C70=6` at`1640F6` before its text pass. Actual disc `font_table.txt` slot6 is `conduit-13`. `1FA30` consumes each widget's UTF16 text and rectangle through the original text pipeline, rather than selecting a UI-tag textbox/font. Native font import/glyph drawing can consume this explicit selector; this suite does not execute font startup or the admitted D3D text draws.
[Font store and text calls](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163ea0.asm),
[actual disc font table](../assets/disc/maps/fonts/font_table.txt),
[UTF16 text dispatch](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0001fa30.asm).

Original emblem kind0 uses Runtime+54C shader`E811068C ui\hud\shaders\waypoint_emblem` and+554 bitmap`E810068B ui\hud\bitmaps\new_hud\emblem_dummy`. Kind1 uses+55C shader`E80C0687 ui\hud\shaders\waypoint_icon_meter` and+564 bitmap`E8070682 ui\hud\bitmaps\new_hud\dead_friendly_arrow`. The actual display-tail composition reaches the kind0 request with genuine original tag values and full player datum; it stops before`2325CA`. No graphics result is supplied.
[Emblem request and fields](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001662e0.asm),
[executed graphics-request registers/stack](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

The complete renderer orders emblems first, then background rectangles, connection bars and current-player pulse borders, then font6 text. Current-player border`162A00` uses its original clock tick/rate, x87 sine and foreground alpha; it is not an arbitrary solid highlight. These renderer bodies are retained statically, but drawing services after the first request are unexecuted here.
[Complete ordered renderer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00163ea0.asm),
[pulse border](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-00162a00.asm).

## Held request, fade and ordinary admission

Native F1 is a host binding to a held scoreboard request. The original held consumer at`15E6BC` requires local user player+28 and input user DWORD player+24 both not NONE. It resolves an active processed48-byte record and reads byte+1D nonzero. It updates an unsigned per-view byte at`4F63C0+view`, and `15C2A0` uses that byte for display fade. Xbox polling ownership is separate; the parent can bind the authorized host key without claiming an original PC keyboard mapping.
[Complete original player phase](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015e540.asm),
[display consumer](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015c2a0.asm).

Ordinary held growth is permitted when original engine-active`15DD10` succeeds and death-observer exclusion`1656D0` is false. Growth increments and caps at x87 FISTP of binary32(rate)*0.25; release decrements with floor0. The executed tail declares FPCW`027F` and MXCSR`1F80`, signed16 rate30, dt bits`3D088889` and an owned-unit identifier API input. It proves counter sequence1,2,3,4,5,6,7,8,8,8,7,6,5,4,3,2,1,0,0. It does **not** establish the Xbox title's FP-control initialization or whole player-update/unit lifecycle from this kernel.
[Executed literal control/player/clock records and counter stores](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json),
[engine lifecycle gate](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015dd10.asm),
[death observer exclusion](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001656d0.asm).

The source fade computes clamp(binary32(counter)*dt*binary32`40800000`,0,1), then original x87 pow with **literal double bytes`000000606666FE3F`** at`46A378` (the value1.899999976158142). That exponent must be imported exactly rather than replaced with a newly rounded1.9 double. The actual display tail executes the original pow and passes its binary32 result to`163EA0`. The full-fade executed fixture passes`3F800000`. Normal HUD predicate`1645E0` returns false when this pre-power fade is at least1, suppressing the ordinary HUD. Its partial-fade gate retains the normal HUD. At rate30/dt`3D088889`, counter8 suppresses, counter7 does not.
[Actual fade/pow caller](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015c2a0.asm),
[normal HUD predicate](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001645e0.asm),
[literal exponent/float words and executed calls](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

Additional automatic display gates remain in the complete instructions: a shared counter admits display after its signed clock-rate threshold when the engine is not active; dead-player admission checks current unit NONE, player+170<=1, flag4000 clear, actual`160570` false, tick strictly greater than rate*3 and player flag1. The death observer suppresses its special display for its original10*rate interval. These alternative gates are retained as source instructions, not generalized from `Unit.alive` or claimed executed by the ordinary held fixture.
[Full original gate order](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0015e540.asm).

## Executed positive fixture and limits

The fixture declares a valid four-slot pool and source-shaped options/stat/clock records as API inputs. Names and signed16 scores are Ada8, Grace8, Edsger3, Barbara-1, with full player datums`80000000..80000003`. It executes actual`3825D0` to produce the engine table rather than assigning an engine object. Complete`163360` returns four rows in source order **Grace, Ada, Edsger, Barbara**, places **1,1,3,4**, scores **8,8,3,-1**. Current player Ada is highlighted by its complete datum. Output, caller registers, original stores, profile/player/engine/clock records and all called source functions are retained.
[Complete positive fixture](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

For declared outer rectangle(0,0,480,640) and inner rectangle(24,32,456,608), the actual source output is:

| Field | Inclusive source rectangle |
| --- | --- |
| Header place | top48,left186,bottom67,right210 |
| Header Name | top48,left211,bottom67,right392 |
| Header Score | top48,left393,bottom67,right446 |
| Grace place | top68,left186,bottom87,right210 |
| Grace Name | top68,left231,bottom87,right392 |
| Grace Score | top68,left393,bottom87,right446 |

Later rows advance20 in Y. Name's20-pixel emblem reservation explains the different header and player-name starts. These rectangles are outputs for **declared** viewport inputs, not a claim that this viewport is original hardware's default.
[Literal output rectangles/text rectangles](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

Raw Lockout's serialized matg English locale record has its runtime loaded flag false. The full-builder fixture leaves that field and all raw cache bytes unchanged; its header strings are therefore empty. Independent `1A9EE0` searches use explicit lookup-record API inputs pointing to the actual file index/string bytes, and recover Name/Wins/Score without assigning a locale-ready byte. The one supplied platform boundary is host XGetLanguage code1 at the existing XAPI entry; its original kernel/EEPROM producer is not executed. Native Cache locale import is a distinct delivery boundary.
[Locale provider and separate lookup records](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json),
[original locale loaded gate](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-001a9f60.asm),
[XAPI language caller](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/instructions/u5-0033a4a6.asm).

Full`15C2A0` is separately attempted and stops at`BCFBD`, reading absent object-pool header+44. The fixture's current-unit datums are identifiers, not allocated world bodies, so the preceding player-proximity portion cannot complete. The positive display composition begins at the actual tail`15C448` with the literal saved-register frame and required registers declared; it reaches actual`2325CA`, then stops before drawing. The native portable boundary is roster construction/text/rectangle requests with actual match names, scores, full identities and source constants. Complete local scheduling, LAN name/profile replication, palette-loaded state, network bars, D3D emblem/text drawing and live world-dependent display prefix are not established by this suite.
[Exact first full-controller stop and tail ABI](../analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json).

Reproduce the frozen deterministic evidence with:

```sh
.tools/python/bin/python tools/oracle_mp_full_scoreboard_1_5.py --output analysis/multiplayer/title-update-1.5/full-scoreboard/reproduction/report.json
```

The independent run must match the measured report, every complete instruction file and the instruction manifest byte for byte. The final verification also checks the original input identities and prior KBM pins.
