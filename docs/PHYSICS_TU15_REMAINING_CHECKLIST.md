# TU1.5 remaining physics checklist

**October25 goal clarification: native kernels and1:1 TU1.5 behavior are both
required.** See [the required end state](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md#required-end-state-native-kernels-with-unchanged-tu15-behavior).
The historical counts and passes below describe bounded behavioral coverage;
they do not certify complete native-kernel conversion.

Native execution and parity — required across every completion group:

- Initial source findings are recorded in the
  [native-kernel migration audit](PHYSICS_NATIVE_KERNEL_MIGRATION.md). This
  inventory remains incomplete; existing live player/world paths still use
  instruction lifting and partial native rate kernels retain fallbacks.

- [ ] Inventory every live player/vehicle/object/world physics path and replace
  remaining instruction-lifted, virtual-CPU or emulated execution with native
  semantic kernels; no production fallback to the old execution backend.
- [ ] Preserve English Xbox TU1.5 rules, ordering, timing, numerical behavior,
  state, callbacks and lifetime semantics. Validate each replacement against
  independent original evidence without relaxed gates or altered tuning.
- [ ] Requalify composed native paths visibly in multiplayer using the prepared
  imported MCC cache, actual vehicle cameras and exact host/peer checks. Record
  per-path native-kernel coverage and prove no lifted/emulated fallback was
  exercised, including with required observations enabled. Keep refused/skipped
  branches and Xbox-specific numerical proof gaps explicitly open.
- [ ] Keep original execution and emulation confined to offline oracle tests;
  TU1.1 results do not count toward this goal. Retain small results and required
  reference evidence, and verify disposable build/capture cleanup.

Latest completed frozen cohort, **v1480/00: two observed Frame residual
owners in all three driving/grenade/rocket runs**,2E5320 and304350. Native
2EA380 completes986/1,573/1,775 calls, with zero refusals and no old2EA380
owner. All three strict original frames and FP pass (114,363,573 driving bytes;
114,887,921 grenade/rocket bytes), with262/305/300 actual seated camera frames.
The grenade run now exercises native31D930 positively:78 complete joint calls,
156 angular rows and234 linear rows, zero refusals/FP errors; its required joint
gate passes. World-deferred/island-listener positive coverage remains open.
This Frame census excludes player/object schedulers and internal generated
look callers; it is not the total remaining interaction/dependency count.
Runtime SHA256:8639bf3ed591efb0f69d1c61d46a2d536a18ace667d1e84b64d547b0e85c14ab.
[Final1480 census, source checkpoint and cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-broad-phase-edges-original-v1480-20261102/latest-census.json).

| Current1480 observed Frame routine | Driving calls | Grenade calls | Rocket calls |
| --- | ---: | ---: | ---: |
|2E5320|234|156|514|
|304350|2067|2693|3217|

Previous full frozen cohort, **v1479/00: three observed Frame residual
identities in each driving/grenade/rocket run**. The remaining observed owners
are2E5320,2EA380 and304350. Native body update,
post-step, deletion, wake and discrete querying remove the earlier top-level
owners. Native Host look/camera/query now run positively; internal generated
look callers and the player/object schedulers remain outside this Frame census.
These are observed routines, not a count of every interaction or transitive
dependency. All three strict original frames pass:114,363,573 driving bytes and
114,887,921 grenade/rocket bytes, plus FP. All cases retain unexercised
World-deferred/island-listener gates; native31D930 joint building is also zero
in this workload. The historical1478 driving107-byte/26-run failure remains red;
the new run does not establish a fix or cause.
[Current census and hashes](../analysis/physics/native-live-residual-census-v1479-00-20261102.json).
Native-only closure remains open.

| Remaining observed routine | Driving calls | Grenade calls | Rocket calls |
| --- | ---: | ---: | ---: |
|2E5320|221|194|445|
|2EA380|978|1559|1789|
|304350|2047|2718|3184|

Latest live checks:

- [x] Deliver native18A2A0 reorientation in1479/00: one native call and one
  yaw wrap per driving/grenade/rocket run, zero errors, strict original bytes/FP
  and231/250/268 actual seated camera frames. Native look/query gates also pass.
  Reviewed driving video shows occupied chase camera, look turn, exit and peer.
- [x] Deliver native Host18A0C0 initialization and18A300 look/context/camera,
  including the bounded16C620 geometric-query closure. v1478/00 driving/
  grenades/rockets record1,578/5,986/5,866 native look calls,6/6/10 native query
  calls, zero query refusals/unknowns and271/283/273 actual seated camera frames.
  Grenade and rocket strict original-state comparisons pass; driving remains red.
- [x] Merge the completed1474–1478 narrow integrations into canonical Root.
  Thirty guarded source changes preserve unrelated source/BV test profiles;
  98 native dependencies already matched exactly. Both compilers pass181-CPP
  coherent World/four models, the Frame model and complete Host/LiveWorld
  contexts. [Canonical merge qualification](../analysis/physics/native-canonical-root-coherent-stage-20261102/qualification.json).
  These checks use an immutable source snapshot; later unrelated shared-Root
  lighting changes are recorded separately and preserved.
  This does not promote the launcher/runtime or close remaining native domains.
- [x] Qualify native18A2A0 reorientation and31D930 joint-constraint builder,
  including the same production helpers and genuine1478 BEFORE inputs under
  both compilers/four FP modes/all114,429,109 and114,953,457 captured bytes.
  Declared reorientation251 profiles/all30 sites and joint1,373 profiles/all207
  parent sites pass; their independently reviewed177-CPP combined World models pass.
- [x] Complete selected visible positive native31D930 joint-builder delivery
  in1480/00 grenade1200:78 attempts/calls/headers/motors/completions,156 angular
  and234 linear rows, zero refusals/FP errors, strict original bytes/FP and305
  actual vehicle-camera frames. Genuine source/name evidence identifies the
  Warthog hull–bumper hinge. Authored condition IDs remain opaque; a sufficient
  deterministic damage/SDL recipe and wider joint/FP domains are still open.
- [x] Qualify and wire the sole native2EA380 edge reorder into canonical Root.
  Both compilers pass630 physical profiles/all980 source sites, including six
  real read-before-write hardware faults, native allocations and three mutants.
  Independent World peer and all context/counter models pass;1480/00 delivers
  it positively in all three strict multiplayer cases with actual vehicle cameras.
  Additive671-case retained-array growth proof covers owned/borrowed nonempty
  outputs with the same unchanged kernel. General allocator domains stay open.
- [x] Verify genuine vehicle destruction in1479/01: two ordinary grenade
  commands/detonations produce13 moving debris objects and13 submitted parts in
  each view. Full grenade1200 and driving450 strict original bytes/FP pass;
  joint building remains zero. Individual debris pixel visibility is not certified.
- [x] Correct the sole native2E5DB0 sweep-support FLD/read and FSTP/write-fault
  phase behavior. Both compilers pass168 direct/all43-site and247 composed
  profiles, with four rejected mutants and independent peer review. Applied
  narrowly to canonical Root; no new shape/FP domain or live closure is claimed.
- [x] Deliver native1D77D0 body updates and282C10 post-step in frozen1475.
  Normal/grenade strict original-frame+FP comparisons pass with actual vehicle
  cameras. Body impulse/wake and positive dirty post-step work remain unexercised.
- [x] Deliver native320260 pair deletion and2F81B0 reversed-list destruction
  in1474/01:3/6 deletion calls and one reversed-list return per normal/grenade,
  zero refusals, both full strict original frames and actual vehicle cameras.
- [ ] Close the common contact/solver producers2E5320/2F8B60/30E930,
  retained contact producer304350/303EE0 and all true child services.
  Bounded fast returns, genuine child-entry prefixes and unconnected native
  publication components do not establish complete native closure.
- [x] Qualify native MOPP AABB30A1E0/2E9670 setup and309940 traversal:
  both compilers pass285 setup prefixes,819 traversal profiles (748 full returns,
 445/464 sites), and323 composed profiles through real original full returns.
  Ordered DATA/FP/faults and three mutations per component remain strict.
  [Source checkpoint](../analysis/physics/native-mopp-aabb-qualified-20261102/source-pins.json).
- [x] Compose native1F6D10 key querying with both AABB/oriented MOPP queries,
  native visibility and key pruning. Both compilers pass540 profiles/68 full
  original returns, all174 parent sites,38,354 ordered retained reads/9,315
  stores and three rejected mutations. The same typed binding uses guarded
  native local cache storage; no unprovided word or CPU state is supplied.
  Native oriented setup separately passes289 prefixes and pruning321 profiles.
  Added six native kernels to the normal strict-FP build; both compilers pass
  the immutable187-CPP World closure and all four models.
  [Qualified source](../analysis/physics/native-collision-key-query-qualified-20261102/source-pins.json).
- [ ] Bind that query into actual2F36F0→304350 native candidate/producer
  services; finish positive array growth, actual input and live delivery.
  Raw private-cache observation/address faults, additional property slots,
  unknown instance owners and wider lifetime/alias domains remain open.
- [ ] Complete the sole existing native304350 producer/2F36F0 candidate/buffer
  service closure and303EE0 constructor-input lifetime proof. Earlier component
  proofs execute genuine remaining services only offline; production requires
  native children and a proved current DATA preimage. No GP image, fabricated
  constructor bytes or heap-byte waiver may close this path.
- [x] Qualify the genuine class23/24→2E4EB0 factory domain with the sole native
  factory/cache/support/pool implementations. Both compilers/four FP modes/
  A5 and5A owned scratch compare all89,795,481 input-bank bytes, the256B agent,
  result and FP. Independent review passes; the small AB4B0 getter admission
  is applied to canonical Root. Actual producer/World/live delivery remains open.
  [Qualified factory](../analysis/physics/native-surface-convex-factory-qualified-actual1479-20261102/source-pins.json).
- [x] Compose the shared native149DA0/E00→233250/233420 pool service helper.
  Both compilers pass176 public array/pool cases, ordered DATA/FP and three
  rejected arithmetic/copy mutants. Default heap, alternate owners and large
  frees remain explicit requirements. The helper supplies no original executor.
  [Qualified pool services](../analysis/physics/native-pool-array-services-qualified-20261102/source-pins.json).
- [x] Return from the selected full native moving-contact graph on genuine1479
  DATA:2E5320→2F8B60→30E/30DE/hull→result→publication→creation→gameplay
  listeners→computed rejection→native removal→distance publication. Both
  compilers/four FP modes match all89,795,481 captured bytes,538 retained reads,
  64 stores and FP/RET16. Zero-count REP metadata and an actual283120 read-phase
  defect were corrected with their raw failures retained. This is selected
  component proof; production helper, World wiring and visible delivery are next.
- [ ] Qualify and wire that exact moving-contact production helper into the
  frozen1480 World/Frame closure, then test visibly with actual vehicle cameras.
  Broader initial-simplex/projection/FP/listener and raw-private observer domains
  remain refused. Preserve the unconfirmed later flag1 solver edit separately;
  the selected flag0 baseline does not qualify it.
- [ ] Finish the guarded native2F36F0 Host closure's public/default/fault/alias
  controls and independent review. Its current private component returns under
  both compilers/four FP modes with all89,795,481 bytes,634 physics DATA reads,
  114 persistent stores/preimages and FP equal; three real creations/resolutions/
  filters have zero refusals. Separate authenticated control-dispatch CODE
  mappings account only for exact immutable code-fetch PCs. No bank byte is
  waived. World/live admission and the304350 parent remain open.
- [x] Qualify native30CD00 publication through genuine native322D20 creation
  and30CAA0 reduction: both compilers pass42 profiles/all187 parent sites,
  ordered retained DATA/FP and three rejected mutants. Eight publication,
  storage, registration and listener kernels are in the normal native build.
  Both compilers pass the immutable196-CPP dependency cohort/four World models;
  this excludes preserved later Root phantom/audio work and is not live admission.
  [Build/model checkpoint](../analysis/physics/native-contact-publication-world-stage-20261102/source-pins.json).
- [x] Compose the sole native322D20 point-creation service binding with the
  same guarded event DATA through storage, registration, velocity and listeners.
  Both compilers pass77 profiles (29 returns/48 faults),191/195 parent sites,
  6,358 ordered reads/1,681 retained stores, complete FP/point ID and three
  arithmetic mutations. Bound raw-observer, padding-read and native-only
  remaining-owner refusal/restore models pass; independent peer review is green.
  [Qualified binding](../analysis/physics/native-contact-point-creation-binding-qualified-20261102/source-pins.json).
- [x] Replace eligibility2E4F30/3038C0 with native class-dispatch logic. Both
  compilers pass130 profiles/all35 sites,120 returns/10 fault prefixes,24 output
  aliases and32-bit index wrap. The fallback rereads current DATA; a value-
  preserving cached-read mutant fails the ordered-read gate. Independent review
  passes and the sole kernel is build-wired; parent/live admission remains open.
  [Qualified eligibility](../analysis/physics/native-contact-pair-eligibility-qualified-20261102/source-pins.json).
- [ ] Admit that creation binding through actual304350/World services and
  visibly test it in multiplayer. Positive activation/removal providers, four
  rejection/removal source sites and unknown listener/velocity owners remain open.
- [x] Authenticate the genuine304350 typed BEFORE input with an exact-schema
  loader:4,033 bank files/89,795,481 bytes, no captured GP/SIMD, after-image or
  outcome. Twenty-two malformed/oversized controls refuse; extraction has a
  prechecked192MiB bank/1MiB metadata budget. Independent review passes.
  [Loader review](../analysis/physics/native-contact-process-before-loader-independent-peer-review-20261102/review.json).
- [ ] Finish the actual moving-shape slow solver. The selected30DE caller
  lifetime audit proves four copied undefined rows are unread through full
  original2E return; poison/SNaN controls preserve all retained bytes and FP.
  Native30DE composition must keep the same guard. Separately313790 private W
  changes MXCSR and has no writer on the studied count2 caller path; its actual
  preimage remains unowned and cannot be replaced with zero or CPU/control bytes.
- [x] Deliver native316D40 wake with the existing qualified native factory
  closure, plus native281530 discrete queries. v1473/04 driving450 and
  grenade1200 match all89,729,945/114,887,921 original bytes and FP. Native wake
  completes2/3 times, bound factories1/2 and discrete queries5/6, zero refusals;
  actual occupied vehicle-camera frames285/258. Eight official regressions pass.
- [x] Qualify and wire the native body-deactivation2815F0 → component1D7260 →
  motion1D8B20/frame1D8B90 chain. Both compilers pass181 physical profiles,
  102 full returns,3,183 ordered reads/553 stores, complete FP and four mutants.
  The exact World builder passes both coherent models and12 negative gates.
- [ ] Finish strict live body-chain qualification. v1473/05 delivers14/20
  native snapshots and126/180 stores, zero refusals, with186/267 actual vehicle
  camera frames. Grenade1200 matches all114,887,921 original bytes and FP;
  driving450 fails strict equality by104 heap bytes in26 runs. Its saved input
  roundtrips all4,415 files; original hardware watch traces512-byte303EE0 →
  2E3250 →2E2E30 copying plus30479C/304820 vector stores. No cause is waived.
  Keep this source cohort distinct from the passing04 cohort.
- [x] Replace the selected Host18A300 look route with the native controller and
  its qualified context/camera/query services in frozen1478. This path has
  positive native activity and actual vehicle-camera proof, as recorded above.
- [ ] Close internal generated1088DC→18A300/18A0C0 caller handoffs, positive
  marker SourcePC observer correspondence, alternate CRT/acos policies and
  unsupported FP/query/lifetime domains. Canonical Root wiring remains above.
- [x] Deliver native317200 merge queue,320DA0 deferred pair queue,3232E0
  manager factory and the qualified282700→B1870→BA570→C0F60→spatial binding.
  v1472/03 driving450 and grenade1200 match all114,363,573/114,887,921 original
  bytes and FP. Both queues have5/2 completions and zero refusals; actual
  occupied vehicle-camera frames352/348. Added/deletion/spatial activity is
  zero in these runs, so their positive live coverage remains open.
- [x] Complete the actual1472/01 listener/deletion/spatial component pipeline
  against unchanged TU1.5 under both compilers: all114,953,457 bank bytes,
  116 ordered reads,14 stores and FP, with a genuine full return. No supplied
  child result, private GP transport or compared-byte exclusion.
- [ ] Repair the v1472/03 fatal-player strict42-byte heap mismatch. A hardware
  data breakpoint identifies original2E2E43 REP MOVSD as the first writer. A
  subsequent entry census traces2E3250 from303EE0, copying512 bytes from
  600EFD20 to10092F80; all42 differences lie in this allocation. Record
  production/lifetime and a native repair remain to be proved. The captured
  frame has zero31FFC0 entries; cumulative phantom creation does not explain
  this frame's mismatch. Keep the saved input and strict test red.
  [Writer census](../analysis/physics/native-player-original-write-watch-v1472-03-20261102/report.json).
- [x] Qualify and deliver the actual Elongation2F1CC0 →2F7190→2E4EB0 wrapped
  factory closure. v1473/03 completes18 factory entries/30 class queries and
  six native discrete queries, zero refusals,900 ticks/905 exact LAN checks.
- [ ] Finish ordinary-input authored conveyor coverage and repair the strict
  Elongation182-byte heap mismatch. Actual spawn slot1 is across the map from
  the former assumed target; the player never reaches the belt. Hardware watch
  identifies original2E2E43 copies and30479C/304820 vector stores; the genuine
  record-producer/lifetime cause remains open. No compared byte is waived.
- [x] Wire the qualified native316D40 pair wake and finite11-kind PART/point
  evaluator into Root through narrow guarded changes. The combined139-CPP Root
  model passes four checks under both compilers. The actual2F8030 wake factory
  now passes the full native component proof and live04 as recorded above.
  Positive eleven-kind multiplayer coverage stays open.
- [x] Qualify and wire the sole native281530→281F10/1D7530 discrete pair
  binding:72 authored-input profiles per compiler; the authenticated inventory
  contains11/90/20 parent/helper sites. Ordered persistent operands, return and
  FP checks, two rejected mutations. Seven diagnostic fields, both JSON paths,
  eight negative gates and three coherent World models pass both compilers.
  Unknown filter/motion owners count and refuse; positive live delivery passes04.
- [x] Wire320260 native pair deletion with a per-operation RAII native scope.
  Both compilers pass36 genuine-DATA parent cases/188 of247 source sites,
  coherent World models and counter/JSON/gate controls. An unknown nested owner
  is counted separately and refused before any ABI/FP transport or execution.
- [ ] Finish320260's59 remaining source sites/children. Selected live deletion
  now passes1474/01; the following1473 refusal is historical evidence.
  v1473/06 passes eight official regressions, then both full cases encounter
  actual2F81B0 and correctly refuse before a complete frame. Capture its real
  typed destructor call and port the true child; no live-deletion pass yet.


- [x] Deliver native point/reference, manager/base-lock, List/transformed
  factory, wrapped/compound processing, World merge and regroup in visible
  imported-MCC multiplayer. v1471/00 driving450 and grenade1200 pass all of
  these positive gates and strict114,363,573/114,887,921-byte+FP comparisons.
  Driving has382 actual vehicle-camera frames,0.6-radian camera turn,41 exit
  frames and388 exact LAN comparisons; grenade has363 camera frames and1,801
  exact LAN comparisons. The reviewed driving video uses the occupied Warthog
  chase camera above and the independent peer below.
- [x] Exercise genuine native repartition during the same strict driving run:
  12 graph builds, three created islands/three moved bodies, two moved
  constraints/three moved contacts, balanced6/6 group allocations and36/36
  temporary constructions/destructions, with zero unknowns/refusals.
- [x] Wire native reversed2F7480/2F7FB0 processing and both constraint-info
  owners31D630/31F840. v1471/05 driving450 and grenade1200 match all
  114,363,573/114,887,921 original bytes and FP; both genuinely invert one
  contact normal. Explicit normals-positive driving06 passes. Info kernels
  have independent original proofs; positive live info queries remain unexercised.
- [x] Complete v1471/07 fatal-player1200 with native deferred queue growth:
  strict114,887,921-byte+FP match, two real growths, first death867,
  99 source-dead ticks, authoritative corpse submissions1,330 per view,
  318 actual vehicle-camera frames and2,282 exact LAN comparisons. Reviewed
  video shows chase camera, grenade input, respawn and corpse in the top view;
  sampled lower-view corpse pixels remain unproved.
- [ ] Complete the full live coverage matrix. Overall05 and07 acceptance stays
  red because genuine World delete/update queues and island step listeners
  remain unexercised. Keep these gates enabled and use authentic gameplay.
- [x] Qualify native phantom creation31FFC0 and native315300/315B00/314F80
  children against actual1471/09 BEFORE DATA under both compilers, including
  all114,953,457 captured bytes, ordered DATA, FP and return. A separate
  declared eight-call sequence proves natural capacity8-to16 growth.
- [x] Complete visible positive native phantom creation in1473/04 grenade1200:
  one complex and one simple phantom, two notifications, zero unknowns, positive
  gate and strict114,887,921-byte+FP equality with258 vehicle-camera frames.
  Later05 has zero creation activity; that case's positive gate stays red.
- [x] Qualify native282700 overlap listener and sharedB1870 predicate on both
  compilers:80 declared profiles/all43+17 source sites,893 ordered reads and49
  stores,18 fault prefixes/three DATA mutations/three rejected mutants. Real
  BA570 object-deletion entries remain explicit child dependencies.
- [x] Capture the actual282700 event before refusal and wire its qualified
  native BA570/detach/spatial closure on the captured input. Broader BA570
  lifecycle children and positive live deletion remain open below.
- [x] Qualify native PART/material/property/conveyor response and the exact
  point callback/scratch lifetimes under both compilers:217 cases/all418 PART
  sites and32 transported counter/gate checks. Whole221-site gameplay parent
  composition passes109 cases with native PART children; no unbound prefixes.
- [x] Wire PART into Root and qualify its coherent129-CPP World model under
  both compilers. Exact seven-file base matched the frozen1472 source.
- [ ] Complete visible positive PART/conveyor coverage. The prepared MCC cache
  has zero Coagulation conveyor materials; Elongation and Colossus have genuine
  authored conveyors with kinds0/1. Use their actual surfaces and ordinary input.
  Wider authored kinds and exceptional FP domains remain explicit requirements.
- [x] Qualify all11 finite authored function kinds composed with native PART:
  261 cases (217 baseline plus44 positive),16 function calls per kind under
  both compilers, all418 PART sites/39,046 reads/3,296 stores/six mutants.
  The qualified expanded evaluator is now wired into Root; its combined model
  and visible eleven-kind qualification are separate from finite component proof.
- [x] Qualify native memory-pressure1496F0/149AD0/2335B0 and allocator/batch
  helpers against unchanged TU1.5 under both compilers:168 cases/four mutants.
  The exact production Frame binding passes both compiler models and is wired
  into Root; selected unsupported object-service children count/refuse.
- [x] Qualify native1CCE00 pressure pruning and its exact native classifier
  builder on both compilers:117 profiles,70 complete returns (including dense
  contact classification), three genuine candidate-entry prefixes and three
  rejected mutants. Its exact Frame binding passes both compiler models and
  is wired into Root. Candidate/deletion children remain counted refusals.
- [x] Requalify the composed Root Frame with both pressure bindings: all65
  independent original prefix/tail cases pass and both state/read mutations
  reject. Temporary source and build trees were removed.
- [x] Deliver both pressure bindings visibly in1472/01: driving450/grenade1200
  match all114,363,573/114,887,921 original bytes and FP, with348/340 actual
  vehicle-camera frames. Native memory-pressure completions1,298/4,134 and
  contact-pruning completions649/2,067, zero refusals/unbound services; both old
  owners are absent. Dense/candidate work is zero in these selected runs.
- [ ] Qualify returned positive object collection/candidate/deletion closures,
  exceptional lifetime and unmasked selected SSE exceptions. These are separate
  from the successfully delivered ordinary pressure paths.
- [x] Capture the actual282700 input at1472/01 fatal-player before refusal:
  all114,953,457 bank bytes plus the actual16-byte event, mode1/depth0 and
  realFP. Required archive13,668,925 bytes roundtrips; raw132,878,336 allocated
  bytes removed. No pose, flags, results or original ABI were manufactured.
- [x] Wire native3232E0 manager creation at the World pair route. The sole
  native builder passes20 original contexts,19 transported counters and both
  coherent124-CPP World models. True body destruction and registered lock
  releases remain explicit refused children. v1472/02–03 delivers5 factories/
  10 registrations per driving/grenade run with zero refusals and strict equality.
- [ ] Complete native BA570 object deletion, its real spatial/detach/controller/
  AI/player callbacks and visible fatal-player composition. The native parent
  and selected helper returns plus exact production builder pass290 profiles
  under both compilers;89 full returns/six actual child-entry boundaries.
  Nine atomic RMW write-fault profiles remain explicitly unqualified; separate
  read and completed-store observer prefixes pass. NativeC0F60 tree detachment
  and selected spatial children now qualify the actual listener full return;
  wider lifecycle children and positive live deletion remain open.
- [x] Qualify actual v1470 List2F8030 and reversed List2F81F0 inputs against
  unchanged TU1.5: both compilers match all114,429,109 captured bytes, FP,
  ordered parent reads/stores and genuine child services. The reversed input
  exposes the real transformed2F7190 child. Wire the existing native List and
  transformed constructors and all17 authenticated constant getter leaves.
  These parent proofs use original child execution offline; they do not prove
  the entire native child composition.
  [Reversed input proof](../analysis/physics/native-reversed-list-factory-actual1470-20261101/report.json).
- [ ] Complete v1470 visible acceptance. Iterations00–05 stop at explicit
  shape/factory guards;06 passes eight regressions and reaches the actual
  multiplayer window/first frame, then all three cases stop at the manager
  lock-profile guard. Actual manager1007BF40 has collector flagsBCDC,
  A4=0 and owned32-byte array capacity2 (grenade case4). Iteration08 preserves
  its genuine BEFORE input. Its registry lookup is absent, and the full native
  manager/collector/base-lock lifetime now matches all114,429,109 bytes,178
  ordered reads/77 stores, FP and return for three disposing controls under both
  compilers; two mutations reject. Registered lock storage remains refused.
  [Actual manager proof](../analysis/physics/native-contact-manager-base-lock-actual1470-tu15-20261102-first/report.json).
  The06 video visibly shows the occupied Warthog chase camera above and the
  independent peer below, but supplies no completed strict frame or driving
  gate. Iteration09 combines point/regroup/base-lock for full450/1200/1200
  tests and passes the old lock guard, then stops at the explicitly unbound
  gameplay22E1B0 reference-release service. Iteration10 captures that exact typed
  boundary before refusal for native integration; no old backend fallback is
  admitted. Fatal-player and native-only acceptance remain open.
- [x] Qualify the complete measured point322F70 pipeline using native gameplay
  listeners, metadata, world update and constraint detach/info. Both compilers
  match all89,795,481 captured bytes, FP and retained DATA access order across
  three initialization seeds; all children are native. The same production
  builder passes guard tests and is wired into Root. The coherent World model
  and16-field/getter/two-JSON/positive-gate checks pass.
  [Binding](../analysis/physics/native-contact-point-root-binding-20261101/source-pins.json).
- [x] Exercise native point/reference removal in completed v1471 driving and
  grenade runs. Native PART is separately qualified and pending delivery;
  wider runtime effects, zero-reference destruction and unknown listeners
  remain refused. Complete private raw-ledger parity remains red separately.
  Superseded point-stage-v2 remains historical failure evidence.
- [x] Qualify native3187C0 whole normal-completion connectivity/partition
  processing and its native graph, arena, pool, temporary-array, constructor
  and edge children. Both compilers pass48 parent profiles; the sole production
  service builder passes72 profiles including depths0–5. Genuine09 skips
  active partition work; positive split worlds are explicitly declared inputs.
  [Parent qualification](../analysis/physics/native-island-regroup-parent-qualified-20261101/qualification.json).
- [x] Wire native regroup into Root World with retained caller DATA, native-only
  services and24 transported counters. The124-CPP Root model passes all ten
  default profiles and actual factory/merge prefixes. The frozen multiplayer
  cohort has its own two-compiler model and separate phase/splitting gates.
  [Root binding](../analysis/physics/native-world-island-regroup-root-binding-20261101/source-pins.json).
- [x] Compose genuine1470 point DATA through native point removal, declared
  standalone316D40 wake and native3187C0. Both compilers match all captured
  bytes and FP after each phase on12 profiles: wake updates one body; regroup
  builds its graph, finds full connectivity and clears the partition flag.
  No source flags, poses or contact rows are manufactured; no new island is
  created. This is declared source entry on genuine DATA, not a captured wake
  caller or positive live splitting result.
  [Composition](../analysis/physics/native-point-wake-regroup-actual1470-qualified-20261101/source-pins.json).
- [x] Visibly exercise native regroup and genuine positive live splitting in
  v1471/00 driving, as counted above. This supersedes the earlier live-splitting
  gap; the older inactive input and declared standalone wake still keep their
  narrower proof scopes.
- [ ] Finish native wake at every reached caller and wider regroup
  redistribution/destruction domains. The fresh driving census still contains
  two316D40 calls; a shared native service at one caller does not convert all
  callers or prove all wake effects.
- [x] Implement and wire native wrapped contact-process2F6CA0 and compound
  contact-process2F7A50, using native2E44D0 geometry and separate caller DATA
  slots through recursive children. Both compilers pass116 strict original
  parent-to-child boundary cases,2,838 reads/1,086 stores and three mutations.
  A separate nested model checks descriptor/geometry lifetime and late source
  header changes. The coherent World model, factory prefix and merge prefix
  pass; eight counters and two positive multiplayer gates are wired.
  [Binding](../analysis/physics/native-contact-process-root-binding-20261101/source-pins.json).
- [ ] Complete visible delivery of these process parents and genuine returned
  producer304350/other child processing. The complete raw private composition
  remains red at2F7A6E's prior bytes reused after2E44D0 saved ESI. The strict
  parent boundary pass does not waive that result or the persistent128-byte
  producer/heap failures. No register afterimage is reconstructed.
- [x] Replace the literal manager-array capacity admission with the qualified
  native pool domain. Both compilers pass the independent reserve-to-destruction
  family, including all three owned-array branches; the integrated World model
  passes all ten profiles. Positive+A4, lock/body-zero and unwind remain open.
  [Freeze](../analysis/physics/native-contact-manager-array-domain-world-binding-20261101/source-pins.json).
- [x] Wire native island merge3190D0, sorted contact merge318DE0 and lifetime
  children317A40/317AD0/322100/3213A0 into World. The private GCC/Clang cohort
  passes the ten manager profiles, genuine02 factory prefix and genuine09 merge
  prefix; the actual Root model also passes. The live-world and multiplayer
  acceptance objects compile. Shared native wake services and World activation
  counters are included; nested unported merge services refuse before invoking
  the old backend. This is component/model evidence, with live delivery pending.
  [Root freeze](../analysis/physics/native-world-island-merge-root-binding-20261101-revision-02/source-pins.json).
- [ ] Complete the new combined visible cohort with native manager, factory and
  World merge gates, exact host/peer checks, imported MCC and the actual vehicle
  camera. v1468 stopped during compilation and produced no runtime or live
  acceptance. Its expired, process-free build has been removed; its source and
  logs remain. v1470/00 passes all eight regressions, but all three full visible
  attempts stop at the explicit shape-class qualification guard before a
  completed frame or camera result. Iteration01 adds the native contact-process
  binding and records the genuine refused shape/owner tuple. Normal450,
  grenade1200 and fatal-player1200 remain required;00 is not a live pass.
- [x] Capture genuine typed322F70 BEFORE input in v1470/00: actual manager
  1007C000, point0, original owner322F70, actual FP depth0. The required
  11,999,089-byte archive roundtrips4,034 files and its raw duplicate is removed.
- [ ] Finish the wider point-removal and3187C0 branch domains and their live
  composition described above. Preserve separate retained-DATA, raw-private,
  declared split-world and genuine live evidence.
- [x] v1466/07 runs native57 recursive cleanup parents in normal driving,
  wall contact and grenade cases. Driving450 and grenade1200 match the original;
  the finite-wall heap mismatch remains open.
- [x] v1466/08–09 runs native159 cached/wrapped cleanup and deleting children.
  Normal activity includes73 cached,5 wrapped and108 native destructors, with
  zero child residual/refusal; manager-point32 and manager-destructor14 remain.
  The identical08 binary's09 grenade run matches114,887,921 original bytes plus
  FP, with418 actual vehicle-camera frames and1,803 exact LAN comparisons.
- [ ] Resolve strict normal08/wall03/07 heap mismatches at10081B8C/1008038C;
  later passing frames do not waive these failures. Preserve their original
  BEFORE inputs and ordered-write diagnostics.
- [x] Capture genuine positive authored+80 component DATA during09 frame724:
  one authored row, two bodies and an active island, actual depth0. The required
  13,613,721-byte archive is hash-verified; raw duplicate removed. This is a
  frame-level eligibility input, not a captured22D460 invocation or creation pass.
- [x] Qualify positive22D460 creation on the genuine09 frame-level BEFORE DATA:
  16 declared formal calls per compiler, one created/published constraint each,
  all1,839,255,312 persistent bytes per compiler,337 ordered events per call,
  FP/RET4/livevalues match. Preserve the actual1F3460 classification-table read;
  native code validates its target and uses typed branches.
  [Report](../build/test-results/native-body-constraint-actual09-classification-tu15/report.json).
- [x] Compose both real09 body listeners through native2816A0→22D460, then
  remove each independently computed published handle through native22D0A0.
  Sixteen profiles per compiler match the448-event closure ledger,
  1,839,255,312 bytes, FP/RET/livevalues. Global rows/references, list removal/
  deletion and instance deletion execute16 times each; unbound owners0 and
  the wrong global-reference-clear mutation rejects.
  [Aggregate](../analysis/physics/native-constraint-authored-lifetime-actual09-20261101/qualified-aggregate.json).
- [ ] Qualify actual live activation-listener invocation and visible positive
  lifetime effect; these are declared component/listener entries on real frame
  inputs, not a captured original22D460 call or injected gameplay event.
- [x] Wire manager3232B0/3231B0 and the existing sole collector cleanup owner;
  the seven-profile World model passes positive unregister/defer/storage lifetime.
- [ ] Extend manager profile qualification: v1466/10 passes eight regressions,
  but all three live cases stop before stores at the explicit storage/current-world
  guard. No complete frame, camera or fatal-player result is claimed for10.
  The11 readonly tuple identifies collector+3C capacity1, an owned32-byte array;
  A4/lock are zero and the other two arrays are borrowed. Capture its genuine
  BEFORE was retained in12 and the owned32 release now passes both compilers:
  89,795,481 bytes per case,160 reads/71 stores/10 genuine allocator entries,
  actual flag1 plus declared flags0/3, three mutations rejected each.
  The eight-profile World model also passes, and the specific cap3C==1 binding
  is queued for v1467/01 visible requalification. Keep positive+A4,
  owned arrays, lock/body-zero and error-unwind coverage explicit.
- [x] Qualify native convex factory2E4EB0/2F8300 on the actual class23 parent
  domain:40 parent cases/32 native factory completions per compiler,
  3,589,363,400 persistent bytes, parent ordered events and FP match; three
  mutations reject. Independent private DATA00/A5/5A and volatile-register poison
  controls pass. This does not admit other shape classes or unwind domains.
- [x] Wire native320EF0 and the bounded convex factory; the full default World
  cached-confirmation model passes without the old320EF0 call. Twelve native
  parent/factory completion, allocation, shape and refusal counters are wired.
  The positive fresh-agent World prefix and visible full flow remain separate.
- [ ] Finish positive factory delivery and genuine point-removal322F70,
  distinct wrapped-process2F6CA0 and create/delete closure. No injected admission, class
  substitution or supplied postimages.

Native conversion work currently under validation (these bounded proofs do not
close a gameplay completion group):

- [x] Qualify native cached-motion `325810` and its closed quaternion/origin
  helpers: 697 physical original cases pass with GCC and Clang, including
  ordered reads/stores, FP state, rejection prefixes and five rejected mutations.
  [Proof](PHYSICS_TU15_NATIVE_CACHED_MOTION_PROOF_20261026.md).
- [x] Compose native `3205B0` and native cached/fixed motion against eleven
  actual/declared BEFORE-input cases: 73 native motion calls and zero original
  motion returns; the remaining agent still executes its genuine original body.
  This offline proof supplies independently captured agent-entry ABI state and
  does not qualify construction of that state in production.
  [Results](../build/test-results/native-island-contact-advance-native-motion-tu15/report.json).
- [ ] Qualify the new live cached-motion binding, genuine downstream agent
  dependencies and a visible imported-MCC multiplayer cohort. Require actual
  dynamic and fixed native activity, matching completed rewind/advance counts,
  zero contact-motion fallback, and strict original-frame comparison. No guest
  GP/SIMD postimages or captured-after results may be supplied by the kernel.
- [x] Wire cached motion and qualify its world binding:110 original world
  cases,96 native force compositions, a genuine-context model that executes
  native dynamic/fixed contact motion, and the rejected world mutation pass.
  [Results](../build/test-results/native-world-cached-motion-binding2-20261101/report.json).
- [x] Port post-integration listener cohorts `322820/322950` and the closed
  `3161F0` getter. Both compilers pass210 physical cases and three rejected
  mutations. Wire both cohorts and pass the world model, including a callback
  changing the late final listener and stable null compaction.
  [Proof](PHYSICS_TU15_NATIVE_WORLD_STEP_LISTENERS_PROOF_20261101.md).
- [ ] Qualify these listener cohorts in visible multiplayer with positive
  world/island activity and strict original-frame comparison. Custom listener
  bodies remain explicit dependencies requiring their own native conversion.
- [x] Qualify native contact-query `2E4FE0`, eligibility `283380`, surface
  geometry `1F5FE0`, component visibility and native cache `309060/309150`.
  The composed query/cache proof passes42 cases per compiler across all seven
  retained producer entries, with3,795,642,138 persistent bytes and3,447 ordered
  operand events per compiler. Genuine GJK and shape-method dependencies remain.
  [Cache proof](PHYSICS_TU15_NATIVE_CONTACT_CACHE_PROOF_20261101.md).
- [ ] Close the real contact-constructor lifetime and production handoff.
  The resolver's120-case cohort matches persistent bytes, FP and all written
  data, while its strict before-byte ledger stays red on private control-origin
  cells. The native compound-parent proof stops at genuine304350 entry; there
  is no native-parent-to-original-child full-return qualification. No saved GP,
  EBP or return metadata may be serialized to manufacture a pass.
  [Lifetime proof](PHYSICS_TU15_NATIVE_COLLISION_COMPONENT_PROOF_20261101.md).
  The v1463 Gemini replay is independently reproduced with130 differing heap
  bytes across six adjacent128-byte records. A read-only hardware watch
  identifies original repeat-copy2E2E43 and304350 vector stores30479C/304820.
  This locates writers; it neither proves bytes inactive nor permits a waiver.
  [Diagnosis](../analysis/physics/native-gemini-heap-mismatch-diagnosis-20261101/report.json),
  [writer watch](../analysis/physics/native-gemini-heap-write-watch-20261101/report.json).
- [x] Qualify the native compound contact-process parent `2F7A50`:97 physical
  cases pass under both compilers, including recursive genuine native children,
  actual `304350` entry stops and three rejected mutations. Genuine noncompound
  child return/context construction remains open.
  [Proof](PHYSICS_TU15_NATIVE_COMPOUND_PROCESS_PROOF_20261101.md).
- [x] Port and wire native deferred world operations, added actions, deletions
  and updates (`319750/316E90/319CD0/316E10`) and merge allocation monitors.
  Both compilers pass452 physical cases; the corrected world binding passes
  source comparisons and callback/reference-lifetime checks. Nine RMW prewrite
  observer prefixes remain explicitly excluded.
  [Proof](PHYSICS_TU15_NATIVE_WORLD_DEFERRED_PROOF_20261101.md).
- [ ] Qualify the deferred queues visibly with all four native counters,
  actual reference releases, no calls to the old queue owners and strict original
  comparison. Finish their genuine nested merge, removal and destructor owners.
- [x] Port contact-manager completion `3230C0` and world/body notification
  `322560/31FF50`:296 physical cases pass under both compilers with three
  rejected mutations. One atomic RMW prewrite-observer prefix remains excluded.
  [Proof](PHYSICS_TU15_NATIVE_CONTACT_COMPLETION_PROOF_20261101.md).
- [x] Wire contact completion and pass the complete world-binding model,
  including custom-callback private-frame poison, cached metadata owners and
  callback-published listener counts. The110 original world cases,96 force
  compositions and world mutation check also pass.
- [x] Exercise native contact completion in v1463/09 multiplayer:1,236 calls,
  2,009 copied contacts,1,236 world and2,472 body notifications,1,368 genuine
  custom callbacks; the old3230C0 owner is absent and strict original-frame
  comparison passes. The actual driver vehicle camera and independent peer
  were visually reviewed. Custom callback bodies remain migration work.
- [x] Diagnose the real blast failure: the actual D91D0 query selects both the
  grenade and target vehicle, with genuine vehicle physics-body ownership.
  Delivery then reaches the missing fifth read-only response-table operand
  DCDA8. Replace its TU1.5 lookup with the authenticated native five-kind
  decision; both compilers and the rejecting type4 mutation pass.
  [Results](../build/test-results/native-damage-response-dispatch-tu15-20261101/report.json).
- [x] Requalify the native response selector and actual grenade delivery.
  v1463/13 confirms the vehicle is admitted; distance2.0034 exceeds the authored
  radius1.75 and correctly yields zero falloff. The declared range6 throw in
  v1463/14 delivers one actual target impulse and reduces body fraction to
  0.4133; four authored debris owners submit geometry to both views. Its late
  114,887,921-byte original comparison passes.
- [x] v1463/15 delivers two genuine target impulses and reduces body fraction
  to0.1613, with three authored debris owners submitted to both views and strict
  original comparison passing at tick1200. Submission counts are separate from
  framebuffer visibility; full destruction is still red.
- [x] Exercise source-authored rocket destruction in v1464/02: six genuine
  target impulses, body fraction−1.60000002 and1,347 death-flag samples; nine
  authored debris owners submit geometry to both views. The strict original
  frame compares114,887,921 bytes and actual driver camera renders362 frames.
  Independent recording review verifies driver camera, exit and rocket input;
  sampled frames do not isolate every blast flash or individual debris object.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-02-summary.json).
