# Track `weapon`

Roots: the type table's item "new" slot FA820 (→ FD100 → B96B0) and weapon
"new" slot 102AD0, with everything 102AD0 executes. The other item/weapon
slots (+30, +40, +4C) are not on the factory's birth path and are not ported.

Sources:

- `src/gameplay/objects/native_object_factory_weapon_tu15.cpp`
- `include/halo2/native_object_factory_weapon_tu15.hpp`
- `tests/native_object_factory_weapon_kernels.cpp`
- `tools/native_object_factory_cases_weapon.py`

The factory callbacks live in `halo2::native_object_factory_tu15`. The
animation-manager subtree under 1D5320 lives in the nested namespace
`weapon_animation`. That nesting keeps the symbols apart from any other
track's port of the same routines, because 1D3380 (track `modelstate`)
executes the same subtree.

## Ported owners

Cases are physical-original fixture cases per kernel, at seed 0x5C41.
Coverage is the share of the owner's original instructions that the
track's cases execute. The uncovered sites are listed under "Refusals and
unreached sites". A mutation counts only when the oracle rejects it.

| Owner | Native | Cases | Coverage | Mutations |
|---|---|---|---|---|
| 102AD0 | `weapon_new` | 300 | 111/111 | 3 |
| 10A980 | `weapon_set_state` | 400 | 97/121 | 2 |
| 106FF0 | `weapon_owner` | 60 | 11/11 | 1 |
| 1D5320 | `weapon_animation::manager_set_state` | 800 | 322/362 | 6 |
| 1D46A0 | `choose_animation` | 400 | 593/613 | 1 |
| 1E3420 | `animation_lookup` | 300 | 967/1007 | 1 |
| 1D45A0 | `resolve_keys` | 250 | 76/76 | 1 |
| 2A0100 | `mode_iterate` | 300 | 101/101 | 1 |
| 1E33D0 | `mode_entry` | 150 | 28/28 | 1 |
| 1E5810 | `tag_block_search` | 200 | 42/42 | 1 |
| 1E3270 | `graph_child` | 150 | 17/17 | 1 |
| 1E3150 | `graph_animation` | 200 | 52/52 | 1 |
| 1E5C80 (+1E5AF0) | `graph_require_resident` | 200 | 74/76 (+17/19) | — |
| 1E5E40 / 1E5F20 / 1E5FB0 | `mode_prefetch`, `mode_prefetch_fallbacks`, `graph_prefetch` | 120 / 100 / 100 | 77/81, 49/49, 57/57 | — |
| 1D6430 | `manager_prefetch` | 100 | 30/30 | — |
| 1D5050 | `physical_query_state` | 250 | 155/231 | 1 |
| 1E4B70 | `physical_query_select` | 200 | 82/544 | 1 |
| 1E3320 | `physical_query_find` | 250 | 69/69 | 1 |
| 1D4F80 | `physical_query_selector` | 250 | 74/74 | 1 |
| 1D4E50 | `manager_reset_channels` | 40 | 47/47 | 1 |
| 1D36C0 | `manager_start_channel` | 300 | 49/49 | 1 |
| 1D3660 | `manager_variant` | 150 | 29/29 | 1 |
| 1E58E0 | `random_variant` | 400 | 113/114 | 3 |
| 1E5880 | `variant_alias` | 200 | 33/33 | 1 |
| 1CE720 | `channel_start` | 150 | 43/43 | 1 |
| 1CE810 | `channel_set_frame` | 400 | 50/50 | 2 |
| 1CEC60 | `channel_mark_events` | 300 | 168/168 | 2 |
| E8D50 | `channel_stopped` | 60 | 16/16 | 1 |
| B8780 | `channel_animation` | 80 | 12/12 | 1 |
| B8760 | `channel_valid` | 60 | 8/8 | 1 |
| 1CF5B0 | `channel_finished` | 100 | 18/18 | 1 |
| B9390 | existing native `object_mark_for_update` | (via 10A980) | 26/26 | — |
| FA820 | `item_new` | none | not physically provable | — |
| FD100 | `item_restore_scale` | none | not physically provable | — |

