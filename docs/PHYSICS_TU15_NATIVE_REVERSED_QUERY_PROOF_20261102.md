# Native TU1.5 reversed and conditional query parents

The authority is the unchanged English TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This work concerns source contact-query processing, not another factory.

| Owner | Native semantic responsibility | Sites |
|---|---|---:|
| `2F1A50` | Reverse the input payloads; flip only newly appended record normals | 39 |
| `2F0F10` | Build the query descriptor, query, and manage the conditional process agent | 125 |
| `2F6F40` | Build wrapped geometry and descriptor; call child query virtual `+4` | 46 |
| `2F8410` | Compose inverse geometry, call the simplex solver, test distance, collect | 91 |
| `30BA80` | Write exactly `collector+4=1`, `RET8` | 2 |
| `8EB50` | `RET16`, with no operand or FP effect | 1 |

The four new native modules are `reversed_query_process`,
`conditional_contact_process`, `wrapped_contact_query`, and
`convex_overlap_query`, under `include/halo2` and `src/physics`, with the
`native_tu15_` prefix. They reuse the existing native `2E44D0` and `2E4550`
geometry kernels. They contain no instruction decoder, virtual CPU register
file, supplied collision result, or implementation of the remaining solver.

## Actual Elongation ownership

The protected multiplayer 1473/03 BEFORE archive is
`analysis/multiplayer/physics-live-tu15-20261012/mcc-native-agent-wake-conveyor-original-v1473-20261102/iteration-03-native-wrapped-discrete-authored-elongation-conveyor-player-camera/original/required-failure-reference/actual-failed-capture.tar.zst`,
SHA-256 `30ab86b3bc0b0127ded36f09cbf30d30b5dae4fa81e8d9f6b2361b1fbec08133`.
Only BEFORE banks were extracted. No expected AFTER bank was used as input.

Executing that unchanged original frame reaches two `2F1A50` entries, receivers
`10083060` and `10083080`. Both select:

`2F1A50 → 2F0F10 → 2F6F40 → 2F8410 → 30E930`.

The conditional query receivers `1007A8D0` and `1007A8C0` have actual table
`41CC64`; their child query owners are `2F8410`. Cached process agents
`1007C710` and `1007C740` select actual `8EB50`. The real query sets its
collector flag to one. Output contact count remains zero, and no constructor
or cached-agent destruction is selected. This is a source-execution witness
derived from an actual world BEFORE, not a captured native leaf ABI.

The small original entry census is retained in
`analysis/physics/native-reversed-query-actual1473-frame-census-20261102-third`.
The first census compile failure and later successful reports remain intact.

## Strict independent parent qualification

`analysis/physics/native-reversed-query-parents-tu15-20261102-second/report.json`
is GREEN for GCC and Clang: 146 declared physical profiles, including 48
genuine child-entry boundaries, 82 read/write/observer fault prefixes, 1,682
ordered reads and 666 stores with their exact preimages. Every declared bank
and FP state is compared; source `RET16` and collector `RET8` are checked.
Three mutations—wrong reversal, wrong wrapped child shape, and wrong simplex
flag—are rejected by both compilers. Closed empty-query profiles execute the
real `8EB50` source method rather than receiving an invented query outcome.

The original and native descriptors use the same explicitly owned DATA in
these independent parent tests. No saved source GP/control bytes are made
native inputs. These checks do not exercise the positive appended-record loop,
the conditional constructor or clear branch, or the zero-result solver path.

## Actual whole-return composition, with the solver still explicit

`analysis/physics/native-reversed-query-actual1473-composition-20261102-second`
compares both actual entries through their genuine `RET16`, with both GCC and
Clang. Both processes independently execute the original frame to the selected
entry; one process's GP file or later bank state is never installed into the
other. All parents, geometry, collector and cached no-op run natively.

The **sole remaining `30E930` service runs unchanged original instructions in
an offline reference child**. Its current typed shapes, cache, initialized
64-byte relative geometry, simplex flag and FP inputs match the independently
observed real call boundary. Nonformal GP and XMM registers receive independent
poison values. The service computes its real result; no return or output
postimage is supplied. Both actual calls return three, select native
`30BA80`, and complete all four parents. This offline service is not a native
solver implementation and must not become a production fallback.

Each profile compares all 89,213,849 captured bytes raw. All 89,148,313 bytes
outside the input's existing void-frame private range match exactly, as do the
explicit returned 20-byte output header, contact records and FP state. The
raw full-bank result remains **RED**: 1,188 and 1,419 bytes differ in the existing
returned private control/DATA range. Those differences and examples remain in
both compiler reports. They have not been masked, zeroed, reconstructed from
saved GP state, or reclassified as a raw GREEN result. The private range comes
from the original void-frame comparison contract, not a new exclusion added
to make this component pass.

## DATA and ABI lifetime

`2F1A50` allocates no semantic scratch. It snapshots the initial output count,
calls `2F0F10` with reversed payloads, then rereads the actual holder and header.
For each newly appended 48-byte record it reads the current backing, reads and
XORs the vector at `+10`, stores it, then rereads the holder and count. Signed
count comparisons and unsigned address arithmetic preserve the source order.

`2F0F10` owns independent 16-byte descriptor and eight-byte collector areas.
All four descriptor words are derived from the current first payload and
shape, in source read/store order, before the query. It initializes the
collector table and flag; its other three bytes are not read by this parent.
Later processing overwrites descriptor shape and inherited-word fields from
current source inputs. The factory, query, process and clear services borrow
these DATA areas through return and remain explicit typed dependencies.

`2F6F40` owns a different 16-byte descriptor and 96-byte geometry area. It
copies the two source vectors, uses native affine geometry for the other 64
bytes, then initializes every child descriptor word before calling virtual
`+4`. The conditional descriptor remains alive during this nested call.

`2F8410` independently owns the retained shape/agent identities, 16-byte plane,
64-byte inverse geometry and simplex DATA. The exact real solver receives only
its ordinary typed arguments. In the actual profiles no DATA pointer escapes
into persistent output: all persistent bytes compare equal despite using
different native scratch identities. This observed profile is not an escape
proof for every unqualified solver path.

The local authenticated CFG audit in
`analysis/physics/native-reversed-query-context-audit-20261102-first/report.json`
passes all seven owners, including `30E930`, with XMM definitions invalidated
after every call. It finds no incoming semantic GP dependency beyond typed
ECX and ordinary ESP arguments. Control-only prologue saves are kept separate.
This is local liveness evidence, not transitive native solver qualification.

## Remaining work and binding boundary

No World, shared generated source, CMake or acceptance gate was changed by this
work. A production binding must use native implementations for selected known
owners, retain disjoint caller-owned descriptor/collector/geometry/simplex
areas until each call returns, and count and refuse unknown owners. It must not
silently route any new branch through a virtual CPU.

The native `30E930` solver, positive appended-contact reversal loop, conditional
constructor/clear branches, profiling-enabled input domain and solver-zero
distance path still need their respective proofs before broader admission.
The actual solver-three/current cached-noop profile has complete semantic
return evidence around an explicit offline solver service. It does not certify
full native physics, the separate strict Elongation heap mismatch, a successful
authored conveyor approach, or visible multiplayer delivery of these modules.

All probe/build/raw-bank scratch is owned by `tools/run_test.py` and removed.
Only source, small reports and the previously protected required reference
archive are retained. Storage removal receipts are recorded in
`analysis/physics/native-contact-native-kernels-storage-20261101.json`.