- [ ] Finish full vehicle retirement/presentation proof and genuine flip
  input. The surrounding damage-response parent and other damage owners still
  require native conversion. The completed v1463/00–15 evidence is preserved;
  interrupted16 produced no live pass and its1,139,437,568-byte temporary owner
  was removed. The exact source cohort is reconstructed for v1464.
- [x] Port and wire `3174A0/3173C0/317430` body reference compaction. Both
  compilers pass323 physical cases; the world binding preserves a store-observer
  change to the current retirement entry and uses native reserve for growth.
  [Proof](PHYSICS_TU15_NATIVE_BODY_REFERENCES_PROOF_20261101.md).
- [ ] Qualify body reference compaction visibly with positive native calls,
  all three cohorts, no `3174A0` residual and strict original comparison. Complete
  the remaining body destruction/lifetime owners and growth/fault domains.
- [x] Qualify the declared Warthog live cached-motion route: ten native calls,
  ten completed rewinds/advances, four fixed-motion calls, zero motion fallback
  and a strict89,729,945-byte original frame in v1463/01. Other motion owners
  and unexercised branches remain separate.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-motion-owners-original-v1463-20261101/iteration-01-summary.json).
- [x] Qualify the declared Warthog live body-compaction route:34 root calls,
  102 cohorts and22 moved entries with no old3174A0 owner in v1463/03; actual
  vehicle camera and strict original frame pass. Growth remains unexercised.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-motion-owners-original-v1463-20261101/iteration-03-summary.json).
