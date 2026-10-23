# EOS completion goal

Active goal requested October 22, 2026. Keep Halo playlist, party, rank and
accepted-result rules; use EOS for account-bound general stats, standings,
Sessions and peer traversal/relay. Full carnage history remains on the master.

Implementation language: new EOS transport, account/credential and online
coordination logic belongs in the existing Rust crates. The trusted EOS worker
also stays in Rust; the website bridge stays in TypeScript. Keep C++ changes at
the official SDK boundary and the existing native game/UI/state owners. Retaining
an existing native camera or pose in a checkpoint must not introduce a second
implementation of its original game behavior in Rust. Existing Python Halo
rules and the accepted-result ledger keep their current ownership.

- [x] Install the qualified client and single Play → intro → title → menu route.
- [x] Bind two distinct real Steam accounts to EOS from installed clients.
- [x] Read back both profile revisions through the trusted publisher and public
  website API: 27 fields for the legacy account, 29 for the new account.
- [x] Preserve the legacy unknown join date and publish the new registration's
  actual timestamp. Live aggregate counts show two users and zero guests.
- [ ] Finish a two-account match through the production EOS handoff.
- [ ] Verify the accepted native carnage report, guest exclusion, both players'
  history, exact career/accuracy increments and duplicate-safe EOS read-back.
- [ ] Verify ranked-result publication and playlist isolation.
- [ ] Verify an actual match through forced relay, then automatic routing.
- [ ] Verify two consumer networks and real microphone capture/playback. The
  user can arrange that hardware later; two local clients do not close this item.
- [ ] Complete native world/presentation/input/audio adoption and pre-election
  readiness; test graceful, abrupt and repeated host losses through EOS.
- [ ] Retire legacy transport after compatibility and gameplay cutover pass.
- [x] Deliver profile-scoped remembered Steam sign-in, Steam/clan invite tabs,
  and corner party notifications using the project's controller artwork.

## Current regression

The October 23 live retry still fails about two seconds after finding the other
player. The displayed text is “Matchmaking service unavailable. Retrying...”;
the production trace shows successful session, playlist, ticket, transport and
signaling responses followed by cancellation/requeue. The installed client
discards its retained transport failure reason at that boundary. The underlying
client failure remains under investigation; successful HTTP is not playable
matchmaking acceptance.

The two installed clients reach “Attempting to join a game,” remain in joining
for approximately 40 seconds and retry. Both WebSocket signaling connections
receive HTTP 101; neither account reaches playing. The exact public symptom is
captured by:

```sh
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/check_online_population_phase.py \
  --phase playing --users 2 --timeout 65 \
  --report analysis/eos-two-account-join-20261022/before.json
```

This command failed against the live repeated attempt. It checks aggregate
activity only; native session diagnostics and actual gameplay must establish
the cause and subsequent acceptance. No test scores or fabricated reports are
inserted into the production account database.

The minimized official-SDK regression reproduces `EOS_AlreadyConfigured` after
the profile platform releases the last SDK owner and matchmaking attempts to
initialize again. Platform release now retains the process SDK; explicit main
shutdown occurs after session owners are destroyed and before static teardown.
The real SDK profile/recreate/concurrent-platform/shutdown regression passes.
The original two-client playable check remains required after installation.

The real host OS credential-store regression also passes profile/origin
isolation and deletion. Refresh credentials remain in that secure store;
non-secret stable profile slots survive profile renames. The master issues
eight-hour sessions from fixed thirty-day refresh credentials and revokes the
refresh credential and its issued sessions on explicit Sign Out.

The scoped production master rollout for remembered sign-in and Steam/clan
invitations, with independent bounded refresh storage and indexed expiry,
passed nineteen applicable HTTP regressions, actual-image checks, health and
source-hash checks. The separate in-progress party roster appearance extension
is excluded from that rollout. No Steam passwords or private account identifiers
appear in the evidence receipts.

The replacement runtime `bin/engine-builds/2571dcba509a4e73a68866885022f7de/halo2_native`
is installed and selected by both launchers. Four focused identity/party tests,
eight launcher checks, twelve MCC submitted-frame cases and four frontend
screens passed. All eighteen Xbox/PlayStation prompt runtime assets match the
project's authenticated banks. The frozen source and executable hashes are
recorded in `analysis/eos-join-fix-runtime-final`; unrelated renderer edits after
that freeze remain preserved in the checkout. The selected installed runtime
is intentionally retained; its current hash belongs to the selection manifest.
The build scratch and raw captures were
removed. The user deferred the next real two-account join attempt until tomorrow;
this build does not establish live matchmaking or completed-match acceptance.

