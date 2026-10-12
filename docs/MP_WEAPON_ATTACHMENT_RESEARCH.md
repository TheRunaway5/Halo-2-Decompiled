# TU1.1 carried weapon attachment and marker transforms

The original `FB560` attachment, render-model marker lookup, and parent/list transforms now execute successfully for allocated stock Lockout SMG and BR weapons. Initial SMG and primary BR use the Spartan's authored `right_hand` marker on node **20 (`r_hand`)**. A secondary SMG uses authored `left_hand` on node **19 (`l_hand`)**. Holstering detaches a weapon while retaining its full datum and inventory owner; switching reattaches that same object. This is a separate five-case freeze, extending the declared attachment boundary in the [equipped pickup research](MP_PICKUP_EQUIPPED_RESEARCH.md). Earlier tools and reports are unchanged. [Original attachment oracle](../tools/oracle_mp_weapon_attachment_1_1.py), [captured executions](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

## Evidence and scope

The authority is the untouched TU1.1 update executable `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, and real Lockout cache, SHA-256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Shared animation resources come from the actual `shared.map`; their descriptor bytes and resource hashes are recorded and audited. The report contains 5 cases, 214 routine/fragment root rows, 10 hardware input samples, 2,055 captured attachment/transform calls, 140 object-link stores, and 27 original instruction listings containing 2,600 instruction rows. [Read-only audit tool](../tools/oracle_mp_weapon_attachment_audit_1_1.py), [audit](../analysis/multiplayer/integration/inventory-source/weapon-attachment/audit.json), [reproduction record](../analysis/multiplayer/integration/inventory-source/weapon-attachment/reproducibility.json).

The subclass removes only supplied services `FB560`, `1DC8F0`, and `1DC940` from the previous equipped fixture. Those entries and their parent/transform callees execute their original instructions. Item allocation, player assignment, equipped Spartan capability, input translation, ordinary pickup action, actual registry enumeration, BSP terrain visibility, and graph pose generation retain the previously proven original paths. This report records their new attachment results; it does not replace prior admission evidence. [Oracle](../tools/oracle_mp_weapon_attachment_1_1.py), [frozen fixture](../tools/oracle_mp_inventory_equipped_1_1.py).

Allocated unit/player arenas, heap backing storage, resident-resource delivery, hardware packets, and the explicitly nearby unit root remain fixture inputs. A local view of `-1` leaves the first-person graph outside this execution; `16A356` notifications and audio/effect services retain their recorded boundaries. Complete engine scheduling, world settling, and Havok are unresolved. The drop case reaches the supplied `246A50` Havok shape-test boundary once; its false return is explicitly recorded. That drop output must not be promoted to a complete terrain-plus-Havok placement proof. [Report `events`, `live_events`, and `collision_queries`](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json), [world boundaries](MP_ITEM_WORLD_RESEARCH.md).

The fixture deliberately excludes the unit and dormant index-0 weapon from the world registry; it registers the actual allocated target. The starting BR target has its original collidable link in cluster9; successful pickup removes that link. Registry observations remain empty immediately after the direct BR drop. This suite establishes the source attachment lifecycle and immediate transform, not the later normal dropped-item tick or completed world registration. [Registry observations](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json), [fixture storage setup](../tools/oracle_mp_inventory_equipped_1_1.py).

## Fixture and exact root order

The inherited fixture executes the original unit initialization, `14E240` player assignment, then `1A28A0` default FFA loadout. Original allocation creates the starting SMG **`00010002`**, at supplied heap backing address `04000000`; it is not the dormant preallocated weapon at index 0. The allocator's full datum and header salt survive every attachment transition. The initial SMG is equipped through the original manager and slot transition; its selected combat idle is clip **185**. [Frozen construction/order contract](MP_PICKUP_EQUIPPED_RESEARCH.md), [new report `initial`](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

The fixture then initializes/resets netgame entries and executes `15C110` for authored BR row **1** or SMG row **12**, allocating target **`00020003`** at `04002000`. Actual authored row bytes and the eight-byte runtime row entry are retained in this report. The Spartan root is explicitly placed at the target's original bound center plus `0.05` on x. Facing comes from authored player start 0 through original `11F680`; `BDF80` evaluates the equipped graph, `BEDE0` produces bounds, and `11D4A0` produces the current BSP location. This is an explicit proximity fixture, not an executed walk from the start. [Oracle `place_player` inheritance](../tools/oracle_mp_inventory_equipped_1_1.py), [report `pose_fixture` and `authored_row_record`](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

Each pickup case seeds the already held duration at **232 ms**, then executes the ordinary source input/control/unit phase for a 1 ms poll to reach **233 ms**. Primary button 2 picks up the BR; secondary button 3 picks up the other SMG. This bounded threshold probe has one hardware sample per pickup, and inherits the prior frozen suite's full held/release admission evidence. After initialization and each tested transition, this new oracle explicitly runs **`BDF10(unit)`**, then **`BACC0(item)`** for each allocated item, recording actual current node matrices, bounds, world points, and registry. These observation roots are not a fabricated full object pass. [Input/root sequence](MP_PICKUP_EQUIPPED_RESEARCH.md), [oracle `observe_pose` and `run_cases`](../tools/oracle_mp_weapon_attachment_1_1.py).

The delayed-switch case releases primary twice, presses/releases secondary twice, then advances the ten previously proven graph phases. Each phase runs `1D0ED0`, `C6C50`, both weapons' `1021D0`, `E7E60`, `112E10`, and `BDF80`, in that order. Current primary remains BR through phases 0–3 and becomes original SMG at phase 4. The drop case instead invokes original **`CFD80(1,0,0)` with `EBX=unit`** directly after BR pickup; it does not claim recovery of a drop-button producer. [Frozen continuation](../tools/oracle_mp_inventory_equipped_1_1.py), [drop instructions](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000cfd80.asm).

## Actual cache records and marker contracts

Model resolution is object definition **`+38` → `hlmt`**, then `hlmt` **`+4` → `mode`**. `hlmt+0C` is not the render-model link. The original `B9C20` reads `hlmt+4` at `B9CAB`; the report includes both exact link words, node/group block descriptors, all actual nodes, and all marker group/entry bytes for these three models. [B9C20 listing](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000b9c20.asm), [report `authored_fields`](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

| Object | `hlmt` | `mode` | Relevant authored fields |
|---|---|---|---|
| Spartan `E9F2086D` | `E9F3086E` | `E9070782` | `+158=0A000798` right_hand; `+15C=09000100` left_hand; `+160=0D001457` right_hand_mc |
| SMG `E57B0406` | `E57C0407` | `E57D0408` | weapon `+284=00000000` |
| BR `E192001D` | `E193001E` | `E194001F` | weapon `+284=00000000` |

`D1D20(unit, hand)` receives the weapon datum in **EDI**. Hand 0 reads unit definition `+158`; hand 1 reads `+15C`. It tries definition `+160` as a marker on the weapon through `B9C20` with `CL=1`. Neither actual weapon render-model contains `right_hand_mc`, so this query returns **AX=0**, and `D1DC0` falls back to the actual weapon definition's `+284`, zero. It calls `FB560` with **EAX=unit**, **ECX=weapon**, and two stack words: parent marker and child marker. `FB560` pops eight bytes; its return EAX is not a boolean attachment contract. [D1D20 instructions](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000d1d20.asm), [FB560 instructions](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000fb560.asm), [captured registers/returns](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

Render-model nodes use count/pointer at `mode+48/+4C`, stride **60 hex**. Marker groups use count/pointer at `+58/+5C`, stride **0C**: string ID at `+0`, marker count at `+4`, marker pointer at `+8`. Marker entries have stride **24**: signed region byte `+0`, permutation byte `+1`, node byte `+2`, translation at `+4`, quaternion at `+10`, trailing scalar at `+20`. `1DC8F0` receives mode datum in **ECX** and marker ID in **EDI**, returns the first matching group index or `FFFFFFFF`. [Original lookup](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/001dc8f0.asm), [marker evaluation](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/001dc940.asm).

| Spartan marker | Group | Node | Actual translation | Actual quaternion |
|---|---:|---:|---|---|
| right_hand | 9 | 20 r_hand | `(0.0465319417, 0.00432991888, -0.00443904288)` | `(-0.0160801131, -0.0736130998, 0.0850803182, -0.993520975)` |
| left_hand | 7 | 19 l_hand | `(0.0385863706, 0.00105646206, 0.000397729425)` | `(-0.993520975, 0.0850799531, -0.0736134127, -0.0160778426)` |

These rounded readable values accompany the exact raw records in the report. Both entries have region/permutation `FF/FF` and scalar zero. The direct drop case also evaluates group **1 `drop`**, node **15 `head`**, before the original detach; groups `drop_left` and `drop_right` are captured cache metadata but not claimed as exercised drop choices. [Exact group/node/marker records](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

`1DC940` receives group index in **EAX**, mode datum in **ECX**, then stack arguments: region-state pointer, optional node-map pointer, current node-matrix pointer, mirror byte, output pointer, maximum count. It pops **18 hex** bytes. Our actual calls use no node map, mirror 0, and maximum 1; region `FF` bypasses the permutation filter. Missing group returns zero without defining output bytes. Positive output records have stride **70**:

| Offset | Defined output |
|---|---|
| `+0` | Signed 16-bit selected node; `+2` padding is not defined |
| `+4` | 52-byte local marker matrix |
| `+38` | 52-byte world marker matrix |
| `+6C` | Authored trailing scalar |

The 52-byte matrix has scale at `+0`, three three-float basis vectors at `+4/+10/+1C`, and translation at `+28`. Original `143580` builds the marker rotation from the quaternion; `1DCA0C`–`1DCA1A` copy translation; `1441D0` composes it with the actual current node matrix. Mirror handling at `1DCA42`–`1DCA64` negates world second-basis fields, but that branch is unexecuted in these ordinary cases. [1DC940 listing](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/001dc940.asm), [quaternion conversion](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/00143580.asm), [matrix composition](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/001441d0.asm).

`B9C20` receives the object/marker/maximum as three stack words, `CL` as the direct-object selector, and output in **EBX**. On missing marker it writes defined node 0, identity local matrix, a copy of the object's first current node matrix as world matrix, and scalar zero. The result count is still **AX=0** for missing nonzero `right_hand_mc`; marker zero produces **AX=1**. Upper EAX bits and output padding are not part of this contract. Consequently ordinary SMG/BR attachment takes `B9DD0`'s zero-child-marker branch; the general nonzero-child-marker `BA1C0` branch is listed but unexecuted. [B9C20 fallback at B9D12–B9DBC](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000b9c20.asm), [B9DD0 branch at B9E40–B9E6B](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000b9dd0.asm).

## Link, transform, and inventory ordering

`FB560` first executes `FB490(child)` to clear item `+12C` bit 1 and object `+4` bits `80/01`, with original activation callbacks. It then invokes `B9DD0(parent, parent_marker, child)`, child marker in EAX. Afterward it tests parent definition `+BC` bit 5. The actual Spartan word is **`00448008`**, with bit 5 clear, so the subsequent branch that sets child `+4` bit 0 is unexecuted here. The report includes the exact definition flag word and recorded child flags. [FB490](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000fb490.asm), [FB560 at FB5A7 onward](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000fb560.asm).

`B9DD0` obtains the actual parent marker and child root fallback records. In the exercised zero-marker branch it inverts the child world matrix through `142D00`, composes the original scalar SSE operations with the parent marker world matrix, normalizes with `BA0C0`, and writes world placement through `B8320`. It then calls **`BA2A0(parent, child, parent_node)`**. This routine validates admission, links, and converts the current world placement into the selected parent's node-local space. Copying the authored translation directly would miss the original inverse/composition rounding. [B9DD0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000b9dd0.asm), [BA2A0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba2a0.asm).

`BA2A0` first requires child parent `+14` and sibling `+0C` to be `FFFFFFFF`. When `(1 << child_type) & 1C` is nonzero and item `+12C` bit 0 is set, owner `+154` must equal the proposed parent; the actual weapons satisfy that type gate. It then walks the proposed parent's full-datum parent chain and rejects a cycle before stores. Existing parent/sibling failure is accumulated before this chain check. Ten requests succeed; the direct drop path's request to link the already attached BR at the `drop` marker is rejected, leaving its existing parent/node unchanged until the later detach. Do not make that attempted drop link succeed in a port. [BA2C3–BA330 admission, BA310 cycle check](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba2a0.asm), [report `calls` including failed BA2A0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

| Original store | Field and meaning |
|---|---|
| `BA3FA` | child `+14` = full parent datum |
| `BA405` | child `+18` = node byte |
| `BA418` | child `+0C` = previous parent first-child datum |
| `BA41F` | parent `+10` = full child datum |
| `BA422` | child header `+2` gains bit `80` |
| `BA42A/BA42D/BA438` | child location is reset with active BSP index |
| `BA448` onward | inverse selected node → source local placement/normalization/write |
| `BA6C8/BA6D4` | original pose/bounds update, then header dirty bit `20` |

Full source-datum resolution and runtime callbacks follow these stores. Constructor-created unparented target node byte is 0 in this fixture; detached node byte becomes `FF`. Those are distinct source states. [BA2A0 listing](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba2a0.asm), [captured object bodies/headers and writes](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

Holstering uses `FB3F0`, which calls **`BA980` with EBX=child** if it has a parent. `BA980` captures the old parent and selected current node matrix before calling `BA780`. `BA780` calls `BF540` to walk the parent's child list and splice the matching full datum: `BF571` writes the removed child's sibling into the parent/previous-sibling link; `BF573` clears child `+0C`. `BA7C0` clears child `+14`, and `BA7C3` sets child node to `FF`; header parent bit `80` clears. `BA7D0` then composes the child's old local placement with the retained node matrix and writes its world transform through `B8490`. The subsequent `B86C0` call uses the old parent's linear/angular fields; it is not an independently guessed detach impulse. [BA980](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba980.asm), [BA780](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba780.asm), [BF540](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000bf540.asm), [BA7D0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000ba7d0.asm).

`FB3F0` sets item `+12C` bit 1 and object `+4` bits `01/80`, with original unregister/activation calls. Owner `+154` stays the unit during holstering. Conversely `FB120` with owner `FFFFFFFF` writes item creation tick and clears ownership at **`FB2EC`**. Inventory slot ownership, physical parent linkage, current/requested indices, and world registration must remain distinct. [FB3F0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000fb3f0.asm), [FB120](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000fb120.asm), [slot transition CFED0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000cfed0.asm).

## Frozen results and numerical boundary

| Case | Physical result after original observation | Ownership/current selection |
|---|---|---|
| Starting SMG | SMG `10002` parent unit `10001`, node20, first child10002 | SMG owner10001; current `[0,FF]` |
| Primary BR row1 | SMG detached/nodeFF; BR `20003` parent10001/node20, first child20003 | Both retain owner10001; current `[1,FF]` |
| Secondary SMG row12 | New SMG20003 parent10001/node19, sibling10002; old SMG still node20 | Both owner10001; current `[0,1]` |
| Switch back | BR detached/nodeFF; original SMG10002 reattached/node20 | Both retain owner10001; current `[0,FF]` after phase4 |
| Direct BR drop | Both detached; first childFFFFFFFF; BR nodeFF | BR ownerFFFFFFFF; old SMG owner10001; current still `[1,FF]`, requested `[FF,FF]` immediately after root |

All item definitions, authored row identities, loaded/reserve ammo, and zero heat survive these attach/detach operations. Initial SMG remains 60/120; BR remains 36/72; secondary SMG remains 60/120. Carried scale is source 1.0; dropped BR returns to its source 1.7. The drop routine consumes the shared stream from `12345678` to **`4B5C952C`**; other cases leave `12345678` unchanged. [Raw object and RNG evidence](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

Actual right-hand local position after initial equip is `3D3E9848 3B8DE240 BB917570`. BR pickup produces `3D3E9900 3B8DE300 BB917800`. Delayed SMG reattach produces `3D3E9840 3B8DE400 BB917600` under the then-current animated node matrices. These differences are source arithmetic under different current poses, so the authored marker translation is an input, not a universal final local-position bit pattern. `BACC0` resolves a parented object's world point through the selected node and local position; an unparented object copies its world root directly. [BACC0](../analysis/multiplayer/integration/inventory-source/weapon-attachment/instructions/000bacc0.asm), [report `observations`/`calls`](../analysis/multiplayer/integration/inventory-source/weapon-attachment/report.json).

The original instructions execute unpatched in Unicorn. This new trace observes `RCPSS` at **1439DF** and `RSQRTPS` at **29277B/2931E7** in its pose environment. Marker conversion `143580` itself uses the original division path, but its animated parent matrices depend on those original pose instructions. The existing [Xbox reciprocal boundary](SPARTAN_LIVE_POSE_1_1_RESEARCH.md#reciprocal-instruction-emulator-boundary) therefore remains: reproducible emulator matrices do not establish exact Xbox matrix bits. Do not replace the Xbox reciprocal approximation with host reciprocal/division and call that verified parity. No replacement gravity, friction, hand offsets, or Havok behavior is supplied here. [Instruction-byte audit](../analysis/multiplayer/integration/inventory-source/weapon-attachment/audit.json).

## Reproduction and integration gate

Run the separate oracle, then its read-only audit:

```sh
.tools/python/bin/python tools/oracle_mp_weapon_attachment_1_1.py
.tools/python/bin/python tools/oracle_mp_weapon_attachment_audit_1_1.py
```

Use `--output` with a separate absolute report path when reproducing; this oracle also resolves relative paths itself. The freeze records a complete second execution with byte-identical report output and original listing hashes. [Reproduction](../analysis/multiplayer/integration/inventory-source/weapon-attachment/reproducibility.json).

The frozen source oracle hash is `9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c`; report hash is `657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f`. Dependency hashes and preservation checks for all seven earlier inventory reports are in the audit.

The implementable slice is stable full-datum parent/child ownership, source-selected hand marker lookup, exact attach/detach store order, source transform composition using the actual current unit/weapon node matrices, and source pose/bounds callbacks. Retain the independent slot/owner/current state and the recorded allocation/resource/physics boundaries. This evidence closes the prior supplied `FB560` and marker-query services for the tested stock cases. Exact Xbox reciprocal results, full first-person channels, complete world scheduling, nonzero child-marker attachment, mirrored variants, and Havok settling remain unresolved.
