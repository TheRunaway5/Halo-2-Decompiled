# Spatial owners, part 2: BD080 sphere query, 14DD70 distance, 1E6980 access cost, census bridges

Continues `1532A0_pvs_flood.md` (2026-11-14). Four items, in this order.

## 1. BD080 object sphere query closure (`BD080` / `BA970` / `BAA40`)

BD080 returns, in AX, the objects of a type mask whose bounding sphere meets a
query sphere. Arguments (RET 0x1C): `[E+4]` list flags (bit0 first reference
table, bit1 second; 0 means all), `[E+8]` type mask (0 means all), `[E+0xC]`
location (cluster word at +4), `[E+0x10]` center, `[E+0x14]` radius,
`[E+0x18]` output, `[E+0x1C]` output capacity (word).

With a positive radius it floods the location cluster through the genuine
child 14D0E0 into a 0x200-word frame list (count clamped to 0x200);
otherwise it uses the location cluster alone. A new stamp `[4EABCC]+1` then
dedupes objects in `[4EABD0+(index&0xFFFF)*4]`. For each cluster it walks
the first table (BA970: `[4EABB0]` heads, pool `[4EABB4]`) and/or the second
(BAA40: `[4EABA4]`, `[4EABA8]`). Each reference record holds the object at
+4, the next reference at +8, a type byte at +0xC, the center at +0x10 and
the radius at +0x1C. An object is kept when `d^2 < (r+R)^2`. The two walks
sum the squares in different orders (dz²+dy²+dx² and dz²+dx²+dy²), and the
closure keeps both orders. A full output takes the BD443 exit.

`src/physics/runtime/native_tu15_object_sphere_query.hpp` is ordinary native
control flow (host integers, no register file). Every original memory access
happens in program order at the same address and width, including pushes,
spills, the cluster frame, both BA970/BAA40 frames and the return addresses.
Every store goes through the unit's store helper.

SSE is the original scalar sequence, using `subss`/`mulss`/`addss`/`comiss`
with explicit destinations and memory clobbers. That keeps the NaN payload
owner, rounding and MXCSR sticky flags identical. General registers, XMM0-7
and the final `ADD ESP,0x420` flags are published. The cursor is published
before each access and set to `BD440`/`BD457` at exit.

14D0E0 stays a genuine child, called through the unit's own original route:

- live support and AI frame: `run_14d0e0`;
- combat action and scene device: `dispatch(...,0x14d0e0,0xbd114)`, which is
  `s.required`.

The call gets the exact ABI: ESP at the pushed return address `BD119`, the
`INC ECX` flags with CF from the radius COMISS, and the live XMM file. Its
registers and XMM come back.

BA970/BAA40 are native inside the closure. In combat action and scene device
they used to cost an `s.required` round trip per cluster: the dispatch chain,
then ABI save/load with FNSTCW/LDMXCSR/FLDENV, then another runtime.

Binding: `tools/native_object_sphere_query_replacements.py`, called from
`spatial_owner_bindings` in `tools/specialize_native_operands.py`, receipt key
`native_spatial_owners` (`bd080`). It pins three canonical BD080 bodies:

| Body | Units | 14D0E0 / BA970 / BAA40 calls |
|---|---|---|
| live support | live support | direct / direct / direct |
| AI frame | AI frame | direct / direct / dispatch |
| combat | combat action, scene device (identical) | dispatch / dispatch / direct |

Any BA970/BAA40 body a unit carries must match its pin. Every `selected()`
operand must be the identity. The 14D0E0 route must match the unit (direct
body present, or absent for dispatch). Anything else keeps the translated
route.

Admission requires: the TU1.5 image, no active MMX, masked MXCSR exceptions,
literal cursor without traces, observation-only stores, and no service site
in BD080/BA970/BAA40. `HALO2_CENSUS_NATIVE_BRIDGES` is honoured.

- `HALO2_REFERENCE_OBJECT_SPHERE_QUERY=1` restores the translated bodies.
- `HALO2_VERIFY_OBJECT_SPHERE_QUERY=1` runs the native closure and the
  unmodified translated body on separate copy-on-write overlays, each with
  its own genuine 14D0E0 child. It requires identical ordered stores (pc,
  address, preimage, bytes), registers, flags, XMM and x87, or the identical
  fault after an identical store prefix. Then it runs native on the live
  bank and prints `Object sphere query verify: N calls, M ordered stores
  identical, coverage X` at exit.

