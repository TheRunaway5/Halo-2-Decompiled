# Owned CXX bridge for the ONLINE protocol

This crate is the single native static-library interface to the
[unsafe-forbidden parser](../online_protocol/README.md). CXX and cxx-build are
pinned together at 1.0.205, with transitive versions/checksums in Cargo.lock.
Handwritten Rust is denied unsafe operations. The exceptions are the CXX-generated
schema/shims and a documented `Send` implementation for the uniquely owned native
snapshot byte bank. CXX and its generated/native dependency code remain in the S13
review surface. The bridge borrows packet bytes synchronously and returns owned
fields/Opus bytes and native request controls/profiles. These parser interfaces
carry no callbacks or native-world owners into Rust. The shared crate now also
contains a separate EOS SDK adapter with native callbacks/ownership; parser
qualification does not accept that transport or its unsafe dependency surface.

The EOS adapter registers only the fixed `eos_sdk_worker` sibling of the native
executable (`eos_sdk_worker.exe` on Windows). Packaging must stage this Rust
worker even when the licensed SDK is disabled. Missing or redirected siblings
fail registration; there is no environment/PATH override or game self-execution
fallback. The worker accepts only `--eos-sdk-worker` as its sole argument. Its
process main owns every SDK platform, SDK call and final SDK shutdown; the game
retains the existing Rust transport policy and explicit request/poll cadence.
Bounded versioned stdin/stdout IPC, caller deadlines, child invalidation and
kill/reap behavior remain shared with component qualification. Credentials use
private protocol frames, never arguments or diagnostic errors.
On Windows the worker uses the GUI subsystem to avoid opening a console for a
GUI parent; its standard handles still come from the parent's explicit pipes.
That Windows process/packaging behavior remains unvalidated on a Windows host.

Authority decoding additionally returns an opaque Rust owner of a flat value
tree. Its views stay with the immediate native materializer/hash call, and its
native allocation claims share a synchronized reservation budget. See
[archive qualification and limits](../../multiplayer/docs/online-security-authority-archive.md).

Malformed frames return typed failure status. Unexpected Rust unwinding is
caught before the extern shim; allocator/OOM abort is a separate limit. The
C++ adapter owns its returned vector; it cannot retain a borrowed Rust vector or
the caller's packet storage. Parsed data grants no sender or speaker authority.

The authority-transfer adapter also holds an opaque Rust `PeerPollCursor` and
records successful emissions through this bridge. Its scan order is a fixed
array; it adds no scheduling allocation per poll. Native migration activation
passes the authenticated physical membership and sixteen exact-cut preparation
slots to Rust's all-survivor guard. These coordination interfaces retain no
borrowed native state and do not certify complete native owner preparation.

The native host-agreement adapter also holds one opaque Rust `HostAgreement`.
The Rust protocol crate owns control-packet encoding/decoding and the complete
election/retry state machine. The generated interface returns fixed sixteen-peer
ready banks and owned packets. Native synchronization covers every borrowed
Rust reference; invalid wide native indices become an invalid sentinel before
conversion. Expected validation errors retain the native exception boundary,
and unexpected Rust unwinding is caught before the generated shim.

The common-cut adapter likewise owns one opaque Rust `CommonCutAgreement`.
Rust owns its nine control codecs and complete association/Abort/release state
machine. CXX returns owned packets and complete optional cut/state values.
Every borrowed Rust call is synchronized, including immutable context access;
wide invalid native peer indices cannot narrow into valid physical peers.
Native world and foreground owners remain outside this metadata interface.

The survivor-selection adapter owns an opaque Rust `SurvivorSelection` with
three-entry frozen inventories, exact transcript/selection state and bounded
owned controls. Rust owns the codec, unanimous selection and terminal-failure
policy. C++ synchronizes calls, maps native values and connects the explicit
Session generation fence; it has no second selection implementation. Neither
the bridge nor its metadata tests provide actual native inventory leases or
complete pre-election readiness.

