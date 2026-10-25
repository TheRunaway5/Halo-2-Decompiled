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

- [x] Install the prior qualified client and single Play → intro → title → menu route.
- [x] Bind two distinct real Steam accounts to EOS profile publication from the
  prior installed clients.
- [x] Promote the account-bound v14 replacement and pass post-installation
  authentication checks and current-tool runtime verification.
- [x] Read back both profile revisions through the trusted publisher and public
  website API: 27 fields for the legacy account, 29 for the new account.
- [x] Preserve the legacy unknown join date and publish the new registration's
  actual timestamp. Live aggregate counts show two users and zero guests.
- [x] Restore both cached local profiles and enter the same actual two-player
  Lockout Slayer world through production EOS on one PC.
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

The latest qualified **uninstalled candidate** is
`bin/engine-builds/8ef531cf15624b8daead5ed088dab67e/halo2_native` (SHA-256
`92f8a321cf28eb3c24b629ec25a0f42212d26bbb72da948ffcf13e0c2d4a1b40`),
retained with its resources at 316,334,099 bytes. All eight native tests passed
(native report SHA-256 `fed94d36992484efeeaa4879cb6fea304dc85497b42a9aceb1b9ae47023bce6b`),
as did eight launcher checks, twelve map/player frame cases, four menus and
eighteen custom controller assets. The candidate report is
`Halo-2-Decompiled/analysis/eos-sdk-main-thread-candidate-20261024/report.json`
(SHA-256 `fd4abfe7bd169a584737cd7ee773b0e7a47271fc84ddbeb4d3a2093baae886c1`).
Prepare-only packaging published no ROOT launcher or selector.

The latest cached-profile run queued at **12:19:50 UTC on October 24**, using
the same uninstalled 8ef/92f8 candidate and a private worker-stderr observer.
EmkMage and Halo0001 entered Lockout Slayer at 6:23/6:21, then 5:16/5:14,
with scores 0/0. One-second mouse holds visibly reduced both HUD magazine
icons; the exact shot count is unverified. Later F3 captures were already
terminal menus, so this run provides no scoreboard or PCR claim.

This run also failed, around SDK time 234 seconds. Near 225 seconds, slot 0
(the native host and SDK session owner) stopped advancing its 92,611 data sends;
the SDK queue stayed at 65,479 bytes while reverse reception continued.
Backpressure reached 4,766. The client last accepted sequence 5,572/tick 5,570
and timed out at age 10,004 ms with zero rejected packets and 20,608 buffered
bytes. The host reached tick 5,883 with 6,008 accepted inputs, removal reason 0
and last-input age 397,660,793 ns. Actual failures now include both native-heavy
role alignments; that observation does not identify a role-only cause.

The SDK-free observer method initially failed loading; the diagnosed undefined
`rust_eh_personality` was resolved by a private LOCAL/HIDDEN abort-only symbol.
Version 4 passed all nine controls (report SHA-256
`98990d82b4f5f26f96fb96b136e1ae160826c8e98fa722435a25ed86c4d29a3e`),
then repeated all nine successfully inside the live owner before either client.
Its bounded captures were 14,808/13,854 bytes, with zero omitted windows or
collector errors; producer losses remain unmeasured. The scrubbed 248,298-byte
diagnostic contains connection/session/closing messages without a proven SDK
cause (SHA-256
`8c69c100805d303dc722d5434485319673b4033e994400decfb801a048ffbf10`).

Cleanup receipt `sdk-stderr-two-profile-cleanup-20261024.json` (SHA-256
`5f87d271dc5bb33b5f58c332618e2944a6a94bd6c2b89bd94ab8c7fcebc5fed8`)
verifies both parents exited zero and are absent, owner `f_jwixxw` was removed,
both SDK readers reached EOF and joined, and all 41 source pins were unchanged.
Private logs, captures, observer binaries and caches were removed; UI/retainer
helpers were archived as nonexecutable 0444 sources. The frozen closed UI proof
`ae7f2edd…` and final finished UI proof `12ccde60…` remain distinct.
The full-game-ELF worker versus standalone component context is under
source-only audit; it is not an established cause or fix. Completed-match,
result/publication, WAN, microphone and full-migration acceptance remain false.

An earlier actual cached EmkMage/Halo0001 run queued at **09:54:36 UTC on October 24**.
Both entered Lockout Slayer at 7:06/7:04, score 0/0; real F3 scoreboards later
showed both names tied at zero. The run still failed after about 206 seconds of
SDK traffic: the client timed out after 10,021 ms without an accepted snapshot,
and the host SDK outgoing queue remained at 65,530 bytes/107 packets while
reverse reception continued. Process-main ownership did not resolve this live
stall; longer duration alone does not prove improvement. Vendor SDK messages
were unobserved because child stderr was null, not because SDK errors were
proved absent. No PCR, completed match, accepted result or publication/readback
was obtained.

