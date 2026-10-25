# Vehicle destruction and explosive interactions

Status: active, 2026-10-24. This ledger covers the user's complete vehicle
destruction and explosive/damageable-material interaction request. A component
test, one destroyed vehicle, or an asset inventory does not complete the goal.

The original Xbox TU1.5 executable and authored Xbox tags are offline behavioral
references. Playable validation uses the prepared, port-owned MCC cache. Preserve
original inputs, historical evidence, selected runtimes, and unrelated work.
Use `tools/run_tests.py` for CMake and `tools/run_test.py` for probes; remove owned
builds, frames and binaries, retaining bounded reports and source hashes.

## Native execution and one-to-one behavior

The user's 2026-10-23 steering requires native kernels while preserving original
TU1.5 behavior. Compiled generated owners, authenticated operand specialization
and semantic native kernels are distinct evidence categories. A compiled source
translation alone does not establish that its register/memory bookkeeping has
been replaced by a semantic kernel.

Qualify each semantic replacement against the unchanged original owner and the
retained literal route before admitting it. Preserve floating-point bits and
observable status, ordered stores and calls, fault/admission boundaries, RNG,
source clocks, full object generations, attribution and publication order.
Unproved inputs retain their qualified literal path. Do not substitute guessed
health, radius, geometry, collision or physics results to make a test pass.
Performance measurements do not close behavior gates, and a native build does
not by itself establish one-to-one gameplay, visuals or audio.

## Completion matrix

Every row requires the original owner, actual MCC execution, a direct positive
regression and relevant negative controls. Delayed/recursive requests must retain
the source clock, RNG, full object generations, attribution, and source ordering.

| ID | Required behavior | Current evidence / next gate | Complete |
| --- | --- | --- | --- |
| D01 | Inventory every vehicle and damageable environment definition, section, response, material, damage/effect/constraint reference and BSP breakable family | Object contracts, response graphs, placements and BSP surface comparisons retained; original gates and remaining consumers still need qualification | [ ] |
| D02 | Direct projectile, melee, collision and radial damage select the actual node, material and damage section | Existing source dispatcher and posed collision owners; all destruction producers need current MCC coverage | [ ] |
| D03 | Partial damage thresholds, shield/body coupling, rider/occupant coupling and special-death/skip gates | Preserve original damage-info and response consumers; no replacement health thresholds | [ ] |
| D04 | Every authored vehicle destruction class, including chassis/attached turret and campaign-only vehicles | Twenty-two death/retirement cases cover 21 definitions; the guntower and two Pelican attachment definitions have scoped no-information-gate evidence. All 37 complete-definition gates remain open | [ ] |
| D05 | Pending responses, delayed effects, model-region/permutation changes and subsequent collisions | Host prop tests cover one module's six-tick delay; native TU1.5/MCC path and other responses remain required | [ ] |
| D06 | Destruction damage/effects use current marker/world location, source owner and strength | Original `DC660 -> DC3C0 -> D91D0`; verify real requests rather than reconstructing events from names | [ ] |
| D07 | Explosive-to-explosive chains, multiple simultaneous/delayed blasts and reciprocal vehicle/prop damage | Native module chain and both vehicle/coil directions pass with pristine witnesses and distant controls; simultaneous/delayed families remain | [ ] |
| D08 | Explosives affect nonexplosive damageable scenery, machines, equipment and breakable BSP materials | Terrain bit/collision removal, coil-to-scenery and coil-to-crate pass; other materials and fragments remain | [ ] |
| D09 | Blast candidate selection, obstruction, falloff, directional/stochastic branches and physical impulse | Actual original radius/per-target owners; a health reduction does not establish impulse or obstruction | [ ] |
| D10 | Detached parts, constraints, debris births, solved motion, wreck collision, occupant exit/death and object retirement | Historical vehicle/debris evidence is bounded; current MCC path and every authored transition remain required | [ ] |
| D11 | Multiplayer state, late join, replay/takeover and repeated snapshots preserve destruction and chains | The qualified native cohort has 30 bounded cases and 496 full-world comparisons; earlier SDL Warthog destruction passes 1,745 exact LAN snapshots; late join, pending transport and takeover remain | [ ] |
| D12 | Visible damage states, explosions, debris and authored audio reach normal single/split/LAN consumers | Native-rate SDL inspection shows the explosion, overturned wreck and a detached ground part in the actor view; complete original permutations, all debris identities, framebuffer matching and audio remain open | [ ] |
| D13 | Native kernels preserve one-to-one TU1.5 destruction, blast and collision behavior | Rate, indexed-record, shared model-lookup, model-child selection and material-multiplier kernels have scoped original-code qualification; remaining damage, blast and collision semantic conversions stay open | [ ] |

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

## Current-source revalidation after owner expiry, 2026-10-23

The former owned source/build `halo2-test-wfapge2t` and its process are absent;
[the cleanup receipt](../analysis/destruction-goal-20261022/runtime-v5-cleanup-verified-20261023.json)
records their removal. The corrected posed-query mask compiled in that owner,
but its LAN execution did not run before the interruption. It has no runtime
acceptance yet. No large captures or binaries were retained from that owner.

A fresh frozen source/build is running the 21-case cohort, original death-cycle
helper tests and native/SDL targets through the official worker. The
[current report](../analysis/destruction-goal-20261022/runtime-v7/validation/report.json)
is authoritative for that run. Older green reports remain bounded evidence of
their recorded sources. The cohort now privately freezes its small configuration
files, helper headers and qualified original oracle fixture.

The next added case is the authored campaign Covenant turret. Its fixture uses
the actual origin BSP, a surveyed initial observer position, original runtime
scenario creation when initially absent, and original equipment construction.
This new case is not yet executed and does not close a definition gate.

## Fixed human-turret authored response tuple, 2026-10-23

The source-backed model census isolates one scalar discrepancy in
`objects\vehicles\h_turret_ap\h_turret_ap`: damage block0, section0,
response0 flags are `00000000` in the original maps and `02000000` in MCC.
Every other normalized field in that complete model contract matches.
[The reviewed profile](../tools/mcc-destruction-model-scalar-profile.json)
authenticates both contracts and all25 paired original/MCC map owners.

The new Cache conversion restores that literal word only for the exact model
name, original1/1/1 block shape and complete reviewed response scalar tuple.
Unknown tuples stay unchanged; malformed metadata fails before writing.
This assigns no meaning to the added bit25 and supplies no runtime damage,
destruction, collision or physics result. It is authored-input normalization,
not a semantic native-kernel replacement or full model acceptance.

