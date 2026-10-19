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

Incoming native preparation now retires the predecessor's CPU lens-query bank
using original20E30 for each allocated request, following original20700's
retirement loop. This happens only on the completed private incoming executor,
before its adoption boundary is sealed. It preserves the original high-water
count and unused bitmap bits; a malformed count is rejected before a native
write. No predecessor GPU completion, timestamp or sample is invented.

The renderer-side owner has an explicit completed-frame rebind operation. It
requires the incoming CPU bank to be retired, allocates its replacement private
device ABI first, then waits out/retires old GPU query ownership. Only the
incoming world's identity survives the read-only inspection; the next native
frame obtains its actual borrower. Old results/generations/source pointers do
not cross that boundary. Invalid, duplicate and foreign-executor requests reject
without retiring the live GPU owner. This primitive is not yet the full windowed
presentation adoption coordinator.

Evidence: `.local/lens-query-handoff-20261018/verification.json`. Sixteen
unchanged-original CPU differentials and eight current-source native/session
selected-owner ASan/UBSan/leak CTests pass. Both twelve-player role transitions
retire33 declared pending CPU requests from their accepted cut before resuming
the existing native/input/voice checks. RTX5080 query assertions and Khronos
validation pass, including45 original quads and fresh alias reuse after actual
GPU retirement. The GPU process remains LSan-failing on shutdown allocations;
minimal Vulkan controls without any Halo/SDL/query code reproduce those vendor
allocation families, including glcore/glsi while the device is alive. This is
not a fully leak-clean GPU qualification. The supervisor removed its1,512,947,527
byte generated workspace and temporary layer; compiler/control scratch are
gone. No large artifacts are retained. Other renderer/effects/HUD/device adoption,
rendered migration, readiness, physical/WAN, default delivery and remaining full
ONLINE acceptance stay open.

2026-10-18 final lens owner: gated migration clients now prepare authored map
flare images before renderer creation without running a native producer.
SourceLensDrawFrameTu15 can bind a retired incoming bank at a completed frame.
It allocates its replacement private owner before publication, retaining bitmap
and material identities while discarding SDK/header/FIFO/camera/counter state
and the predecessor borrower. Invalid bank, active frame, wrong executor,
duplicate world, mismatched native borrower and expired renderer checks reject.
Repeated original header initialization over two native worlds retains the
sealed adoption boundary. A cold owner stands for uploaded client resources;
this is not an executed ONLINE client-window migration.

The native executable builds and six CPU/reference tests pass in the refreshed
source cohort. Both GPU functional reports and Vulkan validation pass; the GPU
processes remain LeakSanitizer failures, with no suppressions or disabled leak
checks. Evidence: `.local/renderer-lens-adoption-20261018/verification.json`.
The supervisor removes 1,519,555,707 logical bytes across 34,761 generated files;
the disk snapshot workspaces are removed separately. No new large artifacts
remain. The complete adopter must prepare every replacement before irreversible
GPU retirement, rebase effects/HUD/event/interpolation histories and source
pointers, then publish the native role. Those integrations, readiness and full
rendered/physical/WAN acceptance remain open; default migration stays gated.

2026-10-18 HUD/device/cue preparation: MultiplayerHud::reset_lifetimes clears
private winning, shield, radar and speaker histories at a completed frame,
preserving resident resources, authored Vista sampling offsets and all local
prompt bindings. Every view must be observed/animated again, and the enclosing
runtime must invalidate its radar upload tick even when the accepted world uses
the same tick and salted unit identity. NativeDeviceEventCursor::rebase validates
the incoming journal before replacing its delivered boundary. Accepted requests
stay silent, and later events may reuse serials from a discarded future.
NativeEffectSounds::retire_pending clears only unsubmitted cues; accepted active
effects and already playing audio need their own coordinated owners.

Eleven focused selected-owner ASan/UBSan/leak CTests pass in the recorded full
source freeze plus explicit HUD/test updates. Both artwork paths compare16
declared replacements and320 full CPU local-view streams to independent fresh
HUDs, with16 stale-history controls. Shader-only controls initially missed
geometry differences; the corrected test then caught the erroneous retirement
of static Vista shield sampling offsets. The original HUD fixture now uses each
actual biped's authored HUD rather than expecting a profile change to reskin an
already selected biped. Failure diagnostics remain with the passing reports.
Evidence: `.local/hud-event-handoff-20261018/verification.json`.
All owned scratch is removed, including34,163 generated files and1,653,612,981
logical bytes in the main workspace. These primitives do not activate normal
runtime migration or establish rendered/GPU acceptance. Complete active-effect,
interpolation/source/device adoption, readiness/false-alarm lifetime, delivery
and full ONLINE acceptance remain open. Gameplay and match voice remain direct
and player-hosted; default migration remains gated.

