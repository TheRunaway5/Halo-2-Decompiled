# Track `attachments`

The creators that C0CD0 (`object_attachments_create`, modelstate track) calls
for each definition attachment row, and the object function value subtree
(BCB70) they share with C0910.

- Source: `src/gameplay/objects/native_object_factory_attachments_tu15.cpp`
- API: `include/halo2/native_object_factory_attachments_tu15.hpp`
- Kernels: `tests/native_object_factory_attachments_kernels.cpp`
- Cases: `tools/native_object_factory_cases_attachments.py`

Floating-point owners take the live x87 control word and MXCSR. Every SSE
step goes through `native_campaign_script_fp_tu15.hpp`, and every x87
sequence (BEC50's and BC8F0's handle draws) runs as one asm block under the
caller's control word. 13DE70, 17F790 and 17F8A0 are the existing natives
`native_tu15_evaluate_authored_function_extended` and
`native_tu15_evaluate_transition_curve`/`_periodic_curve`; they read their
.rdata literals and curve tables through world memory, so the cases declare
those image blocks (0x46A0C0, 0x4E0 bytes; 0x4752A8, 0x5000 bytes).

Stack outputs: several originals write through pointers that some callers
aim at their own stack (10DE40's EBX/EDI, BCB70's and BC8F0's value, 10FB70's
slot). The native takes an `OriginalOutput`: a world address replays every
store there, address 0 keeps the value locally (the original's stack stores
are never observable).

## Ported owners

Physical-original fixture cases with `HALO2_ORACLE_STRICT_WRITES=1`. A
comparison runs each case twice (with and without the publisher). Coverage
counts every instruction of the owner's body. Mutations count rejected out of
tried (see the mutation section).

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| 18BD80 | `object_looping_sound_attachment_create` | 600 | 32/32 | 2/2 |
| 17EE10 | `contrail_profile_advance` | 500 | 56/56 | 2/2 |
| 17E940 | `object_contrail_attachment_create` | 800 | 72/82 (1) | 2/2 |
| 10FB70 | `object_function_variable` | 600 | 63/63 | 2/3 (E) |
| BEC50 | `object_base_function_value` | 1,200 | 239/252 (2) | 4/5 (E) |
| 10DE40 | `object_type_function_value` | 1,000 | 43/43 | 1/1 |
| 13E670 (+13DE70) | `function_reference_value` | 1,200 | 32/33 (3) | 1/2 (E) |
| BC8F0 | `object_function_evaluate` | 1,500 | 200/201 (4) | 5/6 (E) |
| BCB70 | `object_function_value` | 1,500 | 138/138 | 3/3 |
| 134730 | `color_interpolate` | 500 | 30/84 (5) | 1/1 |
| 134870 | `light_color_blend` | 600 | 65/65 | 2/2 |
| C2B30 | `light_function_scales` | 800 | 76/117 (6) | 3/3 |
| C2CB0 (+BCD80, BCFB0, BBD10, BBD50) | `light_update` | 1,500 | 394/407 (7), 36/36, 13/13, 19/19, 22/22 | 6/6 |
| C2260 | `object_light_attachment_create` | 1,200 | 77/77 | 3/3 |
| 179540 | `object_effect_attachment_create` | — | — | refuses 178E30 |

The existing natives these owners call, as exercised by the same cases:
13DE70 257/492 (kinds 0..8; kinds 9 and 10 stop at the power and control
point owners 37CDE8/13E9A0), 17F790 67/67, 17F8A0 57/60 (8).

Notes:

1. 17EA39..17EA50: an active scale (BCB70 nonzero) sets +2 bit 0 and calls
   17EF70 (contrail reset), which is refused. The census never ran it.
