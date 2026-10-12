# Damage/recharge handoff, 2026-10-12

Work stopped at the user's request to release the checkout for build repairs.
This is partial delivery, not completion of full damage coverage or break/recharge rendering.

## Completed changes

- `update_unit_vitality` in `unit_damage.hpp/.cpp` retains the ordinary original
  recharge effect attempt before the shield update. The old charging flag gates
  the attempt, including zero-rate and overshoot cases; the final charging flag
  is not used as a substitute. Existing scalar recharge arithmetic is reused.
- `Simulation::damage` now retains its ordered depleted/damaged attempts.
  The shared ordinary vitality update retains recharge attempts. A bounded
  journal carries serial, tick, full salted target, authored effect and owner.
  NONE effect requests remain observable. Checkpoint restore clears the journal.
- `MultiplayerMatch::state` publishes that journal, and native LAN snapshots
  encode/decode/validate it. Native protocol is now **50**; host and client must
  use the same version. This is a native protocol change, not Xbox wire parity.
- `unit_vitality_update_tests` and CTest `original_unit_vitality_update` compare
  the ordinary transition against unchanged original instructions. Actual run:
  **42 cases, 34 complete original states and 8 pre-effect boundaries passed**.
  Stopped original executions establish only the effect attempt; their later
  allocation/strength results are deliberately not asserted.

## Validation and build state

The transition test built and ran successfully in `build-damage-shields-20261011`.
The subsequent wider build was stopped to release the checkout for the user's
compile repairs. Its log is `/tmp/halo2-damage-wrap-up-build.log`; this is not a
claim that the entire current game compiles. Other chats are changing this same
checkout, so preserve their changes.

An unfinished class9 particle shader expansion was removed before handoff:
`world_effects.cpp`, `particle_renderer.cpp`, `particle.vert`, and
`generate_particle_graphics_program.py` retain their previous class7/8 behavior.
The added perpendicular shader/CMake targets were also removed. No launcher,
selected demo binary, or imported asset location was changed by this delivery.

## Still required

The new journal is **not yet consumed by shield effect playback**. Do not claim
visible break/recharge completion from request transport. Original attachment
creation, current marker/node frames, initial advancement, PRTM mesh particles,
and the Elite class9 depletion leaf remain necessary. Primary evidence and
fixture contracts are in `SHIELD_BREAK_RECHARGE_LIVE_RESEARCH.md` and
`analysis/damage-shields-goal-20261011/shield-break-recharge-live/`.

Host scalar damage and source raw vitality still have different ownership.
Unconditional source readback would erase actual host hits. Event admission and
retirement of host writes must be joined atomically; see
`DAMAGE_SHARED_AUTHORITY_NEXT_JOIN.md`. Melee/current-pose damage, falling,
kill-volume ordinary delivery, vehicles and the remaining producer classes are
not certified complete by this change. Original kill-volume evidence is in
`analysis/damage-shields-goal-20261012/remaining-live-joins/`.
