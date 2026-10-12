# Dynamic model shader integration

`SourceDynamicModelShaders` retains one material bank for each literal shader
LOD while preserving the selected model geometry. The caller supplies the
original owner collector to body, held weapon, arms, and first-person weapon.
Shader LOD and geometry LOD are separate source decisions.

The all-map integration fixture comes directly from the supplied original
caches and resources. It records the 72-byte part words, shader datums, raw
shader bias, literal template LOD thresholds/layer ranges, and authored
pass/implementation/vertex-program identities. The generation tool is
`tools/audit_dynamic_model_shader_assets.py`; fixture provenance is retained
alongside `tests/fixtures/retail_dynamic_model_shader_assets.tsv`.

`retail_dynamic_model_shader_integration` checks 210 cases across all 21 maps:
Chief and Elite bodies/hands, and SMG, battle-rifle, and sniper held/first-person
models. The private normal and ASAN/UBSAN runs (including leak detection) passed:

| Check | Count |
| --- | ---: |
| Original expanded vertices / emitted parts | 1,128,015 / 924 |
| Near/far source coverage with 1/2/4-player policies | 1,260 |
| Authored resident passes / lower-LOD passes | 3,822 / 1,449 |
| Object-color consumers | 882 |
| Animated passes / lower-LOD animated passes | 1,743 / 651 |
| Animated bitmap consumers | 126 |
| Independent retained lighting bindings | 7,644 |
| Passes distinguishing the two retained lighting states | 1,953 |

Every resident pass is checked against its literal selected template and
original bitmap bytes, dimensions, mips, and sampler state. Shader selection
preserves vertex position, UVs, color, source part/vertex identity, baked/PRT
streams, skin influences, and the complete bind hierarchy. Opposite-order
view selection, retained-color copy-on-write, and stale animation-bank
controls verify that a different view/frame cannot mutate the resident bank.

The color/animation/lighting checks use the independently instruction-proved
binding helpers. Colors first rebuild the authored pixel bank, animation
applies its masks, and current lighting is bound in a per-view copy. The test
uses exact already packed/unpacked colors from the retained Zanzibar fixture;
it does not quantize that bank again.

This is a CPU cache/material integration test. It does not establish original
owner-light-cache eligibility, object registration chronology, controller
phase, view admission, rasterization, or full-frame image equality. The
diagnostic near/far owner spheres are explicit test inputs to the original
coverage arithmetic, not substituted live object bounds. Original arithmetic
authority remains the `source_model_shader`, `source_shader_lod`,
object-lighting, and color/animation untouched-routine fixtures.
