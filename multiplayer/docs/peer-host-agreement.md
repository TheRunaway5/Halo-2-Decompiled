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
