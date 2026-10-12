# SMG impact context, original map-pack 1.1

## Authority and scope

The original instructions in the map-pack English update execute unchanged in
`tools/oracle_smg_impact_1_1.py`. The executable SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
the unchanged Lockout cache SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
This establishes a bounded callback/event/damage/material/deletion path. It
does not establish the native capsule intersection as the original Spartan
collision query, complete unit spawn/pose, damage animations, sound/effects,
or complete original multiplayer simulation.

The collision output and target runtime state are explicit fixtures. The
oracle uses the actual `proj`, `jpt!`, `matg`, `bipd`, and `hlmt` data and original
armor lookup; it does not supply a damage multiplier or maximum vitality.
The original normal mode1 and authority mode5 are examined separately.

## Verified native integration contract

`FCEC0(projectile)` is the normal type callback. It invokes `FCF90` with the
projectile's current velocity. `FCF90` proposes a world segment and calls
`100110`; the first query is
`16B240(0x2480000F, origin, delta, projectile+140, projectile, result)`.
For the stock SMG, the authored thickness field `proj+C8` is zero, so the
secondary lateral thickness sweeps do not run in the clear case.

The result occupies **0x5C bytes**. These fields enter the stock object impact:

| Offset | Type | Verified consumer/meaning |
| --- | --- | --- |
| `00` | uint32 | Kind; `4` is object hit. |
| `04` | float | Segment fraction; remaining tick fraction uses `1-fraction`. |
| `08..10` | 3 floats | Hit point. |
| `24` | int16 in a 4-byte slot | Resolved global material. |
| `28..30` | 3 floats | Hit-plane normal. |
| `34` | float | Hit-plane distance. |
| `40` | object datum | Hit object. |
| `44` | int16 | Object collision region index. |
| `46` | int16 | Collision BSP node index. |
| `48` | uint32 | Packed collision BSP identifier. |
| `4C/50/54/58/59` | surface result fields | Copied by original `16A5C0` from its lower collision result. |
| `5A` | int16 | Model material entry index for object hits. |

The region/node labels follow original `1F3660` packing and `1F3900` output:
the low byte is the authored collision BSP's node byte; the highest byte is
its containing collision region index. `16AF90` writes the former to result
`46` and the latter to `44`. The lower surface material index is copied to
`5A` by `16A5C0`. `DA0E0` resolves it through the object's `hlmt+5C` material
table, entry stride `14`, field `+10`. These producers are source evidence;
the world traversal/intersection itself is not executed by this impact fixture.

`FEB40` handles kind4. For the real SMG it creates the event, applies damage,
then chooses the material response. Implementing those phases in the reverse
order would select the wrong material for shield hits.

### Event creation and actual damage call

`D7C60` zeroes **0x88 bytes**, assigns definition, sets reference sentinels,
sets scale floats `54/58/5C` to 1, and material `7C` to the executable's default
global material sentinel. `FEB40` then populates the following fields:

| Event offset | Verified assignment |
| --- | --- |
| `00` | Original `proj+14C` impact `jpt!` datum. |
| `04` | Initial flags OR `8`. |
| `08` | Projectile `C4`, source player datum, via `BD080`. |
| `0C` | Projectile `C8`, source object datum, via `BD080`. |
| `10` | int16 projectile `C2` team, via `BD080`. |
| `24..2C` | Collision hit point. |
| `30..38` | Same collision hit point. |
| `3C..44` | Incoming velocity normalized by original `30BD0`. |
| `54` | Projectile `188` damage scale; the authored SMG does not enable the `proj+BC` bit4 distance scaling branch. |
| `7E` | int16 projectile `1A8` optional player-specific entry. |
| `84` | byte `proj+128 & 3F`, actual SMG reporting type8. |

The remaining fields retain the original initializer's values. In particular,
`48..50` remains zero in this non-radius event. Do not substitute the hit
normal into this field.

Direction normalization is itself an original operation sequence: `30BD0`
computes `x*x + y*y + z*z` with x87 operations and `FSQRT`, stores the length
as float32, then uses `DIVSS` and `MULSS` for the reciprocal and components.
The oracle uses FPCW027F and MXCSR1F80, matching the existing instruction
fixtures. Stock first-shot input direction from `147A90` has bits
`3F7FF510/3BBD4C91/3C8DF96C`; the impact event direction, derived from the
scaled incoming velocity, has bits `3F7FF511/3BBD4C92/3C8DF96D`. Copying the
firing direction verbatim is not a bit-exact substitute for this call.
The untouched listing is
`analysis/multiplayer/map-pack/smg-impact-original-instructions/00030bd0.asm`.

