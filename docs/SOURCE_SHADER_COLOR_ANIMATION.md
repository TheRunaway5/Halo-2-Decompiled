# Object colors and authored animation binding

The original draw binder `16B90` resolves the extern input bank through
`1AE70`, restores/assembles the pixel program, applies authored constants
(`17170`), and evaluates bitmap transforms (`174D0`). It then binds vertex
externs (`19BE0`) and pixel externs (`19CA0`), followed by authored animated
parameters through `17420`: pixel constants `176A0`, vertex constants
`17960`, raster state `17B60`, texture state `18560`.

Pixel externs and animated parameters write the same packed constant banks
with component masks. Animation wins components both producers address;
object colors survive elsewhere. Vertex extern `1C8A / source3` writes the
primary object's RGB to attribute10 while preserving its fourth component.
An animated vertex parameter can subsequently replace some or all of RGB.
No additional vertex extern destinations are inferred from this proof.

Native `animate_forward_material` now starts with the selected authored
program/constants, applies the retained quantized object draw bank, then
applies animated parameter masks. `apply_object_colors` retains that bank
for each prepared child as well as its immediate binding. Its shared
`131ED0 → 131BB0` boundary retains raw constructor colors until drawing.
Starting each frame from `pass.program` without restoring the extern bank
loses unchanged object-color channels. Reapplying colors after animation
incorrectly replaces overlapping animated components.

The same validation exposed an animated byte-packing precision error.
Literal `131E50` performs x87 `FMUL` with255 followed directly by `FISTP`.
There is no intermediate float spill. Native packing therefore multiplies
the source float at double precision before nearest-even integer conversion.
Adding a float rounding step can change a byte at a half-integer boundary.
This arithmetic contract is independently measured under x87 control027F;
it is not a claim about every physical Xbox floating-point environment.

`tools/differential_source_shader_color_animation.py` executes unchanged
retail executable SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It executes complete raw-color packing/unpacking131ED0/131BB0, complete
19BE0 and19CA0, then complete17420 with its actual176A0/17960 callees.
Raster/texture ranges are explicitly empty authored inputs; no original
callee or result is patched. Clock values and synthetic authored identity
and constant curves are explicit inputs.

Isolated optimized and full CMake Debug binaries match **11,597 cases /
2,180,236 words bit for bit**. Coverage includes overlapping/disjoint RGB
and alpha, both combiner constant banks and final constants, fragment offsets
and permutation filtering, partial/full vertex replacement, and3,060 adjacent
byte-midpoint cases. The856 frozen cases in
`tests/fixtures/retail_source_shader_color_animation.tsv` use the actual
native `apply_object_colors → animate_forward_material` path. Every case
repeats animation after poisoning previous-frame output banks. Separate
negative controls detect missing retained externs and reversed binding order.

Reports are
`analysis/multiplayer-shading/source-shader-color-animation-original.json`
and `source-shader-color-animation-cmake-original.json`. They establish the
binding/packing seam. Object lifetime, scene clock scheduling, other extern
input producers, GPU arithmetic and final image parity require their own
independent validation.
