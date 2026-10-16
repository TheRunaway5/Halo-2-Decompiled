# TU1.5 class3 physics integration

The shared live dispatcher now owns the reached class3 vehicle helpers and
the original static contact-agent initializer. All five definitions respond to
declared controller input in the same immutable runtime, using active authored
placements. The earlier intro-cinematic Pelican case remains inactive; its
cinematic/script context is not established. This is not completed physics
parity or visible campaign multiplayer acceptance.

The sole executable reference is English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Evidence below lives in `analysis/multiplayer/physics-live-tu15-20261012`.

## Live source ownership

- Actual `F2340` dispatches tag class3 to complete `F48C0`. No controller math
  changed: stripping the new read-only diagnostics recovers the exact sealed
  v427 vehicle runtime and motion modules (`class3-vehicle-body-preservation-v447.json`).
- New `source_vehicle_auxiliary_tu15` lowers seven complete original bodies,
  466 instruction sites: `1DA710`, `F85C0`, `288E70`, `305140`, `383130`,
  `30BAE0`, and `383B30`. Their real allocator, collision, effect, and animation
  callees retain their full ABI. The generator authenticates original bytes,
  body lengths, call targets, and the original initializer table entry.
- `_cinit` table slot `46D768` names `383130`, which calls `30BAE0` to construct
  the static agent at `48CDC4`. Its original vtable is `41D638`. Skipping that
  initializer caused the Phantom's real world solve to call through a zero
  vtable. Both source trees now execute the initializer before map physics and
  retain its original `383B30` exit callback through the native CRT boundary.
- `set_unit_controller_active` on the shared world/object runtime calls complete
  `CE440`. The original actor/player ownership and control-exclusion predicates
  remain in charge. Merely calling `C92C0` on an inactive actorless unit resulted
  in its throttle being cleared during the unit update. No physical flag,
  velocity, force, contact, solver result, or post-step transform is supplied.
- The isolated source cut additionally retains complete campaign birth/animation
  callbacks `F0E70`, `C6310`, `285A70`, and `290710`, already owned in the root.
  Its campaign option binding delivers actual kind1/difficulty fields. It does
  **not** adopt the root's complete campaign AI/script startup or prove that work.

## Controller input and complete-call evidence

`class3-causal-matrix-v449/report.json` uses immutable probe ELF
`16d90b2f84e996ff9fddf57ada54efc2f9337b007622fe741edbc4888cc44e9e`.
Each pair independently constructs the original world, two real source players,
and the actual authored vehicle placement. Initial observer positions come from
clear cached BSP floors with valid source interior locations. Original `CE440`
admits the declared external controller; original `C93D0`/`C92C0` deliver input.
The first 30 neutral ticks match exactly. Only the vehicle throttle changes for
the following 60 ticks; both player observers remain neutral throughout.

| Definition | Maximum commanded/neutral position difference | Result |
| --- | ---: | --- |
| Enforcer | 0.665648 | Pass |
| Gravity Throne | 1.806993 | Pass |
| Gravity Throne, no flare | 1.752054 | Pass |
| Phantom | 7.658406 | Pass |
| Pelican, New Mombasa placement 3 | 7.807310 | Pass |

The Pelican's earlier missing component was traced to loading the wrong BSP.
Read-only geometry survey `oldmombasa-pelican-spatial-survey-v433` locates its
placement in intro cinematic BSP 3. Loading BSP 3 lets original construction
produce the component; this does not establish its cinematic/AI activation path.
The separate authored New Mombasa placement 3 responds without modifying that
intro placement. The old v447 matrix remains red and retained.

Unchanged original i386 execution matches all accessible source memory and
GP/SSE/x87/FP state for these actual calls:

| Vehicle | Calls | Executed original instructions |
| --- | --- | ---: |
| Enforcer | CE440 admission, F48C0 controller, F2340 update, B8F40 objects, 1CC8D0 world | 115824 |
| Gravity Throne | F48C0 controller, 1CC8D0 world | 52541 |
| No-flare Gravity Throne | F48C0 controller, 1CC8D0 world | 75386 |
| Phantom | F48C0 controller, F2340 update, 1CC8D0 world | 54725 |
| Pelican | F48C0 controller, F2340 update, 1CC8D0 world | 106933 |

Reports are the corresponding `enforcer-*-original-v444`,
`gravity-throne-*-original-v447`, `gravity-throne-no-flare-*-original-v447`,
`phantom-*-original-v447`, and `pelican-*-original-v448` directories. These are host i386 comparisons, not
Xbox hardware trajectories or pixels. Only the host-fixed interrupt flag is
masked. Native instructions, callees, and physical outputs are not replaced.

