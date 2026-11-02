# Vehicle destruction and explosive interactions

Status: active, 2026-11-01. This ledger covers the user's complete vehicle
destruction and explosive/damageable-material interaction request. A component
test, one destroyed vehicle, or an asset inventory does not complete the goal.

The renewed goal explicitly includes every authored vehicle destruction sound:
partial damage, explosions, detached parts, wreck impacts and looping/stop
transitions. Correct source requests must reach actual native PCM/device output
with original timing, owner, marker position, variation, gain and pitch, without
duplicate emission across local views or accepted LAN updates. An authored NONE
remains silent. Sound inventory and native arithmetic tests alone do not close
this audible-delivery requirement.

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
| D05 | Pending responses, delayed effects, model-region/permutation changes and subsequent collisions | Native countdown, query/eligibility and model-state kernels have original-code qualification plus eight current MCC cases/144 world comparisons; remaining responses and lifecycles stay open | [ ] |
| D06 | Destruction damage/effects use current marker/world location, source owner and strength | Original `DC660 -> DC3C0 -> D91D0`; verify real requests rather than reconstructing events from names | [ ] |
| D07 | Explosive-to-explosive chains, multiple simultaneous/delayed blasts and reciprocal vehicle/prop damage | Native module chain and both vehicle/coil directions pass with pristine witnesses and distant controls; simultaneous/delayed families remain | [ ] |
| D08 | Explosives affect nonexplosive damageable scenery, machines, equipment and breakable BSP materials | Terrain bit/collision removal, coil-to-scenery and coil-to-crate pass; other materials and fragments remain | [ ] |
| D09 | Blast candidate selection, obstruction, falloff, directional/stochastic branches and physical impulse | Actual original radius/per-target owners; a health reduction does not establish impulse or obstruction | [ ] |
| D10 | Detached parts, constraints, debris births, solved motion, wreck collision, occupant exit/death and object retirement | Historical vehicle/debris evidence is bounded; current MCC path and every authored transition remain required | [ ] |
| D11 | Multiplayer state, late join, replay/takeover and repeated snapshots preserve destruction and chains | The qualified native cohort has 30 bounded cases and 496 full-world comparisons; earlier SDL Warthog destruction passes 1,745 exact LAN snapshots; late join, pending transport and takeover remain | [ ] |
| D12 | Visible damage states, explosions, debris and authored audio reach normal single/split/LAN consumers | Native-rate SDL inspection shows the explosion, overturned wreck and a detached ground part in the actor view; complete original permutations, all debris identities, framebuffer matching and audio remain open | [ ] |
| D13 | Native kernels preserve one-to-one TU1.5 destruction, blast and collision behavior | Rate, indexed-record, model lookup/child selection, material, pending countdown/eligibility and model-region kernels have scoped original-code qualification; remaining damage, blast and collision semantic conversions stay open | [ ] |

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

## Renewed native and audio qualification, 2026-11-01

The active goal now explicitly includes all authored destruction sounds. The
material damage route replaces the complete `1408B0` table search and `D8100`
key comparator with native kernels inside `D8110`. It preserves midpoint
duplicate selection, signed wrapping key subtraction, actual callback frames,
original read/store order, full ABI and both observation APIs. Other callers
and older images retain their literal paths.

The [original CPU comparison](../build/test-results/vehicle-destruction-native-20261031/material-search-regenerated/report.json)
passes all 77 cases through both APIs: 154 full owner comparisons. Each route
executes 266 native searches and 366 native comparisons and matches 18,528
instruction boundaries, 11,900 ordered memory accesses and 2,838,528 final bank
bytes. Final GP/SSE/flags and FP controls/status match the retained independent
original i386 results. Intermediate FP memory-access bytes and invalid-memory
faults remain outside this component evidence. These are declared component
inputs, not complete original gameplay or Xbox hardware destruction sequences.

The [pre-conversion regression](../build/test-results/vehicle-destruction-native-20261031/material-search-red/report.json)
rejects both routes for missing native search/comparator execution. The
[private reversed-search control](../build/test-results/vehicle-destruction-native-20261031/material-search-negative/report.json)
is rejected by 66 cases per API while the independent scalar result still
passes. Original references and production source are not mutated by that
control. Regeneration authenticates all 90 original routines and preserves
their literal bodies; [regeneration matches current source byte-for-byte](../build/test-results/vehicle-destruction-native-20261031/route-regeneration-final.json)
and native-hook installation is idempotent.

The [focused destruction sound inventory](../build/test-results/vehicle-destruction-native-20261031/destruction-audio-presence.json)
finds all 13 referenced sound banks and one looping definition across all 37
vehicle identities and 38 original map contexts in the prepared MCC import.
The audit now includes actual resident shared-cache names and the explicit
`highplains2`/`highplains` filename correspondence. It follows original model
damage and section-response roots rather than assuming the whole weapon graph
is a destruction event. Spawned-object and implicit material/global producers
remain separate. This proves namespace availability only: event timing,
variation, PCM/device delivery, spatial behavior and audible original parity
are still required before D12 can close. The broader weapon/vehicle census
retains three exact-name projectile flyby/material differences separately; no
sound is guessed or substituted to make that census pass.

The [native decoder and Classic eligibility check](../build/test-results/vehicle-destruction-native-20261031/destruction-audio-pcm-classic.json)
decodes all 13 banks through the explicitly pinned launcher-selected engine
`1f04e6a955854e238689fbd26d53dfa8`. Each default pitch/permutation produces
complete non-silent stereo PCM. Its temporary WAVs are deleted immediately;
only counts, peaks, hashes and commands are retained. This is sample/resource
evidence from that installed engine, not delivery of the new material kernels,
all permutations, actual destruction cue scheduling or speaker-output parity.
The [37-definition audio matrix](../build/test-results/vehicle-destruction-native-20261031/vehicle-destruction-audio-matrix.json)
keeps those remaining runtime and audible gates explicitly open.

The [first combined current-source run](../build/test-results/vehicle-destruction-native-20261031/current-world/validation/report.json)
passes ordinary-input Warthog, Gauss Warthog, Banshee, Scorpion, Wraith and
Spectre destruction, including 16 complete-world continuation comparisons per
case. Ghost reaches its original dead flag and creates three debris objects,
but rejects its following native execution phase. Nine additional failures are
unbuilt test executables selected by the harness; the audio probe also fails to
link its optional rendered fixture. The run is **failed**, not full acceptance.
The harness now builds its selected reference/cache targets and registers the
PCM probe directly with the real native libraries. Its optional rendered
fixture includes the actual multiplayer runtime implementation.

One [actual live material call](../build/test-results/vehicle-destruction-native-20261031/live-material-original/qualification.json)
from that frozen engine matches unchanged original i386 execution across 582
instructions and 89,871,527 accessible memory bytes. Raw capture/oracle products
and the entire native build are removed. This proves that particular material
entry and return, not the full original destruction timeline.

Audio investigation also exposes a native listener-scale composition defect:
`253070` returned its scalar result while omitting volatile GP/SIMD results used
by its object-loop parents. The adapter now publishes the original results from
the same observed reads and FP evaluation. The [fresh original CPU and parent checks](../build/test-results/vehicle-destruction-native-20261031/native-audio-scale-registers/report.json)
pass, followed by [all eight native sound-kernel suites](../build/test-results/vehicle-destruction-native-20261031/native-audio-kernels-final/report.json).
No comparison mask is weakened. These source qualifications do not install or
promote a runtime, prove actual destruction cue scheduling, or close D12/D13.

The [expanded fragment/particle audit](../build/test-results/vehicle-destruction-native-20261031/destruction-audio-particles-presence.json)
supersedes the 13-bank inventory's graph boundary. Original `bloc`, `prt3` and
`PRTM` definitions have their own creation, collision and death references.
Following these finds **31 sound banks and seven loop definitions**, all present
across the same 37 vehicles/38 map contexts. The additional cues include falling
metal, hubcaps, glass/rocks, vehicle impacts and Covenant damage loops. This
repairs an audit omission; it does not establish that those cues previously
failed at runtime. Implicit global/material producers and all live loop/stop,
selection, spatial and audible-parity gates remain open.
All 31 default samples also pass the [pinned Classic decoder check](../build/test-results/vehicle-destruction-native-20261031/destruction-audio-particles-pcm-classic.json).
The [expanded per-vehicle matrix](../build/test-results/vehicle-destruction-native-20261031/vehicle-destruction-audio-matrix-expanded.json)
records that resource evidence while keeping actual event/device/loop lifecycle
and original audible parity unverified.
The [seven actual loop-controller inputs](../build/test-results/vehicle-destruction-native-20261031/destruction-loop-policy-inputs.json)
retain their resident MCC owners, flags and track references. Gasoline fire and
tiny Covenant damage loops request combined 3D playback; metal and hubcaps have
alternate tracks. Their presence and non-silent samples do not qualify those
controller profiles or source stop/retirement behavior.

The [27-test integrated correction run](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/validation-impulse-fix/report.json)
passes all seven multiplayer vehicle cases, 112 complete-world continuation
comparisons, strict execution-input guards, reference/cache contracts and the
expanded sound probe. The latter [decodes all 273 authored pitch/permutation resources and delivers 93 native PCM cases](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/validation-impulse-fix/evidence/destruction-audio-native.json)
across one/two/four coincident listeners, with non-silent finite output, no
duplicate packet, finite voice retirement and silent NONE. Shared resources
resolve through actual multiplayer/campaign scenario owners for immutable
native metadata publication. These are declared sound requests through manual
SDL output, not original event scheduling, physical speaker capture or all
seven loop controllers.

The Ghost [failure trace](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/observer-stack-functions.txt)
identifies the completed `1D94CE -> 324F80` impulse publication callback. Its
read-only presentation copy incorrectly incremented the Match mutation counter,
invalidating capture. The callback now retains its authority staging/call
boundary while the enclosing MatchStep commits the copied impulse journal
after the source returns. Public mutation accounting and read-only observer
rejection remain intact. Ghost passes 16 continuation comparisons and retains
32 impulse events, with serial 27 advancing to 35 during the captured steps;
no source impulse or publication result is supplied or discarded. A final
stable-source relink and [28-test final run](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/validation-final/report.json)
also pass. The [rendered SMG/Warthog regression](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/validation-final/evidence/destruction-rendered-smg.json)
completes tick 1800 with actual ordinary input, damage/effect progression,
337 valid SDL packets (138 SMG, two reload, 131 ambient), zero invalid packets,
and native shadow submissions. The SDL driver is dummy: physical speaker output
and all destruction cue/loop timelines remain unverified.

The [final scope and cleanup receipt](../build/test-results/vehicle-destruction-native-20261031/current-world-repair/acceptance.json)
pins the compiled collector, current qualified files, effective private
configuration/diagnostic overlays and both unchanged sound resource archives.
All seven cases retain their 112 complete-world comparisons; the final sound
probe retains 273 resource decodes and 93 listener cases. The enclosing storage
supervisor removes the 3,929,160,346-byte source/build owner and every temporary
frame, WAV, executable and archive. No large artifacts are retained and no
runtime is promoted. The complete 37-definition/original-audible goal remains
active.

## Destruction loop controllers, 2026-11-01

The [seven-profile playback regression](../build/test-results/vehicle-destruction-loops-20261101/baseline/evidence/destruction-loops.json)
reproduces four missing native consumer paths: metal and hubcap alternate
tracks, plus gasoline-fire and tiny Covenant combined-3D loops. The ordinary
small/medium/large Covenant loops already start, deliver PCM and stop. These
are declared spatial requests with actual MCC metadata/resources, not proof
of source destruction event timing or original speaker output.

Unsynchronized spatial alternate tracks now use the original-qualified phase
decision kernel and actual native voice rings. Creation/refill use the common
RNG and shared pitch-range no-repeat state; synchronized profiles retain their
separate owner. The [fresh original policy probe](../build/test-results/vehicle-destruction-loops-20261101/original-random-ownership-repeat.json)
executes 52 unchanged instruction sites across 48 creation/frame/range-ownership
cases and repeats exactly. Only the outside `21FED0` read-only range lookup
receives a declared component mask. The fresh 1,200 original track-decision
cases also equal the retained reference; no source voice, admission result or
driver-ready byte is supplied.

The [seven focused regression suites](../build/test-results/vehicle-destruction-loops-20261101/qualified-controls/report.json)
pass original transition/decision and ring checks, actual metal/hubcap
normal-to-alternate-to-normal playback and stop, exact PCM/permutation suffixes
after repeated restores, common RNG/shared-range ownership across two full
emitter identities, and the existing borrowed Effects writer-agreement guard.
Both actual banks reject the [private wrong shared-mask reset](../build/test-results/vehicle-destruction-loops-20261101/negative-range-mask/evidence/destruction-alternates.json).
Production source and original references are not mutated by that control.
The 31-bank probe still decodes all 273 pitch/permutation resources and passes
93 one/two/four-listener PCM cases. The Banshee packet-phase and Audio owner
preflight regressions also pass.

The [full seven-profile result remains failed](../build/test-results/vehicle-destruction-loops-20261101/full-scope-qualified-fixture/evidence/destruction-loops.json):
five profiles pass, while gasoline fire and tiny Covenant damage still require
the original combined-3D aggregation owner. Neither path is bypassed or treated
as complete. The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-loops-20261101/qualification.json)
pins the tested source and original sound archives. Its 890,906,915-byte owned
source/build tree is removed; no large artifacts are retained or runtime
promoted. Live destruction scheduling, aggregation, spatial/audible comparison
and the full 37-definition goal remain open.

## Combined-3D destruction contribution kernel, 2026-11-01

Gasoline fire and tiny Covenant damage require one original aggregate owner,
with four independent speaker gains. The new semantic
`source_loop_combined_mix_tu15` component preserves the original minimum-distance
floor, complete `198D80` attenuation, `30BD0` normalization, four-speaker
`198CF0(flag1) -> 198BA0` calculation and ordered `220EA9..220F21` accumulation.
The separate reset operation compares the actual byte frame counters. Above
unit summed power the original uses reciprocal power, not reciprocal square
root power; clearing on each emitter would also discard other same-frame
contributions.

The [fresh original reference](../build/test-results/vehicle-destruction-combined-20261101/original-mix.json)
executes 215 unchanged instruction sites without substituted calls. Its 512
finite-input cases cover four rounding modes, cardinal/diagonal/coincident and
subnormal points, repeated contributions, frame resets and byte-counter wrap.
The [independent repeat](../build/test-results/vehicle-destruction-combined-20261101/original-mix-repeat.json)
is byte-identical. Native results match every retained intermediate and final
float bit; the component restores the host floating-point environment.
Private [square-root normalization](../build/test-results/vehicle-destruction-combined-20261101/negative-square-root/tests.log)
and [per-emitter reset](../build/test-results/vehicle-destruction-combined-20261101/negative-frame-reset/tests.log)
controls are rejected in 20 and 189 cases respectively. Their source refresh
hashes and rebuild logs are retained; production and original inputs are not
mutated by these controls.

This component accepts an already selected listener's local point. It neither
selects a listener nor claims the parent GP/register/status ABI, source admission,
aggregate identity/stop lifecycle, `2216D0` gain publication or speaker routing.
The current consumer still rejects the combined profile, and the SDL backend
still exposes explicit stereo routes. Both complete combined-loop playback
gates remain open. Connect actual listener orientation/cluster ownership, the
shared source owner and original four-bin output policy before admitting these
profiles; duplicating per-emitter voices or guessing a stereo matrix would not
qualify them. No D01–D13 checkbox is closed by this numerical component.

The [final eight-suite run](../build/test-results/vehicle-destruction-combined-20261101/final-controls/report.json)
passes this component with exact caller x87-control/MXCSR restoration, original
track transitions/decisions and ring checks, metal/hubcap alternate playback,
all 273 resources and 93 listener cases across 31 banks, Banshee packet phases
and Audio owner preflight. The fresh [full loop check](../build/test-results/vehicle-destruction-combined-20261101/full-scope/evidence/destruction-loops.json)
still passes five of seven profiles and rejects the two combined profiles.
The [qualification and cleanup receipt](../build/test-results/vehicle-destruction-combined-20261101/qualification.json)
pins the effective compiled source, source refreshes and bounded diagnostics.
The shared top-level CMake file changed after the freeze; its qualified frozen
hash is retained, and the numerical component files still match. The enclosing
owner removes its 893,269,327-byte source/build tree. No large artifacts are
retained and no runtime is promoted. Original destruction scheduling, aggregate
playback/routing, physical speaker output and the complete goal remain open.

## Combined-loop gain publication, 2026-11-01

The normal native sound registry now owns complete `2216D0`. It validates the
actual parent's full signed generation, pool limit and definition-wide
`definition | 0x6000` key before reading sound metadata or touching output.
For each of the four accumulated gains it executes the native gain conversion,
authored gain control and class ducking, adds the parent gain in source order,
and publishes the four fields and combined-output flag. The world retains a
separate authenticated 421-instruction literal closure for reference/oracle
mode; the existing 353-instruction scalar-gain reference remains unchanged.

The [complete original component capture](../build/test-results/vehicle-destruction-combined-owner-20261101/original-publication.json)
and [unchanged i386 CPU qualification](../build/test-results/vehicle-destruction-combined-owner-20261101/original-publication-hardware-guards.json)
cover 320 cases, including 120 accepted parents and 200 NONE, stale-generation,
zero-generation, range and wrong-key rejections. The physical reference makes
the rejected cases' metadata/output pages inaccessible during the call. Native
and literal implementations match every GP/SIMD output, FP control/status bit,
ordered caller/output store and final output byte. All 68 publisher instruction
sites execute; its closed helper paths contribute 344 original sites overall.
An independent repeat reproduces the physical results and reference ELF.

