# Native objective score HUD binding

The live multiplayer HUD now passes the snapshot's actual registered engine
type and original round selector into the recovered score-display owner.
Previously it left the input defaults at Slayer2/round0. King4 now formats
player/team scores as the original time text; the limit-one query receives the
actual round selector and retained win fields. No variant name selects a format.
[Live adapter](../src/multiplayer_hud.cpp),
[recovered score display](../src/source_hud_score.cpp).

The rules adapter reads parameter0, the original variant+4C selector, and the
match publishes it with source_engine in native snapshots. Native protocol40
carries and validates selectors0..6. This preserves the field without enabling
unfinished later-round lifecycle/reset behavior.
[Rule and state production](../src/multiplayer.cpp),
[native UDP](../src/multiplayer_network.cpp).

All1,330 existing unchanged retail/TU score-display comparisons pass. Native
HUD checks cover actual authored palettes/glyphs and declared snapshot inputs
for King time text, unchanged Slayer numeric text and all seven original
limit-one denominators. Actual stationary/moving FFA/team King scoring on
Lockout reaches the same HUD locally and over fragmented16-player UDP,
including score/rival text and limits after loss/recovery and stale delivery.
[HUD comparison](../tests/hud_score_tests.cpp),
[actual King UDP/HUD check](../tests/king_objective_network_tests.cpp).

A complete fresh frozen build passes all ten selected CTests, FP/scope network
regressions and scoped ASan/UBSan/leak checks. All12 commands and ten binaries
are pinned, together with the source and input assets. Verification SHA256:
`8ba1f7bd52f15a766ecbadf279a7abccfaea21bd46df7146d47d2cd4ef7f0d43`.
[Verifier](../tools/verify_king_hud_native.py),
[frozen verification](../analysis/multiplayer/game-variants/king-hud-native-20261012/verification.json).

This fixes the score adapter. Authored hill/waypoint rendering, live event
handoff, camouflage submission, full retained publication and normal King
admission remain open. The all39-variant goal remains incomplete.
