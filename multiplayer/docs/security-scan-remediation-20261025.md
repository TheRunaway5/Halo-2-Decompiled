# Security Cloud remediation, 2026-10-25

Scope: the completed Halo-2-Decompiled scan against
`cfaae7534d8a777917d607c12b3715c2fbe2c85d`, run
`wfr_8c89af4d74f2e8f7327c653385f32f182e2d83dc62de98f20707ae74d3d52047`.
Its saved findings artifact contains three medium vulnerabilities. Changes are
local; Cloud statuses, deployment and installed launcher runtimes are unchanged.

| Finding | Remediation |
| --- | --- |
| `csf_fa795f6eea905f5260f0f0d0`: unused credential revocation exhausts listings | Revoking an unpublished credential allocates no listing. Rust admits at most 16 retained listings per owner and 488 local listings, reserving 512 slots for federation. Expired local envelopes are reclaimed after the lease/replay horizon. Compact credential-bound revision watermarks preserve increasing revisions across cleanup and restart; credential history still rejects reuse. Revocation works at full capacity. |
| `csf_ff385c688332b465ece06861`: first raw-LAN frame chooses the host key | Raw queries and joins pre-bind the independently exchanged host key. Rust rejects unbound sessions and authenticates roles, both keys, protocol, nonce, content fingerprint and destination. Configuration fragments and rejection messages require authentication as well. The raw protocol is now 107. |
| `csf_939cd12566dbf069184fa29e`: host-controlled authority decompression | Explicit network limits cover full archives, deltas, nested host cuts, grammar metadata, native staging and asset loads. The decoded ceiling is 96 MiB, selected from actual source checkpoint measurements. Canonical images enter the Rust parser directly, removing recompression and repeated decompression. Complete-world reservations survive decoding through native staging; rejected or cancelled work releases them. |

Raw joining now requires `--join 'ADDRESS:PORT#HOST_KEY'`. The host prints the
token, and embeddings can use the host-ready callback. Exchange it through a
trusted independent channel. The key is public and changes with the host
process; an unauthenticated discovery response cannot select it. See
[raw LAN authentication](raw-lan-host-keys.md).

The service security ABI is now 2. The Python adapter rejects an old library
before resolving new symbols; deployments must rebuild the Rust service
library with the service. Authenticated ONLINE transport retains its existing
identity owner.

Authority admission includes exact sparse-index and synchronization expansion
charges, collision triangle preflight, and bounded grenade animation/hierarchy
loads. Existing cache mappings resolve authored Xbox reflexives; tag size is
not treated as a fabricated contiguous ownership boundary. The accounting,
phase inequalities and measured limits are documented in
[authority resource admission](security-authority-resource-budget-20261025.md).
The 1,184 MiB component reservation is not a whole-process RSS limit; independently
loaded map/cache resources, allocator bookkeeping and thread stacks are separate.

Validation evidence is retained under `build/test-results/security-scan-20261025*`:

* All three original attack cases have failing baseline regression evidence.
  The oversized-allocation baseline intercepts the allocation request safely.
* The backend suite passes 329 tests, plus a supplemental stale-ABI rejection test.
* Final Rust validation passes 194 tests with four environment-dependent tests
  ignored; strict all-target Clippy passes for all three crates, using verified
  Rust 1.96.0.
* Twenty focused native checks pass across the final frozen cohorts and
  documented reruns, including real full/delta transfers, raw UDP authentication,
  collision/resource rejection, cancellation, host loss and exact complete-packet
  host continuation. The 16-player budget case verifies all 16 actual source
  unit/player/body bindings after 20 genuine lifecycle ticks, then round-trips
  an 87,707,746-byte canonical checkpoint within the network limits.

The native cohort uses `-O0 -DNDEBUG` for functional checks and optimized Rust,
two build jobs and one concurrent CTest. It preserves the initial CMake cohort
and explicitly records security source refreshes while unrelated rendering,
physics and audio work continues in the checkout. These are source/component
checks, not a clean Cloud rescan, deployed-service test, installed-runtime
qualification or playable WAN acceptance.

The exact host fixture explicitly selects the complete-packet profile. Adaptive
delta sender history, ACK baselines and bandwidth state remain a separate
continuation owner; byte-exact restoration of that default transport is not
claimed by this test. Earlier receiver deadline failures under competing builds
were rerun with bounded timing diagnostics and the same timeouts; both warm and
cold full/delta cases passed.

The source receipt identifies two later unrelated edits in shared files: corpse
pose hot sets and render overlap/timing. All other network bytes and the runtime
prefix containing host-key setup match the tested cohort. These concurrent
changes and the newer build registry were preserved in the workspace.

Owned native and Rust scratch/build paths were removed and removal verified.
No large binaries or captures were retained. The aggregate result and cleanup
receipts are `build/test-results/security-scan-20261025/validation.json` and
`build/test-results/security-scan-20261025/cleanup.json`.
