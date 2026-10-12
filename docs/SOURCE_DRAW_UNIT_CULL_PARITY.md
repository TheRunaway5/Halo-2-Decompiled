# Source draw-unit sphere culling

The bounded service reproduces the complete `44720` sphere branch and the
linear and accelerated `45560` branches from explicit source draw units,
resident geometry, bounds, masks and posed node banks. Gameplay light/shadow
producers and matched original frames remain separate gates.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
[The unchanged-instruction tool](../tools/differential_source_draw_unit_cull.py)
executes the real `44720→45560→B8C00/142A60/142700` instructions. It verifies
executed bytes against that executable; it does not patch code, inject return
values, replace lookups or access the guest.

## Ordered dispatch

`44720` reads current context `4C0B80`, main accepted prefix `4C0B7C`, total
`4C0B78`, context mode+`2A78` and query mode+`2A7C`. A zero context mode visits
`[0, main_prefix)`. Every nonzero mode visits `[main_prefix, draw_count)`.
The endpoints truncate to unsigned16; the actual unit index is signed16.
The native bounded interface rejects indexes outside supplied valid storage.

Each unit is the literal 32-byte record at `4BA138 + index*20` (hexadecimal
stride). Only units with flag1 clear and flag40 set reach a culling service.
Sphere query mode1 takes center/radius from context+`2A68`/+`2A74`. A nonzero
unit+`1C` transforms only the query point into the supplied local frame: zero
scale gives zero; other scales preserve the original reciprocal and ordered
basis products. **The query radius is unchanged**, including for scaled
instances.

The public seam is
[`source_draw_unit_sphere_cull`](../include/halo2/source_draw_unit_cull.hpp).
It mutates only flag1 and the supplied backing masks, returning actual ordered
sphere-call indexes and local query points for inspection. The other seven
unit words, including association, section and pointer identities, are
retained. Geometry-null is explicit through `geometry_resolved`; a missing
lookup cannot silently become null geometry.

## Literal geometry and subpart admission

The caller resolves the same current source geometry used by `45560`:

| Draw-unit owner | Required geometry lookup |
|---|---|
| unit+4 is a render-model datum | current tag→mode+28 section array; packed unit+C bits9..17 select a stride5C section; section+34 supplies its resident geometry |
| unit+4 is NONE, flag1000 clear | active BSP+A0 cluster array; the same packed section index selects a strideB0 cluster; cluster+50 supplies resident geometry |
| unit+4 is NONE, flag1000 set | packed unit+C bits18..28 select active BSP+144 instance stride58; instance+34 selects BSP+13C definition strideC8; definition+50 supplies resident geometry |

These are **runtime resident geometry pointers**, not guaranteed authored
cache pointers. Existing native expanded triangles and material IDs do not
establish those resident banks.

A null geometry does nothing. A positive geometry+30 consumes the actual
resident `SourceBspRangeGeometry` bytecode/header/mapping bank through
`source_bsp_accelerator_sphere`, then the original `458D0` list expansion.
Missing banks are rejected. Zero or negative+30 executes the complete
linear path. Its signed subpart count is geometry+8; negative/zero counts
skip and retain inactive storage. Geometry+C is the **8-byte subpart array**,
whose+4 unsigned short selects a bound. FFFF admits that subpart without
reading bounds, owner or pose metadata. Geometry+14 bounds have a 20-byte
stride: center, radius, node byte+10. These mask bits identify subparts, not
material parts; original `4DAA0` later consumes the mask through each part's
subpart range.

Already marked subparts are skipped. For a model bound, unit+10 points to the
actual context owner's identity. A non-NONE owner permits `B8C00` to obtain its
current node matrices. The source composes the selected current node with
mode+4C node-stride60 inverse-bind transform+28 through `142A60`, then transforms
the original query point through `142700`. It caches the last node byte and
local point across the ordered subpart loop. Empty current-node banks leave
both unchanged. The source MOVZX byte followed by comparison with FFFFFFFF
never recognizes byteFF as a sentinel; a consumed FF requires actual node255.
No node0 substitute is used.

