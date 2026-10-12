# Original campaign BSP requests and world transitions

Research date: 2026-10-07. A campaign BSP request changes the scenario's active
geometry through a deferred main-loop commit. It does not load a new mission
or reset the script VM. Existing objects are retained, hidden, or destroyed by
their placement policy and authored transition volumes; entering a BSP then
adds eligible missing scenery. Checkpoint restore first admits the saved BSP
resources, then restores arena-owned world and script state.

## Primary authority and reproduction

Original executable: `assets/disc/default.xbe`, retail `02.09.27.09809`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Local CC0 recovery is revision `5f08630655e3ddc9ac45909090f11ae799fa0c27`;
Assembly Xbox metadata/opcodes are revision
`fd14ef3c46135e9422966c6a64cd926fb0bd23fb`. Recovered source names aid reading;
the expected behavior below comes from original instructions and serialized
cache metadata. Individual routine bytes/statuses are recorded in
[instruction pins](../analysis/campaign/bsp-instruction-pins.json).

Run `.tools/python/bin/python tools/oracle_campaign_bsp.py` for **143** original
instruction cases, including a cross-BSP checkpoint restore. Run
`.tools/python/bin/python tools/inspect_campaign_bsp.py` to inspect the local
15 campaign caches: 141 compiled BSP requests, per-map hashes, BSP ordinals,
and authored transition volumes. Sources:
[versionable oracle](../tools/oracle_campaign_bsp.py),
[oracle results](../analysis/campaign/bsp-oracle.json),
[cache inspector](../tools/inspect_campaign_bsp.py),
[cache evidence](../analysis/campaign/bsp-authored-cache.json).

## Evaluators, names and pending requests

Both `switch_bsp`, opcode508/`1FC` (short), and `switch_bsp_by_name`,
opcode509/`1FD` (structure_bsp type26), use the same matched evaluator
`2A9580`. It obtains arguments through `209D50`, reads their signed low16
bits, calls request helper `12B850`, then completes HS void. The evaluator
does not wait for a geometry commit. `structure_bsp_index`, opcode510,
`2A95C0`, reads the currently loaded short `4686C4`, not the pending target.
Sources: [evaluators](../tools/reference/halo2-decompiled/src/unknown_29f5b0.cpp),
[2A9580 instructions](../analysis/campaign/bsp-2a9580.txt),
[2A95C0 instructions](../analysis/campaign/bsp-2a95c0.txt),
[Xbox opcode definitions](../tools/reference/assembly/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).

The original by-name evaluator takes a **compiled ordinal**, not a string.
Actual Cairo literal examples are `01_bsp_0 -> 0`, `01_bsp_1 -> 1`,
`01_bsp_3 -> 3`, `01outro_bsp -> 4`, and `high_0 -> 5`. BSP0/1 reference
the tutorial tag paths, while5 references High Charity's `high_0`; substring
matching against a mission name cannot recover these semantics. Preserve the
literal's serialized type26 value. Compiler case/alias matching rules were
not found in this retail runtime and are not established here. Cairo cache
SHA-256 is `5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0`.
Source: [actual compiled operands and BSP tag names](../analysis/campaign/bsp-authored-cache.json).

`12B850` uses `AX=target`, plain `RET`. Pending flag is byte `547F73`, target
short `547F78`; valid range is `[0, scnr.structure_bsp_count)`.

| Request | Result |
|---|---|
| Invalid, including `NONE=-1` | Leave pending flag/target unchanged. |
| Target=current loaded BSP, no pending request | Leave both unchanged. |
| Target=current loaded BSP, pending request exists | Write current target and clear pending flag. |
| Valid different target | Write target and set pending flag; later requests overwrite it. |

Several calls in one script update therefore coalesce before main-loop
commit. Source: [12B850 instructions](../analysis/campaign/bsp-12b850.txt),
[request recovery](../tools/reference/halo2-decompiled/src/unknown_12be90.cpp).

