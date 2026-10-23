# Turret functionality and TU1.5 parity

Scope requested 2026-10-22: stationary human/Covenant turret variants,
Warthog machine gun and Gauss turrets, and Scorpion cannon/coaxial weapons.
Status: **in progress; full TU1.5 parity is not certified**.

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

The [unchanged TU1.5 selector](../analysis/turrets-tu15-20261022/effect-original-selector.json)
executes `17AAB0` ten times from the pinned English XBE
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It returns RNG word zero for each original tuple and word one for each donor.
Its tag table, stack and RNG addresses are explicit component inputs.

The component regression fails before the repair and passes afterward. It
checks exact bytes, selected/advanced RNG words, repeated admission, custom
content guards and truncated-input rejection. This does not establish visible
effect equivalence, spatial sound or a played match.

## Completion matrix

| Family | Seat/aim/exit and firing | Target damage | Effects and sound | Complete TU1.5 acceptance |
|---|---|---|---|---|
| Human AP fixed turret | Current cohort pending | Current cohort pending | Named inputs checked; runtime open | [ ] |
| Covenant AP fixed turret | Current cohort pending | Current cohort pending | Named inputs checked; runtime open | [ ] |
| Human MP/deployable item variants | Inventory only | Open | Open | [ ] |
| Plasma cannon/deployable/handheld variants | Inventory only | Open | Firing RNG fixed; runtime open | [ ] |
| Warthog MG | Current cohort pending | Current cohort pending | Sound delivery under investigation | [ ] |
| Warthog Gauss | Current cohort pending | Current cohort pending | Firing RNG fixed; sound/visual open | [ ] |
| Scorpion campaign/MP cannon | Current MP cohort pending | Current cohort pending | Explosion RNG fixed; sound/visual open | [ ] |
| Scorpion coaxial MG | Current cohort pending | Current cohort pending | Sound delivery under investigation | [ ] |

Remaining acceptance includes original aim limits and convergence, firing cadence,
spin-up/down, heat/overheat/recovery where authored, sustained/repeated triggers,
release, both Scorpion inputs, player/vehicle/material contacts, direct/splash
damage and allegiance, boarding/exit, destruction/ejection, moving chassis,
single/split/LAN/remote ownership, all map/variant applicability, actual audio
output and TU1.5 reference frames. Random turret settings retain the known
unresolved per-birth template owner; absent original selector references remain
NONE. No row is closed from constructor counts or metadata alone.

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
