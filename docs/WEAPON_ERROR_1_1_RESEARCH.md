# Original disc 1.1 weapon error: state, angles, and invocation

Evidence collected 2026-10-07 from the unchanged Map Pack update executable and
unchanged Lockout cache. This establishes bounded original behavior and matching
native scalar kernels. It does **not** establish a newly changed 1.1 balance
multiplier, full trigger behavior, or live multiplayer parity.

## Authority and validation

The instruction authority is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The authored SMG values come from `assets/disc/maps/lockout.map`, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
Original x86 instructions are executed without instruction patches or replaced
callees. Supplied fixtures provide object/tag pools, hand slots and timers, clock,
barrel state, branch entry state, forward direction and RNG state.

`tools/oracle_weapon_error_1_1.py` generated **70** original-instruction cases in
`analysis/multiplayer/map-pack/weapon-error-1.1-oracle.json`. Its SHA-256 is
`2e21bf12b70caafacaf7b5fcb69a03e912413b41e23b42ecf20fd521ea06d579`.
The cases include single/dual weapon slots, either hand, positive/expired/partial
opposite-hand timers, an unheld weapon, and explicitly modified boundary fixtures.
The latter set definition bit 27 or replace rate/angle fields; they are identified
by `overridden_weapon_bit27` and `barrel_field_overrides` in every row and **are not
authored tuning values**. Five initial error/angle-change states include zero
error with positive angle-change state, recovery, and saturation boundaries.

`tools/differential_weapon_error.py` compares the native functions in
`src/weapon_error.cpp` with these original outputs. The saved report
`analysis/multiplayer/map-pack/weapon-error-1.1-differential.json` records:

- all 70 firing/idle state cases and angle cases matched float bits;
- all 70 corresponding original shot direction/RNG outputs matched;
- **13,098 / 13,098** additional complete `147A90` cases matched direction bits and
  final seed: all 1,026 table directions used as parallel/degenerate inputs, 72
  scaled/zero-angle boundaries, and 12,000 varied normalized directions, seeds and
  angle bounds.

