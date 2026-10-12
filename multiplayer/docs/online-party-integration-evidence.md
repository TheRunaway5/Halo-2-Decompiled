# ONLINE parties, clans, team postgame and voice integration evidence

2026-10-11. Read-only investigation; this note is the only research output written. Source-backed gameplay/UI leaves are distinguished below from new Discord-account service policy. No live team match, microphone, WAN, retail Live backend or full party lifecycle is established by these controls.

## Authority and immediate integration points

PC addresses refer to the supplied [halo2.exe](../../Halo%202%20Vista%20installer/halo2.exe), image base `400000`, build `11122.07.08.24.1808.main`, SHA-256 `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`. Its flat image bytes were read directly and executed unchanged in memory. Authored Vista UI measurements come from [mainmenu.map](../../analysis/frontend/vista-asset-inventory-20261009/extracted/maps/mainmenu.map), SHA-256 `eb259c207da26adc1820ae0757d23efb4ce1d2c2018956635d1094c8364f6b44`, decoded by the existing [bounded UI inventory](../../analysis/frontend/hd-ui-followup-20261009/audit-vista-menu-extension.py). Xbox source means the recovered [reference tree](../../tools/reference/halo2-decompiled/src/). Addresses explicitly described as U5 are supplied title-update1.5 instruction owners, not guessed equivalents.

The existing deep seams are sufficient for a bounded implementation: verified private players in `PlayerDirectory`; a party tuple already accepted by `Coordinator`; native screen14/26/31/217 navigation; actual selected-team/profile admission; `SourceGameEndStats` and the existing team comparator; PC screen16/185 result consumers; and the authenticated room voice transport. The missing owners are verified party membership/invite acceptance, a party-aware HTTP adapter, clan queue eligibility, team completion publication/validation, and session voice lifetime through PCR.

## Verified party membership and queue ownership

[player_identity.py](../master_server/player_identity.py) owns stable private `players.id` by verified `(provider,subject)`. Chosen `screen_name` is deliberately nonunique and cannot identify or authorize a party member. The verifier alone supplies Discord username; the existing clan invitation lookup resolves that verified username through `player_profiles`. Reuse that directory and [ClanApi.session](../master_server/clans_http.py) authentication pattern. Do not expose Discord IDs or trust a client-supplied roster, rank, clan role or account identifier.

[Coordinator.enqueue](../master_server/matchmaking.py) already accepts a trusted tuple of1..`maximum_party` players, checks unique player identities, compatibility/all-map eligibility and duplicate tickets. `_teams` partitions whole tickets onto equal-size teams; party balance and effective ranks choose among feasible partitions. However, [MatchmakingApi.enqueue](../master_server/matchmaking_http.py) constructs exactly one `Player` from the caller's session. No party membership/invitation persistence, accept/reject route, leader owner or party snapshot was found at audit time. `ticket(player)` already finds a player inside a tuple. Expanding enqueue therefore needs authenticated accepted members and their current names/maps/compatibility/ranks, not a new team partition algorithm.

Party creation, invitation expiration, acceptance, leaving, leader transfer, queue consent and reconnect behavior are **native service policy**. Implement explicit bounded ownership and revalidate/freeze the whole roster atomically at enqueue; membership/logout changes must cancel or invalidate its ticket/proposal. Current `cancel` and session replacement can cancel a ticket containing the account: decide and enforce leader/member authority instead of accidentally making every endpoint leader-only or letting an unrelated account cancel a party. Preserve the existing no-await mutation interval after request parsing. Catalog responses currently omit `maximum_party`, team size and clan eligibility, so a truthful party UI needs those admitted policy fields.

`PARTY_RULES_2007` contains `minor_clan_match` (3..4, imbalance0) and `major_clan_match` (6..8, imbalance2), but these are party-size policies. They do not establish same-clan membership, clan rank, a different opponent clan, or a clan result ledger. Reuse actual `clan_members(player PRIMARY KEY,clan,role)` and verify each accepted party member belongs to the same clan before a clan-only queue. Freeze/recheck membership against removal/leave. Authored clan permissions screen236 pane0 permits a Peon to participate in clan games; the rank>=1 recruit/broadcast guard is not evidence for banning Peons from clan matchmaking. Retired Live skill/rank updates remain unresolved; public matchmaking policy and existing comparisons do not recover that backend.