The prospective-role helper returns Rust-checked predecessor/successor/seal
metadata. Its native adapter only converts fields and rejects wide indices
before the fixed-width boundary. It cannot create an election certificate or
authorize native publication. The actual private native owner and its retained
publication scope stay on their creating engine executor.

The snapshot stream adapter (`src/network/snapshot_stream.cpp`) owns one Rust
`SnapshotSender` per admitted client and one `SnapshotReceiver` per client
connection. Calls borrow the serialized body, float offsets and anchors
synchronously and return owned datagram payloads or reconstructed bodies.
Unexpected unwinding retires the sender (complete snapshots resume) or rebuilds
the receiver and requests a reset. `H2_NET_PROFILE` is read when a sender is
created.

`cmake/OnlineProtocolRust.cmake` builds this mandatory library and generated
header inside the selected CMake build tree. The build helper verifies the
actual Rust version, uses the locked manifest, reads rustc's actual native
link requirements and never links an rlib as a native archive. Supported-platform
target selection is implemented, including separate macOS slices. Windows GNU
cross-build/Wine component controls pass in the
[native-request qualification](../../multiplayer/docs/online-security-native-requests.md);
MSVC, physical Windows, macOS and full packaging acceptance remain pending.
The release runtime will link
the static library; it does not require an end-user Rust installation.

For local validation with an existing `stable` alias whose actual compiler is
exactly the pinned 1.96.0, use `-DHALO2_ONLINE_RUST_TOOLCHAIN=stable`. A compiler
version mismatch fails; the alias cannot select an arbitrary newer/older compiler.
Automatic toolchain installation is disabled. Rustup users can install the
explicit 1.96.0 toolchain before building. This setting never selects a C++
fallback or relaxes receive-path validation.

See [interface and qualification details](../../multiplayer/docs/online-security-rust-bridge.md).


The direct peer worker uses the typed `NativeMatchPeersIo` interface and an
opaque Rust worker. Native callbacks supply actual transport packets, sampled
links, native resource polls, history owners and IO under the actual generation
lock. Rust owns schedule/order, limits, framing, retry rotation and the Session
capability plan. Media sends recheck Rust's current-generation predicate while
the native lock remains held. The build stages `matchmaking_native_peers.hpp`
with the other generated/native headers; there is no C++ scheduling fallback.
[Qualification and remaining scope](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-peer-driver-rust-validation.json)
keep original-service, real-world/provider and installed-runtime proof distinct.


The source-player admission adapter holds one opaque Rust `PeerSourceAdmissions`
owner. Full-width native indices and the legacy owner sentinel cross as typed
fields; C++ input vectors are borrowed as slices and copied into the Rust owner.
Returned admission rows and packets are owned. Rust owns validation, immutable
identity/context binding, hashing, wire, retries and successor resumption. The
native mutex serializes calls; metadata creates no world or Ready. The
[source admission qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/source-admissions-rust-validation.json)
retains the exact C++ comparison and the corrected initial CXX input-container
link diagnostic, without a C++ policy fallback.


`NativePublicationIo` supplies typed actual observations and native operations
for Rust-owned Ready/proposal/selection-progress/successor-commit callers. Its
outer generation lock remains held, with no mutable Rust Session borrow across
callbacks. Full-width request host/proposer fields are checked before narrowing.
Native resources and row copies are prepared before a genuine optional commit;
refusal cannot publish a successor. The build stages the publication IO header
with the generated interface. [Qualification and remaining callers](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-publication-rust-validation.json)
keep metadata controls distinct from original service, real worlds/providers
and installed-runtime proof.

`NativePreparationIo` supplies actual request identity, inventory, selector,
authority/admission and driver observations to the complete Rust preparation
callers. The native generation lock spans each operation; retirement reservation
uses the same observed snapshot as its request. Raw full-width ticket self/host
indices are checked before conversion. The staged preparation header exposes
typed IO only; native weak ownership and genuine resource lifetimes remain
native. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-preparation-caller-rust-validation.json)
includes both caller cohorts and the exact import/public-test compile corrections.

