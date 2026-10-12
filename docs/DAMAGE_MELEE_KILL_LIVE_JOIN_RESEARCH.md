# Exact melee and kill-volume gameplay seams

2026-10-12. Research only; no production/test edits or supplied contacts,
damage results, pose state, allocator success or gameplay execution. Findings
below distinguish reusable original leaves from their still-required live
producers. The root source is changing concurrently; this supplements the
earlier [remaining-joins audit](DAMAGE_REMAINING_LIVE_JOIN_AUDIT.md).

## Kill-volume admission can be separated from Havok

The original selector and an **unparented** authored-volume point test need no
Havok/raw-world initialization. They still require a real full object identity,
the actual simulation clock, retained disable state and the object's current
query-bounds center. Those inputs cannot be replaced by render state or a
native body-root proxy.

Original TU1.1 `1EFEF0` consumes EDI=full unit datum:

1. Read scenario pointer `*4E7920`; signed count is scenario `+230`, signed
   16-bit row-index bank is scenario `+234`. Nonpositive count returns false.
2. Read actual 32-bit tick at `(*518444)+8`. Add `(datum & 0xFFFF)` with
   wrapping 32-bit arithmetic. Interpret that result as signed, then use
   `CDQ; IDIV signed_count`; **signed remainder** selects one row. Salt does
   not enter this arithmetic, but a full salted live datum remains mandatory
   for the subsequent object lookup. An unsigned modulo is not equivalent at
   the signed overflow boundary. A native API must reject an impossible
   negative/out-of-bank remainder explicitly, never clamp or reschedule it.
3. Disable bank is `*526258`; test DWORD `[row >> 5]` and bit `(row & 31)`.
   A set bit returns false. Read the selected signed 16-bit kill row; `-1`
   returns false. There is no scan of the remaining rows after rejection.
4. `11DB80(full_unit, selected_trigger)` resolves the actual object body and
   passes **body+30** to `11DA40`. `+30/+34/+38` is the retained query sphere
   center; body+64 root position and primitive+40 center are different owners.

`1EFE10` allocates **0x24 bytes**, stores its real pointer at `526258`, and
writes active byte `+20=0`. `1EFE70` map reset clears all **eight DWORDs** and
writes active byte `+20=1`. The selector does not read that active byte. A new
native typed bank can reproduce this initialization/reset, but later script
enable/disable changes must update this same bank. Do not silently invent a
fresh all-enabled bank per tick.

The `11D950` transform owner reads trigger row
`scenario[+10C] + signed_index*0x44`:

| Row offset | Original operand |
| --- | --- |
| `+04` | signed object-name index; `-1` means unparented |
| `+08` | parent marker/node name selector |
| `+0C..+14` | authored forward vector |
| `+18..+20` | authored up vector |
| `+24..+2C` | authored position |
| `+30..+38` | positive local extents |

For unparented rows `143860` constructs scale 1, forward, **up cross forward**,
up and translation in its 0x34-byte matrix; it does not normalize the authored
axes. For named parents, name must be in `[0,0x280)`, resolve through
`*4E58A0`, then `16F510` must find the marker/node. The current source WORLD
node bank uses signed body `+116` offset and 0x34 stride. `1441D0` composes
the parent node with the authored volume matrix. Missing name/marker returns
false; an idle/root transform cannot replace this branch.

`11DA40` rejects zero scale, subtracts matrix translation, and if scale is not
1 multiplies the three deltas by scalar `1/scale`. For each basis column `b`,
its scalar SSE evaluation is:

```text
local = (b.z * delta.z + b.y * delta.y) + b.x * delta.x
```

Every multiply/add is single-precision in that order; preserve the original
ordered comparison behavior. Admission requires strictly
`0 < local.x < extent.x`, `0 < local.y < extent.y`,
`0 < local.z < extent.z`. Every face/edge/corner boundary is outside. Do not
replace this with an epsilon, inclusive AABB or a differently associated dot.

Original verified listings:
[selector](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/001efef0.asm),
[bounds operand](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/0011db80.asm),
[transform selection](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/0011d950.asm),
[point predicate](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/0011da40.asm),
[basis constructor](../analysis/damage-shields-goal-20261012/remaining-live-joins/00143860-tu11.asm),
[allocation](../analysis/damage-shields-goal-20261012/remaining-live-joins/001efe10-tu11.asm),
[reset](../analysis/damage-shields-goal-20261012/remaining-live-joins/001efe70-tu11.asm).

## Exact admitted kill damage request

The ordinary postphysics owner `DEFC0` / TU1.5 `DFEE0` performs these steps;
the selector itself does not deal damage:

1. Resolve actual root with `BBE70`; root byte `+C1` bit0 suppresses the kill
   admission call. On positive selector result set unit byte `+349` bit7.