Same-PC testing must retain the second-client helper's separate XDG/EOS data
namespace. The official Linux SDK 1.19.2.1 now verifies that setup through the
actual Rust transport: two isolated child processes receive distinct anonymous
transport identities, and a fresh process reusing the first namespace receives
the same identity. Credentials stay under each owned XDG data keychain; no
write escapes that namespace. The retained receipt contains hashes only and
its SDK/build/cache scratch was removed. This proves the helper's Linux device
namespace, not gameplay or other operating systems. Ordinary same-namespace
processes intentionally share a Device ID; distinct Steam/profile identities
alone do not isolate that separate anonymous transport identity.

## Accepted-result publication qualification

An isolated HTTP/ledger → TypeScript publisher → actual EOS SDK qualification
now passes against sources matching all 34 deployed master files and both
website publisher modules. A casual result with six guest seats followed by a
ranked result produces the exact cumulative matches/wins/kills/deaths/assists,
shots and hits. The accepted original Halo level 2 is read back from the actual
EOS ranked leaderboard. Duplicate commits, simulated lost SDK publication replies
and publisher restart preserve the verified values and outbox revision.

These are explicitly declared PCR fixtures with local mock authentication and
one isolated generated device-test PUID. Production player rows are unchanged.
This closes the combined publication component gap; the played native match,
real-account result acceptance and all other unchecked goal items remain open.
The source/SDK/Node pins, command results and scope are recorded under
`analysis/eos-accepted-result-pipeline-20261023`; both owned SDK/build trees and
all raw fixture databases were removed. No large artifacts were retained.

## Migration boundary

Current source has native retention, role preparation and several foreground
checkpoint owners. Ordinary matchmaking still leaves `peer_host_migration`
disabled. `prepare_peer_migration` has no production caller, and
`prepare_foreground_handoff` explicitly remains a private partial preparation.
`MultiplayerRuntimeAuthority::prepare_takeover` currently requires an already
learned decision. An explicit private native import can now reconstruct an exact
strong common pair earlier; complete foreground preparation and the production
readiness/election driver still need their own implementation. EOS Sessions or
an election alone do not close this gap.
Preserve these guards while completing the owning transactions.

