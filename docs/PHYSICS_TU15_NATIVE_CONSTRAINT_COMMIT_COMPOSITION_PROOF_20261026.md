# Native TU1.5 constraint continuation with all motion commits

This proof composes the native `326200` post-solver continuation with native
commit owners `3235F0`, `3249E0` and `3255D0`. It starts from genuine return
state produced by complete unchanged original calls: the physical original
executes real accumulator setup, real `326A40`, profiling, commits and cleanup.
Dynamic original setup includes `325410`, `2E4820` and `3250A0`. Native code
receives the actual solver-return memory, result, retained semantic locals and
FP state; no success or commit outcome is supplied.

The initialized original core executes its actual zero-iteration schema tail.
Nine one-row cases write genuine solver outputs into working accumulator lanes,
which the native commit then consumes. Other cases cover one/three-body spans,
all four arena partition modes, commit parameters, paired NaNs, step aliases
and both original dynamic motion tables. Fixed-span traversal propagates each
real native returned accumulator to the next motion.

`build/test-results/native-constraint-commit-continuation-tu15/report.json` pins
TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` and required
backend `unchanged-tu15-full-constraint-genuine-return-native-motion-commits-v1`:

- 277 genuine solver-return compositions: 96 complete original root calls and
  181 postcore read-fault prefixes.
- 7,360 ordered stores, 16,288 ordered reads, 238,210 full original instructions
  and 101,795 postcore instructions.
- Exact retained bank bytes, source store PCs/order, reads and numerical FP
  state across profile, all three commit kernels and allocator rewind.
- GCC 16.2.1 and Clang 23.1.4 strict-FP Release passes.
- A private mutation changing the real status-one commit condition to two is
  rejected by unchanged original commit stores.

The parser requires genuine empty-stack solver-return provenance and pinned
root/core/commit ownership. Native services call real typed commit kernels,
return their actual cursors, and pass current FP state. There are no recorded
commit results or fabricated CPU outputs.

```sh
.tools/python/bin/python tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_constraint_commit_continuation_tu15.py \
  --output build/test-results/native-constraint-commit-continuation-tu15
```

The 158,582,199-byte generated fixture, original binaries/banks, objects and both
private compiler builds were removed with scratch `halo2-test-znqnrhpb`. The
path was checked absent, all ten report source hashes match, and bounded
retained reports total 14,163 bytes. No large artifact was retained.

Native core `326A40` closure, positive-iteration schemas, original initializer
refusal/status zero, unmasked traps, Xbox/PentiumIII RSQRT estimate bits, full
live constrained world comparison and visible multiplayer remain open. This
component proof changes no live raw-byte gate or memory exclusion.
