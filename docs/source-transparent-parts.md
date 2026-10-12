# Original ordinary part metadata

`source_transparent_parts` retains the input bank consumed by retail `45CE0`
and ordinary `41040`. It keeps the literal 72-byte part, its section/part
ordinal, shader datum, raw shader priority flags at +0x16, general command
flags at +0x3C, and every authored template LOD layer mask. Actual per-view
shader selection and ordinary admission remain caller services.

`Cache::load_source_transparent_bsp_parts` returns cluster sections, instance
definitions, actual instances and the ordered 20-byte transparent-plane bank.
Cluster centers come from literal part +0x10. Instance centers come from
instance +0x3C, independent of each part's center. Cluster AABB +0x54..0x68
and instance radius +0x48 are retained for the separate original coverage
helper. No triangle-center or fitted-radius substitutions enter this module.

The transparent-plane key is two unsigned shorts: cluster or instance-definition
ordinal, then `(part * 2) | instance`. Original `1E1C0` prepends each insertion,
so the final matching authored key wins. Instance planes use the exact ordered
separate multiply/add sequence from `45FA6..46056`; their D includes authored
scale and translation. Cluster planes remain literal.

`Cache::load_source_transparent_model_parts` accepts the actual render-model
datum and ordered selected section IDs. It preserves duplicates and empty
collectors. Model centers use actual posed context +0x44 matrix translations:
section +0x2C rigid-node index, or node0 for NONE. Raw render-model +4 mask4
requires an explicit actual render-context node-remap result. The helper
rejects missing remaps and out-of-bank nodes. Model planes are absent.

`source_transparent_model_collector` retains both original region and section
ordinals, including gaps from NONE sections. This matters for model depth bias.
Original `4C0D0` writes a parallel region sort-order bank at geometry context
+0xC2. The geometry context starts at whole collector +4, so `133F70` reads the
same words at whole collector +0xC6. `2CBF0` stores the low nibble in draw-unit
+0x1B. `45CE0` produces zero for nibble0, otherwise `(nibble-5)` times the literal
float word 0x3A83126F. BSP centers/instances explicitly have zero bias. Actual
model count other than1 clears the region bank.

All 741 automatic Slayer render-model placements in the supplied 21 caches
have zero authored region sort orders for their original selected variant.
That is an asset census, not a guessed default nibble. The loader still keeps
arbitrary signed sort orders and the helper retains their low nibble.

Validation passes 12,316 untouched original instruction cases and 135,476
output words with no differences. `45CE0`, pose-bank addressing `1D6B0`, original
plane-bank insertion/lookup/hash/compare, `16D180` and `4C0D0` execute unchanged.
The sole service hook supplies explicit admitted material visibility at
`184000`; it replaces no center, transform, pose, bias, plane or LOD math.
`4C0D0` independently executes model counts0/1/2 for all741 placements.

Frozen tests cover 522 arithmetic cases and 11,470 authored metadata rows.
The native census checks all 21 maps, 6,584 parts, 1,846 instances, 837 planes
and 741 placed models. It verifies 1,791,969 expanded vertex section/part
identities against the authored bank and retains model original-vertex links.
Normal and ASAN/UBSAN/leak tests pass. Four uncertain Zanzibar pillar
permutations use explicit retained fixture bytes only; these fixtures do not
close the production creation-RNG chronology boundary.

This module does not establish resident asynchronous cache timing, source
visibility/collector chronology, actual node-remap producer state, per-view
shader LOD admission, transparent sorting integration or framebuffer parity.
Those services must submit their actual source state rather than substitute
metadata order, a fixed shader LOD or triangle centers.

Evidence:

- `tools/differential_source_transparent_parts.py`
- `analysis/multiplayer-shading/source-transparent-parts-original.json`
- `tests/fixtures/retail_source_transparent_parts.tsv` and provenance
- `tests/fixtures/retail_source_transparent_parts_assets.tsv` and provenance
- `analysis/multiplayer-shading/transparent-parts-ctest.log`
- `analysis/multiplayer-shading/transparent-parts-asan-ctest.log`
