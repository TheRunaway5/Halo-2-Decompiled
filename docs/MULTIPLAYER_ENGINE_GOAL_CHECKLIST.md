# Multiplayer engine completion goal

Status: **active, incomplete**. Started 2026-10-20.

Finish the TU1.5 multiplayer engine logic for all seven families, all 39 built-in
variants, and their supported editable rules. The user's 2026-10-20 clarification
sets **engine parity as the primary completion target**. Tags will be imported
from MCC and converted to TU1.5 equivalents. The converted tags supply content;
the original Xbox/TU1.5 executable and recovered source define engine behavior.
Track the broader match integration evidence in
[GAME_MODE_PROGRESS.md](GAME_MODE_PROGRESS.md).

A compiled callback, successful startup, isolated fixture, or decoded setting
does not establish engine parity. Check core engine logic off after its complete
callback/dependency closure, objective transitions, scoring, timers, round
outcomes, and replication behavior are implemented and compared with TU1.5.
Verify those owners against converted tag inputs. Record live integration and
presentation acceptance separately. General conversion, renderer, and weapon
subsystem completion are separate workstreams; only dependencies actually needed
by a mode's engine belong to its core completion gate.

## Shared engine work

- [x] Inventory the original seven families and 39 authored variants.
- [x] Recover the original 304-byte variant and 112 parameter contracts.
- [x] Establish the original 51-slot engine interfaces and CTF/Assault sharing.
- [x] Connect 100 direct TU1.5 callback bodies, 323 reached engine/shared helpers and the authenticated fresh-player continuation to the native dispatcher. Of the callback routes, 92 were missing and eight replace prior canonical aliases.
- [x] Compare the initial cross-mode ABI/state fixtures with unchanged TU1.5 execution.
- [x] Compare all 39 native variant option buffers with the original TU1.5 initializer, setters, and validator.
- [x] Compare occupied King/Oddball/Territories scoring and selected Juggernaut death/role transitions with complete original returns.
- [x] Declare the missing original Juggernaut metadata, mode publication cells, and King group storage, with strict source-data/ownership tests.
- [x] Recover the 43-body .text closure of the remaining 15 direct engine dependencies and retain the five real D3D SDK call boundaries.
- [x] Compare all five built-in Assault variants through full arming thresholds and arm-progress decay, using authored MCC goal markers and real repeated callbacks.
- [x] Recover 26 additional routines reached by eight missing CTF/Assault lifecycle calls, retaining their world and SDK boundaries.
- [x] Compare all five authored Assault fuse boundaries, scoring, readiness delay, and client authority in explicit placed-item states with MCC goals and absent home/effect/damage inputs.
- [x] Compare the preserve-world round-reset branch with empty player/object pools for all seven selectors.
- [x] Compile the original shared engine frame15EBA0 and its 42 missing routines; compare complete client frames for all seven selectors and empty host frames for Slayer/King.
- [x] Correct the one-byte time-extension latch's native access and declare King's missing four-byte geometry operand from authenticated original code/data.
- [x] Run native session setup, activation, and one shared frame for all 39 variants on MCC Lockout with no players. This is an integration slice, not full-match acceptance.
- [x] Move the genuinely born Neutral Bomb through native B95D0 and return it through whole24B950/160DA0 to the exact authored home position after match-over.
- [x] Recover the two ranking callbacks published indirectly by2486F0, their reached team comparator, four original ranking lists, and the nine-case ranking dispatch table; compare 96 complete original/native ranking cases.
- [x] Join real source player records, activate, and execute one shared frame for all 39 variants on MCC Lockout: two players in 34 variants and four in five Juggernaut variants. All 88 current-unit handles remain NONE; this does not qualify spawned players.
- [x] Birth registered CTF/Assault items with occupied teams and compare12 native active-round physical home returns to exact MCC home positions.
- [ ] Compile and connect every original engine callback and required callee.
- [x] Compile the original post-object phase15F450 and 98 reached missing routines; compare pending-state consumption, intermission timers, round-index limits, selected later-round resets and once-only offline finalization with original execution.
- [x] Compile the later1503D0 player phase and50 reached missing routines; compare56 complete host/client empty/absent-unit timer cases with unchanged TU1.5, including zero/one/three boundaries and flag expiry.
- [x] Correct native local player creation's simulation machine key while preserving the real platform-derived player ID. Whole69AD0 now admits the constructed local machine and14FDC0 publishes activation itself.
- [x] Exercise genuine151DA0/B9B70 unit creation and actual weapon loadouts for all39 presets under bounded caller clocks:87 units,88 joined players and the legitimate2 on1 cap waiter. Every permitted unit now spawns on the first delivery in round zero; full-match acceptance remains open.
- [x] Correct fresh-player initialization through the authenticated14E73C post-allocation continuation and original15A660 initial-lives owner. All39 presets, including Elimination, Phantom Elim and Blast Resort, now pass initial-round native spawning. Compare74 additional complete original/native continuation cases.
- [x] Execute the actual pending-state/181-call phase sequence into round two for all 39 variants on MCC Lockout with real joined rosters and actual reset/factory services. This retains a fixed world clock and does not qualify played rounds.
- [x] Request offline match-over through13B4A0 and finalize all 39 native roster variants through15F450; verify the completion latch, result flag, final phase2 and unchanged engine bank across a repeated finalizer call.
- [ ] Initialize/reset the selected engine with real world and player banks.
- [ ] Deliver every editable setting to its actual runtime consumer.
- [ ] Complete team/role assignment, joins, departures, deaths, and respawns.
- [ ] Complete objective item birth, pickup, carrying, dropping, and deletion.
- [x] Preserve the original191DF0 startup order: register the actual roster
  before first-active15AFB0 selects round roles. Single Flag CTF now births
  its flag on the first ordinary frame. All39 prepared presets still pass
  with88 roster records,87 current units and the original2 on1 cap waiter.
- [x] Preserve that first-active lifecycle value through the coupled authority
  checkpoint. Both pre-start and active firing/reload/grenade continuations
  retain matching complete source banks through40 resumed frames; the active
  cut obtains its ammo baseline after genuine source unit birth.
- [x] Admit original neutral-bomb objective row8 in the HUD reader's nine-row
  bank. Compare168 unchanged disc/TU1.5 queries, including eight neutral-row
  cases; verify the actual born Neutral Bomb through normal pickup/hold/drop.
- [x] Qualify normal pickup, attached inventory/native held-item publication,
  holding and trigger-based drop for all15 authored objective presets.
  All six Oddball presets award two points during60 ordinary holding frames;
  all15 preserve matching source/native score banks throughout holding.
- [x] Qualify ordinary authored capture/arming/plant/fuse/score paths for all
  four CTF and five Assault presets, using actual world services and source
  items. Every goal awards one point and releases the actual carried item.
  Explicit initial/goal B95D0 positions do not qualify played approaches,
  disarming/damage acceptance or complete matches.
- [x] Qualify all ten authored King/Territories presets through ordinary solo,
  contested/defended and exit/retake phases, including actual ownership, score
  banks, capture HUD progress and the original neutralization/capture clocks.
  Territory retakes first publish NONE at the authored neutralization threshold,
  then assign the new full player datum after the separate capture interval.
- [x] Qualify the original authored movement clock and next-hill publication
  for all five King presets without supplying timer or geometry results.
  All five preserve the60-second interval, exact rollover and next player phase.
- [x] Extend the Territories flow to every configured authored region across
  all five presets, retaining independent actual source worlds per region.
- [x] Explicitly verify actual territory marker birth/registration, deletion
  on neutralization and new object generation on recapture across those regions.
- [x] Qualify all five Juggernaut presets through ordinary initial selection,
  role time, credited transfer, role/hunter scoring, suicide reselection,
  current-unit respawn and retained departure, with the actual role/membership
  masks, traits and native/source statistics. Declare the shared death-event
  input boundary separately from projectile-hit acceptance.
- [x] Restore original19A430 retained-result startup after simulation-world
  construction, with authenticated header/event storage and actual enabled
  role-time/transfer/kill counters; validate shared startup and finalization
  regressions before qualifying the new recording path.
- [x] Finalize all39 canonical presets with true enabled recording, actual
  19ACB0 completion and23975C result projection, then211 ordinary postgame
  frames each. All88 retained participants and40 occupied team rows reach
  the source projection. Recording stays enabled/frozen, the original header
  completes and the native/source statistics remain equal through8,229 frames.
  The source match-over request remains an explicit13B4A0 stimulus; this does
  not qualify an objective-driven winner or a fully played match.