[The component regression](../analysis/destruction-goal-20261022/model-scalar-component-v2/validation/report.json)
passes the source tuple, whole-buffer preservation, eight unknown tuple controls,
three block-shape controls, idempotence and malformed-input atomicity. Both
disposable component owners and the separate input-audit owner were verified
removed; no binaries or captures were retained. Native Cache admission on
Foundation/Coagulation and the ordinary-rocket turret death/replay regression
are queued in the still-owned current-source build and remain unaccepted until
their actual reports pass.

The [before-change native Cache control](../analysis/destruction-goal-20261022/model-scalar-native-baseline-v1/validation/tests.log)
loads the real Foundation map through the completed, unmodified frozen Cache
library. Its regression fails precisely at response field `+04`: original0,
actual33554432 (`02000000`). The frozen class headers and read-only archive
inputs are pinned to that cohort, and all borrowed libraries remain unchanged.
The component converter is linked separately and is not called by that old
Cache constructor. This is a direct native admission failure, rather than
evidence inferred only from a synthetic payload. The same Cache regression
must pass after the narrow production refresh. Its separate owned executable,
headers and build tree were removed; no additional binary was retained.

## Qualified current native snapshot, 2026-10-23

[The current summary](../analysis/destruction-goal-20261022/current-native-summary-v8.json)
records the fresh native game/SDL build,33 selected CTests,22 bounded native
destruction/material cases, and368 full-world comparisons across22 post-death
cuts and one actual pending-response cut. The
[definition matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v5.json)
still leaves every full definition gate open. Fourteen bounded vehicle cases
cover13 distinct definitions; the remaining families and full per-definition
requirements retain their original scope.

The pending Lockout module word is actually48 in section1. The fixture captures
that native cut, runs/replays the genuine delayed chain, compares16 complete
worlds, restores the exact initial cut, then resumes the ordinary stimulus. Its
post-destruction cut separately compares16 worlds. Caller clocks are declared
inputs; no pending word, health, RNG, contact, solver or destruction result is
supplied. These are local cuts, not transport/takeover acceptance.

The fixed human-turret normalization now passes its pure regression, real
Foundation/Coagulation Cache admission, ordinary rocket death/region/blast
transition and16 complete-world comparisons. The campaign Covenant turret
separately passes with actual mode1/difficulty1, actual original equipment and
runtime scenario creation, one ordinary rocket, an original D8D8A death-flag
store and changed regions. Campaign setup initially queried the MP weapon bank
on a campaign map. The fixture now queries that bank only in its MP branch;
the production MP admission check remains strict. Story, AI and a full campaign
playthrough are not certified by this finite empty-script physics stimulus.

The corrected posed-model query mask admits ordinary SDL rocket aiming in LAN.
The Warthog reaches body-1.2278193, produces570 original dead-flag samples, six
original target impulses and eleven actual debris objects with8.145m maximum
displacement. The two views submit eleven debris parts each and1,799 snapshots
match exactly. [The retained source vitality subset](../analysis/destruction-goal-20261022/visible-warthog-posed-aim-v3/lan-vehicle/authority-driven-vehicle-vitality.json)
contains the exact ordered rows for that full target generation.

[Pixel inspection](../analysis/destruction-goal-20261022/visible-warthog-pixel-inspection-v1.json)
at26.5/28/31/40/55 seconds shows the vehicle obscured by a rock/weapon and the
observer facing ground/wall. A burst appears at26.5 seconds, but the sampled
vehicle and debris cannot be identified. Submitted draw counts do not prove
pixel visibility. The next presentation gate needs ordinary SDL movement/look
inputs that provide an unobstructed view, followed by actual frame inspection.
The inspection's separate owned frames were removed; no additional binary or
frame is retained outside the still-active main test owner. Audio remains open.

## Later SDL and campaign qualification, 2026-10-23

The observer gamepad now closes its look loop against the processed native
view. Its raw PC angle retained a different birth baseline. The first rerun
still failed destruction: the ordinary rockets hit foreground terrain and the
target remained alive. No engine damage, recovery or collision behavior was
changed to resolve that scene failure.

The fixture's [firing-lane helper](../tests/helpers/destruction_firing_lane_inputs_tu15.hpp)
reads actual cached BSP collision/render triangles and current posed vehicle
collision. It selects supported walking and clear aiming inputs, then delivers
ordinary SDL movement/look/fire. Actual movement, impacts, damage and death
remain native engine results. The
[completed run](../analysis/destruction-goal-20261022/visible-warthog-firing-lane-inspection-v1.json)
starts with pristine body1 immediately before firing, reaches the original
death flag at tick814 after one ordinary rocket, and observes minimum body-1.6,
six target impulses, ten debris objects and3.453m maximum solved debris travel.
It passes1,797 exact LAN comparisons. The frame at18 seconds identifies the
Warthog between rocks; the18.5-second frame shows an explosion at the vehicle in both views.
Later wreck motion goes behind cover. Complete damaged permutations, distinct
debris pixel delivery and authored audio remain unaccepted. The completed
capture phase's140,473,444 bytes were
[removed](../analysis/destruction-goal-20261022/visible-capture-phase-cleanup-v1.json);
bounded source samples, reports, commands and hashes remain.

The campaign legacy human turret additionally passes an ordinary rocket,
observed original death-flag write, full target retirement, twelve object-part
births and16 complete-world replay comparisons. Its original/MCC model contract
has the exact first-response flags `0x81`. Immediate retirement makes a later
region snapshot unavailable; the fixture instead requires the actual death,
retirement and object parts for this authored family. This adds one bounded
definition without closing any full-definition gate. The
[latest qualified summary](../analysis/destruction-goal-20261022/current-native-summary-v10.json)
has23 bounded cases and384 complete-world comparisons, while the
[matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v6.json)
keeps all37 complete-definition gates open.

The Cairo legacy plasma-cannon case initially stopped before firing because
that map has no resident rocket launcher. The original and MCC maps both
contain the declared SMG; the original equipment API now supplies that ordinary
weapon input. Its12m firing-lane survey remained red on the catwalk. Nearer
BSP-supported initial scene positions are declared in the pending retry.
No plasma-cannon destruction acceptance is claimed yet.