2. Options byte `(*4EDF18)+0C == 4` skips damage, after that flag write.
3. Read globals pointer `*4E791C`, its block pointer `+144`, then the damage
   datum at block `+28`. The measured Lockout cache identifies this as
   **EAE90964, `jpt! globals\\distance`**; use the actual imported datum,
   never this measured handle as a universal constant.
4. `D7C60` initializes the request. Its size is **0x88 bytes / 136 decimal**,
   not 88 decimal: it zeroes 0x22 DWORDs. ABI is EDX=request pointer plus one
   stack argument damage datum; `RET 4`.
5. Set request DWORD `+04 |= 4`; set request byte `+84 = 1`. Invoke
   `D7CD0(full_unit, request+08)` for true current attribution. It follows
   unit `+248` ownership recursively; unit player/team fields or ordinary
   object owner fields supply the result. Do not fabricate environment owner
   or substitute the host's `last_damager=-2`.
6. Invoke **`D9180(request, full_unit, -1, -1, -1, 0)`**. No collision-region
   entry, amount, zero health or dead flag is supplied by the caller. Preserve
   ordinary delivery, normalized reports, history, shield effects, callbacks,
   sound, death and cleanup from this dispatcher; publish only its real result.

Constructor output, before the caller's changes:

| Request offset | D7C60 output |
| --- | --- |
| `+00` | actual damage datum |
| `+04` | zero flags |
| `+08`, `+0C`, `+14`, `+18`, `+1C` | `FFFFFFFF` DWORD |
| `+10`, `+20`, `+7E` | `FFFF` word |
| `+22` | original word literal at `46FB64` |
| `+54`, `+58`, `+5C` | original float-1 literal at `465030` |
| `+7C` | original default-material word at `484DE0` |
| `+84` | zero byte |
| other bytes | zero from `REP STOSD` |

Request offsets are relative to the constructor's EDX, not the caller's moving
ESP. Existing source modules already implement the constructor and owner
instead of requiring hand-built packet writes:

- `source_run_biped_falling_damage_runtime`, canonical `D7C60`, complete paired
  constructor, TU1.5 `D8BB0`.
- `source_run_damage_delivery_runtime`, canonical `D7CD0` and `D9180`, paired
  TU1.5 `D8C20` and `DA0D0`; actual reached lower services remain required.
- `source_run_biped_postphysics_phase_1_1` already contains complete
  `DEFC0`, selector, point/transform helpers and their authentic ordinary call
  sequence over a retained source bank. Running it requires that bank's actual
  contact/animation/physics and lower owner dependencies; a smaller native
  selector cannot be advertised as the whole phase.

[Original caller](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000defc0.asm),
[constructor](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000d7c60.asm),
[attribution](../analysis/multiplayer/integration/spartan-kill-trigger-20261010/probe/instructions/000d7cd0.asm),
[paired constructor interface](../include/halo2/biped_falling_damage.hpp),
[delivery interface](../include/halo2/source_damage_delivery_runtime.hpp),
[complete phase](../include/halo2/biped_postphysics_phase.hpp).

## Existing bounds providers and smallest next integration

