# Complete Halo 2 ONLINE

User-requested consolidated goal, 2026-10-13. This is the acceptance checklist
for all ONLINE work discussed in this chat, including the subsequent direct
peer architecture and modern mesh requirements. Earlier narrower completion
claims do not close the remaining items below.

The full scope is registered as the active app goal. This document is its
tracked acceptance checklist; source changes and bounded test passes do not
close requirements that still need integration or physical validation.

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
