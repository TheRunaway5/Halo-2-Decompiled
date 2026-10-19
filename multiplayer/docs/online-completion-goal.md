# Complete Halo 2 ONLINE

User-requested consolidated goal, 2026-10-13. This is the acceptance checklist
for all ONLINE work discussed in this chat, including the subsequent direct
peer architecture and modern mesh requirements. Earlier narrower completion
claims do not close the remaining items below.

The full scope is registered as the active app goal. This document is its
tracked acceptance checklist; source changes and bounded test passes do not
close requirements that still need integration or physical validation.

## Working checklist and goal completion contract

User-requested tracking update, 2026-10-19. This is the authoritative working
list for the existing active goal, whose objective already requires completing
every requirement in this document. Work includes keeping this list current
and checking off each item against its acceptance evidence. The goal must stay
open until every required item below is checked. The later progress notes and
area table preserve supporting evidence; they are not separate completion lists.

These are remaining integration, delivery and acceptance gates. An unchecked
item can already have substantial implementation and passing component tests;
it does not mean that work must be rewritten. Previously verified milestones,
including the user's real Steam login and successful two-way party voice test,
remain recorded. Checkboxes describe final acceptance, not a percentage estimate.
Steam primary login and `master.halo2forever.net` supersede the earlier identity
and hostname choices; optional verified Discord linking/social access remains.
The user's 2026-10-19 clarification also makes installed MCC the asset source
for the Vista ONLINE/clan UI. Extract the available authored banks through
MCC-only onboarding; a separate Vista installation/import is not a release
requirement. Earlier Vista donor checks remain reference evidence, not proof
that the final MCC-only ONLINE/clan path has passed.

Checkoff rules:

- Keep each stable item ID when updating its description or evidence. Record
  the completion date, source/build or release identity, evidence link and
  validation scope beside a checked item. A code change alone cannot close an
  item that also requires integration, delivery or a real-user check.
- Update the current task, completed items, next dependency and blockers at
  each work milestone. Refer to these IDs in progress updates so detailed
  subsystem work stays attached to a remaining outcome.
- Keep component tests, native runtime tests, visible game-window checks,
  physical devices and independent-PC/WAN checks distinct. If required hardware
  or human participation is unavailable, record the missing check and leave
  that item open while continuing independent work.
- Reopen an affected item when later changes invalidate its evidence. Resolve
  missing original behavior with source/public evidence or an explicit user
  scope decision; do not silently substitute invented behavior or remove gates.
- Preserve TU1.5 gameplay/network authority, Vista ONLINE/clan assets, direct
  player-hosted gameplay/voice, guest privacy and test-storage cleanup throughout.

Current task: **M02**, prepare complete world-borrower adoption using the now
qualified owning receipt, runtime/event frontier and deferred-cue history.
M02 remains unchecked; retaining the input to adoption does not adopt the runtime.
**M01** is checked below with its dated GPU/native evidence and verified cleanup. Next
dependencies remain **M03–M06**, presentation/input/audio continuity, bounded
retention and all-survivor readiness, followed by **M07**, the actual windowed
migration path. Migration remains disabled by default until those prerequisites
and the relevant network/voice gates pass. The [next production dependency](../../.local/runtime-receipt-frontier-20261019/next-adoption.md)
and earlier [adoption inventory](../../.local/lens-deferred-retirement-20261019/next-adoption-inventory.md)
record missing owners and dependency order; they are inspection evidence,
not completed runtime adoption. U01/D01 separately track the MCC-only UI
metadata/resource-bank generation and selector wiring found in the latest audit.

### M — Host migration and disconnect survival

- [x] **M01 — Deferred GPU retirement.** Prepared publication performs no
  Vulkan wait/destruction inside the locked commit; cancellation and stale
  preparations preserve old results, and retained GPU owners retire safely
  afterward on the correct executor. Qualified 2026-10-19 against native build
  `d76560c98534` (full hash/source manifest in the [completion receipt](../../.local/lens-deferred-retirement-20261019/verification.json)):
  12 focused CTests, real Vulkan publication/deferred-retirement observations,
  nine GCC syntax checks including both particle configurations, 17 matching
  owner hashes and verified scratch removal. This closes this component gate;
  actual windowed migration, active-flare continuity and hardware/WAN acceptance
  remain separate unchecked items.
- [ ] **M02 — Complete world adoption.** Prepare and atomically adopt every
  runtime borrower, source/model/particle pointer, callback, event frontier and
  cache against the exact validated world. Preserve the complete TU1.5 state,
  salted identities, inventory, RNG, timers, physics, projectiles, grenades,
  props, devices, match/results and local profiles; reject partial adoption.
  Partial milestone, 2026-10-19: exact owning decoded receipts, local slots,
  runtime/event bindings and freshly captured pending cues now share one
  proof-bound history entry. Six normal tests, four selected-owner sanitizer
  tests and nine compiler checks pass with verified cleanup; see the
  [receipt](../../.local/runtime-receipt-frontier-20261019/verification.json).
  Complete borrower/scene/HUD/input/playing-audio adoption remains open.
- [ ] **M03 — Presentation continuity.** Complete exact scene/model/first-person,
  camera, pose, lighting, HUD, culling and active-effect history adoption.
  Verify active gameplay across the handoff; cold reset component controls
  alone do not establish continuity.
- [ ] **M04 — Input and playing audio.** Adopt all four local seats/controllers,
  retained/released inputs, pending and already-playing game sounds and voice
  device ownership without stale-generation playback or duplicated actions.
- [ ] **M05 — Sustainable validated cuts.** Qualify bounded history/checkpoint
  memory and sustained transfer/replay catch-up under live gameplay; retain the
  exact matching native/presentation cut without unbounded backlog or memory.
- [ ] **M06 — All-survivor readiness.** Advertise only actual complete native,
  presentation, input and audio readiness; agree membership/cut/epoch before
  promotion, preserve retry/cancellation, and reject stale or incomplete owners.
- [ ] **M07 — Windowed runtime activation.** Connect the complete prepared
  adoption to normal host promotion and client endpoint rebind. Demonstrate an
  actual rendered match continuing after host departure with surviving players
  controlling it and hearing direct voice, then qualify default enablement.
- [ ] **M08 — Ordinary disconnect survival.** Finish nonhost leave/timeout and
  admission cancellation across supported native/custom modes. Preserve the
  remaining room, retained player/PCR rows and deferred source departures;
  resolve missing-peer confirmation/arbitration without fabricated deaths.
- [ ] **M09 — Repeated failure coverage.** Verify graceful exit, process kill,
  host silence/recovery, repeated replacement-host loss, interrupted takeover,
  couch-host departure and final-player departure across supported modes/maps.
  No stale authority, duplicated results or unintended room termination.

### N — Direct peers, host selection and mesh hardening

- [ ] **N01 — Automatic measured host policy.** Finish reciprocal reachability,
  latency/loss, upload and measured NAT compatibility across physical PCs;
  choose a host that reaches every owner and reevaluate surviving candidates.
  Keep party leader, player seat and gameplay host distinct.
- [ ] **N02 — Decentralized telemetry.** Continuously exchange bounded,
  authenticated peer matrices on the gameplay path, agree telemetry/membership
  generations and conservatively disqualify asymmetric-loss host candidates.
  Unknown NAT or path failure is not proof of cheating.
- [ ] **N03 — Host-silence quorum.** Integrate the requested greater-than-500ms
  suspicion probe, authenticated strict-majority nonhost votes, generation and
  membership fencing, replay rejection and safe stale-peer removal. Qualify
  false alarms, partitions and split-brain prevention in the real runtime.
- [ ] **N04 — Validated shadow queue.** Continuously and deterministically rank
  eligible peers by mesh latency/NAT health with stable ties; retain a complete
  validated successor and measure actual authority/transport transition time.
  An endpoint switch alone cannot satisfy seamless world continuation.
- [ ] **N05 — Outage and compatibility.** Verify direct match/voice survival
  through temporary signaling loss and master process restart, persistent
  admission and retry, negotiated protocol compatibility and old-epoch rejection.
  Confirm no default master/TURN gameplay or match-media relay is used.

### V — Voice coverage

- [ ] **V01 — Party/clan direct voice.** Complete the remaining menu voice
  transport conversion and verify joining, leaving, leadership changes and
  transition into/out of direct player-hosted match voice.
- [ ] **V02 — Original audiences.** Qualify proximity, team/FFA radio, living/dead
  audiences and death grace, mute/block preferences and postgame talk audiences
  in supported modes, including after host migration.
- [ ] **V03 — Every session phase.** Complete source-supported pregame,
  all-participant loading, in-game, postgame and LAN voice coverage.
- [ ] **V04 — Original audio behavior and link quality.** Finish supported radio
  audio/masking behavior and verify real encode/decode, bounded queues, reconnect,
  impaired links and stale-frame rejection without a master media fallback.
- [ ] **V05 — Physical couch audio.** Complete and test independent local
  microphones/headsets and per-seat routing with real controllers/audio devices;
  synthetic encoded packets do not close this gate.

### F — Native entry points, identity, friends and clans

- [ ] **F01 — Community-server joining.** Connect discovery/details to secure
  native admission and actual gameplay, with matching variant/map checks,
  capacity/errors, direct transport and clean return/requeue.
