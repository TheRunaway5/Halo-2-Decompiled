# Original shield shader overlay ownership

Research snapshot: 2026-10-11. This note establishes the ordinary shield-damage
overlay selector and its collector consumers from the original retail
instructions. It does not establish a rendered shield effect, GPU equivalence,
or complete first-person presentation. The wider damage goal remains active in
[DAMAGE_SHIELD_COVERAGE_GOAL.md](DAMAGE_SHIELD_COVERAGE_GOAL.md).

## Primary evidence

- [Retail executable](../assets/disc/default.xbe), SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- [Retail Lockout cache](../assets/disc/maps/lockout.map), SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Original instruction listings are retained in the ignored
  [overlay analysis directory](../analysis/damage-shields-goal-20261011/overlay/).
  The [Ghidra retail function exports](../analysis/retail/functions/) were used
  to navigate; the claims below identify the owning instructions explicitly.
- [Model schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml)
  names the damage-info fields; [object function schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/bipd.xml)
  and [shader schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/shad.xml)
  name the raw cache records. Schema names supplement the executable offsets.

Listings can be reproduced without rebuilding the executable:

```sh
RETAIL_XBE="$PWD/assets/disc/default.xbe" .tools/python/bin/python \
  tools/reference/halo2-decompiled/tools/disasm.py 4bcc0
```

Substitute the routine address from the links below. Raw cache reports were
read with the read-only `Cache` reader in [audit_rendering.py](../tools/audit_rendering.py).

## Selector and ABI

**Retail `0004BCC0` owns admission and first-person/world shader selection.**
Its hidden input is `EDI = geometry context`; six stack arguments are removed
by `RET 18h`. Offsets below are relative to ESP on entry, before its prologue.
`AX` returns the signed accepted model count. The caller subsequently performs
model/section admission, so a selected overlay is not proof of a submitted draw.
[Instructions `4BCC0..4C0CF`](../analysis/damage-shields-goal-20261011/overlay/0004bcc0-retail.asm)

| Input | Original use |
| --- | --- |
| `ESP+04` | Object datum |
| `ESP+08` | Camera-distance metric used by world geometry LOD |
| `ESP+0C` | Caller mode; shield overlay selection runs only when zero |
| `ESP+10` | First-person discriminator output pointer |
| `ESP+14` | Force discriminator false byte |
| `ESP+18` | Output geometry LOD byte pointer |

`4B5D0` supplies its geometry context to EDI and calls this routine before its
accepted model/geometry loop. `133790` passes the render collection's raw mode
at `+2A78` into that chain. The labels of every nonzero mode and all external
callers are outside this note's proof. The selector writes the first-person
pointer conditionally at `4BCF5`, but later dereferences it; the observed
`4B5D0` caller supplies storage. A replacement should not infer a fully optional
pointer contract from the initial null check.
[`4B5D0`, `4B5EC..4B612`](../analysis/damage-shields-goal-20261011/overlay/0004b5d0-retail.asm),
[`133790`, `1339BD..1339F1`](../analysis/damage-shields-goal-20261011/overlay/00133790-retail.asm)

The preliminary gates are an object/parent visibility check through `B9CE0`,
an object definition with model datum at `+38` other than NONE, and a positive
model count from `3D3E0`. A geometry LOD of `FF` later zeros the count. The
first-person discriminator is equality with global `4C1BD0`, then the force
argument may clear it. World geometry LOD uses the existing `4C450` path;
first-person starts at LOD 4. These are geometry gates, independent of the
shield's shader LOD chosen downstream.
[`4BCC0`, `4BCDB..4BE8A`, `4C092`](../analysis/damage-shields-goal-20261011/overlay/0004bcc0-retail.asm),
[`3D3E0`](../analysis/damage-shields-goal-20261011/overlay/0003d3e0-retail.asm),
[`B9CE0` matched reconstruction](../tools/reference/halo2-decompiled/src/unknown_0b8bd0.cpp)

