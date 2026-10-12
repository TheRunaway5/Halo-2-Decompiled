# ONLINE postgame publication, placement and return evidence

2026-10-11. Read-only investigation of the original PC/Xbox owners, plus ten in-memory executions of unchanged PC ranking code. This note is the only research output changed. It identifies a bounded two-or-more-player FFA Slayer adapter; it does not establish a complete retail ONLINE session, Xbox Live identity, results wire protocol or live multiplayer playthrough.

## Authority

PC addresses are virtual addresses in the supplied [halo2.exe](../../Halo%202%20Vista%20installer/halo2.exe), image base `400000`, build `11122.07.08.24.1808.main`, SHA-256 `33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109`. File offset is VA minus `400000`. Original instructions and descriptor bytes were read directly, without patching the executable. Xbox source references are the recovered [reference tree](../../tools/reference/halo2-decompiled/src/), with retail addresses marked in those files. Xbox `assets/disc/default.xbe` SHA-256 is `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

The PC and native Xbox-derived player/stat/session layouts differ. The existing bridge deliberately translates consumed fields and does not cast one layout to another. Native room identities and modern service policy must remain identified as native adapters.

## Exact multiplayer placement subset

PC publication updater `4692CC` calls ranking availability `4D25DF` (actual round-owner flags bit `20`), then `4D3045(output,1,0)` at `4693F3..4693FC`. This is an existing multiplayer producer, not a missing policy to invent. `4D3045` enumerates the actual PC player pool, applies original `48D12B` sort through comparator `4D2CE2`, calls `4D2D0A` for each sorted datum, and stores the encoded rank byte at output `+50`. `469470..46948F` sign-extends that byte, divides it by two with truncation toward zero, and writes zero-based placement to the corresponding retained player slot `+90`.

The literal descriptor lists are:

| Owner/list | Bytes, including terminator |
| --- | --- |
| Live mode `7BDCC0` | `0,2,3,10,13,18,5,FF` |
| Postgame ordinary `7BDCC8` | `0,2,3,10,12,14,18,4,6,FF` |
| Postgame extended `7BDCD4` | `0,2,3,10,12,14,15,16,17,18,4,6,7,8,9,FF` |

Mode one selects the extended list only when variant `+48` bit `10` is set. It is not a default kills/deaths tiebreaker. `4D2A9C(a,b,selector,secondary)` gives the first nonzero comparison; a positive result means `b` precedes `a`:

| Selector | FFA comparison |
| --- | --- |
| `0` | Player without flags bit `2` precedes one with that bit. Team mode returns zero here. |
| `2` | Player whose lives are not exhausted precedes exhausted player. |
| `3` | Membership other than `NONE=-1` precedes `NONE`. |
| `4` | Signed field `7` wins, descending. |
| `6` | Signed field `1` accumulated score, descending. |
| `7,8,9` | Extended only: signed fields `2` descending, `3` ascending, `6` descending. Keep unproved field meanings unnamed. |
| `10..18` | Delegate to `4D25ED`, which returns zero for FFA, absent membership, or equal teams. |

Original exhaustion `46AD5C` is exactly `variant[+7C] > 0 && player.current_unit == NONE && player.lives_remaining == 0`. Its PC inputs are pool stride `204`, current unit `+28`, lives remaining signed word `+190`. Membership is signed `+BC`; flags are word `+2`. Native source fields supply these values through their own owners, not these PC offsets.

`4D2D0A` walks the **already source-sorted** roster, skips the same full player datum, counts strictly better rows and any ties, and stops at its first strictly worse row. It returns `2 * better + (ties != 0)`. Retained placement is this encoded value divided by two. Thus an exact two-player tie publishes placement zero for both; it is not broken by slot, display name, Discord ID, kills, host, or arrival time.

The corresponding recovered Xbox comparator already exists in [full_scoreboard.cpp](../../src/full_scoreboard.cpp): `exhausted`, `compare_statistics`, `compare_team`, `compare`, `original_sort`, and the row encoded-rank loop. Its ordinary FFA game-over branch uses the same eligibility, exhaustion, membership, wins and total ordering. `FullScoreboardPlayer` names its signed original fields in [full_scoreboard.hpp](../../include/halo2/full_scoreboard.hpp). The `stat8/stat_a/stat10` fields correspond to source fields `2/3/6`; `wins` is field `7`. This is a useful existing comparator to share or copy faithfully into a small ranking leaf, rather than call a whole layout operation with fabricated font, viewport, color or local-view inputs.

### Unchanged PC execution controls

Ten controls executed `4D3045`, its actual `48D12B` sort, `4D2A9C`, `4D2D0A`, `4D023A` signed-stat getter and **actual `46AD5C` exhaustion body** unchanged. Declared services supplied the real-format two-record pool enumeration, FFA query, selected variant pointer and engine-bank pointer. No native implementation was imported. These controls are source-function evidence, not a live game or network observation.

| Control | Sorted slots | Encoded ranks | Retained placements in sorted order |
| --- | --- | --- | --- |
| Wins `[1,0]`, totals `[1,20]` | `0,1` | `0,2` | `0,1` |
| Equal wins, totals `[4,9]` | `1,0` | `0,2` | `0,1` |
| Equal wins and totals | `1,0` | `1,1` | `0,0` |
| Slot zero flags bit2, higher wins/score | `1,0` | `0,2` | `0,1` |
| Slot zero exhausted, higher wins/score | `1,0` | `0,2` | `0,1` |
| Slot zero membership NONE, higher wins/score | `1,0` | `0,2` | `0,1` |
| Ordinary equal wins/totals, field2 `[99,1]` | `1,0` | `1,1` | `0,0` |
| Extended equal wins/totals, field2 `[99,1]` | `0,1` | `0,2` | `0,1` |
| Extended equal wins/totals, field3 `[9,1]` | `1,0` | `0,2` | `0,1` |
| Extended equal wins/totals, field6 `[9,1]` | `0,1` | `0,2` | `0,1` |

The equal two-row case deliberately demonstrates the original unstable sort swapping slots to `1,0`. [vista_pcr_order](../../src/vista_pcr_results.cpp) already preserves the PC retained-row `468E88`/`48D12B` sort. A stable sort or invented secondary key changes the displayed order.

Half-open original byte pins:

| Range | SHA-256 |
| --- | --- |
| `4D2A9C..4D2CE2` | `a3bde560d2d6ccd02c6e14b39a5938b434b8c6800a9502705d9ae23fa6f1f15f` |
| `4D2D0A..4D2DCC` | `f0bbe6023764cf965e304ad2737a3fcf74d48e1426fafe54095d14d71d024853` |
| `4D3045..4D31B4` | `303e553b7a9408fe0d65b29aba348231482d7e86f9bae195d5e11213b6eb8916` |
| `46AD5C..46AD98` | `e14314b25a5336a416750ae70957a16a393b64fb6ca08acfedf05912f32f4f79` |

Team selector code is visible in `4D25ED`, but the ten execution controls admit **FFA only**. They do not independently establish a complete team-mode final ranking, team admission or ranked skill update. Ordinary FFA Slayer is the smallest defensible ONLINE completion scope.

## Mapping actual native owners into PC PCR rows

PC `469EAD..469EC4` copies identity as two dwords and final configuration as `21` dwords (`84` bytes). The destination is retained `8B1C80 + 370 + slot * 94`: occupied byte `+0`, identity eight bytes `+2`, configuration `+C`, placement signed byte `+90`, score signed word `+92`. `469373..46937E` separately copies live PC player configuration at live record `+40`. These are identities and configurations owned by actual participants, not derived from the display-name string.

[vista_pcr_publish_consumed_snapshot](../../src/vista_pcr_publication.cpp) already accepts all sixteen explicit participant rows, copies those ranges, copies the complete numeric `4040` bank, then writes current/finalized bytes `+10/+11`. `vista_pcr_write_player_result` preserves the original player-score narrowing to signed sixteen bits; the team writer has a different signed clamp. Do not use the team clamp for players.

PC final score producer `4712E9` reads signed engine statistic field **1** through `4D023A`, then passes the sign-extended AX to `466E93`. PC engine-player stat stride is `20`; native `SourceGameEndPlayerStats` stride is `1C`. Use [vista_pcr_accumulated_player_score](../../src/vista_pcr_publication.cpp) on the actual full source datum and `SourceGameEndStats`, not `MultiplayerPlayer.score`, a kills count, or an unfinalized current-round value. [source_game_end_accumulate](../../src/source_game_end.cpp) is the original owner of field1 accumulation; it wraps signed sixteen bits and retains current score at game-over. Win field7 is written by [source_game_end_award](../../src/source_game_end.cpp).

Names and appearance are separate consumed configuration fields:

| PC configuration field | PCR consumption / current owner |
| --- | --- |
| `+0..3F` | UTF-16 display name, at most31 code units plus zero at62/63. `40C8CE` copies it to the row name; it is not the eight-byte identity. |
| `+40..4F` | Sixteen-byte appearance copied to report row appearance by `40C8CE`; emblem/color/representation consumers remain [VistaPcrAppearance](../../src/vista_pcr_appearance.cpp). |
| `+50..7B` | Account-related bytes: [native_vista_selected_profile_payload](../../src/native_vista_session.cpp) rejects nonzero bytes in its admitted fresh-selected-profile branch. |
| `+7C` | Source-clamped selected membership/team byte. Do not confuse this configuration byte with the live player's ranking membership field. |
| `+7E/+7F/+83` | Must stay zero for this fresh selected-profile adapter; other retained branches are not proved by it. |

[native_selected_player_configuration](../../src/native_player_profile.cpp) constructs the actual fresh selected name/appearance/team payload through the recovered `source_offline_local_configuration` converter. [native_vista_selected_profile_payload](../../src/native_vista_session.cpp) validates the admitted fresh branch and copies the first `84` bytes. This is suitable for a **declared native selected-profile adapter** after the queue's chosen name and selected appearance are authoritative. The native final source configuration is `90` bytes and later sanitization can remap representation `0/1` to `2/3`; blindly truncating that final source record is not the verified PC selected-profile operation.

Important admission invariant: [MultiplayerMatch::join](../../src/multiplayer.cpp) stores `name`, `source_profile_request` and sanitized `source_configuration` separately. The relay path in [multiplayer_network.cpp](../../src/multiplayer_network.cpp) selects `name` from authenticated admission but initially receives profile bytes from the peer. Reconcile the requested profile's UTF-16 name with that admission **before** storing/publishing it. Otherwise `player.name` can be correct while PCR config name disagrees. Preserve selected appearance fields; a queue identity must not rename the profile behind the user's chosen screen name.

An eight-byte opaque key issued by the authenticated room owner can bind the new native PCR, appearance and voice callbacks. It should be explicitly labeled a native room identity, retained consistently across all copies, and never displayed as a Discord account ID. It is **not** proof of retail XUID or the original offline machine-address identity. [source_offline_player_identity](../../src/source_player_profile.cpp) has a different, platform-address-based contract and must not be repurposed for this native key.

The reusable PC producer is [vista_pcr_build](../../src/vista_pcr_results.cpp), the literal `40C8CE..40CDD7` owner. It already supports sixteen rows, placement/score formatting, numerical panes, PvP columns, medals, weapons, sixteen-byte appearance and eight-byte retained identity. Reuse actual PC screen16/185 definitions and [row callbacks](../../src/vista_pcr_screen.cpp), not a custom report UI. `VistaPcrConsumedSnapshot::complete_retained_metadata=false` remains accurate: the consumed adapter does not recover the full event/machine/header publication.

## Result transfer is a separate real owner

Original Xbox [unknown_0b2440.cpp](../../tools/reference/halo2-decompiled/src/unknown_0b2440.cpp) registers `_network_message_type_game_results` at retail `B2CC0`, with literal name `game-results` and payload struct size `4FB8`. Encoder `B2C50` writes two32-bit header values, then calls `197680`; decoder `B2C80` reads those values and propagates the actual `197D80` result. Header meanings are not established here.

[unknown_1967d0.cpp](../../tools/reference/halo2-decompiled/src/unknown_1967d0.cpp) owns that delta codec. `197680` writes explicit changed/full participant/entry records, bounded counter-group updates, player-pair updates, team/general counters and address changes. `197D80` validates decoded counter bounds and bitstream exhaustion. Despite that reconstructed file's provisional input-record names, the real `game-results` registration establishes its use for result transfer. Do not infer wire compatibility merely from a `4040` numeric bank.

The PC has a separate receive/merge/publication chain. `5DECBB` compares its first input with owner `+30`; for an active result receiver, a nonzero second input must match expected owner `+8`. Mismatch writes receiver `+4=1` and declines the update. Accepted input calls `4686E7` into the retained result receiver, then **`469D6A`**, then stores next expected value `input+1`. These checks are observed fields, not a fully recovered authentication or reliability protocol.

`469D6A` refuses updates after retained finalized byte `+11` is set. It copies these payload ranges into the PC bank:

| Decoded payload range | Destination / length |
| --- | --- |
| `+10` | Retained participant properties `+370`, length `940` (16 rows). |
| `+950` | Retained team properties `+CB0`, length `180` (16 rows). |
| `+AD0` | Complete numeric bank `+E30`, length `4040`. |
| `+4B10` | Machine/address region `+DB10`, length `BB`. |

It also copies distinct header fields. A nonzero payload first byte tail-calls finalizer `469C75`: write final byte `+11=1`, reach actual `40C8CE` report build, and execute separate machine/report suffix services. Range `469D6A..469DFD` SHA-256 is `330c581d28f4d5d93fd72c46a8c0b28ccb4fda076d7e1d73f61d620d6ef93b6e`.

At investigation time, native snapshot [encode_state/decode_state](../../src/multiplayer_network.cpp) transports summary score/wins/kills/deaths/assists/suicides and source player configuration/flags/lives, but not the full `SourceResultStatistics` `4040` bank. [MultiplayerRuntimeOutcome](../../include/halo2/multiplayer_outcome.hpp) correctly allows actual local/host authority to retain deep-copied banks; a client snapshot's `results_finalized` bit cannot create those banks. The modern ONLINE relay therefore needs an explicit authenticated final-results handoff before client PCR construction. Shipping the bounded host-produced consumed snapshot is a native adapter option; reconstructing medals/weapons/PvP from summary counts is unsupported. Reliable delivery, acknowledgement, size limits and all-roster receipt are modern transport requirements to implement and verify; this note does not claim the retail codec supplies them to the new relay automatically.

## Completion, PCR16 Done and native requeue

Original PC normal host completion is `5D95A7`: preflight/pending guards, valid phase4 session, not closing, authority, actual `4499A8` game-over countdown complete; call complete phase writer `5C596E(session,5)`. A later handler update requests lifecycle kind4 through `5D75DA`. Manager `5D7738` can consume transitions within the same pump; no extra rendered-frame delay is implied.

The real UI observer `6176B0 -> 616D6C/6154D8 -> 616EF0` detects actual world UI3 to postgame UI5 and calls `409109 -> 408EAF(7)`. Mainmenu options retain reason7 at `+3D0`, pass through protected loader `4089BA`, then successful reason dispatch `60CE70(7) -> 60C724 -> 40E757` creates **PCR16**. PCR185 is the separate lobby-review factory; do not use it as the automatic completed-match screen. Detailed original evidence and independent385-case recording are in [pcr-return-owner.md](../../analysis/frontend/vista-session-flow-audit/pcr-return-owner.md) and [oracle_vista_pcr_return.py](../../tools/oracle_vista_pcr_return.py).

Original `40D04C` supplies input record type5 to **`40C01A`**. The retained [vista_pcr_input](../../src/vista_pcr_screen.cpp) reproduces the literal input route:

| PCR event | Original behavior / adapter seam |
| --- | --- |
| Code3 | Query actual details availability; zero selected-identity output; optionally request details. An absent modern details service should return unavailable. |
| Code1 or13, PCR185 | Close this review screen through `612604(this,3)`. |
| Code1 or13, PCR16, host predicate true | Recheck host, then `continue_session`. Retail `6156D2 -> 5AD9E3 -> 5C884A(session,1)` requests pregame phase1 through the complete phase writer. |
| Code1 or13, PCR16, host predicate false | Dialog98/callback40C00D, then separate session close through `6151E2(0) -> 5AD6AF`. |

[MenuState::vista_pcr_event](../../src/frontend_state.cpp) is already the native event seam; native Back/component1 supplies code1, details/component3 supplies code3, and ordinary Accept does not fabricate Done. Bind `VistaPcrScreenOwner::input.continue_session` to an explicit local return/requeue intent. If each ONLINE player owns their **own completed native postgame UI**, an explicitly native local owner may report host/continuation permission true on that local UI. This does not make each peer the original network session host, and it must not mutate the running game's host phase or claim faithful party-wide continuation. Preserve both source host queries and handle an intent once.

Original postgame phase1 leads through `5D903F -> lifecycle kind1`; observer UI5 to UI2 requests lobby14 via `646752` or `6467E9` according to actual protocol query. **Joining another playlist queue is service policy beyond that source Done leaf.** For the new ONLINE loop, retain the actual selected playlist and chosen profile, release the completed match/voice channel after final report delivery, then enter the real authenticated queue through that native intent. Do not label automatic requeue as a recovered retail phase writer.

Existing retained ownership supports the UI part cleanly:

- [VistaPcrScreenOwner](../../include/halo2/frontend.hpp) retains actual results, rows, appearance, voice, input, elapsed clock, focus and pane. Its row callbacks must not capture a destroyed match. PC16 voice can use the native opaque identity-to-admitted-slot lookup plus actual voice enabled/talking state; PC185 independently hides voice icons.
- [FrontendSession::enter(postgame_report)](../../src/frontend_session.cpp) requests a fresh actual PC16 frame through `MenuState::open_vista_pcr`; content and state survive. That factory clears the old navigation stack, so Back cannot accidentally resume the old match. UI resources must be ready before entry.
- [NativeVistaOfflineFlow](../../src/native_vista_flow.cpp) publishes consumed results before world teardown and waits for real resource-return milestones. It deliberately supports a sole local offline participant; leave those admission guards and its actual offline identity/session owners intact.
- [NativeVistaPcr](../../src/native_vista_pcr.cpp) is the suitable display adapter boundary. A separate bounded ONLINE constructor can validate actual authenticated roster slots/keys/names against a host-produced consumed snapshot, then reuse all PC row, appearance and locale producers. Absence of retail achievement/platform details should remain explicit service absence.

## Remaining evidence and validation limits

This investigation supports exact ordinary FFA game-over placement, signed accumulated score, fresh selected-profile translation, all16 PC report rows and a native local Done callback seam. It does not establish all retail result metadata, team-mode/other-engine placement parity, persistent ranked skill updates, clan result aggregation, party-wide postgame continuation, original result-message header meaning or protected loader internals. The new modern relay handoff and requeue must be verified with two independent real ONLINE clients; passing these isolated source controls cannot demonstrate that loop or live NAT/audio behavior.