## Actual native entry paths and authored Vista bounds

[frontend_state.cpp](../../src/frontend_state.cpp): main6 ONLINE -> Discord/continue20 -> menu186. Quickmatch goes210, Optimatch/Playlists219, Create Party14. In mode3, current screen14 Start simply opens219; `lobby_view()` admits modes1/2/4 and supplies no actual ONLINE party roster. Y opens26, horizontal input selects Friends/Clan/Recent panes0/1/2, and X opens216 Friends Options or217 Clan Options. [frontend_clans.cpp](../../src/frontend_clans.cpp) already binds actual clan roster/invites to26 pane1 and selected-member31 context41. An explicit new context can bind verified party members/invites without aliasing clan commands. Keyboard167 has existing purpose7 for Discord clan recruit; party invitation needs a distinct purpose/command. Preserve chosen screen names separately from the verified username used to resolve recipients.

Measured rectangles below are signed source `[top,left,bottom,right]`; origins are authored `(x,y)`. These are real definitions, not a proposal to copy Xbox geometry blindly into the PC1200 canvas.

| Vista definition | Authored controls suitable for integration |
| --- | --- |
|247 `bungie_news`, pane3 Create Party | List0 skin0,4 rows, origin`(-600,360)`; text0`[420,-50,150,230]` describes inviting Friends and finding games together; bitmap2 `xbox_live_menu` frame3 at`(250,500)`. Suitable existing ONLINE action artwork. |
|14 `pregame_lobby`, pane0 multiplayer |5 buttons,1 roster grid. Privacy text10`[90,-650,50,-30]`; count44`[540,220,500,570]`; invitation tip46`[200,200,-20,600]`; prior stats45`[50,-515,-30,-30]`. This pane uses PC1200 coordinates. |
|14 pane2 matchmaking | Retained legacy480 canvas:2 buttons,1 grid. Leader43`[172,-258,152,75]`, count44`[199,85,179,280]`, privacy10`[75,-260,55,70]`, prior stats45`[55,-195,35,70]`, tip47`[20,118,-167,280]`, subtitle2`[1,-220,-19,-60]`, playlist3`[-17,-220,-37,-60]`; names11..26`[177-22*i,118,157-22*i,276]`. |
|26 Y-menu, panes0/1/2 | Friends/Clan/Recent Players. Skin6,8 rows, origin`(-226,101)`,1 grid, no buttons. Tab0`[193,-262,173,-145]`; footer1`[-186,-272,-210,203]`; Name2`[163,-219,143,0]`; columns4..7 are Level/Msg/Voice/Status at left0/50/100/150 and right50/100/150/205. No authored text8 service-status widget. |
|31 selected-player options | Skin0,6 rows, origin`(-247,29)`, header OPTIONS. Right text0..7`[73-20*i,0,53-20*i,253]`. Suitable Invite/Remove/Promote/Leave actions with actual authority guards. |
|216/217 options | Friends216:skin0,7 rows,origin`(-106,24)`. Clan217:skin0,5 rows,origin`(-106,13)`,no authored text widgets. Longer dynamic action lists need existing scrolling. |
|27 | Collapsed screen with one text/bitmap, no roster list; not a party roster definition. |

The existing [FrontendLobbyRoster](../../include/halo2/frontend_lobby_roster.hpp) consumes actual compact session slots, names, appearance, membership, rank/member counts and voice. A party adapter can supply its verified actual members and explicitly unavailable rank. It must not fabricate a source session or a talking animation. The original lobby presentation maps Live custom subtype to pane0 and Live matchmaking to pane2; PC14's panes use **different coordinate scales**. Current [Vista loader](../../src/frontend_vista.cpp) selected screen set excludes14/26/31/167/186/210/216/217/219/236/237/247. Its global0.4 coordinate bridge cannot be applied to all these mixed legacy panes. The current [canvas](../../include/halo2/frontend_canvas.hpp) preserves a centered640x480 field with uniform `min(w/640,h/480)` scale; importing full PC panes needs a proven per-pane canvas contract. Existing Vista textures/font banks with retained measured legacy controls remain the bounded seam. See [earlier layout evidence](online-match-launch-evidence.md).

