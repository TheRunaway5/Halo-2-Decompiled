# Scenario Euler conversion

`source_euler_basis` implements original `141CE0` and is the shared
`authored_basis` consumer. It evaluates sine and cosine above binary32 precision,
stores each result to binary32, then preserves the original product/add/subtract
order. The original x87 `FCHS` used for left.x remains a sign change, while up.x
and up.y retain their distinct SSE `0 - value` operations.

The prior implementation called float sine/cosine directly and used unary
minus for all negative components. This produced eight scenery constructor
node0 differences across the supplied maps. Untouched cold factory traces
captured actual `1421F0` inputs/local matrices and `142A60` composition banks,
isolating the first discrepancy to Euler conversion before local posing.
No local quaternion decoding, normalization, hierarchy or shared transform
composition was changed for this correction.

The independent untouched `141CE0` oracle covers:

- All 1,496 authored scenario placements in the 21 supplied multiplayer caches.
- Signed-zero/axis cases.
- 12,288 finite random angle triples, with ranges through ±100,000.
- 13,802 cases and 124,218 output words in total, all bit exact.

The production test retains 2,032 original cases, including every authored
placement. Fresh coherent native constructor comparisons now match all 278
SCEN 13-word node0 banks and all 278 cached spheres exactly. All 415 BLOC and
49 MACH initial cached-sphere fixtures also pass. The two remaining Triplicate
machine node0 signed-zero differences were isolated independently to local
affine matrix construction. The original `1421F0` writes rotation, scale and
translation directly; native identity TRS matrix products erased negative
zero before root composition. Removing those extra products now makes all 49
complete machine node0 banks exact. See `source-affine-pose.md` for that
separate correction and its original-instruction validation boundary.

This is an original-instruction differential over the recorded finite input
domain. It is not a claim that native double libm reproduces every x87
`FSIN/FCOS` argument-reduction edge, including inputs outside the x87 supported
range, or every hardware/FP-control configuration. The oracle records nearest
rounding, x87 control word `0x027F` and MXCSR `0x1F80`. The supplied authored
angles are included directly; no source outputs or signed-zero patches are
substituted into production.

Evidence:

- `analysis/multiplayer-shading/source-euler-original.json`
- `analysis/multiplayer-shading/scenery-initial-matrix-trace-original.json`
- `analysis/multiplayer-shading/scenery-initial-matrix-native-probe.log`
- `analysis/multiplayer-shading/scenery-initial-pose-after-euler.log`
- `analysis/multiplayer-shading/source-euler-machine-node-probe.log`
- `tests/fixtures/retail_source_euler.tsv` and provenance
- `tests/fixtures/retail_machine_initial_bounds.tsv` and provenance

Regular and ASAN/UBSAN runs cover Euler fixtures, device initialization,
415 prop constructor bounds and 278 scenery constructor/bounds lifetimes.
