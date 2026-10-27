# Native TU1.5 constraint continuation from real solver returns

The typed native `326200` algorithm retains its real continuation state across
the named solver dependency. `NativeTu15ConstraintStepContinuation` holds the
input, first accumulator, fixed body-span end, allocated block and arena end.
`native_tu15_finish_island_constraints` consumes the actual returned solver
status and FP state. Production code supplies no recorded solver output or
fabricated GP/flags state.

The continuation preserves original `3264E7..32659C` behavior: optional profile
label and signed x87 count conversion, commit only for status exactly one,
genuine motion-owner reload and cursor propagation, workspace active-byte
clearing, allocator identity/marker rereads, rounded rewind or actual virtual
release. Fixed commit `324800` advances the actual accumulator by `80` bytes.
Other commits and release `2E1110` remain named dependencies at this layer.

## Independent complete original execution

`tools/observe_native_constraint_continuation_tu15.cpp` physically single-steps
unchanged i386 TU1.5 code. The original root executes its actual `326A40` solver
call. At the real return instruction `3264E7`, the observer captures original
memory, FP control/status, x87 tag, actual EAX, and real private root locals.
It does not replace an instruction or supply a solver result. Original
execution continues to root return, an actual unconverted callee entry, or
the declared read-fault prefix.

The fixture explicitly declares initialized solver input byte `48D480=1` and
configuration iteration count zero or negative. The unchanged original core
executes its real empty, skip-schema or one-row tail and produces status one.
The one-row tail writes actual result words and exceptional floating-point
outputs into an aliased island count before original profiling executes. This
qualifies the native continuation from genuine service-return state; it does
not qualify a native solver or positive-iteration solver behavior.

`build/test-results/native-constraint-continuation-tu15/report.json` pins image
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
the 306-site root, 2,208-site core and required physical backend
`unchanged-tu15-full-constraint-call-genuine-solver-return-v1`:

- 185 genuine original solver-return continuations, including 124 full root
  completions, four genuine dependency-entry stops and 57 read-fault prefixes.
- 24 actual one-row tail cases, signed/rounding controls, exceptional payloads,
  fixed-body spans, and all three arena spill branches.
- Profile output aliases against island count and allocator cursor/remaining
  fields, proving the original publication and reload order.
- 682 ordered postcore stores, 1,937 ordered reads, 46,583 complete-prefix/core
  original instructions and 8,939 postcore instructions.
- Exact persistent bank bytes, actual store PCs and ordering, numerical x87
  control/status, MXCSR, genuine callee arguments and cursor propagation.
- GCC 16.2.1 and Clang 23.1.4 strict-FP Release passes. A private mutation that
  changes the real FILD input count by one is rejected by unchanged original
  ordered stores.

The parser rejects missing, empty or mismatched authority and requires the
observed empty x87 tag at the original service-return boundary. Native host
x87/MMX and SSE state is restored after each completed or faulted call.

```sh
.tools/python/bin/python tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_constraint_continuation_tu15.py \
  --output build/test-results/native-constraint-continuation-tu15
```

The 100,418,889-byte generated fixture, all original binaries/banks, objects and
both private compiler builds were removed with scratch `halo2-test-s4drnod8`.
The initial refresh scratch `/tmp/halo2-test-9amnl3an` was also removed. No large
artifact was retained; bounded reports and source hashes remain.

Actual initializer refusal/status-zero continuation, positive-iteration core
schemas, native `326A40` closure, nonfixed commit execution, virtual release
execution, unmasked platform traps, full live constrained world comparison and
visible multiplayer remain open gates. This proof changes no live raw-byte
comparison or temporary-memory exclusion.
