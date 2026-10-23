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
| D04 | Every authored vehicle destruction class, including chassis/attached turret and campaign-only vehicles | Seven chassis variants and five attached turrets pass bounded native death/region checks; all37 complete-definition gates remain open | [ ] |
| D05 | Pending responses, delayed effects, model-region/permutation changes and subsequent collisions | Host prop tests cover one module's six-tick delay; native TU1.5/MCC path and other responses remain required | [ ] |
| D06 | Destruction damage/effects use current marker/world location, source owner and strength | Original `DC660 -> DC3C0 -> D91D0`; verify real requests rather than reconstructing events from names | [ ] |
| D07 | Explosive-to-explosive chains, multiple simultaneous/delayed blasts and reciprocal vehicle/prop damage | Native module chain and both vehicle/coil directions pass with pristine witnesses and distant controls; simultaneous/delayed families remain | [ ] |
| D08 | Explosives affect nonexplosive damageable scenery, machines, equipment and breakable BSP materials | Terrain bit/collision removal, coil-to-scenery and coil-to-crate pass; other materials and fragments remain | [ ] |
| D09 | Blast candidate selection, obstruction, falloff, directional/stochastic branches and physical impulse | Actual original radius/per-target owners; a health reduction does not establish impulse or obstruction | [ ] |
| D10 | Detached parts, constraints, debris births, solved motion, wreck collision, occupant exit/death and object retirement | Historical vehicle/debris evidence is bounded; current MCC path and every authored transition remain required | [ ] |
| D11 | Multiplayer state, late join, replay/takeover and repeated snapshots preserve destruction and chains | Twenty post-destruction cases match complete worlds after repeated restores/replays; MCC loading identity and1,753 exact UDP comparisons pass; destruction transport, late join and takeover remain | [ ] |
| D12 | Visible damage states, explosions, debris and authored audio reach normal single/split/LAN consumers | Native/acceptance builds and rendered LAN driving/camera pass; rendered destruction remains red; full visual/audio acceptance remains | [ ] |

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
Coil-to-scenery passes actual native damage/retirement. The earlier crate
placement had a real intervening BSP surface. A corrected supported placement
requires a clear predicted center ray before moving the coil; actual contact,
motion and damage still run through original owners. The
[clear crate regression](../analysis/destruction-goal-20261022/runtime-crate-clear-v1/validation/evidence/destruction-coil_to_crate.json)
passes four SMG shots, one initiator blast, pristine witness until that tick,
actual blast distance1.114m with a clear BSP ray, crate body reduction from1
to-0.559608 and unchanged32m distant control. No destruction damage or material
implementation changed to obtain this result.

Optional `HALO2_DESTRUCTION_CONTINUATION` captures the complete native/Match
cut after a successful destruction regression, records eight ordinary neutral
frames including actual effect updates, restores the cut twice, and replays
each encoded call/clock journal through the original APIs. Each resulting
complete encoded world must match its SHA256; repeated restoration must also
match the initial bytes. Cuts stay in memory and are not retained. This gate
passes on Warthog, glass, modules, both vehicle/coil directions, scenery and crate:
seven distinct cases with16 complete-world comparisons each. Earlier live-clock
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

The third rendered LAN run completes1,753 exact UDP comparisons over1,383
rendered frames. Driver entry, movement, steering, exit and source camera
checks pass. The destruction gate remains red: vehicle body stays1, with no
observed death/debris or point impulses despite19 firing-command frames.
Its host used the older declared-spawn adapter. The next fixture uses the
complete original multiplayer engine for player birth and applies only the
declared scene position through the original object-position API. It requires
source ownership explicitly. The earlier first-frame timeout remains recorded;
this later completed run does not independently establish its cause or fix.

A separate local invocation of the old blast fixture segfaulted before firing.
An authenticated unchanged211,306,720-byte ELF replay under GDB locates the null
LAN-host dereference in its frame observer. The fixture now rejects that
unsupported local invocation explicitly. This is not a vehicle-engine crash
fix or local-renderer acceptance. The first debugger prefix was clipped and
cannot establish a replay result; the second retains the final stack. Both
required ELF copies and probe scratch were removed after bounded diagnostics.

The early blast-location diagnostics used the wrong request-field semantics
and missed two argument pushes; their location/ray fields are invalid. The
corrected producer is authenticated by19 unchanged original instructions:
DC464,DC47B andDC486 store actualZ,X,Y, andDC4A0 follows two argument pushes.
Its request+24 point is the emitter-store address+0C. The originalD9249
consumer takes that exact field. The [corrected native probe](../analysis/destruction-goal-20261022/runtime-blast-diagnostic-v5/refresh.json)
passes both vehicle/coil directions and scenery, with unobstructed analytic
BSP rays. The crate witness stays pristine1.742m from the actual coil blast,
but its BSP ray hits surface3667, plane1383, material104 after0.290m. This
independent geometry observation motivates a clear supported placement; it does
not by itself authenticate the original damage obstruction branch. No crate
positive acceptance or material-gate change is claimed.


The complete-source-engine LAN fixture completes1,805 exact UDP comparisons
but never admits vehicle entry, so it supplies no destruction stimulus. Player
birth and declared position inputs are recorded. A bounded diagnostic compares
the original retained action target with the published prompt to identify this
pre-destruction failure. This run does not establish driver/camera acceptance;
the preceding1,753-comparison run's driving result belongs to its older host path.


## Attached vehicle and cleanup owners