Original clan-to-party invitation is concrete: [unknown_2b116a.cpp](../../tools/reference/halo2-decompiled/src/unknown_2b116a.cpp), retail `2B233D`, adds item2 for online rank>=1; `2B24B2` binds string`190002A4` (Invite Clan To Party). `2B24FF` case2 requires actual source members and `239ABE`, then includes only nonzero XUID, not already in session (`!19ACC6`), not flags`C00`, and online flags`1`. It opens message factory`2B80D9` with kind2/recipient list/count; no recipients invokes`236299(2)`. These are invitations, not immediate membership. Current native217 omits this action; a native verified-username invitation command fits that seam, but the modern acceptance service must own membership. Original selected-player game invite `2390F8` in [unknown_238e42.cpp](../../tools/reference/halo2-decompiled/src/unknown_238e42.cpp) calls `online_friends_game_invite` with the selected actual friend/identity.

## Exact original team ranking and final PCR publication

PC `4692CC` checks ranking availability `4D25DF` (actual round flags`20`) then calls `4D3045(output,1,0)`. Team-mode `4D3045` enumerates team designators0..7 through retained predicate`46E858`, sorts word indices at output`+40` using `48D14B ->4D27CC ->4D2798 ->4D25ED`, then computes encoded ranks at`+60` through `4D2DCC`. The ordinary postgame descriptor at`7BDCC8` is `0,2,3,10,12,14,18,4,6,FF`; extended`7BDCD4` adds selectors15/16/17 before18 and7/8/9 after6. Extended mode is selected only by variant`+48` bit`10`.

`4D25ED(a,b,selector,secondary)` returns zero for NONE, non-team mode or equal teams; positive means b precedes a. Exact team selectors:

| Selector | Comparison, in descriptor order |
| --- | --- |
|10 | `playing(b)-playing(a)`; actual predicate`46E816`. |
|12 | Signed field7 wins descending. |
|14 | Signed field1 accumulated score descending. |
|15,16,17 | Extended only: signed field2 descending, field3 ascending, field6 descending. Do not invent unproved names or derive them from player sums. |
|18 | Team index`a-b` **only if secondary is true**. The actual `4692CC` call uses false, and rank comparators always use false. |
|11/13 | Implemented team exhaustion/current-score selectors, but11 is absent from all three used lists;13 is in live mode, not these game-over lists. |

`4D2DCC` walks the already sorted retained teams, skips self, counts strictly better and any ties, stops on first strictly worse, and returns`2*better+(ties!=0)`. Publication divides signed rank by2 with truncation toward zero. Exact tied teams publish equal placements; team index, host, username, clan or player kills cannot break that rank. The direct final rank leaf`4D2F96(team,game_over)` uses the same selectors but scans **all** designators0..7, skips self and continues after a strictly worse comparison (`4D301E ->4D3025`); it has no sorted-list early exit. Use the retained-list producer for the consumed snapshot path already proved by `4692CC`, rather than replacing the descriptor/rank owner with a simple winner flag.

Player rank is distinct: `4D2A9C` selector0 **returns zero in team mode**, then exhaustion(selector2) and membership-vs-NONE(selector3), then team criteria10/12/14/(15/16/17)/18, then own wins4/total6/(extended7/8/9). Thus player retained`+90` remains the rank from this composite comparator; it is not always the player's team rank. PC report text ultimately obtains a player's displayed place from their actual retained team, described below. Existing [full_scoreboard.cpp](../../src/full_scoreboard.cpp) `compare`, `compare_team`, `compare_statistics`, original sort and encoded-rank loops already implement the U5 equivalent; its secondary flag is explicit. Do not call the layout API with fabricated font/view/color inputs merely to obtain ranks.

Precise masks: PC engine pointer`8BF8F8`, playing mask word`+C` consumed by`46E816`, retained word`+E` consumed by`46E858`; both require active engine, team variant and index<=7. Recovered retail`158140`/TU1.1`1597E0` in [unknown_157450.cpp](../../tools/reference/halo2-decompiled/src/unknown_157450.cpp) is the producer already ported as [source_team_presence](../../src/spawn.cpp): initialize playing0, retain previous mask, iterate actual **connected** bitmap, skip membershipNONE; retain every member team including flags2, mark playing only if !(flags&2). No life/death or unit-presence condition. Native `MultiplayerMatch::join` already updates sticky `retained_team_mask_`; current final playing mask can be recovered from actual connected `state.players` memberships/flags with that same leaf, as [MultiplayerScoreboard](../../src/multiplayer_scoreboard.cpp) already does. A departed team stays retained and can still have statistics.

