# ONLINE protocol Rust module

First implementation for the [security-first goal](../../multiplayer/docs/online-security-first-goal.md).
The toolchain is scoped to this module; it does not change the unrelated DSP
dependency's toolchain. Authority decompression now pins `miniz_oxide` 0.9.1
and its locked Adler dependency; the initial voice-only module had none.

`decode_voice` owns its parsed output and checks the existing native kind-6
voice wire format. Its result is explicitly **unauthenticated**. The caller's
expected native protocol version is mandatory; no version is inferred or
downgraded from incoming data. Speaker/physical-seat permissions, replay and
generation checks still require the real authenticated session. This parser
does not decode Opus, touch devices or publish native state.

The match/postgame C++ voice receive branch now calls this parser through the
[separate CXX bridge crate](../online_protocol_bridge/README.md) and the mandatory
CMake library. It consumes owned fields and retains its existing sender/seat/
host-role, mute/block and delivery-generation checks. Other packet families and
party/clan voice still have separate migration work. No legacy voice decoder
is used as a fallback for rejected frames.

`native_requests::decode_request` also decodes complete native host-bound joins,
one-to-four-seat inputs, leaves and configuration queries. The shared CXX bridge
returns owned profile/control fields to the actual host and saved-input checks;
there is no legacy request-decoder fallback. Native admission, live cookie/seat
and source object/action semantics remain with their C++ owners. See
[native request qualification](../../multiplayer/docs/online-security-native-requests.md).
Native snapshot/state bodies remain separate migration work.

The authority-archive grammar and decompression now also use an owned, flat Rust
tree; [qualification and remaining native-schema/containment work](../../multiplayer/docs/online-security-authority-archive.md)
are tracked separately. Native snapshot bodies, delta/replay formats and the
remaining session protocols still need migration.

`peer_pacing::PeerPollCursor` owns fair physical-peer scan order under the
authority sender's shared byte/packet budget. Only a successfully emitted packet
advances the cursor; an unaffordable scan cannot repeatedly favor one peer.
The native transfer adapter retains its existing admission, token bucket, wire,
ACK, retry and expiry owners. The cursor returns a fixed sixteen-slot array.

`migration_preparation::all_survivors_prepared` requires the identical cut seal
from every fixed physical survivor before native session activation. It checks
the exact predecessor-minus-host membership and all cut fields. This metadata
barrier does not create a native preparation proof or certify renderer, input
or audio owners. Complete pre-election preparation remains separately gated.
See the [EOS completion evidence](../../multiplayer/docs/eos-completion-goal-20261022.md).

`host_agreement::Agreement` owns the host/cut election state machine and its
eight bounded control-packet layouts. Fixed physical membership, promises,
accepted-value recovery, retries, monotonic clocks and generation fencing now
run in Rust. The native `PeerHostAgreement` class keeps synchronization and
typed engine conversions; it has no second election or codec implementation.
Wire version 2 requires unanimous acceptance by the fixed survivor electorate
and locks each honest acceptor's first value across higher ballots. Pairwise
equivocation can block migration, but cannot split honest successor decisions.
The reported majority remains the promise/silence threshold, not the decision
threshold. No timeout removes a withholding or missing survivor.
An agreement certificate still cannot supply native preparation or authorize
activation without the separate all-survivor and native-owner checks.

`lan_auth` owns ephemeral X25519 keys, HMAC-SHA256 packet authentication and
the 1,024-packet replay window for raw-LAN traffic. Protocol107 requires the
host key from an independent trusted exchange before any first frame, query
fragment or rejection can be accepted. Its KDF binds role-ordered keys,
protocol, nonce, content identity and declared destination. Discovery and
first datagrams cannot select a host identity. See [raw LAN host keys](../../multiplayer/docs/raw-lan-host-keys.md).
The pinned `x25519-dalek`, `hmac`, `getrandom` and `sha2` dependencies
are recorded in each consumer's Cargo lock. C++ owns socket I/O only.

`security_policy` owns network replay capability admission, exact Steam
revocation acknowledgement, directory/player capacity policy, EOS principal
admission and the versioned persistable postgame projection. CXX serves native
callers; the separate `online_security_ffi` ABI serves Python SQL/async owners.
See the [October 23 remediation record](../../multiplayer/docs/security-scan-remediation-20261023.md).

