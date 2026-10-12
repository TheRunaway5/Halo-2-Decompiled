# Live native shield presentation

The native multiplayer renderer now submits the authored damaged-shield layer
for Spartan and Elite world bodies and first-person arms. Held weapons keep
their ordinary materials. The layer consumes the real player's current shield
feedback, vitality and biped object functions. There is no forced glow, demo damage assignment,
weapon-name gate, replacement opacity curve or separate feedback timer.

The original `133AB0` damaged-shield clone calls its guarded collector with
child flag zero. That reaches `133F70` with owner filtering enabled: every
additional model must have the first model's object owner. `167A0D` supplies
the unit owner for arms and separate weapon owners for the held models, so
the shield clone excludes held weapons. The native integration follows that
boundary by allocating and submitting only body and arms shield banks.
[Retail owner-filter witness](../analysis/damage-shields-goal-20261012/demo-live-layer/weapon-shield-correction/original-owner-filter-retail.json),
[TU1.1 owner-filter witness](../analysis/damage-shields-goal-20261012/demo-live-layer/weapon-shield-correction/original-owner-filter-tu11.json),
[original FP record writer](../analysis/damage-shields-goal-20261012/demo-live-layer/weapon-shield-correction/00167996-tu11.asm),
[original FP producer](../analysis/damage-shields-goal-20261012/shield-live-research/extra-owners/00167a0d-tu11.asm).

## Reusable integration

`SourceShieldModelShaders` can allocate its resident materials into a separate
output bank without changing the ordinary model's materials or geometry. The
existing constructor remains available. The live renderer retains the accepted
posed vertices before ordinary shader filtering, applies the original override
part admission, and preserves their local coordinates for the authored volume
noise. Each owner has its own material range; sharing a species/model does not
share animated shield inputs between players.

`select_shield_overlays` consumes the existing `DamageFeedbackState` and actual
shield value. `damage_feedback_shader_input` evaluates that owner's authored
biped functions using current health, shield, dead state and the render clock.
Cold and expired feedback therefore stop submitting this layer through the
same admission used for ordinary gameplay. Any admitted damage source that
updates this shared feedback can drive the presentation; the new renderer does
not specialize it to the SMG used for the acceptance test.

The body and first-person layer use the current native renderer's accepted
geometry and presentation policy. This is not a completion claim for the
original `4B5D0` model build, raw object `+F0/+F4` storage, source geometry LOD
lifecycle or whole-scene transparent queue. The ordinary native layer15 path
continues to submit the other transparent geometry and particles. Installing a
partial shield-only source queue would suppress those consumers, so this
integration does not install one.

The separate overshield shader family remains unbound here. It is not treated
as a damaged plasma shader. Full damage-source coverage and original world
contact/region authority also remain open; see
[the damage coverage goal](DAMAGE_SHIELD_COVERAGE_GOAL.md) and
[the original render-owner handoff](SHIELD_SPARTAN_ELITE_COMPLETION_HANDOFF.md).

The live acceptance test also exposed retained presentation callbacks that
outlived their render/audio owners. `release_presentation_owners` and a scoped
runtime guard now detach them on every exit, including exceptional exits.
The reused prepared world's current look pitch is retained when presentation
starts, alongside its yaw.

## Validation and delivery

`tests/source_shield_live_runtime_tests.cpp` loads the real Lockout cache and
authenticated TU1.1 executable. It selects unchanged authored start records
before object birth, uses ordinary walking when the source spawn scorer chooses
the same start, and fires through actual `Controls`. It never assigns position,
shield, health, weapon readiness, collision or feedback. Read-only observers
check actual world/FP submitted vertices and their owner/shader/render-model
identities, including rejection of any held-weapon model in a shield layer.
Cold, hit and expired feedback captures verify both species and isolate the
unhit neighbor. Additional scripted runtime captures advance the actual FP
ready/idle clocks before firing so raised arms/weapons can be inspected.

The selected demo source and runtime are
`.local/player-animations-20261012/source` and
`.local/player-animations-20261012/build/halo2_native`. The focused integration
changes are also in the main source tree for continued final-version work.
Build logs, before-images, runtime logs and captures are retained in
`analysis/damage-shields-goal-20261012/demo-live-layer`.

Game assets were moved by a separate cache-reset operation during this work.
Validation uses the retained original assets in
`/home/eric/.local/share/halo2-native/cache-reset-backups/20261012T201413Z/assets`;
the reset launcher setup configuration is preserved. Vulkan rendering executes
on the RTX5080 with the bundled Khronos validation layer in
`.tools/validation/usr`. The actual cold/hit/fade and running first-person
captures complete with zero validation errors. The initial default-layer smoke
attempt failed because this bundled layer requires its explicit environment;
that failed attempt remains in the evidence directory.

The real first hit reduces each target's normalized shield from 1 to 0.9339285
and publishes recent shield damage 0.0660714. Elite submissions contain 6,804
world and 4,815 FP arms vertices; Spartan submissions contain 9,258 world and
4,980 FP arms vertices. Both unhurt neighbors submit zero shield vertices, and both sampled
expired-feedback frames submit zero. Continuous captures advance 46 actual
updates through the FP ready/idle clocks before firing; they retain the
original material color animation rather than replacing it for visibility.
The 1800 original admission cases and 240 original material/input/constant cases
also pass. [Current demo log](../analysis/damage-shields-goal-20261012/demo-live-layer/weapon-shield-correction/live-validation.log),
[observed states and geometry](../analysis/damage-shields-goal-20261012/demo-live-layer/weapon-shield-correction/live/live-layers.tsv).

Both authenticated retail and TU1.1 executables also pass three bounded original
shield-clone/owner-filter cases each, including two separately owned weapons.
Their ordinary weapon draws remain present while shield overrides are excluded.
The complete original collector fixture passes 73 cases and 695,958 exact output
words. The rebuilt launcher-selected demo SHA-256 is
`368aca05d999fcfd6a14f339e790516fbf662561f3eba751e63fa9e399eb1f05`.

These captures establish the declared native presentation behavior, not Xbox
framebuffer parity or complete original damage coverage.
