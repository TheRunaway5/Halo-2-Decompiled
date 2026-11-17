# Projection-volume MOPP queries (`44E80`, `2E79A0`/`2E6290`, `2E7C40`/`2E6ED0`)

Status: native by default in the Shadow-frame generated unit (bridges at the
entries of `run_44e80`, `run_2e79a0`, `run_2e7c40`). Translated bodies remain
only as the `HALO2_REFERENCE_PROJECTION_QUERY=1` oracle (all three) and
`HALO2_REFERENCE_PROJECTION_CULL=1` (44E80 only). Validation figures below.

## Owners

| Original | Role | Native |
| --- | --- | --- |
| `44E80` | per draw-unit/cluster-entry accelerated cull (MOPP branch) | `bridge_44e80` |
| `45A70` | subpart mask marking (sole caller `4526F`) | `mark_subparts` |
| `2E79A0` | plane-volume query setup (sole caller `4520E`) | `bridge_2e79a0`, `prepare_planes` |
| `2E7C40` | sphere query setup (sole caller `456AB`) | `bridge_2e7c40`, `prepare_sphere` |
| `2E6290` / `2E6ED0` | recursive MOPP walkers (plane / sphere) | `Walk<PlaneVolume>` / `Walk<Sphere>` |
| `309600` / `3092A0` | first / last terminal of an accepted subtree | `Walk::endpoint` |
| `458C0` (from 44E80) | range/leaf mask marking (render-command owner) | `native_tu15_sphere_mask_458c0` |

Native callers can also use `sphere_query_rows` (typed 2E7C40: host rows,
logical x87 status in/out, inside the guest arithmetic environment) instead of
an emulated hkArray frame; it refuses exactly where the bridge does, and a
caller whose inline arrays would overflow keeps the frame route so the genuine
2E3250 allocation happens.

All code is in `src/rendering/shadows/native_tu15_projection_query.hpp`;
`tools/native_projection_query_replacements.py` installs the three entry
bridges (hooked into `tools/generate_physics_live_support_tu15.py`, so a
Shadow-frame regeneration keeps them).

## Semantics kept

* MOPP decoding: rescale (1-4), jumps (5-7), key ops (9-B), properties
  (60-6B, including the literal scratch alias: properties 1..3 overwrite the
  local offsets only once the state is local), terminals (30-53), projected
  splits (13-1C: both children, first recursively with a box copy), axis
  splits (10-12, 20-22, 23-25 with the right-lower-before-left-upper order of
  20-22), slab prefixes (26-2B), and the classification order (second child
  first, then first child; both-partial recursion into the second child).
* x87: `FILD/FMUL m32/FSTP m32` for every quantized bound, `FLD/FDIV/FSTP`
  for the inverse scale, `FLD/FMUL/FSTP` for the 2^24 box, `FLD/FADD/FSTP`
  for the doubled sphere radius, and `FLD r; FLD ST0; FMULP; FLD s; FCOMP`
  for each sphere comparison. The translated model's logical status word is
  reproduced (loads clear C1, memory stores publish their C1, FCOMP publishes
  the whole hardware word); exception flags stay live in the host unit.
* SSE: every packed/scalar operation of the original with its destination/
  source order, executed through `asm volatile` (no speculation, merging or
  commutation), so MXCSR sticky flags and NaN payloads match. Plane lanes
  1..3 of 2E79A0 are x87 FLD/FSTP copies: ordinary values are copied bitwise,
  NaN/denormal values run the real x87 copy.
* Outputs: the ordered direct-key and range-endpoint streams are committed to
  the caller's real `hkArray<uint32>`s after the side-effect-free walk, with
  the original header rereads; a full array grows through the genuine `2E3250`
  child at its original call site. 44E80's inline arrays live in its real
  guest frame because `458C0` (render-command owner) and `2E79A0` read them.
* ABI: callee-saved registers, ESP (`RET` / `RET 8`), x87 depth/TOP/control,
  status and MXCSR are published. EAX/ECX/EDX/EFLAGS/XMM are dead at every
  audited continuation (`45213`, `456B0`, `448FB`, `4491C`, native cull walk);
  2E79A0/2E7C40 admission requires the exact continuation word or the
  `PhysicsLiveWorldTu15::prepare_source_call` root sentinel `60300000` (the
  native 45550 owner calls 2E7C40 that way; its C++ consumer reads only the
  arrays and the FP state).

