# Vehicle destruction and explosive interactions

Status: active, 2026-10-22. This ledger covers the user's complete vehicle
destruction and explosive/damageable-material interaction request. A component
test, one destroyed vehicle, or an asset inventory does not complete the goal.

The original Xbox TU1.5 executable and authored Xbox tags are offline behavioral
references. Playable validation uses the prepared, port-owned MCC cache. Preserve
original inputs, historical evidence, selected runtimes, and unrelated work.
Use `tools/run_tests.py` for CMake and `tools/run_test.py` for probes; remove owned
builds, frames and binaries, retaining bounded reports and source hashes.

## Completion matrix

Every row requires the original owner, actual MCC execution, a direct positive
regression and relevant negative controls. Delayed/recursive requests must retain
the source clock, RNG, full object generations, attribution, and source ordering.

| ID | Required behavior | Current evidence / next gate | Complete |
| --- | --- | --- | --- |
| D01 | Inventory every vehicle and damageable environment definition, section, response, material, damage/effect/constraint reference and BSP breakable family | Census and Xbox/MCC comparison in progress; names alone cannot establish destructibility | [ ] |
| D02 | Direct projectile, melee, collision and radial damage select the actual node, material and damage section | Existing source dispatcher and posed collision owners; all destruction producers need current MCC coverage | [ ] |
| D03 | Partial damage thresholds, shield/body coupling, rider/occupant coupling and special-death/skip gates | Preserve original damage-info and response consumers; no replacement health thresholds | [ ] |
| D04 | Every authored vehicle destruction class, including chassis/attached turret and campaign-only vehicles | Eight historical Xbox-asset live cases and original-call comparisons exist; current MCC, remaining classes and branches remain required | [ ] |
| D05 | Pending responses, delayed effects, model-region/permutation changes and subsequent collisions | Host prop tests cover one module's six-tick delay; native TU1.5/MCC path and other responses remain required | [ ] |
| D06 | Destruction damage/effects use current marker/world location, source owner and strength | Original `DC660 -> DC3C0 -> D91D0`; verify real requests rather than reconstructing events from names | [ ] |
| D07 | Explosive-to-explosive chains, multiple simultaneous/delayed blasts and reciprocal vehicle/prop damage | Direct MCC chain regression required; preserve nested request ownership and exactly-once response dispatch | [ ] |
| D08 | Explosives affect nonexplosive damageable scenery, machines, equipment and breakable BSP materials | Inventory and current native producer/consumer coverage required; indestructible and out-of-range controls | [ ] |
| D09 | Blast candidate selection, obstruction, falloff, directional/stochastic branches and physical impulse | Actual original radius/per-target owners; a health reduction does not establish impulse or obstruction | [ ] |
| D10 | Detached parts, constraints, debris births, solved motion, wreck collision, occupant exit/death and object retirement | Historical vehicle/debris evidence is bounded; current MCC path and every authored transition remain required | [ ] |
| D11 | Multiplayer state, late join, replay/takeover and repeated snapshots preserve destruction and chains | Exact source state/identity checks, no duplicate damage/effect/debris, no stale collider | [ ] |
| D12 | Visible damage states, explosions, debris and authored audio reach normal single/split/LAN consumers | Current engine delivery and visual/audio acceptance required; headless tests cannot close this row | [ ] |

## Existing evidence boundaries

- [Eight vehicle cases](MP_VEHICLE_PHYSICS_WORLDS_TU15_20261015.md)
  establish bounded historical driving/destruction/debris behavior. Their Xbox
  cache inputs do not satisfy current MCC runtime acceptance. The Banshee
  object-update original replay also retains its unresolved kernel wait.
- [Prop source research](MP_SANDBOX_EXPLOSIVES_LIFTS_SOURCE_RESEARCH.md)
  identifies the section, pending-response, marker and area-damage owners.
  Its early TU1.1 executable-availability statement is historical; later work
  has the pinned TU1.5 reference.
- `PropSandbox` is a bounded host adapter: it admits crate placements and
  selected death responses. Extending that adapter by guessed vehicle behavior
  would not complete the actual TU1.5 destruction engine.

## Current work

