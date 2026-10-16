# TU1.5 fixed-turret integration

English retail TU1.5 is the sole physics target. Full players/vehicles/objects
parity remains active; these finite cases do not establish it.

Both fixed turret families now have visible LAN firing integration. Human v426
passes authored800-speed bullet motion, colored original trail triangles in
both views,303 exact UDP states, original vehicle camera, aim, boarding and
exit. Covenant v424 also includes197 original trail draws plus182 MGS2 glow
draws per view, with25-speed bolts and303 exact UDP states. See the stronger
[contrail source-input and GPU packet evidence](MP_SOURCE_CONTRAIL_MULTIPLAYER_TU15_20261015.md).

Human turret target damage remains open: v419's selected target was outside
the original gun's observed turn, and every forward candidate in v420 was BSP
occluded. Those red verdicts are retained. No target vitality, hit or impulse
was supplied to make them pass.

The original15CEF0 dispatch sends class6/7 netgame rows to15CDF0 with the
primary/secondary turret settings at options+20E/+20F (parameters81/82).
Choices1..4 load exact mulg runtime vehicle references+3C,+4C,+44,+54 and
write variant0. Choice0 retains the collection and6 returns NONE. The native
settings adapter previously rejected81/82, and cold render templates retained
the collection regardless of those choices. Both independent settings now
prepare the matching original definition/variant before the source factory
owns the actual birth, controller and physical state. Missing original
references remain NONE. Random5 still requires its per-birth template owner;
it is explicitly unresolved and is not replaced by a guessed vehicle.

`tools/oracle_turret_vehicle_choices_tu15.py` executes unchanged original15CDF0
in252 bounded cases over18 owned multiplayer maps, across5,712 instructions.
Its explicit globals/tag-table/stack bindings are selector fixtures, not a
retail game capture. `tests/turret_vehicle_choices_tu15_tests.cpp` passes all252
native comparisons, including151 NONE results and168 independent setting,
matching-template and source-authority admissions. The original XBE SHA256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Evidence lives under `analysis/multiplayer/physics-live-tu15-20261012/`.
The actual visible `human-fixed-turret-live-v415` run is **red**: original
boarding/exit, turret WORLD node motion0.594584rad, source vehicle-camera
response0.5999999rad, both-view body/turret geometry and302 exact UDP states
pass, but the original accepted firing-effect gate sees zero events despite40
SDL fire samples. The immutable v415 acceptance ELF SHA256 is
`b5c2f099fdeb086914a7e24b4e4e774a0b69ebcb31ecf188368c388401739ca1`.
The inspected screenshot and unaltered10.8-second window video are retained.

`human-fixed-turret-diagnostic-live-v416` also stays red with303 exact UDP
states. Its read-only authority trace proves13 fire-command ticks reach the
actual turret trigger as1.0, with the actual attached authored weapon retained.
Five real108A70 firing requests use that turret/weapon; their actual source
return is NONE. The accepted presentation journal therefore cannot prove
firing. Projectile motion/rendering and the missing presentation still need
verification; no effect instance, projectile or successful result is supplied.

The actual1CC8D0 world capture matches unchanged original execution across
25,162 instructions and all60,084,310 final source bytes and compared CPU/FP
state, within the previously declared host interrupt-flag boundary. B8F40
reaches the unchanged original D3D resource query and then Xbox kernel wait
import800000EA after50,539 instructions. Its full original return comparison
is unresolved; no kernel result is substituted.

The actual14FEC0 player capture exposed a separate x87C1 defect: all retained
memory and the remaining compared CPU outputs match, but the old native status
is544 versus original32. The saved-input native replay independently reproduces
it. Original30BD0 ends in valid-stack FSTP ST(i) operations that clearC1; six
compiled copies had preserved it. Four authenticated register stores are
corrected in each copy and the generators retain the correction. See
`normalize-register-c1-authentication-v417.json`. The post-fix saved-input replay now passes all60,084,310 final bytes and every
compared GP/SSE/x87/FP output against unchanged original execution. The existing
host IF boundary remains the sole EFLAGS exclusion; C1 is not masked.
All four focused CTests pass: turret, light/heavy vehicle choices and the saved
live scheduler. Fresh visible Covenant runs also retain the correction.

The replay retains raw instruction notifications. Four authenticated REP sites
emit one extra native entry notification before their per-iteration callbacks;
the12 additional notifications are recorded explicitly. Their ordered paths
otherwise match. This diagnostic normalization never skips an original
instruction or changes a CPU/memory comparison. See
`human-turret-player-native-before-fix-v416.json` and its raw native trace.

The production v415 executable preserves the turret choice wiring at
`tested-runtime-v415/halo2_native`, SHA256
`48319f78882f64784c24377aeb67de3be0c4ff06f0e690ecac5672486c61c73d`.
The dedicated launcher still selects its earlier tested v381 executable while
the new repair and visible delivery checks are in progress. Canonical checkout
changes from other chats are not covered by this coherent-cut evidence.

## Visible Covenant firing and original continuation

`covenant-fixed-turret-live-v416` passes boarding/aim/exit and302 exact UDP
comparisons, but retains the earlier proxy firing gate as red. Its complete
B8F40 and1CC8D0 captures both match unchanged original execution across51,738
and29,585 instructions, all60MB and compared CPU/FP outputs each.

`covenant-fixed-turret-projectiles-live-v417` proves two actual authored bolts,
25.000002 peak speed and43.333244 travel, with306 exact UDP comparisons. Its
mesh counter was wrong: the callback's tick argument shifted the owner and
definition IDs. Its bolt also has no model; the original tag attaches `cont`,
`MGS2` and `ligh`. The actual recorded video shows its authored glow. The old
run remains red. No production renderer change was needed for that glow.

The corrected v418 observer uses the actual owner/definition fields and counts
real nonempty MGS2 submissions on the same full projectile generations. Its
stronger firing gate requires actual source projectile motion and an authored
model or glow draw in both views, independently of the muzzle-effect journal.
`covenant-fixed-turret-rendered-live-v418` passes:302 exact UDP states, one
observed full bolt generation, speed25,56.666569 travel,112 glow submissions per
view, normal entry/exit, actual WORLD node aim and the exact original vehicle
camera. Muzzle-effect journal count remains0 and is reported separately.
At that historical checkpoint production physics was v417; v418 repaired test observations only. Current production delivery is v427.

The human turret's faster bullets are now aimed at an existing authored Warthog
through ordinary mouse/fire input, with actual source target vitality/impulses
and both-view target geometry as the proposed acceptance signal. No hit,
damage, force, health, object pose or projectile result is supplied.

The broader inventory (`all-original-vehicle-classes-v414.json`, reproducible
with `tools/audit_vehicle_classes_tu15.py`) covers538 definitions in37 original
owned maps with0 decode failures. Five class3 aircraft/throne definitions and
the Creep exist only in campaign maps. The attempted campaign map through the
multiplayer prepared-world loader is red before gameplay due to absent `mulg`
multiplayer globals. Proper campaign session/controller integration remains
required; no multiplayer globals are fabricated to bypass that boundary.