## Refusals (translated body runs; no state published)

Non-TU1.5 image, active MMX, x87 depth > 5, active first/second trace cursor,
another continuation word,
misaligned MOPP/sphere (`MOVAPS` fault), more than 32 planes, an invalid
opcode (original diagnostic), a 309600/3092A0 property 1..3 (would overwrite
that frame's saved registers), the step/recursion budget, and 44E80's linear
(non-accelerated) branch.

## Validation

Physical original (unchanged i386). A freestanding 32-bit runner links the
authenticated TU1.5 XBE (`21b207e7…c4fd`) `.text`, `.rdata` and `.data` at
their real VAs, installs a deterministic allocator object in `[48C938]`, and
calls unchanged `2E79A0`/`2E7C40` (with genuine `2E6290`/`2E6ED0`/`309600`/
`3092A0`/`2E3250`/`2E2E30`) on generated cases, isolating crashes with
alternate-stack signal handlers. The same cases run through the native
bridges in a 64-bit harness that also contains the verbatim translated bodies.
Generated cases cover every walker opcode (00-0B, 10-2B, 30-53, 60-6B), all
nine split outcome pairs, all slab outcomes, both array streams with inline,
owned, aliased and full arrays (genuine growth), plane counts 0..12,
NaN/denormal/infinite/-0 inputs, six x87 control words and seven MXCSR modes.

* Typed `sphere_query_rows` (seeds 401..404): 3,015 growth-free sphere cases
  match the physical original's appended rows, x87 status word and MXCSR.
  Receipt `build/test-results/native-projection-query-20261114/physical-rows.json`.
* Registered CTest (`tools/run_tests.py --sanitizers`, disposable tree):
  `native_projection_query_binding` and `original_tu15_native_projection_query`
  pass; the latter under ASan/UBSan admits 7,967 native executions over seeds
  101..104 with 0 differences (refused programs run the translated fallback and
  are classified separately; one such fallback differs from the original in
  x87 C1 only under sanitizer instrumentation).
* Receipt (seeds 101..115, 2,000 cases each): 30,000 physical cases;
  29,855 complete natively and match the physical original exactly in
  callee-saved registers, ESP, the complete x87 status word, control word,
  MXCSR and every heap byte (arrays and genuine-growth storage), 5,252 of
  them with genuine 2E3250 growth: **0 native differences**. The other 145
  are programs that crash the original itself (a property 1..3 inside an
  accepted subtree overwrites 309600/3092A0's saved frame; or a wild return):
  native refuses them before any effect. Earlier exploratory batches (seeds
  7, 11..20, 31 under ASan/UBSan, 41) agree.
* The translated route differs from the physical original in 4 of those
  NaN-input cases where native matches it (two-NaN SSE operand order is
  commuted by the literal intrinsics), so the physical original, not the
  translated route, is the reference for NaN payloads.
* Non-physical mode (x87 depth 0..2, seed 201): 20,000 cases (3,415 with
  growth), 0 differences from the translated route outside 9 NaN-input cases.
* Out-of-tree mutation screen (`mutate.py`, 3,000 physical cases each): 19/19
  semantic mutants rejected (split order, 20-22 plane, rescale shift,
  property alias, plane inside/outside/radius order, sphere inside/outside/
  lane 3, endpoint child, bound offset, 16/24-bit operand widths, C1
  publication, doubled origin distance, remainder lane, batch count). One
  equivalent control (a spare lane duplicating another present plane)
  survives as expected. In-tree header SHA-256 unchanged afterwards.
* Live Armory (Original, deterministic capture, 300 frames) with
  `HALO2_NATIVE_PROJECTION_QUERY_ORACLE=strict`: 44,700 native/translated
  comparisons with the final binding (44E80 10,500/10,500 native, 2E79A0
  22,800/22,800, 2E7C40 11,400/11,400 from the native 45550 owner, 0
  declines), **0 mismatches**; the first binding run (before the native-root
  continuation) recorded 33,300
  comparisons (44E80 frames, unit/mask arena, instance planes and both
  arrays; 2E79A0 frame window and arrays; callee-saved ABI, x87 status,
  control word, MXCSR), 0 mismatches. Captured live fixtures replay with 0
  failures under Release and ASan/UBSan: 20,000 plane queries (336,168
  ordered events), then 13,350 plane + 6,650 sphere queries (270,652 events).

## Runtime results (Armory 01a, Original, 2026-11-14)

All on one binary with `HALO2_REFERENCE_PROJECTION_QUERY=1` on vs off.

* Final binding (44E80 also calling the native 458C0 kernel), census with
  `HALO2_CENSUS_NATIVE_BRIDGES=1`: 343,022,361 reference vs 202,305,679
  native (-140.7M, -41.0%; 1.14M -> 0.67M per frame). The family (2E6290,
  2E6ED0, 2E79A0, 2E7C40, 44E80, 45A70, 309600, 3092A0, 458C0, 32A660)
  falls from 141.2M to 0.96M (32A660 probes of other owners). Strict oracle on
  that binary: 44,700 comparisons, 0 mismatches; 458C0 native 6,000/6,000.
* Earlier binding without the 458C0 call (`HALO2_INSTRUCTION_CENSUS`, full
  callbacks, 300 deterministic frames): 357,105,693 reference vs 220,979,583 native
  (-136.1M, -38.1%; 1.19M -> 0.74M per frame). 2E6290 93.85M, 2E6ED0 28.17M,
  44E80 4.97M, 2E79A0 2.69M, 309600 2.06M, 3092A0 2.07M, 45A70 0.76M,
  2E7C40 0.62M and 44E80's 32A660 probes all reach 0. The bridges also admit
  under full instruction callbacks, so the census reflects production.
* Parity: deterministic `--campaign-reference-capture` frames and traces are
  identical across all reference/native runs (frame `5627a51badf47762…`,
  trace `8a9b58b38bef450d…`), including the strict-oracle and census runs.
* CPU A/B (user CPU of the 300-frame capture, host load ~30): reference
  44.67/44.39/43.90 s vs native 42.03/42.86/42.88 s; native lower in every
  pair (mean -1.73 s, about -5.8 ms/frame). Wall-clock `armory_ab` at
  1920x1018 was dominated by concurrent-build contention (renderer time alone
  varied 23-44 ms); interval minus renderer was -4.9/+0.4/-3.5 ms per pair.

## Tooling (in tree)

`tools/validate_native_projection_query_tu15.py` (CTest
`original_tu15_native_projection_query`, plus `native_projection_query_binding`
for the installer) builds everything in owned temporary storage:

* Physical runner (`tools/native_projection_query_physical/`): freestanding i386 ELF (`as --32`/`ld -melf_i386`, no libc),
  image sections at their VAs, `[48C938]` deterministic allocator object
  (`+10` bump allocate `RET 8`, `+14` release `RET 0C`), SIGSEGV/SIGILL/
  SIGBUS/SIGFPE on an alternate stack mark a case as crashed.
* Differential harness: the six translated bodies extracted verbatim from the
  generated unit (bridge lines stripped, body SHA-256s recorded), a flat
  paged bank, the shared deterministic `2E3250` owner for the translated and
  native routes, generated MOPP programs/planes/spheres/arrays/FP states.
* `--mutants`: copies the header, applies each mutation, rebuilds the harness
  against the copy and runs 3,000 physical cases; checks the in-tree hash.
  Receipt: `build/test-results/native-projection-query-20261114/physical-mutants.json`
  (seeds 101-104 0 native differences; 19/19 mutants rejected).
* Live fixtures: `HALO2_NATIVE_PROJECTION_QUERY_FIXTURES=file` (with the
  oracle) records oracle-confirmed inputs (program extent, planes/sphere, FP
  state) and the translated streams/FP results for offline native replay.

## Remaining (ranked)

1. The native 45550 owner can switch its 2E7C40 service to
   `sphere_query_rows` and drop its emulated hkArray frame.
2. 44E80's linear-bound (non-accelerated) branch (`45372`..: per-node
   `145560` composition and `167EF0` cluster frustum) still declines to the
   translated body; not reached in Armory.
3. The typed main-cull `source_bsp_accelerator_query0/_sphere` (native, not
   translated) revalidates the whole MOPP tree per query and does not
   reproduce the sphere lane-3 MXCSR operation; it could reuse this kernel.
4. Translated shadow-mode `44710` (2 calls/frame) and residual `32A660`
   probes from other owners.
