# Engine-wide crash repair goal

Status: **active**. This is the repair goal following the [completed audit](../build/test-results/engine-crash-audit-20261102/REPORT.md). The machine ledger is [progress.json](../build/test-results/engine-crash-repair-20261102/progress.json).

An item is checked only after its original failing loop and required native/original-authority qualification pass on the integrated candidate, or conclusive evidence establishes that a suspected defect is not real. Build success, a missing exception, an attempted input, source-only work or one startup frame is insufficient. New dependency failures become new checklist entries. Unknowns remain open.

Preserve unrelated dirty work, original references/assets, frozen source and selected runtimes. Use the owned storage workflow. Audit cohorts and concurrent ROOT work are distinct; do not silently treat unpublished ROOT code as delivered. Unsupported Vista-only content must receive explicit admission handling without fabricating Xbox behavior.

## Confirmed runtime and native component findings

- [ ] **ECR-001** — Camouflage global-pass admission aborts native multiplayer startup.
- [ ] **ECR-002** — Campaign model-material family admission aborts startup.
- [ ] **ECR-003** — Native AI update dispatch lacks callback1BBEE0 after authored actor birth.
- [ ] **ECR-004** — Native World update dereferences an unmapped producer value at170DA0.
- [ ] **ECR-005** — Native bitmap descriptor bridge consumes unmapped values during object birth.
- [ ] **ECR-006** — Four-player Path Tracing admission exceeds the preloaded material namespace budget.
- [ ] **ECR-007** — Random starting weapon reaches an unadmitted body-animation resource after active gameplay.
- [ ] **ECR-008** — Bundled modes dispatch muzzle effects without the completed native17E460 colors.
- [ ] **ECR-009** — Native object lifecycle/update dispatch has unbound deferred owners.
- [ ] **ECR-010** — BV-first pair creation lacks native2F0EB0 factory binding.
- [ ] **ECR-011** — Nested lift phantom-callback pair lacks native2F78C0 binding.
- [ ] **ECR-012** — World-step level-geometry factory is absent although activation binds it.
- [ ] **ECR-013** — Predictive convex factory rejects a live campaign shape-class owner.
- [ ] **ECR-014** — Native actor update reads unavailable backing at2A1653.
- [ ] **ECR-015** — Native compound-agent child processing lacks2F73C0 dispatch.

ECR-009 includes deferred `C5CF0`, `203240` and `C1270` owners. ECR-002 covers all nine affected campaign material families/maps; ECR-005 covers both bitmap-descriptor failures. Older campaign-route failures must be closed on the final coherent campaign runtime, with their original triggers re-run.

## Suspected defects: prove and repair or conclusively rule out

- [ ] **C-R3** — CampaignLights::receivers calls source_effect_light_receiver_view without the large-bound catch MP has; select_native keeps scenario placements. Any selected supported sphere light with spatial radius >=3 escapes CampaignRenderer::draw..
- [ ] **C-R4** — PointLights::receivers lacks the large-bound catch; direct attachments banshee_bomb (3.0) and wraith_mortar_charge (5.62) are >=3..
- [ ] **C-R5** — Sun-flagged lens (lens+0x28 bit0) makes original 20B90/2F240 queue a 46E90 secondary entry; port throws if any was queued. All sun-flagged lenses are sky lenses..
- [ ] **C-R6** — Catalog preparation failures are swallowed silently and throw when that definition is live; projectile set is barrel-derived..
- [ ] **C-EL-4** — NativeWorldEffects presentation pool throws at 256 live effects; 5 callers uncaught.
- [ ] **C-EL-5** — Attached-effect presentation lifetime consistency throws.
- [ ] **C-EL-6** — NativeWorldEffects::impact projectiles.at() can throw std::out_of_range past FormatError catch.
- [ ] **C-PD-04** — merge duplicate cleanup set.
- [ ] **C-PD-05** — phantom added listener 0x282700.
- [ ] **C-secondary-view** — Secondary-view presentation only resolves player, actor and scenario-placement models. Runtime-born equipment, garbage or squad vehicles can lack a presentation-input owner..

