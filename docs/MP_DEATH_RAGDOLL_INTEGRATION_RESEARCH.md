# Spartan death, physical-node delivery and live corpse gaps

Research date: 2026-10-10. Ordinary fatal damage enters a physical-state transition; it does **not** unconditionally select a death animation. The actual cache contains the Spartan ragdoll bodies and constraints. The current native renderer excludes dead players and does not consume a retained corpse pose bank. This audit freezes the exact original consumer contract and metadata before any corpse presentation change. It does not add a standing idle corpse, choose a fall-death clip, construct constraint frames, or claim a completed ragdoll solver.

This audit contains **no new native BDF80/1D7310 port**. Existing original executions have a completed standing biped body/world and genuine animation matrices, but no positive death-produced ten-body/nine-constraint bank with complete physical-node before/after buffers. The canister's verified one-body/no-constraint path cannot supply those missing Spartan inputs. The next experiment must execute the positive body transition on the genuine allocated Spartan; this audit records its starting contract and does not complete the requested ragdolls. [Standing biped proof](MP_LIVE_BIPED_WORLD_RESEARCH.md), [owned player/pose proof](MP_SPARTAN_OWNED_POSE_RESEARCH.md), [bounded native grouping](../include/halo2/physics_body_grouping.hpp), [existing equipped death execution](MP_EQUIPPED_DEATH_TAIL_RESEARCH.md).

## Authority and new evidence

Behavioral authority is unchanged original TU1.1 instruction bytes and actual cache bytes. The authenticated u5 image is used for complete static instruction/branch correspondence, not a new gameplay execution. Assembly schema labels and decompiled C are navigation only; serialized constraint frames are not runtime solver output.

| Input | SHA-256 |
| --- | --- |
| TU1.1 `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Authenticated u5English `default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Original Lockout cache | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |

New tools are [complete-body correspondence](../tools/research_mp_death_update_functions.py), [literal phmo inventory](../tools/inspect_mp_death_physics.py), [read-only graph query](../tools/inspect_mp_death_animation.cpp), and [audit/freeze](../tools/freeze_mp_death_animation_audit.py). Ignored evidence is `analysis/multiplayer/integration/death-animation-20261010/`; its `frozen/manifest.json` pins copies of this note, the tools, complete paired listings/reports, metadata, exact native audit snapshots and earlier frozen authorities. No new original gameplay execution or physical-node native differential is claimed.

## Ordinary death versus conditional animation

Original `CB410` at `CBA16..CBA2E` builds the fatal cleanup call from its actual damage-result buffer: class from signed result+40, vector pointer result+18, fourth argument **zero**, and final force low byte from result+4 bit10. `CBDC0` forwards the fourth argument to the biped callback `E6050`; it does not derive it from downward velocity or a kill volume. [Complete CB410](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000cb410-disc-000cb410.asm), [CBDC0 entry](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000cbdc0-disc-000cbdc0.asm).

`E6050` performs its actual root adjustment and calls `E6DC0`. It selects `landing_dead` only when the forwarded flag tested at **E619D** is nonzero. That branch calls `110BF0` with EAX=210h, current mode/class/weapon identifiers `07000101`, activity `0C000043`, and zero final options. It then calls `1D1940`, sets manager+6C bit0, clears manager+65/+64/+67 and sets unit+349 bit2. The ordinary fatal call above passes zero, bypassing this request. [Complete E6050 bytes](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000e6050-disc-000e6050.asm).

The read-only cache query resolves `0C000043` to `landing_dead`. Under the separately supplied existing combat/rifle/SMG identifiers, the actual graph `E9180793` contains clip **102, 42 frames**. This proves the authored lookup, not action admission, a 42-tick death delay, a fall-death producer, or the flag's live value. The query's default/closing combination is absent for that supplied class/weapon. Both supported executable data providers produce identical query output. [Literal query output](../analysis/multiplayer/integration/death-animation-20261010/metadata.txt), [query boundary](../tools/inspect_mp_death_animation.cpp).

