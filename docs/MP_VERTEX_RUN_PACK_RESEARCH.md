# Complete four-view vertex packing comparison

The isolated run-append candidate preserves the captured native renderer's full vertex bytes, ordered draw records and ordered descriptor request arguments. With retained staging capacity, median packing falls from 5.79 ms for the current iterator scatter to 4.98 ms for run append in this comparison. This is approximately 0.81 ms of CPU packing; it does not establish a live frame-rate or GPU improvement. No production renderer change was made for this candidate.

## Exact input and authority

The input is the first complete grouping-stage capture from the actual four-view native Lockout/stock-SMG runtime, at 1280×720 under isolated X11. The producer is retained at `analysis/multiplayer/integration/render-cpu-profile-20261008/runtime-reserve-profile.bin`; its corresponding group-dump log and timing TSV are retained alongside it. Root supplied this capture after adding the read-only `HALO2_GROUP_INPUT_DIAGNOSTICS` hook at entry to the renderer's existing grouping lambda. The probe supplied no gameplay input and did not supply any Vulkan handles or drawing results.

The new independent evidence directory is `analysis/multiplayer/integration/render-run-packing-20261008`. The copied capture contains **19 exact ordered calls and 368,076 vertices**, each 132 bytes. Its four world/object/body inputs contain 81,201 / 81,057 / 80,625 / 81,201 vertices; four foreground inputs contain 10,344 vertices each; four sky inputs contain 654 vertices each. The three initial global spans and the four reflection-background spans are empty. This captures every input of the grouping stage; later shadow/UI appends are outside this comparator.

The input layout declares 1,676 material slots and 66 lightmap slots. The complete ordered result contains 1,070 draws and 1,070 contiguous input-key runs. The raw input binary SHA-256 is `3fb5da21552c4ffcb585d295616560db215e16e8366494051359338f08134ae6`; layout SHA-256 is `a314eaa8c178d242384b61735b7b825da7d9df088a751f8f0f09561dbafd97a6`.

## Candidate and equivalence contract

`tools/benchmark_mp_vertex_run_pack.cpp` transcribes the current iterator-cached sorted grouping path from the separately retained `renderer-observed.cpp`. Its full key is `(material, lightmap, bsp_cluster, source_section, source_part)`. The original ordered map's lexicographic key order and the input vertex order within each key are retained.

The candidate validates every vertex in original input order, collects contiguous source spans under the same ordered full key, creates all ordered descriptor/draw requests for that group, then appends each key's spans in encounter order. It removes the sorted path's default vector resize and second per-vertex cursor/scatter traversal. It does not merge authored parts, reorder triangles within a part, change material/lightmap choices, change visibility/LOD, recompute floating-point values, or change a shader. The alpha-layer `preserve_order` path is kept literally, including repeated keys and its in-stream partial-output behavior.

The comparator checks complete 132-byte vertex records, complete draw tuples including cumulative first/count/call/key, and the ordered descriptor `(call, material, lightmap)` requests. Every intermediate call output is also compared. CPU comparison preserves provider request semantics; it does not fabricate descriptor handles, execute Vulkan allocation, or prove GPU execution.

The normal and independent ASan/UBSan executions each pass **554** complete output comparisons and **1,082** whole failure-state controls. The explicit controls cover empty inputs, reversed triangle blocks, repeated group sequences and nonempty prefixes, unique/alternating/returning keys, invalid material/lightmap at early and later positions in sorted and alpha-preserving groups, and every descriptor request stop in the actual captured sequence. Altered-key/order/failure cases are labeled algorithm controls, not purported game output. Both methods retain the same whole-output prefix at each stop. Different host allocation failures or allocator behavior are not simulated as original gameplay.

## Measurement

Each method runs 136 times per storage mode with eight warmups discarded, leaving 128 samples. Order rotates across the three methods. Comparisons happen outside the timed interval. The fresh mode constructs a new staging vector. The retained mode keeps only its vertex staging capacity between iterations; draw/request lists remain fresh, matching the renderer's staging/list lifetimes. No GPU run or screenshot occurs in this tool. Ordinary host background load is not eliminated, so these are bounded same-input CPU measurements.

| Median CPU ms | Current iterator scatter | Reserve-once scatter | Ordered run append |
|---|---:|---:|---:|
| Fresh staging | 12.66 | 6.39 | 5.05 |
| Retained staging | 5.79 | 6.08 | 4.98 |

The reserve and retained-capacity changes are separate from this run-copy candidate. Root independently measured actual renderer geometry 16.16→9.19 ms and active frame 58.67→50.58 ms in 90 post-warmup samples after those earlier staging changes; those whole-runtime observations are not attributed to run append. The isolated retained-capacity gain here is modest relative to the candidate's run bookkeeping. Production adoption remains a separate decision and measurement.

## Reproduction and limits

`reproduction.json` records exact producer arguments, input/executable/source hashes and normal/sanitizer compile/run argument arrays. Build the standalone comparator with the installed C++ compiler and the repo `include` directory; it needs no game-system link library. Run it with the copied capture stem and an output TSV path. The immutable `manifest.json` pins this tool, the observed renderer reference, exact capture, comparison outputs, reproduction metadata and this document.

`prior-freeze-audit.json` rechecks the previous 43-pin performance freeze. **42 pins remain exact**. Root's later inner-phase profiling extension changed the current `tools/profile_mp_render_cpu.cpp` from the prior pinned `0174e80f…` to `1a4fb1af…`. The exact earlier source is retained at `analysis/multiplayer/integration/render-cpu-profile-20261008/inner-before-profile_mp_render_cpu.cpp` and copied into this new freeze; its SHA matches the original pin. The old binaries remain separate evidence. This audit records that difference rather than treating the current extended profiler as the old source, rewriting the old manifest or overwriting the current tool.

This proves equivalence for the exact captured grouping inputs plus the declared algorithm controls. It is not original-Xbox GPU timing, physical refresh-rate proof, all-map/full-campaign parity, or a claim that later dynamic body/weapon/world geometry is immutable. It establishes no shader quality, effect, HUD, resolution or visibility reduction.
