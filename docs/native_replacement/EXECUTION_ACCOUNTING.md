# Disposable source-route accounting

`HALO2_NATIVE_EXECUTION_ACCOUNTING=ON` instruments build aliases after exact-body
native binding and operand specialization, before translation-unit partitioning.
The option defaults off and requires a build inside `HALO2_TEST_SCRATCH`.
Canonical bodies, service callbacks, and native admission are preserved.

An instrumented process writes a bounded JSON report when
`HALO2_NATIVE_EXECUTION_REPORT` names an owned scratch output. Each recognized
function activation owns its own counter scope, including nested calls and
recursion. Counters distinguish actual State image, entries, literal body entries,
literal statement observations, normal returns, exception unwinds, and returns
or unwinds without literal body execution. A parent's native return does not
hide a translated child. Literal counters run even when instruction services
are absent. Repeated-string conditional observations retain their source count
condition; dynamic forwarding callbacks remain explicitly unresolved.

Function identities include canonical source path and source/input hashes.
Descriptive, attributed, template and nonvoid literal owners are included,
including animation selection, pickup, weapon routing and Effect code whose
filenames lack `_generated`. Local Effect headers are embedded at their
original inclusion boundary in the disposable CPP and retain separate canonical
source/hash identities. Global identity declarations precede the original
namespaces. Canonical files and lambda captures remain unchanged.
Runtime State image metadata does not authenticate a callback-PC remapper.
The 302 animation-selection/pickup/routing routines retain canonical-only
callback identities; their actual TU1.5 function addresses remain unresolved
until their owning runtime remappers are authenticated. Inventory and queue
metadata do not infer these addresses from State image or `run_` names.
Registration failures, omitted rows, active scopes, unknown image values, and
dynamic forwarders prevent an inference of complete coverage. Counter
registration and scope destruction are `noexcept`; allocation failure is
reported instead of replacing a source exception. Literal sites inside unchanged
lambda captures use their function's static counter and a thread-local live
activation lookup. A deferred event with no owning activation records an orphan;
the reader rejects that report. Nested calls retain their own attribution.

`body_entries` means an activation reached a literal observation site. A
nongenerated semantic kernel can contain a genuine service observation, so this
field alone does not prove translated emulation. Source receipts distinguish
instruction-shape candidates from native observation boundaries.
`tools/native_service_observations.py` classifies the shared millisecond-clock
owner only when its complete source hash and function identity match the audited
catalog. Its three observations, including real platform service `33B595`,
remain intact. Runtime rows and all raw totals stay unchanged; reader summaries
report those observations separately from translated fallback rows, and migration
queues exclude that exact qualified owner. Unknown or changed source hashes stay
unclassified. This classification does not establish service dependency closure
and does not reclassify historical cohorts with different source identities.

Use the existing owned build coordinator with the option and retain the small
`native-execution-coverage.json` report when building
`native_execution_accounting_aliases`. That target generates aliases and
receipts; it does not compile or run the game. Run the resulting engine with
its report inside the same scratch owner, then retain only the bounded report
and analyze it with `tools/native_execution_report.py --runtime REPORT --output
SUMMARY`. The File API closure exporter understands accounted raw, specialized,
and partitioned aliases and retains the corresponding provenance receipts.

The initial all-configured-target Linux Release alias pilot generated 248
variants from 184 units / 8,759 source-qualified functions, with no source drift.
Its receipt is
`build/test-results/native-execution-accounting-aliases-r4-20261108/evidence/native-execution-coverage.json`
(153,747 bytes, SHA-256
`58a7fcd673a1621d3e36125139e02fc2ad8017295fdce1b7aaefc1adc5610d5f`).
This scope includes nonproduction test/tool targets; the configured production
dependency closure is a separate subset. Private generated aliases and build
products were removed by the runner.

The production subset is independently resolved in immutable
`evidence/linux-release-production-closure-20261108-accounting-r6.json`:
180 units, 171 targets, 224 aliases, 180 accounting receipts, 147 operand
receipts and two partition receipts, with no unresolved source/dependency
identities, including the shared native stack arena. The 780,680-byte receipt
has SHA-256
`92bd32f8517def71a67fe1a16869ed5fcdeeed2abe1bbef325b5dfb886719086`.
Every production generated source is accounted directly or through its partition
input. This remains source-generation evidence, without a game build or run.

Focused regressions compare original and instrumented behavior with disabled
and enabled callbacks, real default-native BSP admission, nested fallback,
recursion, source exceptions, concurrent cold registration, registration
allocation failure, images, and repeated-string observations. The actual
145D50 numeric leaf is compared on native and forced-reference routes across
36 caller x87 precision/rounding/depth setups, preserving CW/status, live 80-bit
values, MXCSR, complete GP/flags/all-XMM state and full memory banks.
The small retained fixture runtime reports and validated reader summaries are in
`build/test-results/native-execution-accounting-final-r2-20261108/evidence/accounting-regression.json`.

A completed return without a literal body observation proves only that bounded
route. It does not qualify unentered functions, non-generated/native service
closure, fault prefixes, other platforms, frame parity, or a promoted runtime.
Typed native child calls do not enter their retained generated wrappers, so
their absence from wrapper counts must not be read as absence of native work.
Accounting changes timing, so performance acceptance uses a separate build
with the option off.