`common_cut::Agreement` owns the nine bounded common-cut control layouts,
exact physical membership, association/apply/release and Abort barriers, and
paced retries. Native callers explicitly report completed owner operations;
receiving a control never fabricates a native preparation or retirement. The
native `PeerCommonCutAgreement` class synchronizes and converts values for this
single Rust implementation. The original wrapping retry cursor, full-width
generation/seal fields and receive mutation order remain part of the contract.

`survivor_selection::Agreement` selects one exact cut from independently
authenticated frozen inventories for every fixed survivor. Its bounded kind-13
codec and pinned `sha2` 0.10.9 transcript bind the full generation, leases and
all entries; an Accept never supplies a missing inventory. At least one actual
irreversible Common phase observation must support the shared selected seal.
The one attempt fails frozen on conflict/invalidation and never creates a
native lease, freeze, readiness or host decision. Native preparation and the
opt-in Session capability boundary remain separate caller obligations.

`prospective_role::derive` checks the exact predecessor, selected full seal and
local physical survivor, then derives the possible self-host successor with
checked epoch/revision arithmetic. It returns metadata only. The native engine
can prepare camera, departure and lens changes privately before an election;
only a matching independently learned certificate can later bind that owner.
The descriptor contains no vote, Ready value, native lease or authorization.

`netcode` owns the native snapshot transport (protocol 105): acknowledged
lossless deltas of the serialized state, presentation-only quantized pose
replication with per-body priorities, a per-client budget using the retail
channel constants, fragmentation and reassembly. Native code keeps the snapshot
schema and its validation. `lan_auth` also emits compact `LNA2` frames after
the peer proves the key. See [bandwidth-bounded netcode](../../docs/NETCODE_BANDWIDTH.md).

Linux mixed-language differential and real authenticated session controls are
recorded in [the bridge qualification](../../multiplayer/docs/online-security-rust-bridge.md).
These bounded results do not close S02/S03/S04/S09/S12: complete authentication/
replay migration, other platforms, containment, fuzzing and release qualification
remain open. No installed runtime was replaced or migration gate enabled.

Run tools with `CARGO_TARGET_DIR` inside an owned `HALO2_TEST_SCRATCH`, using
`tools/run_test.py` for Cargo-only probes and `tools/run_tests.py` for eventual
CMake validation. Use `--locked --offline` once the exact dependencies are cached;
keep generated build output out of the repository and retire it after the run.



`matchmaking_native_peers::Worker` owns direct peer scheduling, the receive
budget, epoch framing, media/control ordering, poll budgets and wrapping
retained-generation rotation. The native Session's Rust plan owns capability
barriers against an exact observed authority context and every fixed physical
member. Typed native IO retains actual owners and rechecks media generation
under its native lock before send; Rust metadata creates no native Ready or
world. The [driver qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-peer-driver-rust-validation.json)
records 457 Rust tests, the same 13 C++/Rust native checks, seven staging controls
and the final native clients against real Rust room IO. That milestone left
source-player exchange separate; the next entry records its migration. Native
readiness/publication callers, original service/history, natural gameplay and
delivered-client gates remain separate work.


`peer_source_admissions::Exchange` owns immutable native handle/identity binding,
exact account/couch/guest and physical context checks, Offer/Ack codec and hash,
receipt/retry/cursor state and retained roster resumption after committed host
loss. The C++ class serializes one Rust owner and converts typed values. A
metadata receipt never certifies native preparation. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/source-admissions-rust-validation.json)
records 465 Rust tests, fifteen native checks and an exact 25,054-command public
API comparison with the preserved modern C++ source. This does not recover
original Live wire or qualify a played world, provider or installed runtime.


`matchmaking_native_publication` owns legacy Ready publication, proposals,
selection progress and the complete successor commit caller. Typed native IO
supplies actual admission/ready/device/retirement observations and constructs
successor owners before the optional native commit. The actual outer generation
mutex spans the entire operation; resource publication follows commit success.
No metadata supplies real leases, Ready or a world. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-publication-rust-validation.json)
records 474 Rust tests, seven staging checks and fifteen native checks against
both the preserved C++ caller and Rust caller. Other publication/view callers,
genuine original/natural worlds/providers and full
delivered-client acceptance remain separate obligations.

