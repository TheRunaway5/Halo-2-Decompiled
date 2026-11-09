# 30E930 convex simplex controller

The complete native replacement is in progress. The generated parent remains
unbound. The private shared direction and candidate stages cover352 disjoint
sites of the1,020 original instructions, rather than a complete parent or
descendant-closure claim.

## Owners and algorithm

The hot World owner is `run_30ae40` in
`src/physics/runtime/physics_world_runtime_generated.cpp`. TU1.5 selects actual
entry `30E930`; its complete body SHA-256 is
`4831de546a5dc35bdbd910bbf393eb7438a992a4b2cb8d7c2718ba1a5fed8113`.
The second complete copy lives in
`src/physics/runtime/physics_contact_runtime_generated.cpp`, with body hash
`d62f815eac431c206770af5897ddbf25782acbc20dfa367805f4bc2d7771cc2f`.
World can observe actual TU1.5 or canonical PCs; Contact retains canonical
observation labels. The numerical direction blocks are identical after that
explicit observation-label translation.

The complete controller retrieves the two cached feature sets through virtual
slot34, transforms the second simplex into the first shape's frame, and loops
over vertex/edge/triangle/four-point reductions. It doubles the convergence
epsilon at each support iteration, alternates first-shape support priority,
tests signed projected improvement against the source x87 threshold, and
tries second-shape support in its inverse-transformed direction. Degenerate
distance calls the general simplex reducer; overlap keeps/caps the current
simplex. Its final path performs the source RSQRTSS refinement, publishes the
contact plane, and rewrites the encoded feature cache. Caller slots, counts,
work rows and actual child continuations are live inputs at their original
boundaries.

The exact World selected resolver is
`20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2`;
its complete dispatcher is
`c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c`.
Actual immutable instruction tables are `30F6E0`/`30F70C`; the sign mask is
`461960`, cubic threshold is `41D658`, and face-selection maps are
`41D758`/`41D760`/`41D762`.

| Counts first/second | Actual reduction entry |
| --- | --- |
| 1/1 | 30F253 point direction |
| 1/2 | 30F066 second edge |
| 1/3 | 30EB76 triangle |
| 1/4 | 30EB31 face selection |
| 2/1 | 30F175 first edge |
| 2/2 | 30EA8E closest segments |
| 2/3 | 30EA3E paired reduction |
| 3/1 | 30ECD3 triangle |
| 3/2 | 30EA5C paired reduction |
| 4/1 | 30ECA2 face selection |

Other encoded cases select the source degeneracy path. A future native
classifier must preserve the complete unsigned `((first<<3)|second)-9`
calculation, including wrapped or malformed counts.

Five fixed child owners are `30D3F0` (triangle), `30D5D0` (face selection),
`30D830` (paired simplex reduction), `30DE60` (general simplex distance), and
`312620` (closest segments). Two virtual34 feature and three virtual30 support
call sites also remain part of the complete raw child contract. Existing
typed helpers and handoffs are reusable DATA/FP seams; their presence does
not establish complete raw GP/XMM/caller-frame postimages.

## Shared direction continuation

`include/halo2/native_tu15_simplex_direction_kernel.hpp` owns both edge/vertex
orientations, endpoint compaction, the paired payload copy, and point
direction. `native_tu15_simplex_direction_abi.hpp` transports their complete
normal retained ABI through the stable raw x87 scope. Both projection signs
are source sign-bit decisions, including signed zero and NaNs. Every
four-lane operation preserves the original operand order and upper lanes.
Counts and point rows are freshly read after prior stores; observer feedback
and aliases remain observable.

The old guarded semantic solver has a narrower provided-byte domain. In
particular, an endpoint promotion reads private `work+70` and copies it to
`work+60`. Those bytes are deliberately unavailable in that API until a real
owner defines them. Its endpoint refusal is preserved; no zero fill,
captured afterimage or invented payload widens admission. Its admitted
interior arithmetic now reuses the shared kernel.

The new continuation's GCC ASan/UBSan and independent Clang ASan/UBSan gates
each pass720 complete canonical transactions and920 retained read/write/
after-write-observer prefixes, with all141 sites. The canonical comparison
includes all GP, full flags, all XMM, source CW/SW/MXCSR, logical80-bit values,
complete banks, ordered reads and every store.192 independently declared
mid-owner calls also agree with unchanged authenticated i386 instructions on
all141 sites and complete normal ABI/banks/store operands. These are declared
continuations, rather than captured complete-parent calls.

