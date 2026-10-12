# Original deferred unit damage after ordinary player departure

2026-10-12. Focused follow-up to the retained-player departure research.
All addresses below are **retail Xbox**. The original departure request is a
force-kill damage event with **lethal statistics suppressed**, followed by the
unit's ordinary damage response/death/drop owners. It is not immediate body
deallocation, a normal stock-SMG lethal hit, or a new player respawn event.
The current native ordinary-damage adapter does not implement this complete
branch. No product files were edited in this investigation.

## Reusable original controls and their limits

[Collector](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/collect.py),
[frozen fixture](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/fixture.json),
and [run summary](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/run-summary.json).
Reproduce from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python .local/departure-unit-damage-research-20261012/collect.py
```

The fixture pins retail XBE
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
original Lockout cache, inspected recovered/native source files and collector.
It contains15 controls, complete before/after object800, header12, explicit
event88/report54/accumulator34 inputs, original call arguments/registers,
observed memory writes, and CFG-complete original instruction listings. Every
control verifies the mapped `.text` bytes remain unchanged. **No original
instruction or game/platform return is patched or substituted.**

Four controls return completely: `D6660` initialization, already-depleted
`DBFB0`, mode4 `D5DE0`, and actual SMG projectile/no-info `D5DE0`. Eleven are
explicit partial observations stopped **before** the named original service;
none supplies its return. Three capture `DBFB0` requests, one captures the
`D5DE0` dispatch, two enter ordinary biped damage, two compare statistics
admission, one enters `CA0B0` to its animation request, and two compare object
protection flags. No live network,
native departure, entire object pass, equipped death/drop, full physics world
or surviving-match result submission is demonstrated.

## Deferred dispatch, asset and exact event

Departure `152DF0` sets unit word `+10A` bit10 (`400`) before `14CAD0` clears
unit current player `+13C` and retains it at previous player `+140`.
`B7360` marks activity through object header bit1; it does not delete the body.
The already-established `B6F10 -> BC470 -> D5DE0` active-object pass consumes
the pending request.
[Prior source-backed chain](/home/eric/Developer/Halo-2-Decompiled/multiplayer/docs/halo2-nonhost-departure-evidence.md:115).

`D5DE0` first resolves actual damage info through object definition `+38`
model and model `+60/+64` damage-info block. No info returns false before
consuming the pending flags. It handles delayed region permutations and any
actor bounds before the pending default-damage call. Pending bits5,6 or10
invoke `DBFB0(object,NULL,bit6,bit10,false)` unless game mode4; afterward those
three bits are cleared, including in mode4. A simple departure with only
bit10 therefore calls **`DBFB0(object,NULL,false,true,false)`**. An already
depleted body makes `DBFB0` return. Existing bit6 is significant: departure
ORs bit10 and does not itself clear another pending request.
[Damage-info lookup](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0a4950.cpp:86),
[dispatch](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:3443),
[exact dispatch bytes](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/retail-000d5de0.asm).

The actual Lockout matg `E1740000 globals\globals`, file offset `01D18800`,
has one falling block at `01D1AE5C`. Its datum at block `+14` is
**`EAE80963 jpt! globals\falling`**, tag file offset `01F0CC0C`.
The fixture stores all `0x68` bytes of the falling block and all `0x114` bytes of this
tag. Definition category is1, definition flags0, damage upper minimum and
maximum both125, instantaneous acceleration0. Schema names category1
"Falling" and matg block `+140` "Falling Damage"; the executable itself
proves the `matg+144 -> block+14` selection.
[Literal asset evidence](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/fixture.json),
[selection instructions](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/retail-000dbfb0.asm),
[matg schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml:1104),
[damage schema](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/jpt!.xml:22).

`D6660` zeroes88 bytes, installs the selected datum, NULL damage owner
(player/object `FFFFFFFF`, team `FFFF`), the original default material/index
constants, scale54/58/5C=1 and reporting-kind84=0. `DBFB0` adds event flag4,
sets/clears10 from booleanA,80 from booleanB,800 from booleanC. The departure
event is therefore **flags84, NULL owner, kind0**. It passes node/other/region
indices `NONE` and a null vector to `D7B80`. Do not encode this as reporting
kind8 merely because the native supported gun path is an SMG.
[Initializer](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:507),
[default request](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:1272).

## Actual admitted damage branch and required fields

`D7B80` computes the tag's ordinary amount (`D9B60`, including selected-engine
player scaling where admitted), then event flag4 limits the damage parent list
to the target alone. For the ordinary detached biped branch, the important
requirements and order are:

| Gate or owner | Exact effect/required state |
|---|---|
| Valid object header and tag/model/info | Header strideC and object pointer+8; object tag0/typeAA; object definition model+38, model damage-info count/pointer+60/+64. No guessed default info. |
| Damage admission | Damage>0 **or** event radius68 **or** event58>0; initializer58=1 admits this branch even if computed amount is0. |
| Owner copy | Object owner teamC2/playerC4/objectC8 becomes the event's NULL owner before force depletion. Prior damage history is separate. |
| Force depletion | Requires event flag4, info.flags **without80**, object10A **without4000**, and body-depleted bit4 clear. Sets shieldF0=0; calls `D6A70`; sets bodyEC=0; calls `D6800(object,&owner,false,false)`; accumulator.flags adds41. |
| Shield/body continuation | Ordinary `D9110` and `D8CB0` still run when their definition gates admit. Material tables, actual maximaE4/E8, shield/body stun and default model section remain inputs. Body can become negative after the forced zero. |
| Report and response | `D9490`, unit pre-report `C9E70`, then `D9640`; original effect/animation, recent-damage, unit/type response and optional deletion callbacks remain required. |

[Whole damage owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:2760),
[force-depletion instructions](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/retail-000d7b80.asm:315),
[body continuation](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:1754).

`D6800` sets body-depleted bit4, performs `B8B70`, optionally dirties actual
replication entityD4, destroys authored death regions (region.flags2), handles
vehicle riders, calls `D6A70`, and applies authored accumulated section/deletion
effects. Its parent notification is **false** here because event80 is set.
The current masterchief_mp info flags3 and its two region flags0/80 do not
request death-region destruction through region.flags2. This bounded ordinary
biped path therefore needs no fabricated permutation deletion. Different
models, vehicles/riders, pending regions, non-NONE actors/entities or object
immunity flags need their actual owners and fields; this note does not admit
them by silently ignoring callbacks.
[Body depletion](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:626),
[actual model/info/region bytes](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/fixture.json),
[B8B70 bytes](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/retail-000b8b70.asm).

Additional original controls distinguish object protection bit7 (`80`) from
force-depletion immunity bit14 (`4000`): with otherwise identical detached,
shield-depleted inputs, bit80 still reaches body0/depleted, while bit4000
suppresses forced depletion and `D8CB0` clamps body to a positive minimum.
The scalar armor gate `D9020` tests80 later; it does not gate the preceding
`D7B80` force-depletion branch. A native caller must preserve this distinction.
[Armor gate](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:1371),
[frozen protection controls](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/fixture.json).

Unchanged controls with this actual tag/model, body maximum45, shield maximum70,
actor/parent/entity/current player `NONE`, prior player `80000001`, and an
already-depleted shield reach `D9490 -> C9E70` with body
**`-2.777777910232544`**, flags10A=`040C`, accumulator.flags=`0241`, and report
flags **`0949`**. With a normal shield, execution reaches the actual shield
depletion effect `E95507D0` at `176780`; it stops before allocation/dispatch.
The fixture uses explicit object fields and an empty pending region-state
block, not a source-created live biped.

## Statistics suppression and unit death are distinct

`D9490` maps event80 to report **800**. `D9640` admits statborg `1E9FA0` and
owner notice `1E8FA0` only when mode!=4, report800 is **clear**, target is a
unit, and report1 is set. The actual departure report suppresses this entire
lethal-notice block. Prior-player140 is not used to substitute a victim here.
`CA0B0` still runs, and its hard-report tail still calls `CAA60`.
[Report flags](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp:2395),
[suppression gate](/home/eric/Developer/Halo-2-Decompiled/.local/departure-unit-damage-research-20261012/retail-000d9640.asm:266),
[unit reaction](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:6678).

The fixture passes the actual emitted report as a standalone `D9640` input
with real Slayer registration/selected engine2 and valid statistics. Report949
reaches `CA0B0` without entering `1E9FA0`. Clearing **only input report800**
reaches `1E9FA0`. This negative control changes declared input data, not an
original instruction or service return. It does **not** stand in for executing
`C9E70` or its animation result. Direct actual-report `CA0B0` reaches original
`1147E0` animation request; no animation manager return is fabricated.

After departure, `CA0B0`'s current-player/controller feedback and
player-versus-player betrayal hooks skip because13C is `NONE`. Its history
`CA700`/incoming-owner bookkeeping call skips because both event owner fields
are `NONE`. This does not create a new suicide, credited kill, victim death,
assist, respawn penalty or XP penalty. Retained standings/quit processing is
owned elsewhere and is not inferred from body damage.
[Current-player/owner predicates](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:6725).

## Body lifetime, conditional inventory drops and native seam

`CAA60` calls biped `E4A20`; admits garbage with `BB950(unit,true,rate*10)`;
releases AI/actor owners if present; sets unit death tick2E0; clears control148;
stops selected weapons; handles seat departure; clears unselected inventory,
extra held object and grenades; performs selected-hand drops when allowed;
interrupts action26/27/22 and runs the final animation owner. Its entire
**player-owner branch** (player30=current2C, `14CAD0`, `152340`) is conditional
on current unit13C!=NONE, so departure skips it and preserves previous-player140.
Neither header activity nor this ordinary default-damage request proves
immediate body deletion on the next pass; later garbage/world/type ownership
decides actual removal.
[Complete unit death](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:5678),
[biped transition](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/bipeds.cpp:3724).

Inventory handling is conditional, not "delete all weapons". `CECE0` invokes
`CE920` on unselected slots. `CE920` selects deletion mode1 for mode4, item184
>=1, or multiplayer state2 with neither `100F00` nor `101640` admitting the
item; otherwise it clears the slot through `CEA70 -> CE470 -> CE6B0`, using
actual markers/RNG/velocity for the drop. `CE470` also selects deletion for
unit134 bit10000 or an object with no model. Selected hands receive requests19
then9 **only if unit1F5==0 or force is true**. Both original table entries
point to `E8DE0` (`unit_action_drop_weapon`);19 uses second-hand mode3,9 uses
primary mode2/0, then the same slot/drop owner. `CEB30` creates/drops each
remaining grenade outside mode4. These callbacks need actual inventory,
definition, marker, action/animation and object-allocation owners.
[Slot removal](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:2054),
[drop/deletion choice](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:1442),
[throw owner](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:3923),
[hand handler](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unknown_0a76b0.cpp:1422),
[grenades](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/unit_object_type.cpp:3240).

The safe native reuse boundary is the already-recovered ordinary
`D9110/D8CB0` scalar continuation **after** a new, explicit force-depletion
prelude with its required callbacks. `apply_unit_damage` has no event force-kill,
report40/800, raw damage owner or complete `D6800/CAA60` service interface;
`Simulation::damage` invokes ordinary damage/history/death hooks. Merely feeding
125 or an arbitrarily large amount produces a different shield/body report.
`MultiplayerMatch::damage_event` is presently the ordinary stock-SMG attribution
path and uses reporting kind8. `finish_death` always rewrites previous player
from current player; calling it unchanged after departure would erase retained
140 because13C is alreadyNONE. A detached-body death owner must skip that
player branch while executing the actual admitted body/inventory/lifetime
callbacks. The typed biped physical-state transition can be reused only with
its declared physical/animation/world services installed; it does not supply
the entire unit death or weapon drop policy.
[Unit adapter](/home/eric/Developer/Halo-2-Decompiled/src/unit_damage.cpp:3),
[damage admission/hooks](/home/eric/Developer/Halo-2-Decompiled/src/simulation.cpp:508),
[native attribution](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:612),
[native detach helper](/home/eric/Developer/Halo-2-Decompiled/src/multiplayer.cpp:475),
[physical transition contract](/home/eric/Developer/Halo-2-Decompiled/include/halo2/biped_death_transition.hpp:52).

The deferred request/force-depletion/report/statistics boundaries above are
established. Full equipped unit callback execution, native body binding,
ordinary nonhost relay continuity and authenticated quit/result confirmation
remain separate prerequisites. No survivor-only arbitration policy follows
from these controls.
