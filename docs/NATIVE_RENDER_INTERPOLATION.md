# Native render interpolation

The requested high refresh presentation samples completed 30 Hz states. It
does not increase the game tick rate or add physics, fuse, damage,
inventory, animation or effect updates to fractional sampling.

`multiplayer_presentation` copies the current snapshot and samples player
translation, orientation and crouch; dropped weapon transforms; authored
weapon projectile transforms; released grenade positions; and stable body
throw frames. Full salted owners and definitions match endpoints. New,
removed, respawned, reattached and replaced objects retain current admission.
The existing player teleport distance cut is preserved. Local view rotation
still uses the current input.

`SceneInterpolation` captures every completed local simulation step, even
when several steps precede a submitted frame. Scenery and prop rendering
sample these transforms. Machine rendering samples retained device roots,
power and position channels, with the recovered gear wrap at one. Its portal
and collision consumers retain authoritative channel values. Root rotation
uses quaternion interpolation, with exact endpoint copies.

Initialized, event-free scenery idles retain the original controller update
and interpolate its last two completed local bone poses. Composing the
hierarchy after sampling preserves child bone lengths, including final-to-first
loop transitions. Existing first-person and locomotion presentation clocks
continue to sample their validated source channels at fractional render time.

The TU1.5 source-backed player path also samples **completed** render outputs.
`ModelNodeInterpolation` retains consecutive completed banks after the original
deferred finger/grip/IK callbacks. It reconstructs parent-local transforms,
samples those locals, and recomposes the hierarchy. The ordinary model-builder
base draw, selected LOD geometry, shields, and body/held-weapon observations
consume that same sampled bank. Carried weapons use their retained parent/node
frame, keeping the weapon attached to the sampled hand.

First-person arms and both weapon channels retain their completed output on
each game tick and sample it in camera coordinates. Current mouse look and
camera translation apply after sampling, without blending an old camera into
the current view. New owners, weapon/model changes, death, attachments,
teleports, rewinds, and missing ticks discard incompatible history. Repeated
views or submitted frames within one tick preserve the previous endpoint.
Unchanged poses and current endpoints retain their original matrix words.
Animation selection, events, ammo, damage, melee contacts, and source pose
banks stay on their existing simulation clock.

The renderer decodes the actual graph/index selected by the original manager,
including linked idle variants that are absent from an action-root preload.
The idle audit traverses the authored linked chains for standing, crouched,
single/dual body, FP weapon, and FP arms bindings for both multiplayer species.
Lockout contains 20 applicable weapon definitions: 208 bindings, 256 variant
bindings, 98 distinct clips, and 1,792 fractional/loop samples pass. This is asset
and sampling coverage; it does not show every variant naturally selected during
gameplay or establish completeness for unimplemented campaign AI states.

World and muzzle particles retain their previous completed simulation states.
Draw position, age, phase and velocity interpolate those endpoints; world
particle depth bounds use the same sampled positions. Particle birth/death and
collision callbacks remain discrete. Weather already advances from the
render view's elapsed realtime and needs no additional 30 Hz interpolation.

The campaign workbench also captures each completed step for its player
camera translation/crouch, actor roots, scene roots and projectile debug
geometry. Its decoded animation time follows the fixed-step remainder.
Checkpoint restores, BSP replacements and debug teleports reset history.

This is a native presentation enhancement. The original renderer's current
node bank and separate transition bank are documented in
[the source animation investigation](MP_ANIMATION_RENDER_INTERPOLATION_RESEARCH.md).
The original grenade rendering evidence is in
[the grenade scene investigation](MP_GRENADE_SCENE_RESEARCH.md).
Interpolation requires the actual implemented simulation state producers; it
does not supply missing vehicle/Havok, activation, or replication owners.

Validation includes fractional snapshot/lifetime/rotation tests, bone hierarchy
and loop endpoint tests, actual Triplicate device geometry with unchanged
portal channels, original scenery/prop scene controls, particle/effect
lifetimes, and host Vulkan grenade/animation runtime tests. Host and local
UDP evidence does not establish original Xbox framebuffer or physical LAN
parity.

October 15 evidence is retained in `analysis/animation-interpolation-20261015`.
The high-refresh regression drives a 30 Hz update clock with 60, 120, 144, and 240 Hz
render clocks, checks hierarchy lengths, current-camera attachment, held grips,
loop endpoints, stationary words, lifetime cuts, and multiple views per tick.
The split-screen regression compares half-tick submitted poses and exact current
endpoints against the captured original completed outputs; its diagnostic reads
those outputs without re-entering the source FP evaluator.

An actual firing run reached the previously unbound 1AF670→1AF7E0 collision
attachment query and its 1AE4A0 admission filter. The generator now retains
both complete TU1.5 bodies (99 and 27 instructions), including the original
BCE50/BAD60 calls. The filter passes 128 original-executable instruction,
GP/SSE, flag, and retained-memory comparisons over its object-type, flag, and
camouflage gates. Original listings and generation pins accompany the tests.
These are separate source-owner fixes, not an
interpolation approximation or a substituted collision result.

The final focused set passes 9/9 tests. Both Spartan and Elite split-screen
runs exercise actual SDL firing, reload, melee, and crouch inputs: each half-tick
run submits 282 changed FP poses and 282 changed WORLD poses; the endpoint runs
retain exact original completed words, including the held weapon. All 72 dual
weapon/species pairings pass 111,888 quarter/half/three-quarter FP samples and
exact current-endpoint checks through idle, fire, reload, aim, crouch, walking,
and jumping. The full matrix samples actual original outputs on the CPU;
the split-screen tests additionally validate native Vulkan geometry submission.

Validation uses a stable source snapshot inside the main project because other
work is changing shared renderer/camera interfaces concurrently. The snapshot
omits the separate in-progress lens-draw integration and retains its existing
vehicle-view API. `validation-runtime-vs-main.patch` records that boundary. The
current main runtime also passes a syntax check against the main headers. This
does not verify launcher delivery or a full rebuild of those concurrent features.