`matchmaking_native_preparation` owns request/completion, inventory freeze,
invalidation and retirement-view orchestration. Rust derives preparation
authorization from actual Session observations. Typed native IO retains genuine
weak owner identity, leases/resources and the outer generation lock; one cached
observation spans a request and its retirement reservation. Metadata creates no
lease, Ready or world. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-preparation-caller-rust-validation.json)
records 484 Rust tests, seven staging checks and fifteen native checks with both
the preserved C++ and Rust callers. Original service, genuine worlds/providers
and complete delivered-client gates remain open.

`matchmaking_native_exchange` owns the complete source-player publication/view,
authority full/pipelined publication, capability, Common-cut action/view and host
candidate-view callers. Session facts come from the authoritative Rust owner;
actual source/authority/quality owners are pinned under the generation mutex and
used outside it as before. Common actions keep the outer lock through the
actual operation and Abort/progress, including a refused leaf. Native IO retains
the actual returned values and buffers. Full-width peer/context values cross
without narrowing. `State::queue_world_request` also owns native world-command
phase/role admission and chooses the current epoch or exact retained selected
seal. None of these metadata operations prepares a native world or grants Ready.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-exchange-callers-rust-validation.json)
records 495 Rust tests and the same fifteen native checks with both callers,
including actual public authority byte delivery and explicit fixture validation.
The reliable authority/snapshot owners, receive dispatch, remaining views,
original recovery and complete genuine-world/provider/delivery gates remain open.

`peer_authority_transfer::Transfer` owns the complete reliable authority exchange:
wire/hash, capability negotiation, sender/receiver windows, pacing and persistent
scan cursor, retries/deadlines, pipeline retirement, byte delivery and explicit
native-validation receipts. One Rust mutex synchronizes state; large publication
and completion hashes run outside it, with exact context/seal checks afterward.
Byte/checksum completion cannot create native Ready. Typed C++ integration keeps
the existing public API and genuine immutable returned byte buffers; its
validating calls preserve `std::invalid_argument`. Buffer conversion allocation
identity/performance and genuine native-world resource budgets are separate
acceptance obligations.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/peer-authority-transfer-rust-validation.json)
records 506 Rust tests, the final sixteen native checks and a 37,731-command
preserved-C++/Rust comparison, including a red/green public exception-category
control. That checkpoint left the snapshot owner and remaining receive/view/
postgame policy, original recovery and full genuine-world/provider/delivery
gates open.

`peer_snapshot_candidate::Candidate` now owns the complete modern snapshot
delivery protocol: all eleven codecs, SHA, exact token/context and physical
barriers, batch publication, retry/window/cursor state, clock telemetry, native
bank accounting and explicit capture/release/abort/retirement. One Rust mutex
owns protocol state. Publication and completion hash outside it, recheck exact
state and retain their reservation until the actual native byte banks retire.
The generic buffer interface supplies byte storage; production banks and
private move-only delivery identities remain genuine native IO. Rust reads
their witness and destroys an accepted lease under its mutex before clearing
the outstanding-byte state. A preflight observation never reserves retirement.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/peer-snapshot-candidate-rust-validation.json)
records 515 Rust tests, seven staging checks, eighteen native checks with each
owner and a 57,325-command exact preserved-C++/Rust public API/wire comparison.
That checkpoint left Session receive/phase/epoch/history, remaining views/
postgame decisions, original recovery and complete genuine-world/provider/
delivery gates open.

`matchmaking_native_receive` owns the complete Session receive caller, including
seven ordered protocol recognizers, recognized-frame non-fallthrough, phase/
capability/epoch gates, first exact historical agreement selection and all
Abort/progress/fence/observation/refresh ordering. Native IO retains the actual
generation lock and resource handles; Rust chooses locked operations versus
pinning an owner for unlocked context/receive work. No mutable Rust Session
borrow spans callbacks. Native RAII covers unexpected unwinding, and ordinary
IO failures still unlock. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-receive-dispatch-rust-validation.json)
records 521 Rust tests, seven staging checks, nineteen final native checks and
110,810 preserved-C++/production-Rust caller comparisons, including 61 declared
IO refusals. That checkpoint left feature-limit construction, remaining views/
postgame decisions, original recovery and all genuine-world/provider/installed-
delivery gates open.

