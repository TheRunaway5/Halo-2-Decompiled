# Collision ray `16C620` (native, 2026-11-15)

`16C620` casts a segment through the world: the structure BSP, the cluster
bridges, instanced geometry and the objects in every cluster the segment
crosses, then optionally pulls the hit point back inside the BSP. Its
callers include the AI line-of-sight and perception rays, the camera, combat
and projectile paths, and the `D3D90` lightmap query under `14DA70` (the
last translated service of [object lighting `3DAF0`](OBJECT_LIGHTING_3DAF0.md)).

## What runs natively

The binding is `src/physics/runtime/native_tu15_collision_ray.hpp`. The
specializer installs it (`tools/native_collision_ray_replacements.py`, via
`spatial_owner_bindings`) at the start of both translated owners:
- live support: every other unit reaches this body through its required
  route;
- AI frame: its own copy, whose children are dispatched.

Each owner body is pinned by SHA-256 (state type normalized), and every
`selected()` operand must be the identity under TU1.5. A changed body keeps
its translated route.

The query is the existing native typed owner tree,
`native_tu15_query_bound_camera_scene`:

| Original | Native owner |
|---|---|
| `16C620` | `native_tu15_query_camera_geometry` |
| `16B940`, `16E360` | `native_tu15_camera_bsp_query_flags`, `native_tu15_camera_object_query_flags` |
| `1E68E0` (`1E6980`, `1E6CB0`, `2A0230`, `1E6E80`) | `native_tu15_query_bsp_segment` |
| `24DC90` (`30B890` setup, `30AA70` MOPP traversal, `24DEC0`/`24DEF0`/`24DF20` callbacks, `16C000` instance query) | `native_tu15_query_bound_instanced_scene_ray` |
| `BA970`, `BA9E0` | `native_tu15_begin_spatial_object_query`, `native_tu15_next_spatial_object_query` |
| `120FA0` | `native_tu15_segment_near_sphere` |
| `16C370` (`16B870`, `1F7E00`, `1F8200`/`1F7F60`/`1F7E90`/`144090`, `DB030`, `16B9A0`) | `native_tu15_query_dynamic_camera_object` |
| `16BB40`, `14CDB0`, `11E600` | clearance repair leaves |

It is ordinary C++ control flow over the guest bank. SSE scalar operations
keep the original operand order, and x87 sequences are the original ones,
so NaN payloads, sticky flags, C1 and rounding match.

Two additions to the shared owner complete the parent:
- `mask_slot`: `16C620` rewrites the caller's mask argument word when it
  widens the mask (`16C654`, `16C886`). The binding replicates both stores.
- `exit_eax`: EAX at the `16CE94` RET. AL is the result. Bits 8-31 are the
  displacement pointer, or on the clearance path the last `16BB40`, `14CDB0`
  or `11E600` value.

## Binding ABI

The binding publishes:
- every non-stack store, in program order;
- EAX (exact);
- ESP+0x1C;
- the x87 status word (C0-C3, sticky flags, TOP) and MXCSR.

EBX, EBP, ESI and EDI are unchanged, as in the original. ECX, EDX, EFLAGS
and XMM0-7 keep their entry values.

`tools/analyze_call_site_liveness_tu15.py --target 16c620 --wrappers` walks
every path after all 71 TU1.5 call sites. It tracks EAX/ECX/EDX bytes,
EFLAGS, and XMM low and upper lanes (scalar SSE keeps upper lanes; packed
ops and full stores read them). No caller reads ECX, EDX, EFLAGS, any XMM
lane or EAX bits 8-31 before rewriting them.

It also follows 201 wrapper return paths, where a function returns with
`16C620`'s EAX still live. Two of them (`1B8DC9`, `1BB436`) copy EAX bits
16-31 into an argument register, which is why EAX is published exactly.

The original frame and its children's frames, below the entry ESP, become
private native DATA. A host-backed 64 KiB window at an unowned bank address
(`find_unowned_range(0x61000000, 0x62000000)`) holds:
- the 1052-byte BSP output;
- the BSP context, iterator and payload;
- the instanced-query DATA;
- twelve `16C370` recursion frames.

