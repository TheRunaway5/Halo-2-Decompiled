# Original frontend text color, shadows, and metrics

The native painter previously multiplied every glyph's atlas RGB by the raw
authored text RGB, submitted no text shadow, and omitted each horizontal
bearing from cumulative character advances. The retail Profile Name keyboard
therefore tinted controller symbols, lacked the black label shadow, and
measured `Halo0001` seven pixels too narrowly.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and the original `conduit-12`/`conduit-13` font records. The executable routines
and corresponding decompiled source establish these operations:

- `13EC70` reduces RGB only when all three channels exceed 0.68, scaling by
  0.68 divided by the minimum channel (clamped to 1). `13EB60` clamps each
  component. `13F470` truncates the resulting components multiplied by 255
  into packed ARGB.
- `13F660` classifies private-use glyphs. `13F700` changes eligible glyph RGB
  to `0xFFFFFF` while preserving alpha; their original atlas colors therefore
  survive. Ordinary letter glyphs retain the packed text RGB.
- `1F490` submits a black shadow before each foreground glyph, shifting the
  already clipped geometry by the draw scale in both axes with identical UVs.
  The shifted shadow is not clipped again.
- Standard text setup `253765` maps definition flag 8 to buffer flag 4,
  disabling the shadow, selecting font 1 and the shared scale. Button setup
  `253CFF` does not map that flag: the authored font, unit scale and shadow
  remain. `frontend_text_setup` exposes this complete caller-aware result;
  the glyph material, measurement, clipping and caret must all consume its
  resolved font and scale together.
- `13EF40` measures each advance as `advance + bearingX + kerning + width`.
  `13F700` adds the same bearing to the cumulative draw position. Both native
  operations now include it with the original float evaluation order.
- The original measurement/draw calls to `122570` supply previous character
  in EDI and current character in EBX. The native frontend now looks up the
  serialized kerning pair as `{previous,current}`. This prevents the `P,A,-1`
  pair from being incorrectly applied to CAPS LOCK's `A,P` while retaining
  the original pair in BACKSPACE.

With unit animation alpha, the original authored keyboard RGB packs to
`FF6293C1` (98,147,193); the focused shared title RGB packs to `FFACC3D8`
(172,195,216). These packed colors come from executable calls using the actual
menu metadata, rather than rounding the asset's decimal color values.

`tools/oracle_frontend_text_style.py` executes unchanged instructions and saves
the results under `analysis/frontend/profile-regression`. Its TSV freezes
1,032 byte-exact float normalization cases, 1,031 glyph classifications,
1,088 final packed foreground/shadow colors, and 32 complete text/button
setup flag cases. The JSON also records 36 actual clipped shadow/foreground
draw cases at scales 1, 0.6 and 1.5.

Original `13F0E0` drawing and `13E9C0`/`13EA60` measurement of `Halo0001`, with
the real conduit-13 glyph records and projected bounds `{167,72,207,560}`,
produce endpoint `{137,183}`, ink bounds `{167,73,189,137}`, and line/caret
bounds `{167,137,187,138}`. The keyboard's `22D13D` output retains projected
top/left and uses the ink's bottom/right: `{167,72,189,137}`.

The focused production regression first failed on the missing black shadow in
an actual retail keyboard draw. It now checks the original oracle cases and
the real painter's shadow order/offset, packed label color, private glyph RGB,
and cumulative bearing measurement. Its logs are `text-style-red-tests.log`
and `text-style-green-tests.log`.

A later fixture, `keyboard-labels-original.tsv`, executes the original font
corridor for 77 retail action/footer glyphs. The production painter matches
each original geometry and packed color after the kerning direction fix and
the original `22FF53` screen subtitle-color selection. Its source and scope
are recorded in `FRONTEND_KEYBOARD_SOURCE_PROOF.md`.

The remaining half-pixel Space/Backspace icon edges exposed a separate atlas
issue. Original `1F5C0` clears each decoded glyph's one-texel surround to
transparent white (`0x0FFF`), while the native atlas left it transparent black.
The native loader now copies that original surround policy. An unchanged
`1F5C0 -> 140960 -> 122610` fixture freezes 896 actual glyph/interior/surround
words and the native atlas regression compares every word. The source proof
and two recorded edge-pixel reconstructions are in
[FRONTEND_FONT_ATLAS_SURROUND_SOURCE_PROOF.md](FRONTEND_FONT_ATLAS_SURROUND_SOURCE_PROOF.md).

The probes supply ready font-cache lookups and pixel readiness, empty string
binding, widget parent/definition lookup, and font prediction. Original color,
measurement, setup and callback arithmetic executes unchanged; the final
graphics submission records vertices instead of issuing GPU commands. This
establishes the supported text operations, not a complete framebuffer match.
Complete atlas residency and GPU sampling precision, general line wrapping,
text pulse phase, and full widget projection remain separate comparisons.
The actual Settings small-text fixture below additionally covers its supported
nonunit clipping cases.

The Settings Mjolnir diagram exposed a separate setup integration error. Its
seven labels are authored with flags9/font9, but standard253765 resolves
flag8 to runtime font1. The native painter instead used the authored
conduit-16 font and a hardcoded0.6 scale. The actual `mainmenu.map` shared
globals at wigl+48/+4A/+4C contain bytes `3200DB009A99993E`: signed interval
50ms, cursor U+00DB and scale float word `3E99999A` (0.3). These are source
metadata, rather than fitted screenshot sizes. The previous original setup
fixture supplied a synthetic0.6 shared scale; it proved the setup branch but
could not justify using0.6 for the actual menu.

`FrontendContent::typewriter` now retains all three actual fields, and
`frontend_text_setup(flags, authored_font, button, shared_scale)` selects
font1/shared scale/no shadow only for standard flag8 text. Standard text
without flag8 and buttons retain authored font/unit scale with the original
shadow policy. All52 direct standard flag8 definitions in the original
cache, across screens1,12,19,38,178 and210, author font9; this is a common
source setup rule, not a Settings-specific font adjustment. No actual skin
text or button definition has flag8, although synthetic button controls
remain necessary to preserve the different253CFF caller contract.

The static metadata, complete Settings bounds/strings/animation tracks,
font identities and source inventory are retained in
`analysis/frontend/settings-text-source-audit/metadata-and-font-layout.json`.
`frontend_text_setup_tests.cpp` consumes the independently executed original
setup fixture and checks exact runtime font, scale words and shadow, plus
the actual cache fields and all52 definitions. Its compilation/execution
and the integrated painter/typewriter captures remain separate acceptance
gates; the pure setup helper alone does not prove reveal timing, complete
glyph layout or framebuffer parity. The separate original440-glyph final-quad
fixture and its production comparison are described in
[FRONTEND_SMALL_TEXT_SOURCE_PROOF.md](FRONTEND_SMALL_TEXT_SOURCE_PROOF.md).