The native-rate integration admits only the two original-qualified scalar
owners and the named PC adapter. All28 local dependency files match the
passing unchanged-original qualification. Exact substitutions preserve every
retained instruction body and the original PC mapping. The
[integration receipt](../analysis/destruction-goal-20261022/native-rates-integration-v1/admission.json)
records the private source/build refresh, full native executables and all24
registered destruction cases. Runtime counters are diagnostic only; the Ghost
case must observe an actual live native-kernel call. This build/test phase is
pending. Neither component qualification nor the previous literal cohort closes
D13 or qualifies the newly refreshed native engine.

The native executable build subsequently passed. Both complete24-case runs
passed23 cases and384 full-world comparisons, including the real pending
module response. The Ghost's original live composition admitted eight native
acceleration calls. The approach owner has not yet been observed in these
destruction cases. The two original-qualified rate owners therefore retain
separate component and runtime-coverage claims.

Cairo remains red. A read-only native Cache/BSP
[input survey](../analysis/destruction-goal-20261022/cairo-firing-input-survey-v1/survey.json)
found no clear top-surface candidate at any tested distance, while lower
surfaces provide supported clear lanes. Its compiler/probe owner was
[removed](../analysis/destruction-goal-20261022/cairo-firing-input-survey-cleanup-v1.json),
and every borrowed header/library remained unchanged. The corrected fixture
reached a valid layer4 floor but thirty ordinary SMG shots at the object bound
centre produced no death.

The next current-pose query demonstrated that the centre ray misses the actual
collision model. It admitted a real surface at node1/region1, global material64
and authored damage section0. Thirty ordinary single firing pulses at12m still
produced no section-state writes or death. These are diagnostic failures, not
evidence for changing native health, damage thresholds or collision results.
The [current integration record](../analysis/destruction-goal-20261022/current-native-integration-progress-v11.json)
pins both failures and the passing native cases. The pending closer, supported
SMG stimulus uses ordinary held automatic fire and reads actual request/target
arguments at the unchanged original TU1.5 direct dispatcher. It supplies no
projectile hit or damage result. Full Cairo, driving, approach-owner, visual
and audio acceptance remain open.

The closer posed-surface Cairo run
[passes](../analysis/destruction-goal-20261022/native-rates-direct-hit-v4/validation/evidence/destruction-campaign_plasma_cannon.json):
18 ordinary held-SMG shots deliver12 actual original direct-damage requests to
the full target generation. Native section storage changes, original `D8D8A`
sets its death bit, the target retires naturally, four authored object parts
are born, and16 complete-world replay comparisons pass. No health, collision,
damage, firing spread or response threshold changed. The diagnostic's stack
recovery is separately checked against the
[untouched original prologue](../analysis/destruction-goal-20261022/cairo-direct-dispatch-prologue-v1.json);
that read-only probe owner was removed. A deleted target is unavailable for the
later vitality poll, so its recorded `source_dead=false` is accompanied by the
actual original death store and full-generation retirement rather than being
treated as evidence of survival.

The [updated definition matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v7.json)
now has16 bounded vehicle cases across15 definitions, with every full gate
still open. The current-source
[24-case rerun](../analysis/destruction-goal-20261022/native-rates-final-v5/refresh.json)
passed all 24 cases and 400 exact complete-world comparisons. Its guarded native
driving/LAN recording is described below.

## Native rate kernels and rendered destruction, 2026-10-23

[The unified receipt](../analysis/destruction-goal-20261022/native-rates-final-v5/summary.json)
qualifies the native game, SDL acceptance executable and 24 destruction/material
cases from the authenticated frozen source. The Ghost admits eight actual live
native acceleration calls. The approach owner still has component proof only;
this cohort does not claim complete semantic native-kernel coverage.

The first rendered native-rate retry failed destruction. Its 300 observed
forward commands spanned 539 source ticks, compared with 304 ticks in the earlier
green run. The vehicle therefore reached a different part of the map. The
[fixture correction](../analysis/destruction-goal-20261022/visible-source-tick-budget-v1/admission.json)
budgets driving and the initial steering turn in source ticks. It changes only
ordinary SDL stimulus, preserving original motion, collision and damage.

The corrected [rendered LAN run](../analysis/destruction-goal-20261022/visible-warthog-native-rates-source-clock-v6/lan-vehicle/report.json)
passes 1,745 exact snapshots, observes 125 live native acceleration calls, and
reaches original Warthog death with six target impulses and ten debris objects.
Both views submit ten authored debris parts. The
[frame inspection](../analysis/destruction-goal-20261022/visible-warthog-native-rate-inspection-v1.json)
observes an explosion, overturned wreck and a separate ground part in the
actor view. The observer is distant; complete part identity/permutation pixels,
original framebuffer matching and audio remain open. The
[current qualified summary](../analysis/destruction-goal-20261022/current-native-summary-v14.json)
keeps all 37 full-definition gates open.

The [Hunter authored audit](../analysis/destruction-goal-20261022/hunter-model-authority-v2.json)
authenticates six paired original/MCC map owners and the complete normalized
contract difference: original body flags 0 with a zero-count/null response block,
versus MCC flags 2 and one added response. A
[before-change native Cache regression](../analysis/destruction-goal-20261022/hunter-native-before-v1/component.json)
rejects that actual flag in all six maps. The pure converter passes six exact
tuples plus unknown/malformed input controls, preserving every unrelated byte.
The production correction restores the three original descriptor words only
after the complete reviewed scalar/response guard; it does not assign meanings
to the donor flags or provide damage results. The
[owned refresh](../analysis/destruction-goal-20261022/hunter-native-admission-v1/admission.json)
passes all eight checks: the component, six actual Cache plus relocated native
tag-image cases, and the loading-identity regression. Hunter damage behavior
remains a separate gate. The updated
[complete destruction cohort](../analysis/destruction-goal-20261022/hunter-native-full-cohort-v2/summary.json)
passes all 24 cases and 400 exact world comparisons after the correction. The
[updated matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v8.json)
still leaves every full-definition gate open.

The remaining model census discrepancy has an
[authenticated two-map audit](../analysis/destruction-goal-20261022/flood-clump-authority-v1.json).
The large Flood floor-clump response has a literal null effect handle in both
original maps. MCC adds `flood_infection\\body_destroyed`; every other normalized
model contract field matches. No Flood-clump production correction or runtime
acceptance is claimed from this read-only audit. Its disposable owner was
verified removed.

