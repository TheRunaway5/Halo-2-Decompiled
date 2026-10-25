# Turret functionality and TU1.5 parity

Scope requested 2026-10-22: stationary human/Covenant turret variants,
Warthog machine gun and Gauss turrets, and Scorpion cannon/coaxial weapons.
Status: **six multiplayer weapon paths pass controlled gameplay and presentation
submission acceptance; full TU1.5 parity is not certified**.

## Reference and input coverage

The [read-only census](../analysis/turrets-tu15-20261022/contracts-v2.json)
compares 38 hash-pinned original map inputs with their prepared MCC counterparts.
It contains 794 platform/definition contexts, covering 12 distinct weapon paths
and eight projectile paths. All have paired resident metadata. Weapon fields
from `weap+12C` onward and declared projectile fields are normalized by reference
identity and StringID name; scalar words remain exact. Predicted-resource
prefetch tables are omitted explicitly. The reachable `effe`/`jpt!` graph check
covers event/part records and damage fields through `jpt!+54`; it does not
compare every particle, sound, light, decal or material payload.

The authored variants include `h_turret_mp_weapon`, `h_turret_mp_item`,
`h_turret_ap`, `plasma_cannon`, `c_turret_mp_item`, `big_needler`,
`big_needler_handheld`, `chaingun_turret`, `gauss_turret`, `warthog_horn`,
`cannon_turret` and `cannon_turret_mp`. Inventory is not evidence that each item
or AI-only variant has completed a gameplay sequence.

## Effect correction

Five additional named effects select the wrong event RNG word in raw MCC data:
Gauss firing, plasma-cannon firing, plasma-rifle overheat shared by a turret
variant, Phantom projectile impact shared by a turret variant, and Scorpion
shell explosion. The production scalar converter restores only their complete
known flag tuples. Unknown names/flag words and other tag groups are preserved.
The [manifest](../tools/mcc-turret-effect-scalar-profiles.json) records the exact
original/MCC words and observed maps.

The [unchanged TU1.5 selector](../analysis/turrets-tu15-20261022/effect-original-selector-v2.json)
executes `17AAB0` ten times from the pinned English XBE
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It returns RNG word zero for each original tuple and word one for each donor.
Its tag table, stack and RNG addresses are explicit component inputs.

The component regression fails before the repair and passes afterward. It
checks exact bytes, selected/advanced RNG words, repeated admission, custom
content guards and truncated-input rejection. This does not establish visible
effect equivalence, spatial sound or a played match.

## Input and presentation corrections

The mouse adapter previously suppressed every seated secondary trigger. It now
admits the actual armed weapon's authored input 1, including the Scorpion coaxial
machine gun. The controller route uses the same armed-seat definition and avoids
treating a carried offhand weapon as a dual-wield context while seated. Existing
vehicle boost delivery remains covered by the input regression.

The real SDL coaxial test then exposed a renderer startup gap: projectile
resources were collected from the primary barrel and charged-weapon views only.
The source can publish the Scorpion's ordinary second barrel, which had no
prepared model/invisible-projectile entry. Preparation now reads every actual
resident authored barrel and deduplicates its projectile definitions. Trigger
admission and original projectile construction remain owned by the source.

Original turret firing sounds belong to the armed unit in the retained parent
hierarchy. The local audio route now resolves that complete hierarchy, including
the Scorpion cannon child, and accepts original world-point requests. Native
effect presenters no longer duplicate those source-owned local turret cues.
The audio cohort checks actual Classic PCM voice resources against original
requests. SDL dummy output does not establish physical output or Xbox waveform
equivalence. Contrail scene construction also has its own feature guard; the
vehicle render test now enables the production contrail producer.

Existing cache conversions already restore the human AP firing-effect reference,
Gauss reporting cause and authenticated map-specific Gauss flyby references.
Raw donor differences in the census must not be described as unconverted runtime
defects. The [actual cache dependency cohort](../analysis/turrets-tu15-20261022/cache-dependencies.json)
passes all 28 Gauss contexts against their exact original maps, including both
null and non-null flyby references and available named reference conversions.
The exact Covenant AP Classic flyby path
`sound\vehicles\phantom\phantom_turr_flyby_c_ap` is absent from the prepared
Coagulation and shared MCC directories; its remastered donor remains a separate,
[unresolved dependency](../analysis/turrets-tu15-20261022/unresolved-classic-flyby.json).

## Completion matrix

