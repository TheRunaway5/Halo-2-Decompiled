# Native campaign subsystem scope (2026-11-11)

Scoping results for the four remaining campaign blockers: the object factory,
the AI frame owners, the animation pipeline and unit actions. The user asked
for all four, ported natively ("All of the above"). The factory work itself
is tracked in [NATIVE_OBJECT_FACTORY.md](NATIVE_OBJECT_FACTORY.md).

Sources: TU1.5 XBE disassembly and decompile, instruction censuses of runtime
`17a52de387e14a4d8ca388371353014f` (01a, 03a, 03b, 05a, 08b), and static
closures from `complete_body()` (direct calls and tail jumps; indirect calls
counted, not followed). Field names marked "(inferred)" are not proven.

## The shared object core

One strongly connected component of 145 functions (20,085 instructions)
links almost everything:

- B9B70 (`object_new`)
- BF050/BF0C0/BFF20 (node rebuild)
- B95D0/B9460 (placement)
- BA630 (cluster reconnect)
- BBAC0 (detach) and BAF10/BB3E0 (attach)
- CEA70 (unit detach)
- B96B0 (scale)

Example cycle: BF050 → BFF20 → BA630 → 1496F0 → … → 17D730 → B9B70 → BF050.
Effects that create objects close the loop. Any owner that reaches the core
inherits its static closure: 2,208 functions, about 181k instructions.

What actually executes is much smaller. Across the animation and unit-action
paths, 810 functions and 29,975 executed instructions ran; about 80% of that
is shared with the factory's own executed slice. The core owners are
therefore ported once, through the factory, and every other subsystem
refuses them by name until then.

## AI frame owners

### 1BBEE0 (01a, behavior row 109 "perform")

Status 2026-11-12: the 1BBEE0 root, the 26A2E0 root, the 26B220 type-4 slice,
the complete 26BA30 candidate collection (13 functions), the 26A380 prefix
(complete without candidates), 1BC0A0 and 1ECD40 are native and proven. With
candidates, 26A380 refuses by name at `26A70D → 2013B0` (or `26A6FB → 26A846`
when request +56 is set). 26C3C0 with 1FC540/1FC9B0/200D30 is native and
qualified too (2026-11-12); it refuses next at `27A640` (route smoothing) and
`2013B0`. Details, the 01a live result and the continuation plan are in
[NATIVE_CAMPAIGN_AI_AREA_PERFORM_TU15.md](NATIVE_CAMPAIGN_AI_AREA_PERFORM_TU15.md).

- Reached at `1B0AD9` through the descriptor `0x48A9D8` slot `+0x40` (row 109
  of the table at `0x47B478`). Arguments: `[esp+4]` actor, `[esp+8]`
  behavior state. Returns with `RET 8`, result in AL.
- Logic:
  1. An inactive actor (`+0x40` clear) returns 1.
  2. With a current goal (`+0x418`) and path mode 2, or a remaining
     distance (200C00) under 0.6, it runs 26C510 and area choice 1BBBD0.
  3. Takes a 1D2D90 workspace and fills a 0x758-byte type-4 request.
  4. Selects a goal with 26A2E0 (a 61 KiB frame). If it finds none, it clears
     `+0x40` and returns 0.
  5. Publishes through 26C3C0 (1FC540/1FC9B0, then 200D30 movement).
- Closure: 523 functions / 55,692 instructions. The AI-specific part is
  about 222 functions / 21.4k instructions. The rest is animation pose and
  markers (112090, 23.3k), the collision ray into Havok MOPP (16C620, 7.7k)
  and unit points (CD9E0, 3.3k).
- Already native: 200C00 (wired at `1BBF3B`), 26C510, 1BBBD0 (API only),
  26C600, 26C850, 277D40, 16E940, plus the navigation, jump and perception
  leaves.
- Private candidates under `analysis/campaign-native-ai-actions-20261030/`
  cover 43 closure functions (5,177 instructions) and are not yet promoted.
- Port order:
  1. 1BC0A0 (the descriptor's `+0x48`, 14 instructions; probably the next
     missing owner) and 1D2D90/1D2E30.
  2. Promote the private candidates.
  3. Goal selection: 26BA30, 26B220, 26A380, 26A2E0.
  4. Movement and publication: 200D30, 27A640 with route smoothing, 2A8440,
     1FC540/1FC9B0, 26C3C0.
  5. The 1BBEE0 root.
- Binding: add a `native_X_owns_call` / `native_X_run` pair to
  `required_entry_original()` in `src/physics/physics_live_world_tu15.cpp`,
  using `native_campaign_ai_area_choice_tu15_binding.cpp` as the template.
  Check the call site and return address, honor the floating-point state,
  set AL and pop `sp+12`.

### 29D840 (05a, per-flock update)

- Reached at `29D5C0` from 29D540 (walks the flock array `[0x52D88C]`, 0x28
  rows). EBX holds the flock handle; plain `RET`.
- Logic:
  1. Refresh boid flags (29D5D0).
  2. Spawn boids on a timer (29CE40). Random draws come from the
     `*[0x4F3CD8]` LCG; the spawn uses `fsin`/`fcos`, B9960 placement data,
     a B9B70 creature and the 29D3C0 link.
  3. Compute the sink mask (29D730).
  4. Delete boids that are leaving (BA570); steer the rest (29E090, which
     ends in B95D0).
- The flock code itself is small: 12 functions, 2,204 instructions, no
  indirect calls. Its whole closure (2,220 functions, 183k) comes from B9B70,
  BA570 and B95D0. The "~39k" figure in earlier notes was 1D0670's size.