Main `12B450` processes skip input, revert, reset, **BSP commit**, save, and
quit in that order before driving simulation ticks. `12BD40` calls
`11C1B0(pending_target,true)`, then clears pending flag and target to `NONE`.
Failure calls `18EFA0`; it is not reported as a successful transition.
Source: [main-loop instructions](../analysis/campaign/skip-research-12b450.txt),
[commit instructions](../analysis/campaign/bsp-12bd40.txt),
[commit recovery](../tools/reference/halo2-decompiled/src/unknown_12bd40.cpp).

## Commit order and resource lifetime

`11C1B0` ABI is stack `(signed_short target, bool unload_old)`, `RET8`.
It distinguishes loaded-resource BSP `4686C4` from active-world BSP
`game_options+2`. If both already equal target, return true without callbacks.
Otherwise original order is:

1. Run pre-switch callbacks forward across68 entries. The only non-null
   original pre-switch callback is placement retention `D4E60`, entry57.
2. If active-world BSP differs and is not `NONE`, `137DA0` runs dispose-BSP
   callbacks in reverse order, then sets active-world BSP to `NONE`.
3. Unload the prior BSP when requested, then clear BSP/lightmap/collision
   pointers and loaded-resource BSP.
4. Publish requested loaded index and load its reference. On failure set
   loaded index `NONE`, return false, and skip new-BSP initialization.
5. On success publish BSP/lightmap metadata and collision pointers. When
   active-world BSP differs, `137D40` clears three64-byte option buffers,
   writes active-world BSP, and runs initialize-BSP callbacks forward.

Internal `11C1B0` also supports `NONE` to unload; the HS request helper rejects
it. Object retention occurs before streaming admission, so a load failure
does not imply the old world remained untouched. Sources:
[11C1B0 instructions](../analysis/campaign/bsp-11c1b0.txt),
[dispose](../analysis/campaign/bsp-137da0.txt),
[initialize](../analysis/campaign/bsp-137d40.txt),
[original callback table](../analysis/campaign/bsp-callback-table.json).

Scenario BSP references are stride`44` at `scnr+210/214`: file offset `+0`,
size `+4`, load address `+8`, BSP tag datum `+14`, lightmap datum `+1C`.
Original `122B40` rounds read size to512 bytes, checks the loaded `sbsp`
signature, and publishes tag addresses. `122BC0` releases streamed geometry
blocks and clears tag addresses. Sources:
[streaming recovery](../tools/reference/halo2-decompiled/src/unknown_122870.cpp),
[Xbox scenario layout](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/scnr.xml).

Callbacks disconnect/reconnect object locations (`B6C50/B6BB0`) and recreate
BSP-dependent physics (`1C27A0/1C2690`), AI (`1C7F60/1C7BB0`), effects,
visibility and audio state. The callback table contains no new-map HS runtime
initialization. Existing script/global arena records survive. Preserving actor
objects does not by itself prove brain/path/Havok state parity: those modules
have explicit BSP lifecycle work. Sources:
[object disconnection](../analysis/campaign/bsp-b6c50.txt),
[object reconnection](../analysis/campaign/bsp-b6bb0.txt),
[physics initialization](../analysis/campaign/bsp-1c2690.txt),
[physics disposal](../analysis/campaign/bsp-1c27a0.txt),
[AI initialization](../analysis/campaign/bsp-1c7bb0.txt),
[callback table](../analysis/campaign/bsp-callback-table.json).

## Placement policy and retained objects

Serialized placement fields: flags`+4`, manual BSP mask u16`+26`, origin
i16`+2C`, type byte`+2E`, source byte`+2F`, authored policy byte`+30`.
Source values are0 structure,1 editor,2 dynamic,3 legacy. Runtime fields:
parent datum`+14`, scenario index`+1A`, center`+30`, type`+AA`, source`+AB`,
name`+AC`, BSP byte`+AE`, derived policy`+B0`, flags DWORD`+4`.
Sources: [placement recovery](../tools/reference/halo2-decompiled/src/unknown_0b7300.cpp),
[object creation instructions](../analysis/campaign/bsp-b7b40.txt).

