# Lower shader LODs for multiplayer models

The separate `world_material_tests --model-lods` audit checks lower shader
records for the supplied multiplayer model families without changing the
657-reference / 705-lower-record BSP baseline. It finds **1,068 distinct
map-context shader references, 1,071 lighting contexts and 1,267 unique lower
records**. The three additional lighting contexts represent shaders used in
both baked and dynamic model policies.

## Source reference scope

`tools/audit_model_shader_lods.py` filters the independently recorded original
collector fixtures by the verified automatic Slayer placement gate. It reads
literal 72-byte part material indices from their original raw resource files,
resolves render-model material-table shader references, and retains the
selected part shader union across all six geometry LODs. This covers
274 resolved rendered SCEN owners, 48 rendered MACH owners and 415 BLOC owners.
There are 49 admitted machine placements; the one with no render model has no
shader bank. The four unresolved Zanzibar pillar selections are excluded.
The original collector fixture uses retained explicit selections; it does not
supply complete live creation/RNG history.

The additional player inputs are original `matg` multiplayer representations2
and3 (Chief/Elite body and hands) and the supported SMG, battle-rifle and
sniper-rifle held/first-person references. Their complete declared material
tables are checked as an explicit asset superset. This does not establish that
every table entry belongs to the initial live selected-part collector.
Unsupported weapon gameplay and unused unrelated tag-table models are outside
this inventory.

The exporter reads original shader `+20` and template `+58` postprocess banks,
then each literal 10-byte shader LOD range, mask, projected-height float word
and ordered layer ranges. The frozen fixture is
`tests/fixtures/retail_model_shader_lods.tsv`; its input contexts identify the
source object/model, example selected section, geometry LOD, lighting policy
and PRT capability. Cache hashes and source identities are retained in
`analysis/multiplayer-shading/model-shader-lod-literal.json`.

## Preparation checks

The test validates every placed exemplar shader against its actual original
collector section and verifies the PRT-capable model's real authored bank.
Every shader's decoded LOD mask, raw threshold word and ranges must match the
independent literal fixture. The selected index is assigned before pass
preparation. Baked SCEN uses implementation1; dynamic SCEN, MACH, BLOC, body and
FP uses implementation2 with volume texture support.

A separate expected pass bank starts from the raw selected authored pass.
The explicit null-object change-color input supplies white through original
color-bank indices3/4/21/22. These vector channels remain distinct from scalar
brightness21/22. Dynamic tests use an explicit empty `ObjectLightingState`
through the separately original-verified extern helper. This is a bounded
material-preparation input, not a claim that the live object's retained light
state is empty. Actual owner colors, shader inputs, animation timing and
per-view retained lighting remain separate producers.

Full pixel words and fragment offsets, vertex bytecode, 128 constant float
words, blend/raster resets and authored writes, samplers, bitmap bytes/mips,
scope-default bitmap and ordered layer1/2/3/7/8/17 passes must match. No missing
or additional scheduled pass is accepted. The audit prepares **1,270 lower
lighting contexts: 42 baked and 1,228 dynamic**, covering **1,893 selected
passes**, with no unsupported lower binding in this bounded source bank.

## Ordinary materials on PRT-capable owners

All 1,267 lower records contain layer3; **none contain layer5**. Eleven lower
records occur on six ordinary shaders belonging to PRT-capable placed owners.
Those same shaders already use layer3 at LOD0. No admitted shader with layer5
at LOD0 has a lower record. This inventory therefore demonstrates neither a
same-part PRT-to-dynamic transition nor a new shader binding on a formerly
PRT-only part. An earlier report inferred such a transition from owner
capability; that inference was incorrect. Owner light-cache admission occurs
before part shader selection in source `4B5D0→3DD10→3DB00`; layer5 does not
suppress that cache or its initial query.

The six shaders are Zanzibar's `zanzibar_crane` (two lower records) and
`fusion_bolt` (one), plus the Containment gate and blue gate versions of
`forerunner_rusted_grey_prt` and `containment_gate_light` (two each). The `_prt`
name suffix is not evidence that a selected pass uses layer5. The actual
`zanzibar_crane_prt`, `grating_prt` and `metal_wheel` layer5 shaders each have
one authored shader LOD. A future lower record that uses layer5 must bring its
actual paired geometry-LOD/section bank; this test supplies no synthetic bank.

Shader LOD remains separate from geometry LOD. The current source geometry
collector and PRT stream/bank pairing must be retained when selecting another
shader record. This audit does not replace the original geometry selector.

| Map | Shader references | Unique lower records | Prepared lower contexts | Baked | Dynamic | Lower contexts on PRT owners |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ascension | 45 | 53 | 53 | 0 | 53 | 0 |
| beavercreek | 43 | 53 | 53 | 1 | 52 | 0 |
| burial_mounds | 44 | 55 | 55 | 2 | 53 | 0 |
| coagulation | 43 | 51 | 51 | 0 | 51 | 0 |
| colossus | 46 | 59 | 59 | 0 | 59 | 0 |
| cyclotron | 55 | 63 | 63 | 0 | 63 | 0 |
| foundation | 46 | 55 | 55 | 0 | 55 | 0 |
| headlong | 73 | 85 | 86 | 13 | 73 | 0 |
| lockout | 39 | 52 | 52 | 0 | 52 | 0 |
| midship | 55 | 66 | 66 | 6 | 60 | 0 |
| waterworks | 49 | 55 | 55 | 1 | 54 | 0 |
| zanzibar | 70 | 75 | 75 | 3 | 72 | 3 |
| containment | 51 | 62 | 62 | 2 | 60 | 8 |
| warlock | 44 | 52 | 52 | 0 | 52 | 0 |
| deltatap | 38 | 49 | 49 | 0 | 49 | 0 |
| turf | 67 | 74 | 74 | 2 | 72 | 0 |
| backwash | 41 | 52 | 52 | 1 | 51 | 0 |
| dune | 58 | 66 | 66 | 0 | 66 | 0 |
| elongation | 46 | 57 | 59 | 5 | 54 | 0 |
| gemini | 63 | 73 | 73 | 3 | 70 | 0 |
| triplicate | 52 | 60 | 60 | 3 | 57 | 0 |

Lower masks retain layer1 in259 records, layer7 in364, each of layers10/11/12/13
in590, layer14 in192 and layer16 in all1,267. There are no lower layers2,5,6,8,
15 or17. The supported lower preparation gate does not itself activate
shadow, dynamic-light, camouflage or fog schedules merely because those mask
bits exist. Their exact view/object eligibility and state remain independent.

## Verification boundary

The source metadata and pass-preparation gate is production-linked CPU
evidence. It does not prove live model shader selection, resident alternative
GPU descriptors, actual object-color animation order, per-view dynamic light
queries, geometry/PRT streaming transitions, or framebuffer equality. Those
services have separate source proofs and integration tests.

The CTest is `retail_multiplayer_model_material_lods`. Final normal and
ASAN/UBSAN runs with leak detection, the unchanged highest/BSP-lower baselines,
fixture/source/binary/log hashes and summaries are retained in
`analysis/multiplayer-shading/model-shader-lod-material-proof.json`.