2026-10-18 active effects: both native presentation adapters now retain owning
local checkpoints and prepare their complete replacement before publication.
The world owner rebinds every effect/event/system definition link to its copied
graph; light pools/spatial lists/query epochs, particles, emitters, decals and
pending sounds retain their accepted lifetimes. The muzzle owner retains all
five allocation pools, salted identities, particle histories, event/pose state
and pending sounds. Both preserve resident material identities, reject foreign
checkpoint/restore tokens and consume a publication token once. Capture excludes
collision callbacks and copies RNG values; preparation requires an explicit
accepted-world callback, long-lived RNG words and the original alias relation.
Neither preparation nor publication writes shared RNG. The enclosing quiescent
coordinator must restore it with all consumers and bind the local cuts to the
agreed authenticated native cut before publication.

Two TU1.5 selected-owner ASan/UBSan/leak regressions pass. An actual contact/
grenade presentation at tick210 retains24 particles,11 decals and1 light and
continues40 further match ticks against the independent batched control. Four
local light-query caches/epochs resume after their own restore. Two active
muzzle restorations discard later flashes, retain the accepted sound once and
resume12 ticks, new allocation generations and matching draw/sound outputs.
The wider600-shot/Sword and60-contact/one-grenade/manual-SDL-audio/localhost-LAN
regressions remain green. These are local presentation policies and CPU outputs;
they do not execute an ONLINE role change or prove rendered migration.
Evidence: `.local/effect-presentation-handoff-20261018/verification.json`.
All owned source/build/compiler/check scratch is removed, including34,152 files
and1,078,856,808 logical bytes in the main workspace; no large artifact/runtime
is retained. Accepted-cut association, map/combat caches, scene/FP interpolation,
source/device/audio adoption, coordinated GPU retirement, automatic readiness,
delivery and physical/WAN acceptance remain open. Default migration stays gated.

2026-10-19 grouped effects: SourceMapEffects and SourceCombatAttachments now
checkpoint their local lifetime caches, last successful synchronization tick
and completeness. A failed synchronization cannot be captured; an older
completed checkpoint can repair it. EffectPresentationContinuity captures both
with muzzle/world effects and visual RNG, then binds that owned state after
asynchronous native validation without recapturing a later live world. Bound
peer/context/seal equality, owner and executor checks fence preparation and
single-use publication. All four private replacements precede publication;
shared RNG is restored once, with no intervening allocation/callback. The
enclosing coordinator still owns native/GPU/device readiness and quiescence.

Six selected-owner ASan/UBSan/leak CTests pass, including original TU1.5 Turf
native import and grouped repair at tick45 (4 map births,4 combat instances,
88 particles), twenty continuation ticks, faulted caches, late binding and
rejection of foreign executors/owners, changed cut fields and consumed tokens.
The combined FP presenter is empty; its separate active restore regression
passes too. Context is declared fixture metadata, not a quorum certificate.
Evidence: `.local/effect-cache-cut-20261019/verification.json`. The supervisor
removed34,272 generated files/1,263,200,521 logical bytes; all owned compiler/
check/build scratch is absent, with no large artifact or installed runtime.

Source inspection at that checkpoint identified the next proof requirement: the native host
continuation retains each peer's snapshot_sequence, and the client accepts
same-tick snapshots that change ownership/roster. Its public received_states
then omitted sequence association. Binding a local frame to a native cut
therefore needs the admitted sequence/cookie/epoch and paired host continuation,
not tick equality alone. The grouped API explicitly leaves this proof to its
caller. Normal capture/retention, readiness, complete scene/FP/device/source/
audio/GPU adoption, role activation, delivery and physical/WAN acceptance remain
open. Default migration stays gated and gameplay/match voice remain direct.

2026-10-19 accepted snapshot origins: each fully decoded accepted state now
has an owning opaque receipt alongside received_states, in the same order.
The receipt binds the exact body digest/bytes, transport sequence, fingerprint,
nonce/cookie, physical owner and current admitted context/rebind revision.
The host continuation retains the corresponding last sent stamp for each peer.
LanClient::prove_snapshot checks that receipt against the host sidecar from the
independently authenticated and natively validated paired cut. It does not
authenticate/validate the complete world/sidecar pair itself. Older owned
receipts can await validation after later receives; host rebind retires the old
generation. Receipt-based effect captures cannot use the raw tick/cut binding
overload to bypass this proof. The caller must consume the exact associated
state before capture; an opaque receipt cannot attest arbitrary caller state.