## Concrete nonfatal owner/resource/script failures

- [ ] **NF-01** — Particle environment/camera/flag admission for authored weapon effects.
- [ ] **NF-02** — Nonvisual effect-part dispatch for plasma-pistol charge/flak and dependencies.
- [ ] **NF-03** — Native light-lens owners for grenade/shield effects.
- [ ] **NF-04** — Firing/Ready muzzle lifecycle refusals.
- [ ] **NF-05** — Authored weapon-attachment object-function/color ownership.
- [ ] **NF-06** — pStencil direct-light frustum owner.
- [ ] **NF-07** — Every concrete campaign script fault and failed native object birth recorded by the audit.

- [x] **NF-07-SPHERE-CLEANUP-PHYSICAL** — Native2F9960 matches all17original instructions under GCC/Clang, with89cases/compiler,24fault prefixes,8callback changes and12rejected mutants total. Actual caller context and candidate19 integrated regressions pass. Full birth/lifecycle gate remains open.
- [ ] **NF-07-SPHERE-QUERY** — Forerunner now reaches actual2F6FAB→2F9CA0 sphere-child query. Qualify64instruction native overlap/collector and its actual caller, then re-run the full source loop and controls.
- [x] **NF-07-SPHERE-QUERY-PHYSICAL** — Integrated21 passes all64sphere/2collector instruction comparison,247cases/compiler,44fault prefixes,12observer faults,5mutants/compiler,7invalid-depth controls/compiler, actual caller5positive/13invalid controls and native World regressions. Full gameplay/lifecycle/delivery remains open.
- [ ] **NF-07-CINEMATIC-CLONE** — Forerunner mission_start reaches unsupported cinematic_clone_players_weapon. Implement the genuine builtin and its descendants and qualify the script loop.

- [x] **ECR-006-NAMESPACE-COMPONENT** — Actual77,668slot high-ID regression reproduces old PathTracing cap and passes corrected GPU-byte-bound admission under GCC/Clang, including late texture refresh and untouched unused UI images. Integrated21 Scene/posed-geometry regressions pass; live GPU/view/lifecycle/delivery remains open.

The four zero-radius light seeds are a prior verified no-draw/non-defect disposition; preserve their original rejection behavior and controls. Source-review controls and invalid fixture inputs are not extra engine bugs. Particle/light omissions and caught script faults above still require actual owner resolution.

## Integration, verification and delivery

- [x] **G-01** — Import the complete audit inventory and create this durable checklist.
- [x] **G-02** — Pin current source/runtime/publication state and preserve unrelated work.
- [ ] **G-03** — Build tight red/green native and original-execution loops for every repair group.
- [ ] **G-04** — Integrate qualified concurrent native work into a coherent frozen delivery source.
- [ ] **G-05** — Run every original/minimized repro, candidate reachability probe and dependent lifecycle regression.
- [ ] **G-06** — Qualify campaign and multiplayer across maps, modes, views, weapons, vehicles and resets.
- [ ] **G-07** — Rebuild/package and verify normal gameplay, title and campaign launcher routes use the qualified repair.
- [ ] **G-08** — Retain exact source/validation evidence; close workers and verify scratch cleanup.
- [ ] **G-09** — Audit every checkbox and update the goal to complete only when nothing required remains.

## ECR-010 progress gates

- [x] **ECR-010-PREFIX** — BV-first native World pair construction now publishes both authentic Gemini pairs; reverse/convex/terrain controls publish.
- [x] **ECR-010-PHYSICAL** — Normal-return factory14cases/compiler and5mutations/compiler match unchanged original TU1.5 under GCC/Clang.
- [ ] **ECR-010-UNWIND** — Verify genuine source failure/unwind behavior and preserve shared constructor/release semantics.
- [x] **ECR-010-SHARED** — Requalify the existing reversed wrapper constructor and mutation controls after sharing the native implementation unit.
- [ ] **ECR-010-LIVE** — Original Banshee/campaign repros and all required child-owner/retained-lifetime dependencies pass on integrated runtime.