Three canonical semantic mutants reject the wrong cross orientation, omitted
paired payload and wrong live count. A fourth physical mutant restores the
old reversed positive MUL operands and is rejected by an IEEE counterexample.
At `direction-0-34`/`30F175`, that reversal changes final XMM0 lane2 from
`FFC23456` to `7FC12345`, and changes XMM1 lane2. The original positive MUL is
at `30F1FD`. Exact input words, controls, GP/live80 inputs and both full XMM
banks are retained in the bounded operand-order receipt; no NaN or flag bits
are masked.

The current runtime represents each16-byte MOVAPS store with two ordered
8-byte provider/observer events. The independent physical comparison retains
every actual16-byte operand and explicitly projects it to those two events
using the authenticated instruction, with no equal-value filtering. That
protocol equivalence does not establish physical MOVAPS page-fault atomicity.
Retained RMW exception publication likewise remains distinct from physical
instruction atomicity. Full intermediate physical GP/undefined flags,
unmasked/MMX/sparse x87 domains, complete parent faults, and arbitrary
GP-mutating providers are not qualified by this stage.

## Shared candidate continuation

`native_tu15_simplex_candidate_kernel.hpp` computes the three repeated
post-support checks at `30EEB1`, `30F315`, and `30F491`. Its raw adapter retains
the actual resumed work/count/direction inputs. Direct-stage fixtures declare
those algorithmic inputs; they do not claim a shape support call or provide a
substitute child return. Candidate/direction vectors and count remain cached
at source boundaries, while each rounded score and tolerance is reread around
the live native x87 operations. The primary AH41 test rejects unordered,
equal, and less. The extra AH5 parity test rejects ordered less and accepts
unordered. First-shape growth stores the cached count plus one.

Both GCC and Clang sanitizer gates pass288 canonical normal transactions,
788 retained read/write/observer prefixes, and288 independent unchanged-i386
calls covering all211 candidate sites. The original CPU also supplies2,503
complete pre-access GP/full-flags/XMM/CW/SW/MXCSR/live80 packets. All packets,
normal ABI, banks and every scalar store agree. Three semantic mutants reject
the wrong primary relation, skipped third vertex, and reversed projection.
The registered sanitizer CMake test passes1/1; the module is
`cmake/NativeTu15SimplexCandidate.cmake`, target
`native_simplex_candidate_tests`, test `original_tu15_native_simplex_candidate`,
report `native-simplex-candidate.json`.

There is a retained-reference distinction. The old translated `Flags.shift`
clears AF before the candidate load, whereas the original immediate-SHL sets
AF in the observed context (`602` versus `612` at `30EEC0`). The failed
unmodified-model prefix receipt remains. Normal returns still compare the
unmodified pinned body; retained fault prefixes use a separately hashed
reference with only that immediate-SHL flag primitive executed natively.
Unchanged-i386 pre-access packets independently qualify the complete flag
word, and three original AF1 packets are retained for the three blocks. No
flag or NaN bits are masked. World's36 active-MMX guard sites remain in the
compiled reference; numerical comparison with the authenticated Contact copy
accounts explicitly for those guards under the native no-MMX admission.

The existing semantic solver reuses this core only for its prior count1
first-support trial. Its `current!=1` refusal remains at the same boundary
after primary comparison, and its second-shape/private-row refusals remain.
The genuine retained BEFORE GCC/Clang replay passes all eight rounding
profiles and eight meaningful mutants after reuse. The raw211-site stage is
private and has no production entry binding or reference control.

Its bounded receipts live under
`build/test-results/native-simplex-candidate-20261109/`, including
`official-cmake/evidence/native-simplex-candidate.json`,
`final-stable-clang-sanitizers.json`, and
`shared-selected-regression/report.json`. Original physical inputs are
constructed independently; no original output is fed to the native path.

## Registered evidence

The private module is `cmake/NativeTu15SimplexDirection.cmake`, target
`native_simplex_direction_tests`, test
`original_tu15_native_simplex_direction`, report
`native-simplex-direction.json`. It defines no production binding or
reference control. The current kernel, ABI adapter, fixture and validator
are the reusable implementation seam for the complete parent.

Bounded receipts live under
`build/test-results/native-simplex-direction-20261109/`. Final compiler and
operand-order reports are retained there. The registered sanitizer CMake gate
passes1/1; its retained report is
`official-cmake/evidence/native-simplex-direction.json`. The selected semantic
solver's genuine retained BEFORE regression passes GCC/Clang and four
rounding controls: eight profiles each agree on121,675,707 bank bytes,
101 ordered reads,30 stores, result and FP. Eight semantic mutants are
rejected. Its source path executes789 original instructions, including370
distinct parent sites; those figures describe that independent input replay,
not additional whole-parent ABI coverage. The unchanged required reference
archive is SHA-256
`bb913a242ddc2e13f38269b50cba7c4f340a821beb6486fc87eb1dfc153025e9`.
The receipt is `shared-selected-regression-r2/report.json`.
All
compiler/oracle products and duplicate raw banks use an enclosing owned test
scratch lifecycle. The original reference archive is preserved.

