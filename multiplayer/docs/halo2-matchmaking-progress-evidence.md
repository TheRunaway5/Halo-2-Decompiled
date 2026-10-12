# Original matchmaking progress and the Vista screen 210 boundary

Research recorded 2026-10-12. Product code, existing evidence notes, builds and deployments were not edited for this investigation. This note concerns original UI/data ownership; it does not claim a live original Xbox Live service or pixel-identical native rendering.

The supported integration is to use Vista screen 210's authored searching, waiting and configuring panes, with actual admitted gathering membership controlling the waiting state. Its waiting pane has one text field and a16-slot roster. It has no authored countdown, deficit or minimum/maximum text region. The gathering growth wait is distinct from the original final game-start countdown. Adding either timer or an Xbox-style deficit region would be an explicit UI extension, not restoration of an authored Vista field.

## Primary inputs and reproducible artifacts

Only supplied original caches/executables and their independently checked bytes were used. Decompiled source is cited as a readable owner locator; the unchanged executable controls and extracted bytes are authority where a source function remains `todo` in the decompile's manifest.

| Input | SHA-256 |
| --- | --- |
| `assets/disc/default.xbe` | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| `assets/disc/maps/mainmenu.map` | `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b` |
| `assets/imported/a13438f3719c48bcb914624ed35022c7/vista/maps/mainmenu.map` | `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44` |
| `Halo 2 Vista installer/halo2.exe` | `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109` |

Vista cache build: `11081.07.04.30.0934.main`; executable image base `0x400000`.

New artifacts are under [`.local/matchmaking-progress-research-20261012`](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012):

- [Cache extractor](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/extract_progress_assets.py) rereads the actual caches and saves raw tag fields, localized strings, bitmap dimensions and layout in [progress-assets.json](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/progress-assets.json), SHA-256 `75e4f475595ebb4527818b980417f63b0b6fddd801fdfab874f423000dec2361`.
- [Executable extractor](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/extract_progress_executable.py) saves original Vista code, file offsets and dispatch tables in [vista-progress-code.json](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/vista-progress-code.json), SHA-256 `f08a667883bc10c07546471a5d439fc56be9cfea4900dbcb09cf79ca61755604`.
- [Original-byte runner](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/oracle_progress_controls.py), SHA-256 `eb1c40b1388c0d081a0eb909e5becf2b78c89f72e32181fa3bc6267b90229438`, passes 570 controls over 1129 distinct executed original instructions.
- [Machine-readable fixture](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/retail-progress-fixture.json), SHA-256 **`0c8e2f3fd3f79ac9ef9c38ef95f25511fd19b5d2d541c0b81a9075ca52ac8143`**. Schema 1 has `phase_rows`, `start_rows`, `count_rows`, `deficit_rows`, `property_rows`, `growth_wait_rows`, `vista_raw_state_rows` and individually labelled `controls`. Synthetic memory owners contain no real user identities.
- [Executed instruction trace](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/executed-original-instructions.txt), SHA-256 `97a4c52e372e053444993e6c443a238ddd3882094fb0104ab5275fc49c4b3ccc`; [run report](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/oracle-progress.json), SHA-256 `722b39b81011c18bbd839feb41e80f97b3145ebf1de1e86370cfa7eb6a4aa05a`.

Reproduce with `.tools/python/bin/python` followed by each of the three artifact scripts above. No native implementation is imported. No original instruction or callee is patched/stubbed. Xbox controls run complete original functions/callees against explicit synthetic data, except separately labelled widget/locale and timer blocks. Vista controls enter the original table-selection blocks **after** its Windows-wrapped state-provider call; they do not pretend to execute that provider or the original Live service.

## Actual Vista screen and layout

The menu is screen **210**, tag `ui\screens\game_shell\xbox_live\matchmaking_progress_dialog\matchmaking_progress_screen`. Vista datum `e62d040b`, root file offset 60441224, size 2860, flags 18, button-key 13, shape 3, header `06000234` = `MATCHMAKING`. Screen216 belongs to Friends Options.