The exact call is:

```
D9180(event, result.object40, int16(result.node46),
      int16(result.region44), int16(result.material_entry5A), &result.normal28)
```

`D9180` computes the authored amount via `DB160`. The stock SMG impact has
equal bounds, **4.75**, so that amount selection consumes no random value.
The original armor/material table search `D71C0` runs against actual `matg`
data. Source and target relationship/player state are supplied runtime state;
the oracle does not claim the entire multiplayer ownership/team producer.

For the hit target, `D9180` selects the actual model damage information block
at `hlmt+60/+64`. If `material_entry5A != -1`, it selects the `hlmt+5C` entry
of stride `14`, assigns `entry+10` to event `7C`, and selects damage section
`entry+6`. With entry `-1`, the material comes from damage-info `CE` and the
section comes from damage-info `08`. Node `46` separately selects the damage
information's `C4/C8` node mapping. The collision region is not interchangeable
with a model material entry or damage section.

The original order is **`DA710` shield, then `DA2B0` body**. Shield armor uses
damage-info `CC` independently of the body material in event `7C`. `DA2B0`
invokes `DA620` and `D71C0` in this actual source context, so the existing
source-derived scalar kernels now have a demonstrated projectile caller.
The event's ordinary flags8 do not enter the `D98E0` radius/distribution path.

After both passes, `D9180` compares its normalized shield/body contributions
and records the dominant material in event `7C` and corresponding current
vitality in `78`. `FEB40` resolves that updated material with `18A0C0`, stores
the chosen material at projectile `132` and the normal at `134..13C`, then
calls `1015A0`. That function searches the actual projectile response table
by global material, following `matg` material parent `+8` until it finds a
match. It is not a flat native material lookup.

### Source response record

Before final notification, original `DAA90` constructs a **0x54-byte** response
record from the event and 0x34-byte accumulator. `DAC40` consumes it. Relevant
verified fields are:

| Response offset | Source |
| --- | --- |
| `00` | Reporting type byte from event84. |
| `04` | Flags built from event and accumulator; not equal to event04. |
| `08` | Event damage definition datum. |
| `0C/10/14` | Event owner08/0C/10. |
| `18..20` | Event3C direction for ordinary impacts. |
| `24..2C` | Event30 hit point. |
| `34` | Event5C times event54. |
| `38` | Event58. |
| `3C` | Accumulator int16 `08`, widened to int32. |
| `40` | Accumulator int16 `10` node index. |
| `44` | Accumulator28 normalized body contribution. |
| `48` | Accumulator20 normalized shield contribution. |
| `4C` | Event60. |
| `50` | Zero. |

`DAC40` performs additional original response/accumulated-damage state updates.
Unit notification `CB1D0/CB410` and replication `A8ED0` are recorded sinks in
this oracle; their animation, death presentation, score/network behavior is
not established here. Their sink status does not replace shield/body kernels.

## Actual authored data and observed stock cases

Lockout SMG projectile `E59B0426` starts at file31385132; its impact `jpt!`
`E59D0428` starts at file31386452. Spartan `bipd E9F2086D` uses
`hlmt E9F3086E`, file32232440; damage information starts at file32232828.
The metadata gives maximum body45 and maximum shield70. Model material entry0
is named `head`, selects damage section1, and has global material56. Entry1 is
named `body`, selects section0, and also has global material56. Shield armor
material is147. These named materials do not prove the intersection producer
has classified a native capsule hit as head/body.

Current full callback results with supplied collision fraction0.5 and actual
model body entry1:

| Initial normalized shield/body | Final shield bits | Final body bits |
| --- | --- | --- |
| `1 / 1` | `3F6EA0EA` | `3F800000` |
| `0 / 1` | `00000000` | `3F64FA50` |
| `0.01 / 1` | `00000000` | `3F68F5C2` |
| `0 / 0.01` | `00000000` | `BDC3B2A2` |

The lethal case preserves a negative normalized body value and executes
original `D7E00`/`D8070`, setting damage-state flags `000C`. The original
Spartan data does not enter `DC460`'s takes-full-damage-on-death branch in
these cases. The named body and helmet entries produce the same stock SMG
scalar damage; selecting the helmet entry does not make this weapon execute
the headshot-special kill branch. Death presentation remains outside the
recorded unit notification sinks.

The depleted case executes original `D8070`, sets damage-state flag8 and clears
shield auxiliary `F4`. The supplied runtime damage sections occupy 16bytes,
object `120=16`, `122=A00`, following original `DC0A0/DC460` addressing. Render
collision-region state uses authored count times10, object `118`, relative
offset `11A=B00`, following original `BB6E0`. These are supplied valid storage,
not the original allocator/spawn output.