A distinct `C5190` branch, admitted by its actual packet byte+4 bit0, calls `D7E00 → D8070 → CE440`, removes optional extra equipment, zeroes grenades and calls `CBDC0(unit,NONE,null,1,0)`. Its direct caller `EFF50` first passes packet+4C to `C5190`; packet+50 bit1 then controls default/closing request and its authored idle fallback. There is a literal original data pointer to `EFF50` at `46F010`. It is not found among the imported sixty four-word unit-action descriptors. The pointer and complete bodies are frozen; their natural packet/dispatch producer is still unproved. **This is not evidence that falling out of the world must play clip102.** [C5190](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000c5190-disc-000c5190.asm), [EFF50](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000eff50-disc-000eff50.asm), [static pointer audit](../analysis/multiplayer/integration/death-animation-20261010/frozen/audit.json).

The existing seven-case original equipped-death suite executes complete ordinary cleanup, changes biped state0→3 through `E6050 → E6DC0 → 1E8E60`, and retains the primary animation unchanged. Its physical component is explicitly NONE; that executed negative component branch cannot prove physical corpse matrices. [Immutable death-tail report](../analysis/multiplayer/integration/spawn-source/death-tail/report.json), [exact original limits](MP_EQUIPPED_DEATH_TAIL_RESEARCH.md).

## Physical rebuild and matrix delivery contract

`E6DC0` requests state3 through `1E8E60`. When the old/new state differs and either belongs to {3,4,5}, `1E8E60` calls **1C70D0(unit,0)**. This owner resolves the actual unit+B4 full component datum. If present, it remembers component+4 bit5, optionally removes the component from the world through `1D4C50`, disposes its existing bank through `1D3BB0`, writes the new component flags from the caller, rebuilds through `1D2B10`, and conditionally reinserts through `1D4F30`. This is a body/constraint lifecycle operation, not a renderer clip toggle. [1E8E60](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001e8e60-disc-001e8e60.asm), [1C70D0](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001c70d0-disc-001c70d0.asm), [verified canister lifecycle and its limits](MP_POWER_BODY_GROUPING_RESEARCH.md).

Complete `BDF80` establishes the current-node ownership order:

1. Resolve actual object/header/definition and the current 34h-byte matrix bank. Object signed word+114 is the byte extent; signed word+116 is its relative offset. Clear object+4 bit30. Biped unit+34B=2 takes its original skip path.
2. Initialize the caller node mask. If object+B4 is not NONE **and object+C0 bit6 is set**, resolve the component. For a biped, call `E2CA0` for its real height correction, then call `1D7310(component,node_mask,height_correction)`.
3. Continue the original animation/local callback and hierarchy processing for nodes not supplied by physical delivery, preserving the mask and actual local/current bank offsets. Existing genuine normal-unit evidence records a source-selected 21-node local callback over a retained 33-node bank; neither count can be substituted universally.

The exact branch sites are BDFBD..BDFF1 and BE035..BE097. The following animation/hierarchy stores are in the same complete body. [Original BDF80](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/000bdf80-disc-000bdf80.asm), [existing positive animation matrices](MP_SPARTAN_OWNED_POSE_RESEARCH.md).

`1D7310` receives three stack words and returns with RET0Ch. It first rejects component+4 bit9 or signed body count+74≤0. It resolves component+8's actual object → definition+38 model → model+24 phmo/model+4 render definition. Its body-slot bank is component+70, stride60h; each slot+48 points to signed-byte rigid indices and slot+4C is their count. Each phmo rigid row is90h bytes; its signed word+0 selects the physical node. Invalid nodeFFFF, shape virtual type6, an already-set caller mask, and the original parent-chain filtering each skip their owning path. [Complete 1D7310](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001d7310-disc-001d7310.asm).

For an admitted node, `1D7310` calls `1D42D0` with **ECX=component, EAX=body-slot index, ESI=current output matrix**. `1D42D0` follows slot+40→record+3C to the real current Havok body, copies its four aligned 16-byte transform lanes at body+80..BF to private scratch, optionally executes `2E0B60` with component+30 when component+4 bit6 is set, then stores the source scalar one and the twelve XYZ matrix words into the 34h output. It preserves the original ordered stores and discards each fourth lane component. `1D7310` next adds its caller's height correction to output+30.

