# Security Cloud remediation, 2026-10-23

Scope: the completed Halo-2-Decompiled scan against
`955076c3d3964fd81e9e0ad7ea2eea69388b7614`, run
`wfr_42fe5ca037ce16d8e0c0abb9f9166f1f7112b9df24b414f39a3ada9137e72d77`.
The companion JSON retains all ten canonical finding IDs and original root causes.
Changes are local; Cloud statuses, deployment and installed runtimes are unchanged.

| Finding | Change |
| --- | --- |
| Raw replay memory writes/entrypoints (high) | Rust denies raw writes, ABI/entrypoint calls, camera controller/observer banks, lens/device and pointer-bearing render capabilities in both network encoding and decoding. The complete batch is checked before candidate execution. Local diagnostic tapes retain their existing API. A host frame containing these operations invalidates its network journal and uses the complete-checkpoint repair path for the rest of that authority generation, so subsequent frames cannot repeatedly cancel the replacement checkpoint. |
| Federation listing exhaustion (medium) | Rust admission policy limits each foreign origin to 128 retained records and the aggregate foreign bank to 512 of 1,000. Cleanup repairs existing oversized foreign banks by quarantining the affected origin until all earlier leases expire, reclaims expired records after the anti-resurrection horizon, and removes unpinned origins. Local authoritative rows remain intact. |
| Steam revocation credential loss (medium) | Rust validates the exact successful revocation response before the native adapter deletes protected storage. HTTP, transport, cancellation and schema failures retain retry capability. The backend forget endpoint is idempotent. |
| Unauthenticated established LAN datagrams (medium) | Rust owns ephemeral X25519 keys, HMAC-SHA256 packet authentication, recipient/direction binding, fixed peer keys, monotonic counters and a 1,024-packet replay window. The endpoint proof binds the client's public key. Raw input, leave and snapshot fragments require authentication. Native protocol 101 rejects older peers. |
| Unverified persisted postgame details (medium) | Rust defines the persistable summary projection. New reports persist only independently checked roster, variant, placement, score and KDA/suicides plus checked team summaries. Appearance, medals, weapon/opponent banks and other detailed statistics are excluded from new persistent results. Historical data remains readable. |
| Copied federation identity (medium) | Origin/server pairs remain distinct through storage, discovery, native decoding and browser selection IDs. An origin no longer replaces another origin by copying a public server key. This preserves provenance; discovery still supplies no verified game endpoint. |
| Equivocating successor votes (medium) | Rust requires the entire fixed survivor electorate to accept the same ballot/value. Honest voters lock their first value across ballots for that context. A malicious or missing survivor can prevent migration, but cannot produce two honest decisions through split pairwise votes. Host-agreement wire version 2 excludes old semantics. |
| Steam proxy credential disclosure (low) | Steam's transport disables ambient proxy discovery, including for opted-in literal-loopback HTTP. |
| Linked-player capacity bypass (low) | All new player allocation uses one allocator. A SQLite insertion trigger enforces the Rust-defined global limit atomically across connections, including the dual-provider link route, with no partial identity/profile rows. |
| EOS verification queue monopolization (low) | Rust admits at most one request per authenticated player and 32 globally. Account and room routes supply the verified principal rather than the unverified PUID. The existing bounded lock wait, service timeout and exact worker-stream cleanup are preserved. |

Rust policy lives in `rust/online_protocol/src/security_policy.rs`,
`lan_auth.rs`, and `host_agreement.rs`, with generated CXX adapters and a small
service C ABI in `rust/online_security_ffi`. The protocol crate forbids unsafe
Rust. The service ABI has one documented, fixed-size buffer-copy boundary;
it receives buffers allocated by the trusted Python adapter, never wire pointers.
Python retains SQL transactions and async process orchestration; C++ retains
sockets and game-world ownership. Neither implements a fallback security policy.

The service requires its compiled Rust library. CMake test environments and
the Docker image include it; `tools/test_master_server.py`, invoked through
`tools/run_test.py`, builds a disposable library for source-tree tests.
The library ABI is checked before use and absence fails closed.

Validation records are under `build/test-results/security-scan-20261023-*`.
The final backend suite passes 319 tests. The final Rust checks pass 97 core
tests and 28 bridge tests, with two environment-dependent integration tests
ignored; strict Clippy passes for the policy and service ABI crates. Six frozen
native functional checks pass, covering real authenticated LAN traffic,
configuration/startup, capability rejection, complete-checkpoint recovery and
authorized two-process replay. The repaired native cohort uses `-O0 -DNDEBUG`;
Rust retains its optimized release build. Earlier optimized native validation
passed eight checks and exposed the two cases repaired by this cohort. Focused
ASan/UBSan transport, Steam, directory and all-survivor agreement checks pass.
Both compose configurations validate; the container image was not built because
the Docker daemon is inactive.

Local tests do not establish a clean Cloud rescan, deployed-service acceptance,
live Steam/EOS provider acceptance or a playable WAN session. No large artifacts
are intentionally retained by this investigation.
