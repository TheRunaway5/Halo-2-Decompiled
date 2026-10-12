# Native King hill color and marker records

The native color and marker producers now match 1,488 complete original RGB results, 105 complete invalid-hill marker returns and 84 defined marker records at the actual projection handoff. The corpus covers retail, TU1.1 and the owned Xbox1.5 executable. Both the root CMake test and a full standalone ASan/UBSan/leak run pass. Projection, authored hill rendering, waypoint/arrow drawing and normal King admission remain separate requirements. [Source contract](MP_KING_VISUAL_CONSUMERS_SOURCE_RESEARCH.md), [native interface](../include/halo2/source_king_visual.hpp), [implementation](../src/source_king_visual.cpp), [comparison test](../tests/king_visual_tests.cpp).

The color input retains raw player slots independently from the occupied bitmap. The original owner reads query/local records by low16, skips current-unit NONE in its active iterator, and does not apply the ordinary-player flag-bit1 gate. Empty or contested occupancy chooses the original neutral RGB. A single side chooses the last admitted occupant's authored team palette only with an engine and team mode; other cases use the actual current-view appearance type. Missing native raw slots raise an error. [Complete original color/iterator/provider listings](MP_KING_VISUAL_CONSUMERS_SOURCE_RESEARCH.md), [literal Lockout mulg palette](../tests/fixtures/king_visual_team_colors_original.json).

The marker producer preserves the valid even-count gate, current-unit position lookup, x87 distance/radius fade, four RGB copies and exact field bytes. It writes only original defined ranges `[0,2)` and `[4,6C)`; opaque caller bytes survive. The marker view is distinct from the current render view used by color fallback. This binding ends at the original `24E59F` handoff and does not fabricate a successful projection or rendered icon. [Original marker records](../analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json).

[Verification manifest](../analysis/multiplayer/game-variants/king-visual-native-20261012/verification.json), SHA256 `03191a1d8ee4757db0ce6676aa823a54367aeb6bcfc39f23b16e3cd7bf585816`, preserves the full frozen source, actual root CMake overlay, re-extracted palette, five successful commands, logs and both binaries. Two earlier frozen attempts remain: a test's JSON/string comparison failed compilation, then its service-address assertion incorrectly used the retail address for TU1.1. The repaired test takes the actual projection owner from each image's registration metadata. Original corpus and production behavior were unchanged by those test repairs. [Verification tool](../tools/verify_king_visual_native.py).

```sh
.tools/python/bin/python tools/verify_king_visual_native.py
```

This milestone adds no normal-world consumer or additional admitted variant and does not complete the all39 goal.
