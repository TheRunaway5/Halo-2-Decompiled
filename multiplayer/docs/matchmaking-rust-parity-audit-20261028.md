# Matchmaking parity audit and Rust replacement goal

Audit date: 2026-10-28. **Full original matchmaking parity remains open.**
This is the baseline for replacing matchmaking implementation in Rust, as
requested by the user. The initial audit changed no production behavior. The
source implementation progress below is recorded separately from that baseline.

The separate [hopper reconstruction and exact-frequency checklist](playlist-hoppers-20261028.md)
records the subsequent Rust selector, complete selected options,13 regular
playlists and the separate Lab Test. GAP-08/GAP-11 findings in the initial table below are historical
baseline findings; this bounded implementation does not close their complete
RNG-ownership, immutable-envelope, installed-runtime or live-flow exit criteria.

## Authority and scope

The supplied [master PDF](reference/halo2-original-matchmaking-master-v5.pdf)
is a 42-page document named v5; its embedded title identifies the retained V4
master and V4.1 recovery appendix. Its SHA-256 is
`37851f359e0bf14366da356707c146dc4927d26e2d7e93adfb90c92cca0f658d`.
The appendix keeps all twelve GAP records open. The companion expanded
algorithm registry, coverage register, gap register and handoff package were
not supplied with this attachment or identified in the active source inventory.
The local [gap register](matchmaking-rust-parity-gaps.json) preserves the twelve
IDs and all 53 algorithm IDs transcribed from PDF section 12.2; it does not
pretend to be the missing expanded registry.

The register also preserves all twelve **G01-G12 acceptance gates** from PDF
section11.2, printed pages19-20, with their complete required-evidence text.
Every gate remains open; the component receipts linked under G04 do not close
its authentic-input, service, version or native/delivery boundaries.

The native HEAD is `ec3fdb77161886514b67bcdfd6bc7ec5d6659c88`, the same
commit audited by the PDF. The working tree also contains unrelated changes;
file hashes in the retained receipts identify the files actually examined.
The local independent decompilation is now
`5f08630655e3ddc9ac45909090f11ae799fa0c27`, rather than the PDF's recorded
`ff5f319fcdfc8c0b4ea7879e4bcebd69a384ec8e`. Reconstructed text must be
checked against the relevant pinned executable before treating a caller as
original-byte equivalent.

OG Xbox/TU1.5 is the gameplay authority. Retail defaults and dated service
profiles remain separate inputs. MCC supplies imported runtime data; its
presence does not establish original rules or original Xbox framebuffer parity.

## Differences reproduced at the initial audit

1. **The real gathering finalizer chose the wrong two-team mechanism.**
   `GatheringSessions.proposal()` calls `Coordinator._teams()`. Ordinary two-team
   games use Python exhaustive partitions, effective-rank total difference
   and a random tie. For eight admitted solo raw ranks
   `24,24,22,22,22,22,20,20`, seed 2 produces `[24,24,20,20]` versus
   `[22,22,22,22]`, recovered cost 24. Compiling the existing Rust kernel
   produces `[24,22,22,20]` on both teams, cost 0. This reproduces PDF section
   5.5 through the actual gathering-to-coordinator route, with synthetic seat
   admission. It is not a live online match.
   [Receipt](evidence/matchmaking-rust-parity-20261028/audit-receipt.json).

2. **Guest ranks change the gathering advertisement's average.** For signed-in
   raw ranks 20 and 40 plus a guest inheriting raw 20, production
   `party_rank()` returns 30 while production `GatheringSessions.bank()`
   advertises 26. The latter averages every seat. The original summary owner
   excludes identity flags 1/2 from its rated aggregates; see the existing
   [rank-floor evidence](halo2-rank-floor-evidence.md). This audit directly
   reproduced the internal mismatch; it did not rerun the original guest
   producer for this case.
   [Receipt](evidence/matchmaking-rust-parity-20261028/guest-advertisement.json).

The existing Rust balancing kernel itself passed a fresh unchanged-TU1.5
comparison: **256 cases, 768 complete original calls, 1,111 distinct original
instructions**, with no instruction patches or replaced callees. This covers
the kernel, count and capacity leaves, not the complete hopper caller or
online integration.
[Original replay receipt](evidence/matchmaking-rust-parity-20261028/team-balance-original.json).

## Gap register

Every row remains open until its complete exit criteria pass. Existing useful
components are retained; an evidence gap is not automatically an absent feature.
The table records the initial audit. Later source progress and current
integration limits appear below and in the machine-readable register.

| ID | Current finding against the master | Required correction and proof |
| --- | --- | --- |
| GAP-01 | Confirmed two-team divergence above. Clan assignment bypasses the generic caller. More-than-two-team routing uses Rust leaves, but the Python adapter hardcodes party flags to false/zero. The policy model lacks the complete original hopper type/getter contract. | Implement the actual ordered caller in Rust: FFA, original hopper options, count feasibility and allowed imbalance, weighted optimized assignment, and type-3-only individual retry. Preserve raw weights, grouping/traversal order, failure transitions and clan admission. Replay complete callers and real dispatch, not just a `>2` branch edit. |
| GAP-02 | Guest aggregation mismatch reproduced. `effective_ranks()` uses a separately described 2007 inference; `PlaylistSkills.commit()` supplies persistent raw rank as comparison skill. Search, assignment, published adjusted skill and persistent XP are not proven to have the original separate producer chains. | Give Rust distinct raw, effective, previous and public-rank types/fields. Trace and replay original producers/consumers for remote parties, independently signed local accounts, guests and clans. Distinguish `F(H)` from the decompiled double-inverse error. Do not guess a universal adjustment. |
| GAP-03 | Seven source families and all 39 built-ins already exist in component evidence. The retained acceptance matrix still has `pending_full_scope=true`; the twelve-seat source fixture has `runtime_passed=false`. | Verify genuine admission, exact variant, ordinary actions and natural endings through results, public/hidden/clan progression, PCR, party return and requeue for every applicable profile/variant. Include local/remote ownership, ties, departures and migration. Keep shortened/component tests labeled. |
| GAP-04 | A constructed retail fallback blob is available, with the expected 1,453,248-byte size. It is not an authenticated final-2010 download. No authenticated final-service configuration or complete April-2006 workbook was identified in this audit. A filename search is not proof that such an artifact does not exist. | Preserve and parse legitimate original/default and downloaded inputs separately. Recover complete dated hopper/network data, weights and variant buffers, including the historical inventory and Team Carnage/HBO checkpoint. Record provenance and unresolved cells/fields. Never synthesize historical values. |
| GAP-05 | `GatheringSessions.query()` explicitly substitutes structurally compatible sessions sorted by `(born,id)`, truncated to 50. The retired title-specific procedure's filtering, ordering, truncation and ties are unrecovered. | Move the replacement policy into an explicitly named Rust provider boundary. Preserve ordered banks into the client decision core. Seek distinguishing original request/response evidence or procedure definitions; keep original-service parity open until recovered. |
| GAP-06 | `MatchResults.commit()` requires every originally admitted physical owner. An unreachable owner cannot satisfy that rule. Transactions and immutable-digest retries already protect exactly-once writes; they do not prove original arbitration or independent simulation truth. | Implement Rust validation/finalization and an explicit authority/dispute/quarantine policy. Test absent, late, duplicate, conflicting and disconnected reports plus couch ownership and restart/retry. Preserve security; label modern arbitration until original title-specific evidence exists. |
| GAP-07 | Rust migration components and C++ native handoff APIs exist. Python continuity selects by measured milliseconds/index and a native timeout; `_host()` uses native route scores. These are declared replacements, not the original host comparator. Old documentation saying no handoff API exists is stale. | Put matchmaking host/session decisions in Rust. Trace original eligibility, generations, clocks, QoS units and caller state. Prove actual existing-world continuity under graceful/abrupt host loss and ordinary departures for all families, including meaningful objective/role/fuse state. Keep route-measurement differences explicit. |
| GAP-08 | Both gathering and legacy selection use Python `random.choices`; weights accept 1..10000 rather than the selector's available-map clamp 1..1000. Admission requires every pool map, whereas the original selector zeros unavailable rows. `Game` carries names/weight, without complete selected settings. | Implement original ordered LCG draw and full-row selection in Rust, with explicit RNG ownership/call order, duplicate rows, map predicates, clamps, output/failure behavior and complete 304-byte settings. Replay signed-short boundary behavior before extending the demonstrated safe domain. Do not use a random distribution approximation. |
| GAP-09 | Both legitimate original executables are present and hash-verified. The system Python lacks oracle packages, but `.tools/python/bin/python` has capstone/unicorn and successfully reran the team oracle. Existing fixtures/harnesses provide a usable starting point. The full caller suite is still missing. | Build repeatable original/Rust comparisons with binary, runner, fixture, config, compiler and instruction identities. Use existing genuine inputs. Regenerate missing fixtures from originals rather than the implementation under test. Preserve rejected/failing cases and declare synthetic inputs. |
| GAP-10 | Both local selection manifests name runtime `9d531caa3b6a4b0c9284b954c3874139`; its binary hash matches the declared hash. This audit did not qualify its matchmaking flow or inspect the running remote service/library/catalog. Prior runtime IDs in older notes are not current observations. | Bind Rust source/build/library, actual selected client, server, catalog, profile and feature identities to acceptance receipts. Verify production-equivalent flow, config changes during an outstanding match, restart, rollback and installed delivery. A matching local binary digest alone does not close this row. |
| GAP-11 | Catalog `Game`/`Proposal` and the room handoff carry map/variant names; result validation maps 39 known names and otherwise assumes Slayer. No complete selected-options digest, map revision, profile epoch and statistics namespace are carried in this audited matchmaking chain. Some frozen skill/configuration checks already exist. | Introduce an immutable Rust admission/result envelope binding complete settings, executable/profile/catalog/map/rank identities, roster and authority generations, expiry and replay identity. Verify every upstream/downstream consumer and transactional player/clan namespace. Reject unfamiliar objective variants with missing authenticated settings rather than treating them as Slayer. |
| GAP-12 | Documentation mixes legacy timed lab search, recovered gathering, old Discord-only descriptions and obsolete host-role limitations. Current `Directory.verify_identity()` accepts Steam sessions when configured; queue creation uses that provider-aware owner. | Update operational docs from actual selected routes and fresh receipts. Keep historical notes identified, distinguish source/component/live/deployed evidence, and maintain the intentional-deviation ledger. Do not replace accurate seven-family implementation with old Slayer-only claims. |

Primary implementation locators are
[`matchmaking.py`](../master_server/matchmaking.py),
[`gathering_sessions.py`](../master_server/gathering_sessions.py),
[`team_balance.py`](../master_server/team_balance.py),
[`playlist_skill.py`](../master_server/playlist_skill.py),
[`clan_skill.py`](../master_server/clan_skill.py),
[`session_candidates.py`](../master_server/session_candidates.py),
[`search_lifecycle.py`](../master_server/search_lifecycle.py),
[`match_results.py`](../master_server/match_results.py),
[`match_transport.py`](../master_server/match_transport.py), and
[`matchmaking_http.py`](../master_server/matchmaking_http.py).
The Rust starting points are
[`online_protocol`](../../rust/online_protocol/src/lib.rs),
[`team_balance`](../../rust/online_protocol/src/team_balance.rs),
[`online_security_ffi`](../../rust/online_security_ffi/src/lib.rs), and
[`online_protocol_bridge`](../../rust/online_protocol_bridge/Cargo.toml).

## Rust assignment implementation progress

`rust/online_protocol/src/team_assignment.rs` now implements the original
hopper getters and ordered assignment corridor. The ordinary two-team Python
partition/random-tie optimizer has been removed. Actual coordinator and
gathering finalization now send FFA, two-team, multi-team and clan formation to
the Rust caller through a bounded service ABI. A separate ABI accepts exact
0x614-byte original hopper headers and preserves their flag and imbalance
fields; it does not synthesize a header from a playlist name.

Fresh unchanged-retail execution validates **336 assignment decisions and
2,688 getter calls**, covering **1,248 distinct original instructions**. The
fixture includes the low-byte type-3 imbalance boundary, preserved-party
failure, individual retry, FFA and rejected assignments. All original callees
execute without instruction replacements. The harness starts at `0x71B1E`
with explicitly supplied ready-summary operands and stops at `0x71CAD` before
publication or `0x71C73` before session-failure continuation. This is stronger
than a standalone kernel test, and still does **not** execute the upstream
session/readiness/`0x7E210` producers or downstream `0x71FF0` publication.
It does not certify the entire `0x71730` owner or TU1.5 caller correspondence.

The numerical [original fixture](../../tests/fixtures/matchmaking_assignment_original.json)
is retained with its generator
[`oracle_matchmaking_assignment.py`](../../tools/oracle_matchmaking_assignment.py).
The [original comparison receipt](evidence/matchmaking-rust-parity-20261028/assignment-corridor-original.json)
records exact input, executable, tool, fixture and Rust source identities.

Four Rust regression tests and **392 backend tests** pass, including the
actual gathering-finalizer seed-2 counterexample and the original-header ABI.
The complete backend rerun uses `tools/test_master_server.py --freeze-source`,
which owns a copied service/Rust cohort and records its source hashes. The
earlier nonfrozen run retained two last-opponent transport failures while
unrelated transport edits were in progress. The frozen rerun passed using the
then-current transport changes; this assignment work did not author those
transport changes.

GAP-01 remains **open, with verified partial implementation**. The current
native catalog is still projected from declared service fields; it is not a
historical hopper file. Its separate significant-party preadmission remains
an explicitly native rule while the full original summary producer is recovered.
Python partial-gathering capacity and other queue/search/API/result owners also
remain to be replaced. Full caller/producer/publication, original profile
binding, actual native online acceptance and delivered-bundle verification are
still required. No installed runtime or production service was replaced.

The [assignment validation receipt](evidence/matchmaking-rust-parity-20261028/assignment-validation.json)
identifies the tested cohort, bounded logs, source hashes and removed scratch.

A later frozen cohort includes concurrent Rust hopper-selection changes and
additional transport tests. It ran 395 tests with four failures and one error:
the old gathering-creation draw assertion, three Tombstone admission/draw tests,
and a team-departure fixture rejected by catalog dimension validation. These
are explicit integration work in the receipt. The earlier 392-test pass does
not qualify the later changing checkout. The legacy all-maps admission test
now rejects an actually absent pool map, and an admission/formation/draw test
checks partial map ownership and disjoint pools. These updates do not close
the full selected-profile, RNG-owner or map-revision gates.

The subsequent ratings/catalog integration cohort below passes all **409
backend tests**. It includes the concurrent selector/transport updates and
resolves those previously recorded integration failures. Its frozen manifest,
rather than either earlier test count, identifies that passing source cohort.

## Rust raw ratings and search-bound progress

`rust/online_protocol/src/matchmaking_ratings.rs` now owns raw rated aggregates,
the separate adjusted-player array, the original rank-map inverse, advertised
minimum and the search initialization/query bounds. The temporary service
adapter in `matchmaking_ratings.py` only validates and marshals bounded inputs.
`party_rank()` and actual gathering advertisements use Rust statistics; search
and advertisements read the selected header's rank table, or the explicitly
declared retail executable default when no original-layout header is supplied.
There is no Python arithmetic fallback.

The actual guest regression was recorded failing at **26 instead of 30**, then
passed through the Rust ABI: signed raw ranks 20 and 40 contribute to the
divisor; the guest's copied 20 remains a player but does not bias the average.
Raw operands remain unchanged. A synthetic identity-table header additionally
checks the selected table at the actual search and advertisement seams, where
its bounds differ from the retail defaults.

Fresh unchanged-retail replay covers **300 roster controls**, including **219
complete `0x7E210` calls**, **516 inverse lookups**, **40 search initializations**
and **65 bounded queries**. All **888 Rust comparisons** pass across **3,293
distinct original instructions**, without instruction patches or substituted
callees. The other roster controls stop at `0x7E550` after raw statistics,
before group-capacity production; complete-function controls require a rated
seat in every declared group. Advertisement controls enter at `0x736A8`, search
initialization after allocation at `0x9098B`, and queries stop before the Live
wrapper. Inputs are declared diagnostic controls, not observed historical
service sessions.

These controls verify `max(raw,F(H))` for nonzero catalog indices, the original
catalog-index-zero bypass, NONE handling, flag-bit exclusions, signed division
and independently retained raw/second-XP/adjusted arrays. The original table
validator checks signed bytes pointwise against their indices; it does not
require monotonicity. A valid nonmonotone diagnostic table is included to avoid
silently narrowing the original domain.

The [original comparison receipt](evidence/matchmaking-rust-parity-20261028/ratings-original.json)
and [numerical fixture](../../tests/fixtures/matchmaking_ratings_original.json)
pin the executable, runner and Rust source. Eight frozen Rust regression tests
cover ratings and assignment; 69 focused backend tests and the subsequent full
**409-test backend cohort** pass. The
[validation receipt](evidence/matchmaking-rust-parity-20261028/ratings-validation.json)
records failing-before-fix controls, the passing source manifest, reproducible
catalog check and eight verified removed scratch directories.

The full-suite run additionally exposed invalid zero-filled rank maps in the
concurrently reconstructed online catalog. The generator now explicitly copies
the retained retail-default table, records its source header/executable identity
and labels later downloaded values as unknown. Rust rejects invalid maps at
catalog load, before a ticket can reach search. This corrects the admitted
configuration without inventing a historical downloaded map.

GAP-02 remains **open, with verified partial implementation**. Full summary
mask/capacity/validity production, independently signed local and clan producer
correspondence, TU1.5 owner mapping, published effective-to-result comparison
skill and transactional persistent/public progression remain required. The
adjusted array is exposed with an explicit catalog index, but has not been
guessed into the persistent result path. Search scheduling, candidate-input
producers, coordinator/API/state and result authority still require Rust ownership.
This work does not qualify native played matches or delivered service/runtime
identity.

## Rust candidate core and state progress

`rust/online_protocol/src/session_candidates.rs` now owns retail candidate
admission, scoring, comparator/sort, radius-threshold interpolation, selection
and the exact sorted byte cache. Rust also owns the selected search/configuration
operands and executable-default configuration. The Python module at the former
algorithm location only marshals typed inputs and copies returned bytes; it has
no scorer, sort, threshold or selection fallback. Actual gathering queries
reach this Rust owner through the private service ABI.

The fixed-middle-pivot sort retains the original eight-row short sort and
one-byte partition-span comparison. Its comparator retains the asymmetric
first-record readiness check. Single-precision RTT and threshold operations,
signed wrapping score arithmetic, truncating conversions, the strict partial
QoS wait boundary and complete selected records are preserved. A configuration
change intentionally retains the original identical-byte cache behavior.
Opaque bank bytes remain part of the cache key and full-bank output.

Fresh unchanged-retail execution exactly reproduces the existing 735,608-byte
compressed numerical fixture. Rust matches all **264 full 50-record banks,
27 admission controls, 124 selection controls and 100 threshold controls**.
The supporting oracle records **339 original controls and 1,987 distinct
instructions**, including additional QoS extraction and configuration-reader
controls; those extra supporting owners have not all been ported by this work.
Threshold prefixes stop before real Live query `0x8FA80`. Synthetic records and
configuration are explicit inputs, not original online service observations.

Private monotone Rust handles retain each query's state. Explicit close and
finalization retire it; a retired handle cannot read or select a record.
Rejected records/configuration do not publish a partly scored bank or change
output buffers. The bounded private carrier and 4,096-handle resource limit are
declared modern implementation limits, not original population rules.

The passing frozen cohort contains **12 Rust regression tests and 412 backend
tests**, including the actual gathering/HTTP paths, state isolation and invalid
ABI inputs. Use `tools/test_master_server.py --freeze-source` with repeatable
`--rust-test` arguments to run the Rust integration suites and backend against
one owned source cohort. The
[candidate validation receipt](evidence/matchmaking-rust-parity-20261028/candidates-validation.json)
pins that cohort, the configured Rust toolchain and fresh original fixture
comparison. All four owned scratch directories were verified removed. No native runtime or remote service
was replaced by this work.

The broader goal remains open. Original input producers, TU1.5 correspondence,
Live task and provider behavior, lifecycle/coordinator/API/results/persistence,
authenticated profile/epoch binding and native/delivery qualification remain
required. The native provider's `(born,id)` ordering and 50-result truncation
remain explicit replacement policy, with no recovered original service contract;
the following lifecycle increment moves that ordering into Rust. A verified
candidate decision core does not close GAP-05 or certify the whole flow.

## Rust lifecycle state and native provider progress

`rust/online_protocol/src/search_lifecycle.rs` now owns signed tick/timeout
arithmetic, role choice with the declared default NAT policy, the two-draw signed
low-16 jitter stream, result-driven radius/total/active state, failed-session
memory, historical peak/growth clocks and countdown gates. It also owns each
native party's choose/search/gather/joined transition state and timestamps. The
Python lifecycle module supplies typed IO projections and private handles;
independently mutable Python progress/clock/seen/party fields were removed.
New-search initialization clears the seen bank; radius retries retain it.

Private monotone search generations reject stale progress/seen views. Rust
computes a candidate next state and commits it only after command validation
and output-capacity checks. The ABI test proves an undersized output cannot
consume a jitter draw. These generations, carrier/handle bounds and native mode
projection are explicit modern safeguards, not original wire fields or a claim
that every original session transition has been recovered.

The real finalizer had another bounded source mismatch: with an explicit
`INTMAX` countdown and equal elapsed time, `GatheringSessions.proposal()` emitted
a proposal. The original `0x719E9` gate rejects that sentinel independently of
elapsed time. The retained failing-before-fix test now passes through the Rust
gate. This is a synthetic admitted finalizer/clock control, not a live match.

Fresh unchanged-retail replay and Rust comparisons cover **68 jitter controls,
37 wrapped elapsed calculations, 72 role entries, 199 completion controls, 578
growth-clock controls, all seven built-in tables and 126 countdown getter
outputs, 49 countdown gates, 63 gather-timeout boundaries, 12 prepared search
timeouts, 154 seen-bank appends and seven lookups**. The seen-bank controls
retain full eight-byte original IDs, duplicate suppression, the 150-entry cap,
no eviction and the successful-new-search clear. The oracle verifies **2,236
distinct executed original instructions**, without patches or substituted
callees. Its numerical fixture is 206,883 bytes.

Whole initialization/getter/seen functions and the prepared search owner run
unchanged. Role entries, growth/countdown/timeout blocks, successful initializer
tail and query resubmission retain explicit boundaries before platform/launch
work. The full original Live allocator, logon/NAT/readiness/quality producers,
remaining session states and TU1.5 correspondence are separate requirements.

The explicitly named Rust `native_birth_id_order` provider now orders already
structurally admitted descriptors and truncates at50. An actual gathering query
with60 synthetic sessions verifies that route and its ties. Structural admission
still comes from the temporary coordinator; original Live filtering/order/ties
remain unrecovered. GAP-05 stays open.

The frozen cohort passes **18 Rust regression tests and 422 backend tests**.
The [lifecycle validation receipt](evidence/matchmaking-rust-parity-20261028/lifecycle-validation.json)
records original/fixture/compiler/loader identities, red/green sentinel evidence,
tested source hashes and five verified removed scratch directories. Later
concurrent security/service/FFI edits are recorded as drift; the passing frozen
cohort does not qualify those later files. The lifecycle core/bridge/adapter
hashes remain equal, and its shared FFI module registration remains present.
Required original numerical metadata
is embedded inside the Rust crate for normal/container builds; its original
source copy is preserved, and all seven tables are compared to fresh getters.
No installed runtime or remote service was replaced by this work.

Full queue/gathering/query registry and coordinator/API/results/persistence
ownership, authenticated profile/epoch bindings, complete original callers,
historical service recovery and native played/delivered acceptance remain open.

## Rust queue and proposal authority

`rust/online_protocol/src/matchmaking_model.rs` now validates complete owned
playlist/player models, original hopper/rank-map headers and complete variant
buffers. `matchmaking_coordinator.rs` owns admitted catalog snapshots, tickets,
leases, measured connection records, reservation and readiness maps, matching
stages, bounded legacy assembly, host selection and both RNG streams. Immutable
Python carriers reach that owner through private typed ABI handles. Gathering
fit checks and final proposal creation also reach Rust; Python cannot insert
or overwrite a ticket, proposal or ready set.

Every ticket is pinned to the complete admitted profile hash. A catalog change
under the same textual revision rejects idempotent enqueue, lease renewal and
proposal refresh until a fresh admission. Rust validates distinct ticket IDs,
compatibility, original team assignment and full selected-row equality before
creating reservations. It binds the exact roster/profile/game snapshot inside
the owner. This local binding is not the complete end-to-end GAP-11 envelope:
map revisions, executable and historical identities, authority generations,
statistics namespaces and result consumers still require closure.

The native scheduler/host stream is now an explicitly versioned modern v2 LCG,
separate from the original hopper-draw stream. Bootstrap entropy is the only
Python RNG use in the queue adapter. Legacy bounded subset assembly, reciprocal
published-table stages, lease intervals and measured route host ranking remain
native replacement policies. Their Rust implementation is not recovery of Live
scheduling, original host arbitration or upstream retail RNG ownership.

The whole gathering/query registry, query and advertisement producers, platform
identity/seat consent, API handlers, session/world transitions and transactional
rank/results ownership remain pending. All twelve gaps, all 53 owner IDs and
G01-G12 remain open. No launcher-selected runtime or remote service is changed
by this increment.

The [queue validation receipt](evidence/matchmaking-rust-parity-20261028/coordinator-validation.json)
records **23 passing Rust checks and 430 passing backend tests** in one frozen
cohort, including the actual service ABI and HTTP shutdown. Its manifest still
matches the source at receipt time. Five new native authority tests accompany
the eighteen retained original-fixture regressions; this increment does not run
new original executable controls or claim authentic service inputs.

A failing native regression reproduced an outsider acknowledgment selecting a
host before returning403. Rust now validates membership before refresh, so the
rejected acknowledgment cannot draw a host or mutate readiness. Additional
checks cover forged rows, duplicate reservations/acknowledgments, same-revision
settings changes, failed assignment RNG preservation, private-owner retirement
and response-buffer cleanup. Renewal/cosmetic tests now compare the complete
ticket value and ID; Python object identity is no longer authoritative.
All eight owned scratch/build trees were verified removed. Only small reports,
commands and source/library hashes were retained; no new large artifact remains.

## Rust gathering and query authority

The complete native gathering owner now lives in
`rust/online_protocol/src/matchmaking_gatherings.rs`, inside the same Rust
coordinator as tickets and proposals. It owns session/party/query registries,
birth and growth clocks, lifecycle jitter, ordered query membership, candidate
banks/cache, mapped-rank advertisement projection, clan/language fields, probe
allocation outcomes, failed-session memory, radius/exhaustion transitions,
selected joins, founding game preservation and proposal publication.

The Python gathering module is a temporary typed IO adapter with immutable
snapshots. Rust requests specific transport facts and probe allocations through
private, monotonically generated IO continuations. Python reads the existing
authenticated QoS owner and executes only those requests. Wrong/replayed
continuations and malformed replies cannot advance current work. Cancellation,
expiry or profile changes retire Rust registry entries and saved search views;
old views cannot mutate a requeued party. Read-only progress does not allocate
queries, consume RNG or advance growth clocks.

This is a native projection boundary: it supplies the existing relay median
and `(0,-1)` unknown quality fields to the original Rust candidate core. It does
not recover Xbox XNet measurements, original platform reservations/NAT, complete
original summary validity/capacity producers or the retired Live procedure.
Birth/private-ID ordering and50-record truncation remain an explicitly named
modern provider. Original/default and dated downloaded inputs remain distinct.

Synthetic registry/clock restoration is compiled only with the
`matchmaking-test-inputs` feature. The backend harness enables that feature for
old numerical/fixture input setups and records it. Its `--production-abi` mode
omits it and exercises genuine service entry paths, including the actual
three-sample HTTP/socket probe. Docker and CMake release routes do not enable
that diagnostic feature. Neither test build is installed or deployed here.

The API, matching-related authenticated seats/guests/parties/clans, QoS sensor
and transport policy, persistent ranks/XP/results/finalization, full admission
chain and native played/delivered acceptance still require the remaining goal
work. The original/service evidence requirements and all12/53/12 scope records
remain open.

The [gathering validation receipt](evidence/matchmaking-rust-parity-20261028/gatherings-validation.json)
records **26 passing Rust checks and436 passing backend tests** with diagnostic
fixture inputs enabled. A separate default-feature build passes **four Rust
checks and14 focused backend tests**, including the genuine three-sample
HTTP/socket probe and rejection of diagnostic restoration. Both configurations
share the same tested matching/lifecycle/adapter source hashes. The default
library and compiler hashes are retained; the full-suite manifest also records
a later harness-only change that added automatic compiler/library reporting.
All six owned scratch/build trees were verified removed. No generated binary,
bank, frame or other large artifact was retained. These are source/service
component results, not native gameplay or deployed activation proof.

## Rust result, XP and transaction authority

The queue-derived result admission, physical-owner electorate and report state
now live in `matchmaking_results.rs`. Rust freezes the exact admitted game and
profile binding, ordered players, guest/connection/clan ownership, configured
stored skill snapshots, opaque per-room identities and statistics namespace.
Python room projections cannot rewrite those facts or manufacture votes.
Every selected304-byte variant is compared in full with the PCR. The39 original
preset identities remain authenticated separately. Unfamiliar names without
settings are rejected. The existing native Fiesta Slayer/Infection Slayer assets
have exact legacy registrations; they are authored extensions and their complete
extension/executable attestation remains open.

`matchmaking_skill.rs` owns the authenticated retail/TU1.5 executable-default
rank tables, signed pair/player/clan XP arithmetic, clipping, strict midpoint
hysteresis and separate comparison/persistent/public domains. The fresh
[original replay](evidence/matchmaking-rust-parity-20261028/results-original.json)
executes1,912 complete original calls and retains1,910 output controls, covering
1,266 retail and1,356 TU1.5 distinct instructions. Every executed instruction
byte matches the pinned executable. Rust matches every retained control. These
synthetic comparison/result records do not establish the original production
comparison-skill producer or downloaded profile. The native result input policy
remains explicitly named persistent-raw; that original producer chain is open.

Rust also owns consent, duplicate/retry and quarantine decisions. An admitted
physical connection can vote once for the frozen digest; departed owners retain
votes already cast. Unconfirmed departed owners cannot acquire consent through
transport retirement. Conflicting reports/digests quarantine an uncommitted
result. This is declared native unanimous arbitration, with no claim about the
retired Live service or independent simulation truth.

