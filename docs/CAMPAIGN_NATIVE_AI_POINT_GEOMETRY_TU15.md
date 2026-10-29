# Campaign point geometry: bounded TU1.5 native proof

The later typed value-projection seam is qualified together with `27B100`.
Current-source pins and the rerun of all 778 original geometry cases are in
`analysis/campaign-native-ai-path-query-start-20261028/geometry-regression.json`.
The original proof below remains evidence for its recorded source cohort.

The complete native semantic point geometry prerequisite is implemented in
`src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp`. It contains no
virtual CPU, instruction interpreter, private guest stack or original-code
execution. This does **not** close the Johnson `1C9F10` navigation update.

| Original owner | Native operation | Original sites |
|---|---|---:|
| `219030` | Authored signed-axis/variant point permutation | 17 |
| `145070` | Scaled affine float3 transform, exact scalar SSE association | 49 |
| `218780` | Resolve object/node frame and transform; actual invalid-frame copy | 84 |
| `218B20` | Project framed point, including duplicate invalid-frame copy | 29 |
| `218D00` | Distance squared, with its two source reduction associations | 34 |

Public typed APIs are declared in
`include/halo2/native_campaign_ai_point_geometry_tu15.hpp`. The CMake module is
`cmake/CampaignNativeAiPointGeometry.cmake`. These APIs are internal native
dependencies; no arbitrary numeric-call register contract or live router is
claimed for these five entries.

The transform reads scale and the retained one constant before the source XYZ.
Non-unit scale rereads the matrix scale for Y and Z. Each output is published
before subsequent matrix rows are read, preserving source/matrix/destination
aliasing. All arithmetic uses actual scalar SSE operations, with original
operand priority and rounding boundaries; no compiler reassociation or FMA is
used. The scoped SIMD environment owns actual MXCSR and restores the host
environment on success and source/observer exceptions. x87 CW, status and live
values are untouched. Masked SIMD exceptions are covered; unmasked traps require
an explicit platform owner and are rejected.

Frame IDs and node offsets remain signed where the original is signed. Node
byte-count division retains the original unsigned reciprocal multiply even
when the retained signed16 byte count is negative. Missing objects or invalid
nodes copy real source XYZ and return false. Frame NONE copies XYZ and returns
true. The framed-point projector repeats the source-copy reads/stores after a
failed frame resolution. None of these returns substitutes for pathability.

Equal frame IDs bypass frame validation and compute `(dx² + dz²) + dy²` from
local coordinates. Differing IDs project both points first and compute
`(dz² + dy²) + dx²`. The different rounding boundary is observable and tested.
Original private coordinate temporaries are ordinary typed local arrays; they
are not materialized as guest stack storage or marshaled as dead CPU outputs.

The existing immutable owner
`src/campaign/native_tu15_campaign_geometry_projection_data.inc:2` supplies the
36 bytes at `44ACB0`; its six signed16 triples are byte-equal to the original
`.rdata` section (flags 6). `native_tu15_final_startup_storage_data.cpp:124`
already includes it. No new mount/initialization was added. Kernels retain
actual SourceMemory reads of those selectors, object/frame pointers and scalar
constants. See `analysis/campaign-native-ai-point-geometry-20261028/storage.json`.

The original English TU1.5 image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
`tools/oracle_native_campaign_ai_point_geometry_tu15.py` builds an unchanged
physical i386 original reference in owned scratch; it neither patches an
instruction nor supplies a dependency return. Only declared pre-entry data,
GPRs and FP inputs are installed. The observer authenticates original sites
and records persistent memory operands, stores, REAL results and actual FP.
Private argument/temporary stack is excluded. Fault prefixes stop immediately
before an actual source read/store or after an actual completed publication;
they never resume with a supplied result.

The retained required fixture is
`tests/fixtures/native_campaign_ai_point_geometry_tu15.json.gz`, 160,299 bytes,
SHA-256 `5834b622b13670f749dd9eac33c505bd3c35c621e166ad881d3f3cd2c649f634`.
An independent physical repeat produced identical bytes. Its 778 cases comprise
406 whole calls and 372 read/store/observer prefixes, covering all 213 original
sites, 72,475 unchanged original instructions, 18,879 persistent reads and 1,107
stores. Cases include every axis permutation, frame NONE/missing object,
negative/equal/out-of-range nodes, signed offset/count edges, output aliases,
all four SIMD rounding modes and DAZ/FTZ combinations, exceptional values,
paired NaNs and the distinct distance reduction boundary.

GCC normal and GCC ASan/UBSan each passed 1/1 CTest. Clang passed 6/6, including
five mutations rejected against the unchanged original: wrong permutation,
omitted scale reread, omitted fallback recopy, wrong node bound and distance
reassociation. The tests compare exact retained bytes/results, ordered reads,
ordered stores/sites, full owed FP and unrelated host FP preservation.

Commands, source hashes and cleanup are retained in
`analysis/campaign-native-ai-point-geometry-20261028/proof.json`. All compiler,
original ELF, observer executable, raw-bank and duplicate-reference scratch
was removed. Only the bounded required compressed reference and small receipts
remain. Existing fraction/count and prior geometry proofs were unchanged.

The remaining `1C9F10` closure includes selector `1C9770`, point-state owner
`1FC720` and actual path update `200D30`, including workspace/search owners
`27B340`/`27BD30`. This component establishes neither those algorithms nor
campaign runtime promotion, global script FP ownership, full campaign parity
or physical-Xbox CPU equivalence.