When the selected rigid node differs from the slot's first rigid node, the source shape-container callback+34 must produce the actual child frame. The owner converts that returned frame to its34h matrix and calls `1441D0` with the original aliases. It then updates the caller mask, marks the physical node and each real phmo ancestor in its private mask, and invokes **1D7710** for remaining hierarchy propagation. `1D7710` uses real render-node bind data and parent/sibling/child links; its corresponding u5 call target is recorded, but this audit does not claim a complete independent full-body match or positive execution for that callee. No fabricated child frame, identity matrix or successful virtual result can replace it. [1D42D0](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001d42d0-disc-001d42d0.asm), [1D7310 arithmetic/calls/masks](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001d7310-disc-001d7310.asm).

## Actual Spartan phmo inventory

Actual `bipd E9F2086D → hlmt E9F3086E → phmo E94E07C9` retains **33 nodes, 10 rigid rows, 9 node edges, 5 ragdoll constraints and 4 limited hinges**. The other four constraint blocks are empty. The inventory preserves all root/row bytes, source virtual addresses, file offsets, node hierarchy, constraint references and both authored13-word frames. StringID names come from the literal map's index/string tables. Field labels come from the pinned Assembly schema; the original consumer independently proves the rigid stride90h/node+0 and node stride0Ch/parent+6 layout. [Literal inventory](../analysis/multiplayer/integration/death-animation-20261010/physics-v3.json), [reader](../tools/inspect_mp_death_physics.py), [original node consumer](../analysis/multiplayer/integration/death-animation-20261010/correspondence/instructions/001d7310-disc-001d7310.asm).

| Rigid row | Node | Authored node name | Serialized shape type/index |
| --- | --- | --- | --- |
| 0 | 15 | head | 1 / 0 |
| 1 | 17 | l_forearm | 1 / 1 |
| 2 | 14 | l_upperarm | 1 / 2 |
| 3 | 18 | r_forearm | 1 / 3 |
| 4 | 16 | r_upperarm | 1 / 4 |
| 5 | 4 | l_calf | 1 / 5 |
| 6 | 1 | l_thigh | 1 / 6 |
| 7 | 5 | r_calf | 1 / 7 |
| 8 | 2 | r_thigh | 1 / 8 |
| 9 | 0 | pelvis | 2 / 0 |

All ten retain serialized motion selector2 and flags0003. The selected shape blocks contain nine pill records and one box record; these serialized selectors are not initialized runtime shape pointers or an instruction to install replacement colliders. Constraint node order is preserved below, including the ragdoll rows whose names reverse the literal A/B order. [Raw rigid/shape records](../analysis/multiplayer/integration/death-animation-20261010/physics-v3.json).

| Constraint block/index | Node A → B | Edge |
| --- | --- | --- |
| limited_hinge0 | l_thigh1 → l_calf4 | 0 |
| limited_hinge1 | l_upperarm14 → l_forearm17 | 1 |
| limited_hinge2 | r_thigh2 → r_calf5 | 2 |
| limited_hinge3 | r_upperarm16 → r_forearm18 | 3 |
| ragdoll0 | l_thigh1 → pelvis0 | 4 |
| ragdoll1 | r_thigh2 → pelvis0 | 5 |
| ragdoll2 | head15 → pelvis0 | 6 |
| ragdoll3 | l_upperarm14 → pelvis0 | 7 |
| ragdoll4 | r_upperarm16 → pelvis0 | 8 |

Each actual edge has one retained constraint reference, flags0 and friction word3E800000. The existing complete-body grouping correspondence already maps constraint owner `1DB1C0→1DFA80` and iterator `1EEAD0/1EEB20/1EEB70→1F33C0/1F3410/1F3460`. Its measured canister constraints are empty; the positive constraint constructors, current frames, property/solver admission and multi-body aggregation remain required for this Spartan. [Prior immutable paired report](../analysis/multiplayer/sandbox-source/power-body-grouping/final/later-correspondence/complete-report.json), [bounded native API](../include/halo2/physics_body_grouping.hpp).

