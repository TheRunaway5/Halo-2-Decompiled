# Original stipple pixel mapping

The native stipple helper preserves the observed Xbox framebuffer pattern with
an explicit raster phase. For the retained 640×480 hardware profile, the phase
at pixel `(0,0)` is `{column_origin=0, row_origin=31}`. Coordinates use integer
pixels in the target framebuffer's top-left memory order.

```cpp
column = (target_x + phase.column_origin) & 31;
bit    = (column & 24) | (7 - (column & 7));
row    = (phase.row_origin - target_y) & 31;
visible = ((uploaded_rows[row] >> bit) & 1) != 0;
```

The DWORD byte order remains unchanged; bits within each byte are read from
bit 7 toward bit 0. Rows run in the opposite direction to top-left framebuffer
Y. `source_stipple_pixel` implements this rule in
`include/halo2/source_stipple.hpp` and `src/source_stipple.cpp`, with unsigned
modulo-32 arithmetic.

## Hardware evidence

The [nxdk pgraph hardware results](https://github.com/abaire/nxdk_pgraph_tests_golden_results/tree/4acbb6ac3dcdc4d02cf4bf2d70ddf89b6e1618cb)
identify their source as an original Xbox 1.0. The pinned
[stipple test](https://github.com/abaire/nxdk_pgraph_tests/blob/7f40a15958e837e0cc4209e58e365bc8f0a4e9dc/src/tests/stipple_tests.cpp)
uploads literal 32-row patterns, including an asymmetric pattern that resolves
row direction, byte order, bit direction, and both coordinate phases.

Six images contribute 23,064 interior white-quad pixels. Across 8,192 candidate
coordinate mappings, the asymmetric result uniquely selects the rule above.
Four patterned images add 6,336 interior pixels from colored triangles
translated by `(160,48)`. Every retained color matches the disabled-stipple
control; every rejected pixel matches the background. These translated tests
confirm that the pattern follows global framebuffer coordinates rather than
restarting at a primitive origin.

The source
[SaveBackBuffer implementation](https://github.com/abaire/nxdk_pgraph_tests/blob/7f40a15958e837e0cc4209e58e365bc8f0a4e9dc/src/test_host.cpp)
copies framebuffer bytes in their existing order and changes color channels
before PNG encoding. It does not reverse image rows. The test's
[passthrough vertex shader](https://github.com/abaire/pbkitplusplus/blob/27a09692ccee84965ec4bb1438419096c9c75eb0/src/shaders/passthrough.vsh)
writes `oPos` directly. Thus the measured Y direction is framebuffer memory
order, with no screenshot inversion to undo.

`tools/audit_source_stipple_hardware.py` numerically reads the pinned results
in memory and writes 29,400 scalar observations to
`tests/fixtures/xbox_hardware_stipple.tsv`. The adjacent provenance JSON retains
source commits, source and image SHA-256 hashes, the selected mapping, and the
fixture hash. It does not save remote images for display. CTest
`xbox_hardware_stipple` checks every observation independently through the
native helper. It also checks the Vulkan rank-table lowering against every
alpha byte and all 17 original upload banks. This cross-check caught swapped
rank 6/7 entries before final integration.

## Original game upload

Retail `3F8CC0` writes pushbuffer header `0x801480`, followed by the 32 supplied
DWORDs in ascending order. Its optional device-state copy also retains those
literal words. There is no row reversal, byte reversal, or framebuffer-height
adjustment in that routine. The game selects the supplied bank through
`4D640 → 1C290`; `1C1B0` constructs the 17 masks retained by
`source_shader_instance_stipple`. The hardware mapping applies to those
literal words.

## Viewports and resolution

With a positive Vulkan viewport whose framebuffer Y follows top-left order,
the integration uses `uvec2(floor(gl_FragCoord.xy))` as the global integer target
pixel. A split viewport does not subtract its origin from those coordinates.
The full target supplies the explicit phase `{0,31}`.

When rendering the same physical pixels into a separate cropped target with
original offset `(ox,oy)`, the caller instead supplies phase
`{ox,31u-oy}` and the crop's local integer pixels. CPU controls verify this
equivalence at both even and odd offsets, including non-multiples of 32.

All retained physical hardware images have height 480. They do not distinguish
a fixed row phase `31-Y` from a possible height-derived phase `height-1-Y`.
The game masks repeat every four rows, so these alternatives also agree at
all target heights divisible by four. Preserving `{0,31}` at higher native
resolutions and odd heights is an explicit port policy. Native odd-resolution
GPU tests establish that policy consistently; they do not prove physical Xbox
behavior for odd framebuffer heights.

Nonzero NV2A surface-clip origins and per-sample MSAA stipple behavior remain
outside the retained hardware evidence. The helper takes actual raster phase
explicitly and does not invent those producers. No original guest input,
resume, or state mutation was needed for this audit.

## Retained emulator reference boundary

The retained original-game session uses Xemu 0.8.136, commit
`fc24584ce88f0915ad7f04775bb7712c2e3f49ee`, with its Vulkan renderer. This exact
version ignores the original pattern uploads: compiling its
[method table](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/methods.h.inc)
against its register definitions yields null handlers at every address from
`0x1480` through `0x14FC`. Its
[dispatcher](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/pgraph.c)
routes these null entries to an unhandled-method trace and returns without
storing the uploaded words.

The Vulkan backend obtains fragment code from the shared
[pixel-shader generator](https://github.com/xemu-project/xemu/blob/fc24584ce88f0915ad7f04775bb7712c2e3f49ee/hw/xbox/nv2a/pgraph/glsl/psh.c),
whose retained state and code have no stipple mask. The exact-version audit
found no stipple references in its 70 PGRAPH source files or its register
definitions. `tools/audit_xemu_stipple.py` retains the executable/config hashes,
git-verified source hashes, and all 32 compiled dispatch results in
`analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/report.json`.

An offline inspection of `original_capture_20.rdc` additionally retained all
96 shader resources and all 46 fragment modules, with no reflection errors.
Their captured uniform banks contain clip regions, colors, depth parameters,
and texture state, but no stipple rows. No fragment module uses a bitwise AND
or right shift. The only two left shifts and two bitwise ORs belong to texture
color packing. This corroborates the missing source path; the absent method
handlers and generator state establish the omission. Raw captured SPIR-V,
disassembly, reflection, and capture hash are retained under
`analysis/multiplayer-shading/stipple-coordinate-audit/capture20-shaders`.

An original-game image from this emulator is therefore insufficient to reject
the hardware-proven mask. Differences caused by enabling the original mask
remain a documented emulator-reference boundary; the native implementation
must preserve the hardware rule.
