# Adaptive integer-only GPU skin admission

2026-10-23. This replaces the fixed minimum-magnitude qualification with a proved per-vertex exponent qualification in `include/halo2/model_skin_gpu_admission.hpp`. Shader arithmetic, GPU service, existing tests and host admission were not edited by this subtask. Root owns snapshot refresh and test execution; no compiler, numerical probe or runtime was started here.

## Observed reason for the change

`build/test-results/presentation-offload-20261022/gpu-real-rejection/tests.log` records the failed genuine-MCC handoff gate: Master Chief 18,516, Elite 16,884 and SMG 4,140 selected records all took CPU fallback. Every post-palette host state was admitted (`MXCSR=0x1fa0`). The numeric failure was the former fixed minimum, not a missing host/device control:

| Owner | Minimum nonzero attribute magnitude | Minimum nonzero palette magnitude | Minimum nonzero weight |
|---|---|---|---|
| Master Chief | `2.09999995e-9`, bits `31104f97` | `2.32830644e-9`, bits `31200000` | `0.101960786` |
| Elite | `2.49999998e-9`, bits `312bcc77` | `2.43278768e-15`, bits `272f4d03` (row14/component1) | `0.101960786` |
| SMG | `2.65117789e-10`, bits `2f91c003` | `0.0700000003` | `1` |

These values are normal binary32. Replacing constants with smaller guessed constants would not prove intermediate behavior. The new predicate examines only the palette references actually used by each vertex, while preserving every active/interior-zero bone check. **New actual MCC admission remains a test requirement**, not a result of this proof note.

## Preconditions and proof