Unicorn omits inexact exception flags in the 120 successful cases. The initial
FP-only diagnostic is retained; the final fixture uses the unchanged original
instructions' physical CPU flags. Original instructions, callee results,
admission flags and native expected output are not supplied to that reference.
Private [generation-check removal](../build/test-results/vehicle-destruction-combined-owner-20261101/negative-generation/tests.log)
faults at the protected metadata read, and [combined-output flag removal](../build/test-results/vehicle-destruction-combined-owner-20261101/negative-route-flag/tests.log)
changes 40 cases. Both private mutations are restored and their refreshed
objects rebuilt; production inputs and original evidence remain intact.

The [eleven-suite regression](../build/test-results/vehicle-destruction-combined-owner-20261101/final-qualified/report.json)
passes the new publisher, 48,000 existing native gain comparisons, the
[94 successful/three faulting physical scalar-gain cases](../build/test-results/vehicle-destruction-combined-owner-20261101/final-qualified/evidence/sound-gain-acceptance/report.json),
the combined-contribution/phase/ring tests, 273 resources across 31 banks and
93 listener cases, Banshee phase packets and Audio owner preflight. The source
world routing adapter and retained reference compile in the frozen audio owner
closure. The initial broader run selected system Python without Capstone;
its diagnostic is retained, and the qualified run explicitly selects the
repository's dependency-complete Python.

The [full loop check remains five of seven](../build/test-results/vehicle-destruction-combined-owner-20261101/full-scope/evidence/destruction-loops.json).
Gasoline fire and tiny Covenant damage still need their shared controller,
actual listener-local inputs and four-bin device routing connected to normal
native playback. This gain caller does not establish live invocation from a
destruction event, physical speaker output or complete audible parity. The
complete goal and D01–D13 remain open.

The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-combined-owner-20261101/qualification.json)
pins the effective source, regeneration, physical reference and negative
controls. The shared CMake/world files acquired separate physics changes after
the freeze; their qualified hashes and the bounded world diff are retained.
The audio publication implementation/reference/fixture files still match.
All ten recorded scratch owners are absent, including the 905,599,872-byte
source/build owner and every original-reference ELF/raw image. No large
artifacts are retained and no runtime is promoted.

## Combined-loop mix-bin routing, 2026-11-01

The native sound registry now also owns the complete original `2B9450`
routing-plugin callback. It preserves the four separate gain fields, appends
the original dry bins in order (`6,7,8,9`, or `0,1,4,5` with bypass enabled),
and retains the class sends to bin10. Class0 appends one send; class1 appends
two entries to that same bin using the plugin's authored send gain. Other
class categories append no send. The native kernel preserves that duplicate
entry and the original normalized/clamped driver-volume conversion rather
than combining routes or assigning one volume to all four directions.

The static caller trace identifies the original connection: the owner table
at `455B68` exposes `2216D0` at offset `0C`; `12CA12` invokes that callback
and copies its successful 56-byte parameter publication. Driver `226490`
builds the combined routing request and calls the plugin table at
`4633FC+18`, whose entry is `2B9450`. Stream/buffer submission goes through
`227FE0`/`228100`. This trace establishes the source call sites, not that
normal native destruction playback has traversed the entire chain.

The [original instruction capture](../build/test-results/vehicle-destruction-combined-routing-20261101/original-routing.json)
and [unchanged original i386 CPU run](../build/test-results/vehicle-destruction-combined-routing-20261101/hardware-routing.json)
cover 384 complete calls: both bypass states, six class categories, existing
route prefixes, floor/ceiling/out-of-range gains and all four rounding modes.
All 178 callback instruction sites execute. The separate retained literal
reference recovers its complete 212-instruction closure with `12DB00`.
Native and literal results match full GP/SIMD state, FP control/status, ordered
output/caller stores and final backing bytes. For 320 calls the plugin page
is inaccessible, verifying that only class1 reads its send-gain field. An
[independent repeat](../build/test-results/vehicle-destruction-combined-routing-20261101/original-repeat-check.json)
reproduces every result, instruction byte and original reference ELF.
As with the gain publisher, Unicorn omits inexact FP flags; the fixture keeps
the unchanged original CPU's measured flags instead of clearing native flags.

The [final twelve-suite regression](../build/test-results/vehicle-destruction-combined-routing-20261101/final-restored-clean/report.json)
passes the new routing callback, the publisher, existing scalar/native gains,
combined contributions, phase/transition/ring controllers, actual destruction
PCM resources, alternate playback, Banshee phase packets and Audio preflight.
The native world dispatch adapter and literal fallback compile in the frozen
audio closure. Private wrong-bin and collapsed-send-count controls are
recorded with restoration in the
[negative-control receipt](../build/test-results/vehicle-destruction-combined-routing-20261101/negative-controls.json).
The retained [native test output](../build/test-results/vehicle-destruction-combined-routing-20261101/final-restored-clean/evidence/Testing/Temporary/LastTest.log)
includes all original comparison counts. A repeated scalar-gain validator
initially refused its existing scratch output; the previous bounded reference
report was authenticated and preserved before removing only that generated
private output. The final fresh run passes. The
[full loop run](../build/test-results/vehicle-destruction-combined-routing-20261101/full-scope/evidence/destruction-loops.json)
still passes five of seven profiles and rejects both combined profiles.

This does not close the shared gasoline-fire/tiny Covenant loop controller,
listener-local request inputs, the four-bin DSP/device consumer, source event
timing or physical audible acceptance. The complete goal and D01–D13 remain
open. Qualification hashes, source drift and scratch cleanup are recorded in
the [routing receipt](../build/test-results/vehicle-destruction-combined-routing-20261101/qualification.json).
The frozen CMake file carries the validator's test-environment append; the
current shared world file also has a separate contact-feature statistics
change. Both deltas are retained without overwriting current work. Temporary
source/build and original-reference ELF/image storage is removed after the
finite session; no large artifacts are retained and no runtime is promoted.

## Shared combined-parent controller, 2026-11-01

The native phased controller now implements definition-wide combined ownership
for the spatial root80 profile. Requests use `definition | 0x6000` instead of
the emitter key, preserve one group's voice-ring state across emitter
generations, copy or preserve the complete 68-byte location as requested, and
reset/accumulate four independent amplitudes using the original byte frame
counter. It selects the original authored/overridden distance bounds and
clears source flag bit1 after the combined contribution. The phased state's
frame counter and combined-parent fields travel with its existing checkpoint
copy. Ordinary and synchronized playback keep their existing owner checks.

The normal Audio dispatcher can find a shared group from another emitter
without reparsing metadata for every active request. Packet parameters now
carry four combined amplitudes explicitly. A backend must provide the actual
listener-local query and four-bin gain/routing consumer before the controller
can allocate or select playback. The current stereo consumer refuses that
payload before owner, RNG or PCM mutation. This introduces no synthetic source
availability flag, admission result or Xbox voice datum.

The [original request reference](../build/test-results/vehicle-destruction-combined-controller-20261101/original-request-qualified.json)
executes unchanged `220CB5..220CF3` and `220D4E..220F37` components with their
actual listener-selection, point-selection, inverse-transform and contribution
callees. It covers 256 keys and 256 existing-parent updates, including 150
missing-listener cases, 52 preserved locations and 86 frame resets. All 494
observed original instruction sites retain their original bytes. An exact
[repeat](../build/test-results/vehicle-destruction-combined-controller-20261101/original-repeat-check.json)
reproduces the complete reference. The private frozen fixture is materialized
from its borrowed reference, with its refresh and hashes recorded.

The native comparison checks the resulting location, source flags, byte frame
and four amplitude fields. Cross-emitter reuse, frame wrap and preflight are
also checked. This parent-only test has no tracks and throws on any metadata,
voice, packet, fade or admission operation. The native test's listener-local
inputs are the recorded outputs of the original queries; it does not certify
the pending production listener resolver, fresh-parent allocation or shared
PCM delivery. Production fixed numeric operands are additionally checked
against the independent original contribution fixture.

The [regression report](../build/test-results/vehicle-destruction-combined-controller-20261101/final-qualified/report.json)
and [native details](../build/test-results/vehicle-destruction-combined-controller-20261101/final-qualified/evidence/Testing/Temporary/LastTest.log)
cover the shared-parent component alongside the previous native gains,
publication, routing, phase/ring and actual PCM resource tests. The
[private negative controls](../build/test-results/vehicle-destruction-combined-controller-20261101/negative-controls.json)
replace definition ownership with an emitter key and suppress frame reset,
then restore and rebuild the actual native controller.

The [full loop check](../build/test-results/vehicle-destruction-combined-controller-20261101/full-scope/evidence/destruction-loops.json)
still requires the real four-bin playback owner for gasoline fire and tiny
Covenant damage. Correct oriented camera/BSP listener inputs, coherent native
gain/routing publication to the DSP/device, complete shared voice lifecycle,
actual destruction-event timing and physical audible comparison remain open.
The complete goal and D01–D13 remain active. The
[qualification receipt](../build/test-results/vehicle-destruction-combined-controller-20261101/qualification.json)
records the effective frozen/current source hashes and scratch cleanup; no
runtime is promoted.
The final 13-suite run passes after both private mutations are restored and
rebuilt. Current audio/controller files match the qualified source; the only
recorded source delta is the validator's private CMake environment append.
The 926,420,493-byte source/build owner is removed and no large artifacts are
retained.

## Submitted-camera combined listener, 2026-11-01

The runtime now carries the submitted camera's forward/up vectors and actual
BSP leaf alongside its position/cluster. Audio loads that BSP's own leaf and
PAS tables from its retained Cache and binds the complete view slots without
compressing their indices. The native combined query composes the qualified
`12A810` listener policy with `12D6F0` point selection and semantic `145200`
inverse-point arithmetic. Its matrix retains the original forward/up inputs
and `up × forward` left vector; it adds no normalization or guessed direction.

The Core rejects mismatched leaves, clears oriented inputs after a
position-only update, and retains the immutable BSP/view frame through actual
checkpoint publication. The production phased output calls this query. Its
four-bin playback preflight remains closed until a real gain/routing/DSP/device
consumer is available; this listener work does not turn that gate into a
readiness byte or supply a voice/admission result.

The [rooted original reference](../build/test-results/vehicle-destruction-combined-listener-20261101/original-listener-rooted.json)
and [exact repeat](../build/test-results/vehicle-destruction-combined-listener-20261101/original-repeat-check.json)
cover 512 listener-to-four-gain chains, 808 camera matrices and 128 inverse
points under all four rounding modes. They include raw/encoded PAS, spatial
columns, range/cluster overrides, missing and tied listener slots, translated
and nonorthogonal views, and zero/unity/scaled transforms. The native result
matches selected listener, distance, local point and resulting four amplitudes.
The actual MCC binding test checks real BSP locations, invalid-leaf rejection,
stale-orientation retirement and exact checkpoint restoration with zero voices.

The initial reference slice began at `12A4B3`, one byte into the instruction at
`12A4B2`; native matrix comparison caught its stale first axis. The
[correction receipt](../build/test-results/vehicle-destruction-combined-listener-20261101/reference-boundary-correction.json)
invalidates those diagnostic captures. The qualified probe starts at `12A4B2`
and verifies every entry and executed site against complete rooted function
traces as well as original instruction bytes. The native implementation was
unchanged for this reference correction.

The [runtime compile](../build/test-results/vehicle-destruction-combined-listener-20261101/runtime-compile/report.json)
uses the native runtime owner's generated CMake flags for the modified actual
translation unit. It is a compile check, not a linked or installed engine.
The [regressions](../build/test-results/vehicle-destruction-combined-listener-20261101/final-qualified/report.json)
and [negative controls](../build/test-results/vehicle-destruction-combined-listener-20261101/negative-controls.json)
cover the native PAS, orientation and camera-restoration paths alongside the
previous destruction audio components.

The complete `129F10` producer's first-person vehicle-seat listener blend is
separate from its matrix slice and remains unqualified here. Actual source
event timing, fresh shared voice lifecycle, four-bin DSP/device output and
physical audible acceptance remain open. The
[full loop scope](../build/test-results/vehicle-destruction-combined-listener-20261101/full-scope/evidence/destruction-loops.json)
and [qualification/cleanup receipt](../build/test-results/vehicle-destruction-combined-listener-20261101/qualification.json)
retain those limits. The full vehicle-destruction goal and D01–D13 remain active;
no runtime is promoted.
The restored implementation passes all 15 regression suites; the complete loop
check remains 5/7. Qualified audio and runtime sources match the current tree;
the sole frozen/current delta is the validator's private CMake append. The
950,576,067-byte temporary source/build owner is removed. Retained evidence is
bounded reports and reference metadata, with no generated binaries, archives,
PCM or captures retained.

## Vehicle-seat listener interpolation, 2026-11-01

`source_vehicle_audio_listener_tu15` implements `129F10:12A05C..12A308` with
complete numerical `120060`, `120310`, `11F9C0` and `11FFD0` dependencies.
The authored seat factor is not clamped. Position interpolation retains the
original separate SSE subtraction, multiplication and addition. Orientation
uses the original opposite-vector fallback, shortest-hemisphere normalized
linear quaternion blend and ordered forward/up rotation. The original pointer
at `474D2C` identifies the verified identity quaternion at `44AC04`.

The [original reference](../build/test-results/vehicle-destruction-seat-listener-20261101/original-blend-qualified.json)
covers 1,024 blends under all four rounding modes, including coincident and
opposite directions, zero/subnormal input directions, nonorthogonal up vectors
and negative/greater-than-one seat factors. All 302 executed instruction sites
have complete rooted function provenance and unchanged original bytes.
The [physical i386 comparison](../build/test-results/vehicle-destruction-seat-listener-20261101/physical-i386.json)
repeats every position, forward/up and quaternion result against the unchanged
original code on the host CPU. A child-only hardware breakpoint observes the
component boundary; no original instruction or callee result is patched.
This is not physical Xbox or audible-device comparison.

The retained-world adapter reads the actual full unit/parent identity, signed
seat, authored definition factor and unit aim, and executes complete `CD440`
for the listener point. Its admission follows both original camera-mode tests;
ordinary mode0 does not run this seat branch. The host runtime applies the
native blend to the audio view and resolves its actual BSP location, retaining
the observer location if that blended point has no cluster. This does not
modify the rendered camera or publish sound-driver readiness/voice admission.
Actual seated-world binding and runtime compilation require the qualification
receipt below before being treated as verified. Peer listener transport,
complete velocity/water-plane production and the four-bin device consumer
remain outside this component. The complete destruction goal stays active.

The [actual Ghost input report](../build/test-results/vehicle-destruction-seat-listener-20261101/final-qualified/evidence/vehicle-audio-listener-binding.json)
contains nine genuinely seated frames, each observing exactly one complete
`CD440` invocation and consuming its actual output. Excluded camera modes and
absent units invoke none. These additional native outputs match both the
[unchanged original component](../build/test-results/vehicle-destruction-seat-listener-20261101/original-actual-seated-inputs.json)
and [physical i386 execution](../build/test-results/vehicle-destruction-seat-listener-20261101/physical-actual-seated-inputs.json),
for 1,033 total original comparisons. This observes the point provider's real
invocation/consumption; it does not independently prove its entire dependency
closure against physical original CPU inputs.

The initial live fixture redundantly rebound local views after the constructor
had already bound and spawned them. Its existing first-person temporal guard
correctly refused that call. The fixture now uses constructor-owned admission;
no guard, availability or temporal result is supplied. The
[four restored checks](../build/test-results/vehicle-destruction-seat-listener-20261101/final-qualified/report.json)
include the previous camera/BSP query and checkpoint tests. The
[negative controls](../build/test-results/vehicle-destruction-seat-listener-20261101/negative-controls.json)
catch omitted position interpolation, bypassed quaternion normalization/blend,
and incorrect mode0 admission, with actual affected units rebuilt. The numeric
test also preserves a live host x87 stack, tags, sticky flags and FP controls.

The [runtime translation-unit check](../build/test-results/vehicle-destruction-seat-listener-20261101/runtime-compile/report.json)
compiles the actual native owner's modified camera/audio/ambient call sites;
it does not link, install or promote an engine. Audio changes match the tested
frozen sources. Concurrent physics CMake/census changes are preserved and
recorded separately in the
[qualification and cleanup receipt](../build/test-results/vehicle-destruction-seat-listener-20261101/qualification.json).
These checks do not certify complete current-engine or audible parity.
The 2,506,941,454-byte owned source/build tree and the separate original-code
probe/hardware owners are removed. Required reference JSON and bounded reports
remain; verbose compiler-warning bulk is compacted with its original size/hash
recorded. No generated binaries, archives, raw memory, PCM or captures remain.

## Native loop gain envelopes and independent bin gains, 2026-11-01

The production `source_sound_control_gain` metadata adapter now calls native
`127CF0..127D2D` arithmetic instead of the previous standard-library exp/log
expression. Its actual class/scale bounds use the original SSE additions;
`220000` keeps the upper amplitude extended while rounding the lower one to
single precision, and `220200` retains the original logarithm and gain floor.
The numerical module owns the specified FP controls and restores its caller.
The diagnostic transcription of the previous expression also matched the 192
sampled nearest-rounding outputs; this conversion establishes the original
native arithmetic/control contract, not a demonstrated audible difference.

`source_loop_combined_gain_tu15` consumes each of the four accumulated
amplitudes independently. It retains the original class/offset/envelope/parent
addition order. Parent identity/admission, actual ducking state and the native
DSP/device consumer remain caller-owned requirements; no legacy pool or
readiness byte is manufactured. The four-bin playback preflight stays closed.

