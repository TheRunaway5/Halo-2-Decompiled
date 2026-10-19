# Peer host/cut agreement

`PeerHostAgreement` is an authored C++20 component for a single admitted physical
membership and host generation. Its packets travel over the existing
authenticated player DTLS links. It agrees both the replacement PC and the exact
retained complete-world/host cut. Native import, generation fencing of gameplay
and voice, and controller/device promotion remain application responsibilities.
`OnlineMatchSession` now invokes it through an internal qualification gate.
Ordinary matchmaking leaves that gate disabled while the rendered native
world/controller/device handoff and compatibility negotiation remain open.

## Inputs and lifetime

- Construct once from the already agreed room digest, host epoch, membership
  revision, physical peer mask and host. Preserve the object throughout that
  live generation. Couch seats have no independent votes.
- Supply trusted monotonic local milliseconds. Only accepted simulation-host
  traffic may update `observe_host`. Strictly more than 500 ms of silence starts
  suspicion and periodic verification probes.
- Before `publish_ready`, freeze and retain the exact natively validated paired
  world/host state. `ValidatedAuthorityCut` is caller-supplied metadata; the
  component cannot inspect or retain native state. Keep this one immutable cut
  for the entire agreement lifetime, including interrupted proposals. A false
  alarm does not authorize dropping a possibly accepted cut.
- Supply each received packet's independently authenticated physical endpoint.
  The packet's claimed sender must match it. Keep authentication and room
  membership outside this component.
- Use measured survivor QoS to choose a ready candidate, then call `propose`.
  The caller schedules stable-leader retries; this component has no automatic
  host-quality or NAT-selection policy.

The nonhost electorate is immutable. A local timeout never removes another
voter from its denominator. Removing peers requires a separately agreed
membership transition that preserves quorum intersection. This permits a sole
survivor in an admitted two-PC room; larger partitions must retain a strict
majority of the original nonhost PCs.

## Agreement and failure model

