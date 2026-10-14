# Cairo Station original environment pass

Authority: original Xbox cache assets and the unchanged English OG Xbox TU1.5
executable. This is a native pass-admission correction, not a completed matched
original-frame comparison.

The native Cairo capture rejected thirteen authored spacestation materials at
layer8 / implementation0. Their vertex owner is
`rasterizer\vertex_shaders_xbox\stencil_env`; the environment guard admitted only
`stencil_env_bumped_tangentspace`. The new guard accepts both original names in
the world environment path. It retains the selected original instructions,
declaration, register combiners, texture states and pass order. Dynamic and PRT
guards retain their independent contracts.

The actual Cairo `stencil_env` datum is `F7A2156A`. Its world class0 program has
nineteen original instruction slots and declaration semantics `[0,3,4,5,6]`.
The program SHA-256 is
`6d33c1f3c06089097cbb03b049991a6b4146c59ca2baedc577083aa6aaa72f88`.
The existing native interpreter executes this program; no replacement shader
arithmetic or hand-tuned lighting constants were introduced.

`original_campaign_environment_materials` reproduces the rejected original
pass before the correction and checks thirteen materials, twenty-six authored
LODs and fifty-two prepared environment bindings afterward. The final focused
suite passes sixteen checks, including original model ranges, raw skinned
geometry, GPU vertex outputs and multiplayer material/LOD regressions.

Fresh native Cairo captures retain the same controlled Marine squad, camera
and tick at1280x720 and1920x1080. Both report zero environment-program
rejections and zero Vulkan validation errors. The720p before/after changes
161,150 pixels, with maximum channel change16. These are native before/after
frames, not matched original frames. The remaining pale scene shapes have not
been attributed to this pass and remain unresolved.

Evidence: `analysis/shader-parity-20261013/campaign-environment-red-v2.log`,
`regressions-v4.log`, `source-overlay-v3.json`,
`cairo-native-v2/marines.png`, `cairo-native-1080-v1/marines.png`, and
`campaign-environment-before-after-v1.json`.
