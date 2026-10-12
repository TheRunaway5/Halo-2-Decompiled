# TU1.5 phantom forces and body velocity delivery

English TU1.5 is the sole parity target. Twelve complete native bodies add
1,314 authenticated instruction sites to the retained vehicle dependency
graph, bringing it to 158 bodies and 16,402 sites. The additions follow the
actual vehicle call to the full phantom updater `1DA750`, its direct physical
dependencies and the original motion vtables' linear/angular velocity setters.
The owned executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

| TU1.5 entry | Sites | Ownership |
| --- | ---: | --- |
| `1DA750` | 982 | Phantom membership iteration/removal, authored force expressions, linear/angular delivery, physical state flags and timer-byte updates. |
| `1DA530` | 134 | Force factor, damping, limit and acceleration expressions; retains the actual CRT power dependency. |
| `1D9080`, `1D9100`, `1D92C0` | 130 | Linear velocity assignment, angular velocity assignment and linear delta delivery through real motion methods. |
| `315C90`, `315CB0` | 28 | Body island-state query and activation dispatch; world activation remains a required owner. |
| `3239F0`, `323A00` | 8 | Original 16-byte linear/angular motion stores. |
| `024570` | 16 | Original x87 vector magnitude. |
| `0E1480`, `11B930` | 16 | Actual biped/object physical state-byte updates. |

`source_run_physics_phantom_force_tu15_runtime` composes these complete native
bodies with the retained vehicle owners. It keeps GP/SSE/x87/flags, FP controls,
the original stack and ordered writes through nested calls. Generated production
code uses static native operations and branches, with no executable bytes,
decoder or original-code execution. Unknown callees preserve their real
required-owner boundary; they supply no successful force or activation result.

The independent corpus contains 528 original TU1.5 procedure cases: 377 complete
calls and 151 required-owner prefixes. These include 193 complete updater calls;
153 reach both motion setters, 81 change linear velocity and 15 change angular
velocity. They compare 195,014 normalized instruction observations, 39,156
ordered writes and 153,663 retained bytes. A test-only i386 harness single-steps
194,838 unchanged original instructions. It supplies capture-only stubs at
unported owner entries, outside the authenticated graph, and no callee returns.
Emulation discovers sparse input footprints; hardware supplies final FP/state
and write results.

Inputs explicitly declare component, body, tag/model/physics, membership,
motion and island backing. Neither a source factory nor a collision producer
created those records in these cases. Original code computes its own forces,
membership removal, flags and velocity stores. Cases cover missing/stale
identities, signed member/body indices, dynamic/static bodies, active/absent
islands, zero/positive/negative coefficients, radial/tangential/angular paths
and all four FP rounding modes. Entry x87 stacks are empty and MMX is inactive;
other entry FP stacks and precision modes are outside this corpus.

1,214 of the 1,314 new sites are exercised. The 100 remaining sites include
deeper activation/allocator and math-dependent branches. 150 prefixes reach
the original CRT power owner `37CE0C`; one reaches world activation `3165D0`.
Empty-membership storage retirement still requires its actual allocator owner.
No host `pow`, allocation success, contact or solver result is substituted.

Evidence and sealed source/build records live under
`analysis/multiplayer/sandbox-source/phantom-force-tu15-20261011/`.
Recovery, generation, original execution and original hardware execution are
independently repeated and compared. Release, ASan/UBSan and normal project
validation are recorded separately from live gameplay evidence.

This adds physical force/velocity ownership; it does not establish a working
live lift, vehicle suspension, solved collision trajectory or authored scenario.
TU1.5 process/world/BSP/body creation, collision production, remaining math and
activation owners, player/vehicle/object integration, ragdolls, split-screen and
LAN physics remain required. The temporary live terrain mover remains present.
See the [owner classification correction](PHYSICS_TU15_OWNER_SCOPE_CORRECTION_20261011.md)
before treating the retained dependency graph's sound/effect routines as physics.
