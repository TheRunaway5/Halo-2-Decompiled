# Chief visor environment response

The current Chief visor combines diffuse and environment shading in one pass.
Its highest shader LOD uses `lightmap_env`; the other two use
`lightmap_tex_env`. Changing only their facing/glancing strengths does not
execute the recovered May 2003 six-combiner environment response.

pStencil now separates that reflection into its own companion material. It
keeps the current visor bump atlas, reflection cube, selected bitmap frames,
both affine bump UV rows, source shader identity and selected shader LOD.
The companion uses the already validated independent Chief environment vertex
interface and the recovered six-combiner response. Its otherwise unused texture
stage 1 samples the current independent texture-accumulation alpha; reflection
adds after the base and baked shadow pass. Only the two isolated environment
strength bytes are zeroed in the original combined pass. Its diffuse controls,
tint and output alpha remain intact.

This adapter admits the exact Chief visor path and the two known compiled
control fingerprints. Missing carrier/mask residency, incompatible vertex
transforms, unsupported bitmap shapes or animated parameter overlays keep the
existing material path. It does not apply a faction preset or invent values
for other materials. Original does not draw the companion or suppress the
combined reflection. All code and tests live in the pStencil subfolders except
the small shared renderer and fragment-shader seams.

The companion has no authored template LOD/layer rows of its own. The renderer
retains the carrier's actual environment layer 8 policy separately. This avoids
misinterpreting the carrier's LOD table when the visor selects LOD 1 or 2.

## Reference binding evidence

The original initializer at `00144CD4` publishes the rasterizer globals row to
`00743A20`. Original texture override 1 resolves through `000962B9` to that
row's `+1C` reference: `rasterizer\vector_normalization`, datum `E24000CC` in
the pinned `ec_interior_test.map`. The final pass override is distinct from
the constructor's default fallback cube.

The original 64×64 cube has six faces, seven mips per face and padded source
storage. After decoding all 42 mip images, its 131,064 RGBA bytes match the
actual current Chief armor and visor normalization image exactly in both
Cairo and the opening cache:

`f370eb1086cdb9332bfa76472b12f410233775c351f58ba60c65b32919e747ad`

The read-only current binding audit and original comparison are retained in
`analysis/pstencil/visor-response-v1/`. This establishes the selected image,
not a complete original GPU frame or every original target/alpha producer.

## Verification and limits

The actual Cairo and opening caches each pass all three visor shader LODs,
including source ownership, exact bump/cube pixels and frame banks, UV rows and
vertex interface. Across 3,072 varied combiner cases per cache, removing the
two environment strengths leaves diffuse independent of the cube and preserves
output alpha. There are 2,933 positive cube controls per cache. Mutated compiled
controls and an unrelated material name are rejected.

The revised native close-up has 48,234 reflection-dependent pixels. Its point
shadow control darkens 470,708 pixels and brightens no channels. Original is
byte-identical to the previous campaign delivery and restores exactly after
pStencil. The three-character capture has 422,259 point-shadow pixels,
514,165 baked-shadow pixels and no shadow brightening. These actual Cairo
photos use 2560×1920 output and 5120×3840 internal targets. They are staged
current-game scenes, not matched original frames.

The full opening's native submission trace is separate from the CPU LOD test.
The delivered candidate submits the companion 367 times, all at shader LOD 0,
in its complete 222.66-second, 2,668-frame opening run. All 20 same-frame
Original roundtrips restore exactly; 14 image pairs have positive light and
shadow contributions, with zero shadow brightening or Vulkan validation
messages. The initial candidate separately submitted it 391 times at LOD 0.
Its trace's first field is the source color-buffer counter, which remains zero
in this presentation path; it must not be interpreted as a game-frame index.
CPU support for all LODs does not establish native draw coverage at every LOD.

Current independent texture alpha is an explicit adaptation boundary. Complete
May 2003 bump/texture/utility target scheduling, original alpha ownership,
matched GPU frames and later combined material families remain unfinished.
See [accumulation construction](ACCUMULATION_REFERENCE.md) and
[opening presentation](OPENING_CAMPAIGN.md).