The Vista definition has exactly**three** panes. Every pane has one roster list: skin palette index 20, datum `e52d030b`, `ui\list_skins\matchmaking\matchmaking_progress`; visible count 16; position(x,y)=(81,150); animation 4. Pane0 list delay is 0ms; panes1/2 list delay is 75ms. No pane has a button record or grid record. These are raw cache records, not inferred layout.

| Vista pane | Authored main text | Text records | Bitmap records | Roster lists |
| --- | --- | ---: | ---: | ---: |
| 0 | `1300043b`: `Searching for the best possible game...` | 4 | 10 | 1 |
| 1 | `0e00043c`: `Waiting for additional players...` | 1 | 11 | 1 |
| 2 | `0b001afa`: `Configuring game...` | 4 | 12 | 1 |

All three primary text records have font 1 and `(top,left,bottom,right)=(263,100,183,450)`. Pane1 has **no other text record**. The other three pane 0/2 texts are authored illustration callouts, with their exact individual bounds retained in `progress-assets.json`. Do not place counters in those regions while describing the result as unchanged Vista UI.

The main illustration bitmap is `ui\screens\game_shell\xbox_live\matchmaking_progress_dialog\matchmaking`: Vista frame 0=1024×512, frame 1=1024×512, frame 2=512×512, format 11. Pane0 uses frame 0 at(-320,80), pane 1 frame 1 at(-320,100), pane 2 frame 2 at(-180,110). Xbox's corresponding frames are 512×256,512×256,256×256, format 15. Vista's independently authored positions/text bounds are not a universal doubling of Xbox coordinates. Use the PC tag records, PC bitmap bank and existing PC layout transform together; no widescreen scale coefficient or new hit region was recovered here.

Each pane also authors step numbers/done marks/frame art. Their exact frames, positions and raw56-byte bitmap records are in `progress-assets.json`; retain those records rather than reconstructing the decoration by eye. This investigation did not render a framebuffer or prove the native painter's widescreen transform.

## Vista executable selector: separate numbering from Xbox

Original Vista title selection starts `0x64692B`, calls `0x6151F6`, then dispatches on `(raw_state-2)` using the table at `0x646960`. Pane selection `0x646A82` calls the same provider and dispatches using `0x646AB0`. The following results were executed from unchanged blocks beginning `0x646930` and `0x646A8A`; they cover every raw state -1..13.

| Raw Vista provider state | Title result/string | Pane |
| --- | --- | ---: |
| 2,3 | `1300043b`, `Searching for the best possible game...` | 0 |
| 4,5 | `0e00043c`, `Waiting for additional players...` | 1 |
| 6 | `0c00043d`, `Attempting to join a game...` | 2 |
| 7 | `1300043a`, `Cancelling matchmaking...` | 0 |
| 11 | `12000439`, `Matchmaking failed!` | 0 |
| -1,0,1,8,9,10,12,13 | 0 | 0 |

The final row means the selector returns string-ID0, not a newly invented fallback title. Do not equate these numeric states with Xbox UI state numbers or matchmaking party modes.

Vista screen update `0x646BA6` calls the pane selector (`0x646BEC`), changes its authored pane when required, then dispatches pane 0→`0x646988`, pane 1→`0x6469CE`, pane 2→`0x6469AB`. Each of those three handlers only finds child text index2 (`0x611973`) and sets its title (`0x61BF85`) from `0x64692B`. In particular, the **original Vista waiting handler does not render a deficit, count or timer**. The authored configuring text can be replaced dynamically by `Attempting to join a game...` for raw Vista state 6. See the extracted [pane 1 handler](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/vista-006469ce.asm), [pane 2 handler](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/vista-006469ab.asm), [title selector](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/vista-0064692b.asm) and [screen update](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/vista-00646ba6.asm).

Provider thunk `0x6151F6`→`0x5AD704`→owner 0x822100/`0x5D7919` enters a Windows wrapper. It calls `KERNEL32!GetLastError` through IAT0x79B220 and later `SetLastError` through 0x79B228. Those imported services were not substituted. The full provider's state production/transition ownership remains outside this bounded control. It is therefore supported to reuse these exact visual states for a declared replacement backend boundary, but not to claim the central proposal/relay states are recovered Vista raw state 6.