1. Compare original destruction contracts with prepared MCC inputs, retaining
   literal fields and normalized tag/StringID references.
2. Add direct current-source native regressions for vehicle destruction and
   environment chain reactions, using constructor/scene inputs and ordinary
   weapon commands; never supply damage, health, solver or destruction results.
3. Fix demonstrated missing owners or conversion defects from original evidence,
   then run appropriate regressions and normal presentation/replication checks.

## Current-source census, 2026-10-22

The offline [object-contract census](../analysis/destruction-goal-20261022/contracts-v5.json)
reads 39 physical reference map files (including duplicate revisions), retaining
184 distinct named definitions: 37 vehicles/attached turrets, 58 scenery, 40
crates, six machines, 32 bipeds, five creatures, three projectiles, two equipment
and one weapon. Their 158 distinct normalized model damage contracts include
sections, responses, material selections, seats, nodes, constraints, timers and
effect/damage references. No matching MCC object definition is absent. This is
a model census, not proof that every resident tag is placed or damageable.

There are 33 differing map/definition rows: 25 human turret response-flag
changes (`00000000` vs `02000000`), six Hunter section/response differences,
and two large Flood floor-clump response effect differences. The original
consumers and required conversion behavior remain to be established; this
census changes no runtime flag or effect. The v1 comparison is preliminary and
includes unnormalized map-local StringIDs/pointers. Version5 supersedes earlier
input coverage and uses the same prepared import as runtime-v2.

The [placement census](../analysis/destruction-goal-20261022/placements-v2.json)
finds direct scenario or netgame collection context for 122 of these definitions.
It preserves actual row indices, flags and collection membership. AI, scripts,
model children and debris remain separate creation owners; lack of a direct row
does not establish that a definition is unused.

The separate [BSP census](../analysis/destruction-goal-20261022/bsp-breakables-v4.json)
compares 89 scenario BSP pairs and all 2,210 terrain surfaces carrying collision
flag bit3. Their literal surface records match MCC, including the authored
breakable indices. The schema-named `sbsp+BC` arrays are empty in these inputs;
their emptiness must not be mistaken for absence of breakable terrain. Revised
Xbox `highplains2` is compared with MCC's logical `highplains` filename, as in the
existing PC tag-bank audit; actual BSP names/records still compare independently.
These totals include duplicate map inputs and do not certify destruction,
runtime bitmap updates, physical fragments or visible disappearance.

Reproduction (bounded reports only; original caches remain offline inputs):

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python -B tools/audit_destruction_contracts_tu15.py \
  --mcc-root PATH/halo2 --report PATH/contracts.json
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/audit_breakable_materials_tu15.py \
  --contracts PATH/contracts.json --mcc-root PATH/halo2 \
  --report PATH/bsp-breakables.json
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 -B tools/test_destruction_current_cohort.py \
  --mcc-root PATH/halo2 --output PATH/runtime --jobs 4