`NativeExchangeIo` pins actual source/authority/quality owners while Rust runs
the complete source publication, capability, Common, authority publication and
candidate-view callers. The callback snapshot comes from the Rust Session under
the genuine generation lock. Source/authority publication preserves its unlocked
owner work; Common actions preserve their locked operation/Abort/progress order.
Raw peer and context host values retain all 32 bits. Actual accepted values,
archive buffers, native metadata/resource owners and lease lifetimes remain
native IO. The staged exchange header supplies the typed callback interface.
The world-command bridge delegates phase/role and selected-seal decisions to
the Rust Session instead of receiving a C++-chosen current/selected seal.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-exchange-callers-rust-validation.json)
keeps actual public Session byte/metadata controls distinct from native-world
preparation, original Live and installed delivery proof.

The authority ABI uses shared references to one synchronized Rust
`PeerAuthorityTransfer` owner. Raw 32-bit peer/host/window fields cross before
validation. Rust owns wire, hashes, state, pacing, expiry, delivery and native
acknowledgement policy. Native C++ converts actual buffers/results and preserves
the public invalid-argument exception category for validating calls. An immutable
returned byte bank is real storage, not proof of native preparation or a lease.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/peer-authority-transfer-rust-validation.json)
includes compiled linkage, actual public API/wire comparison and the preserved
invalid-argument ABI, separately from genuine world and installed delivery proof.

The snapshot ABI owns one synchronized Rust `PeerSnapshotCandidate`. Rust owns
all eleven packet codecs, hashes, protocol state, physical barriers, publication
reservation and capture/retirement decisions. `NativeSnapshotBuffer` keeps the
actual uniquely owned C++ vector bank; CXX moves its `UniquePtr` through Rust
without a second archive bank. Storage reads borrow synchronously and writes
require pinned exclusive access. Its scoped `Send` implementation records that
this vector has no thread affinity; no `Sync` implementation is added.
`NativeSnapshotLeaseIo` only reads actual private delivery identity or issues
and destroys the real move-only native lease. Those infallible storage-only
calls run under the Rust state mutex and retain no owner pointer or executor
callback. Refusal preserves the caller's lease and bytes; actual retirement
rechecks the observational preflight. Large hashes remain outside the mutex,
with exact completion checks and reservation through genuine bank destruction.
The build stages the native IO header.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/peer-snapshot-candidate-rust-validation.json)
records 515 Rust tests, seven staging checks, eighteen native checks before and
after migration and an exact 57,325-command public API/wire comparison. These
controls do not certify original Live, whole-world memory budgets, allocation
or timing performance, genuine providers or installed Linux delivery.

`NativeReceiveIo` connects production Rust dispatch to the actual Session mutex,
resource handles and agreement history. The interface carries no Rust Session
reference across callbacks: it reads the existing projection, Rust-derived
capability observations and actual native resources, then performs the operation
chosen by Rust. Locked survivor/Common/agreement paths borrow actual owners;
the four unlocked paths retain their genuine shared owners. Full-width source/
epoch values and checked native history indices cross without aliases. Native
RAII releases the mutex on exceptional unwinding. The staged receive header and
actual compiled Session object are included in the
[qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-receive-dispatch-rust-validation.json),
which records 521 Rust tests, nineteen final native checks and 110,810 matching
caller observations. The exact prior C++ caller is test-only. The known native
lifecycle banner is handled explicitly by the new fixture reader; that harness
correction changes no production policy. Genuine worlds/providers, original
Live and installed Linux delivery remain separate acceptance obligations.

The access interface returns Rust-selected transfer limits and immutable local/
physical seats, presence and continuity. Raw C++ owner operands cross before
legacy-sentinel normalization and validation. The successor DTO carries the
complete Rust-chosen limits; C++ has no feature-mask implementation. Snapshot
view IO supplies actual phase/capability observations and retains the selected
real owner. `NativePostgameIo` retains the actual audio owner, mutex, readiness,
time and frame buffers while Rust controls the complete caller order. No mutable
Rust Session reference spans callbacks. The inner Rust exchange retains its
phase check. The local-seat value is computed before worker startup and can be
read during actual generation-locked native commit without mutex re-entry.
The staged access header and
[qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-session-views-postgame-rust-validation.json)
record 529 Rust tests, twenty final native checks and 31,360 matching caller
cases, including complete frame fields/bytes and 31 declared IO refusals. Actual
worlds/providers, original Live/XHV and installed Linux delivery remain open.

