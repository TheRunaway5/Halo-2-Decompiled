# Disc 1.1 assigned Spartan and carried-SMG animation requests

This independently frozen research executes the assigned-player ownership and equipped-SMG request builders, original graph lookup, animation clocks, control conversion, firing/reload action requests, and decoded local channels. It does not substitute the previous unassigned `combat:sword:idle` result for an equipped player. The executable's own lookup chooses every recorded clip.

This is a bounded request/channel fixture. Existing player/unit/weapon storage, controller input records, resource residency and several world/allocation services are explicit inputs. The complete original moving-player physics producer and full original carried-object allocation are separate gates. No runtime files or earlier frozen suites were changed.

## Reproduction and immutable evidence

```sh
.tools/python/bin/python tools/oracle_spartan_equipped_animation_1_1.py --suite analysis/multiplayer/map-pack/spartan-equipped-animation-suite
.tools/python/bin/python tools/oracle_spartan_equipped_animation_1_1.py --instruction-inventory analysis/multiplayer/map-pack/spartan-equipped-animation-instructions
```

The suite contains six input/tick cases, **519 ticks**, and twelve separately supplied classified-physics selector cases. Its 18 original-execution reports retain phase states, complete 144-byte manager, 292-byte unit extension, 68-byte action/control block, secondary channel, source input/control/action records, all 33 local transforms, original resource/codec dispatches, firing state and explicit services. Expected values come from unchanged original instructions, not native implementations.

