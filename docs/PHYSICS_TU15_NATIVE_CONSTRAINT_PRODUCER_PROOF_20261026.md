# Native TU1.5 constraint producer composition

The native `31F770` axis-factor and `31F860` scaled-accumulator producers now
have independent physical composition proof through their complete known child
chains. They construct real Jacobian rows and emitted schema data. No recorded
row, cursor, property lookup result, builder result or callback outcome is an
input.

## Authenticated source and API

The original English TU1.5 image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Source listings are retained in
`analysis/physics/native-constraint-producer-inventory-tu15-20261026/`.

| Original owner | Authenticated sites | Native operation |
| --- | ---: | --- |
| `31F770` | 57 | Axis-factor producer |
| `31F860` | 125 | Scaled-accumulator producer |
| `325EF0` | 15 | Read real base inputs and construct |
| `1D7630` | 20 | First property `2003` lookup |
| `3296E0` | 20 | Begin schema header |
| `329720` | 921 | Complete base-row construction |
| `32A390` | 15 | Schema14 emission |

The wrappers consume actual object, input and builder identities. Their typed
`build_base(request, fp)` dependency is bound in the proof directly to
`native_tu15_build_constraint_base`. Emission calls the proper native emitter
directly. The original reference executes all of its genuine nested functions
without instruction patches or supplied returns.

| Stable source | SHA-256 |
| --- | --- |
| `src/physics/native_tu15_constraint_producer.cpp` | `68192bdd9b6675d0f4898e394333872e4d784cd2b0afd9b4e7ef64146b5131cb` |
| `include/halo2/native_tu15_constraint_producer.hpp` | `70dc0dd455b6eb11edcbb5ee9c8512269bc2efa3d6742ccc4bb3c129d2200453` |
| `src/physics/native_tu15_constraint_base.cpp` | `345217d2f9d74395faaaf8e2f196890daf906fa31a06016f8414887a9294c347` |
| `include/halo2/native_tu15_constraint_base.hpp` | `a712fb4ef236841179520d53e3691cbe67bde0f617b518a881cdb7a1c903e447` |
| `src/physics/native_tu15_constraint_schema_emitter.cpp` | `c86079b224b5bfe00627201ff56321a08e26dba2b39b0c91a32c5350426771e2` |
| `include/halo2/native_tu15_constraint_schema_emitter.hpp` | `d9632814dd0eed2849159bc10e6d98611733f73821e88ef56ee58facc5d6cc31` |

## Physical differential result

`build/test-results/native-constraint-producer-first-tu15/report.json` is GREEN
with GCC 16.2.1 and Clang 23.1.4. `coverage.json` records all 1,173 authenticated
sites exercised by the original complete input cases.

| Check | Result |
| --- | ---: |
| Original calls and bounded prefixes | 922 |
| Complete calls | 222 |
| Read rejection prefixes | 386 |
| Store rejection prefixes | 157 |
| Completed-store observer rejection prefixes | 157 |
| Original ordered reads | 106,429 |
| Original ordered stores | 41,514 |
| Unchanged original instructions | 576,454 |

All retained bytes, ordered source reads, ordered public store PCs and
before/written bytes, x87 control/status and MXCSR match. The native driver also
guards the complete 108-byte host FP state and host MXCSR. Property variants
exercise absent, missing, first, late, duplicate and signed-negative counts.
The suite includes normal and aggregate base construction, shared accumulator
identities, source scale/NaN/control variants and interruption inside the real
base/emitter chain. Private mutations of factor indexing, restore destination
and property key are rejected by both compilers.

The eight source accumulator words are read in the actual source order
`A0,A1,A2,B0,A3,B1,B3,B2`. Scaling and subsequent stores remain ordered. The
original restores those words only after completed base construction; the
native implementation does not invent exception cleanup or restore scaled
values on a rejected store. Coupled outputs and direct schema14 payloads are
computed from current source inputs.

## Integration and limits

The producer wrappers' own maximum private x87 depth is one. They keep no
private FP value live across `build_base`. The complete base reaches private
depth seven, so any full producer invocation must preserve at most **one**
genuine incoming value. The wrapper's isolated preserve-seven capacity must
not be applied to its base dependency.

The declared ABI consists of ordinary native value arguments and private cached
locals. It excludes guest control-frame aliases. Transient source private raw
x87 payload on exceptional return, original platform exception/unwind behavior
and unmasked FP traps remain open, as in the separate base proof.

Direct producer emission does not execute the generated `32A390` consumer seam.
Live activity must record `NativeTu15ConstraintProducerStats::emissions`
separately and must not credit the old consumer ABI scope as if it had run.
This proof qualifies native computation; live world binding, per-world activity,
actual MCC inputs, visible multiplayer and complete physics parity require
their separate runtime evidence.

## Storage lifecycle

The whole proof uses `tools/run_test.py`; compiler validation uses disposable
`tools/run_tests.py` builds with one build job. The owner
`.local/test-scratch/halo2-test-hrb9ubqi` was removed, including 92,819,456
allocated temporary bytes measured after inner build cleanup. `cleanup.json`
records removal. Small source hashes, commands, coverage and result logs remain;
no new large artifact was retained.
