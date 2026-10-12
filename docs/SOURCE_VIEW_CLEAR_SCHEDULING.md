# Original view clear scheduling

The source clear planner reproduces the clear portion of original `132D0`,
`14B60`, `14AC0` and `131ED0`. The native renderer consumes a captured, admitted
Zanzibar initialization through reference-player fixture version 4. Ordinary
live caller/transition production and complete resource scheduling remain open.

## Original execution

`analysis/multiplayer-shading/original-clear-scheduling/original-session/`
contains an independent original session restored to `zanzibar-shading-baseline`.
Original guest code/data were not patched. Hardware breakpoints observed one
main-view interval, with 64 MiB RAM exports at its entry and the next main entry.
Twelve complete code-block checks across these exports match the pinned disc
executable. This was xemu 0.8.136 with OpenGL llvmpipe, not physical Xbox hardware.
Four additional SDK Clear/SetViewport block checks also match the original.

The actual main entry has header mode 0, temporary-clear byte 0, clear short 2,
transition byte `4670CD=0`, x87 control word `023F`, and RGB words
`3E8E8E82 3EB6B6BF 3EC6C6BD`. The original packer and actual SDK call both
produce `00475B63`. The observed SDK clear order is:

| Last bound target | Hardware flags | ARGB | Depth bits | Viewport extent |
| --- | --- | --- | --- | --- |
| 0, main | F3 | 00475B63 | 3F800000 | 640 x 480 |
| 1, accumulation | F0 | 00000000 | 00000000 | 640 x 480 |
| 10, shadow | F0 | 00FFFFFF | 00000000 | 128 x 128 |
| 29, icon | F0 | 00000000 | 3F800000 | 64 x 64 |

All four stencil arguments are zero. The icon call reaches SDK Clear directly,
outside `14B60`; flags F0 do not clear depth. The trace retains duplicate hardware
breakpoint observations at some binds and does not use their raw count as an
invocation count. A nested `132D0` entry has clear short 0 and performs no clear.
These targets have distinct color resources; the planner does not infer aliases.

## Module and native admission

`source_view_clear.hpp/.cpp` expose two pure operations:

- `source_view_initialization_plan` consumes the complete actual 0x298 caller
  header, transition byte at entry, explicit x87 control and the device viewport
  after the external projection/bind services. Target 18 is selected only for
  mode exactly 1. It emits a depth-enabled bind and preserves the clear-short
  and transition branches.
- `source_clear_plan` converts semantic flags to hardware channel/depth/stencil
  masks. A nonzero mask, nonzero header byte 7 and main target 0 prepend the
  temporary 640 x 480 black color clear, restoring all six viewport words before
  the requested clear. Attachment aliases and prior history are not inferred.

The native reference path admits only target-0 RGBA/depth/stencil initialization
without the temporary viewport operation. Version 4 supplies the actual header
and context; versions 1–3 remain unchanged. Unsupported target-18, color-only
and no-clear reference states reject before renderer resource mutation. Those
states need their real resource service or prior attachment history.

Vulkan uses the packed original RGB bytes, alpha zero, depth 1 and stencil 0.
Native clipping/resolution conversion supplies the physical clear rectangle.
This is a bounded source initialization consumer; it does not activate the
complete source view scheduler for ordinary gameplay.

The actual caller's reserved x87 bit 6 is clear in `023F`. Former helper
admission incorrectly required it set. The unchanged original gives identical
results for 33,984 paired inputs with either reserved-bit encoding. Both are now
accepted; unmasked exceptions, unsupported precision and other reserved
encodings still reject. The captured original input failed before the fix with
`Unsupported source clear-color x87 control word` and now passes.

## Verification

Evidence: `analysis/multiplayer-shading/original-clear-scheduling/`.

- `schedule-final-pinned/report.json`: 1,306 native/original event comparisons. Clear
  selection, mask conversion, RGB packing and viewport save/restore instructions
  execute unchanged. Projection/resource binds and SDK submissions are recorded
  service boundaries; their resource implementations are not executed.
- `math-final.log`: 864 terminal fog RGB cases, 67,968 packing cases across 24
  explicit control encodings, and the actual captured caller case pass.
- `ctest.log`: four focused reference/math/schedule tests pass.
- `sanitized.log`: 1,306 planner cases pass address/undefined sanitizers.
- `gpu-history-x11.log` and `gpu-history-lavapipe.log`: RGB `47/5B/63`, D24
  `FFFFFF` and S8 zero readbacks pass for controlled empty views at 640x480/1p,
  853x479/2p, 1281x721/3p and return to 640x480/1p. Foreground, sky, split isolation,
  fresh/reused renderer and repeated-frame controls pass too. These storage
  checks do not restore original split-screen caller headers.
- `comparison-final-1/` and `comparison-final-2/`: original frame 20 compared
  with final native captures using retained player/lighting/device inputs,
  actual source clear context, native tick 56 and explicit interpolation 0.
  The independent source trace matches all 11 camera words in frame 20's fixture.
  Native FP pose frame 56 is explicit. Pose tick 55 and shader time remain native
  diagnostic inputs; the original HUD clock/full simulation are not restored.

The first GPU fixture omitted the renderer's required atmosphere input. It was
corrected to supply an explicit empty atmosphere. Wayland/NVIDIA resize attempts
then failed inside `vkDestroySwapchainKHR`; the unchanged prior binary reproduces
that failure too. X11/NVIDIA and software Vulkan complete all four layouts.
Failed attempts are retained and are not passing evidence. Intermediate
comparisons 2/3 belong to the earlier private ELF; final comparisons pin the final
ELF and shader files. Shared launchers/build outputs were not replaced.

## Remaining source work

The [producer audit](SOURCE_VIEW_CLEAR_POLICY_AUDIT.md) defines the wider boundary:
real `2BA10` caller construction and route gates, transition/loading lifecycle
before `132D0`, target 18 resources, source generations and retained regions,
`13C20` pop/alpha-clear scheduling, independent target/layer chronology and
source phase/owner admission.

The original window screenshot is an actual emulator image; presentation lag
relative to its CPU breakpoint has not been excluded. Camera, retained lighting
and device inputs, and admitted initialization support the bounded Zanzibar
comparison. Complete shader clocks, controllers, visibility, startup history,
HUD and all-map matched original framebuffers remain unverified.