The [original reference](../build/test-results/vehicle-destruction-bin-gains-20261101/original-envelope.json)
executes complete unchanged `127CF0`/`2216D0` and their actual callees. It covers
768 scalar envelopes under all four rounding modes, 192 production metadata
adapter cases, and 120 accepted four-bin publications whose complete outputs
also match the retained physical original publication fixture. All 344
executed sites have rooted original instruction provenance. The
[physical i386 reference](../build/test-results/vehicle-destruction-bin-gains-20261101/physical-envelope.json)
independently observes every scalar envelope, actual class gain and complete
return through child-only hardware breakpoints without modifying source code
or supplying a callee return. This is not audible-device or physical-Xbox
acceptance. Runtime PCM regressions, restored controls, effective source hashes
and cleanup are recorded in the qualification receipt when complete.
The [seven restored suites](../build/test-results/vehicle-destruction-bin-gains-20261101/final-qualified/report.json)
pass, including the actual PCM resource, alternate-loop, Banshee phase,
complete publication/routing kernel and output-preflight regressions. The
[three private controls](../build/test-results/vehicle-destruction-bin-gains-20261101/negative-controls.json)
fail when the upper amplitude is rounded early, FP isolation is omitted, or
the four amplitudes collapse to the first bin; affected native objects are
rebuilt and restored. The
[complete loop check](../build/test-results/vehicle-destruction-bin-gains-20261101/full-scope/evidence/destruction-loops.json)
remains five of seven, with gasoline fire and tiny Covenant damage requiring
their native four-bin playback owner. The
[qualification/cleanup receipt](../build/test-results/vehicle-destruction-bin-gains-20261101/qualification.json)
records these limits and concurrent physics-source drift; no engine is
installed or promoted. The full goal remains active.
The 888,076,569-byte source/build owner and separate original/hardware probe
owners are removed. Only required reference JSON and bounded reports remain;
no generated binaries, archives, raw memory, PCM or captures are retained.

## Native combined dry DSP chain, 2026-11-01

`tu15_sound_combined_dry::process` connects the original four chorus records
4–7 to crosstalk record10 over `GraphStorage`'s real native owners. Bins6–9
remain independent, share the actual bin14 modulation block, and retain one
1601-word chorus history with inclusive shared endpoints. The crosstalk state
and its two eleven-word stereo history pairs persist between calls. Existing
output bins0/1/4/5 are caller-owned input contributions and retain the original
additive behavior. Supported modes for all five records are checked before
an earlier record can mutate its owner; malformed data/state still belongs
to the individual processor's failure contract.

The [qualified original reference](../build/test-results/vehicle-destruction-dry-graph-20261101/original-dry-graph-qualified.json)
uses authenticated original GP/title words and the pinned corrected offline
DSP reference core. It executes all15 original records for each of32 frames:
four isolated input-bin cases with four retained frames each, followed by16
frames of distinct raw24-bit samples and declared nonzero chorus gains. No
history, feedback state, flags, cached binding or module status is replaced
between frames. The reference verifies that intervening reverb records leave
this dry chain's inputs, output baselines and retained chorus history intact.
Only mix samples, modulation and four gain parameters are declared inputs;
this does not establish a hardware voice producer or normalized PCM scale.

The native composition matches the four chorus outputs, all1601 history
words, chorus cursors/flags, crosstalk feedback/coefficient/cursor state and
four speaker blocks. It produces96 nonzero speaker blocks across the32
reference frames and crosses the chorus cursor wrap. All numeric values,
original program hashes and reference-source pins reproduce in the
[independent repeat](../build/test-results/vehicle-destruction-dry-graph-20261101/original-repeat-check.json).
Whole report bytes differ because compiler paths and hashes of pointer-bearing
host telemetry belong to their separate disposable reference processes.

The [seven restored checks](../build/test-results/vehicle-destruction-dry-graph-20261101/final-qualified/report.json)
also cover the existing chorus, crosstalk and graph-storage vectors/guards.
The [four private controls](../build/test-results/vehicle-destruction-dry-graph-20261101/negative-controls.json)
catch collapsed input ownership, cleared retained history, omitted crosstalk
and a missing early mode check. The native target links the compiled typed
processors; the original DSP core is reference-only.

This function owns records4–7/10, not the complete original graph. Wet sends
and returns, actual voice-to-bin production/scale, native supervisor/caller
context, first-six record0 input production, output/device ownership, shared
loop lifecycle and audible OG acceptance remain open. The four-bin production
playback preflight remains closed and no engine is promoted. The
[qualification/cleanup receipt](../build/test-results/vehicle-destruction-dry-graph-20261101/qualification.json)
records effective source hashes, bounded evidence and these limits. The full
vehicle-destruction goal remains active.
The 674,030,129-byte source/build owner and separate original graph probe
owners are removed. Required numeric reference JSON and bounded reports remain;
no generated binaries, archives, raw program/memory banks, PCM or captures are
retained from these invocations.

## Native combined wet DSP chain, 2026-11-01

`tu15_sound_combined_wet` connects the actual native fanout, distortion,
two accumulators and two reverbs over `GraphStorage`. `prepare` runs records0–3;
`reverbs` runs records8/9, allowing records4–7 to retain their original position
between those stages. `process` composes both stages and checks every mode
before starting. Malformed data/state still has each processor's own admission
boundary, rather than an atomic whole-graph contract. The existing arithmetic
is unchanged; control checks are shared with each individual processor.

The original bindings are preserved: distortion uses exact bin13 input/output
aliasing; each accumulator consumes bin10 and the shared fanout workspace and
adds to its distinct bin11/12 destination. Record8 consumes bin12 and adds to
bins19–22; record9 consumes bin11 and adds to bins15–18. Both reverb owners use
one shared workspace without a reload between calls, with separate persistent
fourteen-ring histories and filter/cursor state. These are logical sample
owners, not a physical DMA-format certificate or an inferred speaker return.

The [qualified original reference](../build/test-results/vehicle-destruction-wet-graph-20261101/original-wet-graph-qualified.json)
runs all15 unchanged original records for each of128 frames. Four isolated
bin10–13 cases retain sixteen frames each; another64 frames exercise distinct
raw samples, additive output contributions, then forty frames of zero-input
tail. The original program validates its literal installation before declared
mix inputs enter. No history, status, flags, cached pointer, program instruction
or availability/callee result is overlaid. Boundary assertions establish that
the intervening chorus records preserve these wet inputs/workspace and that
the remaining graph preserves the reverb histories. All numeric values and
original program/reference-source hashes reproduce in the
[independent repeat](../build/test-results/vehicle-destruction-wet-graph-20261101/original-repeat-check.json).
Disposable paths and pointer-bearing telemetry make whole JSON bytes differ.

The native comparison covers prepared send words, distortion parameters and
eight history words, accumulator state, reverb parameters/filters/transitions,
all45 read/write cursors per reverb, complete logical-ring numeric digests,
final shared workspace digest, filtered/decimated input and additive wet output.
It retains the same native history allocations above4GiB. Both split-stage and
combined calls match; the128 frames produce544 nonzero wet output blocks,
including320 tail blocks, and cross a reverb ring wrap. The comparison digests
are portable FNV64 over little-endian logical words; SHA256 separately pins
original programs, reference sources, numeric fixtures and native source.

The [twelve restored checks](../build/test-results/vehicle-destruction-wet-graph-20261101/final-qualified/report.json)
include both combined paths, graph storage and the individual fanout,
distortion, accumulator and reverb vectors/guards. Four
[private negative controls](../build/test-results/vehicle-destruction-wet-graph-20261101/negative-controls.json)
catch omitted distortion, collapsed accumulator destinations, cleared retained
histories and missing complete-stage mode preflight. Each mutant rebuilds its
native object and is restored before final qualification.

Actual voice-to-bin production/scale, the first-six fanout input producer,
coefficient/control ownership, wet return-to-speaker routing, supervisor/device
delivery, destruction event/loop lifecycle and audible original acceptance
remain open. The four-bin production playback preflight stays closed. No
launcher-selected runtime is replaced or promoted, and no D01–D13 row is closed
by this component. The [qualification and cleanup receipt](../build/test-results/vehicle-destruction-wet-graph-20261101/qualification.json)
records the frozen source, current-source drift, reference limits and removal
of the disposable build/reference owners. Required numeric reference inputs
and bounded diagnostics remain; generated binaries, archives, raw program or
memory banks, PCM and captures are not retained.

## Native wet-return construction and sample ownership, 2026-11-01

The actual startup caller now uses `source_sound_driver_wet_returns_tu15`
for its original eight mix-in voices. The shared native route owner preserves
inputs15–22, flags`0x100000`, zero-dB volume, distinct driver slots and ordered
destination bins6/7/8/9 followed by6/7/8/9. It does not mix wet outputs directly
into final speaker bins. Each successful creation performs volume, routing,
then play; each failed creation skips that voice's setters and continues the
remaining attempts. Slot identities are re-read after external callbacks, and
the next iteration uses the current driver. Missing resource services still
throw; no default device identity, successful HRESULT or ready flag was added.

The [original parent correspondence](../build/test-results/vehicle-destruction-wet-return-20261101/original-startup/report.json)
authenticates the full224EC0 instruction body and its225333–225452 fragment
boundaries. The [new dynamic reference](../build/test-results/vehicle-destruction-wet-return-20261101/original-wet-returns.json)
executes that unchanged TU1.5 fragment in14 cases: all-success/positive-status,
all-failure/alternating failures, failure at each individual creation, changing
buffer identities and a changing driver. It observes112 create attempts and
388 external ABI calls across65 original instruction sites. SDK returns and
external identity mutations are explicitly declared component inputs; SDK
callees, whole device startup and audible playback are not executed by this
reference. No original instruction is patched. An exact byte repeat and
[physical i386 single-step comparison](../build/test-results/vehicle-destruction-wet-return-20261101/physical-i386.json)
reproduce all cases and external arguments, descriptors, routes, final slots
and driver selection under those same declared boundaries.

`tu15_sound_wet_return::Sources` retains an actual `LogicalCopyReceipt` from
the native GP output owner and exposes the original eight input/destination
bindings. Its samples reference the corresponding32-word blocks in that
owner's existing auxiliary values. These are current retained resources, not
immutable frame snapshots; returned spans require a retained Sources/receipt.
The original-copy ordinal is diagnostic metadata, not a hardware clock.
The new regression composes the actual native wet chain, GP output copies and
these views for128 original-reference frames. All32,768 return-source words
match the qualified original wet outputs. Allocations remain above4GiB and
stable, retained views observe subsequent copies, and the resource survives
destruction of its original producing owner.

The [four restored checks](../build/test-results/vehicle-destruction-wet-return-20261101/final-qualified/report.json)
include the new return boundary, both combined DSP paths and the existing GP
output values/ownership suite. The focused target also compiles and links the
actual startup translation unit. Four
[private mutants](../build/test-results/vehicle-destruction-wet-return-20261101/negative-controls.json)
are caught: collapsed output channels, cached buffer identities, aborting the
loop on a failed creation, and shifted GP return-source offsets. Mutated
objects are rebuilt, restored and checked before final qualification.

The [supplemental pinned xemu source](../build/test-results/vehicle-destruction-wet-return-20261101/supplemental-reference-boundary.json)
places VP processing before GP processing and exposes the SGE/sample-format
read path. That model is not physical MCPX timing evidence. This change does
not infer feedback latency, sample-format conversion or gain arithmetic from
the routing metadata, and it installs no replacement VP mixer or output clock.
Actual voice/sample production, coefficient/control ownership, feedback
scheduling/conversion, complete speaker/device output, destruction event/loop
lifecycle and audible OG acceptance remain open. The production four-bin
playback preflight stays closed; no D01–D13 row or full goal is complete.
The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-wet-return-20261101/qualification.json)
records effective source hashes and current-source drift. The owned build and
all original/static/physical probe scratch are removed; only small numeric
references, source listings and bounded reports remain. No generated binaries,
archives, raw program/memory banks, PCM or captures are retained.

## Native wet-voice format, attachment and auxiliary consumer, 2026-11-01

The actual SDK-produced format and attachment fields now enter the native
return-source adapter through `source_sound_wet_voice_layout_tu15` and
`Sources::logical_view`. Format and sample owners are read independently from
the original voice's`+70/+80` identities. The adapter admits the eight literal
startup wet voices only after the source has its actual attachment fields,
borrowed128-byte block, expected bin-relative offset, hardware format and
published cursor word. It exposes values over the retained native GP output
owner. It supplies no playing state, device success, hardware playhead or
source-address/native-resource association; callers still own association,
exclusion and scheduling.

The [unchanged TU1.5 SDK reference](../build/test-results/vehicle-destruction-wet-voice-20261101/original-layout.json)
executes64 profiles and288 complete initialize/format/attach calls across321
original instruction sites. Every original default format field is retained:
PCM tag1, mono,48000Hz,192000bytes/second, four-byte stride and24 sample bits.
The32-sample attachment therefore contains128 bytes. The CPU-published
special-source cursor uses page`7FF` and the source buffer's low twelve address
bits. This cached publication is not an observation of hardware consumption.

Cases cover all eight wet inputs, four source-base patterns, all four rounding
modes across the matrix, distinct format/sample owners and warm repeat
attachment. In the distinct-owner cases both objects are initialized by the
unchanged original CPU. Only the sample owner's unused format header is then
poisoned as an explicit adversarial input, exposing any incorrect owner reuse.
State/device guard methods remain declared ABI boundaries; no kernel body,
allocator or playback/device success is supplied. All2,272 ordered stores,
448 guard boundaries, complete retained source bytes and typed returns match
the compiled existing SDK owners. The reference has an exact-byte repeat and
an independent [physical i386 comparison](../build/test-results/vehicle-destruction-wet-voice-20261101/physical-i386-final.json).
The native guard bodies use a real recursive mutex; this is native exclusion,
not a claim of Xbox IRQL or device readiness.

The [auxiliary-page join](../build/test-results/vehicle-destruction-wet-voice-20261101/original-auxiliary-join.json)
identifies the original downstream consumer. The authenticated GP descriptor
copies640 words from bins11–30 to scratch`B000` with control`49E2`/format2.
Original38DE07 appends descriptor`391410` at GP table index11 for the declared
eleven-page prefix and publishes count12. Original38E159/38E168 selects VP
input page`7FF` and publishes that same descriptor's physical page from`391414`.
The SDK attachment reads the source pointer from`391410` and adds its actual
bin-relative block offset. Three declared resource profiles execute these
original CPU paths without instruction changes, device reads or supplied
success results. MMIO stores are write-only observation sinks. This establishes
the CPU/dataflow join, not actual allocation/device acceptance or format2
byte placement. The pre-existing MSB/LSB ambiguity remains open.

The [five restored suites](../build/test-results/vehicle-destruction-wet-voice-20261101/final-qualified/report.json)
include the new full SDK/layout path, original wet-return construction, both
combined DSP paths and GP output ownership. The new test also verifies2,048
logical GP source words using an independently qualified original wet frame,
and832 unsupported layouts are rejected without source mutation. Five
[private mutants](../build/test-results/vehicle-destruction-wet-voice-20261101/negative-controls.json)
catch incorrect format/sample owners, missing format/cursor admission and
collapsed logical source identity. Every affected native object is rebuilt and
restored before final qualification.

No VP sampler, physical DMA serializer, feedback clock, native device output
or ready flag is installed by this work. Actual voice/PCM production,
coefficient/control ownership, feedback byte packing/time, speaker delivery,
destruction event/loop lifecycle and audible OG acceptance remain required.
The four-bin production playback preflight stays closed and the full goal
remains active. The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-wet-voice-20261101/qualification.json)
records the effective frozen source and current-source drift. The disposable
build and all original/static/physical probe scratch are removed. Required
numeric fixtures and bounded diagnostics remain; no generated binaries,
archives, raw program/memory banks, PCM or captures are retained.

## Native PIO publication and actual input-page owner, 2026-11-01

`source_run_sound_pio_native_tu15` now implements the complete original
`38E0CE` integer owner and its `38E159` suffix as semantic native C++. The
actual `SourceRuntimeSoundPioResources` uses this implementation. The previous
literal translation remains an independent regression route. There is no
opcode decoder or lifted instruction dispatcher in the new implementation.
Original observation/cancellation sites, register results, defined flags and
ordered source writes remain explicit, including pointer reloads between
publications. Pure PIO writes require the actual action owner and expose no
invented device preimage. Integer execution preserves unrelated x87, XMM,
MMX and optional floating-point provenance.

The [unchanged original reference](../build/test-results/vehicle-destruction-pio-native-20261101/original-pio-v2.json)
contains 36 cases: 24 complete CPU returns and 12 interrupted prefixes. It covers
FIFO polling, 256 voice rows, six table/notifier publications, five tracking
requests, the shared auxiliary-page selection/binding and four-byte HRTF
routing publication. Declared pointer changes expose cached reads and changes
after an operand was already loaded. All 57 source sites are authenticated;
the external return sentinel is declared caller data. An exact-byte repeat
and [physical i386 execution](../build/test-results/vehicle-destruction-pio-native-20261101/physical-i386-v2.json)
agree on 33,895 instructions, 4,146 source stores, 236 ordered write-only MMIO
requests, registers, defined flags and complete declared source storage. FIFO
values and MMIO sinks are declared CPU-test inputs, not device readiness or
successful tracking/HRTF implementations.