The native session activation boundary now calls Rust's exact-cut preparation
guard before constructing successor owners or invoking a native commit. A
majority decision alone cannot activate: every fixed physical survivor must
advertise the identical cut, and successor membership must be exactly the old
membership minus the departed host. Couch seats do not add participants.
Readiness remains write-once per generation, so future complete preparation must
use the agreed common cut before publishing any Ready. The Rust protocol's 51
tests, ten bridge/configuration tests and strict exact-version Clippy checks
pass. Two opt-in SDK/credential-service tests remain excluded from this offline
Rust cohort; their earlier live checks are recorded separately.
Three native session cases passed before the subsequent pacing change: repeated direct handoff, measured host
selection and queued synthetic audio retirement. They use declared native
preparation metadata, actual local DTLS and four couch seats per physical peer.
The separately retained-world takeover qualification exposed a sender fairness
bug: advancing the peer cursor on every inspected slot repeatedly favored one
recipient after the shared byte credit ran out. The second physical peer stopped
progressing and expired. Scheduling now belongs to a bounded Rust cursor,
advanced only after an actual emitted packet. A real-core regression with two
8,388,745-byte recipients, the unchanged 512 KiB/s rate, 16 KiB burst and 15-second
idle limit now delivers both exact payloads and their byte ACKs in 34.13 simulated
seconds at packet budgets two and four. The previous version starves one peer
under each budget. Byte receipt does not establish native validation/readiness.
The fresh selected-MCC native cohort measured the actual paired payload as
19,380,091 bytes. Two lossless copies need at least 78.87952 seconds of charged
wire credit before native validation, exceeding the fixture's sixty-second
publisher override. The fixture now uses the existing five-minute production
publisher default; transfer rate, burst, fifteen-second idle and five-minute
transfer lifetime are unchanged. Its ninety-second stage and 240-second process
limits remain unchanged. The native engine compiles and seven focused CTests
pass, including selected-MCC common-cut publication. The full takeover retry now
passes both independent native deliveries, repaired-cut retention, the Rust
all-survivor activation exclusion, native publication and exact client rollback.
Its CPU sound fixture also resolves the actual typed plasma-fire asset and all
four authored pending cues instead of Xbox-only literal IDs.
The previous whole takeover test failed at its later post-rebind heartbeat:
the private replacement host started its liveness clock before native
preparation, and its first asynchronous packet could arrive after that clock
expired.
The existing native host now preflights a one-shot liveness companion before
adoption and rebases its clock only after a successful native commit. Saved
idle debt, actual input receipts and the ordinary expiry policy are retained.
Any actual receive/broadcast service consumes eligibility; ordinary hosts,
foreign executors and repeats cannot reset it. A focused queued-transport
regression covers six seconds of private preparation, an unavailable first
genuine packet, thirteen actual input acceptances and normal five-second
expiry. Fresh qualification now passes: the native executable builds, the
focused original-reference clock test runs in 15.25 seconds, and the selected-MCC
takeover CTest completes both direct-receiver and retained-worker paths in
301.56 seconds, with no skips. The latter covers actual local DTLS, both exact
native preparations, atomic CPU adoption/rollback, twelve continued native
ticks and encoded direct voice. Each path services its real post-rebind
heartbeat for more than five seconds while the shadow warms, without stepping
the native world or consuming its carried input. Physical controllers, rendered
adoption, playing microphone audio, EOS/WAN delivery and full pre-election
preparation remain outside this result.
The earlier failed steps remain recorded under
`analysis/eos-rust-native-fanout-20261023`; the fresh passing source pins and
limits are under `analysis/eos-host-activation-clock-20261023/final-report.json`.
Both owned source/build/SDK trees and one-off coordinators were removed. No
replacement runtime was installed from these migration cohorts.
This guard does not itself supply the missing complete owner transaction.

The host/cut agreement state machine and all eight control-packet layouts now
also belong to Rust. The native class retains synchronized ownership and typed
conversions, including invalid-index rejection before narrowing. The C++ codec,
ballots, promises, vote learning, retries and election policy were removed from
that adapter. Existing snapshot-selection, source-admission, QoS and
remaining authority-transfer policy still have C++ owners; this does not claim
that every online protocol has been ported. The read-only next boundaries are
recorded in `analysis/eos-rust-agreement-20261023/remaining-boundaries.json`.

The scoped native executable builds with this Rust agreement adapter. Its
public boundary, actual direct-DTLS, native session, measured handoff and queued
audio tests pass. The selected-MCC takeover also passes both direct-receiver
and retained-worker paths in 301.94 seconds. A separate original/Rust comparison
matches all 216,986 emitted control packets and 1,393,700 complete public-state
snapshots across the existing deterministic loss, partition and competing-driver
schedules. The 504,523,863 canonical bytes were streamed into SHA-256, not retained
as a capture. Both implementations produce
`dcc33f25997887537057e99a3d01c8a19aec2538961e94d612bb5696fcdedd20`.
New public integer/clock/queue boundary assertions also pass against the original
implementation. The initial comparison wrapper included two SDK lifecycle log
lines and the first configuration omitted the MCC test option; the corrected
focused cohort passes both registered cases. All seven distinct native checks
therefore pass on identical production-code pins, with no skipped CTests.
These checks retain the same native-preparation and live-acceptance limits above.
Both disposable source/build/SDK trees and the one-off coordinator were removed;
no large output or replacement runtime was retained from this qualification.

The common-cut agreement now also belongs to Rust: all nine control layouts,
fixed-membership association/apply/release and Abort barriers, monotonic clocks,
queued retries and the wrapping scan cursor. Its C++ class only synchronizes and
converts owned values. The original C++ coordinator and codec were removed.
The expanded nine-group native boundary suite passes against both implementations.
The original six-group differential mesh matches 185,646 emitted packets,
213,920 deliveries and 70,397 complete public states across 50 meshes and 22,839
steps. All 107,676,272 canonical bytes were streamed into SHA-256; both versions
produce `386dd77b7fd52ce462f66ee65e7f1683e11b0afacacf04a997acf057435e672c`.
Actual authenticated session checks also cover selected-Abort publication refusal
and fenced retirement ACKs, with an unrelated-Abort positive control. Common
receive and final native publication now serialize through the session lock;
the pure Rust metadata owner invokes no native callbacks under that lock.
The late-arrival ordering is source-reviewed rather than a scheduled race test.
Receipts and the remaining foreground/pre-election owner map are under
`analysis/eos-common-cut-native-20261023`.

