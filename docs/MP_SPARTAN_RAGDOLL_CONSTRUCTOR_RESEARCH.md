# Original Spartan ten-body constructor

The unchanged original constructor builds the actual dead Spartan's ten rigid
bodies and nine constraints without supplied physics results. This is a
separately declared constructor-root experiment. The natural contact/+34B
admission, world insertion, solver updates and rendered ragdoll remain required.
No standing corpse or substitute death clip is installed.

## Actual input and ordering

Authority is TU1.1 original executable SHA256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and original Lockout/shared cache bytes. The new tool is
[oracle_mp_spartan_ragdoll_constructor_1_1.py](../tools/oracle_mp_spartan_ragdoll_constructor_1_1.py).
It first repeats every stage, call, read and store of the
[frozen moving-death experiment](MP_SPARTAN_MOVING_DEATH_RESEARCH.md)
byte-identically. That experiment's declared axes/heading, selected original
startup/platform context, inherited timestep and audiovisual boundaries remain
explicit. No live device or whole-title frame admission is inferred.

Only after this complete comparison does the new experiment invoke unchanged
original **1D4C50 → 1D3BB0 → 1D9330(component,0,0,1)**. The first two remove and
dispose the actual retained dead controller. The third uses the exact callee
arguments pushed by the positive `1DA570` branch at `1DA616..1DA61D`. That branch
was not admitted in the earlier ordinary death execution; calling its callee
explicitly does not change unit+34B or prove that missing admission.
[Complete 1DA570 and 1D9330 instructions](../analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/measured/instructions/001da570.asm).

The actual retained identity is unit **E26F0000**, raw unit **82D87FC0**, full
component datum **E1680000**, raw component **06200050**. Python resolves those
pointers from the source-created object/component pools using the original
layout; it does not assign them to guest storage. Unit state+3DC is3 and mode+34B
is0. The actual selected `bipd E9F2086D → hlmt E9F3086E → phmo E94E07C9`
postprocess and original current animation node bank precede construction.
[Actual phmo inventory](MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md).

No source mode, pose, node, shape, constraint frame, root, contact, world/PVS
membership, clock, allocation success or return is supplied. All three new roots
return with **zero supplied services**. Original allocator, shape, current-node,
body, slot, constraint and reference operations execute unchanged.

## Measured output

| Original stage | Body count | Constraint count | World membership |
| --- | ---: | ---: | --- |
| 1D4C50 return | 1 | 0 | Controller removed |
| 1D3BB0 return | 0 | 0 | Removed |
| 1D9330 return | 10 | 9 | New bodies have NULL world |

Complete 1D9330 returns EAX1, `RET16` (ESP delta20 including the return address).
It calls **1D95E0** 34 times for the actual grouping traversal, commits ten slots
through **1D3A70**, and completes **1DA4C0 → 1DB1C0** with all nine actual
constraints. **1DB8E0** executes nine times with actual current frames and body
keys. Component+70 is the produced slot bank **0601E380**, +74/+78 are10/10;
+7C is the produced constraint bank **0601AD20**, +80/+84 are9/16. These are
observed allocator outputs, not live addresses to replay.
[Original grouping](../analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/measured/instructions/001d9330.asm),
[original constraint owner](../analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/measured/instructions/001db1c0.asm).

The report retains complete physical backing, raw current unit/node storage,
component, body/motion/slot fields, player/clock/world inputs and outputs for each
root. It also retains all original call ABI registers/stack inputs, MXCSR/x87
control, 24,349 reads and 18,318 ordered stores. The complete grouping stack
frame is recorded before its owning initialization and at return. Original
x87 `FSIN`/`FCOS` inside constraint construction are emulator execution; this is
not verified Xbox hardware floating-point parity.

## Independent repeat and audit

[Measured report](../analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/measured/report.json)
and fresh-process repeat have SHA256
`60fb25e152ae0eb5b531927df6b72bb5428465c144694ec4a9da615a0102060d`.
All153 report/listing files repeat identically. The separate
[auditor](../tools/audit_mp_spartan_ragdoll_constructor_1_1.py)
checks18,103 listed original instruction rows, including6,692 executed
addresses, against the original image. It replays45 complete retained buffer
intervals using4,019 overlapping reads and3,632 stores, accounting for4,786
changed bytes. It separately verifies the full owning stack frame using874
reads and204 stores. The audit itself repeats byte-identically.
[Compact audit](../analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/audit.json).

Previously frozen complete static correspondence maps
`1D9330 → 1DDBF0`, `1D95E0 → 1DDEA0`, `1DA4C0 → 1DED80`, and
`1DB1C0 → 1DFA80` against the authenticated1.5 image. The existing complete
correspondence report is the authority for those exact mappings; this new
experiment dynamically executes1.1 and does not relabel executed addresses1.5.
[Prior paired grouping evidence](../analysis/multiplayer/sandbox-source/power-body-grouping/final/later-correspondence/complete-report.json).

## Native integration seam and remaining owners

The current native grouping API already owns the one-rigid, no-constraint crate
path and exposes real memory, allocator, body-factory, wrapper, slot and
reference services. It deliberately stops at non-root current-node selection,
biped motion classification and positive constraint/property construction.
The new generated records are suitable for independent fixture comparisons,
not a callback that supplies completed live construction.
[Native grouping contract](../include/halo2/physics_body_grouping.hpp),
[actual stop sites](../src/physics_body_grouping.cpp).

The existing body factory/quaternion/matrix, slot/reference disposal and
constraint-world membership owners can consume genuinely produced body/shape/
constraint storage where their required services exist. A live producer must
first close the biped/current-node and positive constraint constructors; the
recorded nine constraints cannot substitute for those producers. World/island
merge, scheduling, contact/solver delivery, natural +34B admission, physical-node
pose delivery and corpse lifetime/render/network integration remain distinct.
[Factory](../include/halo2/physics_body_factory.hpp),
[slots](../include/halo2/physics_body_slots.hpp),
[constraint membership](../include/halo2/physics_constraint_membership.hpp).

A tractable next consumer is **1D42D0**: resolve one actual committed slot's
body/motion transform, perform the conditional original component correction,
and emit the 34h-byte current-node matrix. Complete `1D7310/BDF80` require this
consumer and real hierarchy/mask inputs. This new constructor report does not
execute those consumers or install captured matrix output. Positive current-node
measurement must use these genuine generated bodies and source-owned matrices.
[Exact consumer requirements](MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md).

```sh
.tools/python/bin/python tools/oracle_mp_spartan_ragdoll_constructor_1_1.py \
  --output NEW_DIRECTORY/report.json
.tools/python/bin/python tools/audit_mp_spartan_ragdoll_constructor_1_1.py \
  --directory analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/measured \
  --repeat NEW_DIRECTORY --output NEW_DIRECTORY/audit.json
```
