# Track `modelstate`

Owners that `object_new` (B9B70) calls for model state: change colors (C0270),
regions and permutations (BC5C0), function-scaled colors (C0910), the
animation manager setup (1D3380) and the type finish (C06C0).

- Source: `src/gameplay/objects/native_object_factory_modelstate_tu15.cpp`
- API: `include/halo2/native_object_factory_modelstate_tu15.hpp`
- Kernels: `tests/native_object_factory_modelstate_kernels.cpp`
- Cases: `tools/native_object_factory_cases_modelstate.py`

Floating-point owners take the live x87 control word and MXCSR. Every SSE
step goes through `native_campaign_script_fp_tu15.hpp`. Every x87 sequence
(the position hash, FPREM, the seeded draws, the 1349E0 packing) runs as one
asm block under the caller's control word.

## Ported owners

Cases are physical-original fixture cases. A comparison runs each case twice
(with and without the publisher). Coverage counts every instruction of the
owner's body. Mutations count rejected out of tried.

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| C0270 | `object_change_colors_initialize` | 1,200 | 287/290 (1) | 5/5 |
| 37DC0A/37D46C/331ECB/37DC14/331F68/37D536 | inside C0270 (`fmod_by_one`) | (C0270) | (2) | (C0270: x87 precision) |
| 1343E0 | `color_rgb_to_hsv` | 600 | 84/84 | 2/2 |
| 134510 | `color_hsv_to_rgb` | 600 | 71/72 (3) | 2/2 |
| 1349E0 | `color_pack` | 400 | 27/27 | 1/1 |
| BCD80 | `object_color_channel` | 300 | 36/36 | 1/1 |
| 3DD00 | `object_color_publish` | 400 | 57/57 | 2/2 |
| C0910 | `object_function_colors` | 800 | 183/183 (4) | 2/2 (before the wiring) |
| BC5C0 | `object_regions_initialize` | 800 | 88/88 | 2/2 |
| BC6C0 | `object_region_bank` | 300 | 33/33 | 1/1 |
| 1700E0 (+170930, 170B40) | `model_region_permutations_select` | 1,200 | 281/281, 135/160, 85/90 (5) | 5/5 |
| 17E060 | `object_region_effect_restart` | 400 | 46/54 (6) | 1/1 |
| 1CE0E0 | `object_region_physics_check` | 400 | 82/87 (6) | (7) |
| 1D3380 | `animation_manager_setup` | 600 | 101/101 | 4/4 |
| D6E40 | `object_widgets_create` | 300 | 56/95 (6) | 1/1 |
| C0CD0 | `object_attachments_create` | 800 | 111/124 (6) | 2/2 (before the wiring) |
| C06C0 | `object_type_finish` | 600 | 19/19 | 0/1 (8) |

1D3380's 1D5320 call is the weapon track's
`weapon_animation::manager_set_state`, with its whole subtree (see
[weapon.md](weapon.md)). 1D3380's 600 cases cover its guard, its record
stores and that subtree together.

The full in-tree builtins oracle with this track loaded (all 139 kernels:
the 16 modelstate kernels, 8,500 cases, plus every builtin and base factory
kernel) passes:
`PASS 35440 physical original evaluator tails, 70880 native comparisons,
306171 ordered original stores`. That run used `HALO2_FACTORY_TRACKS=modelstate`,
seed 0x5C41, and a harness built with the tracks `modelstate;weapon;core;nodes`.

Every mutant except the equivalent one in note 8 is rejected. Three of
them needed targeted fixtures:
- the `>=`/`>` threshold ties of C0270 and 170930: a zero position, which
  hashes color 0 to 0, and an LCG seed whose next draw is 0 against a zero
  first weight;
- C06C0's projectile branch: projectile header types (5). This covers the
  branch (19/19), but the mutant is equivalent (note 8).

Notes:

1. C04D9..C04E5 cannot run: the pick loop always stops at its last
   selected permutation.
2. Only the TU1.5 data path runs. [46ECD0] is nonzero, the [48C8DA] kind is
   not 5, [48D4F4] is zero, and the precision exception is masked. A
   normal dividend selects FPREM (37DC14); a zero or denormal one selects
   FLDZ (331F68). An infinite or NaN hash would reach the CRT error methods
   (3334F0), which are refused.
3. 134551 (`DEC EAX`) only follows a negative hue, which then jumps past the
   six-entry table. The native refuses any table index outside 0..5. The
   scaled index wraps, so a CVTTSS2SI indefinite (NaN or a huge hue) selects
   entry 0, as in the original. Fixtures avoid the inputs that make the
   original jump through garbage: hues that round to sector 6 under upward
   rounding, zero-y blends that wrap a hue to exactly 1.0, and denormal
   permutation channels (a tiny negative hue wraps to exactly 1.0).
4. C0910's function and scale entries call the attachments track's
   `object_function_value` (BCB70) and `color_interpolate` (134730). The
   cases run them over that track's function world (see Integration below):
   BCB70 138/138, 134730 30/84 (its HSV branch, flags bit 0, is refused and
   kept out of the fixtures).
5. Branches that 1700E0's only call (filter -1, id 0, flags 0, random,
   capacity 32) cannot reach: the filter test, the id/flags search, the
   retry, the 2^32 FILD correction, and 170B40's loop fall-through.
6. The refused callees: 17B1F0 (from 17E060) and 1CC310 (from 1CE0E0, now
   the havok track's `havok_component_state_flags`, still a closure stop:
   it refuses at 1D9510/1D73D0). In D6E40, the widget creator calls. In
   C0CD0, only the effect call (C0E1A..C0E33: 179540 refuses at 178E30) and
   the second count check (C0D23, which repeats the first).