The bounded diagnostic is
`two-profile-sdk-main-thread-diagnostics-20261024.json` (223,984 bytes; SHA-256
`c0d9cbd3c4ecdcc12a53db5be9126d60bb864a2602dd29df1f5d87f731d633e3`).
The finalized UI report has SHA-256
`e3866c8f90898c71daa7849c454edfe0430f88973d54385fabfc5d0eff751d83`.
Cleanup receipt `sdk-main-thread-candidate-live-cleanup-20261024.json` (SHA-256
`29dc9438e6cd40ba901d5fa961daacb852d26174c04095602111ad2a535b6ab6`)
verifies both clients exited zero and are absent, build/UI owners were removed,
and raw logs, captures and build products were removed. Three one-off helpers
were retired as nonexecutable 0444 sources; the qualified reference candidate is
intentionally retained. Foreign ROOT selections were preserved.

Two private 240-second Rust native-size component controls have completed. The
debug/traced run failed its progress criterion: 160,800 offered forward datagrams
accounted exactly for 114 refusals, 26,852 Rust expiries, 472 stop cancellations
and 133,362 received datagrams; only 737/7,200 complete groups arrived. It did
not reproduce a stationary full SDK queue. Report SHA-256:
`204aba9d79242b573c1ac0642189c4f220076768275655a6514c633b4e6726a1`.

The Release/untraced control received all 160,800 forward datagrams/7,200 groups
and all 9,600 reverse inputs with zero expiry, refusal or SDK backpressure.
Host mean loop/pump times were 2.199/0.142 ms, but observed tick-gap maxima of
554/575 ms prevent a tight-bound claim from the means. It also reproduced no
stationary full SDK stall. Report SHA-256:
`486f12a865e38959ca297b6575da6eeccf4ef72bbf3a94986c8a3139e67f96ce`.
This combined execution-context comparison does not attribute the difference to
debug optimization or tracing separately and does not fix or accept the failed
8ef/92f8 native match. Both controls preserved all 41 production source pins,
completed SDK/remote shutdown and removed owned outputs. Cleanup receipts
`eos-native-size-main-thread-probe-cleanup-20261024.json` (SHA-256
`45cde9ac9ad259117a8e9e792145fc167b55b50446321573f323368daf9b26c1`)
and `eos-native-size-context-control-cleanup-20261024.json` (SHA-256
`639d4e137d53f28fb51418c1a23c787c41006da1d554a77beb536e195c080679`)
verify owner removal and no retained raw logs, SDK/build binaries or caches.

The first authenticated-profile prelude attempt (`777b3dd9…`) stopped at
library compilation: its 53-field JSON macro exceeded the recursion limit.
Report SHA-256 `0ab2429049086bb28d1a6b1d2c9e9dd1fcf082c154b0b270721f42aaefffd114`
remains failed; no SDK worker, credential-config read or network started. The
private reporting repair split the unchanged 38 baseline and 15 profile fields
without changing production code, policy or lifecycle.

The corrected prelude passed its 240-second component criterion. The production
identity API logged in, copied the proof and released its profile platform.
Official proof verification then preceded a fresh same-subject match grant,
retaining the same PUID and live SDK child.
The acknowledged parent proxy-handle ledger went 0→1→0 with no pending
retirements; these are not independent inside-SDK counts. No stationary SDK
stall appeared, but forward delivery was 160,609/160,800 datagrams and
7,189/7,200 complete groups: 153 refusals and 38 Rust expiries remain explicit.
All 9,600 reverse inputs arrived; the maximum complete-group gap was 946 ms.
This is neither zero-loss nor native-match acceptance. Report SHA-256:
`c515bece835ae97afd5142ec6873ab629789a0aac21584ad3374012f7c840639`.
Cleanup `eos-native-size-profile-prelude-cleanup-20261024.json` (SHA-256
`8a2135efc4d69d73c925ec07ba335f2485d6be37d5f6f9cbdc2de82a01a8ab4a`)
verifies the `oq031e8s` owner and private outputs removed; helper `3f58a7bf…` is
retained only as archived source. No idle duration was assumed.

The 240-second direction control also passed its component criterion. With
heavy traffic on slot 1 and light traffic on slot 0, while SDK session owner/P2P
initiator stayed slot 0, all 160,800 heavy datagrams/7,200 groups and all 9,600
reverse inputs arrived. Refusals, expiries and SDK backpressure were zero.
Maximum complete-group gaps were 89 ms at the heavy receiver and 102 ms at the
reverse receiver; slot 0/1 mean loops were 2.179/2.213 ms and SDK outgoing
high-water marks were 384/893 bytes. Both real profile/proof preludes retained
the same PUID/child and acknowledged handle lifecycle. No stationary full SDK
stall appeared, so the conditional 1,404-byte control did not run. Report
SHA-256 `c4eb3ab088dfa2e867f7cb16b79916ca3993c6bb09d3f4a73700bca75d0e411f`;
cleanup SHA-256
`b8848c63f3635be132e21c24e46d4f019a1bd28adf020fa56b17853b131c024d`
confirms owner `itlrso2u` removed, remote cache cleaned and helper `2871af26…`
retired as 0444 source. This synthetic role-alignment result does not establish
a role-only cause or native gameplay acceptance.

