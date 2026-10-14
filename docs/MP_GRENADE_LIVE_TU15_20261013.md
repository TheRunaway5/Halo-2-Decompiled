# TU1.5 live grenade integration

The live physics option delivers the actual grenade flag and signed selector
through the original player command record. Complete14FEC0/1522E0/E8E50 action
owners now create, hold and release the actual FD200 type5 body. Native grenade
motion, fuse, contact and damage updates are disabled when this source world
owns physics. The source action component, selected grenade, body pose, creator,
fuse, arming progress and age are read back after the completed world tick.
Projectile age is body+1AA; body+132 is a collision surface and is not an age.

`tools/research_physics_grenade_live_tu15.py` retains29 original TU1.5 bodies and
verified call/storage contracts. The only reference executable has SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Original cached render-state and breakable-surface effect storage constructors
and map resets now run. Geometry resource I/O fills the source-allocated cache
block with actual sector bytes and signals its caller's completion byte. It is
an explicit synchronous native file boundary; Xbox asynchronous/GPU completion
is not asserted.

The101BC0 call at101DB8 produces an exact96-byte effect request and four named
locations. The native bridge observes that request, executes the complete
original178E30 owner through the normal live dispatcher, and retains the
completed effect's original3C location records. Source damage and projectile
retirement remain in101330 and the world owners. A failed effect admission does
not become a successful native explosion.

Protocol62 carries those retained location names and full world frames alongside
the explosion event. Native presentation resolves each authored effect marker
from those actual frames. It does not choose a flight-derived direction or a
replacement chase/effect offset. Legacy native explosions keep their existing
presentation path. The64-event retention limit is a native transport policy.

The visible objects harness now requires an actual grenade command, held and
released bodies, downward source velocity, completed explosion publication,
projectile geometry and explosion particles for each local view. The UDP mode
compares all projectile fields and all explosion location frames against
same-tick authority publications. These are acceptance requirements, not a
claim that the current run passed.

## Current evidence and remaining work

`live-integration-v142/objects` passes181 observed ticks with two local players.
One normal grenade command produces the actual held/released body, downward
source velocity, explosion at tick77 and completed salted-header retirement.
The renderer submits63 projectile batches and933 explosion particle batches
per view. The unaltered window video visibly records flight, explosion and
smoke. Particle submission counts do not establish visibility through occluding
geometry or retail framebuffer equality.

`live-integration-v143/lan-objects` passes182 exact same-tick authority comparisons
with three actual participants and two client views. Comparisons include every
projectile field, explosion creator, selected effect and each named source
location's full transform. Both client views submit990 explosion particle
batches. The same binary passes local/UDP vehicle camera, movement and lift
regressions. Each run retains its executable hash, exact inputs, frames,
comparisons and unaltered X11 video.

Original BA3E0's eight notification callbacks and BA490's light, physics,
observer and header retirement owners now execute. The generator authenticates
the literal474BC4 table and indexed call reader, plus the direct deletion calls.
No disappearance-only or fabricated explosion result satisfies acceptance.

The current checkout also passes local and UDP grenades in `live-root-v145`,
including the original13AB49 retained-effect update phase after object
retirement. Local rendering records1285 explosion particle submissions per
view; UDP records1033 per view and181 exact authority comparisons. Movement,
vehicle camera and UDP lifts pass in the same current-root build. The local
lift's first two X11 recordings are empty despite gameplay PASS; its repeat in
`live-root-v148` retains207 actual window frames and exits0. Do not treat the
failed recordings as video evidence.

Full physics parity remains incomplete. Source effect update/retirement across
repeated explosions, prop impulses and destruction, plasma attachment, damage
attribution, complete source fatal lifecycle, every weapon input owner, and
retail trajectory/contact comparisons are still required. Source action-word
adoption covers grenade gating but does not by itself complete every action.
The private source-v3 host/client pair is a coherent isolated test snapshot;
it lacks unrelated current production wire fields even when its version number
matches. Production compatibility and default-launcher delivery require their
own current-root build and tests.