`matchmaking_native_access` owns initial/successor transfer limits, raw legacy
owner normalization, local/physical seats, presence/continuity/snapshot views and
the complete outer postgame voice caller. Rust chooses device/phase/prefs/frame/
fence/exchange/delivery order and control-worker postgame pumping. Real native
owners and the generation mutex remain IO; the inner Rust exchange rechecks
phase after fencing. Caller frames survive a refused fence until unlock. The
immutable local-seat projection is computed by Rust before worker startup, so
its existing getter remains safe inside a generation-locked native callback.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-session-views-postgame-rust-validation.json)
records 529 Rust tests, seven staging checks, twenty final native checks and
31,360 matching preserved-C++/Rust caller comparisons. Remaining native caller/
API ownership, original recovery and complete genuine-world/provider/installed-
delivery acceptance remain open.

`matchmaking_native_migration` owns complete Session fence, selection-start and
candidate caller sequencing, including phase/context short circuits, actual
agreement/QoS/selection sampling order, forced-fence behavior and result bounds.
Native callbacks retain real resources and the generation lock. No Rust Session
borrow spans reentrant callbacks, raw context hosts are checked before narrowing,
and actual refusal details survive the ABI. The configured Session object calls
all three new Rust entries; its public header and mutex-free local-seat getter
remain intact. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-migration-callers-rust-validation.json)
records 538 Rust tests, seven staging checks, 21 final native checks and 18,110
matching caller commands with exact error text. It includes the red/green control
for refusal text lost by the initial bridge. Qualified Rust crate inputs match;
one unrelated weapons fixture added between freezes is separately source-audited,
without a whole-inventory identity claim. Remaining API/production owner closure,
original recovery, genuine worlds/providers and installed Linux delivery remain
open.

`matchmaking_native_api` owns complete Session packet/voice/preferences/collection,
retained-audio and close callers, plus standalone audio sync/fence order. Rust
chooses lock/fence/readiness/output/shutdown sequencing and actual owner
acceptance. The received container moves without bank clones and retires before
unlock; receive failure preserves the existing output order. Native IO keeps
the genuine mutex/devices/workers and exception objects, with no Rust Session
borrow across callbacks. Close releases its audio pin before stopping/joining
outside the generation lock. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-session-api-rust-validation.json)
records 546 Rust tests, seven staging checks, 22 final native checks and 6,902
exact caller comparisons, including actual exception identity and a negative
control for its loss. All 3,430 qualified input identities match. Original
recovery, complete production route closure, genuine worlds/providers and
installed Linux delivery remain open.

`matchmaking_account_client` owns the complete accepted-party/account client
controller: strict state parsing, private grants/subjects, generation fences,
command IDs/replay, privacy/countdown, polling/profile/EOS cadence, Steam fetch,
activity/voice lifetime and follower/PCR return. Native IO keeps real threads,
mutexes, Curl, clocks, entropy and provider resources. Known late/rejected grants
retire without stale publication. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-account-client-rust-validation.json)
records 565 Rust tests, 19 new client controls, 7,775 modern C++/Rust snapshot cases
and actual rendered SDL playlist/Start flow against Rust HTTP. Original Live,
genuine providers, full played-world scope and installed delivery remain open.

`matchmaking_native_results` owns native ONLINE result publication/admission
policy: completion, exact 304-byte settings, roster/identity/team history,
placements, signed scores, numeric statistics and accuracy words. Native IO
retains actual gameplay ranks, selected profiles and the numeric PCR writer.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-result-authority-rust-validation.json)
records 578 Rust tests, 4,748 exact modern C++/Rust commands and the production
publisher/validator in a bounded timed native team match. A separate combat
accuracy world fails before publication identically on both implementations;
full original Live, normal played-world scope and installed delivery stay open.

matchmaking_frontend_lobby owns native pregame countdown/consumed-serial and
Leave state plus Start eligibility, consent and Start/Back/X/Leave decisions.
Typed facts produce immutable command plans and UI effects; no Rust borrow
spans native IO or menu reentry. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-rust-validation.json)
records 591 Rust tests, 4,387 modern C++/Rust operations, fresh unchanged-XBE
countdown controls and actual rendered playlist/Start flow against Rust HTTP.
Wider functional failures, the remaining search/launch controller, original Live,
full played-world scope and installed delivery remain open.