- Flocks exist only after the new-BSP handler 1CFE60 → 29CC20. Headless 05a
  (1,500 ticks, dummy video) created none, so reproducing it needs the hidden
  window run.
- Port order:
  1. Leaves: 29F2C0, 29D3C0, 11B6C0, 11B670, 27C800, 29F1A0, 29F310, 29DD60,
     29DA40.
  2. 29D5D0 and 29D730.
  3. 29CE40 and 29E090.
  4. The 29D840 root.
- Hard dependencies: creature construction in B9B70, a wired BA570, and
  native B95D0.

## Animation pipeline

| Builtin | Owner | Executed slice | Core needed |
|---|---|---|---|
| `device_set_overlay_track` | 10D5E0 → 10D870 | 12 fns / 1,164 | no |
| `device_set_position_track` | 10CF80 | 328 / 19,024 | B95D0, BF050 (moved roots) |
| `custom_animation` (4 variants) | 11DC00 → 1128C0 | 384 / 22,925 | with an anchor: CEA70/BBAC0, B95D0, BA630, BF050 |
| `scenery_animation_start` | 10F710 | 357 / 20,897 | as above |
| `objects_attach` | 10FFD0 → BAF10 | — | yes |
| `object_set_scale` | 110030 → B96B0 (native) | — | BC380, BF050 |

Device layout:

- `+0x12C` flags: 4 moved, 8 position track, 0x10 overlay.
- `+0x150` track speed.
- `+0x18C` position channel and `+0x1AC` overlay channel, 0x40 bytes each.
- The animation manager is at `body + word[+0x12A]`.

Port order:

1. 10D5E0 overlay track. It needs 1D3770, 1E3F30 (1,007 instructions, not
   ported anywhere), 1CE720, 1E5810, 2A0100, 1E5360, 1E3270 and 1E5C80/1E5AF0.
   The same code also delivers 10D790, the position channel.
2. Root sampling: 1D4090, 1D4200, 1D4280, 1D4170, 144910, 1E5BA0, 1D4100,
   then the 283A90 codec chain.
3. 10CF80 position track. It uses x87 `fsqrt`/`fdiv` and FCOMI epsilon
   compares. The 08b Scarab moves an unparented root, so it needs B95D0 and
   BF050.
4. Custom animation without an anchor: 1128C0, 11DDF0, 11DD90, 111940,
   116A10, 1D3380, 1D4D90, 1D3740, 1CE6D0, 112420 and BC400 (BC400 is
   native).
5. Scenery, then attach and scale with the factory core.

TU1.1 native algorithms in `src/gameplay/animation/` map to several u5
owners (1D5320, 1D4E50, 1D45A0, 1E33D0, 2A0100, 1E5810, 1CE720, 1D36C0,
1D46A0, 1D34E0, 1E3420). They can be re-bound to the TU1.5 bank. 1E3F30 has
no port.

## Unit actions

- The table at `0x473D28` holds 60 descriptors `{start, update, stop,
  interrupt}`. The action block is at `body + word[+0x346]`.
- Dispatchers:
  - E8E50 requests an action (EAX unit, EDI request). It runs B9390, then
    `start`, then the 1EFA70 player history.
  - E8E10 builds the request.
  - E8D80 runs `update` each tick.
  - E8EB0 runs `stop`; E8F10 runs `interrupt`.

| Type | start | update | stop | interrupt |
|---|---|---|---|---|
| 0x1D vehicle exit | ED110 | ED440 | ED3F0 | — |
| 0x1E immediate exit (CEA70) | ED430 → ECE70 | — | — | — |
| 0x1F | ED640 | ED8F0 | ED820 | — |
| 0x20 | ED970 | EDB50 | EDAD0 | 8FDB0 (empty) |

Builtins:

- `vehicle_unload` (11D990) requests 0x1D for each rider found by 2199C0.
- `unit_exit_vehicle` (11D670) maps mode 0 → 0x1D, 1 → 0x20, 2 → 0x20 with
  flag 1, 3 → 0x1D with flag 1.
- `ai_vehicle_exit` (27EAB0) walks actors and requests 0x1D.

ECE70, the seat-detach core, needs:

- 1115D0 and D37F0;
- the stores `+0x250` (vehicle), `+0x254` (tick) and `+0x1FC` = −1;
- BBAC0 and BF050 from the core;
- weapon readiness: CE230, D0390, EAC70 (59.9k static) and F09C0;
- the AI hooks 1C4650/1C48A0.

The 03b `custom_animation_relative_loop` fault is 11DC00 → BBA80 → CEA70
inside the AI-frame translated unit.

Port order:

1. Generalize the dispatch layer. The existing native E8F10 handles only
   families 1C/1D/1F/20.
2. The builtins 11D990, 11D670 and 27EAB0, with 2199C0, CB440 and 219B00
   (under 1k instructions).
3. ECE70, leaves first; EAC70 last.
4. Action 0x1D. ED110's AI speech call 1C4600 → 213D30 (21.7k, never run)
   refuses by name when an actor is present.
5. Action 0x20.

## Validation, shared by all four

1. **Leaf owners.** Physical-original fixture cases in
   `tools/validate_native_campaign_script_builtins_tu15.py` (ROUTINES). They
   compare ordered stores, the result and bank digests. Every branch must be
   covered, and every kernel must be mutation-proven.
2. **Whole owners.** A live differential at real call sites, using a private
   overlay of the world bank. The original serves only as the oracle.
3. **Scenarios.** 08b `e7_scarab_main` and 03b `mission_start` must get past
   their faults. Then rerun the 900-tick mission sweep and byte-compare
   frames.