The two-phase algorithm follows the prepare/promise/highest-accepted-value and
accept rules in Leslie Lamport's primary description,
[Paxos Made Simple (2001)](https://lamport.azurewebsites.net/pubs/paxos-simple.pdf).
This is a crash/partition model with authenticated, honest nonhost participants.
It provides no Byzantine consensus, malicious-nonhost attestation or permanent
cheating-ban evidence. Progress requires eventual delivery and a stable proposer.

Each ballot is `(round, physical proposer)`. An acceptor promises monotonically
and retains its highest accepted ballot/value. A proposer obtains a strict
majority of promises, carries the highest previously accepted host/cut forward,
and requests acceptance of that value. Each accepting PC must itself have
observed host silence, or already be fenced by an earlier acceptance. A newly
preferred candidate cannot overwrite a majority's earlier value when a proposer
disappears before learners have received its votes.

Learners count matching `Accepted` packets directly from distinct authenticated
voters. Repeated packets count once; a forwarded numeric voter mask cannot
produce a decision. The decision advances epoch and membership revision once,
removes the predecessor host, and retains all other admitted physical PCs. The
old host can learn that same decision but cannot vote for its own replacement.
Late predecessor packets never erase an accepted value or learned decision.

Promises and accepted values live in process memory. Reconstruction/rejoin in
the same live generation is forbidden; durable restart recovery and membership
readmission are still required before using this protocol across such restarts.
A chosen candidate failing before native takeover also needs a separately
certified recovery transition; agreement alone does not replicate its owning
world to other PCs.

## Wire and resource bounds

The current native protocol is 89, with packet kind 10, component version 1.
Eight control types cover verification, silence, readiness, prepare, promise,
accept, accepted and preemption. Exact datagram sizes range from 58 to 134 bytes.
Room, epoch, membership revision/mask, predecessor host, authenticated sender,
target, ballot and cut seal are checked before their respective state changes.
Truncation, trailing bytes, unknown types/versions and invalid boolean fields
are rejected. A candidate's declaration can only originate from that candidate.

Public operations use a shared mutex. Sixteen per-voter receipt/promise slots
and 128 replaceable per-target/type pending slots bound storage. Poll budgets
are capped at 128; periodic retransmission is every 100 ms. The default output
budget is four packets. Native executor, graphics, audio and transport owners
remain outside this lock and object.

## Validation, 2026-10-18

`peer_host_agreement` covers the exact 500-ms boundary, valid host observations,
physical/couch separation, healthy-majority veto, two-PC survival, four different
predecessor IDs, exact 50-percent splits, healed partitions, all sixteen physical
peers, an interrupted proposer with a chosen but unlearned value, authenticated
duplicate/impersonation rejection, all eight parsers and every strict prefix,
64 competing-proposer schedules, and concurrent observation/read/poll/receive.
Finite loss is bounded per route/type; explicit filters model lasting partitions.

`peer_host_agreement_direct_udp` creates four real ICE/DTLS/UDP endpoints with no
STUN or relay, then actually destroys the predecessor endpoint. Three survivors
agree on the same declared cut after the silence boundary while exchanging
parallel 200-byte payloads. Its native readiness is fixture metadata. It does
not execute native promotion or establish audible voice, WAN/NAT, headset,
rendered/controller continuity, or a frame-count handoff guarantee.

Both CTests pass normally and with ASan/UBSan plus leak checking of the new
agreement/fixture owners. Existing peer transport and the linked RTC stack are
outside that instrumentation.
The final expanded parser corpus also passes its own sanitized CTest and a
TSan build of the agreement/fixture. Receipts and source hashes are under
`.local/peer-host-agreement-20261018/verification.json`. All CMake builds use
`tools/run_tests.py`; compiler probes use `tools/run_test.py`. Generated private
builds and scratch are removed; retained outputs are small reports and logs.

## Session integration, 2026-10-18

`OnlineMatchTransportOptions::peer_host_migration` enables the direct-session
qualification path. Every admitted endpoint must opt in to the same epoch-prefixed
protocol. Ordinary matchmaking does not advertise or enable this gate. It is
incompatible with the older master-controlled complete-world continuation mode.

The session creates agreement state at loading and starts verification after
initial running admission. Accepted simulation-host game packets refresh its
trusted local silence clock; peer QoS, account signaling and voice packets do
not conceal a stalled simulation. Prepare/accept/ready/receipt traffic uses
the established player DTLS channels with its own four-packet budget after
gameplay and voice. Each receive independently verifies the outer epoch and
inner agreement context.

Once this endpoint accepts a proposal, it clears queued gameplay and voice and
fences ordinary media at send, receive and worker-drain boundaries. A learned
decision exposes `Migrating` or `Rebinding`, but retains the predecessor endpoint
until the native caller finishes its exact role change. `finish_peer_migration`
requires this endpoint's own immutable learned decision, including its locally
observed receipt set. A caller-invented certificate cannot open the channel.
The caller must first apply the actual complete native world/peer role; this API
cannot prove that work from metadata alone.

The native receiver now has a concrete handoff boundary:
`SourceAuthorityReceiver::prepare_peer_host_takeover`. It requires Replay mode,
both private native worlds to be quiescent, the exact paired validated cut, the
independent allocation admission and this endpoint's own learned decision.
The replacement must be this physical PC. A wrong decision, recipient, cut or
phase is rejected before consuming the receiver's world. The receiver then
stages the recipient's independently admitted local views using its reusable
native workspace and transfers the resulting unique owner on the same executor.
Receiver export and successor replay also compare the full execution boundary
recorded when the cut was validated. An intervening native write/call cannot
retain a receipt merely by keeping the same simulation tick or writing the same
bytes; a fresh authenticated full cut is required to repair that candidate.

`SourcePeerHostTakeover` restores the original connection/cookie/input bank,
rebinds and restores the promoted couch cameras, and applies the original
deferred departure to the failed host's couch bank. Allocated player/result
rows remain retained. Construction does not open gameplay or voice. The caller
must quiesce rendered-world borrowers and adopt renderer/controller/audio
owners before calling `activate`. Activation checks the unchanged native
execution boundary and still-current local decision/admission before asking
the session to publish the successor epoch. It is one-shot. A failed native
preparation destroys the private candidate and leaves session media fenced.

This owner is executor-local. It must not be moved out of the background replay
worker as a native pointer, or substituted into the current renderer while its
borrowed Match/world references remain live. `SourcePeerHostTakeover::prepare`
also accepts the worker's owning `SourceAuthorityRetainedWorld` value after
retention. It rechecks the exact local decision, complete native admission,
producer/recipient view maps and selected local configurations, then imports
and rebinds through a new private native pair on the receiving executor. The
background worker may be destroyed before this import. No original native
pointer, callback or device changes threads. The native host accessor remains
unavailable until successful one-shot activation.

The ordinary runtime still uses the owning retention value at its teardown
boundary; adoption into the renderer's long-lived references, physical
input/device rebinding and negotiated default migration remain
separate work.

### Runtime world and role ownership

`MultiplayerRuntimeAuthority` owns the live world/host/client role and the
native publication/recovery stream on the foreground executor. The initial
prepared world stays alive for immutable cache/content and resident rendering
resources. Runtime callers resolve the current Match and transport through this
owner rather than holding a Match reference permanently bound to the initial
role.

Every simulation/render frame holds its `Frame`, including frames without a
recording publisher. Nested frames, publication service, recovery export,
staging and activation are rejected while that scope is live. Normal completion,
early loop exits and exception unwinding retire the recording before releasing
the frame guard. The runtime must outlive its frames on the same executor.
A newly fenced session cannot begin another frame.

Staging consumes an independently prepared `SourcePeerHostTakeover` only at a
completed frame and leaves the current world/role visible until activation.
Rejection leaves the caller's unique owner intact. Renderer/input adapters must
bind the pending world's camera, local input and presentation state before
activation. All throwing authority construction precedes the session's successor
publication; success then replaces the live Match and transport together and
retires the old client. Audio retirement can leave the same candidate pending.

This lifetime seam does not itself reset per-world renderer caches, rebind
physical controllers, retire graphics queries, choose a candidate or certify
rendered handoff. Those remain required before connecting automatic adoption in
the windowed loop or enabling the default migration gate. Native-owner tests and
current-source renderer syntax checks have separate evidence under
`.local/runtime-authority-20261018`.

### Measured replacement preference and replay retirement

`OnlineMatchSession::migration_candidate()` and the no-argument
`propose_peer_migration()` select from independently declared native-ready cuts
using fresh reciprocal survivor rows. Only the agreed predecessor host is
excluded from scoring. A stale or disconnected second PC does not shrink this
fixed successor mask or the voting electorate. The scorer retains the existing
authored loss, acknowledged upload bottleneck, RTT and private peer-ID order;
it supplies neither recovered Xbox coefficients nor a measured NAT class.
A sole physical survivor needs its own retained cut and one-voter quorum,
without inventing a performance row for a nonexistent inter-PC path.

The measured proposal snapshots preference and starts agreement under the same
session-generation lock. The gated peer path now gives QoS, authority transfers,
independent native admissions and votes the same membership revision. Ordinary
setup keeps its existing revision scheme. Differing local preferences still
pass through Paxos: an earlier accepted host/cut outranks a later preference.
Measurement alone cannot activate a world or open gameplay/voice.

Foreground recovery now distinguishes an owning export receipt from completed
native destruction. After the export, it requests replay-worker cancellation
and polls `stopped()` before releasing the wrapper. That acknowledgement is
published only after its receiver, decoder worker and private native worlds
have been destroyed on their executor. OS events can continue between polls;
no borrowed native object crosses threads. This does not certify a frame-time
bound for subsequent foreground import or rendering.

Automatic proposal scheduling/retry, false-alarm/readiness lifecycle, additional
agreed membership removal, rendered controller/camera/cache adoption, compatibility
negotiation and default delivery remain separate gates. Keep the internally
qualified path distinct from ordinary delivered migration.

### Audio retirement and quiescent runtime callers

Retained `GameVoice` now fences its local delivery generation when the session
first fences the predecessor. Incoming decoded-delivery frames and outgoing
microphone frames carry a process-local stamp which is never serialized on the
peer wire. A held caller batch cannot be relabeled for the successor: `voice()`
rejects its old stamp, and the final transport drain also rejects old voice
frames already removed from the outgoing queue. Incoming frames are tagged by
the receiving session; a late held receive batch cannot enter a newer decoder.

`GameVoice::fence()` closes delivery without doing device I/O or joining the
worker on the network/simulation executor. Its worker retires talker frames,
decoder sequence/predictor state, the microphone encoder and queued capture/
playback samples. `ready()` acknowledges that all in-flight device work has
returned and queues have been discarded; `resume()` requires that exact stamp.
Enabled/microphone mute, manual mutes, service blocks and device ownership are
retained. Closed devices can acknowledge retirement but cannot reopen. Native
session completion waits for this acknowledgment before publishing the next
epoch and resuming audio. A passive postgame voice-leader/membership change also
retires the old local route. Hardware already beyond SDL's queue is a separate
physical acceptance gate.

Both runtime loops now suspend completed-frame work in `AwaitingWorld`,
`Migrating` and `Rebinding`, before source publication/recording or native scene
and presentation calls. The windowed path continues OS quit/resize/controller
events and held-key releases, stops rumble, drains relative mouse motion and
resets elapsed-time/voice-route baselines when a separately completed role
returns to `Running`. These caller changes pass current-source syntax checks;
they have not yet been qualified in a delivered rendered migration.

`game_voice_handoff` exercises actual Opus/PCM against queued synthetic devices,
including capture/playback calls held in flight. `peer_host_audio_session`
extends the real four-session/two-host-loss test with managed microphone and
decoded stereo. In-flight playback holds native completion while peer agreement
continues; late old microphone/receive batches are rejected, and every surviving
PC must receive fresh gameplay, voice packets and decoded audio after each
handoff. Native world readiness/role completion remain declared fixtures in
that session test. The preceding twelve-player native-world tests are separate
evidence, and physical microphones/headsets are not opened by these checks.

Five focused CTests pass with compiled C/C++ ASan/UBSan and leak checking; three
pass with compiled C/C++ TSan, with both session cases repeated in three fresh
processes. TSan uses `ignore_noninstrumented_modules=1`; system-library crypto
internals remain outside this qualification. See
`.local/voice-handoff-20261018/verification.json`. Generated builds/scratch are
removed. Automatic rendered migration, controller/device rebinding, sustained
memory/WAN acceptance and delivery remain open.

Completion constructs successor transfer, admission, QoS and agreement owners
before publishing the next host/epoch/membership together. Already running
old-context workers keep old objects. Queued media from an older epoch is
dropped at the final send boundary; new-host media arriving before a recipient's
native rebind is discarded. Native source allocation rows carry forward from
the independently accepted original roster, including departed-host PCR rows.
A promoted host's changed native handles/identity bytes cannot replace them.
Fresh offer acknowledgments still require an actual successor offer.

All agreed predecessors stay as a bounded receipt chain for delayed learners:
at most fifteen host losses can remove PCs from a sixteen-PC immutable room.
The chain shares the single four-packet budget. The active generation alone
controls current media. The next agreement uses the surviving physical PCs,
while couch memberships are derived through the original admitted owner map.
Post-setup master control messages cannot overwrite a peer-owned generation;
loss of the signaling socket or an old admission close does not terminate the
surviving direct path in this gated mode.

`peer_host_native_session` exercises four actual session/DTLS workers with
sixteen admitted couch seats, two real host socket/signaling teardowns, exact
host/cut agreement, forged-completion rejection, both native-completion media
barriers, immutable allocation identities, 16/12/8 seat membership, and resumed
gameplay/encoded-voice packets. Native ready cuts and role completion are
declared fixtures; devices and rendered simulation do not run. The ordinary
measured-host session and original variant-buffer checks also pass after moving
the shared variant encoder into `halo2_game_variant`, removing session setup's
build dependency on the entire asset cache library. That encoder's behavior is
unchanged. Evidence is under `.local/peer-session-migration-20261018`.

Normal and selected owner ASan/UBSan/leak checks pass. A C++ TSan run with
`ignore_noninstrumented_modules=1` also completes both handoffs, but this scoped
setting excludes the system OpenSSL report seen in the unsuppressed run.
Another scoped run produced an intermittent ICE candidate/transport error;
failure stages are now reported explicitly. Adding C instrumentation and
repetition exposed an atomic write/plain-read race on SCTP's global
`ipi_free_chunks` counter under different association locks. The new reviewed
`usrsctp-free-chunks-atomic.patch` changes both live reads to the dependency's
portable zero-add atomic primitive, preserving layout and the advisory value.
The patch applier verifies each patch and preserves conflicting local edits.
The next repeated C/C++ TSan run completed one full two-handoff process before
finding a separate global tick write/read race in `sctp_handle_tick` versus
`sctp_timer_start`. The full dependency stack remains unqualified; none of these
reports is silently treated as a proven false positive or a full race-check pass.
The latest seven-case selected-owner sanitizer run passes six cases, including
the new two-handoff session and both older continuation transport modes. The
ordinary measured-host fixture fails at exit with 4,848 bytes in 18 OpenSSL
allocations reported by LSan in each child process. Earlier five-case passes
remain recorded, but this later cleanup failure keeps broad sanitizer/teardown
acceptance open as well.

The next runtime integration must freeze the actual paired native cut and run
`LanHost::resume_after_host_loss`/`LanClient::rebind_host` at a verified native
boundary, retain controller/profile and headset ownership, flush/fence already
decoded voice-device buffers, select/retry candidates automatically from survivor
measurements, negotiate the new mode, and publish after-match authority/results
correctly. The current v12 master still closes its room on an explicit host
leave; this gated session keeps peer transport alive but has no completed
after-match admission repair. Keep default migration disabled until these
operations are verified together, including sustained memory, rendering and WAN.

## Transport teardown follow-up, 2026-10-18

The earlier report named `sctp_handle_tick` as its writer. The global tick
counter already uses the timer queue lock; the conflicting word is a callout's
pending/active flags. Association/address locks held by observers do not protect
the timer thread's updates. `peer_sctp_callout` reduces the actual race to a
dependency callout with a ticking thread and a pending/active/deactivate/start
observer. It fails TSan before the fix and passes after
`usrsctp-callout-flags-lock.patch` routes those observations/deactivation through
the existing queue mutex. The patch preserves flag masks, callout layout, tick
accounting and callback execution outside the queue lock.

A deeper LSan trace of the ordinary two-process measured-host session locates
the 4,848-byte exit leaks in per-thread OpenSSL random contexts created during
DTLS setup on ICE workers. The pinned RTC initializer holds a global runtime
token and starts its cleanup asynchronously. `OnlinePeerTransport` now owns a
shared runtime lease count: the last wrapper waits for `rtc::Cleanup()` after
releasing its channel, connection and callback-state owners. Starting
a new wrapper is serialized against that final cleanup. Closing an unrelated
room or unwinding a rejected constructor leaves existing rooms running. Crypto
providers stay alive while RTC workers stop; global OpenSSL cleanup is not
requested by this wrapper.

The original leaking session now passes with ASan/UBSan, leak checking and full
allocation unwinding, alongside direct datagram and runtime lifecycle cases.
`online_peer_runtime` covers two independent rooms, a rejected constructor,
continued authenticated maximum-size payloads, a fresh runtime restart, and
concurrent room creation/exchange/destruction. Full compiled C/C++ TSan also
passes the callout, runtime-lifetime and three independent two-handoff session
processes. That run uses `ignore_noninstrumented_modules=1`, so system-crypto
internals remain outside its guarantee. Native world/role completion is still
declared fixture metadata in the migration sessions. Receipts are under
`.local/peer-teardown-20261018`; actual native/controller/headset handoff and
default-mode delivery remain open.

The final broader run passes all nine CTests with compiled C and C++
ASan/UBSan and leak checking: callout, agreement, source admissions, original
variant buffers, both older continuation transport modes, ordinary measured-host
session, runtime lifecycle, and the two-handoff session. System shared libraries
remain outside instrumentation. This closes the reproduced timer-flag and
ordinary-session exit-leak regressions at that boundary; full native migration
acceptance remains separate.

## Rendered adoption boundary, 2026-10-18

`MultiplayerRuntimeAuthority` now exposes its restored camera and delivers the
retained local input bank within a native frame. Repeated polling before that
tick steps is idempotent. Hardware admission must remain suspended until its
pending saved actions are consumed; importing a world does not authorize
discarding new keyboard/controller taps. Both actual native takeover cases now
exercise three real pre-cut couch input packets, one consumed before capture
and two retained ticks after promotion. Twelve native ticks and direct encoded
voice packets continue after old-host teardown.

Completed-frame machine/prop/scenery reset APIs preserve resident geometry and
material slots while retiring presentation lifetimes. Original-asset CPU checks
pass with earlier clocks and identical salted handles. They do not reset every
renderer borrower, effect/HUD/interpolation cache or device observer. Automatic
windowed adoption and default migration therefore remain gated.

The native lens-query owner cannot simply cross world pointers. Inspection of
the authenticated TU1.5 implementation establishes that `3803B0` only copies a
float to `532098`; it is not a complete query-ring reset. `20680` clears request
admission flags under its original mode/frame rules, not all pending GPU query
bits. `20700` conditionally delegates to `20E30`, which clears a slot's three
ring bits and request record; calling it outside those original conditions is
not yet a qualified world-adoption policy. Device retirement must distinguish
the imported native request state from actual local GPU completions. Do not
invent samples or clear checkpointed state merely to bypass that owner check.

All ten selected-owner native/scene/peer/voice ASan/UBSan/leak CTests pass in the
coherent cohort. Current-root runtime syntax and critical source comparisons
also pass. The supervised 2,961,309,972-byte generated tree and all recorded
compiler scratch are removed. Receipts and explicit limits are under
`.local/rendered-handoff-20261018/verification.json`; no hardware/GPU/WAN or full
default runtime acceptance is claimed.

## Automatic proposer scheduling, 2026-10-18

The gated direct session now drives elections on its network worker after an
independent native owner publishes a retained cut. Each service pass obtains
the measured survivor preference in the same session generation and calls
`PeerHostAgreement::drive`. No positive caller proposal is required. The
driver never creates readiness, changes the electorate or acknowledges native
role completion.

An initial proposer is staggered by its order in the fixed physical electorate
at 50 ms per position. A proposal retry waits 1,000 ms plus that same position
offset. New promises, phase transitions, accepted votes and higher rejections
defer another attempt. Duplicate messages do not refresh that deadline, so a
failed proposer cannot monopolize it through replayed old prepares. Ordinary
100 ms packet retransmission stays separate from creating a new ballot. Host
silence must still exceed 500 ms, and retries still carry the highest previously
accepted value. These retry timings are new client policy, not recovered Xbox
constants.

The automatic unit cases cover delayed promises without per-poll ballot
restarts, a vanished proposer with repeated old prepares, preservation of its
accepted cut, a healthy-host majority veto, sixteen physical peers and 64
varying-preference schedules under lost/reordered/duplicated traffic and a
four-packet polling budget. Both ASan/UBSan/leak and unsuppressed owner TSan
checks pass, including concurrent driver/state/receive/observation operations.
Six broader C/C++ ASan/UBSan/leak CTests pass. Three broader C/C++ TSan CTests
also pass, with the measured and decoded-voice sessions each repeated in three
fresh processes; `ignore_noninstrumented_modules=1` excludes system-library
internals in that broader run. Session-native readiness and completion there
remain declared fixtures. Seven selected-owner native-cohort ASan/UBSan/leak
CTests also pass. Both actual twelve-player native takeover paths now elect
through their session workers without a positive caller proposal, then retain
the existing explicit native staging/activation and surviving-client rebind
barriers. Four local seats, an existing four-seat client, saved input/camera
state, twelve native ticks and twelve encoded direct voice packets continue.
Native/generated/RTC/system interiors remain outside this selected native-owner
instrumentation; no full rendered or physical-device acceptance is implied.

The native fixture now withholds readiness until the original host's actual
socket/session teardown; its private validation can legitimately pause that
fixture host for longer than 500 ms. This avoids accidentally electing while the
test is still setting up a supposedly healthy host. The measured transport
case similarly withholds readiness until predecessor telemetry expires, then
requires the session worker to initiate the election. The healthy ready-host
veto remains covered in the ordinary and decoded-audio sessions.

This closes the caller-driven election loop at the tested scopes, not the
full recovery coordinator. Automatic readiness retention and its false-alarm
lifecycle, foreground world preparation/adoption, rendered/input/device
retirement, compatibility/default delivery and after-match repair remain open.
Receipts: `.local/automatic-election-20261018/verification.json`. All six root
private build/scratch trees and the supervised native workspace are removed.
The latter contained 27,199 generated regular files totaling 2,732,562,234 bytes.
Small reports/source hashes/diagnostics remain; no new large artifacts are
retained. Frozen inputs, original assets, selected runtimes and unrelated work
are preserved.

## Native preparation and atomic publication, 2026-10-18

`MultiplayerRuntimeAuthority` owns a filtered gameplay recipe shared with replay
and takeover construction. It does not retain desktop, prepared-world or
presentation callbacks. At a completed frame it can retire the replay worker,
import its exact retained value and stage the locally learned replacement once.
The disposable import uses the producer's independently admitted views, validates
all native identities and the recipient profiles, then binds recipient views in
that same private world. This follows the existing native role-rebind sequence;
the second temporary import world is removed. Worker replay still retains its
existing candidate/workspace pair. No memory or frame-time bound is claimed.

Both runtime pause loops now prepare their learned host or surviving-client
role. They still do not advertise readiness or activate before the remaining
presentation/input/device adoption work. A native pending owner is unchanged
by repeated preparation calls. Later menu/profile changes cannot alter its
owned recipe, and a wrong standalone profile or view mapping is rejected.

`finish_peer_migration` can run an allocation-free native commit on its caller's
executor. Independent decision/admission checks, audio retirement and all new
transport/admission/quality owners precede the callback. A false callback leaves
the transport epoch unchanged. Continuity admission copying also occurs before
it. The host callback moves the staged world/authority/host ownership before
transport publication; old client destruction is deferred outside the session
lock because it can send a leave. The client callback commits an opaque native
endpoint preparation bound to its owner, cookies, slots and role revision.
Neither callback calls the session or performs device I/O.

Client preparation changes no endpoint, input serials or accepted receive bank.
Stale/foreign commit rejection keeps the caller's prepared value; successful
commit clears old receive assemblies and preserves its admitted client/slots.
The runtime uses its own locally learned certificate, rather than requiring
another learner's ballot/voter set to be byte-identical. After rebind it can
restart a cancelled/failed old shadow on its own executor without joining live
native destruction on the caller.

Final evidence is `.local/native-prepare-coordinator-20261018/verification.json`:
seven selected-owner native ASan/UBSan/leak CTests, three root C/C++ ASan/UBSan/
leak session CTests and two root C/C++ TSan CTests pass. Both TSan session cases
run three fresh processes and exclude system-library internals. Generated/native,
RTC and system interiors remain outside the selected native instrumentation.
The two actual twelve-player source cases preserve local input/cameras and
allocation/results, an existing four-seat client, twelve native ticks and twelve
direct encoded voice packets. Current-root runtime syntax checks pass separately
from the frozen coherent native ABI plus current hashed owner overlays.

The final CMake/CTest steps all completed successfully before the supervising
recipe terminated with143 while awaiting finish. Its cleanup supervisor removed
the native workspace; both root private trees and all compiler scratch are also
gone. Passing bounded reports and the pre-build source manifest remain. An
optional final detailed trace and pre-cleanup byte count are unavailable; no
passing result is inferred from the supervisor exit. No new large artifacts
are retained. Full rendered/input/device adoption, automatic readiness and its
false-alarm lifecycle, runtime activation, sustained memory/timing, hardware/WAN,
default delivery and after-match repair remain open.

Input follow-up from the preceding owner audit: `LanHost::LocalContinuation`
contains the predecessor's received/delivered press counters at the retained
cut. `LanClient::send` can advance its own outgoing counters afterward, but
host activation currently retires that client without merging those newer
local commands into the promoted bank. The pre-cut pulse checks above do not
cover this interval. Preserve and independently validate the client's latest
owning input before retirement, then test post-cut presses/held controls before
admitting fresh hardware. Keep default migration gated until that path and the
rendered/device adoption work are verified.

The owning input follow-up is now qualified at the explicit native boundary.
`LanClient` retains its latest encoded input; before retirement, runtime host
activation checks it against the original independently accepted connection,
session, cookie, couch/profile table and received/delivered serials. The ordinary
wire decoder validates all fields before the local bank changes. Only controls,
received counters and completed interaction data change; native world/camera and
already-delivered counters stay at the accepted cut. The pending owner retains
its immutable cut until successful activation, so refresh/refused-adopter retries
are idempotent and cannot duplicate consumed presses.

Evidence: `.local/promoted-input-continuity-20261018/verification.json`. Seven
selected-owner native ASan/UBSan/leak CTests and current-root syntax checks pass.
Both twelve-player source cases deliver two post-cut packets across four local
seats, four remaining action ticks, latest movement/look/zoom and one completed
interaction per seat, then retain the existing four-seat client and twelve
direct voice packets/native ticks. Foreign/malformed/consumed banks are rejected.
The frozen coherent native ABI plus current owner overlays is the native test
cohort; generated/native/RTC/system interiors are uninstrumented. The supervisor
exited successfully and removed its 2,738,244,560-byte private workspace; compiler
and cleanup scratch are absent. Forty-four unused old test executables reclaimed
another 5,703,429,208 bytes. Only bounded reports remain. This does not qualify
hardware input ownership or rendered/device adoption; default migration and the
full ONLINE goal remain open.

Windowed input now uses the authority's retained-input ownership at command
delivery. Device admission is suspended across the network fence and drain;
native input owns movement/look/zoom and interaction targets until its final
stepped tick. The frame then stops catch-up before reacquiring hardware. OS/focus/
hotplug events continue, relative motion is discarded, held interaction clocks
exclude the pause, existing pad edges are retained, and changed controller
instances cannot inherit an old hold. KBM also preserves a completed old hold
through intervening post-resume events before admitting another held request.

Evidence: `.local/runtime-input-handoff-20261018/verification.json`. The actual
SDL adapter passes 287 ASan/UBSan checks; current-root syntax passes. A frozen full
current source cohort plus latest device/fixture updates builds and passes a
154-tick windowed TU1.5 normal-input regression. Its ordinary runtime path does
not exercise migration. GPU validation layers are unavailable, and the private
CTest explicitly runs normal Vulkan. The original timer-based reload assertion
was replaced by conserved ammunition transfer after a recorded R request, with
the failing trace preserved; no gameplay state/timing was modified to pass it.
The successful supervisor removed its 1,063,983,811-byte private source/build/
cache tree; the earlier interrupted tree and adapter/compiler scratch are also
absent. No new large artifacts remain. Rendered migration and independent couch
hardware, full presentation/GPU/device adoption, current native ABI migration
coverage, default delivery, readiness/false-alarm and remaining full-goal gates
are still open.