An explicit quiescent native import now retains one exact strong common pair
before an election, without exposing its mutable world, endpoint, publication
token or Ready claim. Certificate-gated role finalization consumes that same
private target. A constructor failure after consumption clears its invalid
wrapper while retaining the authentic pair for fresh reconstruction; refused
certificates preserve an unconsumed target. The selected-MCC fixture verifies
cold native clock consumers, idempotent preparation, retained ownership through
history eviction, Release, unrelated Abort and later pair rotation, and the
genuine fingerprint-failure/retry path. It passes in 170.44 seconds; the complete
native takeover fixture passes both paths in 301.87 seconds. All eight distinct
native checks pass across the initial and repaired cohorts on identical
production-code pins, with no skipped cases. The initial failures were fixture
setup errors: an outstanding frame must throw before certificate checks, and an
already-evicted replay receipt cannot be associated retroactively. The repairs
preserve the existing history, rate and deadline limits.

These results do not supply a production early-preparation/Ready driver or full
foreground adoption. Selected pending-import Abort retirement and AwaitingWorld
native-import admission remain source-reviewed rather than directly run by the native fixture.
The windowed paused branch now services exact Abort/context retirement through
a scoped outer-owner gate. It drops matching prepared collision/lens bindings
and checkpoint/reservation aliases before the native target can retire or ACK.
A refused gate retains native owners, prevents a new frame and defers service.
The guard clears its callback before captured holders die, and the callback
cannot reenter the native runtime. Provisional snapshot candidates keep their
separate copied history/delivery lease until genuine SnapshotClosing: waiting
for that retirement before CommonAborted would deadlock the two protocols.
Candidate-to-common lineage is recorded only after actual native/foreground
association; a candidate nonce is never treated as a seal id.
The renderer, camera/input, playing-audio and aggregate publication companions
remain open. The migration gate stays disabled, and disposable validation
builds do not replace the launcher-selected runtime.

Rust now owns the bounded survivor-selection handshake and its kind-13 codec.
The explicit `survivor_cut_selection` option defaults off and requires every
predecessor physical peer's feature-2048 capability; mixed groups cannot use
the diagnostic legacy Ready path. Each survivor independently authenticates
one immutable inventory of at most three actually frozen/retained cuts. All
survivors must accept the identical SHA-256 transcript, including protocol,
generation, fixed membership, every inventory lease/entry and the selected full
seal. Selection chooses the highest exact cut shared by every survivor with
at least one genuine irreversible CommonCommit/Release observation. Native
imports, associations and peer Accepts cannot supply that witness or another
peer's missing inventory. Session publication checks every claimed witness
against its actual exact Common phase while holding the generation lock.
Missed Offer and partial Release therefore use retained data rather than local
committed-slot labels. Healthy rolling imports still leave Ready unset.

The same session lock serializes incoming Common Abort, local selected-cut
Ready and final activation checks. Early actual election Ready values stay in
the existing immutable election bank until local selection completes. Neither
an incoming inventory nor completed selection creates a native freeze, native
readiness, a lease or a role change. The caller must already hold the actual
owners before publishing an inventory, then finish the complete preparation
before Ready. Invalidation is terminal for this one generation attempt: no
timeout removes a survivor, replaces Ready or resumes gameplay. A future retry
needs an actual all-survivor owner-retirement barrier. The production freeze/
preparation driver and complete adopter remain unimplemented. Component Rust,
CXX and native receipts are under `analysis/eos-survivor-selection-20261023`;
they do not constitute played-match or host-loss acceptance.

The fresh scoped Release engine builds with these changes. All 51 Rust protocol
tests, ten bridge tests and strict Clippy checks pass; the two opt-in SDK/keyring
service tests remain excluded. All twelve focused native CTests execute and
pass without skips. This includes real CXX packet/field boundaries, a sixteen-PC
predecessor mask under loss/reordering, authenticated partial-Release selection,
mixed capability refusal, fabricated witness rejection and genuine delayed
Abort. The native takeover fixture passes both paths in 302.43 seconds. The
selected-MCC common-cut fixture passes in 170.28 seconds and exercises refusal
of actual pending native-pin retirement: outer foreground aliases stay held,
frame entry and callback reentry are blocked, and only actual alias destruction
permits Abort ACK. An unrelated selected native import survives. Selected
pending-import Abort, context-retirement and obsolete-client gate branches are
still source-reviewed rather than separately exercised. Early actual Ready
reordering is tested through the real election/selector component bank; the
equivalent authenticated Session interleaving and selection-enabled successor
round are not directly scheduled by this cohort.