## Native integration order and remaining owners

The current renderer's body loop requires connected and not dead, resets gait for a dead player, and similarly excludes it from object lighting. `Simulation::damage` and the Match kill-trigger path clear velocity and mark the native unit dead. Match already retains the full old unit datum, previous owner, death tick and garbage lifetime through `finish_death`/`retain_unit`/`unit_from_datum`. The current player snapshot and body draw do not enumerate that retained corpse bank, and Unit does not retain the original corpse multi-body/current-node/constraint bank. These are observed native integration gaps, not original game behavior. [Runtime](../src/multiplayer_runtime.cpp), [Simulation](../src/simulation.cpp), [Match ownership](../src/multiplayer.cpp). Exact copies of these audited files are retained under the new freeze's `files/` directory.

The minimum source-backed bridge is:

1. Retain the actual fatal cleanup inputs and full dead-unit identity; perform the original biped state/body lifecycle, including existing ordinary/root adjustment before physical rebuild. The fall/forced-death packet producer is separate and remains unknown.
2. Extend the actual component grouping to the authored ten bodies and nine constraints, with real shape fixup, current node frames, reference lifetimes, property/body keys, world membership and solver updates. Existing canister-only grouping or the completed standing controller body does not close this owner.
3. Execute the physical-first `BDF80→1D7310→1D42D0/child-frame→1D7710` delivery with its real masks and blend remaining animation nodes, then let skinning and collision consume that same current matrix bank. Preserve established reciprocal/RSQRT emulator-versus-Xbox limits in any lower math path.
4. Expose retained corpse identity and pose lifetime to Scene/LAN/presentation, independently from the new live player after respawn. Stop drawing only at the actual garbage/removal owner, rather than the first dead flag.

Steps2–3 lack positive genuine Spartan raw before/after evidence. Porting a universal reject or converting copied canister transforms into a corpse would leave the requested live behavior unresolved. No runtime, native production module, CMake, shader, release or prior immutable report is changed by this audit.

## Authenticated correspondence and reproduction

All18 selected complete bodies have unique normalized instruction matches, equal internal branch topology and equal applicable switch-table topology; the report preserves every paired absolute memory operand and direct-call target independently. The36 original/later listings contain6,510 instruction rows. Address relocations and separately unpaired callees are not silently promoted to full gameplay or Xbox hardware parity. [Complete report](../analysis/multiplayer/integration/death-animation-20261010/correspondence/report.json).

| TU1.1 | Authenticated u5 |
| --- | --- |
| E6050 / E6DC0 / 1E8E60 / 1C70D0 | E6F70 / E7CE0 / 1ED780 / 1CC310 |
| C5190 / EFF50 | C6310 / F0E70 |
| E2520 / E2CE0 / E3190 / E3270 / E3A10 | E3440 / E3C00 / E40B0 / E4190 / E4930 |
| BDF80 / 1D7310 / 1D42D0 | BF0C0 / 1DBBD0 / 1D8B90 |
| 110BF0 / 1D1940 / CB410 / CBDC0 | 111B10 / 1D6200 / CC590 / CCF40 |

Two independently generated reports and all36 listings are byte-identical. Literal physics inventory and graph inspection are independently repeated; graph output also agrees between the two supported data providers. All new listed instruction bytes and literal row bytes are checked against the pinned original images/cache. Earlier death-tail, owned-pose, player-visibility, grouping and live-prop freezes are verified read-only. These are source/metadata audits, not new original death execution, native ragdoll tests, sanitizer validation of a new port, or a playable fall-death reproduction.

```sh
.tools/python/bin/python tools/research_mp_death_update_functions.py \
  --output analysis/multiplayer/integration/death-animation-20261010/reproduction/correspondence/report.json
.tools/python/bin/python tools/inspect_mp_death_physics.py \
  --output analysis/multiplayer/integration/death-animation-20261010/reproduction/physics.json
.tools/python/bin/python tools/freeze_mp_death_animation_audit.py
```