- [x] Port and wire the newly identified sphere-bounds owner2E8D90:295
  physical original cases under both compilers and the complete world binding
  model pass. Private transient x87 fault prefixes remain explicitly open.
  [Proof](PHYSICS_TU15_NATIVE_SPHERE_BOUNDS_PROOF_20261101.md).
- [x] Exercise native sphere bounds in v1463/05: two actual calls, zero bounds
  fallback and strict original-frame comparison pass in the vehicle, grenade
  and object cases. Other sphere branches and fault domains remain open.
- [x] Wire the qualified direct constraint owner `325EF0` to native `329720`.
  The strengthened binding model passes; v1463/05 driving completes596 native
  calls and1,923 rows with no old owner, and grenade/object cases also pass
  their strict original comparisons. Axis-producer coverage remains open.
  [Binding results](../build/test-results/native-direct-base-binding2-20261101/report.json).
- [ ] Exercise post-integration island listeners and deferred delete/update
  work with source-authenticated multiplayer input. Current driving cases have
  zero island-listener/delete/update counts; their gates remain red.
- [x] Qualify the admitted activation family `315CB0/3165D0/316D40/322660/283450`:
  598 physical cases pass under both compilers. Four RMW prewrite observer
  prefixes remain excluded; this is not complete fault-contract qualification.
  [Results](../build/test-results/native-island-activation-admitted-tu15/report.json).
- [x] Qualify native `282870` activation agents and the composed group filter,
  discrete dispatch, body listener and ancestor wake services. A typed B9800
  binding and the actual F2091 transport pass48 physical cases per compiler,
  preserving incoming x87 values at all admitted depths0–5.
  [Activation proof](PHYSICS_TU15_NATIVE_ACTIVATION_PROOF_20261101.md).
- [x] Qualify native `281E10` contact-entity notification and its two property
  lookups:231 physical cases pass under both compilers, with three rejected
  mutations. The complete world binding/model passes; the genuine processor
  receives its actual implicit event input and five typed parameters. Its
  production gate is exercised in v1464/00–01:01 records3,096 native calls,
  12,384 properties and3,096 genuine child dispatches, with no old281E10 and a
  strict tick1200 original match. The historical child1D8A20 was a genuine
  residual there; its subsequent native conversion is recorded below. [Proof](PHYSICS_TU15_NATIVE_CONTACT_ENTITY_PROOF_20261101.md).
- [x] Port and wire gameplay sleep wrappers `1D9B20/1D9B70` with their closed
  eligibility chain `1D9BC0/F20C0/E7790/E65A0` and native body policy. Both
  compilers pass422 physical cases and three rejected mutations. The world
  model exercises both owners and blocked/body-test routes; its original
  world/force comparisons also pass. The next live refresh requires positive
  native activity and no old wrapper residuals.
  [Proof](PHYSICS_TU15_NATIVE_GAMEPLAY_DEACTIVATION_PROOF_20261101.md).
- [x] Exercise that native gameplay sleep-policy gate in v1464/02:92 native
  calls reconcile to5 blocked/87 body-policy calls,31 biped/61 vehicle tests,
  no old wrapper and strict original comparison passing. Genuine flip and
  sleeping-island gates remain separate.
- [x] Qualify and wire the two contact-feature virtual34 source calls in
  actual30E930 to native swept/SoA/box/capsule/polygon kernels. Both compilers
  pass28 complete original root returns,142 production-adapter calls and
  2,530,428,092 compared persistent bytes. Independent volatile-register poison
  and adapter mutations verify the void ABI without old GP/SIMD postimages.
  [Proof](PHYSICS_TU15_NATIVE_CONTACT_FEATURES_HANDOFF_PROOF_20261101.md).
- [x] Exercise that native contact-feature binding visibly with both actual
  source-call counters positive, reconciled owner counts, zero refusals and
  strict original comparison. The whole GJK/producer parents remain open.
  v1464/04 driving completes15,470 attempts/calls with zero refusal, sources
  `[7735,7735]` and direct owners `[2798,0,0,4,7056,5612]` (swept,SoA,box,
  capsule,polygon,sphere). Its strict original frame matches89,729,945 bytes.
  The earlier v1464/03 builds/passes eight regressions but all three live cases refuse an
  unqualified feature owner. Normal/rocket vtable4613A0 selects authenticated
  sphere2E8D40. Its native port now passes147 physical cases per compiler and
  four mutations;04 has no host refusal on any of its three cases. No extra
  Gemini algorithm was inferred from the old outer-register diagnostic.
- [x] Qualify and wire native triangle30D3F0 and closest-segment312620 at all
  five authenticated solver call sites. Both compilers pass28 complete original
  roots,73 strict typed-return stages and73 actual-adapter calls with zero
  refusal, comparing2,530,428,092 persistent bytes each. Runtime/ABI models
  reject three mutations; no original GP/SIMD postimage is supplied.
  [Proof](PHYSICS_TU15_NATIVE_SIMPLEX_HELPERS_HANDOFF_PROOF_20261101.md).
- [x] Exercise both native simplex helper kinds visibly with reconciled
  per-source counts, zero refusals and a strict original-frame comparison.
  v1464/04 driving completes4,956 native calls (3,886 triangles/1,070 segments)
  with no refusal and a strict89,729,945-byte original match. Rocket04 also
  passes:9,499 helper calls, zero refusal and114,887,921 compared bytes.
- [x] Qualify and wire native contact-row matching1D8A20:146 physical cases
  pass under both compilers, including18 genuine1D77D0 entry stops and three
  rejected mutations. The world model changes row backing/count and event
  subject during two genuine callback boundaries, with a poisoned private
  frame. Its110 original world/96 force comparisons also pass.
  [Component](../build/test-results/native-contact-entity-processor-tu15-20261101/report.json),
  [binding](../build/test-results/native-world-contact-processor-binding-20261101/report.json).
- [x] Exercise native1D8A20 in visible multiplayer:04 driving records1,355
  native parent calls and2,089 matched/updated rows; rocket04 records7,289
  updates. Both strict original frames pass. The new native parent crosses the
  actual typed1D77D0 service boundary; that child remains unconverted.
- [ ] Port the genuine1D77D0 update service and its dependency closure; a
  positive native matching parent does not make this numeric/callback child native.
- [x] Qualify native F1DD0 flip arithmetic and direct native BC200/B9800
  activation composition. Both compilers pass208 controller cases and288 full
  activation/depth cases, including208 completed declared flips/setters with
  zero residual calls. The coherent actual-caller adapter/model passes after
  correcting its test-only borrowed-observer lifetime. These inputs declare
  flip states and do not prove genuine gameplay rollover.
  [Full composition](../build/test-results/native-vehicle-flip-full-activation-tu15/report.json),
  [caller adapter](../build/test-results/native-vehicle-flip-handoff-coherent-tu15/report.json).
- [x] Exercise the wired native F24D8→F1DD0 controller visibly in v1465/00:
  837 native handoffs/kernel calls/flag skips, zero refusals/unbound/residuals,
  399 actual vehicle-camera frames and387 exact LAN comparisons. The selected
  driving frame450 matches114,363,573 original persistent bytes and FP state.
  Independent recording review confirms the actual chase camera and peer view.
  All837 calls are upright skips; completed flip/setter activity stays zero.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/iteration-00-summary.json).
- [ ] Produce genuine rollover/use-action8/reentry and positive native flip/
  activation. Upright controller selection alone does not qualify this action.
  The separate controller/action gates prevent ordinary flag skips from
  masquerading as a completed flip. Unqualified activation services count and
  throw; there is no lifted fallback. v1464/05 passes eight regressions but
  its official live case is budget-not-run; the separate bounded child expires
  before completing controller/camera/original proof. Neither is live acceptance.
  A fresh owned v1465 cohort is rebuilding the exact source before the retry.
- [x] Port the321510 pair consumer and its native normalization, quicksort,
  merge and temporary-record lifetime helpers:521 source sites across six
  owners. Both compilers pass32 positive declared-input cases, ordered reads/
  stores, genuine callbacks and three rejecting mutations. Independent callback
  GP/SIMD poison and authenticated callback-input audits pass. These inputs
  derive from real existing pairs; captured-positive pair changes are not claimed.
  [Component](../analysis/physics/native-pair-consumer-tu15-20261101-callback-context-qualified/report.json),
  [callback audit](../analysis/physics/native-pair-callback-input-context-tu15-20261101-qualified/report.json).
- [x] Qualify native2E9A00 collision group/mask filtering:1,155 physical
  original cases pass with GCC and Clang (1,146 full returns and9 read/write/
  observer-failure prefixes), all32 matrix rows/bits, output aliases,3,411
  ordered reads/1,148 stores and three rejected mutations. The composed
  native consumer/filter also passes all32 parent cases per compiler and three
  rejected mutations; both kernels are selected in the new world binding.
  Its model and visible qualification remain separate pending checks.
  [Results](../build/test-results/native-shared-pair-filter-tu15-20261101/report.json).
- [x] Qualify the native BDA50 object-publication root, BCFB0 root selection,
  BE7E0 recursive tree and existing native B9390 ancestor marking:372 physical
  original cases under both compilers, including117 failure prefixes and three
  rejected mutations. Atomic RMW prewrite-observer and callback-mutated bank
  domains remain explicit gaps. Its actual frame binding is wired for testing.
  [Results](../build/test-results/native-object-publication-tu15-20261101/report.json).
- [x] Qualify the actual native frame/publication binding: the model exercises
  two completed native publication calls/tree visits, four ancestor-wake calls
  and actual header stores without the old BDA50 callback. All65 original frame
  prefix/tail cases,248 ordered stores and both rejected frame mutations pass.
  This is component plus binding evidence; full frame child-return parity needs
  the live comparison below.
  [Results](../build/test-results/native-frame-object-publication-binding-20261101/report.json).
- [x] Qualify native object publication visibly in v1466/01:1,070 completed publications,3,401 tree visits and2,925 ancestor wakes, zero refusal, and strict114,363,573-byte+FP original frame450 agreement. BDA50 is absent from the residual census. Root-selection positive work is zero and stays open.
- [x] Qualify the native pair-consumer world binding: all110 original world
  cases,96 force compositions, the full callback/lifetime model and rejected
  world mutation pass. The model exercises native filter decisions and genuine
  create/delete boundaries. Its initial stale allocation-monitor expectation
  remains a retained failure; corrected allocation DATA and source monitor
  behavior pass without fabricated321510 effects.
  [Results](../build/test-results/native-world-pair-consumer-binding2-20261101/report.json).