For an otherwise admitted object, `4BF83..4BFDC` selects the damaged shield
overlay exactly when:

1. The raw caller mode is zero.
2. `object+F4 > 0`, using `COMISS/JBE` (unordered also skips).
3. `hlmt+60`, the signed damage-info count, is positive.
4. The selected shader datum is not `FFFFFFFF`.

The damage-info pointer comes from `hlmt+64`. A nonzero first-person
discriminator selects **datum `damage_info+80`**; otherwise it selects
**datum `damage_info+88`**. Successful selection ORs geometry-context flag
`+16C` with bit 0, stores the shader at `+170`, and copies **current shield
vitality `object+F0`** to `+180`. The source does not copy recent damage `+F4`
into that vitality field. There is no species switch, damaged-threshold test,
shield-damaged flag test, or separate `current shield > 0` gate in this block.
[Original selector](../analysis/damage-shields-goal-20261011/overlay/0004bcc0-retail.asm)

The schema's tag-reference starts are `+7C/+84`; the executable reads their
datum words at `+80/+88`. The threshold effect at `damage_info+A8` is a
different owner. Lockout Spartan's threshold is zero and damaged effect is
NONE, while its two overlay shaders are present. Ordinary hit shimmer must
therefore remain separate from threshold-effect dispatch.
[Schema](../tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml),
[raw model references](../analysis/damage-shields-goal-20261011/overlay/lockout-model-shield-refs.json),
[shared damage foundation](DAMAGE_SHIELD_COVERAGE_GOAL.md)

The adjacent overshield block is independent: `object+F0 > 1`, positive
damage-info count, and selected non-NONE datum `+EC` first-person / `+F4` world.
It sets context flag `20h`, shader `+178`, and scalar `+188 = 1 - current shield`.
The literal subtraction produces a negative value above one; this note does
not reinterpret or reverse it. Overshield presentation still needs its own
fixture and renderer integration.
[`4BFE2..4C032`](../analysis/damage-shields-goal-20261011/overlay/0004bcc0-retail.asm)

## Collector, geometry and shader placement

The whole collector is `1E4h` bytes and includes the object datum before the
geometry context, shifting the selector's offsets by four. `133AB0` requires
whole-collector model count `+166 > 0`. With its recursion guard clear, its
special branches run in this order: flag `80h`, camouflage flag `2`, damaged
shield flag `1`, overshield flag `20h`, then the ordinary collector subject to
the existing camouflage branch. This is **collection order**, not a proof of
final pixel blend order.
[`133AB0`](../analysis/damage-shields-goal-20261011/overlay/00133ab0-retail.asm)

| Whole collector field | Damaged shield use |
| --- | --- |
| `+170` | Admission flags, bit 0 |
| `+174` | Selected damaged shield shader |
| `+184` | Current shield vitality |
| `+1AC` | Draw flags; shield clone ORs `8000h` |
| `+1D0` | Override shader passed to draw-unit allocator |
| `+1D4` | Scalar destination discriminator; initialized zero by `133790` |
| `+1DC` | Shield clone's scalar, copied from `+184` |

At `133CEF..133D52`, `REP MOVSD` copies all `1E4h` bytes to a shield clone,
then the original collector's bit 0 is cleared. The clone receives shader
`+1D0`, vitality `+1DC`, and draw flag `8000h`. Recursive `133AB0` is called
with child flag zero and guard one; that guard reaches `133F70` without
emitting additional special clones. The overshield branch performs the same
kind of clone using `+17C/+18C`. The copied accepted geometry, nodes, materials,
coverage and source identities are retained.
[`133CDF..133DCA`](../analysis/damage-shields-goal-20261011/overlay/00133ab0-retail.asm),
[`133790` initialization, `133960..133977`](../analysis/damage-shields-goal-20261011/overlay/00133790-retail.asm)

