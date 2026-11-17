# Private native sphere ray cast

Actual TU1.5 owner `2E8DD0` is the authored sphere interface at
`4613A0 + 20`. The original image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete body has 147 sites and no children. No production prefix,
public API or launcher runtime is changed.

`native_tu15_sphere_ray_cast_kernel.hpp` owns the approach, recentering,
discriminant and root decisions. Its raw view uses native SSE/x87 operations,
lazy current DATA reads, staged numerical stores and physical caller ABI
transport. The view preserves the late radius/end/origin reads after aliased
result stores and distinguishes normal failure cleanup from live numerical
interruption states. It does not execute an instruction interpreter.

The both-compiler sanitizer proof passes 833 original paths: 320 complete
returns and 513 read/write/observer interruptions. Each compiler compares
2,306 native runs / 65,024 complete pre-I/O packets, all ordered reads/stores,
result banks and final ABI. All 147 original sites execute. Cases cover
precision/rounding/TOP, dense live depths through five, IEEE edges, unrounded
incoming SP and result/ray/shape/private-local aliases. Hostile callbacks
return and throw with altered host FP state. Both compilers reject four real
compiled defects: accepting tangency, omitting the late radius read, caching
the ray endpoint and widening the Boolean store. Reference packets are unchanged.

Receipt: `native-sphere-ray-full-mutants-20261110/report.json`.
The first expanded fixture overlapped saved EBP at unrounded SP and escaped
the declared body; `native-sphere-ray-full-20261109/report.json` remains failed.
The corrected separate alias-bounds proof passes before the final mutant run.
All completed owned builds/fixtures are removed; no large artifacts are retained.

Independent review confirms the declared masked/dense domain and the maximum
three live x87 temporaries. An eventual default must admit at most five incoming
live values. Unmasked exceptions, sparse/MMX software states, partial vector
provider writes, other images, production routing and complete native MOPP
parent closure remain unqualified.

The actual `30AA70` parent now also executes a genuine authored list collection
(`461F98`, methods `306220`/`306230`), optional group filter and this sphere.
The smoke passes 12 original returns / 3,932 complete native packets / 44 real
sphere entries. Full composed read/write/observer qualification passes both sanitizer compilers:
1,673 paths (320 complete returns + 1,353 interruptions), 3,986 comparisons /
220,502 complete packets and 2,260 real sphere entries per compiler. Receipt:
`native-mopp-sphere-query-call-prefix-fixed-20261110/report.json`. Its dependency
closure hashes remain unchanged, and the complete owner is removed.
Its initial indirect-CALL prefix fixture incorrectly treated a before-instruction
stop as a post-target-read return-push failure. That failed report is retained;
the corrected proof represents the exact physical boundary without claiming
post-target provider-write failure coverage. No child response is supplied.


Both exact registered CMake modules now pass their four sanitizer/Release
checks from a frozen disposable source tree. Receipt:
`native-sphere-components-official-r2-20261110/report.json`. The earlier outer
metadata wrapper failed before CMake because its retention call omitted the
root argument; that failed receipt remains unchanged. The corrected run
retains bounded source hashes/reports, and its complete owner is removed.

A read-only source census identifies one literal sphere body, in Primary
`physics_live_support_tu15_generated.cpp`; AI has no second local sphere body.
Its exact canonical body SHA-256 is
`455f182e913e774969e2a3bd348c5e8f93e048ada793c1bfc254a8e3c7c4dc8e`.
Private raw/prepared source prefixes, an image/FP/live-cursor admission bridge
and explicit own-reference/unrelated-control checks are under qualification.
The actual canonical CPP and production generator are unchanged.


The private actual-source smoke passes both sanitizer compilers in raw and
prepared contexts. Default and unrelated control each execute24 native calls
with zero retained body/statement events. Own-reference executes24 exact
retained bodies /2,110 statements. Eight whole-body/context/include mutations
are rejected. Receipt: native-sphere-source-literal-accounting-fixed-smoke-20261110/report.json.
The preceding missing-owner mutation fixture and prepared-statement accounting
failures remain preserved, with both owners removed. Full frozen source and
admission checks remain pending. A borrowed refusal test incorrectly treated
virtual DF as unsupported for this non-string owner; the original numerical
corpus already exercises both DF values. That test now rejects unknown image99
instead, preserving the unchanged bridge and its declared native behavior.


The corrected full frozen source/admission proof passes both sanitizer
compilers across all833 paths:1,666 actual raw/prepared native calls per
control, zero retained events, and1,666 positive retained-reference calls /
116,954 statements. Each compiler passes22 untouched admission refusals plus
the forced-reference guard. Positive references retain their known old-source
ABI differences (GCC24 /Clang12); they are compared to their exact preserved
source bodies, rather than relabeled as original-hardware parity. Receipt:
`native-sphere-source-full-guard-fixture-fixed-20261110/report.json`. The whole
frozen owner is removed. Expanded empty-service and compiled binding-defect
checks are running before a production/source-generation installation.


Generation drift is now fully explained and restricted to two exact bodies:
the existing455f... body and fresh6e4eb51a... body. Fresh emission clears C1
at its explicit FP sites and uses the exact native_single_spill helper
(e7000a01...). An authentication-only projection must reproduce the original
455f... body byte-for-byte; neither retained continuation is rewritten. The
helper itself is fully pinned. Independent review passes.

All four current/fresh raw/prepared source contexts pass the both-compiler
smoke; the complete four-context source and compiled-defect proof is running.
Full generation passes six fresh context mutations and four complete Primary
translation-unit syntax checks. Full current/fresh units remain historically
different and are not claimed byte-identical. Receipt:
native-sphere-generation-pinned-delta-20261110/report.json. Earlier generation
authentication failures and bounded exact diffs remain preserved, with all
completed owners removed. No sphere production prefix is installed yet.


The installed family scope is closed. Exactly one Primary default and additive
source-generator/catalog hooks are installed; all2,025 other complete routines
and the full resolver/dispatcher are preserved by exact reverse reconstruction.
The registered installed seven-test cohort passes, independent installed Clang
passes all four source contexts and three binding defects, and the catalog
regression suite passes23/23 after exact include-position restoration was
added to its strict reconstruction. The earlier catalog failure stays failed.
Closure: native-sphere-ray-cast-closure-20261110/report.json:16 passing receipts,
67 recorded owned paths verified removed, no retained large artifacts. This
default is newer than R21 and has not been delivered to the launcher.
