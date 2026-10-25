# Native sound engine

Goal: the sound engine runs native C++ in parity with TU1.5, like a true
decompilation, with no lifted (machine-code-translated) routines and no
emulated Xbox audio hardware.

## Inventory (2026-10-25)

| Layer | Code | State |
|---|---|---|
| Mixer, SDL output, frontend music, DSP effects | `MultiplayerAudio`, `native_tu15_sound_*`, `source_loop_*` | Native |
| Xbox DSP56300 interpreter | `src/audio/dsp/sound_gp_cpu_tu15*` | Test-only, not linked into the game |
| Halo 2 sound manager (`0x127b20`–`0x2290c0`) | 13 `src/audio/source_sound_*_tu15_generated.cpp` files | Converting (below) |
| Xbox DirectSound/XDK internals (`0x383d40`–`0x38f53e`) | 15 lifted files plus `src/audio/runtime/` hardware models | Lifted |

## Converted sound manager routines

`src/audio/native_tu15_sound_manager.cpp` replaces these entries in the
live-world dispatcher (`physics_live_world_tu15.cpp`):

| Entry | Routine |
|---|---|
| `127E00` | Random value between a playback row's 16-bit bounds |
| `127D70` | Playback gain: base + random(0, variance) |
| `127B20` | Playback scalar: request override, sound-class row or playback row |
| `228220` | Sound-class row lookup |
| `21F960` | Pitch range selection (keep, containing, or nearest range) |
| `21FA80` | Permutation selection (forced, history mask, skip-fraction rerolls); includes `21FB20`, `21FCC0`, `21FD20`, `21FD80`, `21FDE0`, `21FED0`, `21FF70` |

Each kernel reads the original game state and reproduces the routine's
arithmetic under the caller's x87 control word and MXCSR. Retail `.rdata`
constants are literals (`1/65535` = `0x37800080`, `FLT_MAX`, `0.5`, `1.0`).
A kernel owes its compiled callers the return registers, every register the
original preserves, x87/SSE state and memory stores. Scratch registers the
original clobbers and EFLAGS are not part of that contract.

### Verification

- `native_tu15_sound_manager` (ctest): about 48,000 randomized states per run,
  each run through both the native kernel and the lifted routine. It covers
  six x87 precision/rounding controls, five MXCSR modes, NaN, infinity,
  denormal and signed-zero inputs, and every branch, and requires identical
  faults.
- `HALO2_SOUND_MANAGER_ORACLE=1`: in a live game, predicts each call natively,
  runs the lifted routine, and throws on the first difference.
- `HALO2_REFERENCE_SOUND_MANAGER=1`: restores the lifted routines.

Still required before release: an oracle pass over live matches on real map
data.

## Remaining

1. Sound manager: gain (`127CF0`, `129B20`, `21F780`, `21F7F0`, `220000`,
   `220200`), properties (`2290C0`, …), listener selection (`12A810`,
   `12A930`, …), binding, cache query, chunk readiness, startup, startup
   storage, and the object looping-sound updater
   (`source_object_loop_audio`, about 4.8k lifted lines).
2. DirectSound/XDK layer: replace the game's DirectSound calls at the API
   boundary with the native mixer, so that no SDK internals (AC97, DMA,
   codec, frontend DPC, timed notifications) execute.
3. Then remove the lifted sound files and the `src/audio/runtime/` hardware
   models from the game build.
