# Native King result recording and retention

The native match now calls the recovered shared result-event recorder at
King's actual pre-world scoring boundary. Recording uses the same original
enable/freeze flags as retained statistics. Its pose provider resolves the
player's current unit, or its retained previous unit only when current is NONE,
and reads actual native position bits. A parent transform remains an explicit
unbound boundary. The default platform clock samples system time32; tests may
supply a declared clock. The first eligible tick sets the recording start
timestamp after King's pre-world updates, preserving the original ordering.
[Match implementation](../src/multiplayer.cpp),
[shared original recorder](MP_RESULT_EVENT_NATIVE_BINDING.md),
[original lifecycle evidence](MP_KING_RECORDING_EVENT_SOURCE_RESEARCH.md).

Actual stationary/moving FFA/team King runs on Lockout with Xbox1.5 data
exercise all four combinations. FFA records two exact type2 events per run,
including argument fields, relative times29/59, reserved bytes and pre-world
XYZ bits. Team runs produce no such events. Query counts, first-tick timestamp,
game-over freezing and a deep copy surviving world destruction all pass.
The observer callback is optional after internal recording.
[Runtime test](../tests/king_recording_runtime_tests.cpp).

The runtime outcome now retains the complete1000-entry event ring and its
index/last/start metadata beside statistics. Client outcomes reject these local
producer banks. Ownership tests check independent storage; the actual headless
runtime checks unfinished handoff and normal three-minute Slayer expiration
using the default system clock.
[Outcome ownership](../src/multiplayer_outcome.cpp),
[runtime handoff](../tests/multiplayer_outcome_runtime_tests.cpp).

The complete frozen build passed12 of13 selected CTests. The remaining test
incorrectly assumed an absent explicit XBE path meant absent original data,
although Cache loads that data automatically. The failing source, executable
and log are preserved. A separate corrected test passes against the same
hashed runtime and archives; FP/scope network regressions and the scoped
ASan/UBSan/leak run also pass. All11 repair commands pass. Concurrent rendering
edits are preserved in a separate patch; the exercised headless prefix matches
the frozen source. Verification SHA256:
`7c55e11f1d1700611b4e90fac1151456fb3b9ed375171f80a0a29248c8bfffaa`.
[Full build verifier](../tools/verify_king_recording_native.py),
[test repair verifier](../tools/verify_king_recording_native_repair.py),
[frozen verification](../analysis/multiplayer/game-variants/king-recording-native-20261012/verification.json).

Declared debug positions and wall-clock inputs isolate these recording checks.
Full retained participant/machine/header publication, parent transforms,
normal King presentation/admission and complete playable match flows remain
open. The all39-variant goal remains active and incomplete.
