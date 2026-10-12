# Original player binding, loadout, PVS activation and first biped motion

The original player join and complete multiplayer spawn now execute on the
source-created object heap and Lockout world. The source binds the true player
to its allocated biped, equips its allocated SMG, derives player cluster 4,
copies the real cached cluster PVS and activates the biped. The four object
phases in the original game-tick order return, including the original biped
motion request producer and consumer. No game or Havok return service executes
in the player-cluster, PVS or motion phases. This result excludes the player
command-input tail, the complete game tick and hardware contact parity.
Production and all earlier freezes are untouched.
[Independent oracle](../tools/oracle_mp_spawn_player_visibility.py),
[semantic report](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json),
[preceding allocated-body/world freeze](MP_LIVE_BIPED_WORLD_RESEARCH.md).

## Immutable authority and reproduction

The original TU1.1 XBE SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Actual Lockout SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The earlier whole compatibility KPCR generator is reused read-only, with its
real worker kept alive. Whole produced records and backed stack/process inputs
are retained. Unknown-byte zeros and null host TLS are compatibility outputs,
not recovered Xbox defaults. The produced ETHREAD field at +12C remains zero;
the separately printed native thread ID is not assigned to it.
[Whole platform producer and scope](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

| Evidence | SHA-256 |
| --- | --- |
| Oracle | `aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572` |
| Semantic report | `f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd` |
| Original instruction manifest | `22d4a91c623aa3bf1fa0c05566f678e41b4ff0b4bcae112b1eec556c64285dfe` |
| Complete executed instruction listing | `afa7675b6874d1f4ae83f308a1832ab55850ddfaa44e3a59d81abd0332caa047` |
| Observed original store-site listing | `616d198c29b96a43d6b64ff13f98a1177e74cb13147726b0fbd38eec3e1a6ba4` |

Three cases verify 43,305 unique executed original instruction addresses. The
249 routine listings contain 20,235 rows; the separate executed listing has
43,305 rows and the observed store-site listing has 201 rows. Every executed
instruction is checked against the XBE and all three emulator images. All
37 earlier pinned dependencies are checked before and after execution.
[Manifest and case results](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json),
[executed original bytes](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/executed-original-instructions.asm),
[observed source stores](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/observed-source-store-sites.asm).

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_player_visibility.py \
  --output analysis/multiplayer/integration/spawn-source/player-visibility/reproduction/report.json
```

An independent run produces an identical semantic report and all 251 instruction
files. Per-worker host snapshots are separate evidence and may differ. The
freeze verification records every compared artifact and earlier dependency.
[Independent verification](../analysis/multiplayer/integration/spawn-source/player-visibility/independent-freeze-verification.json).

## Owning preparation order and declared input boundaries

The new original CRT callback `37EB40`, stored at original table word `468C20`,
installs engine identity tables including the Slayer implementation. Original
callback records are at `447428 + index*0x24`. The selected initializer order
is 19,22,25,27,28,30,58 prefix,59; selected reset order is
19,22,25,27,28,30,59. The original mask-clear prefix `1394B0..1394F0`
executes before object/world activation. This is recovered ordering among
selected original callbacks, not complete title/session/all-object startup.
[Exact callback record bytes and preparation](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json),
[engine table CRT producer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0037eb40.asm).

| Record | Owning initializer | Owning reset | New retained output |
| --- | --- | --- | --- |
| 19 | 14CC00 | 14CCD0 | Original player pool and globals |
| 22 | 158BA0 | 158C10 | C9C-byte engine globals; default Slayer selection |
| 25 | 1EEF40 | 1EEFE0 | Original physical constants |
| 27 | B76B0 | B78C0 | Object headers, body heap, world and registries |
| 28 | 10D490 | 10D4F0 | 1D88-byte unit queue at global 517FE0 |
| 30 | 1EFE10 | 1EFE70 | 24-byte trigger bitset at global 526258 |
| 58 | 1CB180..1CB1F4 | Included in prefix | Bounded AI storage/reset |
| 59 | 1C5F00 | 1C5FD0 | Physical pools and BSP datum table |

Original `10D4F0` clears the queue and stores +1D84=1. Original `1EFE70`
clears eight dwords and stores +20=1. These replace the earlier queue fixture
and supply the actual trigger producer required by the first unit update.
Neither capacity is enlarged. Source fixed-memory allocation, fallback NULL,
whole platform context and real requested raw storage remain inherited and
explicit. [Queue initializer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0010d490.asm),
[queue reset](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0010d4f0.asm),
[trigger initializer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/001efe10.asm),
[trigger reset](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/001efe70.asm).

The exact frozen output of original default-variant producer `1A0A90` is
**delivered as game-options input** at Game+13C. Complete original `158C10`
selects mode +C14=2 and executes its callbacks. The exact source-produced
controller-0 published/host-effective 90-byte property is delivered at
creation+1C. Creation identity is a declared zero-key nonlocal input, with
short +2=-1 and dword +4=NONE. Sign-in, lobby, transport and complete
session/game-global producers remain open; no player current-unit or cluster
field is supplied.
[Delivered input bytes](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json),
[original default/property authority](MP_SMG_EVENT_STRENGTH_RESEARCH.md),
[engine map reset](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/00158c10.asm).

The frozen item-resource importer appends actual BR/SMG graph chunks after the
existing resident Spartan chunks inside the existing mapped storage. It does
not enlarge storage. The report records each source file offset and raw hash;
only the SMG chunk is requested by this loadout. Earlier services at
`111490`, `111500`, `1DC8F0`, `1DC940`, `FB560`, `EFEE0`, and `C9A40`
are removed and their original animation/marker instructions execute. Remaining
used loadout services are optional attachment `178120` returning NONE, looping
sound `C10E0` returning NONE, and audio `170060` returning zero. These are
explicit boundaries, not recovered subsystem results.
[Resource and service requests](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json),
[resource importer authority](MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md).

## Original ownership, loadout and cluster stores

Complete `14D350→14D800→14D600→14D3D0` allocates player **EC700000**,
sanitizes representation +88=2 and produces handicap +C1=0. Complete
`1509C0` chooses through real `150640` using every unchanged authored start.
Both bound-player cases select start **37**, at Lockout file offset
`01D26E9C`, virtual address `8015369C`. Its root bits are
**BFB9579D,3C855B09,C1235809**. Its entire 34-hex-byte row is retained;
other shorts in the row are not treated as a proven actual BSP identity.
[Join](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0014d350.asm),
[profile sanitizer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0014d3d0.asm),
[complete spawn](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/001509c0.asm),
[chooser](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/00150640.asm),
[selected original row](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json).

The actual allocator/constructor produces biped **E26F0000**, 5750 bytes,
with physical component **E1680000**, and equipped SMG **E2700001**,
1260 bytes. Original `14E240` owns the binding stores:

| Original store | Result |
| --- | --- |
| 14E345 | Unit+13C = EC700000 |
| 14E34B | Unit+140 = NONE |
| CD415 then CD427 | Unit+134 changes 2 then 3 |
| 14E37C | Player+2C = E26F0000 |
| 14E37F | Player+30 = NONE |
| FB18F | Weapon+154 = E26F0000 |

The source unit inventory contains the allocated SMG; source magazine +22A
is 120 reserve and +22C is 60 loaded. The source clears the initial-spawn bit.
Original allocated model/body/current-node fields and source attachment/graph
functions provide this state. No unit body, ownership, head pose or animation
result is supplied. The complete authored allocation sequence and all chooser
branches remain unproved.
[Binding routine](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0014e240.asm),
[stores and complete object snapshots](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json).

The original `14EAE0..14ED0C` fragment preserves its own stack frame. It enters
`14E9E0→688F0` membership eligibility and the current/previous-unit parent-chain
cluster writer. Original `14EA73` stores the eligibility flag; `14EDC4` stores
player+2A=4 from the source object cluster. Player+28 remains NONE. The null
command-packet argument is not read before this exclusive stop. The subsequent
input/action tail and `C8140` are excluded.
[Player early update](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0014eae0.asm),
[observed ownership and cluster stores](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/observed-source-store-sites.asm).

## Real PVS and admitted first motion

Complete `13A380→13A2A0→151EC0→16F8A0` iterates the actual occupied player
pool. Lockout cluster 4 has +80 flags zero, so `16F8A0` uses its simple cached
PVS copy rather than the portal branch. Actual BSP PVS pointer `8010F898`
yields dword **0000DFFF**, bytes **FFDF0000**. Source visible Game+1138 and
active Game+11B8 banks become these bytes followed by 60 zero bytes. The
local-view bank remains zero. Actual activation `B7CE0→B8180` changes the
biped header flags **4A→4B**. No chosen cluster or all-active mask is supplied.
[Full visibility scheduler](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0013a380.asm),
[player mask consumer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/00151ec0.asm),
[actual PVS consumer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/0016f8a0.asm),
[activation](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/000b7ce0.asm).

The bounded roots follow original `139750` order:
**B7E00→1C7660→B7F50→B8040**. All four return in the complete case.
Biped flags are 4B after prephysics, 4F after the world step and postphysics,
and 47 after the new-object pass. The attached SMG flags finish at 8A.
Original `1E9540` creates the motion request and `E3B20` consumes it; exact
154-byte before/after records, registers, stack arguments and source returns
are retained. `1E9540` is void: its raw EAX `FFFFFFFD` is not a failure
classification. `E3B20` returns raw EAX `0101A001`, low byte 1. Unrecovered
request-field semantics are not labeled as floor-contact success.
[Original game-tick ordering](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/00139750.asm),
[request producer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/001e9540.asm),
[consumer](../analysis/multiplayer/integration/spawn-source/player-visibility/instructions/000e3b20.asm),
[all phase inputs and outputs](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json).

The authored biped root stays bit-identical through this first pass. Its final
velocity bits are **00000000,00000000,BE6F9DB2**. These are actual source
outputs, with no chosen velocity, gravity or contact. No game/Havok service
runs in these phases. The inherited time input is 30 Hz, dt 1/30 and tick 0;
the source whole-tick increment is excluded. Bounded roots use the retained
FPCW 027F/MXCSR 1F80 fixture ABI rather than executing `139750`'s initial
`control87` call. This is a constructed/equipped owner in source-default control
state, not a complete player-input tick, session, camera or Xbox hardware run.
[Exact phase state and service audit](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json).

## Negative cases and stable composition API

`joined_player_without_current_unit` runs the original join but no spawn.
Current/previous-unit and cluster stay NONE; the full mask scheduler returns
zero masks. Occupied player membership alone does not activate objects.

`bound_player_without_trigger_storage` omits only owning trigger initializer
and reset. Join, spawn, PVS and the first three object phases return, then the
real motion prefix enters `DF042→1EFEF0` and stops at **1EFF31**, reading the
actual NULL global `526258`. No replacement bitset or success is supplied.
Including the original record-30 producer closes that exact dependency in
`bound_player_first_ordinary_motion`.
[All three bounded cases](../analysis/multiplayer/integration/spawn-source/player-visibility/report.json).

The frozen importable API is `PlayerTrace`, `prepare(data,trace_type=...,trigger=True)`,
`join(t,index=0)` and `spawn_and_activate(t,index=0,activate=True)`.
`spawn_and_activate(...,activate=False)` returns after actual join/spawn and
cluster stores, before full `13A380`; it allows a separate fixture to allocate
real authored items and then run the full mask scheduler once. It enters no
motion phase. `t.spawned_player` and `t.spawned_unit` are original outputs.
The host generator worker must remain alive until the trace is complete.
[Read-only composition helper](../tools/oracle_mp_spawn_player_visibility.py).

The next independent gate is the original command producer feeding the tail
after `14ED0C`, then source-produced `C8140` control and ordinary motion.
Full device dispatch, local player/view binding, earlier/later tick subsystems,
all authored world objects and transport remain open.
