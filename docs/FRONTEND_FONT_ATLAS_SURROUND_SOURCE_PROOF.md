# Original glyph atlas surround

The remaining Space and Backspace icon edge differences came from the native
font atlas's transparent-black surround. Original `1F5C0` clears a one-texel
surround to A4R4G4B4 `0x0FFF`, which is transparent white, before copying each
decoded glyph. Transparent RGB contributes to linear filtering even when its
own alpha is zero. The icons' source-proven half-pixel positions sample that
surround at their left edge.

The [oracle](../tools/oracle_frontend_font_atlas_surround.py) executes unchanged
`1F5C0 -> 140960 -> 122610` from original XBE SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`. It consumes
the actual conduit-12 compressed glyph streams for Space `E105`, Backspace
`E102`, and the letter C. The supplied pixel cache is ready; the texture-lock
boundary returns a linear 256x256 A4R4G4B4 buffer. The execution stops at
`1F9B5`, after the original clear/copy and before texture upload. Each original
function is counted once per glyph.

The [frozen fixture](../analysis/frontend/profile-regression/frontend-font-atlas-surround-original.tsv)
contains all 896 texels of the three glyph rectangles and their one-texel
surrounds. SHA-256 is
`ed286efd7755ebb758972694c45e70515ac80ada62f9009b1fe545ed9494a01b`. The
[JSON](../analysis/frontend/profile-regression/frontend-font-atlas-surround-original.json)
retains the glyph headers, compressed-stream hashes, original function counts,
and words. Every surround texel is `0x0FFF`.

The separate [font material probe](../analysis/frontend/profile-regression/font-combiner-original-audit.json)
executes original `1F070` descriptor construction and unchanged `35B90`
sampler/program construction. It selects addressU/V3 (clamp), mag/min/mip2
(linear), blend0, and a texture-times-vertex RGB/alpha program, SHA-256
`4975670784843c86693f87b1598716d991b2e8e88591781ed9efb5e0f4a287fd`.
Its declared inputs are a ready 256x256 linear font bitmap and viewport;
graphics submission is excluded. Native font materials now use the original
clamp addressing on both atlas axes. The two glyph samples here are inside the
atlas, so this separate addressing correction does not affect the reconstruction.

The [focused edge report](../analysis/frontend/profile-regression/keyboard-icon-edge-cause.json)
retains the two discriminating pixels from binary `84eecfd0...` and the original
keyboard reference. At Space's left edge `(140,366)`, its first glyph texel is
`0xF111`. The original half-pixel geometry linearly samples that texel and the
surround equally. Transparent-black padding reconstructs the captured native
RGB `(9,26,45)`; transparent-white padding reconstructs original `(73,90,108)`.
Backspace's first texel at `(300,336)` is `0xD248`. The same difference explains
its 55-level edge error, with at most one RGB level between float reconstruction
and the retained original result. Exact host/NV2A filtering and blend rounding
precision remains outside this proof.

The native font loader now fills that one-texel surround with RGBA
`(255,255,255,0)` before copying decoded glyphs. Atlas packing, positions, UVs,
and text colors are unchanged; font material addressing separately follows the
original clamp descriptor above. The regression compares all 896
original words with the real native font atlas, including glyph interiors.
It first failed at Space's top-left surround; retained red/green results are
[here](../analysis/frontend/profile-regression/font-atlas-surround-red-tests.log)
and [here](../analysis/frontend/profile-regression/font-atlas-surround-green-tests.log).

This verifies the local glyph copy and surround policy. It does not prove
complete cache residency/eviction, the original atlas's entire lifetime, or
whole-frame keyboard parity. The full screenshots still have independently
timed backgrounds and caret phases.