`133F70` submits accepted model subparts to `2CBF0`. In the latter's entry
stack, `+24` is override shader, `+28` coverage, and `+44` the shield scalar;
`RET 54h` removes 21 stack arguments. `2CBF0` stores a non-NONE override in
`4C1228`, with its byte index in `4C0ED6`, only while the override bank contains
fewer than `C0h` entries. Its draw-unit bank limit is `352h`. These original
budgets are distinct from overlay admission.
[`133F70`, `13413A..1341D0`](../analysis/damage-shields-goal-20261011/overlay/00133f70-retail.asm),
[`2CBF0`, `2CC01..2CC15`, `2CDB0..2CDED`](../analysis/damage-shields-goal-20261011/overlay/0002cbf0-retail.asm)

When the accepted lighting-cache pointer is nonzero and the override is
non-NONE, `2CBF0` quantizes the scalar by multiplication by 255, truncation,
and integer clamp to `[0,255]`. For the ordinary zero `+1D4` discriminator it
writes the scalar into lighting-cache `+77`; the other branch writes its
other supplied scalar into `+76`. This establishes scalar transport, not its
complete eventual shader-extern use.
[`2CE7B..2CEE3`](../analysis/damage-shields-goal-20261011/overlay/0002cbf0-retail.asm)

World dispatcher `44AC0` and first-person dispatcher `402B0` recover the
override through that table, inspect the original accepted geometry/part,
and call `41040` with the chosen override. `41040` requires a supported
source part and a non-NONE authored material, then chooses the override
before `1BF50` shader LOD selection. Its override path additionally checks
the authored and override template masks at `41275..4129D`; those rejection
conditions must remain part of any complete port.
[`44AC0`, `44BBD..44C5C`](../analysis/damage-shields-goal-20261011/overlay/00044ac0-retail.asm),
[`402B0`, `403D1..4046A`](../analysis/damage-shields-goal-20261011/overlay/000402b0-retail.asm),
[`41040`, `410FC..412A3`](../analysis/damage-shields-goal-20261011/overlay/00041040-retail.asm)

Lockout's Spartan first-person/world damaged shield shaders both use template
`E9F50870`, `shaders\shader_templates\transparent\plasma_shield`. Its sole
authored shader LOD exposes only layer 15 (`8000h`), containing pass
`E9F60871`, `shaders\shader_passes\plasma\plasma_shield`. The separate shader
datums have different animation records. In view zero with force clear,
`41040` queues this layer through `1E2D0`; the ordinary path runs when other
layers exist or force is set. `402B0` supplies view one and force one, so the
first-person shield cannot be assigned the world transparent queue solely
from the template name. Exact first-person pass execution and final blending
remain external gates.
[Raw shader/template/pass report](../analysis/damage-shields-goal-20261011/overlay/lockout-shield-shaders.json),
[`41040`, `412A3..4138A`](../analysis/damage-shields-goal-20261011/overlay/00041040-retail.asm),
[`402B0`](../analysis/damage-shields-goal-20261011/overlay/000402b0-retail.asm),
[existing transparent owner research](SOURCE_TRANSPARENT_SORT_PARITY.md)

`3D2C0` initializes the current first-person owner/model bank. Its fallback
local-user path calls `165F16(user, unit, 4, 4C1BD8)` and records the returned
count at `4C1BD4`; special override branches have their own earlier gates.
`3D3E0` returns that bank count for first-person. `3D4F0` takes AL as the
first-person discriminator, EBX as the object datum, and six stack arguments
(ordinal and five output pointers), returning with `RET 18h`. First-person
record stride is `D0Ch`: actual render model `+0`, owner `+4`, flags `+8`,
current matrices `+0C`, with model node count capped at 64. World selection
uses the object's model->render-model reference, current matrices at the
signed `object+116` offset, and the actual render model's node count `+48`.
The shield clone reuses these admitted banks; using a new static body shell
would require separate evidence.
[`3D2C0`](../analysis/damage-shields-goal-20261011/overlay/0003d2c0-retail.asm),
[`3D3E0`](../analysis/damage-shields-goal-20261011/overlay/0003d3e0-retail.asm),
[`3D4F0`](../analysis/damage-shields-goal-20261011/overlay/0003d4f0-retail.asm)

