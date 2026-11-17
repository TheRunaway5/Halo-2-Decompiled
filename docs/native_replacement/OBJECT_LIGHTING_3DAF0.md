# Per-object render lighting `3DAF0` (native, 2026-11-15)

Scope: `3DAF0` and its executed callees. `3DAF0` fills an object's render-cache
lighting bank. Its only original caller is `4B5C0` (`4BA11`), which is now the
native model builder (`source_model_build`); the builder's `lighting` service
in `PhysicsLiveWorldTu15::Impl::model_build_world` calls it for every view,
ordinary and light. See [render commands](RENDER_COMMANDS_ALL_MODES.md) for the
builder and the light-view boundary.

## What runs natively

`native_tu15_object_lighting_3daf0` (`include/halo2/native_tu15_object_lighting_3daf0.hpp`,
`src/rendering/models/native_tu15_object_lighting_3daf0.cpp`) is ordinary C++
control flow over the current banks. Every SSE scalar operation keeps the
original operand order, so NaN payloads and MXCSR flags match. Every x87
sequence is the original one over host copies of its operands, so precision
control, sticky flags and C1 match.

| Original | Role |
|---|---|
| `3DAF0` | default (`+7` bit 0) and probe (`+D0`) banks; root (`BCFB0`) record; child copy of the root bank and samples |
| `3DDC0` | record lookup, `16E6A0` allocation or `16EA60` eviction of the record rendered longest ago, `2C390` defaults, region-bank copies, `3DD00` colors (`BCD80`, `1349E0`) |
| `3DFD0` | query cadence by lighting metric (3/6/9/15/45/60 frames), near skip, frozen roots, query stamp, snap or interpolation |
| `BC200`/`1D8C80`, `24570`, `3EA20`, `BCDF0` | root velocity, speed band, moved test, machine gate |
| `3E590` (`30BD0`, `32CCD8` acos) | incident rotation toward the target, or per-component steps |
| `3E4D0` (`1346C0`, `1349E0`) | packed RGB steps |
| `D65C0` (`D5D30`, `11A360`, `30BD0`) | the 84-byte bank from a sample: authored variable, or the undefined-variable fallback; final clamps |
| `13DE70` | authored scales on the existing native evaluator (`native_tu15_evaluate_authored_function_extended`) |

Services (`NativeTu15ObjectLightingServices`):
- `query`: `D3D90`, the collision/lightmap query (`D4F90` → `14DA70` ray and
  surface UV, `D4350` lightmap sample). It still runs translated, through the
  same CPU-bitmap projection as the original root: the dispatcher's `3DAF0`
  first-row adapter now also admits a `D3D90` root
  (`run_model_3daf0_with_inline_cpu_bitmaps(..., entry)`).
- `function`: translated `13DE70`, used only when the native evaluator refuses.
  The evaluator stores nothing, so the refusal has no effect. Armory and MP
  logged 0 refusals.

`NativeTu15ObjectLightingDecline` is thrown, before any store, when the root
has no record (the `3DC10` static-bank path). That path's sample is an
uninitialized original stack local, so the binding runs the translated
`3DAF0` from the start. It never ran in any workload.

The campaign `model_lighting_correction` is unchanged: the native bank is the
uncorrected original, and the ordinary views (not light views) apply the
correction afterwards, exactly as before.

## Switches

- `HALO2_REFERENCE_OBJECT_LIGHTING=1`: translated `3DAF0` at the service.
- `HALO2_OBJECT_LIGHTING_TRACE=<file>`: per call, the object, metric, returned
  bank and every touched non-stack byte (before the first store, after the
  call), for a cross-run comparison of the two routes.
- `HALO2_OBJECT_LIGHTING_ORACLE=1`: native in an undo journal, then the
  original, comparing every stored byte and the returned bank. A call that
  reaches `D3D90` is skipped and counted, because the query commits and
  re-protects backing that no journal can replay.
- `HALO2_OBJECT_LIGHTING_STATS=1`: path counts and service time.

## Proof

Census slice. Armory (01a, Original, `--campaign-reference-capture .05 0 305419896`,
1280x720, 300 frames, `HALO2_CENSUS_NATIVE_BRIDGES=1`) ran the tree about
11,093 times. Its translated instructions per 300 frames were:
- `3DFD0`, 600 k;
- `3DAF0`, 466 k;
- `D5D30`, 291 k;
- `D65C0`, 101 k;
- `13DE70`, 54 k (lighting share);
- `3E590`, 45 k.

`3DDC0`, `BCFB0`, `BC200`, `BCDF0`, `24570`, `3EA20`, `3E4D0`, `2C390` and
`3DD00` were already instruction-shaped bridges inside the translated unit, so
they were uncounted. They are ordinary native code now too.

Physical original (`tools/validate_native_tu15_object_lighting_3daf0.py`, CTest
`original_tu15_native_object_lighting`). The whole unchanged tree listed above
runs on physical i386 with no patch and no supplied outcome.
- 568 cases, 783,187 original instructions; 508 change persistent bytes.
- Every non-stack byte, EAX and the x87 CW/SW and MXCSR words match.
- Native coverage:
  - 493 roots, 46 children, 16 default and 13 probe banks;
  - 248 allocations, 155 evictions, 403 color publications;
  - 13 unchanged, 9 frozen, 366 due, 10 near skips, 356 queries;
  - 86 interpolations, 23 rotations, 376 snaps;
  - 462 conversions, of which 262 took the undefined-variable fallback.
- Inputs include NaN payloads (48 cases with NaNs in the same current/target
  lanes), infinities, denormals and signed zero; metrics on every cadence
  band boundary; MXCSR round-down/up/zero, DAZ and FTZ.
