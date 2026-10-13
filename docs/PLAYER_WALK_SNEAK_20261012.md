# Walk and third-person sneaking verification, 2026-10-12

The native multiplayer presenter uses the real cache's directional standing and
`crouch` weapon actions. Sneaking now follows the capsule's accepted crouch
amount instead of the held key. Releasing sneak preserves the body stance until
stand-up completes, including when a low ceiling prevents standing.

This is an explicit native mover presentation policy. It does not reconstruct
the retail physical-state producer. Existing signed inherited graph bindings,
variant selection, animation clocks, aim/grip overlays and saved-local
transitions remain the owners of their respective animation operations.

## Regression and playback coverage

`build/multiplayer_tests --crouch` reproduced the old defect with
`Releasing sneak made the third-person stance stand before its capsule`.
After the fix, both ordinary release and a real native collision ceiling pass.
The same cases run in the normal multiplayer test, which also passes its
combat/respawn and UDP membership/snapshot checks.

`native_locomotion_tests` now checks all four directions for each of the 84
species/weapon/dual/stance combinations: 336 directional cycles. Each must
advance, loop and change authored thigh/calf/foot locals. Both TU1.1 and TU1.5
operand runs pass, alongside the existing 154 original footstep selections,
126 codec8 decoder comparisons, native grounded contacts and SDL sound output.

The GPU probe now exposes the actual third-person skinning node bank to its
read-only observer. It drives SDL sneak, four directional movement segments and
release, and verifies:

- Authored crouched idle and directional clips reach body submission.
- Head-to-foot height is lower than standing, after the stance transition.
- Leg rotations change independently of world translation and root yaw.
- The standing idle returns after release.
- Existing run, jump, landing, aim, fire/reload, moving grenade and melee
  presentation checks continue to pass.

Actual four-view Vulkan runs with Lockout and TU1.5:

| Run | Standing head-to-foot height | Sneaking height | Submitted poses |
| --- | ---: | ---: | ---: |
| Spartan | 0.487078 | 0.304714 | 8,080 |
| Elite | 0.454958 | 0.212139 | 8,528 |
| Spartan, native UDP loopback | 0.487067 | 0.304648 | 10,528 |

Heights are measured from the actual submitted node bank in native world units;
these are observations, not added animation tuning constants. Logs and pose
traces are in `analysis/player-animations-20261012/sneak-{spartan,elite,lan}.{log,tsv}`.

## Reproduction

Build `halo2_native`, `multiplayer_tests`, `native_locomotion_tests`, and
`multiplayer_animation_runtime_tests` in the root `build` directory. Run the
GPU probe with `MAP SHARED_MAPS UPDATE_XBE --split4`; add `--elite` for the Elite
case or `--lan` for an actual local UDP host/client. The probe needs an X11
display, including Xvfb, and a working Vulkan device. `HALO2_ANIMATION_TRACE`
selects the output trace file.

Both project launchers resolve `build/halo2_native`; no launcher replacement is
needed for this root build. Asset selection remains managed by the launcher.

These checks cover the default native mover/body presenter, including split
views and native UDP snapshots. The alternate authoritative source-object pose
path, physical Xbox framebuffer parity and physical-network behavior are not
established by these runs.
