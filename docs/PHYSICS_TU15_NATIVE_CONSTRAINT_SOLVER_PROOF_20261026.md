# Native TU1.5 constraint row solver proof

`native_tu15_constraint_solver.hpp`, `native_tu15_constraint_solver.cpp` and
`native_tu15_constraint_solver_rows.cpp` implement actual `326A40` as typed
native schema and motion math. The private access/header-state definitions are
in `native_tu15_constraint_solver_internal.hpp`. The algorithm consumes real
configuration, schema, accumulator and result-slot identities. It does not
execute the old solver behind the native implementation or maintain a guest
register/flags machine.

The native loop preserves signed iteration limits and their reloads, source
header read order, the first copy and later additive accumulator warm starts,
secondary velocity projections and final result publication. Schema types
`15/16` retain one saved configuration pair with original store order. This is
the game's physics schema format, with genuine row widths and cursor advances.

All twelve built-in primary row families `2..13` are closed native math:
positive-only/full and bilateral/angular rows, caps, two-sided limits, clamped
rows, coupled unilateral pairs, radially capped pairs and coupled triples.
Source first/repeated pair paths remain distinct. The radial pair retains
`sqrt(cap²/norm²)`; the triple retains `cap/sqrt(norm²)` and its separate third
weight. Ordered SSE instructions preserve grouping and NaN operand priority.
Real extended x87 impulses survive their binary32 `FST` spill for vector
updates and are then used directly in result accumulation.

## Independent physical original evidence

`build/test-results/native-constraint-solver-tu15/report.json` pins unchanged
TU1.5 image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` and backend
`unchanged-tu15-constraint-solver-physical-operands-full-fp-v1`:

- 3,000 physical original cases, including 1,923 read-fault prefixes.
- Every built-in primary family, every secondary class and all eight final
  export classes; zero to three iterations; repeated pairs and pair-to-triple
  transitions; header fast paths and one-slot configuration save/restore.
- Finite and exceptional values, signed zeros/denormals, paired NaN payloads,
  cap/limit controls and isolated-pair branch controls.
- 49,155 ordered stores, 296,611 ordered reads and 1,110,116 original
  instructions. Tests compare actual returned status, all declared retained
  bank bytes, original store PCs/order and complete numerical x87/MXCSR state.
- 2,189 of 2,208 original root instruction sites physically visited. The only
  19 unvisited sites belong to post-initializer return/status-zero handling and
  the custom-schema callback call/return sequence. Those services remain
  explicit dependencies at this proof boundary.
- GCC 16.2.1 and Clang 23.1.4 strict-FP Release passes.
- Private negative controls rejected by unchanged original: replacing later
  additive warm starts with copies, and rounding away the retained extended
  impulse by replacing `FST` with `FSTP/FLD` of its binary32 spill.

The required parser rejects missing or mismatched original authority. Tests
start from an empty true x87 stack with varied TOP/status, precision, rounding,
DAZ and FTZ. Source fault comparisons also check temporary x87 depth through
status TOP, which caught and corrected a triple-row read occurring before its
real private `FLD`.

`tools/observe_native_constraint_solver_tu15.cpp` single-steps unchanged i386
original instructions and records source memory operands and instruction-site
coverage. It supplies no physics result. Source snapshots, generated reference
executables, banks and compiler outputs are owned by `tools/run_test.py`.

```sh
.tools/python/bin/python tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_constraint_solver_tu15.py \
  --output build/test-results/native-constraint-solver-tu15
```

The 172,392,790-byte final fixture and all reference executables, objects and
compiler builds were removed with scratch `halo2-test-2uvrjwn5`. That path was
checked absent. All seven recorded source hashes match the qualified snapshot.
The bounded report tree, including diagnostic receipts, totals 106,327 bytes.
No large artifact was retained.

## ABI and remaining service boundary

`build/test-results/native-constraint-solver-abi-tu15/report.json` separately
audits all 2,208 source CFG sites with partial-GP and SIMD-lane must-definition
tracking. Meaningful inputs are the real stack identity/four arguments and
explicit FP context. Initializer and custom callbacks conservatively clobber
volatile GP/XMM values; the original root still has no undefined incoming
GP/SIMD dependency. Callee-save pushes are preservation, not physics inputs.

A structural x87 walk finds maximum private depth six and private depth zero
at all eight calls. The native entry therefore diagnoses incoming depth above
two. Actual initializer/custom callbacks must preserve genuine incoming live
x87 values/depth; the residual binding must diagnose an unsupported callee
contract. The custom call passes `(jacobian, schema+8)`, uses caller cleanup
and consumes no return value. Those caller facts do not prove an arbitrary
selected custom callee has no additional implicit inputs; each real owner
still needs authentication.

Initializer `3268D0` and hash `326640` now have a separate
[native initializer proof](PHYSICS_TU15_NATIVE_SOLVER_INITIALIZER_PROOF_20261026.md).
Complete native core/initializer composition, custom callback owners, genuine
logger execution, platform clock closure, unmasked traps, malformed accumulator
tags, full live world comparison and visible multiplayer are separate gates.
No raw live-memory exclusion or comparison gate changed in this work.
