# Third-person jump and landing playback, 2026-10-12

The native body presenter selects E5700's authored `airborne` activity while
unsupported, then E5A80's admitted soft/hard landing activity on contact, and
returns to ordinary idle or directional movement after recovery. The existing
saved-local transition blends the composed poses; grip and aim remain applied.

A floor-probe contact could mark the capsule grounded without publishing its
incoming downward speed. The landing owner then saw no impact and never
started recovery, even on the next tick, because support was already retained.
`CollisionWorld::move` now records that speed for both native contact paths.
This changes contact presentation delivery, without adding animation durations
or changing the native floor-probe tolerance.

The new real-cache regression reproduced the missing animation before the fix.
It exercises the native floor support probe for both species at soft and hard
impact speeds, compares admission/duration with E5960's authored operands, and
feeds the result into the actual body channel owner. Both TU1.1 and TU1.5 runs
pass.

All 84 species/weapon/dual/stance combinations now verify changing airborne
poses through ascent/descent, changing admitted soft/hard landing poses, and
recovery back to idle. Existing directional walking, sneaking, overlays and
footstep checks continue to pass. The original landing arithmetic differential
passes 3,840 cases; the native systems physics/AI/combat/script/audio test passes.

The actual multiplayer biped tags supply a soft-recovery duration operand of
0.6 seconds and a hard-recovery operand of zero, with impact thresholds 1.5/7/12.
E5960 scales the retained timer by impact severity. These values remain intact.
The hard-landing clip test supplies an explicitly admitted recovery state; it
does not claim the unchanged multiplayer tags admit a nonzero hard recovery.

The Vulkan probe drives an actual SDL jump and observes the submitted body node
bank. It requires advancing airborne frames and leg rotations, advancing
landing frames and leg rotations, and return to standing idle. Its leg signal
excludes world translation and root yaw, so the physical jump alone cannot
satisfy the animation checks. Logs and posed traces use
`analysis/player-animations-20261012/jump-{spartan,elite,lan}.{log,tsv}`.

All three actual four-view Vulkan runs pass with Lockout and TU1.5:

| Run | Airborne frames / distinct leg poses | Landing frames / distinct leg poses | Returned to idle |
| --- | ---: | ---: | --- |
| Spartan | 41 / 67 | 4 / 7 | Yes |
| Elite | 42 / 75 | 4 / 8 | Yes |
| Spartan, native UDP loopback | 42 / 85 | 4 / 9 | Yes |

The root `halo2_native` and affected test targets build successfully. Both
launchers select this root `build/halo2_native` runtime.

This verification covers the native mover/body presenter. Alternate authoritative
source-object poses, retail Havok contacts, physical Xbox framebuffer parity,
and physical-network behavior remain separate.
