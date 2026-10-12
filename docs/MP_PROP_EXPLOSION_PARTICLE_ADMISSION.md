# Actual Lockout destruction effect admission

The actual E70A0595 effect has flags1C, two locations04000DE9/06000DEB and four
events. All delays are zero. Events0/1/2 have duration bounds3D088889/3D088889;
event3 has40000000/40000000. Read-only
[metadata inventory](../tools/inspect_prop_explosion_definitions.py) retains
complete original records, pointers, parts, system/emitter rows, movement
definitions/controllers/parameter functions and cache/tool hashes.
Assembly schemas are navigation, not semantic behavior authority.

| Event/system | Particle | Shape | Movement tag | Native vertex class |
|---|---|---|---|---|
|1/0|E71C05A7 flash_explosion|2|E71E05A9 explosion_small|7|
|1/1|E71F05AA metal_bits|4|E37301FE stone_bunch|7|
|1/2|E72005AB shrapnel_sparks|2|E1FD0088 stones|7|
|1/3|E72205AD panel_impact|2|E1FD0088 stones|7|
|1/4|E72405AF plasma_in_beams|2|NONE|8|
|2/0|E72505B0 white_rolling_flame|2|E42202AD hot_smoke_puff|7|
|3/0|E72905B4 smoke_trail_large|2|E72A05B5 engine_smoke|7|

All seven systems have actual world coordinates0 and flags0. Six have nonempty
pmov controller blocks; the only controller-free system needs classification8.
The current closed SMG muzzle runtime admits one event, local coordinates1,
first-person mode, no controllers and classification7. Reusing it by discarding
movement or events would change these authored effects.

The [actual material probe](../tools/inspect_prop_explosion_runtime.cpp) first
observed four failed imports. The real textures are valid2D with three mips,
but metal_bits/panel_impact/smoke use BC2 and flame uses BC3. The existing material
importer always invoked the BC1-only decoder. It now invokes the already
verified generic BC1/BC2/BC3 dispatcher while retaining its2D admission guard.
No shader is modified. A focused cache test imports **all seven actual
materials**, checks six class7/one class8, and verifies literal RGBA expansion
from the existing decoders. Old negative probe output is preserved separately.
Material availability does not admit classification8 drawing or particle motion.

The original unchanged aging chain is24A820→249210. After ordinary position,
phase, retirement and bounds bookkeeping,249210 invokes2C06C0 for actual
movement definitions. That owner walks controllers and calls2BF0C0, whose
type dispatcher selects physics0, collider1→2BF380, swarm2→2BFE30 or
wind3→2C0420. Physics0 reads actual526254 gravity/constants, source value-cache
inputs and parameter functions before ordered velocity/angular updates.
Collider1 requires actual terrain/contact delivery; it is not replaced by
free linear motion. These original bodies are available under
`analysis/map-pack-update/functions/`; unmeasured controller branches are
listed-only evidence, not original live execution or native success.

The smallest next visual owner is complete multi-event/system retained state
plus the actual controller dispatch/value evaluation for the two class7 sparks
systems, followed by genuine current terrain collision when their definitions
request it. Existing original birth/emitter and render-input/sprite/material
kernels can supply lower pieces. A world-effect runtime must retain the local
visual RNG bank, actual event/location identities and lifetimes, independently
of projectile gameplay RNG and source shot ticks. No closed complete world
explosion particle runtime is claimed by this audit.
