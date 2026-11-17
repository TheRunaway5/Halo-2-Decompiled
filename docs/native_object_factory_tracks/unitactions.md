# Track `unitactions` (unit action system)

Native in `src/gameplay/objects/native_object_factory_unitactions_tu15.cpp`
(API in `include/halo2/native_object_factory_unitactions_tu15.hpp`, namespace
`halo2::native_object_factory_tu15::unit_actions`); oracle cases in
`tools/native_object_factory_cases_unitactions.py`, harness kernels in
`tests/native_object_factory_unitactions_kernels.cpp`.

Build with
`-DHALO2_NATIVE_OBJECT_FACTORY_TRACKS="unitactions;units;weapon;core;nodes;deviceanim;nodebuild;placement;havok;attachments;objtypes"`
(the units track now links objtypes' `object_wake`)
and run the oracle with `HALO2_FACTORY_TRACKS=unitactions` and
`HALO2_ORACLE_STRICT_WRITES=1`.

The track links `native_tu15_wake_object_ancestors` (B9390),
`native_campaign_unit_notify_action_tu15` (1EFA70/1EEC30, in
`src/campaign/ai/native_campaign_unit_action_support_tu15.cpp`, a source of
the builtins harness target), the units track's `unit_animation_mode_listed`
(C9550) and the weapon track's `weapon_animation::choose_animation` (1D46A0).
The oracle cases import the weapon track's graph fixture builder
(`native_object_factory_cases_weapon.Fixture`).

## Action table and dispatchers

`action_table()` is the 0x473D28 table as literal TU1.5 data: 60 rows of
{descriptor, start, update, stop, interrupt}. The oracle module rechecks the
literal against the authenticated XBE every time it loads (between the
`// BEGIN 0x473D28` and `// END 0x473D28` markers).

Each dispatcher binds a callback by its address: either a native function or
a named refusal (`Native object factory: <callback> unit action <type>
<kind> callback`). A type outside the table refuses before any store.

| Owner | Native | Shape |
|---|---|---|
| E8E50 | `request(mem, unit, Request, fp)` | B9390, start, then 1EFA70 for a player unit; returns AL |
| E8E10 | `request_type(mem, unit, type, fp)` | E8E50 with a zeroed request |
| E8D80 | `update(mem, unit, fp)` | per active type: update; false clears the bit; returns "any true" |
| E8EB0 | `stop(mem, unit, type, fp)` | stop callback, bit cleared |
| E8F10 | `interrupt(mem, unit, type, fp)` | interrupt callback, bit cleared |
| E9530 | `active(mem, unit, type)` | the type's bit |

`Request` is the 0x20-byte block E8E50 receives in EDI; `Request::of(type,
flag4, flag5)` builds one. 11D670's modes 1..3 leave the tail of their
stack request uninitialized; the native passes zero there. Only +0, +4 and
+5 are read by the ported start callbacks.

Callbacks bound natively:

| Callback | Table use | Native |
|---|---|---|
| EAF00 | start of 8 and 0x12 (weapon ready) | `weapon_ready_start` |
| ED110 | start of 0x1D (vehicle exit) | `vehicle_exit_start` |
| E9870 | update of 0, 1, 0xA, 0xB | `weapon_trigger_update` (+105950 `weapon_trigger_ready`) |
| E9910 | interrupt of 0, 1, 0xA, 0xB | `weapon_trigger_interrupt` |
| EB190 | interrupt of 8 and 0x12 | `weapon_ready_interrupt` (through native queries, stops at lowering/readiness dependencies) |
| EB360 | start of 9 and 0x13 | `weapon_put_away_start` (empty hands complete; armed hands retain named refusals) |
| EC750 | interrupt of 0x1B | `action_target_clear` |
| EA970 | interrupt of 0x16 (weapon pickup) | `weapon_pickup_interrupt` (+E9E50 up to BBAC0) |
| EA970 | interrupt of 0x16 | `weapon_pickup_interrupt` (E9E50 prefix) |
| 8FDB0 | interrupt of 0x20 | empty (RET 8) |

Every other non-null callback is a named refusal.

## Entry points for other tracks and the script host

- Units track, `unit_place` (C6850): after `unit_create_starting_weapons`,
  call `unit_actions::request(mem, unit, unit_actions::Request::of(8), fp)`
  (it performs E8E50's B9390 wake itself, so drop the separate wake), then
  `unit_actions::seat_drivers_update(mem, unit, fp)` (CECF0).
- Script host:
  - `vehicle_unload`: `std::uint16_t vehicle_unload(mem, mapping, vehicle, fp)`
    (11D990: ECX mapping, EDX vehicle; returns AX).
  - `unit_exit_vehicle`: `void unit_exit_vehicle(mem, unit, mode, fp)`
    (11D670: EAX unit, DX mode).
  - `ai_vehicle_exit`: `void ai_vehicle_exit(mem, mapping, next_actor, fp)`
    (27EAB0: ECX AI reference, EBX mapping). `next_actor` is the 27CAA0/27CB30
    walk over the reference (actor row, 0 at the end); bind the host's
    `ActorWalk` in `native_campaign_script_ai_tu15.cpp`.
- Weapon readiness owners are public for the tracks that will call them:
  `unit_animation_keys` (111D10), `unit_animation_state_active` (116720),
  `unit_animation_mode_locked` (C9590), `weapon_change_blocked` (113190),
  `weapon_animation_available` (1123B0), `weapon_ready_allowed` (CFB80),
  `weapon_slot_flagged` (F0E00), `weapon_pickup_interrupt` (EA970),
  `weapon_select` (EAA60) and `weapon_ready_animation` (EAC70).
- `UnitFp` (= `NativeTu15PhysicsFrameFp`) is passed through for the callbacks
  that will need it (111B10 and the weapon readiness owners); the owners
  ported so far execute no floating point.

## Ported owners

Cases are physical-original fixture cases (seed 0x5C41), run with
`HALO2_ORACLE_STRICT_WRITES=1`. Coverage is over each original's complete
body; "reachable" excludes only instructions that follow a refused callee
(the original's run ends at that callee's entry).

| Owner | Native | Kernel (cases) | Coverage |
|---|---|---|---|
| E8E50 | `request` | `unitactions_request` (600), `unitactions_request_any` (300, every type) | 33/33 |
| E8E10 | `request_type` | `unitactions_request_type` (400) | 18/18 |
| E8D80 | `update` | `unitactions_update` (600) | 51/51 |
| E8EB0 | `stop` | `unitactions_stop` (300) | 29/29 |
| E8F10 | `interrupt` | `unitactions_interrupt` (400) | 29/29 |
| E9530 | `active` | `unitactions_active` (200) | 22/22 |
| EAF00 | `weapon_ready_start` | `unitactions_weapon_ready` (600), through E8E50/E8E10 | 103/132: all reachable (the rest follows EAC70, which ends at a refusal, or needs EAA60 to return 0, which needs a held weapon that E9910's 106670 refuses first) |
| E9870 | `weapon_trigger_update` | `unitactions_trigger_update` (400), through E8D80 | 35/35 |
| 105950 | `weapon_trigger_ready` | `unitactions_trigger_ready` (400) | 39/39 |
| E9910 | `weapon_trigger_interrupt` | `unitactions_trigger_interrupt` (300), through E8F10 | 31/31 |
| EC750 | `action_target_clear` | `unitactions_target_clear` (100) | 9/9 |
| ED110 | `vehicle_exit_start` | `unitactions_vehicle_exit` (500), through E8E50 and the script cores | 59/81: all reachable (the rest follows ED060's refused D0520) |
| ED060 | `seat_release` | `unitactions_seat_release` (300) | 28/55: all reachable (the rest follows D0520) |
| CECF0 | `seat_drivers_update` | `unitactions_seat_drivers` (600) | 188/188 |
| CB440 | `seat_occupant` | `unitactions_seat_occupant` (300) | 30/30 |
| 219B00 | `seat_mapping_admits` | `unitactions_mapping_admits` (400) | 53/53 |
| CAF20 | `object_seats` | `unitactions_object_seats` (500) | 96/96 |
| 2199C0 | `mapped_seats` | `unitactions_mapped_seats` (600) | 104/104 |
| 11D990 | `vehicle_unload` | `unitactions_vehicle_unload` (600) | 53/54: the rider count increment (11DA0F) needs ED110 to succeed (D0520) |
| 11D670 | `unit_exit_vehicle` | `unitactions_exit_vehicle` (500) | 44/50: modes 1/2 end at the refused 0x20 start ED970 |
| 27EAB0 | `ai_vehicle_exit` | `unitactions_ai_vehicle_exit` (400; squad references) | 63/63 |
| 1EFA70, 1EEC30, B9390 | existing natives (support, activation services) | through E8E50 | 14/14, 25/25, 26/26 |
| 111D10 | `unit_animation_keys` | `unitactions_animation_keys` (300; ESI/EDX outs in the track bank) | 27/27 |
| 116720 | `unit_animation_state_active` | `unitactions_state_active` (400) | 34/34 |
| C9590 | `unit_animation_mode_locked` (+ units' C9550) | `unitactions_mode_locked` (200) | 22/22 |
| 113190 | `weapon_change_blocked` | `unitactions_change_blocked` (800), through EAF00 | 325/325 |
| 1123B0 | `weapon_animation_available` (weapon's 1D46A0) | `unitactions_animation_available` (400) | 27/27 |
| CFB80 | `weapon_ready_allowed` | `unitactions_ready_allowed` (500), through EAF00 | 81/81 |
| F0E00 | `weapon_slot_flagged` | `unitactions_slot_flagged` (200) | 33/33 |
| EA970 | `weapon_pickup_interrupt` | `unitactions_pickup_interrupt` (300), through E8F10 and EAF00 | 18/18 |
| E9E50 | (EA970's `held_object_release`) | through EA970 | 52/410: all reachable (the rest follows the refused BBAC0) |
| EAA60 | `weapon_select` | `unitactions_weapon_select` (400; BL primary), through EAF00 | 55/96: all reachable (the rest follows the refused 105200 and EA9B0) |
| EAC70 | `weapon_ready_animation` | `unitactions_ready_animation` (600; AL primary), through EAF00 | 101/195: all reachable (the rest follows the refused EAB70, 112420 and D2DB0) |

All 32 kernels together give 13,400 cases (alone: PASS 13,400 tails,
26,800 native comparisons, 16,036 ordered stores). The whole builtins oracle
with this track (`HALO2_FACTORY_TRACKS=unitactions`,
`HALO2_ORACLE_STRICT_WRITES=1`; built with
`unitactions;units;weapon;core;nodes;deviceanim;nodebuild;placement;havok;attachments;objtypes`),
every kernel but the presentation builtin `object_function_value` (which
fails in its own harness ABI, outside this track): PASS 40,292 physical
original evaluator tails, 80,584 native comparisons, 183,575 ordered
original stores.

Harness notes:

- Refusals are STOPS: each case row declares every refused callee entry in
  `stop_before_entries`, and the subtrees are `closure_stops`. Rows set
  `observe_stop_entry`: the kernels report the refused owner's address as
  `stopped_at` and the oracle compares it with the entry where the
  original stopped (the harness's STOP for a normal return). The kernels
  report the original's EAX at that entry (see `boundary` in the kernels
  file): a refusal message carrying "(original AL xx)" (BBAC0, EAB70,
  112420, D2DB0, 105200, EA9B0), 106670 the slot weapon, D0520 the rider's
  seat word, a refused callback its own address.
- Every world carries an animation manager per body (+12A = 0x280, from the
  weapon track's fixture builder over graphs at track bank 0x4000..0x8000,
  zeroed so no resource is resident and 126200 is never entered), an aim
  block (+33E = 0x270; keys at body +314/+334 drawn from 113190's key sets
  and their neighbours), the E9E50 held object (action block +10: NONE or a
  live object), weapon words +16C/+17C and definition classes +288/+28C
  (weapon track key pools), +12C bit 29 and seat +4 keys and flag bit 5.
- 113190's key sets were extracted by emulating its two compare trees over
  every constant (and chained SUB sums) +/-8; the cases draw each set and
  the near misses.
- The dispatchers' register-indirect calls are declared in `REGISTER_JUMPS`
  (E8E67, E8DDE, E8EE1, E8F41) with every table callback as a target.
- CAF20 and 2199C0 fill caller stack buffers in the original. Production
  returns the entries natively (`std::vector<Seat>`); the kernels replay the
  original's buffer stores (CAF20: seat word, object, row, count word;
  2199C0: three dwords per entry, the seat dword's high half being CAF20's
  zeroed stack) so the oracle compares them.
- 27EAB0's kernel walks squad references (tag 0) with the 20CFF0/20D030
  member chain; production takes the walk as `ActorWalk`.

## What unit place gets

C6850 requests action 8 with a zeroed request. Every unit birth enters EAF00
with request +4 = 0; when the desired and current weapon slot bytes (+216 and
+212, set to 0xFF by C6440) agree, EAF00 returns 0 at once. That is every
vehicle and every unit born without starting weapons: their whole E8E50 call
(wake, EAF00, player history) and CECF0 run natively.

A unit whose starting weapons set +216 now runs natively through 113190
(+C9590/C9550), the B9390-free checks, CFB80 (1123B0 onto the weapon
track's 1D46A0), the interrupts of 0/0xA (E9910: no held weapon at birth),
0x1B (EC750) and 0x16 (EA970/E9E50: no held object), EAA60 (the current slot
is empty at birth, so it returns 1 at once) and EAC70's head: the desired
slot's weapon and 1123B0 on its classes. It then stops at EAC70's first
unported callee: 112420 (the ready animation) when an animation exists or
the unit is a vehicle, else D2DB0 for a seated unit or EAB70. (In the units
track its starting weapons still stop earlier, at B9B70 in CEF90.)

Census slice: EAF00's executed closure is 296 functions / 17,498 executed
instructions (six-map union, object-core roots excluded). Its weight is
EAA60 (278 / 16,564: EA9B0 → FC310 → B92C0/B9330 → 1CC310 Havok shape
rebuild 8,707; 116570 → 111B10 4,328) and EAC70 (104 / 7,655: EAB70 →
112420; D2DB0 → FC480). EAA60's heavy subtree runs only when the current
slot holds a weapon (a weapon switch), which E9910's 106670 refusal blocks
first today. CECF0 executes 95 of its 188 instructions and is fully native.

## Refused (not native) dependencies

| Callee | Reached from | What it is |
|---|---|---|
| 106670 | E9910 with a weapon in the slot (also EA9B0) | weapon trigger release (46k static) |
| BBAC0 | E9E50 with a held object (block +10) | object detach (core) |
| 112420 | EAC70 when 1123B0 finds an animation or for a vehicle | ready animation (121/157 executed; 115AF0, 115E90, 1D34E0, 1D5320, BC380) |
| D2DB0 (→FC480) | EAC70 for a seated unit without the animation | weapon switch effects |
| EAB70 (→112420) | EAC70 without a desired weapon | weapon unready |
| 105200, EA9B0 | EAA60 with a weapon in the current slot | weapon lowering, holster (EA9B0 → FC310 → 1CC310) |
| CABC0, F0D50, 116570, 105010 | EAA60/EAC70 after the refusals above | channel clear, weapon event, 111B10 state, weapon ready state |
| D0520 | ED060 for a seated unit | Native reset helper added separately; ED060 admission still waits for its seat-release tail qualification |
| 1C4600 (→213D30) | ED110 for an actor | AI vehicle exit speech |
| 1C49D0 | CECF0 when the driver changed | AI driver change hook |
| every other table callback | the dispatchers | e.g. ED970 (0x20 start), EB0A0 (8 update), ED430 → ECE70 (0x1E) |

Refusals reached from boolean oracle kernels name the original's AL at the
refused entry ("(original AL xx)"); the kernels report it as the result.

## Shared integration

- Units track (`native_object_factory_units_tu15.cpp`): `unit_place` now calls
  `unit_actions::request(m, unit, unit_actions::Request::of(8), fp)` and
  `unit_actions::seat_drivers_update(m, unit, fp)`; its `units_place` cases
  then stop at EAC70's callees (112420, D2DB0 or EAB70) instead of EAF00.
- Script host (`native_campaign_script_ai_tu15.cpp`): `ai_vehicle_exit`
  binds the host's `ActorWalk` around `unit_actions::ai_vehicle_exit`.

## Resumed weapon-ready qualification (2026-11-12)

The interrupted second pass had already added 111D10, 116720, C9590,
113190, 1123B0, CFB80, F0E00 and EA970, plus the admitted prefixes of
E9E50, EAA60 and EAC70. Its saved run completed after the agent stopped:
13,400 physical cases, 26,800 comparisons and 15,762 ordered stores. These
are the 22 original dispatch kernels plus ten readiness kernels. The
readiness rows of the coverage table (111D10 through EAC70, and EAF00's
103/132) were measured over all 32 kernels with the 2026-11-12 final
readiness case set; 113190 reached 325/325 once the aim keys drew every
compare-tree leaf.

The resumed mutation screen found a gap in the comparison itself: returning
zero could be indistinguishable from incorrectly reaching an unported
callee with AL zero and the same stores. Unit-action cases now also compare
the physical original's stop address against the native refusal's named
entry (or the physical STOP sentinel for a normal return). This is harness
observation only; no production behavior or original outcome is supplied.

The readiness fixtures also isolate the 113190 state/aim compare trees,
exercise armed hands with an already-active selection animation, and use
authored positive animation graphs for CFB80/EAF00. Those graphs distinguish
an explicit mode, the current-mode placeholder, a seat mode and a missing
mode. The focused positive-query proof passed 1,100 original cases, 2,200
comparisons and 1,649 ordered stores with strict writes and stop-entry
comparison. Evidence is under
`build/test-results/resume-unitactions-20261112/`.

The completed out-of-tree mutation screen used 5,100 relevant physical
original cases, a passing 10,200-comparison baseline, and 52 mutated source
copies. Fifty mutants were rejected directly. An additional 500-case
physical control isolated C9590's three-key check from C9550's lock list and
rejected its formerly masked mutation. Thus 51 of 52 mutations are detected
across these two proofs. The one remaining control has the read-trace limit
below. The native source stayed unchanged, both proof cohorts' input hashes
were stable, and all owned compiler/reference scratch was removed. See
`build/test-results/resume-unitactions-20261112/closure.json`.

The `1123B0 flags 2 -> 0` mutation removes repeated reads of the same failed
lookup because its explicit state key is already 0x400000C. Returns and
stores on immutable banks do not distinguish it; read/fault-trace equivalence
is not established. Production retains the original flags value 2.

## Continuation plan

### Resumed dispatch and seat-reset qualification (2026-11-13)

The previously interrupted readiness proof was already complete. The next
increment adds native `weapon_ready_interrupt` (EB190) to the action 8/0x12
interrupt slots, and `weapon_put_away_start` (EB360) to the action 9/0x13
start slots. An inactive ready action now returns normally. Put-away of an
empty hand returns false, and an armed hand follows CFB40 and the original
interrupt sequence until a named unported dependency. CFB40 reuses the
units track's `unit_dual_wielding`.

`seat_weapon_actions_reset` (D0520) composes the five interrupts, action
0x13 request and `weapon_channels_clear` (116920). The latter preserves all
28 individual stores, including the different order of the two channels.
These helpers have direct physical-original cases. ED060 still refuses at
D0520 until its complete seat-release continuation is qualified together;
vehicle exit completion is not established by this increment.

New direct kernels cover EB190 (500 cases), EB360 (600), 116920 (300,
including signed aim offsets), and D0520 (500, including complete unarmed
paths with and without player history). As before, named refusals, return
values, final banks and ordered stores are compared against unchanged
original execution. Armed EB190 stops at 105200 after the native 116720
query. Armed EB360 stops at 106810, a refused interrupt dependency, or
EA9B0; the remainder after holstering remains unported.

The default all-track harness build and its CTest self-test pass. The
physical-original cohort passes 6,200 cases, 12,400 native comparisons and
27,183 ordered stores across the four new kernels and nine affected
dispatch/script kernels. All nine out-of-tree mutation controls are
rejected. Original instruction coverage is EB190 59/84, EB360 102/156,
D0520 35/35 and 116920 40/40; the unexecuted callback tails remain behind
the named refusals above. An independent source/disassembly review found
no actionable issue.

Evidence and cleanup receipts are under
`build/test-results/resume-unitactions-20261113/closure.json`. The first
oracle attempt was blocked by sandbox ptrace restrictions; the next
identified a fixture overlap with the harness's excluded HS argument
staging region. Restricting the first body's aim offset removed that alias
while retaining negative-offset cases on other bodies. Native source was
unchanged by that correction. Both failures are retained as bounded logs.
All owned builds, mutant binaries and raw reference banks were removed;
no large artifacts were retained. This is component qualification, not
whole campaign or launcher acceptance.

Leaves first, each with fixture cases, coverage and mutations:

1. EAC70's callees, in birth order: 112420 (121/157 executed: 115AF0,
   115E90, 1D34E0, 1D5320 on the weapon track, BC380 on units), D2DB0 (→
   FC480 → FC3B0/BAF10, core roots), EAB70 (→ 112420), then 105010 (EDI
   weapon, stack flags; B9390, 109150), F0D50 (BCDF0, 16B736), CABC0
   (BCDF0, 106040) and 116570 (111B10 on units, 111D10 native, 1D37D0).
   EAC70's tail after D2DB0 is already written (the +212 slot, +228 tick,
   F0E00, the +1C8..+1E8 stores, the 116570 keys) and gets exercised as the
   callees land. Its fixtures need the units track's aim block layout for
   111B10 (body +300) rather than this track's +270.
2. Integrate the separately qualified D0520 reset into ED060. EB190 is now
   native for inactive actions and up to 105200/EAC70 for active ones. Its
   armed tail still needs 105200, CABC0,
   F0D50, 116570, EA9B0 with a weapon, or EAC70(0x7000101, 0, 0) without),
   E9910, EC750 and E8E50(0x13) are composed by D0520. EB360 now returns AL 0
   when its hand holds no weapon and runs CFB40 and the six interrupts;
   its armed continuation still needs 106810, EA9B0,
   D0E10, EAC70 and BCDF0/197F2C. D0520's 1EFA70 and 116920 are native.
   Then ED060's tail: ECE70 for vehicles
   (type byte +AA 1), else the units track's 111B10 (flags 2; 0x7000101 x3,
   0x400004A, 0, 0), BBC90, B2D40 and D37F0; ED110 then succeeds natively and
   `vehicle_unload` counts riders. ED060 needs 111B10 under unit fixtures.
3. 106670 (weapon trigger release) unblocks E9910 for armed units and with
   it EAA60's weapon switch subtree: 105200, CABC0, F0D50, 116570, EA9B0 →
   FC310 → 1CC310 (call havok's `havok_component_state_flags`, which still
   refuses deeper in Havok).
4. ED970 (action 0x20) for `unit_exit_vehicle` modes 1/2.
5. ED430 → ECE70 (0x1E seat detach: 1115D0, D37F0, BBAC0/BF050,
   CE230/D0390/EAC70/F09C0, 1C4650/1C48A0) leaves first; it also unblocks
   CEA70 (unit detach) for the 03b custom-animation fault.

## Mutations

Each mutant is compiled from a scratch copy of the source with the kept
tree's flags, linked into a scratch binary and run against its kernels under
strict writes; an unmutated control passes, and the in-tree source hash is
verified unchanged afterwards (the kept binary is newer than the source).

Weapon readiness batch (2026-11-12): 52 mutants, 51 rejected, one
return/store-equivalent control (read/fault traces are not established).

- Rejected: 111D10's graph offset, handle word, mode key and failure value;
  116720's +C4 key and manager key; C9590's key list and C9550 call;
  113190's primary and secondary rounds tests, overlay bit, player gate,
  overlay answer, state 0x5000024, the 0x5000768..B range, C9590 result,
  aim overlay keys and aim unloaded test; 1123B0's result test (high word);
  CFB80's seat bit, key default, vehicle type, definition bit, player gate,
  class order and seat key; F0E00's bit 2 and index; E9E50's idle value,
  +14 word, held bit 17, +C0 bit and boundary AL; EA970's clear and
  binding; EAA60's empty result, slot, key, 116720 gate, holster gate and
  105200 AL; EAC70's other hand, empty-slot AL, hand loop, other class,
  vehicle, seat test and 112420 AL; EAF00's EAC70 key, 113190 gate and
  CFB80 gate.
- Equivalent: 1123B0's 1D46A0 flags 2 versus 0. Flag 2 retries the lookup
  with 0x400000C, which is already 1123B0's fourth key, so the retry repeats
  the same search (no stores).
- The first screen left 113190's secondary rounds test (`> 0` versus `>= 0`)
  and 1123B0's result test (`!= NONE`) alive; the 113190 cases now arm a
  player's two hands with rounds around 0 under an "unloaded" key, and the
  1123B0 cases answer an exact query with handles whose high word is
  0xFFFF. Both are rejected on rerun.

First batch (dispatchers, CECF0, seat queries, script cores): 66 source
mutations, 61 rejected, five survive:

- Rejected: E8E50's wake, history gate, history type and result, live-unit
  test; E8D80's bit clear, result and null-update skip; E8EB0's bit; E8F10's
  callback/clear order; the action word bank and E9530's bit; EAF00's slot
  compare (through E8E50's player history), slot index, flag byte, interrupt
  order, NONE-weapon result and empty-slot test; E9870's trigger map;
  105950's bounds, any-trigger test and stride; E9910's slot; EC750's offset;
  ED110's three action bits, interrupt order and +258 store; ED060's seat
  test; CECF0's bit 26 gunner, parent type byte, free-gunner rule, bit 25,
  alternate bit 11, bit 13, player gunner, pass 0 0x20 test, pass 1 0x1F
  test, alternate seat's driver flag, +134 driver gate, driver-gunner rule,
  recursion and hook; CB440's type mask; 219B00's NONE mapping, count and bit
  bank; CAF20's seat limit, child bit 26 and +10A bit; 2199C0's filters 0/2,
  occupancy and limit; 11D990's started count; 11D670's seat gate; 27EAB0's
  mapping gate, seat test and NONE unit.
- Equivalent: E9870's slot test `type > 1` versus `> 2` (only types 0, 1,
  0xA, 0xB reach it); CAF20's count check before the child loop (each
  recursion repeats it first); 11D990 asking 2199C0 for occupied seats (it
  skips empty seats itself through CB440).
- Unobserved: 11D670's mode 1 flag and mode 3 flag. The request flag is read
  only by ED970 (refused) and by ED110 after D0520 (refused).
