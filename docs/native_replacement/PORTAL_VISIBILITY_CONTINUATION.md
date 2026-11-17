# Portal visibility and light receiver: bridge access cost and continuation

Scope (coordinator, 2026-11-14): portal visibility and the pStencil light
receiver. That covers `134C00`/`134D30`, `135B60`, `135970`, the bridge cost of
`136EA0`, `136A80`, `1362A0`, `136230` and the translated remainder of `31570`.
Three other owners are taken elsewhere: the projection executor
(`44E80`/`2E79A0`/`2E6290`/`2E6ED0`), render-command production (`40E20`,
`40E80`, `41030`, `458C0`, `45CD0`, `460C0`, `44AB0`, `45550`, `44680`,
`4B5C0`, `4BCB0`, `4C440`) and PVS/MOPP/`BD080`/matrix leaves.

## Done: native bridge access cost

The native `136EA0` tree and its children are already defaults: `135970`,
`136E10`, `171070`, `167EF0`, `184690` and `166C00`. Before this change they
still cost about 11% of the Armory frame. Nearly all of that was
`native_raw_x87_detail::Scope::io`: every source read, every per-PC hook and
every store parked and restored the whole x87 file, at about 230 ns per access
on this host. The microbenchmark gives FNSAVE+FRSTOR 96 ns, FNINIT+FRSTOR
63 ns, FXSAVE 17 ns, FNSTCW 0.4 ns.

The change keeps each access's meaning exactly. It is shared by every
`Frame`-based bridge, and also by cluster/sphere frustum and the async queue:

- `native_raw_x87_detail::direct_read` copies an operand directly in two
  cases. The first is a complete committed ordinary page
  (`SourcePhysicsContactMemory::direct_page`). The second is the provider's
  read-only block (`try_read_view`, for sub-page image globals). Both are the
  same transfer that `SourceRuntimeMemory::read` performs. Neither calls a
  provider, observer or guard, allocates, throws or touches FP state. Any other
  operand keeps the scoped provider read, including its exact fault.
- Silent hooks (`Noop` types declaring `silent_hook=true`) skip the empty
  scoped hook call. The instruction-cursor position store is unchanged.
- `supported()` samples CW with FNSTCW and MXCSR with STMXCSR, instead of a
  full FNSAVE/FRSTOR capture. `controls_supported` reads only CW, SW and MXCSR.
- `Scope::io` has an exact fast path when the register file is empty and TOP
  is 0. That state is already its own parked image, so park reduces to
  publish. After the callback, FXSAVE compares CW, SW, tags and MXCSR, and
  restores the exact source environment on any difference.
- `HALO2_REFERENCE_SCOPED_ACCESS=1` restores the previous route for every hook,
  read and io (comparison/oracle only).

Stores still go through the provider's `write` inside `Scope::io`. Registry
guards, observers, write serials and undo journals are untouched.

### Bridge access proof (status 2026-11-14)

- The `136EA0` physical-original oracle passes with the direct reads and view
  reads in place (r2): 1,048 physical cases plus 720 prefix cases; 7,812
  comparisons; 2,054,977 fully compared pre-I/O packets; 4,554 genuine callee
  entries; and the guard and forced-reference checks.
- New test mode 9 (`tests/native_tu15_bsp_admission_tests.cpp`) runs the
  production access route: a silent hook, committed direct pages and the
  `try_read_view` block. It compares the final ABI, every ordered store, the
  genuine child entries and the complete backing. It ran 328 comparisons with
  124,080 direct-page reads, 1,200 view reads and 0 provider reads.
- All 11 compiled mutants are rejected. Four of them are new access-path
  mutants: `direct-page-offset`, `direct-page-unfilled`, `view-read-short` and
  `silent-hook-stale-cursor`.
- The r2 report says `passed:false` only because
  `native_tu15_raw_x87_scope.hpp` was edited during the run.
- The r3 rerun on the final header finished after handback. It covers the
  `Scope::io` fast path and adds the `io-fast-path-keeps-provider-fp` mutant.
  Everything matches r2: 7,812 comparisons, 328 production-route comparisons
  (124,080 direct-page reads, 1,200 view reads, 0 provider reads), and the
  guard and forced-reference checks pass. All 12 compiled mutants are
  rejected, including the io fast-path mutant. Its `passed:false` comes only
  from `changed_inputs: tools/specialize_native_operands.py`, which another
  session edited during the run. Report:
  `scratchpad/vis/bsp-admission-r3.json`.