- ASan/UBSan replay of the same 568 cases passes.
- Out-of-tree mutation screen: 15 of 16 mutants killed. The survivor drops the
  frozen-root branch of `BC200`, which is unreachable from this caller:
  `3DFD0` interpolates only a non-fresh record, and a non-fresh frozen root
  returns at `3E05F` first. Source SHA-256 `7416ee5b9c2a9538…` was unchanged
  afterwards.
- `tools/run_tests.py --target native_tu15_object_lighting_tests --filter
  '^original_tu15_native_object_lighting$'` passes (1/1, 36 s).

Live byte record (`HALO2_OBJECT_LIGHTING_TRACE`), same binary, reference versus
native. Every byte either route touched is compared; a byte only one route
touched must keep its before value:

| Workload | Calls | Bytes compared | Mismatches | Frame (both routes) |
|---|---|---|---|---|
| Armory 01a, 300 frames | 11,093 | 1,082,260 | 0 | `5627a51badf47762`, trace `8a9b58b38bef450d` |
| MP Coagulation 1p, firing, tick 150 | 2,679 | 2,085,889 | 0 | `05f31264…` |
| MP Zanzibar 1p, firing, tick 150 | 11,901 | 3,997,972 | 0 | `e8053beb…` |
| MP Coagulation 4p, firing, tick 150 | 7,433 | 4,201,885 | 0 | `13fa114e…` |
| MP Coagulation 4p, tick 90 | 4,253 | 4,132,657 | 0 | `9c14a330…` |
| MP Coagulation 1p, `--motion`, tick 150 | 1,243 | 2,951,985 | 0 | `7d1a9399…` |
| MP Lockout 1p, `--motion`, tick 150 | 1,269 | 3,741,888 | 0 | `233cb31f…` |

Both routes touched exactly the same bytes in every workload, and every
returned bank matched. The live paths include:
- rotation-path calls: 1 to 73 per firing workload and 240–277 per motion one;
- child copies, cadence and near skips;
- queries (19 to 186 per workload) and conversions.

No route declined, and the native evaluator refused no function.

The two `--motion` rows ran on a later shared binary (03:02). The node-rebuild
default had been narrowed back to translated there. The earlier binary aborted
both routes at tick 21 in that work, on "284090 animation node mask
intersection is not native yet"; its 217 and 161 calls before the abort also
matched. The frame hashes therefore differ from the 4B5C0 record's motion
hashes; reference and native are identical within each binary.

Frame parity under the Khronos validation layer (Armory): reference and native
both give `5627a51badf47762`, trace `8a9b58b38bef`, with 0 validation messages.
The native route repeats that without the layer.

Instruction census, same binary, `HALO2_REFERENCE_OBJECT_LIGHTING=1` versus the
default (Armory, 300 frames, native bridges):
- Translated instructions fall from 403,979 to 398,625 per frame (−1.33%,
  −1.61 M per 300 frames).
- `3DAF0`, `3DFD0`, `D65C0`, `D5D30` and `3E590` reach 0.
- `13DE70` falls from 103,562 to 49,874 (other callers remain).
- `D3D90` (14,450), `D4F90` (14,310), `14DA70` (13,734) and `D4350` (6,711)
  are unchanged.

Same-run store oracle (`HALO2_OBJECT_LIGHTING_ORACLE=1`, Armory 300 frames):
- 10,907 journaled calls, 50,910 stored bytes on each side;
- 0 mismatches and 0 returned-bank mismatches;
- the 186 query calls are skipped (they are in the byte record above);
- frame `5627a51badf47762`.

Timing. Three interleaved Armory pairs, 300 frames under xvfb, same binary,
host load 4.7–13.9. Time inside the lighting service (`service_ns`, 11,093
calls per run):
- reference: 254.0, 254.7 and 243.3 ms, about 22–23 µs per call;
- native: 183.1, 172.0 and 162.3 ms, about 15–16.5 µs per call;
- medians 254.0 → 172.0 ms, −82 ms per 300 frames (−32%, about −0.27 ms per
  frame).

What remains inside the service is mostly the translated `D3D90` query.
Process CPU time is level (medians 48.9 s and 48.9 s): the saving is below that
measure's noise. All six runs reproduced frame `5627a51badf47762` and trace
`8a9b58b38bef450d`.

## Remaining

1. The `D3D90` query service: about 1.6 M translated instructions per 300
   Armory frames, nearly all of it under `14DA70`:
   - `16C620` collision ray (`1F8200`, `1F7F60`, `BA9E0`, …);
   - `14D410` object surface UV (`120FA0`, `1211C0`, `243F20`, `144090`, …);
   - `14D790` BSP surface UV.

   `D3D90`/`D4F90` themselves are about 28 k, and `D4350` sampling about 61 k.
   `native_tu15_query_bound_camera_scene` is a native `16C620`, so a native
   `14DA70` could start there. The surface-UV owners have no native port yet.
   The unexecuted `D3F70` sky fallback (117 static callees) is also behind
   `D3D90`.
2. pStencil `native_incident_tu15` calls `3DDC0` and `3DFD0` through
   `run_actual`. That route reaches `D3D90` without the CPU-bitmap first-row
   projection, which applies only to a `3DAF0` (or now `D3D90`) root.
   - Binding it to `native_tu15_object_lighting_acquire_3ddc0` /
     `native_tu15_object_lighting_update_3dfd0` would send its query through the
     projection, as the ordinary views do.
   - That is a visible change in pStencil mode, so it is left to the pStencil
     owner.
3. The MapPack11 image's model builder (`multiplayer_object_runtime.cpp`)
   still calls its own translated `3DAF0`.