- [x] Recover the original878C0 recording-recipient predicate and the two
  retained-result comparators19DA10/19DA60 reached indirectly by19DAB0.
  Their three complete bodies contain76 instruction sites. Compare144
  recipient,576 whole19BAA0 award-writer and96 retained-sort ABI cases
  with unchanged original execution, including the tracked arithmetic-flag mask,
  plus51,707 original cold bytes.
- [x] Restore the original7 weapon,2 PvP and32 award descriptors and their
  pointed names. All15 objective carry/drop flows and all9 authored goal
  flows now pass with real recording enabled. All four CTF presets record
  one flag grab; all five Assault presets record one planted-bomb award.
- [x] Qualify one genuine canonical fatal weapon hit in each of the seven
  families through ordinary aim/fire controls, original1F2280 attribution,
  original15F8B0 engine delivery and matching live/retained kill/death banks.
  Seven cases take1,113 ordinary frames, with no vitality, damage, credit
  or callback result supplied. The3 on1 case transfers the role itself.
  Declared initial positions do not qualify all weapons or full matches.
- [ ] Complete engine-owned weapon/equipment selection, traits, spawn rules, and vehicle-option consumers using converted TU1.5 tag contracts.
- [ ] Complete later rounds, round reset, overtime, ties, and match completion.
- [x] Qualify one-point score-driven endings across all seven families:
  both canonical Slayer modes, all15 objective presets, all10 zone presets
  and the3 on1 role-holder case. These28 distinct presets consume only
  parameter2=1 as their rule edit. Ordinary weapon hits, possession, goals,
  capture and periodic zone scoring select the original winner, retain wins,
  and complete postgame or a genuine150/30 reset with replacement units.
  These are declared-position flows, separate from all39 score-limit and
  editable-rule acceptance. Evidence:engine-round-runtime-tu15-20261021,
  runs5,6,7. Source hashes identify each frozen test cohort.
- [x] Exercise all39 authored presets with only original time parameter10
  edited to one second. Verify the engine selects its outcome, single-round
  games finish, and first-to-three presets complete real intermission/reset,
  replacement-unit birth and another natural expiry without a supplied phase.
  All39 pass50 genuine timer-selected tied endings through9,388 ordinary
  frames:28 completed matches and11 continuing-round resets. Each completed
  match includes211 further ordinary frames through retained results/fade.
  The initial observation harness missed internal entries; final coverage
  observes actual15DDC0 stores and the unchanged15DD50 ranking buffer.
  Evidence: `engine-round-runtime-tu15-20261021`, run4. Winning score limits and
  edited round selectors remain separately open below.
- [ ] Qualify engine-driven winning score limits and edited round selectors
  through actual scoring inputs, including accumulated wins and later resets.
- [x] Cover score-limit1 endings for all39 authored presets. All15 objective
  and10 zone presets use ordinary controls/occupancy; all9 Slayer and5
  Juggernaut presets also pass the declared15F8B0 credited-death input before
  actual physical cleanup. That latter14-case boundary does not qualify
  upstream1F2280 projectile/damage attribution or every starting weapon.
  Each case checks the original awarded win and real postgame or continuing
  reset. Evidence:engine-round-runtime-tu15-20261021,runs6,7,10.
- [x] Exercise every original round selector0..6 in canonical Slayer and
  Team Slayer with score-limit parameter2=1. All14 cases pass44 actual
  aim/fire fatal-hit wins,30 original150/30 resets, replacement generations,
  accumulated live/retained wins and14 completed retained-result/fade paths.
  No clock, phase, score, winner, timer or completion result is supplied.
  Evidence:engine-round-runtime-tu15-20261021,run8. This does not establish
  every score-limit value or edited option combination.
- [ ] Qualify CTF/Assault overtime through ordinary held, dropped and returned
  objectives, preserving the authored enable bit, actual extension countdown,
  original announcement latch and final expiry. The authenticated slot25
  hook24A620 is shared by these two families;25BC40 passes the remaining
  clock through in the other five. Original instruction receipts are retained
  in `engine-round-runtime-tu15-20261021/original-overtime-owners.json`.
- [x] Exercise all9 authored enabled CTF/Assault overtime policies through
  ordinary possession beyond the nominal limit, trigger drop and distant
  authored player positions. Original24C490 clears eligibility;24A620
  preserves the five-second window and decrements it once per frame.
  All9 expire after151 delivered frames from that transition, retaining
  149 measured decrements, then complete3 postgames and6 actual resets.
  The original announcement latch is observed; audible announcement delivery
  and complete objective-return variants remain separate. Evidence:run9.
- [x] Disable original overtime parameter29/60 across all9 CTF/Assault
  presets with a five-second time limit. Exact byte audits preserve the
  remaining authored settings. Genuine possession no longer extends the
  clock, and each original tied ending completes postgame or an actual
  continuing-round reset. Evidence:run14.
- [x] Export original engine entity state through whole slot44/45 callbacks
  without writing or advancing the retained world. Validate39 canonical
  presets at two actual frame states each, with caller storage disjoint from
  all world reservations. Preserve the separate Juggernaut relationship
  operand consumed by198680 and compare every native membership pair.
  Unchanged Xbox execution matches20,124 complete returns:156 entity
  constructor/difference calls and19,968 slot27 relationship queries.
  Evidence:run12 andengine-entity-original-comparison.json. The subsequent
  native wire/consumer slice is recorded below; complete network matches and
  remaining engine event producers remain open.
- [x] Carry the original engine entity and actual relationship operand in
  native protocol97, with a distinct owner discriminator for bounded custom
  Defusal. Admit all seven original source-owned host families and use the
  original relationship predicate for source-owned motion-sensor blip types.
  All39 unchanged authored presets pass real loopback UDP:78 admissions,
  authenticated departure with the retained flag2 row/current-unit release,
  new late participants and genuine factory admission across1,521 ordinary
  frames. Complete received snapshots match authority. The source factory
  supplies the late-join wait; no activation or timer is fabricated.
  Existing bounded multiplayer/team/custom Defusal regressions also pass.
  Evidence:runs15,17,18. This is not cross-machine, played network objective,
  rendered/audio or full event-transport acceptance.
- [ ] Complete engine outputs for objective geometry/waypoints, HUD, audio, and results.
- [x] Connect the nine CTF/Assault presets to native objective waypoint/goal
  materials, original recipient/alternate/delay/recent-event audio kernels,
  actual native PCM, and genuine local two-team PCR/Done/fresh-lobby owners.
  See [the consumer evidence](MP_CTF_ASSAULT_CONSUMERS.md) for original CPU,
  actual GPU, complete local match and authenticated packet cohorts. This
  scoped gate does not close other engine families or rendered remote clients.
- [ ] Complete authoritative engine state/event transport and late join recovery.
- [ ] Connect the shared source frame and its authoritative state to the ordinary match scheduler and consumers without duplicating existing Slayer/King adapters.
- [x] Deliver15EBA0 in the source world's ordinary step_objects at original13AB1C before PVS/object updates, with the actual player/input banks. The existing match adapter explicitly retains its separate engine interval pending state/consumer migration.
- [x] Deliver15F450/1503D0 in source-world ticks at original13AB53/13AB58 after effects/lights and before first-person updates and the clock increment.
- [ ] Adopt source-born current-unit identities and their inventory, control and lifetime in ordinary matches.
- [x] Replace ordinary source-world Slayer/Team Slayer spawn selection and repeated equipment setup with whole151DA0, retaining actual activation, unit identity, pose, equipment and completion stores. Preserve separately declared placement inputs. Full seven-mode state ownership remains open.
- [x] Start canonical source-world Slayer/Team Slayer through15AFB0 and consume the actual lifecycle, player lives/timers, score/win banks, retained-stat bytes and finalization latch. Disable the parallel adapter writers on this path; automatic source replacement units are adopted after the world phase.
- [x] Recover whole13B990/18BAC0/228390/32B8C0 and their exact sound-class data. Consume actual15DDC0 winner/NONE inputs, once-only awards, game-over countdown and fade publication. Keep internal generated round-end calls visible at the required-owner boundary.
- [x] Add an original-validated rule projection requiring the retained source engine for all seven selectors. Preserve bounded native-only adapters and declared placements; original runtime owners consume the complete variant instead of the old adapter's option allowlist.
- [x] Move initial canonical player birth into the real frame after roster construction, preserving original spawn admission, queue order and active-unit caps. Read actual membership, King state and camouflage; native relationship/trait queries use the selected original methods.
- [x] Preserve the initial first-person definition setup boundary after that
  genuine source startup frame. The two-player regression attaches actual
  local/replicated definitions and advances the ordinary frame; its formerly
  rejected setup and the missing original branch-table read are resolved.
