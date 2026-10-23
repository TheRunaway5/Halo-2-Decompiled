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
| D01 | Inventory every vehicle and damageable environment definition, section, response, material, damage/effect/constraint reference and BSP breakable family | Object contracts, response graphs, placements and BSP surface comparisons retained; original gates and remaining consumers still need qualification | [ ] |
| D02 | Direct projectile, melee, collision and radial damage select the actual node, material and damage section | Existing source dispatcher and posed collision owners; all destruction producers need current MCC coverage | [ ] |
| D03 | Partial damage thresholds, shield/body coupling, rider/occupant coupling and special-death/skip gates | Preserve original damage-info and response consumers; no replacement health thresholds | [ ] |
| D04 | Every authored vehicle destruction class, including chassis/attached turret and campaign-only vehicles | Seven MCC multiplayer variants pass ordinary rockets, original death and debris births; campaign and individually stimulated attached branches remain required | [ ] |
| D05 | Pending responses, delayed effects, model-region/permutation changes and subsequent collisions | Host prop tests cover one module's six-tick delay; native TU1.5/MCC path and other responses remain required | [ ] |
| D06 | Destruction damage/effects use current marker/world location, source owner and strength | Original `DC660 -> DC3C0 -> D91D0`; verify real requests rather than reconstructing events from names | [ ] |
| D07 | Explosive-to-explosive chains, multiple simultaneous/delayed blasts and reciprocal vehicle/prop damage | Native module chain and both vehicle/coil directions pass with pristine witnesses and distant controls; simultaneous/delayed families remain | [ ] |
| D08 | Explosives affect nonexplosive damageable scenery, machines, equipment and breakable BSP materials | Zanzibar terrain bit/collision removal and coil-to-nonexplosive scenery pass; crate positive, other materials and fragments remain | [ ] |
| D09 | Blast candidate selection, obstruction, falloff, directional/stochastic branches and physical impulse | Actual original radius/per-target owners; a health reduction does not establish impulse or obstruction | [ ] |
| D10 | Detached parts, constraints, debris births, solved motion, wreck collision, occupant exit/death and object retirement | Historical vehicle/debris evidence is bounded; current MCC path and every authored transition remain required | [ ] |
| D11 | Multiplayer state, late join, replay/takeover and repeated snapshots preserve destruction and chains | Six post-destruction cases match complete worlds after repeated restores/replays; MCC loading identity fix passes; live snapshot timeout, late join and takeover remain | [ ] |
| D12 | Visible damage states, explosions, debris and authored audio reach normal single/split/LAN consumers | Native/acceptance builds pass; rendered LAN reaches its first frame then times out; full visual/audio acceptance remains | [ ] |

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
removed without executing a case. Runtime-v4 covered seven multiplayer vehicle
variants, Lockout modules, Foundation fusion coils, Colossus gas containers,
and Zanzibar terrain glass. Its failures and subsequent focused corrections
are recorded below. Runtime-v5 rebuilds thirteen cases from the current source,
including the reciprocal Headlong cases. Original bodies
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
green test/build scratch were removed. Runtime-v5 includes this edit in its
authenticated source snapshot; its ordinary destruction results and subsequent
fixture corrections are recorded in the combined acceptance section below.
No D-row is closed by this component result.

## Reciprocal vehicle/prop gate

Two additional Headlong cases execute through the native world:
`coil_to_warthog` and `warthog_to_coil`. They select actual source-created
Warthog/fusion-coil identities. The declared setup repositions one coil using
its original root-to-terrain height, places the shooter on supported terrain,
and then uses ordinary SMG or rocket input. The first original initiator
`DC4A0` request must see a pristine witness; subsequent witness damage and an
unchanged distant same-class control are required. No health, damage amount,
impulse or contact result is supplied. Both directions now pass after the
missing negative-control constructor input and moved-coil setup were corrected.
Their complete-world continuation results are recorded below.

## Current ordinary-weapon and collision evidence

The initial runtime-v4 production build passed, as did its BSP visibility
component. Its eleven native cases were red: seven vehicles and one prop used
an incorrect shot counter/unseeded raw look baseline, two prop scenes found no
supported six-metre firing lane, and glass reached an unregistered original
collision query. These failures remain in the reports; they are not destruction
acceptance. The Scorpion scene's fixed heavy selector was corrected against the
unchanged original selector fixture before the test unit compiled.

The direct [aim diagnostic](../analysis/destruction-goal-20261022/runtime-aim-diagnostic-v1/validation/report.json)
observed four real projectiles but a43-degree yaw offset. Original18A300 takes
relative changes from the retained native raw input; the fixture's first zero
packet established a different baseline from the actual spawned view. Seeding
the initial command from the published source view fixes the test input.
The [Warthog regression](../analysis/destruction-goal-20261022/runtime-aim-seeded-v1/validation/report.json)
then passes: two ordinary rocket shots, original death flag, changed model
regions,13 source-created debris objects and original chassis/turret destruction
requests. Solved debris trajectories, occupants, framebuffer/audio and other
vehicle families remain separate requirements.

The glass path now includes complete original2FC690/2FC150 collision bodies
(24/187 instructions), the original bounded MOPP opcode selector/target tables
(108/128 bytes), and eight independently enumerated fixed readonly query operands.
All earlier query instruction bodies compare equal after normalizing their
verified identity-selector spelling. No hit, contact or solver result is supplied.
A [captured live query](../analysis/destruction-goal-20261022/collision-query-original-v1/report.json)
matches unchanged TU1.5 i386 execution for all captured accessible bytes and
GP/SSE/x87/control state. This is host i386 evidence, not Xbox hardware or every
query branch. Its92,955,579-byte required reference capture and oracle binaries
were removed after comparison.

