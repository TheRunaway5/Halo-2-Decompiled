# Texture accumulation

Status: **campaign-v10 is installed** through `./Halo2-pStencil-Campaign`.
The recovered texture stage passes native group/visor controls and the complete
opening. The regular Elite's blue armor response matches the accepted v8 image
(ratio1.0, mean channel error0.0000453). This is a compatible current-asset
adapter, with complete original presentation parity still unfinished.

The original target11 producer selects builtin3 in `default_texture` and
`default_texture_zwrite`. Its complete constructor outputs are retained in
`2003-presentation/accumulation-construction-v1/reference.json`. The default
60-word program has SHA256
`cba50a5a3d7fe85d954df37d2cb0d575253907b77dbb4baa442e40c449f6f848`.

The primary executable's descriptors at 004C2F94 and 004C2FA0 identify
0048F494/0048F498 as `ssc_texture_diffuse` / `ssc_texture_specular`; both contain
binary32 1.0 in the unchanged image. These are authenticated initial/debug-global
values, rather than material gloss presets or observed live-console state.
`texture-accumulation-inputs-v1/default-globals.json` retains the literal bytes
and executable hash. The fixed program generator checks those descriptors and
matches the retained unchanged original constructor output.

Original vertex programs 93/94/95 retain base/detail texture coordinates and V10
color. Program95 transforms source UV through C13/C14, scales detail by C12.xy,
and moves V10 to oD0. Original `0023A300` color override4 reads actual owner color
when available. Current `stencil_texture` has its own completed base/detail UV
and current owner color/multiple color-change regions. The candidate preserves
those current producers, rather than substituting old images or fixed colors.

Compatible current texture programs have one, four or seven combiners with an
authenticated base/detail/color-change subgraph. The adapter computes the
completed current color factor by evaluating that subgraph with unit base/detail
product, while retaining sampled alpha and color-change inputs. Original builtin3
then owns the base/detail RGB and material-alpha output. This adaptation keeps
current Elite rank colors and Chief color regions; it is not a claim that later
assets had those exact original compiled programs. Other subgraphs retain the
current route until their input crosswalk is established.

The original layer-begin clear is now checked through primary `00033A20`:
`render_layer_texaccum` supplies color flag 1 and color 0, which become D3D color
clear bits 0xF0 with transparent-black color. This does not clear depth/stencil.
The authenticated exports are in `accumulation-state-2003-v1/`. The bump-layer
`00031AA0` helper instead reaches the palette method at 00339710; it is not a
normal-target clear. Unobserved original live scheduling remains a separate gate.

The first native candidate exposed an integration regression despite passing
scalar/GPU composition checks: changing the pass type bypassed the existing
primary-matched regular-Elite neutral-detail policy. Its native blue-armor
response fell to 31.17% of the previous image. That candidate was not installed.
The conversion now requires the completed neutral-detail context from its caller
and retains the primary RGBA(128,128,128,255) effective detail sample for those
exact Elite materials. Original and Arbiter detail images remain authored.
The conversion regression failed before the fix and passes afterward. The
corrected native color gate also passes. See v9/context-{red,green}-tests and
elite-color-red-tight.json, then v10/elite-color-green.json.

The candidate uses the actual current texture target and current authored
geometry/UV/depth/stipple. Exact original names use the primary default-texture
samplers; later assets retain their authored addressing/filtering. Independent
and combined roots select a converted companion only from the same shader datum
and CPU input transform. Disabling the adapter must preserve the prior authored
LOD routing. The current pass remains available for Original mode.

The focused constructor/color test passes 8192 independent scalar cases across
actual Chief armor, visor, Elite torso and Arbiter armor programs. The routing
test checks independent and combined roots, foreign shader/CPU-transform
isolation, and unchanged authored LOD selection on the current route. These are
source/arithmetic checks. An additional2048 GPU composition fixtures match the
independent scalar result. The native group and visor controls darken426,394
and472,817 pixels respectively, with no brightening;278,862 and353,469 pixels
have positive specular response. Their internal stencil targets are5120×3840.

The complete native opening returns0 in228.54 seconds with1180 main frames,
zero script faults and zero Vulkan validation messages. All20 Original
roundtrips and20 material restores match exactly. Fourteen light/shadow pairs
are positive, with no shadow brightening. The earlier rejected v9 opening
sampled13 positive pairs; its4700 target landed at tick4748 and showed no
light/shadow difference. It remains a rejected diagnostic, not a delivery receipt.

All new validation builds and raw frames run through `tools/run_tests.py` or
`tools/run_test.py` in owned scratch. Small reports remain under
`analysis/pstencil/texture-accumulation-native-v9/` and
`analysis/pstencil/texture-accumulation-native-v10/`.
`texture-accumulation-native-v10/retained-artifacts.json` lists the requested
runtime,71 SPVs and native photographs with their sizes and hashes. The
opening-result receipt lists its three selected native images. Temporary
build trees, archives, object files and raw frame controls have been removed.

Complete original lighting-model selection, live overlays, all current material
families, exact target scheduling and a matched original GPU framebuffer remain
unfinished. This stage must not be described as complete presentation parity.