- [x] Deliver the actual shared match-over request for all39 ordinary prepared
  presets and complete211 normal postgame frames each: once-only finalization,
  stable completed scores/wins, matching native/source countdown and fade,
  unchanged frame ownership and matching clocks. This explicit request does
  not qualify an objective-driven winning outcome.
- [ ] Reconcile the source engine's scores, roles, phases and events with existing match adapters and consumers so each owner advances once.
- [x] Capture actual15EBEE ->1A6830 teleporter outputs during the full source
  frame. Deliver original event22, blocked literal/tick/recipient and the proved
  immediate sound selection to native consumers, and rebase retained input
  through18A2A0. Preserve the bounded adapter's separate single execution.
- [ ] Complete the other source-frame event, HUD message and audio output
  producers, including remaining recipient/alternate/delay/announcer paths.
- [ ] Compare every built-in variant and editable rule group with original TU1.5 engine execution.

## Runtime integration acceptance

- [ ] Connect converted MCC tags and actual world/object/player services to every mode.
- [ ] Exercise local/split-screen and authoritative host/client mode flows.
- [ ] Verify the engine's objective, HUD, audio, and result outputs reach their consumers.
- [ ] Exercise full matches and return to the lobby.
- [x] Exercise all nine original CTF/Assault presets through real local
  sign-in/team selection/countdown, natural objective completion, actual audio
  and waypoint CPU consumers, two-team PCR, world destruction, Done to a fresh
  lobby, and the next Start/cancel. Rendering has separate native GPU evidence;
  placed approach/goal inputs and physical played routes remain distinguished.
- [ ] Finish frontend admission and state/event integration for every original
  family. Protocol97 now transports all seven actual engine entity schemas;
  original Assault and bounded custom Defusal have separate payload owners.
  All39 source-owned host variants pass loopback join/departure/late-join
  snapshot checks. Full frontend flows, other families' network objective
  actions and remaining event/consumer delivery still require acceptance.

## Mode acceptance

| Family / source type | Built-in variants | Current implementation and remaining acceptance |
| --- | --- | --- |
| Slayer / 2 | Slayer; Team Slayer; Team Phantoms; Rockets; Swords; Snipers; Phantoms; Elimination; Phantom Elim | Ordinary FFA/team adapters exist. Finish bonus scoring, remaining settings/sandbox consumers, later rounds, and complete match flows. |
| Oddball / 3 | Oddball; Rocketball; Swordball; Team Ball; Low Ball; Fiesta | All six pass genuine birth, normal pickup, completed carry binding, two periodic points over60 holding frames and trigger drop. Complete remaining ball lifecycle/trait/team rules, presentation, and replication. |
| Juggernaut / 7 | 2 on 1; 3 on 1; Ninjanaut; Phantom Fodder; Dreadnaut | All five pass initial selection, actual role/membership masks and traits, recorded role time/transfer/kill counters, credited transfer, role/hunter scoring, suicide reselection, genuine respawns and retained departure across4,582 ordinary frames. The2 on1 cap also retains its actual waiting/admission handoff. Death events are declared15F8B0 inputs. Complete full damage attribution, edited rules, match outcomes, outputs and replication. |
| King / 4 | King; Team King; Phantom King; Crazy King; Team Crazy King | All five pass ordinary source-world solo/contested/exit scoring and their60-second movement interval, including exact rollover, next hill and following player phase. Complete remaining edited rules, output/presentation integration, full matches and source-mode replication. |
| CTF / 1 | Multi Flag CTF; CTF Classic; 1 Flag CTF; 1 Flag CTF Fast | All four pass ordinary birth/pickup/carry/drop, illegal-side pickup rejection, unattended authored home return and unchanged-preset full score/round wins through retained results/fade. Single Flag first-active role selection follows roster registration. The 54-case edited-rule cohort and both nine-case short/touch-return controls pass. Remaining acceptance covers broader rule combinations, physical played routes, presentation and complete consumer delivery. |
| Assault / 9 | Multi Bomb; Single Bomb; Single Bomb Fast; Neutral Bomb; Blast Resort | All five pass birth/pickup/carry/drop, illegal-side rejection where applicable, unattended home return and interrupted arming/resumed plant/fuse/score. Native and literal-control case reports agree exactly except execution counters. All five pass unchanged-rule complete matches through original wins, retained results and fade, including Multi Bomb's actual between-score respawns. The 54-case edited-rule cohort includes all three bomb kinds and immediate/maximum arming/fuse inputs; nine-case short/touch-return controls pass. Remaining acceptance covers broader placement/detonation branches, rule combinations, physical played routes, presentation and complete consumer delivery. |
| Territories / 8 | 3 Plots; Land Grab; Gold Rush; Control Issues; Contention | All15 configured region/preset pairs pass ordinary capture, defended occupancy, two-stage neutralization/retake, scores/HUD data and actual marker birth, retirement and replacement generation. Complete simultaneous zone ownership, remaining edited rules, outputs, full matches and replication. |

The2026-10-23 [CTF/Assault native rules audit](MP_CTF_ASSAULT_NATIVE_TU15.md)
adds semantic native Assault arming/decay kernels. They pass986 unchanged
original CPU windows with ASan/UBSan and the additional288 complete untraced
CTF/Assault callback comparisons. All9 ordinary return cases and all5 paired
native/reference arming cases pass. Unchanged-preset complete matches pass
across all9 presets; the separate corrected Multi Bomb CTest and initial
short-wait failure are documented there. This bounded kernel conversion does
not close frontend, presentation or network acceptance.

The subsequent [57-routine native conversion](MP_CTF_ASSAULT_NATIVE_CONVERSION.md)
implements the complete inventoried shared CTF/Assault mode closure with native
rule kernels and structured callbacks. It retains original literal controls and
real shared service boundaries. Its stronger production-first component and
paired full-match qualification is tracked separately; earlier runtime receipts
do not automatically qualify that later source cohort.

The 2026-10-24 integration follow-through in that same audit fixes the actual
Slayer-only frontend rejection for original CTF/Assault. Both menu catalogs now
pass 48 original local/System Link Starts apiece, including all three authored
flag/bomb kind-picker choices. Separate complete native/literal matches pass
54 explicit original-rule edits; nine short timed-return and nine touch-return
cases also agree on every reported gameplay field. These declared cohorts
include Neutral Flag CTF without fabricating another stock preset. Broader
rule combinations, played routes and presentation/consumer delivery remain
open. The mode kernels and dispatch files remain identical to their qualified
57-owner source cohort.

All nine stock CTF/Assault matches also pass complete authenticated loopback
UDP objective actions, round resets, late joins and departures across 13,362
replicated frames and 22 remote goals. Two adapter publication fixes preserve
original round results and unit teardown while exposing consistent final-match
winner and player-availability fields. Strict transport validation remains
unchanged. This does not close cross-machine play, event/audio consumers or
the physical played-route and return-to-lobby gates.

The complete native application also builds, and all nine actual menu-to-runtime
handoffs pass with an authenticated MCC map, a genuine signed-in profile, exact
304-byte original option buffers, original team/current-unit ownership, three
ordinary source/effect frames and retained outcomes after world destruction.
That headless startup check does not certify rendered navigation or postgame
session return. The disposable runtime/build owner is removed after validation.

All seven families now pass ordinary prepared-match startup and three subsequent
source frames for all39 presets:88 joined players,87 genuine current units and
the original2 on1 cap waiter. Separate edited-cap cases retain all four roster
entries while admitting one FFA unit or one per team. These checks cover the
shared frame/state handoff. All39 also pass the controlled8,229-frame postgame
cohort, including source finalization and stable completed results. They do not
close the mode acceptance rows above.