The physical component test still delegates selected dependencies to unchanged original CPU code. The native World prefix separately constructs the actual chain with native kernels and modeled broadphase/container inputs. Neither closes the full live/delivery item.

## ECR-011 callback dependencies

- [ ] **ECR-011-ENTER** — Qualify and bind original shape-enter1F34D0 and its real1D7530/1D75B0/1DA0F0 dependencies.
- [ ] **ECR-011-LEAVE** — Qualify native cleanup2F7790, leave/destruction1F3550,1DA320 and agent retirement/merge ownership. Current Floodzone/HighCharity render startups reach2F7790.

The36-byte native constructor/factory and enter/leave owners are integrated. Complete physical original/native component comparisons pass, including source-selected resource cleanup. Authentic World child reachability, full lifecycle and launcher delivery remain open; the finite lift replay did not call the new factory.

## Additional dependencies exposed by live repair replays

- [ ] **ECR-002-DRAW-CONTEXT** — Qualify the original draw-context publication before campaign draw-mode consumption. The complete context move removes null+2A78 across13campaign starts:9render both frames,4reach separate native owners; two Delta maps retain independent ECR-005 faults.

- [x] **ECR-010-BV-CLEANUP-PHYSICAL** — Original/native comparison passes61cases/compiler (GCC/Clang),13fault prefixes,5callback input changes and6rejected mutants; all17BVcleanup instructions are covered.
- [x] **ECR-010-BV-CLEANUP** — Qualify native current-child BV cleanup2F0DA0 and its actual scalar deleting destructor; latest Banshee/campaign replays now reach this required dependency.
- [x] **ECR-010-NESTED-DATA** — Qualify disjoint retained DATA for nested native BV creation under list/transform parents, preserving parent/merge/wake packets and genuine missing-owner refusals.
- [x] **C-PD-04-2F81B0** — Qualify the native reversed-list destructor in the integrated World and original campaign/vehicle loops. Native World model, Intro3600tick replay and strict positive Wraith462tick gate pass (2native calls,0refusals,0unconverted owners). Broader cleanup parent remains open.
- [x] **C-PD-04-2FBB10-PHYSICAL** — Original/native single-point and inherited cleanup comparisons pass74cases/compiler,18fault prefixes,6callback changes and9rejected mutants.
- [ ] **C-PD-04-2FBB10** — Qualify actual single-point cleanup and native point-removal/destruction descendants; SentinelWalls now reaches this required pair-deletion owner. Merge-duplicate reachability remains separate.
- [ ] **NF-07-1CD130** — Implement and qualify the native Havok statistics owner required by memory-pressure handling and its dependencies; current Cairo startup refuses it. Pinned Xbox disassembly establishes that1CD130 reports statistics rather than trimming objects.

- [x] **ECR-011-LEAVE-PHYSICAL** — Complete composed cleanup/leave/property/membership comparison passes96cases/compiler,27fault prefixes,8input changes and6rejected mutants. All26/47/20/20/159original owner instructions covered; real pool releases and ordered reads/stores/FP agree. Live/enter/factory gates remain open.

- [x] **ECR-011-ENTER-PHYSICAL** — Complete admission110/member165/enter48instruction original/native comparison passes175cases/compiler,39fault prefixes,11input changes and12rejected mutants, including real allocator exhaustion and all leave regressions. Factory/live qualification remains open.

- [ ] **NF-07-FP-ROLE-TEMPORAL** — Repair the genuine temporal-input producer/role bind order: current2player Lockout lift loop refuses an active native FP view without its completed registry packet before entry. No guard suppression or invented packet.

- [x] **ECR-011-FACTORY-PHYSICAL** — Original/native factory, constructor and class owner pass192cases/compiler,40fault prefixes,11input changes and17rejected mutants. All94/50/2original instruction sites covered.
- [x] **ECR-011-UNWIND-RESOURCE** — Composed resource-cleanup comparison passes209cases/compiler,57fault prefixes and20rejected mutants.17additional faults select22original cleanup actions; retained bytes, reads, stores, pool requests and FP agree. This does not certify the Xbox exception dispatcher or secondary termination.

