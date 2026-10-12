# Physics engine port: world and motion advancement

The native physics modules now have an original world-step entry point and
connected unconstrained island/motion advancement. **The whole physics engine
is not ported or installed in playable multiplayer.** `Simulation::move_unit`
still uses the host `CollisionWorld` mover. Lockout lift, vehicle handling,
rigid-body settling and ragdolls are not fixed by this extension.

The later [compiled solver extension](PHYSICS_SOLVER_PORT_20261011.md) and
[Jacobian production extension](PHYSICS_JACOBIAN_PORT_20261011.md) connect the
complete solver and verified virtual producers to the island wrapper. Their
diagnostic comparisons include actual producer/solve/readback/release paths;
live contact production and the mover replacement remain unfinished.
The remainder of this document records the preceding v2/v3 extensions.

## Implemented sequence

`source_step_physics_world_1_1` implements the original
`317650 -> 316200 -> 316440` sequence. The collision phase visits current
active islands in ascending order through the existing native `31E2C0`
scheduler. Integration visits them in descending order, retains the original
step/substep fields, deactivation tests, mutation gates, deferred operations,
listener order and stable NULL removal. Missing lower owners stop at their
actual call; no success or solved-body result is substituted.

The world's explicit current-broadphase binding now connects `31E2C0` to
native `31DB70`, current shape bounds, endpoint update and pair management.
Its selected agent binding connects `300870` to native terrain/MOPP queries,
child agents, narrowphase and GJK. The caller retains the current world and
supplies actual lower services. Neither binding can provide a recorded contact
or substitute a completed heap. The original Lockout world collision prefix
matches through the first required packet-frame owner, before contact append.

`31CFE0` executes actions first. Unconstrained bodies advance in descending
order through native `321760` forces/damping and `321600` integration.
Constrained islands can explicitly bind native `322690` orchestration. Its
actual iterative solver and Jacobian producers are still required; the native
code throws when a lower provider is absent. `31D0F0` retains strict clock comparisons, signed-byte
sleep-counter behavior and required real body deactivators.

The motion implementation includes accumulated force times inverse mass,
world gravity, linear/angular damping with the source lower clamp, the
original torque-admission branch, body/world inertia transforms, force/torque
clearing, saved previous velocities/position, position integration, angular
travel, quaternion multiplication/normalization, orientation matrix and
center-of-mass offset. Source SSE operation order and x87 binary32 spills are
retained. No contact, impulse, friction, restitution or lift strength is tuned.

Files: `include/halo2/physics_motion_step.hpp`,
`include/halo2/physics_world_step.hpp`, `src/physics_motion_step.cpp`,
`src/physics_world_step.cpp`, private `src/physics_step_internal.hpp`, and
`tests/physics_world_step_tests.cpp`. Both modules are in `halo2_systems`.

## Constrained motion and island extension (2026-10-11)

Native `3218A0` prepares the original 128-byte dynamic-body accumulator after
applying forces and damping. It transposes the body matrix, transforms angular
velocity to local space, retains untouched accumulator fields and returns the
next accumulator address. Native `321A60` advances pose using averaged solver
velocities, then restores final instantaneous velocities with the source
coordinate transform and lane masks. Both have authenticated TU1.5 operand
bindings. Neither produces a constraint solution.

`physics_constraint_step.hpp/.cpp` translates the entire `322690` orchestration:
the primary arena and three fallback allocation branches, null accumulator,
body preparation, constraint/Jacobian traversal, schema terminator and impulse
bank initialization, iterative solver call, conditional readback, profiling
and scratch release. The world service's optional `constrained_islands` binding
connects it to actual `31CFE0`. Allocator virtuals, alternate motion classes,
Jacobian producers and `322ED0` remain required real lower owners.

The solver-motion correspondence extension authenticates **22 complete paired
bodies**, including full `3218A0`, `321A60`, matrix transpose `2E0D30`, and the
still-unported solver/initialization dependencies. Its report separately pins
the extension tool and base researcher. Full correspondence is evidence for
translation, not a claim that those dependencies have native implementations.

`oracle_physics_solver_motion_20261011.py` executes the untouched original
constructor and preparation/readback routines against each executable. There
are **160 cases, 15,552 ordered persistent writes, 3,276,800 full stored-state
bytes**, including repeated readback and four masked CPU rounding modes.
Readback cases declare their averaged/final velocity fields as arithmetic
inputs; they are not results of a native or original iterative solve.

`oracle_physics_constraint_step_20261011.py` executes **60 original constrained
prefixes, 2,772 ordered persistent writes and 3,937,200 full stored-state bytes**.
These exercise all arena selection branches, real original body preparation,
one/two bodies and both direct/connected island entry. They stop at the actual
iterative solver, Jacobian producer, allocator or alternate-motion entry, with
the exact call-site/stack/argument ABI. The native prefixes are also rerun with
each provider removed. No lower return or completed heap is substituted.
Conditional post-solver readback and scratch-release tails are translated from
the complete listing but **not yet validated through a completed solve**.

