# Owned unit animation metadata for retained source managers

The cache now supplies the original channel definition/event data and owned graph lookup banks needed by the retained animation managers and source selection code. It preserves the packed inherited-graph/clip identifiers, authored event order, sound markers, reference flags and original virtual pointer words. This data seam does not select an activity or make live characters animate.

## Imported data and ownership

`Cache::load_source_unit_animation_definition(graph,packed)` resolves the signed low short through the original graph+4C inherited records, then uses the signed high short for the resolved graph's 6C-byte definition. It retains definition+14 frame count, +2E loop frame and +18 authored flags; +4C timing records, +54 sound records and +5C effect records; and the **resolved graph's** +14/+1C sound/effect references. NONE references, signed fields and all flags remain literal. No codec/resource decode or named-action replacement is performed.

`Cache::load_source_unit_animation_graph_metadata(graph)` returns an owned full authored jmad payload. `UnitAnimationGraphMetadata` retains its full datum, original metadata address, C4-byte header view and bounded `bytes_at(original_address,extent)` access. Source pointer words are opaque tokens and never host pointers. Each graph owns its exact tag extent; an escaped or unavailable token stops. Callers retain these packets while graph services use their spans. Runtime graph postprocessing and resource/residency flags still require their actual producers.

The original source owners establishing these layouts are `1CA5A0`, `1DE890` and `1CAE40`. The selected full payload preserves the source hierarchy, mapping and inherited records for the separate [selection constructor work](MP_UNIT_CONSTRUCTORS_NATIVE_RESEARCH.md), without choosing a fallback action.

## Independent original evidence

`tools/oracle_unit_animation_definitions_1_1.py` executes unchanged **1CA5A0** on all valid direct and inherited metadata queries in the owned Lockout map: **3,372 definitions**, including **930 inherited**. It reads definition/event/reference arrays from the actual original returned graph. Two separate processes reproduce all five report/fixture/listing files exactly. The records contain 1,604 timing events, 2,104 sound events and 19 effect events across those queries; these counts include shared graph records reached through inherited queries.

The guest supplies literal resident metadata and its tag-table pointer. Queries exhaust authored definitions for testing; they are not gameplay requests or evidence of source action admission. `1DE890` and `1CAE40` are fully source-listed, but the new metadata suite executes only `1CA5A0`. It does not supply resource notification, codec, audio/effect return, frame, RNG, transition, pose or physical state.

`tools/export_unit_animation_graph_metadata.py` independently retains all **58 graph payloads / 481,520 bytes** and checks them against the original returned graph/definition addresses. Its two complete outputs repeat exactly. Every original lookup span measured by the separate source-selection observer falls within its actual graph's owned tag extent.

Owned authorities:

- Executed disc 1.1 XBE SHA-256: `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Original Lockout map SHA-256: `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Authenticated English u5 XBE SHA-256: `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The complete normalized bodies, internal branches, encoded bytes, direct call pairs and memory operands uniquely correspond as follows:

| 1.1 owner | Authenticated u5 | Role |
| --- | --- | --- |
| 1CA5A0 | 1CEE60 | Resolved graph |
| 1DE890 | 1E3150 | Packed definition and resource notification |
| 1CAE40 | 1CF700 | Ordered event traversal |
| 1E13C0 | 1E5C80 | Resource notification boundary |

This is static u5 correspondence; no u5 title/runtime/character playback is claimed.

## Native verification and reproduction

`unit_animation_definition_tests` compares **3,377 definitions**: all 3,372 original metadata queries plus five definitions previously measured during actual unit manager updates. Every scalar and every ordered timing/sound/effect/reference record matches. Both normal and ASan/UBSan builds pass. All 58 complete owned graph payloads and opaque pointer words match, with 174 pointer-boundary checks. Five additional unavailable graph/packed cases stop at the native owner boundary; these are host bounds checks, not original invalid-memory execution claims.

The final audit checks independent repeat bytes, 311 original source instruction rows, authenticated image identities, four complete u5 owner correspondences, native logs/binaries/linked archives and **102 preserved earlier animation-owner pins**, with zero mismatches.

From the project folder:

```sh
.tools/python/bin/python tools/oracle_unit_animation_definitions_1_1.py --output analysis/multiplayer/integration/unit-animation-definitions-20261008/measured
.tools/python/bin/python tools/export_unit_animation_graph_metadata.py --output analysis/multiplayer/integration/unit-animation-definitions-20261008/graph-metadata/measured
cmake -S . -B build-unit-animation-definitions-20261008 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-unit-animation-definitions-20261008 --target unit_animation_definition_tests -j 4
ctest --test-dir build-unit-animation-definitions-20261008 -R '^original_unit_animation_cache_definitions$' --output-on-failure
```

The sanitizer build uses `-DHALO2_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug`. Frozen tested binaries, linked archives, relevant source copies and audit inputs are retained in the ignored `analysis/multiplayer/integration/unit-animation-definitions-20261008` directory. Extracted bytes remain outside the source-only repository contents.

Principal report hashes:

| Artifact | SHA-256 |
| --- | --- |
| measured/report.json | 291d62a14321ffcc9dcf436f3054290baf74838ce6c652ae3bc34d7aa4bdbecf |
| graph-metadata/measured/report.json | b5444d3a33ff20fb402e61018ac049632f65062b88e66e78d0d0f5df8836d57f |
| u5-correspondence/report.json | 2aa9316a399660fc3c39396b7fd7ef0f77070474c6fc87ad1cb15403d0f3f3fb |
| audit-final.json | a87712135509d3dae6db81dfedea85779641bd3f19dff0082d35124438a7af2f |

## Integration boundary

These imports are actual data providers for the retained source constructors/selection/clock modules. Graph default selection, variant RNG, resource notifications, event consumption, decoded local banks, current unit callback/IK, physical admission and collision matrices still have separate owners. A valid imported clip does not replace those owners. Live switching, melee/throw animation playback and the full native unit action schedule remain integration work.
