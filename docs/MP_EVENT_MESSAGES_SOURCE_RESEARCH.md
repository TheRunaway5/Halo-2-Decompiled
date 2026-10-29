# TU1.5 multiplayer event messages: source policy and authored catalog

Research snapshot: 2026-10-28. The original behavior audit uses unchanged Xbox TU1.5 instructions and the original English Lockout cache. Subsequent implementation consumes actual imported MCC text and response owners; it does not establish complete runtime parity or original Xbox LAN/LIVE startup.

Ordinary personal kill/death messages are **not online-only and are not generally disabled in split screen**. The original display owner checks the number of local players, the selected response audience, event family/subtype, locality and postgame state. For an ordinary kill, the killer can receive `You killed #effect_player` and the victim `You were killed by #cause_player` with one through four local players. With two or more local players, the additional team-witness versions of that kill are suppressed. There is no authored everyone-audience ordinary-kill response in this reference bank. CTF, Assault, Slayer, Territories, Flavor, Oddball, King and Juggernaut each have their own authored responses; they must not be replaced with one universal killfeed rule. [Original display owner](../src/physics/physics_live_support_tu15_generated.cpp#L111419), [unchanged admission execution](../build/test-results/mp-event-messages-20261028/source-research/admission-probe.json), [original rows and English templates](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json).

## Native implementation

`source_event_messages_tu15` loads actual MCC response definitions and localized
text, team names and territory names. Shared `unic` owners index their selected
`matg` locale bank. Resident per-map `unic` owners index the independent
header `+2E4/+2E8` locale globals, with language descriptors at `+8 + language*1C`.
Using the shared table for Coagulation's local territory range incorrectly
selects four weapon prompts; the regression requires its actual four names:
“the hill”, “the red base”, “the blue base”, and “the rockslide”.

`consume_multiplayer_event_messages_tu15` drains the real replicated source
journal into local per-recipient queues, independently of audio, mute and
network role. It applies original audience/required/excluded selection,
explicit-target/broadcast dispatch, postgame and split-screen admission. It
resolves only the five tokens used by these authored response banks. Full
salted player identity protects name lookup, including retained departures.
Existing source literal notices share the same four-row queue; changed rows
retain source time/sequence ordering, and already expired packets cannot evict
fresh text. The serial frontier, queues and primary channel travel with the
runtime's existing receipt continuation values. They do not mutate authority
snapshots or create a new broadcast text channel.

The unit HUD now paints event text while dead, retaining the ordinary held/
automatic scoreboard gate and native font selectors 6/5. Weapon, ammunition
and radar artwork remain conditional on the actual live unit. The separate
primary channel preserves authored integer duration, including zero's empty
visible interval. None of the 168 checked MCC response rows uses quantity;
modified caches requesting that unsupported channel fail explicitly.

A real Territories capture also exposed a missing immutable operand at
`468F24`: whole `2CA170` compares the original UTF16 `#territory_name` token.
The native world now mounts its exact authenticated 32 bytes, without
replacing any code, callback or event result. The original method-record
exporter already pins this independent operand after its final prefix.
[Exact original operand receipt](../build/test-results/mp-event-messages-20261028/territory-static-operand.json),
[local MCC territory locale receipt](../build/test-results/mp-event-messages-20261028/source-research/mcc-local-territory-locale.json).

The first King policy interpretation was corrected by unchanged executable
receipts: `1A84AA` decrements subtype, so split-screen suppression covers
1/2/5/6 (control/contest), while start 0 and hill-moved 4 remain eligible.
All 4,960 admission cases and 64 locality/postgame guards are persistent test
inputs. The 167 original catalog rows remain byte-exact reference inputs.

## Focused implementation validation

The final coherent cohort passes **8/8** checks: synthetic delivery/continuation
controls; original policy/catalog plus actual Coagulation and Lockout assets;
all seven real mode families; native CPU glyph geometry; four-view Vulkan
framebuffer isolation; and existing literal-queue/announcer regressions.
Slayer's ordinary SMG fire took 51 delivered frames to produce the fatal hit,
69 attribution stores, 64 complete death-owner stores, actual kill/death
statistics, a real kill event, and both recipient messages. Other verified
transitions include CTF pickup/capture, Assault teammate pickup/arming/plant/
fuse/score, Oddball pickup/periodic score, hill control/contest, Territories
capture/retake/loss, Juggernaut transfer, and game over. A real UDP client
retains its targeted admission while skipping earlier broadcast history, then
receives and deduplicates the later game-over event.

Initial/goal/zone positions remain declared source-mover inputs. Additional
suicide/betrayal/Juggernaut deaths use explicit complete death-owner ABI inputs;
those cases do not prove their earlier damage attribution. Source policy is
checked against 4,960 unchanged executable receipts plus 64 guards, and CPU
rendering checks 360 font/glyph/view/size cases. GPU captures are temporary and
verify nonblank pixels only in the intended recipient's actual cell.

[Acceptance and cleanup](../build/test-results/mp-event-messages-20261028/implementation-acceptance.json),
[actual trigger report](../build/test-results/mp-event-messages-20261028/implementation-v8/evidence/trigger-report.json),
[full focused test output](../build/test-results/mp-event-messages-20261028/implementation-v8/evidence/Testing/Temporary/LastTest.log),
[exact build inputs](../build/test-results/mp-event-messages-20261028/implementation-v8/cohort.json),
[current runtime/receipt compilation](../build/test-results/mp-event-messages-20261028/runtime-wiring-compile.json).

The native tests recompile this task's source over a copied coherent engine
cohort. Concurrent edits to other immutable memory operands are preserved in
the checkout; only the authenticated Territory token is applied to that
cohort's memory owner. This is not full current-tree/launcher or authenticated
ONLINE-backend qualification. No launcher runtime was promoted. All owned
builds, binary copies and framebuffer captures were removed; small reports
and required original reference fixtures remain.

## Primary inputs and reproducibility

- TU1.5 original executable: `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. Addresses below are virtual addresses in that image, not retail-disc addresses.
- Original cache: [assets/disc/maps/lockout.map](../assets/disc/maps/lockout.map), SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. `matg +16C` selects `mulg` at file offset `1E14BB0`; its runtime record starts at `1E14C4C`. Runtime `+8C` selects `unic F0DE0F59` at file offset `20B3154`, English locale rows 1600 through 1845. Each catalog row has an exact file offset in the [extraction receipt](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json).
- The read-only parser is [HudCache](../tools/inspect_multiplayer_hud.py#L12) over [Cache](../tools/inspect_multiplayer.py#L25). Original locale layout is independently consumed by [the player-status oracle](../tools/oracle_player_status_tu15.py#L99). Symbolic string names come from this cache's SID bank; localized text comes from its English `matg` locale bank.
- [Assembly's mulg schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/mulg.xml#L50) is useful field nomenclature, not evidence that an event is produced or a UI is drawn. Field use below was checked against the original executable/lifted instructions.

The original admission probe ran through `tools/run_test.py -- .tools/python/bin/python -c …`. It executed unchanged `1A83B0` with declared ABI event, selected-response, player, game, player-pool and local-census inputs. It stopped on actual entry to `1A9D90` (text lookup), `1A8290` (audio continuation), or the caller return sentinel. It supplied no admission result, formatted text or function return. Its 4,960 cases cover general subtypes 0–48 plus representative populated family events, modes 1/3/4/5, one through four local players, and all five audience values; 64 additional locality/postgame guards and five `1A87A0` dispatch controls are retained. This establishes the guard policy, not full locale/audio/render/network execution. [Probe receipt](../build/test-results/mp-event-messages-20261028/source-research/admission-probe.json), [decoded original jump tables](../build/test-results/mp-event-messages-20261028/source-research/dispatch-tables.json).

## Bounded MCC metadata comparison

The actual imported MCC Lockout and Coagulation maps each contain **168** response rows. A semantic multiset comparison, preserving duplicate responses, matches all **167** original rows exactly for type/subtype, audience, required/excluded selectors, flags, named primary/secondary/plural keys, and integer duration. Row order differs in places, so ordinal-by-ordinal equality would be an invalid comparison. Each MCC map has one extra general subtype **49**, audience **4**, with all three text keys blank, flags zero, duration zero and required/excluded selectors zero. All named keys resolved; both maps contain 154 nonempty ordinary-literal rows and no quantity flags. This verifies those response fields and names in these two assets; it does not verify MCC localized template bytes, sounds, other languages, all maps, or live event delivery. [Comparison and pinned map hashes](../build/test-results/mp-event-messages-20261028/mcc-catalog/comparison.json), [actual MCC Lockout extraction](../build/test-results/mp-event-messages-20261028/mcc-catalog/evidence/mcc-lockout.json), [actual MCC Coagulation extraction](../build/test-results/mp-event-messages-20261028/mcc-catalog/evidence/mcc-coagulation.json).

Both MCC maps also retain authored `hudg` hold **2.0** seconds, fade **2.0** seconds and spacing **1.100000023841858**, matching the original reference timing. The check uses the current response loader to read the actual assets; it does not substitute the original bank as native runtime data. [Actual two-map timing and scope](../build/test-results/mp-event-messages-20261028/mcc-catalog/comparison.json), [runtime response loader](../src/audio/source_announcer_assets_tu15.cpp#L7).

## Delivery and recipient selection

The event prefix has type at `+0`, subtype `+4`, optional target player `+8`, cause player/team `+C/+10`, effect player/team `+14/+18`, and quantity/data word at `+1C`. Target `NONE` makes `1A8740` iterate source players, skipping rows whose byte `+2` has flag `2`; otherwise it passes the explicit target to the recipient owner. The recipient owner tries audiences in this order: cause player **0**, effect player **2**, cause team **1**, effect team **3**, everyone **4**. For each audience, authored row order wins. A player who fits several roles receives the first eligible response, not every matching row. [Dispatch and iteration](../src/physics/physics_live_support_tu15_generated.cpp#L111573), [broadcast iterator filter](../src/physics/physics_live_support_tu15_generated.cpp#L112166), [original selection owner](../src/physics/physics_live_support_tu15_generated.cpp#L111239), [native transcription of the selector](../src/audio/source_announcer_events_tu15.cpp#L38).

A response matches signed subtype `+4` and audience `+6`. Required-field selector `+10` values 1–4 require, respectively, a non-NONE cause player, cause team, effect player or effect team. Excluded-audience selector `+12` values 1–4 reject a recipient matching those corresponding event fields. These two selectors are distinct from the response's audience. For example, Oddball pickup's everyone row excludes the cause player, so the picker does not receive its witness caption. Blank display keys are meaningful authored omissions and do not acquire a generated fallback. [Eligibility owner](../src/audio/source_announcer_events_tu15.cpp#L38), [authored complete rows](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json).

## Online/offline and local-view admission are separate

Original `1A87A0` rejects raw game-options mode **4** before calling either `B1E80` or local dispatcher `1A8740`. For the other modes it invokes the network owner first, then local dispatch. `B1E80` itself proceeds only in raw mode **5**. In contrast, `1A83B0`'s text-vs-audio decision does not read this mode byte; it reads the actual local-player count at `[4F54F0]+8`. Therefore a network session with two or more local views still has the same event/audience text suppression, and one local view is not required to be online to reach text lookup. [Publication owner](../src/physics/physics_live_support_tu15_generated.cpp#L111607), [network gate](../src/physics/physics_live_support_tu15_generated.cpp#L5911), [display admission](../src/physics/physics_live_support_tu15_generated.cpp#L111450), [executed independent mode controls](../build/test-results/mp-event-messages-20261028/source-research/admission-probe.json).

The original XBE's indexed debug-name table at `4751C4` contains `none`, `local`, `sync-client`, `sync-server`, `dist-client`, `dist-server`, `none`. In particular index 4 points to `45F3B4` (`dist-client`) and index 5 to `45F3A8` (`dist-server`). The executable consumer of this table and actual original LAN/LIVE startup assignment have not been traced here. These names must not be used to assume that every ONLINE session is mode 5, nor may raw mode 4 be called playback on the evidence in this note. [Original indexed-name extraction](../build/test-results/mp-event-messages-20261028/source-research/world-mode-names.json).

Current native startup has a separate transport. Join/ONLINE nonhost clients consume replicated authority state rather than constructing their own source world; every owned source authority world, including network hosts, is configured with mode **1**, rate 30. The native source authority validator enforces this. ONLINE/System Link routing consequently needs the existing native transport boundary, rather than forcing original mode 5 to obtain captions. [Source-world storage boundary](../src/gameplay/multiplayer/multiplayer_world.cpp#L69), [actual configuration](../src/gameplay/multiplayer/multiplayer_world.cpp#L110), [authority validation](../src/runtime/source_match_authority.cpp#L26), [transport selection](../src/gameplay/multiplayer/multiplayer_runtime.cpp#L471).

The local recipient must have a non-NONE signed local-user field at source player `+28`; otherwise `1A83B0` returns without text or audio. When game `+1128` is true, it rejects all events except general type 0/subtype **19** (`Game over`). These checks precede the split policy. [Original guards, 1A83F7–1A8431](../src/physics/physics_live_support_tu15_generated.cpp#L111437), [executed controls](../build/test-results/mp-event-messages-20261028/source-research/admission-probe.json).

## Exact split-screen suppression

The following table describes the **additional** `local_count >= 2` guard in `1A83B0`. A suppressed response bypasses both text channels and continues to the sound owner `1A8290`; that does not assert an authored sound exists or that the audio queue accepts it. One local player bypasses these suppression cases. All ordinary locality, recipient, key and postgame conditions still apply. [Original instructions, 1A8431–1A84C8](../src/physics/physics_live_support_tu15_generated.cpp#L111450), [actual table bytes](../build/test-results/mp-event-messages-20261028/source-research/dispatch-tables.json), [execution matrix](../build/test-results/mp-event-messages-20261028/source-research/admission-probe.json).

| Event family | Subtypes / meaning established by authored keys | Two to four local views |
| --- | --- | --- |
| General | 0 ordinary kill; 35 collision; 36 melee; 41 sticky; 42 sniper; 43 sneak | Suppress **only audiences 1 and 3** (team witnesses). Personal audiences 0/2 remain eligible. |
| General | 7–10, 47–48 time/score-to-win thresholds; 14–17, 32–33 remaining-time notices; 25–30 lead changes; 44 boarding | Suppress all audiences. |
| General | All other populated subtypes, including 1 suicide, 2 betrayal, 13 unknown killer, 19 game over, 31 round over, 34 falling, 37 sudden death, 38 boot, 39/40 objective-carrier kill, 45 start-team notice, 46 telefrag | No additional split-screen suppression in this owner. |
| Flavor (type 1) | Double/triple kills, Killtacular, sprees and other authored flavor events | No additional split-screen suppression. |
| Slayer (type 2) | Every populated response | No additional split-screen suppression. |
| CTF (type 3) | Every populated response | No additional split-screen suppression. |
| Oddball (type 4) | 2 ball pickup | Suppress text; start/reset retain ordinary admission. |
| King (type 6) | 1/2 hill control/contest, 5/6 team hill control/contest | Suppress these subtypes. Start 0 and hill moved 4 retain ordinary admission. The unchanged 4,960-case execution fixture confirms the switch indexes subtype minus one. |
| Juggernaut (type 8) | 1 new Juggernaut | Suppress text; start retains ordinary admission. |
| Territories (type 9) | Every populated response | No additional split-screen suppression. |
| Assault (type 10) | Every populated response | No additional split-screen suppression. |
| Types 5 and 7 | No selected response bank | Do not fabricate response rows. |

This is why a blanket “split screen has no kill messages” rule would be wrong. The four original ordinary-kill rows are:

| Recipient role | English template | One local player | Two to four local players |
| --- | --- | --- | --- |
| Victim (2) | `You were killed by #cause_player` | Eligible | Eligible |
| Killer (0) | `You killed #effect_player` | Eligible | Eligible |
| Killer's team witness (1) | `#effect_player killed by #cause_player` | Eligible | Suppressed |
| Victim's team witness (3) | `#cause_player killed #effect_player` | Eligible | Suppressed |

An unrelated FFA observer does not gain a kill caption from these rows. Cause/effect priority and required/excluded selectors still determine actual selection. [Exact original rows 1–4](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json), [selector](../src/audio/source_announcer_events_tu15.cpp#L54).

## Localization, formatting and the three message channels

After admission, row `+C` is resolved through runtime `+8C` by original `1A9D90`, then expanded by `1A7C10`. The formatter finds `#` tokens, reads lowercase letters/underscores, and delegates to `1A7CE0`; output is bounded UTF-16. General substitutions include cause/effect player and team, cause-team score/opponent score, local player/team score, score-to-win, local spawn time/lives, and switch-side time. Unhandled tokens are offered to the selected engine through virtual `+C8` before literal fallback; mode-specific names such as `#territory_name` require that owner. Player/team substitutions must consume actual event/player data and the actual current view, rather than a generic string replacement with guessed display names. [Primary lookup/formatter calls](../src/physics/physics_live_support_tu15_generated.cpp#L111492), [formatter body](../src/runtime/source_map_world_support_tu15_generated.cpp#L9828), [token resolver](../src/gameplay/units/source_combat_action_tu15_generated.cpp#L27423), [engine delegation](../src/gameplay/units/source_combat_action_tu15_generated.cpp#L27820), [original comparison strings](../build/test-results/mp-event-messages-20261028/source-research/channel-resolver-disassembly.json).

| Authored response field | Original output owner | Source behavior |
| --- | --- | --- |
| Nonempty `+C`, flags bit 0 clear | `25AAF1`, called at `1A857B` | Ordinary literal four-row queue for this local user. |
| Nonempty `+C`, flags bit 0 set; plural key `+1C` | `25A9AF`, called at `1A8567` | Quantity/plural queue: accumulates signed16 quantity at row `+86`, selects singular/plural text, sets row `+84=1`. |
| Nonempty `+14`, integer duration `+18` converted through `1494F0` | `25AB76`, called at `1A85DF` | Separate per-view primary message at `+450`, text `+454`, active `+4D2`, created tick `+4D8`, duration `+4DC`. |

These are distinct consumers. Ordinary sound-caption identifiers are not sufficient evidence to feed the literal queue. After these outputs the owner calls audio `1A8290`; audio's response/locale/deduplication is separate. [Original channel branches](../src/physics/physics_live_support_tu15_generated.cpp#L111508), [quantity owner](../src/gameplay/multiplayer/source_multiplayer_engines_tu15_generated.cpp#L21217), [original secondary-owner disassembly](../build/test-results/mp-event-messages-20261028/source-research/channel-resolver-disassembly.json), [schema field names](../tools/reference/assembly/src/Assembly/Plugins/Halo2/mulg.xml#L131).

The original reference catalog contains **167 rows**, **154 nonempty literal templates**, **zero quantity-message flag bits**, and one nonempty separate primary-message key: general subtype **37**, `gen_sudden_death_all` → `Sudden death!`, integer duration **0**. Its ordinary `+C` key is empty. Duration-zero rendering semantics and the complete secondary/quantity renderer are not dynamically validated here. Row `+0`'s upper16 stores event type; do not mistake it for low16 flags. [Complete channel census](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json).

Existing [map-event string oracle](../tools/oracle_map_event_strings_tu15.py#L1) explicitly validates locale/UTF lookup and **non-template** formatting. It does not prove token expansion, full message production or all three channels.

## Drawing, dead players and held scoreboard

Original message draw entry `25AE69` requires HUD globals (`25A70E`), a valid local view, global HUD-hide predicate false (`13F620`, byte `[51DF50]+5`), and normal-HUD predicate `1645E0` true. The latter follows the view's source player and held/death scoreboard fade; it hides normal HUD once the full-scoreboard fade reaches one. No current-unit/dead test occurs in this draw entry. This supports removing an unconditional dead-player exclusion at this owner, but does **not** alone prove every upstream caller invokes it in every death-camera state. [Original draw-entry disassembly](../build/test-results/mp-event-messages-20261028/source-research/message-draw-entry-disassembly.json), [leaf-gate disassembly](../build/test-results/mp-event-messages-20261028/source-research/message-draw-gates-disassembly.json), [native source-faithful scoreboard predicate](../src/ui/hud/full_scoreboard.cpp#L339).

The original Lockout `hudg E7F50670`, `ui\hud\default`, has hold time `+58 = 2.0` seconds, fade time `+5C = 2.0` seconds and line spacing `+80 = 1.100000023841858`. An ordinary literal therefore expires at four seconds absent age extension/replacement; opacity decreases during the final two seconds using the source fade curve. These are extracted authored values, not the 3/1-second declared inputs used in some queue fixtures. [Actual hudg extraction](../build/test-results/mp-event-messages-20261028/source-research/hudg-timing.json), [schema field names](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hudg.xml#L42), [queue expiry and fade](../src/ui/hud/source_hud_messages.cpp#L49).

In the literal row loop, `25B280` calls `165AC0(view)`; when true, `25B289` increments the row's creation tick before age calculation. That leaf checks an existing selected engine and a per-view bit at engine state `+F4`. Its semantic producer is not identified here, so this note does not call it a dead/scoreboard flag. The behavior is an additional original age-extension gate to preserve when its live owner is bound. [Literal-loop disassembly](../build/test-results/mp-event-messages-20261028/source-research/message-draw-disassembly.json), [165AC0 instructions](../build/test-results/mp-event-messages-20261028/source-research/message-draw-gates-disassembly.json).

## Current native seams observed during this audit

The current source event announcer preserves selected `+C/+14` identifiers but consumes them only as fields in audio selections. The ordinary world text capture accepts `25AAF1` only from the blocked-teleporter callsite `1A65DB`; generic event callsite `1A857B`, quantity `25A9AF`, and separate primary `25AB76` do not become native messages through that capture. This is a concrete bridge gap, not evidence that the corresponding original event should be silent. [Audio-only publication](../src/ui/vista/native_vista_announcer.cpp#L327), [teleporter-only capture](../src/physics/physics_live_world_tu15.cpp#L1461).

The existing native literal queue represents four `88`-byte rows, bounded UTF-16 storage, source sequence/tick sorting, lifetime and opacity. Its validator deliberately rejects quantity/nonliteral flags `+84/+85`. The literal renderer uses each player’s queue and currently appears after `MultiplayerHud::draw`'s dead-player early return. Those limits must be considered when implementing generic events rather than synthesizing strings in the renderer. [Queue owner](../src/ui/hud/source_hud_messages.cpp#L30), [nonliteral rejection](../src/ui/hud/source_hud_messages.cpp#L78), [draw exclusion](../src/ui/hud/multiplayer_hud.cpp#L1042), [literal rendering](../src/ui/hud/multiplayer_hud.cpp#L1367).

## Scope still requiring proof

- The two-map MCC check establishes the named response fields and timing above. MCC localized template bytes, audio fields, other locales and all-map equivalence still require their own runtime owners; the appendix's English template text is the original Xbox reference.
- The catalog below states every authored response and the consumer policy. It does not establish that every subtype's gameplay producer is wired, that each preset can produce every event, or that remote delivery/de-duplication survives a complete match.
- Original LAN/LIVE startup assignment, distributed-client receive path, primary-message duration zero, full quantity/primary rendering, upstream death-camera scheduling and the engine `+F4` age-extension producer remain untraced or unexecuted here.
- End-to-end tests should separate cause, effect, both team witnesses and unrelated recipients; one vs two to four local views; local authority vs native network replica; postgame; personal kill/death, suicide/betrayal, Flavor, all seven game-mode families, and separate Sudden Death output.

All one-off emulator/disassembly/cache probes used the managed scratch workflow. Retained evidence is small JSON only (roughly 0.4 MiB total in `build/test-results/mp-event-messages-20261028/source-research`); no raw memory banks, binaries, object files, frames or captures were retained. Original inputs and unrelated active work were preserved.

## Complete original authored catalog

The following tables preserve original authored row order. Audience numbers: **0** cause player, **1** cause team, **2** effect player, **3** effect team, **4** everyone. `R/E` are required/excluded selectors (0 none; 1 cause player; 2 cause team; 3 effect player; 4 effect team). `Split` describes extra text admission with two or more local players: `yes` retains eligibility, `no` bypasses both text outputs. `—` is an authored empty key/template. `U+E…` denotes an original private-use medal glyph, not a substituted emoji. The two-map MCC check verifies these named keys and response fields, but English template bytes here are the original Xbox reference. Exact original source offsets, raw SIDs, sounds, flags and duration are in the [catalog receipt](../build/test-results/mp-event-messages-20261028/source-research/response-channels.json).

| Type | Family | Runtime offset | Authored rows | Nonempty literal templates |
| --- | --- | --- | --- | --- |
| 0 | General | 0x98 | 79 | 75 |
| 1 | Flavor | 0xa0 | 14 | 14 |
| 2 | Slayer | 0xa8 | 2 | 2 |
| 3 | CTF | 0xb0 | 22 | 19 |
| 4 | Oddball | 0xb8 | 3 | 3 |
| 6 | King of the Hill | 0xc8 | 8 | 8 |
| 8 | Juggernaut | 0xd8 | 3 | 3 |
| 9 | Territories | 0xe0 | 11 | 10 |
| 10 | Assault | 0xe8 | 25 | 20 |
| 5 / 7 | No selected response bank | — | 0 | 0 |

### 0 — General

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 31 / Round Over | 4 | 0/0 | yes | Literal: gen_round_over_all → Round over |
| 1 | 0 / Kill | 2 | 0/0 | yes | Literal: gen_kill_effect_player → You were killed by #cause_player |
| 2 | 0 / Kill | 0 | 0/0 | yes | Literal: gen_kill_cause_player → You killed #effect_player |
| 3 | 0 / Kill | 1 | 0/0 | no | Literal: gen_kill_cause_team → #effect_player killed by #cause_player |
| 4 | 0 / Kill | 3 | 0/0 | no | Literal: gen_kill_effect_team → #cause_player killed #effect_player |
| 5 | 1 / Suicide | 0 | 0/0 | yes | Literal: gen_suicide_cause_player → You committed suicide |
| 6 | 1 / Suicide | 1 | 0/0 | yes | Literal: gen_suicide_cause_team → #cause_player committed suicide |
| 7 | 2 / Kill Teammate | 2 | 0/0 | yes | Literal: gen_kill_teammate_effect_player → #cause_player betrayed you |
| 8 | 2 / Kill Teammate | 0 | 0/0 | yes | Literal: gen_kill_teammate_cause_player → You betrayed #effect_player |
| 9 | 2 / Kill Teammate | 1 | 0/0 | yes | Literal: gen_kill_teammate_cause_team → #cause_player betrayed #effect_player |
| 10 | 3 / Victory | 4 | 0/0 | yes | Literal: gen_victory_all → #cause_player wins! |
| 11 | 4 / Team Victory | 4 | 0/0 | yes | Literal: gen_team_victory_all → #cause_team wins! |
| 12 | 25 / Gained Lead | 4 | 1/0 | no | Literal: gen_gained_lead_all → #cause_player took the lead! |
| 13 | 25 / Gained Lead | 0 | 0/0 | no | Literal: gen_gained_lead_cause_player → You took the lead! |
| 14 | 26 / Gained Team Lead | 4 | 0/0 | no | Literal: gen_gained_team_lead_all → #cause_team took the lead! |
| 15 | 26 / Gained Team Lead | 1 | 0/0 | no | Literal: gen_gained_team_lead_cause_team → Your team took the lead! |
| 16 | 27 / Lost Lead | 0 | 0/0 | no | Literal: gen_lost_lead_cause_player → You lost the lead |
| 17 | 28 / Lost Team Lead | 1 | 0/0 | no | Literal: gen_lost_team_lead_cause_team → Your team lost the lead |
| 18 | 29 / Tied Leader | 4 | 0/0 | no | Literal: gen_tied_leader_all → #cause_player tied for the lead! |
| 19 | 29 / Tied Leader | 0 | 0/0 | no | Literal: gen_tied_leader_cause_player → You are tied for the lead! |
| 20 | 30 / Tied Team Leader | 4 | 0/0 | no | Literal: gen_tied_team_leader_all → #cause_team tied for the lead! |
| 21 | 30 / Tied Team Leader | 1 | 0/0 | no | Literal: gen_tied_team_leader_cause_team → Your team is tied for the lead! |
| 22 | 14 / 30 Minutes Left | 4 | 0/0 | no | Literal: gen_30_minutes_left_all → 30 minutes remaining |
| 23 | 15 / 15 Minutes Left | 4 | 0/0 | no | Literal: gen_15_minutes_left_all → 15 minutes remaining |
| 24 | 16 / 5 Minutes Left | 4 | 0/0 | no | Literal: gen_5_minutes_left_all → 5 minutes remaining |
| 25 | 17 / 1 Minute Left | 4 | 0/0 | no | Literal: gen_1_minute_left_all → 1 minute remaining |
| 26 | 19 / Game Over | 4 | 0/0 | yes | Literal: gen_game_over_all → Game over |
| 27 | 11 / Player Quit | 4 | 0/0 | yes | Literal: gen_player_quit_all → #cause_player quit |
| 28 | 12 / Player Joined | 4 | 0/0 | yes | Literal: gen_player_joined_all → #cause_player joined the game |
| 29 | 24 / Player Rejoined | 4 | 0/0 | yes | Literal: gen_player_rejoined_all → #cause_player rejoined the game |
| 30 | 13 / Killed By Unknown | 2 | 0/0 | yes | Literal: gen_killed_by_unknown_effect_player → Killed by the Guardians |
| 31 | 20 / Respawn Tick | 0 | 0/0 | yes | — (authored English sound) |
| 32 | 21 / Last Respawn Tick | 0 | 0/0 | yes | — (authored English sound) |
| 33 | 22 / Teleporter Used | 0 | 0/0 | yes | — (authored English sound) |
| 34 | 23 / Player Changed Team | 3 | 0/0 | yes | Literal: gen_player_changed_team_effect_team → #cause_player joined your team |
| 35 | 23 / Player Changed Team | 0 | 0/0 | yes | Literal: gen_player_changed_team_cause_player → You joined the #effect_team |
| 36 | 32 / 30 Seconds Left | 4 | 0/0 | no | Literal: gen_30_seconds_left_all → 30 seconds remaining |
| 37 | 33 / 10 Seconds Left | 4 | 0/0 | no | Literal: gen_10_seconds_left_all → 10 seconds remaining |
| 38 | 35 / Kill (Collision) | 2 | 0/0 | yes | Literal: gen_kill_col_effect_player → You were splattered by #cause_player |
| 39 | 35 / Kill (Collision) | 0 | 0/0 | yes | Literal: gen_kill_col_cause_player → U+E045 You splattered #effect_player |
| 40 | 35 / Kill (Collision) | 1 | 0/0 | no | Literal: gen_kill_col_cause_team → #cause_player splattered #effect_player |
| 41 | 35 / Kill (Collision) | 3 | 0/0 | no | Literal: gen_kill_col_effect_team → #effect_player splattered by #cause_player |
| 42 | 36 / Kill (Melee) | 2 | 0/0 | yes | Literal: gen_kill_melee_effect_player → You were beat down by #cause_player |
| 43 | 36 / Kill (Melee) | 0 | 0/0 | yes | Literal: gen_kill_melee_cause_player → U+E046 You beat down #effect_player |
| 44 | 36 / Kill (Melee) | 1 | 0/0 | no | Literal: gen_kill_melee_cause_team → #cause_player beat down #effect_player |
| 45 | 36 / Kill (Melee) | 3 | 0/0 | no | Literal: gen_kill_melee_effect_team → #effect_player beat down by #cause_player |
| 46 | 34 / Kill (Falling) | 2 | 0/0 | yes | Literal: gen_killed_by_falling_effect_player → You fell to your death |
| 47 | 37 / Sudden Death | 4 | 0/0 | yes | Primary: gen_sudden_death_all → Sudden death! (duration 0) |
| 48 | 41 / Kill (Sticky Grenade) | 2 | 0/0 | yes | Literal: gen_kill_sticky_effect_player → You were stuck by #cause_player |
| 49 | 41 / Kill (Sticky Grenade) | 0 | 0/0 | yes | Literal: gen_kill_sticky_cause_player → U+E048 You stuck #effect_player |
| 50 | 41 / Kill (Sticky Grenade) | 1 | 0/0 | no | Literal: gen_kill_sticky_cause_team → #cause_player stuck #effect_player |
| 51 | 41 / Kill (Sticky Grenade) | 3 | 0/0 | no | Literal: gen_kill_sticky_effect_team → #effect_player stuck by #cause_player |
| 52 | 42 / Kill (Sniper) | 2 | 0/0 | yes | Literal: gen_kill_sniper_effect_player → You were sniped by #cause_player |
| 53 | 42 / Kill (Sniper) | 0 | 0/0 | yes | Literal: gen_kill_sniper_cause_player → U+E044 You sniped #effect_player |
| 54 | 42 / Kill (Sniper) | 1 | 0/0 | no | Literal: gen_kill_sniper_cause_team → #cause_player sniped #effect_player |
| 55 | 42 / Kill (Sniper) | 3 | 0/0 | no | Literal: gen_kill_sniper_effect_team → #effect_player sniped by #cause_player |
| 56 | 43 / Kill (Standard Melee) | 2 | 0/0 | yes | Literal: gen_kill_sneak_effect_player → You were assassinated by #cause_player |
| 57 | 43 / Kill (Standard Melee) | 0 | 0/0 | yes | Literal: gen_kill_sneak_cause_player → U+E047 You assassinated #effect_player |
| 58 | 43 / Kill (Standard Melee) | 1 | 0/0 | no | Literal: gen_kill_sneak_cause_team → #cause_player assassinated #effect_player |
| 59 | 43 / Kill (Standard Melee) | 3 | 0/0 | no | Literal: gen_kill_sneak_effect_team → #effect_player assassinated by #cause_player |
| 60 | 44 / Boarded Vehicle | 0 | 0/0 | no | Literal: gen_boarded_cause_player → U+E053 You boarded an enemy vehicle |
| 61 | 44 / Boarded Vehicle | 2 | 0/0 | no | Literal: gen_boarded_effect_player → You were boarded by #cause_player |
| 62 | 39 / Kill (Flag Carrier) | 0 | 0/0 | yes | Literal: gen_kill_flag_carrier_cause_player → U+E049 You killed #effect_player |
| 63 | 40 / Kill (Bomb Carrier) | 0 | 0/0 | yes | Literal: gen_kill_bomb_carrier_cause_player → U+E051 You killed #effect_player |
| 64 | 38 / Player Booted Player | 4 | 0/0 | yes | Literal: gen_player_booted_player_all → #effect_player was booted from game |
| 65 | 45 / Start Team Notice | 0 | 0/0 | yes | Literal: gen_start_team_notice_cause_player → You are on the #cause_team |
| 66 | 46 / Telefrag | 2 | 0/0 | yes | Literal: gen_telefrag_effect_player → You were telefragged |
| 67 | 7 / 1 Min To Win | 4 | 0/0 | no | Literal: gen_1_min_to_win_all → #cause_player 1 minute to win! |
| 68 | 7 / 1 Min To Win | 0 | 0/0 | no | Literal: gen_1_min_to_win_cause_player → 1 minute to win! |
| 69 | 8 / Team 1 Min To Win | 4 | 0/0 | no | Literal: gen_team_1_min_to_win_all → #cause_team 1 minute to win! |
| 70 | 8 / Team 1 Min To Win | 1 | 0/0 | no | Literal: gen_team_1_min_to_win_cause_team → 1 minute to win! |
| 71 | 9 / 30 Secs To Win | 4 | 0/0 | no | Literal: gen_30_secs_to_win_all → #cause_player 30 seconds to win! |
| 72 | 9 / 30 Secs To Win | 0 | 0/0 | no | Literal: gen_30_secs_to_win_cause_player → 30 seconds to win! |
| 73 | 10 / Team 30 Secs To Win | 4 | 0/0 | no | Literal: gen_team_30_secs_to_win_all → #cause_team 30 seconds to win! |
| 74 | 10 / Team 30 Secs To Win | 1 | 0/0 | no | Literal: gen_team_30_secs_to_win_cause_team → 30 seconds to win! |
| 75 | 47 / 10 Secs To Win | 4 | 0/0 | no | Literal: gen_10_secs_to_win_all → #cause_player 10 seconds to win! |
| 76 | 47 / 10 Secs To Win | 0 | 0/0 | no | Literal: gen_10_secs_to_win_cause_player → 10 seconds to win! |
| 77 | 48 / Team 10 Secs To Win | 4 | 0/0 | no | Literal: gen_team_10_secs_to_win_all → #cause_team 10 seconds to win! |
| 78 | 48 / Team 10 Secs To Win | 1 | 0/0 | no | Literal: gen_team_10_secs_to_win_cause_team → 10 seconds to win! |

### 1 — Flavor

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 6 / Broke Killing Spree | 0 | 0/0 | yes | Literal: flavor_broke_killing_spree_cause_player → You ended #effect_player's killing spree |
| 1 | 0 / Double Kill | 0 | 0/0 | yes | Literal: flavor_double_kill_cause_player → U+E038 Double kill! |
| 2 | 1 / Triple Kill | 0 | 0/0 | yes | Literal: flavor_triple_kill_cause_player → U+E039 Triple kill! |
| 3 | 2 / Killtacular | 0 | 0/0 | yes | Literal: flavor_killtacular_cause_player → U+E040 Killtacular! |
| 4 | 3 / Killing Spree | 0 | 0/0 | yes | Literal: flavor_killing_spree_cause_player → U+E033 Killing spree! |
| 5 | 4 / Running Riot | 0 | 0/0 | yes | Literal: flavor_running_riot_cause_player → U+E034 Running riot! |
| 6 | 6 / Broke Killing Spree | 0 | 0/0 | yes | Literal: flavor_broke_killing_spree_cause_player → You ended #effect_player's killing spree |
| 7 | 6 / Broke Killing Spree | 2 | 0/0 | yes | Literal: flavor_broke_killing_spree_effect_player → #cause_player ended your killing spree |
| 8 | 7 / Kill Frenzy | 0 | 0/0 | yes | Literal: flavor_killing_frenzy_cause_player → U+E041 Kill frenzy! |
| 9 | 8 / Killtrocity | 0 | 0/0 | yes | Literal: flavor_killtrocity_cause_player → U+E042 Killtrocity! |
| 10 | 9 / Killimajaro | 0 | 0/0 | yes | Literal: flavor_killimanjaro_cause_player → U+E043 Killimanjaro! |
| 11 | 10 / 15 In A Row | 0 | 0/0 | yes | Literal: flavor_15_kills_cause_player → U+E035 Rampage! |
| 12 | 11 / 20 In A Row | 0 | 0/0 | yes | Literal: flavor_20_kills_cause_player → U+E036 Berserker! |
| 13 | 12 / 25 In A Row | 0 | 0/0 | yes | Literal: flavor_25_kills_cause_player → U+E037 Overkill! |

### 2 — Slayer

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: slayer_game_start_all → Slayer |
| 1 | 1 / New Target | 4 | 0/0 | yes | Literal: slayer_new_target_all → #cause_player has new target #effect_player |

### 3 — CTF

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: ctf_game_start_all → Capture the Flag |
| 1 | 1 / Flag Taken | 3 | 0/0 | yes | Literal: ctf_flag_taken_effect_team → #cause_team has your Flag! |
| 2 | 3 / Flag Returned By Player | 3 | 0/0 | yes | Literal: ctf_flag_returned_effect_team → Your Flag has been returned |
| 3 | 4 / Flag Returned By Timeout | 3 | 0/0 | yes | Literal: ctf_flag_returned_effect_team → Your Flag has been returned |
| 4 | 1 / Flag Taken | 4 | 0/0 | yes | Literal: ctf_flag_taken_all → #cause_team has the #effect_team Flag! |
| 5 | 1 / Flag Taken | 0 | 0/0 | yes | Literal: ctf_flag_taken_cause_player → You have the #effect_team Flag! |
| 6 | 3 / Flag Returned By Player | 4 | 0/0 | yes | Literal: ctf_flag_returned_player_all → #effect_team Flag returned by #cause_player |
| 7 | 4 / Flag Returned By Timeout | 4 | 0/0 | yes | Literal: ctf_flag_returned_timeout_all → #effect_team Flag returned |
| 8 | 1 / Flag Taken | 1 | 0/0 | yes | Literal: ctf_flag_taken_cause_team → #cause_player has the #effect_team Flag! |
| 9 | 6 / Flag New Defensive Team | 1 | 0/0 | yes | Literal: ctf_flag_new_defensive_team_cause_team → You are on defense |
| 10 | 6 / Flag New Defensive Team | 4 | 0/0 | yes | Literal: ctf_flag_new_defensive_team_all → You are on offense |
| 11 | 5 / Flag Captured | 4 | 0/0 | yes | Literal: ctf_flag_captured_all → #cause_team scored! |
| 12 | 7 / Flag Return Failure | 0 | 0/0 | yes | — (authored English sound) |
| 13 | 8 / Side Switch Tick | 4 | 0/0 | yes | — (authored English sound) |
| 14 | 9 / Side Switch Final Tick | 4 | 0/0 | yes | — (authored English sound) |
| 15 | 12 / Flag Contested | 1 | 0/0 | yes | Literal: ctf_flag_contested_cause_team → Flag contested! |
| 16 | 12 / Flag Contested | 3 | 0/0 | yes | Literal: ctf_flag_contested_effect_team → Flag contested! |
| 17 | 13 / Flag Capture Failure | 0 | 0/0 | yes | Literal: ctf_flag_capture_failure_cause_player → Flag must be home to score |
| 18 | 13 / Flag Capture Failure | 1 | 0/0 | yes | Literal: ctf_flag_capture_failure_cause_team → Recover your Flag to score |
| 19 | 1 / Flag Taken | 1 | 0/0 | yes | Literal: ctf_flag_taken_cause_team → #cause_player has the #effect_team Flag! |
| 20 | 3 / Flag Returned By Player | 0 | 0/0 | yes | Literal: ctf_flag_returned_player_cause_player → U+E090 You returned your flag |
| 21 | 5 / Flag Captured | 0 | 0/0 | yes | Literal: ctf_flag_captured_cause_player → U+E056 You scored! |

### 4 — Oddball

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: oddball_game_start_all → Oddball |
| 1 | 2 / Ball Picked Up | 4 | 0/1 | no | Literal: oddball_ball_pickup_all → #cause_player picked up the Ball |
| 2 | 4 / Ball Reset | 4 | 0/0 | yes | Literal: oddball_ball_reset_all → Play ball! |

### 6 — King of the Hill

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: king_game_start_all → King of the Hill |
| 1 | 4 / Hill Move | 4 | 0/0 | yes | Literal: king_hill_moved_all → Hill moved |
| 2 | 1 / Hill Controlled | 0 | 0/0 | no | Literal: king_hill_controlled_cause_player → You control the hill |
| 3 | 5 / Hill Controlled Team | 0 | 0/0 | no | Literal: king_hill_controlled_team_cause_player → You control the hill |
| 4 | 5 / Hill Controlled Team | 1 | 0/0 | no | Literal: king_hill_controlled_team_cause_team → #cause_player controls the hill |
| 5 | 2 / Hill Contested | 0 | 0/0 | no | Literal: king_hill_contested_cause_player → You are contesting the hill |
| 6 | 6 / Hill Contested Team | 1 | 0/0 | no | Literal: king_hill_contested_team_cause_team → #cause_player is contesting the hill |
| 7 | 6 / Hill Contested Team | 0 | 0/0 | no | Literal: king_hill_contested_team_cause_player → You are contesting the hill |

### 8 — Juggernaut

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: jug_game_start_all → Juggernaut |
| 1 | 1 / New Juggernaut | 4 | 0/0 | no | Literal: jug_new_all → #cause_player is the Juggernaut! |
| 2 | 1 / New Juggernaut | 0 | 0/0 | no | Literal: jug_new_cause → You are the Juggernaut! |

### 9 — Territories

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 / Game Start | 4 | 0/0 | yes | Literal: ter_game_start_all → Territories |
| 1 | 0 / Game Start | 0 | 0/0 | yes | — |
| 2 | 1 / Territory Control Gained | 4 | 0/0 | yes | Literal: ter_ctrl_gained_all → #cause_player controls #territory_name |
| 3 | 4 / Team Territory Control Gained | 4 | 0/0 | yes | Literal: ter_team_ctrl_gained_all → #cause_team controls #territory_name |
| 4 | 1 / Territory Control Gained | 0 | 0/0 | yes | Literal: ter_ctrl_gained_cause_player → You control #territory_name |
| 5 | 4 / Team Territory Control Gained | 1 | 0/0 | yes | Literal: ter_team_ctrl_gained_cause_team → #cause_player took #territory_name |
| 6 | 4 / Team Territory Control Gained | 0 | 0/0 | yes | Literal: ter_ctrl_gained_cause_player → You control #territory_name |
| 7 | 2 / Territory Contest Lost | 2 | 0/0 | yes | Literal: ter_contest_lost_effect_player → You lost #territory_name |
| 8 | 5 / Team Territory Control Lost | 3 | 0/0 | yes | Literal: ter_team_contest_lost_effect_team → You lost #territory_name |
| 9 | 3 / All Territories Controlled | 0 | 0/0 | yes | Literal: ter_all_territories_cntrld_all → #cause_player controls all territories! |
| 10 | 6 / Team All Territories Controlled | 0 | 0/0 | yes | Literal: ter_team_all_territories_cntrld_all → #cause_player controls all territories! |

### 10 — Assault

| Row | Event subtype / schema label | Audience | R/E | Split | Authored key → English template |
| --- | --- | --- | --- | --- | --- |
| 0 | 7 / Bomb Return Failure | 1 | 0/0 | yes | — (authored English sound) |
| 1 | 0 / Game Start | 4 | 0/0 | yes | Literal: invasion_game_start_all → Assault |
| 2 | 2 / Bomb Dropped | 1 | 0/1 | yes | Literal: invasion_bomb_dropped_cause_team → #cause_player dropped the Bomb |
| 3 | 3 / Bomb Returned By Player | 3 | 0/0 | yes | Literal: invasion_bomb_returned_player_effect_team → Your Bomb was returned by #cause_player |
| 4 | 4 / Bomb Returned By Timeout | 3 | 0/0 | yes | Literal: invasion_bomb_returned_timeout_effect_team → Your Bomb has been returned |
| 5 | 3 / Bomb Returned By Player | 4 | 0/0 | yes | Literal: invasion_bomb_returned_player_all → #effect_team Bomb returned by #cause_player |
| 6 | 4 / Bomb Returned By Timeout | 4 | 0/0 | yes | Literal: invasion_bomb_returned_timeout_all → #effect_team Bomb returned |
| 7 | 5 / Bomb Captured | 4 | 0/0 | yes | Literal: invasion_bomb_captured_all → #cause_team scored! |
| 8 | 6 / Bomb New Defensive Team | 1 | 0/0 | yes | Literal: invasion_bomb_new_defensive_team_cause_team → Defend the base |
| 9 | 6 / Bomb New Defensive Team | 4 | 0/0 | yes | Literal: invasion_bomb_new_defensive_team_all → Bomb the base |
| 10 | 7 / Bomb Return Failure | 4 | 0/0 | yes | — (authored English sound) |
| 11 | 8 / Side Switch Tick | 4 | 0/0 | yes | — (authored English sound) |
| 12 | 9 / Side Switch Final Tick | 4 | 0/0 | yes | — (authored English sound) |
| 13 | 1 / Bomb Taken | 1 | 0/1 | yes | Literal: invasion_bomb_taken_cause_team → #cause_player has the Bomb |
| 14 | 1 / Bomb Taken | 0 | 0/0 | yes | — (authored English sound) |
| 15 | 12 / Bomb Returned By Defusing | 4 | 0/0 | yes | Literal: invasion_bomb_returned_by_defusing_all → #cause_player defused the Bomb! |
| 16 | 13 / Bomb Placed On Enemy Post | 4 | 0/0 | yes | Literal: invasion_bomb_placed_on_enemy_post_all → Bomb planted! |
| 17 | 14 / Bomb Arming Started | 4 | 0/0 | yes | Literal: invasion_bomb_arming_started_all → #cause_player is arming the Bomb! |
| 18 | 14 / Bomb Arming Started | 0 | 0/0 | yes | Literal: invasion_bomb_arming_started_cause_player → Bomb is arming... |
| 19 | 15 / Bomb Arming Completed | 4 | 0/0 | yes | Literal: invasion_bomb_arming_completed_all → #cause_player armed the Bomb! |
| 20 | 15 / Bomb Arming Completed | 0 | 0/0 | yes | Literal: invasion_bomb_arming_completed_cause_player → Bomb armed! |
| 21 | 16 / Bomb Contested | 1 | 0/0 | yes | Literal: invasion_bomb_contested_cause_team → Bomb contested! |
| 22 | 16 / Bomb Contested | 3 | 0/0 | yes | Literal: invasion_bomb_contested_effect_team → Bomb contested! |
| 23 | 3 / Bomb Returned By Player | 0 | 0/0 | yes | Literal: invasion_bomb_returned_player_cause_player → U+E091 You returned the bomb |
| 24 | 13 / Bomb Placed On Enemy Post | 0 | 0/0 | yes | Literal: invasion_bomb_placed_on_enemy_post_cause_player → U+E057 Bomb planted! |

