# Retail keyboard bitmap/text alignment

The geometry regression in `tests/frontend_tests.cpp::keyboard_geometry`
opens **Choose Profile → Create New Profile → Profile Name** through the real
`MenuState`, then examines the quads returned by `FrontendPainter::draw` using
the original `mainmenu.map` button definitions, textures, and fonts.

Authority:

- `assets/disc/default.xbe` SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- `assets/disc/maps/mainmenu.map` SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Original button renderer `0x253EA8`, disassembled in
  `analysis/frontend/profile-regression/draw-0x45a628.asm`.
  It sign-extends the horizontal bitmap offset from definition `+0x2C`,
  the vertical offset from `+0x2E`, and computes the image top with
  `subss xmm4,xmm0` at `0x253FB0`. Horizontal offset is added at
  `0x253FCB`.
- Original text baseline uses the font ascent and glyph bearing independently
  of the button bitmap offset; see `function_13f0e0` and `function_13f700` in
  `tools/reference/halo2-decompiled/src/unknown_13e8a0.cpp`.

All 47 retail keyboard button definitions have bitmap offset `(0,-5)`.
The previous painter added that Y offset to the authored top, placing key art
10 authored pixels lower than the original subtraction. This made labels
appear too high relative to the key borders. The correction changes bitmap
placement and preserves the text bounds and font metrics.

The regression checks the positions of the 45 visible key bitmap quads
(including the selected key), as well as the first digit's unchanged `conduit-12` glyph
baseline. It failed before the correction with:

```
keyboard key art offset differs from original 0x253EA8; labels appear too high
```

Recorded feedback:

- Before: `analysis/frontend/profile-regression/keyboard-geometry-red-tests.log`.
- After: `analysis/frontend/profile-regression/keyboard-geometry-green-tests.log`.
- Re-run: `ctest --test-dir build-frontend -R '^(frontend_bounds|retail_frontend)$' --output-on-failure`.

## Visibility and focus color

The original profile-name keyboard type has flags `2`. The original
`flags & 0xA` branch hides button definitions `43`/`44` (Symbols/Accents) and
shows the inactive definitions `45`/`46` at their authored positions. Those
inactive keys are visible and cannot take focus. Thus the visible authored
indices are `0..42,45,46`, while all 47 definitions remain in the cache.
The regression checks that selecting any of `43..46` leaves focus on DONE.

Original button color getter `0x253E24` returns the authored definition RGB
for an unfocused button, or the retail `wigl` title RGB at `+0x16C` for a
focused button. `tools/oracle_frontend_profile_skin.py` executes the original
getter with bounded focus and tag-lookup seams and compares returned float
bytes to those actual source fields. For this keyboard the RGB values are:

| State | Red | Green | Blue |
| --- | ---: | ---: | ---: |
| Unfocused normal key | 0.388235003 | 0.580392003 | 0.756862998 |
| Focused key | 0.678430974 | 0.764706016 | 0.847059011 |
| Inactive key | 0.211765006 | 0.372548997 | 0.537254989 |

The painter regression checks the first unselected digit and focused digit
foreground after the original one-pixel black shadow. Original font packing
produces RGB bytes `(98,147,193)` and `(172,195,216)`, respectively; eligible
private controller glyphs instead retain the texture's RGB. The renderer applies
the widget's own animation scale; it does not add the painter's previous
unfocused brightness multiplier `0.65`. The five-second steady keyboard
animation ends at opacity `1` for both checked glyphs.

The original Title → Choose Profile → Profile Name flow removes the profile
channel on keyboard entry and retains the title channel beneath one popup
boundary. The test locks that boundary separately from the bitmap geometry;
the channel predicate and live-Xemu checks are documented by the popup work.

Additional feedback:

- Visibility before: `analysis/frontend/profile-regression/keyboard-visibility-red-tests.log`.
- Button color before/after: `analysis/frontend/profile-regression/keyboard-color-red-tests.log` and `keyboard-color-green-tests.log`.

These checks prove the bounded geometry, visibility/focus state, and steady
RGB/opacity corrections. They do not prove complete keyboard pixel parity,
intro/focus transitions, GPU popup pixels, or aspect-ratio presentation.

The input-field layer and generated default-name regressions are documented
separately in `docs/FRONTEND_KEYBOARD_SOURCE_PROOF.md`, with an executable
original constructor/comparator/generator/string-setup oracle.