The source snapshot preserves unrelated renderer/GPU work in the shared tree
and qualifies only the declared native caller patch on the prior scoped
baseline. It does not qualify their combined dirty runtime. All owned Cargo,
source/build/SDK scratch and one-off helpers were removed. The compiled engine
was not installed and no large artifacts were retained. The exact remaining
camera/input, HUD image and playing-audio owners, plus the genuine prospective
native-publication dependency, are recorded in `remaining-owner-map.json` and
`predecision-native-seam-plan.json` beside the new receipts.

The private prospective native role now derives its exact predecessor,
successor and full selected seal in Rust. Native engine hooks perform the
existing departure, camera and lens operations before sealing the private
target. The same actual native publication scope survives the preparation
wait and certificate binding; rejection preserves it, and certification moves
the prebuilt authority without importing or mutating the native world again.
Ordinary getters and frames stay excluded until explicit post-adoption scope
release. This metadata and native-owner seam does not publish complete Ready.

Audio can now prepare against that target through its genuine weak binding,
including exact strong Cache ownership. It checks immutable authored/PCM
compatibility while preserving saved voice, RNG and variation histories.
Native, timing and SDL exclusion are ordered at final publication; no timing
or SDL hold spans the election. The new native-target fixture passes both
takeover routes with exact PCM/RNG replay, fresh voice receipts and default
inactive clock-bank preservation. Active script admissions under this new
native binding are not separately exercised.

All 56 Rust protocol tests and 12 bridge tests pass with strict Clippy; the two
existing opt-in service tests remain excluded. The scoped Release engine
builds and contains the actual Rust role and bridge symbols. All 16 focused
native CTests pass without skips: the original 13 integration checks plus
three existing Audio/announcer regressions. The new common-cut mode checks
actual preparation before election, retention through history eviction and
unrelated Abort repair, certificate/fingerprint refusal, missing-survivor and
callback veto, and native-only adoption with explicit release. Its survivor
readiness remains deliberately declared and does not certify missing outer
owners. A focused Audio linker regression was reproduced and fixed by adding
the three genuine missing implementation files to its object closure. Updated
wrapper marker guards were checked against each captured real native output.

Receipts are under `analysis/eos-prospective-native-20261023`. The same owned
build was reused for the focused link repair; all source/build/SDK/Cargo
scratch and the one-off coordinator were removed, with no large artifact
retained or runtime installed. Qualification keeps the earlier scoped native
caller on its authenticated baseline and preserves unrelated shared GPU work.
The complete production freeze/preparation driver and full
renderer/input/playing-audio transaction remain open, as
do played-match publication, WAN relay and microphone acceptance. Both
migration defaults stay disabled; M02-M07 remain open.

Local scope producer/observer polynomials and full zoom/unit/reset/death
lifetimes now belong to the existing native pose checkpoint. The authentic
four-view pose rollback and the original 28-frame/112-phase FOV oracle pass.
The combined selected-MCC common-cut run exposed an earlier native collision
closure gap during shadow-world warming, before the new restoration assertions.
The authenticated original capsule-overlap routine is now translated without
changing the existing 310 bodies. The native executable builds, and all three
camera/receipt checks now pass, including actual independently validated common
cuts and composed historical publication for three physical sessions and twelve
couch players. Their owned build/source/SDK scratch was removed. No replacement
runtime was installed from this migration cohort.
These changes do not enable default migration or close M02-M07.

Held foreground and lens preparations now bind once to an opaque witness from
the actual retained native scope, exact target, original Cache owner, validated
cut and full execution boundary. Expired or replacement scopes cannot revive a
preparation. Private counter and Cache bridges preserve the public native
guards; the receiver passes its original validated cut into takeover creation.
The prospective common-cut fixture publishes its headless routing owner once
after certification, retaining the same native scope across repair and vetoes.

