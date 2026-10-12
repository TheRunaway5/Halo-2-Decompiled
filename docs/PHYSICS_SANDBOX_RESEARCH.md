# Retail physics and sandbox recovery

Research date: 2026-10-07. This note describes instruction-level candidates for
the user's original Xbox build `02.09.27.09809`. It does **not** certify complete
physics or sandbox parity. A velocity kernel with supplied contacts cannot
establish contact generation, object filtering, Havok constraint solving,
update order, or campaign behavior.

## Authority and reproducibility

The executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`;
the hash was rechecked for this investigation. Its original `.text` hash is
`70c5fbef9836295e4895149eba14c961da5f9fe6168c39c2a0f28315b2910d7b`.
The local exports are `analysis/retail/functions.tsv`, `all.c`, and
`provenance.json`. Ghidra C is navigation evidence; executable instructions
and a bounded instruction oracle are behavioral authority.

The primary source reconstructions and their local copies are:

* [CC0 Xbox recovery](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27),
  commit `5f08630655e3ddc9ac45909090f11ae799fa0c27`, at
  `tools/reference/halo2-decompiled`. The function manifest records `todo`
  separately from `matched`; a `@retail` annotation alone is insufficient.
* [CC0 native recovery](https://github.com/BirchWoodGod/halo2-decomp/tree/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888), local
  commit `080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888`, at
  `tools/reference/halo2-native-reference`. Its README explicitly describes
  pool/configuration/codecs recovery, not Havok simulation or a runnable game.
  The local checked-out commit is the relevant pin.
* [Assembly tag layouts](https://github.com/XboxChaos/Assembly/tree/fd14ef3c46135e9422966c6a64cd926fb0bd23fb),
  commit `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`, local
  `tools/reference/assembly/src/Assembly/Plugins/Halo2/bipd.xml` and `matg.xml`.
  Layout facts are used; implementation bodies are not copied from GPL source.

Generated instruction listings for this pass are in
`analysis/systems/{1f0870,2b400,30bf0,a75d0}-research-disassembly.txt`.
Recreate them with the pinned local disassembler, for example:

```sh
RETAIL_XBE="$PWD/assets/disc/default.xbe" .tools/python/bin/python \
  tools/reference/halo2-decompiled/tools/disasm.py 1f0870
