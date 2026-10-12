# Original camera and portal frustum construction

`source_frustum_camera.hpp/.cpp` reproduces the complete `441B0` camera bank
and `163DB0` rectangular frustum constructor. The types retain the original
byte offsets and sizes, so the portal producer can consume their actual fields.
They do not substitute a host projection matrix or a generic camera frustum.

## Inputs and layout

`441B0` has three distinct inputs in the original calling convention:

* EBX points to the camera position, followed by forward at `+0xC`, up at
  `+0x18`, near distance at `+0x40`, and far distance at `+0x44`.
* The stack argument points to `2FD90`'s separate view/projection output. Its
  `+0x68..+0x74` fields supply `xmin,xmax,ymin,ymax`.
* ESI points to the zeroed `0x1BC` output bank.

The camera record's special `+0x58/+0x68` feature bytes and `2F970`'s input
subrectangle are separate fields. The helper does not infer or activate them.
`source_frustum_projection` now generates the actual separate rectangle through
the source `2F970 → 2FB70` arithmetic. Its complete projection bank and modifier
are retained alongside the normalized input bounds.

| Camera bank offset | Field |
| --- | --- |
| `+0x00` | Orthographic byte, cleared |
| `+0x04` | Inverse view transform, 13 floats |
| `+0x38` | Camera-to-world transform, 13 floats |
| `+0x6C/+0x70` | Near-plane enabled byte / near distance |
| `+0x74` | Near-plane normal and D |
| `+0x84/+0x88` | Far-plane enabled byte / far distance |
| `+0x8C` | Full-frustum enabled byte |
| `+0x90` | Embedded `0x108` frustum |
| `+0x198/+0x19C` | Initial polygon count / four XY corners |

| Embedded frustum offset | Field |
| --- | --- |
| `+0x00` | Source group |
| `+0x04..+0x0C` | Retained runtime words |
| `+0x10` | Rectangle, four floats |
| `+0x20` | Retained metric |
| `+0x24` | Four world-space far corners |
| `+0x54` | Eye position |
| `+0x60` | Six bounds: minX,maxX,minY,maxY,minZ,maxZ |
| `+0x78` | Six `(normal.xyz,D)` planes |
| `+0xD8` | Four corner-minus-eye rays |

Compile-time layout assertions cover both complete types and every consumed
offset. The standalone rectangle constructor preserves all retained fields.
An empty or reversed rectangle returns false without changing any output byte.
The full constructor still retains the source enabled byte and initial polygon
when its embedded rectangle is empty.

## Original arithmetic

`441B0` normalizes the authored up and negative-forward vectors independently,
then normalizes their cross product for the right vector. It does not rebuild
up from the other two normalized axes. `141590` computes the inverse through
the existing independently tested source transform helper. The near plane uses
the authored forward vector and ordered `(fz*pz+fy*py)+fx*px+near`; it does not
replace that vector with the normalized backward axis.

`30BF0` evaluates `(x*x+y*y)+z*z` and the square root in x87 precision, spills
the length to float, then computes the SSE reciprocal and component products.
The represented control configuration is FPCW `027F` (53-bit significand) and
MXCSR `1F80`. The source threshold is literal `38D1B717`. This configuration is
explicit in the original differential; no live retail FPU-control capture is
claimed by the camera test.

`163DB0` forms the original four far corners and transforms them through
`1429D0`, pre-scaling the basis and retaining `(X+Y)+(Z+translation)` ordering.
`376F00` initializes the original empty-bounds bank with alternating maximum
positive and negative finite floats. `11F770` receives all four corners followed
by the eye, so the eye participates in the resulting bounds.

The side planes retain `163DB0`'s normalized local vectors and its distinct
normal and D operation orders. Planes 0/1/4/5 use
`((ty*ny+localD*scale)+tz*nz)+tx*nx`; planes 2/3 use
`((tz*nz+ty*ny)+localD*scale)+tx*nx`. An enabled source near plane is copied
with sign reversal. With that byte clear, the original local `(0,0,1,0)` plane
is transformed instead. The optional diagnostic reproduces the final plane
consistency condition that clears source global `47FF86`; it does not change
the frustum bank or successful return.

## Native rectangle producer

`SourceFrustumProjectionInput` contains the post-`2F800` vertical angle and the
literal outer/safe signed-short camera rectangles, ordered top,left,bottom,right.
The existing source observer/grid helper provides that angle; normalized
`2F970` bounds must not be passed directly to the camera bank constructor.

The input also preserves each branch's real caller state:

* Camera `+0x58` enabled byte, `+0x5C/+0x60` center and `+0x64` scale.
* Camera `+0x68` enabled byte and `+0x6C/+0x70` pixel offsets. The `+0x58`
  branch takes precedence when both are active.
* Widescreen global `485AC2`, and the `4E6948` layout-state pointer/byte1/
  signed-short2/integer8 fields. A present, enabled, non-NONE mode3 layout
  suppresses the original widescreen branch.
* `510C50` auxiliary-state pointer/byte5. With admitted widescreen, it scales
  the tangent by literal `4/3` then `9/16`. In the other branch it changes the
  stored Y input bounds, retaining the previously computed centers/half extents.
* The scenario pointer/type for the `4BA019` side effect. The null/non-null
  `2FB70` bounds-pointer branch is also explicit.

`source_frustum_projection` returns all four original `2F970` bounds, all 16
original `2FB70` floats, and the modifier flag. `clipping_rectangle()` returns
the final floats12..15 that `2FD90` forwards to its output `+0x68..+0x74`.
Pass that rectangle, actual camera position/forward/up and source near/far
distances to `source_frustum_camera`. This joins the native producer without a
manually supplied fixture rectangle. The helper never derives source feature
state or the observer/grid aspect from the physical host window.

## Validation and integration boundary

`tools/differential_source_frustum_camera.py` pins the original executable SHA
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
It executes all of `441B0`, `30BF0`, `141590`, `163DB0`, `1429D0`, `11F770`,
`2F970`, `2FB70`, `2FD90` and the original `376F00` initializer without replacing
any callee. It compares the complete camera/frustum byte banks, the complete
projection banks/modifier, and `163DB0`'s result and diagnostic. Full caller
cases additionally assert that `2FD90` forwards the generated rectangle exactly.

The retained Zanzibar and Ivory camera snapshots use their correct position
offset `+0xC`. Untouched `2F970 → 2FD90 → 2FB70` produces their separate
rectangle under the explicit ordinary standard-view inputs. Zanzibar's camera
fields and original projection were independently corroborated against actual
capture20 GPU c0..11 in `prove-zanzibar20-camera.py`; the snapshot does not
establish temporal equality for every other world state.

The completed differential matches **12,674 cases / 1,087,742 words bit-for-bit**:
4,188 full constructors, 4,096 clipped rectangles, and 4,390 projection/full
caller cases. Cases include captured camera inputs, adjacent normalization and
tangent thresholds, cancellation, signed zero, empty rectangles, both near-plane
branches, retained arbitrary runtime bytes, non-unit transform scales, camera
feature precedence, widescreen suppression, both auxiliary branches, and null
projection bounds. The checked-in fixture freezes 644 cases for
`retail_source_frustum_camera`; the same cases pass address/undefined sanitizers.

This establishes the bounded camera-to-frustum producer. Portal/PVS recursion,
the actual live context/feature inputs, section/subpart admission, source
sorting, and complete framebuffer parity require their own source producers
and validation.