Evidence is under
`analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/`;
registered fixtures are `disc-verified`, `u5-verified`, `constraints-verified`.
The earlier world fixtures remain separately pinned. New evidence does not
replace the old validation report or imply playable integration.

## Original evidence and tests

Authority is the unchanged map-pack update executable, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`,
and the supplied TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/research_physics_world_step_20261010.py` verifies **17 complete paired
bodies**, internal branch topology and executable operands. It resolves the
short outer wrapper using the verified identities of both direct callees.
The positive solver's complete listing is retained as a dependency; it is
not counted as implemented native solver code.

`tools/oracle_physics_world_step_20261010.py` executes original instructions
without code patches or supplied contact/solver results. Its declared raw
controls use the real original dynamic motion constructor. They exercise
force/torque branches, damping clamps, varied steps, repeated integration,
multiple bodies/islands, empty world schedules and sleep thresholds. Explicit
unbound virtual destinations measure caller ABI only and supply no result.

`tools/export_physics_world_step_20261010.py` additionally exports genuine
original Lockout state and ordered writes from the frozen original agent
schedule: one complete island integration, the collision prefix to `31DB70`,
and the integration prefix to `313310`. It checks their retained states and
reads against the original timeline. No finished state or callee output is
installed as a native callback. Original stack-resident step input is retained
separately from excluded return addresses/register saves.

Native comparisons cover **167 cases, 10,188 ordered persistent writes and
9,235,322 complete stored-state bytes**: 117 world/island/motion cases against
update1.1, 48 direct motion cases against TU1.5, and two additional connected
world-collision prefixes. Fourteen cases stop at
required owners. Each missing-owner prefix is also executed with its provider
removed, verifying the same writes and rejection instead of completion.
Independent original runs and original-only exports reproduce byte for byte.

The two connected prefixes contain 245 ordered persistent writes. Guest
return-address/register-save stack writes and actual FS exception registration
are excluded as ABI effects; the original executable read/store timeline is
still checked in full before export. The contact-packet boundary remains
because later code would make the omitted stack ABI observable. It is not
cleared, replayed or treated as an empty successful contact.

The selected-image motion APIs bind TU1.5's five actual immutable operand
addresses. Its original code is executed independently; native storage is not
given old-image aliases or constant values to make it pass. The world/allocator
API remains explicitly TU1.1; full TU1.5 mutable-world bindings are unfinished.

Run the production tests with:

```sh
cmake --build <build> --target physics_world_step_tests
ctest --test-dir <build> --output-on-failure \
  -R '^(original_physics_world_step|original_physics_motion_tu15|original_physics_connected_world|original_physics_solver_motion|original_physics_solver_motion_tu15|original_physics_constraint_step)$'
```

Ignored evidence is in
`analysis/multiplayer/sandbox-source/physics-world-step-20261010/`.
`exported-verified-v2`, `u5-verified` and `connected-verified-v3` are the registered fixtures; original
repeats, failed intermediate comparisons, complete listings and correspondence
reports are retained separately. Build/verification pins are in `validation.json`.

RSQRTSS observations are **emulator-only test inputs**. Production callers must
provide the actual platform estimate owner; no captured estimate table is
installed. Host x86 estimates are not established as Xbox hardware parity.
Positive profiling, unconstrained alternate motion classes, successful virtual
listeners/deactivators and deferred-operation tails are source-listed beyond
the executed branches. These comparisons do not prove continuous gameplay,
all constraint types, hardware rounding or all maps.

## Remaining engine work, in dependency order

1. Complete live allocator/world/BSP/body ownership and selected TU1.5 bindings.
   Existing body/shape/component factories must create the actual current
   source world, not restore an oracle's final heap into a match.
2. Finish retained packet-frame ownership in the connected `300870` path.
   Complete contact-manager point allocation, retention and retirement.
3. Complete actual constraint/Jacobian producers and iterative solver `322ED0`, including all
   original schema branches, friction, restitution, limits and warm starting.
   Validate the translated `322690` readback/release tails through real solves.
   Empty contacts or an unconstrained advance cannot stand in for this stage.
4. Complete deferred world mutation, activation/deactivation, island
   merge/split, body retirement and game contact callbacks.
5. Connect original object/biped controllers, readback and update ordering to
   live simulation. Preserve player/body identities, masks, moving supports,
   impulses and LAN authority; remove the temporary host mover only when the
   replacement has real contact/solver ownership.
6. Finish source lift contact/force delivery and teleporter occupancy queries;
   vehicle motion/control classes and seats; grenade attachment/bounce;
   destructive props; and ragdoll constraints/death transitions. Validate actual
   authored map scenarios, including the currently failing Lockout lift.

The separate rendering/shader work and published launchers are not modified.