- [ ] **F02 — Quickmatch/Optimatch/Playlists.** Regress all native entry points
  through real admission, loading, play, PCR, Done and requeue with accepted
  parties and the delivered identity/transport/migration changes.
- [ ] **F03 — Identity regression.** Preserve real Steam default-browser login,
  cancellation/sign-out, hidden IDs, chosen Halo names and verified secondary
  accounts. Verify optional dual-proof Discord linking retains clan/history
  ownership and rejects collisions without enabling unauthorized social access.
- [ ] **F04 — Live Discord friends/invites.** Finish live linked-account friends,
  presence, invitation send/receive/accept, cold launch and party return with
  two real accounts; verify block changes, expiry, revocation and failed sends.
- [ ] **F05 — Party lifecycle.** Verify native leader/follower, full/scrolled
  roster, invitations, withdrawal, explicit playlist consent, launch, migration
  and requeue without losing accepted members.
- [ ] **F06 — Complete clans.** Verify create/join/invite/manage/roles/messages/
  leave/dissolve, Minor/Major lineup admission, clan matchmaking, progression
  and retained audit/history with real users. Original clan-rank policy/artwork
  also requires P05 and U03.

### U — Vista ONLINE and clan UI

- [ ] **U01 — Asset and privacy audit.** Verify every ONLINE/clan screen uses
  the high-resolution Vista UI fonts/bitmaps available in MCC, extracted through
  the MCC-only asset pipeline with source provenance, `ONLINE` capitalization,
  hidden account IDs and chosen screen names, without the obstructing title logo.
  Complete the MCC catalogue's missing runtime sampling pack and widget/locale/
  menu-source metadata admission, then connect ONLINE/clan selection to that
  MCC-owned bank; the existing painter's older Vista-root selection does not
  satisfy this gate.
  Check the actual MCC inventory for every required bank and keep missing or
  changed resources explicit; do not silently require a separate Vista donor.
  The [bounded wiring audit](../../.local/runtime-receipt-frontier-20261019/mcc-ui-wiring.md)
  records the current catalogue/metadata/painter gaps; it is not visual acceptance.
- [ ] **U02 — Widescreen and real dimensions.** Regress actual clan/party/menu/
  PCR geometry, long/scrolled rosters, hit targets and controller navigation at
  the supported aspect/scale matrix in the delivered build; verify on screen.
- [ ] **U03 — Remaining original rendering.** Finish source-supported Vista
  matchmaking progress-ring materials/animation, PCR emblem shaders and original
  clan-rank presentation within the real screen geometry.

### P — Playlists, modes and competitive rules

- [ ] **P01 — Final-2010 catalog.** Restore and version the fullest primary-
  evidence-supported final Xbox Live rotations. Identify unrecovered downloaded
  configuration explicitly; obtain missing evidence or a user scope decision
  before claiming full historical restoration.
- [ ] **P02 — Original matching behavior.** Finish unresolved service ordering,
  query pagination, party/team balancing and direct-path/QoS policy against
  recovered TU1.5 code and primary explanations, including Max Hoberman's.
  Regress casual, ranked and clan gathering/launch with actual admitted parties.
- [ ] **P03 — Native mode coverage.** Complete required objective/game engines
  and selected variant settings, scoring, teams, overtime/end conditions, PCR
  and migration behavior before enabling their historical pools.
- [ ] **P04 — Custom party modes and Tombstone.** Regress authored Fiesta Slayer,
  green-zombie Infection Slayer and Action Sack under direct transport and
  migration; bundle definitions in `data/tags` and releases. Add Tombstone to
  suitable supported Slayer/Team Slayer/CTF/Fiesta and other evidenced pools
  only when the corresponding native engine is accepted.
- [ ] **P05 — Competitive completion.** Finish evidence-supported quit penalties,
  later player/clan ranking and original rank-update/display owners. Version
  policies and verify exactly-once persistent progression across disconnects,
  migration and report retries without inventing missing historical values.

### C — Four-player ONLINE couch play and privacy

- [ ] **C01 — Physical four-seat play.** Verify four local players on one PC can
  join/queue/play against another PC with correct controllers, view ownership,
  HUDs, profiles, scoreboards and widescreen layout.
- [ ] **C02 — Mixed verified/guest identities.** Verify independent secondary
  identities and guests through admission, migration, PCR and requeue. Keep all
  local PCR rows; publish no personal stats/history for unverified guest seats,
  regardless of client-supplied flags or display names.
- [ ] **C03 — Couch lifecycle.** Qualify supported local-seat join/leave,
  controller disconnect/reconnect and primary/host departure without borrowing
  another account's stats, input or voice ownership. Audio also requires V05.

### R — Organized post-carnage database and recovery

- [ ] **R01 — Complete after-match records.** Validate final admission, retained
  departures, result digests/confirmations and normalized full report details
  after normal and migrated matches; do not publish unfinished matches as final.
- [ ] **R02 — Safe retry and persistence.** Verify transactional/idempotent
  report/progression writes, missing-peer arbitration, master restart, offline
  retry and database backup/restore without duplicate or lost accepted results.
- [ ] **R03 — Future website lookup.** Preserve privacy-safe report APIs,
  pagination, stable public aliases, verified linked Discord username lookup
  and old summary-only records. Keep Steam personas separate from identity keys;
  the future website itself is outside this completion request.

### D — Delivery, real-world acceptance and cleanup

- [ ] **D01 — Coherent release build.** Build a consistent source/dependency
  cohort and run focused regressions for affected paths; package authored
  variants, official SDK dependencies and required notices. Verify fresh MCC-only
  onboarding supplies all required ONLINE/clan UI assets without a Vista install
  or historical donor registry; preserve installed MCC files and source manifests.
- [ ] **D02 — Both launchers.** Install the qualified native runtime, verify
  each launcher's selected paths/hashes and retained asset banks, and repeat
  startup/login/ONLINE smoke checks. Document and resolve any remaining
  gameplay-readiness prerequisite before claiming delivered playable ONLINE.
- [ ] **D03 — Safe backend deployment.** Verify retained state, private secrets,
  schema/integrity, isolated backup restoration, actual running release hashes
  and HTTPS health on `master.halo2forever.net` before/after activation.
- [ ] **D04 — Independent-PC/WAN acceptance.** Complete real matchmaking,
  community join, couch gameplay, graceful/abrupt host migration, direct voice,
  PCR and requeue across independent consumer NATs. Record latency/loss/NAT and
  host-quality results separately from localhost and VPS probes.
- [ ] **D05 — User-visible/hardware acceptance.** Verify rendered migration,
  Vista/clan widescreen screens, real social/clan flows, physical controllers
  and bidirectional headset voice in the qualified release. Keep unavailable
  required participation explicit and unchecked.
- [ ] **D06 — Test-storage cleanup.** Use the required owned scratch lifecycle;
  audit/remove unused generated executables, objects/build trees and captures
  after retaining bounded diagnostics. Verify absence and report reclaimed
  storage/explicitly retained large artifacts; preserve source, references,
  launcher runtimes and unrelated active work.
- [ ] **D07 — Final checklist audit.** Reconcile every item above with dated
  evidence and the installed/deployed release, verify earlier accepted behavior
  has not regressed, and resolve every required open gate. Only then mark the
  existing full ONLINE goal complete.

Cleanup is part of completion, including each intermediate validation run:
delete unused generated test executables, object/archive copies, private build
trees and scratch captures after retaining bounded diagnostics. Verify their
removal before reporting a step finished. Test source and required original
reference evidence remain part of the project; do not delete them to reclaim
storage. Audit older generated artifacts before removing them so active builds,
installed runtimes and concurrent work remain intact.
The user's latest continuation reaffirms this cleanup requirement for the final
delivery as well as intermediate tests; record reclaimed storage and any
explicitly retained large artifacts in the completion evidence.

Additional cleanup on this continuation: 44 unused, untracked generated ELF
test executables were removed from `build-multiplayer`, reclaiming
5,703,429,208 logical bytes (5,703,528,448 allocated bytes). Both launchers still
select the preserved `build-multiplayer/halo2_native`; its inode, size and
modification time are unchanged. The audited files had no observed live users
or active builds in that directory. No test sources, libraries, assets,
reference inputs, frozen source or concurrent scratch were removed. Evidence:
`.local/promoted-input-continuity-20261018/old-test-cleanup.json`.

## Required architecture

- **Xbox TU1.5 is the sole gameplay and networking parity target** (user confirmation,
  2026-10-14). Earlier-image decompiles are supporting evidence only where their
  correspondence to TU1.5 is established. TU1.1 construction/compatibility is
  not an ONLINE completion requirement. The selected high-resolution Vista
  ONLINE/clan UI assets remain the UI requirement, sourced from the installed
  MCC asset set through the MCC-only import pipeline (user clarification,
  2026-10-19). TU1.5 remains the behavior/reference target; users need only MCC
  as the external game asset source.
- The master verifies identity, stores social/clan state, discovers servers,
  matches players, authenticates connection setup and accepts post-carnage
  reports after matches. It does not simulate or relay active match gameplay
  or match voice in the intended default architecture.
- One actual player's PC hosts the authoritative world. Other players connect
  directly over encrypted peer UDP. STUN address discovery may assist setup;
  TURN and master WebSocket media fallback are disabled in the new direct path.
