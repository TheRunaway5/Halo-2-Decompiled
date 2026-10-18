# Complete Halo 2 ONLINE

User-requested consolidated goal, 2026-10-13. This is the acceptance checklist
for all ONLINE work discussed in this chat, including the subsequent direct
peer architecture and modern mesh requirements. Earlier narrower completion
claims do not close the remaining items below.

The full scope is registered as the active app goal. This document is its
tracked acceptance checklist; source changes and bounded test passes do not
close requirements that still need integration or physical validation.

## Required architecture

- **Xbox TU1.5 is the sole gameplay and networking parity target** (user confirmation,
  2026-10-14). Earlier-image decompiles are supporting evidence only where their
  correspondence to TU1.5 is established. TU1.1 construction/compatibility is
  not an ONLINE completion requirement. The selected high-resolution Vista
  ONLINE/clan UI assets remain the UI requirement.
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
| UI | ONLINE capitalization; high-resolution Vista assets/fonts throughout ONLINE and clans; readable screens without the obstructing Halo 2 logo; correct real clan-screen dimensions and widescreen bounds | Existing UI work must be retained and checked in delivered builds |
| Social | Complete Discord friends/presence, native invitations, accepted parties and correct launch/return behavior | Existing SDK/native flows require regression and remaining live checks |
| Clans | Complete create/join/invite/manage/leave flows, matchmaking and original supported clan-rank presentation | Existing clan flows; remaining competitive/presentation coverage stays open |
| Couch players | Up to four local players per PC can queue/play together against others; verified secondary accounts retain their own stats; guests retain local PCR rows but publish no personal stats | Eight-player/two-PC native fixture exercised; physical controllers and independent voice devices are separate gates |
| Gameplay transport | Default ONLINE gameplay travels directly between players, with one player authoritative; no master media relay | v12 installed in both launchers and deployed; local native/rendered flows and one consumer-to-Vultr UDP probe passed; two consumer NATs remain unverified |
| Host quality | Select physical hosts from actual reciprocal reachability, latency and upload tests; reevaluate surviving candidates under agreed membership | Native session setup now runs bounded direct UDP upload/echo tests and broadcasts complete peer rows over authenticated DTLS. The coordinator ranks physical owners using measured reciprocal loss, upload bottleneck and RTT while retaining playlist connectivity thresholds; all-legacy setup retains its prior RTT policy. Four real socket peers exchange 768 probes beside payload traffic; two separate eight-seat native sessions select a physical host from actual measurements and retain fresh candidate lists. Five owner cases, 44 backend checks and selected owner ASan/UBSan/leak/TSan pass at the frozen protocol 86 cut. These are observed application delivery rates under bounded load, not ISP capacity or anti-cheat attestations. NAT classification, agreed telemetry/membership generations, automatic validated shadow selection, survivor reelection, full runtime delivery and WAN/hardware acceptance remain open |
| Match voice | Direct peer proximity, team/FFA radio, dead-player audiences, muting and postgame voice; complete source-supported lobby/loading, LAN and device coverage | Match/postgame voice now uses direct UDP; party/clan menu voice still uses its relay and remains to convert; LAN voice, all-participant loading voice, independent couch headsets/mics, masking and original radio audio remain open |
| Master outage | Peer simulation and voice continue through temporary signaling loss; reports retry safely afterward | Temporary master-socket outage acceptance passed:89 simulation ticks and86/88 audible frames continued, then reports uploaded; master-process-restart admission persistence remains open |
| Disconnect survival | A nonhost departure does not end the room; a graceful or abrupt host loss preserves and resumes the actual world | TU1.5 native Match and localhost UDP graceful/silent-peer departures retain roster/profile/result rows and original deferred corpse/equipment without added numeric deaths. The master explicit nonhost-leave and result-review paths preserve remaining peers and store complete admitted rows with survivor-confirmation metadata. A real native nonhost DTLS/signaling shutdown now leaves the host session running. Custom native-owned worlds, complete source admission cancellation, unannounced missing-peer result arbitration, production delivery, physical/WAN checks and host migration remain open |
| Migration | Preserve complete player/unit/weapon/inventory, allocation identities, RNG, timers, projectiles/grenades, props, devices, physics, recording/results, local profiles and voice; promote/rebind a validated survivor | Coupled TU1.5 checkpoints and the 29-operation source journal retain complete native/CPP state, typed inputs, actual returns, clocks, identities and local-view bindings. A negotiated two-cut direct-peer pipeline overlaps transfer/decode with ordered native validation. Matched cold/background fixtures validate 82 phases, reject wrong chains and forged identities with a prefetched successor, repair from full cuts and compare four complete worlds. The background retention case validates 122 phases, drains both admitted batches across actual host loss, survives worker teardown, imports/rebinds four recipient views and continues ten native ticks after old-host departure without fabricated deaths or unfinished reports. Transport normal/ASan/UBSan/TSan and selected native-owner sanitizer checks pass; current-root syntax checks pass. Normal runtime disconnect and stalled-host timeout paths retain a validated recovery world before teardown. Extended rendered scheduling shows reduced backlog but was interrupted by OS memory pressure; sustained catch-up remains unqualified. Owning private camera checkpoints also reproduce driver, passenger and death trajectories across independent native executors in the retained cohort, with complete-world equality and selected owner sanitizers. Native fatal readback now avoids premature FP publication and stale-unit command delivery. Owning host connection/input/camera sidecars also resume the same six-player world across native executors, retaining four remote couch views, queued/consumed action serials and exact complete snapshots without rebirth; normal and selected network-owner sanitizers pass against a freshly built immutable current-source cohort. A private replay pause now admits both pipeline cuts before controlled native execution; repaired current-cohort cold/background cases validate 82/122 accepted phases, with selected receiver-owner ASan/UBSan/leak checks and real UDP graceful/silent-peer departure regression. Explicit camera/host and paired complete-world wire codecs now bind the entire connection/input/camera state to room/generation/membership/world content. A real two-process direct-UDP case imports the paired state, reproduces four native phases and exact snapshots, and retains ten private ticks after old-host shutdown; normal and selected owner sanitizers pass. These fixtures retain producer view mapping, do not elect/promote a host and are not current full runtime delivery. Runtime publisher/receiver integration now negotiates paired full/replay cuts and retains validated host/input/camera state at the exact accepted native boundary. A production two-process stream verifies 70 native phases, two-cut prefetch, complete world/host equality, downgrade rejection, owning retention and ten ticks after actual old-host shutdown; normal and selected owner sanitizers pass. Private hardware continuity, broader mode/caller/combat/PCR and scenery lifecycle coverage, generation-bound identity continuity, agreed membership/cut/epoch certification, actual host/transport/profile/controller/voice promotion, memory qualification, delivery and physical/WAN acceptance remain open. See [continuity evidence](player-host-continuity-progress-20261013.md). |
| Historical playlists | Final-2010 target with the fullest evidence-supported rotations/game coverage; casual/ranked matching follows recovered/public Halo 2 logic, including Max Hoberman's explanations | Final downloaded catalog not recovered; full objective engines/rotations remain open |
| Custom modes | Finish and bundle authored Fiesta Slayer and Halo-3-inspired Infection Slayer, including green zombies, under data/tags and release packaging; include Action Sack | Existing authored variants/packaging must survive transport/build changes |
| Tombstone | Include the installed map in sensible supported Slayer, Team Slayer, CTF, Fiesta and other suitable pools | Fiesta active; staged Slayer/Team Slayer and unfinished objective coverage need their native engine acceptance |
| Competitive policy | Finish supported quit penalties, later ranking behavior and original clan-rank display without inventing missing historical values | Remaining policies/evidence gates stay open |
| Post-carnage history | Upload after the match, validate admission/results/confirmations, store organized persistent report details, support later website lookup by verified Discord username, exclude guest personal history | Existing DB/history and guest tests; preserve across peer migration and master recovery |
| Delivery | Rebuild/install both native launchers, safely deploy backend with state backup/restore verification, package authored data and dependency notices | Every changed delivery needs fresh recorded verification; source presence alone is insufficient |

## Modern mesh extension requirements

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
Retain dirty/concurrent workspace changes and existing Vista asset selection.
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
