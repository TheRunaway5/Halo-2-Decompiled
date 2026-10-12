# Multiplayer scenery shader-detail integration

`source_scenery_shader_tests` checks the production `SourceSceneryScene`
resident shader union and per-part per-view selector across all 21 supplied
multiplayer maps. The gate covers **274 resolved scenery owners: 196 baked
and 78 dynamic**, **789 resident material banks**, **252 explicit view
fixtures**, and **2,796 lower / 3,012 highest-detail selected source parts**.
The four Zanzibar pillar selections that require actual initial RNG history
are explicitly excluded. Foundation and Lockout have no admitted resolved
SCEN owner in this automatic Slayer gate.

## Independent source inputs

`tools/audit_scenery_shader_scene.py` reads literal 72-byte original part
resources and their render-model material-table shader datums. Selected
sections come from the retained untouched original `4C2B0` collector outputs,
filtered through the separately verified automatic Slayer gate. The new frozen
fixture `tests/fixtures/retail_scenery_shader_parts.tsv` contains 274 owners,
1,644 geometry collectors and 219 distinct source section-part records. The
exporter verifies the original resource hashes before preserving their words.
Its provenance is `analysis/multiplayer-shading/scenery-shader-scene-literal.json`.

Each shader's bias and threshold words come from the independent literal
6450-bank fixture `retail_source_shader_lod_assets.tsv`. The native decoder
must match it word for word. The cached owner center/radius, flags, scale,
zero velocity, frozen state and node0 come from the independently retained
original cold type6 factory fixture `retail_scenery_initial_bounds.tsv`.
The scene's current initial node0 and full-handle identity must match those
original words before the callback returns the literal `+30 / +3C` sphere.
The membership sphere at `+40 / +4C` is a separate source field and is not used
as a shader-coverage substitute.

The camera fixture follows original grid/angle, `2F970 → 2FB70 → 2FD90`, shader
view and player-count helpers. Features and auxiliary globals are explicit:
640×480 source grid, safe equal to outer, no rectangle or screen override,
no widescreen flag, no auxiliary context. Near cameras use the first authored
spawn plus an explicit 0.35 Z fixture offset; distant consumer cameras move
100000 units along X. This is a bounded shader consumer fixture, not an
original live camera or a claim of source visibility admission for an
outside-map position.

## What is checked

The test independently composes original object coverage, the collector byte,
the literal part scale, local-player multiplier and `1BF50` shader choice.
For every emitted vertex it checks the original owner, section, part and shader
datum, selected shader LOD, and all 33 retained vertex fields. Only the material
binding may change. No world incident vector is rotated, no baked RGB is
replaced, and no part disappears unless the independent source choice omits it.

The resident union must contain exactly the actual selected geometry-part
material table union across all six geometry LODs and each material's authored
shader alternatives. Each expected bank starts from a fresh raw shader,
assigns the selected index before preparation, uses source owner implementation1
for baked scenery or implementation2 for dynamic scenery, and applies that
owner's original-verified initial colors. Full pixel words/fragment offsets,
vertex bytecode, constant words, bitmap bytes/mips, samplers, blend/raster state
and ordered prepared child passes must match. The separate MODEL material
census validates these same source pass preparations against authored banks;
this integration gate tests residency and selection rather than generating
expected banks from resident output.

One-, two- and four-player fixtures hold the geometry draw metric fixed while
changing only the shader camera. They check **50 distinct close/far pairs at
the same geometry LOD**, repeat the first view after the second and require
exactly retained first-view vertex storage and material choices. Every selected
dynamic alternative remains inside its actual owner's full private lighting
range. Its supplied cached sphere is retained once, and later views reuse the
first source light-cache result rather than replacing it.

For baked owners, original full identity/checksum selects the authored BSP
bucket; section offset plus original source vertex ordinal supplies incident
and RGB. **792,954 submitted reference vertices** compare those literal bucket
values exactly, including lower shader selections. Owner baked/dynamic policy
remains stable across shader detail. The MODEL census demonstrates no
same-part layer5-to-layer3 transition; an owner's PRT capability does not imply
that an ordinary material was ever a PRT pass.

All supplied admitted SCEN shader banks and tested collectors are nonempty:
**zero empty geometry collectors and zero rejected shader parts**. The gate
asserts that source census rather than fabricating an empty authored bank.
Hidden-owner controls require empty vertex, draw and lighting submissions and
no new cached-state reads. Pure source helper fixtures cover empty-bank return
semantics separately.

## Per-map consumer coverage

Counts include repeated first views and the three geometryLOD0 controls.

| Map | Resolved owners | Resident banks | Lower parts | Highest parts | Distinct same-geometry view pairs |
| --- | ---: | ---: | ---: | ---: | ---: |
| ascension | 2 | 10 | 12 | 66 | 3 |
| beavercreek | 27 | 60 | 164 | 214 | 3 |
| burial_mounds | 27 | 85 | 576 | 66 | 3 |
| coagulation | 4 | 20 | 32 | 124 | 3 |
| colossus | 1 | 12 | 38 | 22 | 3 |
| cyclotron | 2 | 2 | 0 | 24 | 0 |
| foundation | 0 | 0 | 0 | 0 | 0 |
| headlong | 18 | 78 | 218 | 256 | 3 |
| lockout | 0 | 0 | 0 | 0 | 0 |
| midship | 28 | 77 | 360 | 600 | 3 |
| waterworks | 14 | 40 | 92 | 184 | 3 |
| zanzibar | 23 | 88 | 356 | 172 | 3 |
| containment | 14 | 40 | 154 | 122 | 3 |
| warlock | 17 | 38 | 80 | 232 | 3 |
| deltatap | 9 | 9 | 0 | 108 | 0 |
| turf | 1 | 7 | 20 | 19 | 3 |
| backwash | 4 | 6 | 20 | 28 | 2 |
| dune | 6 | 26 | 64 | 83 | 3 |
| elongation | 45 | 93 | 358 | 254 | 3 |
| gemini | 23 | 63 | 104 | 406 | 3 |
| triplicate | 9 | 35 | 148 | 32 | 3 |

## Verification boundary

This is production-linked CPU integration evidence for source input composition,
resident prepared banks, source-part remapping, retained baked streams,
dynamic owner binding ranges and independent view storage. It does not establish
original live object visibility/registration chronology, actual current global
camera/clock/activation history, shader animation order, GPU descriptor
consumption or original/native framebuffer equality. Those source services and
GPU tests keep their own acceptance boundaries.

The CTest is `retail_source_scenery_shader`. Release and ASAN/UBSAN runs with
leak detection, map counts, original/cache/fixture/source/binary/log hashes and
source changes during execution are retained in
`analysis/multiplayer-shading/scenery-shader-scene-proof.json`.
