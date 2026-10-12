# Disc 1.1 equipped Spartan normal unit-control wrapper

This independently frozen follow-up executes the original normal unit callback `C7E90` after the original player input conversion. It preserves the previously frozen [equipped animation suite](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md) unchanged. It does not claim execution of the complete original moving-player object update or physical classifier.

## Frozen evidence and reproduction

```sh
.tools/python/bin/python tools/oracle_spartan_equipped_unit_phase_1_1.py --suite analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite
.tools/python/bin/python tools/oracle_spartan_equipped_unit_phase_1_1.py --instruction-inventory analysis/multiplayer/map-pack/spartan-equipped-unit-phase-instructions
```

| Authority / output | SHA-256 |
|---|---|
| Disc 1.1 `default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original `lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| [Oracle tool](../tools/oracle_spartan_equipped_unit_phase_1_1.py) | `cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38` |
| Unchanged equipped dependency | `3cff76069e4770392b0cee0df2bafaec5a4def1ba4945493047cdb57977e7a65` |
| [Suite index](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/suite.json) | `a9724a76d52551cea10c1b379ecb8503e1c9c2d96cb553bd0643d5f8289f418e` |
| [Instruction inventory](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-instructions/inventory.json) | `705240869c7222f694810c808563883ddfc4c19e247bfd70872dcf6658dcd3c5` |
| [75-tick idle report](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/idle.json) | `79b2691501cb4244232960f013de9fb7c2d810d3cc1f7bd4ce31b1c4f4984bfd` |

The six cases execute **519 ticks and 110 original projectile requests**: idle75, aim12, move-input12, fire60, requested positive-ammo reload135, natural empty-magazine reload225. Every case retains raw 0x400-byte unit state, complete selected manager/channels, complete weapon phase fields, all 33 source-decoded local transforms, original input/action/resource calls, and entry/return snapshots of ten unit routines. The suite index pins each report separately. The canonical `spartan-equipped-unit-phase-1.1-oracle.json` is an exact byte copy of idle75. The inventory retains 17 unchanged original instruction slices.

For all 519 ticks, primary manager bytes, secondary channel bytes, and every decoded local-transform byte match the previous isolated-aim fixture. This proves these outputs agree under the recorded inputs; it does not prove unexecuted world or physical paths.

## Original callback producer and ABI

The original executable's type table at `0046FAD0` selects type0 descriptor `0046EF18`. Its inheritance array at descriptor+84 lists `0046ED88` (base), `0046EE50` (unit), and `0046EF18` (biped). Their callback+40 values are respectively zero, **C7E90**, and **DE960**. These bytes are recorded directly in the inventory, not reconstructed from semantic names.

Original [10CDF0](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-instructions/0010cdf0.asm) at `10CE3A` pushes the unit datum and at `10CE3B` calls the descriptor callback in EAX. **C7E90 consumes its unit datum from stack argument1 and returns with RET4.** The final frozen oracle explicitly supplies `(00010001)` and EAX=`000C7E90` for every call. Its prototype must not be inferred as register-only from Ghidra's incomplete signature.

Original [BD360](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-instructions/000bd360.asm) runs `10CDF0` before traversing carried children through object+10. Thus the actual unit callback is an inherited object-update phase. Type2 weapon descriptor `0046F170`, reached through type-table slot `0046FAD8`, has the inherited item callback `F9B10` and weapon callback `1021D0`. This follow-up runs the proven callbacks directly; it does not execute the complete BD360 recursion or replace biped DE960 with a supplied no-op.

## Executed wrapper and phase contract

Original [C7E90](../analysis/multiplayer/map-pack/spartan-equipped-unit-phase-instructions/000c7e90.asm) orders:

`C5D10 -> C6C50 -> C7420 -> C7210 -> C7AA0 -> 116420 -> C7CF0 -> C7B70 -> C6400`.

Every helper executes its original instructions. The oracle supplies the same 0x5C device-input records as the equipped dependency and executes `150DD0 -> 150F00 -> 151C90`; it then performs manager blend and **one C7E90 call**, followed by the separately bounded weapon, action, animation, local-channel and projectile-postpass phases. **There is no separately invoked C7420 in this follow-up.** Its `unit_wrapper_events` give each helper's input registers, stack, raw unit/weapon state before and after, and return register.

The aim fixture's converted input snapshot precedes C7420. Current aim changes during the actual C7420 invocation on tick0. The previous document's statement about the first recorded current aim describes this pre-C7420 snapshot; it is **not a one-tick aiming delay**. Exact desired/current bits and constraint operations remain recorded.

The original ready countdown remains15 after equip. First held-fire preparation/dispatch is tick14, then16,18,...44; reload and natural-empty behavior match the prior source suite. The actual source chooses primary idle185, secondary ready207, fire172 and reload215. These are original graph lookup results, including the authored `combat:rifle:br:fire_1` fallback selected for the equipped SMG.

## Explicit producer boundaries

Player/unit/weapon storage is preallocated as in the equipped dependency; original `14E240` ownership assignment and original `101BB0 -> E9D50` initialization/equip execute. This fixture does not supersede the separate full original object allocator work. Player team storage has its supplied +0C field zero; C7CF0's copy to unit+138 is recorded. Device/controller records, original resource residency, unavailable audio/effect allocator, clear muzzle world service and failed projectile allocator retain the dependency's explicit boundary contracts. Shared RNG begins at supplied `12345678`; this is not a claimed Xbox match seed.

Original 10CDF0 invokes biped **DE960 after C7E90**. Ordinary DE960 requires valid physical state, invokes the physical classifier and animation/movement producer, and can affect later owner/child phases. This report proves C7E90's real producer and complete control wrapper, not that following DE960/physics or the entire world tick has run. Classified movement selectors remain separate explicit inputs in the previous 12-selector suite. A forward axis alone in the current no-physical-state fixture leaves the source idle clip; that observation is not evidence that player movement works.

Matrix reciprocal fidelity is a separate [instruction/emulator boundary](SPARTAN_LIVE_POSE_1_1_RESEARCH.md#reciprocal-instructionemulator-boundary). The manager, selected clip, clocks, raw decoded channels and local quaternion/translation outputs here do not require claiming host/Xbox RCPSS agreement. No production files or prior frozen suites were changed.