`matchmaking_persistence.rs` owns skill schemas/migrations/snapshots, generated
SQL, read/compare/write guards and private continuation generations. The temporary
`native_database.py` executes those statements on the existing SQLite connection.
One transaction covers ledger, immutable admission/finalization envelope,
player/clan XP, guest-filtered approved PCR fields, public aliases and profile
outbox revisions. A shared SQLite write lock serializes concurrent completion;
current skill/configuration rows must match the frozen snapshots. Schema DDL is
emitted individually so implicit script commits cannot split migration rollback.
The result namespace is private `native-matchmaking-v1`. The full native
executable/map/authority chain remains unclosed.

The [validation receipt](evidence/matchmaking-rust-parity-20261028/results-validation.json)
records32 passing Rust checks and440 backend tests in one frozen cohort, plus11
Rust checks and35 focused backend tests against a separate default-feature ABI.
The default ABI rejects synthetic result-admission and accuracy overrides.
Checks include two real SQLite connections completing exactly once; actual SQL
and schema failures rolling back all rows; personal/clan skill drift; original
consent/departure/retry/security cases; all 304 option-byte mutations across seven
synthetic family fixtures; and mutation of Python room projections. The CMake
Rust bridge build and helper test pass. Both final source manifests match the
working tree; compiler/library identities and bounded diagnostics are retained.
All16 named scratch/build trees were verified removed; no generated large
artifact was retained. The new411,945-byte original fixture is a required
reference input.

The existing service executes result control and the complete synchronous SQL
pump on one event loop. SQL data races are checked; concurrent/distributed
dispute mutation during a future parallel API/executor is not qualified. Consent
is frozen when the private database object is created. Pending-result recovery
after process restart is also open. The authenticated catalog/API and matching
seat/party/clan rules, QoS IO policy, gameplay room/world transitions, original
standings/wire/arbitration, every required historical profile and full native
played/delivered acceptance remain goal work. Python transport/SQLite IO and
public history reads are temporary. All 12 gaps, 53 algorithm IDs and12 acceptance
gates remain open; no runtime/service activation or selected-launcher change
occurred in this increment.

## Rust catalog, profile and playlist API authority

`matchmaking_catalog.rs` now interprets the supplied versioned JSON catalog and
owns its metadata registry, named policy/default normalization, original retail
clock aliases, client compatibility, common-map/guest/size/clan eligibility and
public playlist response. `matchmaking_json.rs` rejects duplicate decoded keys
(including escaped aliases), invalid Unicode/nonfinite data and trailing JSON
at the native carrier boundary. The Python loader reads UTF8 file bytes and
projects the Rust result into immutable carriers. Forced edits to those Python
snapshots do not change the admitted catalog.

The authoritative profile now includes catalog metadata, client requirements,
rank configuration and its authenticated table SHA, alongside the full queue
policy. Ticket, gathering, query, pending-IO and proposal owners all use that
same identity. Proposals retain the complete profile material and catalog SHA
in their private admission binding. A same-revision metadata/rank change cannot
renew or silently reuse an earlier ticket. Invalid replacements fail before
changing the catalog or current ticket. Result admission also requires the
catalog's selected personal/clan rank configuration; caller-supplied room
configuration cannot override it.

Rust builds the public playlist payload and decides which public rank reads
are needed. The actual Rust catalog selects configured player/clan XP or the
legacy display store, and the Rust transaction planner performs the storage and
display projection. Hidden casual rank stays absent from the public payload;
missing accepted-clan membership yields no level and does not commit a caller's
transaction. Personal/clan table selection no longer depends on mutable Python
catalog objects. The existing native client/continuation gates are declared
modern compatibility policy, distinct from OG Xbox Live protocol evidence.

The initial enriched pin exposed a real integration mismatch: gathering/query
contexts still stored the policy-only hash. Actual Lab/couch/full-option HTTP
admission failed with `Gathering IO retired`. All contexts now use the shared
catalog/profile pin, with direct Rust finalizer and pending-IO regressions plus
the real HTTP admission cohort. The [catalog validation receipt](evidence/matchmaking-rust-parity-20261028/catalog-validation.json)
retains that bounded failing diagnostic and the final successful manifests.

The final frozen source cohort passes39 focused Rust checks and all444 backend
tests. A separate default-feature ABI passes17 Rust checks and50 focused backend
tests, including real probe/relay, ranked/hidden-casual/clan result transactions,
metadata replacement and the existing diagnostic-entry rejection. Existing
pinned original numerical fixtures are replayed; this increment adds no original
executable run, authenticated historical download or original service capture.
The CMake Rust bridge/helper check passes with a retained small archive/worker/
lock identity report. Final source manifests match the worktree and all10 named
private scratch/build trees were verified removed. No generated large artifact
was retained. This work installed/deployed/restarted no runtime or service and
did not modify launcher selection.

This is a catalog/public-playlist API ownership increment. Session bearers,
local-seat/party/clan consent and request/failure state, remaining authenticated
endpoint policy, QoS IO policy and gameplay room/world transitions still require
Rust ownership. Whole original result/summary/service producers, historical
profiles, pending-result process recovery, concurrent dispute/SQL IO and full
native played/delivered acceptance remain open. All12 GAP records,53 algorithm
IDs and G01-G12 requirements retain their complete scope.

## Rust authentication session, local-seat and request authority

`matchmaking_sessions.rs`, a private child of the Rust coordinator, now owns
native authentication sessions separately from persistent parties, temporary
search/gathering sessions and running-match capabilities. This follows the PDF's
section 9 ownership boundary; the 900-second lease, bearer scheme and client gates
remain declared modern service policy, not recovered Xbox Live authentication.
Rust parses session and queue bodies; validates title language, map capabilities,
31-UTF16-unit names, exact nine-byte cosmetic profiles and up to four local seats;
and issues opaque tokens with stored SHA256 hashes and private session generations.

Provider verification and shared account persistence remain trusted identity IO.
Every primary and signed local grant completes before any account/profile write.
Rust receives the verified provider identities, canonical account lookup and
accepted-party/current-room facts, rejects duplicate linked aliases, foreign PC
seats, active foreign room owners and signed locals in separate persistent
parties, then rechecks resolved canonical identities at admission. Guests receive
Rust-generated transient IDs and never become persistent accounts. A failed
local grant or rejected preflight preserves the existing account profiles.

Fresh verified renewal preserves token, generation, guest IDs, queue request,
ticket and party consent only when the physical roster/name/maps/protocol/language
are unchanged. Cosmetics can refresh within that same identity. Changed
connections rotate the session and retire their old queued state. Expired or
foreign renewal fails. Native time is monotone; HTTP expiry regressions advance
the input clock instead of mutating a Python lease.

Rust now owns the session-bound request UUID registry, 64-request limit, replay
checks, full-profile fencing and terminal failure storage. Whole-party preflight
uses live registered generations, continuation/client requirements, guest policy,
size and common maps before skill database IO. Final enqueue rechecks generation
and every ordered ID/name/guest/map/physical-owner fact atomically with ticket and
request publication. The temporary party service still supplies accepted roster
and consent facts; it is not yet the final Rust party owner.

Revocation/expiry clears requests and failures and cancels queued tickets,
reservations and gathering/query/IO continuations in Rust. Retirement events
request socket/QoS and persistent-party notifications from Python. A previously
admitted running match retains its separate capability. Rust generations prevent
old session handles from enqueueing or overwriting a replacement's failure state.
`native_sessions.py` exposes read-only session/request projections and a typed
failure adapter; its weak projection cache preserves socket/room object identity
without storing an independent admitted registry. Production libraries reject
the diagnostic-only expiry restoration operation.

The broader clan result cohort caught a request-order regression: malformed
queue fields reached a live-room conflict before schema rejection. Rust now
validates the queue body before authentication/room policy, restoring the exact
400 response without reintroducing Python schema decisions. The retained
[session validation receipt](evidence/matchmaking-rust-parity-20261028/sessions-validation.json)
identifies both failing cohorts and the corrected source manifests. The final
frozen source passes 47 Rust checks and all 450 backend tests; the separate default
ABI passes 25 Rust checks and 102 backend tests. Both 226-file manifests match the
current source. The CMake Rust bridge/helper also passes, with retained small
archive/worker/lock identity metadata. All 12 named private scratch/build trees
were verified removed; no generated large artifact was retained.

This increment adds modern session ownership, not original executable auth
proof or complete API migration. Accepted-party/clan consent and persistence,
retirement IO acknowledgement/retry, remaining poll/cancel/content-failure and
QoS sensor policy, match rooms/countdowns/world continuity, original input/result/
service producers, historical profiles, pending-result recovery and the entire
native played/delivered acceptance matrix remain open. All 12 GAP records,
53 algorithm IDs and G01-G12 gates retain their original completion requirements.
No runtime or service was installed, deployed or restarted; launcher selection
and original assets remain unchanged.

## Rust accepted-party consent and queue admission

A fresh HTTP reproduction found a real admission gap: a remote accepted member
could consent, the server could replace catalog metadata at the same visible
revision, and the leader's new search still returned 201 using the earlier consent.
The old check compared only playlist ID/revision and accepted-party version.
`matchmaking_parties.rs` now owns the matching consent and queue admission rules,
and the same request rejects stale consent with 409 until a fresh Ready action.
The public party readiness projection also hides consent that no longer matches
its actual native profile/connection context.

Rust generates the consent schema/migration and SQL plans. Each persisted consent
binds accepted-party ID/version, the member's opaque membership handle, playlist/
revision, complete catalog/rank-table profile pin, bearer hash, session generation
and a hash of the connected roster. The roster includes every physical owner,
signed local/guest ID, display name, installed map set, protocol and title language;
clan queues additionally bind accepted clan identities. Cosmetics, observation
clocks and lease refresh do not replace this identity. The leader selects the
current roster when starting; remote consent must match that complete roster.
A remote connection replacement cannot reuse another member's consent even
before the directory's retirement notification is applied.

Ready writes compare the accepted-party revision/leader and member handle before
updating both the legacy presentation fields and native consent record inside the
caller's SQLite transaction. A trigger-induced failure rolls both back. Startup
preserves accepted membership while clearing search consent, including a guarded
migration from the earlier consent-storage shape. Rust also generates accepted-
party change/consent invalidation and activity-link revision plans. The generic Python
adapter executes statements and enforces Rust's expected affected-row counts;
it does not choose their mutation rules.

Rust selects the accepted leader and ordered physical roster, requires current
remote consent and live connections, applies party size/build/guest/map/clan
rules, and checks active match/clan observations. Actual HTTP enqueue uses a
private Rust permit bound to request UUID, actor, complete facts, profile,
ordered session generations and the existing proposal lease. Final publication
rechecks that permit and every admitted seat before atomically adding the ticket
and request. Changed facts/profiles/requests, expiry and retired generations reject
publication. The proposal's private admission now carries ordered party bindings,
which the existing Rust result owner freezes in its source binding. Public views
keep those private IDs, bearer hashes and generations hidden.

The new directory-seat operation supplies the signed subjects needed for clan
lookup in one bounded Rust request. Party IO no longer walks every live session
through separate ABI calls. Existing original assignment, rating, candidate,
lifecycle and XP fixtures remain the numerical references; no original Live
party/authentication or historical-service procedure was invented or recovered
in this increment. These are explicitly native authorization rules.

The [party consent validation receipt](evidence/matchmaking-rust-parity-20261028/party-consent-validation.json)
records the stale-consent red/green reproduction, whole-roster/profile/permit
checks, actual SQL failure rollback, restart/migration, preserved countdown and
clan/private-rank behavior, full/default ABI cohorts and CMake component identity.
An integration correction preserves the leader's valid own Ready presentation
when another accepted member first opens matchmaking; remote consent still binds
the whole current roster. Counts, source hashes and scratch cleanup are retained
in that receipt rather than inferred from earlier increments. The final frozen
229-file source passes 52 Rust checks and all 455 backend tests. A separate default
ABI passes 30 Rust checks and 128 backend tests. Both manifests match current
source. CMake bridge/helper checks pass, all 13 named private test/build trees
are removed, and no generated large artifact is retained. The restart control
reconstructs the service and native owner against the same SQLite database within
one test process; crash/distributed recovery is not qualified by that control.

At that increment, accepted membership/invitation/social/clan CRUD and much of the party HTTP API,
party countdown publication state, remaining enqueue skill-IO planning, poll/
cancel/content-failure policy, QoS sensors and running room/world transitions
still need Rust migration. Directory and room observations remain trusted IO;
future parallel/distributed mutation between fact reads, SQL and queue publication
has not been qualified. Conservative queue invalidation can precede a later
outer SQL rollback; it cannot grant consent, and transactional retry/notification
acknowledgement still requires completion. Full client-visible profile consent,
original producers and all native played/delivered G01-G12 gates remain open.
All 12 gaps and 53 algorithm-owner requirements keep their complete exit criteria.

## Rust pregame countdown publication

An actual three-member HTTP regression exposed a difference in member requests.
After member B publishes the mode-1 ten-second tuple, member C requests the same
tuple 900 ms later. Python replaced the requester, serial and clock. Both original
writers leave the tuple unchanged, preserving B and the earlier clock; the next
ordinary publication occurs at 1,000 ms from B's request. The regression now passes
through the Rust owner and real HTTP endpoint.

`party_countdown.rs` now implements the complete original parameter writer in
addition to the existing Start/timer leaves. It preserves the nonleader active
flag, signed watermark comparison, tuple equality that excludes requester,
mode/requester clearing and wrapping update counter, including two increments
when both clearing and tuple publication occur. The fresh
`tools/oracle_matchmaking_countdown.py` executes the complete unchanged 243-byte
retail `5DF30` and TU1.5 `5EC20` bodies. All 68 instructions execute and every
executed byte matches the pinned image. The 1,536 calls produce 768 paired output
controls, matched by Rust; the retained 40 timer/press/parameter controls also
remain covered. Inputs are synthetic session fields/caller operands. This does
not recover upstream original caller/Live/network/framebuffer provenance.

`matchmaking_party_countdowns.rs` owns the live registry, serial and publication
generation, original clock arithmetic, Start/shorten/member-request/cancel rules,
public requester projection, command replay/SQL plans and zero consumption.
Unchanged tuples keep the clock and serial; stalled pumps decrement once; signed
32-bit elapsed and wrapping clocks are preserved. Consumed zero retains its
complete serial until a new countdown starts. Profile, accepted-party version,
connected roster and session generations fence live and prepared state. Retired
connections, changed profiles, party effects and stale prepared publications
cannot publish a countdown. An unchanged Start at zero cannot restart it.

Rust stages the publication and emits party/member CAS plus request-ledger SQL.
The generic adapter commits that transaction before publishing the native
continuation; a trigger-induced SQL failure leaves the live countdown unchanged
and the same request can retry. Native commit rejects changed facts, publication
generations, profile/roster or expiry. Cancel also retires an unpublished Start.
Python supplies bounded clock/directory/room observations and executes statements;
its weak projection cache is read-only. The standalone numerical sample API has
no live restoration path, including in the default production ABI.

The [countdown validation receipt](evidence/matchmaking-rust-parity-20261028/countdown-validation.json)
retains the red/green reproduction, original inputs/outputs and hashes, frozen
full/default ABI test manifests, CMake component identity and scratch cleanup.
The final frozen 234-file source passes 59 Rust checks and all 462 backend tests;
the separate production ABI passes 37 Rust checks and 137 backend tests with
diagnostic restoration disabled. Both source manifests match current source.
The CMake bridge/helper check passes; its generated archive, worker and libraries
are removed with the private build. The required original output fixture is
277,109 bytes at `tests/fixtures/matchmaking_countdown_writer_original.json`,
SHA-256 `0fb4d1f349e341a834ecb5cab320cc8ca9bd1a10e421c4a97d3c20d178d32d92`.
All named private test/build trees are removed, with no generated large artifacts
retained.
Service reinitialization preserves accepted membership and resets ephemeral
countdown/consent state in one test process. That control does not qualify an
OS-process crash or distributed recovery. In particular, crash/retry recovery
between the SQLite commit and native publication remains open. Clock and room
observations remain trusted IO, and future parallel fact/SQL/publication
coordination still requires qualification.

Accepted social/clan CRUD, remaining endpoint/skill-IO/poll/cancel/content-failure/
QoS and running room/world policy still require Rust migration. Complete original
producer/history/caller recovery, native played flow and actual delivered
client/library/service/catalog identity remain unqualified. All 12 gaps, 53
algorithm IDs and G01-G12 requirements retain their complete exit criteria;
this component increment closes none of those full gates.

## Rust queue endpoints and admission-bound failures

The destroyed-room content retry had an actual stale-admission bug. A first
launch closes, the same authenticated sessions start and fail a second launch,
then a missing-content report arrives for the destroyed first room. Python's
recent-room branch accepted it with HTTP 200 and overwrote the second failure;
it compared session object identity without retaining the failure's admission.
The Rust owner now rejects that older report with 409. Current-room reports and
current destroyed-room retries still work within the existing native window.

`matchmaking_queue_api.rs` owns queue endpoint decisions and continuations:
enqueue skill-store selection, signed-local roster construction, guest raw-skill
inheritance, clan search rank, ordered seat publication, request replay, poll
heartbeat, queue advancement, legacy/optional response shape, ranks, appearance
selection, cancellation and content-failure interpretation. It reuses the actual
Rust catalog/session/party/gathering/queue owners. Enqueue continuations bind
the current complete profile, accepted facts, request, native session generation
and party permit. Changed profiles, generations, facts, expiry, malformed skill
results, abort or replay cannot publish a ticket or request.

Rust chooses all skill and appearance database operations. `DatabaseBank` runs
the bounded snapshot plans within one SQLite transaction and exposes a monotonic
continuation sequence across its children. A real signed-local migration failure
on the second skill read rolls back the first profile's initialization, publishes
no ticket/request and leaves the original draw seed unchanged. The same request
then succeeds after the stored migration condition is corrected. Appearance
overrides come from the current native seat; guests retain their native identity
and public rank rules. Public queue views keep raw skill, account/session keys,
proposal IDs, roster bindings and continuation frames private.

Rust freezes failure participants and physical party groups from the admitted
proposal. It owns local cancellation exclusions, failure provenance, the latest
room for each generation, and the bounded 15-second destroyed-room cache. New
searches clear prior outcomes. An older room cannot overwrite a newer admission
on the same connection. Current party content failures affect that physical
party; other parties receive search failure. Current failure/session ownership,
phase, active-room observations, ticket absence and expiry guard retries.

The Python HTTP layer now marshals bodies, query/accepted-directory/room facts,
executes Rust-requested SQL/gathering/start-observation IO and formats the native
response. Room failure/close hooks forward phase observations to Rust; they hold
no independently admitted failure cache. The room's start-readiness/clock producer,
world/host/transport state and effects remain separate migration boundaries.
Calling that existing start producer is trusted IO in this increment, not proof
that all room/countdown policy has moved into Rust.

The [queue endpoint validation receipt](evidence/matchmaking-rust-parity-20261028/queue-api-validation.json)
records the stale-room HTTP red/green result, native continuation/roster/privacy/
failure controls, real snapshot transaction rollback, retained original numerical
cohorts, frozen full/default ABI and CMake component identities and scratch
cleanup. These are declared native transport/storage policies. This increment
adds no original Live procedure, historical download, unchanged-original call,
rendered/played match or deployed client/service qualification.
The frozen 237-file source passes 65 Rust checks and all 464 backend tests; the
separate production ABI passes 43 Rust checks and 142 backend tests with diagnostic
restoration disabled. Both source manifests match current source. The CMake
Rust bridge/helper check passes. All owned private test/build/report-helper
trees are removed; the queue-API scratch root is empty and no generated large
artifact is retained.

Accepted social/clan/party CRUD and remaining social/presentation policy, QoS
sensors, running room/start/host/world policy, cross-store crash/distributed
coordination, complete original producers/history/callers and actual native
played/delivered acceptance remain open. All 12 gap records, 53 owner IDs and
G01-G12 full requirements retain their complete exit criteria.

## Rust room setup, readiness, QoS and start clock

The actual loader transition had a timing gap: two matching authenticated
`loaded` reports moved the service to `running` with five seconds remaining on
the original start clock. The HTTP regression now remains in `loading`, publishes
5,000 ms and enters `running` only after both the matching data and an observed
clock zero. A separate HTTP control uses the real service clock for the full ten
seconds. Component gameplay/result helpers explicitly advance the trusted clock;
they retain the actual gate and are not natural-time native gameplay evidence.

`matchmaking_room_setup.rs` freezes the admitted seat/physical-owner/team/protocol
roster and owns the bounded RTT history, direct report validation, reciprocal
peer masks, original connected-team predicate, relay/direct score production,
physical upload/loss/RTT host ranking, readiness acknowledgements, shared start
clock and loading/running setup decisions. The Python room executes the returned
effects and exposes read-only sample/measurement/loaded/clock projections.
Authenticated socket/challenge and timestamp observations remain trusted IO.
The setup capability survives retirement of its separate account session.

Host scoring preserves the existing declared native policy: relay peers use the
two three-sample medians; direct peers use the worse reciprocal RTT. Measured
hosts rank by worst loss, bottleneck outgoing upload, mean reciprocal RTT and
physical slot. A missing reciprocal upload/loss column cannot silently fall back
to unmeasured host ranking. Guests and signed local seats retain their actual
physical owner and representative slot. These native measurements/ranking are
not recovered original XNet QoS units or the unrecovered original host comparator.

`matchmaking_start.rs` implements the original zero-clock sentinel, wrapping
32-bit elapsed/subtraction, signed nonnegative remaining value and signed
wrapping ceiling arithmetic. Fresh unchanged TU1.5 execution supplies 512 edge
and deterministic clock controls, plus the earlier 16 timer/dispatch/init/clear
controls: 528 outputs over 1,572 bounded block invocations, covering 52 distinct
instructions with every executed byte verified. A fresh readiness replay covers
12 predicates, four policy reads, four policy producer/consumer controls, ten
caller routes and constructor deferral clearing; 498 distinct instructions execute
with every executed byte verified. Inputs are synthetic. The initial host-leave/platform transaction,
complete start owner, original network publication/mode-change callees and Live
policy provenance remain outside this proof. Replacement authentication never
supplies an invented original bit31/logon/override value.

The [room setup validation receipt](evidence/matchmaking-rust-parity-20261028/room-setup-validation.json)
records these original controls, source/fixture hashes, early-loader red/green,
real-clock HTTP, native producer/transition tests, frozen full/default ABI cohorts,
CMake component identity and scratch cleanup. Ordinary current-client countdown
values are qualified in the 0..10,000 ms range. The existing C++ parser/display
guard still bounds that range; the signed/wrapping outlier producer outputs are
numerical Rust evidence and do not establish complete native-client acceptance
of those outliers. The unused C++ elapsed helper's unsigned sentinel test is
also not evidence for the newly replayed full source arithmetic domain.

High-level review/postgame, migration/departure, socket/bearer/effect dispatch and
actual world/host continuity still require Rust ownership and native acceptance.
Search QoS sensors, party/social/clan CRUD, cross-store/parallel/crash recovery,
historical/service/original producer gaps and selected/deployed delivery remain
open. This increment does not qualify a native played game, running-world import
or original-hardware presentation. All 12 gaps, 53 owner IDs and G01-G12 full
criteria remain intact and open.

The frozen 242-file source passes 70 Rust checks and all 467 backend tests.
The separate production ABI passes 48 Rust checks and 174 backend tests with
diagnostic restoration disabled. Both manifests match current source. CMake
Rust bridge/helper checks pass. All owned private scratch/build trees are removed,
with no generated large artifact retained. The required new numerical reference
is `tests/fixtures/tu15_matchmaking_start_original.json` (61,441 bytes, SHA-256
`2580f93b33c2701f23e06e7455ca46df0edde5f2c5fc942fa2176b2c7294fc61`).

## Rust legacy-relay complete-world coordination

`matchmaking_continuity.rs` replaces the authoritative Python v11 relay state
machine. The admitted setup owner creates it after the loader/source-zero gate,
using its frozen physical seat owners, selected host and measured quality. The
Python `PlayerHostedContinuity` view attaches to that exact room owner; standalone
Rust objects exercise the same numerical state machine and cannot be installed
through the live room API. Neither constructor accepts a restored snapshot.

Rust owns the active physical peers and whole couch-seat member mask; monotonic
u32 authority generations; immutable completed-tick seals and at most64 retained
entries; standby proofs; latest-world-only candidate admission; pinned-world
retry ordering; acceptance; surviving rebinds; bounded failure metadata; and
deadline/expiry rules. Exhausted generations fail without partial departure.
Malformed controls are rejected atomically, old generations remain benign and
the late previous-epoch acknowledgement exception is limited to awaiting-world.
Late seals/acks after a submitted report are suppressed using Rust result state.
Packet and expiry readers use a small projection that excludes proof history.

A fresh failing HTTP/storage regression reproduced a separate provenance bug:
the private result database bridge accepted an arbitrary `epoch=1234` supplied
by Python. The bridge now rejects that input before SQL. Finalization instead
reads the current generation from the Rust room owner. For a v11 result, an
absent/non-running continuity source is an explicit error. Unanimity, admission,
quarantine, namespace and exactly-once storage policy remain in their existing
Rust owners; world proofs do not count as result votes.

Election by observed milliseconds then physical slot, the15-second deadlines,
the central relay protocol and its failure handling remain **declared native
policy**, without a recovered-original ordering claim. The unchanged numerical
start/readiness and other prior original evidence are reused; this increment
executes no new original host-election oracle. Test world digests are declared
fixtures. They do not establish preservation/import of the actual object,
objective, role, clock or player world. The separate direct-peer Rust agreement
and native engine handoff paths retain their own authority and acceptance.
Python still owns high-level review/postgame and departure/socket/bearer/effect
dispatch; original host recovery, full native played/delivered qualification,
remaining policy migration and all G01-G12 gates remain open.

Validation and cleanup are recorded in
[continuity-validation.json](evidence/matchmaking-rust-parity-20261028/continuity-validation.json).
It includes the failing arbitrary-epoch case, same-owner/read-only HTTP controls,
60 uniform grouping controls spanning2..16 slots and1..4 seats per connection,
latest-ack races, candidate/rebind
timeouts, immutable/bounded proof history, strict generations/operands and the
separate production ABI. No game runtime is activated by these component tests.

The final frozen246-file cohort passed160 Rust unit tests,77 Rust integration
tests and471 backend tests. The separate default-feature production ABI passed
160 Rust unit tests,55 integration tests and193 backend tests. Two existing
capture/performance unit tests remain ignored. The CMake Rust build/helper
passed with two build jobs and one test job, and its small build identity report
is retained. All ten named private test/build trees and the owned scratch parent
were verified empty/removed. Only46 small reports (566,105 bytes) are retained;
no generated large artifact, installed runtime or live service was changed.

## Rust room lifecycle, result authority and departures

`matchmaking_room_lifecycle.rs`, a private child of the setup registry, now owns
the admitted room's phase, presentation/PCR authority, connection/loading/running/
review/postgame deadlines, active physical owners, departure slot mask,
negotiated-handoff flag and recorded/unrecorded completion state. Python exposes
read-only projections and executes native effects. Clock injection in component
tests supplies IO observations, never installs phase/deadline snapshots. Native
room clocks cannot be advanced by account/queue expiry clocks. Expired rooms
reject a report before a writer/cleanup task has run; closure remains available.

A fresh failing regression demonstrated that the old result adapter accepted a
valid nonhost PCR even though the transport rejected that publisher. Submission
and confirmation adapters now enter the Rust room authority/phase guard, followed
by the existing immutable Rust result/admission owner. The actual socket route
uses the same operation. Native result ownership checks precede report mutation.
Review completion requests generic database IO only when full admission consent
permits it. Successful commit is required before a recorded postgame grant;
SQL failure leaves review pending and the same finalization can retry exactly
once. Surviving-owner presentation never becomes a missing original owner's vote.

Rust distinguishes direct master-TCP loss from explicit authenticated leave.
TCP loss preserves UDP admission. Explicit direct-host departure retains the
existing negotiated-handoff or last-opponent GAME OVER conditions, without
inventing a gameplay successor. Legacy relay loss enters the Rust complete-world
owner; prestart loss closes because no completed world exists. Postgame socket
loss selects only a presentation/voice host. Departure masks, result revocation,
voice membership and rejection/closure dispositions are native decisions.
Socket identity/replacement observations and actual network effects remain IO.
Cleanup cancels only the frozen admitted ticket IDs, including when Python's
proposal projection has changed.

Tests that formerly assigned Python phase/deadline/closed fields now deliver
actual loaded/report/consent/departure events or trusted clock observations.
An existing history rollback test now reaches running before reporting. New
controls cover adapter host fencing, read-only lifecycle projections, failed SQL
and retry, expiry before writer cleanup, independent queue/room clocks, native
deadline arithmetic, direct TCP loss versus departure, and unrelated-ticket
preservation. Validation and private scratch cleanup are recorded in
[room-lifecycle-validation.json](evidence/matchmaking-rust-parity-20261028/room-lifecycle-validation.json).

The final frozen248-file cohort passed160 Rust unit tests,80 integration tests
and476 backend tests. The separate production ABI passed160 Rust unit tests,
58 integration tests and212 backend tests with restoration disabled. Two existing
capture/performance unit tests remain ignored. The CMake Rust build/helper passed
with two build jobs and one test job. All nine named private trees were removed
and the owned scratch parent is empty. The46 retained small reports total603,987
bytes; no generated large artifact was retained or runtime activated.

The lifecycle timeouts, PCR consistency policy and direct/relay transport remain
declared native replacements. No new unchanged-original execution is claimed by
this ownership increment. Full original host recovery, actual world/objective/
role/clock continuity, native played/delivered flows, remaining signaling/bearer/
packet/voice policy migration, service replacement and all full G01-G12 gates
remain open.

## Rust room capabilities, descriptors and peer signaling

`matchmaking_room_wire.rs` now owns the room's physical capabilities, opaque
256-bit bearer generation and trusted rotation, per-capability revisions,
checked socket generations, upgrade/reconnect policy, EOS pair/socket material,
room identity proofs, descriptors and signaling state. Actual public descriptor
names/teams/slots/guests/identities, protocol fields and selected EOS session
identity come from frozen native admission rather than mutable Python views.
Python exposes read-only capability/identity projections and owns socket handles
and generic provider/signing/storage IO. No caller-chosen bearer or snapshot can
be installed into the native owner.

