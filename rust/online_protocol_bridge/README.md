# Owned CXX bridge for the ONLINE protocol

This crate is the single native static-library interface to the
[unsafe-forbidden parser](../online_protocol/README.md). CXX and cxx-build are
pinned together at 1.0.205, with transitive versions/checksums in Cargo.lock.
Handwritten Rust is denied unsafe operations; the exception is the CXX-generated
schema/shims. CXX and its generated/native dependency code remain in the S13
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
