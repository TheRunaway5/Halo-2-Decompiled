# Menu travel, carrier intersections and washed-out output

Verified on 2026-10-07 against the user's current native-build report.

## Causes and changes

The native menu rendered the Covenant carrier in its bind pose. Original scenery
initialization (`10A1B0 -> 10A3F0 -> 10A660`) starts its named `idle` loop. That
single-frame animation has zero root translation and root scale **0.3937**. The
bind pose made the carrier about 2.54 times too large. Both the native and
original camera traces crossed that oversized hull eight times; both clear the
correctly posed model. `pose_scenery_model` now applies the same named-idle path
for menu scenery, retaining bind pose when an idle clip is absent. This menu
loader samples the initial pose; it does not implement the complete dynamic
scenery animation state machine. Original initialization, decoder results and
cache pins are in [MENU_CAMERA_RESEARCH.md](MENU_CAMERA_RESEARCH.md).

The menu camera now uses the recovered remaining-time quintic observer,
preserves derivatives across overlapping authored targets, performs the original
`.0001f` command flush, and decrements the director timer before the normal
observer update. The default scripted FOV is **70 degrees horizontal**. Rendering
samples a copy of the observer between 30 Hz updates using the frame accumulator;
sampling cannot advance script time or mutate authoritative derivatives. This
removes repeated 30 Hz poses on faster displays. Fractional sampling applies to
the menu's camera-point path; animated campaign cameras retain their existing
sampling path. Acceleration cancellation follows the original group boundary.

Vulkan presentation previously selected the first compatible surface format,
including sRGB attachments. Driver enumeration order could therefore change
the output transfer. The original backbuffer is `D3DFMT_A8R8G8B8`
([local reconstructed source](../tools/reference/halo2-decompiled/src/unknown_01cf50.cpp),
lines 2381 and 2495), and the current combiner/texture pipeline emits display
code values. An sRGB attachment encoded those values again. Vulkan specifies
this implicit encoding for sRGB image writes in its
[color-space reference](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html).
Presentation now explicitly prefers BGRA8 UNORM, then RGBA8 UNORM, with the
nonlinear sRGB surface color space, independently of driver ordering. A surface
offering only sRGB attachments is rejected instead of silently changing the
transfer. Shader sources and cinematic lighting implementations were not edited
by this fix; their remaining parity work stays with the separate conversation.

Unsupported vertex programs are classified separately from malformed known-format
programs. The renderer retains its existing fixed vertex path for unavailable
instructions, preventing an incomplete program interpreter from aborting menu
startup. Invalid lengths and final markers remain errors. A focused regression
checks this distinction; it does not establish shader arithmetic parity.

## Discriminating checks

`menu_camera_tests assets/disc/maps` replays the actual original menu syntax
graph through the native VM and Presentation, and loads the actual carrier
through the frontend's shared scenery-pose function. Before correction it
reported eight hull crossings and 8,658 repeated 120 Hz render samples. With
the idle pose alone the crossings became zero while repeated samples remained.
With fractional rendering, the same loop has **zero crossings and zero repeated
samples** across 2,868 moving intervals. Eight pinned original trace anchors
check the director/command integration, not just the isolated observer helper.
Render calls also check that the authoritative camera remains unchanged.

The original-instruction research fixture supplies the authored call schedule,
30 Hz simulation time, flush service callbacks and a no-BSP location service.
It executes the original camera-set, director, observer adoption/motion and
final-view routines. The native replay's **2,990 updates / 8,970 position axes**
match its float bits, with zero differences. Maximum forward-component error is
recorded separately in `analysis/frontend/camera-integration-final.json`; this
orientation comparison is bounded, not bit exact. The fixture does not run the
full original HSC scheduler or compare framebuffers.

The isolated original observer differential passes **4,217 updates**, plus
**1,000 authored basis comparisons**. `camera-observer-final.json` reports all
component errors and the tolerance scope. Position and derivative cases are
separate from the full menu integration above; they are not added to the
physics/sandbox comparison count.