```

The first full production build passed, including the changed BSP light
collector after an authenticated refresh before that unit compiled. Its two
runtime cases failed before gameplay because the older selected import lacked
`native_shader_library`. No shot, damage or chain assertion executed. Both
its build and scratch were removed. The external expanded-cohort watcher also
executed no case: the source owner's cleanup won its copy race. Its receipt
is retained only as a tooling failure, not a gameplay result.

[Read-only input admission](../analysis/destruction-goal-20261022/prepared-input-v1.json)
now derives all 43 verified map owners from the complete prepared import with
39 native banks. The cohort performs that admission before freezing/building.
Runtime-v2 was cancelled during the turn interruption; its source/build were
removed without executing a case. The active runtime-v4 build covers seven multiplayer vehicle variants, Lockout
modules, Foundation fusion coils, Colossus gas containers, and Zanzibar terrain
glass. These remain unverified until the actual tests finish. Original bodies
are unchanged; the private build partitions the large AI translation unit using
the existing partitioner.

The chain assertion observes the original `DC4A0` owning-object store immediately
before `DC4A4 -> D91D0`, requires a pristine witness before that blast, and checks
one original destruction request from each generation. Scene placement is a
declared test input. No vitality, damage, impulse, response or solver result is
supplied. Terrain coverage requires an ordinary rocket, an actual source write
to the breakable bitmap, and removal from subsequent original collision queries.

## BSP explosion-light visibility connection

`SourceBspEffectLights` initialized a private all-intact visibility bank and
continued using it for secondary BSP light commands, even when drawing from a
live TU1.5 world whose breakage state had changed. The native draw path now takes
an owned copy of the actual current `4F9B50` allocation on each call and passes
those bytes to the existing original material-visibility selector. Missing
source backing fails explicitly. Each read owns all `4204` bytes, so earlier
views cannot alias later source writes and source state is never changed by
presentation. Reference-only calls without a native world retain their declared
initial-state input.

The address/extent are established by actual `186CB0` allocation/publication and
`186D30` activation; radius owner `D91D0` reaches `1870C0`, with `186E70/186ED0`
reading/updating this same bank. This fix changes the presentation readback,
not an original damage predicate, threshold or bit producer.

[Focused regression](../analysis/destruction-goal-20261022/bsp-visibility-component-v1.json)
passes intact/broken/restored terrain and instanced-part command emission,
another-BSP isolation, owned-copy/trailing-byte preservation, and rejection of
NONE/unmapped/short backing. It uses declared component input banks and the
production part-command code; it does **not** prove a naturally admitted
explosion, whole renderer, original pixels or LAN continuation.
[Production syntax validation](../analysis/destruction-goal-20261022/bsp-visibility-production-syntax-v1.json)
also passes, as does focused CMake/CTest and compilation in the full production
build. The authenticated production-source refresh is retained under runtime-v1.
Actual broken-surface visual/lighting regression remains required. No D-row is
closed by these component results.

## Destruction-effect random stream conversion

The [normalized response graph census](../analysis/destruction-goal-20261022/effect-graphs-v3.json)
retains 227 distinct nodes over39 physical map inputs. It records885 differing
map/node rows, including139 damage-field observations. All139 are the known
`jpt!+0C` bit2 difference covered by the existing named area-damage profiles.
MCC-only remastered sound parts are separately filtered by the existing native
`MultiplayerAudio::source_impulse` before variation/voice creation. Those raw
metadata differences alone do not prove an unhandled runtime defect.

Twenty-five named destruction/shield effects have opposite `effe+0` bit2 values
from the original authored records. Actual TU1.5 `17AAB0` reads the definition
through `4F0414`, selects `[4F3CD8]` for bit2 and `[4F3CD8]+4` otherwise. The
[unchanged original selector](../analysis/destruction-goal-20261022/effect-rng-original-selector-v1.json)
confirms all50 original/MCC choices. The previous named scalar converter did
not accept effect tags, and a direct converter/event-RNG regression failed.

The converter now restores only those25 whole named tuples, in both bit
directions, before ordinary Cache consumers. Unknown names, other flag tuples,
and other tag groups remain untouched. The checked generator authenticates the
profile manifest. Its component regression passes the original event-word
choice, exact bytes, repeated admission and custom-content guards. [Cache integration](../analysis/destruction-goal-20261022/effect-cache-green-v1/validation/report.json)
now passes on Coagulation, Lockout, Foundation, Colossus and Zanzibar after
the same test failed against the completed pre-fix Cache archive. The focused
CMake run recompiled the production Cache and converter against unchanged
owned dependency archives; all borrowed hashes stayed unchanged. Both red and
green test/build scratch were removed. Ordinary destruction validation with
this converter change remains pending. The active
runtime-v4 source predates this edit; its completed build will be refreshed with
recorded hashes inside the same bounded owner before qualifying the change.
No D-row is closed by this component result.

## Reciprocal vehicle/prop gate

Two additional Headlong cases are implemented and syntax-checked:
`coil_to_warthog` and `warthog_to_coil`. They select actual source-created
Warthog/fusion-coil identities. The declared setup repositions one coil using
its original root-to-terrain height, places the shooter on supported terrain,
and then uses ordinary SMG or rocket input. The first original initiator
`DC4A0` request must see a pristine witness; subsequent witness damage and an
unchanged distant same-class control are required. No health, damage amount,
impulse or contact result is supplied. These cases are not in runtime-v4's
older executable; their runtime result remains unverified until the recorded
source refresh and next official worker iteration.