The next attached-target extension uses the original campaign Scorpion cannon,
distinct from `cannon_mp`. Its
[parent placement and resident weapon audit](../analysis/destruction-goal-20261022/campaign-scorpion-inputs-v1.json)
matches both original/MCC inputs. The fixture asks the original scenario owner
to create the parent when absent, then selects its actual named child. It
supplies no attachment, child object or damage result. This native extension
passes in the [native extension](../analysis/destruction-goal-20261022/campaign-scorpion-native-summary-v1.json):
four ordinary rockets trigger the original death-bit store and change model
regions, followed by 16 exact complete-world comparisons. Its minimum body
fraction is still 0.5 when the original death path wins; no global-health-zero
assumption is used. The campaign turret and Ghost controls also pass; the Ghost
admits eight live native acceleration calls. Those repeated controls add no new
unique cuts. The combined evidence now has 25 bounded cases and 416 unique world
comparisons, with 17 vehicle cases across 16 definitions. All 37 full-definition
gates remain open in the [matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v9.json).
Inspected native-rate recordings
and previews, totalling 113,153,076 bytes, were
[removed](../analysis/destruction-goal-20261022/native-rate-visible-capture-cleanup-v1.json);
the finite source/build owner remains active for the continuing full vehicle and
material investigation. Its [storage receipt](../analysis/destruction-goal-20261022/campaign-cannon-test-storage-v1.json)
confirms the completed CTest scratch was removed and records the active owner.

## Flood-clump authored inputs and native collision/damage owners, 2026-10-23

The [direct native before-change check](../analysis/destruction-goal-20261022/flood-native-before-v1/component.json)
rejects the added effect reference in both maps. The pure correction preserves
every other byte and rejects unknown scalars, shapes, full tag identities and
malformed owned views. The
[native admission](../analysis/destruction-goal-20261022/flood-native-admission-v1/validation/report.json)
passes twelve checks: the component, both actual Cache/relocated tag images,
six Hunter maps, two human-turret maps and the rebuilt LAN-loading identity
test. This closes the named input discrepancy; live scenery behavior remains
separate.

All twelve [Flood Lab scenery rows and the resident fuel-rod weapon](../analysis/destruction-goal-20261022/campaign-flood-clump-inputs-v3.json)
match the original inputs, including the complete literal scenery rows. The
map has no resident rocket launcher. The ordinary fuel-rod fixture reaches
actual campaign mode and uses the original equipment API. Initial actor
settling exposed a missing collision target at `2F7038 -> 304C90`.

The complete original `304C90` collector wrapper and `2F4DE0` query are now
compiled in the existing collision module: 34 and 248 instructions. All earlier
32 complete instruction bodies remain
[byte-identical](../analysis/destruction-goal-20261022/flood-query-body-preservation-v1.json).
The [actual live-call comparison](../analysis/destruction-goal-20261022/flood-query-original-v1/report.json)
passes 6,695 unchanged original i386 instructions, all 110,092,707 captured
accessible memory bytes and the complete GP/SSE/x87/control state. This is host
i386 component proof. It does not establish every query branch, semantic-kernel
conversion or Xbox hardware parity. The temporary banks, reference executable,
outputs and duplicate candidates, totalling 456,482,959 bytes, were
[removed](../analysis/destruction-goal-20261022/flood-query-original-v1/cleanup.json);
only bounded ABI metadata, memory hashes and the comparison report remain.

The [next live run](../analysis/destruction-goal-20261022/flood-query-live-v4/validation/evidence/destruction-campaign_flood_clump.json)
gets through actor collision and reaches ordinary weapon damage. It then
exposes `D91AF -> E89B0`. The complete original 40-instruction callback and its
105-instruction `1E9290` dependent owner are bound through a narrow native
world registration. Their [build/live validation](../analysis/destruction-goal-20261022/flood-damage-live-v5/admission.json)
passes the new scenery case, Ghost and campaign Scorpion-cannon controls. Two
ordinary fuel-rod shots produce one explosion, the actual clump's original
death-bit transition and natural full-generation retirement, followed by sixteen
exact world comparisons. The world-wide new class11 objects are not attributed
to the clump. The stronger projectile-to-area-call
[qualification](../analysis/destruction-goal-20261022/flood-attribution-live-v7/admission.json)
also passes, together with both controls. The earlier effect-part-only observer
missed the projectile's direct area call. The probe observes original `D91D0`
entry/return, resolves the original caller's live projectile generation and EBP
body, and checks the target's full generation at its death store. That call's
projectile and definition match the recorded fuel-rod detonation; its damage tag
is the actual `flak_cannon\\damage_effects\\flak_explosion`. The
[bounded result](../analysis/destruction-goal-20261022/flood-clump-native-summary-v1.json)
includes sixteen exact world comparisons. The read-only hook preserves every
original instruction body. The complete older cohort still needs qualification
on this extended engine before a current broad cohort is claimed.

The [actual E89B0 CPU comparison](../analysis/destruction-goal-20261022/flood-damage-original-v2/report.json)
passes 28 unchanged original instructions and all 110,223,839 captured memory
bytes plus the ABI/control state. This covers the observed guarded return;
the dependent `1E9290` branch was not exercised. Its temporary reference banks
and executable outputs were [verified removed](../analysis/destruction-goal-20261022/flood-damage-original-v2/cleanup.json),
including the 112,111,372-byte capture. The cleanup's first attempt removed those
required paths, then reported an already absent failed-import output; the final
receipt verifies all three paths absent. Only bounded reports and hashes remain.

The [native model-contract census](../analysis/destruction-goal-20261022/native-model-contracts-v2/audit.json)
passes all 1,154 map/definition contexts across 39 paired map owners, 184 names
and 155 distinct original contracts. Each actual Cache model and delivered model
is checked against every field recorded in `contracts-v5`, with full salted
object/model/reference handles resolved through the actual 16-byte native index
rows. This closes the recorded model-input differences, including the Flood
clump. It does not cover unrecorded tag fields or replace behavior, presentation,
network, or structured native-kernel acceptance. Borrowed libraries and reference
maps were unchanged, and the audit's private build owner was removed. All 37
full-definition gates and D01-D13 remain open.

