# Original allocated biped construction, world insertion and inactive world step

The original allocator and complete physical construction now execute against
the source-created object heap and retained Lockout world. Authored starts 0
and 32 produce real allocated bipeds, physical components, Havok bodies and
motions. Their bodies enter the source world. Original `1C7660` returns with
these bodies present and no supplied game/physics return service. These cases
have **source-produced empty player membership and zero active-cluster masks**;
the resulting bipeds are inactive. Ordinary owned-player motion/contact and
the complete game tick remain separate gates. Production and earlier freezes
are untouched.
[Independent oracle](../tools/oracle_mp_spawn_live_body_world.py),
[report](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json),
[preceding fixed-memory/world freeze](MP_FIXED_MEMORY_WORLD_RESEARCH.md).

## Authority and immutable evidence

Original TU1.1 XBE SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Actual Lockout SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The complete native compatibility generator executable is reused read-only,
with a real worker kept alive. Its whole KPCR, ETHREAD, backed host stack and
process record are transferred. Unknown-byte zeros and null host-branch TLS
remain compatibility-produced values; neither native thread identity nor an
Xbox thread layout is supplied by inference.
[Whole platform context and limits](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

| Frozen evidence | SHA-256 |
| --- | --- |
| New oracle | `5b2550b5f09ff214372c51e832407353e3f5da8e18713aca46d89855ee0c2586` |
| Semantic report | `2bf6fd373182b3db29d7409012f9ae566dcf2d9d496bbf15700184c4c8858951` |
| Canonical original instruction manifest | `6cd222419898d1d098eb64688c286eed16fb5757d38d0e4a07ec653940983480` |
| Complete executed instruction listing | `4cbfb77e70e2ce77081cfcfb64bc51e45c3cd5519a1e0f9a908ef8ce7d878446` |

Three cases verify **20,652 unique executed original instruction addresses**.
The223 routine listings contain17,749 instruction rows; the separate executed
listing contains20,652 rows. Twenty-nine earlier immutable inputs are checked
before and after execution.
[Complete executed original listing](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/executed-original-instructions.asm).

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_live_body_world.py \
  --output analysis/multiplayer/integration/spawn-source/live-body-world/reproduction/report.json
```

The semantic report, every original routine listing and the complete executed
instruction listing are independently compared byte for byte. Per-worker raw
host snapshots are separate evidence and may differ. Original instruction bytes
are checked against the XBE and each emulator. Earlier immutable input pins are
audited before and after execution.
[Full source-byte manifest](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

## Recovered producer and callback order

Original `139390` calls `125280`, allocates the game-global block and visits
initialization callbacks at `447428 + index*24`. Original `139410` visits map
reset callbacks at `447430 + index*24`. Original `1394B0` clears all three
cluster-mask banks, then visits activation callbacks at `447438 + index*24`.
The loops advance in ascending record order. Here `24` is hexadecimal and each
record has nine pointer words. The new suite captures the exact original bytes
of these selected records:

| Record | Initializer | Map reset | Activation |
| --- | --- | --- | --- |
| 19,4476D4 | 14CC00 player storage | 14CCD0 | 187550 |
| 25,4477AC | 1EEF40 physics constants | 1EEFE0 | NULL |
| 27,4477F4 | B76B0 object storage | B78C0 | B7AA0 |
| 58,447C50 | 1CB180 AI storage | 1CB3D0 | 1CB5A0 |
| 59,447C74 | 1C5F00 physical storage | 1C5FD0 | 1C5FF0 |

[Original initializer loop](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00139390.asm),
[original reset loop](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00139410.asm),
[original mask writer and activation loop](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001394b0.asm),
[exact callback-record bytes](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

The bounded preparation executes selected initializers in that recovered order:
19,25,27,58 prefix,59. The AI prefix is `1CB180..1CB1F4` exclusive and includes
its original storage/reset producer; its remaining initializer tail is excluded.
The selected reset order is19,25,27,59. The original activation zero writer
`1394B0..1394F0` executes before selected activation27,59. Original `13A380`
then derives zero visible/active masks from the source-created empty player
pool and produced AI state. No active mask, nearby player root or readiness
flag is written by the fixture.
[All executed preparation stages](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

Other initialization/reset/activation callbacks, the complete game-global
allocation/session/options producer and whole title/map scheduling are excluded.
The selected ordering does not establish the complete authored allocation order.
The retained session/options/tick inputs remain the declared earlier fixture
boundary. The actual `125280` frame is preserved around the exact excluded
`217090` savegame/thread-I/O call; the other original arena instructions execute.
[Exact exclusions and inherited input scope](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json),
[preceding arena evidence](MP_FIXED_MEMORY_WORLD_RESEARCH.md).

## Source-owned storage and capacities

Complete original `B76B0` includes widget, object-type and light initialization,
the original2048-by-12 header pool, and the previously omitted dynamic tail.
Its owning `125520/18FC80` instructions construct the **1 MiB body heap**;
it also allocates the84-byte object-world record andA00 scratch. Original
`1CE5B0` owns both collidable and noncollidable registry triplets, with2048-entry
forward/reverse pools using32/12-byte records. Reset `B78C0` and activation
`B7AA0` return through unchanged source instructions.
[Whole source object initializer](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/000b76b0.asm),
[original heap constructor](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/0018fc80.asm),
[original registry constructor](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001ce5b0.asm).

Original `1EEF40` allocates1C arena bytes, assigns `526254` and writes the
literal gravity/scale/air constants, zero velocity and zero history field.
Original `1C5F00` allocates four arena bytes, assigns `526230` and zeroes the
physical counter. It then calls `1F15B0→2284C0→1D2620→1482A0→1482F0`.
Original `1C5FD0` resets the BSP datum table with2000 `FFFFFFFF` words.
All these storage pointers and contents are original outputs.
[Physics-constant storage producer](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001eef40.asm),
[complete physical storage initializer](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001c5f00.asm),
[BSP map-reset producer](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001c5fd0.asm).

The retained raw memory service admits only the original requests: impact-pool
`VirtualAlloc(0,1450,101000,4)` and `(0,850,101000,4)`, component1409B and
Havok106470 backing. The impact requests now occur inside their full owning
`1C5F00` caller. Raw storage is supplied under the earlier OS boundary; original
constructors populate it. The fixed allocator remains15000 bytes. Fallback
backing `518434` remainsNULL and no fallback allocation is requested by these
cases.
[Source requests and full allocation traces](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json),
[fixed/scratch/fallback ownership](MP_FIXED_MEMORY_WORLD_RESEARCH.md).

The old Python observers would write supplied heap pointers at18FCE0/18FD70,
even after removing their services. They are bypassed only at those entries.
AtB8A30, the old supplied-object queue is hidden from its observer because its
Python attribute name collides with the fixed allocator's evidence list. These
changes alter no guest register, memory, return value or instruction. Actual
`18FCE0`, `18FD70`, datum allocation and all construction instructions execute.
[Exact observer exclusions](../tools/oracle_mp_spawn_live_body_world.py).

## Actual biped and world results

The creation descriptors are unchanged frozen outputs of original player setup
for authored starts0/32. The unit definition, model, graph, serialized shapes,
BSP and animation resources use actual Lockout/shared cache bytes. Original
selected tag fixup and full node/animation/physical construction execute. No
reserved biped body, root node, physical component, transform or contact is
supplied for the new allocation.
[Source-generated descriptor evidence](MP_SPAWN_RESEARCH.md),
[original allocated-biped and resource authority](MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md),
[full new constructor inputs/outputs](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

| Output | Original produced value |
| --- | --- |
| New header datum | E26F0000 |
| Generic body size | 5750 decimal bytes |
| Physical component | E1680000 |
| Havok body | 0601A480 |
| Motion | 0601A5C0 |
| Body world pointer+08 | 06005470 |
| Physical counter after creation | 1 |
| Unit root | Exact authored XYZ, bit for bit |

Complete `B8A30` returns the new salted datum. Its original heap-growth path
creates the generic/runtime blocks. Original `318100` returns the rigid body
and motion; original `1D4F30→1C7320→315820` inserts the body into the actual
world. The report retains full generic body, component, slot, body, motion-prefix
and transform bytes, together with original constructor inputs and resource
requests. The static BSP body remains in the same world.
[Complete allocator](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/000b8a30.asm),
[body constructor](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00318100.asm),
[component insertion caller](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001d4f30.asm),
[actual world insertion](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00315820.asm).

The source-empty player pool and zero active mask leave the allocated biped
inactive: its header lacks bit1. Player current-unit binding, loadout and owned
player eligibility are not entered. Optional attachment-effect178120 and
looping-soundC10E0 unavailable services remain explicitly supplied; neither
supplies geometry or a physical result.
[Admission/header state and used services](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

## First world consumer and remaining gate

The empty-world control and both allocated-biped cases execute complete
`1C7660`, including its original `317650` world call and aftermath. No supplied
game/physics return service is used in this stage. Complete source-owned world
state before/after is recorded. The unit root remains authored; no fixture
grounding, Z correction, contact or velocity is added.
[Original complete world-step caller](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/001c7660.asm),
[original world step](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00317650.asm),
[executed cases](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).

This is **inactive-body world-step completion**. Ordinary character `E3B20`
and `1E9540` contact production are not entered. Original `139750` schedules
`13A380→B7E00→1C7660→B7F50→B8040` after earlier player/engine/world phases;
this suite does not execute that entire tick. A source-owned player/current-unit
binding and its actual activation inputs are the next gate. There is no claim
of ordinary FFA motion/contact, complete authored startup, Xbox execution or
full Havok parity from these completed routines.
[Original game-tick scheduling](../analysis/multiplayer/integration/spawn-source/live-body-world/instructions/00139750.asm),
[precise case boundaries](../analysis/multiplayer/integration/spawn-source/live-body-world/report.json).