### Proof

`tools/validate_native_object_sphere_query_tu15.py` (CTest
`native_tu15_object_sphere_query`, `cmake/NativeTu15ObjectSphereQuery.cmake`)
extracts all three pinned variants plus their BA970/BAA40 bodies and compiles
`tests/native_tu15_object_sphere_query_tests.cpp`.

In the harness, 14D0E0 is a RET 0x14 child whose every effect is a hash of
its complete entry ABI (GP, flags, XMM, arguments). Those effects are the
cluster list (including > 0x200, ≤ 0, upper-bit garbage and -1 clusters),
registers, XMM, flags and a store. Any entry difference therefore diverges.

The fixtures cover:

- both tables with shared objects;
- pre-seen stamps;
- type bytes ≥ 32;
- link and object upper-bit garbage;
- special floats (NaN payloads, SNaN, ±inf, denormals, -0);
- all MXCSR rounding, DAZ and FTZ modes;
- small, negative and upper-garbage capacities;
- output aliasing the cluster frame or the dedupe table;
- center aliasing the location;
- fault injection (one region removed): exception, cursor and store prefix
  must match.

Results (2026-11-14):

- Release: 6,000 comparisons, 4,734,169 ordered stores, 482 faults, 9,270
  genuine children. 39/39 branches covered, none unreachable.
- Census mode: 1,500 comparisons, with zero closure callbacks delivered.
- Verify mode: 1,000 comparisons.
- ASan/UBSan: 1,500 cases plus 300 in verify mode.
- Refusals: MapPack11, active MMX, unmasked MXCSR, full callbacks without a
  cursor, active trace, arbitrary observer, and changed service sites.
- Binding, for all four units: route, install, idempotence, and refusal of
  changed BD080/BA970/BAA40 bodies and of a nonidentity selected operand.
- Mutation screen: 28/28 mutants rejected; sources unchanged afterwards.

## 2. 14DD70 point/segment distance (scene device leaf)

`kernel_14dd70` in `src/physics/runtime/native_tu15_matrix_leaves.hpp` is
bound by `LEAVES['14dd70']` (selected identity required) with admission
`x87_room(s,2)`: two free logical x87 slots, no MMX, masked x87 exceptions.

It is the original SSE sequence (ordered sub/mul/add, COMISS), then one of
two exits:

- in-radius: `FLD/FABS/FSTP`;
- otherwise: `FLD/FSQRT/FLD/FSUB/FXCH/FSUB/FMULP/FLD/FMUL/FADDP/FSQRT/FSTP`.

Both run on the host x87 under the live control word. XMM0 comes from the
spill, C1 from the final FSTP, and each exit publishes its own cursor
(`14DE5A`/`14DE82`).

Each m32 operand of the x87 sequence is an `FLD m32` followed by a
register-form operation. That matches the retained translated route exactly,
sticky flags included. Hardware `FSUB m32` raises no #D for a denormal
operand next to a NaN, but the translated FLD+FSUBP does. The retained
behaviour is preserved, so sticky x87 flags equal the reference.

Proof: `tools/validate_native_point_segment_distance_tu15.py` (CTest
`native_tu15_point_segment_distance`) runs the production-form body
(`instruction_literal`) against `tests/native_tu15_point_segment_distance_tests.cpp`.
The cases cover live x87 depth 0..6, every precision/rounding control,
MXCSR modes, special floats, a point aliasing the origin, row-index
upper-nibble wrap, and faults. Each case compares stores, bank, GP, flags,
XMM, x87 status (C1, TOP, sticky), lanes, CW, MXCSR and cursor.

Results:

- 20,000 comparisons: 4,006 near exits, 14,328 far exits, 3,250 with C1 set,
  1,666 faults.
- Leaf verify mode: 3,056 comparisons.
- ASan/UBSan: 5,000 cases.
- Refusals: depth 7, MMX, unmasked x87, MapPack11.
- Mutation screen: 22/22 mutants rejected.

## 3. 1E6980 / 1E6CB0 adapter access cost

The BSP segment adapter (`native_tu15_bsp_segment.hpp`), its leaf-surface
base (`native_tu15_bsp_leaf_surface.hpp`) and the two surface children in the
same subtree (`native_tu15_bsp_surface_polygon.hpp` for 1E6E80,
`native_tu15_bsp_surface_2d.hpp` for 2A0230) used to wrap every hook, read
and store in `native_host_fp_io_detail::preserve` (FNSAVE+FRSTOR, then
FNINIT+FRSTOR). They now use the shared production access route:

