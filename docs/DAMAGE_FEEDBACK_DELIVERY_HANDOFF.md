# Damage feedback delivery and shield rendering handoff

2026-10-12. This investigation authenticates the next complete TU1.1/TU1.5
feedback owners and their actual lower calls. The parent implementation owns
native dispatch, generators and comparisons. This note does not claim that a
paired body has executed every branch, that a shot was naturally admitted, or
that a shield was rendered. The full [damage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md)
remains active.

## Authority and complete correspondence

The unchanged TU1.1 image is
[default.xbe.03](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03),
SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The locally owned TU1.5 image is
`/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Each paired report retains both raw instruction listings, encoded bytes,
normalized operands, internal branches and literal tables. No address delta,
decompile label, supplied result or instruction patch establishes a pairing.
[Pairing tool](../tools/research_unit_pose_requests_u5.py),
[original first-boundary report](../analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/report.json).

| TU1.1 | TU1.5 | Instructions | New report |
| --- | --- | ---: | --- |
| `110B00` | `111A20` | 26 | [feedback lower](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/report.json) |
| `E5680` | `E65A0` | 35 | same |
| `E5530` | `E6450` | 75 | same |
| `CAAD0` | `CBC50` | 92 | same |
| `B97D0` | `BA910` | 29 | same |
| `1CE7E0` | `1D30A0` | 26 | same |
| `CAA10` | `CBB90` | 47 | [response](../analysis/damage-shields-goal-20261011/delivery-next-boundary/response/report.json) |
| `CB410` | `CC590` | 423 | same |
| `D2310` | `D33A0` | 48 | [feedback services](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/report.json) |
| `CAC00` | `CBD80` | 284 | same |
| `115FA0` | `116EC0` | 233 | same |
| `C9A40` | `CABC0` | 108 | same |
| `32E59C` | `33210C` | 7 | same |
| `329185` | `32CCF5` | 51 | same |
| `329168` | `32CCD8` | 6 | [actual caller resolution](../analysis/damage-shields-goal-20261011/delivery-next-boundary/conversion-v2/report.json) |
| `11E3B0` | `11F650` | 39 | [feedback next](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-next/report.json) |
| `111700` | `112620` | 201 | same |
| `20F3D0` | `213CD0` | 28 | same |
| `115CE0` | `116C00` | 76 | same |
| `115E40` | `116D60` | 43 | same |
| `25980` | `25980` | 14 | same |
| `C6AA0` | `C7C20` | 81 | same |
| `32E529` | `332099` | 6 | same |
| `32E540` | `3320B0` | 7 | same |
| `32E5B2` | `332122` | 5 | same |
| `32E5BF` | `33212F` | 16 | same |
| `32E4D7` | `332047` | 23 | same |

These fresh reports authenticate **27 bodies / 2,029 instructions** beyond the
two already paired first-boundary bodies `B9A60/CB1D0`. Reports are independent
of concurrent production edits. Their complete-body status is static evidence,
with actual lower calls retained rather than assigned successful outcomes.
[Research generators](../analysis/damage-shields-goal-20261011/delivery-next-boundary/).

## Ordinary response selection and exact inputs

All stack offsets in this note refer to ESP on function entry. `CB1D0` takes
four stack DWORDs: unit handle at `+4`, caller damage/report flags at `+8`,
damage request pointer at `+C`, and constructed report pointer at `+10`;
it returns with `RET 10h`. It resolves the unit header/body from `*4E78D0`
and the literal definition/tag table from `*4EB114`. The report's `+8` is its
damage-tag handle, whose tag pointer is advanced by `10h` before reading damage
fields. It writes report `+50=0` before its response logic. The request `+4`
bit4, damage-field `+4` bit4 or unit `+134` bit19 suppresses response selection.
The complete original body preserves the partially written local bytes and
whole DWORD argument copies; a replacement must not zero unrelated local bits.
[CB1D0 instructions](../analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/instructions/000cb1d0-old.asm).

For the admitted response call, `CAAD0` receives `EAX=unit`, `BL=report+4 bit0`
and six stack DWORDs: the locally selected first flag, the animation-test flag,
the locally selected response-limit flag, the locally selected fourth flag,
`unit+F4 + report+48`, and `unit+F8 + report+44`. It removes all six with
`RET 18h`. With BL clear and unit `+10A` dead bit2 clear, it calls `CAA10`
with `EAX=unit`, `EDI=local output DWORD` and the last two float arguments.
The fourth flag forces response2; the third flag can cap a result above1;
unit `+134` bit5, the original options/cheat condition, or non-NONE unit `+13C`
can also cap a result above1. With the dead bit set, it returns0 without
calling `CAA10`. BL-set branches return literal response6/5/7/3 according to
their original flag and definition-threshold tests.
[CAAD0 instructions](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/000caad0-old.asm),
[caller argument construction](../analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/instructions/000cb1d0-old.asm).

`CAA10` sets its output DWORD to0. A present actor uses the actual actor-array
stride `888h`, actor `+54` and the already paired `1E83A0`; a returned override
supplies four threshold fields. Actor NONE or a null override uses the actual
unit definition: the second float greater than definition `+10C` selects2;
otherwise the second float, then the first float, greater than definition
`+104` selects1. The strict `COMISS/JA` branches include their actual unordered
behavior. Both Lockout multiplayer unit definitions have literal zero at
`+104`, `+10C` and `+114`; these are authored bytes, not defaults invented for
the diagnostic body. Unit/actor runtime storage still needs its genuine owner.
[CAA10 instructions](../analysis/damage-shields-goal-20261011/delivery-next-boundary/response/instructions/000caa10-old.asm),
[fresh cache extraction](../analysis/damage-shields-goal-20261011/delivery-next-boundary/authored-feedback-inputs.json).

The report-bit0 branch first calls `110B00(EAX=unit, stack keys7000101/D000042)`.
That routine uses the signed component offset unit `+12A` and the actual
`1CFDE0` animation request owner; only AL is set from the returned WORD
discriminator. `E5680(EAX=unit)` returns true only through its original byte
`+C1`, byte `+348`, signed byte `+399`, byte `+3DC`, parent `+14`, and clock
tests. `E5530(EBX=unit)` writes byte `+39D` from the original rounded
tick-rate/definition `+214` product capped at255 and always calls `116320`
before its conditional RNG/action branches. The current two authored unit
definitions have `+214=1.7000000476837158` and `+1F0=10h`, so their `E5530`
extra-action bit10 test is clear. Authored flags do not establish constructed
component bytes or a successful animation request.
[110B00](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/00110b00-old.asm),
[E5680](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/000e5680-old.asm),
[E5530](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/000e5530-old.asm),
[authored unit fields](../analysis/damage-shields-goal-20261011/delivery-next-boundary/authored-feedback-inputs.json).

`B9A60` receives its unit in **ESI**, with no stack arguments. Its ordinary
header-byte `+2` bit40-clear path returns. When that bit and body `+4` bit9
are set, it builds an actual 20-byte record through `B97D0(EDX=unit,EDI=output)`
and visits the unit's `+60` list through `1CE7E0`. `B97D0` can call `16CEB0`
before copying body byte `+AA`, position `+40` and DWORD `+4C`. `1CE7E0`
uses EAX as its first list handle, EDI as the descriptor, and two stack
arguments; it follows each record's `+8` link and calls `29C590` on the
descriptor-selected callback. Those lower services are not supplied here.
[B9A60](../analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/instructions/000b9a60-old.asm),
[B97D0](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/000b97d0-old.asm),
[1CE7E0](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-lower/instructions/001ce7e0-old.asm).

## Report writer, subsequent services and cold fields

The four shimmer-related recent-damage stores belong to **DAC40**, before
its later typed impulse/reaction calls. Body report `+44` is accumulated into
object `+100` and `+F8`, clamped at1, then signed byte timer `+109` is reset0.
Shield report `+48` is accumulated into `+FC` and `+F4`, clamped at1, then
timer `+108` is reset0, unless object `+10A` bit11 suppresses that shield
branch. Report bits0/1 can substitute an amount1 before the ordinary
`amount>.0001` test. CB1D0 computes response before DAC40 writes these banks;
the caller sequence must remain intact.
[DAC40 original stores](../analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/instructions/000dac40-old.asm),
[D9180 original caller](../analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/instructions/000d9180-old.asm).

`CB410` takes stack `{unit,report}` and returns with `RET 8`. It reads the
already written recent-damage banks. At `CB645`, positive `object+100 + FC`
updates last damage category `+2C8`, tick counter `+2CA`, amount `+2CC` and,
when present, last owner `+2D0`. Positive report body or shield requests call
`D2310(EAX=unit,stack{damage_tag+30,0})`. A positive report `+50` calls
`CAC00(ECX=report+18,stack{unit,response,unsigned WORD report+3C})`.
Subsequent branches include player delivery, animation/action processing,
zoom cleanup, AI notification, camouflage/feedback and lethal reaction.
The complete original direct-call list is retained with both-image sites.
[CB410 instructions and call list](../analysis/damage-shields-goal-20261011/delivery-next-boundary/response/report.json).

The ordinarily reached lower owners require real current state:

- `D2310` tests unit byte `+2BC` first; nonzero calls `C6AA0`. Its other branch
  updates `+2B8` and `+2B0`, and a non-NONE entity `+D4` tail-calls the existing
  replication owner `B6760` with dirty mask `800000h`.
- `CAC00` reads unit signed component `+33E`, animation component `+12A`,
  forward fields `+70/+74`, current weapons and authored thresholds. Its
  direction path calls the existing `2B3E0`, then `11E3B0`, then animation
  selector `111700`; later branches have additional live animation/weapon
  owners. A raw zero-filled direction or fabricated component does not prove
  actual impact reaction.
- `115FA0` uses signed component `+342` and actor/RNG/action banks. It calls
  `CAA10`, `25980`, `20F3D0`, `115CE0` and `115E40` on their actual branches.
  `115E40` copies12 DWORDs into its action component and calls `115DB0` after
  possible `116640`; those are real remaining lower calls.
- `C9A40(EAX=unit,stack{force_byte})` resolves actual player and active weapon
  slot byte `+212`, clears bytes `+240/+241` and float `+26C` on its admitted
  branch, and can call `105120` or tail-call `18B0F0`. A non-NONE player also
  needs the existing player/control banks.

[D2310](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/instructions/000d2310-old.asm),
[CAC00](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/instructions/000cac00-old.asm),
[115FA0](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/instructions/00115fa0-old.asm),
[115E40](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-next/instructions/00115e40-old.asm),
[C9A40](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/instructions/000c9a40-old.asm).

CB410's other direct callees are `BACC0,D7C60,BB1F0,D8280,1781F0,BBCB0,
155460,15E250,CBA60,1CCFC0,1CD670,1CD800,CBDC0`. Some are already native-owned
elsewhere; this report does not assert their live bank readiness. `111700`
also calls `111B60,CA320,E86E0,E5680,1C9D80,1CF540,110BF0,1CF6A0` and reads
its actual animation/control components. The complete-body correspondence is
not a reason to replace those services with successful returns.
[CB410 complete call edges](../analysis/damage-shields-goal-20261011/delivery-next-boundary/response/report.json),
[111700 complete call edges](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-next/report.json).

## Conversion ambiguity resolved by actual edges

The initial prefix search found identical normalized six-instruction bodies at
TU1.5 `32CCD8` and `37D0B4`; that [ambiguous report](../analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/report.json)
is unchanged. The already authenticated D98E0 caller contains TU1.1
`D9A7F: CALL 329168` paired with TU1.5 `DA9CF: CALL 32CCD8`. The fresh `11E3B0`
pair independently calls the same destination. The retained resolved report
includes exact call bytes and both lower call mappings.
[Actual incoming edge and complete body](../analysis/damage-shields-goal-20261011/delivery-next-boundary/conversion-v2/report.json),
[11E3B0 call](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-next/instructions/0011e3b0-old.asm).

The body is part of the **acos** path, not an integer conversion: it stores
x87 ST0 as a double, classifies it through `32E59C` and calls `329185`.
The ordinary finite in-range branch computes the original `FSQRT/FPATAN`
sequence. Original control-word, exceptional input, error and restoration
paths remain significant. Five more paired CRT owners lead to remaining
`32F980` error dispatch; an arbitrary FP environment or a successful error
callback is not supplied here. The global `4881F4` branch and the x87 status
checks must retain their actual effects.
[329168](../analysis/damage-shields-goal-20261011/delivery-next-boundary/conversion-v2/00329168-old.asm),
[329185](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-services/instructions/00329185-old.asm),
[CRT lower owners](../analysis/damage-shields-goal-20261011/delivery-next-boundary/feedback-next/report.json).

## Current shield renderer handoff

The current host damage path updates its separate `Unit::damage_feedback`
through `report_damage_feedback`, and ordinary host updates decay it through
`update_damage_feedback`. `MultiplayerMatch::snapshot` copies that state into
`MultiplayerState::Player::damage_feedback`; network serialization transports
the values and timers. These current sources do not establish that the native
source object's `F4/F8/FC/100` bank is the host state's authority.
[Simulation damage/update](../src/simulation.cpp), [snapshot](../src/multiplayer.cpp),
[network](../src/multiplayer_network.cpp), [state](../include/halo2/multiplayer.hpp).

The inspected multiplayer body draw takes actual posed model vertices and
ordinary `SourceModelShaders` selection, but it does not read the copied
player damage-feedback state. Its frame material input callback resolves
weapon inputs/ammunition and otherwise returns the ammunition fallback0;
it does not resolve body/shield recent damage. The current renderer has no
live call to `select_shield_overlays`, `damage_feedback_input` or
`SourceShieldModelShaders`. Those standalone selector, shader-input and
plasma-material helpers remain real implementations with isolated tests,
but current symbol-reference inventory finds only their definitions,
declarations and internal helper use in production.
[Current multiplayer body/material paths](../src/multiplayer_runtime.cpp),
[current renderer](../src/renderer.cpp),
[fresh symbol and source-hash audit](../analysis/damage-shields-goal-20261011/delivery-next-boundary/native-render-bridge-audit-v2.json),
[selector](../src/shield_overlay.cpp), [shield model materials](../src/source_shield_model_shaders.cpp).

Original retail overlay admission uses **object+F4 > 0**; the separately
transported overlay vitality is **object+F0**. The ordinary threshold-damaged
effect is independent: the actual Spartan/Elite damage-info `+A0` threshold
is0, while world and FP overlay shader references exist. The prior original
retail listings establish clone order, exact geometry/shader part admission,
lighting scalar and shared transparent scheduling. Those actual source
contracts must be joined to the source-backed recent-damage state and the
live renderer; a new independent sphere, material loop or immediate draw
would not establish that chain. This note does not newly prove TU1.1/TU1.5
counterparts or pixels for those retail render owners.
[Original selector/collector evidence](SHIELD_OVERLAY_SOURCE_RESEARCH.md),
[plasma pass evidence](SHIELD_PLASMA_PASS_RESEARCH.md),
[shared live collector/scheduling evidence](SHIELD_LIVE_PRODUCER_RESEARCH.md),
[fresh untouched shield references](../analysis/damage-shields-goal-20261011/delivery-next-boundary/authored-feedback-inputs.json).
