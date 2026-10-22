# Native temporal upscaling

Settings > Video Settings > Upscaling is also available from campaign and
multiplayer pause Settings. The former Off / 2x SSAA / 4x SSAA control is removed.
Select Off, FSR 3.1, or DLSS 4.5, then choose the quality mode. Changes apply
without restarting and persist in `pc_upscaling` in the existing menu preferences.
The obsolete numeric AA field is read only for backward compatibility and saved
as zero. Restore Defaults selects Off and Quality.

| Quality | Input pixels per output axis |
| --- | --- |
| Native AA (FSR) / DLAA (DLSS) | 1 |
| Ultra Quality | 1 / 1.3 |
| Quality | 1 / 1.5 |
| Balanced | 1 / 1.7 |
| Performance | 1 / 2 |
| Ultra Performance | 1 / 3 |

Manual Internal Resolution applies when Upscaling is Off; while enabled, the
quality mode determines the input size from the actual drawable. Window mode
and output resolution remain independent. Selecting the automatic Internal
Resolution row opens the active quality control instead of saving an unused
manual size. Quality can be chosen while Off and takes effect when a method is enabled.
Resizing reconstructs the SDK context
and clears its history. Device/driver errors appear in the existing menu error
dialog before the preference is accepted. DLSS requires NVIDIA RTX hardware.

## Copied runtime inputs

`external/upscaling/manifest.json` records the original client-source paths,
byte counts and SHA-256 hashes. These are copied precompiled files, not links to
the donor checkout:

- FSR's self-contained Linux and Windows portable Vulkan bridge (FSR 3.1.3).
- NVIDIA's signed Linux Super Resolution 310.5.3 runtime and Windows 310.6.0
  runtime, plus their matching NGX link inputs and SDK headers from client-source.
- NVIDIA and AMD license notices.

The FSR bridge exports scalar native JNI entry points that do not dereference
the JNI arguments for creation, dispatch or destruction. The C++ adapter calls
only those entries with null JNI arguments; no JVM is loaded. DLSS calls the
copied NGX link library directly. Its K/M/L preset selection follows the donor
and NVIDIA's recommended DLSS 4.5 mapping: K for DLAA/Ultra Quality/Quality/
Balanced, M for Performance, L for Ultra Performance. Ultra Quality is a custom
input scale evaluated through the SDK's supported Quality resolution range.

CMake copies the current platform's runtime libraries and notices to
`upscalers/` next to the executable. Local staging and MCC test-build packaging
preserve that directory. The executable loads this private directory first;
the source directory is only a development fallback. NGX uses the writable SDL
application preferences directory for its model cache, with online updates
disabled unless an explicit NGX configuration already exists.

## Render inputs and lifetime

`TemporalUpscaler` owns its input conversion, motion, normalized depth, reactive
mask and output images; `NativeUpscaler` owns vendor contexts. Vendor feature
creation is performed before accepting a menu selection. Vulkan devices enable
the queried NGX extensions, extended storage-image formats and supported buffer
device addresses. Device work retires before resizing or destruction.

Upscaling is restricted to the in-game 3D world. Frontend worlds and model
widgets use native rendering through `Renderer::upscale_world(false)`; a frame
containing interface model viewports also retains its native presentation.
Changing quality never reduces the resolution of frontend text, images or models.

Each game view snapshots its depth after world/foreground rendering. A compute pass converts the original integer
W-depth to normalized perspective depth and reconstructs camera motion. An
auxiliary geometry pass uses prior vertex positions and independent foreground
projection history to supply animated object motion. Changes to vertex counts or source section/part identities reject
history. Split-screen views retain separate camera and vertex history. Their
independent world cells complete before the SDK evaluates the world atlas.
Changing world effects use the reactive mask. A separate D32 image supplies the unchanged FSR bridge with
the depth-target resource type required by its ABI, preserving the prepared
float samples through a device-local GPU buffer copy. SDK output initializes a
separate presentation-sized composition target. HUD, reticle, screen flashes,
pause menus and their ordered blur/tint passes then draw at native resolution,
with their existing blend rules. No interface pixels enter either vendor SDK
or its history. One fenced composition target and a separate full-resolution
postprocess source are reused between source color slots. The completed native
composite is copied to presentation;
there is no spatial fallback presented as successful temporal upscaling.

Raw point-light12FA0 viewport constants and clipping regions also follow the
actual fractional input scale, preserving the source half-pixel bias.
The jitter offset uses the SDK's pixel-to-clip convention, including its Y sign;
the actual jittered raster offset is used when reconstructing motion. See
[AMD's integration guide](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/super-resolution-upscaler/)
and [NVIDIA's DLSS 4.5 preset recommendations](https://www.nvidia.com/en-us/geforce/forums/geforce-graphics-cards/5/580689/dlss-45-super-resolution-faq-updated-jan-14/).

## Validation

`native_temporal_upscaling` exercises both real SDKs at every quality mode,
multiple frames, odd output dimensions, depth conversion and split views, then
checks GPU output pixels. `pstencil_live_graphics_selector` uses the actual
title/campaign-pause/multiplayer-pause menu controllers and paints their UI,
including saved upscaling choices and disabling upscaling again. The existing
fullscreen and Original/pStencil checks remain in that test.
It also compares thin, opaque interface details byte-for-byte against native
output with all six quality modes for both FSR and DLSS, and switches
between all window modes while FSR remains active. Interface masking is no
longer needed because the SDK runs before the interface exists.

`native_dlss_first_frame` is a direct first-frame SDK regression test, without
game shaders, motion preparation, compositor, or optional masks. It originally
reproduced two `VUID-vkCmdDraw-None-09600` diagnostics with Performance/M; the full
probe also reproduced two with Ultra Performance/L. A Vulkan call trace found
four private input-sized RG16 working images, of which two were used without
their initial layout transition. Explicit masks, exposure and extension queries
did not resolve that first-use defect.

The Linux 310.5.3 adapter now uses the NGX API's
[application-provided Vulkan function resolvers](https://github.com/NVIDIA/DLSS/blob/v310.6.0/include/nvsdk_ngx_vk.h)
to retain ownership of the affected private image group during M/L feature
creation. It transitions and clears these newly created images once, before
their first evaluation, with an explicit transfer-to-shader dependency. Matching
requires the feature's device and input size, RG16 format, a single optimal 2D
subresource, and the SDK's sampled/storage/transfer usage. Destroyed images are
removed immediately, and active/prepared features retain separate ownership.
Later evaluations preserve their accumulated history. K modes and the Windows
runtime do not use this workaround.

The signed vendor binaries and the K/M/L model mapping are unchanged. All
validation messages still count as failures; no message filter or suppression
is used. The strict first-frame and SDK output probes check every quality,
including Performance and Ultra Performance at odd output dimensions, and pass
without warnings. Whole-game visual quality and Windows gameplay still require
separate runtime validation.
