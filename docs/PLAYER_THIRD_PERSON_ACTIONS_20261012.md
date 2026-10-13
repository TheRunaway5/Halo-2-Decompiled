# Third-person aim, weapon stance and melee, 2026-10-12

All changes and builds use `/home/eric/Developer/Halo-2-Decompiled`.

The native body composer now applies both authored aim screens, followed by the
weapon grip. Previously it applied only upper aim and placed grip before aim.
`114BD0` publishes the upper/lower screen activities; `1DE920` resolves inherited
screens and `2800F0` maps their angle grids. The complete retained `113BC0`
composes upper, lower, then grip (`11447B..114568`). Zero screen step angles now
follow `2800F0`'s epsilon guard instead of disabling the gait presenter.

Pose action lookup now exhausts overlay rows before trying ordinary action rows,
matching `1CF4C0 -> 1CF3C0 / 1CF360`. Full-body primary actions can therefore reach
the body presenter. Melee activity changes also drive the existing saved-local
transition owner. Render observations retain the actual composed primary clip
and frame when split views reuse a pose.

The SMG's actual `rifle,smg` identifiers retain the graph's species aliases:

| Lockout body | Base class and idle index | SMG grip index |
| --- | --- | --- |
| Spartan | rifle, 185 | 213 |
| Elite | pistol, 156 | 201 |

The regression checks both complete aim-screen compositions and these bindings;
it does not replace the authored stance based on appearance alone.

## Verification

Artifacts are in `analysis/player-animations-20261012/actions-flicker/`.

- Root `halo2_native` and both GPU probes build successfully. Both launchers
  report the root `build/halo2_native` runtime.
  Runtime SHA-256: `1e4339b7ae5fa316f7de9000eed464802a7d4b7d51a88fd4fdcbd08b86a94374`.
- Five focused CTest checks pass: retail animation selection/resolution, body
  overlay arithmetic, disc player animations and frame vertex reuse.
- Explicit locomotion runs pass against Base, TU1.1 and TU1.5 operands. Coverage
  includes standing/crouched turns in both directions, source binding and
  complete SMG aim/grip composition, directional cycles, jump and landing.
- The real-cache melee test passes ordinary admission, no early damage and
  impact-event damage, plus eight sword special cases: both species, both
  stances, ground and air. These use actual pickup and fire admission.
- Actual SDL/Vulkan two-view action probes pass for Spartan and Elite. They
  verify changed pitch/yaw node banks, advancing left/right turns, retained
  sword pickup, ground/air/crouched specials, three admitted ordinary strikes,
  full-body primary replacement and bounded submitted body geometry.
- The four-view local and two-view Elite UDP probes pass existing walk, all
  four sneak directions, stance height, jump/landing/recovery, aim, fire/reload,
  moving grenade and ordinary melee checks.
- Skinning comparison passes 131,072 synthetic records and 3,890,853 complete
  posed vertex records from 403 models across 21 maps. The optional map-pack
  assets are read from the existing cache-reset backup through a temporary
  fixture; root assets are not restored or changed. Root CTest's broader skin
  invocation still needs its removed map-pack path; the TU1.1 CTest invocation
  skips its removed executable path. Explicit backup-path runs pass.

These are native host/GPU checks, not physical Xbox framebuffer proof. Targeted
sword lunge admission/motion and complete reload-interruption state remain the
separate boundaries documented in `PLAYER_ANIMATION_HANDOFF_20261012.md`.

## Triangle-flash investigation

The user reports flashes even alone, while walking. The solo probe exercises
walking with fixed camera, stationary camera rotation and combined movement.
One run submitted 1,749 bounded first-person draws over 601 simulation ticks.
No geometry-range or upload-consistency assertion fired. A second solo run is
recorded in `solo-walk.mp4` for visual inspection.
Its contact sheet and twelve largest short-lived image differences were
inspected; no isolated narrow triangle flash was identified in those samples.

`HALO2_VERIFY_FRAME_GEOMETRY=1` now checks grouped dynamic input for incomplete
triangles, nonfinite positions and mismatched draw identities. The existing
`HALO2_VERIFY_BORROWED_FRAME_VERTICES=1` independently packs an owned copy and
compares every byte with the borrowed upload immediately before GPU upload.
Both checks were enabled for all five GPU acceptance runs.

Whole-GPU VRAM samples across the solo/local/LAN sequence range from 2,780 to
3,545 MiB; the solo resident plateau is approximately 3,402–3,419 MiB and drops
after teardown. This short, whole-GPU measurement does not establish or exclude
a longer-term per-process leak. The reported visual flash has no confirmed
cause or fix yet; clean CPU/upload checks do not rule out shader or raster
behavior.

Example diagnostic run:

```sh
SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy \
HALO2_VERIFY_FRAME_GEOMETRY=1 HALO2_VERIFY_BORROWED_FRAME_VERTICES=1 \
xvfb-run --server-num=193 -s '-screen 0 1280x720x24' \
build/third_person_actions_runtime_tests assets/disc/maps/lockout.map \
assets/disc/maps assets/disc/default.xbe --geometry
```