Explicit sidecar bytes are H2HOST02; H2HOST01 rejects. Native gameplay protocol89
and gameplay packet layout are unchanged in this step. Coherent feature
negotiation/version delivery is still an activation gate. Four selected-owner
ASan/UBSan/leak CTests pass with a full current source freeze and the explicit
allocation update: exact snapshot origins, host continuation, native role
rebind and grouped effects. The new real TU1.5 cut case covers four couch seats,
a same-tick late join, an older valid body relabeled with a newer sequence,
duplicate/stale/foreign/unbound receipts, version rejection and epoch retirement.
Logical transport, locally validated cuts and empty presenters do not establish
live DTLS/WAN/quorum/runtime/GPU migration. Evidence:
`.local/snapshot-origin-20261019/verification.json`. All recorded scratch is
removed, including34,314 files/1,271,417,932 logical bytes; no large artifact or
runtime is retained. Runtime bounded history/readiness, same-tick ownership
refresh without duplicate animation advancement, complete owner adoption,
GPU/native publication, delivery and physical/WAN acceptance remain open.
Default migration stays gated; gameplay and match/postgame voice stay direct
and player-hosted. The full ONLINE goal remains active.

2026-10-19 normal replica adoption: runtime now indexes the accepted state and
origin banks together and adopts full newer same-tick bodies. It preserves
the earlier completed-tick endpoint while explicit WORLD/scene refresh retires
missing or changed owners. Gait lifetimes resolve without advancing a channel,
aim consumes zero elapsed ticks and same-tick FP channel pairs do not replay
events. The interpolation interval does not restart. A read-only replica
observer retains receipt association; actual completed locomotion endpoints
can be observed independently of render interpolation. No packet, master or
voice transport behavior changes.

One actual TU1.5 authority and four local viewport views pass the localhost UDP
same-tick join/departure case on RTX5080 with a private Xvfb display and Khronos
validation: tick2,39 receipts/36 refreshes,28 frames,192 unchanged gait draws and
four real local weapon cues without replay. Native departure preserves its
connected report row with the original leave flag. Five CPU/native component
checks pass again with selected interpolation-owner ASan/UBSan and leak checks.
The separate upgraded native late-audio case remains red (10 FP-adapter cues,
20 WORLD-client submissions, no stale prejoin playback). Its actual effect
contains two equal-name locations; original TU1.5 WORLD/FP sound admission is
unresolved. No arbitrary halving/deduplication was applied. Native Match also
exports complete FP channel pairs only for its producer-local views; this case
observed zero remote pairs and does not establish remote FP channel audio.

Evidence: `.local/replica-refresh-20261019/verification.json`. All recorded
scratch, including private displays/validation dependencies, is absent;34,710
generated files/1,528,119,727 logical bytes are removed from the main workspace.
No large artifact or installed runtime is retained. Bounded presentation/cut
history and readiness, authentic remote FP ownership, the sound-admission
regression, complete device/source/audio/GPU/native publication, timing/memory,
delivery and physical/WAN acceptance remain open. Default migration remains
gated and the full goal stays active; match gameplay/voice remain player-hosted.

2026-10-19 WORLD/FP admission: the sound regression above now passes. The source
WORLD-location export walked both raw WORLD and encoded FP records sharing one
marker name, whereas original17D210 dispatches parts through17DCE0(mode0).
Whole unchanged English TU1.5 iterator replay covers all65,536 node words in
modes0/1/3, mixed chains and source-write fencing, and checks both actual caller
push0/call instructions. The exporter now applies the exact WORLD/unbound-node
predicate before original transform resolution. Distinct admitted WORLD nodes
remain distinct; this is not sound-count or marker-name deduplication.

The live native allocation audit observes one WORLD and one FP node and exactly
one published WORLD location. Original scheduled firing supplies10 postjoin
cues and the remote viewport submits10 actual SDL packets, with no stale
prejoin replay. Eight normal checks and six selected-owner ASan/UBSan/leak
checks pass. The four-view RTX5080/Xvfb/Khronos-validation case still passes;
remote FP channel pairs remain absent and are not fabricated. Evidence:
`.local/world-location-admission-20261019/verification.json`. All recorded
scratch is removed, including34,792 files/1,553,200,174 logical bytes in the
main workspace; only one small65,536-byte reference fixture and bounded reports
remain. No installed runtime is changed. Authentic remote FP ownership,
bounded presentation/cut readiness, full owner/audio/GPU/native publication,
delivery and physical/WAN acceptance remain open. Default migration stays gated
and the full goal remains active; gameplay/match voice remain player-hosted.