- Automatic host choice tests actual lobby connectivity, latency and upload
  capacity. A host must reach every physical connection owner. Select again
  among surviving eligible PCs during migration. Do not confuse the party
  leader, matchmaking gatherer, player seat and simulation host.
- Keep original Halo 2 behavior faithful wherever recovered code or primary
  public evidence establishes it. Native replacements and unresolved original
  units/coefficients must be labeled explicitly; never fill gaps by guessing.

## Acceptance checklist

| Area | Required final behavior | Current acceptance boundary |
|---|---|---|
| Identity | Steam primary login; private account IDs and chosen Halo screen names; preserve existing Discord accounts/history through verified linking | Real Steam OpenID/account verification and the operator-controlled native frontend login/normal-browser confirmation passed on 2026-10-16; live linked social/gameplay acceptance remains open |
| Native navigation | Quickmatch, Optimatch and Playlists launch real admitted online games; server browser joins community servers | Match launch exists; community server joining remains outstanding |
| UI | ONLINE capitalization; high-resolution Vista UI assets/fonts sourced from MCC throughout ONLINE and clans; readable screens without the obstructing Halo 2 logo; correct real clan-screen dimensions and widescreen bounds | MCC-only resource catalogue exists, with runtime activation/sampling explicitly disabled; complete widget/locale/menu-source and sampling-pack admission, replace the painter's older Vista-root selection, then pass fresh-import/delivered widescreen checks without a separate Vista donor |
| Social | Complete Discord friends/presence, native invitations, accepted parties and correct launch/return behavior | Existing SDK/native flows require regression and remaining live checks |
| Clans | Complete create/join/invite/manage/leave flows, matchmaking and original supported clan-rank presentation | Existing clan flows; remaining competitive/presentation coverage stays open |
| Couch players | Up to four local players per PC can queue/play together against others; verified secondary accounts retain their own stats; guests retain local PCR rows but publish no personal stats | Eight-player/two-PC native fixture exercised; physical controllers and independent voice devices are separate gates |
| Gameplay transport | Default ONLINE gameplay travels directly between players, with one player authoritative; no master media relay | v12 installed in both launchers and deployed; local native/rendered flows and one consumer-to-Vultr UDP probe passed; two consumer NATs remain unverified |
| Host quality | Select physical hosts from actual reciprocal reachability, latency and upload tests; reevaluate surviving candidates under agreed membership | Native session setup now runs bounded direct UDP upload/echo tests and broadcasts complete peer rows over authenticated DTLS. The coordinator ranks physical owners using measured reciprocal loss, upload bottleneck and RTT while retaining playlist connectivity thresholds; all-legacy setup retains its prior RTT policy. Four real socket peers exchange 768 probes beside payload traffic; two separate eight-seat native sessions select a physical host from actual measurements and retain fresh candidate lists. Five owner cases, 44 backend checks and selected owner ASan/UBSan/leak/TSan pass at the frozen protocol 86 cut. These are observed application delivery rates under bounded load, not ISP capacity or anti-cheat attestations. NAT classification, agreed telemetry/membership generations, automatic validated shadow selection, survivor reelection, full runtime delivery and WAN/hardware acceptance remain open |
| Match voice | Direct peer proximity, team/FFA radio, dead-player audiences, muting and postgame voice; complete source-supported lobby/loading, LAN and device coverage | Match/postgame voice now uses direct UDP; party/clan menu voice still uses its relay and remains to convert; LAN voice, all-participant loading voice, independent couch headsets/mics, masking and original radio audio remain open |
| Master outage | Peer simulation and voice continue through temporary signaling loss; reports retry safely afterward | Temporary master-socket outage acceptance passed:89 simulation ticks and86/88 audible frames continued, then reports uploaded; master-process-restart admission persistence remains open |
| Disconnect survival | A nonhost departure does not end the room; a graceful or abrupt host loss preserves and resumes the actual world | TU1.5 native Match and localhost UDP graceful/silent-peer departures retain roster/profile/result rows and original deferred corpse/equipment without added numeric deaths. The master explicit nonhost-leave and result-review paths preserve remaining peers and store complete admitted rows with survivor-confirmation metadata. A real native nonhost DTLS/signaling shutdown now leaves the host session running. Custom native-owned worlds, complete source admission cancellation, unannounced missing-peer result arbitration, production delivery, physical/WAN checks and host migration remain open |
| Migration | Preserve complete player/unit/weapon/inventory, allocation identities, RNG, timers, projectiles/grenades, props, devices, physics, recording/results, local profiles and voice; promote/rebind a validated survivor | Coupled TU1.5 checkpoints and the 29-operation source journal retain complete native/CPP state, typed inputs, actual returns, clocks, identities and local-view bindings. A negotiated two-cut direct-peer pipeline overlaps transfer/decode with ordered native validation. Matched cold/background fixtures validate 82 phases, reject wrong chains and forged identities with a prefetched successor, repair from full cuts and compare four complete worlds. The background retention case validates 122 phases, drains both admitted batches across actual host loss, survives worker teardown, imports/rebinds four recipient views and continues ten native ticks after old-host departure without fabricated deaths or unfinished reports. Transport normal/ASan/UBSan/TSan and selected native-owner sanitizer checks pass; current-root syntax checks pass. Normal runtime disconnect and stalled-host timeout paths retain a validated recovery world before teardown. Extended rendered scheduling shows reduced backlog but was interrupted by OS memory pressure; sustained catch-up remains unqualified. Owning private camera checkpoints also reproduce driver, passenger and death trajectories across independent native executors in the retained cohort, with complete-world equality and selected owner sanitizers. Native fatal readback now avoids premature FP publication and stale-unit command delivery. Owning host connection/input/camera sidecars also resume the same six-player world across native executors, retaining four remote couch views, queued/consumed action serials and exact complete snapshots without rebirth; normal and selected network-owner sanitizers pass against a freshly built immutable current-source cohort. A private replay pause now admits both pipeline cuts before controlled native execution; repaired current-cohort cold/background cases validate 82/122 accepted phases, with selected receiver-owner ASan/UBSan/leak checks and real UDP graceful/silent-peer departure regression. Explicit camera/host and paired complete-world wire codecs now bind the entire connection/input/camera state to room/generation/membership/world content. A real two-process direct-UDP case imports the paired state, reproduces four native phases and exact snapshots, and retains ten private ticks after old-host shutdown; normal and selected owner sanitizers pass. These fixtures retain producer view mapping, do not elect/promote a host and are not current full runtime delivery. Runtime publisher/receiver integration now negotiates paired full/replay cuts and retains validated host/input/camera state at the exact accepted native boundary. A production two-process stream verifies 70 native phases, two-cut prefetch, complete world/host equality, downgrade rejection, owning retention and ten ticks after actual old-host shutdown; normal and selected owner sanitizers pass. A fresh matched-source native rebuild also passes host-loss role rebinding: four promoted couch seats retain pending/consumed input and camera state while an existing three-seat client follows the replacement endpoint for twelve native ticks, rejecting old-host traffic and retaining native allocation/result identities. Selected owner sanitizers, paired stream and real UDP departure regressions pass; this role test supplies its generation decision and logical transport explicitly. Private hardware continuity, broader mode/caller/combat/PCR and scenery lifecycle coverage, generation-bound identity continuity, agreed membership/cut/epoch certification, actual host/transport/profile/controller/voice promotion, memory qualification, delivery and physical/WAN acceptance remain open. See [continuity evidence](player-host-continuity-progress-20261013.md). |
| Historical playlists | Final-2010 target with the fullest evidence-supported rotations/game coverage; casual/ranked matching follows recovered/public Halo 2 logic, including Max Hoberman's explanations | Final downloaded catalog not recovered; full objective engines/rotations remain open |
| Custom modes | Finish and bundle authored Fiesta Slayer and Halo-3-inspired Infection Slayer, including green zombies, under data/tags and release packaging; include Action Sack | Existing authored variants/packaging must survive transport/build changes |
| Tombstone | Include the installed map in sensible supported Slayer, Team Slayer, CTF, Fiesta and other suitable pools | Fiesta active; staged Slayer/Team Slayer and unfinished objective coverage need their native engine acceptance |
| Competitive policy | Finish supported quit penalties, later ranking behavior and original clan-rank display without inventing missing historical values | Remaining policies/evidence gates stay open |
| Post-carnage history | Upload after the match, validate admission/results/confirmations, store organized persistent report details, support later website lookup by verified Discord username, exclude guest personal history | Existing DB/history and guest tests; preserve across peer migration and master recovery |
| Delivery | Rebuild/install both native launchers, safely deploy backend with state backup/restore verification, package authored data and dependency notices | Every changed delivery needs fresh recorded verification; source presence alone is insufficient |
| Test storage cleanup | After validation, delete unused generated test binaries, objects, private build trees and scratch captures; keep small reports, commands, source hashes and failure diagnostics. Preserve test source, original assets/reference evidence, frozen source snapshots, launcher-selected runtimes and unrelated active work | User addition, 2026-10-18. Use `tools/run_tests.py` for CMake validation and `tools/run_test.py` with `HALO2_TEST_SCRATCH` for one-off probes. Verified on this migration step: 105 unused generated files and two private configure trees removed, reclaiming 5,300,504,655 bytes; new native/transfer build trees, per-test scratch and temporary snapshot caches are absent. No new large test artifacts retained. Preserve this lifecycle for subsequent validation |