- `validate_native_raw_x87_scope_tu15.py --negative-controls` passes on the
  final header: 3,552 physical i386 cases, 56,832 comparisons and 576 write
  faults.
- The chained validators on the final header finished after handback:
  - Sphere frustum passes: 3,563 comparisons, 116,571 complete pre-I/O packets.
  - Cluster frustum passes with `--prefixes`: 5,340 comparisons, 638,835
    packets.
  - The async queue ABI passes with `--mutations`.
  - Geometry visibility stopped on input drift: another session edited
    `tools/specialize_native_operands.py` while it ran. Rerun it on a quiet
    tree. The nested `135970` paths are already compared inside the passing
    `136EA0` oracle.
- Sampled Armory profile, Original mode at 1920x1018, under host load: with
  only direct-page reads and silent hooks, the shadow frame went from 27.6% to
  21.2% of the frame, particle `136EA0` from 5.5% to 1.75% and the light
  receiver from 10.5% to 4.7%.
- Not yet run: unloaded A/B timing (the final build, default against
  `HALO2_REFERENCE_SCOPED_ACCESS=1`) and the `parity.sh` frame and trace
  comparison.

Run the A/B on the same binary, with and without
`HALO2_REFERENCE_SCOPED_ACCESS=1` (`armory_ab.py --env`), because the shared
tree also carries other agents' changes. Compare the parity frames and traces
the same way. A meaningful A/B needs an unloaded host.

## Census caveat

Instruction census needs `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`, which
removes the literal instruction cursor. Every literal-cursor-gated native
bridge then refuses admission and runs its translated fallback. In the
2026-11-14 Armory census, `136EA0` (17.4k/frame), `135970` (16.7k), `166C00`
(7.0k), `184690` (3.6k), `167EF0` (3.2k), `171070` (2.2k) and `136E10` (1.6k)
are therefore counted as translated, but they are native in production. The
same applies to other agents' bridged owners. Use the instruction census only
for owners without an installed bridge. Use the sampled profile for bridged
ones.

## Done: 135B60 and 136A80 (2026-11-15)

Both are native by default in the generated Shadow owner, each with a
reference switch. Details and full proof figures:
`135B60_object_admission.md` and `136A80_model_draw_submission.md`.

| Owner | Switch | Physical cases | Mutants | Census (own + leaves) |
|---|---|---|---|---|
| `135B60` (+ inlined `135AE0`, `135620`, `166B60`, `C0CA0`) | `HALO2_REFERENCE_OBJECT_ADMISSION=1` | 1,410 (248 normal, 1,162 prefixes), 314/314 sites | 12/12 | 20.2k/frame -> 0 |
| `136A80` | `HALO2_REFERENCE_MODEL_DRAW_SUBMISSION=1` | 1,419 (128 normal, 1,291 prefixes), 201/201 sites | 12/12 | 7.0k/frame -> 0 |

- Both pass GCC normal (with mutants) and ASan/UBSan runs, the production
  access route (silent hook, direct pages), guards, reference guards, forced
  reference, and a census route.
- Both admit the `HALO2_CENSUS_NATIVE_BRIDGES=1` uncursored census callbacks,
  so the census now counts them as native. Their own closures hold no service
  PC; their children keep their own admission.