`matchmaking_frontend_search` owns source frontend search/consent, playlist and
profile-generation intent, party search notices, progress state and native
launch admission. C++ supplies typed native IO and rendering/input effects;
Rust guards end before callbacks and menu reentry. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-search-rust-validation.json)
records 605 Rust tests, 5,563 modern C++/Rust operations, 532 variant controls and
actual rendered playlist/Start flow against Rust HTTP. Four wider frontend
failures, remaining native matching policy, original service/full played-world
scope and installed delivery remain open.

The safe `matchmaking_frontend_progress` module owns progress panes/titles,
roster privacy/placeholders and original CRT sort, admitted-slot rank/voice
routing, cancel/leave rules and widget/tip kernels. Native text/assets and actual
voice observations remain IO. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json)
records 613 Rust passes,2,087 fresh original controls and 14 focused native
passes. Four broader failures, complete original producers and installed
delivery acceptance remain open.

`matchmaking_frontend_presentation` owns native playlist/progress text policy
through opaque-byte facts and actual locale/asset IO. The next-playlist Start
path now creates a fresh Rust countdown binding after a consumed prior search,
while retaining its completed serial for late observers until commit.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-presentation-rust-validation.json) records 620 Rust passes,
1,752 captured modern C++/Rust cases and 18 passing native checks. Legacy clan
sampling, MCC party emblem binding, original producers and full installed
G01-G12 delivery acceptance remain open.

`matchmaking_frontend_party` owns accepted-party seat projection, voice/status
columns, notification sounds and invitation-notice selection over typed actual
profile/login/seat IO. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-party-policy-rust-validation.json)
records 626 Rust passes, 3,105 exact modern C++/Rust cases with 14 genuine native
exception controls, and 22 passing native checks including actual SDL/Vulkan
playlist/Start and one-/two-player party emblem pixels. Legacy artwork and
selected animated-page audits now pass. Remaining party action/privacy/invite
routing/state, original producers, full played-world scope, genuine providers
and installed G01-G12 delivery acceptance remain open.

`matchmaking_frontend_party_flow` owns native party action/target, invitation
tab/dismissal/version and privacy acknowledgment state. All ten commands and
retained navigation, local-player and voice action choices run in safe Rust;
actual workers/devices and primitive menu effects remain typed native IO.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-party-flow-rust-validation.json)
records 641 Rust passes, 5,439 modern caller cases with 54 genuine exception
observations, native callback reentry/copy independence, and 23 passing native
checks including actual SDL/Vulkan privacy/playlist/Start and party pixels.
Party menu/text, lobby kernels and remaining account/voice intent policy,
original producers, full played-world scope, genuine providers and installed
G01-G12 delivery remain open.

`matchmaking_frontend_lobby_view` owns source-backed lobby bindings, Start
eligibility, icon/help/count and authored quick-option selection in safe Rust.
C++ supplies cache records and the actual gameplay default descriptor getter;
its old selection bodies are removed. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-view-rust-validation.json)
records 653 Rust passes, 5,927 exact modern caller cases, fourteen preserved native
exceptions, 2,389 original fixture rows including a separately executed unknown
choice, and 27 passing native checks. This fresh original proof is pinned retail
Xbox, with declared oracle query/widget/formatting services. Party menu/text and
lobby query producers, remaining account/voice policy, TU1.5/full original owner
recovery, natural played-world scope, genuine providers and installed G01-G12
delivery remain open.

`matchmaking_frontend_party_view` owns party menu rows, invitation/member
target presentation and party/lobby text selection in safe Rust. The existing
Rust party controller supplies its state directly; C++ supplies cached facts,
ordered native resource/query IO and primitive frame writes. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-party-view-rust-validation.json)
records 668 Rust passes, 2,828 exact modern caller cases, 2,763 native reads and
same-controller reentries, 78 preserved native exceptions and 28 passing native
checks. This is a modern caller migration, with no new original Live producer
execution or installed-client claim. Lobby query/roster and remaining account/voice
policy, original recovery, natural played-world scope, genuine providers and
G01-G12 delivery remain open.