## Modern mesh extension requirements

Component progress, 2026-10-18: [peer host/cut agreement](peer-host-agreement.md)
now implements generation-bound two-phase majority agreement on the replacement
PC and its exact retained cut. Partition, healthy-majority, interrupted-proposer,
duplicate/authentication, parser and concurrency cases pass, as does an actual
four-endpoint direct-UDP host-shutdown case beside player payloads. Normal,
ASan/UBSan/leak and owner TSan checks pass. Native readiness in the direct test
is declared fixture metadata; production session integration, native/controller/
voice promotion and full generation fencing remain outstanding. Disposable
test builds and compiler scratch are removed; small receipts remain under
`.local/peer-host-agreement-20261018`. The full goal stays active.

Follow-on session progress, 2026-10-18: an internal direct-session gate now
exchanges these votes over real admitted DTLS links, fences gameplay/encoded
voice until each native owner acknowledges its exact learned decision, advances
the host/epoch/physical and couch membership together, and preserves the
original native/PCR allocation roster across generations. Four real session
workers pass two host socket/signaling teardowns with 16/12/8 couch seats and
resumed payloads. Ready native worlds and role completion are declared fixtures.
Normal and selected owner ASan/UBSan/leak checks pass. Broader C-stack TSan
found an atomic/non-atomic global free-chunk counter conflict in the pinned SCTP
dependency; its live reads are now patched. A subsequent repeated run completed
one two-handoff process, then found a separate global tick read/write conflict.
Dependency-wide race qualification remains open, including the system OpenSSL
diagnostic and an intermittent ICE setup error. Ordinary matchmaking keeps the
new gate disabled until actual native/controller/device promotion, candidate
selection, negotiated compatibility, after-match admission/results and full
runtime delivery are verified. Receipts are under
`.local/peer-session-migration-20261018`; continue disposable test cleanup.
The latest broader seven-case selected-owner sanitizer run passes six cases;
the ordinary measured-host fixture reports OpenSSL allocations at child-process
exit. Its cleanup diagnosis remains open alongside the SCTP tick race. The new
two-handoff session and both old continuation transport modes pass in that run.

Subsequent teardown progress: the earlier “tick” race is now narrowed to callout
pending/active flags crossing queue and association lock domains. A real
two-thread dependency regression fails before the new timer-flag lock patch and
passes afterward. Shared last-owner RTC cleanup also resolves the reproduced
ordinary-session OpenSSL exit leak; independent rooms, rejected construction,
fresh restart and concurrent lifetimes pass. Compiled C/C++ TSan passes those
owners and three independent two-handoff session processes with uninstrumented
system libraries explicitly excluded. Actual native world/controller/headset
promotion, crypto-internal race qualification and production/default migration
remain outstanding. See `.local/peer-teardown-20261018` and the
[transport integration contract](peer-host-agreement.md).

The final broader teardown run passes all nine focused CTests with compiled
C/C++ ASan/UBSan and leak checking, including the previously leaking ordinary
session and both old transport modes. C/C++ TSan passes three focused CTests,
with the host-loss fixture repeated across three independent processes. Its
system-library exclusion stays explicit. Native world/device handoff, default
migration and delivery remain open; these checks do not close the full goal.

Native takeover progress: the receiver now consumes its exact paired native cut
only after its own session learns the replacement decision, restores the new
host's couch views/input/cameras, applies original deferred departures and keeps
media fenced until explicit activation. An owning retained-worker export can
also reconstruct that host on a different executor after the worker is destroyed.
Two actual twelve-player/three-session cases pass with complete native state,
old native world/session teardown, two-survivor agreement, four promoted seats,
an existing four-seat client, twelve subsequent connected native ticks and direct
encoded voice packets. A same-tick unrecorded native write is rejected and repaired
before readiness. The new case also found and fixed the continuation codec's
incorrect eight-group limit for FFA admission rows. Four focused CTests pass with
selected owner ASan/UBSan and leak checking, including paired replay and prior
role/session regressions. These are internally gated, in-process native/transport
checks with explicit candidate selection and activation; rendered adoption,
controller/headset and decoded-audio handoff, automatic compatibility/candidate
policy, after-match recovery and WAN/delivery acceptance remain open. See
`.local/native-peer-takeover-20261018/verification.json` and the
[native handoff contract](peer-host-agreement.md). All owned generated trees are
removed, including the final 2,662,993,859-byte supervised workspace; bounded
diagnostics and source manifests remain, with no new retained large artifacts.

Audio handoff progress: retained voice now retires its old decoder/talker,
microphone and device queues asynchronously, rejects held old-generation
microphone/receive batches and acknowledges retirement before native completion
opens the successor epoch. Real four-session/two-host-loss checks resume fresh
gameplay, voice packets and decoded synthetic stereo after each handoff without
reopening devices. Five ASan/UBSan/leak CTests and three TSan CTests pass; both
session cases repeat across three fresh processes in TSan. System shared-library
internals are excluded from that TSan run. Runtime callers now suspend source
frame work during fenced handoff phases and keep OS events responsive; those
changes have current-source syntax checks, not delivered rendered acceptance.
The complete goal remains active for renderer adoption, physical input/headsets,
automatic compatibility/candidate policy, after-match recovery, memory/WAN and
delivery. See `.local/voice-handoff-20261018/verification.json` and the
[audio retirement contract](peer-host-agreement.md). All generated test
build/scratch trees are removed; no new large artifacts are retained.

Runtime ownership progress, 2026-10-18: both runtime loops now resolve the live
Match/world/transport through `MultiplayerRuntimeAuthority`, which retains the
initial immutable map resources and owns publication/recovery lifetimes. A frame
scope rejects nested frames and role replacement, even without a recording
publisher. Private staging leaves the original role visible until the exact
native takeover activates. Two actual twelve-player native cases pass the
foreground/background-retention paths, active-frame rejection without ownership
loss, exception unwinding, resident cache lifetime, four-seat host adoption and
twelve connected native ticks/encoded voice packets. Five selected-owner
ASan/UBSan/leak CTests pass, including role, paired-stream and synthetic decoded
voice regressions. The current-root renderer passes warning-clean syntax checks;
its automatic camera/input/presentation adoption handler, default migration,
hardware/WAN, memory and delivery acceptance remain open. Receipts are under
`.local/runtime-authority-20261018/verification.json`. The owned 2,711,637,063-byte
validation workspace and compiler scratch are removed; only bounded reports,
commands and source hashes remain, with no new retained large artifacts. The
complete goal stays active.

Measured survivor progress, 2026-10-18: replacement preference now requires
native-ready cuts and fresh reciprocal measurements among the fixed survivors,
excluding only the agreed predecessor host. The gated path binds QoS, native
admissions/cuts and votes to the same membership revision. The selector does not
locally drop a second missing PC or invent speed/NAT metrics for a sole survivor.
Four root C/C++ ASan/UBSan/leak CTests pass, including normal measured-host setup
and decoded voice regression. Two root C/C++ TSan CTests pass; measured migration
runs in three independent processes, with system-library internals excluded.
Seven selected native-owner ASan/UBSan/leak CTests also pass. Both actual native
takeover paths now choose their ready PC from measurements; foreground recovery
polls until replay-worker/native-world destruction completes before releasing
the wrapper. Transport cases cover 16/12/8/4 seats across three host losses, with
native readiness declared there and actual twelve-player native worlds tested
separately. Receipts: `.local/survivor-selection-20261018/verification.json`.
The 2,731,636,383-byte native validation workspace, three other private builds
and compiler scratch are removed; bounded diagnostics remain without new large
artifacts. Automatic scheduling/retry and false-alarm/readiness lifecycle,
additional agreed membership changes, rendered/input/camera/cache adoption,
default compatibility, memory/hardware/WAN, after-match recovery and delivery
remain open. The full goal stays active.

Rendered-owner progress, 2026-10-18: the foreground authority exposes the restored
native camera and tick-idempotent retained local input. Both actual native
takeover cases send three pre-cut DTLS input pulses for four couch seats,
consume one before capture, then consume the remaining two after activation
without repeating their serials on a second poll of the same tick. Machines,
props and scenery can retire their presentation lifetimes at a completed frame
while preserving resident geometry/material slots. Original-asset CPU checks
rebind identical salted handles at an earlier cut and compare the geometry.
All ten selected-owner ASan/UBSan/leak CTests pass in the coherent native/renderer
cohort; current-root runtime syntax and critical source hashes also pass.
Receipts: `.local/rendered-handoff-20261018/verification.json`. The supervised
2,961,309,972-byte generated workspace and all six recorded compiler scratch
trees are removed; only bounded diagnostics remain. Test source, original
fixtures, frozen inputs and installed runtimes are preserved. This supplies
owner APIs, not a completed windowed adopter: hardware input suspension/drain,
interpolation/HUD/effects/GPU query retirement, automatic scheduling and the
other migration/delivery acceptance gates above remain open.

