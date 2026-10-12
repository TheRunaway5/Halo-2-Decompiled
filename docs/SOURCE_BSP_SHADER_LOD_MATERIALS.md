# Authored BSP shader LOD material preparation

The 21 supplied original Xbox map caches contain **657 distinct BSP shader
references, 1,362 authored shader LOD records and 705 nonzero LOD records**.
Every lower record points to a different ordered layer-range bank from LOD0.
Shader LOD is separate from render-model geometry LOD.

`Material::source_shader_datum` identifies the actual shader reference, and
`selected_shader_lod` supplies the authored template index. The material
preparer must select that record's mask and ranges. Looking for a familiar
pass in LOD0 would retain effects and textures the original deliberately
removes at another projected size.

## Independent literal metadata

`tools/audit_bsp_shader_lods.py` reads the original version-8 cache bytes using
the independent asset audit parser. It collects unique references from BSP
material records at `+A4`, resolves shader `+20` postprocess definitions,
template `+58` postprocess definitions, and literal 10-byte LOD records.
It records the packed layer range, 32-bit available-layer mask, raw projected
height float word and every ordered packed layer range. Expected values never
come from native material preparation.

The frozen `tests/fixtures/retail_bsp_shader_lods.tsv` retains all 1,362 records.
`analysis/multiplayer-shading/bsp-shader-lod-literal.json` additionally retains
cache hashes, shader/template names, per-map masks/counts and fixture hash.

| Map | Distinct BSP shaders | Nonzero shader LODs |
| --- | ---: | ---: |
| ascension | 19 | 20 |
| beavercreek | 25 | 33 |
| burial_mounds | 9 | 11 |
| coagulation | 25 | 27 |
| colossus | 21 | 28 |
| cyclotron | 48 | 48 |
| foundation | 29 | 36 |
| headlong | 69 | 72 |
| lockout | 25 | 26 |
| midship | 28 | 49 |
| waterworks | 18 | 20 |
| zanzibar | 39 | 37 |
| containment | 20 | 27 |
| warlock | 25 | 23 |
| deltatap | 35 | 33 |
| turf | 57 | 53 |
| backwash | 32 | 21 |
| dune | 17 | 18 |
| elongation | 21 | 20 |
| gemini | 47 | 50 |
| triplicate | 48 | 53 |

All 705 lower masks include layers3 and16. Twelve include accumulation layer1;
104 include self-illumination layer7; seven include alpha bloom layer17; 149
include each of layers10,11,12,13. No lower BSP record includes layer2,6,8 or15.
In particular, the BSP water-pool and edge material references have no authored
nonzero LOD. Independent infinite-water geometry has its own source scheduler
and is outside BSP-part shader-LOD selection.

## Material checks

The `--lower-lods` mode of `tests/world_material_tests.cpp` exercises every
nonzero authored BSP record in static lighting implementations0 and1. It
checks the selected layer/pass/implementation against the literal record,
retained shader identity/index, full pixel program including fragment offsets,
vertex bytecode, all128 constant float words, complete reset-plus-authored
raster state, blend factors and sampler fields. It independently resolves each
selected bitmap remap from the original material, then compares prepared
format, dimensions, layers/depth, all image bytes and the full mip layout.

The pass set must exactly match the selected layer3 plus its authored
accumulation/overlay/self-illumination/environment/bloom passes. A missing or
unexpected pass fails even when the main lighting pass can draw. No failures
or unsupported lower bindings occur in the supplied 705-record static bank.
The checks cover **1,410 lighting variants / 1,656 selected world passes**.

Where the actual map contains a source fog palette, `prepare_bsp_planar_fog`
also exercises all three selected layer16 implementations. It checks literal
program/fragment-offset/vertex/constant/sampler/raster output. These cover
**936 planar variants**; maps without a palette correctly admit no planar pass.

The original highest-LOD mode remains separate and keeps its baseline:
657 referenced shaders, 566 lighting shaders in each static implementation,
164 accumulation passes, 88 emissive passes, 11 standalone overlays, two
environment passes, 71 layer15 passes and ten bloom masks.

## Verification boundary

This is production-linked CPU material-preparation evidence against original
asset banks. Explicit selection of every record proves the preparer consumes
the supplied index; it does not prove a live camera chooses that index.
The separate source projected-coverage, `1BF50` selection and BSP per-part
selector supply that producer. GPU resident descriptor updates, per-view
selection and source-matched framebuffer appearance are separate tests.

Layer10/11 shadow and layer12/13 dynamic-light scheduling are not activated by
this static preparer test merely because their metadata is present. Their
source admission/services remain separately scoped. This test does not claim
all original effects are drawn or classify independent water as ordinary BSP.

Final normal and ASAN/UBSAN runs, binary/source/fixture hashes and summaries
are retained in `analysis/multiplayer-shading/bsp-shader-lod-material-proof.json`
and the `bsp-shader-{highest,lower}-lod-*` logs. Sanitizer runs enable leak
detection. The highest and lower modes run separately so their coverage cannot
silently replace one another.