This proof applies to the exact existing kernel in `src/rendering/models/model_skin_vertices.hpp:58–83`: descending influence products, first product initializes each row, separately rounded adds, `(x+y)+z`, translation only for position. Explicit float32 RTE, signed-zero preservation and NoContraction remain mandatory; no relaxed math or algebraic reduction is admitted. [Vulkan arithmetic requirements](https://docs.vulkan.org/spec/latest/appendices/spirvenv.html). Binary32 has 24 significant bits and normal magnitudes start at `2^-126`. [Intel SDM volume1, table4-2](https://cdrdv2-public.intel.com/843827/253665-sdm-vol-1-dec-24.pdf).

Every evaluated palette operand, weight and deformation attribute must be **normal or signed zero**. Upper magnitudes remain palette/attribute `2^20` and weight `1`; at most four influences exist. All stored palette data retains the finite upper-bound sanity check, but unused rows have no minimum: the shader reads uint words only until a referenced arithmetic load. Every referenced row, including a row multiplied by an interior zero weight, must be normal/zero. This avoids introducing or removing the host denormal-operand exception. Unused tail bones remain unread, as in the original kernel.

For each vertex, extract unbiased exponents by integer operations on absolute binary32 encodings. Let:

* `p` be the minimum exponent of nonzero palette components referenced by **nonzero** weights;
* `w` be the minimum exponent of nonzero weights;
* `a` be the minimum exponent of the twelve nonzero deformation components;
* `n` be the number of nonzero weights (sign does not matter).

For a nonzero normal value with exponent `e`, magnitude is at least `2^e`. Thus an exact palette×weight product has magnitude at least `2^(p+w)`. If that lower bound is normal, monotone RTE cannot round it below the representable power-of-two bound. Each rounded product is a binary32 lattice point at spacing no finer than `2^(p+w-23)`.

For `n>=2`, use **`b=p+w-23`**. Require `b>=-126`; each product and every descending partial sum lies on that normal lattice. Cancellation either produces signed zero or magnitude at least `2^b`. The lattice is closed under RTE: small lattice points are exactly representable; larger values round to a coarser power-of-two lattice. No partial sum can underflow.

For `n==1`, use **`b=p+w`** as a **magnitude bound**, not a lattice claim. The sole nonzero influence cannot suffer nonzero blend cancellation; interior zero products/adds can affect zero signs but not reduce a nonzero row's magnitude. Requiring `b>=-126` therefore ensures every row is normal/zero.

For either case, row×attribute products have nonzero magnitude at least `2^(b+a)`. Require **`q=b+a-23>=-126`**. Each separately rounded product lies on lattice `2^q`, and both DP3 partial sums remain normal/zero on that lattice. If every deformation attribute is zero, skip this test: there is no nonzero dot-product cancellation. If every contributing palette component is zero, all evaluated products/sums are signed zero after the referenced-operand checks.

**DPH translation needs one additional check for `n==1`.** A magnitude bound `b` does not imply translation lies on lattice `2^b`. Let `t` be the minimum exponent of nonzero translation coefficients from the sole nonzero influence. When position has any nonzero component and translation is nonzero, require **`t+w-23>=-126`**. That bounds its rounded translation lattice. Adding it to the normal dot lattice cannot produce a subnormal. If position is entirely signed zero, its dot is zero and adding a normal translation cannot cancel. For `n>=2`, translation already lies on lattice `2^b`, so the original `b`/`q` checks suffice.

Upper bounds are unchanged: each row product is at most `2^20`, four blended terms at most `2^22`, each transformed product at most `2^42`, and the final three-term sum plus translation is below `2^44`. Their representable ceilings prevent overflow. Therefore every admitted arithmetic operand/result is normal/zero, and the only possible host exception is precision/inexact. The existing all-masked/RTE/already-set-precision host gate remains required; x87/errno/diagnostic/fault ownership remains a separate existing contract. No floating-point arithmetic or comparison is used in qualification itself.

The old fixed domain is a subset: multiple-influence `p>=-40,w>=-10,a>=-24` gives `b>=-73,q>=-120`; one influence gives `b>=-50,q>=-97` and translation lattice at least `2^-73`. Existing `2^-120` GPU boundary cases stay admitted.

## Exact boundary and counterexample fixtures

All unmentioned palette rows/components and attributes are zero; preserve the original descending order. `bits(x)` below means `bit_cast<float>(x)`; these constructions require no decimal rounding assumptions.

| Case | Inputs | Expected |
|---|---|---|
| Smallest normal DP3 cancellation | One weight1, palette row0 `{1,-1,0,0}`, position `{bits(0x0c000001),bits(0x0c000000),0}` | `q=-126`, admit; x−y is exactly `+2^-126`. |
| First smaller DP3 lattice | Same, position `{bits(0x0b800001),bits(0x0b800000),0}` | `q=-127`, reject; exact result `+2^-127` is subnormal. |
| Smallest normal blend cancellation | Two weight1 bones; only component3 nonzero: bone0 `bits(0x0c000001)`, bone1 `bits(0x8c000000)`; all attributes0 | `b=-126`, admit; final position.x is `+2^-126`. |
| First smaller blend lattice | Same, components `bits(0x0b800001)` / `bits(0x8b800000)` | `b=-127`, reject; translation is exactly `+2^-127`. |
| Necessary one-influence translation guard | One weight1; row0 `{bits(0x0b800000),bits(0x8b800000),0,bits(0x8b800001)}`, position `{3,2,0}` | `b=-104,q=-126` alone would admit, but dot equals `2^-104` and DPH yields exactly `-2^-127`. New translation guard rejects (`t+w-23=-127`). |

The last construction is the counterexample found during independent review of the proposed two-inequality rule; the implemented predicate includes its repair. Tests must also pin signed zero, all-zero influence/attribute banks, ignored invalid tail bones versus checked zero interior bones, unused subnormal palette data versus referenced subnormal operands, NaN/Inf rejection without FP effects, maximum bounds, and exact preservation of old admitted cases.

## Required integration/test follow-through

Root must update `ModelSkinGpu::prepare` consistently: whole palette finite-upper sanity; weights normal/zero within1; every referenced palette component normal/zero within`2^20`, including interior zero-weight references. That service cannot inspect destination vertex values; the full per-vertex arithmetic proof still belongs at producer/`FrameModelSkinPlan` admission. Do not merely broaden its old fixed-range checks while retaining an old full-predicate receipt.

Root owns existing test updates in `tests/model_skin_handoff_tests.cpp:105–126` and `tests/model_skin_gpu_tests.cpp:200–210`, plus these exact boundaries, complete vertex-byte GPU readback, full SSE/x87/errno effects, host fallback and nonzero genuine-MCC handoff work. The first failed receipt identifies real inputs; it does not prove this new predicate passes all three models. Any remaining rejection must be diagnosed rather than bypassed.

At this edit, admission-header SHA-256 is `7a5539bda19f59738aa84d01a33afaed5414fe50fafb940ebdcc92194410fc0a`. Shader/service were unchanged by this subtask; no build/test scratch was created.