Original `D5A70` derives runtime policy:

| Authored input | Runtime policy |
|---|---:|
| Policy1, always placed | 0, transition-volume retention |
| Policy2, manual | 2, manual mask |
| Policy0/source0 | 1, origin BSP |
| Policy0/source1, originNONE or type mask`181F` | 0 |
| Policy0/source1 otherwise | 1 |
| Other combinations | NONE |

Type mask`181F` contains0..4,11,12. Runtime policy0 uses the object's current
BSP and transition volumes; policy1 requires requested BSP=authored origin;
policy2 requires the authored mask bit. Dynamic creation changes unspecified
source to2, scenario index toNONE, and origin/current BSP to the loaded BSP,
with default runtime policy0. Dynamic projectiles therefore participate in
transition-volume retention. Sources:
[D5A70](../analysis/campaign/bsp-d5a70.txt),
[D54E0](../analysis/campaign/bsp-d54e0.txt),
[D5460](../analysis/campaign/bsp-d5460.txt),
[B7930 defaults](../analysis/campaign/bsp-b7930.txt),
[B7B40 creation](../analysis/campaign/bsp-b7b40.txt).

`D4E60` ignores child objects and sourceNONE. Source0/1 objects obtain their
authored placement; dynamic policy0 needs no authored record. Protection
`BEB30` overrides a failed retention decision. Already-hidden objects that
are not retained remain hidden. A retained BSP-hidden object clears hidden
bit7 and BSP-hidden bit27; other hidden reasons remain. Reconnection is
requested only when bit8 is clear and object globals are active. Kept objects
receive new BSP`+AE` unless targetNONE.

Visible non-retained objects run `D52B0`: named objects, or persistent/damageable
definition types, are disconnected and marked hidden/BSP-hidden. Others run
destruction services `B83B0` then `B8460`, except mode4 objects with a bound
`+D4` identifier. Hidden/removed objects retain their old BSP index. Players
and protected object hierarchies are handled by `BEB30`; the fixture supplies
that admission, not a complete player hierarchy. Sources:
[D4E60](../analysis/campaign/bsp-d4e60.txt),
[D52B0](../analysis/campaign/bsp-d52b0.txt),
[protection instructions](../analysis/campaign/bsp-beb30.txt).

## Authored transition volumes

The recovery's “shared cluster” terminology for `scnr+380` is misleading.
These are **BSP transition volume** records, stride8:
`{u32 pair_key, i16 trigger_volume, i16 padding}`. Pair key is
`max(old,new)<<16 | uint16(min(old,new))`; records are sorted by key.
`D5390` allows retention if the pair has no records or only volumeNONE records.
Otherwise the object center must lie inside at least one listed valid trigger.
Cairo records are0↔2 with triggers3 and1, and0↔3 with trigger5. All three are
unattached, and their exact bases/positions/extents are in cache evidence.
Sources: [Xbox scenario schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2Xbox/scnr.xml),
[D5390 instructions](../analysis/campaign/bsp-d5390.txt),
[cache evidence](../analysis/campaign/bsp-authored-cache.json).

Trigger records at `scnr+108/10C`, stride`44`, contain object name`+4`, node
SID`+8`, forward`+C`, up`+18`, position`+24`, extents`+30`. `1420F0` builds
scale1, copies forward/up, and sets left=cross(up,forward), without vector
normalization. Attached triggers compose this with the named object's node
matrix. `11C470` subtracts position, divides by matrix scale when not1, projects
onto forward/left/up, then requires strict `0 < coordinate < extent` on all
axes. This checks the object's center, not capsule overlap. Sources:
[11C380](../analysis/campaign/bsp-11c380.txt),
[1420F0](../analysis/campaign/bsp-1420f0.txt),
[11C470](../analysis/campaign/bsp-11c470.txt).

## Initial creation and entering a BSP

