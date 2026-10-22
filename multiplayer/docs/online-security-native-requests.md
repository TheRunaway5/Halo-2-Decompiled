# Rust native host requests

S04/S07/S12 implementation, 2026-10-21. These full checklist items remain open.
The [earlier inventory](online-security-threat-model.md) is the historical
pre-migration source baseline. This change extends the
[existing voice bridge](online-security-rust-bridge.md) to native host requests.

## Production boundary

The safe [Rust request reader](../../rust/online_protocol/src/native_requests.rs)
fully consumes the existing native host-bound packet grammar. Local policy
supplies the protocol version; incoming data cannot negotiate a downgrade.

| Native request | Owned result and wire validation | Retained native authority |
| --- | --- | --- |
| Hello, kind 1 | Nonce, fingerprint, one to four signed team choices, exact profile-presence bits and optional complete 144-byte profiles | Current map/configuration identity, queue-admitted physical peer and seats, team policy, profile/name semantics, available capacity and transactional join |
| Input, kind 2 | Nonce/cookie, sequence, one to four complete control records; finite/ranged movement/look, legal flags, signed zoom, durations, ten serials and checked vehicle-intent claims | Live endpoint/nonce/cookie, exact admitted seat count, packet/press ordering, legal native objects and source action ownership |
| Leave, kind 4 | Exact nonce/cookie request, no trailing bytes | Current peer connection and actual four-seat departure lifecycle |
| Session query, kind 6 | Exact nonce request, no trailing bytes | Read-only configuration service; no player admission or world tick |

This namespace's kind 6 is a query. Match/postgame voice has a separate kind-6
grammar selected by its actual session route. The new parser does not infer
which namespace applies from a shared kind number.

[The CXX schema](../../rust/online_protocol_bridge/src/lib.rs) returns typed
owned fields, profile vectors, fixed control arrays and intent values. The
[C++ adapter](../../src/native_request_protocol.cpp) copies profile bytes into
owned fixed arrays before the Rust result retires. It does not expose pointers,
callbacks or an unchecked remainder for a second legacy request decoder.
The bridge catches Rust unwinding; allocator abort and native failures retain
the limits documented in the voice bridge qualification.

Three real [native call sites](../../src/multiplayer_network.cpp) use this
decoder: the loading/configuration worker, `LanHost::receive`, and retained
latest-input verification during native promotion. The old production
`decode_control` and `decode_vehicle_intent` byte readers are removed. Native
controls are adapted from typed fields only. Both raw UDP and matched transport
reach the same host implementation. Endpoint, cookie and role checks remain
necessary; parsing does not upgrade raw UDP to authenticated internet transport.

## Qualification and outstanding work

All ten Rust test groups pass and the actual pinned bridge passes strict
Clippy. The [normal mixed-language build](../../.local/online-security-native-20261021/bridge-normal/report.json)
and [C++ ASan/UBSan build](../../.local/online-security-native-20261021/bridge-sanitized/report.json)
each start/pass both requested CTests without skips. The native comparison
uses a test-only extraction of the removed reader plus the original host's
one-to-four seat grammar: **337,231 differential frames**, profile-buffer
retirement and **1,200 concurrent calls**. Existing voice coverage still passes
12,355 framing comparisons and 1,200 concurrent calls. These are deterministic
controls, not coverage-guided fuzzing or complete security acceptance.

The same source also passes the [Windows GNU cross-build and Wine controls](../../.local/online-security-native-20261021/windows-gnu/report.json).
The actual Rust target is `x86_64-pc-windows-gnu`, with matching MinGW C++ and
statically linked test executables. Both CTests start/pass under Wine with no
skips. This qualifies this bridge toolchain path, not MSVC, physical Windows,
full-game packaging or release mitigations. The temporary Wine prefix, server
and executables belong to the enclosing managed scratch owner.

The [repository-native qualification](../../.local/online-security-native-20261021/root-normal/report.json)
starts and passes all eight selected CTests without skips. Its dedicated
regression sends malformed joins, early and late invalid control fields, wrong
cookies, replayed sequences and malformed leaves through the actual
`LanHost`/`LanClient` matched endpoint interface. It observes unchanged native
world tick/healthy four-seat state after each rejection, successful valid
controls afterward and a valid complete departure. This
fixture declares its endpoint/account admission; it does not establish DTLS,
rendered native-world, physical-device or WAN acceptance. Separate existing
UDP vehicle input, discovery/loading and startup controls also pass. The
direct DTLS/synthetic audio session passes its authenticated voice rejection
and two host-loss controls with no master gameplay relay. That fixture declares
its native cut/role completion; it does not prove full synchronized-world
promotion. The real saved-input promotion call site is migrated in source, but
these results do not independently accept that complete native promotion path.

Native snapshot/state/profile bodies, fragment/archive assembly, source-world
schema and replay/delta decoding, control/signaling formats and migration
protocols still have further migration work. Profile bytes here remain data
subject to native semantic checks. Containment, aggregate resource budgets,
cryptographic admission for alternate paths, all-platform packaging, independent
review and installed-runtime acceptance remain open. No migration gate or
master gameplay/media relay is enabled by this implementation.

The [source pins](../../.local/online-security-native-20261021/source-pins.json)
declare the inspected inputs. During repository compilation, an unrelated
`EXCLUDE_FROM_ALL` export-tool addition changed the root CMake file. The
[drift receipt](../../.local/online-security-native-20261021/concurrent-cmake-drift.json)
preserves the exact qualification input, reconstructs it against its recorded
hash and distinguishes the concurrent addition. The root build later
auto-regenerated its CMake configuration; both observed configurations are
retained. A later post-test addition registers unrelated FOV regressions; its
separate source copy is retained without attributing it to the earlier tests.
The selected request/bridge sources remain unchanged; this is not a claim that
the entire dirty checkout was frozen or globally qualified.

The [final verification receipt](../../.local/online-security-native-20261021/final-verification.json)
records the retained results, source scope and scratch removal. Full native
consumer sanitizer coverage remains open; the sanitizer runs here cover the
selected C++ bridge/test and generated CXX code, not stable Rust or every native
dependency. No launcher/runtime was installed and no backend was deployed.