The source-derived QoS control completed its 240-second component criterion.
The unchanged native QoS owner ran beside heavy slot 1/light slot 0 traffic:
all 160,800 heavy datagrams/7,200 groups and 9,600 reverse inputs arrived, with
zero refusals, Rust expiries or SDK backpressure. Each peer admitted five
64-packet upload trains (one setup, four selected) without QoS refusals; both
actual QoS rows were measured with zero reported loss. The declared context
reset preceded load by 2 ms. Receiver maximum complete-group gaps were 79/51 ms,
slot 0/1 mean loops 2.165/2.192 ms, and SDK high-water marks 3,520/1,082 bytes.
No stationary full SDK stall appeared; the conditional 1,404-byte control did
not run. This source-QoS workload is component evidence, not native acceptance.
Report SHA-256:
`032277ef40de51b0f6202df4262f5ea39acc4ecc05f146f38dbfbb696f1c414b`.

Both real profile/proof preludes, same-PUID/live-child acknowledged ledgers and
all 41 production source pins remained intact. Cleanup receipt
`eos-native-size-qos-control-cleanup-20261024.json` (SHA-256
`6b26e6b3d3b5e4482c6c1cca25384c6f3483c54ee9d7ab1b4d78d904c0fb2b46`)
confirms owner `kpd1udni` and private outputs removed, remote cache cleaned, and
helper `10210e92…` retained only as a nonexecutable 0444 source.

The actual 8ef/92f8 native match remains failed. Completed-match, PCR,
ranked-result, publication, WAN, microphone, full-migration and ROOT-delivery
acceptance remain false. Historical V6–V10 evidence and the immutable deployed
backend candidate remain unchanged.

The earlier qualified **uninstalled candidate** is
`bin/engine-builds/cfdc426a2ef441c1b252ee4c35185290/halo2_native` (SHA-256
`8e5076550e4046cf4d47486fb4984bc1c69df5dd18379fa1cecf4b62f5bee044`),
retained with its resources at 315,360,388 bytes. Its five native checks, eight
launcher checks, twelve map/player frame cases, four menus and eighteen custom
controller assets passed. The first scoped native run remains failed at 3/5:
receiver and cold-receiver assertions incorrectly assumed a pristine tick zero.
The fixture correction records the actual owner/tick/full body hash and still
requires identity rejection without mutation or acknowledgment; the fresh run
then passed 5/5. Offline Rust qualification passed 103 protocol and 38 bridge
tests, with two existing ignored tests and both strict Clippy checks. The
packaging policy passed 29 unit/fault controls; the narrowed actual tool guard
passed six controls. These are component checks, not live-match acceptance.

The actual cached EmkMage (slot 0) and Halo0001 (slot 1) run queued Casual Lab
Slayer at **06:50:43 UTC on October 24**. Both joined visible Lockout Slayer
worlds at 7:57/7:54, score 0/0, then slot 0 returned to party and slot 1 showed
“Match connection failed.” Diagnostics recorded the SDK outgoing queue
stationary at 65,535 bytes/102 packets for about twelve seconds while pumping
continued; the complete cause remains unresolved. No PCR, shooting, accepted
result or result readback was verified. Both clients exited zero; the UI/build
owners, captures, private logs and GPU caches were removed. The candidate was
never globally selected. The unrelated shared selection, observed as `1775b14a…`
at review, was preserved. Acceptance and cleanup are recorded in
`analysis/eos-live-matchmaking-failure-20261023/two-profile-candidate-v2-acceptance-cleanup-20261024.json`
(SHA-256 `cf9c83f6c083772149a49b489155e2a3619e7b0a691d94730f72d600d4038e51`).
Full migration, WAN, microphone, ranked-result and completed-result acceptance
remain false.

A second run of the same cached profiles and unchanged cfdc candidate queued at
**07:53:07 UTC on October 24**. Both entered Lockout Slayer at 7:12/7:10,
score 0/0, before returning to party/player-left screens. Fresh poll samples
showed the host SDK queue stationary at 65,488 bytes/104 packets while reverse
reception continued. This does not establish the cause or prove a thread-related
stall. The numeric diagnostics and final UI reports are
`Halo-2-Decompiled/analysis/eos-live-matchmaking-failure-20261023/two-profile-sdk-observation-diagnostics-20261024.json`
(SHA-256 `4023f73c4b799e40d7d0b6db5c0a03a18b4d4d648fc99200bf57497c4d203114`)
and `Halo-2-Decompiled/analysis/eos-native-preparation-candidate-20261024-v2/two-profile-sdk-observation.json`
(SHA-256 `8c102b99e21059f995c90f95c1ced72c0c08c71dd7b6dc123b9ff4c7c2356b04`).
Both clients exited zero; their owner, raw captures, private logs and GPU caches
were removed (`sdk-observation-completed-helper-cleanup-20261024.json`, SHA-256
`d4e4220bd4c71458ee836f5237f7ebe874641e7ac670796a3134c56a984c515a`).
No completed match, PCR or result readback was obtained.