- [x] **ECR-015-QUERY-PREFIX-PHYSICAL** —146profiles/compiler,82fault or observer prefixes and3rejected mutants. Exact real child/solver entry ABI; full solver return remains open.
- [x] **ECR-015-REVERSED-QUERY-PHYSICAL** —132cases/compiler,45fault or observer prefixes,32source-selected cleanup actions and4rejected mutants. Reversed wrapper/collector DATA and callback order agree; real solver/World/live closure remains open.

- [x] **ECR-015-QUERY-BINDING-PHYSICAL** —132cases/compiler,45fault or observer prefixes,32source-selected cleanup actions and4rejected mutants. Typed wrapper/collector composition; full convex query and live handoff remain separate.
- [x] **ECR-015-SIMPLEX-OVERLAP-PHYSICAL** —8profiles/compiler,4rejected mutants/compiler; both flag inputs across4rounding modes with89,795,481captured bytes compared.228of1020source sites reached; wider simplex/projection domains remain open.

- [x] **ECR-015-CONVEX-QUERY-RETURN-PHYSICAL** —4profiles/compiler,3rejected mutants/compiler; full real query/solver/collector over source-derived inputs. Retained bytes and FP agree. Live projection/wider-simplex domains remain open.
- [x] **ECR-015-CALLER-COMPONENT** —Strict native caller RET16/no-op and12invalid input controls, plus17World profiles pass. Live208tick repro reaches a genuine native projection-iteration refusal, so ECR-015 remains open.

## Current work

Twenty of67setup/component/dependency gates are checked; all15parent ECR findings remain open. Frozen cohort12 imports current ROOT, pins14024source inputs and73extra operational files, and passes the official native World model regression. The previous owned build worker is closed and its scratch removed. Full candidate/runtime requalification remains pending.

Build11 lift acceptance passes179ticks after the test prepares its declared partial-match inputs with two actual neutral ticks. Read-only tracing confirms2genuine FP phases before view binding. The new phantom factory/enter/leave counters are all0 on that replay; the authentic captured-pair probe publishes a list agent but has not established its phantom child path. That evidence stays open.

Composed phantom original/native comparisons pass under GCC and Clang, including real allocation exhaustion and selected constructor/factory failure cleanup. SentinelWalls/Floodzone/HighCharity render starts pass finite controls. Forerunner still reaches the missing ECR-0152F73C0 query; query proposals remain unqualified and unbound. Broader campaign/MP content, lifecycle, respawn, view/reset, custom-mode, performance and delivery gates remain open.

A concurrent workflow changed the normal launcher selections to824185e7306d46cfa61015806277e63a after the baseline. That delivery is preserved; none of the private repair candidates has been published by this goal. The old selected runtime/source archive also remains preserved.

Cohort12 full native/acceptance executables build, the179tick lift passes, and the15map4frame resize census completes:9finite startups pass; Cairo/Arbiter/twoDelta maps retain recorded native owner/backing faults; OldMombasa and HighCharity retain caught script faults. Forerunner208tick simulation reproduces2F73C0. Cohort13 adds the qualified query/kernel composition and a strict native caller adapter; its build and full query/live qualification are still pending.

Cohort13 builds and passes its two registered regression targets. Forerunner208ticks now enters native2F73C0/2F8410/30E930, then refuses an unimplemented projection iteration. A private read-only typed-BEFORE probe is being built to qualify that genuine branch. No bypass, supplied query result, full gameplay fix or launcher delivery is claimed.

- [x] **ECR-015-PROJECTION-PHYSICAL** — Native first-shape projection on the genuine Forerunner typed input: four rounding controls/compiler, four rejected mutants/compiler; all121,675,707retained bytes,101ordered reads,30stores,result and FP match789original instructions/370of1020solver sites. Existing simplex/query controls pass; integrated candidate16 builds and passes caller/World regressions. Wider solver/World closure remains open.

