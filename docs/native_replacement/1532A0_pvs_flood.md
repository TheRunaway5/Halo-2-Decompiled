# Native PVS update closure (`1532A0` / `170C80` / `170FB0` / `44680`)

13B6F0 (PVS update, once per tick) reaches 13B610, which calls 1532A0 to OR
the portal-flooded visibility of every flagged scenario cluster into the
caller's cluster vector. 1532A0 is the complete closure root: it calls only
170C80, which calls only 170FB0 and 44680. All four are integer-only (no
x87/SSE), have no `dispatch`/`required` edges and exist only in
`src/physics/physics_live_support_tu15_generated.cpp`.

| Routine | Role |
|---|---|
| 1532A0 | Scan `[[4F54F4]+0x48]` cluster bits; for each set bit with a usable scenario record (`+0x2A` cluster, optional `+0x28` portal word), flood it into a 0x40-byte frame vector and OR that into `[ESP+4]` (high word first) |
| 170C80 | Fixed point over the cluster's PVS: start from its second-plane row (or copy its plain row when cluster flag 2 is clear), then for each visible cluster's portal not yet visited, if portal flag 4 and the global portal bit `[505E7C]` are set, add `(row(a) | row(b)) & row(c)`; repeat until the vector stops changing (REPE CMPSB) |
| 170FB0 | Row pointer of a cluster in the second PVS plane when `[bsp+0x54]` holds it, else in the first |
| 44680 | Next set bit of a word-vector iterator `{int16 words, int16 word, u32 pending, u32 *vector}` |

## Native route

`src/physics/runtime/native_tu15_pvs_flood.hpp` is ordinary native control
flow (loops, host integers, no register file, no flags except the final
`ADD ESP,0x40` flags the caller can observe). The guest bank keeps the
original ABI at this boundary: every original memory access still happens in
program order at the same address and width, including the stack frame
(pushes, spills, the three 0x40-byte local vectors, return addresses), and
every store goes through the unit's store helper, so guards and observers see
the identical store stream. The instruction cursor is published before each
access and set to `1532F2` at exit, as the translated route leaves it.

Admission (otherwise the translated body runs unchanged): TU1.5 image, DF
clear, literal instruction cursor without traces/service sites in the closure,
observation-only stores, and `[bsp+0x9C]`, `[bsp+0x5C]` vectors of at most
16 words (the original's 0x40-byte frame buffers; larger counts would overflow
into the frame itself).

`tools/native_pvs_flood_replacements.py` pins the four complete canonical
bodies (state type normalized) and installs the prefix only into the
build-local specialized copy (`tools/specialize_native_operands.py`,
receipt key `native_spatial_owners`). A changed body keeps the translated
route.

- `HALO2_REFERENCE_PVS_FLOOD=1` restores the translated bodies.
- `HALO2_VERIFY_PVS_FLOOD=1` runs, for every admitted call, the native closure
  and the unmodified translated closure on separate copy-on-write overlays of
  the live bank, requires identical ordered stores (pc, address, width, bytes),
  registers and flags, then runs native on the live bank. It prints
  `PVS flood verify: N calls, M ordered stores identical, coverage X` at exit.
- `HALO2_CENSUS_NATIVE_BRIDGES=1` (diagnostic, census runs only) admits the
  closure under `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`, so the instruction
  census counts only what still runs translated. Without it every
  cursor-gated native bridge refuses during a census and its translated
  fallback is counted, which is why already-native owners (309940, 145560,
  144CF0, 14CEA0, 14D0E0, BCEE0, 1E6980) still rank in callback censuses.

## Proof

`tools/validate_native_pvs_flood_tu15.py` (run through `tools/run_test.py`)
extracts the four pinned bodies, compiles
`tests/native_tu15_pvs_flood_tests.cpp` against them and compares seeded
complete worlds (cluster/portal tables, both PVS planes, padding bits,
negative/zero portal counts, empty cluster sets, scenario records with and
without portal words, random caller registers and frames): ordered store
events with preimages, the whole bank, registers, EFLAGS, x87/SSE state and the
final cursor. It also checks admission refusals, the binding (install,
idempotence, refusal of a changed body), complete reachable branch coverage,
an ASan/UBSan replay, and a 20-defect out-of-tree mutation screen (mutated
header copies first on the include path), then rechecks the source hashes.

Unreachable branches: `scan_negative` (a 2^31-entry scan) and `copy_bytes`
(every vector length is `words*4`).

## Results (2026-11-14)

- Seeded proof: 5,707 Release comparisons (22,151,467 ordered stores) and
  1,430 ASan/UBSan comparisons (6,073,117 ordered stores) identical, 293/70
  out-of-domain cases refused; 25/25 reachable branches covered; 20/20
  mutants rejected; source hashes unchanged after the screen. Registered as
  CTest `native_tu15_pvs_flood` (passes through `tools/run_tests.py`).
- Live Armory (01a, deterministic `--campaign-reference-capture`, 300 frames):
  `HALO2_VERIFY_PVS_FLOOD=1` verified all 900 calls (the census entry count),
  5,421,600 ordered stores identical, 16 branches reached. Frame and trace
  hashes identical for reference, native, reference repeat, native under the
  Khronos validation layer (0 messages) and verify.
