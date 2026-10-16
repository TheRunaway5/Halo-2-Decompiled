# Original particle depth binding and layout

The subsequent [per-view initializer work](SOURCE_PARTICLE_VIEW_INITIALIZER_TU15.md)
now adds the complete reset/depth arithmetic owners and tests fresh world
inputs against unchanged original execution. This document retains its earlier
35/41-owner checkpoints below.

The particle graphics ABI now binds its depth-presence check and W mode to the
same real D24S8 attachment as its renderer/target capabilities. Unissued,
foreign, duplicate, stale-generation, retired-renderer and foreign-thread
access remains rejected. The original `3FF5D0` setter uses this actual presence
input; unknown SDK fields and the unconstructed `4116C0` control cache remain
unavailable.

The depth-presence checkpoint has35 complete original owners and2,744
instructions. Its actual Backwash callback reaches534 completed instruction
callbacks before the original `3FF611` cache load. Unchanged original i386
matches GP/SSE, defined flags, x87/MXCSR, ordered path and all60,371,030 retained
bytes. The faulting load is a535th callback, not an executed instruction. The
hardware tracer records532 single steps. No SDK/game/GPU return is supplied.
The104 published command bytes do not include a completed depth command.

The subsequent layout work adds six authenticated original bodies:
`400A60`, `407850`, `407860`, `407900`, `407EE0` and `407A50`. All35 prior
body-site sets remain unchanged, bringing the module to41 owners and3,089
instructions. The depth-scale indirect jump at `407A57` has an explicitly
retained eight-slot table; its missing local bounds check is not mistaken for
a direct jump. Original format, pitch, texture-dimension and jump lookup bytes
are mounted read-only in a separate metadata workspace.

`configure_depth()` now computes metadata from the actual attachment dimensions
using whole original normalization, pitch, layout and depth-scale owners.
The admitted native SDK D24S8 format42 becomes original linear format `2E`.
The constructor's actual depth path calls the pitch producer at `406E11` and
the layout producer at `406E3A`; `406D87` belongs to the separate auxiliary
color allocation. The pitch producer matters:1920 D24S8 pixels select8192
bytes, whereas automatic64-byte alignment alone gives7680. The implementation
uses the former. Returned storage bytes describe the original layout and do
not claim a native Vulkan allocation size or original physical allocation.

The native identity exposes only the produced original-format fragment to its
authenticated reader. It does not expose fabricated Common/Data/refcount,
physical-address or complete Xbox surface bytes. The full framebuffer and SDK
allocation/history owners remain separate. The FVF builder is now compiled
and independently checked; it has not been substituted for the missing live
shader/pass owner.

Thirty-four pure metadata calls match unchanged original CPU execution across
registers, defined flags, x87/MXCSR, ordered paths and all4,396 retained bytes
per case. They cover normalization, pitch selection, linear/swizzled/compressed
layouts, all eight depth-scale slots, vertex layouts and the explicit cold
depth pitch/layout chain. Six native dimension cases include actual640/480,
1280/720 and1920/1080 interfaces plus odd, minimum and maximum admitted linear
dimensions. Invalid native extents are rejected. The initial32 independent CPU
comparisons are reused only after every current input bank, output bank, ABI
and execution trace is verified byte-identical; the two added explicit-pitch
cases have fresh original CPU comparisons.

The live-renderer contract test checks the produced format/dimensions/depth
scale against its actual attachment, retains its mode/upload ownership checks,
and reports zero Vulkan validation errors. This is resource and arithmetic
acceptance. The earlier seven-particle12-pixel cleared-depth test remains
component evidence; this stage adds no ordinary-map visibility acceptance or
complete original particle GPU submission.

The next owner is `13430`, called by `2BCB0` before the main `40E80` collector.
It also clears both particle queues. It must execute before collection with
its real viewport, depth-bounds and shader inputs. Running it after collection,
restoring queue counts, or accepting a zero `4116C0` cache would discard or
invent the requested source behavior. The complete parent, shader/material/
bitmap/sprite stream and native command consumer still need integration.

Evidence is retained under
`analysis/map-world-features/particle-depth-binding-20261015/` and
`analysis/map-world-features/particle-depth-layout-20261015/`, each with its own
acceptance and continuation receipt. Later shared launcher/runtime deliveries
are preserved. These source changes have not installed a new normal runtime;
both launchers' actual selections are recorded in the latest receipt. Native
MCC gameplay entry, the full21 Xbox plus2 Vista map scope, network/latejoin/
reset, original histories and Xbox framebuffer acceptance remain open.