2026-10-19 foreground host-cut proof: the replay worker can now export an owning,
immutable `SourceAuthorityHostProof` beside its ready cut. The native receiver
alone creates it after independently admitted complete world/host validation;
allocation precedes acknowledgement and status publication uses the existing
mutex. Its client query checks the exact accepted receipt's connection,
generation/rebind revision, sequence and body. It exposes no native borrower,
and the validator peer is not confused with the producer host.

Current proof clears with readiness during incomplete replay, cancellation,
world consumption and executor failure. Native-only transfers cannot create
it; a rejected host-stripped downgrade keeps the previous paired proof. Owning
copies survive later validation and worker destruction, while the old client
proof fails after rebind. These are historical cut-association facts; a copy
is not a current-world readiness certificate or permission to elect/activate.

Five normal focused cases pass, with a stream fixture correction to keep normal
gameplay receiving alive during asynchronous replay (70 accepted snapshots).
Three native cases pass selected receiver/fixture ASan/UBSan/leak checks and
two uninstrumented companion controls pass. Local direct-DTLS takeover covers
both survivor clients, twelve couch seats, receiver/background-retention paths,
twelve resumed native ticks and encoded direct voice packets. This does not
establish a rendered handoff, headset/controller or WAN acceptance. Evidence:
`.local/validated-host-proof-20261019/verification.json`. All recorded scratch
is removed, including34,386 files/1,358,022,460 logical bytes from the owned
workspace; no large artifact or installed runtime remains. Concurrent work and
reference/launcher inputs are preserved. Bounded presentation history, actual
readiness, authentic remote FP ownership, complete owner/audio/GPU/native
publication, delivery and physical/WAN acceptance remain open. Default migration
stays gated and the full goal remains active; match gameplay/voice stay
player-hosted.


2026-10-19 completed CPU effect history: the windowed runtime now retains exact
accepted receipts with completed grouped effect state under the internal
migration gate. Independent session context, native modulo32 sequence ordering,
duplicate idempotence, eviction/replay refusal and failed-capture preservation
prevent tick-only association or recapturing old receipts. The receiver-only
paired native proof binds retained state through the actual client connection.
Current policy is128 entries plus one independently owned matched checkpoint;
it is not an original Xbox constant, a byte budget or an election certificate.
No readiness/activation is advertised by this new owner.

Nine normal cases pass across the expanded run and corrected ONLINE fixture.
The latter captures257 accepted receipts, retains at most128, binds two native
world/host cuts and renders463 four-view frames over real local DTLS. Synthetic
identity,30Hz host pacing and10Hz snapshots are declared fixture boundaries;
no host is promoted in this rendered case. Strict native configuration checking
caught the fixture's pre-override advertised variant; using the resolved world
variant fixes it without relaxing validation. Three native/history/group cases
pass selected ASan/UBSan/leak checks with two uninstrumented companion controls.
Raw-LAN clock/gait/audio and native late-audio regressions pass.

Whole fixture peak RSS7,408,412 KiB includes collocated native worlds/renderer;
immutable checkpoint sharing, byte budget, sustained catch-up and all-map/load
qualification remain open. Evidence: `.local/effect-history-20261019/verification.json`.
All recorded scratch is absent;34,857 files/1,742,441,803 logical bytes removed,
no large artifact or installed runtime retained. Concurrent work/reference/
launcher inputs remain intact. Full readiness, authentic remote FP/event
ownership, complete owner/audio/GPU/native publication, delivery and rendered-
migration/physical/WAN acceptance remain open. Default migration stays gated
and the full goal remains active; gameplay/match voice remain player-hosted.


2026-10-19 active-light memory: the effect checkpoint now shares owning immutable
BSP atmosphere/light geometry/terrain while retaining independent mutable light
pools, spatial registration, fades and query caches.128 actual active-light
cuts add40,763,552 live heap bytes versus95,980,784 before (57.53% reduction)
in a matched normal glibc allocation probe. Four normal cases and three
selected-owner ASan/UBSan/leak CPU checks pass, including active light identity/
restore and grouped continuation. The ONLINE fixture exercises256+ receipts,
128 retained entries and two actual native associations. Whole-process RSS
shows no reduction, so this does not qualify the full client memory budget or
sustained catch-up. Evidence:
`.local/effect-light-residency-20261019/verification.json`.