- [x] **ECR-013-GAUSS-WHEEL** — Reproduce and qualify the newly reported Gauss Warthog crash near the Zanzibar wheel on the qualified private candidate; normal launcher delivery is tracked separately. User log ends at the existing class23-only collision-factory guard on the824185 shader route; user confirmed driving, with turret firing kept as a separate control.

- [x] **NF-07-SMALL-FACTORY-PHYSICAL** — Native sphere factory2F9990, strict caller and native pool composition. 32cases/compiler in each composition,8fault prefixes,2callback input changes,4rejected mutants/compiler. Caller11invalid controls pass. Genuine native pool services compare full banks/ordered operands/stores/FP; callback journal additionally checked by direct/kernel caller controls. Cohort18 integrates constructor/table and builds/passes registered regressions; original script now advances to actual2F9960 cleanup, which remains open.

- [x] **ECR-013-SPHERE-FACTORY-PHYSICAL** — Actual class4 sphere admitted through native convex factory owners. Genuine Forerunner class4 typed input reproduces old admission refusal. Native getter2E8CF0/center2E8D70/support2E8D20/encodingAD740 compose to match486original instructions,all121675707bytes,50ordered reads,18stores,return/FP with3independent scratch poisons/compiler,3mutants/compiler rejected. Integrated component only; later World/script faults and Gauss-specific class owner remain open.

- [ ] **NF-07-SPHERE-CLEANUP** — Actual2F6C88 sphere-child cleanup requires native2F9960. Qualify17instruction2F9960 kernel, exact caller handoff, real manager/scalar/pool descendants; integrate and re-run original Forerunner birth, lifecycle and broader regressions.

- [x] **NF-07-BACKING-200005BE** — Forerunner reports unavailable runtime backing at200005BE after failed object creation. Re-run after genuine2F9960 object-creation cleanup is implemented; diagnose actual producer/reader if the backing fault persists. Do not infer an independent memory defect from the earlier partial-birth state.

- [x] **ECR-006-GPU-STARTUP** — Candidate21 uploads/renders the original Coagulation two-frame trigger with1/2/3/4players; lifecycle and final delivery stay open. [Evidence](../build/test-results/engine-crash-repair-20261102/live-replays21.json).

Candidate21 reopened **ECR-010-NESTED-DATA**: Forerunner reaches tick66 and exceeds the three-slot constructor reservation under two live parents. The previous bounded disjointness proof did not cover this deeper chain. Gauss driving21 remained stationary26.33m from the wheel, so its normal process completion is not wheel-crash qualification.

Candidate22 cold Forerunner completes600ticks without the earlier fatal/backing errors; the distinct caught cinematic-clone fault remains open. This closes the conditional backing follow-up, not all births or campaign lifecycle. Native22 turret firing control passes327ticks. Gauss22 driving misses the surveyed turn and stalls17.84m from the wheel; Gauss23 is a further ordinary-input fixture correction, still awaiting live qualification. Bundled-mode firing failures remain reproduced in all three affected modes.

Gauss driving23 passes the bounded original trigger in default Original graphics:4.9358m minimum wheel distance, all14waypoints,644904wheel receiver vertices,1353native physics frames and16/16native convex creations with zero refusals. [Qualification](../build/test-results/engine-crash-repair-20261102/gauss-wheel23-qualification.json). Final delivery remains a separate open gate.

The reopened nested-DATA dependency is requalified on22/23: deeper-chain model/sentinels and600tick Forerunner pass, followed by all four current Banshee/Wraith control cases. The Banshee secondary-input case emitted no projectile and is not bomb/large-light proof. Parent lifecycle and large-light live gates remain open.

## Effect-color dependencies exposed by source audit

