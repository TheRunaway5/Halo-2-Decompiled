# Remaining MOPP ray traversal (`30AA70`)

The r11 eight-run queue ranks this parent first with 21,114,221 literal
observations. Two production copies remain translated: Death and AI-frame,
each 899 source lines / 896 original sites. Their exact complete body hashes
without a trailing newline are respectively
`f5d6ae21129685622ea289febf4dae15317283b7cfb2c14c299d3853fc25070e` and
`0965ba2dfbc6d947bea64e48f8224b3f85d36efbe9c39739f46d7ec012d0064d`.
The bodies match exactly after removing the AI-frame copy's 94 explicit C1-clear
annotations; their owning FP primitives must still be authenticated separately.
This normalization is an identity observation, not permission to weaken a pin.

## Shared native implementation

`include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp` now owns the native
numerical operations and complete authored-ray DATA controller. Public
`native_tu15_traverse_mopp_ray` consumes that controller through its existing
value-frame contract. Geometry program bytes describe quantization, projected
splits, slabs, primitive keys and properties. The controller performs native
x87/SSE arithmetic and typed geometric decisions without a CPU register file,
instruction decoder or flag-driven instruction execution.

The ten non-axis projection rows share
`native_tu15_mopp_ray_projection_kernel.hpp`, retaining axis/sign/read order and
native extended precision. Immutable opcode-class/handler code fetches become
compiled DATA classification; ordinary captured DATA banks have no exclusions.

The public environment preserves the complete native x87 register bank and
MXCSR around actual DATA reads, vector reads, writes and observers, including
provider exceptions. Providers run in the saved host environment. Explicit
filter, shape and cast services retain their separate FP handoff policy.
This prevents a normal host provider's FP work from destroying a live native
projection or slab intermediate. It does not publish a retained CPU frame.

## Evidence

The current shared integer/quantization/key/plane/branch/crossing/resume/slab DATA GCC/Clang gate passes 24/24: each compiler checks 2,201
unchanged-i386 fixtures, including 1,971 full returns, 200 genuine first child
entries and thirty read-fault prefixes; 788/896 parent sites are reached. Nine
compiled mutations change scale, key base, clipping direction, a projection
sign, actual provider FP protection, encoded-byte order, primitive-key
assembly, plane quantization, branch direction, crossing numerator or resumed axis direction and are rejected. Every fixture runs with
ordinary and deliberately clobbered x87/MXCSR providers: 4,402 native comparisons
per compiler, including exceptions. Typed views also preserve all four authored
property writes within their declared value lifetime. The exact receipts and
removed-owner checks are in
`build/test-results/native-mopp-ray-branch-data-first-20261109/report.json`
(the earlier integer/scale receipts remain preserved).

The actual-input shared-controller gate also passes GCC and Clang against the
unchanged original executable, using four independently invoked typed entry
profiles and all 89,795,481 captured before-input bytes. It consumes no captured
GP or after state, supplies no filter/shape result, and retains no raw bank.
The existing required reference archive is preserved. Its receipt is
`build/test-results/native-mopp-ray-primitive-key-actual-20261109/report.json`.
All completed probe/build owners have been removed. The registered DATA gate
passes in the r11 coherent engine checkpoint and the r12 component checkpoint.
Neither result installs a raw production parent prefix.

## Private caller and integer frame

`src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp` owns the real aligned
0x194-byte frame, lazy argument words and empty-geometry return. Its separate
integer view consumes the shared native authored DATA controller for jumps,
key-base changes and four property slots. It preserves interleaved vector/scalar
copy, overlapping variable feedback, live callee-saved GP/stack return values,
post-success PUSH publication, and RMW flag retirement after a successful store.

