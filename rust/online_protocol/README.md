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
An agreement certificate still cannot supply native preparation or authorize
activation without the separate all-survivor and native-owner checks.

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

Linux mixed-language differential and real authenticated session controls are
recorded in [the bridge qualification](../../multiplayer/docs/online-security-rust-bridge.md).
These bounded results do not close S02/S03/S04/S09/S12: complete authentication/
replay migration, other platforms, containment, fuzzing and release qualification
remain open. No installed runtime was replaced or migration gate enabled.

Run tools with `CARGO_TARGET_DIR` inside an owned `HALO2_TEST_SCRATCH`, using
`tools/run_test.py` for Cargo-only probes and `tools/run_tests.py` for eventual
CMake validation. Use `--locked --offline` once the exact dependencies are cached;
keep generated build output out of the repository and retire it after the run.