`D5990` persistent-definition predicate returns true for mask`199F`: biped0,
vehicle1, weapon2, equipment3, garbage4, machine7, control8, crate11,
creature12. Scenery6 is additionally true when its model exists and the
model's damage-info count at`+60` is positive. Other types return false.
`D59F0` returns `automatic && (persistent || global)`, where automatic means
flag bit0 clear, persistent means named or valid D5990 definition, and global
means named or derived policy!=1. Sources:
[D5990](../analysis/campaign/bsp-d5990.txt),
[D59F0](../analysis/campaign/bsp-d59f0.txt).

Initial `18F000` calls new-map initialization, then `D5560(false)` to create
D59F0 entries **including named/persistent objects**, then `11C1B0` to adopt
the initial BSP. Its retention callback can hide those objects. The optional
D5560 skip-existing mode resolves the full placement identifier. Named hidden
objects retain their name binding and can return later. Sources:
[startup order](../analysis/campaign/bsp-18f000.txt),
[D5560](../analysis/campaign/bsp-d5560.txt),
[name binding](../analysis/campaign/bsp-bf050.txt).

New-BSP `D4E30` calls `D5640` unless cinematic byte`interface+5` **and**
suppress-placement byte`+7` are both set. `D5640` builds an existing bitmap
for source0/1 placements by type/index. It skips existing, named, persistent
D5990 definitions, and flag bit0. Manual policy requires the current mask bit;
other policies allow originNONE/current or D59F0=true. `BF0F0/D5060` then
reject never-placed bit6 and invalid palette/tag entries and build the object.
Thus switches re-add eligible absent scenery while persistent object state
survives through hiding. Sources:
[D4E30](../analysis/campaign/bsp-d4e30.txt),
[D5640](../analysis/campaign/bsp-d5640.txt),
[BF0F0](../analysis/campaign/bsp-bf0f0.txt),
[D5060](../analysis/campaign/bsp-d5060.txt).

The original auto-selection/construction path tested here creates eligible
scenery with authored flag masks2,4,8,14 at each supplied difficulty0..3.
Flags1 and64 block creation. Do not impose an unconditional flag4 exclusion
as an established retail Normal-difficulty filter. Creation and object event
effects are supplied in this proof; it does not validate every object's
post-creation behavior or AI squad difficulty rules. Source:
[automatic-placement oracle cases](../analysis/campaign/bsp-oracle.json).

## Checkpoints across BSPs and integration boundary

Initialize-BSP callback `123D20` writes loaded BSP to arena header`+1248`.
During revert, `12BA30` first cancels pending BSP flag and target. Original
`123F60` validates the saved header/options, calls `11C1B0(saved_bsp,true)`
**before** pre-restore callbacks and arena replacement, then restores the
saved memory and runs post-restore callbacks. The cross-BSP fixture saves0,
visits1, queues2, and reverts: resources return0 before memory copy; pending2
is cancelled; an arena VM sentinel returns to its saved value; reverted
timestamp matches the restored tick. A native same-BSP-only restore guard
would block this authored behavior. Sources:
[123D20](../analysis/campaign/bsp-123d20.txt),
[12BA30](../analysis/campaign/skip-research-12ba30.txt),
[123F60](../analysis/campaign/skip-research-123f60.txt),
[cross-BSP original trace](../analysis/campaign/bsp-oracle.json),
[skip checkpoint timing](CAMPAIGN_SKIP_RESEARCH.md).

Native integration needs a pending-request seam, a pre-tick resource commit,
new collision/navigation/render/lighting data, policy-based existing-object
lifetime, and deterministic eligible-placement addition. Preserve script and
global state, actor/object identities, health and other saved world fields.
Restore must select saved geometry before replaying saved VM/world state.

The143-case fixture executes unchanged original control decisions, actual
static-trigger transforms/containment, selection/construction helpers, and
cross-BSP save/revert ordering. It supplies streaming and compatibility,
object iteration/protection/admission, allocation/destruction/event effects,
and unrelated module callbacks. The VM preservation case is an arena sentinel;
the real Cairo thread proof is the separate skip oracle. Full renderer,
physics, AI, audio lifecycle effects and campaign playthrough remain outside
these bounded proofs. No production files or existing docs were edited.
