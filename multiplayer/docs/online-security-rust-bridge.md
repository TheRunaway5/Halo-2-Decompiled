# Rust voice framing: owned CXX integration

Implementation and Linux qualification, 2026-10-21. This advances S02/S03/S04/
S09/S12 of the [security-first goal](online-security-first-goal.md); those full
items remain open. The [S01 inventory](online-security-threat-model.md) records
the earlier source baseline and other receive paths still requiring migration.

## Real receive path

`OnlinePeerTransport` retains the authenticated physical endpoint for each
DTLS channel. `OnlineMatchSession::datagram` checks current session/epoch and
host-role policy, then calls
[decode_online_voice_frame](../../src/online_voice_protocol.cpp) for kind-6
match/postgame voice. That adapter calls the generated
[CXX interface](../../rust/online_protocol_bridge/src/lib.rs), which invokes the
[Rust wire reader](../../rust/online_protocol/src/lib.rs). The old production
C++ subtype/field/payload reader is removed. There is no failure fallback.

The return value is an owning C++ variant: preferences (with an optional
forwarded-source claim) or audio (source claim, sequence, radio, gains and owned
Opus bytes). The actual [session](../../src/online_match_session.cpp) retains
sender, couch-seat owner, membership, host-forwarding, receiver-local block
and voice-generation checks. Parsed fields do not manufacture authentication,
native readiness or callback authority. Other native/control packet families
and service-mediated party/clan voice are not migrated by this change.

## Lifetime, errors and threading

- The C++ span must describe live contiguous byte storage for the synchronous
  call. The real receive loop retains that vector until the call returns.
- CXX generates both sides from one schema. Its borrowed Rust slice never
  escapes. Rust owns the parsed payload; C++ copies it into its own vector
  before the temporary Rust Vec is retired by its Rust allocator. No borrowed
  payload or manually transmuted wire struct is published to gameplay.
- The parser crate forbids unsafe Rust. The separate bridge denies handwritten
  unsafe operations, with a scoped exception for the generated CXX schema/shims.
  No custom pointer dereference, native callback or World owner is exposed.
- Normal malformed input returns status and an empty optional. A Rust unwind
  is caught before the extern shim; OOM abort cannot be caught this way. A C++
  allocation exception follows the existing dispatch exception policy. The
  direct receive loop rejects it without retiring healthy room peers.
- Calls are stateless/reentrant; returned allocations belong to their caller.
  No source memory, SDL lock, device queue or native publication gate is held
  by this module. The session's existing mutex still protects its own state.

The new saturating `rejected_direct_dispatches()` diagnostic counts actual
exceptions at the direct receive/dispatch catch. It contains no payload or
account ID and grants no permission. Protocol modules that consume/reject
internally retain separate accounting; this is not a count of every rejected
packet family or evidence that a peer is cheating.

## Build and dependency contract

[OnlineProtocolRust.cmake](../../cmake/OnlineProtocolRust.cmake) adds a mandatory
dependency to `halo2_online_matchmaking`. The
[build helper](../../tools/build_online_protocol_rust.py) verifies Rust 1.96.0,
builds the locked staticlib and matching CXX 1.0.205 header in the CMake tree,
and derives the final native link arguments from `rustc --print=native-static-libs`.
Cargo output and generated headers remain inside that build owner. The pure
parser still has no third-party dependencies; the bridge lock adds 32 registry
dependencies, whose identities/checksums are recorded in
[dependency-pins.json](../../.local/online-security-bridge-20261021/dependency-pins.json).
Advisory/license/release review remains S13 work.

Linux validation uses the already installed `stable` alias only after checking
that the actual compiler is exactly 1.96.0, with auto-install disabled and locked
offline builds. At this initial voice milestone, Windows/MSVC, Windows/GNU,
macOS and universal-slice code paths had no platform execution or packaging
acceptance. The subsequent [native-request qualification](online-security-native-requests.md)
adds a Windows GNU bridge cross-build and Wine controls; physical Windows,
MSVC, macOS/universal and full release packaging remain open.
The generated/native CXX code is still an audited unsafe surface. See the
[CXX safety model](https://docs.rs/cxx/1.0.205/cxx/) and
[interop build requirements](https://cxx.rs/build/other.html).

## Qualification and limits

The [normal bridge CTest](../../.local/online-security-bridge-20261021/normal-retry1/report.json)
passes 12,355 differential frames against a test-only extraction of the former
framing rules, including single-byte mutations, short/trailing data, invalid
versions/sources/booleans, payload limits and exact float-bit handling. The test
also retires/mutates the caller's original packet and runs 1,200 independent
concurrent calls. The reference decoder is linked only into the test executable.
The [C++ ASan/UBSan bridge run](../../.local/online-security-bridge-20261021/sanitized/report.json)
passes the same control. This is mixed-language interface coverage, not a claim
that stable Rust code or every external native library is sanitizer-instrumented.

The [repository session build](../../.local/online-security-bridge-20261021/root-session-adversarial/report.json)
compiles and links the actual production `OnlineMatchSession` against the Rust
archive. Both selected CTests start/pass without skips. The retained
[session result](../../.local/online-security-bridge-20261021/root-session-adversarial/evidence/test-artifacts/peer-audio/results.json)
records four real local DTLS endpoints, sixteen admitted couch seats, direct
Opus/PCM through synthetic devices, two host-loss handoffs and unchanged identity/
media fences. Native cuts/role completion and accounts are explicit fixture
inputs; no rendered native world, physical controllers/headsets, Steam browser
login, independent-PC/WAN or full migration acceptance follows.

The [repository C++ ASan/UBSan session run](../../.local/online-security-bridge-20261021/root-session-sanitized/report.json)
also starts and passes both selected CTests without skips, including those
authenticated malicious-voice controls. Its retained link command and Cargo
build receipt record the sanitizer flags for the selected C++ consumers and
generated CXX code. This does not claim instrumentation of stable Rust or all
external native dependencies.

The authenticated malicious-peer controls require actual dispatch rejection
and a delivered valid control separately for a foreign-seat speaker claim and
non-finite gain. They do not infer rejection from an empty unreliable-channel
queue. Ordinary gameplay and decoded voice then continue through the existing
handoff controls. This covers two concrete negative paths, not every malicious
host/peer strategy. Coverage-guided fuzzing, full anti-replay/admission migration,
aggregate resource budgets, native containment and independent review remain open.

The initial CMake configure failure caused by a comma inside an implicit
`LINK_ONLY` expression is retained in
[normal/report.json](../../.local/online-security-bridge-20261021/normal/report.json).
The corrected generator expression uses CMake's `COMMA` expression; no compiler
or parser policy was weakened to pass. The
[final qualification receipt](../../.local/online-security-bridge-20261021/final-verification.json)
records source pins, actual retained results and disposable build cleanup.
No launcher/runtime was installed or backend
deployed, and no migration gate was enabled.