- `136A80` is not a pure leaf: each part calls `2CBD0` through the State's
  required boundary (the World's native draw unit). Its admission therefore
  also requires a certified required owner
  (`native_required_owner_policy_detail::preserves_instruction_policy`). Its
  oracle stops before `2CBD0`, applies a deterministic stub, and resumes at
  `136CE5`; the harness uses the same stub as a certified required owner.

Runtime on one shared build (Armory, `parity.sh` 300 frames, 1280x720,
census with `HALO2_CENSUS_NATIVE_BRIDGES=1`):

- `135B60` (build with only 135B60 bound): reference route and twice the
  native route all give frame `5627a51badf47762` and trace `8a9b58b38bef`.
  Census 448,627 -> 428,466 translated instructions/frame (-20,161); the
  census runs also reproduce both hashes.
- `136A80` (later build with both bound): `HALO2_REFERENCE_MODEL_DRAW_SUBMISSION=1`,
  both reference switches together, and twice the native route all give
  frame `5627a51badf47762` and trace `8a9b58b38bef`. Census on that binary
  385,258 -> 378,279 translated instructions/frame (`136A80` 7,031 -> 0); the
  census runs reproduce both hashes. (The census totals differ between the
  two binaries because other agents' bridges landed in between; compare only
  within one binary.)
- Timing (same binary, interleaved pairs, Armory first room 1904x1018,
  native default vs both reference switches, mean frame interval ms, host
  load 6.5-23 from other agents):

  | Mode | Pair 1 ref -> new | Pair 2 | Pair 3 |
  |---|---|---|---|
  | Original | 48.0 -> 35.3 | 39.4 -> 42.8 | 69.2 -> 73.3 |
  | pStencil | 68.2 -> 37.6 | 35.3 -> 34.5 | 37.4 -> 35.6 |

  The pairs move in both directions with load; the removed work (about 27k
  translated instructions/frame) is far below this noise. Not measurable on
  this host; rerun on a quiet host if a figure is needed.

## Remaining translated owners in scope (ranked)

The ranking uses the census of genuinely translated routines (instructions per
frame) and the sampled profile from the 1920x1018 Original run.

1. `134D30`, 5.7k/frame, about 415 instructions. It calls `171070` (bridged),
   `135B60` four times (now native), the `166B60` leaf, and four required
   externals through the unit's dispatcher (`135540`, `C5980`, `C5170`,
   `C5140`; none is in this unit, so `dispatch` reaches `State::required`).
   Notes for the port:
   - Like `136A80`, admission needs a certified required owner, and the
     oracle needs deterministic stubs for the four externals, each with its
     own ABI: `135540` and `C5980` return with `ret 8`, `C5170` and `C5140`
     with plain `ret`; `C5980` fills a caller buffer of up to four handles and
     returns the count in EAX; `C5170`/`C5140` return a byte in AL.
   - The `171070` and `135B60` children should stay genuine in the oracle
     (as in the `135B60` proof), which means a corpus where every object the
     cluster lists reach has a consistent `135B60` world (object table,
     stamps, queue, frustum context).
   - The body repeats one datum-link lookup about ten times
     (`(handle&0xffff)*[table+0x24]+[table+0x44]`, then `+8` next link and
     `+4` handle); a single typed helper with per-site PCs covers them. Two
     sites need hardware flags because the flag result reaches the next
     access: `IMUL ecx,ecx,0x110` at `1350D4` and `SHL esi,2` at `135149`.
2. `1362A0`, 6.7k/frame, and recursive `1365C0`, 2.4k/frame. Both call `4B5C0`
   through the State's required boundary (render-commands owner). `1365C0`
   calls `136A80` directly; with `136A80` native that call stays a direct
   child.
3. `4BAE0`, 3.7k/frame. It calls `32CC0C` twice and `BCFB0`.
4. The `31570` remainder: `C3AB0` (a leaf of 196 instructions, 1.8k/frame) and
   `C3DC0` (641 instructions, 0.9k/frame, calling `166C90` at 1.3k and one
   dispatch). `C3820`, `31570` and `134C00` are below 0.2k each.
5. `136D50`, 1.8k/frame, under `136230`, with one dispatch.

## Coordinator measurement (2026-11-14)

The final bridge fast path was measured on one binary, with and without
`HALO2_REFERENCE_SCOPED_ACCESS=1`. The workload was the Armory first room at
1920x1018, host load 2.6–5.6, in interleaved pairs.

| Mode | Old route | Fast path |
|---|---|---|
| Original | 43.5 / 44.7 ms | 32.8 / 33.3 ms |
| pStencil | 45.0 / 45.7 ms | 33.3 / 32.9 ms |

That is about −11 ms per frame (−25%). Renderer time is unchanged.

Parity: the deterministic `--campaign-reference-capture` run (300 frames,
1280x720) gives the same frame hash `5627a51b…` and trace `8a9b58b3…` for the
reference route and twice for the fast path, matching the pre-fix original
baseline.

Validators on the final header:

- geometry visibility, sphere frustum, cluster frustum and async queue all
  pass;
- 136EA0 r3 passes with 12/12 mutants rejected; its rerun reported only input
  drift from other sessions' tool edits.

Porting the remaining translated owners is paused. Safeguards stopped the
continuation agent (cyber classification), so the ranked list above is still
open.

## Paused (2026-11-15)

A second continuation agent was halted by a safety classifier while planning
134D30, the same kind of stop as on 2026-11-14. Nothing was changed. The
remaining routines are paused until the user gives guidance:

- 134D30;
- 4BAE0;
- C3AB0 and C3DC0;
- 136D50;
- the 1362A0/1365C0 remainder.

135B60 and 136A80 are done and validated; see their docs.
