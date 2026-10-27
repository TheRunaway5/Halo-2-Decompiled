# Native sound engine goal

Goal (requested 2026-10-25): the game's sound engine runs as native C++ in exact
parity with TU1.5, like a true decompilation. No lifted (machine-code-translated)
routine and no emulated Xbox audio hardware runs in a shipped game.

Status: **every sound routine a multiplayer match executes is native**
(2026-10-25). 37 sound manager entries and all 23 object looping-sound
routines run as native C++. A live snapshot oracle matched the lifted
originals byte for byte on every frame of Coagulation weapons, Lockout weapons
and Coagulation walking. Remaining: campaign audio units, removing the
unreached lifted SDK/hardware code from the game build, and publishing.

## Done when

- [x] Every sound manager entry a multiplayer match executes has a native
  kernel. Each passes its randomized native-vs-lifted test, and the reached
  ones pass the live-match oracle (`HALO2_SOUND_MANAGER_ORACLE=1`).
- [x] The object looping-sound updater (`18EE50` and its group) is native.
- [ ] Campaign audio (`source_campaign_audio`, 6092 lifted instructions;
  `source_unit_speech`, 721) is native, if campaign worlds ship.
- [x] No SDK internal or hardware model executes in a shipped match: the
  census reaches only the four SDK CRT static initializers (now native). The
  game never constructs the retail sound driver or binds the DMA/reset owners.
- [ ] The lifted sound files and `src/audio/runtime/` hardware models are out
  of the game build (tests may keep them as parity references).
- [ ] The launcher build passes its audio checks and is published.

## Method

1. Decompile from the lifted body
   (`awk '/^static void run_[0-9a-f]+\(WorldState &s\) \{/,/^}/' FILE`).
2. Read the retail `.rdata` constants from the TU1.5 XBE
   (`21b207e7…`) and write them as literals.
3. Write the kernel in its group file `src/audio/native_tu15_sound_kernels_*.cpp`,
   using the helpers in `src/audio/native_tu15_sound_kernel.hpp`.
4. A kernel owes its compiled callers the return registers at their width,
   every register the original preserves, x87/SSE state (values, TOP, C1,
   sticky exception flags, control word, MXCSR) and every memory store.
   Clobbered scratch registers and EFLAGS are not part of that contract.
5. A differential test (`tests/native_tu15_sound_test_support.hpp`) checks
   randomized states over six x87 controls and five MXCSR modes, special floats
   and every branch. Native and lifted must agree on every owed output, or
   fault at the same address.
6. Routing is native by default in `physics_live_world_tu15.cpp`.
   `HALO2_REFERENCE_SOUND_MANAGER=1` restores the lifted routines.
   `HALO2_SOUND_ENTRY_CENSUS=1` prints which sound entries a run executed.

## Sound manager

| Group | Entries | Lifted instrs | State |
|---|---|---|---|
| Random, scalar, class row | `127E00`, `127D70`, `127B20`, `228220` | 120 | Native, tested |
| SDK CRT statics | `387BAA`, `387BB5`, `38A988`, `38A993` | 4 | Native, tested |
| Pitch range | `21F960` | 70 | Native, tested |
| Permutation | `21FA80` (with `21FB20`, `21FCC0`, `21FD20`, `21FD80`, `21FDE0`, `21FED0`, `21FF70`) | 330 | Native, tested |
| Gain | `127CF0`, `129B20`, `21F780`, `21F7F0`, `220000`, `220200` | 353 | Native, tested (48k states) |
| Properties | `18FE80`, `18FF60`, `1974B0`, `21FEB0`, `2290C0` | 189 | Native, tested (40k states) |
| Listener selection | `12A810`, `12A930`, `2287B0`, `253070`, `2531E0` | 312 | Native, tested (40k states) |
| Cache binding, query, chunk readiness | `129470`, `12B1C0`, `1296E0`, `128970`, `1293F0`, `21F260`, `21F3B0` | 268 | Native, tested (56k states) |
| Startup | `12EB50`, `1972C0`, `1973E0`, `224EC0`, `2254F0`, `2275A0`, `227EA0` | 934 | |
| Startup storage | 17 routines | 667 | |
| Object looping-sound update: frame | `18EE50`, `223DA0`, `22C560`, `226370`, `18ED80`, `18F510`, `18F900`, `129DD0`, `18D390`, `18D3D0` | 742 | Native, tested (41k cases, full trace coverage) |
| Object looping-sound update: update | `18E970`, `18DC70`, `190A80`, `129170`, `18D960` | 1944 | Native, tested (20k cases) |
| Object looping-sound update: sources | `18D5A0`, `18F9B0`, `18FFB0`, `190460`, `1904D0`, `220640`, `18B580` | 793 | Native, tested (170k states) |
| Object looping-sound update: `220C60` | `220C60` | 763 | Native, tested (60k states, 90% trace coverage; live oracle on ~3,500 real calls per match) |

Live-match oracle (2026-10-25): with all 23 object-loop routines native (the
reference frame runs fully lifted under `NativeTu15SoundReferenceScope`), the
snapshot oracle matched the lifted frame byte for byte on all 351 frames of
each of Coagulation weapons, Lockout weapons and Coagulation walking. Native
mode passes the same matches' SDL submission checks. The walking footstep
check (`footstep_packets=0`) fails identically with every native kernel off,
so it predates this work.

Tests: `ctest -R '^native_tu15_sound_'` runs the eight differential suites
(manager, gain, properties, listener, resources, object_loop_update,
object_loop_sources, object_loop_frame).

## Census (2026-10-25)

`HALO2_SOUND_ENTRY_CENSUS=1` counts every sound entry crossing the live-world
dispatcher, plus the object-audio frame.

| Run | Lifted sound entries reached |
|---|---|
| Coagulation weapons, 2 cycles | `18EE50`×351, `220C60`×3510, `387BAA`/`387BB5`/`38A988`/`38A993`×1 |
| Coagulation walking | `18EE50`×351, `220C60`×3604, the same four ×1 |
| Lockout weapons | `18EE50`×351, `220C60`×2106, the same four ×1 |
| Coagulation LAN | the same four ×1 |
| Campaign speech (00a), sound points | none |

In a shipped match the only per-frame lifted sound code is the object
looping-sound group. The four SDK CRT static initializers (now native) are the
only SDK entries reached. No startup, SDK-internal, AC97/DMA/codec/DPC or
hardware-model routine runs, because the game never binds the native
DMA/reset owners (`bind_native_sound_output` is test-only).

The object-loop group calls about 40 routines outside itself (core object
helpers, plus `220C60` through the live world's loop-sound request hook), so
its live check is a snapshot oracle: run the lifted frame under a write
journal, roll it back, run the native frame and compare every byte. With
lifted fallbacks on both sides, a full Coagulation weapons match matched on
all 351 frames.

## DirectSound/XDK layer

Not reached by a shipped game (census above). The remaining step is to take
these lifted routines and hardware models out of the game build, keeping them
as test references.

| Group | Routines | Lifted instrs |
|---|---|---|
| SDK (`source_sound_sdk`) | 35 | 905 |
| DSP image | 19 | 782 |
| Frontend DPC and events | 44 | 1192 |
| Timed notifications | 13 | 359 |
| AC97 output, retire, page release | 31 | 1104 |
| Reset, codec events, codec initialization, configuration | 24 | 747 |
| DMA, hardware, PIO storage | 12 | 362 |
