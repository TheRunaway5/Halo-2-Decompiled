# ONLINE security-first goal: Rust and hostile-peer protection

Requested 2026-10-21. Complete this first, then resume the existing
[full ONLINE goal](online-completion-goal.md). The authoritative checkboxes are
at the top of that document; the matching IDs below define acceptance.

## Objective and invariants

Reduce remote-code-execution exposure from untrusted network data, reject
forged/replayed/unauthorized messages and contain malicious-peer resource use.
Use safe Rust for network-facing parsing and protocol state, with a small
reviewed interface to the existing source-faithful C++ runtime. Rust alone does
not prove security, make a player host trustworthy or remove native dependency
risks. Authentication identifies a sender; it does not make their input safe.

Preserve direct player-hosted gameplay and match/postgame voice. The master
remains responsible for accounts, discovery, matchmaking, authenticated setup
and post-match reports; this goal adds no gameplay/media relay or mandatory
mid-match master arbitration. Preserve Steam-primary identity, optional Discord
social linking, hidden account IDs, chosen names, four couch seats and guest
personal-stat privacy. MCC imports remain the sole runtime asset source, with
TU1.5 gameplay/network and Vista UI evidence used as references.

Retain current semantic and lifetime checks while replacing byte handling.
Keep native simulation, authority ownership and world publication in their real
C++ owners. No gameplay or transport rewrite may silently change TU1.5 behavior.

## Checklist acceptance criteria

### S01 — Threat model and inventory

Record actual callers, trust transitions, format versions, dependencies and
owners for public ONLINE, LAN/community joins, signaling/control, input/state,
authority transfer/deltas/replay, votes/telemetry and voice. Include outsiders,
malicious admitted peers, malicious hosts and colluding voters. Distinguish
memory corruption, impersonation/replay, gameplay cheating, denial of service
and IP exposure. Record baseline safeguards and unresolved findings without
claiming an exploit from source inspection alone. Decide migration order from
this inventory and identify every legacy bypass that must be retired or gated.

### S02 — Rust build and dependency lifecycle

Add pinned toolchain/lockfile, reviewed dependency features/licenses and a
reproducible Cargo-to-CMake integration for supported Linux, Windows and macOS
release targets. Retain version/build identities and establish update/advisory
checks. Run CMake validation through `tools/run_tests.py`; place Cargo target
directories, fuzz corpora and compiler/probe output inside the owned scratch
lifecycle described in [TEST_STORAGE](../../docs/TEST_STORAGE.md).

### S03 — Rust/C++ ownership interface

Define owned typed messages and opaque validated handles whose construction is
restricted to the real validation path. Bind authentication context separately
from packet-supplied identity. Document lengths, lifetimes, thread/executor
rules, cancellation, error conversion and shutdown order. Minimize and review
unsafe/FFI code; malformed input returns bounded errors rather than panics or
unwinding across the interface. Test stale/foreign/cancelled handles and owner
closure. Do not return untrusted pointers/callbacks, transmute wire structs or
feed accepted bytes into an unchanged general-purpose C++ network decoder.

### S04 — Application packet decoding

Replace real production receive-path decoders, including input, state and
session/control formats, rather than adding an optional Rust prefilter. Check
framing, length arithmetic, counts, enums/flags, numeric validity and trailing
data before allocating or dispatching. Preserve valid source bit patterns where
required; reject non-finite/out-of-range values where the field contract demands
it. C++ gameplay still validates live object identity and legal operations.

### S05 — Snapshot, archive, delta and replay handling

Move complete untrusted format decoding, fragment bookkeeping and decompression
into the Rust module, including nested schema traversal and delta/base identity.
Preserve accepted versions and exact world-state fidelity; never omit fields to
obtain an easier pass. Bound encoded/decoded bytes, cumulative allocations,
containers, fragments, simultaneous jobs, compression expansion and replay work.
Account for raw output, decoded objects, old/base/target snapshots and native
staging together. Individual allocation caps are not a whole-process budget.
Verify truncated, overlapping, duplicate/conflicting, corrupt, stale and
oversized streams plus cancellation without partial publication.

### S06 — Peer authentication and encrypted transport

Keep established DTLS/TLS protection and verify actual connection fingerprints
against authenticated admission/signaling for the correct match and physical
peer. Use reviewed libraries and protocol constructions; no home-grown crypto.
Provide equivalent authentication for every internet-exposed alternative path;
LAN nonce/cookie checks and unkeyed hashes cannot qualify that requirement.
Prevent downgrade, credential leakage and cross-match reuse. No single shared
lobby credential may let one member impersonate other members. Define original
sender verification for forwarded messages where that property is required.
Surviving peers must authenticate allowed transitions without a mandatory new
master round trip during migration.

### S07 — Replay, role and seat authorization