## Species and authored function binding

Species selection comes from actual object/model references. In the inspected
Lockout cache, `masterchief_mp` damage-info selects `E9F4086F` first-person and
`E9F70872` world; `elite_mp` selects `EA860901` and `EA870902`. Their overshield
datums are separate. This bounded cache census does not establish every map,
campaign species, transformation or first-person representation.
[Raw model reference census](../analysis/damage-shields-goal-20261011/overlay/lockout-model-shield-refs.json)

There are two uses of recent shield damage: `4BCC0` reads `object+F4` directly
for draw admission; model input `15000557` returns that same field through
`BCC20`. The actual shield shader overlay records consume authored exports:
Spartan uses `0A0012B4`; Elite uses `12001A0B`. Biped function records map
import `15000557` to those exports. Elite also contains the Spartan export.
All four inspected shield function curves are the same 20-byte type-zero
identity, with flags, turn-off input and scale input zero. Binding only the
literal built-in identifier at the shader overlay would miss these exports.
[Original BCC20 listing](../analysis/frontend/customization/000bcc20.asm),
[shader inputs](../analysis/damage-shields-goal-20261011/overlay/lockout-shield-inputs.json),
[biped functions](../analysis/damage-shields-goal-20261011/overlay/lockout-biped-functions.json),
[identity curve bytes](../analysis/damage-shields-goal-20261011/overlay/lockout-shield-function-curves.json)

Original callback `BE8B0` calls `BAB40` and returns zero if lookup is inactive.
`BAB40` searches definition function records by export (`+8`) and passes the
actual record in EAX to `BA8C0`. `BA8C0` resolves the import through `108D90`,
which iterates object-type input handlers. The scalar output pointer is in
EBX and active output pointer in EDI at that call. `BA8C0` applies the authored
curve through `13BB90`, recomputes activity for its implicit mapping mode,
then processes scale, threshold, always-active and turn-off gates before its
final clamp. The Ghidra export drops these hidden outputs and consequently
shows zero-valued intermediate expressions; that export is not a usable
replacement specification for the imported-input path.
[`BE8B0`](../analysis/damage-shields-goal-20261011/overlay/000be8b0-retail.asm),
[`BAB40`](../analysis/damage-shields-goal-20261011/overlay/000bab40-retail.asm),
[`BA8C0`, especially `BA93B..BA9BD`](../analysis/damage-shields-goal-20261011/overlay/000ba8c0-retail.asm),
[`108D90`](../analysis/damage-shields-goal-20261011/overlay/00108d90-retail.asm)

The retail ABI anchors for an original replay are:

| Routine | Inputs and outputs established from instructions |
| --- | --- |
| `BCC20` | Entry stack `+4 object`, `+8 input name`, `+C scalar out*`, `+10 active out*`; `RET 10h`; AL is recognized status, separate from active output |
| `108D90` | Entry stack `+4 object`, `+8 input name`; EBX scalar out*, EDI active out*; `RET 8`; AL recognized status from the object-type handler chain |
| `BA8C0` | EAX actual `20h`-byte function record; entry stack `+4 object`, `+8 scalar out*`; `RET 8`; AL activity |
| `BAB40` | Entry stack `+4 object`, `+8 exported input name`, `+C scalar out*`; `RET Ch`; AL activity |
| `BE8B0` | Entry stack `+4 object`, `+8 exported input name`; `RET 8`; scalar returned on x87 ST0 |

These are original retail addresses. TU1.1 and TU1.5 correspondence for the
overlay selector/collectors and authored function chain has **not** been
established by this research. Existing update correspondence for scalar
damage kernels cannot be extended to these owners by an assumed address
delta. Complete-body comparison or independently verified instruction
anchors, including hidden register arguments, are required before replaying
those versions.
[BCC20 instructions](../analysis/frontend/customization/000bcc20.asm),
[authored chain listings](../analysis/damage-shields-goal-20261011/overlay/)

