# CTF and Assault native rules audit

This is the initial two-kernel audit and its retained runtime evidence. The
subsequent [complete 57-routine conversion ledger](MP_CTF_ASSAULT_NATIVE_CONVERSION.md)
tracks the new native implementations and their separate qualification receipts.

2026-10-23. Xbox TU1.5 remains the gameplay authority. Prepared MCC caches
supply runtime content. Native execution must preserve the original timing,
legality, score, inventory, round, overtime and result owners.

The four CTF presets are Multi Flag CTF, CTF Classic, 1 Flag CTF and
1 Flag CTF Fast. The five Assault presets are Multi Bomb, Single Bomb,
Single Bomb Fast, Neutral Bomb and Blast Resort. Their shared class does not
make their option buffers or round rules interchangeable.

## Native implementation boundary

`source_objective_native_tu15.hpp` implements two semantic native integer
kernels: accumulating Assault arming progress to the authored threshold, and
decaying interrupted progress by three ticks per delivered frame with the
original -1 sentinel. The signed16 publication remains separate from the
signed32 threshold comparison, including zero-time and narrowing boundaries.

The compatibility adapter preserves the original read/fault order, volatile
register results, comparison flags, ordered store and write notification.
Active instruction traces retain the literal control. The exact admitted
ordinary-world cursor contract permits the native kernels without suppressing
service callbacks. `HALO2_REFERENCE_OBJECTIVE_NATIVE_KERNELS=1` selects the
same-binary literal control for differential runtime validation.

These kernels are ordinary compiled C++ rule calculations. No executable
payload, runtime instruction decoder or binary replay is added. The Xbox
executable is used offline by the reference generator. Its small fixture stores
declared inputs, expected outputs and original instruction-window hashes.

The surrounding shared engine still contains compiled lifted routines. This
change does **not** establish conversion of the complete CTF/Assault engine to
semantic native kernels. Legality, objective lifetime, callbacks, score awards,
round transitions and recording remain with their existing owners; the new
kernels cannot supply those outcomes.

## Verification

- Five initial CTests pass against retained original initialization,
  activation, configuration, class-data and shared-engine fixtures.
- The shared-engine test passes all 1,164 original complete-return cases and
  all 39 original option buffers. Its additional untraced execution compares
  complete register/FP state and final memory banks for all 288 CTF/Assault
  cases, requiring real execution of both native kernels.
- The dedicated kernel test passes 986 unchanged original CPU windows under
  AddressSanitizer and UndefinedBehaviorSanitizer. It compares all general
  registers, defined flags, signed16 progress and ordered source stores.
  Neutral row8, zero-time arming and signed-width limits are included. Raw ABI
  boundary cases do not enlarge the original settings validator's domain.
- Native source injection is idempotent. Historical whole-module regeneration
  also observes three helpers now routed to other owners and two corresponding
  caller differences; that replay was not substituted for the existing engine.
- All nine unchanged presets pass actual illegal-side pickup rejection (where
  a side is illegal), normal pickup/carry/trigger drop and unattended timed
  home return. Neutral Bomb uses its original separate home marker. The
  original authored return clocks remain 900 frames, 450 for 1 Flag CTF Fast
  and 600 for Single Bomb Fast.
- All five unchanged Assault presets pass partial arming, three interrupted
  frames, resumed arming, planting, fuse and score. Progress decays by exactly
  three each interrupted frame. Every case report field is identical between
  native execution and the same-binary literal control, except the diagnostic
  kernel counters. The native runs record 160 arming calls for the five-second
  presets and 310 for the ten-second presets, with three decay calls each;
  the controls record zero native calls.

The complete-match cohort and corrected Multi Bomb regression pass all nine
presets with their unchanged 304-byte authored options. Multi Flag CTF and
CTF Classic require three captures; first-to-three presets complete three or
five actual rounds, including genuine replacement units and accumulated wins.
Every completed match retains its statistics through 211 postgame frames and
the original result/countdown/fade path.

The initial Multi Bomb test failed because its fixed 256-frame wait was
shorter than the actual remaining original respawn countdown of 292 ticks
after a detonation. The corrected harness waits that observed countdown plus
the existing factory margin. It changes no gameplay timer or respawn owner.
A focused native probe and the registered CMake regression pass all three
scores, with source/native unit IDs and countdowns matching. Actual replacement
admission takes 314 and 312 frames, within the observed countdown plus margin.

All runtime cases use the prepared MCC Lockout map as content with original
TU1.5 execution and rule ownership. The source/native live and retained
statistics banks must agree on every delivered complete-match frame.
Player approach/goal positions are explicit original B95D0 inputs. Objective
positions, possession, timers, score and outcome results are never supplied.

Bounded worker reports, source refresh hashes, original-reference receipts and
the exact runtime differential are under `build/ctf-assault-audit-20261023-*`.
The initial runtime worker correctly records a failed CTest for the short
Multi Bomb harness wait; its eight passing complete-match cases are retained
alongside that diagnostic. The corrected Multi Bomb worker passes configure,
build and CTest; its separate report must be read with that cohort. This does
not claim an all-green initial worker.

| Preset | Actual goals | Completed round wins |
| --- | ---: | ---: |
| Multi Flag CTF | 3 | 1 |
| CTF Classic | 3 | 1 |
| 1 Flag CTF | 5 | 3 |
| 1 Flag CTF Fast | 5 | 3 |
| Multi Bomb | 3 | 1 |
| Single Bomb | 5 | 3 |
| Single Bomb Fast | 5 | 3 |
| Neutral Bomb | 3 | 3 |
| Blast Resort | 5 | 3 |

`build/ctf-assault-audit-20261023-source/acceptance.json` records the individual
case receipts and exact source hashes. The disposable source/build owner and
private probe executables are removed after validation; retained evidence is
bounded text/JSON and losslessly compressed source manifests.

## Remaining acceptance

The full multiplayer goal remains incomplete while frontend admission,
played routes, broader edited-rule combinations, objective presentation/audio
and network objective actions remain open. In particular,
`MenuState::selected_multiplayer_configuration` still calls the bounded Slayer
admission path. Kernel/component proof must not be described as ordinary
frontend access, physical Xbox framebuffer/audio parity, or complete network
matches. Existing launcher-selected runtimes are preserved by this audit.