The2026-10-21 recording/role follow-through repeats that full39-preset ending
with original recording actually enabled, completed retained result rows and
the original formatter. Its evidence is
`analysis/multiplayer/game-variants/engine-role-runtime-tu15-20261021`.
Validation uses an owned frozen cohort with explicit refresh receipts; the
runtime/test files changed by this pass match that cohort. Source/data
generation independently reproduces the compiled outputs. Concurrent match/network/custom
Defusal and other edits are preserved separately and are outside this claim.
The owner removed its2,816,880 allocated KiB of private source/build/case
storage. No large generated artifacts are retained; cleanup.json and summary.json
record the verification. This does not attribute host free-space changes.

- [ ] Slayer: all nine variants and editable rules accepted.
- [ ] Oddball: all six variants and editable rules accepted.
- [ ] Juggernaut: all five variants and editable rules accepted.
- [ ] King: all five variants and editable rules accepted.
- [ ] CTF: all four variants and editable rules accepted.
- [ ] Assault: all five variants and editable rules accepted.
- [ ] Territories: all five variants and editable rules accepted.

## Current implementation pass

Implemented on 2026-10-20 in
`src/source_multiplayer_engines_tu15_generated.cpp`, generated by
`tools/generate_multiplayer_engines_tu15.py` and connected through
`src/physics_live_world_tu15.cpp` and `cmake/MultiplayerEnginesTu15.cmake`.

The authenticated TU1.5 registration cells provide seven source selectors, six
distinct engine objects, 357 method slots, and 127 distinct callback targets.
The module compiles 100 direct TU1.5 callback targets, 323 reached
engine/shared helpers and one authenticated post-allocation continuation
(424 entries: 423 complete functions and the continuation; 27,942 instruction sites). Existing
named dispatcher modules provide the other 27 callback targets. The current
ownership audit also reads the canonical alias table: 92 callback routes are
newly provided and eight direct TU1.5 bodies replace previous aliases. Three
helper bodies also replace aliases. The earlier named-predicate-only inventory
misclassified those eight callback replacements as missing routes. Every external
call keeps its actual ABI and requires its native service owner.

| Family | Distinct callback targets | Direct TU1.5 bodies | Reused named-owner bodies |
| --- | ---: | ---: | ---: |
| Slayer | 36 | 23 | 13 |
| Oddball | 38 | 29 | 9 |
| Juggernaut | 37 | 28 | 9 |
| King | 37 | 28 | 9 |
| CTF / Assault, shared object | 45 | 27 | 18 |
| Territories | 39 | 30 | 9 |

Targets shared across engines are counted once in the 127-target total.
The offline executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The fresh variant inventory reads all 39 authored rows from `mainmenu.map`;
that file is an offline reference for the variant definitions. Runtime content
will use the MCC-to-TU1.5 conversion described above.

Validation passes:

- Byte-identical fresh source regeneration and independent original-fixture regeneration.
- 1,164 original/native ABI cases across all seven source types, all with
  complete returns and no unresolved service stops. Cases include initialization,
  phase guards, trait selectors, team relationships, Juggernaut team masks,
  entity-bank initialization/difference/application callbacks, and the occupied
  scoring/death cases described below.
- All 39 native 304-byte variant option buffers match the unchanged TU1.5
  initializer, descriptor setters, and validator. Variant initialization and
  trait callbacks run with each of those original buffers.
- The preceding 417-entry snapshot passed seven focused Release CTests: the cross-mode comparison, CTF activation,
  CTF configuration, CTF initialization, King engine, multiplayer cold data,
  and shared CTF/Assault cold data.
- The preceding 420-entry module passed the complete 1,164-case comparison and 17-region cold-data test in Release.
- The complete 1,164-case comparison and 17-region cold-data test with explicitly
  enabled ASan/UBSan in Debug.
- The real native cold-world ownership test in Debug with symbols disabled,
  including exact bytes/access for all 17 regions. Its metadata word
  reads use the shared `Bytes` decoder, keeping that reader a direct static-link
  dependency in unoptimized builds.
- All 39 native MCC Lockout roster/next-round/offline-finalization cases, plus
  the real constructor ownership test, in the final controlled Debug snapshot.
- Fourteen focused Debug CTests for ordinary prepared startup across all39
  presets, FFA/team active caps, shared frame adoption, genuine player movement,
  deaths/respawns, finite lives, declared placements and Slayer/team/tie endings.
  The engine integration evidence uses a frozen source snapshot while unrelated
  renderer/loading work continues in the live checkout.
- A subsequent seven-test Debug cohort passes the all39 ordinary postgame
  cases, actual local/replicated first-person setup, edited FFA/team unit caps
  and the three existing native session/configuration regressions. The postgame
  stimulus is original13B4A0, followed by211 ordinary frames per preset. It is
  separate from objective-driven endings and played matches. All19 immutable
  branch tables in the existing production provider also match the original
  executable byte for byte (577 bytes); the controlled world now mounts that
  provider before local-view execution. This pass adds no generated engine body.
- Compilation of the actual native dispatcher translation unit, plus its
  accompanying CTF initialization check.
- The existing 281-case CTF oracle also passes stricter original read/write
  bank checks (211 full returns, 70 documented external-owner stops). This
  separate historical regression does not enlarge the 1,164-case complete-return matrix.

The fixture currently exercises 192 of its 553 original/dependency entries
(7,361 of 35,266 instruction sites counted within those entries). One entry is
the authenticated fresh-player continuation. Compilation coverage exceeds
exercised behavior coverage. The isolated test composes 81
complete bodies already owned by production services; it supplies no successful callee result. Unknown service
owners fail the test. This pass does not qualify complete core engine parity or
full matches.

The occupied-bank expansion includes 120 King cases covering solo/contested
hills, FFA/team rules, and 30/60-tick boundaries; 12 Oddball score/threshold/limit
cases; 12 Territories owned-zone tick cases; and 18 Juggernaut killer/victim/
suicide role cases. Two active salted player rows and source-valid host options
exercise the actual player iterators and score services. For example, original
Oddball score 119 advancing to 120 sets match-over, and the selected Juggernaut
death cases transfer the role mask from player 0 to player 1. Later phase
transitions, complete role selection, capture/lifecycle, and full round sequences
remain separate acceptance work.

Five additional King initialization cases exercise empty/invalid marker groups,
duplicates, and all eight distinct groups in both orders. The source initializer
filters marker types 11..18, deduplicates them, and publishes the exact count and
two-byte indices. The comparison includes every resulting bank and ordered store.

The dependency pass adds 220 complete cases: 28 authority predicates, 96 Oddball
carrier callbacks, 36 CTF/Assault carrier callbacks, 24 capture-score events,
three vector normalizers, eight player-palette selectors, and 25 Assault arming
sequences. Original trait1 reads bit1 for Oddball and bit7 for CTF/Assault.
Host/client and enabled/disabled inputs reach those distinct original consumers.
Capture-score events test each authored score limit at variant+0x50, including
the next-state request for one-flag CTF and match-over for a third multi-flag
capture. These score events do not establish full capture legality/lifecycle.

Arming sequences execute the complete registered callback repeatedly on the same
banks, using the authenticated MCC Lockout marker rows from the earlier CTF
reference fixture. The original initializer chooses the goal centers/bounds.
At 30 Hz, Multi Bomb, Single Bomb and Single Bomb Fast reach their armed flags
at 150 ticks; Neutral Bomb and Blast Resort reach them at 300. The immediately preceding
ticks remain unarmed. Explicit caller clock/position inputs also move the carrier
out of the region: the original counter falls by 3 per callback and saturates
at -1. These external stimuli are separate from source instruction/store traces;
every intermediate callback and source write executes normally. Bomb birth,
placement, disarming and detonation still require acceptance.

The lifecycle expansion adds 34 complete cases: 14 preserve-world reset calls
(host/client, all seven selectors, empty pools) and 20 placed-item fuse sequences
(all five variants). The latter run 1,360 actual per-item callbacks. Every
authored fuse is three seconds: tick 89 remains at one second/one subtick;
tick 90 clears the timer and planter and awards one player/team point. Single
Bomb, Single Bomb Fast, Neutral Bomb and Blast Resort request the next round;
Multi Bomb continues the round. An item without its ready bit delays expiry;
client execution leaves the authoritative timer and score unchanged. These
are explicit synthetic placed-item caller states with real MCC goal positions,
no eligible home marker and NONE effect/damage tags. They qualify those branches,
not placement, home movement, damage/effect delivery, or a complete round reset.
The full fixture runs 6,200 entry callbacks, including the earlier arming cases.
Of the 26 newly compiled lifecycle routines, this matrix executes two: 50 of
181 sites in preserve-world reset and 88 of 97 sites in placed-bomb goal lookup.
The other 24 have compilation evidence and still need behavioral acceptance.