The separate serialized-caller SDK A/B comparison passed both ninety-second
cases: each received all 45,000 host and 2,700 input datagrams without SDK
backpressure or Rust expiry. The observed one-versus-three caller-role shapes
reproduced no stall. This generated workload does not prove thread causality,
process-main compliance or native gameplay acceptance; its report is
`eos-sdk-thread-ownership-ab-20261024.json` (SHA-256
`cd69b79031de084d3f0293b2987796727dfa038a8c770de85ce8e5779c4b22f2`).

Rust now routes the SDK through an explicitly registered self-executed worker
whose true process main owns SDK calls and lifetimes. The bounded IPC startup
repair consumes at most 4,096 preamble bytes and one versioned 22-byte marker;
subsequent frames remain strict. Fresh offline and SDK builds each passed 156
unique Rust tests (103 protocol, 53 bridge), with two existing ignored tests and
strict Clippy for both crates. Four private subprocess controls passed. Actual
SDK observation recorded six creates, six releases, at most two live platforms,
one shutdown, all on child process main, followed by child reap. This qualifies
the platform lifecycle, not authentication or gameplay. Evidence:
`sdk-main-thread-validation-20261024.json` (SHA-256
`62cac10d11f4e3200cc77d926451fb184d6e5baa5d42d79bc2e019a47a402d5c`);
its owned source, binaries and SDK scratch were removed.

The separate ninety-second generated SDK workload received all 45,000 host
and 2,700 input datagrams on the direct route. Forced relay did not receive 137
host and two input datagrams. Both component runs avoided a ten-second complete
group stall; neither is native snapshot, completed-match or zero-loss relay
acceptance. See `eos-sustained-route-conclusions-cleanup-20261024.json` (SHA-256
`ce99e5429b20febf6fc9ee371807e8f2a3142b4fe034f22cb1af3418554160ce`).

The earlier installed V10 diagnostic was tested twice with the actual cached EmkMage and
Halo0001 profiles. Both attempts matched and loaded, but neither reached postgame
or a carnage report. In the second attempt, queued at 03:42:02 UTC on October 24,
both actual Lockout Slayer worlds were visible at 7:51 with score 0. By the next
capture one client was back at the party and the other displayed “Match
connection failed. Search again.” No drawn scoreboard was observed. The host
role swapped between attempts. Each client timed out after about ten seconds
without a fully accepted native snapshot; each host recorded an authenticated
Leave. Host outgoing progress stalled with queue-full observations and increasing
expiry, but the complete cause remains unresolved. No public player/report or
EOS result-readback queries were made. Both client processes and both live
owners closed cleanly; raw captures, private logs and GPU caches were removed.
See `analysis/eos-live-matchmaking-failure-20261023/two-profile-acceptance-v10.json`
and `v10-live-owner-cleanup.json` in the same directory.

The earlier actual v6 test on October 23 restored both cached profiles and queued Casual
at 23:34:48 UTC. Both native clients entered the same two-player Lockout Slayer
world. The host rendered 1,822 frames over 2,229 ticks; the client rendered 857
frames over 698 ticks. Each recorded one connected remote peer and a peak of two
players. By 23:38:13 UTC, the host had returned to the online party while the
client remained on an unchanged 7:36 game frame. Both processes subsequently
closed cleanly with exit code zero. This proves the initial account-bound join
reached the native world in this same-PC test; completion failed.

No genuine postgame, accepted report or EOS result readback was observed. No
public report/player queries or publication-readback helper were executed. The
initial disconnect cause remains unidentified. A separate source audit found a
client recovery wait for a migration decision unavailable in ordinary
matchmaking; it does not identify the initial disconnect cause.
The completed failed test is recorded in
`analysis/eos-live-matchmaking-failure-20261023/two-profile-acceptance-v6.json`.