The [focused native/resource suites](../build/test-results/vehicle-destruction-pio-native-20261101/final-qualified/report.json)
also exercise the real bounded queue and input scatter/gather owner through
the semantic implementation, using the actual PipeWire output device. Three
initial-fill cases compare all 16 retained DMA allocations, execute the
eight-slot/32-byte payload queue and audit each real eight-byte input-page
binding. Four failure cases cover observer, permission and cancellation
boundaries; separate foreign-thread destruction remains rejected. These checks
submit no destruction PCM and do not establish audible delivery. The dummy
backend is correctly rejected by the existing audio preference owner. An
initial dummy-backend attempt failed at that guard, before queue acceptance.
The focused targets now compile their actual SDK/settings dependencies without
the transitive full-game startup link; two superseded dependency builds were
stopped and are retained only as diagnostics.

Six [private negative controls](../build/test-results/vehicle-destruction-pio-native-20261101/negative-controls.json)
check cached mutable tables, unowned writes, a wrong feedback page, incorrect
HRTF byte selection, lost interrupted registers and discarded opaque MMX state.
Each affected native object is rebuilt; the restored source passes all three
suites. The physical runner also repeats the previous wet-voice matrix with
its unchanged default bank size after adding explicit larger caller storage.

Actual resource execution still stops before unsupported tracking at`38E140`.
Its separately admitted page-binding component stops before HRTF at`38E16D`;
the whole cold initializer has not returned. Full raw CPU-reference returns
must not be confused with implemented device methods. Feedback format2 byte
placement/time, VP sampling, coefficient/control ownership, complete native
speaker output, every destruction event/loop transition and audible OG
acceptance remain required. The four-bin production preflight stays closed;
D01–D13 and the complete goal remain open. The
[qualification/cleanup receipt](../build/test-results/vehicle-destruction-pio-native-20261101/qualification.json)
records effective source, current-source drift and removal of owned source,
build and probe scratch. No generated binary, archive, raw bank, PCM or capture
is retained.

## Native effect sound-part admission, 2026-11-01

The native world-sound path now applies original `17D298..17D2BA` selection
to completed source/transport flags before resolving markers. Part `+2` is
compared with selector 1 or 2 according to the actual effect's bit 5. This is
separate from part `+0` and its later environment query. Attached effects use
their completed source allocation flags; other source effects use their
retained render packet. Host-only presentation retains its existing path and
still requires original flag-production qualification.

Both native sound readers now use `source_effect_sound_part_tu15` for the
literal sound reference, signed location, selector and A/B control fields.
The native reference predicate preserves the original signed count comparison,
invalid-location rejection and authored NONE. An invalid sound location is
skipped before marker lookup, matching the original rather than rejecting its
entire effect graph. The fixed host reader retains its previous clear-bit-5
profile; this work corrects its misleading environment comment without
supplying an unobserved effect flag or environment result.

The [unchanged TU1.5 reference](../build/test-results/vehicle-destruction-part-admission-20261101/original.json)
executes the actual `17D210` owner for 1,024 declared part cases. Its 890
rejections return naturally. The other 134 stop before the first `17DCE0`
location-iterator call; no iterator or downstream result is supplied. All 67
executed original instruction sites are authenticated. The exact-byte repeat
and [physical i386 comparison](../build/test-results/vehicle-destruction-part-admission-20261101/physical-i386.json)
agree on 46,922 instructions, 8,764 ordered stores, complete declared caller
storage, registers and defined flags. The semantic native predicates and actual
shared byte reader match every original admission outcome.

The [current MCC census](../build/test-results/vehicle-destruction-part-admission-20261101/stock-parts.json)
reads 40 maps and finds 368 matching serialized sound-part records across 48
effect definitions. These reference 13 sound names from the vehicle destruction
inventory. Every matching record has selector 0, environment 0, part flags 0
and a valid location. The actual reader preserves admission for all 368 byte
records with either selector-bit branch. Thus this change does not newly mute
or enable these known unconditional parts. Other inventory sounds use particle,
material, looping or other dispatchers and are not absent merely because they
do not appear in this direct `effe` part census.

The [restored focused check](../build/test-results/vehicle-destruction-part-admission-20261101/final-qualified/report.json)
also compiles both actual consumer translation units. This is compile and
component execution evidence, not a complete engine link or live destruction
audio test. Six [private negative controls](../build/test-results/vehicle-destruction-part-admission-20261101/negative-controls.json)
catch a missing NONE gate, unsigned count comparison, wrong selector bit,
inverted selection, a wrong literal field and a wrong dispatcher group. The
native unit is rebuilt for every control and restored before the final pass.

Source event timing, complete environment/location ownership, host-only flag
production, every sound/loop lifecycle, feedback conversion/time and audible OG
acceptance remain required. D01–D13 and the goal remain open. The
[qualification and cleanup receipt](../build/test-results/vehicle-destruction-part-admission-20261101/qualification.json)
records effective source, current-source drift and removal of owned build and
probe scratch. Only bounded diagnostics and required numeric references remain;
no generated binary, archive, raw bank, PCM or capture is retained.

## Native GP output to EP input windows, 2026-11-01

`tu15_ep_input::copy_input_window` now copies the original five-channel input
window from the actual retained native GP output owner. The caller supplies
the selected half at word offset 0 or 256. Each result owns five arrays of
256 `Word24` samples; later GP copies and destruction of every source owner
cannot change those values. The sixth GP ring and auxiliary return planes are
outside this EP acquisition. No phase is inferred from a GP write cursor or
copy ordinal, and acquisition does not advance any cursor.

The [fresh original reference](../build/test-results/vehicle-destruction-ep-input-20261101/original.json)
acquires the original EP loader, module5, configuration and descriptor through
unchanged TU1.5 CPU instructions. It then executes actual loader commands
10/6, the complete module at P0304, five command7 reads, both command8 calls,
command15 diagnostics and command11 half toggles. The source storage and
already-initialized caller context are declared component inputs. The
reference DMA copy/busy model is the previously qualified bounded emulator
model; physical DMA or clock behavior is not established.

All 33 original GP value-copy outputs from the existing authenticated fixture
enter this fresh EP reference. Its 66 natural module returns cover both
halves, with 330 actual input reads. Every read is 1,024 bytes from the first
five 2,048-byte rings at the selected half. The sixth ring is not read.
An independent [repeat comparison](../build/test-results/vehicle-destruction-ep-input-20261101/repeat-comparison.json)
matches every reference result and source pin; only the actual compiler argv
differs because its disposable owner has a different path. Original program
bytes and callee/clock results are not patched or supplied.

The [native composition](../build/test-results/vehicle-destruction-ep-input-20261101/final-qualified/evidence/native-sound-ep-input-report.json)
uses actual GP copies, the new input windows and the existing native EP stereo
kernel. All 84,480 input words agree through ordered logical-value digests.
All 34,716 EP output/coefficient/diagnostic words and 67,584 packed FIFO bytes
match the original module. This is numeric packing comparison, not a receipt
for submitting those bytes to a device. Sixty older GP receipts correctly
read their current retained resources; 66 copied windows remain exact after
the sources are destroyed. Native 64-bit ownership, 198 rejected half inputs
and 33 source non-mutation checks also pass.

The [three restored suites](../build/test-results/vehicle-destruction-ep-input-20261101/final-qualified/report.json)
include the new join, existing GP output ownership and original EP stereo
loader regression. Six [private negative controls](../build/test-results/vehicle-destruction-ep-input-20261101/negative-controls.json)
catch swapped center/right channels, an ignored second half, a write cursor
used as phase, a missing final sample, the sixth ring substituted for surround,
and an unsupported half accepted. Every mutated object is rebuilt and the
restored source passes all three suites.

The separate [feedback-format research](NATIVE_SOUND_FEEDBACK_FORMAT_RESEARCH.md)
pins the original CPU join and primary emulator/hardware-experiment sources.
It finds no retained hardware bytes that resolve format2 placement or VP
sample interpretation. An alignment-independent identity round trip would
still assume producer/consumer agreement that the current evidence does not
prove. The new EP window join does not make that assumption or install a wet
feedback reader. It also provides no actual frame/half producer, outgoing DSP
status, boot/install completion, endpoint/backpressure receipt or audible OG
acceptance. Both combined destruction loops still require their real four-bin
playback owner; the production preflight remains closed.

D01–D13 and the complete goal remain open. The
[qualification/cleanup receipt](../build/test-results/vehicle-destruction-ep-input-20261101/qualification.json)
records effective source and current-source drift. The owned build and both
original probe contexts are removed. Only required numeric references, primary
source research and bounded diagnostics remain; no generated binary, archive,
raw bank, PCM or capture is retained.

## Destruction sound payloads and MCC chunk submission, 2026-11-01

The [named payload audit](../build/test-results/vehicle-destruction-waveforms-20261101/payload-audit-final/report.json)
pairs all 273 pitch/permutation names in the 31 destruction sound banks. There
are no unmatched names and no differing payload variants within either inspected
platform's maps. Thirty-six permutations retain identical 44.1 kHz Xbox ADPCM
format/payload bytes and produce identical PCM through the same current PC
decoder. The remaining 237 pair Xbox 44.1 kHz ADPCM with MCC 48 kHz Opus; their
decoded waveforms differ. These measurements do not authenticate the PC
resampler/decoder against Xbox hardware, select an alignment correction, or
establish audible equivalence. The Xbox permutation `ugh! +8` sample-size word
equals compressed byte extent for all 273 pairs; it is not labeled a decoded
frame count in the final report.

The [primary binary research](DESTRUCTION_SOUND_CODEC_FRAMING_RESEARCH.md)
separates MCC's rounded decode allocation from its submitted logical chunk
length. An [unchanged instruction observation](../build/test-results/vehicle-destruction-waveforms-20261101/original-chunk-submission.json)
executes 122 declared post-decode entries at `77923E`, stopping before the Miles
import at `77928E`: 17 instruction sites and 1,628 executed instructions. It
observes actual chunk-length/cursor reads, pointer and byte-length arguments,
ordered stores and the one-shot cursor reset. Code, chunk and nominal PCM
regions have explicit read/execute/read-only protections. No program bytes or
callee results are substituted. This slice does not execute the decoder,
complete the original caller, or call the device. An independently owned repeat
agrees byte for byte.

`decode_mcc_opus` now removes each chunk's allocation padding after decoding
the complete packet contribution to decoder state, before appending the next
chunk. Its separate permutation tail bound remains. Eight added non-silent
real-packet cases exercise mono/stereo, continuing/reset decoder state and
whole/shortened tails. The previous global-only trim fails the isolated
regression. The [restored focused validation](../build/test-results/vehicle-destruction-waveforms-20261101/decoder-qualification.json)
passes both native MCC framing and frontend movie routing, and rebuilds the
actual decoder consumers. The measured destruction cohort contains ten
multi-chunk permutations and zero non-final allocation-padding cases; this
correction does not change their PCM ordering/length or establish a newly
audible destruction fix. No constant Opus pre-skip is introduced.

The user confirmed that original Xbox capture hardware is unavailable and
requested continued native work. Physical feedback format/time, actual source
event/loop timing, all sound delivery and all complete destruction gates remain
open. The combined four-bin loop preflight remains closed. D01–D13 and the goal
are not complete. The [qualification and cleanup receipt](../build/test-results/vehicle-destruction-waveforms-20261101/qualification.json)
retains effective source hashes, isolated negative/final checks and asset pins;
root CMake drift is reported separately from the frozen decoder cohort. Owned
build/probe storage is removed. Only bounded metadata, numeric observations and
diagnostics are retained; no generated binary, raw bank, PCM or capture remains.

## Native pending-response countdown, 2026-11-01

`native_tu15_pending_response_step` now owns the semantic per-section countdown
inside original `D8330`. The low three bits retain the response index; the upper
13 bits count ticks. A zero count is inactive even when its response index is
nonzero. A live count decrements once, preserving the index, and only its
transition to zero continues into the existing eligibility/response owner.
No host seconds, guessed tick rate, eligibility result or effect/damage result
enters this kernel. The remaining `D8330` logic is not claimed as a completed
semantic conversion.

Both actual retained vitality and damage-delivery parents select the new kernel
for TU1.5. Their compatibility adapter preserves the original reads, dirty-byte
and pending-word stores, instruction observations, registers and defined
arithmetic flags. Older-image literal code remains intact. Both regenerated
parents are byte-identical to the current sources; the observation generator
also strips and reinstalls the existing five-kind response route exactly,
retaining its original canonical-source guard rather than adding a new baseline.
The [regeneration receipt](../build/test-results/vehicle-destruction-pending-native-20261101/regeneration.json)
records both results and installer idempotence.

The [unchanged TU1.5 reference](../tests/fixtures/pending_response_step_tu15.json)
executes `D83F0` through its actual inactive/counting continuation or stops before
the due path reads the object pool at `D842C`. All 65,536 input words execute:
eight inactive states, 65,520 counting states and eight due transitions, totaling
1,048,512 instructions over 16 sites. Its independently owned repeat is
byte-identical. The native kernel matches every numeric outcome. Forty-nine
varied-register cases also match both observer conventions: 98 adapter replays,
including complete declared banks and ordered accesses. The independent
[physical i386 replay](../build/test-results/vehicle-destruction-pending-native-20261101/physical-i386.json)
matches those 49 cases, 720 instructions, 82 stores and 12,544 declared bank
bytes. No opcode or callee result is substituted. This is host CPU component
evidence, not physical Xbox or complete destruction-event acceptance.

The [fresh linked-parent comparison](../build/test-results/vehicle-destruction-pending-native-20261101/parents-baseline/evidence/native-pending-response-parents-report.json)
uses the existing unchanged-code oracle's 72 declared pending/region cases with
actual original Lockout tags and 144 explicit metadata input overlays. Each
parent matches 15,948 original instructions, 1,956 ordered writes and 126,930
retained state bytes, including 126 pending-step entries. There are 54 natural
returns and 18 real first-callee boundaries per parent. The delivery comparison
stops before newly owned callees at the same original entry; it supplies no
return or eligibility result. Both linked parents reject a private two-tick
decrement at the exact pending-word store, proving their new route is selected.
The [restored parent selection check](../build/test-results/vehicle-destruction-pending-native-20261101/parent-selection.json)
and final focused checks pass. Four additional private controls reject an
inactive word treated as due, a truncated response index, a double decrement
and reversed publication order; production headers and original references
are unchanged by these controls.

Actual MCC gameplay, delayed-response eligibility/effects/damage, sound timing
and audible delivery remain separate required gates. D01–D13 and the complete
goal stay open. The [qualification/cleanup receipt](../build/test-results/vehicle-destruction-pending-native-20261101/qualification.json)
records source hashes and removal of the owned source/build, physical helper
and parent fixtures. Required numeric references total 223,035 bytes; no build
products, separate raw-bank files, PCM or captures are retained or runtime promoted.

## Native pending response in an actual module chain, 2026-11-01

The [current frozen-cohort receipt](../build/test-results/vehicle-destruction-pending-live-20261101/qualification.json)
keeps the goal active. Physical Xbox capture access is unavailable; the user
explicitly directed continued native work. This does not authorize guessed wet
feedback formats or change Xbox TU1.5's authority.

The pending adapter now publishes synchronous, bank-bound read-only observations
after its real stores. The exhaustive scalar, both adapter conventions and both
linked-parent checks still pass with that observer installed. The destruction
fixture correlates each active step with the committed original pending-word
store, verifies the full target generation/body/definition and section address,
and requires the native due transition to precede the target's original DC4A0
blast publication. Diagnostic replay steps are excluded from that live journal;
the replay itself still compares the complete encoded world.

The actual Lockout power module records six native transitions:
`48 -> 40 -> 32 -> 24 -> 16 -> 8 -> 0`, at input match ticks282..287. All six
match the unchanged TU1.5 numeric reference. The single due transition precedes
the original target blast; the pristine nearby witness then chains and retires,
while the distant control stays unchanged. Two repeated restores at the real
pending cut pass16 complete-world comparisons and return to that cut. The
post-destruction cut passes another16. No eligibility, damage, blast, physics,
retirement or callee result is supplied.

The new native contact cleanup branch comes from an actual Warthog cleanup
BEFORE whose collector has bit8000 set and whose current lock registry lacks
that collector. Native `2E12A0(flags=0)` now performs the current registry hash
lookup and clears the lock bit on this absent-key branch. It does not call a
generated release routine or fabricate an allocator result. Registered entries,
unsupported table extents and nonterminating tables remain refused. Existing
manager/collector destruction and their current storage owners are retained.
The world guard admits only the newly qualified absent-key case; positive
current-world updates and registered-lock storage work remain separate.

The [original-code qualification](../build/test-results/vehicle-destruction-pending-live-20261101/base-lock-component-v2/report.json)
passes25 cases:13 absent entries and12 registered-entry refusals. One case uses
the actual manager BEFORE's registry fields as a declared direct leaf input;
the other cases declare empty/colliding tables and bit patterns. This is not a
capture of the original2E12A0 entry ABI. Physical i386 execution matches1,646
unchanged original instructions and the emulator's complete branch/read/write
ledger. GCC and Clang each match168 ordered reads,13 committed byte stores and
all declared persistent fields. Hash, slot-stride, bit-clear and registered-key
controls all fail. The [registered project CTest](../build/test-results/vehicle-destruction-pending-live-20261101/base-lock-registered/report.json)
also passes. Its required numeric reference is53,892 bytes; raw BEFORE banks
and the physical executable are disposable.

The [eight-case summary](../build/test-results/vehicle-destruction-pending-live-20261101/live-summary.json)
is deliberately red overall. The module, Banshee and Wraith pass, totaling64
complete-world comparisons including the pending cut. Five cases reach their
original death-bit store but refuse later native pair creation:

| Case | Remaining native owner |
| --- | --- |
| Warthog, Ghost, Scorpion | Shape-class owner `AD710` |
| Gauss Warthog, Spectre | Pair-create owner `31FFC0` |

