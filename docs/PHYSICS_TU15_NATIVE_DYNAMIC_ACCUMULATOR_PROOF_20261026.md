# Native TU1.5 dynamic constraint accumulator proof

`native_tu15_dynamic_accumulator.hpp/.cpp` implements complete actual `325410`
(124 sites) and its ordered matrix transpose `2E4820` (24 sites). The source
calls the existing native angular impulse `3250A0` (53 sites) at its actual
call site. Free-step force `3252D0` is a different algorithm and is not used.

The typed storage interface names motion, step, gravity and output-accumulator
fields. It preserves damping/clamp grouping, torque-dependent dispatch, force
and torque clearing, accumulator type and flags, inverse inertia, velocity,
position and rotation stores. The native return is the actual next accumulator
address, output plus `80` bytes. There is no virtual GP/flags machine and no
supplied solver result.

Scalar and SIMD operations retain original operand order, including paired
NaN payload priority. Native x87 keeps the actual live `dt` value across the
angular child call; the scope publishes numerical FP state before the child
and resumes its retained value/tags while adopting the child's real FP state.
Fault unwinding captures native numerical flags and restores the host's saved
x87/MMX and SSE state. Source reads, component stores and whole-vector stores
keep their actual ordering, including when motion, accumulator or step fields
alias.

`build/test-results/native-dynamic-accumulator-tu15/report.json` records
unchanged physical i386 original execution from TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

- 322 cases: 248 complete calls and 74 original read-fault prefixes.
- 32 paired NaN cases, 6,736 ordered stores, 11,363 ordered reads and 52,370
  original instructions.
- Exact original returned cursor, retained bank bytes, numerical FP control
  and status, ordered read/store addresses and actual store-site identities.
- Output/motion aliases, step aliases against velocity/torque/indicator fields,
  and a model whose gravity changes at the actual observation boundary.
- GCC 16.2.1 and Clang 23.1.4 Release passes with strict FP options.
- A private wrong-destination transpose mutation rejected by the unchanged
  original ordered-store oracle.

The required parser pins the physical backend
`unchanged-tu15-dynamic-accumulator-physical-operands-full-fp-v1`; missing or
mismatched evidence is a failure.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_dynamic_accumulator_tu15.py \
  --output build/test-results/native-dynamic-accumulator-tu15
```

The 12,032,292-byte generated fixture, original ELF/observer and both private
compiler builds were removed with scratch `halo2-test-lbvx3ucf`. The earlier
private negative-harness scratch `halo2-test-ggun23rp` was also removed. Both
paths were checked absent, all seven recorded source hashes match, and the
small retained reports total 12,140 bytes. No large artifact was retained.

Unmasked platform traps remain unqualified. Torque-enabled incoming x87 depth
five exceeds the reused angular helper's declared depth limit; the real world
boundary at depth zero is covered. Production binding, complete constrained
solver/commit composition, live world comparison and visible multiplayer
acceptance remain separate gates.