V7 was intentionally canceled with exit code 143 before native authentication
qualification or publication. Its offline Rust tests and strict Clippy passed,
but it was not installed and its owned build was removed; v6 remained selected
at that point.
The cancellation followed an actual SDK burst regression that reproduced
queue-full send failures with successful Session callbacks. That component
finding does not establish the cause of the v6 live disconnect. The cancellation
receipt is
`analysis/eos-live-matchmaking-failure-20261023/v7-cancelled-before-publication.json`.
V8 passed 138 offline Rust tests and both strict Clippy checks, with two
service-dependent tests ignored. Its native qualification stopped at 9/10:
the positive common-cut subprocess completed with exit code zero and all
required markers, but the Python postflight inspected the live room dictionary
after intentional host departure allowed normal cleanup to prune it. The
ordinary disconnect case passed in 119.03 seconds. The failed delivery never
reached publication or installation, and its owned tree was removed. Its report
is `analysis/eos-native-session-delivery-20261023-v8/report.json` (SHA-256
`3e408ca9f2cb9a0ba2795cf9dd764d50867d82c9bddc35fa74bbabfff3fbfdd4`).
The fixture-only V9 correction retains and validates the original actual room
objects across that deliberate host loss. V9 then failed both MCC native checks
on a native memory conflict. V10 applies a reviewed three-file fade-data
correction, pinned by
`analysis/eos-live-matchmaking-failure-20261023/v10-refresh-manifest.json`
(SHA-256 `696cb1d9bdfb8ec6019ddbdf12f98866a71677ceb86245cc96846f15b5057573`).
All 138 Rust tests and both strict Clippy checks passed, with two tests ignored.
The first native run passed 10/11: the positive common-cut case reported a
serviced native peer expiry after 180.61 seconds; its actual cause is unproven.
An unchanged full eleven-case repeat passed, including positive common-cut in
205.01 seconds and ordinary disconnect in 87.28 seconds. The original failure
and intermittent status remain recorded; the repeat is not a readiness claim.
V10 then passed eight launcher/shadow checks, twelve MCC submitted-frame cases,
four rendered menus and all eighteen project controller assets. Its installed
diagnostic runtime is `bin/engine-builds/b77edf39fc734632b695adcc4d11e182/halo2_native`
(SHA-256 `ae29ac68910496f7962ff6c909630e18ebc9c0f7ef1216e10d8880559befea22`).
The post-native run passed the other ten cases but failed positive common-cut
again after 242.02 seconds. The full delivery report remains false. A separate
read-only diagnostic admission allowed the bounded live test above; it does not
qualify full delivery or migration. Unrelated work subsequently selected
`bin/engine-builds/8681e66c5d024bf98a96539bb2f38890/halo2_native`; that shared
selection and its launchers were preserved. The two tested processes continued
to run the exact b77 diagnostic binary.

The separate four-fragment pacing probe failed its strict delivery check:
one direction received 948/948 datagrams and the other 940/948. It is not accepted
as zero-loss or native-match proof. No new live-match, result-publication, WAN or
microphone acceptance is claimed.

The October 23 retry in the previous installed build failed about two seconds after
finding the other player. The displayed text was “Matchmaking service unavailable. Retrying...”;
the production trace shows successful session, playlist, ticket, transport and
signaling responses followed by cancellation/requeue. That build discards its
retained transport failure reason at the boundary. The confirmed defect is the
anonymous DeviceID transport path: ordinary same-PC clients inherit one Linux
SDK keychain namespace, so distinct Steam/profile accounts can receive the same
anonymous transport PUID. The minimized actual-SDK regression reproduces that
identity reuse and rejection by the installed Rust peer-identity guard; a
distinct namespace passes the same guard. This diagnosis does not establish a
successful player match after the fix.

The earlier October 22 attempt reached “Attempting to join a game,” remained in
joining for approximately 40 seconds and retried. Both WebSocket signaling
connections received HTTP 101; neither account reached playing. That earlier
public symptom was captured by:

```sh
python3 -B tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/check_online_population_phase.py \
  --phase playing --users 2 --timeout 65 \
  --report analysis/eos-two-account-join-20261022/before.json
```

This command failed against the live repeated attempt. It checks aggregate
activity only; native session diagnostics and actual gameplay must establish
subsequent acceptance. No test scores or fabricated reports are
inserted into the production account database.

The earlier minimized official-SDK regression also reproduced `EOS_AlreadyConfigured` after
the profile platform releases the last SDK owner and matchmaking attempts to
initialize again. Platform release now retains the process SDK; explicit main
shutdown occurs after session owners are destroyed and before static teardown.
The real SDK profile/recreate/concurrent-platform/shutdown regression passes.
A completed two-client match remains required after the v6 join-then-disconnect test.

The real host OS credential-store regression also passes profile/origin
isolation and deletion. Refresh credentials remain in that secure store;
non-secret stable profile slots survive profile renames. The master issues
eight-hour sessions from fixed thirty-day refresh credentials and revokes the
refresh credential and its issued sessions on explicit Sign Out.

Steamworks integration is deferred at the user's direction. This work retains
the existing Steam OpenID sign-in and server Steam friend-list/privacy behavior;
Steamworks is not a prerequisite for the account-bound room transport fix.

The scoped production master rollout for remembered sign-in and Steam/clan
invitations, with independent bounded refresh storage and indexed expiry,
passed nineteen applicable HTTP regressions, actual-image checks, health and
source-hash checks. The separate in-progress party roster appearance extension
is excluded from that rollout. No Steam passwords or private account identifiers
appear in the evidence receipts.

