# Original teleporter occupied-exit execution, 2026-10-10

The original TU1.1 engine blocks entry when another living unit occupies the
destination. It flags the occupying player; this path is not a telefrag. The
native port still has no live teleporter traversal consumer. This experiment
establishes a positive original occupancy case for that remaining integration.

`tools/oracle_mp_teleporter_occupied_20261010.py` creates two actual original
players on the owned Beaver Creek map using the existing complete join/spawn
and object/registry construction harness. The original pair initializer
`1A0340` selects the actual netgame source and destination rows. Original
`B8490` receives those two authored positions as explicit diagnostic inputs:
one player at the entrance and the other at its exit. No root, capsule,
membership, overlap, contact, success, velocity or damage output is assigned.

Complete `1A0580` then executes both bidirectional pairs. Its actual
`1A0400 → E0C20 → 16BEC0 → 247FC0` queries each return a positive contact,
identifying the other genuinely allocated unit. The shape collection contains
two sphere records and one capsule record. Both player flag words change from
`0001` to `0085`; the blocking-player mask `0004` is set. Both roots and velocities
remain unchanged. Neither original orientation nor position-delivery helper is
called from the blocked update. The source pair bank and all relevant calls,
stores, original stages and admitted services are retained in the report.

This is distinct from the earlier one-player empty-exit case, where the same
original update reaches `BA940/B8490` and changes the root from the authored
entrance to the destination. It does not establish a native capsule collector,
native destination geometry, a world tick, teleporter effects, native traversal
or network parity. The harness retains its documented cache-input relocation,
source-neutral device admission, compatibility/platform and Xbox estimate
limits. The source-generated entrance placement is diagnostic, not player
movement through an entire original match.

The independently matched complete u5 bodies are retained under
`analysis/multiplayer/integration/teleporters-lifts-20261010/paired`. This new
positive two-player experiment executes TU1.1 only; corresponding u5 bytes are
static correspondence, not a second original gameplay execution.

Reproduce into a fresh directory:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_mp_teleporter_occupied_20261010.py \
  --map assets/disc/maps/beavercreek.map --output /tmp/halo2-occupied-exit/report.json
```

The measured and independent repeat reports are in
`analysis/multiplayer/integration/teleporters-lifts-20261010/occupied-measured-v2`
and `occupied-repeat-v2`. They retain literal source state and assertions on
actual query identities, unchanged roots/velocities and blocked delivery.