Static inspection shows `AD710` is the original class10 getter; no new admission
or shape/agent result was added for it. `31FFC0` dispatches actual type2 payload
callbacks and remains unconverted here. Their failures are preserved, rather
than counted as successful destruction because the target died. The fixture's
read-only store collectors use the explicit read-only API required by the
native flip/contact owners; the earlier observer-admission failures are retained.

All results belong to the authenticated frozen source plus explicit refresh
receipts. The changed world object was rebuilt after its source refresh. A
concurrent workspace edit to a convex-factory diagnostic is preserved and
recorded as drift from that tested source. No launcher runtime is promoted.
The fixture is muted: this turn establishes no new audible, visual or original
full-gameplay parity. Gasoline-fire/tiny-Covenant combined loops still require
the native four-bin DSP/device consumer. All37 full-definition gates, D01-D13
and proper destruction-sound acceptance remain open.

The receipt verifies removal of the owned source/build, raw manager capture
and original CPU helper. No large artifacts are retained.

## Native phantom pair creation and overlap insertion, 2026-11-01

Native `31FFC0`, `315B00`, `315300` and `314F80` now own supported phantom
creation, insertion and added-listener dispatch. The [source contracts and
component evidence](NATIVE_PHANTOM_PAIR_CREATION_TU15.md) preserve the complete
notification argument, current pair/listener rereads, callback decisions,
genuine native array growth and both append orders. Parent and insertion
comparisons pass39 and85 cases under GCC/Clang. The component's dead private
event storage and one synthetic RMW write-fault read-trace limit are explicitly
reported; they do not establish a complete original execution/fault domain.

Executor-local counters have also been added to prove actual kernel selection
in the destruction fixture; the source comparisons still pass with them
installed. These counters never supply or restore gameplay state. The frozen
cohort's eight literal type getters, including `AD710 -> 10`, have separate
physical original-code qualification. The concurrent additional `AE690 -> 21`
getter has now been independently qualified too, and is staged for the live
cohort's next declared source refresh.

The [fresh eight-case gameplay validation](../build/test-results/vehicle-destruction-phantom-live-20261101/live-summary.json)
has completed, including the declared observation refresh and nine qualified
type getters. The power module, Banshee, Wraith and Gauss Warthog pass, totaling
80 complete-world comparisons including the real pending cut. Gauss Warthog
selects/completes the native phantom parent and simple insertion once; Spectre
does so twice before its later failure. Warthog, Ghost, Scorpion and Spectre
now refuse the remaining original list contact-agent factory `2F8030` after
their original death-bit stores. The eight-case suite remains red overall.

An actual typed Warthog factory BEFORE is retained as the required reference
input for implementing/qualifying `2F8030` and `2F7D20`. Its archive is
`analysis/physics/native-list-contact-agent-20261101-required-before/destruction-warthog/actual-native-before.tar.zst`,
12,470,906 bytes, SHA256
`3889dc619e0bdbae5f5e91622057624bbca5fb1b18ea7183159ea5435403804b`.
The archive contains90,002,691 raw bank bytes and its manifest; every member
was round-trip hashed. This is native typed input, with an empty preserved
x87 stack, not an original GP/SIMD frame or expected result. The original
factory/constructor and exception ownership were derived from the pinned
TU1.5 XBE. No child filter, allocation or factory result is supplied.

The active owned source/build is being reused for this next native factory
investigation; its [status](../build/test-results/vehicle-destruction-phantom-create-native-20261101/active-status.json)
records refresh receipts and lifecycle. It is not a retained deliverable.
Component CMake scratch has been removed. No launcher runtime is promoted.
The fixture is muted: no proper-sound or rendered full-gameplay parity is
established. No full-definition gate, D01-D13 row or destruction-sound
acceptance is closed; the complete goal remains active.

## Native list contact agents and final eight-case cohort, 2026-11-01

Native `2F8030`, reversed `2F81F0` and their `2F7D20` constructor now own the
actual Warthog/Scorpion list-agent paths. The [contracts and evidence](NATIVE_LIST_CONTACT_AGENT_TU15.md)
retain original allocation, child-count/filter/class/factory calls, late
argument/backing reads, signed bounds, inline storage and source exception
ownership. Both factories have independent unchanged physical TU1.5 comparison
against their actual typed native BEFORE, with zero/negative counts, one child,
group rejection and native ordinary collection closure. No expected after
image, child admission, class or agent result is supplied. The declared CPU
callers are explicitly distinguished from captured original GP/SIMD frames.

The collection filter separately passes27 declared graph cases under GCC and
Clang, with original ordered field reads, stores, real callbacks, FP and output
pointer. This covers parent chains, group masks, shape unwrapping and the
special user-data/motion branches. These component callbacks still execute
real original service owners; the frozen gameplay cohort admits only its
qualified ordinary native filter closure. Unknown BSP-special/nested owners
retain explicit refusals. Concurrent root wrapper/type/collection work is
preserved outside this declared frozen source delta.

The [final eight-case summary](../build/test-results/vehicle-destruction-phantom-live-20261101/reversed-list-summary.json)
passes the power module, Warthog, Gauss Warthog, Ghost, Banshee, Scorpion,
Wraith and Spectre. It passes144 complete-world restore/replay comparisons,
including the actual module pending cut, and retains all six native countdown
steps and due-before-blast ordering. This is a current MCC/native replay
cohort; it is not original Xbox full-gameplay or framebuffer acceptance.

Required Warthog and Scorpion BEFORE archives are retained under
`analysis/physics/native-list-contact-agent-20261101-required-before/`:
12,470,906 and12,423,702 bytes respectively,24,894,608 bytes total. Every archive
member was round-trip hashed. The [cleanup receipt](../build/test-results/vehicle-destruction-list-contact-agent-20261101/cleanup.json)
verifies removal of989,937,664 allocated disposable probe/project/raw-bank bytes.
All component CMake trees were removed. The finite owned world source/build
remains active for the next pending-response eligibility integration in this
same investigation. No launcher runtime is promoted.

The runtime fixture is muted. Actual destruction sound requests, proper
original waveform/device output, both combined loops, visible full-definition
coverage and remaining semantic gameplay owners stay open. The lack of Xbox
capture hardware does not authorize assumed wet-feedback format or timing.
All37 complete-definition gates, D01-D13 and the complete goal remain active.

## Native region query and response eligibility, 2026-11-01

The [native implementation and qualification](NATIVE_PENDING_RESPONSE_ELIGIBILITY_TU15.md)
now covers `BC6C0` and `DC4C0`. The semantic kernels preserve the original's
query-first behavior, signed descriptor division, two count stores, optional
pointer outputs and count rereads, signed region/state comparisons and
default-allow behavior for absent regions. Invalid object/backing inputs retain
their actual read/write faults; the query does not supply an admission result.
Legacy boundaries preserve full registers, defined flags, source stack/data
stores, observer conventions and partial AL/AX/DX writes. Old-image literals
remain intact, and guarded regeneration retains the new native routes.

Independent original execution passes399 declared cases,22,060 physical i386
instructions and every one of77 source sites, including11 fault prefixes.
The original count arithmetic is separately exhausted for all65,536 signed
descriptor words. GCC and Clang each pass1,596 adapter variants, including both
observer conventions and empty/two-live-value x87 states; seven wrong
implementations are rejected. The registered frozen-project CTest passes.
Required numeric references total48,557 compressed bytes, with no raw banks.

The [actual eight-case native/MCC rerun](../build/test-results/vehicle-destruction-pending-eligibility-20261101/live-eligibility-summary.json)
passes118 full-generation target eligibility observations across the power
module and all seven vehicle cases. Actual authored-field/current-state
readbacks match8 missing-region allows,100 below-requested allows and10
already-at-or-above rejections. The module's pending due -> matching native
eligibility -> original target blast sequence is8 ->9 ->10; its original
six-step countdown remains verified. All144 complete-world restore/replay
comparisons pass. Diagnostic restores are excluded from the live journal.

The admission verifies21 actual transitive headers against the frozen world;
unrelated root work and launcher-selected runtimes are preserved. The original
read-only data mount is authenticated without writing through it. Failed
refresh/fixture attempts and the corrected retry remain separate diagnostics.
This does not qualify the rest of `D8330`, every response/model-state/effect
owner, all37 complete definitions, visible output or any proper destruction
sound/device delivery. Original hardware feedback alignment/time remains open.
The complete native destruction/audio goal stays active.

## Native region-transition parent, 2026-11-01

The [BC720 native parent](NATIVE_REGION_TRANSITION_TU15.md) now owns the semantic
region-count, snapshot-copy and type/attachment decisions with the original
guest stack, DATA lifetime, late operand reads, registers, defined flags and
return behavior. Its model-state, attachment, model notification and object
notification callees retain genuine execution. The selected TU1.5 route uses
the kernel; the guarded generator preserves the route and retained literal.

The [qualification receipt](../build/test-results/vehicle-destruction-region-transition-20261101/qualification.json)
records actual Warthog/state-4 and power-module/state-3 entry inputs. GCC and
Clang together pass25 complete parent returns and six explicitly bounded
type-branch prefixes, covering all83 parent instruction sites and2,197 physical
parent instructions. Each side executes its own original callees on its current
mutable bank. Ordered operands/stores, instruction GP/defined flags, FP/SIMD,
call inputs/returns and2,791,504,733 compared bytes agree. Captured native AFTER
bytes, callee outcomes, allocation results and RNG values are never supplied.
Four private wrong implementations are rejected. Invalid Warthog-to-scenery
type mutations fault in the real attachment owner; their bounded prefixes do
not claim complete scenery behavior.

The authenticated frozen admission verifies22 transitive support headers and
preserves unrelated root work. All eight current MCC/native destruction cases
pass, including144 complete-world restore/replay comparisons and118 native
region-eligibility decisions. The module's due/eligibility/blast order remains
8 ->9 ->10. The current fixed-image generator's only allowed normalization is
`selected(identity,TU15)` to the same immediate; both exact literal hashes are
guarded, and the complete normalized parent reproduces.

Required Warthog and power-module BEFORE archives are retained under
`analysis/physics/native-region-transition-20261101-required-before/`, at
12,459,595 and12,615,018 bytes:25,074,613 bytes total. Every member was round-trip
hashed. AFTER banks and binaries are excluded; an archive-only original/native
recheck passes after raw capture removal. Disposable probes/raw banks totaling
552,849,408 allocated bytes were removed, and all component/regeneration test
owners cleaned their trees. The finite owned world source/build remains active
for the next `1704C0` model-state/candidate/RNG/effect investigation in this same
destruction goal. No launcher-selected runtime is promoted.

The runtime fixture remains muted. This does not prove proper sound requests,
waveform/device delivery, the combined loops or exact original hardware wet
feedback. The user has no Xbox capture access and explicitly requests continued
native work. All37 complete-definition gates, D01-D13, visible/audible acceptance
and the complete destruction/audio goal remain open.

## Native model-region state, 2026-11-01

The [1704C0 native kernel](NATIVE_MODEL_REGION_STATE_TU15.md) now owns the complete
model-region state control flow with the original caller/return ABI. It retains
live candidate records and effect packets, the signed state/count comparisons,
low-byte force guard, five-bit property shifts, signed16 mask, NONE/filter
short circuits, and the model-count reread after each region. All five remaining
candidate/RNG/effect owners retain genuine execution. Guarded regeneration
retains the native route and the exact original literal fallback.

The [component qualification](../build/test-results/vehicle-destruction-model-region-state-20261101/component-qualification.json)
passes45 complete original/native comparisons,14,153 physical model instructions,
all154 instruction sites, every callee boundary and4,051,739,015 compared bank
bytes. GCC and Clang each pass20 declared/actual cases. Additional checks
preserve two exact live80-bit x87 values through authored effect creation and
2/5/32-candidate weighted selection. The32-candidate case chooses index19 and
advances the actual original RNG. No original instruction, candidate answer,
RNG draw, allocation result or native AFTER image is substituted.

The read-only input inventory finds221 resident authored state-effect references
across155 models. A genuine Warthog state supplies the tested effect tag and
parameter; the real source creation owner allocates handle `0xE681000C`. Six
private wrong implementations are rejected. These are component inputs and
original-code evidence, not audible or full-definition gameplay acceptance.

The earlier finite world owner has closed and removed its source/build tree.
A fresh frozen cohort authenticates26 transitive support headers and passes
the seven vehicles and power module with a new actual-target native model-call
journal. [Its live receipt](../build/test-results/vehicle-destruction-model-live-20261101/eight-native-v2/refresh.json)
and [joined qualification](../build/test-results/vehicle-destruction-model-region-state-20261101/qualification.json)
retain138 target-native calls,114 calls with changed region bytes,106 same-step
authored-response matches and42 effect-handle field changes. All144 full-world
comparisons pass, and the module's due/eligibility/blast order remains8 ->9 ->10.
The journal observes real
tag/generation, region pointers/bytes and the `RET40` boundary; it supplies no
damage, model, effect or sound result. Existing BEFORE archives are reused;
there is no new retained large reference and no launcher runtime promotion.

The broader30-case validation completed with20 passes. Five further cases
pass after the fixture's evidence-only store collectors use their read-only
registration API, preserving native physics admission and every destruction
assertion. The [focused rerun](../build/test-results/vehicle-destruction-model-live-20261101/readonly-observers/refresh.json)
and [joined result](../build/test-results/vehicle-destruction-model-region-state-20261101/expanded-current-summary.json)
retain25/30 passing cases and416 complete-world comparisons. That cohort's five
campaign failures stopped at cold animation-resource I/O during immutable
body-pose publication. The subsequent native ownership fix is recorded below.

The [actual sound-request run](../build/test-results/vehicle-destruction-model-live-20261101/actual-sound-requests/refresh.json)
passes all eight core cases with165 complete original160-byte caller requests,
source ticks and resident sound-tag names. Verification replays are excluded.
The [sound journal summary](../build/test-results/vehicle-destruction-model-region-state-20261101/actual-sound-request-summary.json)
records seven original destruction-bank names observed and24 unobserved,
including combined loops and wreck impacts. Sound-name matches do not establish
causal target attribution; source requests and model effect-handle changes do
not substitute for PCM, normal-flow/device, loop or original audible acceptance.
The [PCM/manual-output replay](../build/test-results/vehicle-destruction-model-live-20261101/actual-sound-output-v2/qualification.json)
passes all165 exact callers with1,2 and4 coincident listeners:270 classic PCM
voices are non-silent, emit once and complete;225 authored remastered-only
routes produce no voice. Selected pitch range/permutation and PCM hashes match
across coincident listener counts. This uses declared component listener/RNG
inputs; original timing/permutation, physical device and normal runtime delivery
remain separate gates.

All37 full-definition gates, D01-D13, proper waveform/device output, combined
loops, exact original wet-feedback format/time and remaining visible/local/
network acceptance remain open. The user has no original hardware and asks
for continued native work. The complete destruction/audio goal stays active.

## Cold native animation reads and remaining factory input, 2026-11-01

The [original call graph](../build/test-results/vehicle-destruction-campaign-resources-20261101/original-body-resource-call-paths.json)
and two failing live baselines identify the genuine`126139 ->1A3B70` read below
`BF640`. Native immutable body publication now reuses that existing validated
synchronous animation I/O owner. Both the source cache/stack writes and its
temporary completed native job records/counter are restored. Other I/O, new
backing and synchronization remain refused; no readiness or pose is supplied.

The [qualified cold reads](../build/test-results/vehicle-destruction-campaign-resources-20261101/cold-pose-qualification.json)
pass18 complete native-bank/owner comparisons across nine actual cases. Eight
destruction cases pass128 full-world replay comparisons. The ordinary original
body-render control also
[passes on its declared Lockout input](../build/test-results/vehicle-destruction-campaign-resources-20261101/body-render-lockout-v8/report.json).
The earlier Coagulation control did not reach its required dual-held setup and
is retained as a failing input diagnostic.

The Covenant turret passes its cold reads and reaches a later bounded convex
factory refusal for the actual`AD390`/class8 query. Its shape center/support
pairs are the original sweep and SoA owners. A required typed
[BEFORE reference](../analysis/physics/native-convex-factory-destruction-20261101-required-before/campaign_c_turret_ap/retention.json)
retains22,328,168 compressed bytes and verifies all4,369 archive members.
It contains no original GP frame, AFTER bank or supplied factory outcome.
The subsequent [constructor qualification](../build/test-results/vehicle-destruction-campaign-resources-20261101/convex-bound-class8-control-qualified-v15/report.json)
passes with GCC and Clang across three independent native scratch patterns.
Each comparison matches all113,018,055 captured bytes, return value, all FP
words,81 ordered data reads and18 stores. Two immutable original compiler
switch-table reads are pinned and reported separately from native C++ control
dispatch. All three constructor mutations fail. Only the observed`AD390`
owner is added to the existing class23/empty-x87 binding. Its original failing
fixture verdict remains preserved; live destruction is qualified separately.

The [original sound-shape audit](../build/test-results/vehicle-destruction-waveforms-20261101/original-sound-shapes-v2.json)
covers all31 named destruction banks and750 map-dependent metadata shapes,
with before/after input hashes. The existing prepared original bank container
and AP-specific exporter are not a production destruction-sound binder.
Seven banks exceed that container's literal domain: five single-permutation
banks lack its required used-flag bytes; glass and rocks each retain three
pitch ranges and non-NONE promotion owners. Those domains must be preserved
explicitly before importing exact original payloads. No bank or runtime asset
was published by this read-only audit. Original waveform/device, loop and
normal-flow fidelity remain open alongside all37 full-definition gates.

## Native class8 and complete request/PCM cohort, 2026-11-01