That window is never visible in the bank. Dead stack bytes below the
caller's ESP are therefore not rewritten, which matches the
[`3DAF0`](OBJECT_LIGHTING_3DAF0.md) precedent (non-stack comparison).

Every guest store is journaled and committed only after the native query
completes. A refusal (an owner outside the authenticated domain) leaves the
bank untouched, and the call runs the translated body. Admission requires:
- TU1.5;
- no active MMX;
- masked SSE and x87 exceptions;
- an empty guest x87 stack;
- the BD080 instruction-cursor policy (literal cursor, no active trace,
  observation-only store observers).

## Switches

Native is the default (since 2026-11-16, see the last section).

- `HALO2_REFERENCE_COLLISION_RAY=1`: translated bodies (the off switch).
- `HALO2_VERIFY_COLLISION_RAY=1`: for every admitted call, the translated
  body runs on the live bank under a recording observer and its stores are
  undone. The native query then runs. The two must have:
  - identical non-stack final bytes (a byte only one route wrote must keep
    its before value);
  - identical EAX, ESP, x87 SW and MXCSR;
  - unchanged callee-saved registers.

  The native result is kept. Ordered-store differences are counted.
- `HALO2_COLLISION_RAY_STATS=1`: calls, native, declined and refused counts,
  native time, and reasons.