The source-world branches already expose a real non-render provider:
`MultiplayerObjectRuntime::object_sphere(full_datum)` reads retained body
`+30/+3C`; TU1.1 resolves the full salted header and TU1.5 uses the validated
`object_body(full_datum)`. Match exposes `source_object_sphere`. Read it after
the phase that actually produces bounds. A retained valid handle alone does
not prove fresh bounds after an out-of-band native teleport.
[Provider](../src/multiplayer_object_runtime.cpp#L571),
[TU1.5 provider](../src/multiplayer_object_runtime.cpp#L752),
[Match API](../src/multiplayer.cpp#L439).

Without source world, current ordinary `Unit` has no authoritative current
WORLD pose bank or query-sphere publication. Renderer fallback computes a
sphere from its model/idle pose; native `center(Unit)` rotates the authored
offset by yaw. Neither is the missing gameplay producer.
`source_shadow_sphere` is an existing exact regular-type leaf for computing
the query sphere **when actual current WORLD node0 is provided**;
`source_object_light_spatial_bounds` then derives separate dynamic/primitive
bounds. These leaves do not produce node0. Rendering should consume the
gameplay-owned pose/bounds publication, never supply it.
[Query-sphere leaf](../src/source_shadow.cpp#L83),
[spatial contract](../include/halo2/object_light_spatial.hpp#L13),
[renderer fallback](../src/multiplayer_runtime.cpp#L2147),
[native center proxy](../src/simulation.cpp#L304).

The smallest defensible next routine is a typed original **one-row selector
plus unparented strict transformed-point predicate**, with mandatory current
bounds provider and retained mask/reset owner. Verify it against unchanged
original bytes for row scheduling, disable and NONE rows, float boundaries,
rotations and signed wrapping. Then connect positive admission to the ordinary
authored request/delivery owner above. Replace the current direct-death bypass
only for branches that have genuine current bounds and completed delivery;
do not replace it by another direct-death formula. Native fallback needs the
gameplay pose/bounds producer first. Named-parent support separately requires
the actual name/marker/current-node services.

## Melee impact: genuine reusable lower seam

`EB700` checks actual action component signed progress `+32` against impact
byte `+33`. At equality it invokes
`D0830(unit, semantic, unsigned_modifier_byte_30,
float(unsigned_speed_byte_31)*original_literal_465024)`.
The unit's signed component offset is `+346`, semantic is component `+2C`.
This is the exact event input; an immediate input-edge hit bypasses this owner.

`D0830` resolves the current weapon from unit signed `+212` and full slots
`+218`, calls **CC320** for its source-produced origin, reads aim `unit+168`,
constructs perpendicular axes with `11E720` and original normalization, then
issues ordered **25** casts through
`16B240(0x2480000F, origin, delta, unit, NONE, contact)`.
Outer and inner sample indices are each `-2..2`. Weapon `+1B0/+1B4` define
the half-width axes; `+1B8` defines reach. First object wins; later biped is
preferred over a non-biped; two bipeds use strict smaller fraction. A weapon
contact redirects through its actual parent when present. Do not turn these
into a generic single ray or pick a fake material/normal for a capsule hit.

After selecting the semantic damage/effect references, actual object delivery
is **D10F0**, not D0830's later self-effect branch:

- It initializes selected damage with `D7C60`, ORs request flags with1 and
  obtains actual attacker attribution from `D7CD0`.
- Real marker `B9C20(unit, 04000095, count1, out)` supplies request `+24`;
  attacker bounds center supplies `+30`; attacker aim supplies `+3C/+48`.
- Its hit packet supplies request `+54` scale, `+7C` material and `+80`
  weapon-type flags. Ordinary object dispatch is
  **`D9180(request, target, -1, -1, -1, 0)`**: melee intentionally does not
  pass a collision-region entry. Preserve real contact fields for selection,
  material and impact effects, without inventing region-aware melee damage.
- Target type7/`+1CC` bit5 reaches required `10B500`; no-object structure
  contact reaches **185B00** instead. `D0340` owns the actual material/weapon
  impact effect path. These are separate genuine lower services.

The typed reusable contact chain exists:
`source_vector_contact` → ordered `source_object_light_spatial_scan` →
`source_object_vector_contact` → `source_object_posed_target`. It accepts
actual owner flags/hierarchy, registered source primitives, model collision,
current WORLD node matrices and runtime permutations; it does not generate
those banks. Props already use the posed-target leaf. Existing vector/posed
parity fixtures prove these leaves over declared retained snapshot banks,
not ordinary native player pose production or real input-to-hit scheduling.

Concurrent root changes now connect the native animation impact to a typed
5x5 native cast and ordinary scalar damage. That is a bounded native adapter:
its eye-origin, unit capsules and substitute contact context do not execute
CC320, the full vector/registration/current-pose path or D10F0. Complete final
melee coverage still needs these actual producers and lower owners.

[Original impact and D0830](MP_MELEE_ACTION_SOURCE_RESEARCH.md),
[fresh CC320](../analysis/damage-shields-goal-20261012/remaining-live-joins/000cc320-tu11.asm),
[fresh D10F0](../analysis/damage-shields-goal-20261012/remaining-live-joins/000d10f0-tu11.asm),
[fresh impact effect](../analysis/damage-shields-goal-20261012/remaining-live-joins/000d0340-tu11.asm),
[vector API](../include/halo2/source_vector_contact.hpp),
[posed API](../include/halo2/source_object_posed_target.hpp),
[vector evidence limits](SOURCE_VECTOR_CONTACT_PARITY.md),
[posed evidence limits](SOURCE_OBJECT_POSED_TARGET_PARITY.md).

## Verification scope

Freshly byte-checked 2,098 instruction rows in 16 TU1.1/TU1.5 listings against
the owned original executable bytes, including complete D0830/EB700 pairs,
kill admission helpers and newly exported constructor/basis/origin/effect
helpers. Exact image/listing hashes and row counts are retained in
[primary-verification.json](../analysis/damage-shields-goal-20261012/remaining-live-joins/primary-verification.json).
`000bdef0-tu11.asm` in that static manifest is an unrelated TU1.1 routine and
is **not** evidence for the retail BDEF0 query-sphere leaf.

No new original execution or live gameplay check was performed in this
research. Prior positive kill execution reached the authored dispatcher and
preserved momentum, but started from declared placement and originally stopped
at a missing sound owner; it does not prove naturally falling live gameplay.
No root assets were restored or rewritten.
