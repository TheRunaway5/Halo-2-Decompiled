# Compiled original constraint solver

The later [Jacobian production extension](PHYSICS_JACOBIAN_PORT_20261011.md)
adds the verified producers and known callbacks and compares their real outputs
through the solver. This document records the immutable preceding v4 evidence.

The complete paired solver bodies now have a native C++ translation. This is
one component of the unfinished physics engine replacement. The playable
multiplayer simulation still uses the existing host `CollisionWorld` mover.

`include/halo2/physics_solver.hpp` exposes the solver. `src/physics_solver_generated.cpp`
contains 2,494 instructions lowered to direct C++ branches, memory operations,
integer arithmetic, SSE intrinsics and x87 arithmetic. It contains no executable
blob, opcode fetch, production decoder or production emulator. The generator
rejects unsupported instructions. It preserves the source operation order and
uses strict floating-point compilation. Its x87 backend currently requires
the supported GCC/Clang x86 platform; other backends remain unavailable.

The translated owners are `322ED0` (all 17 iteration schemas, warm-start
dispatch and result traversal), `322D60` (initialization), `322AD0` (checksum),
and the `322AB0`/`322AC0` pointer helpers. Full-body comparison authenticates
these routines against the supplied update 1.1 and TU1.5 executables. Production
addresses and data operands are selected from those complete correspondences.
Schema 14's actual callback, the alternate initialization clock and error
owners remain explicit required dependencies. An absent owner throws at the
exact boundary; it cannot fabricate a completed solve.

`SourcePhysicsConstraintStepServices::solver` connects the compiled solver to
the existing `322690` island wrapper and hence its `31CFE0` world entry. Without
that explicit connection the old required-owner boundary remains. Jacobian
production, allocators and alternate body motion classes are still real
dependencies, not replacement values.

The differential fixtures execute untouched original executable instructions.
They compare complete instruction sequences, every ordered write (including
the full retained ABI stack), full read/written banks and return values. The
422 cases per image cover all iteration selectors, all seven warm-start table
destinations, four rounding modes, repeated rows, mixed row chains, cold
initialization through its real checksum and required callback stops. No
initialization flag, instruction, returned solve result or native expected
state is patched into the original. These are declared component arithmetic
inputs. They do not establish that the game's real constraint producers emit
every tested combination, nor Xbox hardware floating-point behavior.

Sixteen further original/native orchestration comparisons execute real
initialization, solver, motion readback, optional profiling and all four scratch
layout/release branches. They deliberately use declared existing arena header
data with zero producer calls. This validates completed orchestration and does
not establish real contact/Jacobian production. Readback RSQRTSS observations
are explicitly emulator-only comparison inputs, never a production estimate
provider.

Evidence is under `analysis/multiplayer/sandbox-source/physics-solver-20261011/`:

- `correspondence-verified/complete-report.json`: 25 complete paired bodies.
- `generated-manifest-v3.json`: source instruction and generated C++ pins.
- `disc-verified`, `u5-verified`: the full solver fixtures.
- `completed-verified`: completed orchestration fixtures.
- Corresponding `*-repeat` directories: independent deterministic reruns.
- `validation.json`: frozen source, fixture, build and check provenance.

The isolated source/build generation is
`analysis/multiplayer/integration/physics-engine-20261010/{source,build,asan}-v4`.
It extends the pinned v3 source with this work only. Other conversations'
concurrent changes and published launchers are not copied into this snapshot.

Remaining engine gates include real Jacobian producers, retained contact packet
production, deferred world mutation, allocator and controller ownership,
vehicle and ragdoll classes, TU1.5 world bindings, production Xbox-compatible
reciprocal-square-root behavior, and live all-map split-screen/LAN validation.
Passing these component checks does not finish the multiplayer physics goal.