## Visible multiplayer checkpoint and remaining work

The final rebuilt acceptance ELF is
`68ee1b6afcf5540ddc610591e76639cf3aaadb1fe09e47f17e18d5b0a54b92c6`.
Its `ghost-static-agent-live-v448`, `lift-static-agent-live-v448`, and
`objects-static-agent-live-v448` runs pass 665 exact UDP observations. The Ghost
travels 8.906328 units with actual original vehicle-camera response 0.600000
radians, boarding, steering, and exit. The lift reaches 3.964367 upward velocity;
object recreation/blast/debris passes. Continuous live desktop mirrors and
stable executed ELFs are retained alongside each `window.mp4`. Fresh Ghost
object/world calls match 109,465 unchanged original instructions. This is
bounded stock multiplayer regression, not visible class3 multiplayer proof.

The normal v448 production preview passes 120 rendered frames, 357 source ticks,
and two local players with authored spawns and no physics fixture inputs.
`Halo2-Physics-TU15` now selects that tested immutable runtime. See
[normal delivery](MP_PHYSICS_RUNTIME_DELIVERY_TU15_20261015.md).

Earlier wrong-BSP, invalid observer-floor, missing-owner, unadmitted-input,
stationary Pelican, and interrupted display trials remain retained. In
particular, v441's finite/moving body gate admitted gravity settling; the later
paired input matrix is required to establish controller response.

Required next gates include visible class3 multiplayer integration, the actual
Pelican cinematic/AI control context, remaining carrier contexts, wider physics
branch/lifetime coverage, and quantitative OG Xbox TU1.5 reference validation.
The all-player/vehicle/object goal remains active.

## Static-agent authority continuation and occupied-vehicle restoration

The earlier restore regression was real: `class3-checkpoint-red-v450.log`
rejects the initializer's original `383B30` exit callback. Both world owners now
admit it and require exactly one registration plus the constructed `41D638`
vtable at `48CDC4`. Missing, duplicated and corrupt static-agent bindings are
rejected without publishing any target state. `class3-checkpoint-v450.log`
passes a complete source-world handoff, sixty later exact source ticks, later
model allocation and biped birth/retirement, repeated transfer and recovery.
The malformed-version case now increments the actual schema version, covering
both the root's version2 and isolated source cut's version1.

`ghost-authority-restore-live-v453/lan-vehicle` passes491 exact UDP snapshots.
At authority tick593, while a player is seated in a moving Ghost, the actual
coupled Match/world transaction preserves the complete source bank, clock,
source player bindings and published participant identities, seats, positions
and velocities. Ordinary controls continue through8.740213 units of Ghost
travel, steering and exit. The original vehicle camera is submitted for278
seated frames, turns0.600000 radians and returns to the ordinary on-foot camera
for241 frames. The stable executed acceptance ELF is
`21918cf70fdfc023912b242992b8912d2336e4e5cf9ec9168255df0b9825da5e`.
Its continuous desktop mirror has no errors. Actual video frames at5seconds
(vehicle camera) and8.95seconds (after exit) were inspected.

The first complete post-restore object/world calls match68,496/52,065 unchanged
original instructions, respectively:120,561 total, with60,084,310 accessible
memory bytes and GP/SSE/x87/FP state compared per call. Reports are
`ghost-post-restore-{object,world}-original-v453`. This remains host i386 proof,
with only the fixed interrupt flag masked, rather than Xbox hardware proof.

The v452 and v452b visible trials remain red. Their client-clock finite input
schedule could consume its driving/exit interval while the authority was busy
restoring/exporting/capturing. V453 schedules this restore stimulus by completed
authority ticks, using the existing passenger/blast fixture policy. No physical
output or acceptance travel threshold changed. V451's earlier live restore
also passed302 UDP snapshots; the final v453 case additionally retains complete
post-restore original-call comparisons.

Production `Halo2-Physics-TU15` now selects immutable v451,
`c3d25c01fad7dbf63d34076168bac78c5bae27bc4714de8b275d3d2de23e5a6f`.
It passes normal120-frame/357-tick/two-player startup and explicit review of
recorded gameplay at106seconds, with authored starts and no physics fixture
inputs. The source delta fromv427 throughv453 retains the class3 integration
and restore fix. Earlier compacted fixtures remain exactly reconstructable
from the verified parent source archive chain. Visible class3 multiplayer and
the other full-parity gates above remain open.