The [qualified native class8 binding and current cohort](../build/test-results/vehicle-destruction-campaign-resources-20261101/class8-and-complete-sound-qualification.json)
pass all30 registered destruction cases on the same frozen production source,
including the previously failing Covenant turret. The cohort retains60 cold
body-pose bank/owner comparisons and480 complete-world replay comparisons.
The bounded native constructor proof compares the actual class23/class8 input
against unchanged TU1.5 on the CPU, with both compilers and independent scratch
patterns. It supplies no class, allocator or constructor outcome.

Actual sound observation now covers the early-return terrain-glass and four
reciprocal blast-chain paths. It stops before explicit verification replays.
The [complete caller journal](../build/test-results/vehicle-destruction-campaign-resources-20261101/complete-cohort-sound-requests-v20.json)
contains561 exact160-byte requests across all30 cases. Eight of31 original
dependency bank names are observed; name matches still do not establish causal
target attribution. The earlier missing-journal failure is retained explicitly
in `sound-journal-coverage-red-v18.json`.

The [PCM replay](../build/test-results/vehicle-destruction-campaign-resources-20261101/complete-cohort-sound-output-v20/qualification.json)
passes all30 cases with1,2 and4 coincident listeners. Each configuration has
304 non-silent finite classic voices, exactly one packet per admitted request,
and257 authored MCC remastered-only silent routes. Listener count preserves
the selected pitch/permutation and PCM hash. This uses declared auxiliary RNG
and listener inputs; it does not verify original voice admission, original
permutation/timing, Xbox waveforms, normal gameplay/device delivery or combined
loops. The original sound-bank runtime binder and its seven unsupported literal
metadata domains remain required work. All37 gates, D01-D13 and the full goal
remain open; lack of original hardware does not authorize guessed wet feedback.

The offline constructor ELF, expanded banks and probe builds are removed.
The required BEFORE archive remains22,328,168 bytes with its verified provenance.
Engine owner cleanup is recorded in
[owner-closure.json](../build/test-results/vehicle-destruction-campaign-resources-20261101/owner-closure.json).

## Original destruction bank domains retained, 2026-11-01

The [V3 native prepared-bank path](PREPARED_ORIGINAL_DESTRUCTION_SOUND_BANKS_TU15.md)
now retains all31 named original banks, including all pitch ranges, literal
promotion rows/rules/counters, optional used storage, original import StringIDs
and complete chunk descriptors. This closes the seven V2 format restrictions
identified above. It does not provide a runtime binder or publish assets.

The [qualification](../build/test-results/vehicle-destruction-original-banks-20261101/qualification.json)
matches all908 map/bank contexts across38 original maps against the independent
shape audit. Both compilers pass65,403 literal/payload comparisons,9,450
permutation comparisons and273 unique same-PC original PCM hashes. Each also
rejects18 malformed controls and five source-fidelity mutations. Original
V1/V2 disk inputs and the existing C++ consumer remain qualified.

The actual runtime still needs allocated/rebased native tag, variation,
promotion and resource owners before this original data can replace its
current content inputs. Correct original flag semantics, source admission,
timing, combined loops and normal live playback remain required. The complete
vehicle destruction/audio goal, all37 gates and D01-D13 remain active.

## Original native sound table/resource owner, 2026-11-01

