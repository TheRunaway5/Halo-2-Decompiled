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
Interpolation does not implement missing vehicle/Havok motion, corpse
rendering, source activation scheduling, or generic scene-state network
replication. Those owners still need their actual simulation state producers.

Validation includes fractional snapshot/lifetime/rotation tests, bone hierarchy
and loop endpoint tests, actual Triplicate device geometry with unchanged
portal channels, original scenery/prop scene controls, particle/effect
lifetimes, and host Vulkan grenade/animation runtime tests. Host and local
UDP evidence does not establish original Xbox framebuffer or physical LAN
parity.