The attempted [whole-blast original-CPU comparison](../analysis/destruction-goal-20261022/flood-area-original-v2/boundary.json)
is unqualified. Unchanged original execution reaches `126139 -> 1A3B70`, then
reads the Xbox mutable queue global `569AF0`, which is absent from the native
capture. The exact frozen native owner uses its explicit animation file-I/O
boundary there: real bound resource bytes, a retained native read identity and
completion after the destination write. It does not assert Xbox asynchronous
queue scheduling. No queue state, original instruction or call result was
replaced to make the CPU run pass. The bounded signal/register journal identifies
this platform boundary; its 246,197 executed instructions are not counted as
matched instructions. Component CPU proofs and native blast attribution remain
valid within their stated scopes.

The next [focused extension](../analysis/destruction-goal-20261022/attached-emitter-native-v1/refresh.json)
keeps two setup failures visible. The original multiplayer Wraith variant has
only `mortar_mp`; its default variant declares a campaign mortar and two
miniguns. The [pinned original/MCC child lists](../analysis/destruction-goal-20261022/wraith-default-child-inputs-v1.json)
match. A second fixture uses original `B9960/B9B70` with that explicit named
default variant; the constructor must create both children and select the
recorded model variant. It supplies no child object or damage result. The
Sentinel-emitter input audit matches all [75 placements and the resident rocket launcher](../analysis/destruction-goal-20261022/campaign-sentinel-emitter-inputs-v2.json),
but the original fixed 12-unit lane search found no clear shot to the first
actual target. Its fixture now surveys the existing supported 2-12-unit distances
and floor layers. Both [revised cases](../analysis/destruction-goal-20261022/attached-emitter-native-v2/refresh.json)
ran without changing the engine's birth, damage or collision behavior for either
setup failure. The Sentinel emitter passes: two ordinary rockets, original death
and region change, and sixteen exact world comparisons. The admitted lane uses
the next supported floor layer. The default Wraith reaches a real missing
callback at actual `DC923 -> 105460`.

The complete original [23-instruction callback](../analysis/destruction-goal-20261022/wraith-minigun-105460-original-v1.json)
now has a structured native implementation. Its typed selection preserves the
original signed count/selector comparison and unsigned address wraparound; the
adapter preserves the caller's registers, integer flags, stack stores, ordered
writes and fault order. No new handle or index admission rule is introduced.
The earlier 145 original receiver instructions remain
[byte-identical](../analysis/destruction-goal-20261022/minigun-damage-body-preservation-v1.json).
The [56-case original-instruction comparison](../analysis/destruction-goal-20261022/minigun-record-original-controls-v1/report.json)
covers both branches, signed limits, wraparound, existing flag values, complete
ABI/control state, ordered stores and complete retained banks. The
[private wrong-bit control](../analysis/destruction-goal-20261022/minigun-record-original-negative-v1/report.json)
is rejected. Both private component build owners were removed.

The [actual live i386 comparison](../analysis/destruction-goal-20261022/minigun-record-live-original-v1/report.json)
also passes all 23 unchanged original instructions, 90,378,719 captured memory
bytes and the complete retained GP/SSE/x87/control state under the documented
host IF-bit mask. Its raw capture and oracle products, totalling 427,665,990
bytes, were [removed](../analysis/destruction-goal-20261022/minigun-record-live-original-v1/cleanup.json).
The [final generator reproduces the production source exactly](../analysis/destruction-goal-20261022/minigun-native-regeneration-v1.json).
These proofs qualify this native owner; they do not close the remaining semantic
conversion or complete vehicle gates.

The live minigun has an original full-generation death store and region change
while its global vitality remains 1.0. The regression therefore uses that actual
transition instead of requiring a vitality drop for this named case. An
initial attribution check correctly rejected equating the callback object with
the vehicle unit: [actual DC8EE/DC8FF](../analysis/destruction-goal-20261022/minigun-record-caller-original-v1.json)
loads the unit's selected weapon from `+212/+218`, and `105460` receives that
separate weapon. The [revised full-handle ownership check](../analysis/destruction-goal-20261022/minigun-selected-weapon-v6/refresh.json)
passes: the callback's separate live weapon handle matches the target unit's
actual selected-weapon slot. The source creates both default-variant miniguns;
four ordinary rockets lead to the recorded target death/region transition and
sixteen exact world comparisons. The current
[28-case cohort](../analysis/destruction-goal-20261022/current-native-full-cohort-v15/refresh.json)
passes all 28 cases and 464 exact world comparisons with both rate-kernel and
indexed-record diagnostics, including the module's actual pending-response
countdown. Direct damage versus parent/attachment cascade, complete sections,
parent state, occupants, debris and lifetime remain separate gates.

The failed whole-blast CPU investigation's 686,523,463 bytes of temporary banks
and duplicate oracle outputs were [verified removed](../analysis/destruction-goal-20261022/flood-area-original-v2/cleanup.json).
Its bounded fault journal, ABI metadata and memory hashes remain; no full-blast
original-CPU acceptance was inferred from that failed platform-boundary run.

The [default Wraith mortar extension](../analysis/destruction-goal-20261022/wraith-default-mortar-v1/refresh.json)
also passes: the original named-default constructor creates the single declared
mortar; four ordinary rockets cause its full-generation original death store
and region change, followed by sixteen exact continuation comparisons. The
minigun control passes again and is excluded from unique-case totals. The
[current evidence](../analysis/destruction-goal-20261022/current-native-summary-v16.json)
therefore contains 29 bounded cases and 480 unique full-world comparisons;
the [vehicle matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v11.json)
has 20 cases across 19 definitions. All 37 full-definition gates remain open.

The next native conversion is the complete original `D80A0` model-damage lookup,
canonically `D7160`. Its native route preserves the original low16 index masking,
null-object/model returns, signed positive damage-information count, read order,
registers and flags. It adds no generation admission rule. The older-image body
and other89 bodies remain byte-identical. The
[64-case unchanged-original comparison](../analysis/destruction-goal-20261022/model-lookup-original-controls-v1/report.json)
passes all original observation sites, ordered reads, full GP/SSE/control state
and retained memory banks. The
[private unsigned-presence mutation](../analysis/destruction-goal-20261022/model-lookup-original-negative-v1/report.json)
is rejected, and both [component build owners were removed](../analysis/destruction-goal-20261022/model-lookup-component-cleanup-v1.json).
The [native/SDL build and three live controls](../analysis/destruction-goal-20261022/model-lookup-native-v1/admission.json)
pass: Ghost, Wraith minigun and Flood clump, with 13,727 actual native model-lookup
calls and 48 exact continuation comparisons. Those repeated controls add no new
unique cases to the 29-case evidence above.

