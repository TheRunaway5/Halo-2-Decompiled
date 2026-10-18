# Particle and World execution over one context

`SourceParticleGraphicsBindingTu15::run()` now composes the existing original
particle, collection, model, effect-frame, map-support and lens-request owners
through `source_run_particle_world_execution_tu15()`. Each downstream owner
receives the same memory adapter, instruction callback, store observer and
caller services. This keeps native device-field guards tied to the instruction
actually executing, including across module boundaries. Unknown platform or
resource calls still require the caller's real service or stop with their ABI.
No callee return or graphics state is supplied by this composition.

The existing `40E80` collection owner remains in the shadow/frame module. It is
not duplicated in the particle kernel. The retained live call also requires
original `144910` quaternion-to-matrix arithmetic, virtual object callbacks in
the map-support module, and `2DB80` attached lens requests. Those existing
owners now share the particle execution context. The particle kernel itself
remains unchanged at 70 bodies and 5,992 instruction sites.

The graphics binding retains its original root-entry boundary. A direct
`40E80` graphics root rejects before executing any instruction, touching a queue,
changing the ABI or emitting a command. Its downstream availability does not
bypass the actual World/pass caller. The permanent renderer guard test checks
this with nonempty queue canaries, alongside its existing device/thread/target
and lifetime checks. The prepared-view empty-queue guard is unchanged.

The CPU integration check starts from the actual retained Backwash main call at
view1/tick240. Its complete `40E80` execution produces exactly the captured
return ABI and all 60,305,494 source bytes. The instruction callback observes
157,502 sites, including original attached-effect/lens requests and particle
queue production. This is replay of a real native World input bank, rather than
a synthetic empty queue. Earlier stopped runs preserve the additional owner
boundaries used to complete the composition.

Independent execution of the unchanged OG Xbox TU1.5 executable also matches:
general/SSE registers, defined flags, x87 state, MXCSR, the full instruction path
and every retained source byte. The hardware tracer executes 157,495 steps;
its REP/string handling accounts for the callback-count difference. No original
instruction is patched, no native expected result is used, and no game or GPU
return is supplied. The original image hash is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This proves the selected complete collection call on its retained inputs. It
does not establish all World scheduling or Xbox framebuffer behavior.

The seven-draw native GPU regression and separate 77-packet allocation-growth
check pass with the composed executor and current root guard. Five images
remain byte-exact to the preceding stage, with five positive RGB pixels on
cleared depth. All 192,520 compared bytes of the retained RNG and four complete
particle pools remain exact. Vulkan validation reports no errors. The separate
raw draw-parent probe retains its same 535-instruction path and still stops at
`3FF611` because its actual per-view initializer has not been connected before
collection; the new executor does not invent that missing history.

Normal CMake includes the executor and registers the retained-call test. The
CPU test can run without exporting artifacts, so repeated CTest invocations do
not collide with a previous output directory; it skips when its optional local
capture is absent. Four affected normal-source objects compile. The private
runtime checks use the recorded coherent frozen renderer/backend plus current
owned source overlays. A complete link of the current shared application and
installation are not claimed. Both launchers still select the shared
`1fe89481` runtime and report native MCC gameplay unavailable. The accepted
`7501144f` world runtime and 59 shader bank are preserved.

Evidence and build lineage are under
`analysis/map-world-features/particle-world-execution-20261018/`.
The next handoff must provide the actual prepared view before collection,
using real camera/viewport/near-far inputs and preceding-queue consumption and
GPU completion. A post-collection camera receipt cannot justify resetting that
queue. The complete original parent, current shader/matrix/vertex-stream and
GPU-method consumers, original foreground order, ordinary world-depth
visibility, all 21 Xbox plus two Vista maps, authoritative networking, late
joins/resets, combined normal-launcher delivery and Xbox visual acceptance
remain open. The all-map goal stays active.