The historical delivered gameplay runtime was
`bin/engine-builds/2571dcba509a4e73a68866885022f7de/halo2_native`.
Before v6, the selection records kept that gameplay owner separate from the
frontend runtime `bin/engine-builds/dire-valley-menu-20261023/halo2_native`,
using the then-selected Dire Valley menu cache. The earlier delivery passed four
focused identity/party tests, eight launcher checks, twelve MCC submitted-frame
cases and four frontend screens. All eighteen Xbox/PlayStation prompt runtime assets match the
project's authenticated banks. Those historical source and executable hashes are
recorded in `analysis/eos-join-fix-runtime-final`; they do not qualify the newer
frontend or current Rust HTTP source. The actual gameplay and frontend selection
manifests own their respective paths and hashes. Unrelated renderer edits remain
preserved in the checkout, and the selected runtimes are intentionally retained.
The build scratch and raw captures were
removed. At that delivery, the user deferred the next real two-account join
attempt until the following day; the subsequent October 23 failure is diagnosed
above. This build does not establish live matchmaking or completed-match acceptance.

The historical anonymous DeviceID test used the second-client helper's separate
XDG/EOS data namespace. The official Linux SDK 1.19.2.1 verified that setup through the
actual Rust transport: two isolated child processes receive distinct anonymous
transport identities, and a fresh process reusing the first namespace receives
the same identity. Credentials stay under each owned XDG data keychain; no
write escapes that namespace. The retained receipt contains hashes only and
its SDK/build/cache scratch was removed. This proves the helper's Linux device
namespace, not gameplay or other operating systems. Ordinary same-namespace
processes share a Device ID; distinct Steam/profile identities alone did not
isolate that separate anonymous transport identity. This is historical DeviceID
diagnosis, not a namespace requirement for ordinary account-bound v14 clients.

The replacement account transport lifecycle belongs to Rust. It requests a
fresh room-authorized issuer JWT, uses external OpenID Connect login, and permits
CreateUser only for initial InvalidUser admission. It copies the official EOS
ID proof and requires the server's exact canonical PUID acknowledgment before
opening the first socket, emitting local signals or creating a Session. The
identity remains immutable. Refresh requests fresh credentials/proof, never
falls back to DeviceID, and preserves an already admitted network during
transient issuer outages or verification-capacity responses. Request tickets reject
stale completions. Authentication, refresh and retry policy remain in Rust.

The room token/proof HTTP implementation now also belongs to Rust, using the
safe curl binding with the existing system libcurl. It owns fixed endpoint and
header admission, TLS verification, redirect/proxy refusal, bounded replies,
typed object-only JSON parsing, request deadlines and cooperative cancellation.
The existing native worker provides thread lifetime and owned-message conversion,
and invokes Rust outside SDK and Session locks. Actual loopback tests cover the
proof POST and acknowledgment, redirect/header isolation, malformed and oversized
responses, and cancellation of a pending body read. A separate regression rejects
root arrays that Serde's derived struct decoder otherwise accepts. Older account,
Steam/Discord and matchmaking HTTP paths still include C++ I/O; this change does
not claim that every sensitive network request has been ported.
The remaining authenticated native HTTP/WebSocket sites and proposed next seams
are recorded in
`analysis/eos-live-matchmaking-failure-20261023/rust-http-io/sensitive-network-rust-plan-v2.json`.

Actual SDK qualification now runs three independent A/B/A account processes in
the same owned Linux OS/XDG namespace: A and B remain distinct, and repeated A
retains the same identity. Six official proof acknowledgments pass, including
fresh-JWT SDK login/proof repeats from an explicit refresh stimulus. No DeviceID
event is permitted. This exercises actual SDK refresh operations, not natural
expiration-notification delivery. A separate two-process account-bound fixture
creates/registers/invites/joins an actual EOS Session and receives 202 intact
1,404-byte packets across both directions through forced Epic relay. Both SDKs
shut down cleanly. These controls use generated qualification subjects and a
memory-only peer router; they do not create production player/room/stat records
or establish native gameplay, two consumer networks or microphone acceptance.
They use the production issuer/official verifier directly; the new room HTTP
path has separate fixture and public guard checks.

The exact production party tuple exposed a separate Session update failure when
its original bucket representation was used. Rust now derives a 38-byte bucket
from a domain-separated, length-framed digest of playlist and revision. Capacity
and the explicit session attributes are preserved. The actual production-derived
party tuple at capacity four with two physical peers received 202 intact
1,404-byte packets through forced relay; Casual at capacity two received 203.
Account proofs and Session callbacks passed, and both SDKs shut down cleanly.
The Session admission clock starts after the first account acknowledgment and
local SDK identity, preserving its full budget during a slow initial proof.
These are SDK component checks, not a four-player or native played match; no
undocumented bucket-length limit is inferred. Source pins and scope are in
`analysis/eos-live-matchmaking-failure-20261023/party-bucket-fix-validation.json`.

