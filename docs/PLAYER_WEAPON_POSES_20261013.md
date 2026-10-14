# Player weapon poses, 2026-10-13

All edits, diagnostics and builds are in
`/home/eric/Developer/Halo-2-Decompiled`. The SSD checkout was not modified.

The audit covers all 15 multiplayer held weapons on Spartan and Elite:
Sentinel Beam, Beam Rifle, Battle Rifle, SMG, Rocket Launcher, Energy Sword,
Magnum, Needler, Plasma Pistol, Brute Shot, Brute Plasma Rifle, Plasma Rifle,
Shotgun, Sniper Rifle and Covenant Carbine. Species-specific class aliases
remain authoritative. In particular, the Elite SMG and plasma rifles use the
authored pistol family; they are not forced into the Spartan rifle stance.

## Corrections and source evidence

The complete unchanged TU1.1 `113830` and `114BD0` routines establish the order
and masks. Diagnostic disassemblies and baseline packets are retained under
`analysis/weapon-poses-20261013/`.

- `113928..113A1A` composes action channels before dual offsets, aim and grip.
  The native presenter previously composed actions last, allowing fire/reload
  to overwrite the authored weapon pose. The new regression failed first on
  Spartan Sentinel Beam `fire_1` before this correction.
- `114DA6..114E1B` selects each held weapon's `dual_screen` and grip separately.
  `113A22..113AC6` applies the dual offsets before aim. Installation flags
  `400` and `800` select the graph's `+7C` and `+5C` masks (`1CA950`).
  After correcting action order, the regression exposed the missing SMG dual
  offset. Both hands now use their own weapon's binding, including mixed pairs.
- `114BD0` selects aim activities from the actual primary state. A primary
  action outside that state switch no longer acquires an unrelated idle aim
  screen. Raised/lowered screens retain complementary weights before grip.
- `112D50` bit4 gates the body aim slot independently of hand IK. The native
  body now uses the selected primary/secondary animation flags and the
  selected executable's `.267`/`.1335` transition operands through the existing
  `1DCC30` counter implementation. Elite SMG reload authors flags20 and releases
  aim; Spartan SMG reload flags0 retains it.
- `1112F0`/`111590`/`CD660` gate support-hand IK using the same actual channels
  as rendering, including melee, grenade, ready and put-away actions.
  `11443E..114493` also restricts the final grip to the right-hand mask when
  that IK blend reaches zero. Released support-hand fingers can therefore
  follow the reload/throw instead of being overwritten by the held grip.

Missing authored channels remain absent. For example, the multiplayer dual
bindings have a `dual_screen` but no separate grip, and the Elite SMG has no
authored support-hand IK constraint. The all-weapon marker audit found named
child attachment on the Sentinel Beam; all six dual-capable weapons use the
unnamed child attachment on both hands. No replacement hand positions or
weapon-specific pose angles were invented.

## Verification

`native_authored_weapon_poses` covers both species, standing/crouching,
idle/moving, and single/dual stances: 168 combinations, 5,328 endpoint/weighted
pose comparisons, 2,418 authored action samples, and 288 mixed dual samples.
Expected screen endpoints and hand masks are read from the original tags;
the test composes their source order independently of the production composer.
The current tested on-foot action bindings are masked overlays, not type0
full-body actions; the zero full-body counter is retained in test output.

The test passes on all 12 disc multiplayer maps, and on Lockout using Base,
TU1.1 and TU1.5 executable operands. Seven focused CTest checks pass, including
original animation selection/resolution, overlay arithmetic, hand IK, body
locomotion and the new all-weapon composition test.

Actual two-view SDL/Vulkan probes pass for both species with geometry-range
and independent borrowed-upload checks enabled. They exercise aim, turns,
SMG reload, sword pickup, ground/air/crouch special melee and three ordinary
strikes. Reload poses are compared with the selected channels after their
aim/IK transitions settle: 15 Spartan and 14 Elite samples. The Spartan also
passes 169 support-hand marker checks; the Elite correctly has zero authored
SMG support-hand constraints.

```sh
cmake --build build --target native_weapon_pose_tests native_locomotion_tests \
  native_player_overlay_tests third_person_actions_runtime_tests halo2_native -j 6
ctest --test-dir build -R '^(native_authored_weapon_poses|native_player_body_overlays|native_disc_player_animations|retail_unit_animation_selection|retail_unit_animation_resolution|retail_frontend_model_ik|first_person_marker_ik)$' --output-on-failure
```

Both root launchers report the root `build/halo2_native`. Tested runtime SHA-256:
`f65098675bc1718a44a1358fa7ff25acb6ce68a2a16a80cb7e6c09dbca2b755f`.

This establishes authored native pose composition and the tested live paths,
not framebuffer parity with a physical Xbox. The lowered-weapon state remains
an explicit pose input; the full original extension+88 state producer is not
published by native snapshots. The separately reported triangle flash has no
confirmed reproduction or fix; see `TRIANGLE_FLASH_INVESTIGATION_20261013.md`.
