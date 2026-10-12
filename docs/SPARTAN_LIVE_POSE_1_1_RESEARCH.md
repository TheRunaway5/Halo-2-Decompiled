# Original disc 1.1 valid Spartan animation and region inputs

The original executable now runs a bounded valid-animation sequence from actual Spartan graph binding and full unit initialization through animation ticking, original sampling, the unit local-pose callback, world matrix production and posed collision. The recorded slice is an initialized, unassigned unit using the original biped's default `current/current` request. It does not establish the equipped, moving multiplayer player controller or its full physics/action producers.

No production files or previous frozen suites were changed. This report also records a newly isolated reciprocal-emulation limit that applies to both default and animated matrices.

## Reproduction and pins

```sh
.tools/python/bin/python tools/oracle_spartan_live_pose_1_1.py
.tools/python/bin/python tools/oracle_spartan_live_pose_1_1.py --mode 1 --output analysis/multiplayer/map-pack/spartan-live-pose-mode1-1.1-oracle.json
.tools/python/bin/python tools/oracle_spartan_live_pose_1_1.py --regions --output analysis/multiplayer/map-pack/spartan-region-state-1.1-oracle.json
.tools/python/bin/python tools/oracle_spartan_live_pose_1_1.py --instruction-inventory analysis/multiplayer/map-pack/spartan-live-pose-original-instructions
.tools/python/bin/python tools/oracle_pose_reciprocal_1_1.py
```

The executable is disc 1.1 `default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`; Lockout is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The live tool verifies the unchanged default oracle dependency `d3a8a87bcadd810ff0e47beb57f0b03fa1fc592164cb64cfe8819eccf7c6a0d4`, which in turn verifies the frozen posed-collision oracle. Expected results come from unchanged original instructions.

| Output | SHA-256 |
|---|---|
| `tools/oracle_spartan_live_pose_1_1.py` | `28afd44db12c96a67bc82bca1c617cbd9fb8fa631e949f71e9b74743d2de10f5` |
| `spartan-live-pose-1.1-oracle.json` | `7b81ca2ac3d5f43d0e8abf2c3a8d7b13472b31073605def2d78984b28bd9126d` |
| `spartan-live-pose-mode1-1.1-oracle.json` | `904272027894a123efda01694a8d44d6efc4230419f622b6c4d3254f31c49faa` |
| `spartan-live-pose-mode-comparison.json` | `db45e225cc1bb67ed01e426518d9f4cab3c313be3e5a44630665816638886370` |
| `spartan-region-state-1.1-oracle.json` | `f5c7cbbb9d69cb74e19ec670892d520af4c0e99fcda84acce606adfe1f4489b4` |
| `spartan-live-pose-original-instructions/inventory.json` | `0f3b65c0b6312154a160242addd878e9012bc9734192e2b4115dc57e5ce0929d` |
| `tools/oracle_pose_reciprocal_1_1.py` | `2f010b5d88e1b3af477a90b9574a607e2960635ae7019256ea9b8120bd591a91` |
| `pose-reciprocal-1.1-boundary.json` | `aef6207fe587330414d72e3dbcc6966e87e21b5ce2d2436f1e3d92a737f12286` |

Report paths are under `analysis/multiplayer/map-pack/`. The new instruction inventory contains 45 original entries, including graph/state lookup, clock/looping, source constructors, the unit callback and dynamically observed codec dispatch targets. Each listing is separately hashed.

## Actual initialization route

The actual metadata route is Spartan `bipd E9F2086D` -> `hlmt E9F3086E` -> graph at hlmt+14, `jmad E9180793`, `objects\characters\masterchief\masterchief`, root file offset 32035472. Render model/node/region selection comes from the already verified actual mode/hlmt route in the default-pose report.

The fixture provides existing object storage, placement and pose allocations, plus a 90-hex-byte (144-byte) manager at object-relative +3800. It executes original `1CE8C0` and binds that manager with `1CEAC0(1)`, ESI=manager, EDI=actual graph datum, EDX=actual hlmt datum. The original node-count compatibility check succeeds. Original `1CEAC0` calls `1D0A60` with authored default state requests; `1CFCE0` resolves them to `combat/unarmed/any/idle`, and the original graph lookup selects packed animation ID `0104FFFF`.

That ID points to actual clip 260 named **`combat:sword:idle`**. The lookup result is recorded exactly; the name is not a manually selected substitute for `unarmed`, and this result does not imply that an equipped SMG uses this clip. Its original metadata has 59 frames, loop frame zero, group index 16 and group offset 29F0.

