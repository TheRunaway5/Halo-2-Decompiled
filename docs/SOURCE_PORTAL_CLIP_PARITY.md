# Original portal polygon math

`source_portal_clip.hpp` / `source_portal_clip.cpp` implement the bounded pure
polygon services used by the original portal visibility constructor. Authority
is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The implementation does not substitute generic culling or construct a complete
`173130` visibility set.

## Clipping inputs and order

`source_clip_portal_plane` reproduces `11FC80` for finite input polygons and the
64-vertex scratch capacity of its `173520` caller. That caller first transforms
the authored portal vertices through its source frustum matrix using `1429D0`,
then clips with keep-positive true and epsilon `3C000000` (0.0078125). The helper
also supports the original keep-negative branch and a supplied epsilon/capacity
within the bounded contract.

The plane metric is `normal dot position - plane.w - signed_epsilon`. Vertices
on the plane are positive, including equality. The previous vertex starts as
the polygon's final vertex. Each crossing emits
`previous + (current - previous) * (previous_distance / (previous_distance - current_distance))`
before the retained current vertex. Arithmetic is separately rounded; no FMA,
normalization or vertex reordering is introduced.

The source has different SSE dot-product addition orders. The initial final
vertex uses Z+Y, then X. The first vertex of each four-vertex block uses X+Z,
then Y; the other three use X+Y, then Z. The scalar tail uses X+Z, then Y. A
single uniform dot helper changes the original crossing-coordinate words.

If another vertex would exceed the supplied output capacity, `11FC80` returns
the count and emitted prefix immediately. It does not return `NONE` or replace
the clipped polygon with its input. The caller's later polygon intersection
service has a separate fallback contract; this helper does not conflate them.

## Portal-facing classification

`source_portal_facing` reproduces `173890`, including its actual pure callees
`173910`, `120850` and `23A220`. Inputs are the original plane, source eye,
authored world-space portal polygon, and count of the separately clipped
view-space polygon. Those two polygons have distinct roles.

`173890` computes signed distance in x87 with Z+Y, then X, then subtracts the
plane distance. Its inclusive near-plane threshold is the **double** at
`45E060`, `3F90000000000000` (0.015625). It retains that unrounded distance for
the near-plane comparison, but uses the stored float for an outside-near-plane
side decision. `173910` separately recomputes an SSE distance for projection.

The returned classification and output byte follow these literal rules:

| Source condition | Classification | Projected-eye-inside byte |
| --- | --- | --- |
| Near plane and projected eye inside authored polygon | 3 | 1 |
| Near plane, projected eye outside, clipped count greater than zero | 3 | 0 |
| Near plane, projected eye outside, clipped count zero | 0 | 0 |
| Outside near-plane interval, clipped count zero | 0 | 0 |
| Outside near-plane interval, clipped count 1 or 2 | 3 | 0 |
| Outside near-plane interval, clipped count at least 3, stored distance negative | 1 | 0 |
| Outside near-plane interval, clipped count at least 3, stored distance nonnegative | 2 | 0 |

The projected-eye test does not normalize the plane. It computes
`eye + normal * (0 - signed_distance)` with the source SSE order, selects the
normal's dominant axis (ties prefer Z, then Y), and selects the two projected
axes from the actual sign-dependent `440B94` table. `23A220` tests every ordered
edge with inclusive cross-product tolerance `38D1B717` (0.0001). An empty polygon
retains the original inside result.

## Independent verification

`tools/differential_source_portal_clip.py` runs the untouched source routines and
all their pure callees directly. There are no service replacements. The oracle
supplies literal plane/eye/polygon memory and records every returned coordinate
word, classification and output byte. Its x87 control is `027F` and MXCSR is
`1F80`; this is a CPU instruction differential, not GPU rasterization evidence.

The current differential passes **6,334 cases / 139,011 words bit exactly**:
2,168 plane clips, 2,108 facing classifications and 2,058 projected-eye tests.
Coverage includes every input count/capacity from 0 through 64, both clipping
sides, empty/degenerate polygons, output saturation, inclusive plane vertices,
mixed SSE addition orders, signed normal orientations and dominant-axis ties,
extended-distance cancellation, adjacent floats around the near-plane threshold,
and adjacent floats around projected-edge tolerance.

The retained `retail_source_portal_clip` fixture contains 830 independent source
cases: 376 clips, 316 classifications and 138 projected-eye tests. All expected
words come from source execution, never from the native helpers. The focused
CTest and standalone address/undefined-sanitizer fixture run pass. The report
and binary hash are in `analysis/multiplayer-shading/portal-clip-differential.json`;
logs are `portal-clip-focused-tests.log` and `portal-clip-asan.log`.

## Visibility constructor boundary

These helpers do not provide the source frustum matrix, authored portal list,
runtime portal availability, visible portal mask, clipping rectangles, polygon
queue, merged cluster/frustum chains or overflow handling of `173130` and
`172840`. Those services and the complete bounded `173130` constructor are now
implemented separately in `source_portal_frustum` / `source_portal_visibility`;
see [SOURCE_PORTAL_FRUSTUM_PARITY.md](SOURCE_PORTAL_FRUSTUM_PARITY.md). The ordinary source camera uses constructor mode 0; a PVS
upper bound or sphere-membership mode is not a replacement for that path.
No full visibility or original/native frame parity is claimed by these pure
math fixtures.