Admission is inclusive:
`(bound_radius + query_radius)^2 >= (dz^2 + dy^2) + dx^2`, with the original
SSE addition order. Admission sets unit flag1 and the subpart mask bit. Masks
are separate banks keyed by the literal unit+14 backing identity. Shared
masks retain ordered writes: a later unit skips previously marked subparts
without gaining flag1 merely from those existing bits.

## Retained post-state boundary

The immutable `32893-after-part-admission` dump was taken **after** `165A20`,
`2CBC0` and `44720`, at source tick12475, render frame111186, view0. Its
secondary context has mode2, query mode1, main prefix57 and total63. Units
57..62 have flag1; units58..62 also lack flag40.

Re-executing `44720` on this literal AFTER-state preserves all **504 unit
words** and makes zero additional part-culling calls. No before-44720 unit
array was exported. In particular, unit57 could have gained flag1 inside
`45560` during the original invocation. This control does **not** establish
the original call count, its admitted subparts, its bound/accelerator bank,
or earlier collector allocation success. The report keeps the snapshot
filenames, pointer bases, sizes and hashes. This OpenGL/llvmpipe live phase
also remains distinct from the original20 Vulkan framebuffer.

All other geometry/pose cases are explicitly controlled valid source inputs;
they are not labeled as live authored admission.

## Validation and remaining services

The independent fixture contains **1,258 cases / 41,015 exact output words**,
including 957 original `45560` calls, 7,315 `B8C00` calls and 7,306 complete
matrix composition/inverse-point calls. Coverage includes main/secondary
prefixes, truncated endpoints, unit skips, null geometry, signed subpart
counts, inactive mask words, FFFF bounds, adjacent inclusive sphere
boundaries, zero/negative scales, unchanged radius, repeated/alternating
nodes, actual node255 storage, shared mask order and finite-overflow derived
IEEE/unordered rejection. Ten unresolved-input
controls and an unread admitted-unit control also pass.

Normal and ASAN/UBSAN tests with leak detection enabled pass using the
production helper and transform implementations in a private CPU build.
CMake target `source_draw_unit_cull_tests` / test `retail_source_draw_unit_cull`
links `halo2_cache`. No shared build, GPU, renderer, runtime, existing shadow
module or guest state was changed by that historical linear-path audit.

The separate sphere accelerator now matches **7,296 whole original
`2DDBD0→2DCE60` calls / 27,713 ordered output words**, using456 retained
header/bytecode banks and explicit controlled query spheres. Both source
lists retain their512-word capacities. Classification preserves SSE squared
sum order, the x87 PC53 radius square and strict box containment. The same
traversal module continues to pass the existing1,398 query0 cases.

The full culler additionally matches **2,282 whole original `44720` calls /
66,404 words**, including779 positive `45560→2DDBD0→2DCE60→458D0` calls.
Controls cover translated headers, scales, low/high subpart bits, mapped
contained ranges, direct keys, model/BSP/instance geometry resolution and
shared masks. These are controlled resident banks; they do not establish live
grenade collector chronology or gameplay appearance. The two new CTest gates
are `original_effect_light_accelerator` and
`original_effect_light_draw_unit_cull`; fixtures retain independent source
outputs and provenance. Reports are under
`analysis/multiplayer-shading/world-effects-goal-20261011/`
(`light-accelerator-oracle-v2` and `light-draw-unit-oracle-v2`). The historical
sanitizer result above does not cover these accelerated additions.

Before gameplay integration, callers still need actual pre-cull draw-unit
allocation/resource results and shared prefix history, current resident
geometry/subpart/bounds/accelerator banks, compact current pose/context
owners, and original mask backing allocation/aliasing. Query modes other
than1 require the separate complete `44E90` admission and actual association
inputs. The helpers do not supply those caller results or infer them
from model size, rendered triangles, source metadata order or a containing
cluster.
