# Player owner bounds used by model and first-person shading

The original model collector `133790` computes projected shader coverage from the actual owner object's cached center at `+30` and radius at `+3C`. Attached collectors inherit that owner coverage through `133AB0`. First-person hand and weapon vertices, the camera position, and the biped physics position at `+270` are separate inputs and cannot replace this sphere.

`BDEF0` produces a type0 biped's cached sphere through its regular branch. Its optional Havok path requires the object type mask `0x800`, meaning **type11 only**, as well as a non-NONE physics datum and C0 bit6. A biped's own physics datum or C0 bit6 therefore does not enable that branch. The regular center is the authored object-definition bounding offset at `+8..+10`, transformed through the actual posed WORLD node0 at the object's relative `+116` offset. Radius is the actual object `+A0` scale multiplied by the definition radius at `+4`. Node0's scale and object scale are separate source inputs.

The existing `source_shadow_sphere` helper implements this regular producer. The name reflects its first consumer; the same output supplies object lighting and model shader coverage. The native player caller can compute it once for an explicitly admitted body pose/bounds update, retain it with that player's lifetime, and copy its coverage to the player's attached and first-person collectors. This proves the arithmetic when the actual node0 and scale are supplied. It does not establish that native idle phase, aiming, blending, death/lunge state, or bounds-update scheduling reproduces the original current node0 and cache lifetime.

## Retained original inputs

Both retained original records replay **untouched retail `BDEF0` entry through `BE07F`**, including the original type/physics gates. No original services are replaced. All eight output words agree with their actual cached object `+30/+3C` words and with the native helper; neither record calls `1D44F0`.

| Map | Actual owner | Definition | Cached center words | Cached radius |
|---|---|---|---|---|
| Ivory Tower (`cyclotron`) | `E2C60057` | `EB23099E` | `C0A3AE17 C0B05C37 3EBB4240` | `3EE147AE` |
| Zanzibar | `E2E60077` | `EDF10C6B` | `C122AAED 41545EE0 3EF6AC99` | `3EE147AE` |

The radius is `0.4399999976158142` in both records. Zanzibar's center is `(-10.166729927062988, 13.273162841796875, 0.4817855656147003)`. Its physics position is `(-10.165480613708496, 13.262889862060547, 0.12192559242248535)` and differs from the cached center.

`tests/fixtures/retail_ivory_player.reference` and `retail_zanzibar_player.reference` are version3 and retain a mandatory `owner_sphere` after the existing physics `root` field. `ReferencePlayerState::owner_sphere` is optional solely for backwards compatibility with versions1/2; those earlier versions provide no bounds evidence. The parser rejects nonfinite centers/radius, nonpositive radius, missing fields and trailing fields. A rendering diagnostic that requires owner coverage must reject an absent sphere instead of deriving it from the reference physics position.

The untouched replay reads the already retained `root-unit.bin` plus `unit-root-node.bin` for Ivory Tower and the complete `zanzibar-player20-unit.bin` for Zanzibar. Original process state, assets and these retained exports are unchanged. These are the paused original records' values, not a claim that an arbitrary original or native simulation tick has the same pose. The existing reference fixtures independently identify the original frame and rendering inputs.

Evidence: `tools/audit_reference_player_sphere.py`, `analysis/multiplayer-shading/reference-player-spheres.json`, `tests/fixtures/retail_player_owner_spheres.tsv`, and `tests/reference_player_tests.cpp`. The bounded native test verifies every frozen sphere word and the two reference identities, plus version1/2 compatibility and rejection cases.