Specify and enforce sequence/replay windows, valid reordering, wrap/exhaustion
and rekey/reconnect behavior per message family. Bind messages to match,
membership and authority generations. A peer may control only its admitted
local seats; seat IDs, physical endpoints and account identities remain distinct.
Reject stale hosts, departed members and unauthorized host-only operations.
Test cross-match, cross-seat and old-generation messages, including those sent
over a valid encrypted connection. Preserve guest privacy in result submission.

### S08 — Migration, voting and telemetry

Replace network-facing membership/telemetry/vote parsing and state transitions
with Rust implementations preserving authenticated sender and generation
context. Test duplicate/conflicting votes, partitions, delayed messages,
colluding minorities and membership churn. The requested host-silence threshold
over 500ms triggers suspicion, not unilateral proof; preserve strict-majority
and split-brain prevention rules. Poor connectivity alone is not proof of
cheating. Keep actual native-cut readiness and world promotion checks in their
own owners. Qualify rejection/transition behavior through supported existing
protocol paths; full windowed migration remains downstream M02/M06/M07 work.

### S09 — Voice framing and delivery

Use Rust for packet framing, authenticated speaker/seat checks, sequence policy,
bounded queues and generation fencing. Preserve proximity/team/radio/dead/
postgame routing and mute/block behavior where supported, and retain explicit
gaps for unfinished modes. Never replay microphone/device queues as historical
world state. Native Opus calls remain a separate audited decoder surface even
after a Rust wrapper; test malformed authenticated audio and voice continuity
through supported generation changes. Keep match/postgame media on direct peers.

### S10 — Native state validation and containment

Rust decoding must still enter the existing coupled C++ authority preparation,
validate the locally admitted map/tag/BSP bindings, immutable image/topology,
salted identities, executor/TLS and allowed callbacks, and publish only a
complete accepted replacement. Network bytes cannot supply OS pointers,
arbitrary executable code, callback targets or external file paths. Source32
addresses remain checked data tokens, never host pointer authority.

Document and implement a containment design for remaining unsafe snapshot
import/native replay, evaluating a restricted worker process with bounded IPC,
minimal privileges and memory/CPU limits. An in-process thread is not a security
sandbox. Any retained in-process exposure needs an explicit reviewed rationale,
implemented compensating controls and evidence for the stated guarantees;
unresolved blocking exposure keeps this item open. Exercise process/job failure,
timeouts and malicious-but-well-formed native state without damaging the active
world, gaining privileges or retiring unrelated owners.

### S11 — Resource abuse and peer isolation

Apply measured per-peer and aggregate packet/byte/work limits to every route,
including signaling, telemetry, discovery and voice. Bound receive-loop work,
queue residency, incomplete transfers and malformed-packet logging; prevent
one sender from starving gameplay or filling the disk. Cancellation and timeout
must release only owned resources and preserve healthy peers. Cover repeated
authenticated floods and resource churn as well as unauthenticated traffic.
Record target hardware, budgets and normal-gameplay latency/memory overhead.

### S12 — Fuzzing, differential and adversarial tests

Use the current supported wire formats and source-bound behavior as compatibility
oracles, with explicit version negotiation for intentional changes. Fuzz the
actual Rust decoders/state machines and the mixed-language handoff; exercise
retained C++ consumers under appropriate sanitizers. Include integer overflow,
invalid numerics, depth/count expansion, corrupt compression, replay/reordering,
unauthorized seats/roles, stale epochs and repeated cancellation/teardown.
Keep meaningful regressions and small reproducible failure inputs. Parser-only
fuzzing does not qualify native import, FFI or end-to-end sender authorization.

### S13 — Native dependencies, hardening and independent review

Inventory remaining libdatachannel/DTLS/SCTP, crypto, Opus, compression and other
network-reachable native code. Pin/update and review security advisories and
required features; a Rust binding does not rewrite its native implementation.
Verify applicable release mitigations such as non-executable data, ASLR and
stack/control-flow protections, and test the resulting binaries. Independently
review unsafe/FFI ownership, trust transitions, cryptographic binding, replay
policy and native admission. Record findings, fixes and revalidation; unresolved
blocking findings prevent acceptance.

### S14 — Integrated acceptance and return to ONLINE work

Exercise actual Rust-enabled supported ONLINE join/play/voice paths between
independent processes, including malicious authenticated clients/hosts. Verify
normal multi-seat behavior, direct peer traffic, supported native preparation
controls and packaged runtime linkage without silently falling back to the old
decoder. Cover supported platform builds and distinguish local, physical-device
and independent-PC/WAN evidence. Required unavailable checks stay open.

Retain source/build identities, commands, reports, limits, security review and
runtime observations; verify disposable builds/corpora/captures are removed or
explicitly retained with path/size. Check S01–S14 only against their full criteria.
This gate does not assert complete host migration, all ONLINE modes or immunity
to every attack. After acceptance, resume the existing checklist at its recorded
unfinished item. Apply these security requirements to every later network path
before enabling it; downstream M02–M09 and other acceptance gates stay open.

