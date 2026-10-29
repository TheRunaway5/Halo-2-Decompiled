# Native TU1.5 vector math and perception-distance publication

`src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp` implements complete
`30BD0` normalization and the authenticated `3EA20` distance followed by its
actual `266EF9` caller publication. The public header is
`include/halo2/native_campaign_ai_vector_math_tu15.hpp`; the CMake module is
`cmake/CampaignNativeAiVectorMath.cmake`. No numeric router or full perception/
navigation/campaign acceptance is added.

Both algorithms use actual host x87/SSE arithmetic under explicit source
CW/status/MXCSR. The typed algorithms contain no CPU interpreter, virtual FP
stack, private guest stack, captured reference return or guessed result. The
hardware norm is recovered as `sqrt((x*x + y*y) + z*z)` with original operand
priority and precision boundaries. Dead arithmetic copies are eliminated;
one exact native extended scalar is materialized at its typed boundary. It is
not prematurely rounded to float/double. Real hardware temporaries span lazy
source reads, preserving FP effects and source-failure order. Unrelated host
x87 values/environment and MXCSR are restored on every exit.

Normalization reads Z/Y/X for the x87 norm, spills its real single magnitude,
then compares the retained extended absolute norm against actual epsilon
`46A0EC`. The source refusal is strict: equality still normalizes. The SSE
reciprocal phase reads actual one `46A0D0`, then vectorX before division. It
publishes X before reading/scaling Y and Z, then publishes Y and Z. Source
multiplication operand priority, NaNs, signed zeros, DAZ/FTZ and partial stores
are retained. The returned REAL bits are zero on refusal and the original
rounded magnitude otherwise. Both literals already have exact immutable native
world owners; no new mount or initialization was added.

The primary caller proves `266EF4 CALL3EA20`, immediate `266EF9 FSTP[EBP+28]`,
then `266EFC`. `native_campaign_ai_publish_perception_distance_tu15` binds only
that composed profile. Its [[nodiscard]] result distinguishes committed output
from a failed publication: a successful store carries written REAL bits; a
rejected store carries the real pending80-bit distance and its actual source
exception. `rethrow_if_failed()` retains that quantity/cause in
`NativeCampaignAiPendingDistanceErrorTu15`. An observer rejecting a completed
store leaves it committed and carries no pending value.

The adapter tentatively computes the float spill because its storage interface
can reject a store. If rejected, only that speculative native FSTP is undone;
the source's true pre-publication FP status/TOP and pending value are retained.
No successful spill is invented and no owed distance is silently discarded.
This boundary is checked against three actual original unmapped-destination
FSTP faults, including NaN and overflow, in addition to before/after-store
prefixes. The original does not pop its pending distance or commit the float
spill/overflow state on these memory faults. Six independent pending quantities
are retained in `analysis/campaign-native-ai-vector-math-20261028/pending-boundaries.json`.

`tools/oracle_native_campaign_ai_vector_math_tu15.py` executes unchanged physical
i386 code from authenticated English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Distance calls use their actual `266EF9` return continuation and stop at
`266EFC`; no replacement store instruction or dependency outcome is supplied.
The original caller store's bytes and operand are authenticated. The observer
records actual pending extended values, complete FP status, persistent reads
and committed stores. Private stack and dead GPR/SIMD outputs are excluded.

The required fixture is
`tests/fixtures/native_campaign_ai_vector_math_tu15.json.gz`. Its 421 cases
include 362 completed calls, 56 source/store/observer prefixes and three actual
unmapped-store faults, exercising all58 original sites:19 distance,38 normalize
and the actual caller publication. They execute10,553 unchanged instructions,
2,639 persistent reads and646 stores. An independent original repeat produced
byte-identical evidence. Cases cover CW rounding/precision, SIMD DAZ/FTZ,
exceptional values, paired NaNs, epsilon equality, source/output aliases,
unaligned inputs, zero/large/small vectors and0..3 occupied original x87 inputs.

GCC normal and ASan/UBSan each passed1/1 CTest; Clang passed6/6. Five actual
mutations were rejected: premature single rounding, discarded pending value,
omitted speculative-spill rollback, inclusive epsilon and reciprocal division
before the sourceX read. The latter control uses a clear initial SIMD precision
flag and a real source-read prefix to distinguish arithmetic order. Tests check
retained bytes, return bits, ordered reads/stores/sites, complete FP, exact
pending80-bit values, their typed exception/cause and host FP preservation.

`analysis/campaign-native-ai-vector-math-20261028/proof.json` retains hashes,
commands, fixture provenance and verified cleanup. All original ELF/observer,
compiler products, duplicate references and raw banks were removed; only the
required compact fixture and bounded receipts remain. Earlier query, geometry,
character, filter, workspace and count/fraction proofs were not modified.

The scope requires masked FP exceptions and ordinary callers with room for five
actual x87 temporaries; original occupied depths0..3 are tested. Device adapters
that partially commit rejected scalar stores or clobber active arithmetic FP
are outside the retained-memory contract. Standalone arbitrary `3EA20` numeric
entry/return handling, full `266D30` perception, actual graph search/path update,
`1C9770` selection and full generic native campaign runtime remain open.

The current normalization implementation also exposes
`native_campaign_ai_normalize_vector_value_tu15`, a typed three-REAL private
intermediate for composed native algorithms. One shared Input/Sink algorithm
owns both retained-vector and private-value arithmetic; no guest stack address
is created. The real epsilon/one constants remain lazy source reads.
`analysis/campaign-native-ai-vector-value-20261028/proof.json` supersedes the
initial core/header pins: GCC and ASan/UBSan1/1 and Clang6/6 rerun every421
original case, all five mutation controls and191 completed original30BD0
cases through the value entry. Magnitude/vector bits, FP and constant read
order match; private values leave retained storage unchanged. The original
fixture and all distance pending-publication semantics remain unchanged.
Owned compiler/build scratch was removed. This extension supplies no jump-arc
or path-query outcome; their actual composition remains open.