```

## Floating-point state is part of the physics contract

The game sets its x87 precision before each game tick. Original instructions
`00137FEA..00137FF4` call CRT `__control87` at `003212D6` with value `0009001F`
and mask `000FFFFF`. Startup `0001202E..00012038`, lifecycle
`00137C64..00137C7C`, and `00137CA1..00137CAB` set the same state. CRT conversion
`003211FE..0032128B` maps precision flag `00010000` to x87 precision-control
bits `0200`: **53-bit double precision**, round to nearest/even, rather than
the 64-bit significand of default extended precision. Running the unmodified
CRT routine in Unicorn with initial control words `037F`, `027F`, and `007F`
produced `023F` in all three cases. Reserved bit 6 accounts for the usual
`027F` spelling; both have the relevant precision/rounding bits. This is a
small controlled probe of the original setup, not a full engine boot.

Consequently, an oracle that simply uses Unicorn's default `037F` tests a
different arithmetic condition. Ordinary native x86-64 `long double` with its
default 64-bit significand is also a different condition. Recovered x87
intermediates need the configured 53-bit behavior; native double operations
with explicit float stores at the original boundaries are an actionable
implementation strategy. SSE scalar instructions round each result to float;
contraction into FMA changes those boundaries. Sources: the above original
addresses, the CRT disassembly, and `analysis/retail/functions.tsv` entries for
`__control87`/`__controlfp`.

## Biped movement inputs and velocity

| Routine | Manifest status | What it establishes | What it does not establish |
|---|---|---|---|
| `000DD7D0` | `todo` | Crouch delta; already has a local instruction differential harness | Whole crouch collision/camera update |
| `000DF5F0` | `todo` | Interpolated capsule query with radius/scale | Shape construction, contact solver, ceiling admission |
| `001E5BB0` | `todo` | Common character-physics state and animation movement input | Animation sampling/movement-rate semantics when callbacks are supplied |
| `001E6120` | `todo` | Player movement target, crouch blend, ground/air acceleration input | World velocity or contact integration |
| `001F0870` | `todo` | Desired velocity delta, slope scaling, acceleration, gravity, damping and history cap | Contact discovery, stepping, supporting rigid bodies |
| `000E4680`, `000E4770` | `todo` | Jump gating and upward kick relative to support velocity | Authored actor refusal, effects and animation transition without callees |

Primary reconstruction paths: `src/bipeds.cpp:205,1087,1436,3610,3649`,
`src/unknown_1e67a0.cpp:84,442`, and `src/unknown_1f03e0.cpp:468`, under the
pinned CC0 recovery. The statuses above come from `config/functions.csv`;
the local retail export confirms the addresses exist.

`000DC5C0` supplies `bipd +264` as the definition pointer to `001E5BB0`.
`001E6120` uses the movement settings at `matg +134`; its desired local
forward/backward speed, sideways speed and acceleration blend run/sneak values
by crouch fraction. It normalizes input if full `x²+y²+z² > 1`, but scales
only x and y. It selects backward speed for x equal to zero. Its limited mode
allows only positive forward input. The acceleration at output `+148` comes
from the settings' air-acceleration field `+4C`.

Two distinct airborne conditions must stay distinct. `001E6120` suppresses
player tuning when the current animation-state StringID is `0E0000C2`.
`001E5BB0` sets output flag `4` from `000E4050`, a sustained airborne test:
unsupported flag `biped+348` bit 0, non-sleeping, signed ticks at `+399` at
least round(ticks/sec × .18), walking/dead physics mode, and no parent.
The recovered parameter name `grounded` is misleading. `001E5BB0` initializes
velocity to zero, ground acceleration to `4.8`, and air acceleration to zero;
valid movement animation can replace the velocity before `001E6120` runs.
`001E55D0` dispatches walking directly through `001F0450` to `001F0870` without
replacing these target/acceleration fields. Sources: `bipeds.cpp:1087`,
`unknown_0e4050.cpp:32`, `unknown_1e67a0.cpp:84,442`, and original
`001E568F..001E5695`.

`001F0870` has a custom ABI: ESI = input state, EDI = output, stack
`[entry ESP+4]` = history, `[entry ESP+8]` = current velocity; it returns `ret 8`.
Its state fields are:

| Offset | Meaning supplied to the velocity kernel |
|---|---|
| `08` | Pointer to `bipd+264` settings |
| `14` | Opaque update flags: bit 0 enables acceleration branch; bit 2 selects air branch; bit 3 is gravity flag; bit 4 enables xy damping |
| `20` | Movement speed scale |
| `8C` / `98` | Supplied contact normal / damping fraction |
| `9C` / `100` / `118` | Facing / up / alternate basis |
| `A8` | Suppresses the fixed normal push |
| `124` | Supplied velocity offset for history cap |
| `130` | Gravity magnitude |
| `138` | Local desired velocity |
| `144` / `148` | Ground / air acceleration |

Without flag 0 it transforms local velocity by facing, left and up, ignoring
acceleration and clearing gravity. With flag 2 it limits **horizontal** delta
to air acceleration × dt, then computes `current.z - gravity × dt`; this z
subtraction happens even if the separate gravity flag is clear. Ordinary
walking ground movement projects desired velocity onto the supplied contact
plane, normalizes it, applies authored slope speed factors, then limits the
**three-dimensional** delta to ground acceleration × dt. If the delta already
fits the cap, it clears gravity. A branch named `flying` in the reconstruction
actually tests settings flag bit 3, whose layout name is **Climb Any Surface**;
the separate biped flying bit is 4. That branch requires arbitrary contact
bases and is distinct from ordinary walking.

The actual XBE retains the reconstruction's apparently unreachable downhill
interpolation: `001F1356..001F1367` tests `settings[58] >= desired.z`, followed
at `001F1379..001F1381` by the same operand with `settings[58] > desired.z`.
For finite inputs, the second interpolation is unreachable. Preserve it in
an instruction-faithful port; replacing the first comparison with cutoff `5C`
would change the original behavior. Slope offsets relative to the complete
biped definition are `2B0/2B4` downhill/uphill scales and `2BC/2C0/2C4/2C8`
cached sine bounds. A parser using them needs at least `2CC` root bytes.
Sources: the saved `1f0870` instruction listing and local `bipd.xml:603..615`.

After acceleration limiting, ground mode can subtract contact normal ×
`0.23399999737739563` (bits `3E6F9DB2`, constant VA `00445628`) unless `A8`
is nonzero or the physics gate has not expired. It then damps xy by
`1-damping`, conditionally subtracts gravity × dt from z, and adds current
velocity. Gate `physics_globals+18` is written by builtin 129's evaluator
`002A2BE0`: clamp seconds to `[0,5]`, round-even ticks/sec × seconds, add
current tick. Physics reset initializes it to zero. Source:
`unknown_29f5b0.cpp:2879` and `001F0E92..001F0F58`.

Normalization and the history cap must retain their numeric boundaries:

* `0002B400` / `00030BF0` use x87 sums and sqrt, retain sqrt precision for
  epsilon comparison, store length to float, then use SSE float reciprocal
  and component multiplies. Epsilon is `9.999999747378752e-5`, bits
  `38D1B717`, VA `0045DBDC`. Below epsilon they leave the vector unchanged and
  return zero. Their accumulation order is x² + y², then z² where applicable.
* `000A75D0` forms squared length in SSE float in x/y/z order and compares
  against float maximum². If capped, x87 computes maximum/sqrt(squared) and
  multiplies components before storing each to float; rounding a scale to
  float first changes results.
* The history cap is used only when settings' player-physics bit 2 is clear
  and the update is not airborne. Consecutive previous tick and non-NONE
  history produce maximum `length(input - old_input + carrier_velocity) + .5`.
  History input/tick writes occur even when no cap is applied.

## Jump, collision, projectiles and damage candidates

The retail jump is not simply assignment of positive z speed on a Space-key
edge. `000E4680` accepts held jump and requires elapsed grounded timing greater
than `.16` seconds. Its player held-input gate is **held byte `+39E` less than**
round(ticks/sec × `.3`), rather than the opposite condition in the recovered
C. Original `000E46EF..000E46FA` sign-extends `+39E`, compares it to the delay,
and branches to the finish on `>=`. After a successful kick it sets `+39E` to
the delay and increments it, blocking repeated jumps until release clears
the byte. Both timers use signed-byte comparisons. `000E4770`
projects current velocity minus support velocity onto biped up and only adds
the deficit to authored jump speed. A tired player scales jump by
`1 - matg_movement[7C] * biped[2E4]`. Actor jump admission is an external call;
support velocity, up vector, actor refusal and jump effects remain required.
Sources: pinned `bipeds.cpp:3610..3708`.

The gating correction above comes from the user's actual instructions, saved
in `analysis/systems/e4680-research-disassembly.txt`; this is a concrete reason
the upstream `todo` reconstruction cannot be treated as finished retail code.

The ray API at `001697C0` is still a stub returning false in the CC0 reference's
`src/stubs/lane_c.cpp:84`; it cannot supply collision parity. Projectile
`000FC030` first casts a center line with mask `2480000F`, owner and projectile
ignored. A radius at least epsilon and valid owner enables two parallel side
lines with mask `04800008`, ignoring the owner only; the side direction derives
from motion crossed with world up with a fallback basis. Damage obstruction
`000D6F90` uses masks `04A08C2D` and `14A08C2D`; biped eye obstruction uses
`04808C2D`. A single two-sided triangle ray mode loses these distinctions.
The named bit semantics of every mask were not recovered here. Sources:
`projectiles.cpp:1017`, `damage.cpp:3204`, and `bipeds.cpp:1018`.

`000F8EB0` applies projectile gravity with trapezoidal displacement:
`new_velocity.z -= gravity * remaining`, while average_velocity.z subtracts
half that fall. Displacement using the fully updated velocity instead is a
material difference. Retail range clipping uses pre-gravity average speed,
then applies the resulting time fraction to average velocity. Guidance,
air/water gravity, speed curve, impact response and up to ten collision
iterations live in the same routine. Porting only the no-hit gravity portion
does not establish these other branches. Sources: `projectiles.cpp:1829..2060`
and retail entry `000F8EB0`.

Additional bounded candidates for instruction differentials:

| Entry | Manifest status | Candidate coverage / boundary |
|---|---|---|
| `000FA170` | `matched` | Projectile gravity scale × global gravity, negated; synthetic tag table |
| `000FA580` | `todo` | Linear aim direction/distance/time and epsilon/fallback; synthetic vectors |
| `000FA1A0` | `todo` | Ballistic aiming discriminant and minimum/fixed-speed choices; sqrt/numeric ordering |
| `000D5BC0` | `matched` | Ordered product of both damage-group × both armor combinations; include missing and duplicate keys and original search routine |
| `000D8B50` | `matched` | Recent-damage timer/decay, NONE and saturating signed timer, round-even delay; synthetic damage/time records |
| `000D9110` | `todo` | Normalized shield damage and overflow back to body; controlled maximum-shield/material lookups and explicit effect callbacks |
| `000D5DE0` | `todo` | Recharge and stun update plus flags; ordinary branch alone does not cover overshields/death/regions |

Primary paths: pinned `src/projectiles.cpp`, `src/damage.cpp`, and
`config/functions.csv`. Damage effect armor multipliers, difficulty/team
scaling, regional damage/headshots, shield-only/radius behavior, impulses,
owner exceptions and death side effects must be exercised through the full
damage pipeline before making sandbox parity claims.

## Ordinary recharge oracle boundary

Original `000D5DE0` takes object index on `[entry ESP+4]`, returns changed in
AL, and returns `ret 4`. It preserves the ordinary shield/body distinctions:

* A living object with maximum shield vitality `>0` and shield fraction below
  the maximum enters the ordinary shield path. There is **no positive-rate
  guard**. Zero or negative rate still sets shield-recharging, clears depleted
  if necessary, starts the recharge effect and adds `dt × rate × team scale`.
* Nonzero shield stun decrements only when the mode-freeze predicate is false
  and stun is not `7FFF`. Negative signed-short stuns also decrement.
* Body recharge requires positive maximum body vitality, living body, current
  fraction below the authored recharge maximum, **and rate `>0`**. Its stun
  decrements under the mode predicate without the shield's `7FFF` exclusion.
  Thus body stun `32767` becomes `32766`, while shield stun `32767` stays put.
* Clamp tests use `>` after addition. Exact equality leaves the corresponding
  recharging flag set and changed true for that call; an overshoot clamps,
  clears recharging, and does not by itself set changed true.

The predicate at original `00138880` simply tests game globals byte `+0C ==4`.
It is the mode-4 predicate, not independently verified general pause behavior.
Rates come from damage info `+D0/+D4`, body recharge maximum from `+38`; an
actor character bounds record overrides them with `+6C/+68/+14`. Team factor
`001E9720` is called for biped/vehicle types with EAX=3 and ECX=team. Source:
`damage.cpp:3443..3704` and
`analysis/systems/d5de0-research-disassembly.txt`, especially
`000D6256..000D64B5`.

A narrow differential can execute the **whole original routine** with its
original tag lookup and timer functions, rather than replacing the routine's
recharge instructions. Supply synthetic pools and these fields:

| Fixture | Fields |
|---|---|
| Object pool | `g_4E0300` points to pool; pool `+44` points to 12-byte headers; header `+8` points to object |
| Tag/model/info | `g_4E3B44` points to 16-byte tag records; object tag points to definition; definition `+38` is model datum; model `+60=1`, `+64` points to damage info |
| Object | `+12C`, `+13C`, `+248` and `+D4` are NONE; `+120=0` region bytes; `+108/+109=FF` recent-damage timers; scalar fractions/maxima at `E4/E8/EC/F0`, stuns `104/106`, flags `10A` |
| Info/globals | Info `+BC=0` regions; game state `+8=1`; game mode `+0C` supplied as 0/4; `g_4F55E4=0`; time `+2` ticks/sec and `+4` dt |

`000D5B60` (ECX object, EAX info, plain ret), `000A7A30`, `000D66D0`,
`000D8B50`, and `00138880` then run unchanged. Controlled callbacks needed
for an ordinary biped fixture are:

| Boundary | ABI / effect represented |
|---|---|
| `001E9720` | EAX kind=3, ECX team; supplied factor returned in XMM0; plain ret |
| `00176780` | EDX object, EBX owner; four stack arguments; EAX result, ret 16; record recharge-effect request without creating host effects |
| `000BA7F0` | Four stack arguments, ret 16; record depleted-to-recharging transition |

Eight original-routine probes under x87 `027F` and MXCSR `1F80` independently
confirmed zero/negative shield rate, body/shield sentinel distinction, negative
stuns, mode freeze, body positive-rate gate, and transition callback order.
For zero shield rate and body rate 1 at dt `1/30`, initial fractions `.25`
became body `.28333333134651184`, shield `.25`, flags `0300`; with shield rate
`-1`, shield became `.21666666865348816`. A depleted shield calls `BA7F0`
before `176780`. These controlled probes establish those branches only;
overshields, multiplayer drain, death/regions, real character lookup, effects
and difficulty semantics remain outside this ordinary fixture.

## Acceptance boundary for the requested full sandbox

Scalar differentials should compare float bits, flags, history writes and
callback order, with the original game's floating-point state and unchanged
retail instructions. Controlled external callbacks must be named in the
report. Composed movement traces then need authored floor/wall/slope/stair/
ledge/ceiling scenes, support motion and all biped modes; native-only synthetic
tests cannot establish source parity. Full physics also includes moving
scenery/devices, vehicles, ragdolls, objects, collision groups, friction,
restitution, constraints, stepping and breakables.

Sandbox completion additionally requires every weapon trigger/barrel mode,
magazine, charge/heat, spread/error RNG, firing/reload animation timing,
dual wielding, melee, grenade/rocket splash, projectile bounce/penetration/
tracking, pickups, shields/body damage, difficulty, effects and sound selection.
These are unimplemented or provisional in the present host according to
`docs/NATIVE_SYSTEMS.md:62..65`; none becomes exact merely by implementing a
movement kernel. Campaign acceptance follows these prerequisites and requires
authored encounter/script paths, saves/reverts and BSP transitions described in
`docs/CAMPAIGN_RESEARCH.md`, rather than independently placed actors.

## Serialized slope fields and world gravity follow-up

Direct read-only inspection of the 30 local version-8 caches found 242 resident
`bipd` records, with 43 distinct tag names and seven distinct bit patterns for
`2BC/2C0/2C4/2C8`. These cached sine fields are present in the files; they do not
need host-side trigonometric reconstruction. Twelve records, belonging to
infection forms, monitors and the Regret hologram, have signed negative zero
for the downhill fields and positive zero for the uphill fields. The remaining
230 records contain nonzero bounds. Preserve their float bits, including the
downhill negative signs. The scanner validated the full `2CC` root extent for
each resident record. This establishes serialized availability, not correct
contact normals or slope admission.

Example: Cairo Station `assets/disc/maps/01b_spacestation.map`, SHA-256
`5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0`, Chief tag
`FB66192E`, root file offset `EA439F4`:

| Field | Serialized bits | Float value |
|---|---|---|
| `264` physics flags | `00000004` | Use player physics |
| `2B0` downhill scale | `3FC00000` | 1.5 |
| `2B4` uphill scale | `3F000000` | 0.5 |
| `2BC` downhill falloff sine | `BF3504F3` | -0.7071067690849304 |
| `2C0` downhill cutoff sine | `BF441B7D` | -0.7660444378852844 |
| `2C4` uphill falloff sine | `3F3504F3` | 0.7071067690849304 |
| `2C8` uphill cutoff sine | `3F441B7D` | 0.7660444378852844 |

The scan artifact is `analysis/systems/physics-cache-fields.json`; it uses the
repo's `tools/audit_rendering.py::Cache` pointer/bounds validation and reads the
original local bytes. Field interpretation is supported by pinned
`bipd.xml`; the table values are direct cache evidence.

The similarly labeled `matg` Player Control block at `F0`, stride `80`, field
`68` is **zero (`00000000`) in all 30 caches**, including Cairo's control
record at file offset `E5E8BE4`. The Assembly label “Gravity Scale” does not
establish that this field should multiply native world gravity. The original
movement builder supplies the first float of runtime global `g_51E9C4`, and
`1E6120` does not apply that `matg` field. Applying it unconditionally would
remove gravity from every inspected cache.

The original runtime-global initialization is explicit:

* `001EB584..001EB592` loads VA `004455D8` and writes the gravity float.
* `001EB5F5..001EB603` restores the same float on lifecycle reset.
* Script reset `002A2AD5..002A2AE3` also restores it.
* Script gravity setter `002A2B59..002A2B6B` uses SSE `MULSS` to multiply its
  supplied real argument by that constant and writes the runtime gravity.

VA `004455D8` is float bits `40857AF5`, value `4.17125940322876`. Sources are
the original instructions and the pinned `src/unknown_1eb550.cpp` and
`src/unknown_29f5b0.cpp:2832..2859`. The recovered global writer search found
initialization, lifecycle/script reset and the script setter; it did not find a
Player Control `+68` multiplier. The observed default should therefore be this
runtime constant. The setter's public builtin index is 127, and its evaluator
ABI is `(short function_index, long thread_index, bool initialize)`, stack
parameters, `RET 12`. The script argument query/return callbacks are external
to this scalar setter boundary.

## Unaccelerated projectile endpoint and distance oracle

The entire `000F8EB0` entry accepts projectile index at entry `ESP+4` and has
`RET 8`; its second stack parameter and broad state machine are outside this
fragment's acceptance boundary. A narrower **unchanged-instruction** fragment
can start at `000F909A` and stop at `000F981A`, before collision/action dispatch.
It includes initial speed, air/water gravity selection, the half-step average,
range clipping, and endpoint construction. Supply:

| Input location | Fixture value / meaning |
|---|---|
| EBP | Projectile object address |
| ESP | Mapped writable scratch base, at least `130` bytes |
| XMM3 | Zero |
| x87 / SSE state | PC53 nearest-even / normal nearest-even SSE |
| scratch `20` | Remaining dt float |
| scratch `3C` | Projectile definition pointer; fragment reloads EBX |
| scratch `44` | Collision-count word, zero |
| scratch `12C` | Projectile object datum |
| object `64/68/6C` | Initial position |
| object `88/8C/90` | Initial velocity; fragment initializes ESI itself |
| object `144` | `FFFFFFFF` to bypass guided-target queries |
| object `12C` | Include bit `100` to bypass accelerated-speed branch |
| object `4` | Bit 3 selects water gravity scale |
| object `168` | Previously traveled distance |
| object `130` | Existing signed action word, normally zero |
| definition `164/170` | Air/water gravity scale |
| definition `E0` | Maximum range, zero means unbounded |
| `g_51E9C4` | Pointer to the runtime world-gravity float |
| `g_4E0300` / pool `44` | Object pool and header-array pointer for action setter |
| header `8` | Projectile object pointer for datum zero fixture |

At the stop, new velocity is scratch `10/14/18`, endpoint is `54/58/5C`, and
the action word may have been raised to 1. Scratch `80` is the float-rounded
initial speed. No hooks are necessary: range clipping's original `000FAA30`
uses EAX=object datum, ECX=proposed action, plain `RET`, preserves SSE values,
and raises the existing signed action only when the new action is greater.

Initial norm (`000F909A..000F911D`) and helper `00024590` both compute
`sqrt((x*x + y*y) + z*z)` with x87 53-bit operations. The inline initial norm
is explicitly stored to float at `F9116/F911D` **before** range clipping. Each
following gravity, half-step, division, multiply and endpoint add is SSE float:

```
gravity = float(definition_scale * world_gravity)
fall = float(gravity * dt)
new_z = float(old_z - fall)
average_z = float(old_z - float(fall * 0.5))
range_exhausted = maximum != 0 && float(traveled + float(speed * dt)) > maximum
fraction = range_exhausted ? (speed != 0 ?
    float(float(maximum - traveled) / float(speed * dt)) : 0) : 1