The [actual full-target Ghost lookup](../analysis/destruction-goal-20261022/model-lookup-live-original-v1/report.json)
matches all 24 unchanged original i386 instructions, 89,854,371 captured memory
bytes and retained ABI/control state under the documented host IF-bit mask.
Its 373,241,967 bytes of temporary capture/oracle products were
[removed](../analysis/destruction-goal-20261022/model-lookup-live-original-v1/cleanup.json).
The model lookup is admitted within this qualified frozen cohort; the complete
current destruction cohort still needs refresh after this native conversion.
The [existing observation regression](../analysis/destruction-goal-20261022/model-lookup-owner-regression-v1/report.json)
passes. Its four historical delivery/vitality replay tests skip because their
original fixture files are absent. Those skips add no original-execution proof;
the new 64-case oracle and actual live i386 comparison provide this conversion's
fresh execution evidence.
The [native-route regeneration check](../analysis/destruction-goal-20261022/model-lookup-native-regeneration-v1.json)
reproduces the qualified production module exactly and verifies that removing
only the native hook restores the complete existing tracked literal source.
The older whole-module observation generator rejects that tracked source's
pre-existing canonical hash; its guard was preserved. This result establishes
the bounded native-hook regeneration path, without asserting a fresh rebuild
of every literal body from the executable.

The guntower negative case uses the
[matched original/MCC scenario and resident rocket inputs](../analysis/destruction-goal-20261022/campaign-guntower-negative-inputs-v1.json).
Its [original DA0D0 frame and D80A0 call](../analysis/destruction-goal-20261022/guntower-model-rejection-producer-v1.json)
are authenticated from all 532 original dispatcher instructions. Acceptance
requires a real full-target damage request from the actual shooter/player,
the original dispatcher calling the native lookup on the same full generation,
an actual zero return for the authored absent damage-information block, unchanged
vitality/regions and exact continuation. Shots that miss without evaluating that
target cannot satisfy it. Its bounded acceptance now passes.

The retained guntower runs isolate two fixture constraints. Both placements have
supported cached floors, but every otherwise eligible center ray is blocked.
A survey of the unchanged authored bound admits a valid floor; subsequent aiming
uses the actual posed collision surface. Real rocket impact and explosion
requests then reach the full guntower target. An initial strict attribution check
identified another copy: native zero returns from `D835D` did not establish the
actual `DA0D0` damage graph's rejection. That graph now routes its own `D80A0`
copy through the same qualified native kernel. Its admission changes only that route
and the matching generator hook in the frozen source, preserving unrelated root
support work. The [four-case native/SDL admission](../analysis/destruction-goal-20261022/model-lookup-support-native-v1/admission.json)
passes with 64 continuation comparisons. Two ordinary guntower rockets produce
actual impact/explosion requests from the full shooter/player handles and two
full-target `DA180` calls returning zero. Vitality, regions and death state remain
unchanged. The [exact rejection capture](../analysis/destruction-goal-20261022/model-damage-lookup-live-original-v1/report.json)
matches all 23 original i386 instructions on this path and 116,576,675 captured
memory bytes. Its [480,131,175 temporary bytes were removed](../analysis/destruction-goal-20261022/model-damage-lookup-live-original-v1/cleanup.json).

The [complete 30-case cohort](../analysis/destruction-goal-20261022/current-native-full-cohort-v16/summary.json)
passes 496 unique complete-world comparisons, including the actual pending module
response. It records 117,039 native model-lookup calls and four indexed-record
calls. The [current summary](../analysis/destruction-goal-20261022/current-native-summary-v17.json)
and [vehicle matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v12.json)
separate 20 vehicle death/retirement cases across 19 definitions from the additional
guntower negative case. All 37 full-definition gates and D01-D13 remain open.

The paired direct-damage and vitality API copies now share the same kernel,
retaining their canonical observation PCs and byte-identical TU1.1 bodies.
[192 original comparisons](../analysis/destruction-goal-20261022/model-lookup-paired-original-controls-v2/report.json)
pass across delivery/direct/vitality routes; the private wrong-logic mutation is
[rejected through all three APIs](../analysis/destruction-goal-20261022/model-lookup-paired-original-negative-v2/report.json).
All 27 included headers [match the frozen runtime source](../analysis/destruction-goal-20261022/model-lookup-paired-header-closure-v1.json).
Their [native/SDL build and four integration controls](../analysis/destruction-goal-20261022/model-lookup-paired-native-v1/admission.json)
pass another 64 comparisons, excluded from unique totals. The TU1.1-only unit
module keeps its original addresses. The [native owner ledger](../analysis/destruction-goal-20261022/native-kernel-acceptance-v1.json)
tracks remaining damage, blast and collision conversions explicitly.

The [cleanup receipt](../analysis/destruction-goal-20261022/native-lookup-test-cleanup-v1.json)
verifies removal of the private source/build owner, four component owners and
raw entry captures. The [required source reference](../analysis/destruction-goal-20261022/source-reference-v1/retention.json)
retains a 65,768,291-byte compressed source snapshot (62.7 MiB), with source-only
qualification harnesses and existing reference symlinks. It preserves the exact
qualified frozen source independently of unrelated workspace drift. No compiled
test binaries are retained.

The native scalar material multiplier initially differed from original TU1.5
in four distinct-NaN controls. C++ had commuted MULSS's operands: original
`D81B1` puts the modifier in the destination and the accumulated product in the
source. The shared native product now retains those roles. The
[77 original i386 controls](../analysis/destruction-goal-20261022/material-multiplier-hardware-v1/report.json)
establish the failure independently of the native code. Emulator-only FP status
and one NaN result disagreed with actual original i386 execution; the CPU results
are the scalar authority for these controls.

The complete `D8110` owner now uses two native loops, preserving its original
two-group/two-armor order, stack and register effects, observation sites and
existing complete search/comparator calls. The
[fresh original comparison](../analysis/destruction-goal-20261022/material-native-owner-original-v2/report.json)
and [exact frozen-source qualification](../analysis/destruction-goal-20261022/material-native-frozen-owner-v1/report.json)
pass 77 cases through each observer API: 154 owner comparisons, each API checking
18,528 instruction boundaries, 11,900 ordered access addresses/sizes and 2,838,528
final memory bytes. Final GP/SSE state and FP controls/status match the original
CPU. Intermediate FP access bytes, invalid-memory fault paths and other numerical
backends remain outside this qualification. The
[private operand-reversal mutation](../analysis/destruction-goal-20261022/material-native-frozen-negative-v1/report.json)
is rejected by both owner APIs and the scalar kernel. These component controls
do not increase the unique world-case count.