- [x] Qualify that native pair binding in visible multiplayer with positive
  pair/filter input, no old321510/2E9A00 owner, zero refusals and strict original
  comparison. v1465/01 passes eight regressions, then genuinely reaches an
  unqualified persistent record release before its first rendered authority
  frame. No live gate or strict frame is counted. Typed original entry capture
  and native allocator closure are in progress. Create/delete bodies remain
  explicit migration dependencies; slow arena and persistent growth services
  refuse until qualified. The actual02 typed BEFORE now qualifies native
 149DA0/149E00 routing and pool release/allocation across genuine1→3 growth:
 89,734,085 bytes and146 ordered parent events match under both compilers.
 Native2E10A0/2E1110 slow arenas also pass33 component cases and nine full
 parent cases. Both are wired with strict unknown-owner refusals. In03/04,
 eight regressions pass and live execution reaches the next unqualified delete
 owner320040 (41 authenticated sites). Its native wrapper is now qualified and wired, including exact callback rereads and native2E4D60. In05, the next315A70 overlap removal is reached; its native80-site closure with314FE0 is qualified and wired. Final06 reaches315100 on receiver1007BCC0. Its new native variant passes38 component cases and all89,795,490 bytes from the authenticated earlier02 input on both compilers, including count5→4 and the9-byte event packet. The wired world model exercises both symmetric removals. No completed camera/original frame is counted for v1465/01–06. v1466/01 now completes966 native pair calls,12 nonempty inputs,7 filters,7 creates and6 deletes, plus3 owned/pool allocations and releases each. Native320040 runs3 times, dispatching one315A70 and two315100 removals with zero unknown/refusal. Strict frame450 matches114,363,573 persistent bytes and FP, with388 actual vehicle-camera frames and386 exact LAN comparisons. The genuine320EF0/320260 children remain residual owners; this closes the selected pair parent/removal path, not full native pair lifecycle.
 Alias and observer/fault-prefix coverage remains open.
- [x] Qualify and wire native320040 phantom pair deletion,315A70 complex overlap removal/314FE0 notification, and315100 simple overlap removal. Unknown callbacks still count and refuse.
  [Deletion](../analysis/physics/native-phantom-pair-delete-tu15-20261101-qualified/report.json),
  [complex removal](../analysis/physics/native-phantom-overlap-remove-tu15-20261101-qualified/report.json),
  [simple removal](../analysis/physics/native-simple-phantom-overlap-remove-tu15-20261101-qualified/report.json),
  [world binding](../build/test-results/native-world-simple-phantom-overlap-remove-binding-20261101/report.json).
- [x] Include genuine frame-level callees in the residual-owner census, merging counts with world-level owners. The65-case original frame comparison and histogram model pass; future completed live frames must account for these additional residuals.
- [x] Remove the completed v1465 owner and all its temporary source/build outputs:1,144,471,552 allocated bytes removed. Keep requested working-camera video and the required compressed BEFORE reference.
  [Cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-flip-controller-original-v1465-20261101/owner-cleanup-verification.json).
- [x] Wire the qualified native slow-arena owners2E10A0/2E1110 into broad-phase and island contact buffers as well as pair consumption. An exhausted-capacity world model passes with real native pool backing and balanced release, with neither old owner called. Visible positive slow-arena coverage is next; unknown backing still refuses.
  [Binding](../analysis/physics/native-general-stack-arena-world-binding-20261101/source-pins.json).
- [x] Port and wire native282AB0 inactive-island pair cleanup and native31B730 policy selection. Both compilers pass176 physical cases (121 full returns,3 true policy-entry stops,52 fault prefixes),4,223 ordered reads,163 stores and3 rejected mutations. The world model clears a real declared pair and replaces a kind2 policy with the native default. Invalidation/destructor/base-lock children remain separately counted residuals; callback-mutated cohorts and visible qualification remain open.
  [Original comparison](../build/test-results/native-inactive-island-cleanup-tu15-20261101/report.json),
  [binding](../analysis/physics/native-inactive-island-cleanup-world-binding-20261101/source-pins.json).
- [x] Qualify selected inactive-island cleanup/destruction visibly in v1466/06:11 native282AB0 calls clear12 pairs, select12 native policies and retire8 policies through native1D9C80. Cleanup/refusal/policy-residual counts are zero; old282AB0/1D9C80 owners are absent. Strict original450 matches114,363,573 bytes and FP, with333 actual vehicle-camera frames and390 exact LAN comparisons. Nested invalidators2F3100/2F7A20/323220 remain genuine and listed above.
- [x] Fix native flip store observation without changing source addresses: v1466/03 single-frag damage tracing now completes one genuine impulse, target health0.5730 and two submitted debris owners, with377 actual vehicle-camera frames and strict114,887,921-byte+FP original1200 agreement. Native events use the root actual-TU1.5 observer tee and avoid canonical-PC remapping; tracing stays enabled.
- [x] Drive into the actual finite row50 wall using source camera/body-yaw SDL feedback: eight stopped-under-throttle ticks, minimum plane distance0.0703 and native97 acceleration/30 approach calls. Brake input follows the completed wall segment and cannot satisfy its stop criterion.
- [ ] Resolve the wall03 and cleanup04 strict heap mismatches. Wall03 unchanged-original replay differs at225 bytes in88 runs; hardware watch identifies original2E2E43 repeat-copy at the first differing word. This identifies a writer, not inactive padding or permission to waive bytes.05/06 normal passes do not erase04's unique failing input. Required lossless03/04 archives remain; raw duplicates were removed, freeing240,660,480 recorded allocated bytes net of retention receipts.
  [Wall diagnosis](../analysis/physics/native-wall-heap-mismatch-diagnosis-20261101/report.json),
  [write watch](../analysis/physics/native-wall-heap-write-watch-20261101/report.json).
- [ ] Convert island regrouping3187C0 (482 sites) and quantized endpoint
  update2EA380 (980 sites), including allocation/callback lifetimes. These
  are authenticated remaining owners, not completed native implementations.
  [Inventory](../analysis/physics/native-live-residual-inventory-20261101/report.json).
- [x] Qualify the bounded native velocity/wake closure through `22D460`:
  48 physical F2091 cases per compiler preserve4,381,945,008 bytes and FP,
  with zero residual owners/refusals. The genuine positive constraint loop
  still has explicit unbound/count-and-throw services; it never falls back to
  the original whole owner. Actual upright driving has zero velocity work,
  so genuine flip/wake and positive-row gates remain open.
  [Results](../build/test-results/native-object-velocity-body-constraint-transport-tu15/report.json).
- [x] Qualify and wire native payload predicate, contact-manager construction
  and body policy. Forty-eight actual F2091 cases per compiler pass; v1463/11
  builds the binding and passes its driving original comparison. Upright
  driving performs zero velocity/wake calls and is not flip acceptance.
  [Results](../build/test-results/native-object-velocity-next-transport-tu15/report.json).
- [x] Qualify and wire native model-constraint lookup and capacity admission
  (`1DD710/22DEF0/22E040/22E000/22E0D0`, `1DD7B0/22DD90`). Both compilers
  pass152 actual-island cases per component, including64 found/visible lookup
  results and168 genuine capacity calls per compiler. The composed48-case
  F2091 proof passes with zero residual calls/refusals. Its authored constraint
  banks are empty; saturated capacity/removal and creation remain unqualified
  and explicitly unbound. The next v1464 source refresh carries this binding.
  [Lookup](../build/test-results/native-model-constraint-lookup-coherent-tu15/report.json),
  [capacity](../build/test-results/native-model-constraint-capacity-tu15/report.json),
  [transport](../build/test-results/native-object-velocity-constraint-services-transport-tu15/report.json).
- [x] Port native22D0A0 constraint removal and compose its qualified release,
  body-slot, scalar-cache and data-array deletion helpers. Forty physical
  island/FP cases per compiler remove64 actual instances, free28 lists and
  release68 references, preserving5,758,527,040 bytes per compiler. The staged
  actual48-case F2091 binding passes with zero residuals/refusals and is wired.
  That transport has zero removal calls; saturated-capacity composition and
  live positive removal remain unexercised. Actual17A0F0/22FEA0 children remain
  explicit count-and-throw dependencies, with refusal counters in both velocity
  and flip paths.
  [Removal](../build/test-results/native-model-constraint-removal-tu15/report.json),
  [binding](../build/test-results/native-constraint-removal-binding-transport-tu15/report.json).
- [x] Qualify and wire native22CEA0 constraint creation,22EBA0 local points,
 122B60 normalization and22EDE0 motion comparison. Both compilers pass256
 original/native constructor calls from40 island/rounding invocations,176
 normalizer profiles, and the combined48-case F2091 binding. Request DATA is
 explicitly projected from genuine descriptors;96 constructor inputs declare
 a valid real-bank slot where the dormant descriptor resolvesFFFF. The actual
 authored+80 caller and reference-distance loop stay unexercised.
- [x] Qualify and wire native17A0F0/17A030 retained-handle cleanup:280 original/
 native calls per compiler include208 valid operations on26 genuine salted
 handles. The combined transport passes with zero residuals/refusals, but
 performs zero handle calls. The qualified native C6120 value and17A4A0 destruction children are now wired separately below; deeper177FA0/177020/deferred children still count and refuse.
- [x] Qualify and wire native17A4A0 destruction and tagged-value cleanup. Both compilers pass140 destruction calls (104 valid),256 tag-clear calls,256 tag-presence calls and96 value-table calls. Positive child/deferred and value-publication branches remain unexercised. The combined F2091 transport has zero actual destruction calls; the component evidence is separate. Current native velocity/flip libraries compile and their temporary build is removed.
- [x] Wire bounded native22FEA0 reference cleanup into creation/removal.256 actual original/native calls per compiler qualify the existing kind1 counter profile with768 counter writes. Both compilers pass the combined48 F2091 transport calls with runtime work0. Admission explicitly requires flag11zero/kind1/all links and handles None/512 owned DATA bytes. State/effect/frame branches are refused until their positive qualification; original22FEA0 is never retried. Geometry/math/publication helpers have separate positive component proofs.
  [Binding](../analysis/physics/native-activation-constraint-runtime-kind1-binding-20261101/source-pins.json).
- [ ] Qualify these creation/handle/destruction routes visibly with actual authored inputs,
 positive native work and strict original comparison. Their new numerical,
 storage and child-owner refusal counts must remain zero. The inverse-frame
 arithmetic has one shared native archive; its unchanged existing camera
 regression passes. The destruction/tag-value overlay is component and binding qualified and builds in current root; positive authored live work remains open.
- [ ] Build and visibly exercise the wired F2091 native velocity/wake route
  using genuine rollover/flip input. The new per-world gate requires positive
  completed work, zero residual calls/refusals and no unknown factory/getter.
  Bounded native replacements now cover `22D460/31B730/281F10/3232E0`; positive
  constraint creation `22CEA0`, capacity lifetime `22D0A0` and their transitive
  dependencies remain open. A zero-row component pass does not close them.
- [ ] Verify grenade-to-vehicle impulses, destruction/debris and prop impulses.
  The source-camera SDL aim fix and declared ordinary stance input produce
  genuine target impulses and damage in v1463/14–15, with strict original
  comparison passing. The rocket02 case independently reaches vehicle death;
  full retirement and independent framebuffer proof of individual debris
  remain open. Object-case original
  comparison passes, while
  `dynamic_prop_impulses_verified` remains false.

November1 continuation: v1463/00–15 have actual completed multiplayer
results, using the imported MCC cache and actual vehicle camera. The declared
Warthog cached-motion/body/sphere/direct-constraint/contact-completion routes
pass bounded original comparison. Two genuine vehicle impulses and partial
damage pass; island listeners, deferred delete/update, full destruction and prop
impulses remain open. Gemini's original-frame comparison is red without waivers.
Interrupted v1462 never reached live testing; its stale running state is not a pass.

- [x] Prove the native `1DA530` non-power scalar domain against complete original
  calls, numerical controls/status and an arithmetic mutation control: 8,832
  calls pass. [Results](../analysis/physics/native-lift-scalar-tu15-20261026-final/report.json).
- [x] Complete the declared positive-exponent power/special/masked-CRT domain:
  16,560 complete original calls, actual thread-errno ownership and three
  rejected mutations. [Results](../analysis/physics/native-lift-power-tu15-20261026-crt-final/report.json).
- [x] Qualify the typed lift consumer against 16,560 original scalar calls,
  including guarded errno and complete FP state. Then qualify 576 complete
  original `1DA750` parents: 1,152 native calls reach all four real scalar seams,
  with 288 calls per seam. The wrong-delta mutation is rejected.
  [Parent results](../analysis/physics/native-lift-parent-tu15-20261026-final2/report.json).
- [x] Qualify 96 source-phase alias cases with 144 declared observer writes,
  including late exponent/acceleration, captured-clock rereads, ratio/scratch
  aliases and real errno callbacks. This does not admit arbitrary mutable
  observers in live gameplay.
  [Results](../analysis/physics/native-lift-alias-tu15-20261026-first/report.json).
- [x] Qualify the declared live lift scalar/power route on Midship and Lockout:
  all four native seams execute, with counts `[240,54,94,240]` combined,
  240 power calls and zero observer refusals. Both captured whole-frame original
  comparisons pass. The scene bypass fix independently passes 576 original
  parents and a rejected wrong-result control with exactly one observer pair.
  [Scene proof](../analysis/physics/native-scene-lift-parent-tu15-20261026/proof/report.json).
  Broader authored field coverage is running; Pentium III transcendental identity
  and unmasked faults remain separate open gates.
- [x] Prove the typed world frame's declared original root-prefix/tail domain:
  65 cases, 248 ordered persistent stores and 1,330 ordered data reads relative
  to those stores, with original i386 FP verification and two rejected native
  mutations. [Results](../build/test-results/native-physics-frame-read-phases-tu15/report.json).
- [ ] Wire and qualify the native frame with real collision/integration,
  post-step/contact/publication/destruction callbacks. Convert all residual
  owners and verify the composed world visibly in multiplayer.
- [x] Qualify the current partial native frame visibly in multiplayer: v1455
  passes 9/9 player, camera/vehicle, object, gravity-lift and Cyclotron cases,
  with 3,515 native frame calls and 2,215 exact LAN comparisons.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-live-v1455/matrix.json).
  Its 26,731 residual calls keep the preceding closure gate open.
- [x] Independently replay the declared actual composed native-frame inputs:
  v1456 passes vehicle/camera on Coagulation and gravity lift on Lockout, with
  89,729,945 and 89,185,177 persistent bytes compared respectively, complete
  MXCSR/x87 policy/status and an empty x87 return. Only the returned private
  64 KiB callee stack is excluded; volatile GP/SSE are not outputs of this void
  API. [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-original-v1456/matrix.json).
  These two captured frames do not close all callback, trajectory, native-world
  or native-only dependency coverage.
- [x] Prove the complete semantic vehicle-rate algorithms: 2,230 original cases,
  including 792 plain typed value-owner cases, exceptional inputs, ordered
  writes and fault prefixes. The arithmetic mutation is rejected.
  [Results](../analysis/multiplayer/physics-native-rate-observer-tu15-20261026/verification.json).
- [ ] Qualify their composed live use with required observation enabled and
  replace remaining outer controller execution. A few native calls alone do
  not close this gate.
- [x] Prove native force/torque/gravity/damping and angular impulse against
  unchanged physical i386: the expanded 174-case, 1,032-store suite exposed a
  paired-NaN compiler operand-order bug. Ordered native operations now pass
  under [Clang](../build/test-results/native-motion-forces-paired-nan-clang-tu15/report.json)
  and [GCC](../build/test-results/native-motion-forces-paired-nan-gcc-tu15/report.json),
  with full FP state and a rejected gravity-removal mutation. Physical stores
  supply NaN authority; 26 cases differ from emulator-generated stores.
- [x] Prove the native motion pose and quaternion/normalization/matrix closure:
  210 physical original cases, 8,229 ordered reads, 4,301 ordered stores, seven
  read-fault prefixes, 24 paired-NaN cases and 206 actual native RSQRTSS
  observations pass under GCC and Clang. A precise reciprocal-root replacement
  fails. [Results](../build/test-results/native-motion-pose-tu15/report.json).
  Xbox/Pentium III estimate bits remain a separate open gate.
- [ ] Qualify its direct native world binding visibly in multiplayer with
  per-world pose counts and whole-frame original comparison. The binding model
  confirms native body motion and omits the former lifted pose call; current
  [component results](../build/test-results/native-world-integration-pose-binding-tu15/report.json)
  do not replace live acceptance.
- [ ] Wire those force kernels through native island/world integration and
  qualify their real multiplayer execution. A compiled unused module does not
  close this gate.
- [x] Prove the declared native island integration domain: 103 original cases,
  including 96 direct native-force compositions and genuine residual stops.
  [Results](../build/test-results/native-island-integration-tu15/report.json).
- [x] Prove the declared native world-integration domain: 110 original cases,
  1,719 ordered stores and 96 native-force compositions, including external and
  aliased Step packets. [Results](../build/test-results/native-world-integration-tu15/report.json).
- [x] Qualify native island-deactivation scheduling and its default policy:
  290 physical original cases, 3,254 ordered reads, 1,038 stores, 19 read-fault
  prefixes and 16 paired-NaN controls pass under GCC and Clang. Twelve complete
  default-policy calls and mutable callback/alias models pass; custom policies
  remain open. [Proof](PHYSICS_TU15_NATIVE_POSE_CONTROLLER_PROOF_20261026.md).
- [ ] Qualify the newly wired native deactivation controller visibly with its
  actual per-world activity and whole-frame original comparison. The native
  binding model passes, with 19 remaining residual ABI phases.
- [x] Qualify complete native keyframed motion `323730` and its quaternion
  rotation dependency `2E5810`, plus authenticated prescribed-force/fixed-pose
  no-ops. The 275 physical cases include 24 paired-NaN controls and eight
  read-fault prefixes; both arithmetic mutations are rejected under GCC and
  Clang. [Results](../analysis/physics/native-kinematic-motion-tu15-20261026-final/report.json).
- [x] Qualify native island transition `316CD0` and notification `322740`:
  178 physical cases, 7,680 reads, 914 stores and 44 read-fault prefixes pass
  under both compilers. Includes real default callbacks, mutable callback
  models and rejected index mutation. Growth and custom listeners remain
  explicit dependencies. [Proof](PHYSICS_TU15_NATIVE_ISLAND_TRANSITION_PROOF_20261026.md).
- [x] Qualify the native short/long body sleep policies: 432 physical cases,
  including 32 fault prefixes and 48 paired-NaN cases, pass under both compilers;
  the strict-distance mutation is rejected.
  [Results](../build/test-results/native-body-deactivation-policy-tu15/report.json).
- [x] Qualify native array growth/copy, allocator routing and the declared pool
  domain: 176 unchanged physical calls, 3,574 reads, 2,885 stores, five fault
  prefixes and four observer mutations pass under GCC and Clang. Capacity,
  copy-granularity and accounting mutations are rejected. Default/secondary
  allocators and oversized imported free remain named dependencies.
  [Results](../analysis/physics/native-array-growth-tu15-20261026-final2/report.json).