After the successful bind the fixture applies source `B8A30`'s bit-11 success flag, then executes the complete original unit initializer **`C52C0(UI, descriptor, failure)`**. The descriptor is an explicitly recorded zero-initialized C4-byte input, with the actual Spartan definition and category short +64=-1; the complete multiplayer spawn descriptor producer is outside this slice.

`C52C0` calls original `BD270` to allocate and zero these blocks. The initial object extent +3A00 is a supplied arena boundary. The only constructor service is three `18FD70` heap-growth requests returning success, with sufficient already reserved storage and no relocation. The object field writes, alignment, extent increments and memory zeroing execute original `BD270`.

| Field pair | Original requested bytes | Resulting object-relative offset |
|---|---:|---:|
| +33C/+33E unit extension | 124 hex = 292 | 3A00 |
| +340/+342 extra unit state | 58 hex = 88 | 3B24 |
| +344/+346 action/control state | 44 hex = 68 | 3B7C |

The full original initializer executes `110A20`, `116680`, `110BF0` and `114BD0`, initializes the real secondary channels and unit pose inputs, and copies the object forward vector into the source unit aiming/orientation fields. This replaces the previous experimental uninitialized extension that accidentally made the unit callback read a nonexistent graph. Unit +13C is set to **-1 by the original constructor**: no assigned player controller is claimed. Inventory slots are also reset by the original constructor.

## Original tick and pose sequence

For each recorded tick the fixture executes:

1. `1D0ED0`: manager blend counters, as in the object update phase.
2. `E7E60(UI)`: active action callbacks using the original initialized control block.
3. `112E10(UI, request)`: original unit animation update, graph/state checks, channel clock and auxiliary state updates.
4. `BDF80(UI)`: real valid-graph pose production, including `BEA50`, `1D0F80`, `1CA950`, resource selection, unchanged decoder instructions, `113830`, `143960` and `1441D0`.

The 12-byte request contains `07000101/07000101/00000000`, the exact `current/current/0` initialization at original **F1C72..F1C8A**, immediately before `E7E60` and `112E10` in the normal biped update `F1420`. The upstream physical/action state that can activate callbacks or change this unit's modes is not executed here. The full normal object-phase order remains the source order documented in the default-pose report, rather than the entire engine being emulated by this tool.

`112E10` calls `1CF100`, which advances valid channels through `1CA2B0`/`1CA090`/`1C9F50`. The source clock uses separately rounded `(tick_seconds * channel_rate) * 30 + old_frame`. Original looping subtracts `(frame_count - epsilon)` and adds authored loop frame, retaining its residual. With the real 59-frame clip:

| Tick | Before frame bits | After original animation tick |
|---|---|---|
| 56 | 42600000 (56) | 42640000 (57) |
| 57 | 42640000 (57) | 42680000 (58) |
| 58 | 42680000 (58) | 38D00000 (0.00009918212890625) |
| 59 | 38D00000 | 3F800340 (1.0000991821289062) |
| 60 | 3F800340 | 400001A0 (2.0000991821289062) |

Do not replace this loop with a frame-count modulus. The report records all 75 before/after phase states, complete manager and unit blocks, every local transform and all 33 current node matrices.

Original `BDF80` starts with authored mode defaults. Under this actual hlmt/fixture mask, `BEA50` trims primary sampling to a 21-node prefix, which contains all of the original Spartan collision BSP nodes; `BDF80` still produces all 33 current matrices. The remaining local transforms follow the original default/mask path. The report exposes actual sampler mask pointers/counts and original 32-byte codec headers; it does not infer a render-wide animation mask.

## Resident-byte boundary and codec evidence

The real resource-pool state at globals +4EB11C/+4EB124 is not instantiated. The only pose/update service is **`124F30`**, returning the exact resident local-cache group bytes named by the original descriptor in EAX, with the original four-byte argument/return stack cleanup. No animation results are supplied. `1E11B0` performs the source interior-offset selection, and original sampler/decoder instructions consume the bytes.

| Original group | Lockout raw file offset | Bytes | SHA-256 |
|---|---:|---:|---|
| 16 | BB4800 | 78E4 | `d039d4c8457bc1f02905c2f83e0a6c4e6b956a0a01a47905e16d021a348d012c` |
| 17 | BBC200 | 7E48 | `094c69be1eb6aa467c8cc7be87c1ea136790a8aafa7bb2b41298522c178dbbc6` |

