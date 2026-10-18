# Particle camera resources before collection

`Renderer::prepare_particle_views()` now reserves and uploads actual native
camera resources before particle command recording. Each input explicitly names
a submitted `ModelView` index, world/foreground channel, rectangle, source
material and camera bank. This index is not a simulation-player registration.
Duplicate contexts, foreign threads, stale targets and unsupported shader
interfaces reject. The renderer waits for the prior frame before replacing its
uniform allocation.

Preparation owns the real vertex/depth uniform bindings and a source-validated
resident program. It does not invent a particle or record the camera-only
attribute workspace. Material uniforms, texture lookups and current-surface
aliases retain their actual draw-time owners. The backend explicitly rejects
recording a camera-only resource. When it becomes an actual draw, material and
sampler bindings are populated even when the source material pointer is reused.

Whole original `13430` and `12F80` can now operate against this resource before
the eventual draw. `publish_camera()` verifies c0..11 against the actual uploaded
inputs and publishes the original bounds into that same allocation. The input
snapshot remains immutable; `published_state()` reads the real produced bytes.
An already published resource rejects another initializer before any reset
instruction executes, including when an older input snapshot is supplied.
Unpublished resources cannot be consumed as complete output.

The renderer binds each published resource to its explicit submitted view and
world/foreground bank. It verifies the rectangle and actual compatible native
descriptor layout, copies the published camera/clip bytes before replacing the
arena, and retains each draw's material-specific constants. The actual recording
also verifies viewport/scissor ownership. Uses expire when preparation or
recording changes the bank, targets are recreated, the renderer retires or the
caller changes threads. A reference to input state cannot be obtained from a
temporary use.

The native test covers preparation before any particle recording, independent
world/foreground slots, re-preparation, source publication, a deliberately wrong
camera, a wrong submitting rectangle, foreign-thread access and stale-use
rejection. It submits an input near bound38.3999977 and checks that the original
publisher's255.999985 reaches the actual recorded draw buffer. This demonstrates
consumption of the produced resource rather than an unchanged copied value.
Both complete publisher calls also match unchanged TU1.5 CPU execution and all
138,049 retained bytes per case; no original instruction or game/GPU return is
patched or supplied.

Seven retained source draws pass the prepared-resource route with five positive
RGB pixels on cleared depth. All five reference/packet images remain exact to
the preceding correct-scissor check. A separate resource stress test repeats
those captured packets77 times; it creates no simulation particle and is not a
visibility acceptance. The actual Vulkan allocation grows from241,664 to483,328
bytes while camera data survives transfer and the old use expires. The source
RNG and complete effect/system/location/particle pool snapshots retain all
192,520 compared bytes. Validation logs contain no Vulkan errors.

This establishes an early native camera resource, not the complete live World
handoff. The tests still use an explicit native camera caller; original frame
inputs, submitting-camera startup and world/foreground projection ordering need
their full producer history. The current reset guard deliberately rejects a
nonempty queue. Consuming/retiring a prior view's queue must be authenticated
before permitting its next reset; do not erase counters to force admission.
The separate raw-parent probe has not been connected to the prepared view and
still stops at its unowned live`3FF611` cache read. Matrix/current-shader/stream
ownership and the remaining GPU method consumers remain required.

Evidence and pinned builds are under
`analysis/map-world-features/particle-prepared-view-20261018/`. Normal CMake
registration and affected source objects compile, but a combined application
link/install is not claimed. Both launchers retain the shared normal runtime;
native MCC gameplay is still unavailable. The accepted7501144f world binary
and59 shader bank are preserved. All21 Xbox plus2 Vista maps, original
scheduling/resource/RNG history, host/client/latejoin/reset behavior, full live
map verification, normal-launcher delivery and Xbox visual acceptance remain
active requirements.