## Current source starting points

These are inspected migration candidates, not certified vulnerabilities:

- [ONLINE peer transport](../../src/online_peer_transport.cpp) and
  [match-session dispatch](../../src/online_match_session.cpp).
- [Input/state wire handling](../../src/multiplayer_network.cpp).
- [Authority transfer](../../src/peer_authority_transfer.cpp),
  [archive decoder](../../src/source_authority_codec_decode.cpp),
  [archive schema](../../src/source_authority_archive.hpp) and
  [receiver/replay worker](../../src/source_authority_receiver.cpp).
- [Source admissions](../../src/peer_source_admissions.cpp),
  [host agreement](../../src/peer_host_agreement.cpp),
  [common-cut agreement](../../src/peer_common_cut.cpp) and
  [mesh telemetry](../../src/peer_mesh_quality.cpp).
- [Game voice](../../src/game_voice.cpp) and
  [native codec/device adapter](../../src/online_voice.cpp).
- Retained native gates: [coupled authority](../../src/source_match_authority.cpp)
  and [native world preparation](../../src/physics_live_world_tu15.cpp).

Design references: [Rust unsafe/FFI guarantees](https://doc.rust-lang.org/stable/book/ch20-01-unsafe-rust.html),
[CXX ownership bridge](https://cxx.rs/),
[WebRTC communications security](https://www.rfc-editor.org/rfc/rfc8827.html#section-6.5).

Status at creation: checklist and acceptance specification only. No Rust source,
runtime setting, transport policy, installed build or deployment changed.

Progress, 2026-10-21: **S01 source inventory is complete**. The
[threat model](online-security-threat-model.md) records current receive paths,
dependencies, trust assumptions, native semantic gates, outstanding hardening
work and the replacement order. Its [source-bound receipt](../../.local/online-security-s01-20261021/verification.json)
checks twelve receive funnels, source identities and links. This closes inventory
only; S02–S14 and all implementation/runtime security acceptance remain open.

Initial S02/S04 foundation, 2026-10-21: the
[Rust protocol module](../../rust/online_protocol/README.md) uses a scoped
Rust 1.96.0 pin, a locked package with no third-party dependencies and unsafe
code forbidden. The existing native kind-6 voice framing now has an owned Rust
parser with seven passing test groups for literal field vectors, ownership,
malformed framing/version/fields, payload boundaries, non-finite gains, sequence
preservation and bounded arbitrary short inputs. Those deterministic controls
are not a coverage-guided fuzz campaign. Strict Clippy and a Linux release
build pass under the managed scratch lifecycle; the [receipt](../../.local/online-security-rust-20261021/initial-parser/report.json)
pins actual tools, commands and sources. CMake/bridge/routing, authorization,
mixed-language compatibility and release/platform acceptance are still pending.
No existing C++ receiver was replaced or enabled by this initial module.

Following bridge milestone, 2026-10-21: CMake now links a mandatory pinned CXX
bridge and the actual match/postgame session consumes owned Rust-decoded voice
fields. The [interface and qualification](online-security-rust-bridge.md) record
normal/sanitizer differential and ownership controls, plus actual repository
linkage and four-peer DTLS/synthetic audio continuity with authenticated
malicious-voice rejection controls in normal and C++ ASan/UBSan builds.
This supersedes the initial parser-only
routing status, not its historical evidence. The other packet families,
complete admission/replay policy, platforms, native containment and release
security acceptance remain open. No runtime was installed or backend deployed.

Native-request milestone, 2026-10-21: complete native Hello/Input/Leave/Query
decoding now uses Rust in the actual host, loading worker and saved-input
checks, returning owned profile/control fields without a legacy request-reader
fallback. The [qualification](online-security-native-requests.md) records ten
Rust groups, strict Clippy, 337,231 mixed-language native-request comparisons,
Linux normal/C++ bridge sanitizer and Windows GNU/Wine controls. All eight
selected repository tests pass, including actual four-seat rejection/recovery,
UDP input/discovery/startup and direct DTLS/synthetic voice continuity. Accounts,
endpoint admission and native cut/role completion have stated fixture limits.
Full native-world promotion, remaining packet/archive migration, containment,
fuzzing, full sanitizer/platform/release coverage and independent review remain
open. S01 was the only fully checked item at that milestone.

Inventory scope change, 2026-10-22: concurrent EOS SDK/transport and Rust
reassembly/session work adds ingress and native callback owners beyond the
original source inventory. S01 is reopened until that new source is inventoried
with credential/sender/lifetime and direct/relay policy boundaries. Earlier
DTLS and twelve-funnel evidence remains historical. Archive parser results do
not accept the new SDK callbacks or combined transport configuration.