The direct dependency closures now contain 265 entries: 264 complete .text bodies
and one authenticated continuation. They include the earlier
43, 26 reached through eight missing CTF/Assault lifecycle calls, and the shared
frame's 42 routines plus three reached ranking routines and 98 post-object phase
routines, plus50 later-player routines and the fresh-player15A660/1F2220
functions with the14E73C continuation. They leave six D3D, two DSOUND and seven XONLINE calls as explicit SDK boundaries. The producer
code is compiled; complete indirect world/service/SDK ownership and output
delivery remain unqualified. `sdk-and-alias-ownership.json` records the exact
targets and eleven alias replacements (eight callbacks and three helpers).

The shared frame calls original player iteration, engine callbacks, timers,
cleanup, and phase/result owners. Nine new complete cases cover empty client
frames for all seven selectors and empty host frames for Slayer/King. Both host
cases request the next state and set match-over, matching unchanged original
execution. Those empty cases exercise16 of the initial42 routines. The later
ranking matrix adds96 complete cases over both indirect comparison callbacks,
four original ranking lists, both tie flags, scores-1/0/1 and both directions.
The ordinary `MultiplayerMatch` scheduler still
uses Slayer/King adapters, while the new compiled shared entry is available
through the native world's actual source-call boundary. Scheduler/state-consumer
integration remains a required gate.

Original13AAC0 invokes15F450 after object/light updates. Its 98 newly compiled
dependencies include15EFA0's phase consumer and offline/online result producers.
The 130 new complete cases cover stable host/client phases, requests2/3,
intermission expiry at round indices0/1/30/31, client next-round application,
complete 181-call empty-host sequences for Slayer/King, and repeated offline
finalization in phases1/2/3 for all seven selectors. At 30 Hz, the original
intermission lasts 150 phase deliveries and the next-round delay 30; phase3
increments the round index and phase1 reinitializes the selected engine. At
round index 31, expiry requests match-over. Host objective reset reaches the
actual item factory even without markers; the isolated composition supplies no
factory result. The native world test exercises those real factories instead.

The native round probe delivers state2 through whole1628D0, re-enters 15F450
181 times, and checks phase/timer/index boundaries at calls1/150/151/180/181.
This is a bounded phase sequence at a fixed world clock, not181 full world
ticks or a played round. It then requests match-over through whole13B4A0,
executes15F450, checks its completion latch/result flag, and compares the full
engine bank across a repeated finalizer call. Populated combat, nonzero result
statistics, online results and normal scheduler integration remain acceptance
work.
All 39 variants pass this native sequence: 7,137 complete15F450 entries in total,
88 real joined player records,12 active-round objective home returns and39
once-guard comparisons. Every final phase/request is2. The initial current-unit
handles in that historical round/finalization run are all NONE, so it adds no
player-unit spawning acceptance. The subsequent spawn work is described below.

Original13AAC0 follows15F450 with1503D0. That complete phase and50 missing
dependencies are now compiled. Its56 new complete cases cover all seven
selectors, host/client modes, empty rosters and two occupied absent-unit rows
with timers0/1/3. At1, both modes decrement player16C/3F/40 to zero; only the
host decrements180 and clears its associated high-byte flag. At3, those first
three counters become2 while180 becomes2 on host and remains3 on client. The
comparison includes full banks, registers, ordered stores and instruction order.
Positive current-unit/nearby-object, damage/effect and local input/UI branches
need separate acceptance.

Native player construction previously copied the hardware MAC to both creation
fields8 andE. Original14EBE0 copies those to separate player fields14 and4;
whole69AD0 compares the former with simulation_world+D. The actual standalone
world has its own constructed local key, so the native hardware MAC mismatch
left every player inactive even after240 scheduler deliveries. The corrected
builder obtains that simulation key from its owner and retains the real
hardware-derived12-byte player ID. It supplies no activation/spawn result.
Original13B120/14DF40/14EBE0/69AD0/14FDC0/69FF0 code hashes and native negative/
positive reports are recorded in local-machine-key-contract.json.

The new native spawn test executes14FEC0,15EBA0,15F450 and1503D0 with declared
caller clock inputs. Multi Flag CTF creates two genuine bound units on its
first delivery, through original151DA0/B9B70 and the actual world/tag services.
The reached244460 mode selector also required its exact nine-entry2446B0
spawn-zone flag table, including both empty source-type slots. The regression
independently authenticates all17 data regions and1,429 bytes. Full
object/physics/light tick integration remains a separate required gate.

The authored parameter90 is a simultaneous-unit cap, consumed by1601B0 at
game-options+1B4, not the joined-roster count. In2 on1 its value3 permits three
units and leaves one of the four real joined participants waiting. Native
1617A0's actual161828 store sets exclusion bit4000, and15B470 consumes it.
The probe checks the source-derived global/per-team cap, genuine unit count,
exclusion flag and the actual1601B0 result for each absent participant. The42
new original/native cap cases cover limits3/4/16, team/global counting and all
seven selectors with three current-unit identities and a fourth waiting row.
They preserve complete ABI/bank/store comparisons. See
active-player-limit-contract.json; the engine rule is retained unchanged.

The fresh-player correction removes14EAD0's rejoin notification15A950 from new
record construction. Native DatumPool performs the real allocation, then the
authenticated14E73C continuation consumes that datum in EAX, the creation record
in EBX, and14E736's saved-ESI caller frame. The source continuation owns fresh
record mode, census, simulation registration, AI notification and15A660. Its
15A790 store initializes the WORD at player+1AC from options+1B8, using FFFF for
unlimited lives. No native life-count or activation result is supplied.

The74 additional original/native cases cover all39 presets, lives0/1/3 for
all seven selectors, departed-player and failed-allocation inputs, full banks,
instruction/store order, returned datum and restored ESI/ESP. These isolated
cases declare an already allocated datum and inactive simulation registration;
the actual native world separately executes real allocation and active6A8E0
registration. Whole profile/team services and the original32A660 stack probe
are composed for the comparison, with declared caller stack storage.

The native MCC Lockout cohort now creates87 genuine units for88 joined players,
with one source-imposed cap waiter. All39 presets spawn their permitted units
on the first delivery in round zero and record real held weapon tags.
Elimination, Phantom Elim and Blast Resort start with one life and no longer
wait182 deliveries for a later round. The probe rejects any round-index advance
before spawning. See fresh-player-source-contract.json and native-spawn-world.json.
Ordinary match state/consumer adoption, complete control/physics flows and
network flows remain open.

`PhysicsLiveWorldTu15::step_objects` now runs15EBA0 before PVS/object updates,
then15F450/1503D0 after effects/lights and before first-person work and the clock
increment. Its default owner is the source world. The existing
`MultiplayerObjectRuntime` forwards a per-call `SourceEngineFrameOwnerTu15`;
`MultiplayerMatch` explicitly selects `MatchAdapter` at both world-tick call
sites while its scores, lifecycle, player mirrors and King bank remain separate.
This policy is a call input, adding no checkpointed world state. The adapter
option is an unfinished migration point, not the final seven-mode backend.

The native world-tick test uses actual allocation, active units and automatic
world-clock advancement, with no test-supplied phase result or tick increment.
It checks 13 actual scheduled entry calls and eight original instruction/store
anchors in source-world ticks, versus 10 entries/five anchors when the existing
adapter owns the interval. First-person entry calls retain the original guards;
these raw-world cases do not bind the original local input/view records.
A separate regression constructs a real prepared MCC Slayer match and runs two
ordinary match ticks through `MultiplayerMatch` and `MultiplayerObjectRuntime`,
checking the source-world phases, current units and both clocks. Normal match
state/consumer adoption, remaining original13AAC0 owners, control, complete
matches and all seven ordinary mode admissions remain required.

All 39 source-world cases pass the rebuilt entry/store checks, with 87 actual
units and one legitimate active-cap waiter, spawning on the first delivery in
round zero. Five final Debug CTests pass: the 39-case world cohort, actual
ordinary MCC Slayer adapter ticks, baseline multiplayer behavior and both
session-configuration checks. Every changed compiled input matches the final
workspace byte for byte. The private source/build allocation of 1,948,016 KiB
was removed. No new test binaries, memory banks or captures are retained;
the existing original regression references remain. This world-scheduler change
was not separately sanitizer-tested; the unchanged engine module retains the
preceding 1,164-case Release/ASan/UBSan evidence.