Automatic-election progress, 2026-10-18: the gated session worker now starts and
retries a measured native-ready proposal without a positive caller proposal.
Paced ballots preserve the strict 500 ms suspicion boundary, fixed physical
quorum, healthy-host veto and highest previously accepted cut; duplicate
messages cannot postpone a failed-proposer retry indefinitely. Six broader
C/C++ ASan/UBSan/leak CTests and three broader C/C++ TSan CTests pass. Measured
and decoded-audio sessions each run in three independent TSan processes, with
system-library internals excluded there. Expanded agreement checks also pass
ASan/UBSan/leaks and unsuppressed owner TSan across sixteen peers, 64 competing
preference schedules and concurrent scheduling. Seven selected-owner native
CTests pass, including both actual twelve-player takeover paths using automatic
session elections and explicit native staging/activation. Evidence:
`.local/automatic-election-20261018/verification.json`. The supervised
2,732,562,234-byte generated tree and all six root build/scratch trees are
removed, with no new large retained artifacts. Automatic readiness retention
and false-alarm lifecycle, full recovery/adoption coordination, rendered/device
integration and the remaining default/delivery/hardware/WAN gates stay open.

Native preparation/commit progress, 2026-10-18: the runtime now owns a minimal
gameplay recipe and stages its learned host or surviving-client role at a
completed frame. Foreground import uses one disposable native world, validates
the original producer mapping and full admissions, then binds recipient views;
it does not rebuild a second temporary world. Host/world ownership or the
prepared client endpoint now commits before the session publishes its epoch.
Old client destruction runs outside the session lock. Invalid/stale/foreign
endpoint preparations and refused native adoption cannot publish a new epoch.
Actual native cases also reject wrong profiles/views, retain no presentation
callback, preserve recipe selections across later menu changes, and restart a
survivor's failed old shadow. Seven final selected-owner native CTests, three
root C/C++ ASan/UBSan/leak session CTests, two root C/C++ TSan CTests with three
fresh measured/audio processes each, and current-root syntax checks pass.
Evidence: `.local/native-prepare-coordinator-20261018/verification.json`.
The supervised native workspace, both root build/scratch trees and all compiler
scratch are absent; no new large artifacts remain. The outer recipe ended with
signal status143 after the final successful CTest run while awaiting finish;
bounded passing reports and source hashes remain, but its optional final trace
copy and pre-cleanup size measurement were not produced. Native preparation is
still synchronous. Automatic readiness/false-alarm handling, rendered/input/
HUD/effects/GPU adoption and actual activation in the runtime loops, timing/
memory/device/WAN qualification, default compatibility and delivery remain open.
The local-input audit identified post-cut promoted-client commands as the next
required follow-up; the qualification below covers that owning input bank.

Promoted input carryover, 2026-10-18: the native client now retains its latest
owning input packet. Before retiring that client, host activation validates its
original session, physical owner, cookies, profiles, slots and monotonic action
serials against the independently accepted host bank. It imports newer controls,
pending presses and completed interactions without changing delivered counters,
the native world or camera checkpoint. Repeated refresh and refused adoption
preserve the same pending input. Foreign contexts/owners/slots, early consumption
and malformed input are rejected before mutation.

Seven selected-owner native ASan/UBSan/leak CTests pass. Both actual twelve-player
native recovery cases send two additional couch-input pulses after the sealed
cut, deliver all four remaining pulses once, preserve latest movement/look/zoom
and deliver one completed interaction per local seat. They continue twelve native
ticks and twelve direct encoded voice packets with an existing four-seat client.
Current-root syntax checks also pass. This uses the coherent frozen native ABI
plus hashed current owners; generated/native/RTC/system interiors remain outside
the selected-owner instrumentation. Evidence:
`.local/promoted-input-continuity-20261018/verification.json`.

The successful supervisor removed 27,199 generated private files totaling
2,738,244,560 logical bytes, including copied source, build and cache; compiler
and cleanup scratch are also absent. Together with the 44 old test executables
above, this continuation reclaimed 8,441,673,768 logical bytes. Bounded reports
remain; no new large artifacts are retained. Gameplay and match/postgame voice
remain direct peer traffic. Default migration remains gated: actual hardware
input pause/drain/release and renderer/HUD/effects/interpolation/GPU/device
adoption, automatic readiness/false-alarm handling, wider mode/generation/PCR
coverage, sustained timing/memory, delivery and physical/WAN acceptance remain
open with the rest of this full goal.

Windowed input admission, 2026-10-18: the actual runtime now suspends fresh
keyboard/controller commands throughout the transport fence and promoted-bank
drain. It handles OS/focus/device events, discards relative mouse motion, freezes
interaction hold time, and delivers the bank through `continue_local_input()`
before hardware consumption. Saved movement/look/zoom and interaction intent
cannot be overwritten by fresh polling, scripts or the current prompt. Catch-up
stops after the final carried tick so hardware is reacquired on a new frame.
Existing controller button baselines remain intact; a changed device drops only
that view's old hold state. A completed keyboard interaction survives resume and
intervening key events before the next held request starts.

The current adapter passes 287 actual SDL event-queue checks under ASan/UBSan,
and current-root syntax passes separately. A full current-source freeze, with
the latest adapter/fixture updates, builds the windowed runtime and passes its
154-tick TU1.5 input regression using normal Vulkan. It covers ordinary movement,
jump, firing, exact reserve-to-magazine reload transfer, mouse look, focus and
overlay capture; it does not trigger a rendered host migration. Khronos GPU
validation layers are unavailable. The source-bound reload trace corrected an
obsolete assertion against a separate C++ presentation timer; no gameplay reload
code or timing was changed. A first full build was interrupted with143; its
workspace was removed and its cause remains unestablished. The successful retry
and incremental qualification removed 34,345 generated private files totaling
1,063,983,811 logical bytes. Adapter build/compiler scratch are also removed;
no new large artifacts remain. Evidence:
`.local/runtime-input-handoff-20261018/verification.json`.

Default migration remains gated. Complete presentation/interpolation/HUD/effects/
GPU/device borrower adoption and native query-ring retirement, automatic readiness
and false-alarm lifetime, actual rendered migration, independent physical couch
controllers/headsets, current-ABI native migration coverage, sustained memory/
timing, delivery/WAN and the other full ONLINE requirements remain open. Matches
and match/postgame voice retain the direct player-hosted architecture.

Native/GPU lens-query ownership, 2026-10-18: incoming private hosts now retire
their predecessor's CPU query requests through unchanged original `20E30`, the
per-request primitive used by original `20700`. Count/span admission and the
native executor barrier precede mutation. This preserves unallocated bitmap
bits and high-water count, changes no non-presentation source bytes apart from
the declared caller stack, and supplies no GPU samples. Retirement is part of
native preparation before sealing its adoption boundary. The renderer query
owner can explicitly bind the retired incoming bank, allocating private state
before retiring old GPU work; failed/foreign/duplicate binds are rejected.

Sixteen unchanged-XBE differentials and eight current-source native/session
ASan/UBSan/leak CTests pass. Both actual twelve-player promotion cases carry33
declared pending CPU requests across the authenticated cut and retire them
before adoption, then preserve the existing input/voice/simulation checks.
The RTX5080 GPU assertions and Khronos validation pass for45 original quads,
visible/occluded/empty coverage, stale-result retirement, rejected binds and
fresh query-ID reuse. The GPU process CTest still fails LeakSanitizer at
shutdown. A minimal Vulkan instance/device control without Halo, SDL or query
pools independently reproduces the driver allocation families; a live-device
probe also finds allocations in NVIDIA glcore/glsi. No GPU process leak-free
claim is made. Evidence: `.local/lens-query-handoff-20261018/verification.json`.

The supervisor removed34,502 generated files totaling1,512,947,527 logical
bytes, including its full current-source copy, build, captures, validation-layer
package and extracted libraries. Compiler/control scratch are also removed.
Only bounded reports and the5,148-byte required original retirement hash
fixture remain; no new large artifacts are retained. Full rendered adoption,
other presentation/HUD/effects/device owners, automatic readiness/false-alarm
lifetime, physical/WAN and other full-goal gates remain open. Default migration
is still gated and gameplay/match voice remain player-hosted.

Dormant client lens resources and final-draw ownership, 2026-10-18: eligible
clients in the gated peer-migration path now prepare authored map flare images
before renderer material upload, while their native producers stay dormant.
The final-draw owner can bind a separately prepared retired bank at a completed
frame, preserving resident material identities and replacing its private SDK,
headers, FIFO, camera, counters and source borrower. Six rebindings over two
actual native worlds pass their assertions, including a cold owner and preserved
native adoption boundaries. This component case does not execute the normal
ONLINE client window loop or visible flares across an actual migration.

The refreshed private native executable builds. Six CPU/reference CTests pass;
both RTX5080 functional reports and Khronos validation pass. Both GPU processes
still fail unsuppressed LeakSanitizer checks with the previously reproduced
driver allocation families. Evidence:
`.local/renderer-lens-adoption-20261018/verification.json`. The supervisor removed
34,761 generated files totaling 1,519,555,707 logical bytes, including the
executable, source copy, build, captures and temporary validation layer. Disk
snapshot/compiler scratch are removed too; no new large artifacts are retained.
The ONLINE/clan Vista asset selection is unchanged. Coordinated preparation of
all presentation owners, effects/HUD histories, interpolation, source pointers,
device adoption, readiness and actual runtime activation remain outstanding.
Default migration remains gated and the full goal remains active.

