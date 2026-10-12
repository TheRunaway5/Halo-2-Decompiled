# Title and frontend aspect policy

The recovered original main-menu path renders a 640×480 raster with the same
projection when the widescreen input flag is set. Extending scenery across a
16:9, 16:10 or ultrawide host window while preserving a centered, undistorted
menu is a native presentation extension. It must remain separate from claims
about original widescreen video output.

The aspect-policy research uses the retained executable, decompiled routines,
authored cache and captures. The title camera-bank integration described below
has since been implemented and captured; extending the world rectangle remains
separate work.

## Original evidence

- Executable: `assets/disc/default.xbe`, SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
- Cache: `assets/disc/maps/mainmenu.map`, SHA-256
  `1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b`.
- Retained Xemu Vulkan capture:
  `analysis/frontend/xemu-aspect/original-aspect_capture_4.rdc`, SHA-256
  `0c444cee2dafa83c338f03a508bfdd1c6ff95d4128ac7cd0b116319d9226c6e3`.
  Its setup and limits are pinned in
  [mainmenu-widescreen-suppression.json](../analysis/frontend/xemu-aspect/mainmenu-widescreen-suppression.json).

Original `123B0` reads `XGetVideoFlags`; bit 0 becomes `485AC2`. `12490`
creates a 640×480 backbuffer and independently sets
`D3DPRESENTFLAG_WIDESCREEN` when that byte is nonzero. The raster dimensions
and the display flag are separate inputs. These instructions do not establish
the physical shape of displayed pixels or the emulator's presentation policy.
See [unknown_01cf50.cpp](../tools/reference/halo2-decompiled/src/unknown_01cf50.cpp),
entries `123B0` and `12490`.

Original `16A90` returns false when the options pointer exists, its flag at
`+1` is set, its index at `+2` is not `-1`, and its state at `+8` is `3`.
`2FB70` repeats that condition before its widescreen lens branch. The retained
live main-menu state is `(flag,index,state)=(1,0,3)`. With `485AC2=1`, captured
geometry event 37 still has X/Y projection scales
`1.597940601548901 / 2.1305876148818705`.
See [unknown_0167a0.cpp](../tools/reference/halo2-decompiled/src/unknown_0167a0.cpp),
entry `16A90`, and
[unknown_02b400.cpp](../tools/reference/halo2-decompiled/src/unknown_02b400.cpp),
entry `2FB70`.

That retained experiment changes the display input through GDB; it retains
the original executable code and main-menu suppression condition. It proves
the observed lens branch. It does **not** prove physical 16:9 video output,
native matching pixels, or widescreen behavior throughout every submenu.

## Projection and authored UI

The original single-view projection chain is
`171D90 → 2F800 → 2F970 → 2FB70 → 2FD90`. `2F800` applies the original
single-view factor `0.785` to the observer angle. Standard-video startup
`12560 → 16A30` produces outer `(top,left,bottom,right)=(0,0,480,640)` and
safe `(36,48,444,592)`. `2F6B0` retains those rectangles for the single view.
The resulting clip scales are `544/640` and `408/480`, both `0.85`.

[audit_menu_projection.py](../tools/audit_menu_projection.py) executes the
unchanged instructions and retains SD/HD, widescreen, cinematic and
main-menu-suppression fixtures. The measured capture scale error is below
`1.32e-7`; the production lens helper agrees with the display fixtures within
`2.4e-7`. The independent numeric oracle does not prove physical widescreen
presentation. See
[title-original-projection.json](../analysis/frontend/xemu-capture/title-original-projection.json),
[menu_projection.cpp](../src/menu_projection.cpp) and
[menu_projection.hpp](../include/halo2/menu_projection.hpp).

Outside the suppressed main-menu branch, `2FB70` multiplies the raster aspect
by `4/3` for widescreen. When its auxiliary cinematic flag is also set, it
multiplies the tangent by `(4/3)*0.5625`. Enabling that branch in a native
window whose physical aspect is already 16:9 would apply an additional
correction. It is not the verified main-menu solution.

Original `22F5CA` initializes screen bounds from the safe rectangle.
Original `2360C3` projects authored widget coordinates about the supplied
rectangle's center and applies the window manager's depth scaling. It does
not derive coordinates from a host-window aspect. The cache contains signed
authored bitmap points, text bounds, list positions and model-scene viewport
rectangles; those are read directly by the native content loader.
See [unknown_22e27b.cpp](../tools/reference/halo2-decompiled/src/unknown_22e27b.cpp),
entry `22F5CA`,
[unknown_236299.cpp](../tools/reference/halo2-decompiled/src/unknown_236299.cpp),
entry `2360C3`, and [frontend_content.cpp](../src/frontend_content.cpp).

Native [frontend_painter.cpp](../src/frontend_painter.cpp) maps the authored
centered coordinates to a 640×480 UI bank, using `320+x` and `240-y` before
host presentation. Retained profile and keyboard captures support that bank
for those measured states. Keeping all native frontend UI in that centered
bank is the requested extension policy; this report does not establish every
screen's original physical widescreen layout, depth animation or model pose.

## Current native boundary

At this revision [renderer.cpp](../src/renderer.cpp) first creates a full
surface world viewport, then narrows it to a centered 640×480-scaled rectangle
whenever the UI vertex bank is nonempty. World projection, light volumes,
haze, bloom and the title filter inherit that narrowed rectangle. A later,
independent `ui_view` repeats the centered rectangle for authored UI. Thus
current wide-window captures preserve the original 4:3 scene and produce
black side borders; they do not extend the world.