The scoped engine builds and 17 selected checks pass. The final GPU fixture
passed both receiver routes with Khronos validation on private Xvfb/software
Vulkan. Each observed 168 actual ring views on the original strict path and
zero on bound ready/commit, zero Vulkan work during publication, preserved
pending results and deferred GPU retirement after native/timing/SDL holds.
Preborrowed raw const views remain permitted; no read gate was invented.
Sixteen unaffected results were retained only after all eleven targets built
and ten unaffected binaries plus twenty compile/link records stayed identical.
The actual Rust derivation and CXX bridge symbols are linked into the engine;
their sources still match the earlier 56/12-test and strict-Clippy qualification.
Receipts are under `analysis/eos-held-foreground-20261023`.

This is component qualification: the common-cut foreground control disables
render policies, and the GPU case uses empty queries and an empty Audio bank.
Full production preparation/client-role scheduling, render/HUD/input/device and
playing-audio aggregation, playable host loss, played-match publication, WAN
relay and microphone acceptance remain open. Migration defaults stay disabled;
no experimental runtime is installed and M02-M07 remain open.

Retained checkpoint inventory now has an executor-owned opaque native lease;
callers receive only a weak identity. It retains genuine deduplicated
native/foreground pairs across history and committed-slot rotation. Matching
Abort/context retirement invalidates the lease after the outer retirement gate
and before Abort acknowledgment. Rust derives irreversible Common provenance
and classifies admission; the original C++ provenance loops are removed.

The scoped engine and seven selected targets build. All 14 focused CTests pass,
including actual lease-only native retention, pending Abort retirement and
independently frozen physical inventories selecting the exact common cut
without Ready/Decision/activation. Fresh Rust qualification passes 61 protocol
and 14 bridge tests (two existing service-dependent bridge tests remain ignored),
plus strict all-target Clippy. Actual Rust implementations and CXX wrappers are
present in the engine. One frozen source generation was used without repairs.
Receipts are under `analysis/eos-retained-inventory-20261023`; all owned
source/build/SDK/Cargo scratch was removed and no runtime installed.

An immutable historical lease is not a provenance archive: freeze still
revalidates current Common witnesses and may refuse after normal metadata
rotation. Production scheduling must handle that race explicitly. Source-pinned
next-owner audits record the missing pre-election live-client preparation,
physical input and playing Audio/Effects/Damage aggregate, including real
device-cursor association and capture costs. These changes do not enable default
migration or close M02-M07, played-match publication, WAN or microphone acceptance.

The surviving-client alternative can now retain an authentic historical receipt
and one actual resident replica before election, separately from its private
native self-host target. Rust derives the endpoint-neutral role and validates
the eventual successor metadata; the native owner separately checks the full
independently learned certificate. Late binding moves the same prepared owners.
Role-specific outer retirement gates preserve both alternatives on refusal and
retire only the loser on permission. Matching Abort makes metadata ineligible
immediately while the veto may still retain the actual banks until retirement
and acknowledgment.

The scoped engine and all seven targets build. All 16 selected CTests pass,
including both genuine role alternatives, exact historical client publication,
callback and retirement vetoes, full-certificate refusals and matching Abort.
Fresh Rust qualification passes 66 protocol and 16 bridge tests, with the same
two service-dependent bridge tests ignored, plus strict all-target Clippy.
The actual Rust and native bridge functions are defined in the linked engine.
One frozen generation was used without repairs; owned build/source/SDK/Cargo
scratch was removed and no experimental runtime installed. Receipts are under
`analysis/eos-prospective-client-20261023`.

These are native-owner and headless CPU routing/receipt controls. Persistent
replica staging and direct owner moves support no restaging; unchanged native
clock calls are not an allocation counter. These controls do not establish the
complete render/HUD/input/device/playing-audio aggregate.

Atomic preparation scheduling now belongs to a Session-owned Rust driver. It
uses the actual HostAgreement suspicion clock, generation-bound opaque requests
and genuine retained inventory leases. The Session checks fresh host traffic
and current Common provenance under the same mutex as immutable publication.
Healthy return or stale provenance schedules retirement; a matching Common
Abort takes priority. Published rows remain immutable and cannot be reset.

The explicit native executor retains the actual scope, resident replica and
inventory while an outer owner vetoes cleanup. It destroys those unpublished
owners outside the Session lock before Rust can authorize a fresh attempt.
Late Ready, fences or changed generations prevent resumption. A direct native
test covers cancellation, repeated vetoes, actual cleanup, fresh retry and
exactly-once inventory publication without manufacturing Ready or adoption.