HUD/event lifetime preparation, 2026-10-18: completed-frame primitives now
discard HUD shield/winning/radar/voice histories, rebase the validated device
event journal without replaying accepted requests, and retire unsubmitted
discarded-world sound cues. Resident images/materials, authored Vista shield
sampling offsets and local input bindings survive. Eleven focused CTests pass
with selected-owner ASan/UBSan/leak checks; the new HUD cases compare16 declared
world replacements,16 stale-history controls and320 complete CPU vertex streams
against independent fresh HUDs, covering both artwork paths, both species,
one through four local views and4:3/16:9. These are native presentation policies,
not recovered Xbox migration reset routines or rendered migration acceptance.
Evidence: `.local/hud-event-handoff-20261018/verification.json`.
The supervisor removed34,163 generated files totaling1,653,612,981 logical
bytes; all owned build/compiler/source-check scratch is absent. No new large
artifacts or installed runtime remain from this step. The full adopter still
must prepare all owners, preserve accepted active effects/audio, rebase source
borrowers/interpolation, invalidate radar upload ticks, and publish the role
atomically. Readiness, false-alarm, rendered/physical/WAN and full-goal gates
remain open; default migration stays gated and matches/voice remain direct.

Active effect presentation, 2026-10-18: NativeMuzzleEffects and NativeWorldEffects
now capture owning local presentation cuts and prepare replacements before an
allocation-free publication. Live particles/emitter identities, clocks, poses,
decals, lights/spatial/query caches and unsubmitted sounds survive; discarded
future state is removed. World definition links rebase into each private copy.
The caller supplies accepted-world collision callbacks and long-lived visual
RNG bindings; mismatched aliasing and foreign/consumed tokens reject. Snapshots
own their random values and exclude predecessor collision callbacks. Two TU1.5
selected-owner ASan/UBSan/leak regressions pass, including a cut with24 particles,
11 decals and1 light, forty further match ticks against an independent batched
owner, two muzzle restores and twelve continuation ticks with new allocations.
Evidence: `.local/effect-presentation-handoff-20261018/verification.json`.
The supervisor removed34,152 generated files totaling1,078,856,808 logical bytes;
all owned build/compiler/check scratch is absent and no large artifact/runtime
is retained. These local cuts still need exact accepted-native-cut association
and coordinated capture/publication with attachment caches, interpolation,
devices, source borrowers and playing audio. Readiness, rendered/physical/WAN,
delivery and all other full-goal gates remain open. Default migration stays gated.

Grouped effect/cache cuts, 2026-10-19: SourceMapEffects and
SourceCombatAttachments now retain complete local caches and reject capture
after incomplete synchronization. EffectPresentationContinuity captures them
with both effect owners and shared visual RNG, supports later binding after
asynchronous native validation, and stages all replacements before one
allocation-free publication. Exact bound context/peer/seal fields and executor/
owner/token checks reject stale or foreign publication. Six selected-owner
ASan/UBSan/leak CTests pass. A real TU1.5 native checkpoint on Turf restores
tick45 with4 map births,4 combat instances and88 particles after ten discarded
ticks, then continues twenty ticks without duplicated attachment births.
The first-person presenter is empty in this combined case; active FP coverage
remains the separate regression. Room context is fixture metadata and no live
role/GPU/device migration occurs. Evidence:
`.local/effect-cache-cut-20261019/verification.json`.
All owned scratch is removed:34,272 files and1,263,200,521 logical bytes in the
main workspace, plus compiler/check scratch. No large artifact/runtime remains.
The host sidecar already retains per-peer snapshot sequences, while accepted
gameplay snapshots may change ownership within one tick. Runtime readiness must
prove the admitted snapshot-sequence/native-cut association; tick agreement
alone is insufficient. That integration, remaining presentation/device/audio
owners, GPU/native role publication, readiness/false-alarm lifetime, delivery
and rendered/physical/WAN/full-goal acceptance remain open. Migration stays gated.

Accepted snapshot receipts, 2026-10-19: LanClient now returns one opaque owning
receipt for every accepted state, preserving its connection, generation,
membership, sequence and SHA-256 body identity. The host sidecar retains the
last sent body identity for each physical peer. A local receipt can be proved
against that exact independently authenticated/natively validated paired cut;
tick equality or a newer sequence alone cannot substitute for the matching
body. Receipt-based effect captures require this proof for later binding.
The sidecar format is now H2HOST02 and explicitly rejects H2HOST01; the current
native gameplay protocol89 is unchanged. Coherent feature negotiation and
delivery of the sidecar version remain required before activation.

Four selected-owner ASan/UBSan/leak CTests pass, including a real TU1.5 native
cut, four couch seats, a same-tick late join, an older valid body relabeled
with a newer sequence, old/foreign receipts, codec roundtrip/version rejection,
host continuation, role rebind and grouped effects. These use logical endpoint
adapters and locally validated cuts; no live DTLS/WAN/runtime/GPU migration is
established, and the new receipt case has empty effect presenters. Evidence:
`.local/snapshot-origin-20261019/verification.json`. All recorded scratch is
absent; the supervisor removed34,314 generated files/1,271,417,932 logical bytes.
No large artifact or runtime is retained. Normal runtime still needs bounded
receipt/presentation history, readiness binding and an ownership refresh for
newer same-tick states without advancing per-tick animation twice. Full owner
adoption, publication, delivery and rendered/physical/WAN acceptance remain open.
The full goal stays active; default migration stays gated. Gameplay and match/
postgame voice remain direct and player-hosted.

Native same-tick adoption, 2026-10-19: the production client now consumes each
state with its exact receipt and refreshes same-tick roster/ownership instead
of retaining only devices. Surviving interpolation endpoints remain intact;
departed/replaced owners cannot reacquire retired history. Gait resolves new
lifetimes without another channel tick, aim advances zero ticks and repeated
FP clock pairs do not dispatch another sound. Already completed shot stamps
baseline at admission and at an already-connected viewport startup.

The real TU1.5/localhost-UDP four-view renderer passes with a private Xvfb
display, RTX5080 and Khronos validation: tick2,39 receipts,36 same-tick refreshes,
28 frames,192 unchanged completed gait observations, four actual local weapon
cues without replay, and a late join/departure with the original retained report
row. Five component checks also pass with selected interpolation-owner
ASan/UBSan/leak instrumentation. A separate native late-audio regression remains
red: the FP muzzle control emits10 cues while the WORLD client submits20 from
the same weapon; the actual source effect has two equal-name locations.
TU1.5 WORLD/FP location/part/sound admission must be established before changing
either playback or expectations. Native Match currently exports clock pairs
only for producer-local views; this four-view case has no remote FP clock pairs
and does not qualify remote FP channel audio. Evidence:
`.local/replica-refresh-20261019/verification.json`.

All recorded scratch is removed, including34,710 generated files/1,528,119,727
logical bytes in the owned build workspace. No large artifact/runtime remains.
The full source freeze and explicit updates retain later unrelated root drift.
Bounded accepted presentation history/readiness, authentic remote FP ownership,
the red sound regression, full borrower/audio/GPU adoption, coherent delivery
and rendered-migration/physical/WAN/full-ONLINE acceptance remain open. Default
migration stays gated; gameplay and match/postgame voice stay player-hosted.

WORLD/FP sound admission, 2026-10-19: the native late-audio mismatch above is
resolved. The WORLD exporter had bypassed the original location iterator and
published encoded FP records as extra WORLD records. Whole unchanged TU1.5
17DCE0 replays cover65,536 node words in each of modes0/1/3 and verify the actual
17D210 caller's mode0 instructions. WORLD projection now applies that exact
admission before transform resolution, preserving FFFF and separate WORLD
records even when marker names match. No cue-count deduplication was added.
The actual native weapon has one WORLD and one FP record and now publishes one
WORLD record. Ten postjoin cues produce ten SDL submissions; the retained
prejoin cue is silent. All eight focused normal checks and six selected-owner
ASan/UBSan/leak cases pass, including the four-view renderer/continuity cases.
Evidence: `.local/world-location-admission-20261019/verification.json`.

All recorded scratch is absent:34,792 generated files/1,553,200,174 logical
bytes from the main workspace, plus compiler/oracle scratch and a duplicate
reference copy. Only the small65,536-byte original admission fixture and bounded
reports remain. Later concurrent multiplayer/header/runtime-test changes are
preserved and recorded separately from the coherent tested source cohort.
Authentic remote FP clocks/ownership, bounded presentation history and actual
readiness, full borrower/audio/GPU/native adoption, delivery and rendered-
migration/physical/WAN/full-ONLINE acceptance remain open. Default migration
stays gated; gameplay and match/postgame voice remain player-hosted.

Implement as modular C++20 owners over the existing physical peer table, with
thread-safe telemetry and vote snapshots using bounded queues, lightweight
locks/atomics, and no Discord identifiers in peer presentation.

1. **Client-side voter quorum.** More than 500 ms without host packets enters
   `SUSPECTED_DEAD` and sends a lightweight authenticated verification probe to
   the active nonhost mesh. A strict majority of the agreed nonhost membership
   must corroborate host silence before revoking that authority generation.
   Bind every vote to the match, host generation, voter and membership view;
   reject replay/duplicate/nonmember votes. Membership removal must preserve
   quorum intersection, rather than allowing each partition to shrink its own
   denominator. A timeout alone must not manufacture a permanent cheating ban.