2. Since 2026-11-16 every callee is native (see
   [BEC50's callees](#bec50s-callees-2026-11-16)); the console clock's
   calls (BED65..BED6D, BEDD5..BEDDD, BEE04..BEE19) are proven by the clock
   kernels instead, and BEE69 is unreachable (the draw is a 16-bit value:
   FILD never sees a negative).
3. 13E6B6: 13DE70 already returns a value in [0, 1] (or NaN), so 13E670's
   clamp never moves it.
4. BCA31: the same non-negative draw as note 2.
5. 134730's HSV branch (flags bit 0: 1343E0/134510 on stack colors and an x87
   hue compare) is refused. The census never ran it.
6. C2BC0 (the 13DE70 clamp, as note 3) and the color function C2BED..C2C78
   (13E6E0, refused; never in the census).
7. C302D/C3030: the camouflage value is already known to be positive.
   C32C2..C32D3 and C32E0/C32E4 are the C4D30 (placement's `light_reconnect`)
   and C5250 (`native_tu15_detach_light_attachment`) calls. Both natives are
   wired, but their light partition fixtures live in the placement track, so
   these cases keep `[[0x4EABC4]]` zero and +2 bit 3 clear (closure stops).
8. 17F97D..17F983: interpolated table bytes are never negative.

## Type-level callbacks (2026-11-14)

10DE40's unit, biped, vehicle and weapon callbacks are native; the node
rebuild's function overlays (BF9A0 -> BCB70 -> 10DE40) reach them on
Coagulation. Each answers its names from body and definition fields and
returns 0 without a store for any other name (10DE40 has cleared both
outputs).

| Owner | Native | Kernel (cases) | Coverage |
|---|---|---|---|
| C9070 (unit) | `unit_function_value` | `unit_function_value` (1,500) | 144/144 |
| D3750 | `unit_function_ratio` | `unit_function_ratio` (500) | 34/34 |
| D3950 (+1CF0E0) | `unit_animation_fraction`, `animation_channel_fraction` | 600 + 600 | 37/37, 34/34 |
| E0900 (biped) | `biped_function_value` | `biped_function_value` (800) | 74/76 (9) |
| 1046C0 (weapon, +FC510, 165590) | `weapon_function_value` | `weapon_function_value` (2,000) | 374/406 (10), 15/15, 22/23 |
| 107140 | `weapon_trigger_fraction` | `weapon_trigger_fraction` (600) | 36/36 |
| F9270 (vehicle, +F82D0, E9530, F9E10, 24570, F9E40) | `vehicle_function_value` | `vehicle_function_value` (2,000) | 603/764 (11); F82D0 21/21, E9530 22/22, F9E10 11/11, 24570 16/16, F9E40 19/94 (12) |
| 10DE40 over these types | `object_type_function_value` | `unit_type_function_value` (1,500) | 43/43 |

9. E09D4/E09D7: the `0 > value` clamp. The biped values are 0, 1 or a
   length over a positive limit, never negative.
10. Everything after 165590 succeeds (1655F0, which calls 24B430 under game
    mode 9): 24B430 is a refusal and a closure stop.
11. Exactly the refused names' paths: F9484..F94B2, F95FB..F973D,
    F979D..F980A, F9B45..F9B7A, F9C78..F9C8D, F9CB1..F9CC6, F9D95..F9DA9.
12. Kind 0 (|+1F4|) only: F9270 passes kind 2 for its refused names and
    no other kind.

- **C9070:** 0xF0006BF +1B8, 0x6000087 +2C4, 0xE00059F +208, 0x11000088
  +2B0, 0x1100059B +25C, 0x1100059C +260, 0x160005A0 +264 (clamped);
  0xD00059D / 0x1800059E bytes +243 / +1F8 times 1/255; 0x5000581 D3750;
  0x90006AA D3950 and 0xB0006AB 1 - D3950; unclamped 1/0 flags 0x90005A1
  (not +10A bit 2, not +134 bit 18), 0x140005A2 (+248 set or +134 bit 1),
  0x140005A3 (+24C set). 0xB0005A9 and 0x130005A8 (as 0xC000569) go to the
  current weapon (+212 slot of the +218 array) through 1046C0.
- **D3750** (+334 current, +338 floor; definition +1D4, +1DC) and **D3950**
  (the manager at body + word +12A: state +7C 0x700005C fraction,
  0x700005D 1 - fraction, else 1; 1CF0E0's frame +1C over the 1E3150
  animation's frame count).
- **E0900:** 0x6000542 (+3DC == 2), 0x70000C9 (+39D set, else not
  handled), 0xC0005A4 (x87 |+88..+90| / definition +2DC when +2DC > 0; the
  24570 register sequence, FDIV and FSTP under the live control word).
- **1046C0:** heat (0xC000569, and 0x1D0007BA unless [[0x4EABC4] + 82]: the
  maximum of the barrels' +1BC, the trigger fractions times row +28, the
  spin-down rows and +180 times definition +164), barrels +1B4 / +1B8
  (definition +2D0), age (0x10000573/0xE000572, tick - +248 <= 1),
  triggers (107140), magazines (+22C over row +A, digits of +22C), body
  fields +180/+184/+18C/+175 and flags +16C/+16E, FC510 (+12C bit 0
  without bit 1), and the game-mode names through 165590.
- **F9270:** 29 names: D3750 or +148 bit 12, F82D0/+148/+348, E9530 bit
  50 of the +346 block, body fields +370/+374/+3FC, and ratios through F9E10
  (DIVSS when |b| > 0.001 by an exact x87 compare): throttle, speed (24570
  of +88), turn, seat (+38B), the definition's limits (+1F4/+1F8 maxima,
  +204/+208 times pi/180, +20C, +22C/+230, +264) and the dot product of +70
  and +88 (x87 under the live control word). FLD/FABS/FSTP is |x| with a
  signaling NaN quieted. The names that need F9E40's kind-2 limit
  (0xB000587, 0xD000586, 0x1300058A, 0x1400058B, 0x17000592, 0x18000590,
  0x18000593, 0x19000591), FA5D0 (0xB000597, 0xC0005A5) or 11FF40
  (0x10000594) are refusals; no census executes them.

Fixture: TRACK type definitions (tag rows 56..59, 0x400 bytes, the
object definitions' +38 model so BEC50 answers its own names), two jmad
graphs (rows 60/61) with inheritance rows and frame counts, the 1E5C80
request bytes 0x51DF20 zero, magazine and trigger rows, game time, game
mode +8/+180 (mode 9 only with definition +290 != 1) and [0x4EABC4] +82.
Each body plays one role, unit (+212/+218 weapon slots holding weapon-role
handles, vehicle and biped fields) or weapon (+20C triggers, which overlap
a unit's slots). Names come from each tree plus every name one off or 8
past a compared one (BEC50's refused names excluded).

Strict-write proof (seed 0x5C41, `HALO2_ORACLE_STRICT_WRITES=1`):

- the nine new kernels on the final fixtures: `PASS 10100 physical
  original evaluator tails, 20200 native comparisons, 11454 ordered
  original stores`; `vehicle_function_value` again after the targeted
  0x1300057E NaN fixture: `PASS 2000 ..., 1390 ordered original stores`;
- every attachments kernel together (the 14 earlier and the 9 new ones,
  before the final limit fixtures): `PASS 23600 physical original
  evaluator tails, 47200 native comparisons, 136056 ordered original
  stores`;
- ASan/UBSan harness, the nine new kernels: `PASS 10100 ..., 11438
  ordered original stores`, no sanitizer report.

Receipts: `build/test-results/nodebuild-attachments-20261115/`.

Coverage comes from the validator's `HALO2_ORACLE_EXECUTED_SITES=1`
report field (every executed original address).

Mutation screen (out of tree, the in-tree source SHA-256 eb1dab99...
unchanged; `HALO2_ATTACHMENTS_CASE_SCALE=0.3` for the first pass, full
counts for the survivors): 43 mutants over C9070, D3750, 1CF0E0, D3950,
107140, 1046C0, FC510, 165590, E0900, F9270, F82D0, E9530 and the x87
helpers. All 41 non-equivalent mutants are rejected: 32 in the first
pass, 8 of the 9 first-pass survivors at full counts, and 0x1300057E's
maximum order after the targeted NaN fixture (a NaN ratio on one side).
Large definition limits (4, 10, 25, 37.5) were added so speeds and ratios
land inside (0, 1) (E0900's divisor, 0xB000713's half, the dot product
term). Two mutants are equivalent:

- C9070 0x90005A1 clamped: its value is 0 or 1 already.
- FLD/FABS/FSTP without the signaling-NaN quieting: every F9270 use either
  fails F9E10's ordered compare (any NaN) or feeds DIVSS, which quiets the
  same payload.

### The MCC StringID namespace (2026-11-15)

The live MCC cache names object functions in its own StringID namespace:
for example TU1.5's `barrel_spin` is 0x0B0005A9, the cache's 0x0B0005B5.
The translated callbacks compare the cache's operands at their compiled
literals, using the authenticated join
`src/rendering/effects/source_object_function_symbols_tu15.inc`
(`source_object_function_compiled_symbol_tu15`). The native callbacks
compared TU1.5's literals, so a cache function name never matched.

The node-rebuild map sweep found this on Turf (4 players). A vehicle
attached to a parent vehicle has a BF9A0 function overlay on
`barrel_spin`. The original returned 1/255 (C9070 -> 1046C0, the weapon's
+175 byte), the native 0. The overlay frame and node orientations then
differed: 283 of 34,475 calls.

`ObjectFunctionNamespace` (a thread scope) and `object_function_tu15_name`
now join an incoming name to TU1.5's namespace:

- Where they apply: at BCB70's literal checks (`one`, `zero`) and at the
  10DE40 type callbacks' entry.
- Mapping rule: a cache function name maps to its TU1.5 name. Both folded
  SUB chains also map: `scripted_object_function_a/b` and
  `driver/gunner_seat_power`. A cache name equal to a TU1.5 literal that it
  does not denote maps to 0xFFFF0000, which matches nothing.
- What keeps the cache's names: the 10FB70 variables, the definition rows'
  name compares and the parent recursion.
- When the scope is set: the live node-rebuild binding sets it when its
  bank has the MCC symbol binding.
- Effect on the oracle: outside a scope the mapping is the identity, so the
  oracle results stand.

Callers of these natives outside the node-rebuild binding need the same
scope. This includes the modelstate track's function scales, if bound
live.

## Projectile, device and machine levels; the rest of F9270 (2026-11-15)

The last type-level callbacks of 10DE40 and every refused F9270 name are
native. No sweep reached them, but a map whose function overlays (BF9A0)
or model functions name one would have stopped at the refusal.

| Owner | Native | Kernel (cases) | Coverage |
|---|---|---|---|
| 100E70 (projectile) | `projectile_function_value` | `projectile_function_value` (800) | 91/91 |
| 10BE20 (device: machines, controls, light fixtures) | `device_function_value` | `device_function_value` (1,500) | 171/171 |
| 11E490 (machine, crate) | `machine_function_value` | `machine_function_value` (600) | 30/30 |
| 1DB800 | `physics_component_function_value` | `physics_component_function_value` (400) | 34/34 |
| 24B430 (1655F0, mode 9) | `weapon_mode9_value` | `weapon_mode9_value` (600), `weapon_function_value` (2,000) | 24B430 34/34; 1046C0 406/406; 165590 23/23 |
| F9E40 kind 2, FA5D0, 11FF40 (in F9270) | `vehicle_function_value` | `vehicle_function_value` (2,000) | F9270 762/764 (13); FA5D0 7/7; 11FF40 33/34 (14); F9E40 kinds 0 and 2 complete (15) |
| 10DE40 over types 5, 7, 8, 9, 11 | `object_type_function_value` | `device_type_function_value` (1,500) | |

13. F96E5/F96E8: the engine weight's lower clamp. The weight is (byte * 0.2
    + 1) * 0.5 >= 0.5.
14. 11FF6B: the stack fallback for a null output. F9270 always passes a local.
15. F9270 passes only kinds 0 and 2 to F9E40.

- **100E70:** 0xE000562 time_remaining (+158), 0x6000564 tracer (+12C
  bit 1), 0x11000566 projectile_attach (bit 3), 0xF000563 range_remaining
  (+168 / definition +E0, 0 when +E0 is an ordered zero), 0x12000565
  acceleration_range (1 for +12C bit 8 or a zero +17C or +180, else
  (+168 - +18C) * +194 clamped).
- **10BE20:** 0x5000081 power (+134) and 0x8000080 position (+140), clamped;
  0x5000561 delay (game time +4 / definition +10C * word +148, when +10C >
  0); 0x40005AB loop (1 when 0.9 >= +140); 0x6000560 locked (+134 == 0,
  then for a machine (+AA 7) with a +13C device group: +1CC & 0xD or the
  group's +2 bits 0 and 1 set it, and +140 == 1.0 or +1CC bit 4 publish an
  unclamped 0); 0x1200055F change_in_position and 0xF00055E change_in_power
  (x87 |+144| / |+164| under +12C bit 3, else |+144| / definition +C8, and
  |+138| / definition +C0, each FDIV and FSTP under the live control word,
  0 below the 1e-4 FCOMIP floor).
- **11E490 / 1DB800:** the +B4 physics component row (0xA0 rows of
  [0x52BE5C]); 0xA000693 phantom_on (+4 bit 13), 0xD000691 / 0xD000692
  (+1B / +1A over 255). Without a component 11E490 returns 0 unstored.
- **24B430:** 1046C0's bomb names now publish the mode-9 value x: 0xF0006E5
  x == 0, 0xE0006E6 0 < x < 1, 0xD0006E7 x == 1 (all unclamped),
  0x120006E4 x clamped.
- **F9270:** F9E40 kind 2 is the larger of |+1F4| and |+1F8| (ties and NaN
  to +1F8). velocity_up and the tread and tire velocities divide an x87
  term (dot products, +354 -/+ +35C, |+354|) by it. 0xD000586 divides the
  velocity dotted with the forward x up cross product, whose three terms
  are each stored as floats (FSTP) before the dot. The engine names
  0xB000597 and 0xC0005A5 blend the forward-speed and throttle ratios by
  (byte +34C * 0.2 + 1) * 0.5, taking the +3F0 velocity and +3DC
  throttle under +348 bit 7 (FA5D0). They are always published active,
  except engine_power with 0 >= +25C, which is 0 and inactive. Definition
  +1F0 modes 0 and 1 take +378 / +264 instead. 0x10000594 squares the
  velocity without its forward component (11FF40) over 0.3.

Fixtures (units_world plus `device_world`, `vehicle_fields` and
`mode9_world`): a four-row physics component pool, projectile and device
definition fields, device groups from the base world, edge floats (0,
-0, 0.9, 1e-4 and its neighbours, denormals, signaling NaN), vehicle
definition +1F0 modes, +348 bit 7, +3F0/+3DC/+34C/+25C, and the
[0x52D710] mode-9 block with live players for the bomb names. The type
fixture `device_type_function_value` assigns types 5, 7, 8, 9 and 11.

Proof (strict writes, seed 0x5C41):

- The new kernels plus `vehicle_function_value` and `weapon_function_value`:
  `PASS 9400 physical original evaluator tails, 18800 native comparisons,
  11584 ordered original stores`; the targeted device and weapon cases
  again: `PASS 3500 ..., 2042 ordered original stores`.
- Every attachments kernel with the track loaded: `PASS 57716 physical
  original evaluator tails, 115432 native comparisons, 377747 ordered
  original stores`. Modelstate's kernels (which borrow this track's stops):
  `PASS 9100 ..., 120653 ordered original stores`.
- On the final fixtures: the new kernels with `weapon_function_value`,
  `unit_type_function_value` and `unit_function_value`: `PASS 12400 ...,
  26268 ordered original stores`; `vehicle_function_value` after its last
  fixture change: `PASS 2000 ..., 2148 ordered original stores`.
- ASan/UBSan harness (the new kernels, vehicle and weapon): `PASS 9400 ...,
  13098 ordered original stores`, no sanitizer report.
- Mutation screen (out of tree, scratch copies compiled with the kept
  harness flags; in-tree SHA-256 db759e35... unchanged): 37 mutants over
  100E70, 10BE20, 1DB800, 11E490, 24B430, 1046C0's bomb names and F9270's
  new paths, **all rejected**. 24 were rejected at half case counts. The
  other 13 survived at first and were rejected after targeted fixtures:
  - each callback draws mostly its own names, since the global neighbour
    list had diluted them to about 3%;
  - acceleration_range with one zero limit, and a delay inside (0, 1);
  - the engine names on +1F0 modes 2/3 with +348 bit 7 against bit 6 and
    +34C weights 1..4;
  - wingtip_contrail with lateral speeds under 0.3. NaN forward and velocity
    lanes give the forward lane the larger payload, because the x87 length
    keeps the larger NaN significand.
- Receipts: `build/test-results/nodebuild-rare-20261115/` (`oracle/`,
  `asan/`, `mutation/`, `names.py` with `names.txt` for the comparison-site
  audit).

### MCC names of the chained comparisons (2026-11-15)

The translated callbacks join only the instructions the .inc lists. A
SUB chain after a joined SUB keeps its original distance, so the
translated route matches the MCC operand plus that distance. An audit of
every comparison site of BCB70, BEC50, C9070, E0900, F9270, 1046C0,
100E70, 10BE20 and 1DB800 (each site's translated operand against its
TU1.5 literal) found eleven such names the native join did not map:

| Site | TU1.5 | MCC (translated) |
|---|---|---|
| BEC50 BEDF6 / BEDFD | 0xC0006B7 / 0xC0006B8 | 0xC0006C3 / 0xC0006C4 |
| F9270 F9794 | 0xD000586 | 0xD000592 |
| F9270 F9986 | 0xE000583 | 0xE00058F |
| F9270 F9B99 | 0x13000588 | 0x13000594 |
| F9270 F9D7C | 0x18000593 | 0x1800059F |
| 1046C0 104BD8 / 104C0D / 104C12 | 0x14000578 / 0x16000579 / 0x1700056B | 0x14000584 / 0x16000585 / 0x17000577 |
| 1DB800 1DB80D / 1DB815 | 0xD000691 / 0xD000692 | 0xD00069D / 0xD00069E |

Before this change, an MCC overlay on 0xE00058F, 0x13000594, 0x14000584,
0x16000585 or 0x17000577 would have returned 0 natively where the
translated route computed a value. No sweep met one. These names, and
100E70's five projectile names (the translated route joins them through
`source_projectile_function_symbol_tu15`), are now in
`folded_function_names`. No MCC name maps to two TU1.5 names, and no TU1.5
name to two MCC names, across all 145 sites.

## Mutation screen

Each mutant edits only this track's source; the screen restores the pre-run
copy and checks its hash afterwards, and builds through the target's own
makefile. (E) marks equivalent mutants, which no input can distinguish:

- 10FB70, swapped MULSS operands: the interpolation factor is a ratio of
  integers and never NaN, and SSE returns a single NaN operand from either
  position.
- BEC50, the variant ratio without its upper bound: a ratio above 1 is
  clamped to the same 1.0.
- 13E670 without its clamp: 13DE70 already returns a value in [0, 1] (note 3).
- BC8F0, swapped scale product operands: NaN payloads only, and both
  operands are never NaN together.

Targeted fixtures that kill the remaining mutants: a denormal BEC50 0xF0005AC
operand (UCOMISS under DAZ), function flags with only bit 7 (13E670's high
nibble), periodic rows with a denormal or active import and the handle draw
(BC8F0), and an unattached light whose colors are zero but one denormal
(C2CB0's visibility under DAZ).

## Call shapes for C0CD0 (wired 2026-11-12)

These shapes are now wired in modelstate's C0CD0 and C0910 and proven
against the original through modelstate's composed cases (see
[modelstate.md](modelstate.md#integration-2026-11-12)). C0CD0 stores each
creator's result after the creator's own stores, then the kind byte and
handle (`block + 8n`, `+4`):

| Kind | Group | Native call | Then |
|---|---|---|---|
| 0 | 'ligh' | `object_light_attachment_create(m, row[+4], object, n, row[+10], word row[+C], x87, mxcsr)` | non-NONE: body +4 \|= 0x10 |
| 1 | 'lsnd' | `object_looping_sound_attachment_create(m, row[+4], object, n, row[+10])` | non-NONE: body +4 \|= 0x20 |
| 2 | 'effe' | `object_effect_attachment_create(m, object, row[+4], row[+10], row[+14], word row[+C])` | refuses 178E30 |
| 3 | 'cont' | `object_contrail_attachment_create(m, row[+4], object, n, x87, mxcsr)` | — |

C0910's refused calls are BCB70, `object_function_value(m, object, name,
OriginalOutput{address}, x87, mxcsr)` (address 0 for a stack target), and
134730, `color_interpolate(m, entry[+4], entry + 8, entry + 0x14, color, t,
mxcsr)`.

## Refused dependencies

Each one throws `detail::refuse` and never runs original code:

| Address | Reached from | Why |
|---|---|---|
| 178E30 | 179540 | effect_new: the effect system (see below) |
| 17EF70 | 17E940 with an active scale | contrail reset |
| 1CF0E0 empty lookup | D3950 / 1CF0E0 when 1E3150 returns 0 | the original faults |
| 13E6E0 | C2B30 with a color function | light color function |
| 1343E0/134510 (HSV) | 134730 with flags bit 0 | they would target stack colors; modelstate's natives write world memory |
| C2B30 IDIV | a zero game tick rate | the original faults |
| 37CDE8, 13E9A0 | 13DE70 kinds 9/10 | existing native refuses outside its FP profile; the cases avoid them |

Other refusals: 10DE40 with a type outside 0..12.

Real attachments mostly sit on vehicles and units; their type callbacks
(C9070, E0900, F9270, 1046C0) are native since 2026-11-14.

## Effects (179540): continuation plan

179540 only fills a 0x60-byte request on its stack and calls 178E30:
`+0` flags 7, `+4` tag, `+8` object (EDX), `+C`/`+10` NONE, `+14`/`+18` words
NONE, `+1C` the 1791C0 location block pointer, `+20` kind 2, `+34`/`+38` the
primary/secondary scales, `+3C` the marker word, `+40`/`+44` 1.0, `+54`/`+58`
0xFF808080; then BBE00 (core `object_world_position`, which today writes to
world memory only: a value-returning form is a core request) and 1791C0
(location: position, [0x474D1C], 0x70000C0, position, [0x474D10], 0x20000CA).
Nothing observable precedes 178E30, so the native refuses at once.

178E30's executed slice (union census, about 14k instructions) calls 1790A0,
17AFB0, 169112, BCDF0, 17B0D0, 17E5E0, 17B1F0 (effect locations, also
modelstate's 17E060 refusal), 17B880, 17E460, 17A1A0, 17C6E0 and 16E940;
under them sit the particle systems (250C60, 251C80, 252100, 252480), the
light owners above (C2CB0 is now native), 30AA70/16C620 (MOPP and collision
rays, partly native in `include/halo2/native_tu15_*`), 17D210 and C2CB0's
siblings. Port order: 178E30's datum and location setup (17AFB0, 17B0D0,
17E5E0, 17B880), then 17B1F0, then the event/part creators (17E460, 17A1A0)
with their particle and light children; refuse each unported child by name.
B9B70's creation effect 179610 shares 178E30.

## Oracle extension

- `UNREACHED_VIRTUAL_OWNERS` gains 10DE40 (its CALL ECX), and a metadata
  wrapper admits BEC50's closure (with this track's stops) as authenticated
  sites, like the placement track's 10DDE0 callback.
- `JUMP_TABLES[0x13E5F4] = 11` (13DE70's kinds) and `REGISTER_JUMPS[0x331F55]`
  (37DC0A's FPREM/FLDZ methods, as modelstate sets them).
- Every kernel declares `closure_stops` for the refusals above plus C4D30 and
  C5250.

## Shared-file needs

- Modelstate (done 2026-11-12): C0CD0 and C0910 call these natives, and
  modelstate's stops no longer list C2260, 18BD80, 17E940, BCB70 or 134730.
  The cases module publishes its fixture builders in `FIXTURES`; modelstate
  borrows them through `tools/native_object_factory_track_fixtures.py` and
  adds this track's `STOPS` and BEC50 admission to its composed routines.
- Modelstate: a value-returning BCD80 (`object_color_channel` writes world
  memory; C2CB0 copies into its stack, so this track reads the channel
  locally) and a stack-target 1343E0/134510 for 134730's HSV branch.
- Core: a value-returning BBE00 for 179540's stack position.

## Validation

Build with the tracks `attachments;core;nodes;placement;nodebuild`
(placement for `light_reconnect`, nodebuild for placement):

```
PATH=/usr/bin:$PATH python3 tools/run_tests.py --target native_campaign_script_builtins_tu15_tests \
  --build-only --keep-artifacts --jobs 3 \
  --cmake-arg=-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="attachments;core;nodes;placement;nodebuild" --output <scratch>
HALO2_ORACLE_STRICT_WRITES=1 HALO2_FACTORY_TRACKS=attachments python3 tools/run_test.py -- \
  .tools/python/bin/python tools/validate_native_campaign_script_builtins_tu15.py --native <bin> --report <scratch>/r.json
```

Results (2026-11-12, seed 0x5C41, strict writes):

- After the C0CD0/C0910 wiring (2026-11-12, this track's kernels,
  `HALO2_FACTORY_TRACKS=attachments`): `PASS 12000 physical original
  evaluator tails, 24000 native comparisons, 117601 ordered original
  stores`. The composed paths are proven by modelstate's cases.
- All 14 attachments kernels on the final source:
  `PASS 13500 physical original evaluator tails, 27000 native comparisons,
  130072 ordered original stores`.
- The whole in-tree builtins oracle with this track loaded (before the
  `color_interpolate` kernel and the targeted mutation fixtures were added):
  `PASS 39892 physical original evaluator tails, 79784 native comparisons,
  300747 ordered original stores`.

## C3910: the lights' transition value (2026-11-16)

BEC50's name 0x130006BB (a light attachment's function on Coagulation:
the light tick's differential refused it at tick 19) is native:
`lights_transition_value`. [0x51DAE8] holds the lights' on/off state (+0
byte) and the tick its transition started (+4). The seconds since then
(the tick difference, CVTSI2SS, MULSS by the tick length [0x51DF54] +4) are
taken at most 1 (COMISS 1.0 against them; NaN takes 1), go through
transition curve 5 (17F8A0, `native_tu15_evaluate_transition_curve`) and
are mirrored (1 - value) while the lights are off. BEC50 clamps the value
like its other names. Fixtures: `fresh()` sets [0x51DAE8] (TRACK + 0xE800)
with the state byte 0, 1 or 0x80 and start ticks 0..1000 ticks back (and 3
ahead) of the game tick; 0x130006BB joins `BASE_NAMES`, so BEC50's,
BCB70's and C2CB0's cases draw it.

## 13E6E0: the light colour function (2026-11-16)

C2B30's colour function (definition +C0 block +C count, +10 reference),
which Highplains and Waterworks (4 players) reach through the light tick,
is native: `color_function_value` (13E6E0) and the tint in
`light_function_scales`. 13DE70 evaluates the reference for the input;
13E6E0 returns a 32-bit ARGB colour: NONE without data, the first colour
for a constant (+0 type 1 with +1 bit 0 clear or a kind up to 1), grey
(the clamped input times 255, FISTP under the x87 control word, opaque),
or a blend of two (+4, +10), three (+4/+8, +8/+10 over the two halves of
2x: x87 FADD, 16AC0's SSE floor, the x87 fraction) or four (+4.. over the
thirds of 3x: x87 FMUL qword 3.0) colour stops. The blend is the
original's MMX lane arithmetic, written as 16-bit lane steps: c0 +
((((c1 - c0) << 3) * t >> 16) + 1 >> 1) saturated to a byte, t the
fraction times 16384 rounded by CVTPS2PI under the MXCSR. C2B30 then scales
the light's first colour channel by channel (red, green, blue: the byte
over 255, FILD/FMUL/FMUL/FSTP).

Fixtures: the colour reference beside each definition's other references
(count 1, 0 or NONE; data a function blob or none) with the colour kind in
the blob's +1 high nibble (0..5 and 15, bit 0 set or clear); the block's
+C count 0, NONE or 1. `JUMP_TABLES[0x13E948] = 5`. 13E6E0 left
`LIGHT_STOPS`, so composed kernels reach it.

Proof (strict writes, seed 0x5C41, the attachments kernels
`object_base_function_value`, `light_update`, `object_function_value`,
`object_type_function_value`, `object_light_attachment_create`,
`light_function_scales`, `light_color_blend`, `color_interpolate`): `PASS
8300 physical original evaluator tails, 16600 native comparisons, 114765
ordered original stores`.

A direct kernel, `color_function_value` (entry 13E6E0, ECX reference,
stack input; 800 cases: every colour kind, stops with extreme channels,
inputs inside, at and outside [0, 1] and NaN), with `light_function_scales`:
`PASS 1600 physical original evaluator tails, 3200 native comparisons, 2377
ordered original stores`. Coverage: 13E6E0 183/183, 16AC0 9/9, C3910
20/20, C2B30 113/117 (C2BC0, note 6, and the three unsigned-FILD
corrections C2C1F/C2C46/C2C69: a channel byte is never negative).

Mutation screen (out of tree, in-tree SHA-256 checked): 15 mutants over
13E6E0, C2B30's tint and C3910; 11 rejected (8 of them for 13E6E0, three
only after the direct kernel). The four survivors are equivalent: 13E6E0's
byte saturation dropped (with the fraction in [0, 16384] the lanes stay in
0..255), its constant test strict (kind 1 returns the same first colour),
16AC0's floor as a truncation (the clamped input is never negative), and
C3910's cap dropped (transition curve 5 clamps a finite input to [0, 1]).


### 13E6E0's other copies (2026-11-16, continuation)

Two generated units carry a translated 13E6E0: the live-support unit
(called at C2C03, inside C2B30) and the lens draw unit
(`source_lens_draw_tu15_generated.cpp`: 13E960, the colour function to
float RGB through 1346C0, called at 13E971 from 2E9C0's 2EDBA, the lens
reflection draw). The XBE's other callers (17A86, 182C1, 187F9, 1B700,
4F5BF, 5145D, 159CDB, 250131) have no translated body in any unit.

A census of every MP map at 4 players (300 ticks of the sweep's scripted
input) and campaign 01a, 03a and 05a (300 frames), with the native bridges
on, found 13E6E0 entered three times, all at C2C03 (Highplains 2,
Waterworks 1): C2B30 reached through the effects' translated C2CB0 calls
(the paused group E). Nothing was bound. The lens draw copy runs on the
lens execution path, whose services the main-thread census does not count,
so the census cannot show whether it is reached; it belongs to the lens
flare code (group E's `source_lens_flare` area) and stays translated.
Binding it needs a lens-path counter first, then the colour function at
13E971 (ECX reference, stack input, RET 4) with C2B30's FP controls.

## BEC50's callees (2026-11-16)

Every name BEC50 answers is native now (no light-update sweep reached the
refused ones, but a map whose functions name one would have stopped):

| Owner | Name | Native | What |
|---|---|---|---|
| C1F90 (with BECEB) | 0x700055C | `object_heading_value`, `wrapped_angle` | Node 0 upright (\|forward z\| >= 0.995, or NaN) is 1. Otherwise atan2(+4, +8) (x87 FPATAN under the control word) less the scenario's north (+1C), wrapped once (2 pi off at pi or above, on at -pi or below; pi and 2 pi are the startup globals [0x5564C8]/[0x5564C4]), times 1/(2 pi) plus 0.5, clamped |
| BEAA0 | 0x90006B5 | `object_globals_timer_value` | [0x4EABC4] +14 is a tick (0: none): none or not yet, 1; else the seconds since (CVTSI2SS, MULSS by the tick length): 0 past one second, else (seconds - 15) * 0.2 (clamped to 0). The original's 20-second test follows the one-second exit and never passes |
| 15B2D0 | 0xF0005AC | `player_function_scale` | A player's unit or biped in game mode 2: the player scale (game engine 7 with the player's bit in [0x51DF9C]: 3 for +22C bit 1, else 0 for team mode 1, else 1; otherwise by team mode: 0, 3 or 1), times 0.75/0.5/0.25 by the player row's +C1 index; not above 0 publishes 0 (unclamped) |
| BE9A0 | 0xD0005FF, 0x170005FA/B, 0x180005FC, 0x1D0005FD/E | `object_byte_value` | The object's +24 (/64), +25 (/32) and +26 flag bits (1/0, 1/-1); it answers (BEC50's BL: `active` is then 1 whatever the value) unless the object is NONE or its +26 bit 3 is clear in a scenario whose +10 word is 0 |
| BEBE0, BEB00, BEB40, BEB90 (13BB70, 13BB10) | 0x70006BA, 0xA0006B6, 0xC0006B7, 0xC0006B8 | `local_holiday`, `holiday_function_value`, `clock_hour_value`, `clock_minute_value` | The console clock: GetLocalTime (33BA6B, kernel imports) as 13BB10 unpacks it; the native reads the host's local time. Holidays (12/25, 1/1, 10/31, 7/7, 7/4, 12/12) map to 0.25/0.5/0.75/1.0 for the first four; the hour hand is the hour clamped to [1, 24], mod 12, /12; minute and second hands are clamped to [0, 59], /60 |

Fixtures (`fresh`, `body_values`, `base_case`): node 0's forward axis with
upright and tilted z (+4, +8 from 0, -0, +-1 and random), pi and 2 pi (exact
or random), the scenario's north and +10 word (and no scenario for BE9A0),
game globals +180/+1C4/+22C, the player bits word and two player rows,
[0x4EABC4] +14 around the game tick (equal, before, after), the object's
+24..+26 bytes; `CALLEE_NAMES` join BEC50's and 10DE40's name draws, and
0xF0005AC cases put players' units and bipeds in mode 2. The clock names
stay out of BEC50's cases: the observer cannot run 33BA6B. Five clock
kernels enter each owner after its 13BB10 call with 13BB10's unpacked time
as the frame and STOP as the return (BEB0D, BEB4D, BEB9D, 13BB80; BEBEE
with the holiday in EAX, holidays 1..4: the default's local is the
observer's return slot there). `JUMP_TABLES` gains 0xBEC3C and 0x15B39C;
the literals and both switch tables are checked against the XBE.

Proof (strict writes, seed 0x5C41): `object_base_function_value` and
`object_type_function_value`: `PASS 2200 physical original evaluator
tails, 4400 native comparisons, 6792 ordered original stores`; the clock
kernels: `PASS 2000 ..., 4000 native comparisons`; every attachments
kernel on the final sources (35 kernels): `PASS 31800 physical original
evaluator tails, 63600 native comparisons, 159444 ordered original stores`.
Coverage: BEC50 239/252
(note 2), C1F90 13/13, 15B2D0 54/54, BE9A0 75/75, BEAA0 22/24 (the dead
20-second branch), BEB00/BEB40/BEB90 16/20 and 13BB70 59/65 and BEBE0
21/25 (each misses only its prologue and clock call). ASan/UBSan: with
the placement and lights kernels (see the claims doc), no report.

Mutation screen (out of tree, in-tree SHA-256 checked): 22 mutants over
BEC50's callees and the clock; 17 rejected (two only after round 2's
targeted fixture: a heading of exactly -pi against a zero north and exact
pi, and a zero player scale against a zero +F0). The five survivors are
equivalent:

- C1F90's first wrap strict: at a heading of exactly +pi the second wrap
  restores pi (with the startup's pi and 2 pi the round trip is exact);
- BEAA0 without its 20-second test (dead in the original);
- 15B2D0's engine-7 team mode 2 as 3 and its +C1 indices 1 and 3 swapped:
  BEC50 reads only the sign of 15B2D0's scale;
- 15B2D0's player mask read as a dword: player indices are below 16.