- silent hooks (`Noop::silent_hook`) are skipped;
- reads use `native_raw_x87_detail::direct_read` (committed direct pages and
  `try_read_view`, the same transfer the translated `r()` makes);
- the remaining provider/observer calls use the new
  `native_host_fp_io_detail::preserve_empty`.

`preserve_empty` is `Scope::io`'s exact empty-stack fast path: FXSAVE
before, compare CW/SW/tags/MXCSR after, and restore that environment on any
difference. A non-empty host file falls back to the full `preserve`.
`HALO2_REFERENCE_SCOPED_ACCESS=1` restores the old route.

Proof: the existing physical-original oracles pass unchanged, each with
`--require-binding`:

| Oracle | Result |
|---|---|
| `validate_native_bsp_segment_abi_tu15.py` | 4,986 comparisons, 47,133 pre-I/O packets (same as before the change; also under `HALO2_REFERENCE_SCOPED_ACCESS=1`) |
| `validate_native_bsp_leaf_surface_abi_tu15.py` | 1,856 comparisons, 9,410 packets |
| `validate_native_bsp_surface_polygon_abi_tu15.py` | 784 comparisons, 13,549 packets |
| `validate_native_bsp_surface_2d_abi_tu15.py` | 896 hardware comparisons |

All four also pass with `--sanitizers`. They include the bound-unit routes,
which use the production `Noop` hook. The oracle banks publish no direct
pages, so reads there keep the provider path. In production, `direct_read`
is the same committed-page copy that the translated `r()` already performs.

## 4. Census bridges

`src/physics/runtime/native_tu15_census_admission.hpp` adds
`native_census_detail::uncursored(s,first,last)`. It returns true under
`HALO2_CENSUS_NATIVE_BRIDGES=1` when full callbacks carry no cursor and none
of the three live service PCs lies in the closure. The following now honour
it:

- the leaf framework `native_tu15_leaves::run` (145560, 144CF0, 14CEA0,
  123210, 243450, 2E44D0, 2E46E0, 14CDB0, 14CE30, 2E9DF0, 2E6240, 2E9FD0,
  2EA050, 2EB0C0, 14DD70);
- 14D0E0 cluster flood;
- BCEE0 object scan;
- 1E6980 BSP segment;
- 1E6CB0 leaf surface;
- 309940/305E40 MOPP AABB.

Each edit is admission-only; every frame of these bridges already handles a
null cursor. The `Frame`-based render bridges (136EA0, 135970, 136E10,
171070, 167EF0, 184690, 166C00) dereference the cursor in their frames, so
they are not converted here.

## Production results

### Parity (binary `halo2_native_sphere`, sha256 `86a0b315…`)

The Armory run is 01a, deterministic `--campaign-reference-capture`, 300
frames at 1280x720. Each route below gives frame `5627a51badf47762` and
trace `8a9b58b38bef`:

- reference (`HALO2_REFERENCE_OBJECT_SPHERE_QUERY=1
  HALO2_REFERENCE_MATRIX_LEAVES=1 HALO2_REFERENCE_SCOPED_ACCESS=1`);
- native under the Khronos validation layer (0 messages);
- the reference again;
- verify (`HALO2_VERIFY_OBJECT_SPHERE_QUERY=1 HALO2_VERIFY_MATRIX_LEAVES=1`);
- `HALO2_REFERENCE_SCOPED_ACCESS=1` alone.

The verify run checked all 3,461 BD080 calls: 136,087 ordered stores
identical, 30 of 39 branches reached live. The harness covers all 39. The
nine not reached live are default type mask, no cluster, single-cluster
radius, clamp, empty list, first table skipped, second-table empty head,
and output full on both tables.

Multiplayer (Coagulation, `tools/perf_4k60/parity.py`, cases 4:90, 4:150 and
1:150 at 3840x2160): reference vs native is identical
(`9c14a330…`, `13fa114e…`, `05f31264…`) and the reference is deterministic.
Reference vs verify is identical too. A verify run with `--keep-lines` checked
1,072 / 1,792 / 448 calls (16,080 / 26,880 / 6,720 ordered stores identical).
Its frames are identical. The tool still reports FAILED, because the kept
verify line exists only in the candidate role. The PVS verify run behaves the
same way.