## Shared finalization continuation

The shared finalization kernel and retained adapter cover78 sites from
`30F5D7` through the real `RET20` at `30F6DD`. They preserve original and
fresh axis reads, complete upper XMM lanes, the exact RSQRT/Newton grouping,
constant scratch reloads, distance reread, partial count bytes, signed live
key-copy bounds and interleaved saved-register pops. Together with direction
and candidate stages,430 disjoint parent sites now have private raw seams.

GCC and Clang sanitizer checks pass112 canonical returns,1,302 unique retained
read/write/observer/service prefixes, and128 unchanged-i386 calls covering
all78 sites. The physical proof compares5,009 complete pre-I/O GP/full-flags/
XMM/CW/SW/MXCSR/live80 packets, normal ABI, full banks and every store operand.
Four semantic mutants are rejected. Model's source review is clean within
this declared domain. The registered sanitizer CMake gate passes1/1; target
`native_simplex_finalize_tests`, test `original_tu15_native_simplex_finalize`,
report `native-simplex-finalize.json`. The final closure receipt is
`build/test-results/native-simplex-finalize-closure-20261109/report.json`.

The raw adapter calls the real reciprocal-square-root service at `30F612`.
`physics_live_world_tu15.cpp` records/replays that explicit host-CPU estimate;
the proof's independent provider computes its estimate from the input and
checks call count/input, but does not exercise the production input journal.
The existing semantic facade retains its prior direct-host policy and empty
incoming x87 domain. Xbox estimate bits remain a separate hardware-policy
question. Neither provider is replaced with a supplied contact outcome.

The existing semantic solver now reuses all three DATA cores within its
unchanged provided-row/count domain. Its genuine retained BEFORE replay
passes eight GCC/Clang rounding profiles and eight mutants. A separate
genuine1479 input regression with declared ordinary overlap profiles also
passes both compilers and eight mutants, preserving all89,795,481 retained
bytes. Its228 executed parent sites are input coverage, not whole raw ABI
qualification. That regression's inherited metadata stub needed only the
new optional implicit-stack argument; its initial arity failure is retained.

The complete1,020-site controller, wider genuine children, full raw admission,
actual default routing, physical exception domains and a new whole-engine
accounting checkpoint remain open. The four unavailable private rows remain
guarded. Physical MOVAPS/RMW/PUSH page-fault atomicity is distinct from the
retained provider protocol; production estimate journal exercise is also open.
R14's frozen source predates candidate/finalization semantic reuse.

## Initial feature and transform continuation

The initialization DATA core and private retained adapter now cover67 sites
from `30E930` through `30E9FB`. They retain both real virtual+34 feature calls,
signed-byte count acquisition, output/count/PUSH grouping, late frame formal
reads, epsilon publication before owner lookup, child-resumed work/count/cache
registers, the actual SF-after-DEC test and streaming second-shape transforms.
The native stage does not synthesize missing rows or widen the old semantic
solver's provided-byte admission.

GCC's registered sanitizer CMake gate and independent Clang sanitizer proof
pass96 full canonical transactions,922 retained read/write/observer/child-entry
prefixes, three meaningful mutants and128 unchanged-i386 calls. All67 parent
sites and4,988 complete parent pre-I/O/child-entry GP/full-flags/XMM/FP/live80
packets agree, together with complete banks and ordered stores. The physical
fixtures invoke256 genuine `1F6690` feature children through the unchanged
immutable vtable `4620D0+34`. Saved-child-DI/SI aliases deliberately select
new parent count/work rows; both original and native continuations adopt them.
Model's normal/read/callee review found no actionable issue.

The23-site feature child remains literal in the initial initialization
receipt and is counted separately. A later registered composition also runs
that real child through its qualified native ABI: the same96 returns,922
retained prefixes,128 unchanged-i386 executions and4,988 parent packets agree
with zero parent or child literal callbacks. The actual `1F6690` body is now
default-native in both generated owners; see `1F6690_polygon_features.md`.
This is neither a supplied child outcome nor a claim of complete parent or
native descendant closure. CALL target-read and retained PUSH write failures
retain the current State protocol, separate from physical CPU atomicity.
The module is `cmake/NativeTu15SimplexInitialize.cmake`, target
`native_simplex_initialize_tests`, test `original_tu15_native_simplex_initialize`,
report `native-simplex-initialize.json`. Bounded evidence is under
`build/test-results/native-simplex-initialize-20261109/`.

