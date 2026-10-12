# Original fog clear RGB and sky density

Later [clear scheduling verification](SOURCE_VIEW_CLEAR_SCHEDULING.md) observed
the actual original caller control word `023F` and fixed a reserved-bit admission
error in the packer. The expanded 67,968 packing cases and captured caller now
pass. The results below retain the earlier phase's explicit controls and limits.

The native multiplayer atmosphere sampler now performs the original terminal
fog-derived clear RGB selection before applying the authored sky override.
The baseline differed in 138 of 2,163 forced-cluster samples: 129 on Backwash,
7 on Ascension, 1 on Cyclotron and 1 on Lockout. The corrected sampler matches all
2,163 original RGB triples bit for bit across the 21 supplied multiplayer maps.
The same original opaque-sky branch also forces sky-bank density to 1 after
classification. That shader input differed in 22 samples on Ascension,
Cyclotron, Lockout, Zanzibar and Dune; all 2,163 final density words now match.

## Source and implementation

The authority is the unchanged disc executable, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
`14B360` resolves the visible sky and independently produces its model-presence
byte and authored flag 32 override. `12E5E0` zeroes its output bank, aggregates
and classifies fog, then selects clear RGB at `12E68F..12E88B`. `2B790` applies
the authored override afterward.

`source_fog_clear.cpp` ports the RGB selection as a pure operation. With a sky
model, opaque sky fog supplies its premultiplied RGB; otherwise the previous
RGB remains. Without a sky model, zero planar density permits opaque primary,
secondary or weighted fog selection. The weighted branch preserves the
literal SSE operation order. Its interpolation field is bank +11C, distinct
from planar transition +118. Copied colors are not newly clamped.
The subsequent `12E6EC` density write feeds shader scalar extern 32 and
premultiplied color extern 30. It occurs after classification and does not
recompute the earlier classification or premultiplied sky RGB.

`BspAtmosphere::Sky::model_present` comes from the real sky model datum.
`BspAtmosphere::sample` supplies that independent control, the computed fog
banks, the original initial zero RGB and the existing interpolation field.
The authored override retains final precedence. Every local viewport uses
its own atmosphere sample; this arithmetic has no display-resolution input.

`source_clear_color_rgb8` separately ports `131ED0`. It uses integer operations
on binary32 operands to preserve x87 precision/rounding, masked integer
indefinite and per-channel byte wrapping. It neither clamps the RGB nor
changes the host floating environment. The x87 control word is explicit.
This helper is verified but is not yet connected to live clears: the original
caller FP context and complete clear-event lifecycle remain separate inputs.
The literal sky-control, terminal fog and packing instruction blocks match
the supplied disc, map-pack update and selected TU1.5 images after masking
only relocated absolute data addresses. Their literal 1 and 255 words are
also checked. This is a block comparison, not execution of every updated
upstream aggregation routine.

## Verification

The records are under
[`analysis/multiplayer-shading/fog-clear-color`](../analysis/multiplayer-shading/fog-clear-color/README.md).

- `red-all-maps/report.json` retains the 138 baseline RGB mismatches.
- `green-all-maps/report.json` records zero mismatches in 2,163 samples, with
  unchanged original assets and no replaced original instructions.
- `sky-density-red/report.json` exposes the 22 density mismatches after the
  RGB correction; `sky-density-green/report.json` records zero RGB or density
  mismatches after the final correction.
- `math-final-original/report.json` records 864 terminal-selection cases and 33,984
  packed-color cases, including 12 x87 controls, exact-opacity boundaries,
  interpolation, signed zero, retained colors, rounding ties, byte wrapping,
  nonfinite packing inputs and masked overflow.
- `full-atmosphere-actual-sky.json` passes the 2,163-sample fog, shader
  constants, compositor, bloom and PVS comparison using original `14B360`
  controls. Scalar/constant checks retain the declared absolute 2e-5 and
  relative 1e-6 tolerances; compositor words and PVS masks match exactly.
  The older `full-atmosphere-regression.json` is retained as an intermediate
  model-control 0 aggregation check. It predates the final density correction.
- `original-version-instruction-audit.json` records the three-image literal
  block and constant comparison.
- `sanitized-math.log` runs the new math and its test driver with address and
  undefined-behavior instrumentation. Borrowed cache libraries are not
  instrumented by that focused check; leak detection is disabled.
- `final-status.json` records the completed native GPU runs and their exact
  source/build/input identities.
- `installed-observation/report.json` reconstructs a concurrently installed
  ELF byte for byte from frozen link inputs. Its actual cache passes the same
  2,163 original RGB/density samples and all 34,848 combined math cases.
  The saved ELF passes three affected-map launcher checks; the shared runtime
  advanced again after observation, so the record identifies the saved build.

The first clean GPU sweep also exposed an existing startup failure on seven
maps. Their serialized fire-burst `pmov` entries have size 0 and point to the
same resident 20-byte header as another actual `pmov`. The concurrent shared
checkout correction resolves only exact same-address, same-group backing,
preserves the requested datum and still reads the real controller count.
That correction is incorporated into the private build. Its 21-map stock
muzzle definition test fails on the original loader and passes after the
correction; the 600-shot lifetime regression also passes. Literal alias
headers and owners are retained in `literal-movement-aliases.json`.

## Limits

The cluster-centroid and vertical-offset probes deliberately force metadata
selectors, including positions outside the specified nonconvex cluster. They
are arithmetic comparisons, not captured original gameplay.

Native screenshots and Vulkan validation establish execution and native
resolution/layout behavior. They do not establish equality with a matched
original framebuffer. Integer capture tick 6 is fixed, but fractional
presentation pose still uses the wall-clock accumulator; these images are
not deterministic golden frames or a causal RGB-only versus density comparison.
The verified private title launcher is recorded in the phase README. This pass
did not replace canonical launchers or the concurrently rebuilt shared runtime.
This change ports RGB selection and the sky-density
write; the original tail's other flags/distance updates, original 132D0 conditional clear,
transition history, sky/weather activation and complete original appearance
comparisons remain open. Existing native float clear conversion is retained.