Original1872C0 additionally required the loader-initialized47A2B0 byte. Its actual
TU1.5.data initializer is01, and47EE50 registers its address. Cold composition
now provides exactly that writable byte, preserving future source updates and
checkpoint/restore rather than resetting it per frame. The [glass regression](../analysis/destruction-goal-20261022/runtime-glass-query-operands-v1/validation/report.json)
passes with two ordinary rocket shots and one observed detonation: original
187046 clears the selected bit and a subsequent original collision query no
longer hits the surface. Actual fragments, secondary-light pixels and normal
visual/audio delivery remain unverified.

The old bounded owner closed after a long terminal command exceeded canonical
input length. Its1.4GiB rebuildable source/build were removed; all bounded
reports above remain. Runtime-v5 built its recorded source with all preceding fixes,
reviewed scalar manifests, thirteen native cases and six Cache integration
cases. Interactive commands are short; diagnostics remain in bounded runner
logs. No selected runtime has been installed or replaced.

## Additional destruction and continuation regressions

The fixture also supports `coil_to_scenery` (Zanzibar's actual
`no_swimming` scenery) and `coil_to_crate` (Foundation's actual military case).
Both use actual source-born participants, a declared supported coil placement,
ordinary SMG commands, a pristine witness at the first original initiator blast,
subsequent source-owned witness damage and a distant same-class negative control.
Their nonexplosive witness contracts are retained in the object census.
Coil-to-scenery passes actual native damage/retirement. The crate initiator
dies and produces its original blast, but its pristine witness stays unchanged;
this is an unaccepted positive, requiring original location/obstruction/material
diagnosis rather than an assigned damage result.

Optional `HALO2_DESTRUCTION_CONTINUATION` captures the complete native/Match
cut after a successful destruction regression, records eight ordinary neutral
frames including actual effect updates, restores the cut twice, and replays
each encoded call/clock journal through the original APIs. Each resulting
complete encoded world must match its SHA256; repeated restoration must also
match the initial bytes. Cuts stay in memory and are not retained. This gate
passes on Warthog, glass, modules, both vehicle/coil directions and scenery:
six distinct cases with16 complete-world comparisons each. Earlier live-clock
comparisons differed solely in advancing external clock fields; their source
banks and every other encoded value matched. No continuation behavior was
changed to pass that input issue. This does not establish delayed in-flight response cuts,
peer transport, late join, takeover or presentation acceptance.

The [static response-flag audit](../analysis/destruction-goal-20261022/response-bit25-static-v1.json)
authenticates all90 compiled original damage-delivery bodies and finds no explicit
bit25 operation. Byte aliases and callers remain outside that narrow check;
this result alone does not justify converting the25 differing turret records.

## Combined native acceptance and loading identity

[Runtime-v5](../analysis/destruction-goal-20261022/runtime-v5-native-summary.json)
builds the full headless production dependency graph and passes11 of13 ordinary
MCC cases, plus all10 component/Cache checks. All seven vehicle variants pass:
Warthog, Gauss Warthog, Ghost, Banshee, Scorpion, Wraith and Spectre. Two original
fixture failures remain in that report. Subsequent native iterations pass the
module chain with one request per generation, a pristine witness at the first
blast, and unchanged distant control. Pulsed ordinary SMG input allows the
authored spread to recover. The opposite vehicle/coil direction passes after
creating its missing distant control with originalB9960/B9B70 and a declared
supported location. No health, damage or physics result is supplied.

Both `halo2_native` and the explicit visible acceptance executable build in the
same disposable snapshot. Strict native-variant bundling rejected its borrowed
`data` symlink;15,418 bytes of exact small source metadata were privately copied
with hashes, preserving the workspace data. Later unrelated workspace source
drift is recorded, and goal changes are admitted into the snapshot with narrow
authenticated patches. No selected runtime has been installed or replaced.

The actual rendered LAN route exposed a production loading-identity defect.
A normal source-owned host hashed its already born rocket tag4040774092, while
an equivalent client before admission hadNONE. Both retained original configured
selector8. The direct two-world regression also reproduces the mismatch with
the older declared-spawn host fixture. Prepared worlds now supply original
validated selector89 to MCC loading identity, independently of presentation
weapon state or player-birth timing. Equipment selection remains source-owned;
legacy cache identity and raw content hashing retain their previous behavior.
The [green direct regression](../analysis/destruction-goal-20261022/lan-identity-green-v2/validation/report.json)
passes both worlds, presentation-field independence, another-selector rejection,
and the existing raw asset identity suite. MCC peers need the corresponding
rebuilt identity behavior on both ends.

The subsequent rendered LAN run passes loading admission and reaches an actual
first frame, then reports a host timeout. The next diagnostic retains the last
rejected packet reason rather than inferring whether packets stopped arriving
or failed admission. This is still a failed visible/LAN case.

A separate local invocation of the old blast fixture segfaulted before firing.
An authenticated unchanged211,306,720-byte ELF replay under GDB locates the null
LAN-host dereference in its frame observer. The fixture now rejects that
unsupported local invocation explicitly. This is not a vehicle-engine crash
fix or local-renderer acceptance. The first debugger prefix was clipped and
cannot establish a replay result; the second retains the final stack. Both
required ELF copies and probe scratch were removed after bounded diagnostics.

The first crate-location diagnostic incorrectly read private compiled-call
stack fields through the inactive public memory bank; those location/ray fields
are invalid and establish no obstruction result. Actual original marker stores
atDC420/DC43C/DC42B are now captured as produced bytes and matched toDC4A0's
stack identity. Their instruction offsets are independently authenticated from
the pinned TU1.5 executable. The corrected diagnostic remains pending.
