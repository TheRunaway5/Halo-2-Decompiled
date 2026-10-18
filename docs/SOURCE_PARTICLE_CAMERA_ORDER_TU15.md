# Original World camera publication order

The ordinary TU1.5 path publishes its camera before initializing the view pass.
The authenticated chain is `229EF0 -> 2B5B0 -> 2B770 -> 2B9F0`.
`2B9F0` calls `132B0` at `2BBFB`; that context owner calls complete `12F80` at
`13315`. Only later does `2B9F0` call `13660` at `2BC57`, reaching `2BCB0` and
its complete `13430` view initializer. The preceding native interface required
initialization first, preventing this actual ordering.

The ordinary type-zero view also passes literal zero as `2B770`'s second
argument. Its actual stack transfers that value into `2B9F0`'s third argument,
packet byte7, then the scaling argument of `12F80`. The World camera is unscaled.
The audit verifies the complete containing bodies, direct callers, all relevant
stack cleanup, both packet copies and the actual instruction bytes. Alternate
render types, foreground and timed-boundary execution remain separate scopes.
The reproducible audit is `tools/audit_world_particle_camera_order_tu15.py`.

`SourceParticleGraphicsBindingTu15::prepare_view()` now stages the actual
resource without resetting a queue. `publish_camera()` can run complete `12F80`
against that resource before the pass initializer. Camera publication alone
does not make a prepared resource drawable: its GPU publication is deferred
until both original producers have completed. The already prepared pass enters
through `initialize_prepared_view_pass()`, retaining the real source counter and
caller packet. The older explicit initializer-first interface remains supported.

The packet consumer validates and orders all owned method ranges by their
actual positions. It uses the bounds method emitted last. This matters for a
near-boundary pass: the later initializer can replace the earlier camera's
near bound. The actual uniform allocation, consumed draw and eventual recorded
allocation all retain that final value. A pending camera rejects rendering and
public constant consumption. Queue admission is checked again immediately
before initialization, before counter or packet writes. A failed initializer
cannot be retried, republished or used to advance the counter again.

Six calls compare exactly with unchanged OG Xbox TU1.5 execution: three complete
camera publishers, two complete initializers, and the required target/clear
boundary of the remaining initializer. Every retained source byte, general/SSE
register, defined flag, x87/MXCSR value and instruction path matches. The
boundary stops before the unowned `14BA0` service and supplies no return.
The native resource test covers ordinary and near-boundary modes, deferred
publication, last-writer behavior, a queue filled after preparation, failed
initializer reuse, actual recording and resource expiry.

The ordinary multiplayer World particle caller now requests unscaled clip
bounds alongside its existing unscaled camera matrix. The default near W bound
is 255.999985 rather than 38.3999977. Foreground and weather paths are not changed
by this correction. Campaign World particles already use the unscaled input.

Evidence and build lineage are under
`analysis/map-world-features/particle-camera-order-20261018/`. Resource tests use
a coherent pinned renderer/backend with the owned source overlays recorded in
their receipts. The per-view test inputs remain declared native camera inputs;
they do not reconstruct an actual World startup or prior-frame history. Live
collection still needs the real prepared resource and correct preceding-queue
consumption/GPU completion. The complete original draw parent, shader/matrix/
vertex-stream ownership, all-map and networking/reset acceptance, combined
normal-launcher delivery and Xbox visual acceptance remain required.

The seven retained source draws and separate 77-packet allocation-growth GPU
check pass in the corrected camera-first order. All five images remain exact
to the previous accepted packet check, with five positive RGB pixels on cleared
depth. All 192,520 retained RNG/pool bytes remain unchanged. The previous
initializer-first resource regression also passes. These checks report no
Vulkan validation errors.

A separate paired live prototype runs the same neutral 240-tick Backwash input
with one and four local views, changing only the ordinary World clip flag.
The four-view case retains seven authored source submissions and changes their
near bound to 255.999985. Every other geometry word, all 192,520 RNG/pool bytes
and the final frame image match its baseline. The one-view spawn produces no
source glowbug submission and is retained as a negative control, with matching
state and image. No new ordinary scene-depth particle visibility is claimed.
The probe uses the pinned coherent prototype backend, not the complete current
shared application or normal launcher.

Four affected normal-source units compile in the saved build receipts. An
authority-receiver header change was handled by recompiling its actual runtime
dependent; a subsequent Scene header change required the small test units to
be refreshed. Scene/network headers changed again after the saved runtime
compile. The receipts record that drift explicitly; they do not certify a
combined current-tree link. Both launchers retain the shared 1fe89481 runtime
and report native MCC gameplay unavailable. The accepted 7501144f world binary
and 59 shaders remain intact. The all-map goal stays active.