The [native graph qualification](../build/test-results/vehicle-destruction-runtime-bank-20261101/native-runtime-v4/report.json)
now covers the actual38 MCC tag/StringID namespaces and908 original sound-bank
bindings. `OriginalSoundRuntimeBankTu15` allocates original-ABI table rows,
preserves shared aliases and contiguous intervals, rebases actual imports and
nested pointers, and owns exact original resource sectors. Its complete54-row
class owner and map/salted identities come from authenticated original data.
The [bank documentation](PREPARED_ORIGINAL_DESTRUCTION_SOUND_BANKS_TU15.md#owned-native-tables-and-resources)
records the interface and live-publication boundaries.

Each compiler passes38,900 literal comparisons,169,600 native variation
comparisons,9,450 encoded routes,12,722 sector reads and all273 independent
same-PC PCM hashes, across two independently based graphs. It rejects1,326
invalid inputs, including37 mixed original namespaces. Full salted native tag
indices, actual StringIDs and original flag semantics remain distinct from
MCC-only playback guards.

The original raw and rebased promotion graphs match24,320 calls and260,300
ordered stores per compiler, including every glass/rock rule and suppression.
The [earlier red coverage check](../build/test-results/vehicle-destruction-runtime-bank-20261101/native-runtime-v3/g++/evidence/evidence/runtime-bank.json)
correctly found that the short declared burst missed rocks' final rule. The
qualified input uses256 calls at one declared clock followed by64 explicit
clock steps. The same native kernel consumes both graphs; these are graph
equivalence checks, with separate original-code kernel qualification retained.
They supply no live clock, admission or voice outcome.

The [prepared-bank requalification](../build/test-results/vehicle-destruction-runtime-bank-20261101/native-prepared-v1/report.json)
also passes114,435 literal/payload comparisons per compiler, including every
original class row,25 malformed refusals, seven fidelity-mutation refusals and
V1/V2 compatibility. All authenticated inputs remain unchanged during both
qualifications. Disposable banks, source copies and builds are removed through
their owner; the [closure receipt](../build/test-results/vehicle-destruction-runtime-bank-20261101/owner-closure.json)
records cleanup. No new large reference is retained.

The [final dependency audit](../build/test-results/vehicle-destruction-runtime-bank-20261101/qualification.json)
uses actual CMake compiler commands and dependency preprocessing for both
compilers. All129 compiled source/header dependencies remain current. Concurrent
AI-vector and light-up-vector header edits are outside those closures and are
preserved; this component evidence does not qualify the whole dirty checkout.

Live World publication, its resource/continuation owners and the actual Audio
encoded-resource route still require integration. The graph cannot replace
the global UGH with this subset and discard unrelated sounds. Normal gameplay,
source admission/permutation timing, loops, device output and Xbox waveform
equivalence remain open. All37 full-definition gates, D01-D13 and the complete
vehicle destruction/audio goal remain active.

## Complete native sound prefix retained, 2026-11-01

The original bank graph now accepts a retained, already admitted native table
prefix. It preserves all eleven existing tables and their ordinals, including
custom, promotion and extra-info pointers, while appending the original records.
The full existing class bank remains intact; every selected original class must
match its existing row. The prefix's external chunks retain their resource owner.
This resolves the staged-data problem of discarding unrelated sounds when adding
the31 original destruction banks. Canonical World publication remains open.

The [current append qualification](../build/test-results/vehicle-destruction-sound-publication-20261101/native-prefix-v5/report.json)
uses real MCC metadata through the production Source converter and class
projection, with explicitly declared identity logical relocation. Both compilers
preserve1,323,550 prefix rows/16,808,174 bytes and indices used by109,261 existing
sound definitions across38 contexts. All908 original bindings,9,450 encoded
routes,12,722 sector reads and273 original same-PC PCM hashes still compare.
Native variation/promotion comparisons retain their previous scope. Each
compiler rejects1,706 invalid namespace/owner/class/stride/ordinal/resource
inputs; callbacks releasing caller views retain the real prefix lifetime.
The [standalone regression](../build/test-results/vehicle-destruction-sound-publication-20261101/standalone-v1/report.json)
also passes both compilers with the same production implementation.

The initial raw-prefix attempt correctly rejected an unrecognized serialized
globals namespace. The [measured namespace audit](../build/test-results/vehicle-destruction-sound-publication-20261101/serialized-class-contexts.json)
adds16 exact raw contexts for38 maps. Their complete class-bank hashes already
belong to the existing qualified profile; the selected original classes all
match. The48 word corrections and49 prior cloned/publication contexts remain
unchanged. Full names, salts, addresses, extents, pointer-bearing routes and
whole class banks remain checked. Generator mutation tests and actual C++ raw
namespace mutations verify refusals in both compiler cohorts.

The [consolidated record](../build/test-results/vehicle-destruction-sound-publication-20261101/qualification.json)
retains source/dependency hashes and the staging/live boundary. The
[closure receipt](../build/test-results/vehicle-destruction-sound-publication-20261101/owner-closure.json)
records disposable source, bank, fixture and build removal. No new large
reference or selected runtime is retained or published.

Next, the merged root descriptors must join canonical allocation/publication,
typed timing/epoch and continuation ownership. Sound-cache I/O must select the
real original resource owner by its admitted descriptor, and actual Audio must
consume the matching original metadata and payload together. Normal admission,
timing, loops, split/LAN/device output and original waveform equivalence remain
open. All37 complete-definition gates, D01-D13 and the full goal remain active.

## Exact original sound-cache request ownership, 2026-11-01

The allocated native`21F4BD ->1A3B70` boundary now forwards the real physical
descriptor, encoded locator, complete size/flags word and rounded sector extent
as`SourceSoundCacheReadTu15`. The source allocator retains the cache handle at+8;
the native read, receipt and completion ordering remain unchanged. Both actual
production callers use the typed request. Their current legacy resource owner
remains in place until original-bank publication is admitted.

The original bank owner now accepts exact sound-cache requests through
`read_cache_resource`. Descriptor, locator, flags and whole-sector extent must
match its admitted resource route. A matching locator from another bank does
not authorize a read; prefix chunks retain their existing external owner.
No source cache handle, voice or readiness result is supplied.

The [request qualification](../build/test-results/vehicle-destruction-sound-io-20261101/native-cache-requests-v1/report.json)
passes12,722 original sector requests and63,610 invalid-request refusals per
compiler, including12,722 equal-locator/flags cases with a foreign descriptor.
All273 original same-PC PCM comparisons,908 sound bindings and complete native
prefix preservation still pass. The
[standalone regression](../build/test-results/vehicle-destruction-sound-io-20261101/standalone-cache-requests-v1/report.json)
also passes GCC and Clang.

The [actual original call check](../build/test-results/vehicle-destruction-sound-io-20261101/source-cache-read-v2/report.json)
matches47,616 actual resource bytes and the source done byte, preserves the
retained native receipt/frame behavior and passes all nine failure controls in
both compilers. Forwarded descriptor inputs match the original allocated entry.
Both real caller translation units compile. This proves the native synchronous
boundary on that retained original input, rather than Xbox worker scheduling or
complete live World execution. A refreshed caller cohort preserves concurrent
header edits; the original earlier cohort remains bounded frozen evidence.

The [consolidated source/dependency record](../build/test-results/vehicle-destruction-sound-io-20261101/qualification.json)
and [closure receipt](../build/test-results/vehicle-destruction-sound-io-20261101/owner-closure.json)
record qualification and disposable source, bank, fixture and build removal.
No new large artifact or selected runtime is retained or promoted.

Canonical publication must still install the original roots and bind their
descriptor/resource ownership together with immutable resource identity,
timing/epoch and continuation state. Actual Audio must consume the same original
metadata/payload pair. Normal admission, timing, loops, split/LAN/device output,
original wet-feedback format/time and waveform equivalence remain open. All37
complete-definition gates, D01-D13 and the complete goal remain active.

## Staged canonical original-sound plan, 2026-11-01

The canonical sound stage now preserves the full existing native prefix while
installing the31 bound original banks under their actual full-salt sound roots.
Every UGH root receives the merged descriptors. Existing MCC chunk routes move
to their new physical descriptor addresses; original encoded routes remain with
their own immutable bank. The new sealed lineage retains the parent plan and
original resource owner. Canonical metadata delivery has an explicit input
overload and retains this owner with its admitted metadata.

The [staged matrix](../build/test-results/vehicle-destruction-sound-canonical-20261101/canonical-plan-v3/report.json)
checks38 contexts,908 bindings,969 changed/108,429 preserved roots,
460,082 rebased MCC routes,9,450 original encoded routes and494 refusal cases
per compiler. Its source is actual raw MCC metadata through the generic Source
converter and qualified class stage. Cache-specific authored-flag timing and
complete live canonical publication remain separate acceptance.
The [production syntax and original I/O checks](../build/test-results/vehicle-destruction-sound-canonical-20261101/canonical-consumers-v3/report.json)
cover the World/replay, canonical producer, timing and variation provider with
GCC and Clang. Dependency receipts retain the actual compiled source closure.
The [consolidated qualification](../build/test-results/vehicle-destruction-sound-canonical-20261101/qualification.json)
preserves the v3 native cohort's broad concurrent `frontend.hpp` edit separately
from its unchanged actual compiler dependencies. A caller refresh preserves
concurrent World/physics changes. The
[closure receipt](../build/test-results/vehicle-destruction-sound-canonical-20261101/owner-closure.json)
records removal of disposable banks, sources and builds with no new large
artifact or selected-runtime promotion.

Current timing and variation/PCM consumers explicitly reject the new original
lineage. Next work must authenticate its retained parent, join original timing
and encoded-resource facts under actual memory epochs, route Audio and restores
through the same owner, and admit the asset bundle through normal World
preparation. This stage has no live asset catalog, readiness, voice or device
claim. Normal gameplay, loops, split/LAN, original wet-feedback format/time and
waveform equivalence remain open. All37 complete-definition gates, D01-D13 and
the complete destruction/audio goal remain active.

## Original sound timing joined to the native plan, 2026-11-01

The timing builder now independently rebuilds and validates the retained
canonical parent before joining a sealed original-bank replacement. It keeps
MCC owner diagnostics, moves unrelated timing checks/references to the combined
tables, and admits original stored/permutation times and duration operands
against exact headers, native names and actual encoded bytes. Failed joins
remain atomic. Existing full-salt memory, producer-thread, epoch and resource
proofs remain required. The World duration path can select original payloads
only with the exact same lineage held by canonical metadata delivery.

The [native timing qualification](../build/test-results/vehicle-destruction-sound-timing-20261101/original-join-v3/report.json)
checks38 real raw namespaces,908 original stored headers,9,450 permutation
times/resource routes,38 execution transactions and380 refusal controls per
compiler, with zero timing writes. The raw namespace fixture retains actual
serialized null slots and native import names. Its generic parent has zero
admitted MCC timing bindings, so positive Cache-aware prefix-reference rebasing
remains an explicit acceptance gap. Existing timing lease/permutation tests
and class-profile generation also pass. The
[consumer qualification](../build/test-results/vehicle-destruction-sound-timing-20261101/production-consumers-v2/report.json)
checks the World/replay, canonical producer, timing and variation provider with
both compilers and repeats the original allocated sound-cache read case.

The [consolidated record](../build/test-results/vehicle-destruction-sound-timing-20261101/qualification.json)
retains exact source/dependency evidence and the current limits. The
[closure receipt](../build/test-results/vehicle-destruction-sound-timing-20261101/owner-closure.json)
records disposable bank/source/build removal with no new large artifact.
Next work joins immutable original resource identity and descriptor I/O,
variation/Audio metadata and PCM, continuation and the normal asset/preparation
path. Actual World admission/execution, loops, split/LAN/device output and
original waveform/wet-feedback equivalence remain open. All37 full-definition
gates, D01-D13 and the complete destruction/audio goal remain active.

## Original resources bound before publication, 2026-11-01

The original lineage now binds its sealed metadata layout, provenance and full
sector payloads into the resident resource identity. The existing serialized
identity shape and checkpoint comparison remain intact. An explicit original
resource preparation constructs metadata, timing and resources before registry
publication and requires every replacement's timing inputs. Both owners are
consumed once; an active consumed original preparation rejects retries instead
of falling back to ordinary assets.

The World checks the resource/metadata owner before publishing sound tags and
when selecting original duration payloads. Both actual sound-cache callers now
use the typed resource method. Exact original descriptors read retained sectors;
validated prefix descriptors retain their existing external policy. Invalid
physical descriptors, locators, flags or sector sizes reject before delegation.

The [resource qualification](../build/test-results/vehicle-destruction-sound-resources-20261101/identity-descriptors-v2/report.json)
passes12,722 original sector reads,460,082 prefix-route validations and342
refusals per compiler across38 contexts. Identical independent publications
have the same resource digest; moving the canonical allocation changes it and
rejects old descriptors. All existing original timing/PCM/variation/promotion
and class-profile checks retain their component scope. External prefix byte
reads are zero in this cohort; native MCC DAT backing remains unqualified.
The [six-consumer check](../build/test-results/vehicle-destruction-sound-resources-20261101/production-resources-v3/report.json)
compiles the resource/preparation implementation and its real callers with
both compilers and repeats the original allocated sound-cache read case.

The [consolidated record](../build/test-results/vehicle-destruction-sound-resources-20261101/qualification.json)
and [closure receipt](../build/test-results/vehicle-destruction-sound-resources-20261101/owner-closure.json)
retain bounded evidence and verify disposable bank/source/build removal.
Actual Cache/preparation execution, retry and checkpoint acceptance, positive
preserved-prefix timing rebasing, native MCC DAT ownership, variation/Audio
metadata/PCM joining, continuation and product asset admission remain required.
Normal gameplay, loops, split/LAN/device output and original waveform/wet-feedback
equivalence remain open. All37 full-definition gates, D01-D13 and the complete
destruction/audio goal remain active.

## Actual Cache original preparation qualified, 2026-11-02

The [Cache qualification](../build/test-results/vehicle-destruction-sound-cache-20261102/qualification.json)
builds and links the complete frozen Cache library, with 404 compiled objects
and 1,114 source dependencies. All 38 actual Cache contexts pass original
preparation, registry consumption/retry refusal and resource identity checks.
The actual resource boundary reads 12,722 original whole sectors across 908
bindings and 9,450 encoded routes. Cache bytes remain unchanged. The serialized
resource identity shape remains intact; checkpoint restoration is still open.

The [complete matrix](../build/test-results/vehicle-destruction-sound-cache-20261102/cache-matrix-v2/report.json)
also preserves 268,007 admitted MCC timing bindings, 74,603 stored headers and
268,097 permutation-time references at their combined table addresses. Actual
canonical images pass current-memory binding plus preserved/original resource
lookups in read-only fixtures. The fixtures declare their lookup entry and
selector globals; they execute no World or native timing caller. The matrix
passes 342 refusal controls with zero timing-module writes. The ordinary Cache
timing conversion CTest also passes.

The corrected test takes original map context from the pinned inventory,
including original `highplains2.map` paired with MCC `highplains.map`. Its first
37/38 result and explicit test/CMake-only refresh receipts remain available.
No production bypass was introduced. Results apply to the frozen closure;
later concurrent `frontend.hpp` and `physics_contact_runtime.hpp` edits were not
built. The [closure receipt](../build/test-results/vehicle-destruction-sound-cache-20261102/owner-closure.json)
verifies removal of disposable source, banks and builds with no new large
artifact or selected-runtime promotion.

Positive Cache preparation and prefix timing rebasing now have execution
evidence. Native MCC DAT ownership, original variation/Audio metadata and PCM,
continuation, product asset catalog admission, live World playback, loops,
local/split/LAN/device output and original waveform/wet-feedback equivalence
remain required. Hardware access is unavailable; continue native work without
inventing wet-feedback format or timing. All 37 complete-definition gates,
D01-D13 and the complete destruction/audio goal remain active.

## Original variation and PCM joined to Audio, 2026-11-02

The native variation provider now retains the sealed original resource owner
paired with canonical delivery. Its encoded loader validates current native
metadata, full-salt identity, permutation ordering and selected descriptor
locator/flags in one retained transaction. Originals use their exact bank;
preserved sounds use the actual Cache DAT loader. Runtime chunk handles remain
source-owned. Audio uses this loader before decoding a bound provider's PCM.
Restore preparation compares the target and saved original resource identities
before retaining saved PCM; it still requires the genuine target native binding.
This phase executes the resource compatibility check in component fixtures,
not actual original-bank checkpoint restoration.

The [complete matrix](../build/test-results/vehicle-destruction-sound-pcm-20261102/pcm-matrix-v4/report.json)
passes all 38 Cache contexts: 1,060 original variation definitions, 9,450 exact
encoded routes and 908 selected original PCM submissions through the real
Audio/SDL manual mixer. Each submission's frame count matches its selected
original PCM. In 603 cases that count differs from the old MCC load. A finite
256-frame manual mix is checked per voice; complete playback, source event
admission and physical audibility are not claimed. The declared Audio RNG seed
and read-only native memory fixture remain explicit.

Existing actual preparation, sector reads and positive prefix timing checks
still pass, with 722 refusal controls and zero source-memory writes. Different
original resources reject compatibility, stale saved epochs permit immutable
comparison with a fresh matching target, and stale cached Audio cannot advance
the backend. Actual tutorial/Cairo table 6 `{0,0}` is preserved as empty storage;
the provider's previous positive-size allocation check had rejected it.
Nonempty extents and every empty-table ordinal remain checked.

The [qualification](../build/test-results/vehicle-destruction-sound-pcm-20261102/qualification.json)
records the full frozen Cache and real Audio closure, 441 objects/1,547 source
dependencies, ordinary continuation checks, initial failures and declared
refreshes. Unused World functions are removed by ELF section collection; this
does not establish complete World linking or execution. Concurrent worktree
changes remain separate. The [closure receipt](../build/test-results/vehicle-destruction-sound-pcm-20261102/owner-closure.json)
records disposable bank/source/build removal with no new large artifact or
selected-runtime promotion. Normal product catalog activation, native MCC DAT
worker backing, actual original-bank restoration, live World playback, loops,
local/split/LAN/device output and original waveform/wet-feedback equivalence
remain required. All 37 complete-definition gates, D01-D13 and the complete
destruction/audio goal remain active.

## Original banks in actual native Worlds and restores, 2026-11-02

The [complete native matrix](../build/test-results/vehicle-destruction-sound-world-20261102/world-matrix-v1/report.json)
passes all 38 admitted map contexts. It builds and links the actual World and
systems libraries, consumes original-bank preparation, initializes real Worlds,
and executes native `213410` selection and `213690` / `21F590` duration calls.
There are 908 original bindings, stored-time queries and selected PCM
submissions, plus 946 authenticated duration-caller observations. The observer
checks actual original encoded byte/rate/encoding/compression inputs and
supplies no result. Audio uses the actual World provider and manual SDL mixer;
packet frame counts match selected original PCM.

Native checkpoints retain original resource identity in every context.
Ordinary/changed identities reject, native selection/duration replay matches
after restore, stale metadata/Audio epochs reject, and fresh bindings play
again. The matrix passes 380 refusal controls and preserves Cache bytes.
Each World uses the declared multiplayer session variant, mode 1, 30 Hz and
seed `0x6a19d3e1`, including campaign map namespaces. It qualifies sound
integration in those Worlds, not complete campaign/multiplayer behavior.
Audio is quiescent during native bank restore; already-playing Audio handoff
and its genuine owning publication scope remain required.

The [preserved native metadata/Audio and authored-time tests](../build/test-results/vehicle-destruction-sound-world-20261102/preserved-native-v1/report.json)
also pass. The [qualification](../build/test-results/vehicle-destruction-sound-world-20261102/qualification.json)
records 1,844 compiled units and 3,545 consumed frozen source dependencies,
with generated/third-party dependency pages and eight concurrent checkout
differences. This is a frozen execution result and changes no production
behavior. The [closure receipt](../build/test-results/vehicle-destruction-sound-world-20261102/owner-closure.json)
verifies disposal of source, banks, build and checkpoint scratch with no new
large artifact or selected-runtime promotion.

Native World publication and native-bank restoration with original resources
now have positive execution evidence. Normal product asset catalog activation,
owning-factory and playing-Audio continuation, native MCC DAT worker ownership,
source voice allocation, ordinary destruction event-to-device output, loops,
local/split/LAN acceptance and original waveform/wet-feedback equivalence remain
open. All 37 complete-definition gates, D01-D13 and the complete destruction/audio
goal remain active.

## Portable original sound catalog and preparation, 2026-11-02

The new production `PreparedOriginalSoundCatalogTu15` authenticates a portable
catalog against an external digest, selects its exact complete converted Cache
snapshot and filename, then joins original bank provenance and authored pitch/
permutation names to unique Classic tags and the actual native gestalt imports.
Its immutable input retains bank and binding storage. The new preparation
overload carries this input into the same canonical metadata/resource owner;
it creates no World or native eligibility token.

The [38-context matrix](../build/test-results/vehicle-destruction-sound-catalog-20261102/catalog-matrix-v1/report.json)
passes 908 original bindings, 9,450 encoded routes and 342 refusals. Every case
executes actual preparation, retains its complete resource identity after
bundle relocation, preserves returned input storage and leaves Cache bytes
unchanged. The existing MCC timing conversion regression also passes. The
[qualification](../build/test-results/vehicle-destruction-sound-catalog-20261102/qualification.json)
records the frozen compiler closure and the unrelated concurrent checkout
change. The [closure receipt](../build/test-results/vehicle-destruction-sound-catalog-20261102/owner-closure.json)
verifies removal of disposable sources, banks, binaries and builds, with no new
large artifact or selected-runtime promotion.

This qualifies the catalog module and explicit preparation API using declared
portable fixtures. Product bundle production and normal map/factory admission
remain the next integration step. Owning-factory and playing-Audio continuation,
native MCC DAT worker backing, source voice allocation, ordinary destruction
event-to-device output, loops and local/split/LAN acceptance remain open. The
user has no Xbox hardware access; native work continues, and original waveform
and wet-feedback format/timing remain unverified. All 37 complete-definition
gates, D01-D13 and the complete destruction/audio goal remain active.

## Original sound bundle production and default loading, 2026-11-02

The [private bundle producer](../build/test-results/vehicle-destruction-sound-admission-20261102/production-v1/report.json)
now executes actual native Cache conversion/catalog binding in all 38 contexts.
It retains existing MCC assets in distinct private reflink files, admits the
908 authenticated original bank bindings and publishes an exact catalog/map
pointer in the candidate's preparation provenance. Source assets are unchanged.
Normal Cache loading authenticates indexed encoded map ownership and binds the
catalog against the complete normalized snapshot. A bad declared catalog fails
loading; unrelated and resource map owners acquire no original sound binding.

The [default loading matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/admission-matrix-v1/report.json)
passes all 38 contexts, 908 bindings, 9,450 encoded routes and 228 controls.
Default countdown preparation admits originals automatically. Direct resources
retain their own paired canonical metadata and resource owner for one-shot
World consumption. Both paths preserve the same immutable resource identity,
reject wrong layouts, cross-owner plans and repeated consumption, and leave raw
Cache bytes unchanged. The metadata-owner suite adds 15 sound-pointer refusals;
the existing full catalog component matrix still passes 342 refusals. The
[component qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/component-qualification.json)
records exact evidence and limits.

The [full native World build](../build/test-results/vehicle-destruction-sound-admission-20261102/active-build.json)
is running in the same owned investigation. Its actual direct/countdown World,
native sound, PCM and restore checks remain pending. The candidate is disposable;
no product asset selection or selected runtime has changed. Genuine owning-
factory and playing-Audio continuation, native MCC DAT worker backing, source
voice allocation, ordinary destruction output, loops, local/split/LAN acceptance
and original waveform/wet-feedback fidelity remain required. All 37 complete-
definition gates, D01-D13 and the complete destruction/audio goal remain active.

## Default-loading native Worlds and retained DAT resources, 2026-11-02

The [direct](../build/test-results/vehicle-destruction-sound-admission-20261102/world-direct-matrix-v1/report.json)
and [countdown](../build/test-results/vehicle-destruction-sound-admission-20261102/world-countdown-matrix-v1/report.json)
matrices pass 38 actual native World contexts each using the produced sound
bundle and normal Cache admission. Across 76 Worlds they execute 1,816 original
selections, stored-time queries, duration queries and manual PCM voices, observe
1,892 actual duration callers, and pass 760 refusals. Native restore retains
resource identity, replay matches, stale epochs reject and fresh bindings play
again. These remain declared sound-integration Worlds with quiescent Audio at
restore and finite manual mixing, not complete campaign/multiplayer acceptance.
The [native qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/native-qualification.json)
retains the 1,858-object compiler closure and its exact limits/source drift.

The [retained-prefix census](../build/test-results/vehicle-destruction-sound-admission-20261102/prefix-dat-audit-v1/report.json)
also identifies a unique DAT owner for all 460,086 descriptors in 38 contexts.
It finds 346,396 unaligned offsets and 1,842 offsets overlapping Xbox location
bits; full 32-bit MCC offsets and actual sector padding must be preserved.
The new immutable `MccSoundDatResourcesTu15` and resource/preparation integration
are under validation in the same owned build. This phase has not yet qualified
actual native-prefix scheduling, cache readiness or source voice allocation.
The owning-factory/playing-Audio handoff, ordinary destruction output, loops,
local/split/LAN/device acceptance and original waveform/wet-feedback fidelity
remain required. All 37 complete-definition gates and D01-D13 remain active.

## Typed DAT resource delivery qualified, 2026-11-02

The [DAT byte matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-admission-matrix-v1/report.json)
now passes all 38 contexts: 460,086 retained-prefix routes and 11,538,438,144
padded bytes match independent Cache audio and whole source DAT sectors. It
covers every unaligned/high offset, preserves the 908 original bindings and
9,450 original routes, and passes 380 refusal controls. Private full DAT
snapshots now contribute to immutable resource identity, with no guessed
location bits, padding, cache handles or completion values.

The [direct](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-world-direct-matrix-v1/report.json)
and [countdown](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-world-countdown-matrix-v1/report.json)
native World matrices also pass all 38 contexts each after this integration,
with 1,816 original sound selections/PCM voices, 1,892 actual duration-caller
observations, deterministic native restore and 760 refusals. The
[qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-qualification.json)
records full frozen compiler inputs and scope. The
[closure receipt](../build/test-results/vehicle-destruction-sound-admission-20261102/owner-closure.json)
verifies removal of disposable builds, banks and candidate. No selected runtime
or product asset selection changed.

Retained-prefix variation PCM still reads through Cache and must join the same
DAT snapshot and complete resource-identity preflight. Actual native-prefix
scheduling/voice delivery, owning-factory and playing-Audio continuation,
ordinary destruction output, loops, local/split/LAN/device acceptance and
original waveform/wet-feedback fidelity remain open. All 37 complete-definition
gates, D01-D13 and the full destruction/audio goal remain active.

## Retained-prefix PCM joined to immutable DAT owners, 2026-11-02

The [38-context parser matrix](../build/test-results/vehicle-destruction-prefix-pcm-20261102/prefix-pcm-matrix-v1/report.json)
passes 425,146 retained-parent permutations and 460,086 encoded chunk reads,
including exact format and decoder packet/reset metadata. It preserves the
complete sector/unaligned/high-offset coverage, 908 original bindings and 9,450
original routes, with 608 refusal controls and unchanged Cache bytes.

Variation PCM and native duration resources now use the World's retained DAT
snapshots. Audio replacement compares the complete immutable resource identity;
native publication also checks the World's exact retained data owner.

The [preparation matrix](../build/test-results/vehicle-destruction-prefix-pcm-20261102/preparation-matrix-v1/report.json)
passes all 38 contexts and all 9,450 original encoded routes, with 908 manual
PCM voices and 722 refusals. Both [direct](../build/test-results/vehicle-destruction-prefix-pcm-20261102/world-direct-v2/report.json)
and [countdown](../build/test-results/vehicle-destruction-prefix-pcm-20261102/world-countdown-v2/report.json)
World matrices pass 38 contexts each, preserving the previous 1,816 original
voice/selection cases and adding 76 observed retained-prefix duration callers
with deterministic restore replay. Original timing arithmetic is checked
against its authenticated reference separately from actual MCC payload fields.

The [owned-copy controls](../build/test-results/vehicle-destruction-prefix-pcm-20261102/retained-control-v2/report.json)
verify new provider/uncached Audio delivery with all three DAT paths absent,
PCM/native-sector isolation from an actual encoded-byte overwrite, exact World
publication ownership and complete-identity refusal after a real DAT change.
Equal content across independent owners and a stale saved epoch remain valid
for immutable resource comparison. The private clone is removed and original
asset owners remain unchanged. The [qualification](../build/test-results/vehicle-destruction-prefix-pcm-20261102/qualification.json)
retains the frozen compiler evidence and scope; narrow Audio link regressions
also pass with the real resource implementation.

The full scheduling/source-voice, owning-factory/playing-Audio, destruction
event-to-device, local/split/LAN and original waveform/wet-feedback gates remain
open. All 37 complete-definition gates and D01-D13 stay active; this work does
not promote a selected runtime.

The [closure receipt](../build/test-results/vehicle-destruction-prefix-pcm-20261102/owner-closure.json)
verifies removal of disposable source/build/banks/candidate and empty scratch
parents. No new large artifact is retained; required original reference
evidence remains unchanged.

## Original-bank native handoff: measured archive prerequisite, 2026-11-02

The [original-bank handoff fixture](../build/test-results/vehicle-destruction-audio-takeover-20261102/qualification.json)
admits the exact produced sound bundle through authenticated local sessions and
normal native shadow admission. Its later checks target the real prospective
common-cut factory. It compiles the playing-voice checks but **executes zero Audio
publication cases**: the complete paired native archive rejects first at the
existing 96 MiB decoded network limit. The failed result remains explicit.

The [matched offline census](../build/test-results/vehicle-destruction-audio-takeover-20261102/archive-budget-summary.json)
measures 118,737,415 decoded bytes for the unchanged pre-catalog source and
119,056,981 for the produced original-sound candidate, each with twelve actual
native players. Both exceed the same 100,663,296-byte limit. No larger-policy
image is sent over the network, and no limit, native guard or readiness check
is relaxed. Exact owned-input admission and three refusal controls pass.
Seventeen concurrent checkout changes remain outside this frozen build.

This establishes a reproducible prerequisite rather than factory or Audio
acceptance. Native cache scheduling/source voices, playing-Audio continuation,
complete destruction event-to-device behavior, loops, local/split/LAN delivery
and original waveform/wet-feedback fidelity remain open. All 37 full-definition
gates, D01-D13 and the complete goal remain active. No selected runtime was
changed by this work.
The [closure receipt](../build/test-results/vehicle-destruction-audio-takeover-20261102/owner-closure.json)
verifies disposable build/candidate cleanup and preserves the required original
reference; no new large artifact is retained.

## Original-bank client playback corrected, 2026-11-02

The immutable client path now admits original sound metadata and encoded
payloads together. Previously it bypassed the catalog and used MCC sounds;
the [pre-fix Lockout test](../build/test-results/vehicle-destruction-client-sounds-20261102/baseline-red-v1/report.json)
fails all 26 original bindings. The corrected native component tests pass,
including the ordinary MCC metadata control. No native World epoch or
preparation-registry owner is fabricated by this worldless path.

The [38-context matrix](../build/test-results/vehicle-destruction-client-sounds-20261102/client-matrix-v1/report.json)
passes 908 original bindings/PCM voices, 1,060 pitch definitions, 9,450 encoded
routes and 190 refusals. All 908 voices also restore a playing cursor and replay
exact PCM with fresh receipts while preserving the physical output clock.
The [561-request replay](../build/test-results/vehicle-destruction-client-sounds-20261102/actual-and-loops-v1/evidence/destruction-actual-audio.json)
passes all 30 captured cases for 1/2/4 coincident listeners. Across those three
configurations, 162 original-bank and 750 retained-MCC PCM submissions complete;
771 authored silent routes remain silent. These are declared component replays,
not live source-event or physical-device acceptance.

The [loop comparison](../build/test-results/vehicle-destruction-client-sounds-20261102/loop-comparison.json)
is explicitly incomplete: original data passes three of seven profiles. Two
require genuine multi-chunk queues, with nine original permutations now pinned;
two require four-bin combined output and also fail on the MCC baseline. Native
queue/retirement and combined playback owners remain required, with all source
guards preserved. The [qualification](../build/test-results/vehicle-destruction-client-sounds-20261102/qualification.json)
keeps its overall verdict failed and records five concurrent source changes
outside the frozen cohort. All 37 complete-definition gates and D01-D13 remain
active alongside genuine native handoff, source cache/voices, normal local/LAN
delivery and original waveform/wet-feedback proof. No runtime is promoted.

The [closure receipt](../build/test-results/vehicle-destruction-client-sounds-20261102/owner-closure.json)
verifies removal of disposable source/build/banks/candidate and preservation of
the required original reference. No new large artifact is retained.

## Stock stream packet CPU owners reconstructed, 2026-11-02

Native sound kernels now own the actual TU1.5 `21F360`, `2B98D0` and `2B9950`
entries. They retain the exact cache-data reference/address lookup, stock packet
descriptor/cursor behavior and accepted-submission bitfields. No source voice,
cache allocation, readiness or device result is supplied by these CPU helpers.

The [fresh direct-XBE comparison](../build/test-results/vehicle-destruction-chunk-queue-20261102/qualification.json)
passes 1,530 raw-ABI cases, with 1,519 complete register/FP/SIMD returns, 11
matching storage faults and 7,074 observable ordered writes outside callee
scratch. All 80 original instruction sites execute unchanged. A separate repeat
has the same fixture hash. Salted/`NONE` cache tokens, reference saturation,
signed count/cursor fields, masked sizes and shifts are covered; four deliberately
incorrect native implementations are rejected. The existing sound-resource
suite also passes all 56,000 comparisons. The compiler closure contains 31
objects/two archives and has no current compiler-input drift.

The full loop result remains three of seven profiles: actual enqueue/completion,
cache retirement and decoder-owned multi-chunk PCM boundaries are still needed,
alongside four-bin combined playback. Source guards remain intact. All 37
complete-definition gates, D01-D13, source voice admission, genuine handoff,
normal local/LAN/device delivery and original waveform/wet-feedback equivalence
remain open. The [closure receipt](../build/test-results/vehicle-destruction-chunk-queue-20261102/owner-closure.json)
records disposable build/fixture removal, with the required original reference
and selected runtimes preserved. No new large artifact is retained.

## Stock chunk retirement CPU owners reconstructed, 2026-11-02

Native kernels now own TU1.5 `2B82A0` and `2B99C0`: queue/submitted count
retirement, pin debit, actual forward CRT pointer compaction, freed-slot clear,
stream mode and callback data-reference debit. Completion releases for success,
flush and failure; the original outer `227140` owns success-only pumping.
These helpers do not supply a source voice, admission or device completion.

The [direct-XBE qualification](../build/test-results/vehicle-destruction-chunk-retirement-20261102/qualification.json)
passes 1,302 raw-ABI cases, with 962 complete register returns, 340 matching
faults and 16,538 ordered observable writes. It executes 196 unchanged original
instruction sites, including the real CRT copy, under the source ABI's clear
direction flag. A separate repeat has identical fixture bytes. Six incorrect
native implementations are rejected, including a stale post-compaction handle
on its actual alias case. Existing packet checks pass 1,530 cases and the
resource suite passes 56,000 states, with 4,752 matching faults. Compiler
closure and any unrelated current checkout drift are recorded separately.

The original loop result remains three of seven profiles. Genuine enqueue,
accepted two-slot PCM delivery, decoder-owned source-byte/frame boundaries and
completion-pump integration remain required, alongside native four-bin combined
playback. All guards, 37 complete-definition gates and D01-D13 remain open,
including source voice admission, genuine handoff, normal local/LAN/device
sound delivery and original waveform/wet-feedback equivalence. The
[closure receipt](../build/test-results/vehicle-destruction-chunk-retirement-20261102/owner-closure.json)
records private build/fixture removal and preservation of the required original
reference. No new large artifact is retained and no runtime is promoted.

## Original resource boundaries preserved through decoding, 2026-11-02

Prepared and native original-bank loaders now retain their authenticated chunk
byte extents in the encoded playback value. The pre-fix check failed on all
9,450 routes; the corrected loaders pass all routes across 908 contexts.
The native PC decoder records the actual emitted 48-kHz frame endpoint at each
encoded chunk boundary, keeps its state across chunks and assigns final flush
frames to the last chunk. MCC Opus mapping follows its existing decoded-chunk
and final-permutation trim. No Xbox timing or waveform is inferred from this.

The [qualification](../build/test-results/vehicle-destruction-decoder-chunks-20261102/qualification.json)
keeps PCM byte-identical to the captured pre-fix decoder for 273 distinct
permutations, including 55 multi-chunk permutations. It covers 375 real chunks,
24 independent known-PCM resampler cases, zero-frame and delayed boundary
witnesses, 121 extent refusals and existing MCC Opus controls. Six incorrect
implementations are rejected. The compiler closure has 26 objects and one
archive, with no current compiler-input drift. Tag/StringID owners are declared
component inputs; no source voice/cache admission or device result is supplied.

Actual Bank/voice publication and accepted two-slot chunk playback still need
to consume this mapping and compose with the native retirement owners. Native
four-bin combined output is also required. Guards, all 37 complete-definition
gates, D01-D13, genuine handoff, normal local/LAN/device delivery and original
waveform/wet-feedback proof remain open. The
[closure receipt](../build/test-results/vehicle-destruction-decoder-chunks-20261102/owner-closure.json)
verifies removal of disposable build/banks and preservation of the original
reference. No new large artifact or runtime promotion is added.

## Original multi-chunk output and two loop failures fixed, 2026-11-02

The selected-loop output now retains its decoder map and unsubmitted PCM
remainder, fills the actual two-slot SDL queue, refills after real successful
consumption and preserves per-chunk gain and checkpoint state. Actual full
queue backpressure prevents another controller selection. Flush accounts for
accepted packets and retires the remainder without synthesizing a successful
completion or restarting playback. Single-chunk ports retain their guard;
multi-chunk admission requires the implemented queue and retained decoder map.

The [qualification](../build/test-results/vehicle-destruction-chunk-playback-20261102/qualification.json)
covers all 55 distinct original multi-chunk permutations in 95 real native
output cases across Coagulation and Sentinel Walls. It compares uninterrupted
PCM bits, first-boundary consumption before refill, dynamic gain, playing
restore, unsubmitted remainder, backpressure and flush. Six incorrect Core
implementations are rejected. Banshee phase, source ring, Core ownership and
retaining-MCC continuation regressions pass. The frozen closure pins 457
objects/14 archives and six unrelated concurrent physics/weapon input changes;
those checkout changes are preserved.

Original destruction-loop profiles improve from three to five of seven, with
both metal-thick/hubcap alternate-only profiles passing. The
[remaining failures](../build/test-results/vehicle-destruction-chunk-playback-20261102/loop-comparison.json)
are tiny Covenant damage and gasoline fire, both still refused by the native
four-bin combined-output guard. These checks use declared native controller
inputs and manual SDL consumption. Genuine source scheduler/cache/voice
admission, native handoff, normal local/LAN/device delivery and Xbox waveform/
wet-feedback proof remain open, alongside all 37 complete-definition gates and
D01-D13. The [closure receipt](../build/test-results/vehicle-destruction-chunk-playback-20261102/owner-closure.json)
records disposable build/catalog removal and preservation of the original
reference. No new large artifact or selected-runtime change is added.


## Native stream submission/completion progress, 2026-11-02

The native sound resource registry now includes original TU1.5 availability
`2B9A00`, rejected-packet cleanup `2B9980`, submission `2B8530` and completion
`227140`. Rejection releases the retained data reference without consuming a
chunk or incrementing submitted state. All callback statuses retire the chunk;
only successful completion with the original guard clear permits refill.
The actual caller chain exposed previously omitted stack writes in
`2B99C0 -> 2B82A0 -> 32A990`: the following pump reuses those bytes. The native
helpers retain them, and the original retirement capture now checks their
final stack values independently.

The [qualification](../build/test-results/vehicle-destruction-native-stream-pump-20261102/qualification.json)
passes 1,367 comparisons against unchanged original instructions, including
176 matching faults, every callee-entry GPR/argument and non-stack page hashes.
The explicitly native test adapter exercises actual SDL queue outcomes and
24 real success/flush/removal events. Nine incorrect compiled variants are
rejected, including missing stack writes and refill after flush. The restored
repeat is identical; packet/retirement/resource regressions pass. The frozen
compiler closure has 36 objects/two archives and no checkout input drift.

This closes a CPU component qualification only. The adapter does not supply
source World, scheduler, cache or voice admission. Its synthetic native PCM
mapping and input-boundary annotation do not prove retail SDK behavior or Xbox
waveforms. Production handoff, full local/split/LAN audible delivery and the two
four-bin combined profiles remain open; no hardware access is assumed. All 37
complete-definition gates and D01-D13 remain open. No selected runtime is
promoted. The [closure receipt](../build/test-results/vehicle-destruction-native-stream-pump-20261102/owner-closure.json)
records owned scratch removal, no new retained large artifact, and preservation
of the required original reference.


## Native stock queue and pin lifetime, 2026-11-02

Native kernels now implement original `2B8210` enqueue and `2B85F0` queued-pin
release, plus the stock codec's exact `2E4D60` ret4. They retain the authored mode
table, signed queued fields, wrapping pin bytes and tail-first release. The
pin request uses the original `21F260(-1, chunk, 4)` path, with no allocated
entry, I/O completion, readiness or voice result supplied. Its private frame
writes are retained so later source callers receive the original bytes.

The [qualification](../build/test-results/vehicle-destruction-native-stream-queue-20261102/qualification.json)
passes 5,579 original-code cases, 82 matching faults, full mapped-memory/stack
checks, 22,026 ordered stores and exact CPU-callee GPR/argument frames. Two
independent original captures match. Nine compiled mutations are rejected,
including wrong pin wrapping, head-first release, data-reference release,
stale post-pin queue indices and an omitted frame byte. The restored cohort
also passes the packet, retirement, resource and real SDL-service regressions;
38 objects and two archives have no compiled-source checkout drift.

This is CPU component progress. It does not qualify a source cache scheduler,
source voice, production SDK handoff, normal local/split/LAN sound delivery or
Xbox waveform/wet-feedback behavior. The earlier twelve-player archive-budget
failure is retained, not rerun or bypassed here. The two combined four-bin
profiles, all 37 complete-definition gates and D01-D13 remain open. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-queue-20261102/owner-closure.json)
records scratch/fixture removal and preservation of the required original
reference. No new large artifact or selected-runtime promotion is added.

