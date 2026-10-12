# Original owned Spartan animation, physical selector and current pose

The original source-created and equipped Spartan produces a changing third-person pose on the first reachable object update. On the actual allocated unit, the unchanged physical classifier naturally produces idle classification zero; `112E10` advances primary clip185 from frame0 to1, then `BDF80→113830` changes **19 local transforms and all33 current matrices**. No movement input, clip, frame, physical classification, pose, contact, activation or successful service result is assigned by this extension.

This independently measured source composition preserves the earlier [player/PVS](MP_PLAYER_VISIBILITY_RESEARCH.md), [active-item](MP_ITEM_ACTIVE_SCHEDULE_RESEARCH.md), [equipped animation](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md), [normal unit phase](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md) and [tick-owner](MP_ITEM_TICK_OWNER_RESEARCH.md) freezes. It establishes the allocated owner's natural **neutral** classification and one actual pose update. Genuine non-neutral device/input production and successive full admitted ticks remain open.

## Evidence and reproduction

```sh
.tools/python/bin/python tools/oracle_mp_spartan_owned_pose_1_1.py \
  --output analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/reproduction/report.json
.tools/python/bin/python tools/oracle_mp_spartan_owned_pose_audit_1_1.py \
  --output-dir analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/reproduction
```

The authority is the unchanged TU1.1 executable `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, with actual Lockout/shared cache data. This is the particular pinned executable's behavior; the experiment makes no unverified assertion about another patch or Xbox hardware.

| New artifact | SHA256 |
|---|---|
| [Oracle](../tools/oracle_mp_spartan_owned_pose_1_1.py) | `cee290577a992a2b8b799409da666ba49cc148fe11982bdcb44c49525c81b6e2` |
| [Independent auditor](../tools/oracle_mp_spartan_owned_pose_audit_1_1.py) | `b03fdcbf311f1abaa32feab98b8b0232e9a724e286b15fecaef9ae7b109995e5` |
| [Measured report](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json) | `4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb` |
| [Literal buffer audit](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/audit.json) | `26aed7140453fe9e1079dc1da3dfe811f00813e9eb6c51b7c9de848a29092463` |

The report contains582 completed calls,67 complete deduplicated states,15,814 original reads,43,993 ordered stores and68 unchanged instruction files/24,588 listing rows. All12 prior active-item runtime sections compare byte-identically under the new read-only observer; their combined runtime SHA256 is `8a019fcd281e8c72140f809e15b740ac56580b31aafaeed43ed4d44a55af48e2`. The auditor verifies200 prior pins, original instruction bytes,544 complete body/header/clock/seed intervals and847 complete ABI packet intervals, including26,273 overlapping read/store checks. None of the expected values comes from a native implementation.

Fresh-process reproduction of the report, auditor output and every instruction file is required by the adjacent `independent-freeze-verification.json`; its `manifest.json` pins the exact final files. Earlier probe/candidate captures under `spartan-owned-pose/` are excluded drafts, retain their bytes, and are not the authoritative measured freeze.

## Why the current renderer cannot express the source animation state

The current native [multiplayer renderer](../src/multiplayer_runtime.cpp) loads the equipped idle action once around lines322–323. At lines1135–1141, body skinning and the carried weapon's parent-hand node both sample that single `player.idle` using `current.tick*tick_seconds`. If that optional load failed, the same call uses model bind pose. The time argument advances; this is not an observation of a zero render clock.

That path does not consume source manager primary/secondary channels, `C7E90` action requests, `DE960` physical selection or `113830` overlays. Walking, firing and reload therefore do not select their source body state there. An idle-only actor can look stationary even while its small idle motion changes. This inspection identifies the missing producers; it is not a capture of the user's running renderer or proof that every reported live actor hit the bind-pose fallback.

The generic [model sampler](../src/model_pose.cpp) also computes `seconds*30` and wraps by ordinary frame count. The original frozen animation clocks retain their epsilon residual at a loop boundary; they cannot be replaced by integer frame modulus. The existing model API can skin supplied local transforms, but no production renderer is changed by this research. Source primary/secondary request selection, channel clocks, local-channel callback and current-matrix construction must remain distinct producers.

## Real owner, allocation and equip

The preparation is the unchanged original selected callback/world/player order from the immutable player helper. It joins through `14D350`, spawns through `1509C0`, allocates the real biped and SMG, executes `14E240`, and follows initial inventory allocation through `CE510→CFED0→FB560/E9D50`. The helper then executes the actual `14EAE0` prefix through the owning cluster writer. No local-device identity, current player unit or cluster is manufactured.

The unchanged selected authored rows0/1/12 are allocated before the real `13A380` activation. All authored roots/retained datums, masks, registry order, queue state, world/body/component allocations and four following object phases match the earlier active-item report exactly. The source PVS is `FFDF0000` followed by60 zero bytes, from source start37/current cluster4; no all-active replacement is used. [Exact prior correlation](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json).

| Source-owned record | Actual value/range |
|---|---|
| Player / current unit | `EC700000` / `E26F0000` |
| Unit body | `82D87FC0`, `0x1676` bytes (5750 decimal) |
| Physical datum / physical state | `E1680000` / unit+3DC byte1 |
| Equipped SMG datum | `E2700001`, slot0 |
| Animation manager, +128/+12A size/offset | `0x90` / `0x1426`, pointer`82D893E6` |
| Unit extension, +33C/+33E | `0x124` / `0x14B6`, pointer`82D89476` |
| Local bank, +110/+112 | `0x420` / `0xBD0`:33 original0x20-byte channels |
| Current matrix bank, +114/+116 | `0x6B4` / `0x4EC`:33 original0x34-byte matrices |
| Region bank, +118/+11A | `0x28` / `0xBA0` |

Complete bodies, headers, physics component/motion/body buffers, player records, all pose banks and the inherited clock record are retained at their owning stages. Extracted offset ranges are admitted only when the actual allocated body bounds them. [States and original allocation/equip calls](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json).

The **real allocator route differs from the old preallocated ready fixture**. Actual `E9D50` call186 has EAX1 and stack `(E26F0000,07000101,1,1)`, from caller`EA12B`. It changes primary from packed`0104FFFF` to`00B9FFFF`, selecting graph`E9180793`, mode`06000086`, class`05000720`, weapon name`03000D26`, activity`0400000C`. Source clip185 is `combat:rifle:idle`,59 frames, resource group14/interior offset`2480`.

The secondary channel remains packed`FFFFFFFF` and carried weapon+17C remains raw`0000FFFF` through this measured update. The older equipped oracle called `E9D50` with different supplied arguments `(unit,current,0,0)` and preallocated weapon state, selecting ready207/countdown15. Its result is valid for that declared fixture; it is not the real allocator's normal ready state. Do not combine those states or append a second equip call after the real `CE510` path. [Measured equip states15→25](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json), [older explicit boundary](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md).

## Natural classification producer and its input ownership

The first admitted unit callback is in the **new-object phase**, not a manually dispatched animation tick. The original type hierarchy supplies unit callback`C7E90` and biped callback`DE960`; actual `10CDF0` calls each with stack unit datum and original callback in EAX, from return site`10CE3D`. Both consume the actual `E26F0000`. The source order is preserved:

`B8040→BD360→1D0ED0→10CDF0→C7E90→DE960`.

`C7E90` executes `C5D10→C6C50→C7420→C7210→C7AA0→116420→C7CF0→C7B70→C6400`. `DE960` then builds its own `current/idle/0` request and executes `E1800→DDBC0→1E9540→1E9480→E3B20`; the final action/animation update is `E7E60→112E10`. The unit is followed by its actual carried child in the original recursion. [BD360](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000bd360.asm), [DE960](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000de960.asm).

Movement classification is **not produced by reading native world velocity**. `DDBC0` at`DDF2E` passes the actual unit+1B0 vector. `1E9540` stores its three words into request+DC/+E0/+E4 at`1E9638/1E963F/1E9645`. Those words are zero in this source-created neutral/default control state. The complete original0x154-byte physics request is at`021FCE68`; its definition/body/component/manager inputs and all source arithmetic are retained. [Caller](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000ddbc0.asm), [literal request writer](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/001e9540.asm).

`E3B20→1E8F60` receives actual physical state pointer`82D8839C` and the source request. Physical state1 dispatches `1F3E10`. Its original `1F5810` queue/parent route returns AL0; `1F58A0` also returns AL0 with the actual component+8C count0. No positive contact is supplied to bypass these gates. Source `1F4230` executes its genuine arithmetic and produces the recorded physical result. [Observed call/input/result buffers](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json), [source1F3E10](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/001f3e10.asm).

`1E94E0` then tail-jumps the actual normal-state classifier **1F52F0**, ECX=request154 and EDX=result70. It reads request+DC/+E0/+E4 with original SSE comparisons and can write result+4 classes2/3/4/5. The executed all-zero input takes its no-write return at`1F5371`, preserving result+4=0. The positive sign/axis branches are source-listed but **unexecuted in this genuine owner fixture**. The entire request/result packet before and after, CPU state and four owning reads are captured at call387. [Classifier](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/001f52f0.asm).

`E3B20` passes result+4 and result+8 to `E44C0` at`E3C5C`: actual classification0/blend0, EAX=unit datum, EBX=request mode pointer, EDI=request activity pointer, stack `(classification,blend,request+8)`. `E44C0` returns with the unchanged request bytes **`010100070C00000400000000`**. No supplied class is substituted. The older twelve-selector freeze separately proves class2/3/4/5 request→clip191/190/189/188 under its declared classification inputs; it does not establish a real moving player's device/control producer. [Measured selector](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000e44c0.asm), [separate selector scope](SPARTAN_EQUIPPED_ANIMATION_1_1_RESEARCH.md).

## Clock, local sampling and current collision matrices

`DE960` calls `112E10` with the source-produced request at return site`DEDE6`; no request or frame is delivered by this extension. `112E10→1CF100→1CA2B0/1CA090/1C9F50` advances actual primary185 from frame bits`00000000` to`3F800000`. Mode/class/name/activity remain the equipped source values, and the secondary remains NONE. [Clock call392 and its complete states](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json), [source112E10](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/00112e10.asm).

The animation clock consumes the **existing earlier dt input `3D088889`**. This extension does not write it, but does not upgrade its provenance to a real full-frame clock. The immutable tick-owner report independently executes `147CC0/147D20` with retained options rate0 and produces infinity, then observes inactive whole-tick admission and an unavailable kernel time import. This bounded one-schedule pose update cannot be presented as successful successive original ticks. [Clock-owner limit](MP_ITEM_TICK_OWNER_RESEARCH.md).

After the unit callbacks, actual dirty handling calls `BDF80` from`BD757`. It uses the genuine animation manager and original graph/resource descriptors, executes `BEA50→1D0F80→1CA950→280030/27FB10`, then the unit local callback **113830**. That callback's actual ABI is `(unit,node_mask,node_count,local_bank)`, RET`0x10`; measured arguments are `(E26F0000,00566C6C,21,82D88B90)`. The **21-node prefix is source-selected**, while all33 local channels/current matrices remain stored. Do not expand the sampler mask or choose a different pose to fill the remaining nodes. [BDF80](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/000bdf80.asm), [113830](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/instructions/00113830.asm).

This measured update changes local nodes`0..11,14..20` (19 total), and **every current matrix0..32**. The exact local/current bytes are retained; no reconstructed expected geometry is used. The root retains actual authored start bits`BFB9579D/3C855B09/C1235809`. Original `143960` receives its actual0x20-byte local channel and0x34-byte output; `1441D0` receives literal left/right/output matrices, including aliasing. The packet auditor replays their actual ordered stores. [Pose states46→49](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json), [buffer audit](../analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/audit.json).

Current matrices have the physical source's reciprocal boundary: `143960` uses RCPSS/refinement. The prior [Xbox reciprocal report](SPARTAN_LIVE_POSE_1_1_RESEARCH.md) shows host/emulator differences. Recorded matrices are original instruction/emulator outputs for these inputs, not asserted physical Xbox bit parity. Local-channel decoder/selection/clock evidence remains separate from that arithmetic boundary.

## Minimum integration contract and remaining gates

A native differential can now use the literal allocated-owner states for the original normal unit wrapper, physical-request construction/classifier's admitted zero branch, source request/clock consumption, local callback and33 current matrices. These are owned fields and phase outputs, not a replacement clip chosen by the renderer. Keep exact datum/owner/slot identity, hierarchy callback order, request12/physics154/classification70 ABI buffers, original primary/secondary channel state and source sampler mask. Skinning, hand attachment and posed collision must consume the same owned pose state for the relevant source phase.

The existing519-tick equipped/unit-phase freezes still provide bounded original firing172 and reload215 requests/clocks/local channels, including the authored `br` fire-name fallback selected for the SMG. Those cases have supplied device/control inputs and a different weapon/ready fixture. Their action state must be represented explicitly; this new neutral allocated-owner report does not make their ready/firing admission natural on this owner. [Existing action evidence and limits](SPARTAN_EQUIPPED_UNIT_PHASE_1_1_RESEARCH.md).

The next unavailable live chain is genuine non-neutral device/local-member input → source submission/delivery/C8140 → actual unit+1B0 movement or weapon action request, together with valid options tick rate, original clock/session/readiness and successive admitted `139750` frames. The source-owned [command suite](MP_COMMAND_INPUT_RESEARCH.md) produces neutral input only through separately selected roots; the local scheduler still has no ready/current local device admission. No axis, velocity, positive classification, full frame, local pose, PVS/queue or Havok result is assigned to close that gap here.

This work does not execute a new player-versus-player damage path, continuous player motion/settling, positive active Havok contacts, full session/world/input startup, or the user's reported live renderer scenario. Those remain separate from the measured native integration slice. Production and every earlier freeze are untouched.
