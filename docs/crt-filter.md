# Optional CRT filter

Open F1, select **Display**, and enable **CRT Filter**. It starts disabled and
lasts for the session, including menu-to-game transitions, window resizes, and
campaign BSP transitions.
The completed game image, including movies, menus, and HUD, is filtered before
the native debug panel is drawn.

The reference is Phase-Distorter's `cpp/resources/shaders/crt_oled.frag` and
`cpp/src/crt_filter.cpp`. This is its flat OLED adaptation of Timothy Lottes'
public-domain CRTS / CRT-Lottes Fast, with hunterk's GLSL adaptation. The pinned
upstream source and complete license are in `external/crt-lottes-fast/`.
The reference checkout was at `9c1b3e5b7391bf246b42759d2b261f461cef9c8c`;
the original fragment shader's SHA-256 was
`1ed390ec31c144d3c5efd96698bc31941f313150a5618a12699f906fd7e0c11a`.

`shaders/crt_oled.frag` retains the reference Gaussian weights, cosine beams,
linear-light RGB grille, display-scale taper, and peak-preserving tone curve.
Halo uses the reference's continuous direct-scene path, since its image is
already rendered at the drawable resolution. The Vulkan conversion changes
shader bindings, fragment output, and image orientation: a Vulkan image copy
does not require the OpenGL viewport's vertical flip. Display code values use
the existing UNORM presentation format, avoiding an extra sRGB conversion.

Halo's existing 640x480 source canvas supplies the 480-row beam raster, with
logical width extended to the drawable aspect. This replaces EarthBound's
224-row raster; the filter's parameters are otherwise unchanged. Filtering
uses the existing full-resolution GPU scene copy and linear-clamp sampler.
There is no CPU readback during ordinary presentation, curvature, vignette,
temporal blending, or frame history. Disabled frames submit no CRT commands;
the pipeline is created on first use and retired with the swapchain.

This is an optional presentation feature requested for the native port, not a
claim about original Xbox CRT simulation or original game rendering parity.

## Validation

The optional debug-panel GPU test exercises the real Display checkbox,
unfiltered pixel identity after disabling, repeat-frame identity, renderer
replacement, and 640x480, 960x720, 1280x720, 1920x1080, 2560x1080, and
1920x1440 resize paths with Vulkan synchronization validation enabled.

A local cross-API check compiled Phase-Distorter's original `crt_filter.cpp`
and fragment shader without changing their implementation, then supplied the
same unfiltered native captures at the same 480-row raster. All RGB channels
matched exactly at 960x720 and 1920x1440 on the local NVIDIA RTX 5080.
The reference capture harness and comparison report are in
`analysis/crt-filter/`. This establishes the filter conversion on that GPU;
other drivers and physical displays have not been compared.

The updated title launcher also rendered 1920x1080 captures with the filter
disabled, enabled, and enabled with F1 open. Scripted SDL input selected the
Display tab and checked CRT Filter; the open-panel capture was visually
verified. These captures pinned the menu fog seed and completed with no Vulkan
validation messages. Results are in `analysis/crt-filter/validation.json`.