Both descriptors explicitly name this cache. Shared-file requests are rejected by this bounded adapter; an alternative file is never substituted. There are 80 resident-byte requests: 75 group-16 and five group-17. The recorded `27FB10` dispatches are 75 static codec-1 and 75 animated codec-4 samples in source sampling mode zero, plus five codec-8 dispatches in mode three. The executed dispatch targets are:

| Codec | Original component decoder entries |
|---|---|
| 1 | 292720, 292790, 2927C0 |
| 4 | 293070, 293200, 293350 |
| 8 | 292970, 2929B0, 2CB1E0 |

These entries and the sampler routines are in the instruction inventory. Metadata includes the original complete clip record and per-dispatch frame, interpolation bits, component masks and codec header. The native decoder does not supply any expected sample.

## Coverage and mode boundary

The canonical run supplies options state 2, options mode 5 and simulation world state 4, as in the prior authoritative multiplayer firing fixture. A separate mode-1/world-1 run produces exactly the same 75 complete phase-state rows, original event records and 16 posed centroid queries. This proves the bounded initialized-unit slice does not differ between these supplied modes; it does not prove an assigned multiplayer player controller, networking or physical movement.

The 75 ticks produce 64 distinct local pose outputs. Every tick stores all 33 original current matrices. The last original produced pose is fed into the frozen posed-collision consumer: all **16** rays through explicit authored BSP vertex-average fixtures along the corresponding original node axis hit. A separate fixed world ray is also recorded and misses; it is not treated as a required hit. Both shared and local seed write monitors record zero stores in this scope.

## Stock Spartan active-region lifecycle

The separate region report executes original `BB6E0` with the exact death/reset call arguments inspected in **D8070** (`UI,-1,2,-1`) and **D73E0** (`UI,-1,2,0`). It also executes `BB5E0` for all four model regions with explicitly varied damage-state arguments 0..5; those 24 varied calls are fixture coverage, not claims that each state occurs in an ordinary live event.

All **26** calls preserve the original initialized 40-byte region state and all four active permutation bytes. No engine service executes during the selectors and no shared seed changes. Original `16F0E0`/`16F550`/`16F760` execute on the actual stock variant: the sole `default` variant has helmet/body/arms/legs entries, each with one `base` permutation and **no authored state records**. The candidate fallback for a flag request selects the actual base entry with zero state/flags; a requested nonzero damage state has no matching candidate and does not overwrite state. A native implementation that simply stores every requested flag/state would therefore differ from this original selection path.

This establishes the tested stock variant's initial/death/reset selector behavior. Arbitrary named permutation overrides, removed region masks, model variants and dismemberment producers remain separate; no general claim is made from this one stock table.

## Reciprocal instruction/emulator boundary

`143960` squares four quaternion components and accumulates **(q0²+q2²)+(q1²+q3²)**. Above its source epsilon it executes:

```text
r = RCPSS(norm)
p = round(norm * r)
p = round(p * r)
r = round(round(r+r) - p)
r = round(r+r)
```

The separate reciprocal tool records each original stage for 33 actual mode-node quaternions and 1000 explicitly varied quaternion fixtures. It then runs this machine's physical SSE intrinsics with MXCSR 1F80 and contraction disabled. The host is an AMD Ryzen 7 9800X3D; it is not Xbox CPU hardware.

The host's raw RCPSS agrees with Unicorn in **0/1033** cases. The refined reciprocal agrees in **636/1033**, and only **4/33** authored nodes. Host DIVSS and the separately rounded division-based refinement agree with the emulator in **1033/1033** cases. For source norm bits 3F800000, the emulator RCPSS returns 3F800000 and refines to 3F800000; this host RCPSS returns 3F7FF000 and refines to 3F7FFFFF.

Consequently, host `_mm_rcp_ss` cannot be substituted while expecting the existing emulator matrices to match. Division matching the oracle is also not proof of the Xbox CPU's RCPSS lookup. Original instruction order is established, but exact physical Xbox reciprocal behavior needs a corresponding hardware-specific authority/capture before claiming bit-exact Xbox matrices. Neither frozen oracle was changed to disguise this boundary.

## Remaining ordinary-player producer

The bounded slice is executable and suitable for differential work under its recorded inputs. The next integration gate is the actual assigned player's unit/weapon lifecycle: +13C ownership, inventory/equip class state, biped/control inputs that select modes and active actions, movement/aim weights, physical node overrides and the relevant parent/attachment flags. `F1420`, `E7E60`, `112E10`, `110BF0`, `113830` and the valid-manager branch in `BDF80` locate the source route. The initial clip proven here must not be wired to every moving or equipped player.
