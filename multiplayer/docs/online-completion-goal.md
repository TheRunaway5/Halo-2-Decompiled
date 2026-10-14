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
| Identity | Discord login required for persistent ONLINE identity; hide account IDs; preserve chosen Halo screen names | Existing implementation; retain in every new transport, invite and result path |
| Native navigation | Quickmatch, Optimatch and Playlists launch real admitted online games; server browser joins community servers | Match launch exists; community server joining remains outstanding |
| UI | ONLINE capitalization; high-resolution Vista assets/fonts throughout ONLINE and clans; readable screens without the obstructing Halo 2 logo; correct real clan-screen dimensions and widescreen bounds | Existing UI work must be retained and checked in delivered builds |
| Social | Complete Discord friends/presence, native invitations, accepted parties and correct launch/return behavior | Existing SDK/native flows require regression and remaining live checks |
| Clans | Complete create/join/invite/manage/leave flows, matchmaking and original supported clan-rank presentation | Existing clan flows; remaining competitive/presentation coverage stays open |
| Couch players | Up to four local players per PC can queue/play together against others; verified secondary accounts retain their own stats; guests retain local PCR rows but publish no personal stats | Eight-player/two-PC native fixture exercised; physical controllers and independent voice devices are separate gates |
| Gameplay transport | Default ONLINE gameplay travels directly between players, with one player authoritative; no master media relay | v12 installed in both launchers and deployed; local native/rendered flows and one consumer-to-Vultr UDP probe passed; two consumer NATs remain unverified |
| Match voice | Direct peer proximity, team/FFA radio, dead-player audiences, muting and postgame voice; complete source-supported lobby/loading, LAN and device coverage | Match/postgame voice now uses direct UDP; party/clan menu voice still uses its relay and remains to convert; LAN voice, all-participant loading voice, independent couch headsets/mics, masking and original radio audio remain open |
| Master outage | Peer simulation and voice continue through temporary signaling loss; reports retry safely afterward | Temporary master-socket outage acceptance passed:89 simulation ticks and86/88 audible frames continued, then reports uploaded; master-process-restart admission persistence remains open |
| Disconnect survival | A nonhost departure does not end the room; a graceful or abrupt host loss preserves and resumes the actual world | TU1.5 native Match and localhost UDP graceful/silent-peer departures retain roster/profile/result rows and original deferred corpse/equipment without added numeric deaths. The master explicit nonhost-leave and result-review paths preserve remaining peers and store complete admitted rows with survivor-confirmation metadata. A real native nonhost DTLS/signaling shutdown now leaves the host session running. Custom native-owned worlds, complete source admission cancellation, unannounced missing-peer result arbitration, production delivery, physical/WAN checks and host migration remain open |
| Migration | Preserve complete player/unit/weapon/inventory, allocation identities, RNG, timers, projectiles/grenades, props, devices, physics, recording/results, local profiles and voice; promote/rebind a validated survivor | Native Simulation/Match composition retains typed state and exact collision traversal, with owned Slayer/King and actual coupled TU1.5 source-world evidence. Initial and active four-player Lockout cuts continue for forty source ticks, including changed SMG ammunition and an attached grenade. An eight-player source fixture rebinds four local views per PC through a disposable staged executor and preserves pending local departure. A complete owning source archive also continues forty ticks in a separately executed process with exact checkpoint equality, including GCC/Clang exchange in both directions. Authenticated direct UDP delivery now restores an eight-player source cut under packet loss with four rebound local views; a separate native session carries archive bytes beside gameplay/Opus through master signaling loss. Complete base-bound deltas now reduce later actual source cuts from about8.7MB to63/35KB, with exact canonical reconstruction and prepared local-view restoration; preparation is still about1.8s in the fixture. A bounded background publisher now uses actual native ONLINE sessions and prepared-world receipts; capture still costs about470ms and the transport sanitizer audit remains open. Initial source identity distribution is now integrated: the actual TU1.5 host exports immutable full allocations/identities over authenticated peer UDP, bound to the accepted couch roster; the normal runtime publishes after native profile admission, and the eight-player restoration test no longer uses an identity file. Metadata receipt remains separate from prepared-world acknowledgement. A private client source executor and bounded asynchronous receiver now prepare complete full/delta cuts using all admitted identities and real grouped profiles. The native test preserves that world across actual host transport loss, transfers the exact cut, applies the original four-player host departure and continues ten surviving ticks; decoder cancellation is covered. The latest protocol-81 native run has about995ms cold construction and2.65/3.34s preparation. The expanded selected sanitizer run reaches one prepared cut but takes about7s and times out native peer liveness; full sanitizer completion and loading-time scheduling/performance remain open. Automatic host publication, input catch-up, certified promotion and retention before runtime teardown remain open. The rendered receiver confirms this exchange, but full rendered acceptance fails at the host source-shield authored-part guard. Newer source-world sidecars/lifecycles, source combat/PCR completion, original scenery deletion `10F2F0`, native-owned custom departure, source identity continuity across certified generation changes, automatic runtime capture/input catch-up, agreed cut/epoch certification, full hardware/profile/voice rebinding and gameplay handoff remain open |
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