All recorded scratch is removed:34,871 files/1,358,228,683 logical bytes from
the owned workspace; only small source/hash/report evidence remains. No large
artifact or installed runtime is retained. Concurrent work/reference/launcher
inputs are preserved. Full readiness, remote FP/event provenance, complete
owner/audio/GPU/native publication, delivery and rendered-migration/physical/
WAN acceptance remain open. Default migration stays gated and the full goal
remains active; gameplay and match/postgame voice stay player-hosted.

## Staged lens presentation transaction, 2026-10-19

`SourceLensPresentationHandoffTu15` stages query and final-draw private owners
before retiring either predecessor. Readiness is bound to the actual owners,
their common live renderer, incoming retired ring and completed-frame revisions.
Even an unsubmitted intervening frame invalidates its token. Paired commit
rechecks both, retires actual old GPU work and swaps both private owners without
allocation or original-world callbacks. The enclosing native transaction still
owns world lifetime, authentication and complete adoption-seal validation; this
local presentation step supplies none of those proofs or election readiness.
The existing Vulkan device wait remains, so bounded migration time is unproven.

The native executable and 11 normal focused checks pass, including two native
worlds, five preserved pending-result controls, stale/cancelled/foreign/consumed
token rejection, original query/final-draw GPU regressions and empty Khronos
logs. Current-root GCC syntax checks pass. Evidence:
`.local/lens-transaction-20261019/verification.json`. All private source/build/
capture/dependency scratch is removed (35,086 files / 1,529,265,786 logical
bytes); no runtime is installed or retained. Concurrent work and reference
assets remain intact. Full runtime adoption/readiness, sustained timing/memory,
visible-flare/rendered migration and physical/WAN/full-ONLINE acceptance remain
open. Default migration stays gated; gameplay and match voice stay player-hosted.

## Scene lifetime retirement seams, 2026-10-19

The windowed adopter now has explicit retirement seams for six scene owners:
scenery, machines, props, dynamic objects, weapon projectiles and released
grenades. The three newly covered owners discard per-world light/pose/LOD and
model/glow draw history while preserving authored resident resources. All six
operations and the shared geometry history reset are `noexcept`; caller-owned
quiescence and complete native adoption remain required. Cold reset is not an
exact retained lighting-history checkpoint or authority/election proof.

Five normal and five selected-owner ASan/UBSan/leak checks pass, including
fresh-scene equality across four views with unchanged salted IDs after a
frame300-to-frame0 rewind. An existing fifth-view prop regression also passes
after moving its input guard ahead of source-cache access. The private native
executable builds; current-root GCC syntax and source audits pass. Evidence:
`.local/scene-retirement-20261019/verification.json`. All private build/source/
capture/dependency scratch is removed (34,926 files / 1,320,113,892 logical
bytes). No runtime is installed or retained. Complete runtime owner adoption,
exact history/FP provenance, readiness, timing/memory and rendered/physical/
WAN/full-ONLINE acceptance remain open. Default migration stays gated and the
full goal stays active; gameplay and match voice remain player-hosted.

## Native runtime presentation commit, 2026-10-19

`MultiplayerRuntimeAuthority` host/client activation now accepts one executor-local
prepared presentation publication callback. The session first validates the
learned decision, native boundary/admissions and audio generation, and allocates
successor owners. The callback then runs before runtime host adoption or native
client endpoint commit, and before transport/voice opens. Refusal retains the
pending transition; callers must preserve their own owners on false. Successful
publication cannot allocate, block, do device I/O or call the runtime/session.
Runtime ownership checks reject reentrancy before entering the session mutex.

Three normal and three selected-owner ASan/UBSan/leak checks pass, including
both actual twelve-player direct-session variants, a receipt-bound four-owner
cold CPU effect publication, rejected/retried hooks and continued native ticks/
encoded voice. Current-root syntax/source audits and the native executable
build pass. Evidence: `.local/runtime-presentation-commit-20261019/verification.json`.
All recorded scratch is removed (34,956 files / 1,345,909,397 logical bytes);
no runtime is retained or installed. Full windowed adoption, deferred GPU
retirement outside the locked callback, active scene/FP/effect/history, device/
playing-audio, readiness, timing/memory and rendered/physical/WAN/full-ONLINE
acceptance remain open. Default migration stays gated and the goal stays active.
Gameplay and match voice remain player-hosted; Vista ONLINE/clan UI is unchanged.

