# Source BSP shader selection

`SourceBspShaderScene` owns resident ordinary BSP shader alternatives and
per-view authored part bindings. It is separate from geometry LOD and from
portal admission. The multiplayer runtime constructs it before planar-fog
materials, water and object material slots, then selects each view after
`134390` produces that view's cluster and instance queues.

## Source inputs and ownership

The input is the actual `2FD90` render-camera bank, physical signed-short outer
camera endpoints, local-player count, `4BA019` view modifier and the exact
`SourceBspAdmission` result. Source authored identities survive triangle
expansion as `Vertex::source_section` and `source_part`. Draw grouping retains
these identities; parts sharing a shader are distinct source draw tokens.

`133390` computes cluster coverage from the authored cluster bounds.
`133520` computes instance coverage from its literal center/radius, including
the small-radius factor. `2CBF0/16AE0` quantize the parallel collector byte.
`45CE0` resolves a nonpositive authored part scale to one; `41040` combines
that scale, the actual collector byte and `2C490`'s shader multiplier.
`1BEB0/1BF50` then select the authored shader template LOD. Shader bias is
the raw shader's `+3C` field, independent of model geometry LOD.

The instance alpha is the original `133520` byte derived from the unquantized
coverage. Alpha zero omits that instance before shader selection. Other
instances retain the exact `1C290` pattern level on their vertices; clusters
retain the fully populated pattern. The fragment shader uses the original
layer gates and hardware-verified within-byte bit orientation and global row
phase. All 17 pattern levels pass pixel checks in one and two views at 480p,
720p and an odd-sized native target; see `source-stipple.md`. Physical Xbox
reference frames establish the phase at 480 lines. Odd-height targets retain
that explicit native phase; physical odd-height behavior remains unverified.

The scene prepares every actual selected-LOD layer/pass bank in advance, using
the original per-vertex or per-pixel static lighting classification. Each view
remaps its own vertex copy into these slots. An exact resident material/lightmap
pair bank is built from the original geometry's lightmap bindings, allowing a
later selected alternative to use its proper baked-lightmap descriptor.
The renderer does not substitute a different geometry's lightmap. Infinite
water retains `3C3C0`'s explicit LOD0 service; ordinary pool/edge shaders use the
ordinary selector. All nine supplied pool and two edge banks have one LOD.

## Verification and limits

The original instruction helper and raw shader-bank evidence is recorded in
`SOURCE_SHADER_LOD_PARITY.md`. All 705 lower BSP shader LODs across the 21 maps
have independently checked layer ranges, programs, constants, raster states,
samplers and bitmap bytes/mips/dimensions. This includes 1,410 static lighting
variants, 1,656 world passes and 936 planar variants; see
`SOURCE_BSP_SHADER_LOD_MATERIALS.md`.

The completed `live-source-shader-binding-all-maps` batch passes all 78
captures: all 21 maps with one, two and four players, plus selected 480p,
1080p and 4K views. Binary and shader hashes remain unchanged within the batch.
The selected resident bindings and all 17 fade-mask levels pass with Vulkan
synchronization validation. The retained Zanzibar original-frame comparison
preserves the sampled first-person result. Those samples do not establish
whole-frame equality, original startup state, model shader selection or a
physical Xbox rasterization match. Complete transparent scheduling and live
object registration remain their own source producers.