step = float(fraction * dt)
endpoint = position + average_velocity * step // each component rounded twice
```

Equality at the maximum does not exhaust range. If the fixture already exceeds
range, the original permits a negative fraction; silently clamping it changes
behavior. The complete routine also handles degenerate remaining times and
actions, so physically admissible fixture cases should be identified separately
from raw arithmetic edge probes.

Committed traveled distance is **not** `speed * step`. After a no-hit endpoint
is supplied by collision, original `000F99B4..000F9A24` subtracts initial
position in SSE, calls `24590` for the actual displacement norm, adds old
traveled in x87 without first rounding the norm to float, then stores float to
object `168`. To oracle this second fragment, retain EBP/ESP, set XMM2/XMM1/XMM0
to endpoint x/y/z, enter `000F99B4`, and stop at `000F9A2A`. This fragment has no
external hooks. Zero initial position compares displacement directly; nonzero
positions additionally exercise endpoint/subtraction rounding. The broader
routine's flag and collision gates are deliberately bypassed by this entry.

The research fixture executed 1,000 randomized air/water cases, including zero
velocity, against these ordered arithmetic formulas, with zero mismatches for
both unchanged-instruction fragments. Evidence is
`analysis/systems/ballistic-fragment-research.json`. This is a boundary and
formula check, **not a native-library differential**, full projectile simulation,
collision, impact-response, guidance, speed-curve or entire sandbox proof.