## Deferred lens GPU ownership and native publication, 2026-10-19

The lens transaction can now detach its old GPU/query-result ownership without
performing device work inside publication. Preparation acquires the actual
device lifetime and allocates replacement CPU owners. Readiness rejects other
executors, retired leases, changed owner/revision/incoming rings, intervening
renderer work and consumed tokens. Lease eligibility is an atomic snapshot;
serialized `acquire()` remains the admission operation. The renderer's executor
exists in both particle-enabled and particle-disabled configurations.

The enclosing adopter must keep the consumed renderer/query/draw/group token
alive across `activate_takeover()` / `activate_client_rebind()`. Release or
move-assign that token afterward on the renderer executor, outside the native
callback/session lock and before destroying the SDL window. Old CPU owners,
GPU pools and completed results stay in that token; its device use outlives
the retired GPU destructor. Destruction still performs the existing device
wait. This is deferred retirement, not a bounded or wait-free migration claim.
The synchronous `rebind()` convenience still retires at its own return and
must not be used as the enclosing locked presentation callback.

The private native build and 12 normal focused CTests pass. Two real native
worlds preserve their sealed boundary/materials and six rejected/cancelled
preparations preserve old GPU results. Real Vulkan observers record zero
wait/destruction/query-read calls inside both commits, then one wait/two pool
destructions and the expected remaining cleanup outside. Original query/draw
and device contracts pass with empty Khronos logs; nine GCC syntax checks
include both particle configurations. No new sanitizer, active-flare/rendered
migration, physical or WAN acceptance is claimed. Evidence:
`.local/lens-deferred-retirement-20261019/verification.json`.