Final producer `4712E9` team branch `4713EC..47146F` visits each actual participant's valid team once using a visited bitmap, reads signed **field1** via`4D02C0`, obtains `4D2F96(...,game_over)/2`, and calls`466ED4`. Getter mapping is PC engine-stat-bank`+204 + team*12(hex) + field*2`; native [SourceGameEndStats](../../include/halo2/source_game_end.hpp) is `+1C4 + team*12(hex) + field*2`. Both team records are18 bytes, but PC/native player banks differ. `source_game_end_accumulate` owns wrapped signed field1, `source_game_end_award` owns team/player wins7. `state.team_total_scores`/`team_wins` are actual native copies, while other team fields must be transported directly if extended ranking is admitted.

`466ED4` only writes when recording enabled, publication not finalized, team0..7 and retained row occupied. It clamps score to signed`[-32768,32767]`, writes word`+2`, byte placement`+1`. This final accumulated-score producer differs from `469410..469460`'s current-score updater: that reads`46FF80` field0 and clamps **0..32767**. The native `vista_pcr_write_team_result` correctly represents the signed final writer; do not change it to the updater's clamp.

Consumed retained mapping and reuse:

| Field | Exact owner/mapping |
| --- | --- |
|Team bank | Retained`+CB0 + team*18(hex)`: occupied byte0, placement signed byte1, score signed word2;0..7 actual teams, rows8..15 unused for the admitted source teams. |
|Player membership | Actual selected PC configuration`+7C` => retained player`+88` (`player +C configuration`). Native sanitized configuration also owns`+7C`; retain the admitted actual selected team instead of writing FF. |
|Player result | Retained`+370 + slot*94(hex)`, individual composite placement`+90`, signed accumulated player score`+92`. Do not overwrite own score with team score. |
|Displayed player place | [vista_pcr_results.cpp](../../src/vista_pcr_results.cpp) `player_placement`: team flag at retained variant`+68 bit1`, actual signed membership at player`+88`, then occupied retained team`+1`; absent/invalid team=>-1. |
|PC report rows | Set actual `VistaPcrServices.team_play`, supply original localized team names, preserve all result stats/identity/appearance, then reuse actual PC16/185 and existing appearance/color/row owners. PC`40C99B` calls team-name owner`45C341`; that follows matg`+16C`→first mulg universal→team_names unic`+C`, ordinal`4CBF9B`, locale`43E3AC`. Existing Vista palette follows the same universal owner. |

The actual Vista name owner is **`45C341`**, not placement formatter`46C675`. Nine unchanged executions of ordinal`4CBF9B` for indices0..8, resolved against only the real referenced unic, returned:

|Team | Source SID | Actual English text |
| --- | --- | --- |
|0 |`0100030A` |Red Team |
|1 |`0100030B` |Blue Team |
|2 |`0100030C` |Yellow Team |
|3 |`0100030D` |Green Team |
|4 |`0100030E` |Purple Team |
|5 |`0100030F` |Orange Team |
|6 |`01000310` |Brown Team |
|7 |`01000311` |Pink Team |
|8 |`01000312` |Observers |

Exact cache path: matg`E1750000 +16C`→mulg`E4490228`→first universal at file offset`60149404`→unic reference at`+C`=`E7B1058D`, tag`multiplayer\team_names`, English first index4344/count9. Import that source-owned slice into a Vista name bank, then bind `build.team_name`; do not supply literal English names or search unrelated global substrings. `45C341` maps NONE to observer ordinal8 and calls the original localized lookup; it clears destination if unavailable. `46C675` is the already admitted placement SID table, with zero-based places0..8→`0C0000EC..0C0000F4`,9..15→`0D0000F5..0D0000FB`, outside0..15→SID0; `native_vista_pcr_placement_key` already owns that recovered table. These names/keys were read with `VistaCache` and the unchanged ordinal body in `tools/oracle_vista_pcr_screen.py::machine`, with no locale/service-result substitution.