## Original Xbox lifecycle-to-UI contract

The Xbox owner chain is independent and fully executed here:

`0x70570` party/game session phase → `0x59570` manager phase → `0x199D7C` UI state → `0x25142F` authored pane. The readable phase owner is [unknown_058dd0.cpp:1268](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058dd0.cpp:1268); UI owner [unknown_19987f.cpp:1097](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_19987f.cpp:1097); pane owner [unknown_250155.cpp:1008](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_250155.cpp:1008). The UI owner is `todo` in the manifest; its original bytes and controls establish this mapping.

With manager state 6 and matchmaking mode 3:

| Party A mode | B requirement | Phase | Xbox UI state/title | Xbox pane |
| --- | --- | ---: | --- | ---: |
| 6,7 | none | 1 | 3, searching | 0 |
| 8,9 | none | 3 | 5, attempting to join a game | 0 |
| 10 | none | 2 | 4, waiting for additional players | 1 |
| 11,12,13 | B not live or no flag49E8 | 2 | 4, waiting for additional players | 1 |
| 11,12,13 | B live(state 3..8) and flag49E8 | 4 | 6, starting the game | 2 |

Other party modes return phase0. Manager state 7 instead supplies phase5→Xbox UI7. That chooses pane 2 while the actual game-start countdown is unavailable, pane 3 once available. Match mode2 selects UI8/`Have fun!`/pane 3; match mode1 selects UI1/`Matchmaking failed!`; match mode4 selects UI2/`Cancelling matchmaking...`.

Xbox210 is datum `e60503e5`, root file offset59569164, with**four** panes. Text counts5/6/5/12, bitmap counts8/9/10/14, one 16-slot roster each. Its fourth pane authors `Game starts in \uE45B`, settings and player total. **Vista has no fourth pane**, although its localized string list retains a game-start string using its different token `\uE478`.

## Accepted membership, limits and deficit

The original count provider is `0x199DC9`→`0x594A0`→`0x705F0`, returning `(players,maximum,minimum)`. It is valid only when all of these hold:

1. Xbox UI state is4 or6, manager's effective matchmaking mode 3 and manager phase 2 or4.
2. Matchmaking state's own mode is3; party A mode is11..13. Mode10 can already show the waiting pane but the count provider remains unavailable.
3. Game B exists, is live(state 3..8), and is not leaving according to original `0x58D90`.

On success `players` is B's membership player count at `B+0x1118`; minimum/maximum come from B's selected hopper using original `0x1932C0`/`0x193300`. It is **not** the global queue population, candidate population, party-size sum chosen after the fact, or raw skill. Invalid controls leave all output sentinels untouched. Controls cover all seven built-in hoppers, representative counts including 0 and 17, missing/invalid session gates, and party modes 10..14. This getter itself does not clamp the player count to 16.

The no-hopper branch falls back to B's maximum-player limit and assigns it to both limits; its original calling convention assumes the minimum pointer is available when maximum is non-null. No public API should copy that unsafe pointer contract. Source owner: [unknown_058dd0.cpp:1363](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058dd0.cpp:1363); original [0x705F0 bytes](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/original-000705f0.asm).

Xbox waiting formatter `0x251F8D` separately finds title child 2, deficit child 5 and tip child 7. At `0x25207A..0x2520FB`, invalid counts hide child 5; `players>=minimum` selects `11000762`=`No players needed`; otherwise `0E000761`=`%d more player(s) needed` with `minimum-players`. Unchanged block controls validate those branches without replacing widget/locale services. The Vista string list retains equivalents `11000760` and `0E00075F`, but Vista pane 1 and its handler do not provide the Xbox child 5 region. Localized-string presence alone does not prove active UI use. Original [waiting formatter](/home/eric/Developer/Halo-2-Decompiled/.local/matchmaking-progress-research-20261012/original-00251f8d.asm).

Xbox's separate game-player-count text parser obtains `max(party A count,game B count)`, caps it at 16 and retains the last nonzero count when the new count is 0. The owners are [parse_game_player_count](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/user_interface_text_parser.cpp:716), [party/game count getters](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_19987f.cpp:202). This is another session-owned count, not evidence for displaying everybody in a server queue.

