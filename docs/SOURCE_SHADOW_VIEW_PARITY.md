# Fixed shadow view volume and secondary camera

`source_shadow_view` ports the complete original `320B0` call and the
nonzero-mode, single-camera branch of `C1D90`. It consumes explicit original
banks and does not activate multiplayer shadows. The unchanged executable
differential checks **4,710 cases / 443,456 output words**, with no mismatches.
The independently retained secondary camera matches all **111 words**.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The new files are [source_shadow_view.hpp](../include/halo2/source_shadow_view.hpp),
[source_shadow_view.cpp](../src/source_shadow_view.cpp),
[source_shadow_view_tests.cpp](../tests/source_shadow_view_tests.cpp), and
[differential_source_shadow_view.py](../tools/differential_source_shadow_view.py).
The frozen inputs and independently produced outputs are
`tests/fixtures/retail_source_shadow_view.tsv`; its companion provenance and
`analysis/multiplayer-shading/shadow-view-original.json` retain source hashes,
executed addresses, captured-input hashes, floating controls and boundaries.

## Actual inputs and arithmetic

`source_shadow_view_volume(main_camera, receiver_box)` takes the first actual
`SourceFrustumCamera` bank in the original main context and the literal
`320B0` ESI input: a 13-float transform followed by three half extents.
That box is the completed `32350` receiver volume. An inferred model sphere
or an axis-aligned replacement is not the same input.

`11F9A0/11FA40` enumerate the eight box corners and twelve edges in their
literal order. `320B0` transforms the 24 endpoints with `(X+Z)+Y`, then
translation. It invokes unchanged `1652E0` with near clipping enabled and
far clipping disabled, independently of the camera's near/far enable bytes.
The near offset adds the caller's `near_distance` to the retained frustum
metric and clamps it against literal `0x37800006`. The front test uses
`0xB8D1B717`; the four-endpoint unroll has distinct dot orders. Mixed edges
intersect the actual plane, preserving the source zero-denominator branch.
The inverse-view projection clamps Z against `0xB7800006` and retains its
ordered products and bounds comparisons. The result tests the projected
rectangle against the actual main clipping rectangle. Empty bounds retain
the six original alternating positive/negative `FLT_MAX` words initialized
by unchanged `376F00`. No generic box/frustum test is substituted.

`source_shadow_secondary_camera(completed_projection, direction, up)` takes
the full 31-word `C1A80` projection bank and actual light fields at `+28/+34`.
The single-camera branch reads mode, left basis `7..9`, virtual near `10`,
eye `11..13`, projection ray `15..17`, width/height `22/23`, and far offset
`25`. Direction and up remain separate from the projection ray and left
basis. `C1D90` copies the bases without normalization or a new cross product;
it computes the near plane, far distance, four projected polygon points,
source `141590` inverse and complete `163DB0` embedded frustum. The helper
returns the entire `0x1BC` camera and its original plane diagnostic.

The fixed `47FDA0` shadow caller uses mode2. Controlled mode1 sets the
orthographic byte. `C1A80` mode1 supplies near zero, and `C1D90` performs
IEEE division without a zero-near guard. Tests preserve that behavior,
including signed NaN/infinity words and the empty-frustum path. Finite zero
projection rays and negative/empty dimensions are also checked. The separate
mode0 six-camera point-light branch is **explicitly rejected**. This module
does not claim that mode1 or the controlled degenerate inputs are admitted by
ordinary gameplay.

## Independent captured control

The immutable input is the original OpenGL/llvmpipe shadow phase
`32875-secondary-admission`, tick12475, view0, in
`analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl`.
The stack export is hashed and addressed by its manifest; actual ESP is
`D00B29D0`. The receiver box is at ESP+`2C`, completed `C1A80` bank at
ESP+`6C`, and completed secondary camera at ESP+`E8`. Direction/up come from
the actual `47FD18` bank. The main camera comes from the `7FD80000` bank's
first camera at `+4`, rather than the global pointer bank at `547F88`.

Unchanged `C1D90→141590→163DB0` over those literal inputs reproduces the
observed output count1 and all 111 completed camera words. The retained main
camera and receiver box also execute through unchanged `320B0→1652E0` and
the native service. That phase follows a successful original volume check,
but it does not independently retain that prior call's temporary bounds or
every endpoint. Those expected words come from the untouched execution.
This phase is distinct from the original20 Vulkan framebuffer; no same-frame
relationship to that image is asserted.

## Validation and remaining callers

The oracle executes all original arithmetic and callees. Its hooks only
observe endpoint/bounds memory, count entries, verify executed instruction
bytes against the pinned executable, and stop after the original return.
There are no output stubs, replacement clipping/projection functions or
patched source branches. Both captured and oracle floating configurations
use x87 PC53 and round-to-nearest; their reserved/exception status bits are
documented separately.

The corpus has 2,611 volume cases (529 admitted / 2,082 rejected) and 2,099
secondary-camera cases. It covers source-constructed main cameras, rotated
and scaled boxes, degenerate extents, adjacent near-epsilon/screen-boundary
floats, far-depth controls, translated cameras and varied light/projection
inputs. Regular and ASAN/UBSAN builds of the production helper, frustum and
inverse services pass all 443,456 exact words with leak detection enabled.
CTest is registered as `retail_source_shadow_view`; it is a CPU check.

Native validation rejects unresolved nonfinite consumed input fields,
negative half extents and unsupported projection modes. These are bounded
input-service checks, not claims that the original executable had those
guards. The helper adds no guessed clock, cache, owner or default light state.

The remaining activation route must still supply the actual receiver box,
completed projection/light bank, secondary context and containing cluster,
`1320F0` admission, `165A20` classification, ordered `44720` draw units and
resource-admitted geometry/material bindings. Original owner pools, local
priority, cache validity and lifetime/history inputs remain separate services.
The normal multiplayer `ModelView::source_shadows` path remains unactivated;
see [SOURCE_NATIVE_SHADOW_ACTIVATION.md](SOURCE_NATIVE_SHADOW_ACTIVATION.md).
