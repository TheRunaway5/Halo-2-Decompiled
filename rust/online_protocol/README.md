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

Linux mixed-language differential and real authenticated session controls are
recorded in [the bridge qualification](../../multiplayer/docs/online-security-rust-bridge.md).
These bounded results do not close S02/S03/S04/S09/S12: complete authentication/
replay migration, other platforms, containment, fuzzing and release qualification
remain open. No installed runtime was replaced or migration gate enabled.

Run tools with `CARGO_TARGET_DIR` inside an owned `HALO2_TEST_SCRATCH`, using
`tools/run_test.py` for Cargo-only probes and `tools/run_tests.py` for eventual
CMake validation. Use `--locked --offline` once the exact dependencies are cached;
keep generated build output out of the repository and retire it after the run.