## Authenticated sound chunks through native lifecycle, 2026-11-02

The [qualification](../build/test-results/vehicle-destruction-native-stream-payload-20261102/qualification.json)
now composes the existing TU1.5 stock stream kernels with actual authenticated
original resources and real manual SDL consumption. It covers all 273 distinct
permutations, 375 chunks and 55 multi-chunk permutations represented by the
908 bank contexts, 9,450 routes and 31 sound names. Every playback interval
matches uninterrupted same-PC PCM bit for bit. All 1,147 playback, flush,
removal and rejection scenarios pass. Native source callbacks account for 532
successful packets, 273 flushed packets and 273 failed packets. Unsubmitted
chunks receive pin cleanup without fabricated callbacks.

The earlier test-only SDL service adapter acknowledged completion too early.
It now retains completed PCM until the native or unchanged-XBE source callback
returns. All 1,367 original-code comparisons pass with that corrected policy.
Eight compiled incorrect variants are rejected, and all six restored resource,
packet, retirement, queue, service and authentic-payload suites pass. Compiler
receipts pin 50 objects and three archives without source dependency drift;
the unrelated concurrent CMake append is preserved separately.

This advances native sound component verification. Already-loaded cache rows,
unity gain/rate and the manual producer schedule remain explicit inputs.
Source World/cache/voice/device admission, original event timing, selection,
gain/spatial behavior, production handoff, normal local/split/LAN delivery and
Xbox waveform/wet-feedback parity remain open. Seeked and zero-output packet
mapping are outside this whole-chunk qualification. The two combined four-bin
profiles, all 37 complete-definition gates and D01-D13 remain open; the earlier
archive-budget failure is neither rerun nor bypassed. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-payload-20261102/owner-closure.json)
records private source/build/bank removal, preservation of the required
original reference and no new large artifact or selected-runtime promotion.

## Native cache-allocation dependencies, 2026-11-02

Core-owned `16E6A0`, `16E770` and `16E860` now retain all eight GPR outputs,
private frames and DF-directed clears for source CPU callers. The sound registry
delegates to these core implementations. The
[qualification](../build/test-results/vehicle-destruction-native-cache-datum-20261102/qualification.json)
passes 4,390 unchanged-XBE cases, 770 matching faults and all 152 static sites,
with complete mapped-memory/final-frame checks. Independent captures match,
and eight compiled wrong variants are rejected. All ten restored suites pass;
the authenticated 273-permutation/1,147-scenario audio report is identical to
the prior result, and the format C1 reference limitation is retained.

The original `21F474 -> 16E770` cache-allocation edge is verified, but native
page allocation `13FE50`, complete cache binding `21F420` and an authenticated
resource read through that new allocator chain remain open. Raw pool/record
inputs do not establish source readiness, World/voice/device admission, original
event timing/selection/gain/spatial behavior, production handoff or normal
local/split/LAN sound delivery. Both combined four-bin profiles, Xbox waveform/
wet-feedback parity, all 37 complete-definition gates and D01-D13 remain open.
The earlier archive-budget rejection is not rerun or bypassed. The
[closure receipt](../build/test-results/vehicle-destruction-native-cache-datum-20261102/owner-closure.json)
records private build/source/bank removal, preservation of the required
original reference and no new large artifact or selected-runtime promotion.

## Native common sound-stream construction, 2026-11-02

Native startup kernels now implement complete TU1.5 `2B8370` stream construction
and `2B7F40` PCM initialization. Creation and Pause remain required platform
services, and Pause is called after creation failure as in the original.
The [qualification](../build/test-results/vehicle-destruction-native-stream-construct-20261102/qualification.json)
passes 892 unchanged-CPU comparisons under a declared real SDL service ABI,
including 50 matching faults, 6,012 ordered stores, all 54 CPU sites and full
mapped-memory/stack checks. Eight incorrect compiled variants are rejected.

All 1,147 authenticated payload scenarios now enter this native constructor
before enqueue and playback. Their 273 distinct original permutations and
375 chunks retain exact uninterrupted same-PC PCM, success/flush/removal/
rejection accounting and callback-held lifetimes. Each scenario creates and
unpauses its actual SDL voice through the CPU caller. All seven restored
regression suites pass; the compiler closure records 56 objects, three archives
and 218 source dependencies without input drift.

This remains native component progress. The source driver/World/cache/voice
admission, full `224E20`/`225EF0` caller ABI, original event timing/selection/
gain/spatial behavior, production handoff, normal local/split/LAN sound delivery
and Xbox SDK/DSP/waveform/wet-feedback parity remain open. Caller formats use
the existing typed CPU owner and independently rechecked original operands;
the test ABI does not qualify retail SDK internals. The prior archive-budget
failure is not rerun or bypassed. Both combined four-bin profiles, all 37
complete-definition gates and D01-D13 remain open. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-construct-20261102/owner-closure.json)
records private build/source/bank removal, preservation of the required
original reference and no new large artifact or selected-runtime promotion.

## Native sound format/caller progress, 2026-11-02

Native `224E20` format selection and the real `225EF0` descriptor caller now
replace typed preparation in the authenticated sound test. The native path
retains the private conversion frame and integer temporary, signed-zero writes,
unwritten PCM tail bytes and reused index/output slot. All 1,147 lifecycle
scenarios execute those native entries before construction/enqueue/playback.
Their 273 distinct original permutations and 375 chunks retain the prior exact
uninterrupted same-PC PCM and packet/pin/reference cleanup.

The [qualification](../build/test-results/vehicle-destruction-native-stream-format-20261102/qualification.json)
passes 698 format/caller cases in its admitted CPU/FP profile, including 20
matching faults and complete final frame/page checks. All eight restored
regression suites pass; legacy constructor/service input and trace hashes
remain unchanged. Eight incorrect compiled variants are rejected. The compiler
closure pins 55 objects, three archives and 217 source dependencies without
checkout input drift.

The raw reference-emulator C1 result remains a recorded mismatch. Independent
native execution of the original x87 primitives checks that bit in 585 cases,
including 295 differences from raw Unicorn; the other status bits and actual
delta words agree. Both kernel-C1 and oracle-C1 mutations fail. This does not
qualify Xbox hardware FP state. The admitted format profile uses masked controls,
contiguous logical x87 values with two free slots and the immutable 1.0/2.0
operands; full physical/sparse/stack-fault ABI and global conversion proof stay
open.

Source World/driver/cache/voice admission, original event timing/selection/gain/
spatial behavior, production native handoff, normal local/split/LAN delivery,
both combined four-bin profiles and Xbox waveform/wet-feedback parity remain
open. The prior archive-budget failure is not rerun or bypassed. All 37
complete-definition gates and D01-D13 remain open. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-format-20261102/owner-closure.json)
records private source/build/bank removal, preservation of the required
original reference and no new large artifact or selected-runtime promotion.