The [native route guard](../analysis/destruction-goal-20261022/material-native-platform-guard-v1.json)
limits the new route to its qualified GNU x86 backend and produces exactly the
same preprocessed source as the frozen build. The
[route regeneration check](../analysis/destruction-goal-20261022/material-native-route-generation-v2.json)
preserves the previous complete module and both native hooks independently.
The search and comparator still require their separate semantic conversions.

The [authored material census](../analysis/destruction-goal-20261022/material-authored-inputs-v2/report.json)
resolves shared-cache globals across 39 original/MCC contexts. Named multiplier
bits and existing material parent/armor bindings agree. Physical table ordering
differs between StringID namespaces; 26 original contexts have 182 material rows
where MCC appends `tough_inorganic_plastic_container` as row 183. The
[native Cache/delivery audit](../analysis/destruction-goal-20261022/material-native-inputs-v2/audit.json)
passes all 39 contexts and 55,692 production scalar lookups using every actual
raw group/armor key. Delivered metadata matches Cache inputs, and recorded
original bindings match by name. This qualifies these inputs and lookups;
weapon damage inputs, other material fields and complete gameplay remain open.

The [component cleanup](../analysis/destruction-goal-20261022/material-native-component-cleanup-v1.json)
records removed private builds and raw banks. Its required original owner
regression reference is 21,400 compressed bytes, with no compiled test binaries.

The first full native build reached final link but
[failed before gameplay testing](../analysis/destruction-goal-20261022/material-native-world-v2-cleanup.json):
the source-only archive had omitted the required NVIDIA SDK archive. Its private
owner was removed. Preparation now verifies all eight frozen SDK manifest
entries and restores the four omitted SDK inputs as read-only references to the
port-owned files. The [SDK link preflight](../analysis/destruction-goal-20261022/material-required-sdk-preflight-v1.json)
passes. The [finite owned session](../analysis/destruction-goal-20261022/material-native-session-v2/session.json)
rebuilds the exact material source overlay independently of concurrent workspace
changes. Its [native/SDL admission](../analysis/destruction-goal-20261022/material-native-world-v4/admission.json)
and [complete existing cohort](../analysis/destruction-goal-20261022/material-native-full-cohort-v2/summary.json)
pass: 30 destruction cases, 496 complete-world comparisons and 560 actual native
material lookups, including the module's pending-response replay. All three
requested native build targets and all 31 CTests pass. The declared source and
SDK references remain unchanged during that validation. The
[13 runtime cache inputs](../analysis/destruction-goal-20261022/material-native-runtime-inputs-v1.json)
are bound to those 30 cases. These are scoped frozen-source results; the
launcher-selected runtime has not been replaced.

The [actual native material entry](../analysis/destruction-goal-20261022/material-live-original-v1/qualification.json)
now passes an unchanged-original CPU comparison. An ordinary Ghost gameplay
call resolves one real group and modifier through the native D8110 owner.
[Original Linux i386 execution](../analysis/destruction-goal-20261022/material-live-original-v1/actual-entry/original/report.json)
executes 582 instructions and matches final GP/SSE/x87/FP state and all
89,854,371 accessible captured memory bytes (with the declared host interrupt
flag comparison mask). This is one actual owner call, not Xbox hardware or
complete destruction acceptance. The separate probe's libraries and headers
remain unchanged. Its temporary build and 374,396,882 bytes of raw capture and
oracle products are [removed](../analysis/destruction-goal-20261022/material-live-probe-cleanup-v1.json).

Two remaining zero-information vehicle definitions are the Pelican rear gun and
rocket pod. The [authored New Mombasa inputs](../analysis/destruction-goal-20261022/pelican-child-constructor-inputs-v1.json)
select `unarmed` for all four Pelican placements and therefore create neither
child. The [declared authored `attack` constructor variant](../analysis/destruction-goal-20261022/pelican-attack-child-constructor-inputs-v1.json)
matches original/MCC data and contains two rocket pods at `hardpoint_left` and
`hardpoint_right`, plus the rear gun at `rear_gun_root`. Each child model has zero
damage-information rows. The [native constructor regression](../analysis/destruction-goal-20261022/pelican-native-construction-v8/probe.json)
creates that parent and all three actual child handles, then passes 16 complete
world comparisons before construction and 16 afterward. Its constructor position
is a declared observer-relative input, using the existing isolated vehicle-variant
fixture's supported authored observer survey.

That scene exposed a [campaign death/respawn readback regression](../analysis/destruction-goal-20261022/pelican-native-construction-v6/validation/evidence/pelican-children.json):
the original player row and first-person owner held the same newly created
Master Chief while the native match retained NONE. The
[native readback correction](../analysis/destruction-goal-20261022/source-player-readback-fix-v1.json)
adopts completed source units after campaign source frames as well as canonical
multiplayer frames. The passing regression requires actual old-unit depletion,
a different live full-generation unit and agreement between all three bindings.
The candidate uses the original compiler flags and a verified replacement object;
all borrowed engine libraries remain unchanged. Every separate probe build is
[removed](../analysis/destruction-goal-20261022/material-live-probe-cleanup-v3.json).
The [standard native/SDL rebuild and expanded cohort](../analysis/destruction-goal-20261022/material-readback-native-world-v1/admission.json)
pass all 32 CTests, including that constructor/death/respawn regression. The
existing 30 destruction cases still pass their 496 complete-world comparisons.
Source, SDK references and all 13 runtime cache inputs remain unchanged during
validation. Complete definition parity remains open.

The [Pelican collision audit](../analysis/destruction-goal-20261022/pelican-collision-inputs-v1.json)
matches original/MCC collision references, region/permutation mappings and raw
BSP geometry hashes for the parent and both child definitions. The rear gun has
no collision tag; its collision-region and permutation mappings are NONE. The
rocket pod has 18 authored collision surfaces. Missing collision does not imply
an explosion-damage admission result.

The [attachment radial probes](../analysis/destruction-goal-20261022/pelican-damage-routing-native-v4/probe.json)
pass both target assertions using one shared attack-family stimulus. Ordinary
source rockets hit the parent's actual collision near a pod. The original
projectile's D91D0 call reaches each requested full child generation through
DA830. Its native D80A0 lookup returns zero at DA875; authenticated original
[DA898 then branches to DAC59](../analysis/destruction-goal-20261022/pelican-area-model-gate-original-v1.json),
before the lower direct dispatcher. Children retain their actual generation,
parent, vitality bits, flags and permutations while the parent takes real damage.
Each assertion passes 16 coupled-world replay comparisons. These two assertions
share the same family scene and do not increase the recorded unique-world count.