- [ ] **ECR-008-17E460** — Native17E460 color selection and strict actual caller binding. Port/qualify original17E460 observable colors, cache/probe/write-back branches and current caller context; no translated or guessed fallback.
- [ ] **ECR-008-PROBE** — Native17E460 location/lightmap probe and cold-owner paths. Qualify native17DCE0/17BA50/D4F90 with actual14DA70/D4350 dependencies; verify cold births, respawns and missing-root sample branches.
- [ ] **NF-05-LEGACY-COLORS** — Completed colors shared by legacy first-person, world and attachment effect requests. Use one completed original color pair for FP/TP requests, preserve allocation/refresh and request ordering, and qualify flags0x18 attachment paths and relevant client behavior.
- [ ] **C-LEGACY-LIGHT-CACHE** — Legacy render-cache unavailable-bank and256record lifetime parity. Prove or repair accepted-without-sample behavior and actual original cache eviction/rebind consequences. A source difference alone is not a runtime crash.

[Claude read-only source evidence](../build/test-results/engine-crash-repair-20261102/claude-effect-color-owner/report.md). These are open owner/parity dependencies, not four newly reproduced gameplay crashes.

- [x] **ECR-008-CACHE-COLOR-PHYSICAL** — The typed native selector matches unchanged original 17E460 on 80 cached and no-probe profiles per compiler, across four rounding modes. Three incorrect implementations per compiler were rejected. [Physical comparison](../build/test-results/engine-crash-repair-20261102/color-selector-physical25-r3/report.json).
- [x] **ECR-008-CACHE-COLOR-LIVE** — Native25 completes the original 180-tick Fiesta, Infection and Defusal firing replays plus neutral Infection. A hardware breakpoint observes two calls to the compiled selector in the Defusal replay. [Replay evidence](../build/test-results/engine-crash-repair-20261102/bundled-modes25-qualification.json).

31 of 85 gates are checked. Cold effect lighting, full ABI and attachment/peer qualification remain open. Armory still exits at the unbound 1BBEE0 actor callback on tick 11; the tick 10 control exits normally on the same candidate. [Armory recheck](../build/test-results/engine-crash-repair-20261102/armory25-recheck.json).

- [x] **ECR-004-BSP-RESET-PHYSICAL** — Native visibility publication and the player callback match the original no-relocation path: 36 cases per compiler, 16,996 source steps and 396 ordered stores. Three incorrect implementations per compiler were rejected. [Physical comparison](../build/test-results/engine-crash-repair-20261102/bsp-reset-physical28/report.json).
- [x] **ECR-004-BSP-RESET-LIVE** — Gas Giant passes the original tick 3 crash and tick 2 control. Hardware breakpoints observe one call to each compiled reset routine during the switch to BSP 2. The 180-tick replay reaches its endpoint and records two separate caught script faults. [Live evidence](../build/test-results/engine-crash-repair-20261102/bsp-reset28-live.json).
- [ ] **ECR-004-BSP-RELOCATION** — Qualify the pending player relocation branch before the reset. The selected native no-relocation path refuses this unported branch.
- [ ] **ECR-005-CPU-PROJECTION** — Trace and qualify native MCC bitmap row delivery at the actual D45A5/D4EBF lightmap callers before the metadata constructor. Both Delta cases still fail.
- [x] **ECR-014-RECHECK-323** — The original v7 Sentinel Walls tick 323 workload exits normally on native25. The distinct C1270 failure still occurs at tick 1094 on v9, and full actor backing qualification remains open.
- [ ] **NF-07-AI-COMBAT-STATUS** — Qualify the original builtin reached by the Gas Giant active_camo_drop script.
- [ ] **NF-07-INTERPOLATOR-START** — Qualify original interpolator startup in Gas Giant and High Charity authored scripts.

34 of 92 gates are checked. All 15 parent findings remain open for the listed dependencies and delivery. The normal Linux launcher still selects e26c2de82603481aa262e219aa31761d.

- [ ] **ECR-005-DELTA-MACHINE-DRAW** — Both Delta four-frame replays pass the bitmap constructor and reach a separate machine scene owner/placement failure. Fix the native owner and qualify the rendered paths.
- [ ] **NF-07-DELTA-PVS-SET-OBJECT** — Delta Control reaches the authored e7_scarab_main PVS object request at tick 11.
- [x] **ECR-009-RECOVERY-LIST-PHYSICAL** — C1270 matches all 39 original instruction sites, 36,243 physical steps and 4,245 ordered stores over 12 cases per compiler. Actual C12F0 recovery and live validation remain open.
- [ ] **ECR-009-RECOVERY-C12F0** — Qualify original recovery-list consumption, eligibility and actual native object cleanup after the C1270 enumeration.