The title already stores the original raw render angle in
`Camera::projection_vertical_angle`. `camera_vertical_field_of_view` returns
that value unchanged, so a full-width world viewport can retain its vertical
lens while exposing more scenery horizontally. The recovered clip scale
remains independent of that host aspect. See
[scene.cpp](../src/scene.cpp) and [menu_projection.cpp](../src/menu_projection.cpp).

`source_view_projection` supplies the exact `2FD90` render bank and original
`12D50/12FA0` shader constants from explicit inputs. It is already wired into
ordinary multiplayer `ModelView` banks and now into the title's primary draw
through `menu_render_view_bank`. The original canonical raster and menu lens
suppression remain unchanged. This removes the title shader's GLM camera-bank
reconstruction while preserving the subsequent embedded-model camera bank.
The pure helper matches 4,608 original output words and the 48-word original
GPU bank; actual submitted-state checks are retained under
`analysis/frontend/title-camera-matrix-audit/`. This is separate from choosing
a full-width host presentation rectangle or proving complete scene pixels.
See [source-view-projection.md](source-view-projection.md),
[source_view_projection.hpp](../include/halo2/source_view_projection.hpp),
[source_frustum_camera.hpp](../include/halo2/source_frustum_camera.hpp), and
[multiplayer_runtime.cpp](../src/multiplayer_runtime.cpp).

## Minimal integration seam

Use two explicit rectangles for the frontend: a full surface **world**
viewport/scissor and a centered **UI** viewport/scissor. Keep gameplay and
authored embedded model-scene rectangles outside this scoped change.

For a wide host surface `W×H`, let `A=W/H`, `A0=4/3`, and preserve the source
vertical lens. The world normalized scale is:

```text
worldScaleX = referenceScaleX * A0 / A
worldScaleY = referenceScaleY
uiScale = min(W/640, H/480)
uiRect = ((W-640*uiScale)/2, (H-480*uiScale)/2,
          640*uiScale, 480*uiScale)
```

| Host aspect | X scale / source X scale | Centered UI width / height |
| --- | ---: | ---: |
| 4:3 | 1 | 4:3 |
| 16:9 | 3/4 | 4:3 |
| 16:10 | 5/6 | 4:3 |
| 21:9 | 4/7 | 4:3 |

These ratios are consequences of the selected native extension policy, not
recovered retail aspect modes. At equal surface height, the central 4:3
world projection remains geometrically identical: reducing normalized X
scale cancels the larger world viewport width. That algebra does not claim
pixel identity across different sampling grids or noise histories.

All dependent paths must receive the same extended world lens and rectangle:

1. Primary geometry, sky, depth, source camera shader constants, visibility
   and any culling/LOD metrics must agree about the world aspect. Preserve
   the original angle, camera path, near/far and 0.85 clip scale.
2. `prepare_light_volumes` currently asks `menu_projection_lens` for a fixed
   4:3 X scale. Pass the aspect-adjusted scale into both its projection and
   viewport-half metrics. Removing the world scissor alone leaves the beam
   and its sprites using a different lens from opaque geometry.
3. Haze direct quads and camera-motion coefficients already consume a
   viewport width/height; supply the world rectangle. Original `1D770`
   provides fixed 320×240 haze and 160×120 bloom targets. Keeping those fixed
   while stretching the world changes their physical horizontal footprint;
   changing their width is a native extension choice. Recover or specify
   that choice explicitly and retain a 640×480 reference branch.
4. Bloom, title color filtering and any source-image rectangle must process
   the full world rectangle. Avoid sampling former side borders or cropping
   the extended scenery back to the centered UI rectangle.
5. Popup tint and blur process the completed lower channels. Tint the full
   lower world surface, then draw the foreground dialog in `uiRect`.
   Current tint uses `ui_view/ui_clip`; that would leave wider scenery
   untinted after a world extension. Popup blur's `(radius,640,480)` controls
   currently scale offsets with its source rectangle. For a full-width
   extension, retain the same physical kernel scale in X and Y: a logical
   world extent `(480*A,480)` with uniform `H/480` scaling preserves that
   policy. The original 640×480 kernel remains separately proved; its
   extension to other logical widths is not a new retail behavior claim.
6. Keep authored UI triangles, their hit-test transform and embedded
   `UiScene` viewports on the same centered 640×480 presentation transform.
   Resize must update world/effect rectangles and the UI transform together.

The effect target-width choices are an explicit remaining design boundary.
A full-width viewport patch without the light-volume and postprocess changes
does not satisfy the requested visual parity or aspect support.

## Verification boundary

Retain numeric fixtures for the unchanged 640×480 source lens and matrix
bank. Test world-center projection invariance, shared beam/geometry lens,
centered UI placement and pointer mapping at 640×480, 1280×720, 1280×800,
1680×720 and a resize. Capture the production title/profile/keyboard flow
with Vulkan validation, checking that the new world fills side regions and
that modal tint/blur also covers them.

Existing [verify_frontend_aspects.py](../tools/verify_frontend_aspects.py) and
[verify-native-profile-ui.py](../analysis/frontend/profile-regression/verify-native-profile-ui.py)
assert black outside the old scissor. Their border expectations must change
with the new policy; passing their current checks would only confirm the
old pillarboxed world. Haze RNG and clock history remain unsynchronized,
so frozen camera/animation captures alone cannot establish full-frame
pixel equality.

Physical original 16:9 output, original UI presentation under that output,
complete frontend layout/flow parity, and full-width native rendering are
**not verified by this report**.