Ordinary source-world Slayer and Team Slayer now use the complete151DA0
player-unit factory when their start bank exactly matches the cache scenario.
Whole14FDC0 supplies real activation when needed;151A20 and the factory own
selection, admission/construction, binding, equipment and completion. Actual
factory failure returns NONE and retains its stores; it does not fall back to
a second placement algorithm. The native match adopts the resulting unit,
source transform/motion/vitality, held equipment, flags and194/198/19A fields.
It verifies the born definition against its admitted biped and does not repeat
binding or equipment creation. Bitwise start/heading and metadata comparison
keeps declared native placements on their existing caller-input path; campaign
and King's separate hill-state adapter are not switched by this step.

In the preceding factory-only pass, lives, respawn admission, scoring and round
state still belonged to MatchAdapter. That unit-adoption change did
not read a stale raw life count back over the adapter's remaining count.
Actual MCC regressions exercise movement commands, genuine native death,
completed replacement-unit adoption and two-life exhaustion/single-round end.
Eight final Debug CTests pass: ordinary MCC adapter ticks, unlimited and
two-life Slayer factory flows, the selected-team two-life flow, declared
placement preservation and three baseline multiplayer/session checks. Each
factory flow observes one actual 151DA0 call during replacement; the final
flows require 179 world ticks after the fatal stimulus. These are bounded
real-engine integration checks, not full match/parity acceptance.
The private source/build allocation of 1,951,064 KiB was removed, retaining only
small reports and the existing reference fixtures. The whole world dispatcher
changed concurrently: its new factory fragment and the other changed inputs
match the tested frozen source. This does not claim a complete build of the
moving worktree or a new sanitizer run.
The full ordinary source-engine state/control/event handoff and the other mode
admissions remain required. Source authority and reports are retained under
`analysis/multiplayer/game-variants/engine-player-adoption-tu15-20261020/`.

### Canonical Slayer engine-state ownership

The subsequent state-ownership pass starts the original15AFB0 owner before
joins and runs15EBA0,15F450,1503D0 and13B990 on ordinary canonical Slayer/Team
Slayer ticks. Native snapshots read the actual phase/ranking/clock bank,
player lives and timers, engine score/win records, retained statistics and
recording bytes, finalization latch, and game-over countdown/fade. Reading the
retained bytes does not enable a recording. The adapter stops independently
advancing those owners. Automatic151DA0 replacements are adopted after the
source world phase. Explicit placements and the other existing adapters keep
their declared owners.

The live dispatcher observes15DDC0's actual EBX winner and completed outcome;
NONE remains a tied/no-winner input. Generated internal calls preserve that
required-owner boundary, so score-limit endings cannot bypass the consumer.
The four recovered game-over routines add257 unique instruction sites and no
new SDK boundary. Their sound-class pointer table and complete names, four
substring selectors, and the five item/corpse retirement-policy pairs are
authenticated original data. The multiplayer declaration now contains59 exact
regions/2,264 bytes. Result-recording bytes and the separate16,448-byte retained
stat bank are declared loader-zeroed storage; original owners retain validity.

The death-flow tests explicitly deliver whole15F8B0 with the original caller's
four-argument shape, then run native D8C20/D8D50/CCF40 fatal cleanup. The physical
helper alone does not deliver an engine death event. These tests establish
engine-event, life/timer, replacement and exhaustion consumption; they do not
establish projectile hit finding or a complete damage-attribution path. The
score-ending tests call the original score writer, verify a single winner
award, and follow actual finalization/countdown/fade; the tie test delivers
original15DDC0 with NONE. New native closure coverage remains separate from the
existing original/native differential cases.

Evidence is retained in
`analysis/multiplayer/game-variants/engine-state-ownership-tu15-20261020/`.
All13 final Debug CTests pass, including all39 raw-world presets (87 units and
the legitimate active-cap waiter), ordinary match clocks/state, player/team/tie
endings, unlimited/two-life replacement and exhaustion, declared placement,
the native cold-world owner and baseline multiplayer/session tests. Fresh
source/oracle regeneration reproduces both compiled compositions and all1,164
complete original returns. The refreshed ABI comparison and59-region cold-data
test pass ASan/UBSan; the native world handoff has Debug integration evidence,
without a new world-wide sanitizer run.

These results qualify the controlled native snapshot. Concurrent audio changes
in the world source/header are preserved in small diffs; the multiplayer frame,
round-end observer, state reader and engine-state declarations match the tested
fragments. The private source/build allocation of2,008,584 KiB and all known
sanitizer trees were verified removed. Only source, small reports and required
reference fixtures remain; no new binaries, raw banks or captures are retained.

Full seven-mode roles/objectives/events, editable-rule admission, physical
combat, transport, HUD/audio/result delivery and full matches remain open.
The remaining native rule adapter still conflates parameter90's active-unit
limit with roster capacity; that initial admission must migrate with the other
mode/rule consumers. No all-variant completion is claimed by this Slayer slice.

The first native MCC frame reached a real ownership failure: whole164EF0 writes
the announcement latch at56E224 (164FAF/164FBB), but the final-startup exporter
had classified that loader-zeroed byte as immutable because startup reads it.
The exporter and production declaration now give that byte its writable cold
owner, preserving the neighboring CRC byte and immutable two-byte padding.
King activation also revealed its unmounted MOVSS operand at46A0D8, loaded by
2C7C86: the original bits are C0C00000 (-6.0f). This is source program metadata,
independent of MCC content conversion.

`tests/multiplayer_engines_world_tu15_tests.cpp` now runs actual native startup,
session delivery, round activation, and whole15EBA0 against imported MCC Lockout
for all 39 original option buffers. Both the historical empty-roster run and the
new occupied-roster run complete in controlled Debug source snapshots. The
occupied run allocates real salted player rows, sanitizes actual profiles through
14BC80 and publishes team membership. The historical cohort used14EAD0/14EBE0;
the current constructor uses the authenticated14E73C fresh-player continuation.
It uses two players for 34 variants and four for all five Juggernaut variants.
All39 remain active after the shared frame. All88 current-unit handles are NONE:
this proves real session/roster integration, not player-unit spawning or control.

The occupied teams cause original factories to birth registered CTF/Assault
items. The probe moves each real body one unit from home through B95D0, then
calls whole24B950 and160DA0. All12 returns reproduce each authored MCC home
position word exactly, during active rounds. The historical empty-roster run
returned only Neutral Bomb after match-over. Carrying/drop/placement, complete
detonation damage/effects and output delivery remain open.

The first occupied Multi Flag CTF frame exposed the unowned comparator248EC0
called indirectly by140BEB. Original2486F0 publishes248EC0 at248798 and248EF0
at248971; the latter reaches248E80. The generator authenticates both immediate
callback publishers and follows the genuine missing closure. Original immutable
lists466164/466170/466180/466188 and the nine-case248E10 dispatch table now have
exact native data owners. Their bounds stop at their actual FF sentinel or
dispatch extent and preserve adjacent owners. These are executable metadata,
independent of converted tags.

The controlled build was needed because concurrent campaign-AI edits changed
CMake/source files during the first attempts. Bounded failure logs remain with
the later passing results. The current occupied-run evidence records the frozen
source hashes, exact changed production inputs and MCC map hash. These results
qualify the controlled snapshot and this engine change, without claiming a full
moving-worktree build. The three concurrent AI cold fields documented by the
earlier shared-frame pass are preserved.

`src/native_tu15_multiplayer_data.cpp` supplies59 exact data owners
to the actual native world constructor: the Juggernaut dispatch cell and full
51-method interface, four mode-state pointer publication destinations, King's
separate group count/eight indices, geometry operand, four ranking lists, and
ranking dispatch table, round-reset response curve channel8, and thirteen
scenario placement capacities and spawn-zone flag dispatch. The later state
handoff adds game-over sound selectors, the54-entry class-name table and its
complete strings, and item/corpse retirement policies. These total2,264 bytes.
Authenticated original section bytes/access and complete initializer stores define their
bounds. Cold loader zeros remain writable until the actual initializers publish
state. Partial overlaps, uncommitted reservations, repeated mounts, immutable
writes, adjacent owners, and authority checkpoint/restore have focused tests.
The existing Slayer and shared CTF/Assault storage owners remain composed.