- `HALO2_COLLISION_RAY_CAPTURE=<file>` (`_EVERY`, `_LIMIT`): sampled live
  inputs, one JSON line per call (arguments, FP words and every 64-byte line
  the query touches plus the frame's stack lines), for offline replay.

## Proof

Census slice. In Armory (01a, Original, `--campaign-reference-capture .05 0 305419896`,
1280x720, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`), the executed
16C620 tree is 29 translated routines. The same binary was run with
`HALO2_REFERENCE_COLLISION_RAY=1` and with the default:
- translated instructions fall from 378,222 to 368,276 per frame (−2.63%,
  −2.98 M per 300 frames);
- `16C620` (552 k), `BA9E0` (343 k), `1F8200` (305 k), `30AA70` (271 k),
  `1F7F60` (166 k), `1E6E80`, `1F7E90`, `16C370`, `16C000`, `24DC90`,
  `1E68E0`, `2A0230`, `30B890`, `24DF20`/`24DEF0`/`24DEC0`, `BA970`,
  `16B940`, `16E360`, `16B9A0`, `1F3600` and the rest of the tree all reach 0;
- `120FA0` (497 k → 2.4 k), `144090` (184 k → 8.8 k), `1F7E00` and `16B810`
  keep only their other callers.

`14DA70`, `14D410`, `14D790`, `D3D90`, `D4F90` and `D4350` are unchanged.

Caller ABI. `tools/analyze_call_site_liveness_tu15.py --target 16c620
--wrappers --require-clean` (CTest `original_tu15_collision_ray_call_site_liveness`)
gives:
- 71 call sites, none reading a leftover register;
- 201 wrapper return paths, whose only reads are two EAX bits 16-31 copies,
  covered by the exact EAX.

Live side-by-side verify (`HALO2_VERIFY_COLLISION_RAY=1`) in Armory, 300
frames:
- 434 calls, all native, 0 declined, 0 refused;
- 107,868 non-stack store bytes identical, with identical EAX, ESP, x87 SW
  and MXCSR in every call;
- frame `5627a51badf47762`, trace `8a9b58b38bef450d` (the reference).

In 358 calls the bytes are written in a different order with the same final
values (see below).

Default native route, same binary: Armory 434 native calls, 0 refusals,
frame `5627a51badf47762`, trace `8a9b58b38bef450d`.

Journal unit test (CTest `native_tu15_collision_ray_journal`, passes under
ASan/UBSan) checks that:
- reads see journaled stores, and nothing reaches the bank before commit;
- the private window is host-only and unobserved, and a straddling access
  is rejected;
- commit preserves program order, with each event after its store;
- a refused query leaves the bank byte-identical.


## Status (coordinator, 2026-11-15; superseded below)

Safeguards stopped the porting agent while its multiplayer verify and rebuild
were still pending. The Armory evidence above is complete. Multiplayer was not
verified, so native is opt-in for now:

- `HALO2_NATIVE_COLLISION_RAY=1` selects native;
- `HALO2_VERIFY_COLLISION_RAY=1` also admits native;
- the default is the translated route.

Before making native the default again, run an MP verify: Coagulation 1p/4p
with firing/motion input, plus vehicle maps. Native refusals already fall back
to the translated body, so the remaining risk is a value difference.

## MP verify result (coordinator, 2026-11-16; superseded below)

The MP verify sweep covered every MP map at 1p/4p with scripted input, plus
campaign, run with `HALO2_VERIFY_COLLISION_RAY=1`. The receipts are in
`build/test-results/collision-ray-mp-verify-20261116/`.

**Result.** 6 of 56 cases complete. Most of the others stop on a real native
difference, reached from several roots (16C620 itself, 179DF0, D3D90, B9090):

    Native 16C620 differs ...: x87 SW translated 0320 native 0120

**The difference.** The x87 status word differs only in C1 (bit 0x200): the
translated route has C1 = 1, the native has 0. C1 after an FST/FSTP records
the rounding direction. Some x87 store sequence on the native path sets C1
differently from the original, probably an FST(P) whose rounding the native
reproduces with a different instruction or operand form.

**What still matches.** Non-stack stores are identical in every case. The
known store-order differences are the same as before (identical final
bytes, different order).

**Status.** Native stays opt-in until C1 matches and the MP verify is clean.
The Armory verify (434 calls) does not reach this case.

## C1 root cause and fix (2026-11-16): native is the default

**Root cause: the reference, not the native.** Two TU1.5 owners translate
16C620:
- AI frame: a newer emission, whose x87 stores set C1 the way the hardware
  does (at every site involved below);
- live support: it, and the copy of `30AA70` its required route reaches,
  predate that emission and lose C1 in two places.

The native runs the original x87 sequences on the host, so it matches the
hardware and the AI-frame copy. The verify differences were the live-support
reference's C1 losses:

1. **`FSTP ST(0)` in `30AA70`.** The live world runs the MOPP ray traversal
   `30AA70` (a required owner of the live-support route) in the Death-cycle
   translation (`src/gameplay/units/death_cycle_runtime_tu15_generated.cpp`).
   That copy pops at `30AE00`, `30B2C7`, `30B6AB` and `30B6AD` without clearing
   C1. The AI-frame copy, the hardware and the current generator ("Register
   stores do not round and clear C1") all clear it. The failing path:
   1. `30B2DD FCOMP` sets C0 = 1;
   2. `30B2FB FSTP m32` rounds up, so C1 = 1;
   3. `30B323 JMP 30AE00`, then `FSTP ST(0)` clears C1;
   4. `30AE02` dispatches a leaf, and the traversal returns with no further
      x87 change.

   The reference reports `0320`; the hardware and native report `0120`. This is
   every "translated 0320 native 0120" stop in the receipts above.
2. **`FSTP m32` in the clearance repair.** The live-support 16C620 stores with
   `u(spill(...))`, which keeps the C1 of the preceding `FLD` (0). The original
   `16CDEF FLD / FABS / FDIVR / 16CDFB FSTP m32` leaves the store's rounding
   direction, and nothing after it touches x87. The AI-frame copy already uses
   `native_single_spill`, which publishes it. This gives "translated 0120 native
   0320" (or `0020`/`0220`), mask `2480000f`, root B9090. It was hidden behind
   cause 1 until that was fixed (stage 1 sweep: Ascension, Elongation and
   Warlock 4p).

Evidence:
- **C1 trail** (temporary diagnostic, Dune 4p, mask `4480000d`). The
  reference's C0-C3 change last at the return of the `30AA70` required call
  (`0000` to `0300`). The native traversal is already at `0100` at its first
  leaf callback and at its exit.
- **Host probe.** `FCOMP` (C0 = 1), then `FDIV` and `FSTP m32`, gives SW
  `3B20`; a following `FSTP ST(0)` gives `0120`.
- **Diagnostic sweep.** In a 9-case sweep (SW differences reported, not fatal),
  17 of 17 differences had signature 1. All were live-support roots.
- **Stage-1 sweep** (`build/test-results/collision-ray-mp-verify-c1-stage1-20261116/`,
  cause 1 fixed): the only remaining differences were 3 of signature 2.

The native is shared by both owners. Matching it to the live-support reference
would make it diverge from the original and fail the AI-frame verify in the same
cases. **The native code is unchanged.**

**Fix** (the live-support reference now models C1 like the AI-frame copy):
- Death `30AA70`: its four `FSTP ST(0)` gain the clear, with the same lines as
  the AI-frame copy (`s.fp.st(0)=s.fp.st(0);s.fp.status&=~0x200u;s.fp.pop();`).
  The two bodies again differ only in the AI-frame copy's 90 load clears, which
  `ExtendedStack::push` already performs. Two related files follow:
  - the not-yet-installed MOPP installer pins
    (`tools/native_mopp_ray_traversal_replacements.py`) add the new Death
    digest `7705b4f4...`;
  - its test (`tests/test_native_mopp_ray_traversal_replacements.py`) expects
    `[94, 4]` clears and identical `FSTP ST(0)` lines (7/7 pass).
- Live support (`src/physics/physics_live_support_tu15_generated.cpp`): the m32
  stores of 16C620 (`16C9BE`, `16CDFB`) and 30B890 (`30B8D2`, `30B8EE`,
  `30B8F8`) use the unit's existing C1-publishing store
  `native_bsp_segment_detail::spill_reference`, already used at `1E6A6F`. It is
  the same `FLDT/FSTPS/FNSTSW` as the AI-frame unit's `native_single_spill`.
  - A new `native_single_spill` helper in this unit would change the E0900
    flight-property installer's spill-helper profile and stop the build.
  - The specializer receipt is unchanged apart from its hashes: every installed
    entry, including 16C620, is still installed.
  - The collision-ray installer pin (`tools/native_collision_ray_replacements.py`)
    moves to the new live-support 16C620 digest `07840145...`.

These changes alter only C1 after those instructions. Stored values, every
other register and the Armory frame and trace are unchanged (see Results).

**Native default.** `reference_mode()` is now only
`HALO2_REFERENCE_COLLISION_RAY=1`. `HALO2_NATIVE_COLLISION_RAY` is gone, and
`HALO2_VERIFY_COLLISION_RAY=1` still compares every call.

**Results** (binary `dd6feea0...`, built through the shared `build-perf-4k60` tree):

- **Routine tests.**
  - `native_tu15_collision_ray_journal` passes (Release, and ASan/UBSan).
  - `original_tu15_collision_ray_call_site_liveness` is skipped by CTest when no
    XBE is configured. Run directly on the TU1.5 XBE: 71 call sites, none
    reading a leftover register; 201 wrapper return paths, whose only reads are
    the two EAX bits 16-31 copies.
  - `tests/test_native_mopp_ray_traversal_replacements.py`: 7/7.
  - The specializer receipt for the live-support unit is unchanged apart from
    its hashes. The 16C620 bridge is installed in both owners.
- **MP verify sweep** (`HALO2_VERIFY_COLLISION_RAY=1`): every MP map at 1p and 4p
  plus six campaign levels, 56 cases. Receipts are in
  `build/test-results/collision-ray-mp-verify-c1fix-20261116/`.
  - 47 cases complete and 0 differ. 437,635 verified calls; 63,021,632
    non-stack store bytes identical, with identical EAX, ESP, x87 SW and MXCSR.
  - The 9 stops are the known pre-existing ones:
    - Needle and Street Sweeper 1p/4p: `active_camouflage` shader inputs;
    - 01b: `1CD130` trim owner;
    - 03a, 03b, 05a: scene-copy renderer thread;
    - 08b: `BF050` runtime backing.
  - Ascension, Elongation and Lockout 4p completed this time.
- **Armory parity** (01a, Original, 1280x720, 300 frames): frame
  `5627a51badf47762`, trace `8a9b58b38bef` in three runs:
  - native default: 434 calls, 434 native, 0 declined, 0 refused;
  - `HALO2_REFERENCE_COLLISION_RAY=1`;
  - a third run with `HALO2_COLLISION_RAY_STATS=1` for the counts above.

Use an X display range of your own for sweeps (`LU_SLOT_BASE`). An orphan
`Xvfb` from a cancelled run on the same numbers produced spurious exit-1 stops
in the stage-1 sweep (Colossus 1p, Dune, Headlong, Triplicate 4p). None of them
was a collision-ray difference.