## Gathering wait versus game-start countdown

These are different original properties and owners.

| Property | Producer/meaning | UI boundary |
| --- | --- | --- |
| Session data49CC kind 1 | Search role publishes current/max radius counters, number of valid candidate records and their filled-player sum; setters pin fields 15/15/63/1023 | Not global queue count; not needed in a privacy-safe progress payload |
| data49CC kind 2/value14 | Gathering timeout remaining, truncated seconds, pinned0..1023 | No timer rendering in inspected Vista210 handler |
| data49CC kind 3/value18 | Hopper growth wait remaining since the gathering high-water player-count growth clock; truncated seconds, pinned0..1023 | Not the final game-start countdown |
| data49CC kind 4 | Gathering members not ready after prior gates | Does not establish a measured remaining time |
| B flag49A8/value49AC | Actual start-match countdown, only available through manager state 7, live B and flag49A8 | Xbox pane 3/token; Vista210 has no pane 3 |

The gathering producer `0x71730` checks complete/proposed membership, then the original hopper wait getter `0x1931D0`. At `0x719E9..0x71A2E`, a not-yet-elapsed wait or the INT_MAX unavailable/below-minimum sentinel emits kind 3 using `(wait_ms-growth_elapsed_ms)/1000`, division toward zero, followed by original `0x599F0` clamping. New controls execute that exact block plus its real getters/setter. For Rumble Pit,4 players/elapsed29999ms still emits kind 3 but **seconds 0**; below minimum(INT_MAX) clamps to 1023. Neither value means the game is starting. The complete owner later publishes data49CC to game B using `0x5C2E0`; if no other kind was assigned, it can publish kind 2 from the gathering timeout. Source is a `todo` locator [unknown_071730.cpp:134](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_071730.cpp:134); original instruction evidence is [0x71730 disassembly](/home/eric/Developer/Halo-2-Decompiled/.local/gathering-lifecycle-research-20261012/original-00071730.asm) and the new `growth_wait_rows`.

The actual final countdown getter `0x59590` returns NONE(-1) except manager state 7/live B/flag49A8. Its token parser `0x22D775` formats available seconds as `:%02d`, otherwise `:--`. Start-row controls prove zero is an available countdown, whereas -1 is unavailable. Owners: [unknown_058ee0.cpp:302](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_058ee0.cpp:302), [user_interface_text_parser.cpp:458](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/user_interface_text_parser.cpp:458).

Kind3's proven use is replication of gathering progress via data49CC; this investigation did not recover any Vista210 text consumer for it. The retained Vista localized string `0F001B01`=`Waiting for game to fill up, start in %02d:%02d` is not an authored text record in this screen. Do not infer a displayed timer solely from that string.

## Supported replacement-service contract and remaining limits

A backend may expose additive `progress{players,min,max,wait_ms}` for an **actual** gathering, preserving separate availability. `wait_ms` should be null/unavailable where no finite growth wait exists. Do not call this field `game_start_countdown`, expose the original INT_MAX sentinel as seconds, or imply that 0 means launch readiness. A future final-start countdown would need its own owner and readiness semantics.

The current supported Vista presentation is pane 0 while searching, pane 1/its exact waiting title while actual gathering exists, and pane 2 during declared proposal/connection preparation. The last mapping is a native service adaptation; the raw Vista state table is not an interoperable backend enum. Keep native state names out of the visible product text. Preserve the authored pane/bitmap/text records instead of adding an Xbox pane or timer into Vista.

Counts and a roster may describe accepted gathering participants; chosen Halo screen names already public to those participants are sufficient. No Discord ID, account ID, ticket/session ID, raw/hidden rank, candidate score, QoS measurement or inferred team is required by these progress fields. Showing a casual searching illustration must not turn its rank-related callout into disclosure of a player's hidden rank. The user-required privacy contract remains separate from original debug/service internals.

Unresolved here: complete Vista provider transition production through its Windows environment, live original Live service behavior, actual native framebuffer/widescreen fit, and final-start countdown binding in a three-pane Vista screen. These limits do not prevent using the proven authored waiting pane and session-owned progress without inventing visual fields.