7. 1CE0E0 stores nothing. Its only effect is a refused call, so mutations
   can only show up as over-refusal.
8. Sending a projectile through D6E40 instead is indistinguishable without a
   refusal: D6E40's only store is the same +DC clear, and its widget loop
   only refuses.

## Refused dependencies

Each one throws `detail::refuse` and never runs original code:

| Address | Reached from | Why it is refused |
|---|---|---|
| 179540 (178E30) | C0CD0 for an 'effe' row with a tag | effect attachments: the attachments track's `object_effect_attachment_create` refuses at effect_new 178E30 |
| 118480, 119060, C5B00 | D6E40 | widget creators (antenna, cloth, 'tdtl') |
| 17B1F0 | 17E060 with a live region-attached effect | effect location restart (birth attachments are all 0xFF at BC5C0 time) |
| 1D9510 / 1D73D0 | 1CE0E0 when a permutation's physics byte changes, through the havok track's 1CC310 | Havok component removal and shape refresh |
| 3334F0 | 37DC0A with an infinite/NaN dividend | CRT math error |

Other refusals: BC5C0 with a region count outside 0..16 (its 16-byte stack
copy), 1CE0E0 reading past the caller's prior states, and 1700E0 with more
than 32 candidates (stack overrun). 

## Integration (2026-11-12)

- **C0CD0** calls the attachments track's creators with C0CD0's own call
  shapes (C0DE9, C0E06, C0E2E, C0E3C): lights `object_light_attachment_create`
  (tag, object, index, row +10, word row +C; a non-NONE handle ORs body +4
  with 0x10), looping sounds `object_looping_sound_attachment_create` (tag,
  object, index, row +10; body +4 |= 0x20), contrails
  `object_contrail_attachment_create` (tag, object, index), and effects
  `object_effect_attachment_create`, which refuses at 178E30. The creator's
  result is stored after its own stores, then the kind byte and handle.
  C0CD0 and C06C0 take the live x87 control word and MXCSR, and so do the
  services track's `function_colors` and `type_finish` bindings.
- **C0910** uses `object_function_value` (BCB70, a fresh stack output per
  call: BCB70 always stores its value first) and `color_interpolate`.
- **1CE0E0**'s 1CC310 call goes to the havok track's
  `havok_component_state_flags(object, 0)`.
- **Oracle.** `CLOSURE_STOPS` no longer lists C2260, 18BD80, 17E940, BCB70
  or 134730. The composed routines (C0910, C0CD0, C06C0 and D6E40, which
  shares their fixture) add the attachments track's own stops (its refused
  type callbacks, BEC50 callees, 13DE70 owners, 17EF70, C4D30/C5250,
  13E6E0 and the 134730 HSV callees), and BEC50 (10DE40's CALL ECX target)
  joins their authenticated sites through that track's metadata wrapper.
  Their fixtures are the attachments track's world, borrowed read only
  through `tools/native_object_factory_track_fixtures.py` (the attachments
  module publishes its builders in `FIXTURES`): function rows, the looping
  sound, contrail and light arrays and their definitions. Contrail rows use
  primary scales BCB70 leaves inactive (an active one reaches the refused
  17EF70); effect rows keep a NONE tag. Widget and attachment rows moved to
  TRACK +5800/+5000, which that world leaves free.
- **Proof** (seed 0x5C41, strict writes, `HALO2_FACTORY_TRACKS=modelstate`):
  all 16 modelstate kernels pass `PASS 9700 physical original evaluator
  tails, 19400 native comparisons, 162835 ordered original stores`;
  C0910, C0CD0, C06C0, D6E40, 1CE0E0 and BC5C0 alone pass
  `PASS 3700 physical original evaluator tails, 7400 native comparisons,
  55064 ordered original stores`. Coverage of the composed paths (C0910,
  C0CD0 and C06C0 cases): C0910 183/183, BCB70 138/138, 10DE40 43/43, BEC50
  203/252, BC8F0 194/201, C0CD0 111/124, C06C0 19/19, C2260 77/77, C2CB0
  279/407, 18BD80 27/32 (C0CD0 always passes an object), 17E940 72/82,
  17EE10 40/56.

## Remaining in this track

- **Effects** (179540 → 178E30, the effect system) and **D6E40's widget
  creators** (118480 antenna, 119060 cloth, C5B00): a definition with any
  of these is refused by name.
- 17B1F0 (17E060) and 1CC310's Havok blockers (1CE0E0): unreachable from
  BC5C0 at birth.

## Oracle extension (shared validator)

C0270, C0910, BC5C0, 17E060, 1CE0E0, 1D3380, D6E40, C0CD0 and C06C0 have
static closures that reach refused callees or CRT error machinery. The
validator provides two tables for this:

- `CLOSURE_STOPS` (a set): functions that `metadata().closure()` never
  admits. Executing one escapes the authenticated sites, and the observer
  rejects it.
- `REGISTER_JUMPS` (a dict, pc to targets): a register-based indirect JMP
  follows only its declared targets. Here that is 331F55 `JMP [EBX]` to
  37DC14/331F68; an undeclared target escapes the sites in the same way.

This track adds the stops {3334F0, 17B1F0, 1CC310, 126200, 1E3090, 1D62C0,
118480, 119060, C5B00, 179540} (scoped to its own routines; the composed
routines also take the attachments track's stops, see Integration) and the
331F55 targets. It also adds `JUMP_TABLES` entries for 134610 (6) and C0E78
(4), and D6E40 to `UNREACHED_VIRTUAL_OWNERS` (its `CALL ESI` creator call).
The kernels that need the stops register only when the validator has them.