35 of 96 gates are checked. [Phase 09 evidence](../build/test-results/engine-crash-repair-20261102/phase-09-report.md).

- [x] **ECR-005-D45A5-BITMAP-LIVE** — Both Delta maps pass the original bitmap failure through native CPU projection. Compiled caller controls match the original DXT1 colors on both call sites. Rendered Delta still reaches the separate machine ownership fault.
- [x] **ECR-009-RECOVERY-C1270-LIVE** — Sentinel Walls tick 1094 crosses the native list collector and reaches the genuine C12F0 callback, which remains unbound. The tick 1093 control reaches its requested budget with a caught script fault.

37 of 98 gates are checked. No private repair candidate has been published to the normal Linux launcher.

- [x] **ECR-005-DELTA-MACHINE-VISIBILITY** — Native33 qualifies exact original visibility on both machine placement disagreements. Delta Control submits all four frames; Delta Cliffs progresses to a separate rain width refusal.
- [ ] **ECR-005-DELTA-RAIN-WIDTH** — Qualify the actual Xbox width producer and native coverage for the Delta Cliffs rain width of 0.75; reproduce on the real device and complete the render replay.
- [x] **ECR-009-RECOVERY-C12F0-EMPTY-PHYSICAL** — Original empty return and positive entry prefix match 16 cases per compiler, 55 original sites, 36,282 original steps and 4,253 ordered stores. Three incorrect implementations per compiler were rejected.
- [x] **ECR-009-RECOVERY-C12F0-EMPTY-LIVE** — Original Sentinel Walls tick 1094 and adjacent 1095 reach their budgets without the C12F0 crash. Both still report the separate caught campaign global 798 fault.

40 of 102 gates are checked. All parent ECRs remain open. [Native33 replay evidence](../build/test-results/engine-crash-repair-20261102/machine-recovery33-live.json). Current normal launcher selection is a899cb350d7c47ae841d67e9701365dd; no private candidate has been published by this goal.

- [x] **ECR-005-DELTA-RAIN-EMPTY** — Hardware breakpoint observed zero vertices at the width refusal. Native34 completes all four Delta Cliffs frames; GPU regression confirms clearing an empty draw and retaining rejection for unsupported nonempty widths.
- [x] **ECR-005-DELTA-PARTICLE-MOTION** — Native35 completes 120 frames; the actual replay calls compiled 2C3E30 12 times. An actual caller snapshot matches the original CPU exit and all 116,394,719 retained bytes; three neighboring original contrail controls pass.

41 of 104 gates are checked. [Native34 replay evidence](../build/test-results/engine-crash-repair-20261102/empty-weather34-live.json) and [launcher source readback](../build/test-results/engine-crash-repair-20261102/selected-a899-source-readback.json). The later particle and nonempty rain routes remain open.

- [x] **ECR-005-DELTA-SCENERY-CLASS3** — The actual redcedar draw uses the original class-3 material selection. Native37 completes all 320 frames to tick 488, past the old frame 251 crash. A hardware breakpoint records the authentic class-3 unit, and the original Cairo class-1/2 controls remain exact.

42 of 105 gates are checked. [Live particle evidence](../build/test-results/engine-crash-repair-20261102/delta-particle35-observer.json) and [original CPU comparison](../build/test-results/engine-crash-repair-20261102/delta-particle36-physical.json). Nonempty rain, later scenery and normal launcher delivery remain open.

43 of 105 gates are checked. [Delta scenery replay](../build/test-results/engine-crash-repair-20261102/delta-scenery37-live.json), [TU1.5 selection comparison](../build/test-results/engine-crash-repair-20261102/scenery-class3-tu15-selector.json), and [Cairo control](../build/test-results/engine-crash-repair-20261102/cairo-scenery37-iso-control.json). Both launchers currently select d4baacd102774209bca18d602645692e; no private repair candidate was published by this goal.