| Authority / output | SHA-256 |
|---|---|
| Disc 1.1 `default.xbe.03` | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Original `lockout.map` | `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| Original `shared.map` | `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af` |
| [Oracle tool](../tools/oracle_spartan_equipped_animation_1_1.py) | `3cff76069e4770392b0cee0df2bafaec5a4def1ba4945493047cdb57977e7a65` |
| [Suite index](../analysis/multiplayer/map-pack/spartan-equipped-animation-suite/suite.json) | `cbc99ee18a0091a48e14505936bf0eaa46a81b922bbd3be3cf4939fee3760352` |
| [75-tick equipped idle report](../analysis/multiplayer/map-pack/spartan-equipped-animation-suite/idle.json) | `def10f4d747e3dd2acc484a055906ef7b9e056a4e0a08bdce2a5550cbe2279bc` |
| [Instruction inventory](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/inventory.json) | `5c8b8f13e313aeee5ba8aee5b3d9f338b225e343846d673760d4796efd6ade79` |
| Unchanged live-pose oracle dependency | `28afd44db12c96a67bc82bca1c617cbd9fb8fa631e949f71e9b74743d2de10f5` |

The canonical `spartan-equipped-animation-1.1-oracle.json` is an exact copy of the 75-tick idle case. The inventory hashes 45 original instruction slices. `1122A0` and `1122D0` are interior slices of `112270`, not callable function entries; neither is invoked as a function by this oracle. Other source entries include the previously missing Ghidra entries `C7E90` and `DE960`. The previous default/live/collision/reciprocal oracle file hashes remain unchanged.

## Ownership and the real equipped state

The inherited [live-pose constructor](SPARTAN_LIVE_POSE_1_1_RESEARCH.md) executes original manager construction/binding and `C52C0`/`BD270` with actual Spartan graph **`E9180793`**. It allocates and initializes the real unit extension and action state; the original constructor leaves `UNIT+13C=-1` and its inventory empty. That state is only the start of this new fixture.

Original [14E240](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/0014e240.asm), with stack player datum 0 and EBX=unit datum `00010001`, executes the actual ownership writes. It makes `UNIT+13C=0`, `UNIT+140=-1` and `PLAYER+2C=00010001`, calls `CD3B0`, and preserves the explicitly absent local view. Player pool storage and the allocated record are supplied. `UNIT+D4=-1` deliberately excludes network-entity reflection; no Xbox session allocation is claimed.

The actual carried weapon is Lockout `weap E57B0406`, `objects\weapons\rifle\smg\smg`, file offset **31365092**. Its original authored fields are:

| Field | Original StringID | Resolved name |
|---|---|---|
| weap+288 | `05000720` | `rifle` |
| weap+28C | `03000D26` | `smg` |

This fixture supplies that actual datum in a preallocated weapon object and slot zero, executes original `101BB0` before equip, then supplies desired primary slot zero while current primary/secondary remain -1. Original [E9D50](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000e9d50.asm) receives `(unit,current,0,0)`, EAX=1, and chooses the desired slot itself. Its source `111490 -> 1CFDE0` animation capability check succeeds; `111500 -> 1D0A60` requests the actual `rifle/smg` class/name. Both current and requested primary become zero.

The ordinary request helper [110BF0](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/00110bf0.asm) adds flag **0x200** for an assigned player when original global `517FE8` is zero. The equip helper uses its own original flag-26 path. The report preserves these source call arguments instead of treating assignment as a clip-name substitution.

| Source-selected channel | Index | Actual clip name | Frames | Resource group / interior offset |
|---|---:|---|---:|---|
| Primary idle | 185 | `combat:rifle:idle` | 59 | 14 / 9344 |
| Secondary ready | 207 | `combat:rifle:ready` | 19 | 15 / 10672 |
| Secondary firing | 172 | `combat:rifle:br:fire_1` | 6 | 12 / 6928 |
| Secondary reload | 215 | `combat:rifle:smg:reload_1` | 50 | 15 / 24128 |

The firing result really has `br` in its authored clip name; original lookup selects it while manager class/name remain `rifle/smg`. It must not be replaced with a guessed SMG-specific animation. The suite retains its complete 108-byte record and actual resource dispatches.

Resource descriptors explicitly identify shared-cache location 2. The new resident adapter supplies **only exact bytes at their descriptor's masked shared.map offset**. It does not execute the resource scheduler or choose alternate groups. Local-cache groups remain those in the frozen inherited tool. Per-resource descriptor bytes, file, location, offset, size and SHA-256 are recorded in the original service rows.

## Original player input and field contract

The controller/device producer is an explicit input boundary: each supplied record is exactly 0x5C bytes. The normal original [14EAE0](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/0014eae0.asm) calls **150DD0, then 150F00, then 151C90**. That same ordering executes here. `150DD0` derives pressed/released edges from original player fields +38/+3C; `150F00` creates original unit action requests before `151C90` writes the unit control record.

Original [151C90](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/00151c90.asm) computes view direction using original x87 sine/cosine instructions from input yaw +4 and pitch +8:

`(cos(yaw)*cos(pitch), sin(yaw)*cos(pitch), sin(pitch))`.

`CCC60` and `151AA0` execute the original parent/seat and movement-basis gates. This fixture is parentless with seat -1 and current physical state zero; it does not establish their seated/vehicle alternatives. Input forward/left are +C/+10; input primary/secondary trigger floats are +14/+18. Input mode is written as original combat StringID `06000086`.

Input **+1E's four bytes must preserve the exact unit selector bytes +214..217**. In this source route they are copied through control+6. Supplying four -1 bytes requests unequip and creates action 8/18; that is an observed different input, not an interchangeable default. Input +22 grenade request and +24 zoom selector are explicitly -1.

Original [C8140](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000c8140.asm) receives the full original 0x7C control record in ECX. These proven transfers are retained without guessed semantic names for unknown vectors:

| Original control record | Unit destination |
|---|---|
| +00 u32 | +14C mode StringID |
| +10 u32 | +148 control flags |
| +14/+18/+1C floats | +1B0/+1B4/+1B8 movement inputs |
| +20/+24 floats | +1C0/+1C4 trigger analogs |
| +28..+30 vector | +150..+158 |
| +34..+3C vector | +15C..+164 |
| +40..+48 vector | +180..+188 |
| +04 byte | +1BC |
| +06 four selector bytes | conditionally +214..+217 |
| +0A short, when not -1 | +23D |
| +0C byte | +241 |
| +4C..+54 vector | +1A4..+1AC |
| +58 nine u32s | +1C8..+1E8 |

Original [C7420](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000c7420.asm) executes the actual aiming update. Authored Spartan +144/+148/+14C/+150/+154 are zero in this cache; its source zero-rate branch copies the original desired aim/head vectors, then executes `C5CD0`/`CCEA0` constraint gates and quantization. It is unsafe to replace this with an unconditional copy just because the authored rates are zero. The explicit yaw=.5/pitch=.25 fixture produces desired direction bits `3F59AD4E/3EEDD5BE/3E7D5777`; the first recorded current-aim state is still the ready-state direction, then the next tick records those bits. Original unit callback `113830` consumes current aim/body bases and source overlay selections while producing actual local channels.

## Firing, ready and both magazine reload situations

The bounded unit/weapon loop preserves original helper execution and records each phase: input/action conversion; isolated aim update; manager blend; `C6C50` unit controls and weapon input; `1021D0` weapon tick; `E7E60` active action callbacks; `112E10` unit animation clock; `BDF80` source local channels; `1019F0` projectile postpass. This is the fixture order, **not a claim that the entire original unit/world tick has been executed**.

Original normal wrapper [C7E90](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000c7e90.asm) orders `C5D10 -> C6C50 -> C7420 -> C7210 -> C7AA0 -> 116420 -> C7CF0 -> C7B70 -> C6400`. The fixture's isolated C7420 call is earlier; full wrapper/world integration remains explicit until independently executed. The established original object postpass samples pose before `1019F0`, as previously documented.

The actual equipped weapon has source ready countdown **15** at +17C. Holding fire from tick zero first emits at tick **14**, then 16,18,...44. The hold-45/release-15 case emits **16 original projectile requests** and ends with 44 loaded/120 reserve. Every source `1021D0 -> CB060(event1) -> E7EF0 -> 115650` firing action chooses secondary clip 172; the source primary remains 185. New firing events restart the secondary frame at zero. The supplied failed projectile allocator is the same explicit world boundary used in the earlier firing oracle; no shot result is invented.

For a requested positive-ammo reload at tick45, input+1C bit0 represents the supplied held interaction/reload button. `150DD0` derives bit2 pressed. With the explicitly empty nearby-interaction state, `150F00` writes flag **0x10000000** in the original unit input. `C6C50` emits primary action0 and secondary action10. Original [E88B0](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000e88b0.asm) invokes [E8740](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000e8740.asm) with primary-hand0, variant0. It first runs original gates and original `105640`, then `C9A40`, `106B80` and `115650(unit,reload_1,3E88B439,0)`.

| Positive-ammo source phase | Tick | Loaded / reserve | Magazine state / duration | Secondary clip / frame |
|---|---:|---|---|---|
| Reload action starts before weapon tick | 45 | 44 / 120 | 1 / 49 after update | 215 / 0 |
| Last countdown row | 93 | 44 / 120 | 1 / 1 | 215 / 47 |
| Actual transfer | 94 | 60 / 104 | 5 / -1 | 215 / 48 |
| Secondary expires | 95 | 60 / 104 | 6 / 0 | none |
| Magazine returns idle | 96 | 60 / 104 | 0 / -1 | none |

The emptied-magazine scenario holds fire for 225 ticks with original initial 60/120 ammo. Last initial-magazine shot is tick132. The natural source `106580 -> 101D50 -> E7EF0 -> E88B0 -> E8740` path starts reload **inside weapon tick137**, therefore its post-update timers are 50 rather than49. Actual first-person magazine timing still selects the original empty clip, as independently established in [SMG firing research](SMG_FIRING_1_1_RESEARCH.md). The third-person body action chooses the same actual SMG `reload_1` clip 215. Transfer occurs tick187 (0/120 →60/60); magazine state6 is188, firing resumes189. There are **78 original requests** over225ticks; final loaded/reserve is42/60.

Calling `105640` alone had changed ammo without establishing the body reload request: `CB060` event5 is ignored by its source switch. That isolated route is not used for the final suite's requested reload case.

The original reload animation clock emits authored sound **E94207BD** at tick59 for the requested reload and151 for the natural empty reload. Only its sound allocator `179B90` is supplied unavailable (-1); event selection and clock remain original. A source audio-pool lifecycle is outside this fixture.

The primary 59-frame clock preserves the original residual: tick58 is `38D00000`, tick59 is `3F800340`. It must not use integer frame modulus. Primary manager state, secondary clock/flags, overlay selectors, local transforms and all phase transitions are in the reports.

## Classified movement selector, and the remaining producer

Original [DE960](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000de960.asm) reaches ordinary biped physics only when unit+B4 has a valid physical datum. It builds the original `current/idle/0` request, invokes `E1800`, `DDBC0`, `1E9480`, then [E3B20](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000e3b20.asm). `E3B20` calls original `1E8F60` to produce physical classification and passes those results to [E44C0](../analysis/multiplayer/map-pack/spartan-equipped-animation-instructions/000e44c0.asm), before root-motion processing through `1D14F0`. That physical datum/state is not manufactured here. Supplying movement axis1 by itself leaves the recorded primary idle, which is explicitly not proof of a working locomotion producer.

The twelve separate selector fixtures supply classification states0..11 and current unit physical state0, then execute unchanged **E44C0**, **112E10** and **BDF80**. Calling convention is EAX=unit, EBX=request mode pointer, EDI=request activity pointer; stack is `(classification, blend_byte, request+8)`, RET12. `E44C0` changes activity only when incoming activity is idle; positive classification writes the supplied byte to request+8. The fixture byte is zero, not a recovered world output.

| Supplied classification | Original requested activity | Equipped source result |
|---:|---|---|
| 0,1 | idle | 185 `combat:rifle:idle` |
| 2 | move_right | 191 `combat:rifle:move_right`,20frames |
| 3 | move_left | 190 `combat:rifle:move_left`,20frames |
| 4 | move_front | 189 `combat:rifle:move_front`,20frames |
| 5 | move_back | 188 `combat:rifle:move_back`,20frames |
| 6..9 | stunned_right/left/front/back | Source request falls back to recorded primary185 under this fixture |
| 10,11 | climb_up/climb_down | Source request falls back to recorded primary185 under this fixture |

Separate unit physical-state branches 2/5 can change mode, and unit+34C=1 suppresses selection; those branches are listed but are not asserted covered by these twelve fixtures. No generic directional classifier, crouch, airborne, stunned or climbing animation is inferred from this table.

## Integration limits

- These are original executable outputs under copied multiplayer options state2, mode5 and simulation world state4, with player0 ownership established by original14E240. They establish a bounded authoritative input/request path, not a whole distributed session.
- A separate [equipped inventory oracle](../tools/oracle_mp_inventory_equipped_1_1.py) is recovering full `1A28A0 -> B8A30 -> CE510` initial loadout allocation. It closes the preallocated weapon boundary independently; it should not repeat E9D50 after CE510 already equipped the weapon. Its mutable research results are not silently imported into this frozen suite.
- This suite's carried-object marker/physics attachment `FB560`, muzzle-world query, projectile/effect allocation and audio allocation are supplied services. Input/device sampling, local view/first-person visual playback, normal physical classification, full weapon-object manager/attachment and world/session producers remain separate.
- The decoded-channel and manager/request results do not resolve Xbox matrix bit parity. Original `143960` uses RCPSS plus refinement; the [separate reciprocal report](SPARTAN_LIVE_POSE_1_1_RESEARCH.md#reciprocal-instructionemulator-boundary) demonstrates that this host's SSE approximation differs from Unicorn. No host division or host RCPSS output is asserted to match actual Xbox hardware. The original-input/request/clock/decoder evidence remains separate from that open hardware reciprocal boundary.