A fresh actual HTTP regression reproduced an expired room completing WebSocket
upgrade before writer cleanup. Rust now rejects it before the upgrade. Admission
is revalidated after the upgrade await; two preflight checks cannot both attach
the same connecting peer. Direct running reconnects receive a new generation.
An old reader cannot signal, drain or detach its replacement. Body/provider
callbacks revalidate the same native capability revision. Provider account/PUID
and immutable room binding checks precede provider storage and native proof
publication. A physical recipient needs a verified room proof before publishing
an account-authenticated EOS identity. Native socket feature grants also fence
the lifecycle's negotiated-handoff observations.

Rust owns physical signaling destinations, direction/stage/protocol/payload
validation, per-pair65-message budgets, single SDP descriptions, EOS identity
uniqueness, retired-endpoint handling and at most512 pending frames per owner.
Validation/capacity rejection does not partly consume a counter or identity.
Pending snapshots expose counts rather than payload copies. Attach publications
and pending data are separate: batches of at most16 bound each private response,
and the IO queue applies backpressure. Retirement removes queued traffic both
to and from that physical owner. A16-seat fixture queues worst-case escaped SDP
payloads, retires a sender and verifies that bounded drains omit that sender.

Room bearers now use64 hex characters for256 entropy bits; they remain opaque
credentials. Current native descriptor consumers accept up to128 characters
(`src/network/online_matchmaking.cpp`). This is a source/typed-IO observation,
not delivered-client or runtime qualification. The modern protocol namespace,
crypto/SDK providers and native transport remain distinct from original Xbox
authentication or wire recovery. Packet/voice policy, generic abuse budgets,
remaining endpoint/service replacement, cross-store/concurrent recovery, actual
world continuity and all full native played/delivered/G01-G12 gates remain open.

Validation, intermediate diagnostics, identity receipts and private scratch
cleanup are recorded in
[room-wire-validation.json](evidence/matchmaking-rust-parity-20261028/room-wire-validation.json).

The final frozen250-file cohort passed160 Rust unit tests,84 integration tests
and479 backend tests. The separate production ABI passed160 unit tests,
62 integration tests and215 backend tests with restoration disabled. Two existing
capture/performance unit tests remain ignored. The CMake Rust build/helper passed
with two build jobs and one test job. All eight named private trees were removed
and the owned scratch parent is empty. The37 retained small reports total483,569
bytes; no generated large artifact was retained or runtime activated.

## Rust room packet, voice and budget ownership

`matchmaking_room_packets.rs` now owns legacy relay framing, immutable loaded
native protocol checks, authenticated physical sender and logical recipient
roles, continuity epochs, postgame game-packet suppression, bounded reader
failure dispositions and queued delivery. A fresh actual HTTP regression
reproduced the legacy path forwarding protocol0x1d after loading0x1c; Rust now
rejects that mismatch. Loaded signatures must supply the bounded16-bit protocol
for every admitted transport. The postgame fixture which sent0x1c after loading
0x1b was corrected to exercise its actual admitted protocol.

The same private room owner generates one Ed25519 voice key per physical owner,
returns only that owner's signing seed with authenticated attach, projects the
public key across its admitted couch seats, verifies the complete room/epoch/
recipient/body proof and retires signing eligibility with physical membership.
The independent Python test signer checks compatibility with the production
Rust verifier. Python's production voice key registry and verifier were removed.
These use pinned `ed25519-dalek=2.2.0` and the existing H2VOICE2 envelope; they
are explicitly modern security policy, not recovered original Xbox voice.

Rust chooses privacy lookup subjects from frozen signed-local/guest ownership,
requires the exact admitted account pairs and builds the shared-device mask.
Guests inherit their physical owner's account subject. Python performs generic
account-service lookups. A stateless privacy plan is revalidated before route
completion; it does not consume another packet credit. Denied audio still
consumes its bounded speaker/destination budget. Queued frames carry source and
recipient connection generations; Rust rechecks generation, epoch, active
origin and current recipient privacy before delivery. Host delivery retains the
existing relay exception; incoming privacy checks still cover every recipient.
No mutable Python roster, packet key bank, route policy or live limiter remains
in this path. The separate budget fixture cannot install state in a connection.

`matchmaking_packet_budget.rs` keeps each connection's independent control,
game-destination and speaker/destination buckets. Limits remain the declared
native75-per-second/75-burst voice and1000-per-second/512-burst game/control
policies. Invalid keys/clocks do not allocate streams and regressing observations
cannot refill credits. Full16-seat codec/snapshot fanout remains a numerical
load fixture, distinct from native played-world acceptance.

Native tests cover protocol/role/bounds, exact burst exhaustion, malformed
privacy facts, cross-room/recipient/epoch/key/body voice forgery, stale queued
connection generations, continuity epochs and stalled readers. Actual HTTP
checks cover the reproduced mismatch, Python roster aliases, independent
signers, signed-local/guest privacy, postgame, result/requeue and relay voice.
Validation and intermediate diagnostics are recorded in the
[packet ownership receipt](evidence/matchmaking-rust-parity-20261028/room-packets-validation.json).

The final frozen252-file cohort passed161 Rust unit tests,90 integration tests
and482 backend tests. The same source cohort with diagnostic restoration disabled
passed161 unit tests,68 integration tests and242 backend tests. Two existing
capture/performance unit tests remain ignored. The offline CMake Rust build/helper
passed with two build jobs and one test job. All eight named scratch trees and
the report helpers were removed; the owned scratch parent is empty. Only small
reports were retained; no generated large artifact or selected runtime changed.

Network challenges/QoS sensors, remaining endpoint/service replacement, generic
account/provider/storage IO, parallel/cross-store recovery, original Live/wire
recovery, real engine world continuity and all full native played/delivered/
G01-G12 acceptance gates remain open. This increment claims no new unchanged
original execution, deployed activation or complete original parity.

## Rust room text controls, probes and publications

`matchmaking_room_control.rs` now receives the actual room text and applies the
bounded strict Rust JSON decoder before dispatch. A fresh HTTP regression sent
an invalid `loaded` field followed by an escaped duplicate of the same decoded
key. Python silently replaced the first value and the room reached running.
Rust rejects that body before loading admission changes. Nested duplicate keys,
extra root fields, non-object controls and unsupported operations are rejected
by the same native boundary. Text controls reach setup/signaling/continuity/
result/departure owners without Python choosing the matching operation.

Each Rust connection owns its challenge nonce, monotonic probe clock, sent time
and voice-block publication state. Probe nonces are one-use and retire with the
connection generation. The existing0.2-second connecting and5-second later
cadences and strict15-second deadline remain declared native transport policy.
An expired pong cannot clear the challenge, add a QoS sample or revive it through
a regressing observation. A late direct-transport pong closes the master socket
while preserving actual direct gameplay admission. Native presence queries
select immutable admitted seats and use the same probe and lease eligibility;
queries do not renew a challenge or activity. General population counting and
history presentation remain temporary Python readers.

Rust owns control/result/completion/voice publications, frozen socket-feature
filters, source/recipient generations, retirement, backpressure and failure
scope. Completed replies derive recorded/detail fields from native result state
and socket grants. Rust requests generic SQL finalization and publishes only
after the verified transaction continuation; failed SQL leaves review pending.
Departure publications use the resulting membership, and legacy microphone
observations omit a detached generation before IO removes the old handle.
Direct UDP microphone membership continues to follow active gameplay seats.

The wire owner records a bounded detached-generation receipt. Socket-loss
callbacks consume that receipt once; attach invalidates the old receipt and
retirement clears it. Stale callbacks cannot depart a replacement, consume its
nonce or advance its clock. Rejection and IO failure scope execute in Rust.
Python retains socket handles, clock observations, generic privacy/provider/SQL
IO and typed effect execution. Its room challenge fields, text decoder,
matching dispatch and recipient/detail policy were removed.

Native controls cover strict decoding, one-use probes, exact deadlines,
regressing clocks, stale connection callbacks, frozen presence eligibility and
legacy detached-voice filtering. Actual HTTP checks cover the reproduced
escaped duplicate, ignored Python challenge aliases and direct late-pong QoS/
membership behavior, alongside normal loading, continuity, postgame, result
transactions, voice and requeue. Population fixtures now consume declared native
seat observations; Rust and HTTP tests own challenge/lease validation. See the
[room control receipt](evidence/matchmaking-rust-parity-20261028/room-control-validation.json).

The final frozen254-file cohort passed162 Rust unit tests,94 integration tests
and485 backend tests. The separate production ABI passed162 unit tests,
72 integration tests and245 backend tests with diagnostic restoration disabled.
Its matchmaking/backend inputs match; a separate authority-archive file changed
between the frozen cohorts. A fresh current-tree production ABI passed165 unit
tests,23 room integration tests and18 HTTP/reader tests after that additive
archive change. The component receipt records every source version and excludes
that concurrent work from this increment's implementation claim.

The offline CMake Rust build/helper passed with two build jobs and one test job
before the separate archive edit. Two existing capture/performance tests remain
ignored. All ten named scratch trees and the report helpers were removed; the
owned scratch parent is empty. The50 retained source reports total620,228 bytes,
excluding the summary receipt. No generated large artifact or selected runtime
was changed.

No new unchanged-original execution or original Live challenge/ordering/voice
recovery is claimed. Full Rust service/endpoint replacement, remaining search
QoS sensors, social/party/clan CRUD, public readers, cross-store recovery, actual
engine continuity and full original/native played/delivered G01-G12 acceptance
remain open.

## Rust search QoS ownership

`matchmaking_search_qos.rs` now owns the authenticated native relay sensor in
the same coordinator as sessions, tickets and gatherings. Session generations,
private authorization and socket generations bind each endpoint. Actual socket
observations remain IO inputs. A second upgrade cannot replace an attachment
whose IO handle has not yet been published. Native session retirement clears
its peer and pair state atomically; delayed retirement and reader callbacks
cannot remove a newer connection.

A real authenticated guest regression exposed inconsistent pair keys: probe
allocation used physical owners while pending status used logical seats.
Rust now canonicalizes allocation, pending state, measurements and views through
the same admitted physical pair. The HTTP regression completes three actual
two-socket cycles and checks the median through root, guest and reversed queries.

Rust owns both unpredictable challenge legs, dispatch flags and timestamps,
three-sample medians, fixed collection deadlines, measurement leases, heartbeat
validation, strict control decoding, reply reservations and retry work. A reply
cannot complete a leg which IO has not dispatched. Native wakeups carry exact
socket generations and drain once, so queued legs do not wait for a polling
timer. A preceding heartbeat finishes before the route leg is timestamped.
Late pongs cannot revive expired peers or their measurements.

The bounded bank includes unfinished collectors as well as completed
measurements. Native limits cover whole pair rows, active probes, each endpoint,
queued legs and metadata pages. Budget refusal does not publish fictitious
pending work or debit just one endpoint. Partial collection explicitly schedules
bounded native retries. Capacity stress uses a declared internal synthetic bank;
it is separate from the authenticated HTTP and numerical original controls.

`search_qos.py` retains socket handles, asynchronous wakeups, clock observations
and typed IO calls. Its authoritative peer/probe/sample/measurement registries,
nonces, budgets, eligibility and median policy were removed. Read-only views
cannot install measurements or mutate native reservations. Existing gathering
fixtures can still declare sensor facts; production uses the authenticated Rust
owner. See the [search QoS receipt](evidence/matchmaking-rust-parity-20261028/search-qos-validation.json)
for frozen source identities, red/green controls, full and production ABI runs,
CMake validation and verified scratch removal.

The final frozen 256-file cohort passed 171 Rust unit tests, 94 integration
tests and 490 backend tests. The separate production ABI passed 171 unit tests,
72 integration tests and 256 backend tests with diagnostic restoration disabled.
Both manifests match current source. The offline CMake Rust build/helper passed
with two build jobs and one test job; its 79 recorded Rust inputs remained
unchanged. Two existing capture/performance tests remain ignored.

All fourteen reported scratch trees and the receipt helper were removed; the
owned scratch parent is empty. The 58 retained source reports total 716,111
bytes, excluding the summary receipt. No generated large artifact or selected
runtime was changed.

These are explicitly modern relay-cycle measurements and abuse limits. They
do not recover Xbox XNet measurements, original service ordering or original
quality/summary producers. Full Rust service/endpoint replacement, accepted
social/party/clan CRUD, general population/history readers, cross-store recovery,
actual existing-world continuity and all original/native played/delivered
G01-G12 gates remain open. All twelve full gap criteria and all 53 algorithm IDs
remain intact.

## Rust accepted-party directory ownership

`matchmaking_party_directory.rs` now owns accepted party creation, invitation
and target selection, acceptance/decline/block/cancel, departures/removal/leader
transfer, privacy changes, activity capabilities, request quotas/replay and
public party projections. It owns the SQL schema, membership reads and writes,
consent invalidation and the existing Ready/countdown handoffs. Shared account,
friend-tag, presence, Steam and block providers remain explicit IO boundaries.
These are native service rules, separate from original Live recovery.

A real authenticated regression reproduced a local-seat capacity mismatch:
four party connections with four admitted seats each filled sixteen player
slots, but the activity endpoint still advertised four and issued a join secret.
Rust counts admitted local seats. An observed full party revokes its activity
link in a committed transaction before returning the rejection. Freeing seats
allows a new link; it cannot revive the revoked secret. Ordinary rejected
commands and SQL failures still roll back their complete mutation.

The Rust transaction owner emits bounded SQL/provider continuations. Its journal
records each result once and replays decisions without repeating writes or
native effects. Frames, row counts, expected changes, total steps and retained
reply bytes are bounded. Entropy belongs to Rust. Existing Unicode request
payloads retain their prior ASCII JSON representation, including surrogate
pairs. SQL statements contain bound parameters rather than interpolated input.

The FFI binds each transaction to the actual Rust queue owner and resolves its
session/queue/Ready/countdown/effect calls inside Rust. Retiring that source
prevents continuation. Public member handles and activity aliases remain
separate from private account and party identities. Python party-policy aliases
cannot grant closed-party admission. Actual SQL-trigger failure rolls back the
roster, version, invitations and replay row; retry commits once. Directory reads
preserve a caller's existing SQLite transaction.

`parties_http.py` now executes typed HTTP, clock, provider and countdown/SQL IO;
its membership/action/target/replay/privacy/activity/view decisions were removed.
Native presence/room/identity facts and general account/clan authentication,
rate controls, full service replacement and cross-store recovery remain open.
The [party directory receipt](evidence/matchmaking-rust-parity-20261028/party-directory-validation.json)
retains the red control, source identities, full and production ABI regressions,
CMake checks and scratch cleanup. The October 11 integration investigation is
explicitly marked historical and links to the current audit.

The frozen 259-file full cohort passed 174 Rust unit tests, 94 integration tests
and 495 backend tests. The production ABI passed 174 unit tests, 72 integration
tests and 272 backend tests with diagnostic restoration disabled. At that
increment, both manifests matched its source. CMake's offline Rust build/helper passed with two build
jobs and one test job; its 80 recorded Rust inputs stayed unchanged. Two existing
capture/performance tests remain ignored.

All nine reported scratch trees and the receipt helper were removed; the owned
scratch parent is empty. The 35 retained source reports total 420,206 bytes,
excluding the summary receipt. No generated large artifact or selected runtime
was changed.

No new unchanged-original execution, original Live party procedure, actual
played engine/world or delivered activation is claimed. All twelve complete
gap criteria, 53 algorithm IDs and full G01-G12 gates remain open and intact.

## Rust account and clan directory ownership

`matchmaking_accounts.rs` now owns verified-account bearer admission, rotation,
revocation, expiry and independent social/countdown budgets. Its bounded login
permits reserve capacity and entropy before canonical identity/history writes.
Generations, identity bindings and monotonic clocks prevent stale permits or
clock regression from restoring a retired session. Python exposes immutable
snapshots; trusted external verification and canonical shared identity IO remain
explicit boundaries.

`matchmaking_clan_directory.rs` owns accepted clan schema, membership,
invitations, role/lifecycle decisions, request quotas/replay, public views and
transactional SQL. It applies the existing documented one-clan/100-member
contract, role permissions, confirmed Overlord promotion, sole-Overlord leave
guard and last-member deletion. These established client/manual requirements
do not recover the retired Live service's implementation. Replacement bearer,
rate, journal and persistence rules remain modern service policy.

Every directory continuation binds the actual Rust account capability and queue
source. It rechecks revocation and fresh expiry after SQL/provider IO, including
expiry without an intervening authentication request. Active clan-match gates
and consent invalidation execute inside Rust. Declared SQL-trigger failures
roll back membership, invitations and replay; retry commits once. A modified
Python role/session projection cannot grant native authority.

The ownership regression first establishes that an internal insertion into the
old Python session bank could authorize an otherwise unadmitted bearer at the
actual clan HTTP endpoint. Rust ownership rejects it with401. This is an
adapter-authority control, not a claim of a remote unauthenticated exploit.
The initial account-capacity control already passed before the change and is
retained separately rather than described as red evidence.

The [clan directory receipt](evidence/matchmaking-rust-parity-20261028/clan-directory-validation.json)
records identical final 263-file full and production manifests. The full cohort
passed 178 Rust unit tests, 94 integration tests and 502 backend tests. The
production ABI passed 178 unit tests, 72 integration tests and 294 backend tests
with diagnostic restoration disabled. Two existing capture/performance tests
remain ignored. CMake's offline Rust static/CXX/FFI/helper check passed with
two build jobs and one test job; 82 recorded Rust inputs stayed unchanged.

All 13 reported test trees and the receipt helper were removed. The 55 retained
source reports total 740,415 bytes, excluding the summary receipt. No generated
large artifact was retained and this backend increment was not activated.
Trusted provider/canonical identity/friend/presence/Steam IO, general endpoint
limits and population/history readers, complete Rust endpoint/service ownership,
cross-store recovery and all original/native played/delivered gates remain open.

## Rust public history read ownership

`matchmaking_public_history.rs` now owns exact current-username lookup, public
player/report resolution, strict query/cursor validation, stable keyset
pagination, history SQL and public field/detail projections. Python's broad
profile-view dictionary no longer defines the public history payload. A real
HTTP ownership control reproduced an injected private field in that dictionary
appearing in the old public response; the Rust projection excludes it and reads
the canonical current name. This is an internal adapter-authority control,
not a remote unauthenticated exploit claim.

The reader preserves the admitted 304-byte variant, opaque public aliases,
current versus archived names, legacy-summary unknowns and guest exclusion.
Summary/accuracy coverage never expands into invented medal/weapon/opponent
detail. Persisted full weapon banks retain all 360 indexed rows using 128-row
SQL pages. The bounded continuation journal also preserves an existing outer
transaction. SQLite BLOBs cross the adapter as explicit typed cells rather than
unowned byte pointers.

A new read-only control caught the initial adapter advancing the registry's
durable clock floor on every history read. SQL tracing identified `Store.now`
as the writer. The adapter now observes `Store.clock` without that mutation;
valid reads and malformed-cursor rejection leave the database unchanged at the
actual synchronous read boundary. Shared asynchronous maintenance is outside
that witness. Temporary tracing was removed.

The [public history receipt](evidence/matchmaking-rust-parity-20261028/public-history-validation.json)
records original Python ownership red evidence, the read-only regression,
complete detail/legacy/accuracy/transaction controls, final frozen backend and
production ABI cohorts, CMake source identities and scratch cleanup. Earlier
increment counts and source-match statements refer to their recorded cohorts;
this receipt identifies the current public-read cohort.

The final identical 265-file manifests passed 179 Rust unit tests, 94 integration
tests and 506 backend tests. The production ABI passed 179 unit tests,
72 integration tests and 309 backend tests with diagnostic restoration disabled.
Two existing capture/performance tests remain ignored. CMake's offline Rust
static/CXX/FFI/helper check passed with two build jobs and one test job; all
83 recorded Rust inputs stayed unchanged. All twelve reported scratch trees
and the receipt helper were removed. The 48 retained source reports total
583,317 bytes, excluding the summary receipt; no large artifact was retained.

The shared cached current-profile totals provider, account customization,
population/presence observations, generic limits/provider verification and the
complete Rust HTTP service remain open. No original Live history API/epoch,
unchanged-original execution, native played match or deployed activation is
claimed. All twelve complete gap criteria, 53 algorithm IDs and full G01-G12
gates remain intact and open.

## Rust current profile ownership

`matchmaking_profile_directory.rs` now owns current-profile schema, appearance
and service-tag validation, no-op detection, customization/outbox SQL, revision
guards, current-profile projections and career-total queries. The nine-byte
current profile remains distinct from sixteen-byte historical PCR appearance.
Career totals read accepted result rows; verified accuracy coverage remains
separate from legacy summaries and never averages per-match percentages.
History now calls this native profile owner directly. The Python profile and
statistics modules retain typed HTTP/clock/SQLite IO.

A real HTTP regression pauses a profile body upload after authentication,
revokes its account bearer and resumes the upload. The old endpoint returned 200
and saved customization after sign-out. The Rust cursor binds the actual
account owner and revalidates the capability at construction and each IO reply;
the same request now returns 401 without customization or outbox publication.

`matchmaking_profiles.rs` owns the bounded derived-total cache: 1024 entries,
300-second expiry, revision invalidation, LRU replacement, monotonic clocks,
generation fencing and 128 pending plans. Python cannot install cache snapshots.
Only a completed native directory plan can stage totals, and successful owned
SQL commit is acknowledged before publication. Dropping/retiring a cursor
aborts its pending cache plan. Derived-cache retirement after a successful SQL
commit is a no-op rather than a false failure of an already durable write.

An outer caller's transaction bypasses shared cache reads/publication. It sees
its own uncommitted totals without poisoning later readers after rollback.
A deferred foreign-key failure at actual SQLite commit proves that customization,
outbox revision, clock watermark and a staged cache entry retire together.
Retry commits one customization revision. Unchanged heartbeats reuse totals,
accepted revisions invalidate them, and clock regression cannot revive expiry.

The [profile receipt](evidence/matchmaking-rust-parity-20261028/profile-validation.json)
records the upload/sign-out red control, SQL-commit/cache rollback controls,
frozen full/production cohorts, HTTP refresh, CMake identities and scratch
cleanup. The identical frozen broad cohorts passed 275 Rust checks and 512
backend tests, then 253 Rust checks and 313 backend tests with production
features. The later identical HTTP cohorts passed 25 default-feature and 22
production-feature tests. The Rust static/CXX/FFI/helper build also passed with
all 85 Rust input hashes unchanged. Concurrent directory-resource and service
admission edits are preserved and recorded as drift, including edits after the
HTTP refresh; these receipts do not qualify the entire current checkout.

Canonical identity/friend/provider facts, EOS issuer/publication IO, population
and presence policy, generic service limits and complete Rust HTTP service
replacement remain open. This is native replacement-service policy, not new
recovery of original Live profile storage. Original producer, native played
match, installed/delivered and G01-G12 acceptance requirements remain intact.

## Rust population and observed presence ownership

`matchmaking_presence.rs` now owns unique account/guest classification, the
30-second activity window, public aggregate projection and activity precedence.
The FFI resolves actual native account and queue handles; Python session and
seat projections cannot supply authority. Native sessions, tickets, proposals
and catalog rows provide their own authenticated subjects and leases. The
native room owner supplies admitted seats, frozen playlist identity, phase,
current socket generation, challenge expiry and room deadline. Python supplies
only clocks and socket observations, bounded to 64 rooms and 16 physical peers
per room. Private postgame party flags use the same native seat observation,
including independently signed local players; guests remain excluded from
private account-presence maps.

Three actual HTTP controls fail before this port: a replaced Python seat
projection invents online subjects, a clock rollback revives stale activity,
and a status read tries to update the SQL clock watermark. The native path
rejects projected authority, observes monotonic activity/wall clocks without
renewing sessions, and performs no SQL writes. An outer transaction remains
open and unchanged. These are replacement-service policies; they do not recover
original Live presence flags, service ordering or historical configuration.

The old fabricated Python population/room fixtures were replaced with real
authenticated couch, queue and relay flows. They cover unique signed/guest
counts, zero rows for inactive playlists, activity expiry while tickets remain,
handoff after frontend sign-out, running-room presence, disconnect retirement,
postgame review and independent requeue. The
[presence receipt](evidence/matchmaking-rust-parity-20261028/presence-validation.json)
retains the red controls, bounded-bank/owner-type controls, frozen default and
production cohorts, CMake hashes and scratch cleanup. Generic HTTP/provider/
friend/storage authority, full Rust service replacement and cross-store crash
recovery remain open. Original producer, played-match, delivered identity and
all G01-G12 criteria remain unchanged.

The final identical 275-file cohorts passed 278 Rust checks and 536 backend
tests, then 256 Rust checks and 318 backend tests with production features.
Two existing Rust tests remain ignored. The static/CXX/FFI/helper build passed
with all 86 Rust input hashes unchanged. All reported scratch was removed;
40 small diagnostic/source reports totaling 491,134 bytes were retained. No
large artifacts, service activation or runtime replacement are part of this
increment.

## Rust accepted friends, privacy and friend-file outbox ownership

`matchmaking_friend_directory.rs` now owns accepted friend requests/consent,
bilateral blocks, invitation revocation, quotas, replay, public projections and
tag schema/assignment/lookup. Native account capabilities fence every protected
SQL/provider continuation. Public aliases and friend tags remain separate from
private account/provider identities. The roster uses bounded 128-row SQL pages;
names use pinned Unicode 16 NFC/full case folding, with the primary Unicode data and
license retained under `rust/online_protocol/data/unicode16`. This is modern PC
service policy; original Live collation and friend procedures remain unknown.

Three real HTTP controls reproduce the earlier Python authority gaps: replacing
its block cache bypasses a persisted block, a friend command commits an outer
caller's transaction, and revocation during a provider callback arrives after
the mutation has committed. Rust now owns those decisions and keeps the whole
mutation/replay/outbox transaction subject to the actual account capability.
SQL-triggered block epochs include other database connections. The native
derived cache publishes after successful owned commit, rejects older epochs,
and avoids sharing uncommitted outer-transaction observations.

`matchmaking_friend_cloud.rs` owns private-file snapshots, canonical payloads,
fingerprints/revisions, pending selection, retry/backoff and completion CAS.
Two further controls reproduce a transaction held across upload and publication
of an outer transaction's uncommitted snapshot. Native opaque claims bind the
selected player/revision/digest/PUID and become usable after SQL commit. SDK IO
starts with no open selection transaction; outer transactions cannot publish.
The native owner verifies the stored payload hash before issuing a claim;
the explicit damaged-digest fixture cannot reach upload or completion.
Completion preserves superseded revisions, rejects changed bindings and leaves
cancelled/failed operations retryable. Rust validates the worker's exact record
read-back. Python retains typed HTTP/clock/SQLite/process/SDK IO and private-file
configuration handling.

The [friend receipt](evidence/matchmaking-rust-parity-20261028/friend-validation.json)
retains both red cohorts, native cache/claim controls, real SQL/provider/HTTP
checks, Unicode source identities, final feature cohorts and CMake receipts.
At that increment, provider verification/canonical account links, Discord voice preferences,
Steam upstream state, generic middleware and complete Rust service replacement
remain open, alongside cross-store/process recovery and the complete original,
native played-match and delivered G01-G12 gates.

The final identical 285-file cohorts passed 282 Rust checks and 545 backend
tests, then 260 Rust checks and 337 backend tests with production features.
The Unicode differential covers all 1,112,064 valid scalar values against the
local Unicode 16 Python reference; the native mapping test independently
checks all 1557 source rows. The static/CXX/FFI/helper build passed with all
91 Rust input hashes unchanged. Sixty small reports totaling 1,319,828 bytes
are retained, plus 88,650 bytes of required Unicode source/license/provenance.
Scratch and generated binaries were removed. No service activation, selected
runtime replacement or new unchanged-original execution is claimed.

## Rust private voice preferences and social relay ownership

`matchmaking_voice_directory.rs` owns the private preference snapshot contract,
verified-provider-to-canonical-account lookup, self-subject exclusion, capacity,
full SQL replacement and bilateral friend/preference/identity decisions. The
provider proof remains trusted IO; the supplied list remains a client preference,
not an independently verified relationship graph. Current SQL is authoritative;
there is no Python block/subject cache or private resolver endpoint. Caller-owned
transactions remain open and can roll back without publishing derived state.

`matchmaking_social_voice.rs` owns opt-in clan and accepted-party admission,
actual native account bindings, membership incarnations, opaque peer generations,
replacement/retirement, version1 framing, strict u32 sequence and 10/50Hz burst
policy, group/service caps and five-frame dropping queues. Role/leader changes
preserve the channel; leaving and rejoining creates a new membership incarnation.
Queued frames bind both peer generations and are checked against current account,
membership and bilateral privacy before delivery. Media older than five seconds
is discarded. Each relay owner retains at most 1280 frames across 256 peers,
with at most 16 peers per group; attachment plans have a separate 128/30-second
bound. Clan and party channels retain their separate relay owners.

The directory's private native plans publish attachment, stream and queue effects
only after owned SQL commit. Failed commit or dropped continuation aborts the
plan; a stale completion cannot overwrite a replacement. Python retains socket
handles, wake events, typed clock/provider/SQLite execution and transport timing
(half-second idle checks and a 250ms send deadline). It cannot choose an audience,
validate a packet, install a membership, or authorize delivery from an alias.

Four controls went red through actual HTTP/SQLite/sockets before the port:
an adapter alias bypassed persisted blocks, a second connection's block was
invisible, replacement committed an outer transaction, and an old clan socket
received audio after removal and reinsertion of the same membership. These are
internal ownership/state-churn controls, not a claim of remote Python/SQL access.
Further controls exercise queued privacy changes, replacement speakers, actual
WebSocket-upgrade revocation and failed SQL COMMIT. Existing party/activity tests
now use actual preference requests and native room facts instead of mutable
Python policy caches. Validation and cleanup are recorded in the
[voice receipt](evidence/matchmaking-rust-parity-20261028/voice-validation.json).

An additional actual SQL-commit control advances the trusted relay clock by
31 seconds. Before the fix, an expired plan returned its unpublished staging
envelope to the socket adapter. Expired completion now rejects409 and leaves no
peer installed. Empty polls and schema/preflight reads do not require a plan.

The final identical 289-file cohorts passed 287 Rust checks and 554 backend
tests, then 265 Rust checks and 346 backend tests with production features.
Two existing capture/performance Rust tests remain ignored. The native static/
CXX/FFI/helper build, codec and real clan/party Opus HTTP checks passed, including
32 seconds of idle time per channel. Concurrent CMake edits invalidated an
intermediate wrapper's cohort; a later unrelated header change also tripped
its guard. The final CMake/Rust/backend/C++/header source cohort was copied
and stayed stable. Newer live-source drift is recorded separately. These
are local synthetic-audio
checks, not physical headset/WAN, rendered game or delivered acceptance.
82 small reports totaling 2,869,231 bytes are retained; test scratch and
generated binaries were removed. No large generated artifact is retained.