2. **Decentralized telemetry.** During the pregame lobby, continuously probe
   other PCs on the actual gameplay path and exchange bounded latency/loss/NAT
   telemetry matrices. Analyze asymmetric loss claims and conservatively
   disqualify suspect candidates from host eligibility. NAT/path failures are
   not proof of cheating; preserve unknown NAT classifications until measured.
3. **Asynchronous shadow queue.** During gameplay, deterministically order
   eligible peers by the requested mesh-latency/NAT health policy, with stable
   private peer-ID tie-breaks and agreed membership/telemetry generations.
   Identify a shadow host in advance and retain its complete validated world.
   On certified failure, transfer authority and rebind transport within a few
   frames where actual continuation permits. Do not label an endpoint switch
   or a display snapshot seamless simulation migration.

These are new hardening requirements, not claims that the supplied descriptions
of every legacy flaw have already been verified against original Halo 2.
The user requested these after direct transport and the migration foundation.

## Evidence and completion rules

Track focused tests, full native builds, localhost native-process tests, WAN
NAT/throughput tests, visible gameplay, physical controllers/headsets and actual
multi-PC migration separately. Synthetic Discord fixtures do not establish
live authorization, and election fixtures do not establish world continuity.
Retain dirty/concurrent workspace changes and Vista ONLINE/clan behavior while
moving final asset selection to the MCC-derived bank required by U01/D01.
Do not mark the goal complete while required work or acceptance remains.

Related evidence:
[direct peer integration](direct-player-hosted-online.md),
[migration progress](player-host-continuity-progress-20261013.md),
[world inventory](native-migration-state-audit-20261013.md),
[original host behavior](halo2-host-migration-evidence.md),
[completion audit](online-completion-audit.md),
[historical target](online-default-target.json),
[couch players](online-couch-players.md).

## Direct peer delivery milestone

Both launchers now select `.local/direct-peer-20261013/runtime/halo2_native`.
The backend release is `/opt/halo2/releases/20261014T021753Z`; all36 preexisting
retained tables and the master key matched the isolated backup/restore check.
The new `match_result_consensus` table was empty. The delivery evidence is
[verification.json](/home/eric/Developer/Halo-2-Decompiled/.local/direct-peer-20261013/verification.json).
This closes the bounded direct-transport delivery stage, not the full goal or
gameplay host migration.

## Current identity and hostname directive, 2026-10-16

The user now requests Steam as the primary ONLINE login, using the existing
`STEAM_KEY` repository secret in `TheRunaway5/Halo-2-PC-Website`, and the game
service at `https://master.halo2forever.net`. This supersedes the earlier
required-Discord-login decision. Steam IDs remain private; chosen Halo screen
names and four local seats remain supported. Existing Discord accounts, clans
and history must be preserved, with explicit dual-identity proof for linking;
matching display names must never merge accounts. Discord social consent may
remain optional for its friends/invites. Guests without their own verified
identity continue to be excluded from persistent personal PCR publication.
The website apex remains on GitHub Pages. All other requirements and gates in
this goal remain in force. The migration is not accepted until the native
login/queue/social/result paths and deployed backend are verified.

Steam delivery update: the real Steam/account login and deployed new hostname
passed verification. The current full native engine and both launchers have
been built and installed with Steam identity and the bundled Discord Social SDK;
six focused CTests and nine-aspect Vista UI checks passed. The user completed
login in the native frontend and confirmed that both ONLINE and the normal
browser confirmation work. Controlled test tabs alone reproduced the browser
block; the game's default-browser route is accepted. The launcher still enforces the existing MCC gameplay-readiness
gate, and live Steam-linked friends/invites and gameplay acceptance remain open.
See `steam-identity-migration-20261016.md` for exact evidence and boundaries.


## Foreground paired-cut evidence, 2026-10-19

The receiver now exports an immutable `SourceAuthorityHostProof` with its exact
validated world/host cut through the worker status mutex. Only completed native
paired validation can create it. Foreground client receipts are checked against
that owning table's exact connection, generation, sequence and body. No native
executor/world borrower changes threads. Current proof clears on incomplete
replay, cancellation and consumption; retained historical proof survives later
validation/worker destruction, and old-generation receipts fail after rebind.
A historical proof does not assert live-world readiness or authorize promotion.

Five focused normal cases pass after correcting the stream fixture to keep its
foreground gameplay pump alive beside replay (70 actual accepted receipts).
Three native cases pass with selected receiver/fixture ASan/UBSan/leak checks;
two receipt/effect controls are uninstrumented. Real local DTLS takeover checks
both physical survivors' exact receipts across twelve admitted seats, receiver
and background-retention paths, twelve resumed native ticks and twelve encoded
voice packets. No rendered migration, physical headset/controller or WAN
acceptance is claimed. GCC C++20 syntax and source/cleanup audits pass.
Evidence: `.local/validated-host-proof-20261019/verification.json`.

The owned build removes34,386 files/1,358,022,460 logical bytes. All recorded
scratch is absent; no large artifact or runtime is retained. Concurrent source
edits, original assets/references and launcher runtimes remain intact. Bounded
completed presentation/receipt history, actual readiness for all survivors,
authentic remote FP ownership, full borrower/audio/GPU/native adoption, timing/
memory, coherent delivery and rendered/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated; the full goal remains active.
Gameplay and match/postgame voice remain direct and player-hosted.

## Completed CPU effect receipt history, 2026-10-19

Bounded `EffectPresentationHistory` now captures completed local CPU effect
owners with exact accepted receipts. Context comes independently from the
session. Duplicate retention is idempotent; evicted/reordered/retired receipts
cannot be recaptured or replaced by another same-tick sequence/body. Failed
capture preserves older entries. The windowed runtime finishes poses/clocks
and drains queued muzzle cues for each accepted receipt, then retains128
entries under the existing internal migration gate plus one separately owned
matched cut. This is a count policy, not an Xbox constant or byte-budget claim.
Binding requires the receiver's independently validated native world/host proof;
it does not authorize or advertise migration readiness/activation.

Nine normal cases pass after correcting the new ONLINE fixture to advertise the
resolved native variant, preserving strict sidecar checks. The actual four-view
ONLINE runtime captures257/257 receipts, retains at most128 entries, binds two
real native paired cuts and renders463 frames over local DTLS. It uses fixture
identity,30Hz host pacing and10Hz snapshots; no rendered migration or physical/
WAN acceptance is claimed. Whole fixture peak RSS is7,408,412 KiB, including
collocated native worlds and renderer, so memory/catch-up qualification remains
open. Three selected history/group/fixture ASan/UBSan/leak cases and two
uninstrumented companion controls pass. Raw-LAN effect barriers and the native
late-audio regression remain green. Current-root syntax and source audits pass.
Evidence: `.local/effect-history-20261019/verification.json`.

All recorded scratch is absent:34,857 generated files/1,742,441,803 logical
bytes removed from the owned workspace, with no large artifact/runtime retained.
Concurrent changes, original assets/references and launchers are preserved.
Remaining gates include checkpoint sharing/byte budget and sustained catch-up,
authentic remote FP/event ownership, readiness for all survivors, full scene/
model/FP/device/source/playing-audio adoption, coordinated GPU/native publication,
coherent delivery and rendered-migration/physical/WAN/full-ONLINE acceptance.
Default migration stays gated; the full goal stays active. Gameplay and match/
postgame voice remain direct and player-hosted.


## Active-light checkpoint resource sharing, 2026-10-19

Effect checkpoints now share immutable BSP light geometry/atmosphere/terrain,
while light pools, spatial links, fades and query state remain independent.
A matched normal allocator probe holding128 active-light cuts reduces added
live heap from95,980,784 to40,763,552 bytes (57.53%);24 particles,11 decals and
one light retain their exact state and continuation. Four normal cases pass
before/after and three selected-owner ASan/UBSan/leak CPU checks pass. The
four-view ONLINE fixture exercises two full128-entry history turnovers and
two actual native paired-cut associations in both runs. Whole-process RSS does
not show a reduction (6,842,848 versus7,408,516 KiB); a client byte budget and
sustained catch-up/memory qualification remain open. No source/network/UI/media
behavior is changed. Evidence:
`.local/effect-light-residency-20261019/verification.json`.

All recorded scratch is absent;34,871 files/1,358,228,683 logical bytes removed.
Only bounded reports and small baseline source/hash evidence remain, with no
large artifact or installed runtime retained. Concurrent work and original
assets/references/launchers are preserved. Full readiness, authentic remote FP,
complete scene/model/device/source/playing-audio and GPU/native publication,
coherent delivery and rendered-migration/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated and the goal stays active. Match
gameplay and voice remain direct and player-hosted.

## Staged lens presentation transaction, 2026-10-19

Query and final-draw owners now expose separate prepare/readiness/commit
operations. `SourceLensPresentationHandoffTu15` prepares both before any old
GPU retirement, then rechecks both and publishes their private state together.
Cancelled/partial preparation preserves both old owners. Executor, renderer,
incoming retired ring, owner generation and intervening frame activity are
checked again at commit, including unsubmitted frames and consumed tokens.
The enclosing adopter must still retain/quiesce the exact incoming world and
validate its complete native seal. This component does not authorize adoption.
GPU retirement retains its existing device wait; no latency bound is claimed.

