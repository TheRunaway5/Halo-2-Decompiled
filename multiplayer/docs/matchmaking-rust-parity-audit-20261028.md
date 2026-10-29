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
consent/departure/retry/security cases; all304 option-byte mutations across seven
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
public history reads are temporary. All12 gaps,53 algorithm IDs and12 acceptance
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
tests and 272 backend tests with diagnostic restoration disabled. Both manifests
match current source. CMake's offline Rust build/helper passed with two build
jobs and one test job; its 80 recorded Rust inputs stayed unchanged. Two existing
capture/performance tests remain ignored.

All nine reported scratch trees and the receipt helper were removed; the owned
scratch parent is empty. The 35 retained source reports total 420,206 bytes,
excluding the summary receipt. No generated large artifact or selected runtime
was changed.

No new unchanged-original execution, original Live party procedure, actual
played engine/world or delivered activation is claimed. All twelve complete
gap criteria, 53 algorithm IDs and full G01-G12 gates remain open and intact.

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