The native reusable pieces already exist in
[shader_animation.cpp](../src/shader_animation.cpp):
`Cache::load_object_functions`, `evaluate_object_function`,
`shader_function_fraction/value/color`, and material constant animation.
The inspected object curves are type zero; shield shader curves use supported
types one/two, including transition/color records. This is source-level
format support, without a new untouched-original replay in this research.
`evaluate_object_function` currently treats a supplied `ObjectFunctionInput`
for a function's import as an immediate `{value,true}` override, bypassing
the curve/activity path. Identity values can coincide numerically while the
zero-input activity result differs. Its callback contract must distinguish
generic input-handler values from the original interpolation override before
claiming exact `BA8C0` behavior.
[Native implementation](../src/shader_animation.cpp),
[API](../include/halo2/shader_animation.hpp)

Current object-function loading/evaluation callers are the frontend and
netgame weapon loader. The inspected multiplayer animation loop supplies
weapon ammunition inputs and defaults other names to zero; it does not yet
bind each live biped's authored shield functions to its shield overlay.
Parent goal work now retains and validates recent-damage state separately. Neither
that state foundation nor existing general material overlays establishes this
renderer integration.
[Frontend binding](../src/frontend.cpp),
[weapon loader](../src/source_netgame_weapon.cpp),
[multiplayer animation owner](../src/multiplayer_runtime.cpp)

## Subsequent authored-input replay

The goal implementation now replays unchanged retail
`BAB40/108D90/BCC20/BA8C0/13BB90` against the actual resident Lockout
Spartan, multiplayer Spartan and multiplayer Elite function records. The132
cases compare scalar bits and activity for both authored exports, direct recent
shield input, missing exports, zero/signed-zero, fractional, epsilon and finite
out-of-range inputs. No object interpolation override is enabled. The native
comparison passes. The generic handler contract now carries recognized
value/activity separately from the existing interpolation callback, so an
inactive zero is no longer forced active on this tested input path.
[Replay](../tools/oracle_damage_feedback.py),
[original report](../analysis/damage-shields-goal-20261011/shield-function-inputs-original.json),
[native comparison](../tests/damage_feedback_tests.cpp),
[callback contract](../include/halo2/shader_animation.hpp),
[evaluation](../src/shader_animation.cpp).

This closes that finite, ordinary authored identity-mapping slice. Other
function flags/curves, interpolation overrides, update-version correspondence
for these owners, actual live overlay/pass binding and original pixels remain
separate gates. The retail static findings above retain their original scope.

## Evidence gates for implementation

1. Replay untouched `4BCC0` with visible/hidden/parent-hidden, forced world,
   zero/nonzero mode, recent damage zero/positive/unordered, missing model,
   absent damage-info, NONE shader and first-person model-count cases. Compare
   selected flags/datums/scalars, including the separate overshield branch.
2. Replay `BAB40/BA8C0/108D90` with the actual Spartan/Elite records and curves;
   compare value **and active**, especially zero, fractional, one, unknown
   export and original interpolation override paths. Keep actual aliases.
3. Prove the selected shader reaches every admitted current posed subpart and
   correct per-view matrix bank. Cover world/first-person, both species,
   base materials with override rejection masks, and collector budgets.
4. Reproduce chosen template LOD/pass and the world/first-person scheduling
   paths. Trace the `+77` vitality extern and authored animated constants into
   the actual plasma-shield implementation; preserve bitmap, vertex/pixel
   constants, render states, sampling, depth and blend semantics from sources.
5. Capture original and native frames for ordinary hit, repeated hits,
   depletion, recharge and overshield, from first-person and world views.
   Fixture equality is a foundation; pixel/frame comparison and live state
   transitions remain required before calling shield visuals complete.

This research changed no implementation source files. Its evidence is static
original instructions plus raw original-cache reads. No new dynamic fixture,
live match, original framebuffer or native GPU capture was executed here.