### Census (same binary, Armory, 300 frames, `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`)

| Run | Translated instructions/frame |
|---|---|
| A: no `HALO2_CENSUS_NATIVE_BRIDGES` | 721,300 |
| B: census bridges as before this work (PVS only; the new admissions and both new ports forced to reference) | 624,787 |
| C: `HALO2_CENSUS_NATIVE_BRIDGES=1` | 453,987 (−170,800, −27.3% vs B) |

Removed per frame (B → C):

- Newly native, ports from this work: BD080 23,273 + BA970 1,683 + BAA40 854
  → 42; 14DD70 11,844 → 0. That is −37.6k in total. The 42 left in BA970
  come from its other caller.
- Census accuracy only (already native in production, now counted
  correctly): MOPP 309940 41,710 → 0, 14D0E0 15,174, 14CEA0 18,241, 145560
  13,818 → 3,995 (a unit without the pin), 144CF0 13,030, BCEE0 10,833,
  1E6980 6,275, 1E6CB0 298, plus the other leaves. That is about −133k.

The physics bridges no longer inflate the C ranking. Its top entries are
135B60 16.7k, 135970 16.7k, 136EA0 14.6k, B8A30 10.6k, 2847D0 9.1k, B9180
8.2k, 296AD0 7.5k, 131470 7.4k, 136A80 7.0k, 166C00 7.0k, 1362A0 6.7k, B9090
6.2k and 134D30 5.7k. Three of them, 135970, 136EA0 and 166C00, are the
`Frame`-based render bridges, which still refuse under census (item 4) and
are native in production. The rest are translated.

### Sampled profile (same binary, Armory, 1904x1018 Original, `HALO2_REFERENCE_SYNC_RENDER=1`, 120 frames at 500 Hz, two interleaved pairs, host load 7-14)

The reference route sets `HALO2_REFERENCE_OBJECT_SPHERE_QUERY=1
HALO2_REFERENCE_MATRIX_LEAVES=1 HALO2_REFERENCE_SCOPED_ACCESS=1`.

| Scope (inclusive ms/frame) | Reference | Native |
|---|---|---|
| `run_bd080` (includes the 14D0E0 child) | 0.75 / 1.13 | 0.42 / 0.23 |
| `run_14dd70` | 0.067 / 0.067 | 0.000 / 0.033 |
| 1E6980 subtree (`native_bsp_segment_detail`) | 0.95 / 1.00 | 0.15 / 0.05 |
| Nearest-translated total | 29.8 / 36.8 | 19.5 / 11.9 |

These three scopes drop by about 1.5 ms/frame. Whole-frame times on this
loaded host are noise (66.8 / 91.0 ms reference, 71.9 / 47.5 ms native), so
only the attributed scopes are meaningful.

The profiler counts presents on the main thread. With the shared tree's
current asynchronous campaign render it sees one present and never starts,
so it needs `HALO2_REFERENCE_SYNC_RENDER=1`, as the PVS profile did. The
desktop also clamps a 1920-wide window to 1904.

## Remaining work (ranked)

1. **Render `Frame` bridges under census.** 135970, 136EA0 and 166C00 (about
   38k/frame together), plus 184690, 167EF0, 171070 and 136E10, are native in
   production but still refuse under census. Their frames dereference the
   cursor, so each needs a null-cursor path before it can use
   `native_census_detail::uncursored`. This belongs to the portal-visibility
   owner.
2. **135B60 object admission**, 16.7k/frame genuinely translated (see
   `PORTAL_VISIBILITY_CONTINUATION.md`).
3. **B8A30** (10.6k/frame), **B9180** (8.2k), **B9090** (6.2k), **296AD0**
   (7.5k) and **131470** (7.4k). These are translated owners in the new
   census. 131470 is the 14DD70 caller in the scene device. B9180, B8F40,
   B9090, BE4A0, 10DD10 and 131470 are now native
   (`B8F40_object_tick.md`, 2026-11-15); "B8A30" is not a TU1.5 entry.
4. **1BF30** shadow-frame copy: bind the existing native shader LOD kernel
   (render-command owner).
5. **145560 residual**, 4.0k/frame translated in a unit whose body has no
   leaf pin.
6. `tests/test_native_operand_specialization.py` passes, with 33 tests in
   1,605 s, but exceeds its 1,500 s CTest timeout at host load ~17. Raise the
   timeout or split the test.