The supplied CPU controls are x87 `FPCW=0x027F` (53-bit precision, nearest-even)
and `MXCSR=0x1F80`. These are explicit oracle conditions; this research has not
proved the full original startup CPU-control dispatch. Inputs are finite.
The native kernels were not yet connected to live firing when this report was
written. Matching scalar results alone does not prove trigger scheduling,
projectile creation, multiplayer RNG coordination, or live gameplay.

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_weapon_error_1_1.py --output /tmp/weapon-error-original.json
.tools/python/bin/python tools/differential_weapon_error.py --library build-multiplayer/libhalo2_weapon_kernels.so --fixtures /tmp/weapon-error-original.json
```

The differential pins the original executable/cache and records fixture and
native library hashes. Regenerating a fixture report is unnecessary unless the
explicit fixture set changes.

The offline C++ fixture test also passes in the normal and ASan/UBSan builds.
It consumes `tests/fixtures/retail_weapon_error.tsv`, preserving each original
float output bit and seed. After the instruction differential passes, export
that snapshot with `--export-fixtures tests/fixtures/retail_weapon_error.tsv`.
The normal test/build logs are `analysis/multiplayer/map-pack/weapon-kernel-*.log`.

## Original state and field addresses

A weapon's barrel state begins at `weapon + 0x1A4 + index * 0x34`.
The weapon definition's barrel block has count at `+0x2D0`, pointer at `+0x2D4`,
and stride `0xEC`. Here `s` denotes barrel state `+0x1C` (accumulated error), and
`a` denotes state `+0x20` (called `recoil` in the native scalar interface).
Names alone are not evidence that `a` implements camera recoil.

| Role | Base entry | Disc 1.1 entry or segment |
| --- | --- | --- |
| Opposite-hand error blend | `D1490` | `D2940`, complete |
| Both weapon slots / queried-weapon eligibility | `101690` | `105840`, complete, actual `CEAB0` / `CD1A0` callees |
| Per-weapon/barrel update | `FDFB0` | `1021D0`, complete control-flow listing |
| Fired error accumulation | formerly inline in `FDFB0` | `102090`, complete helper |
| Idle blend setup | caller prefix | `1021D0..10227C` |
| Idle error recovery | caller block | `10363E..1036FD` |
| Hand timer setter | `EE680` | `EFCC0` |
| Hand timer decay | `C58F0` | `C6C50` |
| Shot-angle selection | not established here | segment `108F6E..10905C` within `108300` |
| Random direction rotation | helper entry not established here; table `4417F0` | `147A90`, end-exclusive `147CB2`, complete, no direct callees |
| Direct shot-application path | `A2BA0` | `A38F0`; added `102090` invocation before projectile dispatch |

Extracting a formerly inline calculation into `102090` is not itself evidence of
a new balance adjustment. The ordinary update retains the same accumulation
expressions. Separately, the direct shot-application path has a verified new
invocation of that calculation, described below.

## Opposite-hand blend and dual-slot query

`D2940` receives a unit index in EAX and weapon index in ESI; it returns XMM0.
If either supplied index is `NONE` (`0xFFFFFFFF`), its result is **1.0**. Otherwise
it resolves the unit's primary weapon through slot byte `unit+0x212` and the weapon
array at `unit+0x218`. If the queried weapon equals that primary weapon, the timer
is the unsigned byte at **unit+0x1FB**. For the other weapon, the timer is
**unit+0x1FA**. A zero timer returns zero. For a nonzero timer:

```text
b = clamp(float32(timer) * clock_dt * float32(10/3), 0, 1)
```

Multiplications are ordered scalar SSE operations. `clock_dt` is the float at
`[DAT_00518444]+4`; the exact multiplier at `004653C4` is bits `40555555`,
`3.3333332538604736`. This is the **opposite hand's timer**, not elapsed time
holding this weapon's trigger. The authored SMG's weapon-definition flags are
`0x00420000`; bit 27 is clear.

The firing helper and caller prefix also execute `105840`. That function requires
a parent, parent type mask `3`, both slots present through `CEAB0`, and membership
of the queried weapon in the primary/secondary slots through `CD1A0`. If weapon
**definition** `+0x12C` bit 27 is set, the effective blend becomes 1 when this query
is true, otherwise 0. If bit 27 is clear, the effective blend is `D2940`'s result.
The oracle runs these original queries and callees rather than replacing them
with a fixture boolean.

`EFCC0`'s state-switch cases 1, 2, 3 and 4 assign one hand timer. For ESI=0 it
writes **+0x1FA** at `EFDAB`; otherwise it writes **+0x1FB** at `EFDEC`. It converts
clock tick rate (signed short at `clock+2`) to float, multiplies by the original
float at `00452000` (bits `3E99999A`, approximately 0.3), stores to float, and uses
x87 `FISTP` before retaining the low byte. At the supplied 30 Hz clock this is 9.
The exact higher-level switch/event semantics have not been fully validated here.

`C6C50` decrements each positive unsigned byte by one at `C6CE4` and `C6CFB`.
The timer-decay block is skipped when unit definition `+0xBC` bit 11 is set.
The relative scheduling of this unit update, first-person state callback and
weapon update remains unresolved; the oracle supplies timers explicitly.

## Fired and idle error calculations

For finite inputs, `102090` performs these ordered operations:

```text
a = min(1, a + barrel[C4] * dt)
s = min(1, s + ((1 - b) * barrel[DC] + barrel[50] * b) * dt)
```

Products, additions, and float stores use the instruction order in
`tu-weapon-barrel-update-fired.asm`. The `a` upper clamp compares a float value
with double 1.0 at `00465130`; `s` uses scalar SSE comparison against float 1.0.
There is no lower clamp in this fired helper.

For the idle branch, if `s <= 0`, **both `s` and `a` remain unchanged**. If `s > 0`:

```text
a = max(0, a - barrel[C8] * dt)
s = max(0, s - ((1 - b) * barrel[E0] + barrel[54] * b) * dt)
```

The `a` lower clamp uses double zero at `00465050`; `s` uses scalar SSE zero.
There is no upper clamp in the idle block. In particular, positive `a` with zero
`s` does not recover in this path. The oracle's idle execution first runs the
original blend prefix, then enters the original idle block with its per-barrel
marker explicitly false; it does not execute the full preceding trigger logic.

## Shot-angle selection and authored SMG values

`108300`'s segment `108F6E..10905C` computes the maximum angle only when its caller
local `ESP+0x54` is zero; it can reuse that result during the projectile loop.
Let normal range endpoints be `barrel[74]`, `barrel[78]` and dual endpoints be
`barrel[5C]`, `barrel[60]`. Each zero dual endpoint independently falls back to its
corresponding normal endpoint. The minimum at `barrel[58]` has **no such fallback**.

```text
remaining = 1 - b
low = normal_low * remaining + dual_low_or_fallback * b
high = normal_high * remaining + dual_high_or_fallback * b
maximum = (high - low) * s + low
minimum = remaining * barrel[70] + barrel[58] * b
```

The finite-value formulas preserve scalar SSE operation order. The original
conditional sequence handles the endpoint comparison; the fixtures cover both
and single zero endpoints. If barrel flags bit 5 and weapon byte `+0x16E` bit 6
are both set, this path bypasses random rotation. The oracle exercises the normal
rotation branch; the complete behavior behind those flags is not claimed here.

Authored Lockout SMG datum is `E57B0406`, root file offset `31365092`, first barrel
file offset `31366192`. Its field values, before unresolved title-update tag
postprocessing, are:

| Barrel field | Role in these instructions | Exact float bits | Value |
| --- | --- | --- | --- |
| `50` | dual error acceleration | `3FAAAAAB` | 1.3333333730697632 |
| `54` | dual error deceleration | `40555555` | 3.3333332538604736 |
| `58` | dual minimum angle | `3C0EFA35` | 0.008726646192371845 radians |
| `5C` | dual low angle | `3CD67750` | 0.02617993950843811 radians |
| `60` | dual high angle | `3D567750` | 0.05235987901687622 radians |
| `70` | normal minimum angle | `3C8EFA35` | 0.01745329238474369 radians |
| `74` | normal low angle | `3C8EFA35` | 0.01745329238474369 radians |
| `78` | normal high angle | `3D449809` | 0.047996554523706436 radians |
| `C4` | state `+20` acceleration | `3F68BA2E` | 0.9090908765792847 |
| `C8` | state `+20` deceleration | `40555555` | 3.3333332538604736 |
| `DC` | normal error acceleration | `3F800000` | 1.0 |
| `E0` | normal error deceleration | `40A00000` | 5.0 |

## Complete `147A90` random direction routine

Arguments are output pointer EAX, input pointer ECX, RNG-state pointer EDI, then
minimum/maximum angles on the stack; it returns with `RET 8`. The actual shot
caller aliases input/output. It has **no direct callees**.

1. Copy the input vector to the output. Update 32-bit RNG state by
   `seed = seed * 0x19660D + 0x3C6EF35F (mod 2^32)`.
2. Select `index = ((seed >> 16) * 1026) >> 16` from the original table at
   **`00447E40`**, 1,026 three-float vectors, stride 12. Its file offset is
   `4473568`, length `12312`, SHA-256
   `9581ebb544c806b2c88cdc2a094df6a0ac07978c9bb07dcb9f68fe2962fd2feb`.
3. Compute `input × selected_vector` with x87 intermediates and float stores.
   The final Z component uses `FST`, retaining the unrounded intermediate while
   storing a rounded float. Its square therefore multiplies the retained Z by
   stored Z; X and Y squares use stored float values. Add in the order Z-square,
   Y-square, X-square, then `FSQRT`.
4. Store the norm as float but compare the retained absolute x87 norm against
   original float `1e-4`. If smaller, return the copied input after **one** seed
   update; there is no retry. Otherwise normalize via scalar SSE float division
   and products. The separate nonpositive stored-norm branch also returns early.
5. Advance the same RNG a second time. In x87, compute the upper 16 bits times
   original **float** reciprocal of 65535, then multiply by `(maximum-minimum)`
   and add minimum. Compute `FSIN`/`FCOS`, storing each result to float.
6. Apply the rotation using scalar SSE operation order. Dot-product order is
   `(input.z*axis.z + input.y*axis.y) + input.x*axis.x`, followed by multiplication
   by `(1-cosine)`. Output terms are
   `input*cosine + axis*scaled_dot - (input × axis)*sine`. All component products,
   sums, differences and stores follow the listing; no fused multiply-add occurs.

Absolute scalar references inside the complete routine:

| Address | Raw bits | Value / use |
| --- | --- | --- |
| `00465030` | `3F800000` | float 1.0 |
| `0046504C` | `38D1B717` | float threshold 0.00009999999747378752 |
| `00465048` | `00000000` | float zero |
| `004650C0` | `37800080` | float reciprocal 1.5259021893143654e-5 |
| `00465028` | `4F800000` | float 4294967296 for signed-to-unsigned conversion branch; upper-16-bit input is nonnegative, so that branch is not taken here |

`random-direction-table-1.1.json` preserves every table vector's raw float bits;
`tu-random-direction-rotation.asm` preserves every instruction. The final opcode
is `RET 8` at `147CAF`, occupying three bytes through `147CB1`.
`tools/extract_weapon_direction_table.py` pins both full executable hashes and
verifies that base table `004417F0` and updated table `00447E40` have identical
bytes and the same table hash. The base random-helper entry itself has not been
recovered here. A uniform radial
sampling formula or arbitrary yaw/pitch perturbation would not reproduce this
routine's table selection, RNG consumption, or rounding.

## Where the original invokes the calculation

The control-flow listings are saved as `tu-weapon-*.asm`; the inventory with exact
call sites/constants/table file offsets is
`analysis/multiplayer/map-pack/weapon-error-invocation-1.1.json`. The reusable
read-only collector is `analysis/multiplayer/map-pack/capture-weapon-error-invocation.py`.

In the final barrel-update loop of `1021D0`, the per-barrel marker at caller
`ESP+0x15` is reset at **`103260`**. Barrel state 1 executes query `1062B0` at
`1033B8`. The eligible success path sets that marker at **`1033DF`** (written as
`ESP+0x1D` while two arguments are pushed), then calls `107160` at `1033E4`.
A failed-query branch also sets it at **`10341C`** while incrementing the barrel's
byte counter at `+0`, saturating at `0x7F`; that branch does not call `107160`.
Other paths call `108010` instead. At **`10363E..103644`**, the marker chooses
`102090` at `10364E` when true or idle recovery at `10365B` when false.
Consequently the marker must **not** be replaced with an inferred boolean meaning
“a projectile was created.”

`107160` prepares firing and sets barrel flags `+4` bit 1 at **`1078B8`** and weapon
`+0x16C` bit 0 at **`1078C0`**. It does not directly call `108300`. `101A60` scans
barrels with that bit set, calls `108300` at **`101AB7`**, clears each barrel bit,
then clears the weapon bit. `1019F0` is a separate weapon-object pass that calls
`101A60` when the weapon bit is set. Its position relative to all other game-tick
callbacks has not yet been established.

A separate direct path in **`A38F0`** orders these calls:

```text
A3B31 -> 107160
A3B3F -> 108010
A3B49 -> 102090
A3B4F -> 101A60 -> 108300
```

The matching base path is **`A2BA0`**. It has 187 instructions and occupies 663
bytes; the updated path has 190 instructions and occupies 673 bytes. After the
corresponding preparation (`A2DE1 -> 102FB0`) and transition (`A2DEF -> 103E60`)
calls, the base proceeds directly to projectile dispatch (`A2DF5 -> FD980`).
The update inserts exactly this 10-byte block before the corresponding dispatch:

```text
A3B44  8b4508       mov eax, [ebp+8]     ; supplied barrel index
A3B47  8bcf         mov ecx, edi         ; supplied weapon index
A3B49  e842e50500   call 102090          ; complete fired-error update
```

The paired original listings are `base-weapon-shot-application.asm` and
`tu-weapon-shot-application.asm`; the pinned comparison is
`shot-application-update-delta.json`. There are also register-setup differences
before other relocated callees. Address masking does not prove those callees or
pointed-to data identical. The extra error-update invocation is nevertheless
directly present in the original updated bytes and absent at the matching base
site. It is a verified **1.1 behavior change in this bounded call path**, unlike
the helper extraction in the ordinary update loop.

For this concrete updated path the error state changes before projectile/angle
dispatch. Its exact higher-level event/network semantics and conditions relative
to the ordinary update pass remain unresolved.
Only `A38F0` and `1021D0` direct callers of `102090` were found in the recovered
updated code. This local call order does not prove the complete tick order.

Before wiring live multiplayer, recover and preserve the original barrel query
outputs, marker transitions, hand-timer dispatch/decay order, cached angle branch,
shot pass order, and RNG authority. These are the remaining behavioral seams;
substituting continuous trigger time or a guessed shot-count marker is not parity.

## Other changed-routine candidates and remaining title-update work

Normalized instruction matching is a locator, not an equivalence proof: it masks
image pointers and call/branch targets, so changed callees and indirect data may
still matter. First-`INT3` linear boundaries were replaced by recursive
control-flow tracing for these comparisons. In
`analysis/multiplayer/map-pack/traced-object-comparison.json`, 628 uniquely located
object/gameplay routine pairs yielded five nonidentical normalized instruction
bodies. Their bounded differences are:

| Base / updated | Verified local change | Unresolved scope |
| --- | --- | --- |
| `CE6B0` / `CFB00` | grenade-throw caller changes argument/register setup for relocated item impulse callee `10CF80` / `FB620` | full callee behavior and grenade balance |
| `D9640` / `DAC40` | damage-side item-impulse call changes the same argument/register setup | impulse callee semantics and complete damage event |
| `E8720` / `E9D50` | additional writes set unit `1C8/1CC/1D0` to `NONE`, `1E4/1E8` to zero, and word `1E0` to zero | exact state semantics and ready/event dispatch |
| `E9ED0` / `EB530` | additional state-2 gate tests active action bit 29; prior bit-27 eligibility is retained separately | melee event construction, damage and networking |
| `FDFB0` / `1021D0` | fired-error calculation extracted into `102090`, with call-site/register changes | complete trigger marker and shot-pass integration above |

The damage flags at base `004F55E4/004F55E6` and updated
`004FCDBC/004FCDBE` lie in zero-initialized section storage. The identified code
reads them; no direct absolute setter or direct pointer initializer was found.
This does not rule out an indirect or bulk write and supplies no basis for
inventing changed default balance settings.

The parent investigation separately saved 370 unmatched/ambiguous update prefixes
in `analysis/multiplayer/map-pack/unmatched-update-functions.json`. Its new
`C57A0` collision-query candidate and other unmatched routines remain research
leads, not verified numerical balance changes. Raw cache comparisons and matching
selected damage kernels do not establish every title-update fixup. At this stage,
**`native_gameplay_update_applied: false`** remains the correct boundary.