These are modern private Opus-service policies. Original codec/headset behavior,
Xbox Live procedures, canonical/provider verification, Steam upstream policy,
full Rust HTTP/service replacement, cross-store/process recovery and the complete
original/native played/delivered G01-G12 gates remain open. No selected runtime
replacement or service activation is included.

## Rust Steam friend provider and invitation authority

`matchmaking_steam_request.rs` now owns the whole authenticated friend request,
including current canonical Steam-link SQL, fixed upstream request sequence,
strict JSON/size/shape checks, bounded summary batches, display-name/status
filtering, public registration/presence/privacy projection, casefold ordering
and final HTTP body/status/headers. The actual native account owner revalidates
each continuation; the canonical Steam alias is checked after provider awaits
and again before final projection or invitation proof. Display names never
authorize a relationship or select a canonical account.

`matchmaking_steam_cache.rs` owns endpoint source budgets, credential epochs,
the FIFO upstream work slot, cancelled/expired request retirement, the bounded
upstream-only cache and opaque invitation evidence. Cache publication follows a
successful final SQL read transaction; no SQL transaction remains open across
network IO. Outer/uncommitted observations reject rather than publish. Live
registration, Halo presence and bilateral game/party blocks are projected anew
when upstream data comes from cache. SQL failure releases the work slot without
installing shared data. A credential change retires old cache/proof context.

Four controls went red through actual authenticated HTTP/SQL before the port:
lookup returned data after native account revocation, accepted a removed Steam
link after provider IO, trusted a mutated Python cache for a private upstream,
and created an invitation after the actor's link changed following lookup.
The cache mutation is an internal adapter-authority control, not a remote Python
injection claim. The port removes the authoritative Python cache, global lock,
filtering, sorting and provider schema decisions.

Accepted-party invitations now consume an opaque Rust proof bound to the
actual account source/actor/bearer, current Steam subject, exact request payload,
target and expiry. Python result rows cannot replace it. A different command
cannot reuse it; SQL rollback unlocks it for retry, successful commit consumes
it, and durable same-request replay remains idempotent. Existing party role,
version, block and capacity gates still execute in the native directory.

These are modern provider/service limits: ten thousand friend IDs, the ordered
first thousand selected friends, at most one hundred summaries per call, one
active fetch per cache owner, bounded FIFO requests, a75-second request lifetime,
60-second upstream snapshots,30-second invitation proofs and30/minute endpoint
source budgets. The cache holds at most1000 entries and64MiB of canonical JSON.
The upstream body limit is explicitly1MiB; the former generic16KiB limit could
not represent the advertised thousand-friend corridor or full100-row summaries.
Unknown fields are discarded, cross-batch summary identities are ignored, and
malformed/compressed/oversized responses cannot install data. Steam display-name
ordering uses pinned Unicode16 case folding without adding NFC normalization.

The actual native HTTP regression also exposed a separate client receive/parse
limit: a valid thousand-row reply with escaped128-byte names contains340,051
bytes, beyond the former256KiB account limit. Rust now supplies the1MiB limit
for this exact GET endpoint through the CXX bridge. Both libcurl reception and
JSON parsing use that value; other account operations retain256KiB. The fixture
completes the real game's browser confirmation flow using its own private,
disposable code file. Neither provider authorization nor confirmation is bypassed.