- [ ] Qualify its new native island-transition binding and real multiplayer
  allocation lifetime; retain per-world growth/pool activity and original
  comparisons. Unknown allocator owners still execute explicit residual calls.
- [x] Qualify constrained dynamic accumulator `325410`, transpose `2E4820`
  and its genuine native angular-impulse composition: 322 physical cases,
  including 74 read-fault prefixes and 32 paired-NaNs, pass on both compilers;
  the transpose mutation is rejected. This is a component proof.
  [Results](../build/test-results/native-dynamic-accumulator-tu15/report.json).
- [x] Qualify native world collision scheduling `319D50` and both actual
  collision-listener owners `3228C0`/`3229E0`: the final 300 physical cases
  include 173 genuine completions, ten required-owner stops, 89 read-fault and
  28 integer-store fault prefixes, eight paired-NaNs, 7,443 ordered reads and
  3,431 stores. Both compilers and the collision-flag mutation control pass.
  [Results](../analysis/physics/native-world-collision-tu15-20261026-final/report.json).
- [ ] Visibly qualify the new collision/listener binding in a declared build.
  Its 31-phase actual residual ABI/compaction model and 65-case native-frame
  proof pass. Island collision/contact owners and custom callbacks remain
  explicit dependencies; native-only collision closure is still incomplete.
- [x] Qualify native island collision scheduling `321E50`, allocation monitors
  `282B70`/`282B90` and allocated-byte summation `149B90`: 190 physical cases
  include 107 complete helper calls, 35 authentic first-phase stops, 41 read-
  and seven integer-store fault prefixes. Both compilers and the stride
  mutation control pass. Remaining phase owners stay explicit.
  [Results](../analysis/physics/native-island-collision-tu15-20261026-first/report.json).
- [x] Qualify the native constraint continuation from genuine original solver
  returns: 185 cases include 124 complete original roots, 57 fault prefixes,
  four dependency stops and 24 one-row tail cases. No solver result is supplied.
  [Results](../build/test-results/native-constraint-continuation-tu15/report.json).
- [x] Qualify all three native motion-commit kernels: 664 physical cases,
  including 142 read-fault prefixes and 92 paired-NaNs, preserve 13,846 stores,
  28,406 reads and 594 observed RSQRT estimates under both compilers. Three
  arithmetic/cursor mutations are rejected. Xbox estimate identity stays open.
  [Results](../build/test-results/native-motion-commit-tu15/report.json).
- [x] Qualify the complete typed row-solver domain against 3,000 physical
  original cases: 49,155 stores, 296,611 reads and 1,110,116 unchanged original
  instructions, all twelve primary row families, secondary/export branches,
  GCC/Clang and rejected warm-start/precision mutations.
  [Results](../build/test-results/native-constraint-solver-tu15/report.json).
- [x] Qualify the native solver initializer: 113 physical cases, original
  hash/date rules, real clock and logger boundaries, fault prefixes and rejected
  mutations. The ready-zero solver/initializer composition also passes 103
  original cases, including 28 full roots and 73 fault prefixes.
  [Initializer](../build/test-results/native-constraint-solver-initializer-tu15/report.json),
  [composition](../build/test-results/native-constraint-solver-initialized-composition-tu15/report.json).
- [x] Qualify native broad-phase allocation, shape/sweep arguments, pending-pair
  growth and cleanup against 236 physical original cases with both compilers
  and a rejected ownership mutation. Nine completed v1459 refresh cases record
  10,438 actual native broad-phase calls; seven planned cases remain incomplete.
  [Component proof](../analysis/physics/native-broad-phase-tu15-20261026-final/report.json),
  [partial live cohort](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-02-interruption-summary.json).
- [x] Qualify all three built-in native custom-schema callbacks: 336 physical
  cases under both compilers, including actual before-store rejection and
  after-store observer exceptions. Early-pop/C1 and operand-order mutations
  are rejected. Unknown registered owners remain explicitly refused.
  [Results](../build/test-results/native-constraint-custom-schema-final-tu15/report.json).
- [x] Qualify native FILETIME conversion with 292 original division cases and
  ten actual clock returns. The live binding consumes the existing recorded
  world FILETIME input and adjustment. The composed native clock, initializer,
  solver and three custom callbacks pass 143 cases across all twelve row families.
  [Clock](../build/test-results/native-constraint-clock-tu15/report.json),
  [composition](../build/test-results/native-constraint-solver-native-clock-composition-tu15/report.json).
- [x] Qualify native array reserve2E31E0: 57 physical cases, 2,833 reads,
  2,226 stores, five fault prefixes and 122 actual allocator calls. GCC/Clang
  pass; the wrong-capacity mutation is rejected. The expanded64-case proof adds
  seven actual argument-cell cases, preserving early stride/capacity and late
  header reads across real allocation; an early-header mutation is rejected.
  Live reserve activity remains zero; additional internal callers need wiring.
  [Results](../build/test-results/native-array-reserve-tu15/report.json).
  [Input phases](../build/test-results/native-array-reserve-input-phases-tu15/report.json).
- [x] Qualify the scoped vehicle-rate native route with the original frozen and
  current headers: 2,230 cases each. An additional 1,382 physical original calls
  include 6,252 actual instruction FP snapshots and both store-failure phases.
  Source wiring has per-world activity and explicit live refusals; current live
  ordinary Warthog reversal now exercises239 acceleration/60 approach/299 cursor
  calls with no refusal,334 actual vehicle-camera frames and a strict original
  late-frame match. Outer controller conversion remains open.
  [Results](../analysis/physics/native-vehicle-rate-final-handoff-tu15-20261026/report.json).
- [x] Qualify native schema14 emission32A390: 69 original cases, including
  actual argument/header aliases, read/write faults and observer failures.
  The three real caller continuations consume only the returned payload pointer;
  their complete original callsite/ABI audit passes. The scoped live seam and
  per-world counters are wired. All 210 original cases pass with retained and
  current headers under GCC/Clang. The combined live cohort records15,478
  native calls/completions through all three original callers, with no refusal.
  [Emitter](../build/test-results/native-constraint-schema-emitter-final-tu15/report.json),
  [caller audit](../build/test-results/native-constraint-schema-emitter-abi-tu15/report.json).
- [x] Qualify the three native geometry kernels2E44D0/2E47B0/2E4690:
  758 physical cases,38,585 original instructions,6,623 ordered reads and1,673
  stores under GCC/Clang. Alignment faults, aliases and all three arithmetic
  mutation controls pass. Native live binding remains open.
  [Results](../analysis/physics/native-geometry-tu15-20261026-qualified/report.json).
- [x] Qualify native inverse-affine geometry2E4550:269 physical original cases,
  2,939 ordered reads and1,212 stores under GCC/Clang. Transpose, arithmetic
  association and early-translation mutations are rejected. Native caller
  integration through candidate update2F36F0 remains open.
  [Results](../build/test-results/native-inverse-geometry-final-tu15/report.json).
- [x] Qualify the declared2F3140 dispatch input domain with genuine class/factory
  returns:14 cases,112 ordered data reads and1,265,214,046 persistent bytes per
  compiler. Both wrong-factory controls are rejected. This is an offline
  service composition; native factory and actual live creation coverage stay open.
  [Results](../build/test-results/native-contact-agent-dispatch-read-order-tu15/report.json).
- [x] Correct the reviewed2F36F0 timing/cleanup discrepancies and qualify the
  candidate/inverse/dispatch/reserve/buffer composition:42 cases per compiler
  compare3,795,642,138 persistent bytes and9,915 ordered operand events, with160
  real creations,14 releases and43 native reserve calls. All three mutations
  fail. The old review remains historical failure evidence.
  [Final composition](../analysis/physics/native-contact-candidates-helpers-composed-tu15-20261026-qualified/aggregate.json).
- [ ] Wire candidate update and native factories into the live contact path.
  Keep full304350 producer raw-record/constructor lifetime failures open; the
  stronger component composition does not waive those differences or qualify
  complete contact generation.
- [x] Qualify native constraint base329720 against837 physical cases/prefixes:
  all921 sites,60,160 reads and21,388 stores under GCC/Clang; all four mutations
  rejected. Private transient error payloads, unmasked traps and Xbox-specific
  estimates remain separate open gates.
  [Results](../build/test-results/native-constraint-base-final-tu15/report.json).
- [ ] Qualify31F770/31F860 native wrapper composition with that native base,
  then wire and visibly qualify their real per-world execution.
- [x] Qualify their full native base/emitter composition:922 cases/prefixes,
  106,429 reads and41,514 stores under GCC/Clang, all three mutations rejected.
  The working-tree native binding and real-owner model pass; live activation
  and complete world qualification remain open.
  [Composition](../build/test-results/native-constraint-producer-first-tu15/report.json),
  [binding](../build/test-results/native-constraint-producers-binding2-tu15/report.json).
- [x] Qualify seven native shape-bound owners and their geometry composition:
  1,195 physical original cases under GCC/Clang,627 complete chains,23,544
  ordered reads and5,787 stores. All four arithmetic/read-timing mutations
  are rejected. Twelve transient radius-read x87 fault prefixes remain open.
  [Results](../build/test-results/native-shape-bounds-final-tu15/report.json).
- [x] Qualify additional actual2E8120 endpoint/306170 compound bounds:334
  physical calls/prefixes,9,341 reads and2,463 stores under both compilers, with
  two rejected operand-order mutations. Private unbalanced radius faults stay
  explicitly outside the admitted domain.
  [Results](../build/test-results/native-compound-shape-bounds-tu15/report.json).
- [ ] Qualify the newly wired native broad-phase bounds in visible multiplayer:
  require positive native leaf work and zero remaining shape-bound callbacks,
  actual vehicle camera and strict original-frame comparison. The110-case
  world proof, nested-transform caller-storage model and rejected old-overlap
  layout pass; fresh live qualification is queued after the exact-source build.
  [Binding](../build/test-results/native-world-shape-bounds-binding-tu15/report.json),
  [storage control](../build/test-results/native-world-shape-bounds-caller-storage-negative-tu15/report.json).
  v1461/03 now records substantial native leaf/compound work and a passing
  original frame, but45 residual bounds calls keep this gate red. Exact owner
  diagnostics are added in the working tree for the next cohort; the earlier
  single-snapshot census is not an exhaustive trajectory inventory.
  [Latest live](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-input-original-v1461/iteration-03-summary.json).
- [x] Qualify native contact-record copy303BA0, predicate303800 and filter304130:
  796 physical cases,12,741 ordered reads and5,449 stores under GCC/Clang,
  including130 genuine captured payload inputs and both store-failure phases.
  Opaque source gaps stay untouched; four arithmetic/publication mutations
  are rejected. The complete304350 producer and live activation remain open.
  [Results](../analysis/physics/native-contact-records-tu15-20261026-qualified/report.json).
- [x] Qualify the declared native island-agent phase composition with genuine
  original service execution:20 cases,1,228 ordered reads,424 stores and strict
  91,356,057-byte comparisons per case under both compilers. Actual observed
  callback-entry GP/private-stack state is an explicit input dependency.
  [Results](../build/test-results/native-island-agent-hardware-context-final-tu15/report.json).
- [ ] Qualify the newly wired native island-agent phases with the production
  callback transport, genuine contact buffers and visible multiplayer activity.
  Close the service-context dependency and the3205B0 advance-return branch.
- [x] Qualify the declared live flag-set phase on Warthog:986 phase entries,
  1,169 real process/finish calls each and986 constructed/destroyed buffers,
  actual vehicle camera, all required rate/core gates and a strict89,729,945-byte
  original sample. Flag-clear/minimum remain zero in this case; five time
  advances still invoke the unconverted owner. The preceding full gate is open.
  [Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-06-island-agents-vehicle-camera/run/lan-vehicle/report.json).
- [ ] Visibly qualify the newly wired complete solver/initializer/custom
  callbacks, scoped vehicle rates and array reserve. Require positive solver
  iterations/primary rows in the actual vehicle case and record each exercised
  native owner. Native source wiring and ABI models do not close this gate.
- [ ] Qualify complete constraint orchestration, dynamic accumulator setup and
  all actual solver owners, then visibly qualify the newly wired constraint,
  island-collision and commit paths. Actual solver/builder/allocator services
  remain dependencies; prefix and component proofs do not complete this group.
- [ ] Wire and visibly qualify keyframed/fixed motion and native island
  transitions in the declared live build; qualify every exercised motion kind.
- [ ] Finish native collision/pose/constraint/action/deactivation/listener and
  remaining solver ownership; integrate and visibly qualify the full closure.

Latest native world/pose cohort v1457: all nine visible cases and native pose
activity gates pass, including the actual vehicle camera. Six whole-frame
original comparisons pass; three Cyclotron raw-bank comparisons remain red.
A [physical lifetime diagnosis](PHYSICS_TU15_V1457_CONTACT_SCRATCH_DIAGNOSIS_20261026.md)
locates the representative 95-byte difference entirely in stale inactive
contact records freed before frame return. The raw failure is preserved; a
source-owned lifetime audit and matching native writer trace remain separate
from acceptance. No broad goal group or native-only closure is completed by
these results.

v1458 adds the native sleep/wake checker and the four lift scalar seams through
qualified read-only observation scopes. Its five official regressions pass;
all nine visible cases record native frame/world/island/force/pose/checker
activity and 2,215 exact LAN comparisons. Eight acceptance reports pass and
six whole-frame original comparisons pass. The same three Cyclotron raw-bank
comparisons remain red. Gravity lift additionally fails the required native
scalar coverage gate: all four call counts are zero, with no observer refusals.
A dedicated authored force-field case is still needed; prerequisite scalar
proofs do not satisfy that live requirement. The
[fresh vehicle-camera clip](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-01-vehicle-camera/requested-video/window.mp4)
shows the current backend with 439 seated frames, approximately 0.6 radians of
look and 41 post-exit frames. [Cohort summary](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-00-summary.json).

The subsequent v1458 motion/policy cohort passes seven main regressions,
nine of ten acceptance reports and seven of ten whole-frame original
comparisons. It records 5,541 native keyframed poses, 5,541 prescribed-motion
calls, 1,286 body-policy calls and 87 island transitions across 2,779 exact LAN
comparisons. Fixed-pose live coverage remains absent. The dedicated authored
Midship field also reports zero scalar call-site activity; its required gate
remains red while its world-frame original comparison passes. This coverage or
wiring gap and the three Cyclotron raw-bank failures remain open. The gap is
now traced to the scene owner's duplicated force parent. The source and
generator bridge that entry to the dedicated force owner; its independent
original and fresh live qualification are pending.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-01-summary.json).

The test-only diagnostic then captures the actual `319D50` residual from the
same compared frame. Its default full GP/SSE/x87/FP and 90,131,353-byte original
comparison passes; a persistent-byte mutation is rejected. Only the private
replayer's entry admission literal changes, with its diff and source hashes
saved. The whole-frame raw gate remains red and the collision owner remains
lifted. [Proof and cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-02-collision-private-entry-replay/report.json).

The later scene-route fix resolves the native scalar bypass: a duplicate scene
`1DA750` had continued to own the live force path. Its exact ABI/FP bridge passes
576 independent original parents and a rejected zero-result control. All ten
visible cases now pass, with actual scalar counts `[240, 54, 94, 240]`, 240 power
calls and zero observation refusals. Midship and Lockout gravity together cover
all four sites; the earlier zero counters did not prove gravity was unsuitable.
Seven complete original world frames pass; the three Cyclotron raw-bank cases
remain red. The first successful scalar-field recording and a losslessly
compressed exact phase trace are retained.
[Current scene-route results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-03-summary.json).

Native array/allocator integration then passes all 16 visible cases, with
6,917 exact LAN comparisons and 13 growth/13 pool allocation/11 release calls.
Ten complete original frames pass; Gemini0, Gemini1 and Elongation expose three
additional raw-bank failures, preserved with separate exact paired diagnostics.
The next collision/listener cohort passes all ten core/Midship native gates and
records 4,307 collision/world-listener entries; seven original frames pass and
three Cyclotron comparisons remain red. Island-listener live coverage is zero.
No broad closure is inferred. [Array results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-summary.json),
[collision results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-05-summary.json).

v1458 explicitly removes its source/build tree after that cohort: 1,131,307,008
allocated file bytes. Unique requested clips, source pins and required authentic
comparison inputs remain; no executable or build tree remains.
[Cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/cleanup-verification.json).

The corrected v1459 constraint/motion cohort passes eight regressions and all
ten visible/native gates, with 2,629 exact LAN comparisons. Actual commit kinds
are `[748, 2090, 902]` (kinematic/dynamic/restore); all three have real coverage.
Seven full original world frames pass and three Cyclotron comparisons remain
red. An initial caller-packet lifetime bug was held from live execution and
corrected through a guarded rebuild plus a rejected old-layout control.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/iteration-01-summary.json).
Current component wiring replaces the whole solver and its three built-in
custom callbacks with typed kernels; the clock uses the same actual FILETIME
receipt. v1460 owns a fresh source/build for visible qualification. The earlier
frozen cohorts lack the newer typed vehicle-rate module and activity symbol;
its separate component proof is not selected live coverage. Native-only closure
remains open.

The completed v1459 failed captures are now losslessly archived: 24,226 files
roundtrip/hash verified, plus nineteen exact compressed hash reports. The net
scoped allocated reduction is 759,529,472 bytes, counting retained archive,
manifests and redirect receipts. The 71,698,926-byte archive remains required
reference input; its original failures stay red. Vehicle-rate proof cleanup
independently removes 146,665,472 allocated bytes and retains no large outputs.
[Capture cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-constraint-original-v1459/completed-inputs-retirement-01-02/cleanup-verification.json),
[rate cleanup](../analysis/physics/native-vehicle-rate-final-handoff-tu15-20261026/report.json).

Completed v1460 captures now also have a verified lossless replacement:
64,599 files roundtrip/hash checked before retiring their unpacked copies.
The scoped net allocated saving is1,705,365,504 bytes, after the194,532,631-byte
required archive, manifests and receipts. Protected v1460/00 lower0 and all
v1458 direct inputs remain. This storage change does not alter any failed gate.
[Cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/completed-inputs-retirement-except-lower0/cleanup-verification.json).