`native_display_transfer` checks all **24 surface-format permutations**, RGBA
fallback, unrestricted surfaces and rejection of unsupported transfer formats.
For the GPU reproduction, an analysis-only Vulkan shim reordered the real
offered formats to put sRGB first. Before the fix, mean RGB code value rose
from 31.84 to 89.45 in otherwise equivalent title captures. After the fix,
default-order and sRGB-first captures are **pixel identical** (2,073,600 RGB
components). The shim creates no formats and changes no shaders. Source and
captures are under `analysis/frontend/force-srgb-order*`,
`default-order-before/after.*` and `srgb-order-before/after.*`.

The refreshed normal build passes all **18 CTest groups**. Seven focused groups
(vertex-program admission, menu replay, frontend bounds/retail menu,
native systems and retail system content/runtime) pass with ASan/UBSan. Evidence is in
`analysis/frontend/camera-final-ctest.log` and `camera-asan-ctest.log`.
These checks do not establish complete game, sandbox, animation or lighting
parity. The renderer's static scenery loader and original lighting services
still have documented limits.

## Launcher and reproduction

### User-supplied Xbox visual references

The user subsequently supplied Xbox menu and title captures, retained without
alteration as `analysis/frontend/xbox-menu-reference.png` and
`xbox-title-reference.png`. The latter includes a video-player overlay and a
visible `1.1` label. Native counterparts are
`launcher-mainmenu-reference-check.png` and `launcher-final-title.png`.
The references show violet atmospheric haze, cloud variation, softer distant
silhouettes and a high-contrast logo. The native scene still has flatter blue
background shading and lacks that atmospheric treatment. Removing the duplicate
display encoding corrects the reported washout; it does **not** complete fog,
cinematic lighting, shadows or bloom. The frontend currently ignores those
original script services, as documented in [FRONTEND.md](FRONTEND.md).

The captures use different flythrough moments and display aspect ratios, so
they are visual references rather than a frame-aligned pixel-difference test.
The original menu and title upper-center sky examples have median RGB values
`(25,34,61)` and `(19,28,48)` respectively, versus `(0,17,37)` in the native
initial title capture (normalized patch x40..55%, y15..25%). This is evidence
of a remaining color/atmosphere discrepancy, not a color-grading target to
apply globally. Shader and lighting ownership remains in the user's separate
conversation; the complete reference images are preserved for that work.

The rebuilt root `Halo2-Title-Launcher` selects `build-parity/halo2_native` ahead of
the older `build` directory. Default launch passes `--menu-screen 9 --skip-intro`
to start at the logo and Press Start prompt. Explicit movie/menu/game modes
remain available. Desktop and app-menu shortcuts were refreshed with
`python3 tools/install_launcher.py --build build-parity --desktop --title-first`.
The title-first executable and `Halo 2 Native (Title)` shortcuts are installed
separately so concurrent intro-playback work cannot change their default.
The original `Halo2-Launcher` remains the intro-first launcher.
The root launcher was run from `/tmp` with only hidden/frame/validation/mute
and screenshot flags: it rendered 30 title frames on RTX 5080 with synchronization
validation and no Vulkan validation messages. See `analysis/frontend/launcher-final-title.log`
and `launcher-final-title.png`.

```bash
cmake --build build-parity
ctest --test-dir build-parity --output-on-failure
build-parity/menu_camera_tests assets/disc/maps analysis/frontend/camera-path-after.csv
.tools/python/bin/python analysis/frontend/camera-research-trace.py
python3 tools/compare_menu_camera_trace.py \
  analysis/frontend/camera-path-after.csv \
  analysis/frontend/camera-research-retail-menu-trace.csv \
  --output analysis/frontend/camera-integration-final.json
.tools/python/bin/python tools/differential_camera.py \
  --library build-parity/libhalo2_kernels.so \
  --output analysis/frontend/camera-observer-final.json
```

Research fixtures and captures remain local under ignored `analysis/`; the
portable CTest regression embeds selected pinned trace anchors and uses the
user's local original cache. No original assets are added to the source tree.