The track has 8,320 physical cases (16,640 native comparisons), counting
the two first-person kernels added on 2026-11-16 (see
[MCC first-person query names](#mcc-first-person-query-names-2026-11-16));
it had 7,320 before them. The whole
builtins oracle, with the base factory owners and this track
(`HALO2_FACTORY_TRACKS=weapon`), passes 34,260 physical cases with 68,520
native comparisons and 246,303 ordered stores.

The mutation screen has 40 mutations, from one to six per owner. The
oracle rejects all 40 on the final fixtures. Before that, five survivors
showed fixture gaps, which are now closed:

- mixed-sign keys for 1E5810's signed search;
- exact physical-query sub-rows for 1E3320's third byte;
- NONE graph tags with live handles for B8760;
- a planted draw-equal weight for 1E58E0's inclusive test;
- end-of-animation frames for 1CE810's 30 Hz look-ahead.

One proposed 1E4B70 mutation swapped the key-guard test. That mutation is
equivalent, because every non-matching path yields NONE and a match is
refused. The screen replaces it with a mutation that drops the guard's
store.

The prefetch owners (1E5C80, 1E5E40, 1E5F20, 1E5FB0, 1D6430) have no stores
and no return value. Their only effect is the 126200 request, which is
refused, so a mutation of them cannot be observed. Their cases prove that
each one completes on the same inputs as the original.

## MCC first-person query names (2026-11-16)

### Root cause of the Elongation divergence

On Elongation 4p (scripted input) the native 169863 -> 1D5320 found no
animation for user 2's state keys 0xC0005AE, 0x140005B0 and 0xB0005B1 on
graph eff750ea and fell back to the idle state 0x400000C (animation 1). The
translated route found animations 11, 10 and 12. See
[nodebuild.md](nodebuild.md#production-169c46-and-the-callers).

These keys are TU1.5's `reload_enter`, `reload_continue_full` and
`reload_exit`: a staged reload. The MCC cache names them 0x0C0005BA,
0x140005BC and 0x0B0005BD, so the graph's mode leaves hold those names.

The translated route does join them, but not through
`source_object_function_symbols_tu15.inc`, which is where the earlier audit
looked. The join is in a different mechanism:

- `tools/generate_physics_live_support_tu15.py` emits a
  `SourceWeaponAnimationQueryScopeTu15` at the entry of the translated
  1D5320 (and of 1D3D30 and 116570). It also wraps 16AD45's calls at
  16B0D1, 16B0E5, 16B108 and 16B128. The scope is named by the caller's
  return address and the entry key (EAX). The join itself is in
  `src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp`.
- The generator replaces two copied search keys:
  - 1E3420's action-record search key, pushed at 1E34A3:
    `source_weapon_animation_scoped_query_symbol_tu15`;
  - 2A0100's class-record search key, pushed at 2A019A:
    `source_weapon_animation_scoped_class_symbol_tu15`.
- Inside the scope of 169863's call (return 169A56), the 1E34A3 key becomes
  the cache name of the state key that 1D5320 entered with, when that key
  is one of twelve staged-reload and melee names.
- In that scope, and in 16AD45's four channel lookups, the 2A019A key
  becomes the cache's dual class (0x400054B -> 0x4000557).
- Every other key keeps TU1.5's namespace: the manager's +70..+7C, the
  channel key, the iterator's searched-key outputs and 1D45A0's dual
  exception. Any other 1D5320 caller opens a scope that joins nothing and
  hides an outer one.

The native 1D5320 subtree had no such join. It searched 0xC0005AE in a leaf
that holds 0xC0005BA and failed. Because 169863 passes flags 0x82, it then
retried 0x400000C.

### Fix

`weapon_animation` now has the same join, written as a scope rather than as
operand rewrites:

- **The names.** `first_person_state_names` (the twelve staged-reload and
  melee states) and `dual_class_name` hold the TU1.5 and cache IDs with
  their names. They are the same pairs as the translated route's join,
  which checks every cache name against the cache when the world loads.
- **`QueryNamespace(bool mcc)`.** A thread scope that the live binding
  opens. It is on for MCC banks, from `cache_owner.is_mcc()`, which is the
  same condition under which the translated join's registry is bound.
  Without it, as in the oracle's ordinary kernels, every key is unchanged.
- **`QueryOrigin` and `QueryScope`.** `manager_set_state` takes an origin
  (default `other`) and opens a scope keyed by its state key. 169863's call
  passes `first_person_state`. 16AD45 opens a `first_person_channel` scope
  around its 1D37D0 and three 1D3C80 lookups.
- **Where the join applies.** `QueryScope::action_search_key` sits at
  `animation_lookup`'s action search (1E3420). `QueryScope::class_search_key`
  sits at `mode_iterate`'s class search (2A0100). Because of that,
  `physical_query_select` (1E4B70) and deviceanim's `overlay_lookup`
  (1E3F30) follow the class join as well, as their translated copies do.

### Oracle

The physical original has no MCC names. For the joined path, a case may
declare `query_names`. The observer
(`tools/observe_native_campaign_ai_vector_math_tu15.cpp`) then applies the
translated route's two joins to the unchanged original:

- It opens scopes at the 1D5320 and 1D3D30 entries and at 16AD45's four
  channel calls.
- It swaps the pushed register at 1E34A3 or 2A019A under the translated
  route's conditions, then restores it after the push.
- It reports every join it applies (`query_joins`; the validator and the
  composite runner print the count).
- It refuses the translated route's other join sites (116570, 112401,
  1125DB, 16B3F1 and 115AF0's operands) instead of running them unjoined.

The cases read the join table from the translated route's source
(`translated_query_names()`), not from the native header.

New kernels (seed 0x5C41):

| Kernel | Entry | Cases | Shape |
|---|---|---|---|
| `weapon_manager_set_state_first_person` | 1D5320, returning to 169A56 | 600 | MCC-named graphs; `query_names`; the native under `QueryNamespace(true)` |
| `weapon_manager_set_state_first_person_tu15` | 1D5320, returning to 169A56 | 400 | TU1.5-named graphs; no joins |

Both kernels use the failing case's shape. The state key is one of three
first-person names, sometimes its cache ID, 0x400000C or another state.
The mode keys are 0x7000101, 169432's dual or "any" class, and 0x7000001.
The flags are mostly 0x82. The fixtures also include:

- leaves with those names, plus one stray name in the other namespace;
- class rows of both dual IDs;
- 1D4F80 selectors that chain a state key to itself.

The fixture pools are per case (`Pools`). The existing kernels' fixtures and
random streams are unchanged.

### Proof

- **Weapon track** (strict writes, `HALO2_FACTORY_TRACKS=weapon`, every
  weapon kernel): `PASS 8320 physical original evaluator tails, 16640
  native comparisons, 112468 ordered original stores`, with 1,106
  translated-route joins applied (previous: 7,320 cases). Without
  `--only`, a weapon-only run now stops at the base kernel
  `object_function_value` (10FB70): the attachments harness registers a
  BCB70 kernel under the same name. This name clash is not part of this
  change.
- **Weapon and its dependant tracks** (strict writes,
  `HALO2_FACTORY_TRACKS=weapon,attachments,modelstate,objtypes,units,unitactions,deviceanim,havok`,
  the base kernels included): `PASS 128796 physical original evaluator
  tails, 257592 native comparisons, 2032317 ordered original stores`, with
  1,078 joins applied.
- **Nodebuild's first-person kernels on MCC caches** (169863, 16AD45 and
  169C46; see [nodebuild.md](nodebuild.md#mcc-first-person-query-names-and-the-169c46169863-default-2026-11-16)):
  `PASS 1800 physical composite cases, 204448 ordered original stores`,
  with 388 joins applied.
- **ASan/UBSan** (`-DHALO2_SANITIZERS=ON` harness, `halt_on_error=1`, the
  nine first-person composite kernels): `PASS 5800 physical composite
  cases, 1045944 ordered original stores`, with 434 joins applied and no
  report.
- **Mutation screen** (out of tree, against the kept sanitizer harness):
  15 mutants of the join, each run through the composite runner (the three
  MCC kernels, gold cache) and the validator (the two first-person weapon
  kernels). The in-tree sources were checked by SHA-256 afterwards
  (weapon f73be819..., nodebuild 342ce717...).
  - Round 1 rejected 12 of 15.
  - The fixture was then strengthened: some MCC leaves list a pending
    resource, so the prefetch path's keys are compared at the 126200
    boundary.
  - Round 2, on the final fixture, rejected 13 of 15. The rejected mutants
    are: either join off; the action join for any key of the scope, which
    the self-chained 1D4F80 selector kills; the class join only in state
    scopes; the class join of any class; the namespace ignored; the
    namespace always on, which the TU1.5 kernel kills; the searched-class
    output joined, which round 2 kills; the scope keyed by a0; no scope
    in 1D5320; 169863's call unqualified; 16AD45's lookups unqualified;
    16AD45's overlay trials unqualified.
  - Two mutants are equivalent:
    - The action join in channel scopes. No 1E3420 search runs in a
      `first_person_channel` scope, because 1D37D0 and 1D3C80 search
      through 1E3F30, and that scope's state key is NONE.
    - Other scopes not hiding outer ones. No native closure opens an
      `other` 1D5320 scope inside a qualified one: 169863's call contains
      no nested 1D5320, and 16AD45's channel scope contains only
      1D37D0/1D3C80. The masking is kept to match the translated route.

### Other native callers of 1D5320, 1D46A0 and the joined searches

The table compares each native caller with the translated route's scope at
the same site. "Live" means the caller is bound in production:
`physics_live_world_tu15.cpp` (the node-rebuild sites).

| Native caller | Original site | Translated join there | Live | Status |
|---|---|---|---|---|
| nodebuild `first_person_state` (169863 -> 1D5320) | 169A51, returns to 169A56 | state names and dual class | yes: 16B7F5 and, inside 169C46, 16961F and the fidget, plus 16AD45's 0x400000C entry | **was affected** (the Elongation divergence); fixed |
| nodebuild `first_person_rebuild` (16AD45 -> 1D37D0 and 1D3C80 x3 -> 1E3F30 -> 2A0100) | 16B0D1, 16B0E5, 16B108, 16B128 | dual class | yes: inside 16A4DD at 16A479, and at 168D40, 16B1AD and 16B78B (native by default since 2026-11-15) | **was affected** with a dual-wielded weapon (169432's 0x400054B; no sweep dual-wields); fixed |
| nodebuild `first_person_rebuild` -> modelstate `animation_manager_setup` (1D3380 -> 1D5320, labels 0x7000001) | 1D3380's call | none (the scope joins nothing and hides outer ones) | yes (through 16AD45) | unaffected |
| nodebuild `first_person_update` (169C46 -> 1D37D0 motion overlays 0x60000D8/0x6000060, 1D4510) | 169C46's own calls | none (not wrapped) | yes (default since 2026-11-16) | unaffected |
| nodebuild F2CC0 (`manager_overlay_trial`, `manager_aim_handle`), BF050's orientation hooks | F2CC0's own calls | none | yes | unaffected |
| units `unit_animation_state_set` (111B10 -> 1D5320) | 111BCE | body action names only under 116570's fallback chain (body_playback) | no (unit birth; native 116570 refuses) | unaffected for its current callers (C6440, F10E0) |
| units `unit_animation_channels_refresh` (115AF0 -> 1D3C20 -> 1D46A0, 1D3D80) | 115CCF..115D83 operands | arm_offsets 0xB00054A, grip 0x400060B and dual 0x400054B as the cache's names, without a scope | no | **would differ on MCC caches** if bound live: it searches TU1.5's names |
| unitactions 1123B0 (`choose_animation`) | 112401, returns to 112406 | dual class when its class argument is 0x400054B | no | **would differ** for the dual class on MCC caches if bound live |
| unitactions 112420 (1D5320 at 1124E3, 1D34E0 at 1125DB) | 1124E8, 1125E0 | dual class | no (refused natively) | not native |
| deviceanim `overlay_lookup` (1E3F30) | 1E3FB3 | overlay key, only under 116570's body_overlay scope | no | its class search follows the scope (shared `mode_iterate`); the 1E3FB3 join has no native caller |
| objtypes 10F710 (1D5320, key 0xE0000C2) and weapon `weapon_set_state` (10A980 -> 1D5320) | their own calls | none | no | unaffected |
| 16B31B (melee and staged-reload timing, 16B3F1, 1D3D30 at 16B3FD) | by 16B31B's caller | state names, dual class | n/a | translated only |

The units and unitactions rows need the same treatment when those owners
are bound live: `QueryScope` for the scoped joins, and named cache literals
for 115AF0's unscoped operands.

## Frame words

Several owners write their results into the caller's stack frame. These
are 2A0100's iterator and outputs, 1D45A0's keys, and the out-parameters of
1E3420, 1D46A0, 1E3320, 1E4B70, 1D5050, 1D4F80, 1E5880, 1E58E0 and 1D3660.
`weapon_animation::FrameWord` holds such a word. Natively the word stays
private. An oracle kernel binds it to a bank address, so every frame store
the original makes is compared in order. Production callers never bind
frame words.

## Floating point

All SSE steps (ADDSS, SUBSS, MULSS, DIVSS, CVTSI2SS) run the original
instruction under the live MXCSR. COMISS and UCOMISS outcomes use
`sse_compared` (DAZ). The helpers are in
`src/campaign/scripting/native_campaign_script_fp_tu15.hpp`. The random
variant's FILD/FMUL/FSTP runs as one x87 asm block under the live control
word. The APIs take `NativeCampaignScriptFloatingPoint`. The cases sweep
the validator's eight x87/MXCSR control pairs.

## Refusals and unreached sites

Named refusals (`detail::refuse`):

- `126200`: the animation resource request. It covers the 1E5AF0 tail call,
  1E5C80, the 1E5E40 loops, and the inlined prefetch loops of 1E3420 and
  1D46A0. It fires when a resource row's +10 byte is set. The census never
  executes it, and fixtures keep resources non-resident.
- `1E4C39`: 1E4B70's physical-query match selection (462 instructions). The
  census never executes it, and fixture query rows never match the 0xFF byte.
- `1D55AA`: 1D5320's second (blend-query) channel, which calls 1D3660,
  1CE720, 1CE810, 1CF5B0 and then 1D62C0. It needs a non-NONE 1D5050
  handle, which only the 1E4C39 path produces.
- `CFB40`: in 10A980, the owner variant (0x400054B) when 106FF0 finds an
  owner (body +12C bit 0 and +154). This also covers the 10AAEE retry.
- `CC1E0`: in 10A980, the owner notification under the same condition.
- `10AA06`: in 10A980, a state above 10 (an unchecked jump table).
- `B96B0`: in FD100, when no scale service is supplied (see below).

Original sites that no case executes, besides the refusals above:

- 1D5050 1D51FA–1D529B, 1D52BA–1D52DA and 1D52E1–1D52F0. These run only
  after a 1E4C39 match.
- 1D5320 1D546B/1D546F. They need 1D5050's blend flag, which only 1D51FA
  sets.
- 1D5320 1D5515–1D5522. They clamp a phase that is already in [0, 1]
  (x/span with x in [0, span]), so they are dead.
- 1D5320 1D5731–1D5737. These tests contradict the branch that reaches
  them, so they are dead.
- 1D5320 1D577A. A started channel always has a graph and an animation by
  then, so this site is unreachable.
- 1E58E0 1E59C1 (FADD 2^32). It handles a negative draw, but the draw is a
  16-bit value after SHR 16, so the site is dead.

## Dependencies on other tracks

- FA820/FD100 call B96B0. B96B0 is native in
  `include/halo2/native_tu15_object_scale.hpp`, but its refresh service is
  BF050 (track `nodebuild`). `item_new`/`item_restore_scale` therefore take
  an `ItemScaleService` (object, scale bits, seconds bits). Production binds
  it to `native_tu15_object_scale(..., seconds 0, ...)` with its BC380/BF050
  services. The leaf oracle cannot run FA820 or FD100, because their
  physical closure is the whole B9B70 closure through BF050 (2,209
  functions with 329 indirect-call owners). They need the coordinator's
  live differential harness, or a stop-capable oracle.
- B9390 uses the shared native
  `native_campaign_script_tu15::object_mark_for_update`.

## Oracle notes

- The cases register `UNREACHED_VIRTUAL_OWNERS` for the indirect-call owners
  inside the refused 126200, 1D62C0 and CC1E0 closures. They also register
  `JUMP_TABLES` 0x10AB20 (11), 0x1A3BFC (4), 0xCC30C (8) and 0xCC32C (8).
- `weapon_set_state` and `weapon_new` (10A980 and its caller 102AD0) declare
  per-routine `closure_stops` for the refused 126200, CC1E0 and CFB40
  (`OWNER_STOPS`). CC1E0 reaches E8E50, whose start dispatch the unitactions
  track declares (`REGISTER_JUMPS[0xE8E67]`); with every track loaded, the
  closure otherwise grows through every action callback to unbounded
  indirect calls (10DF30, 8D310, 1D97F0). With every track loaded, both
  kernels and `units_place` pass: `PASS 1100 physical original evaluator
  tails, 2200 native comparisons, 32542 ordered original stores` (strict
  writes, 2026-11-12).
- The validator packs at most eight argument words. 1E3420, 1D46A0, 1E3320
  and 1E4B70 therefore repeat words in their stack lists (see
  `ROUTINES` in the cases module).
- `HALO2_WEAPON_CASE_SCALE` scales the case counts for diagnostic screens.
  Proof runs use the default of 1.