At audit time [online_postgame.cpp](../../src/online_postgame.cpp), ONLINE [NativeVistaPcr](../../src/native_vista_pcr.cpp), and server [MatchResults](../master_server/match_results.py) explicitly reject teams, write membershipFF, and require empty team rows. Enable all three together only after publishing/validating actual admitted team membership, playing/retained masks, exact player/team placements and team signed totals. The ledger currently records player placements; for team-mode user placement, read the same occupied team row as PC PCR. Clan win/rank policy is additional service work, not a byproduct of removing the FFA checks.

### Unchanged PC execution controls and reproduction

Eight in-memory controls executed actual`4D3045`, both actual sorts, `4D2A9C`, `4D25ED`, `4D2DCC`, `4D023A`, `4D02C0`, `46E816/46E858` and `46AD5C`. Declared services supplied pool enumeration, selected variant/engine-bank pointers and active/team queries only. No instructions were patched and no native ranking function was imported. Setup:4 real-format player records stride204, teams`[0,0,1,1]`, flags1, no life limit, own accumulated totals`[0,50,100,150]`, retained mask3; call`4D3045(output,1,0)`.

| Control | Team order | Encoded team ranks | Zero-based team placements | Player order / encoded ranks |
| --- | --- | --- | --- | --- |
|Wins`[1,0]`, totals`[1,20]` |0,1 |0,2 |0,1 |1,0,3,2 /0,2,4,6 |
|Equal wins, totals`[4,9]` |1,0 |0,2 |0,1 |3,2,1,0 /0,2,4,6 |
|Exact team tie |1,0 |1,1 |0,0 |3,2,1,0 /0,2,4,6 |
|Playing mask1, totals`[4,20]` |0,1 |0,2 |0,1 |1,0,3,2 /0,2,4,6 |
|Ordinary tie, field2`[99,1]` |1,0 |1,1 |0,0 |3,2,1,0 /0,2,4,6 |
|Extended tie, field2`[99,1]` |0,1 |0,2 |0,1 |1,0,3,2 /0,2,4,6 |
|Extended tie, field3`[9,1]` |1,0 |0,2 |0,1 |3,2,1,0 /0,2,4,6 |
|Extended tie, field6`[9,1]` |0,1 |0,2 |0,1 |1,0,3,2 /0,2,4,6 |

The exact tie demonstrates that different personal totals keep distinct individual ranks while both teams have place0. The unchanged native-compatible U5 source oracle and333 controls already live in [oracle_team_full_scoreboard.py](../../tools/oracle_team_full_scoreboard.py); original comparator/rank instruction pairs are in its [instruction evidence](../../analysis/multiplayer/title-update-1.5/team-full-scoreboard/instructions/). Reproduce those source controls using `.tools/python/bin/python tools/oracle_team_full_scoreboard.py` (this existing oracle writes its normal fixture/report; it was **not rerun** in this read-only task). Direct PC inspection command used here is `objdump -d -Mintel --start-address=0x4d25ed --stop-address=0x4d2771 'Halo 2 Vista installer/halo2.exe'`; other exact ranges are pinned below. The PC control harness was passed on standard input to `PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python`, using `tools/oracle_vista_pcr_screen.py::machine`; it wrote no files. Its declared services were `45B171=>variant`, `45B15E=>1`, `46B8A7=>engine bank`, `46E361=>1`, `4664E5` cursor reset and `4669CD` actual-format4-record enumeration. Pool`+44` pointed to records; variant`+48` was1 or17; `8BF8F8` pointed to actual mask words`+C/+E`. All actual rank/sort/stat/mask routines returned normally.