`matchmaking_frontend_lobby_query` owns the whole native lobby view caller,
including ONLINE roster/status/speaker composition and local/campaign/System Link
map, settings, team and readiness queries. Native code supplies typed profile,
gameplay and resource IO and opaque rendering values. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-query-rust-validation.json)
records 686 Rust passes, 1,055 exact modern caller cases, 23,520 native reads and
Rust reentries, 542 preserved exceptions and 33 passing native checks. Fresh
retail oracles regenerate the unchanged readiness/team fixtures; nine additional
original member-team cases establish the explicit0..15 bound in Rust. These are
source-kernel/modern-caller proofs with declared services, not complete original
Live producer, TU1.5, natural played-world or installed G01-G12 delivery proof.
Remaining native profile/account/team/postgame/voice owners and all full-scope
criteria remain open.

`matchmaking_frontend_account_flow` owns the full native account observer and
pending friend-join continuation, including revision admission, notifications,
join/departure navigation, party voice intent and retained-frame identity focus.
The existing Rust party controller owns the join serial, account name and two
voice flags; their former C++ fields are removed. Native code supplies worker,
device and primitive frame IO. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-account-flow-rust-validation.json)
records 702 Rust passes, 1,547 exact modern caller cases, 8,804 native callback
observations and Rust reentries, 72 preserved exceptions and 34 passing native
checks including SDL/Vulkan playlist/Start/privacy and party pixels. Existing
original-bound fixtures still pass; this adds no original Live or TU1.5 producer
claim. Remaining native account/friends/clan/team/postgame owners, full original
recovery, natural played-world scope, genuine providers and installed G01-G12
delivery remain open.

`matchmaking_frontend_friends` owns the complete native friends caller,
including provider/availability selection, native/social party and invitation
actions, menu/text/column choices and roster refresh/focus. Friend/activity
targets live in the existing Rust party controller; their C++ fields are removed.
Native code supplies typed provider/localization/navigation IO and opaque
snapshot storage. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-friends-flow-rust-validation.json)
records 722 Rust passes, 6,025 modern caller comparisons, 4,993 callback-boundary
observations and Rust reentries, 106 preserved native exceptions and 35 passing
native checks including SDL/Vulkan playlist/Start/privacy and party pixels.
The former undefined C++ INT_MIN negations are excluded from equivalence and
separately controlled in Rust. This adds no original Live/TU1.5 provider claim.
Remaining native account/voice/clan/team/postgame owners, full original recovery,
natural played-world scope, genuine providers and installed G01-G12 delivery
remain open.

`matchmaking_frontend_clans` owns the complete modern clan caller, account/voice
binding order and push-to-talk gates. Clan target, kind, page and confirmation
state live in the existing Rust party controller; their C++ fields are removed.
The exact auxiliary clan-page branch also reaches Rust. Typed native code
supplies lifetimes and provider/device/frame/resource IO.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-clans-flow-rust-validation.json) records 749 Rust passes, 6,876 modern caller
comparisons, 5,071 callback observations/reentries, 88 native exception controls,
12 focused CMake checks and actual compilation/import checks for all four
affected production C++ units. The separate 36-check native HTTP/SDL/Vulkan/
lobby cohort was interrupted without a completed result and remains pending.
Undefined C++ lookup/page inputs are excluded from equivalence; safe Rust
controls are separate. Defined unknown scoped-enum values remain covered.
Original Xbox/Live/historical recovery, remaining native policy owners, genuine
providers, natural played worlds and installed G01-G12 delivery stay open.

`matchmaking_frontend_couch` owns the complete modern couch caller, controller
slot/join/finish/leave and primary identity callers, exact couch/join Back
branches and profile-list predicates. Six former native state fields now live
in the existing Rust party controller; search reads its actual couch revision.
Native code supplies actual provider lifetimes and device/profile/frame IO.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-couch-flow-rust-validation.json) records 787 Rust passes, 2,638 captured caller
comparisons, 4,507 callback observations/reentries, 108 native exception
controls, 13 focused CMake checks and five production object/import checks.
A reproduced early-frame-read mismatch is fixed; the minimal C++/Rust probe
fails before and passes after. Undefined C++ producer/index cases have separate
safe Rust controls. Whole native runtime (37 checks), original Xbox/Live/historical
recovery, remaining non-Rust owners, natural played worlds and installed
G01-G12 delivery remain open. No selected runtime or service changed.
