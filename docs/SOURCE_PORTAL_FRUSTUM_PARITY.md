# Original portal projection and visibility construction

The bounded `source_portal_frustum` and `source_portal_visibility` modules use
`assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
They implement original `173520`, `173380`, `23A2B0`, `23A430`, and the complete
pure `173130` constructor chain. The independent `source_frustum_camera` module
supplies literal `441B0` 0x1BC camera banks and `163DB0` 0x108 clipped frustums.
These CPU services are ready for an explicit source-state caller; normal
multiplayer runtime visibility is not activated by this change.

## Projected portal contract

`Cache::load_source_portal_bsp` decodes the actual collision planes, portal
endpoints/plane/flags/vertices, and ordered cluster portal lists. Source portal
records are 0x24 bytes, with endpoints at +0/+2, plane index at +4, flags at +18,
and vertex count/pointer at +1C/+20. Cluster +8C/+90 supplies its literal portal
order. The loader does not rebuild adjacency from the endpoint pairs.

`173130` initializes its own view-space clipping plane to `(0,0,-1,0)`. `173520`
transforms authored vertices using the source inverse view at camera +4, then
calls `11FC80` with positive clipping, epsilon 0.0078125, and capacity 64. This
polygon plane is separate from the camera's authored near plane.

`1429D0` pre-scales each basis column and groups its additions as
`(x*basis0 + y*basis1) + (z*basis2 + translation)`. Projected coordinates use a
stored reciprocal `-1/z` followed by two products. Classification 1 reverses
winding; classifications 2/3 preserve it. Bounding comparisons preserve the
first equal coordinate word, including signed zero. Minimum depth is the
minimum of `0-z`, not a sum along the traversal path.

Classification 3 follows the original near-plane branch. A projected eye
inside the authored polygon copies the camera's initial rectangle/polygon and
limits its metric to 0.015625. Otherwise the projected bounds expand by
0.00390625 and become four rectangle corners. A far-enabled camera rejects the
classification when minimum depth exceeds its supplied far distance.

Classification 0 before projection reserves 64 scratch entries and retains its
clipped count, but does not write a projected polygon. Its uninitialized scratch
coordinates are never presented as valid output by the native API. A portal
rejected later by the far-distance test retains its populated polygon/count.

## Convex intersection contract

`23A430` classifies positive, negative and neutral vertices with the supplied
epsilon. With no negative vertices it copies the whole polygon, even when that
count exceeds a smaller supplied output capacity. With negative vertices but no
positive vertices it returns zero, dropping neutral vertices too. Mixed cases
retain neutral/positive vertices and add only the original positive-negative
crossings. Crossings whose denominator lies inclusively within ±0.0001 are
skipped. The mixed path emits the available prefix at capacity.

`23A2B0` normalizes each ordered clipping edge with the source x87/SSE boundary.
Its X difference remains double precision on the x87 stack, is also stored to
float, then multiplies the extended difference by that stored float. The Y
difference is already stored float. The double square root is compared to
0.0001, then its stored float supplies the reciprocal. Degenerate edges copy the
current polygon unchanged. Replacing this arithmetic with a usual normalized
2D edge changes source coordinate words.

`173380` admits an intersection only when its count is at least three. Its
original fallback for a -1 result is retained in the source call inventory;
untouched `23A430` did not emit that sentinel in the bounded finite fixtures.
No new capacity-failure sentinel is invented. A nonempty projected subject
requires a nonempty clipping polygon; the actual admitted portals and initial
`441B0` four-corner polygon satisfy that caller contract.

## Complete constructor and ordering

`source_portal_visibility` reproduces `173130→172BB0→172840→172C70→1730A0→172EE0`,
including `173350`, `173B00`, `173B60`, and final `134950` sorting. Inputs are:

- Exact source 0x1BC camera banks, at most six groups.
- An explicit containing cluster, or source `NONE` (-1).
- Actual authored portals/planes/ordered cluster lists.
- The explicit 16-word runtime `4F93A4` open-portal mask.

The constructor projects and caches a portal before consulting its open bit.
Each node retains its parent, entered portal, current cluster, rectangle,
projected polygon, metric, and 512-bit visited-portal history. It checks the
source side/endpoint conditions and flag 2/16 restrictions, ancestor portal
admission for classification 3, inclusive rectangle overlap, then the original
convex intersection. Per-cluster linked nodes are inserted newest first.

`172C70` visits clusters in authored ordinal order and each linked node newest
first. It merges into the first older successor whose combined rectangle area
is strictly less than 1.5 times the sum of the two areas. The successor receives
the combined rectangle and smaller metric; the predecessor is marked merged.
Output generation preserves original node/head/linked-list ordering, source
cluster-entry creation order, each group's first/count range, and the separate
ascending cluster-index indirection.

The output cluster entries match original +A6E with stride 0x1A: source cluster,
six signed counts, and six signed first indices. Clipped frustums match original
+1974 with stride 0x108. `sphere_bank()` exposes their exact bounds/planes and
ranges to the independently verified `165010`/`181800` consumer. Runtime word
+0xC of each frustum is unwritten by these services and unconsumed by that sphere
consumer; it is excluded from the instruction comparison.

Original fixed limits are preserved: 512 authored clusters/portals, 256 nodes,
255 queued nodes, two 0x1400-point arenas, 64 temporary vertices, 128 output
clusters, and 512 output frustums. Projection requires 64 free entries before
attempting a cached portal. Traversal reserves 64 before an intersection, then
shrinks to the emitted count or rolls back rejected children. Output frustum
count increments before `163DB0` rectangle validation, as in the source. On
exhaustion the constructor retains its emitted prefix; it does not substitute a
PVS set. Native `exhausted_limits` reports these bounded diagnostic conditions.

## Independent verification

`tools/differential_source_portal_frustum.py` runs untouched `173520`, `173380`,
`23A2B0`, `23A430` and their pure callees, with no service replacements. It also
executes original startup `376F00`, which initializes the empty bounds to
alternating ±FLT_MAX; raw uninitialized XBE data is not used as runtime state.
The current differential passes **9,721 cases / 334,478 words bit exactly**:
2,048 direct plane clips, 2,048 convex intersections, 3,577 projected portals,
and 2,048 original `173380` admission calls. The projected cases include every
one of the **1,529 authored portals across all 21 multiplayer maps**. The frozen
fixture contains 2,553 independent cases.

`tools/differential_source_portal_visibility.py` executes untouched `441B0` to
create each camera input, then untouched complete `173130`, finally `165010`
against every emitted cluster range. Its current run passes **995 cases /
384,661 words bit exactly**, including all 721 authored cluster inputs across
21 maps, 256 synthetic graphs, eight arena/output saturation graphs, and ten
near-plane endpoint cases. Expected output banks and classifications come only
from the original instructions. Cache bank hashes independently retain every
plane word, portal vertex/field and ordered cluster-list element.

Both oracles use x87 control 027F and MXCSR 1F80. This is CPU source execution
under an explicit floating-point configuration, not a retail GPU framebuffer
comparison. Reports include source/native hashes and call counts in
`analysis/multiplayer-shading/portal-frustum-differential.json` and
`portal-visibility-differential.json`. Focused regular and address/undefined
sanitizer results are recorded separately.

The four focused portal/camera/clip/constructor checks also pass both regular
and address/undefined sanitizer builds with leak detection enabled. The
subsequent original `134390` BSP/instance admission and `16E210` fog membership
services are documented in [SOURCE_BSP_ADMISSION_PARITY.md](SOURCE_BSP_ADMISSION_PARITY.md).

## Required live producers

The ordinary main path `3F2C0→1320F0→132B80` supplies count 1, flags 0, eye/radius
1024, and the runtime containing cluster. Its radius/count selection chooses
constructor mode 0. Sphere-membership mode 1 or PVS-only mode 2 does not replace
this complete portal traversal.

A live caller still needs source `2F970→2FD90/2FB70` projection rectangles,
authored/current camera eye/forward/up/near/far, exact containing-cluster state,
and current door portal availability. These modules accept those inputs and do
not choose unverified defaults. Source `210DF0` initially sets all mask bits,
`210F00` assigns machine portal ranges from BSP +220 and object identity, and
`210E20` clears authored flag-8 portal bits. Later `1152E0` changes assigned door
bits using actual machine position, device-group target and runtime flags;
`115220` restores the machine's assigned bits. The complete runtime chronology
is required before activating this mask in native gameplay.

`173130` itself does not read the authored open/closed PVS tables. Its mask is
portal availability, not a cluster PVS mask. The independently implemented
`16DE20` PVS service remains a separate source path. No whole-frame parity,
portal-aware live weather admission, or dynamic-object visibility is claimed by
these constructor fixtures alone.

## Authored map coverage

| Cache map | Clusters | Portals |
| --- | ---: | ---: |
| ascension | 7 | 19 |
| beavercreek | 33 | 61 |
| burial_mounds | 27 | 38 |
| coagulation | 48 | 95 |
| colossus | 34 | 73 |
| cyclotron | 31 | 78 |
| foundation | 19 | 37 |
| headlong | 65 | 143 |
| lockout | 16 | 41 |
| midship | 11 | 21 |
| waterworks | 56 | 116 |
| zanzibar | 55 | 125 |
| containment | 49 | 96 |
| warlock | 34 | 80 |
| deltatap | 21 | 62 |
| turf | 53 | 123 |
| backwash | 43 | 66 |
| dune | 24 | 51 |
| elongation | 42 | 96 |
| gemini | 25 | 37 |
| triplicate | 28 | 71 |