Valve documents the100-ID summary maximum and a401 response for a private list
in its [ISteamUser API reference](https://partner.steamgames.com/doc/webapi/ISteamUser).
The existing403-as-private compatibility behavior is retained as modern service
policy. Tests use deterministic upstream IO and the independent existing OpenID
fixture; no live Steam grant or relationship is changed. Validation, source
identities and scratch cleanup are recorded in the
[Steam friend receipt](evidence/matchmaking-rust-parity-20261028/steam-friends-validation.json).

The final identical 293-file backend cohorts passed 294 Rust checks and
566 backend tests, then 272 Rust checks and 358 backend tests with production
features. Two existing Rust capture/performance tests remain ignored. All four
native checks passed: build helper, ordinary Steam account HTTP, the full
thousand-friend HTTP reply and the account client regression including the
unchanged general response bound. The actual body-limit control went red
before the fix; two earlier attempts exposed stale test confirmation and
loading-state handling, and are recorded separately. Owned native source
hashes stayed stable. Unrelated later checkout drift is recorded separately.
113 small supporting reports totaling 3,221,603 bytes are retained; owned
test source/build trees and generated binaries were removed. No large
generated artifact is retained.

Steam OpenID/shared canonical identity and sign-in persona verification, full
Rust HTTP/service replacement and generic middleware, cross-store/process
recovery, original Live/history/epoch/arbitration recovery and the complete
original/native played/delivered G01-G12 gates remain open. No selected runtime
replacement or service activation is included in this increment.

## Rust canonical identity and dual-provider adoption ownership

`matchmaking_identity_directory.rs` now owns canonical schema/migration,
serialized capacity and UUID allocation, provider-alias lookup, account/couch
admission identity and separate-party reads, verified-name updates, stable
registration metadata and the existing native public-tag allocator. The Python
identity helpers carry typed inputs and execute Rust SQL. Account-session and
matchmaking preflight callers no longer select canonical aliases in Python.
Names remain presentation data and cannot establish an identity.

Three controls failed before the port: canonical resolution committed its
caller's outer transaction, and the actual link endpoint accepted a Steam
credential that expired or was revoked during Discord verification. Canonical
composition now preserves the caller transaction; failed composed operations
roll back their own SQL savepoint while retaining earlier caller writes.
Linking rejects outer transactions before external provider verification.

An additional real matchmaking HTTP control found partial couch registration:
two new signed-in seats with one free persistent slot returned503 but retained
the first new player/profile/alias/tag. Rust now resolves the entire roster in
one transaction. Rejection leaves all canonical tables unchanged and publishes
no native session. A passing control allocates distinct primary/local identities
and preserves a session-only guest without creating a stored guest account.

Rust owns the complete adoption decision and SQL sequence. It checks the raw
Steam bearer against current session SQL before provider IO and again inside
the final write transaction. It then enforces current canonical aliases and
one-provider membership, checks the actual native session owner for primary
and signed-in couch seats, and walks persistent player foreign keys. Schema
names are quoted, result pages are bounded, and later pages are checked.
Played users, memberships, preferences, customizations, statistics and other
persistent references still require an explicit data migration. Only the
existing empty-profile metadata exemption may be adopted. New registration
timestamps retain the persisted server clock floor; legacy unknown dates stay
NULL.

After SQL COMMIT, the sealed native directory completion revokes grants and
pending account permits for the deleted canonical player. No Python projection
or returned row list selects this effect. A failed real deferred-constraint
COMMIT preserves the old aliases and actual native account, and successful
retry retires that old account. This test exposed an intermediate shared
readiness-flag consumption bug; identity commit readiness is now independent.
A separate fixture correction adds the required native title language so the
existing linked-couch test reaches its intended duplicate-account rejection.

Validation, source identities, failure diagnostics and scratch cleanup are
recorded in the [canonical identity receipt](evidence/matchmaking-rust-parity-20261028/identity-validation.json).
Tests use deterministic providers and local HTTP; no live provider account is
linked. No new unchanged-original execution, rendered played match, selected
runtime replacement or service activation is claimed.

The final identical 296-file backend cohorts passed 294 Rust checks and
579 backend tests, then 272 Rust checks and 371 backend tests with production
features. Two existing Rust capture/performance tests remain ignored. All four
native checks passed against the final copied source cohort: build helper,
ordinary Steam account HTTP, the full thousand-friend HTTP reply and the
native account client. Source hashes stayed stable; later unrelated checkout
drift is recorded separately. These are local component/HTTP checks.
82 small supporting reports totaling 1,825,194 bytes are retained; owned
source/build scratch and generated libraries were removed. No large generated
artifact is retained.

Steam OpenID verification/pending/session/refresh issuance and Discord verifier
policy still require Rust ownership. Shared source-admission middleware, full
Rust service/API replacement, cross-store/process recovery, original Live
history/configuration/arbitration recovery and all original/native
played/delivered G01-G12 gates remain open.

## Rust HTTP and SQLite runtime ownership

A new executable in rust/matchmaking_service owns HTTP dispatch, the private
SQLite connection/transactions and account/clan/profile/history endpoint
coordination in Rust. It uses the existing policy modules through
matchmaking_owners::Owners. That safe registry now lives in the protocol crate;
the bounded C ABI delegates to the same implementation. The former dispatcher
was moved without changing the owner/source/continuation decisions. Independent
registries retain separate opaque grants and enforce actual source types and
retirement.

The implemented routes use real native account reservations and current
canonical SQL, include public history identity allocation, and acknowledge
profile/cache effects after SQL COMMIT. A failed COMMIT retires the unpublished
native plan. Steam login reads the private persisted provider grant and rechecks
it in the canonical transaction. Discord application/scope/user checks execute
against the existing fixed HTTPS endpoints in Rust. Provider receipts and SQL
plans are private implementation values; clients cannot supply native handles
or SQL. Generic Python adapters are absent from these requests.

The Rust HTTP process bounds connections, headers, body reads, source/prefix/
principal accounting and admitted work. Request permits remain held by blocking
IO after a client disconnect; expired queued requests do not publish new work.
Hyper applies the real header deadline, and SQLite owns the durable transaction.
These are modern service/transport policies, not recovered Xbox Live rules.
The [crate documentation](../../rust/matchmaking_service/README.md) records the
scope, pinned dependencies, transport setup and owned qualification commands.

The qualification helper copies and hashes the Rust source and uses the real
Rust executable/HTTP listener. It tests strict input rejection, private provider
grant SQL, clan create/invite/accept/replay, native observed presence, a real
deferred-constraint profile COMMIT failure and fresh retry, public queries,
logout and process restart. Incomplete headers/body are exercised over actual
TCP. The native account client uses real account/profile/clan HTTP against that
Rust process, with only provider handoff supplied by a deterministic fixture.
No live provider authorization or new original execution is claimed. Source,
failure diagnostics and cleanup are recorded in the
[Rust service receipt](evidence/matchmaking-rust-parity-20261028/rust-service-validation.json).

The identical final 297-file legacy backend cohorts pass 295 Rust checks
and 579 backend tests, then 273 Rust checks and 371 backend tests with
production features. Two existing capture/performance Rust tests remain ignored.
The first coherent native cohort passed five CTests. A final two-CTest cohort
repeats the build helper and real Rust HTTP/native client check with an explicit
native cosmetic write and independent SQLite verification of its nine bytes.
The Rust executable qualification records twelve real process/HTTP/native
checks plus its source-budget unit test. Source hashes stayed stable; later
unrelated checkout drift is recorded separately.
61 small supporting reports totaling 1,403,901 bytes are retained. All owned
source/build/database scratch and generated binaries were removed; no large
generated artifact is retained.

This is an implementation increment, not complete endpoint/service replacement.
The health response distinguishes the working module and keeps full service
readiness false; unavailable capabilities return503. Steam OpenID/pending/
refresh issuance, accepted parties, queue/catalog endpoints, match rooms and
transports, EOS/social/provider routes and full integration remain required.
Cross-store/process recovery, original service/configuration/arbitration
recovery and all original/native played/delivered G01-G12 gates remain open.
No selected launcher runtime or production service is replaced or activated.

## Rust service session, catalog, party and queue integration

The standalone Rust listener now executes matchmaking session admission,
playlist eligibility/display, accepted-party command/activity/consent,
pregame countdown and ticket create/poll/cancel requests. All production
coordination on these routes runs in Rust through the existing native owners;
the qualification process has no Python server or coordinator. A local
--matchmaking-catalog input is bounded and parsed in Rust. Health binds the
actual catalog bytes by SHA-256. Separate OS entropy seeds initialize the
existing native scheduling, hopper draw and gathering owners.

Canonical signed-local and guest admission runs in one immediate SQLite
transaction. Every signed provider receipt is checked, current Steam grant
expiry is reread after obtaining the SQL write lock, canonical identities
resolve as one couch roster, and the session owner rechecks admitted facts.
Guests remain transient and separate accepted-party/physical seating is
rejected. The identity schema now supplies the existing playlist_ranks DDL;
its duplicate Python API schema was removed.

The safe owner registry supplies an exclusive queue transaction. Its complete
checkpoint includes sessions, consent/countdown, tickets/gatherings, query
continuations, room state, results and RNG ownership. Provisional replies stay
inside the locked runtime until COMMIT succeeds. Failed COMMIT restores that
queue and SQLite rolls back; account accounting and owner handle generations
are retained. Real deferred-constraint failures verify ticket/rank rollback,
session replacement while consent/countdown are active, new signed-local
allocation, and countdown cancellation with the same request retried once.
Other full process/epoch recovery and distributed authority gates remain open.

The Rust timer invokes the existing original countdown tick. Its input is the
service clock, never an HTTP client clock. A new real regression returned HTTP 429
within 130 countdown reads because the route charged ordinary social activity.
The route now uses native account authentication without that social charge
and applies the existing separate countdown budget. The same 130 reads pass and
an ordinary clan read still succeeds.

The native matchmaking client requires an immediate ticket read after loading
its catalog. That complete endpoint path now uses native API, SQL snapshot and
gathering/QoS continuations. Missing search sockets never become passing
readiness or fabricated measurements, and no match is supplied by these
fixtures. Creation, retry, replay rejection, progress polling and cancellation
execute through the actual Rust HTTP/SQLite process.

Final backend cohorts contain the identical 297 source files with no drift:
296 Rust checks and 579 backend tests pass, followed by 274 Rust checks and 371
backend tests with production features. Two existing Rust capture/performance
tests remain ignored. The first six-CTest native build passed five checks; its
new probe stopped at provider handoff because its fixture omitted the separate
account URL setting. Correcting that fixture required no product-code change.
The final two-CTest cohort passes the build helper and real Rust HTTP/native
qualification, including twelve account/general HTTP controls and thirteen
matchmaking/party/queue controls, plus the service budget unit test.

The actual native matchmaking client reads all fourteen playlists and becomes
ready through the real Rust session/catalog/ticket path. The supplied current
catalog admits zero playlists with v12 and all twelve non-clan playlists with v16.
The actual native party client sends Start/cancel and observes the shared Rust
countdown. Only the external provider handoff is a deterministic fixture; no
real Steam/Discord authorization, rendered game input or played-world acceptance
is claimed. Coherent source hashes, earlier failures, later unrelated checkout
drift and cleanup are recorded in the
[Rust session/party/queue receipt](evidence/matchmaking-rust-parity-20261028/rust-service-party-validation.json).
Generated source/build/database scratch and binaries are removed; only bounded
reports remain.

The earlier runtime increment's session/catalog/accepted-party/ticket gap is
advanced by this work. Full readiness remains false. Search WebSocket IO,
rooms/transports, EOS/voice/results integration, remaining social/provider
issuance, complete clock/epoch/cross-process recovery, original Live/configuration/
ordering/arbitration recovery and the full original/native rendered, played and
delivered G01-G12 gates remain required. No selected launcher runtime or
production service is activated. All twelve complete GAP criteria, 53 algorithm
IDs and twelve acceptance gates remain intact and uncertified.

## Rust service search WebSocket integration

The standalone Rust process now owns actual search WebSocket upgrades, socket
lifetimes, received bytes and physical connection observations. Its adapter
calls the existing Rust search QoS owner for authorization, heartbeat expiry,
challenge dispatch, measurements, leases and readiness. That owner rechecks the
private session proof after the upgrade await. Socket cancellation publishes
closure immediately; generation checks fence replacement and late detach.
No Python server or coordinator participates in this search route.

A real wire failure returned HTTP 101 with `Connection: upgrade, close`.
Hyper's global keep-alive disable appended `close` to the upgrade. Ordinary
HTTP responses now explicitly close, and valid upgraded streams remain open.
The regression checks the actual response headers and keeps an answering
socket alive beyond the HTTP listener's thirty-second deadline. A silent
socket still expires through the existing fifteen-second native heartbeat.

Modern transport bounds include 1000 socket slots, 32 pending blocking
operations, 256-byte frame/message limits and 500ms sends. Actual IO closure,
connection generation and wake destinations are private observations. HTTP
cannot supply successful readiness, measurements or peer destinations. These
are declared modern relay-cycle policies, not recovered original Xbox XNet
measurements or Live service behavior.

Two frozen CTests pass: the bridge build helper and real Rust HTTP/native
qualification. The latter includes twelve account/general controls, thirteen
matchmaking/party controls, seven search socket controls and the actual native
QoS client pair. The two real OnlineSearchQos clients form a two-player Lab
proposal and shut down normally. Separate real HTTP sockets produce ten
heartbeats and six probe legs, forming a three-seat proposal with a transient
guest. Tests also cover wrong grants, duplicate live connections, malformed
controls, session replacement, late detach, cancellation, revocation and
process restart. Both Rust service unit tests pass, including expiry and
revocation between upgrade preflight and actual attachment.

The first raw unmasked-frame control also contained invalid application data.
The strengthened control sends the exact valid heartbeat reply without its
RFC6455 mask, isolating parser rejection. That fresh socket cohort passes all
seven controls. Its 99 Rust source/manifest/lock inputs match the native cohort
and current checkout exactly; the changed input is the test control. An
auxiliary attempt to reuse an already temporary binary missed its owner path
and ran no network checks; its scratch was removed. Reports preserve that
failure separately from the successful fresh qualification.

The prior full and production backend cohorts were not rerun for this socket
adapter increment. Their identical 297-file inputs still match the checkout:
296 Rust/579 backend and 274 Rust/371 backend checks remain earlier component
evidence. Frozen native inputs, current unrelated rendering/header drift, test
harness changes and exact binaries are recorded in the
[Rust search WebSocket receipt](evidence/matchmaking-rust-parity-20261028/rust-service-qos-validation.json).
All generated binaries, copied source, databases and build trees are removed;
only bounded reports remain.

Provider grants are private fixtures. The authored Lab profile is a bounded
control, not original profile or natural full-match acceptance. Rooms,
transports, EOS/voice/results integration, remaining provider/social IO,
clock/epoch/process recovery, original service/configuration/ordering/arbitration
recovery, rendered Linux menu input and the complete played/delivered G01-G12
requirements remain open. Full readiness stays false. No launcher runtime or
production service is activated; all twelve GAP criteria, 53 algorithm IDs and
twelve acceptance gates remain intact and uncertified.

## Rust service room admission and transport integration

The standalone Rust process now serves room descriptors, signal/relay sockets
and admission-bound content failures. Rust derives the roster, statistics
namespace, player/clan snapshot bank and immutable binding from the admitted
proposal. The runtime reads current SQL facts and revalidates the prepared
admission inside the owned transaction. Credentials become visible after
COMMIT. A rejected admission restores result, setup, capability and complete
queue state together with SQL. HTTP cannot supply the snapshot bank or roster.

Actual room socket generations, closure and bounded queue capacity now feed
the existing Rust room owners. Session seating, party/queue admission,
countdown reads and observed presence use these room facts instead of empty
room lists. Upgrades recheck expiry and search cancellation at attachment.
Replacement closes the old physical connection; late detach cannot remove
its replacement. A transferred room bearer survives frontend logout. Packet
delivery rechecks the current source, recipient and native role/privacy policy.
Native close codes, departure masks and content-failure/requeue receipts remain
owned by Rust.

Room IO has 1024 socket slots, 120000-byte message/frame bounds, 500ms send
deadlines and per-peer queues bounded by 512 entries and 2MiB. Pending signals
drain in native batches of at most sixteen per room tick. The original start
and readiness owners retain their full ten-second gate. These socket limits
and the following storage retry are declared modern service policies.

The existing result owner decides consent, quarantine and finalization. Rust
commits the bound admission, XP and PCR transaction before publishing completion.
A real failed COMMIT leaves unanimous review pending and publishes no success;
room ticks retry storage with a one-second IO backoff while the existing native
review lease continues. This supplies no recovered original Live arbitration
or independent simulation truth.

The initial actual transport request returned 503 after a valid source-catalog
proposal. The Rust route and room integration now pass. Qualification also
preserves several test corrections: complete-options fixtures still require
v16; an account-only grant receives 401 at the implemented descriptor route;
another room's content failure receives 403; and the native probe now consumes
each received datagram once. These fixture corrections are not additional
production regressions or grounds to relax compatibility guards.

Two frozen CTests pass, including real Rust HTTP/SQLite/socket qualification
with five service unit tests, twelve general controls, thirteen matchmaking
controls, seven search socket controls, source-v16 descriptor admission, five
relay controls, three direct controls and one content-failure control. Actual
native OnlineMatchSession clients observe loading and the unshortened start,
exchange a 27-byte input and a complete 1400-byte snapshot, then confirm a
synthetic PCR through the Rust transaction. This frozen native cohort uses
protocol 123. A separate three-seat control excludes the transient guest from
persistent hidden XP and records a total of 100 exactly once. Direct controls
use real local UDP exchanges as client-reported RTT inputs; they do not attest
original XNet measurements or peer-world promotion.

Fresh shared-library/backend cohorts pass 296 Rust/579 backend checks and 274
Rust/371 backend checks without test-only inputs. Their identical 298-file
manifests match the checkout. The native build used 4379 frozen inputs with
no changes during validation. Later README, CMake and unrelated renderer drift
is recorded separately; current Rust product inputs match the tested cohort.
Exact commands, source hashes, initial failures, fixture scope and cleanup are
retained in the [Rust room service receipt](evidence/matchmaking-rust-parity-20261028/rust-service-rooms-validation.json).
All private source/build trees, databases and generated binaries are removed.
Only bounded small reports remain.

Legacy v10/v12 fixtures explicitly cover retained transport seams with synthetic
datagrams, PCR, map fingerprints and private provider grants. Source-v16
descriptor admission does not prove EOS/provider integration, complete-options
played results or a native world. Remaining provider/social/voice effect IO,
clock/epoch/process recovery, original configuration/order/arbitration recovery,
all required families/profiles/topologies and natural outcomes, actual Linux
menu input, played-world continuity and delivered G01-G12 acceptance remain
open. Full readiness stays false; no selected runtime or production service is
activated. All twelve GAP criteria, 53 algorithm IDs and twelve acceptance
gates remain intact and uncertified.

## Rust EOS identity issuance and verification IO

The standalone Rust service now implements configured account/room EOS token
and proof routes and public JWKS. Its persistent private RSA key retains the
existing RS256 issuer, audience, canonical player/name, five-second not-before
allowance and 120-second lifetime. Explicit development HTTP cannot enable the
issuer. The key remains in the private service state directory with mode 0600;
process restart preserves its public identity. Configuration and worker setup
are documented in the [Rust runtime runbook](../../rust/matchmaking_service/README.md).

Only a private reply from the configured official-SDK server worker can
authorize a PUID binding. Its existing Rust verifier checks the returned
account/provider, requested PUID and configured game client/product/sandbox/
deployment; the C++ layer supplies SDK calls and callback values. An unverified
JWT subject is a bounded routing hint. Duplicate or malformed claims cannot
reach SDK IO or become account authority. No Python server participates in
these Rust routes.

The shared Rust profile directory now owns immutable, unique PUID links and
idempotent outbox revisions. The legacy Python adapter also uses this SQL
owner. Rust rechecks the actual account/room capability after body upload,
after SDK IO and after acquiring the owned SQL transaction. Room proof state
and persistent identity publication share complete-queue/SQL rollback. A
failed COMMIT followed by a different PUID retry proves that provisional room
proofs do not leak. Social account logout does not revoke a room bearer;
cancelling matchmaking before gameplay retains its existing cancellation rules.

The original configured Rust JWKS request returned 503. That route now passes
real RSA signature verification against its public JWK. A subsequent real body
upload control caught a second issue: revocation prevented persistence but
still consumed SDK work. The new post-body capability check rejects 401 before
the worker receives anything. These modern service regressions are separate
from original Xbox matchmaking evidence.

Verifier IO uses the existing 32-principal admission owner, one active request
per principal, a two-second queue wait and a twenty-second owned operation
deadline. Only the stream owner can retire its child. Cancelling a waiter does
not disturb another caller; cancelling an owner, malformed replies and timeout
retire/reap that exact process before reuse. A response has a total 262144-byte
bound and at most 64 lines. SIGINT/SIGTERM cancellation also retires active
verification before service exit. Room issuance/proof retains a separate
120-per-minute physical-owner allowance. These are declared modern service
bounds, not recovered Live protocol or arbitration.

All eight Rust service unit tests pass, including actual worker-process
cancellation and shutdown. Thirteen real HTTP/SQL/worker EOS controls pass:
signatures and key persistence, namespace/identity rejection, failed COMMITs,
body/provider revocation, queue contention, malformed replies, the real
twenty-second timeout, v16 room proofs/signals, SIGTERM and restart. The source
v16 control admits two physical owners and a transient guest with complete
options, issues signed tokens and exchanges only verified PUID signals on real
sockets. Provider SDK receipts remain explicit private fixtures; the official
SDK and real Epic authorization were not exercised by these controls.

Two fresh CTests pass in 169.23 seconds, including the existing native account,
matchmaking/party, search QoS and room clients alongside the new EOS controls.
The native room pair still uses protocol 123, the full ten-second start and
synthetic 27-byte input/1400-byte snapshot/PCR completion. Fresh matching
backend cohorts pass 296 Rust/579 backend checks and 274 Rust/371 backend checks
without test-only inputs. Their identical 298-file inputs match the checkout.
All 4386 native inputs remained unchanged during validation; current Rust
product inputs match that cohort. Later README and unrelated application
source drift are recorded separately.

The [Rust EOS service receipt](evidence/matchmaking-rust-parity-20261028/rust-service-eos-validation.json)
retains exact commands, hashes, actual failures, corrected fixture assumptions
and bounded reports. It also records removal of every private build, copied
source, key, database and test worker. The selected runtime and production
service remain untouched.

Full readiness stays false. Real provider/SDK and native EOS peer/world
acceptance, Steam issuance, remaining social/voice effect IO, clock/epoch/crash
recovery, original configuration/order/arbitration recovery, all required
families/profiles/topologies/natural outcomes, actual Linux menu/Start input
and delivered G01-G12 acceptance remain open. This increment changes no GAP
closure criterion, algorithm inventory, acceptance gate or parity certification.

## Rust Steam browser, device and remembered sign-in

The standalone Rust service now owns Steam OpenID verification, private pending
state, browser/game confirmation, credential issuance, remembered sign-in,
revocation and dual-provider linking. This supersedes the Steam-issuance
boundary in the preceding EOS increment. No Python server participates in
these routes; C++ retains the existing native SteamLogin/OnlineAccount client.
The [runtime runbook](../../rust/matchmaking_service/README.md) describes the
explicit issuer and optional Web API key configuration.

Only the fixed Valve OpenID endpoint can produce a private verified Steam
owner. Rust validates exact return destination, provider, identity, signed
fields, duplicate-free query and fresh nonce before direct provider
verification. Valve documents browser SteamID authentication through
[OpenID](https://partner.steamgames.com/doc/features/auth); direct verification
uses the OpenID 2.0
[direct signature verification procedure](https://openid.net/specs/openid-authentication-2_0.html#check_auth).
The callback itself cannot supply an authoritative owner or persona. Optional
persona lookup uses the fixed Steam Web API and checks the returned identity.
Provider HTTP disables ambient proxies and redirects, verifies TLS and bounds
time, bytes and decoding. This modern identity mechanism asserts no original
Xbox Live protocol or game-ownership verification.

Pending state binds an immutable PKCE challenge, game-code hash, generation,
issuer and monotonic five-minute expiry. Browser confirmation requires its
private HttpOnly/SameSite cookie, matching origin and the twelve-hex-digit
code displayed by the native client. Five wrong codes retire the attempt.
SQL COMMIT precedes nonce, credential and pending-state publication. A failed
COMMIT permits retry without exposing a provisional sign-in. Saved credentials
cannot authorize account APIs; resume preserves their original thirty-day
expiry and forget atomically revokes only their mapped active credentials.
Only hashes are persisted. Restart retains valid saved credentials and
revocations while dropping pending browser state.

Rust retains the existing four-active-credentials-per-subject, 1000-active and
100000-remembered capacity contracts. Filling the remembered store still
permits verified login without issuing a new remembered credential; foreign
subjects are not evicted to admit it. Pending anonymous limits are four per
source, eight per prefix and 128 overall, within 1000 total pending attempts.
Rejected occupancy preserves the independent global start allowance. These
limits and the eight-hour active credential lifetime are modern service
policy, separate from recovered original matchmaking behavior.

Two provider workers retain their request slots through actual IO, with a
two-second queue wait and twenty-second owned operation deadline. A shared
cancellation guard exists before blocking preflight can change pending state.
Cancellation during that handoff or while queued retires verification and
retains capacity until its actual work finishes. Late receipts recheck caller
liveness, issuer, generation and expiry before SQL or browser publication.

Dual-provider linking uses the existing Rust canonical adoption owner. Both
Steam and Discord proofs are verified; the current Steam credential is checked
again after provider IO and inside the canonical transaction. Failed COMMIT
preserves both identities and grants; retry preserves the clan/profile and
retires only the adopted bare account. An isolated real SQLite wait exposed
an additional shared runtime defect: the transaction reused its pre-lock
clock, allowing an expired Steam proof to allocate one canonical player.
The runtime now refreshes its clock after acquiring the SQL transaction. The
same actual lock/expiry test rejects 401 and writes no player after the fix.
The broader HTTP lock-wait control also passes, but background maintenance can
take the runtime mutex first; the isolated unit is the direct red/green proof.

All fourteen Rust service unit tests and the full HTTP suite pass, including
the real SQL expiry boundary, capacity controls, both cancellation windows,
failed COMMIT/retry, canonical linking, provider-response validation and
restart. Qualification uses the default production Rust executable with only
fixed-provider curl IO interposed by an independent private HMAC fixture.
The native Steam probe drives its real browser URL/device/code/poll flow,
ONLINE clan view and sign-out against Rust. These tests supply no genuine
Steam/Discord authorization. Room controls retain the full ten-second start
and explicitly synthetic datagrams, PCR and map fingerprints.

The final coherent native cohort passes both CTests in 180.65 seconds,
including the build helper and all five native client probes. Its fourteen
service units pass without test-only Rust features. Copied source remains
unchanged during validation; current executable/helper inputs match, with
subsequent documentation-only drift recorded separately.

The [Rust Steam service receipt](evidence/matchmaking-rust-parity-20261028/rust-service-steam-validation.json)
records source pins, commands, real failures, compile/fixture corrections,
native cohort results and owned-storage cleanup. The previous EOS backend
cohorts are reused only where their identical source hashes still match;
their 296 Rust/579 backend and 274 Rust/371 backend results are earlier checks,
not newly executed Steam-service tests. This increment changes no original
math or shared backend implementation.

Remaining social/voice/provider effect routes, real Steam/Discord/Epic/SDK
authorization, EOS native peer/world acceptance, clock/epoch/crash recovery,
original configuration/order/arbitration recovery, every required family,
profile, topology and natural outcome, actual Linux playlist/Start input and
delivered G01-G12 acceptance remain open. Full readiness, parity certification
and goal completion remain false. The selected service and runtime are not
activated or replaced. All twelve GAP criteria, 53 algorithm IDs and twelve
acceptance gates are preserved.

## Rust service friends and observed population

The standalone Rust server now exposes `GET /v1/friends`,
`POST /v1/friends/commands` and anonymous `GET /v1/online/status`. The initial
unauthenticated friends request returned the explicit unported-route 503;
it now checks account authority and returns 401, while valid reads and commands
reach the existing shared Rust owners. No Python server or coordinator
participates in these routes.

Friend commands retain the existing Rust sender/recipient consent, public tag
and Unicode matching, bilateral block, request replay, capacity, sorting and
party-invitation policy. The service owns a private Rust block cache whose
entries bind the SQL epoch. Provisional cache plans publish only after real
SQL COMMIT; failure retires the directory and its pending cache work. HTTP
cannot supply a native handle, actor identity or trusted party context.
Duplicate query pairs reach the native parser intact.

Friend presence and optional `party=1` contexts come from actual admitted
account, matching-session and room owners. The IO adapter drives private
Rust party joinability/context plans within the same SQL transaction; it
neither computes admission policy nor invents presence. A nonfriend search
returns public tag/identity data with offline presence. Accepted friends can
see the existing public party context, subject to the shared privacy,
membership, activity and block gates.

Anonymous population accepts no filters and publishes only aggregate
user/guest and playlist activity counts. It deduplicates canonical signed
accounts, retains transient guests and selects activity through the existing
Rust presence owner. Counts cover idle, queued, joining, playing and postgame
states. Repeated anonymous reads do not renew activity or account grants.
These modern observations and friendship procedures do not recover original
Xbox Live presence flags or friend-service behavior.

Wire controls exercise consent and replay conflicts, Unicode tags, forged
fields and duplicate queries, privacy/context projections, block revocation,
cancel/decline/remove, canonical rename and restart. A real deferred block
COMMIT failure leaves the accepted relationship, block epoch and party
invitation intact; the identical request retries once after removing the
private failure trigger. Revoking an account during actual body upload
prevents any subsequent friend SQL. A declared private 250-row SQL roster
crosses the real 128-row continuation, preserves every unique public row and
rejects the next friendship at capacity. This fixture is not provider or
population evidence for those synthetic rows.

The real HTTP activity-window control waits the full thirty seconds while
reading anonymous counts. The idle account disappears; using its still-valid
account grant restores observed activity. Other controls use real matching
sessions with a transient guest, queue/cancel transitions and private response
checks. Existing relay controls check anonymous counts during actual
loading/running/postgame transitions, including retained room ownership after
frontend logout. Their full ten-second start, synthetic packet/PCR/map
fixtures and lack of a played native world remain explicit.

The native social probe drives two real OnlineAccount clients through friend
search, request, recipient acceptance, private presence, party context/join
and bilateral block over Rust HTTP. Only its provider handoff is a private
fixture. The [Rust social service receipt](evidence/matchmaking-rust-parity-20261028/rust-service-social-validation.json)
records actual validation results, source identities, commands, fixture
corrections and scratch cleanup. Prior shared backend results are reused
only after their source hashes are checked; this increment changes no shared
backend implementation or original mathematical kernel.

The frozen social service cohort passes both CTests in 217.60 seconds.
All fourteen service units and six native probes pass, including the actual
two-account friend/party join flow. The copied inputs remain frozen; current
service and helper hashes match. Unrelated application/rendering source drift
is recorded separately. Earlier unchanged backend cohorts retain their 296
Rust/579 backend and 274 Rust/371 backend results; those checks were not
rerun as new social-service acceptance.

Concurrent CMake edits after validation also changed the build recipe. The
receipt retains the exact tested frozen CMake hash and subsequent observed
hashes. Current service/helper inputs still match; the current complete build
configuration is not requalified by this frozen result.

Remaining standalone service wiring includes private friend-file outbox IO,
voice preferences/social relay and Steam relationship/invitation IO. Genuine
provider/SDK and EOS peer/world qualification, complete clock/epoch/crash
recovery, lawful original configuration/service/order/arbitration recovery,
every required family/profile/topology and natural outcome, installed Linux
playlist/Start input and delivered G01-G12 acceptance remain open. Full
readiness, parity certification and goal completion stay false. No selected
runtime or production service is activated or replaced. All twelve GAP
criteria, 53 algorithm IDs and twelve acceptance gates remain unchanged.

## Rust service Steam relationship and invitation IO

The standalone Rust server now exposes the existing account Steam-friends
route and completes `invite_steam` party commands. The initial unauthenticated
relationship request returned the unported-route503; it now checks account
authority and returns401. Configured valid requests execute the existing Rust
Steam request/cache owner directly. No Python coordinator or server
participates in these routes.

Rust retains the fixed GetFriendList/GetPlayerSummaries sequence, ordered
selection of at most1000 identities,100-ID profile batches, online-profile
filtering, current canonical registration, bilateral blocks, observed presence
and public ordering. These modern provider methods are described in
[Valve's ISteamUser documentation](https://partner.steamgames.com/doc/webapi/ISteamUser).
The native cache retains its60-second entry lifetime, serialized upstream
lease,128 waiters,1000 cached subjects and64MiB byte bound. These are declared
replacement-service policies, not recovered Xbox Live behavior.

Each SQL observation commits before the continuation receives its row banks.
Account authority and current canonical Steam aliases are rechecked after
provider IO. Curl uses fixed HTTPS destinations, verified TLS, no ambient
proxy or redirects, identity encoding and a bounded1MiB response. Neither the
runtime mutex nor a SQL transaction is held during provider IO. Existing HTTP
request capacity bounds lookup workers at64; those slots remain owned through
actual IO and cleanup. A native-owner drop retires its own FIFO entry without
retiring the following owner or releasing its HTTP slot early.

Steam party invitations use a private one-use proof bound to the actor,
native account key, target, exact command payload and provider credential
context. The same Rust party directory validates and commits invitation,
request replay and view state together. Its proof is consumed after COMMIT;
failed COMMIT rolls back SQL and retires provisional directory work. A
committed request replays without another provider lookup. Client-supplied
claims are rejected. Command view flags remain selected by the native
preflight rather than the IO adapter.

The existing request owner permits75 seconds, while ordinary HTTP connections
previously closed after30. Relationship requests now have a typed extension
within an absolute95-second connection ceiling. Ordinary connections retain30
seconds; header/body reads retain five. A real33-second eleven-call lookup
passes. The native OnlineAccount client retains its existing three-second HTTP
timeout, so this long operation qualifies the server boundary. Slow/queued
native client behavior remains an explicit open requirement.

Fifteen service unit tests pass, including actual FIFO/HTTP-slot retirement.
Real HTTP controls exercise canonical/account revocation during provider IO,
independent SQL and health access while IO is active, sealed invitation and
deferred COMMIT rollback/retry, private upstream refusal, duplicate JSON,
compressed/redirected/oversized/unavailable responses and malformed profiles.
The selected1000-row reply crosses the real profile/SQL continuations and
exceeds256KiB. The cache control waits its full sixty seconds before observing
a newly private upstream. Provider payloads and credentials remain private
fixtures; these controls make no real Steam relationship or authorization claim.

The [Rust Steam relationship receipt](evidence/matchmaking-rust-parity-20261028/rust-service-steam-friends-validation.json)
records the native client result, all commands, pinned inputs, compile/fixture
corrections, timing measurements and cleanup. Its frozen CMake source is
retained as a bounded text reference alongside its hash, so concurrent build
recipe edits remain distinguishable. Earlier shared backend evidence is
reused only after its source hashes match; no original mathematical kernel
or shared backend implementation changes in this increment.

The frozen Steam relationship cohort passes both CTests in 318.90 seconds.
All fifteen service units and six native probes pass. The native Steam/account
client accepts the complete1000-row,332044-byte reply. The server lookup
measures33.03 seconds and cache expiry61.01 seconds; these do not qualify
the native client beyond its existing three-second HTTP budget. Copied inputs
remain frozen. Current service/helper inputs match, with post-test README
changes and any concurrent build-recipe drift recorded separately.

Remaining standalone service effects include private friend-file outbox and
voice preference/social relay IO. Native slow/queued Steam lookup, genuine
provider/SDK and EOS peer/world qualification, complete clock/epoch/crash
recovery, lawful original configuration/service/order/arbitration recovery,
all required families/profiles/topologies and natural outcomes, installed
Linux playlist/Start input and delivered G01-G12 acceptance remain open. Full
readiness, parity certification and completion remain false. No selected
runtime or production service is activated or replaced. The twelve GAP
criteria,53 algorithm IDs and twelve acceptance gates remain intact.

## Native Steam HTTP budget in Rust

The native account client reproduced an incomplete caller boundary: the Rust
server could finish a long relationship lookup, while the client's fixed
three-second Curl deadline returned Steam friends as unavailable with zero
rows. The same actual native browser/account path now completes after the
timeout selection moved into the shared Rust owner and CXX bridge.

Rust admits 95 seconds only for the exact Steam-friends GET and a party command
whose bounded, duplicate-safe JSON action is invite_steam. Query-bearing native
party commands retain this allowance. Ordinary account operations retain three
seconds; lookalike paths, fragments, malformed/duplicate JSON and other commands
cannot select the larger budget. C++ passes the typed budget to Curl and keeps
its existing stop-token progress callback.

The [native Steam budget receipt](evidence/matchmaking-rust-parity-20261028/native-steam-budget-validation.json)
records the genuine native unavailable/zero-row red, the fix, and a corrected
cancellation-fixture address after process restart. The native browser/account
lookup flow takes 37.15 seconds; the queued lookup takes 66.08 seconds. The cold
invitation commits exactly one command and invitation after 61.11 seconds of
real cache expiry and 5.05 seconds of request time. Cancellation and destruction
finish in 0.79 seconds during active provider IO.

The final focused CTest passes in 253.42 seconds; the two existing
native service/build-helper CTests pass in 320.27 seconds. Fifteen
service units and seven native executables are qualified across these cohorts.
Fresh identical 298-file backend cohorts pass 298 Rust/579 backend checks with
diagnostic inputs and 276 Rust/371 backend checks through the production ABI.
The Linux-only fixture registration keeps the same command after reconfiguration.
Their libraries are built privately and all generated source/build/DB/provider
storage is removed; bounded receipts and exact source identities remain.

Only fixed upstream provider IO is substituted. This is modern service/native
caller proof, with no new original executable calls or genuine Steam grant.
Private friend-file and voice effect IO, provider/SDK/world qualification,
complete clock/epoch/crash recovery, lawful historical configuration/service/
order/arbitration, every required family/profile/topology/natural outcome,
installed Linux playlist/Start input and all delivered G01-G12 gates remain
open. Full readiness, parity certification and goal completion stay false;
the selected runtime and production service are unchanged. All twelve GAPs,
53 algorithm IDs and twelve acceptance criteria remain intact.

## Rust private friend-file outbox IO

The standalone server previously rejected enabled friend storage at startup.
It now runs the existing Rust outbox policy directly, without a Python
coordinator. Account login/name changes, accepted friend consent and verified EOS
binding enqueue committed public records. Binding and enqueue share their
actual SQL transaction. A reproduced queue-only room failure now creates the
missing public history identity through the existing Rust owner in that same
transaction. Failed COMMIT rolls back history, PUID, outbox and native room
proof; a different verified PUID can then retry successfully. Only existing
canonical EOS identities are eligible;
unbound peers retain pending records without fabricated accounts or PUIDs.

The worker receives a real Rust RS256 delegation for that exact canonical
account, HTTPS issuer, audience and 120-second lifetime. Its dedicated config
is bounded and read through a private regular-file descriptor with final
symlink/FIFO rejection. The private cache belongs to the service user. Only
SDK runtime variables, that config and an operation-owned cache cross the
child boundary; Steam/publisher credentials, preload hooks and master paths
are excluded. The existing Rust SDK worker contract is inspected read-only.
Real SDK/provider/cloud authorization is not attested by these private fixtures.

SQL and Runtime ownership end before worker IO. A reply must contain the exact
downloaded record. The owned child is killed and reaped before its cache is
removed. The native claim binds revision, digest and PUID; old successful
downloads cannot acknowledge a newer revision or changed identity. A failed
completion COMMIT retains the sealed claim for acknowledgement retry without
another upload. SIGTERM cancels owned IO and restart retries the same durable
pending revision through a fresh claim.

The [friend-file receipt](evidence/matchmaking-rust-parity-20261028/rust-service-friend-cloud-validation.json)
records the genuine startup 503 red, real failed consent/binding/completion
COMMIT controls, independently verified signing, stale revision/identity
checks, strict worker framing and reaping. Retry takes
30.12 seconds, the unshortened worker deadline
130.28 seconds, and shutdown
0.008 seconds. The four frozen native/service CTests
pass in 748.04 seconds, with fifteen service units and six native client
probes. Exact bounded source identities and cleanup checks are retained.

Earlier shared backend proof is reused only after all 298 frozen files match:
298 Rust/579 backend checks and 276 Rust/371 backend checks through the production
ABI. These are previous unchanged-core checks, not new original executable,
SDK or delivery proof. Current service/helper inputs match the frozen cohort
except for post-test README changes. Concurrent changes to the native snapshot
and build recipe are recorded separately; this is no whole-game build claim.

Voice preferences/social/clan relay IO, complete clock/epoch/cross-store/crash
recovery, genuine providers/SDK/native worlds, lawful historical service/config/
ordering/arbitration recovery, every required family/profile/topology/natural
outcome, installed Linux playlist/Start input and delivered G01-G12 acceptance
remain open. Full readiness, parity certification and goal completion remain
false. No selected runtime or production service is activated. All twelve GAP
criteria, 53 algorithm IDs and twelve acceptance gates remain intact.

## Rust private social voice IO

The standalone Rust server now exposes private provider-bound voice preference
snapshots and opt-in clan/accepted-party WebSockets. The shared Rust directory
and relay remain the policy owners: canonical identity, accepted membership,
membership incarnations, bilateral blocks, replacement, sequence/burst limits,
five-frame delivery queues and expiry. The service only performs typed HTTP,
SQLite and socket IO. A client-supplied Discord snapshot remains a preference;
it does not certify a provider relationship graph.

The preference route retains the 48 KiB/2,000-subject contract and rechecks Steam
proof expiry after the actual SQL write-lock wait. Relay account and membership
checks run before and after upgrade and use fresh clocks after transaction
waits. Only successful COMMIT publishes attach/frame/poll effects. Actual
SQLite COMMIT denial proves that rejected attachment/frame/poll cannot publish
a peer, consume a sequence or lose queued delivery. Poll also rechecks blocks
after enqueue. Old socket cleanup cannot retire its replacement.

Transport IO retains the ten-second idle heartbeat, five-second response
deadline and 250 ms send limit. Shutdown fences both attached peers and already
upgraded transport owners. A bounded close handshake lets the peer acknowledge
the wire close before the socket is dropped. Initial close diagnostics observed
client 1006; the isolated trace then captured an actual 1001 close frame followed
by failed acknowledgement. The final tests receive both revoked channels and
shutdown closes concurrently, matching independent client receive loops.

The [voice receipt](evidence/matchmaking-rust-parity-20261028/rust-service-social-voice-validation.json)
records real HTTP/SQLite/WebSocket checks, restart, failed preference COMMIT,
SQL-wait proof expiry, membership reincarnation, replacement, replay/burst and
revocation. The measured heartbeat is 10.05 seconds, missing
response deadline 5.03 seconds, and shutdown
0.016 seconds. Five frozen CTests pass in 916.30 seconds
with 17 service units and seven native probes. The existing OnlineVoice clients
exercise actual Opus with synthetic PCM for both clan and party channels,
including idle heartbeat, opt-in capture, push-to-talk, participant/microphone
mute and identity revocation. Provider grants and aliases are private fixtures;
physical microphone/speaker use, real provider authorization and a native played
world are not attested.

The same coherent source cohort rechecks accounts, playlists/shared Start,
search QoS, room loading/results/requeue, Steam browser/social IO and private
friend-file storage, including the queue-only EOS publication regression.
Earlier 298-file shared backend checks are reused only after matching every
input hash; they are not rerun or new original executable proof. The complete
frozen native/build-recipe identities and any concurrent checkout drift are
recorded separately. No whole-game build is claimed.

A separate [registration-clock preflight](evidence/matchmaking-rust-parity-20261028/rust-service-registration-clock-preflight.json)
is red: new social profiles use raw wall time below the persisted registration
floor. Actual legacy Store.now and SteamLogin.owner calls confirm the distinct
contracts. Both valid Steam credentials are correctly accepted, and an expired
one is rejected; the profile dates are 600 seconds behind the declared private
floor. The initial attempt to apply this registration floor to Steam expiry was
an incorrect test expectation and is retained with that classification. This
follow-up defect remains open outside the five passing CTests.

Complete clock/epoch/cross-store/crash recovery, genuine providers/SDK/storage/
native worlds, lawful historical configuration/service/order/arbitration
recovery, every required family/profile/topology/natural outcome, installed
Linux playlist/Start input and delivered G01-G12 acceptance remain open. All
twelve GAP criteria, 53 algorithm IDs and twelve acceptance gates remain intact.
Full readiness, original parity and goal completion remain false; no selected
runtime or production service is activated. These Opus transports are modern
service behavior, not recovered original Xbox voice.

## Rust social registration clock IO

The prior date preflight is now corrected in source. The standalone login used
raw wall time for a new profile even when the canonical registration floor was
later. The actual legacy ClansAPI.login passes Store.now into identity
resolution. Actual Store.now and SteamLogin.owner calls establish separate
registration and credential domains; applying the registration floor to Steam
expiry was an incorrect initial test expectation, retained with that label.

The shared Rust canonical owner now exposes registration_clock and reuses it
for Discord linking. Login invokes this SQL operation inside its actual
transaction, then resolves the profile with that registration value. Wall and
monotonic observations refresh after BEGIN. Steam proof revalidation and
friend-file scheduling retain their independent wall inputs. Returning profiles
keep their original dates, and unknown legacy dates remain NULL.

The [registration-clock receipt](evidence/matchmaking-rust-parity-20261028/rust-service-registration-clock-validation.json)
records the fresh 600-second mismatch and its green replay. Seven real HTTP/SQL
controls pass: distinct domains, floor advancement, returning dates, unknown
dates, restart, a newer floor during the SQL write lock, expiry during that
wait, and deferred COMMIT failure/retry. Two new Rust service units also deny
the actual SQLite COMMIT and observe the blocked BEGIN before advancing the
private floor. The date, canonical identity and floor roll back together;
retry publishes one usable account. No system clock or live account is changed.

The same frozen native/service cohort passes six CTests in 886.74 seconds,
with 19 service units and seven native probes. It rechecks accounts, playlists/
shared Start, search/room/results/requeue, Steam/social/EOS, private friend-file
IO and native clan/party Opus transport using private provider fixtures.
The shared-core change also receives fresh 298-file backend cohorts:
298 Rust/579 backend checks with diagnostic inputs, and 276 Rust/371 backend
checks through the production ABI. Their complete inputs are identical and
both actual libraries report ABI 2 with the expected diagnostic capability.
These are new component/adapter checks; no original executable is called.

The [next native-world input audit](evidence/matchmaking-rust-parity-20261028/rust-native-world-acceptance-inputs-20261101.json)
pins all 39 authored 304-byte presets across seven engine IDs and matches their
recorded original fixture. Current Rust room probes use synthetic datagrams/PCR.
Existing source-world tests use edited fixtures or a Python HTTP driver. Full
unmodified native-world outcomes through the Rust service remain unqualified.
This inventory prepares the complete next scope without claiming runtime proof.

Full clock/epoch/cross-store/crash recovery, genuine provider/SDK/storage/native
worlds, lawful historical configuration/service/order/arbitration recovery,
every required family/profile/topology/natural outcome, installed Linux
playlist/Start input and delivered G01-G12 acceptance remain open. All twelve
GAP criteria, 53 algorithm IDs and twelve acceptance gates are retained. Full
readiness, certification and goal completion stay false. No selected runtime
or production service is activated. This registration policy is a modern
service boundary, not recovered original Xbox behavior.

## Native queue-client authority now resides in Rust

The native queue controller now lives in
`rust/online_protocol/src/matchmaking_client.rs`. Rust owns configuration
admission, session grants/configuration/lease renewal, queue request identity and
uncertain-POST retries, command generations and cancellation, terminal fences,
strict catalog/ticket/transport parsing, full settings/digest admission, local
seats and roster binding, QoS lifetime decisions and the single loading handoff.
Runtime compatibility comes from the actual shared Rust EOS configuration.
C++ supplies typed login, clock, HTTP, cancellation, native connection and
presentation IO. The queue adapter has no C++ controller-policy fallback.
Other native peer policy remains outside this completed queue-client increment.

Commands and snapshot publication share one Rust lock; blocking worker IO uses
a separate owner. A late successful session exchange remains owned for cleanup.
Invalid native UTF-8/profile observations become explicit Rust admission facts;
Join or Cancel cannot reuse a previous valid name after an invalid profile
refresh. A valid refresh restores that profile boundary.

A new red control reproduced lease rotation submitting a second queue POST
after gameplay took the room: two POSTs instead of one. Rust now renews the
external authorization and QoS capability while retaining the transferred-room
fence. Only an explicit new command retires it. The passing control verifies
that renewal actually occurs and that enqueue and native-install counts remain
one. This is a declared controller IO fixture, not a played-world continuity
claim.

The [queue-controller receipt](evidence/matchmaking-rust-parity-20261028/client-controller-rust-validation.json)
records 22 controller tests, all 39 authored settings buffers and mutations at
every one of the 304 digest-bound byte positions. Final identical backend
cohorts of 307 files pass 328 Rust/580 backend checks with diagnostics and
306 Rust/371 backend checks through the production ABI. Both actual libraries
report ABI 2 with their expected diagnostic capability.

The final native cohort of 4,546 frozen files passes eight CTests in 410.80 seconds, including
20 Rust service units, actual native playlist/Start/countdown commands through
the Rust HTTP service, native QoS sockets, room/results controls, original
12/50/5 readiness/error controls through CXX and two complete-settings failed
launches. Twelve undefined Rust controller entry points in the compiled queue
object confirm its IO boundary; that object no longer references C++ JSON,
SHA, request-ID RNG or native variant-validation policy. Private provider grants,
legacy room/PCR/datagram fixtures and the negative EOS handoff remain explicit.
Positive genuine EOS play, native worlds and installed input are unqualified.

The first SDK-enabled service test stopped at the probe's startup marker. The
unchanged pinned SDK prints `Shutdown handler: initialize.` before `main`.
The readiness reader also lost following bytes through buffered pipe reads.
Three of six focused controls were red before the correction; all six pass
afterwards. The reader permits only the known prelude, requires the exact READY
marker, preserves following bytes and bounds startup to 4,096 bytes/five
seconds. No production admission rule was weakened. Earlier compiler/ABI and
source-cohort diagnostics remain retained. A concurrent audio CMake update
required a fresh complete source cohort; that work was preserved.

This is the replacement service's client protocol. No new original executable
calls establish original Live wire, configuration, ordering or arbitration.
Previously authenticated Rust readiness/variant kernels remain shared owners.
Historical recovery, full original callers, every required natural outcome and
continuity, remaining native peer policy, rendered/installed Linux menu input
and delivered G01-G12 acceptance remain open. All twelve GAP criteria, 53
algorithm IDs and twelve acceptance gates are unchanged; readiness,
certification and goal completion remain false. No production service or
selected runtime is replaced.

All owned scratch, source/build and SDK extraction trees are removed. Exact
report deduplication retains 206 bounded reports totaling 7,498,872 bytes and
maps all 326 authenticated source reports to their retained payloads. Duplicate
working reports were removed. No generated binary, raw bank, frame or large
artifact is retained.

## Rust native QoS worker and actual Rust-service lobby controls

The complete native QoS worker now lives in
`rust/online_protocol/src/matchmaking_native_qos.rs`. Rust owns origin and
bearer admission, strict one-field challenge JSON, frame/chunk assembly,
interleaved Ping/Pong controls, partial writes and AGAIN retries, send/idle
deadlines, cancellation and reconnect scheduling. The queue and QoS clients
share `matchmaking_origin.rs`. C++ retains typed URI observations, CURL socket
IO, steady-clock/cancellable scheduler observations and thread/resource
ownership. It has no QoS parsing or retry-policy fallback.

The red worker control acknowledged an empty Pong after CURLE_AGAIN: it sent
once instead of retrying. The passing Rust control now requires a successful
acknowledgement even for a zero-byte payload. Successful partial writes advance
the remaining bytes; AGAIN preserves the pending data and length, following
the [libcurl send contract](https://curl.se/libcurl/c/curl_ws_send.html).
All 13 worker controls and the existing 22 queue-controller controls pass.

An additional one-off native socket control compiles against the actual frozen
archive, borrowed read-only with its hash checked before/after. A private IO
shim returns AGAIN once for the real empty-Pong send. Rust retries, makes two
send calls and delivers the Pong. The then-C++ match-session loop makes one
call and loses it. That report retained this room defect for the next complete
Rust session-controller migration; the session qualification below proves its fix.
Two early socket-fixture teardown failures remain explicit diagnostics;
aiohttp's test helper cancels pending handlers when the native client closes
TCP. The private fixture now disables that cancellation and awaits completion,
without changing production parsing or wire policy. See the
[aiohttp disconnect contract](https://docs.aiohttp.org/en/stable/web_advanced.html#peer-disconnection).

Final identical backend cohorts of 310 files pass 341 Rust/580 backend checks
with diagnostics and 319 Rust/371 checks through the production ABI. Both
actual libraries report ABI 2 with their expected diagnostic capabilities.

The [QoS and frontend receipt](evidence/matchmaking-rust-parity-20261028/native-qos-rust-validation.json)
records 11 unique native CTests across 12 executions in 764.65 seconds, from
13,354 frozen source, generator and shader inputs. The compiled QoS object
requires the Rust constructor and worker entry points; the compiled queue
object still requires all 12 Rust controller entries. Neither object references
C++ JSON, SHA, request-ID RNG or native variant admission policy. Original
readiness/error fixtures, 20 service units, native QoS socket controls and the
complete-settings failed-launch isolation remain part of the native checks.

An earlier native build received SIGTERM before CTest began; its storage owner
removed the partial build. A separate fresh freeze correctly refused a
concurrent audio CMake edit. Those receipts are interruption/coherence
diagnostics, never passing evidence. The final fresh build uses a bounded,
owned supervisor and freezes generator/shader code as well as C++/Rust
sources. It preserves unrelated active work.

The first full suite passed ten tests. The native rendered program completed,
then its Python collector looked in the frontend subfolder while the child
had inherited its parent's scratch path. The private child now receives that
frontend subfolder as HALO2_TEST_SCRATCH. A focused rendered rerun passes with
the same native executable and identical production C++/Rust inputs; this is
the sole declared helper refresh. The receipt records all twelve attempts,
eleven unique successful checks and the original collection failure.

Three new Linux tests drive the actual native menu against the real Rust
HTTP/SQLite service and the unchanged 14-playlist source catalog. An
incompatible reader receives visible Start and playlist errors and retains
the chooser without queueing. The compatible reader selects a playlist,
returns to the retained lobby, exercises acknowledged privacy and busy
operations, starts the authoritative shared countdown, enters search and
cancels back to the lobby. The rendered case derives mouse coordinates from
admitted font glyphs and the menu's actual clipped hit layout, then delivers motion and left-button
down/up events to the actual window for Choose Playlist, a playlist row and
Start Game. No direct MenuState activation is supplied for those actions. It
also sends keyboard events through the Vulkan frontend and records 23 screen
observations, including countdown
3/2/1, search and return. It uses the real UI intro/clock. Nonuniform frames
and phase-specific state are checked; raw frames remain owned scratch and
are removed. This does not establish Xbox framebuffer parity.

Only the external Steam handoff is a provider fixture. A private IO shim
delays actual party replies by 150ms so the busy guard is observable; it
changes no request or response. Direct/eos transport selection is explicit
fixture configuration for the actual v12/v16 reader; EOS stays disabled and
no positive EOS handoff is claimed. No Python API or coordinator supplies the
party, countdown or queue decisions. Result/PCR tables and personal/clan XP
remain empty in all three UI cases.

This qualifies the replacement relay worker and a real native frontend path.
It supplies no new original executable calls or recovered XNet/Live provider,
ordering, historical configuration or arbitration. At this prior source cohort, the native match-session
wire/controller was a separate Rust migration boundary; the session
qualification below now owns that state and wire in Rust. Genuine providers,
positive EOS/native gameplay, all required natural outcomes and continuity,
physical installed Linux input and complete delivered release acceptance stay
open. Every GAP and G01-G12 gate remains open; the 53 algorithm IDs and their
requirements are unchanged. No selected runtime or production service is
activated. All owned source/build/SDK/frame storage is removed; only bounded
deduplicated reports are retained.


## Full variant admission now calls Rust

The actual catalog parser admitted a Crazy King settings buffer with signed-short
timer `0xF4 = -1`. The unchanged TU1.5 read-only validator refused the same
304-byte buffer and preserved its caller. The original normalizer resets that
invalid input to Slayer; matchmaking must refuse it instead of selecting the
reset game.

The shared safe Rust validator now owns this complete validity contract. The
catalog/model, actual Rust service startup, native queue and frontend admission
use it. Native consumers pass bytes through typed CXX adapters. The native
gameplay normalizer retains its separate gameplay reset operation. All 547
current catalog buffers remain valid and unchanged; no catalog row was edited.

The [variant admission receipt](evidence/matchmaking-rust-parity-20261028/variant-admission-rust-validation.json)
records 4,835 fresh unchanged TU1.5 comparisons: 4,249 retained normalization
controls, all 39 authored presets and 547 source-catalog buffers. The validator,
normalizer and reset initializer execute original instructions. Every executed
byte is verified, with no patched instructions, replaced callees or caller
mutation. The explicit null locale operand and synthetic controls remain
declared inputs; this does not qualify the complete original upstream caller.

Identical frozen backend cohorts of 305 files pass 306 Rust/580 backend checks
with diagnostic inputs and 284 Rust/371 backend checks through the production
ABI. Both actual libraries report ABI 2 with the expected diagnostic capability.
A frozen native cohort of 4,510 files passes six CTests in 79.90 seconds,
including 20 Rust service units, all 4,249 original buffer controls through CXX,
runtime fields, and actual cache/tag rejection of invalid authored entries.
Compiled frontend and queue objects reference the shared Rust predicate.

The first native run also exposed an old test that sent `0xFFFFFFFF` through
the reserved native extension namespace while expecting an absent original
descriptor no-op. The corrected test checks raw original absent descriptors
directly and preserves unknown native extension rejection. Production guards
were unchanged. The initial catalog regression and both test-fixture diagnostics
are retained alongside the final passing results.

Full native controller migration, genuine providers and SDK play, rendered and
installed Linux input, natural native-world outcomes and continuity, lawful
historical/service recovery and delivered G01-G12 acceptance remain open. The
compiled frontend/cache checks do not establish installed menu behavior. All
twelve GAP criteria, 53 algorithm IDs and twelve acceptance gates are retained;
readiness, certification and goal completion remain false. No selected runtime
or production service is replaced.

All owned build, scratch and SDK extraction trees are removed. The retained
bounded evidence totals about 5.87 MB; one routine build log retains a 1 MiB
tail with its complete original size and hash. Authenticated duplicate reports
were removed. The existing required 5,265,946-byte original reference is
unchanged. No generated binary, raw bank, frame or large artifact is retained.
Concurrent physics and audio changes remain separate from the frozen inputs.

## Native readiness and lobby policy now call Rust

The native connected-team and eligibility callers now use the same Rust owner
as the service. Original lobby error selection and unsigned error-expiry rules
also reside there. Their former C++ header bodies are removed; the native
adapter marshals typed facts through generated CXX. The final loading handoff
now asks Rust to evaluate session/transfer/loading observations, supported
countdown zero and admitted seat ownership. C++ retains connection ownership
and presentation. There is no C++ policy fallback.

The loading-to-complete-link projection preserves the existing declared native
transport policy. It does not recover original XNet measurements, and it never
invents original Live logon or downloaded local-policy bits. Full client
controller migration remains separate from these five native entry points.

Fresh unchanged TU1.5 runs retain 12 readiness controls, four policy controls,
four producer/consumer controls and ten caller branches. A separate run retains
50 error selections and five state-clear cases. Every executed instruction byte
is verified; widget construction remains a stop boundary. The prior readiness
fixture is preserved, and the new error fixture adds a reproducible original
reference. Full original policy download, membership transport and session
publication are outside these controls.

The [native readiness Rust receipt](evidence/matchmaking-rust-parity-20261028/native-readiness-rust-validation.json)
records 27 Rust tests and three backend tests in a frozen production ABI cohort
of 302 files. The actual library reports ABI 2 with diagnostic inputs disabled.
A frozen native cohort of 4,482 files passes five CTests in 80.03 seconds,
including all 12/50/5 original controls through CXX and 19 Rust service units.
The native archive has five adapter definitions referencing the Rust bridge.
Two full-settings v16 failed-launch attempts still refuse loading without
publishing results, PCR or XP. Provider identities and transport observations
remain declared fixtures.

The operational README now identifies v16 configuration and the required native
Rust linkage. Genuine providers/SDK/worlds, rendered and installed Linux input,
all required natural outcomes and continuity, historical/service recovery and
delivered acceptance remain open. All twelve GAP criteria, 53 algorithm IDs
and G01-G12 gates are retained; readiness, certification and completion stay
false. No production service or selected runtime is replaced.

All owned builds, SDK extracts and scratch are removed. The retained bounded
reports total about 2.84 MB; the new original error reference is 6,572 bytes.
No generated large artifact is retained, and concurrent physics work is
preserved separately from the tested snapshot.

## Native launch refusals and TU1.5 assignment replay

The actual native v16 queue client now has a focused regression through the
Rust service, using its unedited source catalog and complete 304-byte settings.
Two native clients independently check every selected settings byte and the
same game/map/variant identity. The first private build omitted the SDK archive;
an explicit official SDK build reaches the intended missing-product client
configuration refusal. The probe bundles the same SDK worker/library path used
by native builds. Its provider handoff remains a private Steam fixture.

The [native handoff receipt](evidence/matchmaking-rust-parity-20261028/rust-native-handoff-failclosed-validation.json)
records four focused CTests passing in 80.24 seconds. The negative control
reuses the same private identities and Rust service for two attempts, restarting
the native clients between attempts. Both attempts validate full settings and
refuse loading, without result/PCR publication, XP awards or an SDK cache.
The receipt explicitly leaves positive EOS admission, same-process native retry,
installed map admission, native worlds and physical menu input unqualified.

Fresh unchanged TU1.5 execution independently matches Rust on 336 assignment
decisions and 2,688 getter calls. A fresh retail replay matches all retained
retail cases exactly. Each epoch observes 1,248 distinct original instruction
addresses, verifies their original bytes and replaces no callee. Static
comparison separately locates the TU1.5 eight getters, count/weighted kernels
and assignment corridor; an ambiguous prefix is resolved by the full getter
control flow. The new [TU1.5 assignment receipt](evidence/matchmaking-rust-parity-20261028/tu15-assignment-corridor-validation.json)
retains the original fixture and comparisons. A frozen cohort of 300 files
passes the new Rust integration test and three backend checks; the actual
library reports ABI 2 with diagnostic restoration disabled. The production Rust
assignment implementation required no change for these TU1.5 operands.

These runs start from declared synthetic ready-summary/register/stack operands.
They do not execute full original upstream session/summary/readiness producers
or downstream launch publication. Historical configuration/service/arbitration,
all required natural native matches and continuity, installed Linux playlist/
Start input and delivered acceptance remain open. All twelve GAP criteria,
53 algorithm IDs and G01-G12 gates remain intact; readiness, certification and
goal completion remain false. No production service or selected executable is
replaced. Concurrent source changes are preserved and identified separately
from the frozen inputs.

All owned scratch/build/SDK extraction trees are removed. Bounded reports total
about 5.1 MB; the new required original reference fixture is 242,090 bytes. No
generated binary, raw bank, frame or other large artifact is retained.

## Linux playlist eligibility incident

The selected Linux runtime had its EOS worker and SDK library but lacked the
adjacent `eos-client.env`. Its unchanged Rust settings reader and native queue
adapter therefore advertised v12. Read-only inspection of the live catalog
found that all 14 playlists require v16. With the same installed map inventory,
the actual native environment reader and Rust catalog owner admit zero
playlists under v12 and all 12 non-clan playlists under v16. The two clan
playlists retain their membership requirements.

An authenticated MenuState/HTTP regression reproduced the silent click on an
ineligible playlist. The handler now displays an error and retains the picker.
Its unavailable caption includes client compatibility as well as party/maps.
An additional v16 fixture with complete settings passes playlist selection,
accepted consent, the actual shared lobby countdown, search and cancellation.
Future engine builds preserve the selected ONLINE configuration or use the
existing local EOS setup when no explicit override is provided.

The separate operational repair added the missing shared application settings
beside the selected executable. The executable, selections, qualifications,
profiles and maps were preserved. Reopening the game is required to load the
restored configuration. The updated error presentation is a source change;
the selected executable was not replaced with that source change.

The [playlist incident receipt](evidence/matchmaking-rust-parity-20261028/playlist-buttons-validation.json)
distinguishes the configuration repair from the narrow compiled MenuState/HTTP
experiment, which borrowed existing static dependencies read-only. The UI
experiment uses an independently admitted MCC UI cache; the map/protocol
differential uses the selected cache's map inventory. It does not establish a
coherent new native delivery, rendered/physical input acceptance, a played
match or original Xbox parity. The full Rust parity goal remains open.

## Rust native room/session controller

The [native session receipt](evidence/matchmaking-rust-parity-20261028/native-session-rust-validation.json)
qualifies the authoritative room state in `matchmaking_native_session.rs`
and the complete socket worker in `matchmaking_native_control.rs`. Rust now
owns constructor/roster admission, strict JSON, room phases and membership,
loaded-content hashes over all 304 original settings bytes and signed extension
values, result receipts, voice signatures/preferences/credits, bounded queues,
direct readiness/departure observations and socket framing/retry/deadline policy.
C++ retains typed IO and actual native resource operations under the existing
generation lock. Its phase fields are projections of Rust state.

The preserved C++ source loses an empty Pong when CURL returns AGAIN. Both
current native room and QoS clients now retry the real zero-byte send and the
actual server receives it. A shared Rust write owner covers partial and AGAIN
writes. Relay packets also recheck the actual audio generation immediately
before socket IO, after queue drain.

The complete protocol/bridge/service suites pass 437 Rust tests, including 18
new session/worker controls and 13 existing QoS controls. Fifteen native CTests
are qualified across 18 final executions, including relay continuity, direct
handoffs, survivor selection, audio fencing and real mesh observations. The
compiled Session object references 34 Rust controller entries and no C++ JSON,
SHA or signature policy. The actual native room clients also pass the Rust
HTTP/SQLite loading, real ten-second start, relay, review and PCR/XP transaction
fixture. Packets, reports, fingerprints and provider grants in that fixture
are declared synthetic inputs; it does not play a native world.

Fixture failures remain explicit. The pre-migration C++ object reproduces the
same predecessor-retirement, post-disconnect collector and SDK stdout issues.
The collectors now preserve genuine live observations, accept only the actual
fixed-survivor decision's excluded predecessor, and retain SDK diagnostics.
Continuity readers allow 15 seconds for the real ten-second start. No clock,
measurement, Ready or result is supplied by these repairs. A first baseline
attempt lacked its security-library environment and is retained only as an
invalid diagnostic.

The qualifying cohorts preserve 151 identical matchmaking inputs. The fresh
continuity cohort also contains unrelated concurrent audio/generator edits and
a new root CMake file; the receipt lists every difference and does not claim
whole-cohort identity. All unrelated work is preserved.

C++ peer polling/budgets, capability/resource orchestration, host-candidate
coordination and mesh-quality policy remain explicit Rust migration work.
Actual native world and device ownership remains with its genuine owners.
All 12 gaps, 53 algorithm IDs and G01-G12 requirements remain unchanged and open;
original service/history, genuine providers, natural gameplay, installed input
and delivery acceptance remain outstanding. No selected runtime or production
service is replaced.

## Rust peer-mesh measurements and admitted migration-candidate caller

The [peer-mesh/host receipt](evidence/matchmaking-rust-parity-20261028/peer-mesh-host-rust-validation.json)
moves the complete H2QO mesh owner into `peer_mesh_quality.rs`. Rust owns the
wire codec, bounded probes/replies/reports, receipt accounting, duplicate
suppression, RTT/loss/application-upload measurements, row freshness and the
reciprocal full/fixed-survivor ordering. The native adapter retains its mutex
and typed field conversion. It has no packet parser, measurement state or
ranking fallback.

The migration-candidate caller now also lives in the existing Rust session
owner. It requires the same current agreement/quality generation and the fixed
survivor electorate, and uses only actual Ready and selection observations.
A sole survivor still needs its existing Ready value; no inter-PC measurement,
membership reduction, readiness or native world is invented.

A preserved, hashed, test-only C++ implementation receives the identical public
API corpus alongside the compiled Rust owner. All 104,760 commands, 9,018 packet
outputs and 582 combined context/matrix/ranking snapshots match. The corpus
includes loss, delayed duplicate receipts, disappeared peers, stale and malformed
reports, context/sender changes, clock rewind and bounds. Both complete native
runs produce the same observation digest. This reference is the prior modern
H2QO code, not an original Xbox/XNet oracle.

All 449 protocol/bridge/service Rust tests pass. Fourteen native checks pass
twice: exact C++/Rust replay, concurrent snapshots, real four-endpoint
ICE/DTLS/UDP measurements beside media, eight-seat setup, host/selection/audio
handoffs and real relay continuity. The actual Rust service/native room fixture
also passes loading, the unshortened ten-second start, 1400-byte relay and
PCR/result commit. Its provider grants, packets, fingerprints and reports are
declared fixture inputs; no native gameplay or rendered world is claimed.

The compiled mesh object references the exact eight Rust entries and contains
no C++ parser/ranking owner. The Session object references the Rust candidate
caller. An initial audit expected nine symbols; inspecting the actual schema
corrected that test expectation. Both native suites passed without a production
failure.

At that milestone, native peer-worker polling/budgets, agreement-history rotation
and capability/resource/publication orchestration remained migration work.
The next section records the subsequent driver and capability-barrier migration.
All 12 gaps, 53 algorithm IDs and G01-G12 requirements remain open and unchanged.
There are no new original executable calls, recovered historical/service rules,
genuine provider claims, installed-runtime changes or production activation.

## Native peer worker and capability barriers in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-peer-driver-rust-validation.json).
`matchmaking_native_peers::Worker` now owns the direct worker: physical-peer
scans, receive limits and rejection handling, media/control ordering, epoch
framing, per-owner packet budgets, setup measurement cadence, the wrapping
retained-generation cursor, failure handling and the two-millisecond poll wait.
The typed C++ callbacks observe the actual transport and retain actual resource
lifetimes. Media still rechecks Rust's predicate under the native generation
lock immediately before IO. Each native control poll rechecks its Rust plan
under that lock; successive polls may observe an intervening genuine publication.

The native Session's Rust plan now owns common-cut, survivor-selection and
snapshot capability barriers and their phase gates. The new atomic authority
feature snapshot binds the actual Hello observations to their actual context.
Every admitted physical peer remains in the barrier through transport loss;
no local-ready mask removes an unobserved capability or certifies native Ready.

All 457 Rust tests pass (380 protocol, 57 bridge, 20 service), including eight
new driver/barrier controls. Seven existing build/staging controls pass. The
same 13 native checks pass against the exact preserved C++ loop and then the
Rust loop: real direct links, eight-seat setup, host/selection/audio handoffs,
authority capability reset and native/legacy relay continuity. This is an
existing replacement-protocol comparison, not an original Live/XNet oracle.
The final native clients also pass the actual Rust HTTP/SQLite room, the full
ten-second start, 1400-byte relay and unanimous PCR/XP commit. Credentials,
fingerprints, packets and reports are declared fixture inputs, with no played
world, genuine provider or installed-runtime claim.

All owned source/build/SDK/Cargo/probe storage was removed. Only bounded
reports, hashes, commands and the small pre-migration source reference remain.
At that milestone, source-player admission and native readiness/publication
callers remained in C++. The following section records the admission exchange;
host-candidate/capability view gates and publication callers remain open.
Actual world, objective, role, clock, identity and device operations stay with
their genuine owners. Historical/service recovery, all required natural online
outcomes and the complete delivered G01-G12 acceptance remain open. All twelve
gaps and all 53 algorithm IDs are retained; no whole-system parity is certified.

## Immutable source-player admission exchange in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/source-admissions-rust-validation.json).
`peer_source_admissions::Exchange` now owns the complete admission exchange:
physical/couch/guest roster checks and legacy owner sentinel, immutable native
handles and twelve-byte identities, account/authority-context binding, the
bounded Offer/Ack wire and SHA-256 transcript, acknowledgement state, pacing,
wrapping scan cursor, context replacement and certified host-loss resumption.
Native C++ retains synchronization and typed value/owner conversions. Delivery
metadata never supplies a native allocation, readiness or a prepared world.

All 465 Rust tests pass (387 protocol, 58 bridge, 20 service), including eight
new admission controls. The exact preserved modern C++ owner and actual Rust
native adapter agree on 25,054 public-API commands, 591 packet outputs, 3,943
receive calls and 5,989 state reads. The replay covers up to sixteen signed
seats, couch/guest groups, legacy owner operands, drop/duplicate/retry/rewind,
malformed and rehashed bodies, full-width invalid inputs, context reset and
retained identities through two successor generations. The C++ source/header
are required test-only references, not an original Xbox/XNet oracle. Native
error responses are compared as rejection outcomes; diagnostic text is not
part of that equality claim.

The full fifteen-check native cohort passes: concurrent admission exchange,
exact replay, real direct links, eight-seat setup, host/selection/audio handoffs,
authority exchange and native/legacy relay continuity. The initial C++ adapter
failed to link because a slice-only seat type used an uninstantiated CXX vector;
the typed input container was corrected to `std::vector` and the same owned
build was reused. Rust policy did not change. Its failed-source hash and exact
inverse correction are retained with the successful qualification. The compiled
adapter reaches all ten Rust entries and has no C++ parser or hash dependency.

Owned build/source/SDK/Cargo/replay storage was removed. Only bounded reports,
hashes, commands and small reference sources remain; no large artifact was
retained. Nine unrelated current-tree changes were preserved outside the frozen
cohort. Native readiness/publication callers and remaining candidate/capability
view gates still require migration. Actual world/objective/role/clock/identity
and device operations remain with their genuine owners. Original service and
historical recovery, genuine providers, all required natural online outcomes
and the complete delivered G01-G12 gates remain open. All twelve gap records
and all 53 algorithm IDs are preserved; the installed runtime is unchanged.

## Ready, proposals and successor commit caller in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-publication-rust-validation.json).
`matchmaking_native_publication` now owns the complete legacy Ready caller,
explicit/measured proposals, selection Abort/progress/Ready import and the
successor commit caller. Its typed IO observes the authoritative Rust Session
and actual native owners while the outer Session generation lock remains held.
No mutable Rust Session borrow spans a native callback.

The driver checks the independently learned exact decision, unchanged authority
and admission, every fixed survivor's exact retained-cut metadata, Common Abort,
actual audio retirement and snapshot retirement. It derives the successor owner
plan in Rust, constructs all fallible resources and row copies through native
IO before the optional native commit, and publishes only after that commit
succeeds. C++ retains the actual world/device operations, pending resource
lifetimes and owner swaps. No metadata creates a lease, Ready value or world.
Full-width host/proposer operands cross before narrowing and cannot alias an
admitted peer. The nullable commit overload retains its prior caller obligation
that the actual native decision was already applied.

All 474 Rust tests pass (395 protocol, 59 bridge, 20 service), including nine new
ordering/refusal/ABI controls. Seven build/staging checks pass. The same fifteen
native checks pass with the exact preserved C++ caller and with the Rust caller:
forged/full-width certificates, missing survivor preparation, selected Abort,
busy audio, refused commit, successful publication, stale-repeat refusal,
source-player exchange, real direct links, eight-seat setup and relay continuity.
The unchanged source-admission replay still matches 25,054 commands and digest
`47f47dfed33177d1b68ab24d81c09fe6cce69efeb935bbe48ab5f1bd81ad9dad`.
These are existing replacement-protocol controls and declared native readiness,
commit, identity and media fixtures; they certify no rendered/natural game,
genuine provider, original Live protocol or installed runtime.

The compiled Session references all eight new Rust entries and no longer calls
the old C++ caller's standalone commit/prepared leaves. Both frozen native runs
use the same Rust policy sources; only the declared caller adapter changes.
Three unrelated current-tree changes were preserved. All owned source/build,
SDK, Cargo, syntax and replay storage was removed; bounded reports and hashes
remain with the small prior C++ source reference. No large artifact is retained.

Preparation request/completion and inventory publication orchestration, remaining
Common/source/authority/world publication callers, candidate/capability views
and postgame pump phase decisions remain C++ migration work. Actual native
owner and lease operations remain genuine. Historical/service recovery, all
required natural outcomes/providers/topologies and full delivered G01-G12 proof
remain open. All twelve gaps and 53 algorithm IDs are preserved, with no
whole-system parity or selected-runtime activation claim.

## Preparation request and inventory callers in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-preparation-caller-rust-validation.json).
`matchmaking_native_preparation` now owns the complete preparation request,
completion, external inventory freeze, invalidation and retirement-view callers.
Rust also derives support/admission from actual peer, authority, source-player
and selector observations. C++ holds the real Session generation mutex and
supplies typed operations on genuine request identities, retained inventories,
selectors and resource owners. Each observation uses one actual native clock
sample; retirement reservation consumes the same snapshot as its request.

The Rust caller preserves the prior operation order and distinct public outcomes:
foreign-owner refusal, exact tickets and lease IDs, inventory-bearing retirement
refusal, retained-without-inventory handling, publication failure, selection Abort,
retry/terminal fences and invalidation. Full-width request self/host indices are
checked before narrowing. The private native request still carries its actual
weak owner identity; metadata cannot mint leases, Ready or a prepared world.

All 484 Rust tests pass (404 protocol, 60 bridge, 20 service), including ten new
caller/authorization/ABI controls. Seven staging checks pass. The exact preserved
C++ caller and Rust caller each pass the same fifteen native checks, including
the added public-API foreign/inventory retirement refusal assertions. The source
admission replay remains exact for 25,054 commands and digest
`47f47dfed33177d1b68ab24d81c09fe6cce69efeb935bbe48ab5f1bd81ad9dad`.
The compiled Session reaches all six new Rust entries, with no C++ parser/hash
dependencies; all 3,396 Rust cohort inputs match the frozen native sources.

An initial Rust check exposed an ambiguous helper glob import; explicit imports
fixed it. A native test initially accessed a private convenience method; the
assertion now uses the existing public request/outcome/ticket API. Both failed
source hashes and exact inverse corrections are retained. A separate freeze
attempt detected unrelated workshop source drift before any build or CTest;
the fresh successful cohort has 18,414 files and no drift during freezing.
The same owned build was reused for the native compile correction and both
successful caller runs. Twenty-three subsequent unrelated edits were preserved.

All owned source/build/SDK/Cargo/probe storage is removed. Only bounded,
authenticated, deduplicated reports, commands and small source references remain;
no large artifact is retained. These replacement-protocol and declared native
metadata/lease controls certify no genuine world, provider, original Live wire,
physical installed input or delivered runtime. Remaining Common/source/authority
and world publication callers, candidate/capability views and postgame phase
decisions still require migration. Original recovery and every full G01-G12
gate remain open; all twelve gaps and 53 algorithm IDs are unchanged.

## Source, authority, Common and world callers in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-exchange-callers-rust-validation.json).
`matchmaking_native_exchange` now owns the complete source-player
publication/accepted-view callers, full and pipelined authority publication,
capability views, all eight Common-cut actions, Common views and host-candidate
views. Source/authority/quality owners are captured under the genuine Session
mutex and used outside it as before. Common operations keep that mutex through
the actual action and selection Abort/progress, even when the leaf refuses.
Rust derives phase, role and configuration facts from the authoritative Session;
native IO retains actual returned values, resource owners and byte buffers.

Rust checks every source-player row and all twelve identity bytes against
retained continuity. Raw 32-bit capability peers and Common context hosts cross
without narrowing. Feature masks preserve the complete replay/presentation,
pipeline, host-continuation and terminal-camera contracts. The world-command
caller now also lives in the Rust Session: it admits the phase/role and chooses
the current epoch or exact selected world seal itself. A byte receipt never
creates native validation, Ready, a lease or a prepared world.

All 495 Rust tests pass (414 protocol, 61 bridge, 20 service), with eleven new
caller/Session/ABI controls, and seven staging checks pass. Syntax checks use
the actual generated CXX header and frozen native inputs. The exact preserved
C++ caller and the Rust caller each pass the same fifteen native checks.
Added public Session controls use the real admitted DTLS links for full and
pipelined authority byte delivery, explicit declared validation, rejected-cut
refusal, source receipts, complete capability masks and full-width peer/context
guards. Existing checks retain fixed survivor readiness, Common Abort,
preparation retirement, actual synthetic audio-device barriers, measured
election, direct UDP, source identity continuity and legacy/native continuation.
The current protocol-124 C++/Rust source-admission replay matches 25,054
commands and digest
`630e4735387c2212918508ee0f9cda637a2521d3a8eb5f960abc6b10f0cb06d6`.
The cohort preserves a concurrent native protocol update from 123 to 124 that
predates this freeze. An owned two-version probe uses the unchanged preserved
C++ admission source and replay harness to reproduce both the prior protocol-123
digest `47f47dfed33177d1b68ab24d81c09fe6cce69efeb935bbe48ab5f1bd81ad9dad`
and this protocol-124 digest. The actual C++/Rust match is provided separately
by both native caller runs; the version probe supplies no additional Rust or
original-wire claim.

Both native runs use the same 3,399 Rust inputs in a frozen cohort of 18,427
files; only the declared C++ caller changes. The compiled prior caller has none
of the eight new exchange references, while the final caller reaches all eight.
Neither compiled Session depends on a C++ parser/hash. An initial bridge test
build required `Debug` for an enum comparison; comparing its exact underlying
representation corrected the test without changing production policy or ABI.
The failure, failed-source hash and exact inverse correction are retained.

All owned source/build/SDK/Cargo/probe storage is removed. Bounded reports,
commands, hashes and small source references remain; no large artifact is
retained. Eight unrelated current-tree changes were preserved, separately from
the two post-qualification README edits. These are replacement-protocol and
declared identity/byte/validation/audio/commit fixtures, not original Live,
genuine providers, a played native world, physical installed input or delivery.

The complete C++ reliable authority and snapshot transfer state machines still
need Rust owners. Session receive dispatch/phase/epoch/history rules,
feature-limit construction, remaining resource/local-seat views and postgame
pump decisions also remain. Actual native lease/world/device ownership stays
genuine. Original historical/service recovery, every required natural outcome
and all full G01-G12 gates remain open; all twelve gaps and 53 algorithm IDs are
preserved, with no selected runtime or production-service activation.

## Reliable authority exchange owner in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/peer-authority-transfer-rust-validation.json).
`peer_authority_transfer::Transfer` now owns the complete existing reliable
authority exchange: validation and replacement, Hello/capability negotiation,
wire/hash, send/receive windows, token credit and persistent poll cursor,
retries/deadlines, pipeline retirement, byte delivery, rejection and explicit
executor-validation receipts. One Rust mutex owns the mutable state. Large
publication and completion hashes stay outside it; completion rechecks the exact
context and seal before publication. Progress renews only an actual delivered
cut's idle deadline, never its total lifetime or a failed/native-validated cut.
Bytes and checksums alone never create native Ready.

C++ keeps the public signatures and typed buffer/value conversion. Raw native
peer, host and window operands keep their full unsigned width through the ABI.
Returned immutable C++ byte banks are genuine allocated storage; exact allocation
identity, zero-copy performance and played-world resource-budget acceptance are
not claimed. The prior modern C++ owner and header are preserved unchanged as
test-only references, including the owner's existing Rust poll-cursor leaf.
This is a replacement protocol comparison, not recovered original XNet/Live.

All 506 Rust tests pass (424 protocol, 62 bridge, 20 service), including eleven
new raw-input/state/deadline/pipeline/staged-completion controls, and seven staging
checks pass. Before and after owners pass the sixteen native test names covering
real-thread authority pumping, encrypted direct links, exact source continuity,
fixed-survivor and Common barriers, host loss/election, actual synthetic audio
retirement and native/legacy continuation. The final replay compares 37,731
commands, 8,565 packet outputs, 14,520 receives and 3,332 state reads with digest
`4df76820f8203e922a6f5c832fce1cdc83256cfb6d3581cb7b149a9fa1636926`.
It includes all seven packet types, every representative byte mutation and
truncation, full 1,024-byte payloads seeded as the final checksum chunk, malformed
traffic, one/two/four/sixteen physical memberships, raw limits, stale generations,
loss/duplicates, pipeline and native-validation refusal/recovery. Exceptions
compare invalid-argument/other category; message text is outside replay claims.

Qualification retained the initial short-tail replay and its expansion. A
category-aware replay then exposed a real ABI mismatch: the old API throws
`std::invalid_argument`, while an untranslated CXX failure throws `rust::Error`.
Minimal empty-target and bad-constructor controls reproduce that difference.
Typed IO translation on the four validating public calls fixes it without
changing Rust policy. The minimal controls and complete native cohort are green
afterward; the red run, exact inverse adapter correction and expanded corpus
remain recorded. Fifty native test executions include that one deliberate red
control; the final sixteen checks pass, with no native build failure.

The compiled final adapter references all eighteen authority Rust entries and
has no C++ parser/hash or standalone poll-cursor dependency. One over-inclusive
borrowed workshop fixture changed during concurrent work. Its sole registered
consumer is an unselected workshop test; the actual compiled Rust dependency
files do not reference it. All production policy and critical source inputs
match, and the fixture is preserved. This is explicitly not an assertion that
every file in the broad input inventory stayed identical. Seventeen other
current-tree edits were preserved outside the frozen cohort.

Owned source/build/SDK/Cargo/probe storage is removed. Only bounded reports,
commands, hashes and small test-only source references remain; no large artifact
is retained. Genuine native worlds, providers, physical installed input and
delivered runtime/service acceptance remain open. The C++ snapshot transfer
owner, receive dispatch/phase/epoch/history rules, feature-limit construction,
remaining resource/local-seat views and postgame decisions still require Rust
ownership. Original service/history and all full G01-G12 gates remain open, with
all twelve gaps and 53 algorithm IDs preserved and no selected-runtime change.

## Snapshot delivery owner in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/peer-snapshot-candidate-rust-validation.json).
`peer_snapshot_candidate::Candidate` now owns the complete existing modern
snapshot delivery protocol: all eleven packet codecs, exact context/token and
physical membership, SHA, atomic full-recipient publication, foreground Arm,
capture/release/abort/retirement, wrapping cursors, retry windows, clock/stall
telemetry and bounded bank accounting. One Rust mutex owns protocol state.
Large publication and completion hashes stay outside it, with exact rechecks.
Publication reservations last through actual bank destruction, including Abort
during hashing; timeouts never invent retirement or shrink membership.

C++ retains typed IO, actual uniquely owned vector banks and private move-only
Delivery identity. Rust moves the actual native banks through CXX. Storage-only
lease callbacks read the real identity or issue/destroy the actual bank under
the Rust state mutex, with no owner pointer or executor reentry. False capture
or retirement retains the caller's bytes. Retirement preflight is observational;
the actual operation rechecks its witness and destroys the bank before clearing
outstanding storage. The public API and invalid-argument category are preserved.
A narrowly scoped Rust `Send` implementation documents the native bank's unique
ownership, pinned writes and lack of thread affinity. These controls make no
allocation/timing-performance or complete native-world memory-budget claim.

All 515 Rust tests pass (432 protocol, 63 bridge, 20 service), including nine new
constructor/foreground/lease/hash/publication/raw-ABI controls. Seven build
staging checks and the actual generated-header C++ syntax check pass. The prior
C++ and new Rust owners each pass eighteen native checks covering real-thread
snapshot pumping, private foreign-owner leases, explicit foreground retirement,
encrypted direct peers, authority/admission, host loss, fixed survivors, actual
synthetic audio retirement and native/legacy continuation. The final replay
compares 57,325 commands, 9,299 packets, 18,281 receives and 4,110 state reads,
with digest `da3493708394453f8851d6bf4e81f6ac77a8664a097e0ae3700832e29eb58d85`.
It covers all eleven types, every representative byte mutation and truncation,
a full final 1,024-byte chunk seeded after prior bytes to reach SHA validation,
two/four/sixteen physical peers, full-width invalid operands, loss/duplicates,
native lease moves and refusal, early/partial/held-bank Abort, stall and nonce
succession. This compares the preserved modern replacement, not original Live.

The compiled final adapter references all fifteen snapshot Rust entries and no
C++ parser/hash code. The final Rust and native input inventories match. Two
source freezes were rejected before building because unrelated work changed;
a coordinator syntax error was also corrected before launch. The first Rust
check caught the bank's missing scoped `Send` lint exception. Diagnostics remain
bounded and retained. The accepted cohort pins 18,869 files, and ten unrelated
current-tree changes were preserved outside it. Owned source/build/SDK/Cargo/
probe storage is removed. Only bounded reports remain; the selected runtime is
unchanged.

Session receive dispatch/phase/epoch/history, feature-limit construction,
remaining resource/local-seat views and postgame decisions still require Rust
ownership. Genuine native worlds/providers, physical installed Linux input,
delivered runtime/service acceptance, original historical/service recovery and
every full G01-G12 gate remain open. All twelve gaps and 53 algorithm IDs remain
unchanged, and the full Rust matchmaking goal remains active.

## Session receive dispatcher in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-receive-dispatch-rust-validation.json).
`matchmaking_native_receive` now owns the complete Session receive caller:
ordered recognition of survivor, snapshot, Common, agreement, quality, admission
and authority frames; phase/capability/epoch gates; current versus first exact
historical agreement selection; and Abort/progress/fence/host-observation/refresh
order. Recognized frames never fall through to gameplay when refused. Mandatory
snapshot/survivor/agreement epochs remain distinct from legacy epoch-optional
Common/quality/admission/authority handling. Common frames still perform their
original Abort/progress work after a refused receive predicate.

Typed native IO retains the actual Session generation mutex, resource handles
and history. Survivor/Common/agreement work borrows the real Session-owned
resources under that lock. Snapshot/quality/admission/authority work pins the
actual shared owner and releases the lock before context/receive work, as before.
The dispatcher retains no mutable Rust Session borrow across callbacks. Rust
attempts unlock on ordinary IO errors, while native RAII also covers unexpected
unwinding. Raw source/epoch operands retain their full 32 bits; history indices
use a checked native-size conversion. C++ samples time and performs selected IO,
with no second receive-policy implementation.

All 521 Rust tests pass (438 protocol, 63 bridge, 20 service), including six new
recognition/barrier/pinning/history/order/error controls, and seven staging checks
pass. The final nineteen native checks pass, including real encrypted direct
peers, host loss/election, fixed survivors, actual synthetic audio retirement,
authority/admission/snapshot transfer and native/legacy continuation. The
preserved C++ caller is retained as an exact source extraction. A comparison
against production Rust matches 110,810 commands and 364,696 traced events,
including 61 deliberately injected native IO refusals, with digest
`9e15dedbf25621c24fb46a7677d97dec21c81066b6a5a9465c229297361fe7f2`.
The corpus covers every five-flag phase bank, capability and native-owner
presence bank, exact/foreign/zero/full-width epochs, raw peer operands,
duplicate historical epochs and first-match behavior, malformed recognition,
lock lifetime, payload/operand observations and effects after error. These
resource/history/error observations are declared fixtures, not genuine worlds,
provider authentication, original Live or installed Linux acceptance.

The initial baseline passed the eighteen existing native checks; the new harness
failed on its first stdout line. A one-command probe showed a successful process
and matching comparison preceded by the linked library's fixed shutdown-handler
startup banner. The reader now consumes only the two known lifecycle banner
lines, as the other protocol replays already do; unexpected output still fails.
The focused comparison and final nineteen-check cohort pass afterward. All 39
native test executions, including that one harness failure, remain recorded.

The actual final Session object references the Rust dispatcher and none of the
seven former C++ classifier calls. All 3,414 inputs match between the qualified
Rust and native snapshots, with 18,887 native files frozen and twelve unrelated
current-tree changes preserved at qualification. After cleanup, the separate
weapon-effect state fixture changed. Its registered consumers are unselected
weapon tests; it has no Rust source/build-script reference. That newer fixture
is preserved and recorded separately. This does not assert that the current
whole fixture inventory is unchanged. Owned source/build/SDK/Cargo/probe storage
is removed; only bounded reports remain. This milestone leaves selected runtimes
and services unchanged.

Feature-limit construction, remaining resource/local-seat views and postgame
decisions still require Rust ownership. Original historical/service recovery,
every required natural outcome/topology, genuine native worlds/providers,
physical installed Linux playlist/Start flow and complete delivered G01-G12
acceptance remain open. All twelve gaps and 53 algorithm IDs remain unchanged,
and the full Rust matchmaking goal remains active.

## Session views, transfer limits and postgame caller in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-session-views-postgame-rust-validation.json).
`matchmaking_native_access` now owns initial and successor transfer limits,
full-width legacy connection-owner normalization, local/physical seat lists,
presence/continuity/snapshot view gates and complete outer postgame voice policy.
Initial limits use the admitted Rust configuration. Successor limits retain the
prior caller's explicit terminal-false operand. C++ copies raw seat owners into
the ABI; Rust resolves only the original `UINT32_MAX` owner sentinel, without
allowing wide operands to alias valid seats.

Rust now chooses the postgame device order: pin the actual audio owner, sample
phase, disable on failure or skip inactive phases, apply focus/radio/blocks,
capture preferences/outgoing frames, take the real generation mutex, fence,
exchange, unlock and deliver. The existing Rust exchange still rechecks the
current phase after the fence. Actual device/time/readiness observations and
frame buffers remain native IO, with no mutable Rust Session borrow across
callbacks. Fence refusal retains caller frames until after unlock. Control-worker
postgame selection also comes from Rust.

The original local-seat getter remains callable without acquiring the Session
mutex. Rust computes its immutable value before worker startup; C++ retains that
value as an IO projection. A real generation-locked native commit callback now
checks the same four local seats during both host handoffs. This guards against
re-entering the mutex or borrowing the mutable Rust Session from that callback.
Native resource ownership, device retirement and normal inner voice validation
remain intact.

All 529 Rust tests pass (446 protocol, 63 bridge, 20 service), including eight new
owner/limits/view/device/error controls. Seven staging checks and all twenty
final native checks pass. The preserved C++ and production Rust comparison
matches 31,360 cases: 31,185 postgame cases, eight snapshot views, five initial
limit profiles, 106 seat configurations, 54 presence/continuity views and two
control-pump cases. It compares complete frame fields/bytes, phase changes during
the fence, raw phases/generations, device preferences, caller lock order and 31
declared IO refusals, with digest
`993b69af892595d026c3f5f99558be37f58c1fb821ac760fe027b5a481195cba`.
These are declared observation/device fixtures; they do not certify original
XHV, original Live, played worlds, genuine providers or installed Linux delivery.

The first comparison fixture omitted the preexisting required continuity object
from its phase message. The nineteen other native checks passed. A minimal
probe of the actual Rust validator showed rejection without phase advancement,
then acceptance of the complete object. Only the fixture message was corrected;
production validation was unchanged. The corrected comparison and the native
commit/local-seat check pass with the old C++ caller, and the final full twenty
checks pass with the Rust caller. All 42 native executions, including that one
fixture failure, are retained. A temporary probe linker-output correction and
the subsequent correct native relink are recorded as well.

The configured Session object references all eight new Rust entries. The C++
feature function and seat-owner predicates are gone from that caller, and the
local getter still contains no mutex acquisition. All 3,418 inputs match between
the qualified Rust and native snapshots. The cohort froze 18,914 files and
preserved eighteen unrelated current-tree changes. Owned source/build/SDK/Cargo/
probe storage is removed; no large artifact is retained, and this milestone
leaves selected runtimes and services unchanged.

Further native fence/selection/candidate orchestration and API boundaries still
need a complete ownership audit. All historical/service, natural outcome,
genuine world/provider and delivered-client gates remain open, including actual
installed Linux playlist/Start validation. All twelve gaps, 53 algorithm IDs and
G01-G12 criteria remain unchanged, and the full goal remains active.

## Session migration fence, selection and candidate callers in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-migration-callers-rust-validation.json).
`matchmaking_native_migration` now owns the complete Session callers for fencing,
starting survivor selection and choosing a migration candidate. Rust controls
the context/owner/phase short circuits, actual observation order, force/learned
fence combination, selection-state sampling and candidate result bounds. A
forced fence still reads the genuine agreement at the sampled time. Candidate
queries retain the exact agreement, QoS ranking and selection observation order.

The native adapter observes actual resources under the caller-held generation
mutex and applies existing Rust Session operations. No Rust Session borrow spans
callbacks that can reenter that owner. New raw context hosts stay wide until
checked at the admitted boundary. The public Session header is unchanged, and
the local-seat getter retains its mutex-free immutable projection. The actual
compiled Session object references all three new Rust caller entries. No C++
caller fallback was added.

All 538 Rust tests pass (453 protocol, 65 bridge, 20 service), including nine new
caller and ABI controls. Seven staging checks and all 21 final native checks
pass. The exact preserved modern C++ callers and new Rust orchestration match
18,110 commands and 18,430 steps, including 3,142 expected State/IO refusals.
The comparison covers complete observations, ordering, effects, repeated
transitions, results and exact exception text. The corrected baseline and final
production run share digest
`a6357fdab53a98867f1d741e01ecbaf76fca7b5cc91d100b8bffb1a94dbc31e4`.

Adding exact refusal text exposed a regression in the new bridge: it replaced
the existing ONLINE refusal detail with a generic error. A focused native red
run reproduced that difference; Rust now carries the actual IO exception text.
The full Rust suites and full native baseline/production cohorts pass after the
fix. Across four native runs, 63 of 64 test executions pass; the one failure is
that retained regression control. No admission or State guard was weakened.

All Rust crate inputs match across the qualified snapshots. A separate weapons
fixture was added between freezes; a source/current-registration audit found no
Rust or selected native-test consumer. The broad inventories therefore differ,
and no compiled dependency-file identity claim is made for that fixture. Its
concurrent source and reference work remains intact. The cohort froze 18,929
owned source files and preserved five unrelated existing-input changes. Owned
source/build/SDK/Cargo/audit scratch is removed, with no large artifact retained.

Remaining Session public API sequencing and complete production ownership still
need auditing. Historical/service recovery, every required natural outcome and
topology, genuine worlds/providers, actual installed Linux playlist/Start flow
and complete delivered-artifact/restart/config/rollback acceptance remain open.
All twelve gaps, 53 algorithm IDs and G01-G12 criteria are unchanged. The full
Rust matchmaking goal remains active; this component does not establish original
Live, original whole-system parity or installed delivery.

## Session packet, audio and close API callers in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-session-api-rust-validation.json).
`matchmaking_native_api` now owns the complete public send/receive, voice,
preferences, voice-packet collection, retained-audio and postgame-close callers,
plus standalone audio sync/fence sequencing. Rust drives generation locking and
fencing, readiness/resume order, first-packet selection, output mutation order,
real audio-owner acceptance and presence/device/worker shutdown order. Native IO
retains the actual mutex, device, worker, exception and byte-bank owners.

The received packet container moves through the ABI without cloning its banks.
An empty receive preserves both outputs. A nonempty receive writes peer before
copying bytes, preserving the existing partial-output behavior on refusal. All
returned banks, including unselected banks and failed-copy paths, retire before
unlock. Rust core tests observe those lifetimes, and native comparisons require
the original bank address. Partial native voice conversion stays local to its
callback, with successful output retained for the caller. Close releases its
audio pin before stopping or joining the actual worker, outside the generation
mutex. Standalone audio helpers retain their caller-held lock contract. No Rust
Session borrow spans callbacks.

The native audio generation guard can throw `std::overflow_error`. Native IO
retains the genuine exception object with `std::exception_ptr` and rethrows it
after the Rust caller's cleanup, preserving type, text and identity. The fixture
negative control deliberately omits that rethrow: CXX preserves the message but
returns a different exception type/object, which the comparison detects. The
discard switch exists only in test IO. Public Session headers and the existing
mutex-free immutable local-seat getter remain intact.

All 546 Rust tests pass (461 protocol, 65 bridge, 20 service), including eight new
packet/bank/audio/owner/worker/refusal controls. Seven staging checks and all 22
final native checks pass. Exact preserved modern C++ and Rust API callers match
6,902 commands and 8,462 steps, including 464 expected State/IO refusals and 243
native exception-identity checks. Full packet and signed-voice bytes, frame
fields, real Rust Session effects, lock/owner order, repeated calls and raw
targets/generations are compared. The baseline and final production run share
digest `3091e8d9756af0c199448e6d0f05ce2542b9c7a27b59c2981864517f193db7ed`.
Devices, workers and migration observations in this comparison are declared
fixtures; they do not establish a played world, original XHV or genuine providers.

The initial fixture build required explicit pointer/size slices in three CXX
calls. Only those fixture arguments were corrected. Both subsequent full native
cohorts pass all 44 test executions. The actual configured Session object calls
all nine new Rust entries and both native exception-transport functions. All
3,430 qualified Rust/native inventory inputs match. The cohort froze 18,949 owned
source files and preserved 21 unrelated existing-input changes. Owned source,
build, SDK, Cargo and audit scratch is removed; no large artifact is retained.

Remaining production ownership/API route closure, lawful historical/service
recovery, every required natural outcome/topology, genuine worlds/providers and
complete delivered-artifact/restart/config-change/rollback acceptance remain
open. Actual installed Linux playlist/Start validation remains open. All twelve
gaps, 53 algorithm IDs and G01-G12 criteria are unchanged, and the full Rust goal
remains active. This component does not certify original whole-system parity.

## Rust production route candidate, 2026-11-01

The [delivery receipt](evidence/matchmaking-rust-parity-20261028/rust-production-route-validation.json)
qualifies an opt-in host route through actual Caddy TLS/WSS into the Rust HTTP
service and canonical SQLite database. It adds a Rust service Dockerfile and
production Compose overlay, without activating either or changing the selected
Linux executable. The legacy process has an explicit discovery-only mode: it
constructs no account, queue, party, clan, friend, room or voice owners and runs
none of their maintenance. Only signed listing/enrollment/federation routes
reach it. All other public paths, including unsupported capabilities, belong to
Rust. Stopping Rust produces a proxy error while discovery remains live; no
request falls through to Python matchmaking.

Rust retains its loopback-only TLS-proxy guard. Compose shares Caddy's network
namespace rather than weakening that guard or enabling development HTTP. Both
processes keep the same state volume and database. `/v1/health` aliases the
existing Rust component health; full `ready:false` remains. The candidate pins
its image indexes and uses the checksum-pinned official Caddy 2.11.7 executable
for the host checks. The [operations document](rust-matchmaking-delivery.md)
describes configuration, private SDK inputs, state backup and rollback limits.

The controls reproduce the old proxy's HTTP 504 at 10.01 seconds and HTTP 413 for
the full voice-preference body. With the candidate transport, the real Rust
thousand-friend request completes in 12.17 seconds and all 2,000 preferences
persist. Rust still rejects ordinary bodies over 16 KiB and preference bodies
over 48 KiB. All 14 playlist rows, the v12/v16 eligibility differential, ticket
search/cancel, accepted-party consent and a ticking shared countdown traverse
the verified local TLS connection. Actual WSS search heartbeat, voice idle
Ping/Pong and subsequent peer media delivery pass. Service restart and a catalog
revision change retain canonical identities/ranks/signing keys and reject old
grants/revisions; restoring the original catalog preserves those identities.
Compose validates the namespace, state mounts and restart dependency.

All 20 Rust service tests and 18 directory/HTTP/registry tests pass in both build
cohorts: 76 successful test executions. The final 354 frozen inputs match the
current source. An initial checksum-format error, a wrong expected proxy status
and a test preference token of the wrong kind were corrected in the driver;
their failed receipts and exact fixture refreshes remain. No production
authorization check or Rust algorithm was relaxed. Owned source/Cargo/state/
certificate/Caddy/fixture outputs are removed; only bounded reports remain.

This is host candidate acceptance. The Docker daemon is unavailable, so no OCI
image was built or executed. Existing legacy production state-clone migration,
real deployment/recreation and service-image rollback remain unqualified.
Provider replies, grants, advertised maps and the local CA are declared test
inputs. Genuine Steam/Epic authorization, original Live, played worlds, physical
installed Linux playlist/Start input and complete runtime/service delivery
remain open. The selected executable still lacks the latest Session API Rust
source cohort. All twelve gaps, 53 algorithm IDs and G01-G12 criteria are
unchanged; the full goal stays active and uncertified.

## Populated legacy state handoff, 2026-11-01

The [state handoff receipt](evidence/matchmaking-rust-parity-20261028/legacy-state-cutover-rust-validation.json)
extends the delivery checks with a populated fixture generated by the current
legacy HTTP/room adapter and production Rust ABI. It completes a declared
three-seat room through the unshortened 10.05-second start and unanimous
synthetic PCR submission. Two persistent players receive result/history rows;
the guest remains transient. The resulting hidden XP total is 100. Additional
normal account/profile/clan/party/friend/preference routes populate the social
and customization namespaces. Private linked aliases and provider grants are
explicit fixture inputs.

SQLite backup preserves all 67 tables, 33 nonempty tables and 75 rows plus both
signing keys. Rust opens the private clone without changing any existing table
contents, excluding clock-floor maintenance. After fresh login, complete profile,
clan, party, friend, public-history and JWKS views match the legacy baseline.
Old runtime grants fail. The old party command replays without adding a party,
and Rust commits a new profile revision. The legacy host process then reopens
that Rust-written clone without changing its existing rows and reads the same
views and new profile revision. The protected legacy source database and keys
remain unchanged.

All 20 Rust service tests and 18 directory/HTTP/registry tests pass; five full
handoff controls pass. All 356 cohort inputs are pinned. A reused fault-injection
fixture initially expected JSON for a legacy plain-text 500 response; the new
normal-completion producer removes that unrelated setup mismatch without
changing production behavior or weakening any handoff assertion. The test
driver now redacts bearer headers from exception diagnostics; that report-only
change was checked separately after qualification. No source, binaries, state,
certificates or large captures remain in owned scratch.

This proves a populated-fixture host handoff against the current legacy source,
not an existing production database or older deployed image. Actual production
state-clone migration, OCI execution/recreation and service-image rollback stay
open. The tested profile is an explicitly modern legacy-v10 relay fixture;
genuine providers, original Live, all required natural outcomes/topologies,
physical installed Linux input and complete delivered acceptance stay open.
All twelve gaps, 53 algorithm IDs and G01-G12 criteria remain unchanged; the
full Rust goal stays active and uncertified.

## Accepted-party account client controller in Rust (November 1)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-account-client-rust-validation.json).
Rust now owns the shared account client's accepted party/clan/friend parsers,
private session and subject state, command UUIDs/replay, generation fencing,
privacy/version/context checks, countdown and social polling, profile/EOS retry
cadence, Steam friend fetch state, activity-link lifetimes, voice authorization
and follower/PCR-return rules. The party controller shares its worker and state
with account/social operations, so those rules move together. Native C++ keeps
the actual mutex, condition variable, thread, Curl transfer, clock, entropy,
provider handles and presentation projection of validated Rust state. Network
responses reach the Rust parser directly. Credentials and private activity
links stay outside the public snapshot. The compiled adapter calls 21 Rust
account entries; the preserved pre-migration C++ object calls none of them.

Two intentionally corrected modern C++ defects were reproduced before fixing
the port. A complete session reply arriving after sign-out/reconnect orphaned
its opaque grant because cancellation discarded the reply before cleanup. A
replacement reply with expires_in=0 cached its token before validation, allowing
the previous future expiry to authorize later reads. Rust retires known late
grants with an uncancelled request and validates the whole lease before adoption.
Native reference controls reproduce both defects. A real Curl completion
barrier observes POST only before the fix and POST/DELETE afterward. Partial
or otherwise unreadable replies do not provide an opaque token to retire.
Native entropy refusal also retains its genuine exception type, message and
object identity across Rust, with no partially queued command.

All 565 Rust tests pass (480 protocol, 65 bridge, 20 service), including 19 new
client controls; four existing environment/corpus tests remain ignored. Seven
staging tests pass. Preserved modern C++ and Rust snapshots match 7,775 cases:
374 accepted and 7,401 rejected, with no sanitized-value mismatch. This compares
acceptance/data rather than parser-library exception text. The initial replay
harness needed to separate native shutdown preamble from JSON lines; that
fixture correction changed no parser acceptance rule.

The passing native checks include the original account suite, both defect
controls, real Curl, the snapshot replay and the Rust account/playlist/party/
queue/QoS/room/provider/social integration. Actual SDL/Vulkan menus against
the Rust service verify the blocked compatibility case and the complete
playlist/Start/cancel/search/return flow. Three SDL mouse actions operate the
playlist chooser, playlist row and Start Game, across 23 rendered observations.
Four broader frontend checks fail identically with the captured C++ and Rust
account clients: clan-requirement text, matched-slot artwork and two bitmap
sampling checks. Their functional failures remain explicit; the differential
only establishes that the account-controller port did not cause them. Provider
handoff remains a declared fixture. No results or XP are published by
these menu tests, and service full readiness remains false. All 3445
shared Rust/native inputs match. One unrelated weapon fixture added after the
native snapshot is excluded from this paired inventory; it has no crate
references and its current bytes match the Rust cohort. Owned builds, SDKs,
Cargo trees and raw frames are
removed, retaining only bounded reports and the required source reference.

This qualifies the modern client boundary. Original Live/service recovery,
every required played world/outcome/topology, genuine providers and delivered
Linux input remain open. The selected runtime and production service were not
replaced. All 12 gaps, 53 algorithm IDs and G01-G12 criteria are unchanged; the
full Rust goal remains active and original whole-system parity is uncertified.

## Native result admission and publication caller in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-result-authority-rust-validation.json).
Rust now owns the production ONLINE result publisher and validator's completion,
roster, identity, team-history, full-variant, placement, score, statistic and
accuracy admission decisions and complete publication plan. C++ supplies actual
native observations and preserves the original gameplay ranking/profile
conversion and numeric PCR writer. The two previous policy bodies are removed
from production and retained exactly as modern C++ references. The compiled
native adapter calls both Rust result entries; the captured C++ object calls
neither. Original ranking/floating-point helpers, public header and presentation
tail are unchanged. Native IO errors retain their real exception object, type
and message across Rust. This is modern adapter equivalence, with no new original
Live arbitration or historical service recovery claim.

All 578 Rust tests pass (493 protocol, 65 bridge, 20 service), including 13 new
result controls; four existing corpus/performance/environment tests are ignored.
The exact captured C++ caller and Rust bridge agree on 4,748 commands: 316
publication and 4,432 validation commands, with 834 accepted and 3,914 rejected.
Complete publication fields, error text, native observation order and 82 actual
exception-identity controls match. The replay includes every accuracy word,
signed-score bounds, sparse slots, duplicate/unadmitted participants, complete
304-byte settings, team history and the existing stat-marker high-bit behavior.
Its gameplay/ranking/profile/writer observations are declared controlled IO.
Fixture-only declaration errors were fixed before the final replay; the exact
reference function bodies were not edited.

Four of five native CTests pass, including seven staging controls, the paired
replay, all-family numeric PCR carriers and the existing team-switching test.
The latter executes the real publisher and validator after its custom one-minute
timed native match, genuine UDP local/remote couch team changes, numeric PCR,
peer confirmation and retained System Link lobby return. This bounded timed
control does not certify every normal playlist or unshortened natural outcome.
The existing one-kill combat-accuracy test fails in ordinary warmup, before result
publication, with the exact same read-only contact-observer/instruction-trace
guard refusal under the captured C++ and Rust implementations. Both small
reports are identical. The physics guard is preserved and the functional
failure remains open; the five-test suite is not reported as passing.

The Rust and native cohorts share 2,980 copied inputs without a mismatch. Another
473 binary/reference inputs are borrowed unchanged. One unrelated weapon
fixture was added after native freeze and changed concurrently; it has no Rust
crate/result-test reference and is excluded from current qualification. Other
concurrent source changes remain outside this frozen cohort. The 5.35 GB owned
native source/build/SDK context, Cargo outputs and compiler audit scratch are
removed; only bounded reports and required source references remain. No large
generated artifact, selected runtime or production service is retained or
replaced. All 12 gaps, 53 algorithm IDs and G01-G12 criteria remain unchanged;
the full goal is active and whole-system original parity remains uncertified.

## Native pregame lobby controller in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-rust-validation.json).
The native pregame controller now keeps its countdown value/stage, consumed
publication serial, counted party and pending Leave state in safe Rust. Rust
owns Start eligibility and consent priority; active A/Start interception;
leader/member B and X cancel/request behavior; departure notices; accepted
countdown consumption; and acknowledged/refused Leave handling. The old C++
policy bodies and input branches are removed. C++ keeps actual account/queue
IO, menu widgets/navigation, input delivery and sound buffers. Facts, immutable
command plans and returned effects cross typed CXX. No Rust borrow spans native
IO or reentrant menu opening. The native handle keeps value semantics through
a deep Rust clone and preserves genuine native exception objects and types.

All 591 Rust tests pass (506 protocol, 65 bridge, 20 service), including 13 new
lobby controls; four existing environment/corpus tests remain ignored. The
unedited captured C++ methods and four input branches agree with Rust across
976 sequences and 4,387 operations, including full controller state, command
arguments/order, refusals, errors, cues and menu effects. Eleven injected native
IO refusals match. Another 976 actual native-wrapper controls retain exception
object identity and permit callback reentry without a retained Rust borrow;
deep copies stay independent. Both the standalone build and production SDK/CXX
build produce the same replay digest. These compare the modern adapter, not
original Live transport or complete original caller provenance.

Fresh unchanged originals reproduce 768 paired complete writer controls through
1,536 retail/TU1.5 calls, checking every executed instruction byte. Another
40 TU1.5 timer/press/writer controls execute 1,083 instruction visits. Both fresh
fixtures are byte-identical to the references consumed by Rust tests. Timer
publication and pre-evaluated press caller gates retain their explicit limits.

The captured C++ baseline passes eight focused native CTests. The Rust-wired
build passes those checks, the exact replay and staging controls; the preserved
original lobby-policy test also passes, for 11 unique final passing
tests. Actual SDL/Vulkan menus against Rust HTTP verify blocked and complete
playlist behavior, shared Start/cancel/restart/3/2/1, search and retained return.
Three mouse actions and 23 rendered observations use the qualified rebuilt
client. Reports are admitted only when their native binary hash matches that
client. Four wider frontend checks were red in the C++ baseline;
4 remain shared functional failures after the port. Their
diagnostics concern clan-requirement text, matched/empty-slot artwork and
imported bitmap sampling. They remain failures, separate from scoped controller
acceptance. Provider handoff is a private fixture and no gameplay, results, XP,
genuine providers or installed input is certified by these menu controls.

A new replay initially reused an existing test target/source name. The native
configuration exposed the collision. The existing source was restored exactly
to its captured baseline hash; the new replay uses a unique filename/target,
and the original test was built and passed. Fixture declaration/include errors
and an overbroad symbol-audit pattern were corrected separately; exact captured
reference bodies were not edited. The compiled native handle calls 17 Rust
entries; the five old policy methods are defined only in the native effect
adapter. All 2,984 shared copied inputs and 474 borrowed reference inputs match,
with no unused-reference exclusion in this cohort. Owned source/build/SDK,
Cargo, compiler, emulator and capture scratch are removed, retaining bounded
reports and required small source references. No generated large artifact is
retained or selected runtime/production service replaced.

The C++ search/consent/default-playlist/launch controller remains unchanged and
still needs migration. Full original callers, historical/service recovery,
every required unshortened outcome/topology/continuity route, genuine providers,
physical installed Linux input and delivered/restart/rollback acceptance remain
open. All 12 gaps, 53 algorithm IDs and G01-G12 criteria are unchanged. The full
goal remains active and original whole-system parity is uncertified.

## Native frontend search and launch controller in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-search-rust-validation.json).
The source frontend now gives safe Rust ownership of search intent, selected
playlist, asynchronous accepted-party consent, profile/couch generations,
party search notices, progress pane/title/clock and the observed B countdown.
Rust chooses the default eligible playlist and gates search on actual published
consent. It admits the selected full variant/map, transport team mode, local
seat count and profile names before requesting native cache validation and
publication. The old C++ policy bodies are removed. Typed CXX retains genuine
native snapshots, worker/resource IO, gameplay rule/profile writers, rendering,
input and menu effects. No Rust state guard spans a native callback; native
reentry and deep-copy value semantics remain valid. Raw profile/error bytes
avoid converting arbitrary native bytes to UTF8. Native FormatError recovery
discards launch ownership before cancellation; only the actual map validator
produces content rejection. Other exception objects/types propagate unchanged.

All 605 Rust tests pass (520 protocol, 65 bridge, 20 service), including 14 new
search controls; four existing ignored tests remain ignored. Unedited captured
modern C++ methods agree with production Rust/CXX across 3,763
sequences and 5,563 operations, with
26,496 observations made inside native callbacks.
The controls cover consent busy/ack/refusal, leaders/members, invalid retained
choices, generation changes, published countdown boundaries/retry, pane/overlay
transitions and ordered launch refusal/IO. 53 native IO
refusals match, and 3,763 actual public
wrapper controls preserve clone independence, callback reentry and native
exception identity. Another 532 controls compare the unedited modern variant
selector, including all 39 original presets and exact authored-name/suffix order.
The previously qualified full-settings validator and readiness error-dialog
mapping are shared Rust leaves. This turn
executes no new original binary; it compares the modern caller over declared IO.

The captured native C++ baseline passes nine focused CTests. Its build owner
was later terminated before candidate validation and cleaned up. A fresh
frozen candidate includes separately recorded unrelated dependency changes;
the native comparison is not a same-whole-source A/B proof. The rebuilt Rust
client passes those checks, both search/lobby replays and the build-helper test,
for 12 focused passes. Actual SDL/Vulkan menus against Rust HTTP include three
mouse actions and 23 rendered observations of playlist/Start behavior, shared
countdown, search and return. Each report matches the qualified native binary.
Four wider failures share the same observed diagnostics as the baseline: clan-requirement
text, matched/empty-row artwork and imported bitmap sampling. They remain red;
the complete native suite did not pass. Private provider fixtures do not certify
genuine providers, a played world or physical installed Linux input.

Native objects show eight policy definitions removed from the old C++ unit,
with their native methods delegating through 12 compiled Rust entries. Retired
C++ state fields are absent. Syntax/helper include issues and a slice-only CXX
vector link error were corrected before the final native qualification. All
owned Cargo/compiler/native build, SDK and capture scratch is removed; bounded
reports and source hashes remain. No selected runtime or production service was
replaced. Remaining progress/roster/input policy, the four broader failures,
original service/historical recovery, full natural outcomes/continuity, delivered
identities and restart/configuration/rollback remain open. All twelve gaps,
53 algorithm IDs and G01-G12 criteria are preserved unchanged.

## Native progress, roster and input policy in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-progress-rust-validation.json).
Safe Rust now owns the progress pane/title helpers, roster source and row
selection, original roster sort, placeholder/privacy gates, rank and speaker
routing, cancel/Y rules, leave-dialog count decision, widget selectors and
opaque tip clock/RNG state. C++ supplies actual cached observations and retains
localized text, asset/profile/voice lookups, drawing and physical input.
The search controller shares the same Rust pane/countdown leaves. Native status
values retain their full width. No C++ progress-kernel fallback remains.

Fresh unchanged-TU1.5 execution through the production CXX/Rust wrapper passes
2,087 controls: 856 complete sort-caller controls through the real qsort,
comparators and session getters; 536 input controls; 504 bounded roster
decisions; and 191 widget controls. Explicit synthetic session banks and
bounded consumers remain labelled. This does not recover original networking,
slot production, widget allocation, Windows-wrapped PC providers or Xbox pixels.

These controls expose and fix three prior divergences. The old stable sort
differs in 317 of 856 cases; Rust preserves the actual CRT swaps for equal
signed-byte keys. The original cancel caller uses signed elapsed time, including
negative wrapped differences (24 divergent controls). The leave dialog compares
all party players with local player seats, rather than connections (192
divergent controls). Two local players plus one remote player now choose 0x8C.
Changing only the count unit removes all 192 mismatches; changing the host
does not. The native smoke test's four modern ordering assertions were replaced
with all 856 original controls, including its exact four-player example.

All 613 Rust tests pass (528 protocol,65 bridge,20 service), with four existing
ignores. Eight focused progress tests also pass after the final fixture
expansion. The captured modern C++ methods and actual native Rust wrappers
pass 995 cases and 37,417 comparisons, permitting only separately verified
original sorting/timer corrections. Fourteen focused CTests pass across the
retained native runs, including the rebuilt SDL/Vulkan playlist/Start flow,
search/lobby replays, original countdown checks, native menu smoke and explicit
Linux MCC clan HTTP flow. The native baseline and candidate use the same frozen
source cohort except 27 declared edits.

Four broader failures remain red with the baseline diagnostics: unavailable
clan-playlist activation/text, gathering placeholder artwork, legacy clan
bitmap sampling, and MCC party-lobby bitmap sampling. The legacy clan asset
route is distinct from the passing explicit MCC clan flow. Full native
qualification has not passed. No selected runtime or production service was
replaced. All 5,890,901,773 bytes of owned native scratch and the other owned
compiler/Cargo/oracle scratch are removed; bounded receipts, commands and
source hashes remain.

Remaining native text/routing and matching-related party/local-seat producer
ownership, all 53 original owners, original service/historical recovery, all
required profiles/topologies/natural outcomes/continuity routes, genuine
providers, physical installed Linux input, delivery identities and
restart/configuration/rollback acceptance remain open. The full twelve-GAP and
G01-G12 criteria projections and goal objective are unchanged. The goal remains
active and whole-system parity is uncertified.

## Native matchmaking text policy and next-playlist countdown in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-presentation-rust-validation.json).
`matchmaking_frontend_presentation.rs` now owns native playlist/progress text
selection, substitutions, roster/needed counts, countdown/variant/map/quick-option
routing and compatibility explanations. The typed CXX boundary preserves opaque
native bytes, full-width status/count facts, native read order and genuine native
exception identity. C++ retains actual locale/assets/profile readers, the localized
rendering expression and engine drawing/input; it has no independent text-policy
fallback. Missing original clan strings remain absent rather than fabricated.

The captured modern C++ methods and actual Rust/CXX wrapper agree on 1,752 cases,
4,014 native IO observations and 19,646 comparisons, including 256 genuine native
exception controls. Six new Rust text tests pass. These are modern caller and
synthetic-reader comparisons, not recovered original service or locale producers.
Immutable TU1.5 static inspection identifies playlist activation at
`2BBE4B -> 2BBE83`; file/host/UI callees were not executed. Complete original
activation and selectability/eligibility production remain open.

A native search/cancel/next-playlist replay exposed a real Rust service defect.
The completed consumed countdown was correctly retained for late observers, but
its old playlist still blocked Start for a newly selected hopper. Rust now
prepares a fresh profile/roster binding after consumption while retaining the
old publication until commit. A regression reproduces the original 409 before
the fix and passes afterward through completion, accepted queue, API Cancel,
fresh member consent and a different playlist. Running and unconsumed zero
countdowns still lock their hopper, and nonleaders cannot start. This repairs
the replacement service lifecycle; it does not claim recovered original Live
arbitration. All 620 Rust tests pass (535 protocol,65 bridge,20 service), with
four existing platform-dependent ignores.

The wider native tests also required precise fixture repairs. Unavailable clan
requirements are inspected on hover before the explicit rejection dialog;
Back must retain the chooser and the rejection cannot start a countdown or queue.
Placeholder checks use the exact admitted raster registry rather than logical
image descriptors without pixels. Modal validation admits an actually visible
original lower channel, while hidden ancestors remain excluded. The map audit
uses the authenticated scenario alias and exact selected MCC pixels separately
from the original 220x207 logical descriptor. Fast loopback workers exhausted the
fixture's default aggregate prefix bucket; all aggregate fixture budgets are
now explicitly configured and any HTTP 429 fails the fixture. Production budgets
are unchanged. Temporary diagnostics were removed.

The final unchanged native cohort passes 18 CTests: four Rust/CXX policy
comparisons, original countdown/lobby fixtures, native menu and explicit MCC clan
HTTP flow, all native lobby/blocked/legacy/complete controls, actual SDL/Vulkan
playlist/Start input, three standalone Rust service flows and both complete
matchmaking/gathering HTTP flows. The wider legacy clan sampling and actual
selected MCC party-emblem binding checks remain red. No assertion was waived
for either remaining failure. Full native qualification is incomplete. All
owned build/Cargo/compiler/research scratch is removed; only bounded reports,
commands, source hashes and small reference inputs remain.

Remaining matching-related native party/local-seat/routing ownership, all 53
original owners, historical/service recovery, every required family/profile/
topology/unshortened natural outcome/continuity route, genuine providers,
installed Linux input and delivered restart/retry/configuration/rollback gates
remain open. No selected runtime or production service was replaced. All 12 GAP
records, 53 algorithm IDs, G01-G12 criteria and the active goal objective remain
unchanged; whole-system parity stays uncertified.

## Native accepted-party projection in Rust and UI qualification (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-party-policy-rust-validation.json).
The safe `matchmaking_frontend_party` module owns four native accepted-party
projection rules: flattened primary/local/remote seats, voice/status columns,
membership/invitation/error sounds and the first undismissed invitation notice.
Actual saved profile appearance, signed local seats and login authorization
remain typed native IO. Rust preserves opaque bytes, all nine appearance bytes,
optional postgame/appearance fields, remote connection handles, guest precedence,
full-width counts/status and native reader order. C++ supplies the actual readers
and ABI conversion without a projection-policy fallback. No Rust mutable state
borrow spans a native callback or menu reentry.

Captured modern native methods and the actual Rust/CXX wrappers agree on 3,105
cases, 2,688 native IO observations and 5,839 comparisons, including 14 genuine
native exception controls with exact exception identity preserved. Six new Rust
controls pass; the complete Rust suite passes 626 tests (541 protocol,65 bridge,
20 service), with four existing platform-dependent ignores. All 249 shared Rust
inputs match between the full suite and native cohort. These are modern caller
and synthetic-reader comparisons. No fresh original executable call or original
Live identity/seat/voice producer is qualified by this work.

The remaining legacy artwork failure was the original ONLINE bitmap1/page7
news decoration. Exact cache definitions and the existing artwork ownership
document show that the admitted PC bank has no counterpart. The native service
now hides that unused decoration under its exact screen/slot/frame/tag binding;
it does not alias a different PC page. This is a declared modern presentation
choice, not recovered original Live news. The party emblem audit now checks the
actual selected animated page, matching the renderer's descriptor selection,
instead of the immutable initial page0 pixels. Renderer/animation production
owners are unchanged, and selected page dimensions/format/pixels remain checked.

Two later fixture errors are corrected: browser pointer coordinates use the
menu's actual first-visible row, and selecting a playlist performs one Accept,
then explicitly verifies the original lobby with no countdown/search before
Start. Accelerated native account/queue/peer polling exhausted the party
fixture's aggregate source-prefix budget. All aggregate fixture budgets are
explicit, any HTTP429 fails, and real service limits remain unchanged. Temporary
diagnostics are removed. Native creation/invite/accept/leader transfer/leave,
duplicate chosen names, search/member cancellation/repeated Start, all four
countdown cues and retained hopper return pass. Vulkan controls confirm visible
one- and two-player emblem pixels against an emblem-suppressed framebuffer.

The final unchanged cohort passes 22 native checks, including the five Rust/CXX
comparisons, original lobby/countdown fixtures, legacy and explicit MCC clan
flows, complete matchmaking/gathering flows, actual SDL/Vulkan playlist/Start
input and the rendered party flow. No native check is skipped. Owned native,
Cargo, compiler and diagnostic scratch is removed; only bounded evidence and
small required reference inputs remain.

Remaining native party action/privacy/invitation routing/state and other
matching-related policy must still move to Rust. Complete original producers,
all 53 original owners, historical/service recovery, every required family/
profile/topology/unshortened natural outcome/continuity/results/PCR/return route,
genuine providers, installed Linux input and delivered restart/retry/configuration/
rollback gates remain open. No selected runtime or production service was
replaced. All twelve GAP records, 53 algorithm IDs, G01-G12 criteria and the
active objective remain unchanged; whole-system parity stays uncertified.


## Native party actions and privacy acknowledgment state in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-party-flow-rust-validation.json).
The safe `matchmaking_frontend_party_flow` controller now owns native party
action routing and targets, invitation versions/tabs/dismissals and the complete
pending privacy acknowledgment. The corresponding C++ fields and action/privacy
branches are removed from the party menu, account refresh and generic menu.
Rust selects all ten party commands, friend/clan invitation targets, primary or
signed-local player routing, voice operations, retained navigation and privacy
refusal/acknowledgment effects. C++ keeps actual worker/device operations, cached
snapshot encoding, locale/rendering and primitive menu effects. There is no
C++ action-policy fallback, and no Rust state guard spans a native callback.

All command fields remain byte-exact, including private join context, optional
privacy, full-width version/revision/serial and opaque identities. An absent
privacy result remains pending; an explicit false result rejects. The actual
party context and originating frame identity prevent an old acknowledgment
from closing another editor. Same-mode changes still invoke the authenticated
setter and require its fresh revision. The initial Rust port used queue
presence captured before the worker accepted an invitation. It now reads that
resource afterward, matching the captured C++ caller. The regression is red
before that correction and green afterward.

The full Rust suite passes 641 tests (556 protocol,65 bridge,20 service), with
four existing platform-dependent ignores. Fifteen new controller controls pass,
along with seven bridge-staging checks. Captured modern C++ methods and privacy
fragments agree with the actual native Rust/CXX adapter across 5,439 cases,
7,090 state observations and 7,193 native IO observations. All 54 injected native
exception observations retain their actual exception objects or nonstandard
values. Native callbacks reenter the same privacy owner without deadlock, and
copy/assignment/move controls preserve independent target, dismissal and pending
state. No original Live identity, worker, voice or admission producer is inferred
from these declared synthetic IO observations; fresh original execution is zero.

The CMake-linked comparison additionally emits its existing native shutdown
initializer/cleanup framing. A probe identifies the exact pair around a valid
matching JSON response and exit0. The replay driver admits only that exact
boundary pair and rejects unexpected output, missing operations, malformed
responses and process failures. The initial framing failure and correction are
retained; no comparison assertion was waived.

The final unchanged native cohort passes 23 checks with no skips: six Rust/CXX
policy comparisons, original lobby/countdown fixtures, native and standalone
Rust service flows, legacy and explicit MCC clan UI, actual SDL/Vulkan privacy
and playlist/Start input, complete matchmaking/gathering HTTP flows and rendered
one-/two-player party emblem controls. Owned native/Cargo/compiler/probe scratch
is removed, with bounded reports, commands, source hashes and required small
reference inputs retained. No installed runtime or production service is replaced.

Party menu/text policy, native lobby presentation kernels and the remaining
account refresh, party join/departure and voice intent decisions still require
Rust ownership. Complete original producers and all 53 owners, historical/service
recovery, every required family/profile/topology/unshortened natural outcome/
continuity/results/PCR/return route, genuine providers and installed Linux
restart/retry/configuration/rollback delivery remain open. All twelve GAP
records, 53 algorithm IDs, G01-G12 criteria and the active objective remain
unchanged; whole-system parity stays uncertified.


## Source-backed native lobby presentation and quick options in Rust (November 2)

[Qualification receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-view-rust-validation.json).
The safe `matchmaking_frontend_lobby_view` module now owns session-enum and pane
selection, protocol/status/subtitle/leader/privacy/prior-stat bindings, button
visibility and Start eligibility, game-type/icon/category/difficulty/help/count
selection and five-/six-row quick options. The old C++ policy bodies are replaced
by typed Rust calls. C++ supplies immutable cache observations and the actual
shared gameplay default-buffer/descriptor getter, with no selection fallback.

Guarded and unknown bindings retain their exact optional semantics: an empty
known status is `Some(0)`, a guarded widget is untouched, and an unknown privacy
value does not invent a string. Signed/full-width query values and opaque text
bytes are preserved. Quick options retain authored holes, duplicate row order,
first matching choice, override-before-default bounds, title+separator for an
unknown choice, repeated native getters and immediate failure at the observed
read. The native ABI rethrows the original exception object or nonstandard value.

All three Rust suites pass 653 tests (568 protocol,65 bridge,20 service), with
four existing platform-dependent ignores and twelve new lobby controls. Seven
bridge-staging checks pass. Captured modern C++ kernels agree with the actual
Rust/CXX implementation across 5,927 cases and 1,176 native IO observations,
including 387 production cache/default adapter comparisons. All fourteen injected
native failures preserve the exception identity; unexpected differences are zero.

Fresh unchanged retail Xbox executable and literal-cache oracles reproduce all
1,172 header fixture rows and 1,216 game-panel rows. The preserved game fixture
has one additional round-time value8 row that the general oracle did not emit.
An independent run through original19D220,2373BE,2374F0 and25092C confirms that
row: `Round Time Limit: `, with no matching option label. Its fixture remains
unchanged; the native comparator's stale quick-row count is corrected from70
to71. The ordered-variant integration fixture now supplies the real borrowed
asset directory to the unchanged confined reader. It also keeps unsupported
native metadata load/save/reload and explicit refusal separate from valid saved-
variant launch/edit/reload checks; production admission guards are unchanged.
Session queries, locale/widget services and the literal `%s%s%s` formatting
service remain supplied oracle boundaries. No native implementation supplies the
original option-matching result. The executable SHA is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`;
this retail proof does not certify TU1.5 upstream producers or original Live.

The final frozen native Release/O1 cohort passes 27 checks with no skips,
including original lobby/game/countdown/variant fixtures, seven Rust/CXX policy
comparisons, actual SDL/Vulkan privacy and playlist/Start input, complete native
matchmaking/gathering and standalone Rust service flows, and one-/two-player
party emblem pixels. The preceding full native attempt was interrupted by SIGTERM; its incomplete
result is retained separately and its scratch was removed automatically. The
initial isolated compiler harness omitted a descriptor
include; its corrected frozen cohort passes. The pre-existing game-fixture
coverage-count failure and its source-backed correction are retained. Owned
build/Cargo/oracle/compiler/SDK/capture scratch is removed; bounded commands,
source hashes, diagnostics and small required reference captures remain.

Party menu/text selection, lobby query/roster producers, account-refresh/party
join/departure and voice intent, and all other remaining matching-related policy
still require Rust ownership. Complete original callers and all 53 owners,
historical profiles/service ordering/arbitration, every required family/profile/
topology/unshortened natural outcome/continuity/results/PCR/return route, genuine
providers, physical installed Linux input and delivered restart/retry/configuration/
rollback gates remain open. All twelve GAP records and G01-G12 criteria remain
unchanged. No selected runtime or production service is replaced; full parity
stays uncertified and the full goal remains active.



## Rust party menu and text owner — 2026-11-02

The complete `MenuState::populate_online_party` and `online_party_text` policies
now live in `matchmaking_frontend_party_view`. The existing Rust party-flow
controller supplies picker tab and invitation/member targets directly. C++
supplies cached account facts, real expanded local-seat rows, resource and lobby
queries, byte-preserving text expansion, and primitive native frame writes.
There is no second C++ menu/text policy. Original/private locale namespaces,
first-match duplicate identities, raw indices, retained entries and mutation
order remain observable. The text adapter preserves the actual native exception
object, including non-standard exceptions; Rust holds no state lock across IO.

The captured modern C++ caller and exact production adapter match in 2,828
cases, 2,763 ordered native reads/frame snapshots and same-controller reentries,
and 78 injected native failures. Fifteen new Rust controls bring the three-crate
suite to 668 passes, with four existing environment-dependent ignores; all seven
build/staging helper checks pass. 28 native checks pass without skips,
including actual SDL/Vulkan playlist/Start/privacy and party pixels, matchmaking
and gathering, and unchanged source-backed lobby/countdown fixture checks.

The initial pre-change native build captured an unrelated Vulkan handle/viewport
shadowing compile error. Its failed report is retained. The accepted cohort
consumes the live checkout's already-present device qualification and two local
variable renames; this turn does not modify the shared renderer file.

[The bounded receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-party-view-rust-validation.json)
records frozen source identities, modern-versus-original evidence boundaries,
commands and scratch cleanup. This increment does not execute a newly recovered
Xbox/Live party producer. Original locale, identity, presence, rank and privacy
producers remain separate evidence requirements. Lobby query/roster composition,
account refresh/join/departure and voice-intent policy still have native owners.
Historical recovery, all original callers, full natural played-world coverage,
genuine providers, physical installed Linux input and delivered G01–G12 identity
and restart/retry/configuration/rollback acceptance remain open. The full goal,
all twelve GAP records and all 53 algorithm IDs remain unchanged and uncertified.


## Rust complete lobby query owner — 2026-11-02

The whole `MenuState::lobby_view()` caller now runs in
`matchmaking_frontend_lobby_query`, including ONLINE, local/campaign and System
Link paths. Rust owns route rejection, map/catalog selection, session/readiness
inputs, countdown presence, primary/local team-edit composition, ONLINE roster
capacity, requester lookup, actual member count versus physical machine count,
prior-statistics gating, and speaker state. It calls the existing Rust lobby
presentation/quick-option policies directly or through typed resource IO.
C++ supplies actual native profile/seat reads, the gameplay settings and custom
mode getters, localization, immutable cache records and opaque rendering values.
Rust-selected indices preserve native map pointer and all nine appearance bytes.

The literal captured complete caller and exact native adapter match in 1,055
cases and 23,520 ordered native reads and Rust reentries. All 542 injected native
failures retain their exception identity. The final three-crate Rust suite has
686 passes, eighteen new controls and four existing environment-dependent
ignores. Seven build/staging checks pass. 33 native checks pass without
skips, including rendered party and playlist/Start/privacy controls, matchmaking
and gathering, original readiness/team selection, catalog identity and local
profile conversions. The receipt retains source identities and scratch cleanup.

Fresh unchanged retail oracles regenerate all 1,014 readiness rows and all 958
team-selection rows byte for byte; 143 of the latter qualify the map team-count
primitive used here. A separate unchanged `19A2CE` execution supplies nine signed
member-team cases. It ignores IDs outside0..15. Rust makes that bound explicit,
preventing wider native shifts from aliasing a valid team. These nine cases are
checked directly against original outputs through Rust and CXX; they do not
execute the former C++ caller's undefined shifts. This is pinned supplied retail
Xbox evidence, not a new TU1.5 or historical Live producer claim.

[The bounded receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-lobby-query-rust-validation.json)
keeps the producer boundary explicit. Native committed-profile/member producers,
team editing, account refresh/join/departure, friend-join continuation, postgame
review and voice-intent policy still need ownership work. Original Live idle
status, rank/subparty quantity, complete original callers and historical service
ordering/arbitration are not inferred from this modern caller migration. Full
natural played-world scope, genuine providers, physical installed Linux input
and G01–G12 delivered identities/restart/retry/configuration/rollback remain open.
All twelve GAP records, 53 algorithm IDs and the full goal criteria are unchanged.


## Native account observation and join continuation in Rust (2026-11-02)

`matchmaking_frontend_account_flow` now owns the complete native account-refresh
observer and pending friend-join continuation. It selects identity/profile names,
admits changed account/voice revisions, bounds and orders notifications, handles
accepted joins and departures, switches requested party voice, prunes invitations,
and preserves retained-panel focus by opaque member identity. The existing Rust
party controller owns the pending join serial, account name, voice group and voice
intent; their four former C++ fields are removed. Rust also composes the voice flag
passed into matchmaking search. C++ retains worker, device and primitive frame IO.

All three Rust suites pass **702 tests**, including sixteen new account-flow
controls; four existing environment-dependent tests remain ignored. The exact
captured modern observer and production Rust/CXX adapter agree over **1,547
cases**, with **8,804 callback-boundary state observations and Rust reentries**.
All **72 injected standard/nonstandard native exceptions** preserve their original
identity/value and side-effect prefix. The tests cover unsigned serials and
indices, raw identity bytes, revision short-circuits, retained parties at sign-in,
join/departure guards, voice constructor/join failures, duplicate member labels,
search focus, partial population failures and real continuation reentry.

The frozen native cohort passes **34 checks without skips**, including
the actual SDL/Vulkan playlist, Start, privacy and party-pixel controls, native
matchmaking/gathering HTTP, and existing original-bound lobby/team fixtures.
Compiled-object checks confirm that both native observer entry points reach this
Rust owner and that the four duplicate C++ fields are absent. The initial harness
omitted its native-friends flag; its corrected run is the accepted comparison.
Two initial unit fixtures reused an already committed worker revision and were
corrected to advance that revision, preserving the observer's existing guard.
The first full native build exposed an older party comparator's missing voice
getter; that fixture now reads the real Rust controller, with its cases and
expected outputs preserved. Only that test and its link dependency were refreshed.

[The bounded receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-account-flow-rust-validation.json)
records source identities, comparison limits and verified scratch cleanup. This
ports modern client behavior; it adds no original Live provider or TU1.5 recovery
claim. Native account/provider binding, friends/clan menus and actions, team
editing, postgame policy and every remaining matchmaking policy/state owner still
require work. Full original callers, historical profiles/ordering/arbitration,
natural played-world outcomes, genuine providers, physical installed Linux input
and delivered G01-G12 restart/retry/configuration/rollback acceptance remain open.
All twelve GAP records, 53 algorithm IDs and full goal criteria are unchanged.


## Native friends caller and party actions in Rust (2026-11-02)

`matchmaking_frontend_friends` now owns the complete native friends caller:
provider-selection/availability decisions, native/social menu composition,
friend requests and party/invitation actions, selected-target state, roster
refresh/focus and friends text/columns. The existing Rust party controller owns
the friend and activity targets; their former C++ fields are removed. Pending
joins still consume actual worker serials and authoritative account membership.
The native join offer and command handler retain their distinct context guards;
this migration does not invent additional admission rules.

Native code supplies typed account/social observations, localization, opaque
snapshot storage, device/provider calls and primitive frame/navigation IO. The
captured complete modern C++ caller and exact production Rust/CXX adapter agree
on **6,025 cases**, **4,993 callback-boundary observations and Rust reentries**,
and all **106 injected standard/nonstandard native exceptions**, including
their original identity/value and side-effect prefix. Detached frames, actual
roster indices, raw identity/context bytes, unsigned serials, busy/presence/join
gates, provider reentry, resource failures and full cached opaque social data are
compared. Target initialization uses actual selection actions through the Rust
controller. Roster refresh retains the last duplicate identity match; this stays
distinct from account refresh's first-match rule.

All three Rust suites pass **722 tests**, including twenty new controls; four
existing environment-dependent tests remain ignored. The frozen native cohort
passes **35 checks without skips**, including the new whole-friends
comparison, actual SDL/Vulkan playlist/Start/privacy controls and party pixels,
native matchmaking/gathering HTTP and existing original-bound lobby/team checks.
Compiled-object checks confirm that the native entry points reach Rust and the
two C++ target fields are absent. Seven build-staging checks also pass.
The first native build stopped on an unrelated frozen physics compile error.
Its owner's current fix and thirteen-file dependency closure were refreshed
inside the owned snapshot; shared work and matchmaking sources were untouched.
The retained initial failure and refresh hashes do not establish physics parity.

Former C++ invitation expressions can negate `INT_MIN`, which is undefined in
C++. The C++ equivalence corpus excludes that input. Rust uses the equivalent
bitwise index for admitted negative values and separately tests safe invalid
`INT_MIN` handling. This is not a new original-Xbox producer or Live claim.

[The bounded receipt](evidence/matchmaking-rust-parity-20261028/native-frontend-friends-flow-rust-validation.json)
retains source identities, declared provider boundaries and verified cleanup.
Native account/voice binding and lifecycle, clan menus/actions, team editing,
postgame review and every remaining matchmaking policy/state owner still need
work. Full original callers, historical profiles/ordering/arbitration, natural
played-world outcomes, genuine providers, physical installed Linux input and
delivered G01-G12 restart/retry/configuration/rollback acceptance remain open.
All twelve GAP records, 53 algorithm IDs and full goal criteria are unchanged.

Separate read-only launcher observations retain both launcher identities and
the selected runtime/source-manifest hashes. The initial observation found the
new friends inputs and the existing Rust party-flow owner absent from that
manifest. No selected runtime or production service was replaced in this work;
controlled native checks and physical installed-client delivery stay separate.


## Rust implementation boundary

**All production matchmaking code must end in Rust.** This includes catalog
and profile interpretation; eligibility and matching-related party/local-seat
rules; queue, ticket, gathering, query and failed-session state; rank and
player/clan XP decisions; candidate scoring/sort/readiness; the full assignment
caller; RNG and selected variants; matchmaking host/countdown/session decisions;
admission/result validation; and result finalization/namespace/transaction rules.
The Rust service/API must own matchmaking state and the production endpoint
behavior. A Rust leaf hidden behind a still-authoritative Python coordinator is
not completion.

During migration, existing Python may serve as a temporary transport/storage
adapter. Native C++ may retain rendering, physical input, engine gameplay and
typed ABI/transport integration. These adapters must not retain independent
matching, ranking, assignment, draw or finalization policy. Unrelated friends,
identity and other services do not require a speculative rewrite. Preserve
Steam authorization, opaque identity, one-clan membership, secure transport and
transactional history throughout the replacement. Do not introduce a Python or
C++ fallback when the Rust owner is unavailable.

Prefer the existing Rust crates and pinned toolchain/bridges where suitable;
introduce a dedicated Rust matchmaking service crate if needed for state,
HTTP and persistence ownership. The implementation must remove duplicate
production algorithms and prove that every selected route reaches Rust.

## Ordered goal and completion requirements

1. Lock down this baseline as failing Rust/integration regressions. Recover and
   replay original hopper caller/getter behavior; implement and route complete
   assignment in Rust, including the reproduced two-team failure.
2. Move rank/skill producers, search/gather clocks and candidate decisions into
   Rust with explicit versioned inputs. Fix guest aggregates and verify remote,
   signed-local, guest and clan boundaries against original controls.
3. Implement authenticated full-buffer profiles, catalog parsing, exact draw,
   immutable admission binding and all corresponding rejection paths in Rust.
4. Move matchmaking coordinator/API/state/result/persistence decisions into
   Rust and remove Python/C++ policy duplication. Preserve security, idempotency,
   namespace continuity and non-matchmaking XP isolation.
5. Qualify all original-valid families, presets, historical variants and
   topologies through genuine online outcomes, continuity and party return.
   Continue historical/service recovery alongside implementation; unavailable
   original evidence stays explicitly open.
6. Bind the tested and delivered artifacts to all applicable PDF G01-G12 gates;
   reconcile documentation and the 53-owner inventory. Do not mark whole-system
   original parity complete from a bounded kernel or profile pass.

The full target includes Slayer, CTF, Assault, Oddball, King, Territories and
Juggernaut; all 39 built-ins plus recovered historical variants; ranked and
hidden-skill casual playlists; solo/party/signed-local/guest/clan subjects;
original-valid FFA/two-team/multi-team and large sessions; natural wins/ties,
departures, graceful/abrupt host changes, results, PCR and requeue. Do not
invent unsupported Cartesian combinations. Retail, TU1.5 and each dated
historical service profile retain separate provenance and statistics continuity.

Use `tools/run_tests.py` for CMake checks. Use `tools/run_test.py` and its owned
scratch for compiler/oracle/capture probes; keep Cargo targets, binaries, raw
banks and captures inside that lifecycle. Start native builds conservatively
with two build jobs and one test job. Preserve unrelated active changes,
original assets, retained reference evidence and selected launcher runtimes
while developing. Record tested hashes and clean up owned generated storage.

## Reproduction and retained storage

The fresh original kernel invocation was:

```sh
python3 tools/run_test.py -- .tools/python/bin/python -B \
  tools/oracle_tu15_team_balance.py \
  --fixture @HALO2_TEST_SCRATCH@/team-fixture.json \
  --report build/test-results/matchmaking-parity-20261028/team-balance-original.json
```

The legitimate retail and TU1.5 executables match the PDF's recorded hashes:
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`
and `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Original executables/assets were read in place and are not copied into this
handoff. The pre-existing constructed retail fallback is retained at its
existing owner path; see the environment receipt for its identity.

The supplied reference PDF is intentionally preserved as a required goal input
(639,228 bytes), alongside small JSON receipts. Generated Rust probe binaries,
the oracle's temporary fixture and rendered PDF pages were removed by their
scratch owners. No large generated test artifact was retained. The
[cleanup receipt](evidence/matchmaking-rust-parity-20261028/cleanup.json)
records the checked scratch paths.