The unchanged-i386 GCC/Clang gate passes 1,364 physical cases: 300 entries
(255 returns and 45 actual MOVAPS alignment faults), 442 stopped reads,
310 atomic-provider store failures and 312 completed-write observer failures.
Each compiler checks 1,619 native comparisons and 19,146 complete physical
pre-I/O packets over 149/896 sites. Complete GP/flags/XMM/FP/live80,
caller/program banks, ordered reads/stores and observer preimage/written bytes
compare. All sixteen entry stack alignments, DF, 0/1/4/8 live x87 inputs,
precision/rounding controls and DAZ/FTZ are represented. Alignment faults retain
unchanged execution flags and a separate RF-set saved exception record, compared
exactly to the original hardware signal context. The exception distinction is
specified in [Intel SDM volume 3B](https://cdrdv2-public.intel.com/774491/253669-sdm-vol-3b.pdf).

Receipts are `build/test-results/native-mopp-ray-integer-caller-frame-r5-20261109/`
and the registered sanitizer/release 2/2 gate at
`build/test-results/native-mopp-ray-integer-frame-official-20261109/`. All their
owned banks, reference executables and build trees are removed. These private
components introduce no production prefix or supplied geometry result.

## Private numerical frame in progress

The shared `native_tu15_mopp_ray_scale_kernel.hpp` now separates typed scale DATA
order from its public value storage. A private `ScaleFrame` consumes that same
controller through actual caller aliases and lazy 16-byte/scalar reads. It uses
the independently qualified [raw x87 ownership scope](RAW_X87_SCOPE.md) around
providers and observers. A staged conversion/pop commits only after provider
write success and before the completed observer; scalar and vector ABI values
are published at the real I/O boundaries.

The widened unchanged-i386 raw scale/integer gate passes GCC/Clang sanitizers:
1,808 physical cases, including 403 returns, 49 actual alignment faults,
554 read prefixes, 390 atomic-provider store failures and 412 completed-observer
failures. Each compiler checks 4,422 native comparisons across ordinary and
hostile x87/CW/MXCSR providers, including unmasked pending provider exceptions.
All 34,638 original pre-I/O packets, full return/prefix ABI, actual live80,
ordered I/O and whole banks match across 204/896 sites. Bounds/private-frame,
variable/private-frame and bounds/variable overlaps are included. The existing
direct-I/O integer adapter separately reruns its original 1,364 cases and
1,619 comparisons per compiler after the new optional I/O policy.

Receipts are `build/test-results/native-mopp-ray-scale-frame-aliases-20261109/`
and `build/test-results/native-mopp-ray-integer-io-regression-20261109/`;
owned reference executables, banks and builds are removed. A subsequent compiled
premature-spill-commit defect exposed missing FST/FSTP failure prefixes in the
private classifier: Capstone marked these pure stores as reads. The failed
control receipt is preserved at
`build/test-results/native-mopp-ray-scale-frame-negatives-20261109/`; the first
registered run was cancelled and cleaned before acceptance. Authenticated
FST/FSTP operand ownership is now corrected in prefix selection, matching the
independent physical runner. Both compiled integration defects are rejected by
the strengthened 79-case / 160-comparison focused gate at
`build/test-results/native-mopp-ray-scale-frame-spill-prefixes-20261109/`.
The strengthened complete corpus now passes GCC/Clang sanitizers with explicit
signed-zero, denormal, infinity, quiet/signaling-NaN inputs and both integration
mutants rejected: 1,828 physical cases / 4,462 native comparisons per compiler,
including 410 pure store-failure prefixes and 35,190 full original pre-I/O
packets. Its receipt is
`build/test-results/native-mopp-ray-scale-frame-final-20261109/report.json`.
The removed-owner check is in its adjacent `cleanup.json`. The registered paired
sanitizer/release gate also passes 2/2 (110.54 / 33.17 seconds) at
`build/test-results/native-mopp-ray-scale-frame-official-r2-20261109/`.
Its exact current-source closure and removed-owner verification are in
`build/test-results/native-mopp-ray-scale-frame-closure-20261109/report.json`.
No generated bank, dependency copy, executable or build tree is retained.
The adapter remains private/unbound. Its later shared primitive-key integration
also passes the registered scale/key sanitizer/release suite, 4/4, with exact
current source pins at
`build/test-results/native-mopp-ray-shared-frames-closure-20261109/report.json`.
All nested owners and the pinned dependency copies are removed.

## Encoded primitive-key view

The public value traversal and separate raw caller view now share the compact
and big-endian primitive-key decoder. GCC/Clang sanitizers compare 906 unchanged
physical paths: 576 selected boundaries at `30B5F5`, before any actual leaf
cache/variable/filter/shape field is read, plus 330 retained I/O prefixes.
Each compiler checks 2,964 native comparisons, ordinary/hostile FP and readonly
observer modes, complete ABI/live80/banks/events and 7,680 original pre-I/O
packets across 44 sites. All sixteen caller alignments, DF and incoming
0/1/4/8 live values are included. The independent read-only review confirms the
interleaved pre-fourth-byte combination and final shift/add/LEA flag ownership.
These boundaries are partial parent paths, not completed ray queries or child
results. The raw integer/scale/key union reaches 231/896 original parent sites.

Receipts are `build/test-results/native-mopp-ray-primitive-key-shared-20261109/`
and `build/test-results/native-mopp-ray-frames-shared-official-20261109/`.
The public controller separately passes its 16/16 mutation suite and actual
four-profile replay with all 89,795,481 before bytes; these remain value-entry
proofs. No production ray prefix or filter/shape result was introduced.

The observer-only extension derives dense logical occupancy from actual physical
FTW/TOP instead of guessing input depth plus a supplied delta. Final and partial
80-bit values come from the actual hardware register file. No original
instruction, numeric result or dependency outcome is patched or supplied.
An independent read-only source review confirms lazy source access order,
GP/XMM publication and staged pre-write/post-observer FP retirement. Its domain
is dense masked source FP, at most seven incoming live values for scaling,
committed ordinary banks and read-only state observers. Physical page/MMIO fault
atomicity, holes/MMX/unmasked source exceptions and production admission remain
open.

## Native plane and branch stages

The value traversal and raw caller view now share all nineteen native split-plane
forms (axis, projected, thin-axis and long-axis), quantization and the ten signed
endpoint projections. The raw view preserves the source's lazy private scratch,
including the distinct `1B` write/read/spill order, variable-length branch bytes,
actual program advancement, and scalar bounds/caller aliases. The segment-side
controller also shares native FP decisions while the raw view publishes full
FSTSW/TEST state, including the source's deferred low-plane flag publication.

GCC/Clang sanitizers pass 1,859 physical plane paths (494 selected numerical
boundaries plus 1,365 read/store/observer prefixes), with 4,706 native comparisons
per compiler, 238 original sites and 31,732 complete original pre-I/O packets.
The branch stage passes 2,001 physical paths / 4,990 comparisons per compiler /
260 sites / 38,041 full packets, stopping explicitly before the next opcode or
crossing traversal. Inputs include all sixteen caller alignments, DF, live
0/1/4/6 values, PC/RC/DAZ/FTZ, signed zero, denormals, infinities and quiet/signaling
NaNs. Ordinary and hostile providers/Before/observers compare exactly. These are
partial parent continuations, not completed ray queries or supplied child results.

The integrated registered plane/branch sanitizer/release suite passes 4/4 at
`build/test-results/native-mopp-ray-plane-branch-official-20261109/`; all its
owned sources/dependency copies/reference executables/banks/builds are removed.
The public shared controller passes 20/20 including both new compiled numerical
mutants. Actual four-profile replay after plane extraction passes all 89,795,481
before bytes at `build/test-results/native-mopp-ray-plane-actual-20261109/`;
the later branch extraction replay also passes GCC/Clang at
`build/test-results/native-mopp-ray-branch-actual-20261109/report.json`.
An independent source review confirms special-form ordering, FP staging and live
retained publication. Exact current-source proof/cleanup pins are in
`build/test-results/native-mopp-ray-plane-branch-closure-20261109/report.json`;
the combined raw integer/scale/key/plane/branch union reaches 474/896 sites.
All completed owners are removed and no large artifacts are retained.

The private observer's explicit numerical-stop mode captures hardware FTW/TOP
and all actual live80 at an unbalanced declared continuation. It preserves the
existing neutral-prefix invariant by default; it changes no original instruction,
input result or child outcome. Later sections qualify crossing, genuine recursion,
post-child DATA and slabs. Complete returned leaf services and diagnostics still
require their actual owners.
Neither production parent has a native prefix.

## Native crossing preparation

The public traversal and raw caller now share the native crossing deltas,
side/product comparisons, clipping fraction and source-ordered SSE interpolation.
The raw view preserves actual aligned endpoint loads, child-vector stores,
pre-write FP staging and the decreasing fraction's overwritten scratch lifetime.
It stops before the real recursive-call setup at `30AF16` or `30B06F`.

GCC/Clang sanitizers pass 5,722 unchanged-original paths: 798 entry cases,
4,924 retained read/store/observer prefixes, 737 selected numerical boundaries
and 61 actual alignment faults. Each compiler checks 12,918 native comparisons;
163,078 complete original pre-I/O packets, full ABI/live80/FP, ordered operations
and whole banks match. The raw frame union now reaches 532/896 sites. Receipt:
`build/test-results/native-mopp-ray-crossing-all-batched-20261109/report.json`.
The first wide run exceeded the physical runner's finite prefix-process budget;
its failure remains recorded. The passing run batches the unchanged prefix
inputs in groups of 256 without changing any instruction, outcome or case.

After integration, the public DATA suite passes 22/22, including ten compiled
mutants; the new mutant substitutes the fraction denominator for its numerator.
Both compilers check 2,201 unchanged-original cases / 4,402 native comparisons.
The actual four-profile replay also passes all 89,795,481 captured before bytes.
Receipts are `build/test-results/native-mopp-ray-crossing-data-first-20261109/`
and `build/test-results/native-mopp-ray-crossing-actual-20261109/`.
All completed owners are removed; no new large artifacts are retained.
Registered raw sanitizer/release qualification and real returned-child continuation
are still pending. Both production ray parents remain unbound.

## Genuine recursive calls and post-child DATA

The private native recursion adapter performs the actual caller argument and
return-address stores before handing off its FP owner to a genuine native
empty-geometry child. The child owns its own prologue, reads, classification,
callee saves and RET; no entry tuple or child outcome is supplied/restored.
GCC/Clang sanitizers pass 6,008 physical paths, including 5,704 retained prefixes,
12,624 comparisons per compiler and 246,366 complete pre-I/O packets. Receipt:
`build/test-results/native-mopp-ray-recursion-all-first-20261109/report.json`.
The first harness omitted implicit POP/RET read metadata; that failed receipt
is retained. The passing gate records those actual stack reads in both the
ordinary boundary and failure-prefix observers.

The shared native post-child controller rereads current deltas, original bounds,
cache identity/fraction/code information, variable scale/translation and signed
axis. Its staged spills and source-ordered vector stores preserve the distinct
increasing/decreasing overwritten scratch lifetimes. The raw gate passes 7,231
physical paths, including 6,927 retained prefixes, 15,070 comparisons per compiler,
466 stage sites and 366,925 full pre-I/O packets. Genuine native child returns,
cache shortening, scaling and real parent early returns are included. Receipt:
`build/test-results/native-mopp-ray-resume-all-first-20261109/report.json`.

The public traversal now consumes the same resume core. Its integration suite
passes 24/24, including eleven compiled numerical/ownership mutants, with 2,201
unchanged-original cases / 4,402 comparisons per compiler. Four-profile actual
replay also matches all 89,795,481 before-input bytes. Receipts:
`build/test-results/native-mopp-ray-resume-data-first-20261109/` and
`build/test-results/native-mopp-ray-resume-actual-20261109/`. Completed owners are
removed, with no new large artifacts retained. Raw composed-parent qualification
is active separately; these receipts do not install either production parent.

## Native leaf DATA and real call setup

A shared typed leaf controller and raw view own key-base addition, lazy cache/
filter/collection fields and real argument stores. The pre-call raw gate passes
1,118 original paths: 576 selected first-external-call boundaries plus 542 retained
prefixes, 3,388 comparisons per compiler, 64 stage sites and 15,590 full packets.
It stops before the genuine filter or shape call, supplies no dependency outcome,
and checks full ABI/live80/banks/events under ordinary and hostile FP providers.
Receipt: `build/test-results/native-mopp-ray-leaf-first-20261109/report.json`.
Returned filter/shape/direct-cast/collector service paths remain unqualified.
The combined qualified raw-stage union reaches 696/896 original parent sites;
this is a union of bounded paths, not whole-parent or native-only closure.

## Assembled controller, slabs and returned shape provider

The private assembled native controller passes GCC/Clang sanitizers against
7,231 physical paths, with 6,927 retained prefixes, 15,070 comparisons per compiler
and 366,925 full pre-I/O packets. Its 304 declared paths include 81 genuine parent
returns and 223 next-opcode boundaries. It recursively invokes the actual native
controller; no entry tuple or child result is supplied. Receipt:
`build/test-results/native-mopp-ray-composed-all-first-20261109/report.json`.

The native slab core and raw view cover all six short/24-bit axis forms, lazy
cache/variable fields and both clipping phases. The expanded direct/assembled
GCC/Clang sanitizer gate passes 1,926 physical paths: 462 declared boundaries,
158 real rejects, 90 actual aligned-load faults and 1,374 retained prefixes.
Each compiler checks 9,552 comparisons, 53,739 full packets and all 124 slab
instructions (150 total parent sites). Four compiled defects are rejected:
premature spill retirement, unprotected FP providers, hoisted high-byte access,
and reversed dual-NaN SIMD operand order. The registered sanitizer/release pair
passes 2/2; its owner is removed. Receipts are
`build/test-results/native-mopp-ray-slab-final-20261109/` and
`build/test-results/native-mopp-ray-slab-official-20261109/`.
Public slab reuse separately passes 24/24 numerical/ownership checks and the
four-profile actual replay over all 89,795,481 before-input bytes at
`build/test-results/native-mopp-ray-slab-data-first-20261109/` and
`build/test-results/native-mopp-ray-slab-actual-20261109/`.

All invalid authored opcodes pass 3,124 physical paths through the first real
diagnostic-call setup, including 372 retained prefixes and 11,752 comparisons
per compiler. This supplies no diagnostic result and does not qualify the later
calls or null-write failure. Receipt:
`build/test-results/native-mopp-ray-diagnostic-paths-fixed-20261109/`.

The genuine native `24DEC0` provider passes 328 physical paths: 128 complete
returns, 200 retained prefixes, 912 comparisons per compiler and all 13 sites.
The parent leaf then passes 1,738 paths / 4,628 comparisons per compiler /
44,232 full packets through this actually returned native child and subsequent
direct/collector argument setup. It stops before an unprovided cast and supplies
no child outcome. Receipts are
`build/test-results/native-instanced-ray-child-arguments-20261109/` and
`build/test-results/native-mopp-ray-leaf-return-boundaries-20261109/`.

The first returned-leaf harness exposed a distinction between a hardware stop
before an indirect CALL instruction and a provider stack-write failure after
reading its target. The passing gate declares 20 such cases as before-instruction
boundaries, retaining every read, write, bank and ABI comparison. It does not
claim post-target-read provider write-failure parity. The retained State::push
also advances SP before a failed store, while the raw CPU-atomic view commits
SP after success; that existing policy difference remains explicit. The failed
receipt is retained and no source instruction, target read or outcome is masked.

The bounded raw-stage union now reaches 851/896 original parent sites;
separately qualified provider sites are excluded from that count. This union is
not full-path, native-only or production-parent acceptance. Both production ray
parents remain unbound. All completed owners are removed and no new large
artifacts are retained.

## Genuinely returned collector query

The private collector path now executes the actual `24DEC0` provider, its
authored `46625C` vtable, `24DEF0` forwarder, `24DF20` collector and complete
`16C000` scene query through native admission, sphere, bitmap, BSP and collision
publication, then returns through the MOPP parent epilogue. Only the inverse
child still uses its independently pinned retained World body. No query result,
child return ABI or substitute vtable target is supplied.

`native-mopp-leaf-query-full-prefixes-20261109/report.json` passes all 408 whole
calls and 521 interruptions at the newly connected parent/provider/callback
seam. GCC and Clang sanitizers each check 1,337 comparisons and 256,443 complete
pre-I/O GP/flags/XMM/FP/live80 packets. The original observations contain 153,656
packets. All 62 forwarding/collector instructions execute, with real duplicate,
no-hit, any-hit, replacement and keep returns. Dense incoming x87 counts
0/1/4/6, unrounded caller stacks, all ordered accesses, writes, banks and final
parent ABI remain strict. The ordinary-bank/non-hostile-provider domain and
the existing indirect-CALL before-instruction versus post-target-read failure
distinction remain explicit.

The authenticated historical raw-stage union grows from 851 to **861/896**;
only ten newly returned parent instructions enter this count, excluding all
provider/query descendants. The bounded proof-input hashes and remaining 35
sites are recorded in `native-mopp-ray-returned-collector-union-20261109/`.
Sixteen diagnostic continuation sites, one numerical continuation branch, four
returned-filter sites and fourteen direct-cast/returned-hit sites remain.
This is a site union, not whole-function or production-parent qualification.

The actual instanced provider installs `46625C+0x20 = 1CB560` (INT3 padding), while
`46625C+0x24 = 24DEF0` is its genuine collector forwarder. A direct-hit proof must
follow another actual shape owner or qualify the original unsupported path;
replacing that authored direct target would invent behavior. Both production
MOPP parents remain unbound. The completed collector owner was removed and no
large artifact was retained.

## Aligned native vector stores

The private vector-store adapter now validates MOVAPS destination alignment at
the actual source boundary, before observer preimages or provider access. Like
its load counterpart, it keeps execution flags unchanged and records RF only in
the saved exception image. This matters when a returned saved-register alias
changes a destination before an aligned load can validate it.

An unchanged-i386 single-store gate passes 144 physical cases: 120 real alignment
faults, eight completed stores and 16 atomic-provider/completed-observer prefixes.
GCC/Clang sanitizers each check 544 transfers, full live80/FP/ABI and all banks.
The registered sanitizer/release pair passes 2/2; all owned source/dependency/
reference/build products are removed. Receipts:
`build/test-results/native-mopp-ray-vector-store-first-20261109/` and
`build/test-results/native-mopp-ray-vector-store-official-20261109/`.
This closes the ordinary aligned-store boundary, with physical page/MMIO fault
atomicity still separately qualified. No production ray prefix was added.

## Remaining production port

The raw parent adapter must preserve the real aligned 0x194-byte caller frame,
actual variables/bounds/property slots and all alias-driven rereads. Numerical
progress must publish live 80-bit intermediates, status/TOP/C1, XMM and flags at
physical I/O and child boundaries. Single-store conversion/pop must commit only
after an actual successful provider write and before a completed-write observer.
The value API's balanced/depth-zero environment cannot serve as this raw frame.

Genuine recursive, filter, child-shape, direct-cast and diagnostic handoffs
remain required, including returned register/stack aliases, actual backend
returns and descendant faults. The collector composition above returns through
its real scene query; the other external-service corpora still stop at their
declared first unprovided service. No corpus supplies a child result. Malformed programs,
MapPack ownership, wider FP/MMX/exceptional domains, all retained default routes,
actual `30B890` caller composition, full query/runtime native closure and
original frame parity remain open. Neither production `30AA70` copy has a native
prefix yet. No launcher runtime was changed.


## Complete split/recursive query and callback-state repair

The private controller now uses the accepted native inverse and complete native
query subtree. It executes all 19 authored split families in both directions,
recurses through actual child leaves, adopts returned cache fractions, resumes
and returns through the parent. No child results or register packets are supplied.
The full private four-header BSP callback-isolation candidate passes 1,757
original paths (1,064 whole calls and 693 interruptions). Each sanitizer compiler
checks 5,642 comparisons and 2,017,424 complete pre-I/O packets, with 3,192 genuine
queries and 912 hits. The source receipt is
`native-mopp-query-controller-fp-overlay-full-20261109/report.json`.

Hostile providers exposed four existing BSP adapters whose callbacks leaked
host x87/MXCSR changes into subsequent genuine children. A neutral callback
boundary now preserves the opaque host image on normal return and exceptions.
It does not import or publish State software x87/MMX, and genuine child execution
remains outside. This is narrower than wrapping the full adapter in a raw FP
scope, which would conflict with its manual software temporary transport.

Eight existing BSP baseline gates pass on the private candidate. The polygon
gate then passes separately after replacing its stale dispatcher-only hybrid
fixture with complete genuine generated source; its strict body/context pins
are unchanged. The corrected 2D first-store address is independently compared
with actual retained `State::push`: a callback may change SP after the store
address has been captured. Both compilers reject a mutant that retargets that
store, and both reject missing normal-return and exception FP restoration.
The host-image utility compares 2,016 precision/rounding/TOP/tag/payload/return/
throw/nested cases per compiler.

The four exact qualified BSP headers are installed. The overlay helper is
retired. The installed whole-query proof passes both sanitizer compilers with
the same 1,757 original paths / 5,642 comparisons / 2,017,424 complete packets
per compiler. Maintained installed host-state checks pass both compilers and
all three real defect controls; their registered sanitizer/Release pair passes
2/2. Closure: `native-bsp-host-fp-io-closure-20261109/report.json`. Installation provenance and exact hashes are in
`native-bsp-host-fp-io-install-20261109/report.json`. All completed scratch
owners are removed, and no large artifacts are retained. These proofs do not
bind either production MOPP parent.


The missing numerical continuation at `30B01D` now has a directed independent
proof: real recursive empty-geometry children return with shortened fractions
0.75 and approximately 0.9, leaving each of the three axis endpoints beyond its
high plane. Both sanitizer compilers pass six full continuation paths and 900
read/write/observer interruptions, 1,824 comparisons each. The unchanged
original observations contain 48,984 complete packets. No child outcome is
supplied. The exact historical site-union reconstruction now reaches
**862/896**, leaving 16 diagnostic continuation, four returned-filter and
14 direct-cast/returned-hit sites. Receipts:
`native-mopp-numerical-continuation-20261109/report.json` and
`native-mopp-ray-numerical-continuation-union-20261109/report.json`.