v1460/01 records real native solver/custom activation across all sixteen MCC
cases:138,352 iterations,296,920 primary rows and65,544 custom callbacks, with
all three native kinds positive and no refusal. Fifteen visible acceptance
reports pass; the Warthog case's approach-rate gate remains unexercised
(acceleration121/approach0/cursor121). A genuine Ghost driver supplement also
records approach0, with56 actual source vehicle-camera frames. Ten strict original
samples pass; six raw-bank failures remain red. Lower0 passes58 moving-support
ticks/2.3862369 carry on the repeat, while its earlier different-input failure
is preserved. Combined v1460/02 executes15,478 native schema-emitter calls through
all three callers with no refusal, but reserve stays zero on all sixteen cases.
The reserve's internal lifted helper routes remain a wiring blocker. Separate
v1460/05 Warthog ordinary SDL reversal passes the unchanged rate gate with
239 acceleration/60 approach/299 cursor calls,334 original camera frames and
89,729,945 original bytes equal. Its runtime still has other unconverted owners.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-01-summary.json).
[Combined results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-02-summary.json).
[Vehicle rate/camera proof](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-solver-original-v1460/iteration-05-vehicle-rate-reversal-retry-vehicle-camera/run/lan-vehicle/report.json).

Independent original contact traces now follow40 XMM3 provenance segments
through8,589 unchanged instructions, including actual returns/indirect callers,
with no read before each real register overwrite. This is case-specific
register evidence; future memory provenance and native paired histories stay
open. It does not waive raw bytes or prove every path.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/contact-xmm3-liveness-summary/report.json).
The two contact investigations remove1,997,619,200 measured allocated bytes of
owned scratch; native-rate proof cleanup and v1459 compression above are separate.

v1455 cleanup verifies removal of 1,125,720,064 allocated bytes of owned
source/build storage. It retains 57,926,174 bytes of results, including nine
requested recordings, and no binaries. [Cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-live-v1455/cleanup-verification.json).

v1456 cleanup verifies removal of 1,120,522,240 allocated bytes of owned
source/build storage; earlier per-case raw cleanup is additional. No raw banks,
replay programs, exported runtimes or duplicate recordings remain. Input-hash
reports are retained losslessly as `.json.xz`; the v1455 requested videos remain
available. [Cleanup receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-frame-original-v1456/cleanup-verification.json).

v1457 cleanup verifies removal of 1,126,776,832 allocated bytes from the
owned source/build tree. Seven required failing input directories are preserved
in a lossless 83,572,194-byte archive after all original diagnostic probes
finished. Round-trip extraction verifies all 28,264 file hashes; replacing the
raw captures reclaims another 832,573,440 allocated file bytes, net of the
archive and hash manifest. The three Cyclotron raw comparisons remain red.
[Build cleanup](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/cleanup-verification.json),
[archive and unpack receipt](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-original-v1457/required-failure-inputs-retirement/report.json).

Checked October20,2026. **14 of24 goal completion groups remain open**.
These groups contain many interactions;14 is not a count of missing functions
or unique interactions. Full1:1 physics parity remains incomplete.

This checklist is part of [the physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md).
Update it after each completed build/live cohort or material blocker change.
English OG Xbox TU1.5 is the behavioral authority;1.1 is outside this goal.
Live tests use the existing prepared imported MCC cache, preserve actual source
camera/controller/body assertions, and remove private test/build trees.

**14 of24 broad completion groups remain open.** Each group contains multiple
interactions; this count is not a count of individual collision cases.

**Current wired physics runtime: v1396-r2.** `Halo2-Physics-TU15` selects
native SHA `bec211bac82541ac2e8192c1152fb3c9a3e21bdfb6e4ff6a2a9233321669ff68`.
Qualification passes36 linked checks, three actual-MCC DXT3/ownership/lifecycle
checks, core v1434 **9/9**, fresh vehicle/seat v1435 **26/26**,54 compatibility
checks (build1396/iteration14), and3 rendered checks v1436. The26 cases include
22 allowed seat/camera cases and four genuine Scorpion rider rejections.
Banshee uses a declared0.3-radian takeoff input with unchanged travel/climb and
exact source-camera gates. Ordinary native v1437 passes120 frames/178 ticks/
two players; actual wired launcher v1439 passes120 frames/160 ticks/two players.
Both opened-file audits contain only imported MCC maps and no foreign caches/XBE.

Occupied Warthog-driver and Spectre restoration v1433 preserves the exact
physics bank/player bindings and continues471/623 exact LAN comparisons with
actual source cameras. Both original post-restore1CC8D0 calls independently
match89,795,481 final source bytes plus captured CPU/FP state. The original
failure was an omitted DXT3 checkpoint format despite existing64-sample
original constructor proof; the actual-MCC regression fails before the fix.
Successful copied banks/replay programs and each completed live scratch are
removed. Earlier deliveries, exact source archives and required shader paths
remain preserved. The old v1396 and v1453 private source/build trees have been
removed. Later native conversion tests own separate disposable storage.
Full1:1 parity is incomplete;14 broad completion groups remain open.

Earlier full delivery v1303 passes33/33 linked checks,
core v1306 **9/9**, seats v1308 **16/16**, and drivers/turrets v1313 **10/10**.
The core includes strict nonzero object impulses, Lockout gravity lift and all5
Cyclotron supports. Actual Warthog camera proof:430 submitted seated frames,
0.60 radians look,41 after-exit frames and464 exact LAN comparisons. Lower
Cyclotron phase0:58 support ticks,2.3862386 carry and244 LAN comparisons.
Both Spectre null-HUD regressions are fixed: moving occupied turret travel is
5.446917 units, with438 driver/614 gunner source-camera frames and642 LAN
comparisons. This preserves authored null HUDs as empty weapon interfaces;
genuinely pruned typed HUDs still fail selected submission.

Ordinary MCC startup v1315 passes120 frames/319 ticks/two players. Actual wired
launcher v1317 passes120 frames/295 ticks/two players. Opened-file audits have
zero foreign maps or XBE/disc/Vista inputs. Core, seats, drivers and both startup
scratch trees are removed. Required executable/shader/source packages and
requested recordings remain with explicit retention receipts.

**Campaign V03 stays open: current v1381 has34 raw passes/13 failures.**
All47 contexts use one exact v1369-r1 runtime, declared first/bound observer
inputs and imported MCC maps. Machine355's context32 now passes in the full
sweep; all13 remaining failures reject unavailable initial observer starts.
Its private scratch is removed. Current motion review, especially33/42 late
occlusion, remains a separate gate. Earlier v1337 records33/47 below.

Consolidated v1337 verifies47 distinct contexts from v1330/v1336, actual MCC
inputs, stable per-case runtime/map pins and removed private scratch. Thirteen
contexts lack admitted observer starts; context32 reaches machine355's missing
native PRT caller. These are raw controller/projection/LAN results; motion
recording review is a separate gate. All production source matches v1303;
contexts35/42 explicitly use the v1321 initial-observer fixture. The other45
use the original first-clear-floor fixture. The v1330 fixture-switch hash
assertion was a harness bookkeeping failure after saving case35; v1336 runs
only the remaining11 contexts, preserving every raw receipt.

The bounded floor survey makes context35 readable in both reviewed views.
Context42 still loses the target during overhead motion. Current v1342-b
samples at20/50/85 percent show0/1/2 clearly, but33 becomes substantially
occluded late and stays open. All12 decoded review frames are removed; original
requested recordings and small assessments remain. Prior combined sampled
coverage reached at most32/47 across frozen fixtures; a fully reviewed current
33-context motion cohort is still pending. No raw pass alone closes V03.

Independent original-instruction replay v1311/v1314 matches B8F40, C92C0 and
1CC8D0 on actual context35 inputs:172,426,725 memory bytes per call, retained
register/FP state, zero differences, no substituted services/results. Copied
banks and replay binaries are removed. This is bounded call evidence, not
continuous trajectories or Xbox hardware proof. Machine355 still needs its
actual per-view PRT owner; verified cold BSS zero is not a retained incident.
Raw original/MCC SH audit v1331 confirms both have zero records for FloodZone
BSP0/1/2, seven atBSP3 and25 atBSP4. This is a caller-lifetime integration gap,
not missing MCC SH conversion. Native diagnostic v1341 passes the prior450212
ownership boundary and reaches a duplicate47524E binding inside the authored
camera-effect table. Current shared source already removes that duplicate;
v1343 completes both construction fixes and observes actual23670 arguments
and a real completed one-owner/two-node PRT cache. Actual admission still
excludes this unregistered, globally instantiated machine atZ790. A declared
isolated whole-builder call is component evidence only, never a view admission.
The v1343 private build/source is removed. Candidate v1346 passes the new
actual-MCC frustum/resource regression and existing machine presentation tests,
but its linked cohort is29/35: six checks reach the same writable47C1F8
reaction-name table through an obsolete immutable-scalar declaration. Read-only
debugger v1355 observes matching bits0600008D in the existing16-byte owner;
offline TU1.5 audit v1356 confirms all four names and writable .data flags7.
Both old aliases now borrow the complete owner, and the regression checks all
four writable entries. The v1346 private build/source is removed; no v1346 live
case was attempted or counted. The owned v1357 build now passes factory ownership,34/34 linked gates and
immutable export. Newly reached1EF260 is supplied by the existing shared
19-table branch provider; independent v1361 verifies all577 original readonly
bytes and its unsigned0..4 bound. Base v1359 core passes9/9: actual Warthog
camera320 frames,0.60rad look and481 exact LAN comparisons. Its scratch is gone.
Live v1358 still rejects machine355: the old presenter recomputes a node0 shadow
sphere instead of using the retained object+30/+3C bound. Revision v1357-r1
now consumes that actual bound from the authoritative full-handle WORLD-pose
snapshot for machine frustum and LOD selection. Its34 linked gates/export pass;
core v1365 passes9/9, but focused FloodZone v1364 still fails. Read-only
v1367 shows the real retained sphere enters the ordinary camera frustum during
vehicle look; frustum bounds alone cannot establish source admission. Candidate
v1369 reads actual full-generation membership from both original spatial pools,
transports sorted cluster IDs with each source root under protocol95, and gates
machine submission against the current portal frustums. Its36 linked checks and
immutable export pass. Focused FloodZone v1371 now passes:102 submitted
vehicle frames in both views,16.76 units of causal motion and185 exact LAN
comparisons. v1374 reviews20/50/85-percent recording samples: the Phantom is
clearly visible in both views throughout those samples; all3 decoded PNGs are
removed. This closes the context32 machine355 rejection at this boundary;
admitted machine PRT ownership remains a separate open path. Core v1372 passes9/9. All5 Cyclotron sampled frames visibly retain their
machine geometry; support ticks/carry are71/2.38624,72/2.38624,59/2.38638,
44/2.37445 and15/0.71099. Actual Warthog camera394 frames,0.60rad look and453
exact LAN comparisons pass. The26-case family v1373 is running. Its Banshee
camera passes but the first flight reaches only7.47 horizontal units against
the unchanged10-unit gate. Two unchanged v1378 repeats pass14.71/17.55 units;
the retained v1303 reference passes14.57. The original red trace stays retained.
Declared0.3-radian SDL departure look v1380 passes three times at15.84/15.52/
16.11 horizontal units and18.09/18.19/18.38 climb, without changing flight
thresholds or production physics. Consolidated v1385 qualifies26 selections:
25 unchanged-input family cases plus those three Banshee runs; this is not a
single all-green raw cohort. The observer remains inside the base and loses
the climbing Banshee behind its ceiling. New47-context v1381 runs on one
exact r1 binary; v1375 was superseded before running any live case. Existing teleporter/HUD/flash compatibility
v1376 passes54/54 with no skipped checks before any launcher replacement.
The old full47 census is not relabeled as current-candidate qualification. The expired v1357 private source/build are removed. The new
bounded v1369 source/build owner remains active during visible investigation;
per-case raw outputs retain the managed lifecycle.

**Verified storage:** v1304 removes1,443,106,816 observed allocated private
CMake bytes; net volume free change+398,135,296 bytes. v1322 retires3 failed
v1286 executables:291,561,472 allocated bytes removed, measured net free change0.
Source archive, shader/header dependencies and diagnostics remain. v1312
archives72 completed helpers from359,852 bytes into26,836 bytes and removes
standalone originals. v1345/v1347 retire superseded diagnostic executables:
84,283,392 and84,348,928 allocated bytes respectively; measured net free changes
are0 and+120,582,144 bytes. Exact source archives and all diagnostic reports
remain. Latest required v1343 probe/source retention is documented in its
native-probe/retention.json. All9 v1318 and12 v1342-b review PNGs are removed. v1350 archives43 completed
helpers from248,348 bytes to27,280 bytes; loose originals are removed. v1352
verifies v1346 private build/source removal:906,006,528 allocated build bytes
were observed before cleanup, with measured net free change-20,606,976 bytes
while packages and concurrent work changed the volume. This is an observed
removal amount, not a complete build peak or a claim of physical reclamation. Concurrent
filesystem activity means these allocation/removal observations are not summed
as physical free-space gains. v1377 retires9 superseded v1346/v1357/r1
executables:885,324,672 logical and885,350,400 allocated bytes removed, measured
net volume free change+65,986,560 bytes. Exact source archives, shader inputs,
reports, requested recordings, v1303 reference, current v1369 candidate and the
shared launcher-selected delivery remain preserved. v1389 removes22 redundant
port recordings after preserving current core proof, the original Banshee
negative, one old-input positive and one takeoff positive:135,434,367 logical /
135,487,488 allocated bytes, measured net volume free change0. All reports,
input traces, hashes and original reference evidence remain.

Latest device/contact continuation:

- [x] Current MCC Zanzibar switch opens the original gate to1.0 with zero
  source/native phase mismatches and904 exact LAN comparisons (v1394).
  The eight-stage source device script passes745 comparisons (v1386).
- [x] MCC Elongation/Colossus conveyor runtime backing and bounded visible
  neutral-player transport. v1395 authenticates the20-byte mutable lazy owner;
  v1401 authenticates31F728's44-byte readonly interface and both reached method
  slots. The compiled provider now has147 byte-verified callable prefixes.
  v1402 passes both belts:145/177 supported ticks,2.25427/3.83574 travel and
  183/182 exact LAN comparisons, with zero movement/jump commands. Its exact
  tested ELF matches immutable v1396-r1. Full player/solver trajectories remain
  open. Fresh core v1403 passes9/9 and fleet v1410 passes26/26.
- [x] Independent original TU1.5 whole-world call1CC8D0 on actual MCC
  conveyor captures (v1419):89,213,849 Elongation and89,418,649 Colossus
  memory bytes match, with register/FP comparisons. Live cases pass181/182
  exact LAN comparisons. Copied banks/replay binaries and scratch are removed.
  This is bounded call evidence; full trajectories/hardware acceptance stay open.
- [x] Current imported-MCC object-field continuation: scripted Colossus and
  Elongation crates pass (v1422); Midship platform, both Turf doors and both
  Gemini columns pass all force/motion/draw/LAN gates (v1426). Actual angular
  deliveries are252/172/247/1199/1204; exact LAN comparisons361/361/362/903/903.
  Independent original1DA750 calls match89.81/92.46/92.46/91.36/91.36MB of
  final memory plus CPU/FP state. Copies/replay executables and scratch are gone.
  Earlier five red v1422 angular receipts lacked member diagnostic flags and
  field selection; preserve their reports as harness failures. The wrapper now
  rejects that incomplete configuration before launching (v1427,3/3).
- [x] Bounded tall-wall contacts on imported MCC Zanzibar (v1444): Warthog
  and Ghost each stop under throttle for8 ticks, with peaks4.56353/5.10508,
  actual source cameras/look/exit and314/338 exact LAN comparisons. Both
  original1CC8D0 calls match116,067,509 final source bytes plus CPU/FP state.
  Copied banks/replay programs and scratch are removed. Headlong's Warthog
  approach still fails to reach its selected wall; the red recording remains.
- [ ] Quantitative vehicle wall/contact coverage. Alternate authored Warthog
  row50 and Ghost row52 cases v1429 still fail the stop-under-throttle gate;
  camera/entry/exit success alone does not close contact acceptance.
  Banshee passes the current
  wall case; Warthog/Ghost do not satisfy stop/contact gates. Their negative
  recordings remain, including an uncompleted Ghost camera-look gate. The
  independent ordinary driving/camera matrix remains explicitly bounded.
- [x] Superseded v1369 private source/build removed before the conveyor
  rebuild. v1370 verifies1,903,001,600 observed allocated bytes removed and
  measured net volume free change+870,912,000 bytes. Required exports,
  exact source archives, shader dependencies and current recordings remain.

This checklist is part of the goal's completion criteria and is updated after
material results. The goal tool remains `usageLimited`; its API cannot edit the
objective or resume that status. Full1:1 physics parity remains incomplete.

Player/lift core checkpoints:

- [x] Lockout walking, crouching and jumping over LAN: 182 exact same-tick
  host/client snapshot comparisons in v854. This is bounded movement acceptance,
  not all player physics. v941 restores this case with181 exact LAN comparisons.
- [x] Lockout gravity-lift case over LAN: 182 exact comparisons in v854.
  v941 restores this gate after the exact timing conversion fix.
  Other lifts, supports and their retail trajectories remain separate gates.

Player-versus-player scoped acceptance — October23,2026:

- [x] **24/24 native checks pass** on prepared imported MCC Colossus through
  the canonical TU1.5 match and physics world. Coverage includes blocking and
  pushing, head-on contact, crouching, adjacent passing, teammates/enemies,
  all four Spartan/Elite pairings, a supported stack and its unsupported
  gravity control. The existing source physics needed no gameplay changes.
- [x] Standing/crouching teammate boosts: **36 actual input-driven mounts and
  landings**,720 supported ticks across12 cases, jumping off, lower-player
  movement and final support release. Grounding must name the actual lower
  player's full source handle. Four boost cases repeat over real UDP; the
  complete cohort compares **1,831 exact same-tick LAN player snapshots**.
  Network inputs run at30Hz, respecting the128-new-assemblies/second receiver
  bound. The test never advances physics to make a network assertion pass.
- [ ] The15 Deserted jump/boost sites remain separate map-specific gates for
  polished GLB SHA`55d860b81f825bd36f18e9723fa2d1ad396ed17739d11ebf5c1a07fab0f697b2`.
  Colossus acceptance does not qualify that map's approaches, trajectories,
  headroom, landings or escapes. Its exact compiled candidate is owned by the
  map-authoring chat. The older local Dust2 package is a different source.

