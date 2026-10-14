# TU1.5 grenade lighting: source and frame evidence

The latest combined model/map light checkpoint is
[TU15_MODEL_LIGHT_RECEIVERS_20261013.md](TU15_MODEL_LIGHT_RECEIVERS_20261013.md).
It adds exact source command/range coverage, the full 28-draw strict comparison,
the no-op model-reference rejection, and the current launcher delivery boundary.

The reference is the unchanged **OG Xbox TU1.5** executable and original Xbox
cache assets. Vista supplies none of this evidence. The work remains incomplete:
strict Vulkan pixel comparisons fail and the complete model/contact/audio owners
have not been joined. No physical Xbox framebuffer capture is claimed.

## Authenticated original execution

Evidence root: `analysis/multiplayer-shading/world-effects-goal-20261011`.

`tools/prepare_xbox_tu15_visual_reference.py` packages the owned disc assets
with the owned unchanged TU1.5 executable, disables extract-xiso's XBE media
patching, then reads back and hashes all 81 packaged files.

`tools/capture_xbox_tu15_grenade_reference.py` clones the private normally
played Lockout session, sends a normal left-trigger grenade input, and uses
hardware breakpoints at TU1.5 `4A1D0` and `4A3C7`. GDB holds the guest while its
RAM and light/camera records are captured and the actual PGRAPH RenderDoc
capture begins. The capture closes at the next `132B0` view entry before GDB
detaches. Guest code is not patched. Private X11/profile/HDD resources isolate
the capture from ordinary game files and other sessions.

| Artifact | Identity |
| --- | --- |
| TU1.5 executable | SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Complete live `.text` | SHA-256 `d9c6fd7e7475862b1f19c2da80d7f2f6dc41da53619991002e9546e5f9e629fe`; byte-identical to the owned image |
| `xbox-tu15-frag-capture-v7/physical-ram.bin` | SHA-256 `1120cd1b7c4838c7b30fc3504a0c233d537e03147a7b5c368a0a4b4fa7b5ff5c` |
| `xbox-tu15-frag-capture-v7/tu15-light_capture.rdc` | SHA-256 `5e5ff5ee992693f027ba3eb7ea23b85acda55a31044809d9c6dee7bd30e28202` |

The earlier `tu15-frag-before-light` HDD snapshot was saved after guest detach
and is not a reliable source-point snapshot. The `v7` RAM/GPU join above replaces
that attempt. Earlier base-disc Xbox references remain diagnostic only.

## Source-to-GPU join

Whole-function instruction correspondences authenticate TU1.5 addresses
individually; no image-wide relocation delta is assumed. The retained source
replay includes `C35D0`, `31C70`, `40E80`, `3BAA0`, the `4D5D2..4D616`
post-material corridor and whole `4DA90`. Only the final device flush `1CF30`
and indexed submission `405080` are intercepted in the range oracle.

`xbox-tu15-original-owner-v2.json` and the registered fixture
`tests/fixtures/live_bsp_effect_light.tu15.original.json` independently compare
27 native BSP draws: owner/section/part order, topology, every selected index,
and all 36 light-constant words. The actual captured GPU draw counts agree
with those original range outputs. Consumed vertex constants and combiner
constants also match the original GPU banks exactly.

The first 22 BSP draws precede an actual non-BSP model draw. Original command
11 names model `E6FA0585`, `scenarios\\objects\\multi\\lockout\\power_module\\power_module`,
and the intervening GPU call submits 229 indices. Five more BSP draws follow.
The fixture retains the original target immediately after that model call as
the explicit seed for the final BSP group. It does not silently omit that call
from a claimed complete-frame comparison. Seven later cube/volume light-shader
candidate draws retain matching light banks; their precise owner/phase
classification remains open. Earlier selection notes calling all eight model
draws are superseded by this source-confirmed boundary.

`xbox-tu15-frag-gpu-frames-v2` retains the original OpenGL/Mesa replay targets,
depth/stencil, all bound texture mips/faces, shaders and current uniforms.
`xbox-tu15-frag-gpu-frames-nvidia-v1` repeats the unchanged capture on NVIDIA
OpenGL to distinguish driver arithmetic from native ownership or input errors.
These are two emulator replay backends, not two physical Xbox captures.

## Native fixes and validation

The native grenade owner submits authored raw BSP sphere-light passes. The
plasma throw was corrected to detonate inside the actual Midship receiver BSP;
`live-bsp-plasma-light-gameplay-tu15-cube-v3` passes 480p, 720p, 1080p and
four local views. Every layout has real light packets and zero Vulkan validation
errors. Counts alone do not establish appearance parity.

The matched capture exposes non-seamless cube filtering: original cube samplers
clamp within a face. The renderer now enables `VK_EXT_non_seamless_cube_map`
and uses that sampling contract for authored textures. On software Vulkan,
this single change reduces differing channels in the first BSP group from
4,429 to 2,271. This isolates a real filtering mismatch without tuning light color.

The native frame fixture verifies the original color, D24 depth and stencil
seeds byte-for-byte before drawing. Strict comparisons remain failures:

| Comparison | Scope | Remaining differences |
| --- | --- | --- |
| Mesa OpenGL to software Vulkan, corrected cube filter | First 22 BSP draws | 1,221 pixels / 2,271 channels; maximum channel delta 6. First 15 draws are byte-exact. |
| NVIDIA OpenGL to NVIDIA Vulkan, corrected cube filter | First 22 BSP draws | 352 pixels / 461 channels, including 91 alpha pixels. RGB differs at 263 pixels; only 16 RGB channels differ by more than one shade. |
| NVIDIA OpenGL to NVIDIA Vulkan | Final 5 BSP draws, after explicit original model seed | 5 pixels / 17 channels; maximum delta 55. |
| Minimized NVIDIA comparison | First 2 BSP draws | One alpha channel at `(639,385)` differs; all RGB channels match. |

The first-two-draw mesh probe retains actual VS/GS outputs. The screen bank and
texture coordinates in the VS probe match, while the GS capture exposes small
W/clip arithmetic differences between the two APIs. These instrumentation and
driver results identify the remaining investigation; they do not justify adding
an arbitrary depth bias or relaxing the acceptance test.

## Remaining work

1. Resolve the minimized depth and texture-filter arithmetic differences using
   source/hardware evidence and keep the strict pixel gate intact.
2. Join live model receivers, shared sorted light-layer order, and the remaining
   source owner/phase boundaries, including the power module case above.
3. Finish original contact/audio ownership and full matched gameplay frames,
   including campaign actors, split screen, resize and resolution scaling.
4. Repeat complete installed-runtime verification after these gates, preserving
   concurrent checkout work. The current launcher-selected binary has already
   passed the four-layout plasma sweep and a Cairo Marines capture, retained in
   `installed-current-plasma-tu15-v1` and
   `analysis/campaign/character-shaders-20261013/installed-current-marines-v1`.
   Both launchers select its verified SHA-256
   `3d644b8a4dd2c60071bc27c182201170954e014deb7ba66d91dbcad88a9e25c9`.
   A passing CPU fixture or standalone shader test is insufficient to mark the
   full world-impact/grenade or campaign character scope complete.
