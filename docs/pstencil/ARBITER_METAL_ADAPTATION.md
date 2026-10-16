# Arbiter metal and the current combined visor

Arbiter is a later asset, so no 2003 Arbiter material is claimed. The user's
requested distinction applies only to the current exact `dervish_armor` and
`dervish_helmet` shader paths. Cloth, flesh, eyes, teeth and ordinary Elite
materials retain their own response.

The current authored layer-13 sphere specular shader still supplies the bump,
mask, selector, highlight shape, texture residency and destination-alpha blend.
Three additional full-strength contributions use the same completed specular
light RGB, under the same light's stencil test. Together with the original
contribution, this gives the armor a substantially stronger highlight while
retaining its authored mask and shape. A brightness scalar above one would clip
in the existing packed-constant interface; separate contributions preserve the
base pass. This is a user-directed art adaptation, not an original-build value.

The recognized current `lightmap_env` implementation 2 / `lightmap_dynamic_env`
eight-combiner family stores isolated facing/glancing reflection strengths in
words 17/25 alpha. Scalar consumer probes prove that zeroing these removes
stage-1 environment contribution and preserves output alpha. By contrast,
words 13/21 RGB also alter diffuse lighting, so old reflection tints are not
copied into those fields.

Within that exact family, Arbiter's completed facing/glancing strengths double,
with the existing byte quantization/clamping. Current helmet/armor values
`.4/.6` become `.8/1` after clamping. Other selected shader
families keep their current environment path. The native capture log records
whether a combined pass was actually selected.

Chief's exact-path visor reference agrees across 140 original resident records
(70 caches, two lighting models): facing brightness `.4`, glancing `.2`.
These strengths now reach the recognized current combined consumer. Its current
diffuse tint, environment cube, geometry, normal input and alpha remain owned
by that shader. This is not the full original six-combiner response on the
combined visor; the independent response family has that separate support.

The first v7 25% adjustment was too subtle in the user-reviewed screenshots.
Live-v9 strengthens the metal treatment while preserving the same camera,
light, diffuse inputs and masks for comparison.

`pstencil_armor_adaptation` loads actual current materials and checks metal-only
companions, preserved shader/mask controls and position/gel/palette inputs,
cloth/flesh exclusions, combined-consumer discrimination and scalar/GPU
agreement. Native staged level photos additionally omit only the polished
companions: the resulting difference must only brighten the actual metal,
with positive coverage in the Arbiter and group views. Original must restore
exactly after switching modes.