Both [actual child lookups](../analysis/destruction-goal-20261022/pelican-radial-model-original-v1/probe.json)
also pass unchanged-original Linux i386 execution: 23 instructions and
116,576,675 compared accessible bytes per captured call, including final
GP/SSE/x87/FP state under the declared host interrupt-flag mask. This is native
owner evidence, not Xbox hardware or a full original blast replay. Separate
probe builds and 960,439,680 bytes of raw original comparison products are
[removed](../analysis/destruction-goal-20261022/pelican-radial-probe-cleanup-v1.json).
The [34-test native/SDL expansion](../analysis/destruction-goal-20261022/pelican-radial-native-world-v1/admission.json)
passes all 34 selected CTests, the native game and SDL targets in the same
finite owner. The existing 30-case cohort retains its 496 complete-world
comparisons and 560 material calls. Direct-collision recipient behavior,
remaining damage producers, parent destruction, occupants, lifetime, normal
presentation and transport remain open in the
[37-definition matrix](../analysis/destruction-goal-20261022/vehicle-acceptance-matrix-v15.json).

The [Pelican parent regression](../analysis/destruction-goal-20261022/pelican-parent-native-world-v1/admission.json)
passes in the standard native/SDL build and all 35 selected CTests. Six ordinary
source rocket births, with one declared replacement through original equipment
owners, deplete the actual parent and produce its full-generation original death
flag. Its exact contract has zero instant responses; no extra death effect is
supplied. The parent passes 16 complete-world continuation comparisons and
executes 19 native material calls. The existing 30 cases retain their 496
comparisons and 560 material calls; constructor and shared-family attachment
assertions remain separately scoped.

That replacement exposed a [retirement-readback regression](../analysis/destruction-goal-20261022/source-retirement-readback-fix-v1.json):
original equipment removal retired launcher handle 3849323261 (actual slot salt
zero), while native bookkeeping still retained its owner. The native frame now
observes completed source retirements after individual player/pickup cleanup.
It calls no destructor and changes no source pool, damage, ammo or physics
result. Strict generation validation remains unchanged. The negative diagnostic
reproduces the stale owner; the candidate and standard cohort pass. All six
separate parent probe builds are [removed](../analysis/destruction-goal-20261022/pelican-parent-probe-cleanup-v1.json).

The [Sentinel Enforcer probe](../analysis/destruction-goal-20261022/sentinel-enforcer-native-v2/probe.json)
authenticates its current original/MCC model contract, creates the authored
scenario vehicle through the original runtime API and finds a supported firing
lane. Its next original object frame stops before firing at missing owner
`DC540`, called from `DC660` at `DCD72`. The [fresh original inventory](../analysis/destruction-goal-20261022/enforcer-child-owner-original-v1.json)
contains 88 instructions and matches the earlier canonical owner. The [native component qualification](../analysis/destruction-goal-20261022/model-children-native-original-v2/report.json)
passes 54 controls: 45 execute unchanged original i386, while nine test 17
callback admissions with an explicit clobber-only fixture. All 6,114 instruction
boundaries, 3,084 ordered memory accesses and 2,433,024 final bank bytes match.
The [bit26-bypass mutation](../analysis/destruction-goal-20261022/model-children-native-negative-v1/report.json)
is rejected by 14 controls. Callback fixtures do not qualify D9110 gameplay.

The [isolated Enforcer candidate](../analysis/destruction-goal-20261022/sentinel-enforcer-native-v3/probe.json)
now passes: two source rockets deplete the parent, produce its original death
flag and region changes, execute the native child owner twice and pass 16
complete-world comparisons. One [actual child-owner call](../analysis/destruction-goal-20261022/model-children-live-original-v1/probe.json)
also matches unchanged original Linux i386 execution: 108 instructions and
125,804,963 captured accessible bytes, with final GP/SSE/x87/FP state and the
declared host interrupt-flag mask. This is neither Xbox hardware nor a complete
original destruction sequence. The actual call's 518,191,210 bytes of raw
comparison products are removed. The native route calls the existing original
D9110 owner and supplies no child damage or retirement result. The
[36-test native/SDL expansion](../analysis/destruction-goal-20261022/model-children-native-world-v1/admission.json)
passes all 36 selected CTests and the native game/SDL targets. The existing
30-case cohort still passes 496 comparisons and 560 material calls; parent and
Enforcer each add 16 separately recorded comparisons. Source, SDK references
and all 13 runtime cache inputs remain unchanged. All 37 complete-definition
gates remain open.

The [creation-input audit](../analysis/destruction-goal-20261022/vehicle-creation-inputs-v3.json)
now identifies authored definition or model-child contexts for all 37 vehicle
identities across six paired maps. It keeps group identities explicit and reads
shared MCC metadata from its authenticated owner. Variant names, actual child
references/markers and constructor fields match. One Scarab scenery placement
has original storage row 155 and MCC row 135, with identical other fields and
unique ID 487326136. That correspondence is retained explicitly; runtime numeric
row translation remains unqualified. This is metadata evidence, not proof of
all original creation, destruction or transport paths. The earlier two audit
iterations are superseded diagnostics for group identity and shared metadata.

The absorber investigation encounters an existing current-checkout BV-agent
owner that is absent from the older frozen source. Its [frozen-header requalification](../analysis/destruction-goal-20261022/bv-agent-frozen-component-v1/report.json)
regenerates the current compiled source exactly from original instructions and
passes the recorded original hardware-derived witness: 453 instruction
boundaries, 83 ordered stores, 32,808 final bank bytes and full x87/MXCSR state.
This is reuse of a compiled original owner, not a newly qualified semantic
kernel. Unrelated current light/fade table changes are excluded.

The [isolated absorber candidate](../analysis/destruction-goal-20261022/plug-absorber-native-v2/probe.json)
resolves that missing owner and observes 16 actual BV entries. Twelve ordinary
source rockets deliver twelve full-target explosion requests. The shield
reaches zero and damage flags become 8; global body remains 1 and the parent
stays live. The current death-only assertion rejects that outcome. The original
section/body coupling still needs direct qualification before this result can
be classified; no immunity, section health, death or extra response is supplied.