| Family | Seat/aim/exit and firing | Target damage | Effects and sound | Complete TU1.5 acceptance |
|---|---|---|---|---|
| Human AP fixed turret | Passed controlled scene | Damage and source kill credit passed | Classic firing voice and colored trail submission passed | [ ] |
| Covenant AP fixed turret | Passed controlled scene | Damage and source kill credit passed | Classic firing voice, trail and bolt-glow submission passed; flyby dependency open | [ ] |
| Human MP/deployable item variants | Inventory only | Open | Open | [ ] |
| Plasma cannon/deployable/handheld variants | Inventory only | Open | Firing RNG fixed; runtime open | [ ] |
| Warthog MG | Passed controlled scene | Damage and source kill credit passed | Classic firing voice and colored trail submission passed | [ ] |
| Warthog Gauss | Passed controlled scene | Damage and source kill credit passed | Firing RNG fixed; Classic voice and trail submission passed | [ ] |
| Scorpion MP cannon | Passed controlled scene | Damage and source kill credit passed | Explosion RNG fixed; Classic voice and trail submission passed | [ ] |
| Scorpion MP coaxial MG | Passed controlled scene | Damage and source kill credit passed | Secondary input/resources fixed; Classic voice and trail submission passed | [ ] |
| Scorpion campaign cannon | Inventory only | Open | Explosion RNG fixed; runtime open | [ ] |

The coherent [gameplay cohort](../analysis/turrets-tu15-20261022/runtime-final/refresh.json)
passes all six cases plus the scalar/cache/input checks. Each case uses normal
original seat admission, a declared terrain-supported target placement, ordinary
look/fire input, source-owned projectile/damage processing and normal exit.
Observed projectile generations were respectively 41, 16, 55, 2, 2 and 60; each
case gained one actual source kill. These counts describe the declared test
inputs, rather than a measured Xbox cadence comparison. The original turret
selector's 252 cases and contrail-state fixture also pass in
[the reference checks](../analysis/turrets-tu15-20261022/original-choice-and-trail/report.json).

The final [presentation cohort](../analysis/turrets-tu15-20261022/presentation-accepted-v7/refresh.json)
passes all six cases plus the scalar/cache/input checks. Every case submits 90
actual Vulkan frames with two local views after 90 natural neutral source ticks
following entry. Real SDL left/right button events drive the selected trigger.
The test validates full live projectile generations, non-remastered PCM resources,
one native voice per original Classic cue, and authored trail geometry with
nonzero color/area in both views. The Covenant AP case also submits authored
bolt-glow geometry. Counts establish delivery for these declared scenes; they
do not compare final Xbox pixels, every muzzle/impact particle, acoustic output,
or original sound-distance/occlusion behavior.

The standard AP, Warthog and Scorpion weapon definitions have zero authored heat
threshold/loss fields. The inventoried handheld Covenant variant has different
nonzero heat values and still requires its own live acceptance. No heat mechanic
is added to the standard turrets based on another Halo game.

Remaining acceptance includes original aim limits and convergence, firing cadence,
spin-up/down, heat/overheat/recovery where authored, sustained/repeated triggers,
release, both Scorpion inputs, player/vehicle/material contacts, direct/splash
damage and allegiance, boarding/exit, destruction/ejection, moving chassis,
single/split/LAN/remote ownership, all map/variant applicability, actual audio
output and TU1.5 reference frames. The later [complete-vehicle investigation](VEHICLES_TU15_GOAL_CHECKLIST.md)
qualifies native Random light/heavy/turret selectors and actual per-birth model
binding, with normal-input human/Covenant fixed-turret runs. Original absent
selector references remain NONE. The newer flying-selector kernel still needs
its current game and launcher validation. No row is closed from constructor
counts or metadata alone.

## Reproduction and storage

All compiler/build/runtime probes use `tools/run_test.py`; CMake and CTest use
`tools/run_tests.py`. The active cohort freezes the whole source before building,
then authenticates each named refresh. Private generated entry notifications
observe original projectile constructor/retirement results without supplying
any return, damage, vitality or readiness result. Target placement is a declared
scene input and the actual source owns seat admission, firing and damage.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python -B tools/audit_turret_contracts_tu15.py \
  --mcc-root PATH --output REPORT.json
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/test_turret_effect_scalar_tu15.py --output REPORT_DIRECTORY
```

The finite command journal under
`analysis/turrets-tu15-20261022/session-v1/` records snapshot preparation,
private unchanged-body build partitions and the official worker commands.
`tools/validate_turret_cohort_tu15.py` refreshes only declared turret files within
that same owner; it rejects undeclared frozen-source drift. Audio runs additionally
use a private display, real SDL inputs and SDL dummy device submissions.
Builds, cache copies and frames are disposable; only bounded reports are retained.
The [checkpoint](../analysis/turrets-tu15-20261022/checkpoint-v1.json) records the
final cohort/source hashes and cleanup verification. No launcher-selected engine
was rebuilt or replaced by this work.
The disposable cohort source/build, copied dependency tree, graphics cache and
frames were removed on runner exit. No large binaries, banks or captures are
retained. Repetitive compiler logs were compacted with their original sizes and
hashes in `compact-log-receipt.json`; source/reference inputs were preserved.