The comparison preserves GP/XMM registers, x87 values/controls, declared memory,
ordered writes, and instruction order. Both original reads and writes must stay
inside declared banks; sparse input patches round-trip to the full input bytes.
Output flags exclude architecturally
undefined results and conservatively mask variable-shift uncertainty. The
Territories initializer exposed Capstone metadata that treats IMUL's SF as
defined; the oracle applies the older Intel instruction contract, where
SF/ZF/AF/PF are undefined. See the IMUL flags section of the
[Intel instruction reference](https://www.intel.com/content/dam/support/us/en/documents/processors/pentium4/sb/25366621.pdf#page=497).
The existing Unicorn x87/SSE exception-bit exclusions remain explicit in the
oracle report.

Current scheduler evidence is in
`analysis/multiplayer/game-variants/engine-world-scheduler-tu15-20261020/`:
`source-scheduler-contract.json`, the native source-world tick and ordinary
Slayer adapter reports, validation receipts and source hashes. Its cached-test
recompile diagnostic records the superseded executable and the required fresh
source timestamp before the strengthened checks.

The fresh-player ABI evidence is in
`analysis/multiplayer/game-variants/engine-fresh-player-tu15-20261020/`:
`generation-summary.json`, `test-services-generation.json`,
`original-abi-report.json`, `reproduction.json`, and
`validation-and-cleanup.json`. `selected-state-outcomes.json` extracts later
player/cap results and native spawn/loadout/waiting states; `sdk-and-alias-ownership.json` records the
remaining SDK boundaries and corrected ownership classification. The native
spawn/round reports and validation receipts are retained there. The preceding
1,090-case spawn/cap evidence remains in
`analysis/multiplayer/game-variants/engine-player-spawn-tu15-20261020/`.
The preceding
992-case phase/finalization evidence remains in
`analysis/multiplayer/game-variants/engine-post-object-phase-tu15-20261020/`.
The earlier occupied
roster evidence is in
`analysis/multiplayer/game-variants/engine-occupied-world-tu15-20261020/`.
The prior shared
frame reports remain in
`analysis/multiplayer/game-variants/engine-shared-frame-tu15-20261020/`.
Earlier native MCC integration reports are in
`analysis/multiplayer/game-variants/engine-native-world-tu15-20261020/`.
The earlier 757-case lifecycle reports remain in
`analysis/multiplayer/game-variants/engine-objective-lifecycle-tu15-20261020/`.
The earlier
723-case closure and 281-case CTF compatibility evidence remain in
`analysis/multiplayer/game-variants/engine-dependency-closure-tu15-20261020/`.
Cold data and the prior
503-case pass remain in
`analysis/multiplayer/game-variants/engine-state-transitions-tu15-20261020/`.
The initial 150-body/258-case reports remain in
`analysis/multiplayer/game-variants/engine-callbacks-tu15-20261020/` as historical
evidence. The retained regression fixture is
`tests/fixtures/multiplayer_engines_original_tu15.json` (3,001,758 bytes), a
required reference input containing declared stimuli and source result hashes.
Private CMake builds and oracle scratch were verified removed; no large test
binaries, memory banks, or captures were retained.

The initial optimized cold-world build was interrupted in favor of the faster
Debug data check. The first Debug link exposed the reader dependency described
above; the corrected real constructor test passed. Both earlier private trees
were removed, with bounded diagnostic receipts retained beside the final results.

## Next engine parity work

- [ ] Replace ordinary `MatchAdapter` ownership with the source world after
  adopting its current units, scores, lives/timers, roles, lifecycle and event
  outputs in the actual gameplay and presentation consumers. Remove family
  admission gates only with that migration and its complete mode-flow checks.
- [ ] Qualify the post-object phase's remaining occupied-state, online-result,
  overtime/tie and full round-outcome paths. The prior post-engine-phase-audit.json
  identified the 98 missing routines now compiled; their compilation alone does
  not qualify every branch or the seven additional SDK boundaries.
- [ ] Connect actual source player-unit spawning/control and15EBA0/15F450/1503D0
  to the ordinary match scheduler, preserving original object/phase order.
  The source world now owns the three phases by default. Ordinary match adapters
  explicitly retain their existing interval until their player/unit banks,
  scores, lifecycle and event consumers migrate. The current
  source-scheduler-contract.json records the original call positions.
  The source audit in player-scheduler-next-frontier.json records the existing
  whole14FDC0/14FEC0/151DA0/160780 owners and identified the later1503D0 player
  phase now compiled. The new spawn probe exercises those owners; normal frame
  scheduling must preserve their order with actual object/physics/light phases.
- [ ] Expand original/native fixtures to occupied player/objective banks,
  all mode-specific settings, deaths, departures, timer boundaries, and scoring.
- [ ] Classify each reached shared dependency as an existing production owner
  to compose or a missing owner to implement; compare the composed behavior.
- [ ] Qualify complete Oddball carrier/score transitions, Juggernaut role
  transfers, King occupancy/movement, CTF returns/captures, Assault arming and
  detonation, Territories capture/contest, and shared Slayer/round outcomes.
- [ ] Consume converted MCC-to-TU1.5 tag rows through their original engine
  field/reference contracts and verify every engine-owned setting consumer.
- [ ] Complete the remaining source engine-bank/wire/event transitions and
  qualify them through the actual selected native engine and world services.

Focused validation commands:

```bash
python3 tools/run_tests.py --config Debug \
  '--cmake-arg=-DCMAKE_CXX_FLAGS_DEBUG=-O0 -g0' \
  '--cmake-arg=-DCMAKE_C_FLAGS_DEBUG=-O0 -g0' \
  --cmake-arg=-DHALO2_MCC_TELEPORTER_MAP=/path/to/MCC/beavercreek.map \
  --target multiplayer_engine_frame_events_tu15_tests \
  --target teleporter_mcc_tu15_tests --target teleporter_blocked_notice_tu15_tests \
  --filter '^(native_mcc_multiplayer_frame_events_tu15|native_mcc_teleporter_(traversal|blocked_notice|approach))$' \
  --retain-report multiplayer-engine-frame-events.json \
  --output build/multiplayer-engine-frame-events --jobs 4 --test-jobs 1
python3 tools/run_tests.py --config Debug \
  '--cmake-arg=-DCMAKE_CXX_FLAGS_DEBUG=-O0 -g0' \
  '--cmake-arg=-DCMAKE_C_FLAGS_DEBUG=-O0 -g0' \
  --cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=/path/to/MCC/lockout.map \
  --target multiplayer_engines_world_tu15_tests \
  --filter '^native_mcc_multiplayer_prepared_.*_tu15$' \
  --retain-report multiplayer-prepared-engines.json \
  --retain-report multiplayer-prepared-finalization.json \
  --retain-report multiplayer-prepared-views.json \
  --retain-report multiplayer-prepared-ffa-cap.json \
  --retain-report multiplayer-prepared-team-cap.json \
  --output build/multiplayer-engine-prepared --jobs 4 --test-jobs 1
python3 tools/run_tests.py \
  --target multiplayer_engines_tu15_tests \
  --target ctf_activation_tu15_tests --target ctf_configuration_tu15_tests \
  --target ctf_initialization_tests --target king_engine_tests \
  --filter '^(original_multiplayer_engines_tu15|original_ctf_activation_tu15|original_ctf_configuration_tu15|original_ctf_initialization|original_king_engine)$' \
  --output build/multiplayer-engine-shared-frame-tests-20261020-final --jobs 3
python3 tools/run_tests.py --sanitizers --config Debug \
  --target multiplayer_engines_tu15_tests \
  --filter '^original_multiplayer_engines_tu15$' \
  --output build/multiplayer-engine-shared-frame-sanitizers-20261020-final --jobs 2
python3 tools/run_tests.py --config Debug \
  '--cmake-arg=-DCMAKE_CXX_FLAGS_DEBUG=-O0 -g0' \
  '--cmake-arg=-DCMAKE_C_FLAGS_DEBUG=-O0 -g0' \
  --cmake-arg=-DHALO2_MCC_MULTIPLAYER_ENGINE_MAP=/path/to/MCC/lockout.map \
  --target multiplayer_engines_world_tu15_tests \
  --filter '^native_mcc_multiplayer_engines_rounds_tu15$' \
  --retain-report multiplayer-engines-rounds-world.json \
  --output build/multiplayer-engine-native-world --jobs 4
python3 tools/run_tests.py \
  --target src/physics_live_world_tu15.cpp.o --target ctf_initialization_tests \
  --filter '^original_ctf_initialization$' \
  --output build/multiplayer-engine-dispatch-compile-20261020 --jobs 2
python3 tools/run_tests.py \
  --target native_tu15_multiplayer_data_tests --target native_tu15_ctf_static_data_tests \
  --filter '^(native_tu15_multiplayer_data|native_tu15_ctf_static_data)$' \
  --output build/multiplayer-engine-cold-data-20261020 --jobs 2
python3 tools/run_tests.py --sanitizers --config Debug \
  --target native_tu15_multiplayer_data_tests --filter '^native_tu15_multiplayer_data$' \
  --output build/multiplayer-engine-cold-data-sanitizers-20261020 --jobs 2
python3 tools/run_tests.py --config Debug \
  '--cmake-arg=-DCMAKE_CXX_FLAGS_DEBUG=-O0 -g0' \
  '--cmake-arg=-DCMAKE_C_FLAGS_DEBUG=-O0 -g0' \
  --target native_tu15_world_memory_tests \
  --filter '^native_tu15_world_memory_ownership$' \
  --output build/multiplayer-engine-cold-world-debug-20261020-v2 --jobs 4
```

To reproduce source and fixture in one owned oracle lifecycle:

```bash
python3 tools/run_test.py -- .tools/python/bin/python - <<'PY'
import os, subprocess, sys
from pathlib import Path
root = Path(os.environ['HALO2_TEST_SCRATCH'])
subprocess.run([sys.executable, 'tools/generate_multiplayer_engines_tu15.py',
    '--directory', str(root / 'generation'),
    '--output', str(root / 'generated.cpp')], check=True)
subprocess.run([sys.executable, 'tools/generate_multiplayer_engines_tu15.py',
    '--test-services', '--directory', str(root / 'generation/test-services'),
    '--output', str(root / 'services.cpp')], check=True)
subprocess.run([sys.executable, 'tools/oracle_multiplayer_engines_tu15.py',
    '--directory', str(root / 'generation'),
    '--fixture', str(root / 'fixture.json'),
    '--report', str(root / 'report.json')], check=True)
assert (root / 'generated.cpp').read_bytes() == Path(
    'src/source_multiplayer_engines_tu15_generated.cpp').read_bytes()
assert (root / 'services.cpp').read_bytes() == Path(
    'tests/multiplayer_engine_services_tu15_generated.cpp').read_bytes()
assert (root / 'fixture.json').read_bytes() == Path(
    'tests/fixtures/multiplayer_engines_original_tu15.json').read_bytes()
PY
```

## Validation and storage

Use `tools/run_tests.py` for CMake/CTest and `tools/run_test.py` for original CPU
oracles or one-off probes. Generated binaries, banks, and intermediate listings
belong under `HALO2_TEST_SCRATCH`; retain only small provenance/results reports.
Record commands, passed/failed/skipped checks, unresolved runtime owners, and
verified scratch cleanup here after each implementation pass. Retain no large
test artifacts unless explicitly requested or required as reference inputs.

The prepared-mode pass retains its bounded results in
[`validation-summary.json`](../analysis/multiplayer/game-variants/engine-prepared-modes-tu15-20261020/validation-summary.json),
with commands, frozen/live source hashes and exact concurrent-work differences
in the same directory. The world checks are Debug native integration checks;
they do not add full-world sanitizer or rendered/live-match acceptance.
[`storage-lifecycle.json`](../analysis/multiplayer/game-variants/engine-prepared-modes-tu15-20261020/storage-lifecycle.json)
records the owned source/build removal and intentionally retained reports.

The subsequent source-frame event pass observes the original1A6830 call rather
than invoking it again. A focused Beaver Creek test uses genuine canonical
player birth and explicit initial position inputs through originalB95D0. Two
local views and a recipient with no local viewport each complete27 ordinary
frames, two actual event22 activations, an occupied-exit wait and release by
ordinary blocker movement. It compares original local HUD literals/ticks/flags,
the source sound submission, native serials, exit-heading preservation and
exactly one phase per frame. Position inputs do not establish a played approach
route; rendered/audible and full network acceptance remain separate.

The bridge's original helper calls use a bounded caller frame below the active
source ESP, preserving the still-running shared frame. Read-only observation
only copies native outputs. The generated engine/teleporter bodies are unchanged.
Original call/store bytes, source identities and bounded validation reports are
retained under `analysis/multiplayer/game-variants/engine-frame-events-tu15-20261020`.
All15 Debug regression CTests pass, including declared-placement traversal,
blocked notices and walking onto the pad, the original HUD-message fixture,
all39 canonical prepared startups, view/cap checks and player/team/tie endings.
The final focused rerun also compares the entire live caller span at actual
15EBEE/15EBF9 and proves preservation in all54 ordinary frames. This is native
integration evidence for the recorded controlled snapshot; concurrent live
input-targeting/renderer/native-world changes remain separately identified.

The objective runtime pass retains its commands, original instruction receipts,
owned production diff, source identities and bounded results under
`analysis/multiplayer/game-variants/engine-objective-runtime-tu15-20261020`.
All15 carry/drop cases complete1,590 ordinary frames, including900 holding
frames. All nine capture/plant cases complete2,092 ordinary frames; the five
Assault presets use their authored five/ten-second arm times and three-second
fuses. Score/stat mirrors agree and each goal awards one point. The generated
engine bodies and1,164-case fixture remain unchanged. Ten distinct Debug
CTests pass across the final startup/objective/continuation cohorts. The
pre-start continuation passes in run14; its active-cut ammo-baseline setup is
corrected and passes in run15. Earlier failed setup/admission cases remain in
the retained diagnostics. This is ordinary native integration with declared
placement inputs, not full edited-rule, played-match, physical-output or
network acceptance.

The objective pass's `storage-lifecycle.json` confirms removal of its owned
source/build tree and2,406,772 allocated KiB. All nested CTest scratch is gone;
only small commands, hashes, instruction receipts and diagnostic/result reports
remain. No new large binaries, raw banks or captures are retained. The frozen
validation snapshot and17 concurrent live-source differences are recorded;
the independently added receipt-capture locomotion timing field is preserved.

The zone pass verifies30 controlled cases across20,710 ordinary source frames:
all ten King/Territories presets, all five initial King hill relocations and
all15 configured Territory region/preset pairs. Its final region sweep checks
actual flag marker construction, registration, deletion at neutralization and
replacement by a new full object generation. The original engine translation
and ABI fixture remain unchanged; this pass adds integration evidence and
CTest coverage without substituting state outputs. Simultaneous zone ownership,
broader edited rules, events/presentation, full matches and replication remain
open. Commands, source identities, original instructions and bounded reports
are retained under `analysis/multiplayer/game-variants/engine-zone-runtime-tu15-20261020`.
The owned source/build tree is verified removed (1,822,716 allocated KiB), with
no new large binaries, raw banks or captures retained. Twelve concurrent live
source differences are recorded and preserved.

## CTF/Assault presentation follow-through, 2026-10-24

[Native consumers and session return](MP_CTF_ASSAULT_CONSUMERS.md) supersedes the
earlier separate consumer gates for these nine stock presets only. The remaining
family-wide unchecked gates above retain their broader scope.

- [x] Preserve the 57 qualified native objective rule owners and their 33 source
  files; no executable payload is added to the runtime.
- [x] Render real remote waypoint packets for all nine presets, using canonical
  resource metadata and exact replicated source coverage bounds. The final
  cohort submits 1,080 native Vulkan frames and 14,838 waypoint vertices.
- [x] Complete all nine local two-player flows through native PCM, retained team
  results, world release, PCR Done, fresh lobby and next Start/cancel.
- [x] Complete all nine two-process ONLINE flows through real encrypted peer
  admission, remote native PCM, unanimously stored original team results, world
  release, PCR Done and fresh admission. Preserve identity, family and team
  validation in the result service.
- [x] Build the full native application and pass all eight focused CTests, plus
  16 service transport/result tests. Commands and exact source/report hashes are
  under `build/ctf-assault-network-consumers-20261024/acceptance.json`.
- [ ] Retain a genuine System Link lobby/session across world teardown and
  complete its PCR-to-lobby return. The current LAN transport borrows the match;
  the local offline session is not an admissible substitute.

All edited rule combinations, cross-machine/physical Xbox comparisons, migration
and launcher installation retain their own acceptance gates. The rendered
metadata-only replica does not acquire local collision/gameplay authority.