The four disjoint private parent stages now cover497 of1,020 sites. The
remaining523-site controller and actual descendant/default routing remain
active work. No new whole original function is bound by these stage receipts.
The separate feature-owner receipt binds one whole23-site function. Its final
four-test registered cohort, including this later initialization composition,
is `build/test-results/native-polygon-features-20261109/official-cmake-final-r2/`.

## Direction projection and threshold continuation

The private projection kernel and raw frame cover44 more disjoint sites from
`30EDD5` through `30EE7E`. They retain Y+X then Z length grouping and full
upper lanes, the fresh axis before its aliasable length store, second minus
cached first, the exact MULPS operand order, three scalar projection rereads,
epsilon/constant/length order, full FCOMPP/FNSTSW/AH1 behavior, late iteration
and tolerance reads, parity flags and staged FSTP retirement. Next owners are
the exact `30F581`, `30EE84` and `30F274` boundaries. Model's full source
review found no actionable issue within the declared domain.

GCC and Clang sanitizer proofs pass128 source-backed normal transactions,
460 retained read/write/observer prefixes and three semantic mutants. The
registered sanitizer CMake gate passes1/1. Independently,160 unchanged-i386
executions cover all44 sites and all three exits, comparing2,168 complete
pre-I/O GP/full-flags/XMM/CW/SW/MXCSR/live80 packets plus full banks and every
store. Original FP opcode bytes and the live-x87 stack delta at each access
are authenticated explicitly. No original code or result is patched or
supplied to the native path.

The unmodified translated reference has a concrete C1 gap: its status is
`1120` where original and native FSTP retire `1320`. That failed comparison
remains a rerun negative control under both compilers. Pure no-feedback
physical inputs independently demonstrate original C1=1, and their bounded
inputs/observations remain in the receipt. The stopped-prefix reference
therefore identifies and hashes ten native x87 primitives (three FLD, five
FMUL, FCOMPP and FSTP) inside the otherwise pinned GP/SSE/control body. This
is an explicit reference distinction, with no ignored FP or flag bits.

FSTP uses the existing shared staged store: providers see the pre-instruction
FP state, successful writes commit the token, and completed observers see
retired FP. This differs from the old modeled observer-before-pop protocol;
that old exceptional behavior is not claimed equivalent. Physical page-fault
atomicity, unmasked/sparse/MMX state, capacity overflow and arbitrary GP or
policy mutation remain outside this stage's qualification.

The module is `cmake/NativeTu15SimplexProjection.cmake`, target
`native_simplex_projection_tests`, test `original_tu15_native_simplex_projection`,
report `native-simplex-projection.json`. Its bounded closure is
`build/test-results/native-simplex-projection-closure-20261109/report.json`.
The five private parent stages now cover541 of1,020 sites;479 sites and the
complete parent/default route remain open. R15's immutable source manifest is
`fec369ef60b849f434b1cc264bf32006f348b935d8c64815f87f957ff7784bff`.
It includes this private stage and the default feature child, without claiming
completion of the parent. Later iteration/reduction work is outside that
snapshot until separately qualified.

## Iteration and exact reduction dispatch

The private iteration kernel covers17 further sites (`30EA00..30EA37`),
preserving live first/second counts, epsilon FLD/doubling, cached counter
acquisition, packed OR and wrapped key, staged FSTP, and counter/progress
publication. It reads the original25-byte class table and11 target words
and maps all11 real reduction kinds; malformed counts retain the original
unsigned range test. Model reviewed normal/read/callee ordering cleanly.

GCC and Clang sanitizer gates each pass128 source-backed transactions,172
retained prefixes,160 unchanged-i386 executions and1,232 complete pre-I/O
ABI/FP/live80 packets across all17 sites and11 kinds. Three semantic mutants
are rejected. The official registered sanitizer CMake gate passes1/1.
No original instruction or reduction outcome is replaced in the hardware oracle.

The module is `cmake/NativeTu15SimplexIteration.cmake`, target
`native_simplex_iteration_tests`, test `original_tu15_native_simplex_iteration`,
report `native-simplex-iteration.json`. Its bounded closure is
`build/test-results/native-simplex-iteration-closure-20261109/report.json`;
all generated scratch is removed with no retained large artifacts.
The six disjoint private stages now cover558 of1,020 parent sites. The
remaining462 sites, actual reduction-child ABI and complete parent/default
route remain active work. Physical atomicity, sparse/MMX/unmasked FP and
arbitrary GP or policy mutation remain outside this stage.