| Half-open PC byte range | SHA-256 |
| --- | --- |
|`4D25ED..4D2771` |`4e2749bab1209d5acbe0c32d1dc89c0ac0272aea1c6a5a3d4f6e3bd73cd9f9ee` |
|`4D2DCC..4D2E9D` |`2da201a01d66a88af875b6ec6b3e644939ddaf75424b2bc35557ab536883e25c` |
|`4D2F96..4D3045` |`1df80ab1b9179ac42f8963048c69f11e825b729ad662ce587496ddf0c6e08f23` |
|`46E816..46E89A` |`a85f600e760e668ac1dc4e69f2e4f912d5b0c365a5c17fc8f76b639d605cd56a` |
|`4713EC..471487` |`7a9f0e9fbb6441ef0a235c873acc6c4fbf29bd5a6cea8c666fc62c160e5b9b01` |
|`466ED4..466F43` |`a15de3ea1f0c91ede11366252d8b5240362745bec91f4ad29dadc9e8c400bcdb` |

## Session voice through actual postgame and party return

Original [network_voice.cpp](../../tools/reference/halo2-decompiled/src/network_voice.cpp) `544E0`: lifecycle1pregame,2startgame,4postgame,5joining,6matchmaking,9postmatch use passive mode2;3ingame uses gameplay mode1 only when game active/ready;7startmatch uses team mode3 if variant`+48 bit1`, otherwise2;8inmatch uses gameplay1 if active/ready, otherwise that7 fallback. `56C60` passive2 returns actual **session player mask** at membership`+10D0`; mode3 filters equal valid configured team`+7C` and excludes self. `56B70` dispatches passive2/3 through`57960`, gameplay1 through proximity/radio/death owners. Postgame should therefore use actual admitted session recipients without running spatial proximity over destroyed units. It is not automatically clan-wide voice or a private party-only channel while opponents remain in the same session.

Current [GameVoice](../../src/game_voice.cpp) already owns native SDL/Opus20ms packets, real transmitting/talking, enabled state, microphone mute and member mute. [multiplayer_runtime.cpp](../../src/multiplayer_runtime.cpp) creates it as a world-local object, routes gameplay using actual source positions/life/team states, then destroys it at runtime return. [main.cpp](../../src/main.cpp) resets the match connection after creating PCR; [online_postgame_screen](../../src/online_postgame.cpp) binds all voice icons hidden. Relay [match_transport.py](../master_server/match_transport.py) drops **all** binary messages in reviewing/postgame before kind validation; original native voice kind6 cannot pass. Completed room timeout is60s and review timeout30s. These three owners must change together for genuine PCR voice.

Retain one native session voice owner and authenticated connection through PCR Done, route only voice kind6 to actual admitted/mutually enabled/unmuted recipients in passive stage, preserve source/sequence validation, and keep gameplay packets rejected after finalization. Bind actual `VistaPcrVoiceServices` retained8-byte native identity→admitted slot→enabled/talking state. PC16 frames are talking0/idle1/disabled2;185 independently hides voice. The PC16 owner must not capture a destroyed world. Keep privacy IDs private and actual screen names visible. On Done, release the completed session channel; persistent premade-party voice afterward requires its own verified accepted party membership/transport policy. The existing clan voice service is a useful authentication/bounded-buffer pattern, not evidence that every clan member belongs to the party. Avoid a second concurrent SDL/audio owner accidentally mixing sessions.

Source Done leaf remains PC`40C01A` host continuation with two host queries, `6156D2 ->5AD9E3 ->5C884A(session,1)`; native local-owned PCR continuation is explicitly an adapter, and party-wide queue start still requires party leader/consent policy. [FrontendSession](../../src/frontend_session.cpp) retained content/state and actual PC16 entry are reusable. `return_online` currently clears launch and returns186 without a retained party owner; `clear_runtime_adapters` signs out account/matchmaking and stops clan voice. Deliberately retain/transfer party/voice authority across these boundaries instead of treating the menu stack as membership.

## Bounded completion and unresolved behavior

The current Team Slayer gameplay path already accepts actual selected teams through `slayer_rules(variant,true)`, profile/config admission and authenticated room slots; real team scoring, masks and source game-end accumulation/award owners exist. Existing team regression controls and final-state publication evidence can verify an implementation, but do not demonstrate a live ONLINE team match. Native relay admits one player per account connection; there is no recovered shared-account split-screen party admission. Other engines, retired Live ranks/clan ratings, automatic retail team assignment, all historical playlist compositions and complete original party wire protocols stay unresolved. Finish verified parties, Team Slayer publication, actual Vista roster/PCR, and retained session voice first; do not label those explicit modern adapters as recovered Xbox Live services.