The private native executable builds; 11 normal focused CTests pass. Two real
native worlds exercise 20 final-draw views and paired publication. Five rejected
or cancelled preparations preserve actual old GPU results; valid publication
retires them without changing the incoming native seal or resident materials.
The 45 original query quads and 45 final-draw callback controls pass with empty
Khronos validation logs. Current-root GCC syntax checks pass. This run does not
claim sanitizer, visible-flare migration, physical or WAN acceptance. Evidence:
`.local/lens-transaction-20261019/verification.json`.

All recorded private scratch is absent: 35,086 generated files / 1,529,265,786
logical bytes removed, with no executable, capture or installed runtime retained.
Two later unrelated source changes are recorded and preserved. Complete scene,
model, device, playing-audio and coordinated native adoption, survivor readiness,
timing/memory, delivery and actual rendered migration remain open. Default
migration stays gated; the full goal stays active. Gameplay and match/postgame
voice remain direct and player-hosted, with the Vista ONLINE/clan UI unchanged.

## Scene lifetime retirement seams, 2026-10-19

Dynamic objects, weapon projectiles and released grenades now have explicit
completed-frame retirement methods. They clear owner pose/light/LOD and model/
glow draw history while retaining authored resident banks. Together with the
existing scenery/machine/prop methods, all six scene retirements and shared
geometry history reset are now `noexcept`. The prop view entry also rejects a
fifth view before indexing its retained source cache, fixing the failure found
by the existing LOD regression.

The native executable builds. Five normal focused CTests and the same five
selected-owner ASan/UBSan/leak checks pass without suppressions. The new case
uses declared inputs with actual TU1.5 resources: four views, unchanged full
handles, frame300 to frame0, empty retired borrowed draw lists, fresh-scene
model/light/LOD/glow equality and unchanged resident material identities. This
does not prove native birth, an exact retained lighting-history checkpoint or
live ONLINE migration. Current-root GCC syntax and source audits pass; four
unrelated later Python changes are preserved. Evidence:
`.local/scene-retirement-20261019/verification.json`.

All recorded private scratch is absent; 34,926 generated files / 1,320,113,892
logical bytes removed, with no binary, capture or installed runtime retained.
The enclosing runtime still must invoke these seams within complete native/
GPU/device/audio adoption. Exact scene-history continuity, survivor readiness,
timing/memory, delivery and actual rendered/physical/WAN/full-ONLINE acceptance
remain open. Default migration stays gated and the full goal stays active.
Gameplay and match/postgame voice remain direct and player-hosted.

## Runtime presentation publication hook, 2026-10-19

Native runtime host promotion and surviving-client rebind now accept a prepared
presentation commit. It runs after decision/native/admission/audio validation
and successor allocation, before native role/endpoint changes and transport
publication. Refusal preserves the pending role for retry; successful publication
must not allocate, block or perform device I/O. Runtime reentrancy is rejected
before it can enter the locked session. Callers must capture/preflight all
owners first and leave them unchanged when returning false.

Three normal and three selected-owner ASan/UBSan/leak CTests pass. Both real
twelve-player direct-session variants exercise refusal/retry, once-only host
publication of an exact receipt-bound four-owner cold CPU effect cut, unchanged
native seal, client publication before endpoint rebind, frame/foreign-executor
rejection and runtime reentrancy rejection. Each case continues twelve native
ticks and twelve encoded direct voice packets after host loss, with no master
gameplay/media relay or unfinished report publication. This is local native
component evidence, not rendered/active-effect or physical/WAN acceptance.
The private native executable builds; current-root GCC syntax and source audits
pass. Evidence: `.local/runtime-presentation-commit-20261019/verification.json`.

All recorded scratch is absent; 34,956 generated files / 1,345,909,397 logical
bytes removed. No binary/runtime is retained or installed; six later unrelated
source changes are preserved. The windowed loop still needs complete prepared
adoption. Existing lens retirement waits for GPU work and must defer that
retirement outside the locked commit before using this hook. Active scene/FP/
effect/history, GPU/device/playing-audio adoption, all-survivor readiness,
timing/memory, delivery and rendered/physical/WAN/full-ONLINE gates remain open.
Default migration stays gated and the full goal remains active. Gameplay and
match/postgame voice remain direct and player-hosted.

## Deferred lens GPU retirement — M01 checked, 2026-10-19

Prepared renderer retirement now retains an actual device use, detaches the old
GPU query owner and cached results with swaps, and consumes eligibility without
destroying the caller's token. Query/draw/group publication retains retired CPU
and GPU owners until the caller releases the token after the enclosing native
activation returns and its session lock is released. That later executor-local
destruction still waits for real GPU work; no few-frame latency bound is claimed.
Draw/loading, resize, harvest and immediate retirement invalidate stale tokens.
Lease eligibility checks now read atomic state without taking the device gate;
new-use admission and retirement remain serialized. The renderer executor is
unconditional, including builds without optional particle graphics.

A coherent private native build and 12 focused CTests pass. Real Vulkan link
observers record zero wait/destruction/query-read calls during both renderer
and paired publication; later token destruction records one device wait, two
query-pool destructions and the expected buffer/memory/pipeline/layout cleanup.
Six rejected/cancelled paired preparations preserve actual pending GPU results.
Two actual native worlds retain their sealed boundary and resident materials;
the original 45 query quads and 45 final-draw callbacks/25 quads still pass.
The device lease regression and nine current-root GCC syntax probes pass,
including renderer builds with and without particle graphics. Three Khronos
logs are empty. This is normal real-GPU/component evidence, with no new
sanitizer, visible-flare migration, physical-controller/headset or WAN claim.
Evidence: [verification.json](../../.local/lens-deferred-retirement-20261019/verification.json).

All 17 selected owners match the qualified 15,903-input frozen source cohort;
five later unrelated root changes are recorded and preserved. Cleanup removes
35,248 generated files / 1,552,133,164 logical bytes (1,649,754,112 allocated),
with no binary, capture or installed runtime retained. All recorded private
paths/processes and the cleanup/finalizer scratch are absent. Shared filesystem
free-space change is recorded separately from removed-file size.

M01 is checked; M02 is the next task. The exact runtime/event cut, complete
scene/model/FP/camera/HUD/input/playing-audio adoption, all-survivor readiness,
catch-up/memory/timing, coherent delivery and rendered/physical/WAN/full-ONLINE
acceptance remain open. Default migration stays gated; the full goal remains
active. Gameplay and match/postgame voice stay direct and player-hosted, with
Vista ONLINE/clan assets and widescreen requirements preserved.

## Owning runtime receipt/frontier history — M02 partial, 2026-10-19

An accepted continuity receipt now retains its immutable decoded native state
and exact couch slots across later batches and client destruction. The actual
windowed loop captures that receipt, grouped CPU effects, full-salt runtime
event/binding frontiers, undelivered sound/request queues and a freshly captured
pending-effect cue checkpoint as one entry. Internal predecessor tracking and
exact native proof reject foreign, incomplete, stale or substituted receipts;
capture failure preserves older completed entries. Already-playing audio and
the complete scene/HUD/input/resource borrower set remain separate owners.

The coherent private native build and all six normal CTests pass. The actual
four-view direct-DTLS runtime accepts 259 receipts, captures 258 with one initial
unproven predecessor left unretained, peaks at 128 entries, binds two native cuts
and renders 381 frames. Whole-fixture peak RSS is 7,098,512 KiB, including native
worlds/renderer; this is not an isolated history-byte budget or catch-up proof.
The actual late-join source impulses are submitted once to SDL; physical audio
is unverified. Four unsuppressed ASan/UBSan/leak tests pass with four affected CPU
owners and their four fixture TUs instrumented; native-generated/dependency
interiors and the rendered runtime remain outside that instrumentation scope.
Nine current-root GCC syntax checks pass. Evidence:
[verification.json](../../.local/runtime-receipt-frontier-20261019/verification.json).

An initial sanitizer takeover failure was traced to the collocated fixture
leaving direct native traffic idle during background shadow warm-up. A controlled
6.002-second idle reproduces peer removal before resumed snapshots. The fixture
now services real neutral traffic through a six-second warm-up without advancing
the native tick or consuming carried input. Both variants pass normal and selected
sanitizer qualification, then continue all twelve gameplay ticks/direct voice
packets. Production timeouts are unchanged; this does not qualify migration latency.

All 17 selected owners match the 15,910-input frozen cohort. Thirty later root
changes are recorded and preserved; the isolated cohort does not qualify their
combined release. Cleanup removes 35,095 generated files / 1,602,889,260 logical
bytes (1,700,376,576 allocated), verifies all recorded private paths/processes
absent and retires nine one-off helpers (54,316 bytes) after retaining reports,
commands, source hashes and failure diagnostics. No binary/capture is retained or
installed. Shared available-space changes are recorded separately from file size.

M01 stays checked; M02 stays unchecked and the full goal stays active. Next is
complete prepared world-borrower adoption, followed by the remaining M03-M07
history/audio/input/readiness and windowed activation gates. Gameplay and
match/postgame voice remain direct and player-hosted. The user's MCC-only Vista
ONLINE/clan source requirement is recorded in U01/D01; the bounded UI audit found
catalogue-to-runtime metadata/pack and painter-selection gaps, still unaccepted.