[Acceptance report and exact pins](../build/player-collision-tu15/acceptance.json),
[raw bounded case evidence](../build/player-collision-tu15/complete/evidence/Testing/Temporary/LastTest.log),
[regression source](../tests/player_collision_tu15_tests.cpp),
[CTest registration](../cmake/PlayerCollisionTu15.cmake).
These are bounded native component/match/network results; the other broad
physics groups and physical-Xbox parity remain open. No launcher-selected
binary was replaced or requalified by this test-only change. The private
build and case scratch are removed; no large test artifacts are retained.

Reproduce from this repository with the prepared MCC map available:

```bash
python3 tools/run_tests.py --target player_collision_tu15_tests \
  --filter '^native_mcc_player_collision_' --test-jobs 1 \
  --cmake-arg="-DHALO2_MCC_PLAYER_COLLISION_MAP=$PWD/assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11/colossus.map" \
  --retain-report Testing/Temporary/LastTest.log \
  --output build/player-collision-tu15/fresh
```

Player external-force acceptance — October23,2026:

- **22/22 external-force cases pass**, plus the player head-on baseline,
  on a pinned native TU1.5 source snapshot with prepared MCC Colossus and
  Coagulation. `tests/player_external_forces_tu15_tests.cpp` covers nearby/distant frag
  blasts, nearby/direct rockets, Warthog bumps, longer-run-up splatters,
  missed strikes and parked-vehicle controls for Spartan and Elite targets,
  including six real UDP cases with **1,506 exact same-tick snapshots**.
  Actual source-born Spartan/Elite definitions, surviving knockback, immediate
  fatal contacts and single kill/death attribution are required.
- Scene placement uses original B95D0 and authored BSP/seat inputs. Ordinary
  weapon, grenade, pickup, driving and look commands supply the stimuli.
  Read-only observations require actual D91D0 radius calls, source vitality
  and motion, original fatal transitions/kill credit where applicable, and
  unchanged distant controls. LAN checks compare same-tick player positions,
  velocities, vitality, identities/statistics, corpse state and emitted WORLD
  node banks. Biped knockback gates measure actual velocity and displacement.
- The initial LAN direct-rocket fixture missed because it treated raw look
  input as an absolute heading after the native join frame. A repeated focused
  red run observed requested yaw1.571 but actual source yaw1.045. Ordinary look
  deltas toward actual source aim fix the fixture; its focused retry and full
  strengthened23-test cohort pass. No gameplay physics/damage changes were made.
- [Acceptance and source hashes](../build/player-external-forces-tu15/acceptance.json),
  [full CMake/CTest result](../build/player-external-forces-tu15/complete/report.json)
  and [later shared-source drift](../build/player-external-forces-tu15/post-freeze-source-drift.json)
  retain the exact scope. Vehicle BV-agent/objective work that landed during
  this source-frozen run remains outside its qualification. Cleanup is verified
  for both disposable source/build and probe owners. No large test artifacts
  are retained.
- This scope does not close all vehicle classes, occupant consequences,
  obstruction/material/falloff families, original instruction differentials,
  visible/audio acceptance, corpse lifetime or launcher delivery. P03, V04,
  E02/E04 and the broader destruction ledger remain open.

Player collision, vehicle-impact and blast acceptance — October24,2026:

- **106/106 CTests pass with no skips** on the current shared source: 73 external-force
  cases, all24 player-collision cases, five bounded original component suites,
  the original vehicle BV-agent case and three compiled-symbol/namespace controls.
  Shared physics, multiplayer, objective, network, cache and renderer changes are
  included in the effective source manifest. The final renderer catch-up builds
  successfully and leaves all ten measured executables byte-identical; its
  head-on and actual UDP vehicle-impact smoke cases also pass.
- External-force cases cover Warthog, Gauss Warthog, Ghost, Spectre, Scorpion,
  Wraith and Banshee against Spartan and Elite targets. The actual full-salt
  definition and authored model variant are recorded; Gauss cases require the
  actual named `gauss` variant. Near/middle/far frag, near/far plasma and rockets,
  direct rockets, actual BSP-wall obstruction, missed strikes and ordinary
  neutral controls exercise genuine source vitality and physical motion.
  Aircraft neutral controls preserve original coasting. Scorpion/Wraith/Banshee
  contact gates allow genuine non-damaging pushes while requiring measured
  target velocity and displacement.
- Nine real UDP force cases pass **2,775 exact same-tick snapshots**, **818,571
  WORLD-node comparisons** and **1,074 corpse-pose comparisons**. Declared fatal
  rocket/splatter cases require immediate source death, single kill/death credit,
  actual corpse state and a new salted source respawn generation.
- **18 independent live-call comparisons pass** against English OG Xbox TU1.5
  (`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`):
  eight post-contact physics calls covering all seven configurations plus the
  Warthog splatter, four post-blast physics calls and six complete radius calls.
  They compare **1,825,436,054 captured bytes**, GP/SSE/x87 state and FP controls/status
  without supplying damage, velocity, collision, RNG, solver or gameplay callee results.
  The i386 harness records its fixed host IF bit separately. Physics-frame
  comparisons use unchanged original instructions. Radius comparisons explicitly
  bind authenticated MCC StringID operands, actual read-only bitmap views and,
  where reached, the original sound leaf's authenticated codec/encoded-byte
  operands and the recorded external UTC input. The sound leaf executes all67
  original instructions; no duration or FPU result is supplied. Captures reject
  newly committed memory outside their compared regions.
- The five independently regenerated component suites pass **426 original
  comparisons: 289 completed calls and137 bounded original owner prefixes**.
  These are separate tests; historical full-corpus tests are not repointed.
  Nested-entry capture regression passes. The original fatal replay goes red
  when its sound input view is omitted, and a wrong authored encoded extent is
  rejected before original execution. These checks retain a real failure signal.
- [Acceptance and source hashes](../build/player-force-current-tu15/acceptance.json),
  [full CMake/CTest result](../build/player-force-current-tu15/accepted-v6/report.json),
  [live original comparisons](../build/player-force-current-tu15/original-final-v6/summary.json),
  [source catch-up proof](../build/player-force-current-tu15/renderer-source-catchup.json)
  and [cleanup receipt](../build/player-force-current-tu15/cleanup.json) retain the
  exact boundary. Disposable builds, source copies and raw banks are removed.
  The five required original component inputs are retained losslessly under
  `build/player-force-current-tu15/original-reference-inputs/` (**664,280 bytes**
  compressed total); no rebuildable binaries, raw live banks or frames are retained.
- This closes the scoped player collision/impact/blast acceptance above. The
  broader P03/V04/E02/E04 and destruction D01–D13 matrices still require their
  other authored definitions, material/directional families, occupant/destruction
  transitions, visible/audio and physical-Xbox evidence. Launcher-selected
  runtimes are outside this source validation.

Core interaction groups — bounded current acceptance passes:

- [x] C01 — Warthog approach, boarding, driving, steering, actual submitted
  TU1.5 vehicle camera, deliberate look, exit and camera handback. v1125 passes
  with265 exact seated-camera frames,0.60 radians of look and41 frames after
  exit. Retail framing and quantitative collision behavior remain under V04/I03.
- [x] C02 — Grenade/prop interaction: v1124 passes the strengthened gate
  with2 real point impulses on the observed target generation. Linear velocity
  changes by up to3.9999998 and angular velocity by11.984085; actual body travel
  is0.46190089, followed by retirement and recreation with a living observer.
  Both multiplayer views render the actual grenade/explosion effects.
  Full quantitative TU1.5 object/solver coverage remains under O01/E02/E04.
- [x] C03 — Cyclotron upper support, body0: v854 records71 actual moving
  support ticks,2.386 units of passive carry and242 exact LAN comparisons.
- [x] C04 — Cyclotron lower support, declared PHMO row1 at phase0.625:
  v908 passes72 actual moving-support ticks across body handoffs,2.386 units
  of passive carry and242 exact LAN comparisons. The old120-tick test exit
  and genuine game-engine retirement binding are fixed; the original input
  minimum and actual-controller carry requirements remain intact.
- [x] C05 — Cyclotron lower support, body1 at phase0: v1124 restores ordinary
  boarding/carry after the LAN first-admission facing correction:58 real moving
  support ticks,2.3862369 units of carry and243 exact LAN comparisons.
- [x] C06 — Cyclotron upper support, body2: v854 records44 actual moving
  support ticks,2.374 units of carry and245 exact LAN comparisons.
- [x] C07 — Cyclotron exit support, body3: v854 records15 actual moving
  support ticks,0.711 units of carry and239 exact LAN comparisons.

All five Cyclotron entries qualified in v1074. v1097 exposed the initial
look-packet admission regression; its negative trace remains. v1124 restores
phase0 using actual source input/controller/carry proof. Full v1125 passes
all five on the same corrected immutable runtime. Fresh v1218 again passes
all five on v1216; v1242 passes all five again on v1240, and v1268 on v1265.

Broader vehicle coverage — 2 groups open:

- [x] V01 — Current MCC driver/controller acceptance for all eight existing
  driver selections and both fixed turrets. v1131 passes10/10 with actual
  submitted source cameras, deliberate look, entry/fire or drive and exit.
  Fresh v1250 passes all10 on qualified/wired v1240, with actual source
  cameras, deliberate look and exit; v1274 again passes10/10 on v1265.
  The human turret has colored contrails in both views; its genuine frame phase
  and packed-color backing are integrated. Current wired v1396-r2 is covered by
  fresh v1435's26 selections: all10 driver/turret selections pass at the
  explicitly recorded input boundaries. The original flat-departure Banshee
  red trace remains preserved; quantitative V04 stays open.
- [x] V02 — All16 seat/camera cases: passenger/gunner entry and exit, moving
  occupied seats, original cameras and the four Scorpion rider rejections.
  v1128 passes all16 with actual source cameras, original entry/exit,
  six moving occupied vehicles and four genuine rider rejections. The original
  complete game-engine method records are integrated and linked-tested.
  v1249 passes16/16 on v1240. v1273 exposes2 Spectre null-HUD preparation
  regressions; repaired v1308 passes all16 on qualified v1303. Current v1373
  passes all16 seats; v1385 verifies the same production source/native binary
  in the previous v1369-r1 package. Fresh v1410 passes all16 on v1396-r1; v1435 repeats all16 on v1396-r2.
- [ ] V03 — All47 authored campaign class-3 contexts: correct activation/BSP,
  ordinary controls, bodies, motion and visible multiplayer observation.
  Current v1381 records34/47 raw passes and13 unavailable observer starts on
  one exact v1369-r1 binary. Context32 passes actual spatial/frustum admission.
  Latest v1418 samples show33 identifiable in both views with partial late
  occlusion;42 still loses the first readable view overhead. Full current
  motion review stays open; raw passes alone do not close this group.
- [ ] V04 — Applicable vehicle suspension/contact forces, torque/gears,
  hover/flight, braking, collisions, impulses, rollover and destruction, with
  quantitative TU1.5 reference comparisons.

V03 concrete integration checkpoints (the broad group stays open):

- [x] Retained full-generation machine bounds and actual spatial memberships
  reach each current LAN view under protocol95. v1371 restores FloodZone32:
  102 vehicle frames/view,16.76 causal travel,185 exact LAN comparisons; v1374
  sampled recording review is clear in both views. All5 Cyclotron supports
  remain visibly rendered and carry correctly in v1372. This does not close
  admitted machine per-view PRT ownership below.

- [x] All5 exact physical-method records, the reached450212 selector operand
  and the3 attached trigger contexts are integrated with original bodies.
- [x] Introduction luminous/gauge/current material closure and exact additive
  sound contexts are converted from authenticated TU1.5 inputs.
- [x] Retained particle selector ownership is repaired; High Charity33/34/36–39
  pass the strengthened raw live gate. Current33 late occlusion stays open.
- [x] Actual submitted camera projection, moving geometry checks, negative
  recording review and finite runtime deadlines are enforced.
- [x] Pruned cinematic weapon HUD residency passes introduction0–2 in v1269.
- [x] Ordinary two-observer SDL look with continuing neutral world ticks:
  v1307/v1325 pass the focused six-context raw cohorts, including35/31/42.
  Occlusion remains separate; actual source cameras are unchanged.
- [ ] Machine355 PRT retained cache-slot lifetime: original22830 preserves
  incident+92..103 across views. v1280 confirms empty SH but live nodes[0,1].
  v1343 observes a real completed two-node cache from the whole native builder,
  while actual admission still excludes the globally instantiated machine.
  The v1346 off-screen/visible-resource regression passes; admitted per-view
  cache ownership remains open. Actual v1371 resolves32's unregistered-owner
  rejection; it does not manufacture an admitted PRT cache slot.
- [ ] Correct original activation/BSP context for13 unqualified observer starts.
- [x] Attempt all47 current contexts in v1330/v1336, consolidated in v1337;
  raw33 passes do not close their separate visual or quantitative gates.

Broader player coverage — 3 groups:

- [ ] P01 — Air control, jump rearm/fatigue, slopes, stairs/steps, ceilings,
  ledges and grounded/airborne contact transitions.
  v1448 now declares a held180-tick/released10-tick/repressed10-tick live
  jump fixture at the original start, with observed source launch/landing
  counts and a post-second-launch original world call. This case is queued;
  do not count it as passed or as fatigue/trajectory coverage.
- [ ] P02 — Other authored movement modes and moving/rotating supports,
  conveyors and carrier transitions beyond the current Lockout lift case.
- [ ] P03 — Biped activation, animation/root-motion physics, external impulses,
  death transitions, ragdolls and corpse lifetime.

Broader object/device coverage — 3 groups:

- [ ] O01 — Dynamic crates/props, dropped weapons/equipment and applicable
  projectile/grenade contact, bounce, friction and impulse behavior.
- [ ] O02 — Parent/child and node attachments, detachments, recreation,
  breakage/destruction and physical-object retirement.
- [ ] O03 — Other authored machines/doors, controls, occupancy queries,
  lifts/conveyors and force-field interactions.

Shared physics completion — 4 groups:

- [ ] E01 — Full composed broadphase, terrain/MOPP and narrowphase/contact
  lifecycle, collision filtering, friction/restitution and game callbacks.
- [ ] E02 — Constraints/Jacobians, warm starting, iterative solving, forces,
  torques/point impulses and linear/angular integration/readback.
- [ ] E03 — World/body/shape/allocator lifetime, islands, wake/sleep,
  deferred operations and retirement across applicable live paths.
- [ ] E04 — Representative composed original-versus-native traces covering
  continuous contacts, support transitions, forces and trajectories; bounded
  component comparisons alone do not establish 1:1 runtime behavior.

Integration and delivery — 3 groups:

- [ ] I01 — Cross-map split-screen/LAN authority, replication, joining and
  world restoration while these interactions run.
  Bounded occupied restore v1433 passes for a driving Warthog and occupied
  Spectre: exact physics bank/players survive and471/623 LAN states continue.
  Both post-restore original world calls match89,795,481 final memory bytes.
  DXT3 checkpoint eligibility now uses the already proved constructor domain;
  the actual-MCC regression fails before the fix and passes afterward. Other
  maps, join/leave phases and authority transitions remain required.
- [x] I02 — Wire the qualified MCC physics runtime into the dedicated physics
  launcher and verify ordinary gameplay, vehicles and actual vehicle cameras.
  v1303 passes33 linked gates, core9/9, seats16/16, drivers10/10, ordinary v1315
  and actual wired v1317 startup with MCC-only opened files. Its private trees
  are removed. Current v1396-r2 wiring and ordinary-launcher acceptance are recorded above;
  previous deliveries remain preserved.
- [ ] I03 — Reconcile every applicable player/vehicle/object path against the
  coverage inventory, close remaining live ownership/conversion gaps and
  retain visible multiplayer proof before declaring full TU1.5 parity.

Evidence for current qualification and open regressions:

- [Qualified physics core9/9, v1306](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-visible-v1306/matrix.json).
- [Strict prop impulses and phase0, v1124](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-focused-v1124/matrix.json).
- [Seats16/16 including repaired Spectre cases, v1308](../analysis/multiplayer/physics-live-tu15-20261012/mcc-seat-acceptance-v1308/report.json).
- [Drivers10/10, v1313](../analysis/multiplayer/physics-live-tu15-20261012/mcc-drivers-v1313/matrix.json).
- [Current full47 campaign census; raw gates only](../analysis/multiplayer/physics-live-tu15-20261012/mcc-campaign-current47-v1337/report.json).
- [Current sampled motion review;33 occlusion retained](../analysis/multiplayer/physics-live-tu15-20261012/mcc-campaign-motion-review-v1342-b/assessment.json).
- [Actual wired physics launcher, v1317](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-launcher-startup-v1317/report.json).

Latest source repairs awaiting full live qualification:

- v1143 independently re-exports34 original introduction template/pass/program
  records and authenticates English TU1.5 offline. v1145 reproduces the actual
  MCC material rejection; v1146 passes the production projection plus8 negative
  controls. v1147's actual Cache material check also passes. MCC constants,
  bitmaps, dimensions and authored IDs remain intact; no MCC shader bytecode is
  interpreted as Xbox microcode. The13,385-byte regression fixture is retained.
- v1147 completes15/16 gates; the new material moves the globals clone and
  exposes the sound namespace publication guard. Read-only v1160 compares both
  actual Cache publications: globals move1,552 bytes, while all54 complete class
  rows and their full typed route remain unchanged except that relocation.
  The additive context is exactly pinned; all existing authentication remains.
- Headless v1159 reaches original F4D1A's missing56ECD0 flag during Phantom
  activation. TU1.5 v1162 authenticates the loader-zero writable56ECCC/56ECD0
  pair and original0.1 operand46A178. Native backing retains their cold storage;
  the original initializer computes its scalar from the actual tick-rate owner.
  It supplies no computed physics result. Native ownership regression is added.
- v1163 stops before tests because its frozen renderer catches duplicate Vulkan
  declarations during concurrent work. Current source already reuses those
  declarations. v1171 adds a current generated-catalog syntax preflight before
  compiling the original closure. Linked and live tests remain mandatory.

Storage is measured separately from qualification. v1169 retires855,427,984
logical bytes of superseded v1073/v1095/failed-v1147 programs and duplicate
shaders (856,104,960 allocated file bytes). Net volume free space rises
266,211,328 bytes during concurrent work. Current launcher v1123, active v1171,
all exact source archives/pins, failures and required proof remain protected.
v1170 archives40 completed helpers:188,841 source bytes become23,972 bytes;
archive SHA25658662b107bf75180a3c22fe779f84c1455df3a993c0a2c4185a52a5b880adf00.
Net volume change for that helper cleanup is0. Their scratch roots are gone.