The scoped engine and seven targets build with all 18 selected CTests passing.
Fresh Rust qualification passes 78 protocol and 18 bridge tests, with the same
two service-dependent bridge tests ignored, plus strict all-target Clippy.
Both source generations passed; the sole declared refresh added the direct
native positive-path assertion, after which all 18 tests ran again. The engine
defines all 12 reviewed driver/adapter symbol groups. Owned build, source, SDK,
Cargo and XDG scratch was removed; no experimental runtime was installed.
Receipts are under `analysis/eos-migration-preparation-20261023`.

The production gameplay caller still needs genuine local admission suspension
before invoking this executor, plus the complete foreground and playing-audio
aggregate. Migration defaults remain disabled, M02-M07 remain open, and
played-match, playable host-loss, WAN relay and microphone acceptance are
unchanged. Legacy transport remains until playable cutover.

## Evidence

- `analysis/eos-migration-preparation-20261023/final-report.json`
- `analysis/eos-migration-preparation-20261023/final-qualification-review.json`
- `analysis/eos-migration-preparation-20261023/cleanup.json`
- `analysis/eos-migration-preparation-20261023/next-production-owner.json`
- `analysis/eos-prospective-client-20261023/final-report.json`
- `analysis/eos-prospective-client-20261023/final-qualification-review.json`
- `analysis/eos-prospective-client-20261023/cleanup.json`
- `analysis/eos-retained-inventory-20261023/final-report.json`
- `analysis/eos-retained-inventory-20261023/cohort.json`
- `analysis/eos-retained-inventory-20261023/source-review.json`
- `analysis/eos-retained-inventory-20261023/cleanup.json`
- `analysis/eos-held-foreground-20261023/final-report.json`
- `analysis/eos-held-foreground-20261023/continuation-3/cohort.json`
- `analysis/eos-held-foreground-20261023/final-source-review.json`
- `analysis/eos-survivor-selection-20261023/final-report.json`
- `analysis/eos-survivor-selection-rust-20261023/final-report.json`
- `analysis/eos-survivor-pacing-20261023/reproduction.json`
- `analysis/eos-bridge-clippy-20261023/report.json`
- `analysis/eos-rust-native-fanout-20261023/cohort.json`
- `analysis/eos-host-activation-clock-20261023/final-report.json`
- `analysis/eos-pending-cue-selection-20261023/raw-data.json`
- `analysis/eos-device-namespace-20261023/final-report.json`
- `analysis/eos-host-agreement-rust-20261023/core-port-report.json`
- `analysis/eos-host-agreement-rust-20261023/bridge/final-report.json`
- `analysis/eos-rust-agreement-20261023/final-report.json`
- `analysis/eos-rust-agreement-20261023/cleanup.json`
- `analysis/single-play-installed-20261022/report.json`
- `analysis/eos-second-account-20261022/report.json`
- `analysis/eos-second-account-20261022/profile-readback.json`
- `analysis/eos-two-account-join-20261022/before.json`
- `analysis/eos-two-account-join-20261022/social-and-sdk-lifetime.json`
- `analysis/eos-social-ui-20261022/scoped-master-focused.json`
- `analysis/eos-social-ui-20261022/refresh-capacity-rollout.json`
- `analysis/eos-join-fix-runtime-final/report.json`
- `analysis/eos-join-fix-runtime-final/cleanup.json`
- `analysis/eos-accepted-result-pipeline-20261023/live-sdk.json`
- `analysis/eos-accepted-result-pipeline-20261023/production-source-comparison.json`
- `analysis/eos-all-survivor-rust-20261023/final-report.json`
- `analysis/eos-survivor-session-20261023/cohort.json`
- `analysis/eos-survivor-session-20261023/final-report.json`
- `analysis/eos-survivor-native-20261023/command.json`
- `analysis/eos-scope-history-20261023/capsule-overlap-generation.json`
- `analysis/eos-scope-history-20261023/final-report.json`

Private test-client preferences, device caches and diagnostic logs belong to
the active `tools/run_test.py` owner and are removed when that client closes.
Retain only bounded redacted findings and source hashes afterward. Preserve the
installed runtime, original assets and unrelated active work.