M01 in the [working checklist](online-completion-goal.md#working-checklist-and-goal-completion-contract)
is checked. All private scratch/processes are absent after removing 35,248
generated files / 1,552,133,164 logical bytes; no binary/runtime is retained or
installed. Complete exact runtime/event, scene/model/FP/camera/HUD/input/audio
adoption and readiness remain M02 onward. Default migration stays gated and
the full goal stays active; gameplay and match/postgame voice remain direct
and player-hosted, with Vista ONLINE/clan UI requirements preserved.

## Exact owning receipt/runtime frontier, 2026-10-19

A continuity-bound accepted `MultiplayerSnapshotOrigin` now owns the immutable
decoded state and exact local slots, beyond later receive batches or destruction
of its client. `state_origin()` returns the latest adopted receipt, retains it
across empty polls and clears it on continuity bind or committed endpoint rebind.
Unbound receive rows remain unbound. `same_connection()` compares historical
ownership/context/generation; it does not authorize a currently live endpoint.

Typed effect history publishes one entry containing that authentic receipt,
the completed grouped CPU effects, and the runtime event/binding/deferred-audio
frontier. Capture validates exact current and previous receipt ownership and
slots; the history tracks the completed-tick predecessor internally. The initial
baseline can be seeded only from the actual client's adopted receipt in the same
context. A missing first predecessor is left unretained, without fabricating one.
The caller cannot supply a preexisting pending-cue checkpoint: history captures
it freshly from the bound sound owner before completing the shared entry.
All copies/captures precede insertion or eviction, so failed capture preserves
previous entries. Native proof must bind the exact connection, context, sequence
and body; a newer same-tick receipt cannot substitute for an evicted receipt.

`MultiplayerRuntimeReceiptFrontier` retains the full-salt firing/object bindings,
event cursors/serials/baselines, completed muzzle/world ticks, local state and
undelivered sound/request queues at the accepted CPU-effects boundary. It does
not advance wall-clock audio once per received snapshot. The enclosing adopter
must resume the correct frame phase and flush those retained queues once.

Pending effect cues have an executor/owner-bound checkpoint and prepared restore.
Readiness rejects stale mutations, foreign owners/executors and consumed tokens;
commit swaps without callbacks or destruction, retaining old pending storage in
the caller's token until later release. Already-playing SDL voices are separate
M04 owners and are not checkpointed by this API.

These are capture and preparation contracts. The actual windowed migration fence
still requires complete borrower/resource/scene/HUD/input/playing-audio adoption
and survivor readiness before activation. M02 remains unchecked and default
migration remains disabled; gameplay and match/postgame voice stay direct and
player-hosted. The [next production dependency](../../.local/runtime-receipt-frontier-20261019/next-adoption.md)
records the missing owners without treating inspection as completion.

Qualification uses one coherent private native build: six normal CTests, four
selected-owner ASan/UBSan/leak CTests and nine current-root GCC syntax checks pass.
An initial collocated-fixture timeout was reproduced by a controlled six-second
native traffic gap, then fixed by servicing actual direct heartbeats while shadow
work warms. Both variants retain the unchanged native tick/carried input through
the six-second regression and continue twelve gameplay ticks/direct voice packets.
Production timeout policy is unchanged. The rendered history test retains 258
completed entries (peak128), two exact native bindings and 381 frames; it does not
activate migration. Four CPU owners/fixture TUs are instrumented; generated native
interiors, rendered runtime and physical devices are outside sanitizer acceptance.
Evidence: [verification.json](../../.local/runtime-receipt-frontier-20261019/verification.json).
All recorded scratch/processes are absent after removing 35,095 generated files /
1,602,889,260 logical bytes; nine one-off helpers are retired separately. No binary
is installed or retained. The full ONLINE goal remains active, with M02 unchecked.

## Passive source resources for a future windowed adopter, 2026-10-19

Eligible direct TU1.5 migration clients now admit the source particle banks and
source model-light sections during startup, before renderer material residency
is frozen. Admission includes known weapon firing, ready and authored attachment
effects. FP and WORLD have different camera filters and are attempted separately;
an unsupported FP system must not hide a supported WORLD bank. Diagnostics keep
unsupported authored owners explicit. Native producers remain pointer-gated and
the client's CPU particle owner is not suppressed by passive loading.

Model-light admission consumes authored cache metadata/materials. It does not run
the original model builder, fill its 512-record native cache, advance any source
particle phase or call observer/view preparation against a sealed candidate.
Source particle resource counts and resident model-light section counts observe
actual owned containers; they do not advertise complete migration readiness.

Complete world adoption still must publish incoming world/camera/collision and
cleanup borrowers together with exact history, input and playing audio. Genuine
native TU1.5 ONLINE retains its existing source scheduler: software weapon/FP
callbacks must not be added merely because a client becomes host. Any genuinely
required native setter belongs before candidate sealing, since later mutation
would invalidate the accepted execution boundary. M02-M07 remain unchecked and
windowed migration stays disabled by default.

Qualification uses a coherent 15,920-file source cohort with all eight selected
owner hashes matching. The native build, two focused normal CTests and five GCC
C++20 syntax checks pass. The four-view direct ONLINE fixture independently
censuses the host cache's firing/ready/attachment banks before player admission;
every one of 257 accepted client receipts matches all 49 particle materials,
105 first-person systems and 127 world systems. Its 125 model-light sections and
58,342 mesh materials remain resident and stable; native particle/model producers
remain dormant. Exact history still captures 256 receipts, peak128, two native
cuts and 486 rendered frames. This exercises preparation, not migration activation.

The separate Backwash fixture censuses 40 distinct authored references including
one null and five non-effect tags, from 36 firing, two ready and two attachment
references. Supported-effect admission is repeated and preserves
resource counts and the exact native execution boundary. Its original 240-frame,
14-draw and particle/RNG/repeated-view controls also pass. No sanitizer run or
physical-device/WAN acceptance is claimed for this milestone. Whole-fixture peak
RSS is 7,707,204 KiB, including native worlds and the renderer, not an isolated
history budget. Evidence: [verification.json](../../.local/source-candidate-residency-20261019/verification.json).

All recorded private paths/processes are absent after removing 35,112 generated
files / 1,313,165,286 logical bytes (1,410,691,072 allocated). The private native
executable is hashed and removed, never installed; six one-off helpers are also
retired after retaining bounded evidence. Existing assets, references, launchers
and unrelated concurrent work are preserved. M02 remains unchecked and the full
ONLINE goal stays active. U01/D01 require MCC-only extraction and selection of
Vista ONLINE/clan assets, with U02 retaining real widescreen/clan dimensions.

## Exact client replica rebind, 2026-10-19

An authentic per-connection cut can now prepare both decoded client state and
the same display owner's scene/device/prop/collision checkpoint. The runtime
checks both before the presentation callback and publishes them before changing
the session endpoint. Old storage retires outside locked publication. Cancellation,
foreign retry, wrong proof and callback refusal preserve the later live state;
outgoing couch input survives publication, and only a real successor packet
creates new-generation receipt eligibility.

The native build, three normal CTests, two unsuppressed selected-owner
ASan/UBSan/leak CTests and eight GCC checks pass. Instrumentation covers four
core owners and two fixture TUs, not generated/native, renderer or dependency
interiors. All 23 selected hashes match the 15,936-file frozen cohort; 16 later
unrelated changes are preserved. Both private workspaces and all recorded helper
paths/processes are removed; no binary/capture is installed or retained.
Evidence: [verification.json](../../.local/client-replica-cut-20261019/verification.json).

This closes a missing client transaction, not M02 or M06. The existing fixture
shares a proof in-process; production must independently validate and pin an
exact locally accepted cut on every survivor. The
[next-step plan](../../.local/client-replica-cut-20261019/common-cut-next-step.md)
keeps common receipt association separate from candidate-election readiness
and complete presentation/input/audio preparation. Default windowed migration
remains disabled. Direct player-hosted gameplay/voice and MCC-only Vista
ONLINE/clan asset, widescreen and actual clan-screen requirements remain intact.

## Every-recipient native proof distribution, 2026-10-19

The production runtime now sends one paired authority chain to every accepted
nonhost physical peer; QoS host ranking does not filter proof recipients. Exact
authenticated recipient/context/full-seal acknowledgements complete a distinct
group identity, never an invented singleton peer. Multi-peer delivery waits for
the complete group; singleton two-cut delivery remains supported. Stream repair
discards the failed base while preserving only historical completed group/host
evidence. Receivers/workers can export their exact paired cut without stopping
native replay, with stale/foreign/unpaired rejection and independent admission/
view checks.

The native build, seven normal CTests and six GCC checks pass against a coherent
15,948-file cohort and twelve matching selected owners. The fanout fixture uses
two independently prepared native receivers and each client's own actual receipt;
both original takeover/input/direct-voice sequences remain. Paired export at 70
continues to 72 real phases and later terminal retention. This is local native
fixture evidence, not independent-PC/WAN or full foreground-owner acceptance.
All owned scratch/processes are absent; no native executable is installed or
retained. Full results, preserved initial failure and source audit:
[verification.json](../../.local/common-cut-fanout-20261019/verification.json).

Historical native validation cannot replace the
[pinned common-cut/foreground barrier](../../.local/common-cut-fanout-20261019/next-common-cut.md).
M06 remains open with complete presentation/input/playing-audio preparation and
actual windowed activation still required. Default migration remains disabled;
direct player-hosted gameplay/voice and MCC-derived Vista ONLINE/clan UI with
correct widescreen/real clan bounds remain unchanged.

## Common native/foreground recovery pins, 2026-10-19

Authority feature 512 negotiates a separate association barrier across the full
physical membership, excluding the current host from recovery recipients. The
host offers one exact context/seal/mask; each recipient pins its independently
validated native world/host and its own authenticated foreground receipt before
Associated. Commit requires every association plus explicit native-all ACKs.
Local Apply requires the full association matrix; host Release requires all
Applied, and local release retires the previous pin before Released. No next
offer can replace a round until every recipient has Released or Aborted.

Each receiving runtime retains its committed pair beside one pending pair.
Early foreground reservations retain at most two cuts, each bounded by ordinary
history capacity; sequence/body proof remains mandatory despite same-tick churn
or overflow. Pre-Commit rejection/Abort preserves the committed pair and drains
an outstanding asynchronous native export before acknowledging retirement.
Loss of the successor journal does not invalidate the held cut; native validation
timeout and exact association failure are separate repair signals. Publisher
Abort cancels its stream and may clear its last-native-ACK evidence; that slot
does not own the recipients' committed recovery pairs.

The frozen native build, seven unskipped normal CTests and nine exact-target GCC
syntax checks pass. The three-runtime fixture independently authenticates twelve
couch players on three physical peers, pins both recipients' exact cuts, captures
334 receipts per recipient through 128-entry history/reservation bounds, and
completes two released rounds around an explicit rejected/aborted round. Existing
native replay, takeover and direct-voice regressions remain included. Evidence:
[verification.json](../../.local/common-cut-pins-20261019/verification.json).

Qualification applies to the frozen cohort; concurrent root CMake changes keep
current-root delivery unqualified. Owned scratch/cache trees and processes are
gone, and the private native executable was removed without installation.

This qualifies headless native/CPU recovery-pin ownership only. It adds no full
renderer/HUD/input/playing-audio adopter, live activation, WAN or physical-device
acceptance, and makes no sanitizer claim. Complete M02-M07 readiness remains
open and default migration remains disabled. Gameplay and match/postgame voice
remain direct and player-hosted; MCC-only Vista ONLINE/clan assets, widescreen
layout and real clan-screen dimensions remain required.