The broadened Linux Release pilot at
`build/test-results/native-accounting-broadened-cmake-20261109` passed all four
focused CTests and generated 272 aliases from 208 configured source units
(184 generated filenames and 24 nongenerated CPP/header owners), covering 9,112
source-qualified functions without source drift. Its coverage receipt is
188,485 bytes, SHA-256
`d84b76aea1a0529d34911a1777dc5d4a7b9c36657824a1eb86bdf1560d8c7ce6`.
The nine accounting regressions retain the numeric/FP, exception and concurrency
checks above and add unchanged explicit lambda captures, nested callback
ownership, orphan refusal and embedded-header namespace/identity checks.
Five actual instrumented CPPs passed syntax checks: animation selection,
pickup, routing, Effect (with nine included headers), and live world. The
live-world probe uses its normal SDL compiler include flags; an earlier probe
without those flags remains recorded as a failed configuration.

Immutable production closure
`evidence/linux-release-production-closure-20261109-accounting-r7.json`
records 173 evaluated targets and 204 configured source/header units: 180
generated filenames plus 24 nongenerated owners. Header receipts authenticate
their canonical hash, exact embedded body and containing configured CPP bytes.
The 822,011-byte receipt has SHA-256
`dfae222939c194999239591f70f0357476d2d0f148188417f596543f767322a4`.
The current source inventory records 212 candidate units / 9,151 bodies; the
additional private heap clock header is explicitly outside the configured
prepared include closure, with production reachability still requiring audit.
Old r4/r6 and the completed eight-workload runtime cohort retain their original
generated-only scope. No captures were repeated or retroactively expanded.
All completed private trees, aliases and binaries were removed; only bounded
reports remain. A new engine cohort is needed to measure the broader scope.

The later r4 **engine** cohort (distinct from the older r4 File API catalog)
measured that expanded universe. Its 48 component tests, full engine build and
eight 90-tick Coagulation/Zanzibar static/moving 1p/4p runs passed. Runtime
`47fddfc6c9d57db20876fcbb1f1a9cb5ed7400f62c9e996c5a0b864d9e7c5885`
records 21,353,668 entries and 891,261,650 literal observations, entering 2,915
source-qualified functions with 2,801 body observations in 86 units. All active
scopes are zero. The three legacy filename omissions are covered in this cohort;
their actual image-qualified identities remain unresolved as described above.
The 1,796 unwind activations map to the arena worker stop and deliberate lens
pass-boundary owners; the raw accounting does not record exception type/throw PC.
Evidence and the measured residual queue are under
`build/test-results/native-execution-engine-r4-20261109/` and
`build/test-results/native-execution-hot-queue-r4-20261109/`.
Owned source/build/captures were removed. This cohort records diagnostic
accounting, with no frame A/B, timing or native-only acceptance. Subsequent
datum/resource/BSP/page/effect-transform ports are outside its frozen source.

The later r6 engine cohort passed 62 component tests and built 275 accounting
aliases over 209 canonical units / 9,113 functions with zero canonical drift.
The full engine build stopped when the partitioner mistook an embedded inline
clock helper's function-local counter for namespace state; captures did not run.
The correction preserves function-local ODR state and still rejects unowned
namespace statics, including macro/raw-string controls. Three CMake regression
checks and both real partition inputs (2,026 primary and 691 AI-frame bodies,
all body bytes unchanged) pass. Bounded proof is in
`build/test-results/native-engine-partition-ownership-20261109/`.
Both the failed engine owner and completed regression owners were removed;
no large artifacts were retained. A later engine run must qualify integration.

The completed r7 **engine** cohort passed 65 component checks, the full engine
build and all eight bounded accounting runs. Its frozen source inventory is
`0af6a316f04bf0e3a156640f521b70301a7351b3d1e2e5916e66fb7ab29a38fb`;
284,873,272-byte runtime SHA-256 is
`a96c9141c7bd97d18874f0fccc5ee8e997b6c21f92e96827062b8a0796accb52`.
It records 839,980,777 raw literal observations across 2,914 entered functions,
with 2,788 literal-fallback functions, 125 no-body functions, one hash-qualified
native-service observer and five mixed functions. All active scopes and omitted
rows are zero. The 1,796 unwind activations have the same three owners and seven
frozen source contracts as r4; their exception types still are not dynamically
recorded. All eight retained frame hashes match the earlier bounded native r4
cohort; this is a cross-cohort native regression check, separate from original
executable frame parity. Bounded queue, unwind and frame-hash comparisons are in
`build/test-results/native-execution-hot-queue-r7-20261109/`.
Owned source/build/captures were removed; only 6,257,945 bytes of bounded
accounting metadata remain, with no retained runtime binaries or frames. Later
source ports remain outside this cohort. Native-only, full original-frame,
all-map/campaign, performance and launcher qualification remain open.

The immutable r11 File API catalog records 173 evaluated targets and 204
configured generated/nongenerated units with zero source/preparation drift:
`evidence/linux-release-production-closure-20261109-accounting-r11.json`,
982,132 bytes, SHA-256
`5c927a7df780ddfdda648124eb8a514522d7d6bc3f33de5b4de9162647e4dd0f`.
Later module registrations are outside this catalog. It authenticates configured
bindings and identities, without proving runtime execution or native admission.