- Multiplayer (Coagulation, `tools/perf_4k60/parity.py`, 4:90, 4:150, 1:150 at
  3840x2160): reference vs native identical and reference deterministic;
  a verify run matched every frame and verified 180/300/300 calls.
- Sampled Armory profile (same binary, synchronous render, 120 frames): the
  PVS closure costs 0.717 ms/frame translated and 0.217 ms/frame native.
- 145560/144CF0 leaf extension (selected-operand pins in AI frame, scene
  device and effect particle): `tests/test_native_matrix_leaf_faults.py` and
  `tests/test_native_operand_specialization.py` pass; on the rebuilt binary
  Armory reference (`HALO2_REFERENCE_PVS_FLOOD=1 HALO2_REFERENCE_MATRIX_LEAVES=1`),
  native (validation layer, 0 messages), reference repeat and verify
  (`HALO2_VERIFY_PVS_FLOOD=1 HALO2_VERIFY_MATRIX_LEAVES=1`) frames/traces are
  identical, and multiplayer reference vs verify passes all three cases.
- Same-binary Armory census (300 frames): 736,595 translated instructions per
  frame without `HALO2_CENSUS_NATIVE_BRIDGES`, 640,085 with it (-96.5k/frame,
  -13.1%). The removed count is exactly the closure: 1532A0 78,300, 170C80
  24,168,600, 170FB0 18,900, 44680 4,687,200 over 300 frames, now 0.

## Spatial owners: production status and continuation plan (2026-11-14)

The Armory census (HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1) disables every
cursor-gated native bridge, so it ranks already-native owners. A sampled
production profile of the same workload (armory_ab.py --sample, 120 frames)
shows what really runs translated:

| Census owner | Census share | Production (sampled) | Status |
|---|---|---|---|
| 170C80 (+44680) PVS | 5.76% | 1.12 ms + 0.23 ms self | native here (1532A0 closure) |
| 309940 MOPP AABB | 2.10% | not translated | native (`native_tu15_mopp_aabb_traversal.hpp`); census artifact |
| BD080 object sphere query | 1.66% | 0.77 ms (combat action) + 0.20 ms (scene device) | native closure with BA970/BAA40 (`BD080_object_sphere_query.md`) |
| 14CEA0 portal predicate | 1.30% | native leaf in all 3 units | census artifact |
| 145560 transform product | 1.13% | 0.02 ms residual | native in live support; now also bound in AI frame, scene device, effect particle (selected-operand pin) |
| 14D0E0 cluster flood | 1.08% | native in all 3 units | census artifact |
| 144CF0 orientation matrix | 0.93% | 0.10 ms (mostly the native kernel) | native in live support; now also bound in AI frame |
| 14DD70 point/segment distance | 0.85% | 0.18 ms | native leaf `kernel_14dd70` (`BD080_object_sphere_query.md`) |
| BCEE0 object scan | 0.77% | native in all 3 units | census artifact |
| 1BF30 shader LOD select | 2.46% | 0.37 ms (shadow frame, callers 41030/40E20) | translated copy in shadow frame; a native kernel exists in `src/rendering/world/native_tu15_shader_lod.cpp` for the native command path |
| 1E6980 BSP segment | 0.45% | 0.97 ms in its native adapter | native; adapter and surface children now use direct reads, silent hooks and `preserve_empty` (`BD080_object_sphere_query.md`) |

Items 1, 2, 4 and 5 below were done in `BD080_object_sphere_query.md`
(2026-11-14), which carries the current ranking. The original plan follows.

Ranked remaining work:

1. **BD080** (combat action and scene device bodies are identical; live
   support and AI frame differ only in direct vs dispatched callees). Args
   (RET 0x1C): flags `[E+4]` (bit0 type-mask object list, bit1 second list via
   BAA40), type mask `[E+8]`, location `[E+0xC]` (cluster at +4), center
   `[E+0x10]`, radius `[E+0x14]`, output `[E+0x18]`, max `[E+0x1C]`; returns the
   count in AX. It floods up to 0x200 clusters through 14D0E0 (native, via
   `s.required`), walks each cluster's reference list (BA970 first, then the
   `[4EABB4]` reference array), dedupes by the `[4EABCC]` query stamp into
   `[4EABD0+i*4]`, and tests `(dx^2+dy^2+dz^2) < (r+object_radius)^2` with
   SUBSS/MULSS/ADDSS/COMISS in that order. BA970/BAA40 are small and still
   translated. Port as one closure like this one (keep 14D0E0 through
   `s.required`), exact SSE order with `ordered_*_ss`, exit XMM0-5 published.
2. **14DD70**: leaf-framework kernel (`native_tu15_matrix_leaves.hpp`) with
   the x87 FSQRT/FABS/FSTP sequence and C1 publication.
3. **1BF30** shadow-frame copy: bind the existing native shader LOD kernel
   (coordinate with the render-command owner).
4. **1E6980**: reuse the shared `native_tu15_raw_x87_scope.hpp` fast path in its
   adapter instead of per-access `preserve`.
5. A census that keeps every bridge admitted: other bridges can adopt the
   `HALO2_CENSUS_NATIVE_BRIDGES` admission used here.
