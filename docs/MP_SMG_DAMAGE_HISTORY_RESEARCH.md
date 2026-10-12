# Original TU1.1 stock SMG damage, history and aftermath scheduling

The bounded original `D9180` damage entry now executes from the stock SMG
amount producer through shield/body arithmetic, result construction, attribution,
history recording and ownership release. This is sufficient evidence for
connecting the supported hit input to source history and counters. Hit finding,
animation, head-marker/death-event writing and later corpse cleanup remain
explicit boundaries. It does not prove complete projectile or magazine parity.

## Frozen reproduction and authority

Run this independent suite without overwriting the earlier frozen reports:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_smg_damage.py
```

Artifacts:

- [oracle](../tools/oracle_mp_spawn_smg_damage.py), SHA-256
  `4d20d8c2e7229dd2bf78a601482d1862535c0208ab7a57b86f47192337c1a378`;
- [report](../analysis/multiplayer/integration/spawn-source/smg-damage-report.json),
  SHA-256 `2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca`;
- [instruction bytes and listing](../analysis/multiplayer/integration/spawn-source/tu11-smg-damage-instructions.txt),
  SHA-256 `d9731b9d4327aac69317a7620be2458388eaae9bc7890397f750592268783b08`.

The executable is original TU1.1
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout is `assets/disc/maps/lockout.map`, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
All Lockout bytes are mapped at their original cache addresses, with original
tag datums and references. No executable instruction is patched.

The report contains 209 source cases: four separately composed kernel/result
cases, three history-initialization prefixes, 18 amount cases, 20 complete
damage-entry hit cases, six sequences containing 150 hit entries, ten scheduling
prefixes and four zero-amount/identity cases. A nonlethal `D9180` entry returns
completely. A lethal entry stops at `CBE95`, immediately after original ownership
release, before subsequent corpse cleanup. Each case records inputs, generated
result bytes, actual source accumulator bytes at `DAA90`, history, ownership,
statistics, source writes, source/supplied calls and both RNG streams.

The report records hashes of all nine earlier spawn/lifecycle/ownership/timing/
queue/rounding/placement/aftermath/Slayer tools and their frozen reports. Those
files remain unchanged. The earlier arithmetic/ranking authority remains
[MP_SLAYER_ACCOUNTING_RESEARCH.md](MP_SLAYER_ACCOUNTING_RESEARCH.md).

Primary executable routines, with decompiles useful for navigation:
[D9180](../analysis/map-pack-update/functions/000d9180.c),
[DB160](../analysis/map-pack-update/functions/000db160.c),
[DA710](../analysis/map-pack-update/functions/000da710.c),
[DA2B0](../analysis/map-pack-update/functions/000da2b0.c),
[DAA90](../analysis/map-pack-update/functions/000daa90.c),
[DAC40](../analysis/map-pack-update/functions/000dac40.c),
[CB410](../analysis/map-pack-update/functions/000cb410.c),
[CBA60](../analysis/map-pack-update/functions/000cba60.c),
[1ED960](../analysis/map-pack-update/functions/001ed960.c).
The byte listing owns register inputs, operation order and packed-field types
where the decompiler's inferred types are incorrect.

## Fixture and input contract

The original player pool has capacity 16 and stride `21C`. Full fixture datums
are slot 0 `00070000`, slot 3 `00160003`, and slot 15 `0052000F`; the salt is
`7 + 5*slot`. Unit datums `00010000`, `00010001`, `00010002` have actual typed
12-byte source headers, salt 1, type 0, flags 1 and supplied `800`-byte backing
storage. They belong to players 3, 0 and 15, respectively.

Each fixture executes original constructor `C52C0` through `C5464` to initialize
unit history. It separately executes the actual `C55D1..C560F` membership
producer from placement `+64`, then complete original `14E240` to bind each
full player datum to its full unit datum. Unit `+138` is therefore 3, 0 and 15,
separately from player `+C0`. No native combat-team field supplies these values.
This is an explicit composition of source initialization/binding slices;
constructor allocation and later animation callbacks do not execute here.

AI actor and replicated entity are NONE, local user is NONE, persistent profile
recording is disabled, and source engine/statistics state is active. Unit header
flags omit active-world registration. The biped state byte `+3DC` is explicitly
3 to bound later death release. The configuration byte consumed by the
`CBA60` tail at pointer `4FCDA8 +1` is explicitly zero. The complete live world
configuration and normal animation-to-ragdoll producer are not recovered by
these fixtures.

`D9180` receives an `88`-byte damage event and target full unit datum, with node,
secondary index and collision region NONE and the extra vector pointer null.
These are supplied hit inputs. No original projectile/world-hit finder executes.
Fresh event fields used by the supported direct-hit fixture are:

| Event offset | Type | Supplied or authored input |
| --- | --- | --- |
| `+00` | u32 tag datum | actual SMG damage `E59D0428` |
| `+04` | u32 flags | 0 |
| `+08` | u32 datum | full salted incoming player, or NONE |
| `+0C` | u32 datum | source object, normally NONE in these fixtures |
| `+10` | i16 | incoming membership designator, independently preserved |
| `+54` | float32 | supplied 1; separate amount cases cover 0 and 0.5 |
| `+58` | float32 | supplied 1 |
| `+5C` | float32 | supplied 1 |
| `+60` | float32 | supplied distance 0 |
| `+68` | byte | supplied non-radius 0 |
| `+7C/+7E` | i16 | NONE before original material/reporting work |
| `+84` | byte | actual SMG projectile reporting type 8 |

Position/direction vectors are explicit zero event fixtures. They are not head
or model-node poses. The report stores every event byte. Original `D9180`
owns its stack accumulator; the actual accumulator image is captured at
`DAA90`, rather than substituting the standalone kernel buffer.

## Stock amount and generated result

Actual Lockout SMG damage tag `E59D0428` begins at file offset `01DEEB54`.
Its raw `+1C` field is 0; the `+20/+24` upper pair is exactly 4.75/4.75.
Original `DB160`, passed definition `tag+10`, reads this upper pair. Equal
bounds skip its random branch. For supplied event `+54/+5C = 1` and player
strength byte `+C1 = 0`, the result is **4.75** (`40980000`), and neither RNG
stream changes. Using tag `+1C` alone as the direct bullet amount produces 0
and does not implement this source routine.

`DB160` computes, in original SSE order,
`((1 - event54) * tag1C + selected_upper * event54) * event5C`, then applies
the original multiplayer scale through `15F520`. The executed strength inputs
0/1/2/3 yield multipliers 1/0.75/0.5/0.25; inputs 4 and FF take multiplier 1.
Those are explicit field fixtures, not a claim about every session property's
default. All 18 amount cases preserve RNG. Authority: `DB160`, `15F520` and
`source_amount` in the report.

The real Spartan damage-info block begins at file offset `01EBD57C`, from
model `E9F3086E`; maximum body is 45 and shield is 70. The original damage
table/material lookup and maximum-vitality paths execute, using the real model
and global cache blocks. With no collision-region entry, source body processing
uses the authored default region. No headshot or fabricated hit material is
supplied. Original shield/body callees execute before `DAA90` constructs the
`58`-byte result:

| Result offset | Original producer |
| --- | --- |
| `+00` byte | event `+84`, actual cause 8 |
| `+04` u32 | source event/accumulator flag translation |
| `+08` u32 | event damage tag `+00` |
| `+0C` u32 | event `+08` full attacker datum, preserving salt |
| `+10` u32 | event `+0C` full source object datum |
| `+14` i16 | event `+10` designator; producer copies its four-byte container |
| `+18..20` | selected event direction vector |
| `+24..2C` | event origin `+30..38` |
| `+34` float32 | event `+5C * +54` |
| `+38` float32 | event `+58` |
| `+3C` i32 | sign-extended accumulator section short `+08` |
| `+40` i16 | accumulator node short `+10` |
| `+44` float32 | accumulator `+28`: normalized body damage |
| `+48` float32 | accumulator `+20`: normalized shield damage |
| `+4C` float32 | event distance `+60` |
| `+50` u32 | original `CB1D0` damage reaction selection |

Both damage amounts are full normalized requests; the lethal body amount is
not clipped to remaining body vitality. The following actual source outputs
are recorded for a valid enemy:

| Initial shield/body | Result `+44` bits | Result `+48` bits | Result flags | Final body/shield bits |
| --- | --- | --- | --- | --- |
| 1 / 1 | `00000000` | `3D8AF8B0` | `00000108` | `3F800000 / 3F6EA0EA` |
| 0.05 / 1 | `3CE38E3C` | `3D8AF8B0` | `0000010A` | `3F78E38E / 00000000` |
| 0 / 1 | `3DD82D83` | `00000000` | `0000010A` | `3F64FA50 / 00000000` |
| 0 / 0.01 | `3DD82D83` | `00000000` | `0000010B` | `BDC3B2A2 / 00000000` |
| 0 / exact `3DD82D83` | `3DD82D83` | `00000000` | `0000010B` | `00000000 / 00000000` |

`CB1D0` and its original reaction selector execute before the result reaches
`DAC40`. The reaction is therefore a generated result input to aftermath,
not a supplied zero reaction field. Source x87 control is nearest even with
53-bit precision (`027F`), and SSE MXCSR is `1F80`.

## History initialization, amount and identity

The original `C52C0` prefix clears counter `+2EA`, stores NONE at timestamp
`+2EC`, and initializes all four damage-history entries at `C5446..C5462`:
timestamp NONE, amount 0, source object NONE, attacker player NONE, cause 0.
Three poisoned-buffer cases execute this reset. The prefix also clears unit
`+13C`; assignment is a later operation, executed separately in normal fixtures.

Each entry occupies `10` bytes at unit `+2F0 + i*10`, with tick at `+00`,
float32 amount at `+04`, full source object at `+08`, full player at `+0C`.
Cause bytes occupy unit `+330+i`. Offsets and sizes here are hexadecimal.

At `CB436/CB43B`, original `CB410` adds result **`+48` then `+44`** in SSE
and saves the single-precision total. At `CB777`, `CBA60` receives this total
in XMM0, the full result attacker/source object, independent result designator,
death-result bit, damage-effect reporting short and cause byte. Every full-hit
case checks the actual XMM0 against the two source-generated result fields.
History therefore weights normalized shield/body damage, not raw 4.75.

Complete `CBA60` preserves the previously established four-entry contract:
matching non-NONE full player or full source object accumulates float32 amount,
refreshes tick/cause and retains original stored identities; otherwise the first
empty entry is used. When full, it protects the first strict maximum-damage
entry and replaces the oldest other timestamp. No expiration service clears
the entries in these paths. Attribution later applies its own source cutoff.

History recording does not validate the attacker player salt. Stale full
datums can be recorded, and later `1ED960` validates them. Four zero-amount
cases also establish that identity, rather than positive amount, controls the
writer call: a non-NONE player or object records amount zero at tick 200;
both NONE skip `CBA60`. Valid zero-damage history must not be omitted by an
invented positive-damage filter. Authority: actual `D9180 -> CB410 -> CBA60`
cases, `zero_amount_identity_cases`, and the earlier isolated history writer.

The lethal `CBA60` tail additionally uses the incoming designator versus unit
`+138`, then a valid player's current unit or valid source object. In the
ordinary enemy fixture it increments the incoming attacker's unit short `+2EA`
and writes current tick to `+2EC`. Previous NONE or elapsed time strictly above
the original 4.0-second constant resets the count first. This short increment
is a source short operation, independent of clamped player statistics.
The conditional configuration-dependent reset branch is disabled by the
explicit fixture byte; its live configuration producer is not established here.

## Executed death scheduling and attribution

The original execution order is:

1. `D9180`: amount `DB160`, shield `DA710`, body `DA2B0`, result `DAA90`.
2. On a new depletion, `D7E00` sets unit body-depleted bit `+10A.2` while the
   player still owns the unit. Its `B9A60` call is a registry helper, whose
   inactive-registration branch returns in this fixture; it is not a supplied
   animation result.
3. `DAC40`: for the eligible lethal result, original `1ED960` reads **previous
   history**, selects credit, updates credited-kill counters, reaches the
   `15E4D0` marker/countdown/event writer, then updates death/suicide counters.
4. `CB410`: damage-reaction service `CAC00`, source `15E250` feedback state,
   then `CBA60` records the current hit.
5. Damage-feedback service `115FA0`, original cancellation `C9A40`, and lethal
   `CBDC0 -> 14E240` detach the current unit.

The source entries and ten stop-before cases establish this order. At
`1ED960`, history equals the pre-hit history. At `CB410` and `CAC00`, the
lethal hit is still absent. Before `115FA0`, it is present but ownership remains
attached. Before `CBDC0`, ownership remains attached. At `CBE95`, player `+30`
holds the old unit, `+2C` is NONE, unit `+13C` is NONE and `+140` holds the full
prior player datum. The credited kills/deaths occur once in the declared
25-hit sequences; these cases do not establish all possible repeated-event
idempotence paths.

The direct-attacker/designator gate, unsigned strict six-second history cutoff,
largest-amount selection, salt validation, assist threshold and direct fallback
are unchanged `1ED960` instructions. The prior Slayer report describes those
reader cases. Here they receive history produced by actual SMG damage entries.

All sequences begin with full body/shield and supply hits at ticks
`200 + 2*i`, with no recharge or world/weapon update between hits. Each hit
has a fresh event. In these fixtures, shields deplete on hit 15 and death occurs
on hit 25. The lethal report has flags `109`; body remains negative. This is
a declared damage-input sequence, not proof of magazine/reload cadence or a
complete live combat timeline.

| Hit inputs | Original credited player and counters |
| --- | --- |
| all 25 from player 0 | player 0 kill +1; victim death +1 |
| first 20 from 0, last 5 from 15 | player 15 kill +1; player 0 assist +1; victim death +1 |
| 24 from 0, uncredited final | prior player 0 kill +1; victim death +1 |
| 24 from 0, self/designator 3 final | prior player 0 kill +1; victim death and suicide +1 |
| 24 from 0, self/**mismatched designator 0** final | victim credited as suicide; victim death/suicide +1 |
| 24 from 0, stale-salt direct player-0 final | credited NONE; victim death +1 |

The mismatched-designator row is an explicit invalid ordinary-input fixture
demonstrating why the two fields cannot be collapsed. It is not a claim that
normal SMG hit finding creates that input. Likewise, self/uncredited finals are
explicit attribution inputs, not proof of a physical SMG self-hit.

With correct self designator, selected credit and suicide counting differ:
the prior enemy receives the kill, while the original incoming full player
equals the victim and still increments the suicide counter. With a stale direct
enemy datum, older valid history from the same slot is not silently substituted.
Native slot-only `last_damager` cannot represent these source outcomes.

Each sequence separately executes original Slayer `2C2350` with actual writer
arguments and cause-class NONE: ordinary credited enemy score +1, source-selected
suicide score -1, NONE no score. This keeps the unchanged callback testable
without fabricating its marker/event intermediates. This separate composition
does not prove cause-class conversion, optional bonuses or event delivery.

## Supplied services and remaining boundary

`1781F0` effect emission and `BB6E0` model/variant updates are supplied void
leaves when invoked. Reaction animation `CAC00` and feedback `115FA0` are
supplied void leaves in completed hit paths. One ordinary reaction prefix runs
actual `CAC00` through the entry of animation lookup `111700`; history remains
unwritten there. No animation, current-node transform or head pose is supplied.
The fixture's zero vectors do not execute a physical world impulse.

`15E4D0` is one recorded supplied boundary for death history/marker/countdown,
events and score dispatch. The original caller passes its actual credited full
player, victim full datum, relation flag and cause byte. Earlier frozen timer
research owns its bounded countdown behavior. This report does not replace
that writer with a new timer, pose or event policy.

The narrow integration contract is: preserve event/result identities separately;
use actual stock amount and source normalized result fields; on lethal damage,
read existing history and run attribution/counters before recording the lethal
hit; record that hit in `CBA60` order; detach ownership in the later aftermath
stage; reset new-unit history with the source initializer. Keep collision-region,
animation, marker/event and corpse boundaries explicit. Full live projectile
admission, generated head poses, active-world registration, broader player
traits/options and all damage categories remain unproved.