The scoped v14 backend is now deployed as
`halo2-master-eos:20261023T173530Z-account-v14-candidate`
(`sha256:98f5fe3e39664418ab365b14cef454cfd579b0c59a226ff2c038b58065a35fa4`).
Only three modules differ from the authenticated deployed baseline; the other
33 source files and existing social/departure behavior remain unchanged. The
actual image passes 44 applicable focused and 35 social checks, including all
17 new authentication/queue cases. Two unrelated canonical/deployed-baseline
test mismatches were reproduced unchanged and are recorded separately. The
directory-only rollout preserves private keys, makes a consistent 542,527-byte
SQLite/private-file backup, and verifies all 36 source hashes, configuration,
internal/public health and unchanged other containers. Public HTTPS health returns
JSON 200; unknown-room token/proof POSTs reach the backend's JSON 404 guards. These checks
do not sign in or create player sessions. The portable release receipt is
`Halo-2-PC-Website/backend/eos-worker/account-transport-v14-rollout.json`.

The earlier native cohort completed all 23 selected checks, but its subsequent
promotion was interrupted when the original storage owner exited. Its unselected copied candidate is not
delivery evidence. That cohort is historical and predates the Rust HTTP I/O port;
its receipts remain under
`analysis/eos-live-matchmaking-failure-20261023/native-qualification`.

The v5 delivery passed twelve MCC frame cases and four menu screens, then stopped
before publication at the strict frontend resource-identity check. Its unselected
candidate and owned build were removed; that failed attempt is not an installation.
The diagnostic now records the changed path and before/after identities, and
allows at most three complete readmissions only for an original pinned cache
resource. Every attempt retains the original runtime, resource, sidecar and frame
report hashes. Runtime, shader, content and publication failures are not retried.

The historical V6 delivery installed
`bin/engine-builds/a1959ce2f4ae48e099a03f19ff2428ac/halo2_native`, SHA-256
`4ba2c2a58e3cc6882cad8a5859c834fa074b785555913768dd024c9aa18275e3`.
At that delivery, both gameplay and frontend selections admitted that runtime with
`assets/imported/mcc-dire-valley-polish-20261023`. Its frozen source passed 97
Rust protocol and 33 bridge tests, strict all-target Clippy and the generated CXX
boundary; two existing SDK/credential-service tests remain ignored in that
offline run. Six focused native checks passed before and after promotion: five
authentication/transport cases and the frontend-owner readmission regression.
Eight packaging/launcher checks, twelve MCC submitted-frame cases and four
rendered menu screens passed. All eighteen project Xbox/PlayStation prompt
payloads matched, and the then-current root tools verified the installed
114-file inventory.
The delivery report is
`analysis/eos-rust-http-delivery-20261023-v6/report.json` (SHA-256
`52b12b2873bea63f084ea1d22f66b62a26ad96d7445084e59aa72b0f3845269d`);
its current-tool verification receipt is
`analysis/eos-rust-http-delivery-20261023-v6/current-tool-delivery-verification.json`
(SHA-256 `aed881859505c99a9cb12341e135712cd464b0df17f8e2500c77d21d063a3fc8`).
The subsequent live v6 test entered the native match, then failed before
completion as described above. These checks do not close result publication,
WAN, microphone, full weapon-audio or complete host-migration acceptance;
the broader optional weapon-audio run remains two of four cases.

Separately, all 28 local population/profile/carnage/Steam HTTP contracts and seven
project prompt/artwork tests pass. These use the authenticated frozen code plus
one explicitly hashed current public security adapter and the genuine Rust
security library, all in disposable storage with no external network access.
They are current local contract checks, not the deployed image's 79 checks or a
played-match result. At that audit snapshot, thirteen canonical master modules
differed from the historical 34-file result-pipeline baseline; that newer security projection was
not deployed by the scoped 36-file v14 rollout. The source boundaries, exact
hashes and cleanup are in
`analysis/eos-live-matchmaking-failure-20261023/feature-completion-audit.json`.

## Accepted-result publication qualification

The earlier isolated HTTP/ledger → TypeScript publisher → actual EOS SDK
qualification passed against sources matching the then-deployed 34 master files
and both website publisher modules. A casual result with six guest seats followed by a
ranked result produces the exact cumulative matches/wins/kills/deaths/assists,
shots and hits. The accepted original Halo level 2 is read back from the actual
EOS ranked leaderboard. Duplicate commits, simulated lost SDK publication replies
and publisher restart preserve the verified values and outbox revision.

These are explicitly declared PCR fixtures with local mock authentication and
one isolated generated device-test PUID. Production player rows are unchanged.
The publisher and fixture source hashes still match that historical receipt;
current canonical master changes have not received a fresh live SDK result run.
The earlier qualification closes that publication component gap; the played native match,
real-account result acceptance and all other unchecked goal items remain open.
The source/SDK/Node pins, command results and scope are recorded under
`analysis/eos-accepted-result-pipeline-20261023`; both owned SDK/build trees and
all raw fixture databases were removed. No large artifacts were retained.

## Migration boundary

Current source has native retention, prospective host and surviving-client
alternatives, genuine held publication bindings, and a Rust preparation scheduler.
These are implemented and component-qualified; inventory publication is not full
Ready. Ordinary matchmaking still leaves `peer_host_migration`,
`survivor_cut_selection` and `snapshot_candidates` disabled.
`prepare_peer_migration` has no production gameplay caller, and
`prepare_foreground_handoff` remains a decision-keyed partial preparation.
The legacy `prepare_takeover` requires a learned decision; the separate prospective
APIs can prepare the real native alternatives before one exists. The complete
foreground/device aggregate and its production caller remain unfinished. EOS
Sessions or an election alone do not close those owner boundaries.
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

