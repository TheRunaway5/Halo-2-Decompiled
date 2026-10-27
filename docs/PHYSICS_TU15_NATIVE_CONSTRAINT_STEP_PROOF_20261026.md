# Native TU1.5 constrained-island orchestration proof

Actual `326200` (306 sites) is implemented as a typed native algorithm in
`native_tu15_constraint_step.hpp/.cpp`. It preserves workspace partition and
spill allocation, fixed accumulator initialization, fixed-span body setup,
constraint pair publication, mutable schema/Jacobian context, result clearing,
core solver dispatch, profiling, conditional motion commit, and buffer cleanup.
It executes no virtual CPU or register/flags machine and supplies no guessed
solver or allocation outcome.

The actual caller `320B70` supplies nine arguments in this order: step packet,
solver configuration, Jacobian input, gravity, island, body-pointer span, body
count, constraint-pointer span, and constraint count. Constrained motion uses
vtable slots `+0C` for accumulator setup and `+10` for commit. Those are distinct
original owners from free-step force `+8` and pose `+4`; substituting the latter
would change the algorithm.

The original motion table matrix is:

| Table | Accumulator setup | Motion commit |
| --- | --- | --- |
| `41F1E0` | `323560` | `3235F0` |
| `41F260` | `3248D0` | `324800` |
| `41F2E0` | `325410` | `3249E0` |
| `41F360` | `325410` | `3255D0` |

Native setup `323560` (25 sites) copies prescribed position/velocities and
constructs its accumulator identity axes. Native setup `3248D0` (17 sites)
constructs a fixed accumulator. Actual `324800` (three sites) advances the
accumulator cursor by `80` bytes. Those closed leaves preserve original store
and input-read order and perform no FP arithmetic. Their native cursor result
is consumed directly; it is not a fabricated CPU postimage.

The root keeps source lifetime and reload decisions explicit. The initial
workspace descriptor is reloaded after setting its active flag. Three arena
spill cases use the original rounded allocator cursor/remaining updates or
genuine allocator virtual calls. Body span bounds are fixed before setup.
Motion offsets are published before virtual setup. Each constraint publishes
its actual two accumulator addresses before dispatch. Builder context fields
are the real four-word sequence: Jacobian cursor, schema cursor, Jacobian
limit, secondary Jacobian start. The virtual table is captured before the
receiver is reloaded, and the owner slot is read afterward. Result count is
reread during clearing. Commit occurs only when the actual solver status is
exactly one, and motion owners are reloaded then. Cleanup rereads workspace
and allocator identities and follows the original marker/rewind/free branch.

The native profile conversion uses real x87 `FILD/FSTP`, preserving signed
conversion, original rounding, and numerical flags. Genuine dependency
callbacks carry the current FP context. A residual binding must retain a real
four-word builder packet through all builder calls, recover actual modified
fields, and preserve the caller's real live x87 context. It must not invent
solver results or volatile GP outputs.

## Independent original prefix evidence

`build/test-results/native-constraint-step-tu15/report.json` records unchanged
physical i386 execution from English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

- 229 original prefix cases and 69 original read-fault prefixes.
- 4,530 ordered original stores, 4,359 ordered reads, and 30,355 instructions.
- Fixed and prescribed accumulator setup composed into the original root,
  all three spill branches, result count variants, and exceptional vector
  payload copies with unchanged FP status.
- Actual physical receiver/stack arguments at dynamic setup, allocator,
  constraint builder and core solver entries. The builder's private four-word
  packet is observed from actual stack storage, not calculated by the oracle.
- GCC 16.2.1 and Clang 23.1.4 Release passes under strict FP options.
- An unchanged-original negative control rejecting a missing `400` schema-end
  marker in a private mutated source copy.

The original constraint builder table `41ED90` and owner `31F770` are
authenticated by constructor store `31F728`. All unconverted calls stop at
their genuine entry before execution. The required fixture parser rejects
missing fixtures or an absent/mismatched pinned physical backend.

Mutating callback models additionally exercise actual allocation/cleanup
parameters, changing workspace identity, secondary-Jacobian rollover, schema
publication, genuine setup cursor propagation, post-solver owner reloads,
fixed native commit, and profile rounding/flags. These model checks do not
qualify original execution after the core solver call.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_constraint_step_tu15.py \
  --output build/test-results/native-constraint-step-tu15
```

The 79,703,031-byte raw fixture, original ELF/observer, objects, and both private
compiler builds were removed with `.local/test-scratch/halo2-test-90lfjelv` in
the source refresh after extracting the typed continuation. That path and the
earlier `halo2-test-5h2uutn7` were checked absent; all five report source hashes
match. No large artifact was retained.

## Remaining closure

Core `326A40` contains 2,208 sites with three opcode dispatch tables and remains
an explicit actual solver module. Its original initialization `3268D0` has 125
sites and calls `326640`, `2E3850`, and `37C5C0`; core row cursor helpers are
`326620`/`326630` and a custom virtual solver. Dynamic setup `325410` has 124
sites and calls `3250A0` and `2E4820` (24 sites). Those dynamic setup owners now
have a separate closed native proof in
[the dynamic accumulator report](PHYSICS_TU15_NATIVE_DYNAMIC_ACCUMULATOR_PROOF_20261026.md).
The commit owners with 88, 129 and 156 sites now have their own
[native math proof](PHYSICS_TU15_NATIVE_MOTION_COMMIT_PROOF_20261026.md) and
[composition proof](PHYSICS_TU15_NATIVE_CONSTRAINT_COMMIT_COMPOSITION_PROOF_20261026.md)
after genuine original core returns. Custom constraint builders and allocator
virtual operations remain explicit.

Post-core profiling, fixed commit, rewind and real free/other-commit entry
boundaries now have independent unchanged-original evidence from genuine
solver return states in
[the continuation proof](PHYSICS_TU15_NATIVE_CONSTRAINT_CONTINUATION_PROOF_20261026.md).
That proof executes the actual initialized core's zero-iteration schema tail;
it does not close native solver execution or positive solver iterations.

Native core execution, custom builder execution,
virtual allocation/release execution, live binding, full-world comparison,
visible multiplayer and native-only complete physics closure remain open.
This component proof changes no raw live-world comparison or temporary-memory
exclusion.