## Stock projectile response, deletion, and expiry

For these actual Spartan body/shield material parent chains, `1015A0` selects
the authored SMG `default_material` response0 at file31385552, original
VA8021AFD0. It does not select the `soft_organic` continuation response.
The body chain is `56 -> 55 -> 54 -> 53 -> 0`; the shield chain is
`147 -> 146 -> 145 -> 144 -> 0`. The original SMG damage group key is
`0B0007D1`, second key0; body armor is `0F0000A4`, second key0, and shield
armor is `120007CB`, second key0. Actual `D71C0` returns multiplier bits
`3F800000` for both. These values are read from original user-owned metadata;
they are not a replacement constant table for other weapons or targets.
`FEB40` changes projectile state `130` from0 to1. In the same callback `FCF90`
runs original `100410`; the SMG detonation-damage datum `proj+10C` is NONE, so
it does not generate a second direct damage event. `B9430` marks header bit10
for deletion. The observed header changes `0A -> 1A`, while the object remains
in supplied storage.

The oracle then executes original `B8040`. That pass sees the marked header,
calls `B92A0` and its eight original cleanup callbacks, then `B9350`, `BD1F0`
and `16D4C0`. The projectile bitmap bit clears (`0000000F -> 0000000B`),
header flags and salt clear, and the object storage pointer becomes0. AI,
sound-listener, effects-pool and lower heap contexts are explicitly supplied
empty; the synthetic lower allocation's 16-byte heap link header is zero.
This proves the original removal order and pool/header bookkeeping under
these boundaries, not original heap placement, storage reuse, or live cleanup
of registered audiovisual objects. The suite preserves state both immediately
after impact and after the removal postpass.

The original clear-world expiry uses authored distance40 and a strict
`limit < accumulated_distance + proposed_travel` comparison, clamps the last
motion to the remaining range, raises state130, and runs the same detonation/
mark-deletion route. Natural clear-flight accumulated distance is
`41555555`, `41D55555`, `42200000` on ticks1,2,3; state130 remains0. On tick4
the distance remains `42200000`, state becomes1, and deletion is marked.
It does not expire on equality at tick3. The suite also supplies initial
distance39 and40 separately to exercise final partial and zero-distance motion.

## Remaining producer boundaries

The native host must eventually produce the actual collision model's posed
BSP surface/material-entry/region/node result; its current body capsule does
not establish those fields. The full original muzzle/world position, posed
Spartan collision model, moving world bodies, player/team relationships,
effects/audio, damage animation, death response presentation, score/respawn,
replication, and native integration of the verified object-pool removal remain
distinct integration work.
The bounded scalar damage match alone does not close these boundaries.

## Frozen verification outputs

The reusable tool `tools/oracle_smg_impact_1_1.py` has SHA-256
`c8cd233522a66035ff0cfec56eb3781da254161f1da6692744eb46c4d05da1e6`.
It generated **58 cases / 64 motion ticks**. Every mode1/mode5 pair matches
full target/projectile state, original removal result, complete damage event,
accumulator and response record. Every covered impact/expiry/removal path
consumes zero additional RNG stores after the actual firing/creation phase.
This is a bounded statement about these supplied cases, not all gameplay RNG.

The frozen suite is
`analysis/multiplayer/map-pack/smg-impact-suite/suite.json`, SHA-256
`401adedf8559b9511bda66dc38f9e1c723d3690a40fd962724e119594ddc37b4`.
It pins each case report individually. Entry-1 cases deliberately supply a
missing model material entry; entry0/1 cases execute original object-hit helper
code with real authored helmet/body surfaces and an explicit lower query.
Fractions0/0.5/1, shield1/0.1/0.01/0, body1/0.01, and natural/partial/zero
range expiry are covered. No native implementation is used for expected values.

The static inventory contains **41 original routines** at
`analysis/multiplayer/map-pack/smg-impact-original-instructions/inventory.json`.
`30BD0` is an additional saved untouched listing. Static inclusion does not
mean every branch or listed routine ran in every fixture.

Reproduction:

```
.tools/python/bin/python tools/oracle_smg_impact_1_1.py --suite analysis/multiplayer/map-pack/smg-impact-suite
.tools/python/bin/python tools/oracle_smg_impact_1_1.py --instruction-inventory analysis/multiplayer/map-pack/smg-impact-original-instructions
```

Existing frozen firing/projectile/muzzle/kernel suites and production source
were not modified by this research.