The expanded native fixture selects actual original-born chaingun, Gauss,
Scorpion MP cannon, Wraith MP mortar and Spectre plasma-turret child handles,
preserving their full parent identities and variant inputs. The first pass
completes Wraith retirement/continuation, but leaves four cases red. Two reach
missing cleanup entry1186D0. Two set original damage flags12 and change authored
regions while retaining body0.0001; the old fixture incorrectly required body
less than or equal to zero as well as the original dead flag.

[Original instructions](../analysis/destruction-goal-20261022/original-death-flag-v1.json)
authenticateD8D8A's `object+10A |=4`, its following originalBABA0 callback and
the subsequent bit2 consumer. The fixture now requires an observed original
flag transition and body reduction, with region changes for these attached
cases. Whole chassis cases retain their debris-birth requirement. This bounded
attached scope does not certify every parent, debris or occupant consequence.

[Original1186D0](../analysis/destruction-goal-20261022/owner-1186d0-original-v1.json)
is a complete four-instruction light-receiver pool deletion wrapper: read the
argument, read4ECC04, call16E8A0, return4. It is appended to the native death
owner without replacing any of its309 qualified bodies, FP helpers or observers.
The append workflow takes a hash-recorded qualified source because newer shared
lowering also emits unrelated FP changes. Its first component run rejected a
missing identity-selector registration; the corrected wrapper is rebuilding.
Thirty-two new unchanged-original executions stop at the genuine16E8A0 boundary,
checking literal arguments, GP/SSE state, flags, ordered stores and stack bytes;
they supply no pool-deletion result. The480 previous cases remain byte-identical
in the required1,383,790-byte reference fixture. Native cleanup acceptance is
still pending the live runs.


[Runtime-v6](../analysis/destruction-goal-20261022/runtime-v6-native-summary.json)
passes all20 declared native cases in one authenticated production build and
320 complete encoded-world comparisons after repeated restores/replays. This
includes all seven chassis variants, five attached turret definitions, three
explosive prop families, terrain glass, both vehicle/coil directions and
nonexplosive scenery/crate interactions. Every attached turret is observed at
actualD8D8A changing flag0 to4, changing model regions and retaining the actual
positive0.0001 body floor. The fixture does not impose another health threshold.
The [37-definition matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v4.json)
links thirteen bounded vehicle cases across twelve distinct definitions; none
closes its complete-definition acceptance gates. The source512-case helper
suite and actual asset identity also pass, and native/visible executables build.

The subsequent native LAN case reaches entry, driving, exit and ordinary rocket
fire but the actor dies before a later disconnect. It records no further
missing1186D0 error; this alone does not authenticate that wrapper's live call.
The next declared firing approach separates the actual target sphere from both
the incoming rocket's upper blast radius and the vehicle model's direct authored
response-damage radii, plus the player capsule radius. It changes input planning,
not damage or motion. A bounded failure receipt now survives a runtime exception.
No rendered destruction or connection-timeout fix is claimed from this retry.


The [explicit native teardown](../analysis/destruction-goal-20261022/runtime-cleanup-call-v1/validation/evidence/destruction-warthog.json)
now independently observes1186D0's actual CALL store at1186DA, completes original
16E8A0 and retires the remaining full Warthog generation through the original
deletion API. Natural destruction and replay precede that declared teardown
input. This authenticates the complete new wrapper in the live composition;
it does not certify automatic wreck-retirement timing.

The radius-aware LAN retry completes2,208 exact comparisons and source driving/
camera checks, but the vehicle stays at body1. Its first actual rocket detonates
beside the shooter at tick1219 while raw PC pitch is saturated at-1.8. TU1.5
18A300 consumes deltas in raw PC samples; vehicle reorientation can change the
processed source view separately. The test's absolute aiming helper incorrectly
compared its desired world angle to raw input. The corrected helper uses the
actual source view for angular error, issues one correction per observed world
tick, and fires only when the source view ray enters the actual target sphere.
These are input-planning changes. The corrected rendered run remains pending.


Foundation's fixed human turret now passes two ordinary rockets, original dead
flag, model-region change, one authored destruction blast and16 complete-world
comparisons. Its first fixture rejected the absence of crate-object debris.
The [original effect graph](../analysis/destruction-goal-20261022/human-turret-object-parts-v1.json)
has light, sound and decal parts plus five particle systems, and no object part.
The [corrected bounded case](../analysis/destruction-goal-20261022/runtime-human-turret-v2/validation/evidence/destruction-h_turret_ap.json)
therefore requires death/region/blast, with particle-fragment and presentation
gates still separate. It supplies no fragment object or new damage behavior;
the differing response bit25 remains unconverted and unresolved.

The processed-view aiming retry keeps the actor alive and reaches vehicle body
0.9043 over1,911 exact UDP comparisons, with driving/camera gates passing. Four
actual rocket detonations lie beyond the vehicle. The next read-only input
planner tests the real collision model against current authoritative WORLD
nodes/permutations and chooses a solid authored surface. A broad bound-centre
ray alone cannot establish a physical hit. Native projectile and damage results
remain entirely original-owned; the new rendered result is pending.


The first posed-planner invocation does not reach entry (3,322 exact UDP
comparisons), so it provides no new collision or destruction stimulus. The same
raw/processed angle distinction also applies to the entry/departure helper.
The next built fixture uses actual source-view angular errors for those stages,
once per observed source tick, and starts its input sequence only after the real
player unit exists. Native runtime behavior is unchanged by these test inputs.