`NativeMigrationIo` supplies actual agreement/QoS/selection observations under
the caller-held Session generation mutex and invokes existing Rust State
operations. Core Rust owns complete fence/start/candidate sequencing; the ABI
keeps new raw host operands wide until checked, preserves actual refusal text
and carries explicit candidate presence. It holds no Rust Session reference
across callbacks. The staged header, unchanged public Session header and actual
compiled object's three caller references are pinned in the
[qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-migration-callers-rust-validation.json).
All 538 Rust tests, seven staging checks, 21 final native checks and 18,110 exact
caller comparisons pass. A retained red/green control covers the initial
bridge's lost refusal detail. An unrelated new weapons fixture is separately
source-audited; no broad inventory identity, original Live, genuine-world/provider
or installed Linux delivery claim is made.

`NativeSessionApiIo` supplies genuine mutex, buffer, audio, worker and exception
IO for nine Rust API caller entries. The received `Vec<NativeSessionPacket>`
moves into an opaque wrapper without cloning its banks; first-packet bytes
borrow that independent returned value. No Rust Session reference spans native
callbacks. Native exception pointers rethrow the actual exception object after
Rust cleanup, preserving its type/message/identity, including audio generation
overflow. The fixture's deliberate discard control detects the different CXX
exception despite identical text; that switch is absent from production.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-session-api-rust-validation.json)
pins the staged header, unchanged public Session header, all nine configured
object references, 546 Rust tests, 22 final native checks and 6,902 complete
caller comparisons. Original Live, played worlds, genuine providers and installed
Linux delivery remain separate full-goal obligations.

`NativeAccountClientIo` carries actual generation mutex, clock, HTTP, identity,
entropy, EOS and wake/cancel operations to `matchmaking_account_client`. Rust
owns shared account/accepted-party controller state and decisions; no Rust data
guard spans an IO callback. The native worker transfers its held mutex into
capture and releases it before HTTP/provider work. The public presentation
snapshot excludes grants, subjects and join secrets. Native exception pointers
preserve original exception type, text and object identity on public commands.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-account-client-rust-validation.json)
includes real Curl cancellation, native defect controls and rendered menus;
the existing modern protocol remains distinct from original Xbox Live proof.

`NativeResultIo` exposes synchronous observations of the actual native match,
selected profile/name conversion, source rankings and numeric PCR writer to
`matchmaking_native_results`. The bridge validates ABI shapes and the safe core
owns admission and publication planning. Native exceptions preserve their real
object/type/message; no Session borrow or Rust state guard spans native IO.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-result-authority-rust-validation.json)
compares the exact captured modern C++ bodies and exercises both production
entries after a timed native team world. This is distinct from original Live
arbitration, all unshortened outcomes and delivered Linux input acceptance.

FrontendLobby retains native pregame controller state in safe Rust. The CXX
handle deep-clones state for native value semantics, returns command plans
before actual account IO, and returns UI effects before native menu reentry.
Native callbacks keep their genuine exception type/message/object identity.
[Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-rust-validation.json)
includes exact modern caller/input replays, production SDK linkage, scoped
native UI and wider functional comparisons. This does not certify original
Live, all original callers/outcomes or physical installed Linux input.

`ProgressFacts` supplies cached native observations to the safe progress policy;
`ProgressRow` returns source/slot/rank/speaker decisions without display-name
identity lookup. The tip clock is an opaque Rust owner with deep-copy native
value semantics. [Qualification](../../multiplayer/docs/evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json)
includes captured modern method comparisons and fresh unchanged-TU1.5 controls
through the actual CXX wrapper. Four broader frontend failures and full
original/delivery acceptance remain open.
