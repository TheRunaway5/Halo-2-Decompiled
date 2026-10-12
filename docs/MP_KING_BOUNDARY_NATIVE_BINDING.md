# Native King hill boundary geometry

The native boundary generator matches all 14,520 captured position, UV and RGBA words across 22 original emission corridors: 1,452 vertices forming 726 lower/upper pairs. It also matches the real draw prefix's fade bounds, both FISTP results and the observed x87 precision/rounding variants, while restoring the native caller's floating point state. The root CMake comparison, existing RGB/marker comparison and full standalone ASan/UBSan/leak run pass. [Original emission contract](MP_KING_HILL_EMISSION_SOURCE_RESEARCH.md), [native interface](../include/halo2/source_king_boundary.hpp), [implementation](../src/source_king_boundary.cpp), [comparison](../tests/king_boundary_tests.cpp).

The source emits a closed strip over the authored hill points. Each point supplies a lower vertex at the hill's center Z and an upper vertex at Z+0.8, with distinct vertical UV coordinates and the source opacity in register11's alpha. UV progression uses the original edge distances, clock and conversion behavior. The comparison includes four declared camera positions, ticks0/59/60/10117, four additional control words and two extra RGB inputs. These are original arithmetic comparisons over declared inputs. [Frozen original corpus](../analysis/multiplayer/game-variants/king-hill-emission-source-20261012/complete/report.json).

The unchanged original emission corridor starts after an explicitly skipped material-preparation interval and stops before SDK restore. It therefore establishes the emission calculations without claiming a whole positive original draw. The independent genuine Snapshot draw probe still stops at the actual hill bitmap request for its center-camera case. Material preparation, residency, world submission, final rendered pixels and normal King admission remain required. [Whole-draw boundary](MP_KING_DRAW_SNAPSHOT_SOURCE_RESEARCH.md).

[Verification manifest](../analysis/multiplayer/game-variants/king-boundary-native-20261012/verification.json), SHA256 `2fc56b9f37fca6a4f9c1270373c3a3f5efc0f151e454b608335c059f51b9a545`, retains 11,173 frozen source files, the exact CMake overlay, five successful commands and three pinned binaries. The original four-file emission reproduction and every frozen source/log/binary hash were independently checked. [Verification tool](../tools/verify_king_boundary_native.py).

```sh
.tools/python/bin/python tools/verify_king_boundary_native.py
```

This adds no variant admission and does not complete the all39 goal. The literal x87 emission backend currently requires supported x86 GCC/Clang; unsupported backends report that limitation.