The current owner map records six remaining integration gaps: production
admission freeze; a complete predecision aggregate; physical input/camera and
persistent local state; GPU HUD/image publication; real playing-audio ownership;
and final publication/retirement callbacks. It preserves the already resolved
native role alternatives, genuine held bindings and Rust scheduling policy.
See `analysis/eos-live-matchmaking-failure-20261023/migration-next-owners.json`.
No component receipt certifies full Ready or enables migration by default.

## Evidence

- `analysis/eos-live-matchmaking-failure-20261023/shared-identity-probe.json`
- `analysis/eos-live-matchmaking-failure-20261023/account-auth-rust-final.json`
- `analysis/eos-live-matchmaking-failure-20261023/account-auth-sdk-probe.json`
- `analysis/eos-live-matchmaking-failure-20261023/account-session-pair-probe.json`
- `analysis/eos-live-matchmaking-failure-20261023/backend-deployed-candidate.json`
- `analysis/eos-live-matchmaking-failure-20261023/backend-rollout-owner-fixed.json`
- `analysis/eos-live-matchmaking-failure-20261023/backend-v14-public-proxy-smoke.json`
- `analysis/eos-live-matchmaking-failure-20261023/native-qualification/cohort.json`
- `analysis/eos-rust-http-delivery-20261023-v5/report.json` (failed before publication; candidate removed)
- `analysis/eos-rust-http-delivery-20261023-v6/report.json` (installed; component and packaging scope)
- `analysis/eos-rust-http-delivery-20261023-v6/current-tool-delivery-verification.json`
- `analysis/eos-live-matchmaking-failure-20261023/two-profile-acceptance-v6.json` (native join observed; completion failed)
- `analysis/eos-live-matchmaking-failure-20261023/two-profile-acceptance-v10.json` (two actual diagnostic attempts failed before postgame)
- `analysis/eos-live-matchmaking-failure-20261023/v10-live-owner-cleanup.json`
- `analysis/eos-live-matchmaking-failure-20261023/party-bucket-fix-validation.json`
- `analysis/eos-live-matchmaking-failure-20261023/rust-http-io/serde-object-shape-fix.json`
- `analysis/eos-live-matchmaking-failure-20261023/rust-http-io/sensitive-network-rust-plan-v2.json`
- `analysis/eos-live-matchmaking-failure-20261023/feature-contract-tests.json`
- `analysis/eos-live-matchmaking-failure-20261023/feature-completion-audit.json`
- `analysis/eos-live-matchmaking-failure-20261023/migration-next-owners.json`
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

## 2026-10-24 worker isolation and client response follow-up

The exact native SDK-worker image reproduces the stationary full outgoing SDK
queue in the otherwise passing Rust/QoS fixture. Both1323-byte and1404-byte
cases fail while reverse datagrams continue. Evidence:
`analysis/eos-live-matchmaking-failure-20261023/eos-native-worker-image-control-20261024.json`
(SHA256 `62875dafc4fc0b7957f14e60351e9a3ad9940c95a44f52d69c71aec8a38f5056`).
The worker executable, linkage and dependency/code-generation context are the
discriminated factor collectively; no individual constructor or SDK bug is
identified. Its official scratch owner was removed. A dedicated adjacent Rust
worker is implemented and an uninstalled candidate is being validated; this
does not yet establish a fixed native match or authorize runtime publication.

The user additionally reports missing vanilla pre/post-game flow and roughly
half-second non-host input/motion latency. Keep these acceptance gates open:

- [ ] Restore the source-backed pre-game roster, match details and start flow.
- [ ] Restore post-game report/review and lobby return after a completed match.
- [ ] Measure input-to-present latency for host and client mouse/WASD; distinguish
  host RTT/tick delay, transport queue age and replica/presentation work.
- [ ] Make local view and movement respond without waiting for a host roundtrip,
  using original control/physics owners and authoritative reconciliation.
- [ ] Validate remote interpolation, packet freshness and loss recovery without
  replaying stale presentation history or dropping discrete gameplay events.
- [ ] Measure datagrams, SDK fragments and bytes per second per peer before/after
  changes; reduce relay bandwidth with bounded Rust codecs/policy while retaining
  the required gameplay state and authentication boundaries.
- [ ] Repeat actual two-profile gameplay through match completion; separately
  qualify forced-relay/consumer-network behavior when the required paths exist.

Current source routes the non-host through snapshot adoption and sends its
controls to the host without locally stepping movement. Source-relative TU1.5
camera presentation also retains host angles. Those call paths support the
reported class of latency; the reported half-second magnitude and its bandwidth
cost still require an actual measured client run. Do not close these gates from
source inspection, synthetic packets or host-only rendering.