<details>
<summary>Earlier evidence and cleanup receipts</summary>

These are earlier checkpoints. The current qualification and checklist above
control the completion status; historical passes do not certify wider parity.


Status checked October 20, 2026. English OG Xbox TU1.5 is the behavioral
authority; the prepared imported MCC cache supplies live runtime content.
TU1.1 is outside this goal.

This is the maintained acceptance checklist for the physics goal. Refresh its
status after every completed validation cohort or material blocker change;
record source/build provenance, visible MCC multiplayer results and scratch
cleanup. The parent goal remains incomplete while required groups are open.

Vehicle-camera implementation/reference work is retained in
[the camera audit](VEHICLE_CAMERA_TU15_20261015.md) and
[authored coverage](VEHICLE_CAMERA_TU15_COVERAGE_20261015.md), from the chat
“Match TU1.5 vehicle cameras.” Reuse those source owners and exact submitted
camera/input-response checks; revalidate current MCC runtime integration.

Frozen v1123 passes14/14 linked gates; private source/build trees are removed.
Focused v1124 **restores C05 phase0 boarding** after the initial LAN admission
packet fix. Ordinary approach/boarding gives58 real moving-support ticks,
2.3862369 units of passive carry and243 exact LAN comparisons. No contact or
solved support is supplied. V01 also closes: v1103 passes10/10 driver/turret
selections with actual submitted cameras. There are **14 open goal completion
groups**. C02 also closes in v1124:2 verified target impulses change linear/angular
velocity, followed by0.46190089 units of travel, retirement and recreation.
The full current **v1125 core passes9/9**, including all five Cyclotron entries.

Frozen **v1095 passes14/14 linked gates**, including paired original/MCC PRT
streams and the source frame phase. Private source/build trees are gone;
v1099 observes1,128,525,824 allocated private-build bytes before cleanup and
300,376,064 bytes net volume increase during concurrent work. v1100 passes
all16 seats again. The dedicated **Halo2-Physics-TU15 launcher selects qualified
MCC v1073**; actual wired startup v1082 passes120 frames/300 ticks/two players
with its MCC-only opened-file audit. The newer package remains gated on its
complete current core and ordinary startup. I02 remains a bounded delivery gate.

**C01 vehicle-camera acceptance passes** with497 submitted exact-source seated
camera frames,0.5999998 radians of deliberate camera look,32.73806 units of
vehicle travel and1.199685 radians of steering. Original exit completes with
24 source-transition frames,17 ordinary post-exit frames and41 total camera
frames after exit. The actual Warthog third-person camera is visible in the
retained recording; no substitute camera or weakened assertion is used.
The original0.08 look scalar, exact461998 physical query zero vector and
retained MCC emblem/DXT3/cubemap resources are integrated.

The finite grenade-effect lifetime fix passes actual MCC multiplayer: original
detonation,36 grenade draws and817 explosion-particle submissions in each of
two views in v988, then passes again in v999. Source identity/world locations
are retained before original first-update retirement; original execution and
return remain intact. This is bounded basic detonation acceptance. Stronger
Lockout blast/prop/recreation and quantitative impulse behavior remain required.

**C05 phase0 passes**: v988 records59 real moving-support ticks,2.3863838 units
of passive carry,242 exact LAN comparisons,11 paused grounded ticks and178
neutral authority ticks. v999 passes again. Ordinary boarding uses a surveyed
actual BSP floor and authored node2; no post-start position, contact, velocity
or solved support is supplied. The original carry/relative-error checks pass.
Original/MCC BSP and PHMO geometry are authenticated in v963.

The strong prop mode1 path now retains original44A9E4=0 and file-initialized
writable47C1F0=StringID0600008B, proved by v1007/v1008 and tested in v998.
The earlier two mode1 NONE sentinels remain distinct. Strong prop v1004 must
qualify the resulting real blast impulses, body movement and recreation.

The completed broader current cohorts remain fully counted:

- **v1077 passes all16 seat cases** with uninterrupted visible mirrors:
  six stationary passenger/gunner seats, six moving occupied seats, and all
  four Scorpion rider rejections. Moving occupied travel spans4.65–6.67 units
  with actual source cameras, exit/handback and genuine cooperative unit birth.
  The full original engine method prefixes fix the reached468C50 callback.
  **V02 is closed** for this current acceptance matrix.
- v1024 attempts all10 driver/turret selections; **9 qualify**: all eight
  drivable selections and the Covenant fixed turret, with actual source cameras.
  The human fixed turret fires13 original projectiles at800 units/second and
  publishes genuine contrail points, but both-view contrail drawing remains
  red. SourceContrailScene explicitly disables MCC; its snapshot fallback also
  tries the legacy executable loader. The compiled original vertex program and
  real MCC tag/bitmap inputs must be connected without accepting MCC shader
  bytecode as Xbox microcode or weakening the visibility assertion.
- Strong prop v1023 reaches original11704E's missing readonly44AA48 scalar;
  real blast impulses, body movement and recreation remain required.
- v1027 attempts all47 current MCC campaign class3 contexts; **0 qualify** and
  all47 mirror transports are verified. Failures comprise13 missing clear
  initial observer starts,19 unsupported source PRT declarations,3 luminous
  plasma shader inputs,5 reached physical interface41D074 slots, and7 attached
  effect/light/particle inputs. These are failed attempts, not certified physics.

Current source fixes and original provenance:

- v1028's complete27-vector physical query directions and reached CRT double,
  v1030/v1035 engine/physical callbacks, v1034 gauss/multiplayer namespace
  bridge, and v1032/v1036 complete receiver request/warning flag all build and
  pass current live paths in v1017–v1026. Authored MCC IDs and tag bytes stay
  intact. The namespace lifetime/admission regression is included.
- v1039 authenticates all four registered engine method prefixes,51 exact
  original code targets per record, before actual padding/next owner/string.
  The native world and independent original-byte regression now include them;
  live moving-seat revalidation follows the next coherent build.
- v1040 authenticates44AA48=3F2AACDA and41D074=304B90 at the actual reached
  instructions. Those original immutable operands are wired with regression
  checks; no callback implementation or successful return is substituted.

Keep the original source camera, actual controller, body/impulse, LAN and
both-view visibility assertions intact. Full TU1.5 physics parity is incomplete.

Storage: v852's private build is verified removed. Retirement v859 removes
278,282,320 bytes of superseded v819 programs and identical shaders while
preserving exact source, reports, recordings, current v842 and all launcher
runtimes. The immediate filesystem free-space increase is14,131,200 bytes
during concurrent compilation. The retirement's private scratch is removed.
Retirement v869 additionally removes278,575,544 bytes of superseded v842
programs and identical shaders, preserving their source/pins and all recorded
results. Its measured filesystem free-space increase is207,491,072 bytes.
Candidate v862's private source/build trees are verified removed in v865;
410,865,664 allocated private-build bytes were observed before removal. Volume
free space changed by-139,223,040 bytes during concurrent writes, so that
observation is not presented as net reclaimed disk space.
Retirement v896 removes another565,475,664 bytes of superseded v853/v862
programs and identical shaders; the immediate volume free-space delta is0.
Their exact source archives, pins, results and requested videos remain.
Completed one-off helpers are retired into the12,340-byte v884 evidence archive.
Another37 completed helpers are preserved in the15,360-byte v938 archive, then
removed from the working tree. Production regression sources remain.

Retirement v937 removes282,781,632 bytes of obsolete v873 executables and
identical shaders. Its exact source archive, hashes and results remain. The
immediate volume free-space delta is-16,764,928 bytes during concurrent builds;
removed logical/allocated bytes are not reported as net physical reclamation.

Capture retirement v948 removes11 redundant successful support recordings
(46,345,143 logical bytes;46,362,624 allocated bytes), keeping the latest v918
recordings, the first phase0.625 success and every failing capture/report/hash.
Its immediate filesystem free-space delta is0 during concurrent work.

Retirement v964 removes569,852,216 logical bytes of superseded v895/v917
executables and identical shaders (570,302,464 allocated file bytes). Current
v940, pending v955 and every exact source archive/hash/report remain. The
immediate volume free-space increase is10,084,352 bytes during concurrent work.

Retirement v995 removes573,343,744 logical bytes of superseded v940/v955
executables and duplicate shaders (573,796,352 allocated file bytes), preserving
source archives/pins, every result and requested recording. Current v970,
active v987 and all launcher-selected runtimes remain protected. The immediate
volume free-space increase is58,765,312 bytes during concurrent compilation.
Completed63 one-off helpers are verified in v996's25,536-byte archive, then
removed (303,897 source bytes). Both retirement scratch directories are gone.

Retirement v1009 removes286,742,536 logical bytes of superseded v970 programs
and shaders. v1016 removes286,746,976 more from superseded v987 (net volume
change-80,752,640 bytes during other work). v1010 removes12 duplicate successful
support videos totaling50,643,900 bytes, retaining current v988 recordings and
every failure/report/hash. Completed helper archives v996/v1013 retain the
small reproduction sources. All completed v999/v1000/v1002–v1005/v1011/v1012/
v1015 scratch roots are verified gone. Explicitly retained broader proof media:
v1002=7,541,083 bytes, v1005=40,730,386 bytes, v1015=3,417,137 bytes. Current
v998 runtime/source package and original/reference assets remain protected.

Retirement v1054 removes286,749,664 bytes of superseded v998 programs and
duplicate shaders (286,973,952 allocated file bytes); immediate filesystem
free-space delta is0 during concurrent writes. The launcher-selected v1017,
exact source archives/pins, every result and required recording remain.
v1055 archives48 completed one-off helpers into21,424 bytes and removes239,794
source bytes from the working tree. Both owned cleanup scratch roots are gone.

Candidate v1041 fails compilation after concurrent debug-ammo removal leaves
four references to its deleted local selector. The ordinary adapter now states
its existing no-override policy; the original live world continues to own its
ammunition decisions. A direct syntax check passes. No v1041 gameplay result
is accepted and all aborted cohort scratch is removed. Cleanup v1044 observes
379,428,864 allocated private-build bytes before removal, with3,366,912 bytes
net volume increase during concurrent work.

Replacement v1059 reaches the renderer, then fails on another orphaned
reference from the concurrent debug-UI removal: loading checks a deleted
graphics-owner member. That dead check is removed without restoring the
removed feature. Its private source/build trees are gone; v1062 observes
500,834,304 allocated private-build bytes before removal and64,417,792 bytes
net volume increase during concurrent work. The dependent gameplay cohorts
are aborted; no unrun game is counted as acceptance.

Completed v1073/v1074/v1077 qualify all12 linked gates, all9 core cases and
all16 seats. Focused v1084 advances both remaining cases: the human turret now
submits265 contrail draws per view, but each has zero colored area; the stronger
prop run reaches the genuine packed-color scale48C770 behind an MMX exception
which masked the original missing operand. Neither case is accepted.

Read-only GDB v1091/v1094 proves actual H-turret points still have state=-1;
its custom authority loop omitted the original frame phase already called by
the ordinary dedicated runtime. The fixture now calls the same complete
advance_effect_frame owner after each accepted tick, before publication.
Original17EAD0 initializes/advances those points; no width/color/state is supplied.
v1092/v1093 authenticates48C770=46800000 in writable initialized .data.

Current v1095's checked PRT adapter passes the paired live-Cache regression:
15 actual model banks/six map pairs, with exact original-expanded cluster values,
weight bytes and five-short LOD offsets. v1090 independently compares all98,719
raw vertices. The old unrun full47 cohorts v1083/v1106 are cancelled in favor of
the corrected camera/input build; no skipped context is accepted.

Focused **v1096 passes human-turret and prop/recreation runs**. The human turret
has144 exact source-camera frames,220 exact LAN comparisons and9 colored-area
contrail draws per view. The prop run has181 exact LAN comparisons,839 explosion
particle draws per view,0.4619 units of target travel, original retirement and
48 living/aimed recreation frames. Its legacy impulse-verification field was a
hardcoded false placeholder: actual source records contain two nonzero impulses
on the selected target, with linear changes2.25/4.0 and angular changes11.98/2.45.
Read-only v1108 verifies those original observations and rejects four negative
controls. The source fixture now requires actual target/force/linear/angular/
travel/retirement checks; **C02 closes after the fresh compiled strict v1124 run**.

Current **v1109** includes the admission-packet repair and stronger prop gate.
v1110 first tests phase0 boarding, human turret and strict props, followed by
v1111–v1120 full core, ordinary,16-seat,10-driver, launcher and47-context cohorts.
Keep original camera, body/impulse, UDP and both-view visibility requirements.

Retirement v1107 removes286,838,216 bytes of superseded v1017 programs and
identical shaders (287,059,968 allocated file bytes). Its immediate net volume
change is-99,307,520 bytes during other work; source archives/pins and all
required evidence remain, with current v1073/v1095 runtimes protected.

Focused v1124 restores C05 and qualifies the strict C02 impulse/movement/
retirement gate; the complete v1125 9-case core passes9/9. The complete
goal has **14 open completion groups**, all broader groups below. A group covers many
interactions, source routines and scenarios;14 is not a count of coding fixes
or unique interactions. Unchecked groups can have ported code and bounded
reference results while still lacking sufficient current MCC acceptance.
Distinct required interactions have not been fully audited.

The wider matrix includes **16 seat/camera cases** (all attempted and qualified in v1077) and **47 campaign class-3 contexts** (all attempted in v1027,
0 qualified). Their MCC identities/placements are resolved; failed runtime or
observer/render prerequisites do not establish physics acceptance. These counts overlap other groups and must not be added together
as a count of unique missing interactions.

Evidence and test storage:

- Core current MCC matrix:
  [v1074](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-visible-v1074/matrix.json).
- Seat/camera suite:
  [v1077](../analysis/multiplayer/physics-live-tu15-20261012/mcc-seat-acceptance-v1077/report.json).
- Authored47-context inventory:
  [v818](../analysis/multiplayer/physics-live-tu15-20261012/mcc-class3-semantic-contexts-v818/report.json).
- Ordinary current MCC startup:
  [Lockout v1000](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-startup-v1000/report.json),
  [Coagulation v1022](../analysis/multiplayer/physics-live-tu15-20261012/mcc-physics-coagulation-startup-v1022/report.json).
- Required source scope and completion rules:
  [physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md).

All nine core v843 attempts verify an uninterrupted live desktop mirror. The
private scratch for v843/v844/v846/v847 is verified removed. Keep bounded
reports, source hashes and explicitly retained visible evidence; use
`tools/run_tests.py` / `tools/run_test.py` for further validation and the
prepared MCC cache for every live runtime test.

Latest completed cohort: v1103 passes all10 drivers/turrets and its scratch is
verified gone. New v1123 builds the admission repair and strict prop gate after
v1109 stopped on a missing shader helper declaration in its frozen source.
Current shared source already includes that declaration and v1135 syntax
validation passes. v1109's private source/build trees are gone; v1113 observes
307,597,312 allocated build bytes removed and59,719,680 bytes net free-space
increase. Dependent v1110–v1120 never ran gameplay and receive no acceptance.
Replacement v1124–v1134 covers focused/core/ordinary/seats/drivers/launcher and
all47 contexts. Their results must qualify before replacing launcher v1073.

Further completed storage cleanup: v1138 removes24 superseded successful
scenario recordings/sample images,260,070,864 logical bytes and260,169,728
allocated file bytes. Latest v1100 seats/v1103 drivers, all reports and failures,
shown Gauss and human-turret proof remain pinned. v1139 archives18 completed or
aborted helpers (93,204 source bytes) into14,524 bytes with SHA256
0d599ac8528123f532a4408ac3a5bf50a360f6ba4e5043d320f2afb042a2e3ca.
Both immediate net volume free-space changes are0 during concurrent work.
Their scratch, v1135 syntax scratch and v1137 single-frame inspection scratch
are verified gone. No extracted inspection images or diagnostic binaries remain.
Read-only v1136 confirms the introduction's luminous plasma material lacks a
retained/native shader instance: it has zero admitted Xbox passes while the
same named instances in Cairo/Coagulation admit60 passes. This renderer blocker
is separate from class3 physics and the already paired-validated PRT adapter.

Completed v1123 build:14/14 linked gates pass and private source/build trees
are verified gone. v1127 observes1,134,292,992 allocated private-build bytes
before automatic removal; immediate net filesystem free-space increase is
135,581,696 bytes. Runtime v1123 retains only explicitly required ELFs,
shader resources, exact source archive and provenance for live validation.

Focused v1124 passes3/3: Cyclotron phase0, actual human-turret camera/colored
contrails, and strict Lockout prop blast/recreation. C02 records
`dynamic_prop_impulses_verified=true`,2 verified target impulses,
0.46190088987350464 actual travel and original retirement; authority/client
adoption matches for182 states. Its outer scratch is verified gone. The actual
raw impulse inputs and before/after motion bytes remain in bounded JSON.

Current v1125 core passes9/9 on frozen v1123, with original vehicle camera,
ordinary boarding and strict assertions intact. v1126–v1133 continue ordinary
startup, seats, drivers and qualified launcher delivery.

The introduction's missing MCC luminous material now has a narrowly checked
conversion in `src/mcc_native_shaders.cpp`, connected to actual Cache startup.
v1143 independently re-exports all34 template/pass/vertex-program records from
the original introduction map (SHA256
5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28)
and authenticates English TU1.5 offline. Native template bytes/typed references
match exactly. v1145 reproduces rejection on actual captured MCC input; v1146
passes the corrected production projector, preserving MCC parameter banks and
passing8 negative controls. A13,385-byte independent regression fixture is
retained in `tests/fixtures/mcc_intro_luminous_tu15.json`. No source texture,
geometry, gameplay tag or physical output is substituted.

v1147 now builds16 gates, adding actual Cache luminous admission to the14
existing gates. After the current driver/launcher suite, v1148 tests the actual
introduction scene; **v1149 then attempts all47 campaign contexts before** the
next core v1150. v1134 was superseded before any context to include this checked
fix; its scratch is verified gone and no unrun case is accepted. Component
projection success does not close V03 or establish any campaign physics pass.

</details>

Latest storage continuation: v1425 removes three superseded initial1396
executables:295,457,632 logical /295,460,864 allocated bytes; measured net free
change0. Required source archives and the1396 shader directory remain because
r1/r2 embed that shader path. v1441 removes five incomplete-diagnostic videos
and five sample PNGs after corrected1426 proof:29,203,122 logical /29,224,960
allocated bytes, measured net free change0. All negative small reports,
execution/input hashes, corrected recordings and original references remain.
