# Native TU1.5 campaign engine implementation

Started October 28, 2026. Goal status: **active; incomplete**.

October30 user clarification: cutscene audio, sound effects and dialogue must
also be 1:1 native kernels. Full campaign completion requires their generic
original scheduling, playback, mixing/spatial behavior and composed delivery.
The [current audio scope](CAMPAIGN_NATIVE_AUDIO_COMPLETION.md) records the
remaining owners and acceptance requirements; the whole audio gate stays open.

November2 user clarification: presentation must match OG Xbox, with resolution
scaling for modern displays and native kernels. The supplied Arbiter comparison
shows a character shading discrepancy. Original material passes, lighting,
camera composition and display-mode behavior are the authority; PC framing
extensions and component-only numerical matches do not establish that match.
The [display-owner audit](../analysis/campaign-camera-20261030/original-display-aspect-owner-audit-r1/report.json)
authenticates the original console-mode query and its projection/aperture
consumers. Current PC drawable-aspect adaptation remains a separate extension.
The user identifies the reference as TU1.5 but does not know its widescreen
setting; the video's aspect ratio does not establish that setting.

Implement the missing capabilities in [the audit](CAMPAIGN_NATIVE_KERNEL_AUDIT_20261028.md) as generic native semantic TU1.5 kernels, so imported tags, authored Halo scripts, AI and cinematic sequences run with original behavior. The native Halo script VM may interpret game-script data. Xbox CPU emulation, instruction lifting, executable-backed gameplay and per-mission replacements do not qualify. Original executable execution is an independent offline reference only.

## Completion contract

- [ ] Every applicable campaign gameplay/control/presentation path uses semantic native kernels. Remove lifted/emulated gameplay fallback and prove reached native/residual identities.
- [ ] Preserve TU1.5 branching, timing, update/callback order, state/lifetime, RNG and observable numerical behavior; imported data cannot substitute for an algorithm.
- [ ] Every kernel has independently authenticated original comparisons, meaningful failing controls, and applicable edge/fault/alias/callback tests through the actual interface.
- [ ] Wire verified kernels into actual campaign consumers, then qualify authored content, complete missions and original/native visible behavior. Component results alone do not close a mission or the whole goal.
- [ ] Deliver a coherently built/qualified launcher runtime only after those gates; preserve existing selected runtimes while development remains incomplete.

## Completed milestones

Checked items are the specific implementation or verification milestones named here. The broader completion contract above remains open.

- [x] **K01/K02:** native actor-query/reset kernels, independent original comparisons, mutation/sanitizer checks, and live dispatcher integration.
- [x] **K01/K02 campaign observation:** the coherent MCC client reaches both native callbacks and continues to the newly recorded missing owners.
- [x] **K03/K09:** native living/swarm/nonswarm/strength queries, original count/evaluator comparisons, and actual script-host bindings.
- [x] **K04/K05/K08:** native sound-class fades, voice routing and ducking dependencies, with original numeric/state/order proof and mutation/sanitizer checks.
- [x] **K06/K07:** native screen reset/capture acknowledgement, guarded world methods, original-address bindings and validated execution-journal support.
- [x] **K10:** native combat-count/actor predicate, original comparisons, mutation/sanitizer checks and script-host integration.
- [x] **K11:** native actor-to-script-key reassignment and hash/free-list ownership, independently validated for the actual actor payload profile.
- [x] **K12:** full native timed-screen/DOF kernels, three tested API paths, guarded world integration and current caller object compilation. The old four-body generated screen module is excluded from production.
- [x] **K13:** native playback fade-request/sample dependency closure and removal of the old register-level fraction body, with original/mutation/sanitizer proof.
- [x] **K14:** native order-reset camouflage and actual dirty-entity publication, with complete original/mutation/sanitizer proof.
- [x] **K15:** native authored-area eligibility/candidate selection, including inheritance and all three scan paths; original timelines/faults, AX16 transport and mutation/sanitizer checks pass; handler is wired.
- [x] **K16 implementation/proof:** native behavior82 reset and its binding, all original sites, independent full/fault cases and four rejected mutations.
- [x] **K16 routing edit:** main build module, fixture admission and world handler are wired. Caller compilation and actual continuation remain separate open steps below.
- [x] **K17:** native clump retirement across twelve owners, all four registered invalidation callbacks and shared pool deletion; original whole/fault/mutation/sanitizer checks pass. The guarded live route and activity counter are wired.
- [x] **K15/K16/K17 caller compilation:** current world, screen wrapper and journal-decoder objects compile after all three routes are added; exact captured world source still matches.
- [x] **K18:** all five listener algorithms converted in the actual listener table; original arithmetic, retained FP operands, result-width contracts and mutation/sanitizer checks pass.
- [x] **K19:** native sound-source/controller retirement reuses the single shared pool-deletion implementation; original full-state/alias/order and compiler/mutation/sanitizer checks pass.
- [x] **K20 implementation/proof:** native combat/perception reset marks and retires actual queries, clears the first matching actor reference and reads the original policy table. Full/fault/mutation/compiler/sanitizer comparisons pass; build and world entry are wired.
- [x] **K15/K16/K17 campaign passage:** the frozen v5 client reaches all three native entries. Elite completes900 ticks after actual clump retirement; Marine/Grunt and Johnson expose the next real owners `278460` and `1C9F10`.
- [x] **K21:** native `ai_living_fraction`, exact full-width signed counters and hardware division, original/evaluator/fault comparisons, five rejected mutations and actual MCC opcode329 layout admission.
- [x] **K22:** native playback stop callback `18F6B0`, actual indirect-caller frames and independent manager route; original, mutation and sanitizer checks pass.
- [x] **K23:** native unit/parent dirty marking, admitted action cancellation and local-player history support; complete original/fault/callback/mutation/sanitizer checks pass. Full action begin remains open.
- [x] **K24:** native point/frame geometry prerequisites, actual association/alias/fault/FP behavior and original/mutation/sanitizer checks pass.
- [x] **K25:** shared native character/navigation/order/movement tag lookups, including inherited movement blocks; all194 original sites,423 comparisons and five rejected mutations pass.
- [x] **K26:** native path-priority heap up/down/insert, actual caller frames, exact signed priorities/ties/store order; all113 original sites and five rejected mutations pass.
- [x] **K20/K23 caller compilation:** actual world, screen wrapper and journal-decoder objects compile after combat-reset and admitted action-support routes; composed actor reset remains open.
- [x] **K27:** native navigation-filter masks use real character/order/vehicle/action data;496 original cases, all247 closure sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K28:** native salted data-array allocation, signed free-slot scan, occupancy/counter publication, alias-sensitive clear/salt issuance;238 original cases, all78 sites, exact repeat and four rejected mutations pass.
- [x] **K29:** native query allocation, reserved-slot attachment and live-slot iterator, including real shared allocation;379 original cases, all190 closure sites, exact repeat and five rejected mutations pass.
- [x] **K30:** real perception position/tracking reference resolution, signed policy indexing and wrapped-pointer null behavior;238 original cases, all56 sites, exact repeat and five rejected mutations pass.
- [x] **K31:** native path workspace initialization, lazy input records and actual BSP/path storage;303 original cases, all53 sites, exact repeat and five rejected mutations pass.
- [x] **K32:** native query slot selection, reservation, broadcast and retirement through six owners plus shared deletion;69 original calls, all343 sites, typed-local request profiles, six rejected mutations and compiler/sanitizer checks pass.
- [x] **K33:** native object-to-actor ownership, all256 object kinds and signed extension/null behavior;315 original cases, all29 sites, exact repeat and five rejected mutations pass.
- [x] **K34 implementation/proof:** native two-overlay animation-channel reset, real transport/RET boundaries and caller liveness;234 original cases, all40 sites, exact repeat, three rejected mutations and compiler/sanitizer checks pass. Build/world route wired; fresh actual caller objects compile.
- [x] **K34 caller compilation:** the actual world, screen wrapper and journal decoder compile after the animation-reset route is added; runtime composition remains separate.
- [x] **K35:** native perception availability and tracking/group-state reset;228 original cases, all77 sites, alias-sensitive post-reset reads, exact repeat and five rejected mutations pass.
- [x] **K36:** native behavior query-start broadcast, actual member ordering/self identity and reservation result;35 original calls, all186 sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K37:** native path-query initialization with exact distance/scale/CRT conversion, real heap admission and original invalid-cost behavior;910 original comparisons, all117 root sites, six rejected mutations and compiler/sanitizer checks pass.
- [x] **K24 current geometry regression:** the typed value-projection extension passes all778 prior original geometry cases; current header/core pins are requalified.
- [x] **K38:** native perception group find/active/point reset/property allocation storage, real shared allocator and original fault/store timelines;376 original comparisons, all164 owner sites, four rejected mutations and compiler/sanitizer checks pass.
- [x] **K39:** native salted-pool iteration, campaign bitset, actor timer lookup and perception-head support;108 original calls, all107 sites, native-local iterator profiles, seven rejected mutations and compiler/sanitizer checks pass.
- [x] **K40:** native object world-point resolution, real signed parent/node transforms, per-component alias publications and native private-value output;237 original cases, all76 sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K41:** native perception-group initialization and team policies, actual shared bitset and point reset;1,219 original comparisons, all187 new-owner sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K42:** native perception actor fanout/position link with real object geometry and native allocation;557 original comparisons, all116 new-owner sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K43:** native vector normalization and actual3EA20→266EF9 distance publication, including real failed stores and typed pending quantities;421 original cases, all58 sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K44:** native campaign-profile track cancellation checks, real policy/iteration and authenticated275770 caller transport;73 original cases, all231 admitted sites, eight rejected mutations and compiler/sanitizer checks pass.
- [x] **K41 cold-world wiring:** actual native constructor/reset, original writable default-point owner and checkpointed pointer changes pass in an unchanged11,665-file snapshot; both integration scratch roots are removed.
- [x] **K46:** native actual275770 admission through real track/timer/team/iteration policies;108 original calls, all450 admitted sites, eight rejected mutations and compiler/sanitizer checks pass.
- [x] **K47:** actual274450 forced-create classifier profile, all256 object bytes and real attachment/generation/team traversal;644 original comparisons, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K48:** native supported path arena/lease profile, genuine primary teardown/reconstruction and explicit secondary-free boundary;263 original cases, all126 admitted sites, five rejected mutations and compiler/sanitizer checks pass.
- [x] **K49:** native unassociated resource retirement, real page protection/metadata unlink/deletion and composed secondary arena leases;902 shared original regressions, exact repeats, six resource and six composition mutants pass.
- [x] **K45:** native pressure allocation, real group pruning/behavior invalidation, scoring and retry;746 original calls/faults, all reachable owner sites, authenticated eight-site exclusion and compiler/sanitizer/mutation/repeat checks pass.
- [x] **K50:** native campaign-profile group creation and first matching property lookup through actual classification, allocation and actor fanout;1,246 original calls/faults, all95 new-owner sites and compiler/sanitizer/mutation/repeat checks pass.
- [x] **K51:** native weighted speech selection with actual RNG backing, source numerical controls and caller-live outputs;221 original calls, all76 reachable sites, six rejected mutations and compiler/sanitizer/module checks pass.
- [x] **K52:** native navigation hash lookup and authored segment candidate math;353 original cases, all74 sites, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K53:** native whole perception lookup, actual object/self/attachment identity and optional real group creation/deadline publication;739 original comparisons, all111 owner sites, six rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K43 current vector-value extension:** all421 previous original cases and191 private-value normalization profiles pass after factoring the shared algorithm; current source/header pins, mutants and sanitizers are requalified.
- [x] **K55:** native tracking allocation, actual pool-pressure retirement, actor iteration and reset;1,365 original comparisons, all452 new-owner sites, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K54:** native model-marker lookup, variant/node enumeration, quaternion basis and transform composition;685 original comparisons, all280 sites, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K56:** native inherited object-marker owner and current motion getters;534 original comparisons, all214 sites, six rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K58:** native speech queue admission, collision postponement, transitive cancellation and cancelled-row reclamation;176 original calls, all384 closure sites, six rejected mutations, exact repeat and compiler/sanitizer/module checks pass.
- [x] **K59:** native character/vehicle/perception policy prerequisites through nine owners;884 original comparisons, all331 new/402 composed sites, nine rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K60:** native jump fallback trajectory and private-value output, including real pending quantities on rejected publications;261 original cases, all82 sites, six rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K57:** native retained-output object marker request and original fallback pose;505 original comparisons, all118 root/453 closure sites, six rejected mutations, exact repeat and compiler/sanitizer checks pass, with rejected-copy pending-read scope recorded.
- [x] **K61:** native unit speech reservation, actual source/permutation/history eligibility and null-owner fault;203 original calls/prefixes, all220 sites, six rejected mutations and compiler/sanitizer/module checks pass.
- [x] **K62:** native campaign/direct-policy perception score;814 original calls/boundaries/faults, all384 admitted root/752 composed sites, eight rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K60 current lazy point interface:** all261 original fallback cases,145 private outputs and145 private input-point pairs pass after the shared native coordinate interface extension; canonical fixture stays byte-identical.
- [x] **K54/K57 current private marker interface:** all1,190 previous raw cases and113 independent original capacity1 private calls pass through one shared native algorithm;19 total mutants, compiler/sanitizer/exact repeat and cleanup checks pass.
- [x] **K63:** native object/stored-state snapshots, attachment center/nearest selection and actor point through the actual private marker interface;1,127 original cases, all307 root/833 composed sites, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K64:** native speech retirement follow-up, actual unit-channel publication and tick conversion;298 original calls, all95 reachable sites, seven rejected mutations and compiler/sanitizer/module/caller checks pass.
- [x] **K66:** native vision timestamp/rescore, attachment-owner selection, unit vector/query and facing prerequisites;1,124 original profiles, all193 new/931 composed sites, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K65:** native transition-plane/private sector, object hierarchy and reaction state/cache gates;973 original cases, all186 sites, eight rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K67:** native authored jump link and actual high-arc solver profile;410 original cases,440 profile sites,284 private-point variants, seven rejected mutations, exact repeat and compiler/sanitizer checks pass.
- [x] **K68:** native cached BSP-cluster speech mask using genuine cache storage and proved listener geometry;121 original calls, all201 sites, seven rejected mutations and compiler/sanitizer/module/caller checks pass.
- [x] **K69:** native attachment/character/player/clump vision records;457 original calls/prefixes, all99 sites, seven rejected mutations, exact repeat and compiler/sanitizer/caller checks pass.
- [x] **K70:** native alert-permission prerequisites and actual gameplay LCG;783 original profiles,97 admitted sites, seven rejected mutations, compiler/sanitizer and exact repeat checks pass. Nested speech and specified private FP fault states remain outside this component.
- [x] **K71:** native campaign next-key and first matching path selection from the actual catalog;329 original cases and174 decoded-catalog profiles pass six mutations, compiler/sanitizer and exact repeat checks. Full completion/save/loading composition remains open.
- [x] **K72:** native cinematic reset/stop and camera FOV commands;440 original cases, all138 sites, seven rejected mutations, compiler/sanitizer and exact repeat checks pass. The live dispatch now selects these native kernels; remaining camera/director owners stay open.
- [x] **K73:** native scenario-to-campaign/map lookup on cold startup;592 original cases and114 decoded-catalog profiles pass seven mutations, compiler/sanitizer and exact repeat checks. The cold session now calls this native helper directly.
- [x] **K74:** native cinematic/director/input constructors, genuine shared CRC and FOV defaults;287 original cases, all141 sites, eight rejected mutations, compiler/sanitizer and exact repeat checks pass. Production dispatch selects these native constructors; controller/control/director frame composition remains open.
- [x] **K75:** native camera-control enable/disable and real object/action/mode selection;621 original cases, all267 sites, eight rejected mutations, compiler/sanitizer and exact repeat checks pass. Production dispatch selects the native control kernel; animation/observer and full cinematic frame composition remain open.
- [x] **K76:** release `map_reset`/`time_code_reset` complete with void and preserve game state/timing. Sixteen full original HS calls and the linked before/after-sleep continuation regression pass; no map restart is requested.
- [x] **K77:** bounded native transition-option publication and actual clock-read protocol;5,673 original cases,43 pre-boundary sites, seven rejected mutations, compiler/sanitizer and exact repeat checks pass. Genuine UI continuation and production clock/storage integration remain open; this helper is not called from the host completion adapter.
- [x] **K78:** native ordinary director reset, with the actual mode2 controller boundary preserved;124 original cases,60 admitted sites, eight rejected mutations, compiler/sanitizer and exact repeat checks pass. Production dispatch selects this native reset. The mode2 controller and five post-call sites remain open.
- [x] **K79:** restored the exact immutable TU1.5 view-angle scalar missing from native startup. Original-section/xref authentication and the real cold-constructor extent/bytes/ownership/write-rejection/checkpoint regression pass. This data fix does not close the camera packet algorithm or renderer.
- [x] **K80:** native reached observer-to-render camera packet and retirement of its old generated body;277 original cases, all74 admitted sites, seven rejected mutations, compiler/sanitizer and exact repeat checks pass. Both reached call sites use the native boundary; the35-site default-camera branch remains explicitly open.
- [x] **K81:** restored the genuine one-byte mutable arena flag used by the view producer. Original loader/constructor/xref authentication, the fresh cold-data ownership/checkpoint regression and actual producer writes pass. No surrounding zero bank or camera outcome is supplied.
- [x] **K82:** texture-camera preparation now requests a real readable scene attachment at native resolution. The independent real-GPU regression passes with no camera materials/upscaling, real backing handles, presentation/readback and stale-resource checks; existing alias/depth/resize tests also pass. Campaign first-frame/progression acceptance remains separate.
- [x] **K83 component:** native `156E40` global fade/per-player flash sampling and finite/zero transition curves;519 physical original cases,223 fault prefixes,271 admitted sites, seven rejected mutations, GCC/Clang/actual-core sanitizers and exact repeat pass. Actual `2B8EC` binding is wired; exceptional CRT handlers and composed graphics/progression remain open.
- [x] **K84:** restored the exact one-byte mutable widescreen storage at `4922E2`. Original loader/initializer/xref proof and the fresh production cold/checkpoint regression pass; the actual GPU run clears this missing-data boundary. Platform settings initialization and complete rendering remain separate.
- [x] **K85 component:** native `170820` model hierarchy composition, `144BF0` basis, `144CF0` packed pose and shared `145560` product;576 physical originals, all271 sites,379 fault prefixes, eight rejected mutations, compiler/sanitizer and exact-repeat checks pass. Actual sky caller is wired; composed graphics and physical Xbox reciprocal-estimate qualification remain open.
- [x] **K86:** restored the independent four-byte mutable render-target selector `4920D8`. Whole-field original ownership, actual fresh cold extent/mutation/checkpoint regression and the next composed GPU observation pass this storage gate. No target publication or completed frame is supplied.
- [x] **Storage:** completed checkpoint validation builds, offline executables and raw banks were removed; required reference data and compact evidence are recorded in the checkpoint manifests.

## Immediate unchecked steps

- [ ] **New Game launch:** qualify the actual menu/difficulty → fresh campaign world → The Heretic authored startup/camera → The Armory → player control flow. Fresh New Game selection and the catalog-driven next-map adapter are implemented; imported Heretic metadata has one valid player start. No complete rendered or native-only progression run is accepted yet.
- [x] **New Game menu regression:** linked actual-MCC validation of all four difficulties, stale Select Level state, cancellation and absent Resume passes in coherent cohort v1.
- [x] **Continuous live New Game menu:** actual title/profile creation → Campaign → New Game → each difficulty → fresh Heretic passes in one native process. Easy/Normal/Heroic/Legendary retain0/1/2/3 and render10/8/7/7 frames; all exit cleanly with zero script/content faults. XTest key events use owned X11 displays/private profiles; no --campaign/--map/--menu-action/--simulate bypass is used. Installed launcher remains unchanged.
- [x] **Requested campaign test build:** separate `Halo2-Campaign-Test` launcher and retained V10 iteration1 package use selected V7 assets and isolated test preferences. The exact delivered launcher passes real Normal menu selection into fresh Heretic with15 campaign frames; independent packaged Heretic6-frame and Armory1-frame RTX5080 checks also pass with zero script faults. The reported inactive texture-camera renewal crash is cleared. The normal launcher selection is unchanged; full campaign/native-only closure remains open.
- [x] **New Game cold startup:** coherent v2 iteration1 starts authored Heretic threads and reaches120 ticks with zero script faults and no injected script/squad. Actual native reset1/FOV1/control1 calls are observed; current native scenario lookup and release-reset regression are included. This is bounded startup acceptance, with full cinematic/native-only closure still open.
- [x] **New Game rendered Heretic:** V9 iteration1 renders2154 actual RTX5080 frames, reaches authored level_finished with zero script faults, and naturally loads a fresh Armory world with Normal difficulty preserved. This qualifies functional rendered Heretic and catalog handoff; framebuffer parity, native-only closure, and separate crossfade-owner claims remain open.
- [x] **Armory first frame:** frozen V10 iteration1 with the selected V7 assets renders an actual RTX5080 frame with zero HS/content faults, one machine-view packet and66 machine GPU draws. Genuine PRT resources, live coefficients and immutable per-view ownership are wired. Separate full training/player-control acceptance remains open.
- [x] **Campaign machine packet ownership:** actual Armory ordinary/secondary view0 captures own disjoint vertex, PRT and depth storage; the first capture stays unchanged after the second. Wrong selected ordinals, rebase-stale generations and retired resources are rejected. Both private packet modes also submit one actual GPU frame. Authored secondary-view activation and framebuffer parity remain open.
- [x] **K87:** exact473628 mutable file-backed1.0 storage passes fresh production cold/checkpoint ownership and the133-check/49-negative portable main-view contract in v6. The actual GPU continuation clears this field and reaches the texture-camera caller bug. Original HS global444 and actual write ownership are authenticated; no field outcome is preset.
- [x] **K88 component/integration:** native2B700→14CDB0 BSP selection passes268 physical originals, all76 sites,95 read-fault prefixes, compiler/sanitizer, eight mutations and exact repeat. The actual2B7E6 binding is wired and its CMake fixture/admission regressions pass; composed campaign graphics remain separate.
- [x] **K89 component/integration:** native2F950 view-rectangle calculation passes550 physical originals, all124 sites,270 prefixes, compiler/sanitizer, eight mutations and exact repeat. Both actual2B8B4/13C8A bindings are wired and its CMake fixture/admission regressions pass; composed graphics remain separate.
- [x] **K90 retirement:** compiled and independently verified removal of both lifted camera helper bodies, exact three native bridges and unchanged21 other retained bodies/address mapping. All four combined packet/BSP/rectangle/admission regressions pass. The producer has23 legacy bodies; the full native camera closure remains open.
- [x] **K91 component:** generic native27D7D0/27DA40 awareness commands pass924 original comparisons,576 reachable sites,700 fault prefixes, eight actual faults, compiler/sanitizer checks, nine meaningful mutations and exact repeat. These update existing perception links; live World ABI and shared HS floating-point authority are separate pending integration gates.
- [x] **AI awareness World integration:** actual2AF0F0/2AF13F held-ABI bindings are wired through the native World dispatcher after1812 transports and35 original caller packets pass. The intro has not yet exercised this route; public script-host FP authority remains open.
- [x] **K92 component/retirement:** native2F5E0/2F620/2F690 grid, tile and rectangle layout passes775 physical comparisons, all173 sites, compiler/sanitizer checks, eight mutations and exact original repeat. Five combined fixture/admission regressions pass after removing the three lifted bodies and preserving all eight native bridges. The producer now has20 legacy bodies; original RAM RMW-cut qualification is explicit.
- [x] **K93 projection storage:** exact read-only four-byte46A260/46A670 operands pass fresh production cold/checkpoint, write-rejection and GPU continuation in v7 iteration2. The11-span original fixed-data preflight found these two gaps; actual rendering now clears both. Dynamic projection inputs remain outside this finite data preflight.
- [x] **New Game first GPU frames:** actual production v7 iteration2 renders six frames on the RTX5080, reaches tick11 with zero script faults and a real checkpoint, and records six native packet/BSP/rectangle/flash/model calls plus18 native layout calls. No injected script, squad, input or completion is used; full introduction, crossfade and Armory handoff remain open.
- [x] **K94 component:** additive native null-camera projection and actual380450 initializer pass683 new physical comparisons,503 prefixes, all35 null/three initializer sites, compiler/sanitizer checks, eight mutations and exact repeat. Existing277-case nonnull reference stays unchanged. Full typed x87 contexts are explicit; sparse ABI exports retain a required-owner boundary.
- [x] **K95 active-ABI component:** awareness binding passes all924 canonical cases through1812 transports plus35 unchanged physical caller packets, compiler/sanitizer and seven negative controls. It consumes genuine held GP arguments, retained FP and actual return words; no source argument prepeek or fresh policy is supplied.
- [x] **Default-camera initializer activation:** V8 iteration2 passes the actual fresh cold/checkpoint owner and renders six frames with authentic380450 initialization observed once. The World bootstrap marker is translated to the sealed binder's explicit bootstrap identity; the original CRT caller remains unchanged.
- [ ] **Null-camera live activation:** the new null-only World route is wired and component-qualified; the current introduction records zero null-camera calls, so actual live activation remains unobserved.
- [x] **K96 projection component/integration:** native2FB50/2FD70 and internal typed144090 pass original whole/prefix comparisons,535 sites, independent FP publication-order witnesses, GCC/Clang/sanitizers and nine rejected mutations. Both old producer bodies are retired; actual V8 rendering records12 native projection calls. Shadow144090 callers remain separately gated.
- [x] **K97 frame-policy component/integration:** native2C470 passes332 original cases with the paired query component,101 controlled prefixes, five faults, compiler/sanitizer checks and eight rejected mutations. Only the exact229B9E caller is routed; V8 records six calls. The25940 backend-query and shadow duplicate remain open.
- [x] **K98 transition-participant component:** native154F80/155190/1550B0/CE340 pass676 physical cases, all430 sites, compiler/sanitizer checks, ten rejected mutations and14 native compositions against28 actual caller packets. Production completion191750 integration remains open.
- [x] **K99 save-metadata component:** native1ACDE0/1AD350/1AD300/1ACFC0 pass1124 physical cases, all214 sites, eight actual faults, compiler/sanitizer checks, ten rejected mutations and13 actual191799 caller profiles. Genuine save/request/session orchestration and live activation remain open.
- [x] **K100 BSP admission transport:** the independent original caller trace distinguishes the owner-NONE request flag from the cluster/instance selector. Eight regression profiles now pass, including zero-alpha ordering, opposite prepared residency, failed-request dropping and mask allocation. The real cluster1/47 control rejected174 pages with blocking disabled; the original failure is retained as normal nonresidency. V9 iteration1 clears the actual failure and completes Heretic on hardware.
- [x] **K101 shadow inverse component:** exact TU1.5 C4879→144090 binding passes303 original frames/prefixes/faults, compiler/sanitizer checks, ten rejected mutations and exact repeat. Real C487E continuation and disjoint record/save/RET storage are required. Combined runtime duplicate retirement and captured-client activation remain pending.
- [x] **K102 initialized CRT component:** real-thread32C075/330179 lookup and retained seed update pass323 original cases,15 actual transition-caller packets, compiler/sanitizer checks, ten rejected mutations and exact repeat. This covers35 of50 source sites;15 postallocation sites, allocation/fatal services and production activation remain open.
- [x] **K103 Shadow duplicate retirement component:**147 production bodies remain after removing2C470/144090;146 unrelated bodies and all1420 address-map pairs stay byte-identical. The sole C4879 bridge is native; independent fresh12 physical policies preserve full GP/defined flags/count/stores through the test-only historical helper and compare native semantic data. GCC/sanitizers pass. Original optional12+3 fixture is unavailable and remains unqualified. Captured-client activation is pending in v9 iteration2.
- [x] **K104 session-support component:** native606A0/7E7F0/599B0 pass1034 physical cases, all102 sites,23 genuine caller packets, compiler/sanitizer checks, ten rejected mutations and exact repeat. Parent session/UI/network orchestration and runtime binding remain open; the address-query caller's live EDX obligation is explicit.
- [x] **K105 direct native frame-policy transport component/integration:** explicit prepared-source-call root domain consumes actual EAX view count, preserves opaque ABI/FP, and authenticates real60300000 RET after stores. Existing332-case suite plus180 original-data root transports and late/missing/wrong-return controls pass GCC/sanitizers; canonical literal229B9E binding/core stay unchanged. V10 iteration1 observes six actual native root calls in six GPU frames.
- [x] **K106 timer data component:** complete38-site five-record retirement algorithm passes253 original cases, ten rejected mutations, compilers/sanitizers, captured-input exact repeat and independent fresh-clock structural checks. Genuine private World Milliseconds producer binding and outer77AB0/77AF0 volatile EAX liveness remain open; no clock/default or completion result is invented.
- [x] **K107 timer caller component:** the typed return and all four actual callsite bindings pass253 state cases plus185 genuine caller profiles, GCC/Clang/ASan/UBSan, exact original replay and25 rejected semantic mutations. Actual EAX, late return-word reads and RET8 transport are retained. World clock routing and local duplicate retirement remain pending.
- [ ] **Shared HS awareness context:** provide the native script host's genuine retained floating-point authority before routing its public awareness commands. The qualified nested World binding alone does not complete this host gate.
- [x] **Natural Heretic handoff:** genuine authored completion loads catalog-selected Armory into a fresh world with Normal difficulty retained. V9 iteration1 observes both initial scenarios, real first-map completion and no supplied script/squad/input/completion.
- [ ] **Armory training/player control:** continue beyond the accepted first frame and qualify authored training, input, player control and progression.
- [ ] **New Game native closure:** replace the remaining generated camera/director and completion/save/request owners, then qualify the composed route without lifted runtime paths. Primary completion191750 retains15 calls, including real options/participant preparation154F80/155190, session64FD0 and request191580/UI5A080. The current catalog-driven host handoff does not qualify that full original lifecycle.
- [ ] Implement the two next actual v5 owners: behavior/query initializer `278460` and navigation selector `1C9F10`, with their complete real dependency graphs.
- [ ] **K20/K23 composition:** finish remaining native actor/order-reset dependencies and qualify the composed reset.
- [ ] Finish authored orders, full playback/speech, HS scheduling, map/save lifecycle, co-op and all final acceptance gates listed below.

Latest delivery evidence: [test-build receipt](../analysis/campaign-native-new-game-20261028/user-test-build-20261029.json), [actual delivered launcher](../analysis/campaign-native-new-game-20261028/user-test-build-packaged-menu-normal.json), [packaged Heretic](../analysis/campaign-native-new-game-20261028/user-test-build-packaged-intro-v2.json), [packaged Armory](../analysis/campaign-native-new-game-20261028/user-test-build-packaged-armory.json) and [incremental checkpoint19/cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-19.json). The requested retained package is separate from the unchanged normal launcher. The old V10 disposable source/build is removed.

Latest component evidence: [actual Armory GPU](../analysis/campaign-native-new-game-camera-20261028/campaign-machine-armory-v10-selected-v7-baseline.json), [actual packet lifetime/ownership](../analysis/campaign-native-new-game-camera-20261028/campaign-machine-packet-v10-selected-v7.json), [typed timer caller qualification](../analysis/campaign-native-session-timers-20261029/proof-v2.json) and [October30 actual timer World integration](../analysis/campaign-native-session-timers-20261030/world-integration/proof.json). These close the named checked milestones; full training and authored secondary-view activation remain open.

### October 30 continuation

The original conversation ended during timer World integration. Its later
[unchanged packaged training observation](../analysis/campaign-native-new-game-20261028/packaged-natural-training-v1.json)
completed Heretic naturally, rendered six Armory frames and then stopped at
the actual `1B0195 -> 1BBB50` AI descriptor callback. This is the next observed
Armory gameplay boundary; the earlier rendering smoke is not training acceptance.

- [x] **K108 component:** native `1BBB50` authored two-area query and the exact
  `1B0195` AX16/RET4 binding pass271 original calls/prefixes, all44 new-owner
  sites,538 typed/binding comparisons, GCC/Clang/sanitizers, eight meaningful
  mutation controls and an identical independent original repeat. The
  [current component proof](../analysis/campaign-native-ai-actor-callback-20261030/component-proof-v2.json)
  records324 observed closure sites and the authenticated descriptor/caller.
  Its450,605-byte original fixture is required input. All completed component
  scratch is removed; no binary, raw bank or capture is retained.
- [x] **K108 shared iterator/integration edit:** the existing first-area query
  now uses the same semantic iterator. All480 previous original cases,720
  typed/binding comparisons and five previous mutations pass GCC/Clang and
  sanitizers. The actual World route, main CMake module and read-only activity
  diagnostic are staged; current source pins are in the proof above.
- [x] Observe K108 through the real authored Armory continuation. The frozen
  [client cohort](../analysis/campaign-native-armory-20261030/cohort-v1/current-source-cohort.json)
  executes the native callback once during natural Master Guns birth, with
  zero script faults, then stops at `1B0D1C -> 1BBEC0` at tick10. Both shared
  area regressions pass. [Cleanup](../analysis/campaign-native-armory-20261030/cohort-v1/cleanup.json)
  verifies the finite source/build owner removed and launcher selections
  unchanged. This does not qualify complete Armory training.
- [x] **K106/K107 bounded timer World integration:** the
  [integration proof](../analysis/campaign-native-session-timers-20261030/world-integration/proof.json)
  records four actual77AB0 calls/64 native timer calls, adjusted wrap,
  capture/replay, cached/inactive behavior and exact local-body retirement.
  The real public map/session lifecycle precedes the anchor; the production
  clock boundary guard is unchanged. The original storage audit authenticates
  642 cold bytes, with exact native-factory ownership/checkpoint tests passing.
  Earlier failures remain recorded; all completed owners are removed.
- [x] **K109 geometry component:** six native direction/inverse/frame/difference
  owners pass1,143 original cases through all328 new-owner sites, including357
  prefixes and six real missing-source faults.136 disjoint completed private
  inverse-frame outputs, GCC/Clang/actual-core sanitizers, nine mutations and
  exact original repeat also pass. [Proof and cleanup](../analysis/campaign-native-ai-frame-geometry-20261028/proof-20261030.json)
  retain the234,809-byte canonical v2 input and preserve the initial diagnostic
  reference. Original native algorithms are unchanged. Numeric World routing,
  partial private outputs and actual1C9F10/navigation search remain open.
- [x] **K110 path-influence component:** native27C520 passes358 original
  calls/prefixes/faults through all98 root sites,174 completed private outputs,
  GCC/Clang/actual-core sanitizers, ten mutations and exact original repeat.
  [Proof and cleanup](../analysis/campaign-native-ai-path-influence-20261030/proof.json)
  retain the426,549-byte canonical v2 reference and initial diagnostic input.
  The frozen client compiles/links this and the shared frame module. Numeric
  World transport, full-stack/unmasked FP and complete navigation remain open.
- [x] **K111 area-choice component:** native1BBEC0/1BBBD0 authored selection
  passes877 original calls/prefixes/faults through all206 root sites, including
  score FP,32-candidate limit, mask reset/retry and mutable reads. GCC/Clang,
  actual-core sanitizers and nine mutations pass. [Proof](../analysis/campaign-native-ai-area-choice-20261030/proof.json)
  records canonical/diagnostic inputs and completed scratch cleanup. Both
  previous area suites pass full reruns with the shared zone accessor.
- [x] **K111 numeric binding:**421 original real CALL/RET8/argument/return
  packets, including411 interrupted prefixes, pass full FP/live values,
  retained writes and lazy input order. GCC/Clang/actual-core sanitizers,
  eight transport mutations and exact original repeat pass. The eight-byte
  original distance-penalty operand and real cold-factory regression pass.
  [Current proof](../analysis/campaign-native-ai-area-choice-20261030/proof.json)
  preserves the earlier component source/report and verifies completed cleanup.
- [x] **K111 bounded natural Armory integration:** the
  [cohort-v2 client](../analysis/campaign-native-armory-20261030/cohort-v2/current-source-cohort.json)
  executes both native area callbacks once, then reaches the next missing
  `1B0AD9 ->1BBEE0` owner at tick10. Both frozen iterations pass all five
  component/timer tests, with zero content/script faults. Full Armory training
  remains open. [Cleanup](../analysis/campaign-native-armory-20261030/cohort-v2/cleanup.json)
  verifies source/build removed, no checkout drift and launcher selections
  unchanged; no executable or frame is retained.
- [x] **K112 retained path-result component:** native27A460 cost, clearance,
  parent reversal and direction pass468 original calls/prefixes/faults,
  including all146 reachable root sites and a checked ordinary-frame invariant
  for two guard sites. GCC/Clang/actual-core sanitizers, seven mutations and
  independent original repeat pass. [Proof](../analysis/campaign-native-navigation-frontier-20261030/path-result-proof.json)
  records19,082 reads/2,143 stores, shared private points, the full353-case
  previous segment regression and completed cleanup. Numeric World routing,
  full search and documented exceptional/private-frame profiles stay open.
- [x] **K114 action identities:** native26C510 history,26C850/277D40 goal
  definitions and generic16E940 salted lookup pass354 original cases through
  all124 sites,1,570 ordered reads/148 stores, five true faults,56 rejected
  prefixes and four mutable timelines. GCC/Clang/actual-core sanitizers,
  twelve mutations and independent exact repeat pass. [Proof](../analysis/campaign-native-ai-actions-20261030/identity-proof.json)
  records canonical/initial references and completed scratch cleanup. Numeric
  World binding and whole action-planner composition remain open.
- [x] **K115 remaining path distance:** native200C00 and real shared frame
  geometry pass309 physical originals through all84 root/224 closure sites,
  12,781 ordered reads,162 rejected reads/five true faults and eight exact
  pending-quantity boundaries. GCC/Clang/actual-core sanitizers, nine mutations
  and independent exact repeat pass. [Proof](../analysis/campaign-native-ai-actions-20261030/path-distance-proof.json)
  verifies cleanup and the required407,411-byte fixture. Numeric World binding
  and retirement of the previous generated body remain open.
- [ ] **K113 next natural Armory owner:** implement whole1BBEE0 and its real
  action-selection/publication dependencies. The126-site root and initial
  200C00/26A2E0/26C3C0/26C510 graphs are authenticated. Existing native area
  choice and arena/resource owners are dependencies; missing action outcomes
  must remain fatal until their actual algorithms are implemented. K114/K115
  close native data dependencies only. The next movement/publication dependency
  audit authenticates15-site200960 and426-site200D30; full navigation-search
  and action-begin dependencies remain substantial and unresolved.
- [x] **K115 two numeric callers:** native200C00 transport for actual1B8938
  and1BBF3B passes1,092 original cases, including792 prefixes,10 true faults,
  14 late RET cuts,28 pending results and44,044 reads. Eleven mutations,
  GCC/Clang/actual-source sanitizers and independent exact repeat pass.
  [Proof](../analysis/campaign-native-ai-actions-20261030/path-distance-binding-proof.json)
  records real coordinate publications, all completed scratch cleanup and
  frozen compilation of the actual World/caller objects. The routes are wired;
  live execution, five other numeric callers and generated-body retirement
  remain open. Shared point/distance/result/area regressions all pass.
- [x] **K116 dialogue-envelope kernel:** native21F860 resolves actual
  sound/range/permutation resources and samples its normalized byte envelope.
  All70 original sites,454 physical cases,7,729 ordered reads,63 prefixes,
  three true faults, pending scalar/late RET and full x87/MMX preservation pass.
  [Proof](../analysis/campaign-native-audio-20261030/speech-envelope-proof.json)
  records actual-manager registration, GCC/Clang/sanitizers, eleven semantic
  mutations, exact original repeat and completed scratch cleanup. This is one
  dialogue dependency; full cutscene/SFX/dialogue audio, the still-closed
  forty-entry campaign gate and generated-speech retirement remain open.
- [x] **K117 dialogue-level smoothing:** native18F7A0 and its real generic
  object lookup pass218 physical original cases through all65 closure sites,
  including ordered faults, complete SIMD and x87/MMX preservation. Twenty
  unchanged original caller-fragment cases include absent and zero envelopes;
  all still execute smoothing. Twelve separate observer cases expose its
  additional diagnostic read and instrumentation failures. The actual root
  registry/module cohort passes GCC, Clang, ASan/UBSan, the454-case envelope
  regression and17 compiling semantic controls. The final header's text-only
  supplement passes3/3 ordinary CMake tests with an exact inverse-diff proof.
  [Scope and evidence](CAMPAIGN_NATIVE_SPEECH_SMOOTHING_TU15.md) retain actual
  caller GP/arithmetic-flags retirement and original operand limits. The
  generated body, full speech/facial/playback chain, forty-owner campaign
  audio gate and audio launcher acceptance remain open.

### October 30 presentation follow-up

- [x] **User-reported missing opening dialogue:** V12 restores the authored
  route and matches all36 nonmuted opening recordings in actual output.
  Most Arbiter/Prophet replies were inaudible in V7. A natural private-sink reproduction
  observes the actual Arbiter cue at1115 and Prophet reply at1304 decoded and
  consuming, but submitted at-6400 hundredths dB. The unselected V10 candidate
  binds the genuine native metadata provider and restores the authored direct
  route. Its [actual unmuted replay](../analysis/campaign-dialogue-20261030/actual-v10-package/capture.json)
  reaches5731 with those voices submitted at-400/-600 hundredths dB, then
  rejects a stale metadata owner during checkpoint Reset. Dispatch Reset before
  reading the replaced World owner, and verify actual output for all36
  nonmuted authored opening lines through the complete restore/handoff.
  Muted graphics captures and voice-queue diagnostics do not close this gate.
  V11's [full output check](../analysis/campaign-dialogue-20261030/actual-v11-package/audio-validation.json)
  passes the natural intro/Reset boundary without audio faults and matches35
  recordings before the next scenario. The remaining x01_0200_pcc crowd bed
  has an original/MCC authored-15dB gain; its mixed-output measurement remains
  unresolved. Preserve that authored gain and the35/36 red result while
  checking independent waveform intervals. V12's
  [actual output review](../analysis/campaign-dialogue-20261030/actual-v12-package/audio-evidence-review.json)
  matches all36 recordings at the unchanged0.25 correlation threshold. The
  quiet crowd bed passes the predeclared disjoint0–2s and4–6s interval method
  with the same independently inferred start; its actual native packet retires
  all395,520 source/output frames successfully. Natural intro/Reset is clean.
  The separate whole-utterance mixture experiment remains red on two unrelated
  controls and is not used to change that result. V12 is now selected by the
  verified test launcher; the later Armory crash and full DSP/Xbox parity stay open.
  The staged final V13 [actual-output review](../analysis/campaign-dialogue-20261030/actual-v13-package/audio-evidence-review.json)
  also matches all36 recordings with the same helper/thresholds, clean natural
  Reset, and successful full395,520-frame crowd packet retirement. Both tested
  interval lengths and the separate red mixture experiment remain recorded;
  no metric was tuned after capture. Its natural Armory continuation again
  stops at tick10 in the same missing1BBEE0 owner. V13 is now selected by
  `Halo2-Campaign-Test.sh`. Its [actual published-script run](../analysis/campaign-camera-20261030/current-final-v13-published-launcher-execution.json)
  reaches natural1454 through the real Normal menu, compares1,317 camera
  pairs without a mismatch, and reports no Vulkan errors. Its separate full
  [camera/Earth replay](../analysis/campaign-camera-20261030/current-final-v13-camera-earth-execution.json)
  compares4,057 camera pairs without a mismatch, reaches5304, and preserves
  the sampled Earth right-edge strip. The November1 user report and
  [corner-specific recheck](../analysis/campaign-camera-20261030/earth-corner-followup-r1/retained-corner-red.json)
  show that V13 still exposes the poster edge at the far upper-right corner:
  the old strip excluded those rows and is not full Earth-pan acceptance.
  These bounded checks leave the recorded
  Armory, complete native audio, hologram image-quality and whole-campaign
  gates open.
- [ ] **User-reported hologram dark patches/flicker:** the visible effect and
  command ledger are integrated, but image quality remains unresolved. The
  actual3400 frame shows the artifact. Three completed V10 frames in the
  [matched capture](../analysis/campaign-camera-20261030/hologram-depth-v10-before/capture.json)
  retain RGB, D24 and exact body/overlay inputs. Their21 matched cases per
  frame have identical WORLD positions and camera rows;4,902 triangle orders
  differ only by cyclic rotation. Determine the actual fragment-depth failure
  and restore the original primitive/W-depth path while preserving authored
  raster state. The later V12 correction is selected for its qualified improvement;
  the broader image-quality gate remains open.
  The [attachment control](../analysis/campaign-camera-20261030/hologram-depth-v10-before/head-depth-attachment-probe.json)
  reproduces22 self-covered pixels rejected by the current body/native-overlay
  depth pair; only changing the body to original ordered W reconstruction
  restores all22 with identical33,070-pixel coverage. The dedicated current-group
  implementation passes focused ownership/GPU tests. V12 passes all30 coherent
  CTests without skips and35 strict shader guards. Its
  [exact natural3400 comparison](../analysis/campaign-camera-20261030/hologram-depth-v12-after-current-single3400/matched-v2-qa.json)
  preserves all21 body/overlay input owners, WORLD position bits, original
  primitive order, camera and raster state; all21 attachment-proven live
  self-rejections clear with zero Vulkan errors. Two earlier natural captures
  missed3400 and remain recorded as red comparisons. Broader dark stippling
  remains visible; full image-quality resolution and Xbox framebuffer parity
  are still open.
  An independently checked immutable-cache/actual-ledger join recovers all11
  overlay index ranges, including strip degenerates and parity. The
  [controlled full-overlay coverage check](../analysis/campaign-camera-20261030/hologram-depth-v12-after-current-single3400/all-parts-actual-depth-coverage.json)
  seeds actual completed D24 and preserves it exactly; much of the residual
  dark pattern lies in passing coverage. Constant-color coverage does not
  qualify the authored pixel program. A later
  [actual input witness](../analysis/campaign-camera-20261030/hologram-stipple-hw-v12-visible/hardware-input-witness.json)
  finds full stipple level16 on all15,666 Regret/throne vertices; the shader's
  level-below16 discard cannot explain that captured pattern. The retained
  [authenticated TU1.5 image](../analysis/campaign-camera-20261030/original-hologram-reference-attempt-2.json)
  is a qualitative reference with a different pose, not framebuffer parity.
  The [paired body-attribute audit](../analysis/campaign-camera-20261030/hologram-body-attribute-conduit-audit.json)
  confirms a concrete MCC CPU-loader tangent/binormal swap: all17,553 packed
  basis words match the paired Original geometry, whose declaration assigns
  tangent before binormal. The staged MCC-only axes correction preserves
  Vista output and MCC float precision. Its reader oracle and genuine full-Cache
  regression pass: all41,706 packed components and physical FLOAT inputs match.
  The [fresh common-source natural-render A/B](../analysis/campaign-camera-20261030/hologram-body-basis-current-ab-final.json)
  preserves all23,840 nonbasis words across745 selected head vertices and
  witnesses exactly4,470 swapped basis words. All selected material/module
  joins pass. Actual shader phases differ, and scene-copy pixels and other
  critical inputs are unobserved, so causal RGB equivalence is unqualified.
  Both images retain the dark contour/stippling; no image improvement is claimed.
  A subsequent [CPU-only complete vertex-program comparison](../analysis/campaign-camera-20261030/hologram-full-vs-actual-report.json)
  evaluates both actual745-vertex head captures. Corrected axes reduce maximum
  lighting/vector errors from roughly0.71/1.40 to0.0010/0.0029, but the actual
  imported FLOAT inputs still fail the predeclared2e-5 tolerance against raw
  packed-basis inputs. A separate control that skins the actual packed basis
  matches all16 reference output registers bit for bit; scalar position error
  is already zero before the observed position prefix is retained. This is a
  CPU arithmetic-reference/input-path result, not GPU/NV2A precision or a pixel
  cause. No production precision change follows from that control.
  The bounded [pixel/scene-copy observation plan](../analysis/campaign-camera-20261030/hologram-next-pixel-scene-copy-plan.json)
  has now run in a fresh private coherent cohort. The
  [independently joined actual capture](../analysis/campaign-camera-20261030/scene-copy-actual-current-640-20261101/bounded-independent-review.json)
  identifies source frame4719/tick3400/group0/head section9 and the actual
  copy fence, mapped uniforms and selected native program. All307,200 target
  channels match the four-tap reference within the unchanged2-byte bound
  (maximum0.5000023); the main image has zero changed channels. Actual BODY
  stage3 binds the exact3992521855 normalization asset: six faces, seven mips,
  all42 uploaded rows and the authored/effective sampler match. Two authored
  pass candidates share those same asset/sampler inputs; this does not choose
  a new template/LOD. Eight focused CTests,35 shader guards and24 positive/88
  negative actual-device ownership controls pass, without changing any of the
  3,308 baseline shader modules. The private observer is unactivated. The dark
  pattern remains visible; the later binding observation below closes only
  actual BODY2D/overlay target identity. Fragment inputs and whole-pixel/
  Original framebuffer comparisons remain open. The
  [cleanup receipt](../analysis/campaign-camera-20261030/scene-copy-source-cohort/completed-cleanup.json)
  verifies removal of the completed source/build/capture owner and six
  superseded helpers; bounded reference captures and diagnostic sources total
  4,111,668 bytes. Test-only Vulkan1.3 fixture admission and the actual cache
  link dependency were imported with guarded preimages; no shader or production
  rendering behavior changed in that compatibility correction.
  The [second actual capture](../analysis/campaign-camera-20261030/hologram-bindings-capture-independent-review/report.json)
  joins source frame4439/tick3400, camera/group0/head9, all12,360 source hashes,
  the actual copy fence and overlay descriptor31/raw10/current upload epoch.
  Ten focused CTests,35 guards and all3,308 unchanged shader modules pass.
  The [native asset review](../analysis/campaign-camera-20261030/hologram-bindings-actual-current-640-20261101/body-native-asset-review.json)
  matches all BODY0–2 uploaded bytes/mips/formats/static frame0 and samplers to
  regret_head/skin_human/regret_head_bump under the captured LOD0/template0/
  implementation2. BODY0 is an uploaded texture, so the proposed accumulation
  readback is not activated for this head. Overlay stage0 binds the exact
  recorded target19. The independent pixel comparison again checks307,200
  channels (maximum0.5000015 within2) and zero main-image changes. The dark
  pattern remains visibly open; actual fragment samples/inputs/combiner and
  temporal behavior are the next boundaries, including overlay stages1–3.
  The private observer was not imported or published. Its
  [cleanup](../analysis/campaign-camera-20261030/hologram-bindings-source-cohort/completed-cleanup.json)
  verifies removal of2,752,205,401 source/build bytes and six completed helpers;
  required actual references/private source evidence total4,900,824 bytes.
  The initial bundler symlink failure and metadata-parent ownership correction
  are retained separately; canonical content stayed hash-identical and the
  successful baseline/diagnostic builds consume fully private44+4 data files.
  A new private [fragment observer component](../analysis/campaign-camera-20261030/hologram-overlay-fragment-witness-candidate/component-gpu.json)
  passes18 controlled RTX5080 cases across both menu shader variants. All
  RGBA bytes, meaningful24-bit depth and stencil remain byte-exact against
  unchanged baseline modules, with zero Vulkan validation errors. Default-off/
  pre-draw read refusal, exact fence/generation ownership and bounded overflow
  are checked. The inputs combine actual retained target19 with declared
  synthetic scanline/noise images and perspective geometry; they do not
  qualify authored overlay1–3 payloads or a live pixel cause. The separately
  [reviewed bank API](../analysis/campaign-camera-20261030/hologram-fragment-bank-api-independent-review/report.json)
  bounds the actual mapped448-byte upload mirror and selected module/variant
  identity, while leaving GPU completion and frame/group/command joins to its
  consumer. R4's successful diagnostic guard excludes temporal upscaling;
  its packet therefore retains zero raster offset and selects the ordinary
  menu variant. This is source-proved variant identity, while direct bank/
  module observation remains a separate gate. The fresh
  [actual bank capture](../analysis/campaign-camera-20261030/hologram-bank-source-cohort-r2/actual-bank-transport-acceptance-v2.json)
  now joins source frame4677/tick3400/group0 to all11 actual448-byte uploads,
  selected ordinary-menu module and33 overlay image bindings. Its coherent
  12,545-source cohort passes eight baseline and ten diagnostic CTests plus
  35 shader guards; all3,308 baseline shader modules remain unchanged. A
  strict pose-schema correction accepts the native ostream's integer1,
  preserving the earlier rejected result and rejecting booleans/other values.
  The [pixel comparison](../analysis/campaign-camera-20261030/scene-copy-pixel-review-r2/report.json)
  again checks307,200 channels within the existing2-byte bound
  (maximum0.501381) and zero main-image changes. The
  [native asset join](../analysis/campaign-camera-20261030/hologram-overlay-native-asset-join-r2/report.json)
  reproduces all33 selected frame/mip/upload/view/sampler tuples: authored
  scanlines at stages1/3 and noise at2, each with six mips. Actual frame0/count1
  is derived from the native animation owner at the captured group clock.
  The subsequent [exact-image component](../analysis/campaign-camera-20261030/hologram-overlay-exact-image-component-r2/component-gpu.json)
  passes20 unchanged-versus-observed GPU cases with those real payloads and
  bank, zero RGBA/D24/stencil differences and zero validation errors. Geometry
  and UV controls remain synthetic; its completed-frame depth seed does not
  establish the pre-overlay visibility frontier. The private
  [runtime API component](../analysis/campaign-camera-20261030/hologram-runtime-fragment-observer-r1/gpu-qualification.json)
  then passes12 native raw-geometry/program cases and156 lifecycle refusals,
  including a genuine pending submitting fence, with identical RGBA/D24/
  stencil and zero validation errors. The reviewed private integration then
  passes ten coherent CTests and35 shader guards. Its
  [fresh natural fragment replay](../analysis/campaign-camera-20261030/hologram-fragment-scalar-replay-r1/actual-r3-report.json)
  joins frame5177/tick3400/group0, all11 overlay draws and81 actual shader
  records. Every recorded sample/combiner replay agrees at rounded UNORM-byte
  precision (maximum error0 within the predeclared1-byte bound). Actual
  scene-copy comparison again passes307,200 channels within2 bytes and leaves
  the main image unchanged. The dark pattern remains visible. These records
  can include late depth failures; neither their append order nor their absence
  establishes visibility or blending. Actual BODY uploaded vertices and
  fragment/coverage ownership are the next boundaries. No production rendering patch,
  hologram defect resolution or new launcher build follows from these checks.
  The private [head upload witness](../analysis/campaign-camera-20261030/hologram-body-input-diagnostic-candidate/manifest.json)
  now captures the actual152-byte expanded vertex stream,3712-byte material,
  optional2544-byte mapped bank,128 pushed bytes and three selected shader
  identities. Its [capture helper](../analysis/campaign-camera-20261030/hologram-body-capture-helper-candidate/root-independent-review.json)
  preserves the combined final8MiB budget. Review caught and corrected a false
  rejection of the real material constants by normalizing the pipeline-control
  comparison exactly as Renderer does. The fresh private build passes ten
  CTests,35 shader guards and all3308 baseline/two observer module identities.
  Its native capture stops at tick48 before the requested frame; no completed
  head input or replay follows. The helper discarded native error facts when
  frame selection raised. A bounded failure-reporting successor passes40 CPU
  controls; actual cause diagnosis remains required. The failed source/build
  owner is removed and its small partial diagnostics are retained.
  The next frozen R6 cohort passes both Rust/C++ bridge checks, all ten
  focused tests,35 shader guards and all baseline/observer module checks.
  It stops before native startup because the canonical snapshot copier was
  edited by concurrent work after the cohort was frozen. No startup or head
  capture ran, and the entire source/build owner is removed. The next pipeline
  must guard the tools actually consumed by each phase while preserving the
  frozen source proof; this pre-startup refusal gives no native crash diagnosis.
  R8 then passes ordinary menu startup and naturally reaches tick3409 with
  native exit0 and no native/Vulkan errors. The capture reader rejects the
  actual two-word material/LOD identity because it expects a scalar, so BODY
  bytes and texture export are not accepted. Its
  [cleanup receipt](../analysis/campaign-camera-20261030/hologram-head-source-cohort-r8/execution/completed-cleanup.json)
  verifies source/build removal and1,085,594 bytes of required frame/diagnostic
  references. The [corrected reader](../analysis/campaign-camera-20261030/hologram-body-material-schema-r1/manifest.json)
  accepts the actual UInt32 pair and a genuinely absent unused vertex bank;
  all three joined SPIR-V modules must prove no set0/binding7 use. It passes306
  author CPU controls and18 independent controls; actual new capture remains
  required. R9 freezes12,841 files after preserving concurrent water and
  network changes. Its six bridge/ten focused checks,35 shader guards,3308
  baseline modules and ordinary startup pass. The natural head run again
  exits0 without native/Vulkan errors, then rejects a pipeline operand
  conjunction after accepting the material pair. Only1,035,182 bytes of
  frame/diagnostic references remain; its owner is removed. The lost full
  input cannot be reconstructed from the PNG. A source-based C++ witness
  demonstrates that compatible main/continuation passes can have different
  handles, invalidating the old universal handle-equality requirement. This
  does not establish which predicate failed in R9. The next private observer
  records actual pass creation operands and the corrected reader requires
  their strict compatibility. A separate bounded, explicitly unaccepted
  scene/BODY/module retention path supports offline diagnosis after future
  profile refusals. Those attempts did not qualify the full BODY capture or
  controlled replay.
  The [R11 cohort](../analysis/campaign-camera-20261030/hologram-head-source-cohort-r11/terminal.json)
  subsequently passes eight bridge tests, ten focused tests,35 shader guards,
  all3308 unchanged baseline modules and ordinary startup. Natural capture
  reaches3410 with native exit0 and validation0. The exact BODY input,
  three SPVs and four Cache texture exports are accepted; source/build/runtime
  scratch is removed. Required capture references total3,064,820 bytes.
  The [actual controlled GPU replay](../analysis/campaign-camera-20261030/hologram-body-replay-actual-r11-independent-review/receipt.json)
  joins frame3958/tick3400,347472 BODY bytes and the captured compatible
  render-pass creation operands. All nine profiles repeat RGBA/D24/stencil
  exactly within each profile on the RTX5080, with zero validation or cleanup
  errors. The retained baseline and diffuse-white PNGs total88,007 bytes.
  Alpha-test enable was already0 and all2244 stipple values already16, so
  those two controls changed no input bytes. Texture replacements change
  color; equal depth-change counts do not prove cross-profile depth equality.
  The BODY opaque state agrees with its authored pass, and the separate
  hologram override does not suppress the ordinary BODY draw. Actual posed
  basis/lighting were subsequently checked against actual GPU outputs: the
  [vertex observer](../analysis/campaign-camera-20261030/hologram-body-vertex-actual-r11-r1/report.json)
  records all2244 vertices without changing BGRA, canonical D24 or stencil.
  The fixed native source prefix matches all posed positions and basis bits;
  the complete55-instruction source program matches all stage coordinates
  exactly and its other fields within the declared tolerance. No vertex or
  pose discrepancy was found. Actual material choice, pixel samples and live
  composition remain open.
  This is capture/replay acceptance, not a hologram fix, temporal/Original
  framebuffer proof or new launcher publication.
  The [predeclared comparison](../analysis/campaign-camera-20261030/hologram-body-basis-ab-predeclared.json)
  requires actual selected materials, constants, pose and phase controls before
  assigning RGB differences to that correction. The axes patch is staged in
  V13, whose32 CTests and35 guards pass, and is now launcher-delivered.
  The broader artifact remains open.
- [ ] **November1 opening feedback:** the user confirms audible audio now works,
  but reports overly bright crowd NPCs, absent mouth movement, flickering
  black hologram faces and the Earth corner defect above. The latest supplied
  V13 log also confirms the same post-loading Armory tick10 failure below.
  The [speech lookup integration](../analysis/campaign-mouth-20261101/unit-speech-symbols-r1/root-integration.json)
  corrects only the two Original `phonemes` and `mouth_closed` lookup operands
  after an authenticated full-name join to the MCC cache. The original graph
  lookup and sampling routines remain responsible for their results. The
  [integrated CMake test](../analysis/campaign-mouth-20261101/unit-speech-symbols-r1/official-cmake-after-integration/report.json)
  executes and passes. Subsequent natural-opening observations find channel2
  and nonzero weights for all five recorded speech owners. A bounded
  [same-shot temporal capture](../analysis/campaign-native-audio-20261031/mouth-temporal-actual-r2-r1/root-visual-review.json)
  shows visible mouth/jaw movement for one speaking Prophet; this does not
  qualify every speaking character or Original timing/framebuffer parity.
  A later [19-frame Arbiter capture](../analysis/campaign-native-audio-20261031/mouth-arbiter-actual-r3-epoch1-wide-r1/root-visual-review.json)
  shows visible mouth/mandible movement in camera13, with79 real device-clock,
  channel2 and nonzero-weight observations. The earlier five-tick attempt
  retained only one completed frame and remains a failed collection.
  The first combined diagnostic run failed on its own fresh-output
  guard when the next scenario constructed another observer; its
  [cleanup receipt](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r1/execution/completed-cleanup.json)
  preserves that failed collection separately from the user's Armory failure.
  The [actual overlay GPU comparison](../analysis/campaign-camera-20261030/hologram-overlay-vertex-actual-r11-r1/report.json)
  preserves all rendered planes when observation is enabled, but finds WORLD
  bit differences at117 of745 source vertices and clip differences at163.
  The [ordered-arithmetic GPU successor](../analysis/campaign-camera-20261030/hologram-overlay-ordered-actual-r11-r1/report.json)
  matches all745 BODY WORLD and clip positions bit for bit with unchanged
  captured inputs. Its four-path catalog proposal passed the focused CTest
  in a private runtime. Actual live binding, visible flicker acceptance and
  new launcher delivery remain open.
- [ ] **TU1.5 Arbiter shading comparison:** the user's version-confirmed
  reference shows much stronger gold armor highlights. The [actual epoch6
  capture](../analysis/campaign-camera-20261030/arbiter-shading-actual-r5-epoch6-r1/capture.json)
  joins54 real draws to two completed images at1401/1402. The [input audit](../analysis/campaign-camera-20261030/arbiter-shading-actual-input-analysis-r1/report-r2.json)
  checks the joined lighting words, authored light-color literals,12 assembled
  shader-control routes, environment blend/depth states and eight actual bound
  carapace texture joins. These checks establish no shading correction or
  framebuffer parity. The seven-level normalization cube is a distinct
  external input; it is not a mismatched carapace texture. The source-pinned
  alpha chain and [bounded GPU observation plan](../analysis/campaign-camera-20261030/arbiter-shading-actual-input-analysis-r1/alpha-observation-plan-r2.json)
  target accumulation, destination alpha and environment contribution. A
  private diagnostic mip-row guard was corrected to count six cubemap faces;
  no shader brightness or blend behavior was changed.
  The [second user reference](../analysis/campaign-camera-20261030/arbiter-user-reference-20261102-r2/reference.json)
  adds an escorted-Arbiter view; its update version, source URL and console
  display mode remain unknown. The authored scene10 escort segment is a
  [candidate match](../analysis/campaign-camera-20261030/arbiter-reference-shot-match-r1/report.json),
  not an established exact frame. The private alpha diagnostic passes57 CPU
  controls across three profiles and an [eight-case GPU copy/resume probe](../analysis/campaign-camera-20261030/arbiter-alpha-gpu-diagnostic-r1/gpu-neutrality-root-r4-review.json).
  An actual renderer compile caught a local cleanup-type scope error; its
  reviewed correction now passes the R6 epoch3 full build, speech test and
  both focused CTests. The [later opt-in capture](../analysis/campaign-camera-20261030/arbiter-alpha-actual-r6-epoch7-r4-root-review.json)
  records a post-fence read of all1,080 torso vertices,152 bytes each,
  matching the ordinary CPU world upload. Its three depth snapshots yield
  zero accepted same-depth interior samples, and the completed1402 companion
  frame is absent. The capture remains rejected; no alpha metrics, shading
  correction or Xbox parity follows. Required compressed diagnostic inputs
  total137,128 bytes; its private scratch is removed. The [late escort image](../analysis/campaign-camera-20261030/arbiter-late4550-actual-r6-epoch3-root-r1/capture.json)
  still shows weak gold reflections; an exact Original frame match is open.
  A later [depth-only diagnostic](../analysis/campaign-camera-20261030/arbiter-alpha-source7-failure-audit-r1/depth-diagnostic.json)
  identifies the classifier's reversed facing sign. The reviewed correction
  follows Vulkan's negative shoelace area and finds1,980 exact depth matches
  in the retained region. It changes diagnostic classification only; the old
  capture remains rejected and no color/alpha metrics were accepted. New
  one-frame alpha and exact3001 Council capture adapters are CPU-qualified
  and independently reviewed, awaiting the R7 build and idle capture lease.
- [ ] **Actual post-cutscene crash:** the user's log reaches01a_tutorial and
  fails at tick10 in actual root1D0670, callback1B0AD9→1BBEE0. The tight native
  Armory reproduction matches its actor, descriptor and GP/stack packet.
  A read-only observer of the actual natural handoff establishes actor byte40
  is zero: Original1BBF1B branches to the early return at1BC094 before mode4
  selection/publication. The [corrected native early owner](../analysis/campaign-native-ai-actions-20261030/action-root-early-integration-r2/report.json)
  passes focused tests and independent review. The [actual R3 replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r3/execution/captures/capture-001-epoch-001/root-review.json)
  passes that original boundary and reaches the next missing callback,
  1B0F56→1BC0A0. Its [complete14-site native owner](../analysis/campaign-native-ai-actions-20261030/action-armory-post-callback-r1/report.json)
  passes both association branches and focused tests; private epoch2 runtime
  validation is pending. Verify the natural handoff into controllable gameplay; deeper
  nonzero actor routes still require their genuine dependency closure.
  The later [R4 epoch2 natural replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r4/execution/captures/capture-002-epoch-002/capture.json)
  passes those early callbacks and the real workspace/primary allocator path,
  then exits at Armory25 on actual26B2AC→1D2CF0. Collection success is separate
  from that runtime failure. Its expired source/build owner is removed;
  [reference cleanup](../analysis/campaign-camera-20261030/opening-five-issues-r5-earth-rebase-r1/inventory-and-R4-cleanup.json)
  verifies21,028,079 bytes of required bounded replay references.
  The [native Origin/marker/plan continuation closure](../analysis/campaign-native-ai-actions-20261030/action-root-origin-binding-r3/report.json)
  has1,303 retained comparisons and128 hostile FP controls per compiler
  profile, plus independent source reviews. The [fresh R5 admission](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/refresh-001/root-review.json)
  composes35 exact imports while preserving newer canonical changes. Its
  epoch3 build and both focused CTests pass. The [actual R5 natural replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/execution/captures/capture-002-epoch-003/capture.json)
  passes the Origin boundary and reaches Armory25, where actual26A33E→26BA30
  remains an unowned collection call. This is a collected runtime failure,
  not a completed handoff into controllable gameplay. The next native
  collection/selection closure and real-memory RMW capability pass three
  compiler profiles and nine compiled negatives. The [14-file epoch4 admission](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/refresh-004/root-review.json)
  passes the coherent official build and speech test. Its [natural replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/execution/captures/capture-003-epoch-004/root-review.json)
  passes collection and reaches actual candidate count3, then refuses the
  genuine nonzero selector continuation26A3F6→26A42E at Armory25. The Plan
  policy bytes are not yet observed. Its9,100,147 bytes of required bounded
  references are retained and raw capture scratch is removed. The crash and
  controllable-gameplay acceptance remain open. V13 remains selected by the
  campaign test launcher.
  The [epoch6 admission](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/refresh-006/root-review.json)
  adds the qualified nonzero selector, policy, filter, real DWORD RMW adapter
  and workspace initializer. Its official build and speech test pass. The
  [actual replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/execution/captures/capture-005-epoch-006/root-review.json)
  instead reaches an earlier policy dependency at275F31. Authenticated call
  bytes identify275BA0, returning to275F36;275CB0 is a later conditional call.
  The filter/RMW/initializer branches are not live-qualified by that replay.
  The private cohort's finite three-hour owner has expired and its source,
  build and exact owned processes are [verified removed](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r5/execution/completed-cleanup.json).
  Its five bounded replay collections retain39,991,899 bytes of required
  diagnostics; selected V13 remains unchanged. Fresh R6 is frozen for reviewed
  rebasing and current-input qualification before another full build.
  The [R6 actual replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r6/execution/captures/capture-001-epoch-001/root-review.json)
  passes275BA0 with an actual nonzero AL and reaches275F3E→275CB0,
  returning to275F43. Subsequent native parent and object-point continuations
  pass the composed build and focused tests. The [latest completed replay](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r6/execution/captures/capture-006-epoch-007/root-review.json)
  reaches actualE2365→E04B0, returning toE236A. The native19-site E04B0
  predicate and seven caller phases pass the epoch8 full build and speech
  test, but its replay expires before the handoff. No actual AL outcome or
  controllable-gameplay acceptance is supplied. The finite R6 owner and exact
  owned processes are [removed](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r6/root-cleanup.json);
  six completed replay collections retain58,586,097 bytes of required
  references. The R7 continuation carries exact epoch7 source files and raw
  links into a new owned source tree, with no build or acceptance transferred.
  Its [first reviewed refresh](../analysis/campaign-camera-20261030/opening-five-issues-cohort-r7/refresh-001/root-admission.json)
  admits the same E04B0 closure; a fresh full build is active. V13 remains
  launcher-selected.
  The [35-site BCDF0 lookup](../analysis/campaign-native-ai-actions-20261030/action-object-datum-lookup-abi-r1/report.json)
  and [13-site E22C0 reset](../analysis/campaign-native-ai-actions-20261030/action-object-reset-abi-r1/report.json)
  now pass bounded raw-ABI component tests and independent source reviews.
  Each preserves real memory/stack reads and ordered stores; no new Original
  execution or comparisons were performed. Their caller compositions and
  actual runtime branches remain separate, unfinished gates.
  Five [private native safety dependencies](../analysis/campaign-native-ai-actions-20261030/safety-components-root-review-v3.json)
  (269CA0/269BF0/269B60/269A60/269940) pass7,315 unchanged original cases,259 reachable sites, exact ordered
  reads/stores, raw byte returns, SIMD status/lanes and original x87/MMX
  preservation. GCC, Clang, separate-core sanitizers,38 semantic controls and15
  immutable-table rejection controls pass. The new269940 component preserves
  both original argument-cell publications in the retained alias domain and
  the pending extended quantity at a rejected conversion store. Its pinned
  four-byte commit also survives a forced observer allocation failure after
  publication. It requires the actual next extended tag empty; two genuine
  occupied-tag controls are refused, and four source-proven unreachable sites
  are not forced into coverage. These are unactivated typed components; numeric caller transport,
  genuine selection/publication and controllable Armory remain open.
  The [private26C1E0 candidate constructor](../analysis/campaign-native-ai-actions-20261030/action-selection-candidate/goal-candidate-report.json)
  also passes572 original calls/prefixes through all64 root/288 closure sites,
  composing real native goal-definition and frame-projection dependencies.
  Ordered data/partial stores, MXCSR, raw return and source aliases match;
  GCC, Clang, separate-core sanitizers and five semantic controls pass. Its
  [operand audit](../analysis/campaign-native-ai-actions-20261030/action-selection-candidate/goal-candidate-observer-read-audit.json)
  authenticates a missing TEST-memory read annotation in the offline observer.
  Numeric GP/SIMD/private-frame transport, data ownership and activation remain open.
  The [private26C740 actor lookup](../analysis/campaign-native-ai-actions-20261030/action-selection-candidate/script-actor-lookup-report.json)
  passes125 physical original calls/prefixes,81 closure sites and1,080 ordered
  reads, using the real native267A30/267A40 hash/equality and140DB0 bucket
  chain for the four-byte payload owner. GCC, Clang, sanitizers and five
  semantic controls pass. Its observer authenticates zero-count REP as
  performing no memory access; original instruction bytes are unchanged.
  Numeric REP/private-frame transport, stack aliases, caller integration and
  action-selection activation remain open.
  The [private26C0B0 goal filter](../analysis/campaign-native-ai-actions-20261030/goal-filter-candidate/root-review.json)
  composes the real goal definition, actor lookup, point projection and native
  distance arithmetic. All433 original calls/prefixes,98 root sites and422
  reachable closure sites match, including the nonpopping FST to the original
  goal argument and its later reread. GCC, Clang, separate objects, sanitizers,
  fourteen semantic controls and a reserved-store allocation-failure control
  pass; the shared production vector owner is unchanged. Actual caller
  argument/stack ownership, private-projection aliases, four specific empty
  FP tags, numeric transport and selection activation remain open.
  The [private area-reference append](../analysis/campaign-native-ai-actions-20261030/area-references-candidate/root-review.json)
  composes real2A78A0/202AB0 and matches266 original calls/prefixes, all130
  sites, signed/wrapping count publications and single-instruction two-byte
  RMW observation order. Compiler/sanitizer checks and thirteen semantic
  controls pass. The admitted two-word seen workspace requires source-phase
  index0..63 authority; no clamp or inferred512 bound is supplied. Real
  workspace/alias/lifetime/RMW binding and whole enumeration remain open.
  The private [26BBD0/26BE90 enumeration replay](../analysis/campaign-native-ai-actions-20261030/enumeration-candidate/existing-reference-native-wide-lifetime-report.json)
  now passes1,817 retained physical reference cases (199 complete and1,618
  prefixes),163,402 semantic reads and68,049 semantic writes/publications.
  GCC, Clang and separate-unit sanitizers pass; all26 native memory-contract
  controls and eleven mutants pass. The exact four-byte lookup copy commits
  before fallible observation, and wide-load span permission is revoked on its
  final call-data release while retained bytes survive. The prior stale-span
  acceptance and broader JSON-allocation failures remain recorded as red.
  This is an explicitly projected data proof: coverage is1,011/1,024 sites,
  full numeric boundary comparisons are zero, and activation is false.
  Original alias/access-fault witnesses, remaining composed coverage, caller
  bounds/lifetimes and the live adapter stay open. The native-only continuation
  used the unchanged7,412,198-byte reference after automated review stopped
  further original-code execution; no tracing was retried. Completed scratch
  owners are removed, and the independent lifetime review preserves these limits.
  The [private200960 target reset](../analysis/campaign-native-ai-actions-20261030/target-reset-candidate/report.json)
  matches165 physical original calls/prefixes across all15 sites,320 ordered
  reads and952 stores, including every before/after store boundary, mutable
  pool/global aliases and the held actor-row result. GCC, Clang, sanitizers
  and nine semantic controls pass; its28,310-byte compressed reference is
  retained and completed scratch owners are removed. Numeric caller/private
  frame transport, genuine200D30/publication closure and Armory activation
  remain open.
  The bounded [200D30 publication dependency audit](../analysis/campaign-native-ai-actions-20261030/publication-dependencies/audit.json)
  authenticates426 sites,32 calls,26 direct callees and all six switch targets.
  Nine existing semantic component sources match their selected proofs; two
  private candidates are separate, and fifteen direct entries have no
  established semantic owner in that audit. Actual27B340 search under27B2B0,
  navigation preparation27A640 and request construction2A8440 remain major
  dependencies. Four shared arena-integration files have drifted from their
  proof epoch and require refreshed integration validation. No publication or
  gameplay success is supplied by this inventory.
  New private native data work remains unactivated. The
  [27B0D0 query-target publisher](../analysis/campaign-native-ai-actions-20261030/query-target-candidate/report.json)
  has168 authored native controls across three compiler groups and eight
  rejected mutants; no independent Original whole-leaf cases exist in the
  declared retained search. The real471-site27BD30 neighbor implementation
  composes existing native218780 frame geometry and278E60/FF1A0 jump owners,
  retaining their410 Original child comparisons separately. Its
  [explicit physical-tag variant](../analysis/campaign-native-ai-actions-20261030/path-neighbors-tag-authority-candidate/report.json)
  passes346 authored controls and eleven mutants, including all eight TOP
  rotations and the actual five next physical x87 tag slots. The independent
  review finds no new defect; actual caller tag authority and whole-parent
  Original/numeric/fault proof remain open. The forced ring can append row64
  and return65, so a guessed global64 bound is not supplied. The
  [E9600 object-component word candidate](../analysis/campaign-native-ai-actions-20261030/object-component-word-candidate/report.json)
  adds a genuine typed caller-local value and optional retained-word output:
  228 authored native controls per compiler group and nine compiled mutants
  pass. Static bytes authenticate all nine leaf sites and the real200DFD
  caller; unchanged Original leaf/caller comparisons and private-output binding
  remain open. Completed native/static scratch is removed for all three.
  The private [202970 special-link predicate](../analysis/campaign-native-ai-actions-20261030/special-link-candidate/report.json)
  reuses the existing186E70 collision bitmap owner for its exact byte-channel
  186E30 profile. Its1,328 authored controls per compiler group and nine
  compiled mutants pass; the reused owner's187 Original cases remain child
  evidence. An independent source review verifies the signed chain, first-kind7
  policy, lazy byte/DWORD inputs and negated bit result. Whole202970/search
  Original comparisons and caller binding remain open, with no activation and
  all three owned static/native/seal scratch paths removed.
  The private [218C40 distance/cost interface](../analysis/campaign-native-ai-actions-20261030/distance-root-candidate/report.json)
  preserves the extended square root across its rounded distance publication,
  lazy prior-cost DWORD read, addition and influence BYTE read. Typed owned
  points reuse the existing projection body. All299 authored controls per
  compiler group,778 point/910 query-start child cases and five compiled
  mutants pass; actual next-slot tag authority is explicit. Independent
  whole-leaf/search comparisons, private publication/alias and partial
  projection fault transport remain open. All four completed scratch owners
  are removed. Whole search also needs genuine inherited upper-word bytes:
  the source writes a WORD neighbor kind and later copies its whole DWORD,
  and similarly copies a private framed point's fourth DWORD after a WORD
  frame publication. Those inherited bytes cannot be guessed or zero-filled.
  The private [owned27C520 interface](../analysis/campaign-native-ai-actions-20261030/path-influence-owned-candidate/report.json)
  now passes268 authored controls per compiler group and six compiled mutants,
  with358 retained influence/1,143 frame/778 point child regressions. Typed
  endpoint inversion and the actual next physical x87 slot are explicit;
  the new API's actual-tag Original comparisons and caller activation remain
  open. Its completed scratch is removed.
  The [whole504-site27B340 semantic search](../analysis/campaign-native-ai-actions-20261030/whole-search-candidate/report.json)
  is now implemented privately with the actual heap, neighbor, framed-point,
  difference, influence and special-link owners. The frozen49-file/21-unit
  cohort passes1,016 authored checks per GCC/Clang/ASan+UBSan group, including
  860 source-memory fault prefixes and96 FP profiles; ten compiled wrong
  variants are rejected. The same cohort replays778 point,1,143 frame,410
  jump and358 influence child cases, including the typed112-frame subset.
  Source order, single-WORD decrement ownership, signed indices/priorities,
  full-DWORD heap copy, collision/update behavior and inherited-byte refusal
  remain explicit. Authored noncanonical address cases do not establish real
  caller allocation consistency. Whole-search Original/caller comparisons
  remain zero; production storage/RMW, padding/tag authorities, partial caller
  FP/alias transport and activation stay open. The
  [next-owner inventory](../analysis/campaign-native-ai-actions-20261030/query-wrapper-next-owner-inventory/report.json)
  authenticates the distinct40-site27B2B0 dispatcher,117-site27B100 query start
  and336-site27A640 result materializer. The dispatcher needs a real retry
  WORD increment owner and fresh count read;27A460 does not replace27A640.
  The subsequent [private27B2B0/27B100 composition](../analysis/campaign-native-ai-actions-20261030/query-wrapper-candidate/report.json)
  implements that dispatcher and query-start entry-tag/pending interface.
  Three compiler groups pass165 wrapper controls, including78 sampled memory
  prefixes,12 retry-RMW controls and32 FP profiles, plus214 query-start tag/
  pending controls and the unchanged910-case legacy child regression. The910
  additional paired contexts use explicitly authored entry tags; they do not
  establish Original full-tag authority. Seven wrapper and five child mutants
  are rejected, and an independent review verifies source/read/store/FP order.
  The wrapper preserves the actual2048-DWORD forward hash fill, held MAX,
  conditional real search and single-WORD retry increment. Actual DF, storage/
  padding/tag authority, Original whole-wrapper/caller and27A640/200D30 live
  composition remain open. Its eight completed owners are removed; no large
  artifacts or production changes are retained.
  A [separate policy extension](../analysis/campaign-native-ai-actions-20261030/whole-search-policy-controls/report.json)
  keeps the sealed search cohort unchanged and checks real Type2 zero/nonzero
  subtype depth, Type5 penalty/depth and forced-ring width equality/rejection.
  All five authored cases and six targeted mutants pass three compiler groups,
  alongside the unchanged1016-case cohort and retained child regressions.
  Its helper-only initial type-ledger failure is retained; source algorithms
  are unchanged and all completed extension scratch is removed.
  The [27A640 dependency inventory](../analysis/campaign-native-ai-actions-20261030/path-materializer-owner-inventory/report.json)
  keeps its six distinct callees separate. The private
  [1B0E10 three-slot writer](../analysis/campaign-native-ai-actions-20261030/route-slot-candidate/report.json)
  now authenticates all51 sites and passes201 authored controls per compiler
  group,144 memory/permission prefixes and ten compiled mutants. It preserves
  the held full-DWORD priority read, first-free override, signed priority/tie
  policy and six interleaved WORD stores/lazy input reads. Independent review
  finds no concrete defect within its semantic AL-only scope. Original/caller
  comparisons and live ownership remain open; all four completed owners are
  removed, with no production changes or large artifacts retained. The real
  27AAA0/279100/2CBEE0/2CE030 algorithms remain separate required route work.
  The genuine70-site [2029E0 edge iterator](../analysis/campaign-native-ai-actions-20261030/route-edge-iterator-candidate/report.json)
  now passes370 authored controls per compiler group,103 fault prefixes and
  ten rejected mutants. Its signed WORD traversal budget and ordered link/
  edge publications remain literal; no extra traversal cap is added to the
  native owner. The [279100 inventory](../analysis/campaign-native-ai-actions-20261030/route-link-endpoints-candidate/report.json)
  authenticates the real model/collision dependencies and the caller's full
  DWORD link read at node+14; its inherited upper WORD cannot be fabricated.
  Original/caller proof and the parent route remain open.
  The private [2185E0/1F7E00 object-frame dependency](../analysis/campaign-native-ai-actions-20261030/route-object-frame-candidate/report.json)
  passes55 authored cases and55 fault prefixes in three compiler groups;
  [27A310 exclusion lookup](../analysis/campaign-native-ai-actions-20261030/route-contact-children-candidate/report.json)
  passes117 authored cases, including maximum signed count and repeating
  sign-extended mask bits. Independent source reviews preserve their typed
  return/output limits. The genuine [276090 segment-contact dependency](../analysis/campaign-native-ai-actions-20261030/route-segment-contact-candidate/report.json)
  composes202970/186E70 and passes42 authored cases and256 before/after fault
  prefixes per compiler group. Root review exposed an FP/read-order defect:
  COMISS must precede the Output-owner read, even when that read faults. A new
  regression fails the old source, passes the correction and rejects a mutant
  restoring the bug. Actual private-cursor2762A0 composition, whole caller/
  Original proof and Armory progression remain open; these are unactivated.
  A subsequent [whole2762A0 traversal candidate](../analysis/campaign-native-ai-actions-20261030/route-segment-traversal-candidate/report.json)
  composes all three genuine children through typed cursor, point, contact and
  association owners. Its55 authored cases,2048 fault prefixes,80 FP profiles
  and32 mutants pass three compiler groups from private pinned source copies;
  the previous child regressions also pass. An
  [independent source review](../analysis/campaign-native-ai-actions-20261030/route-segment-traversal-candidate/root-independent-source-review.json)
  confirms the stated typed-data and masked-FP boundaries.
  No private stack addresses, extra traversal caps or supplied collision
  outcomes are introduced. Whole279100/model/collision, independent Original/
  caller proof, production binding and the Armory crash remain open.
  No Armory or launcher success is inferred. All completed compiler/static/
  seal scratch is removed, with no new large artifacts retained.
  The [202660 area classifier and six real children](../analysis/campaign-native-ai-actions-20261030/route-collision-area-candidate/report.json)
  add307 actual sites,107 authored cases,154 fault prefixes and61 FP profiles
  in three compiler groups, with34 rejected mutants. The
  [146-site pose descriptor dependency](../analysis/campaign-native-ai-actions-20261030/route-pose-descriptor-candidate/report.json)
  implements the actual signed strided search, hierarchy and distinct ID
  normalization rules. Root source reviews confirm both private components;
  actual caller/Original proof, production binding and the crash remain open.
  The actual101-site pose iterator and279-site pool composition also pass
  private compiler controls and source review. The next
  [448-site resource allocator composition](../analysis/campaign-native-ai-actions-20261030/route-resource-allocator-candidate/report.json)
  exposed a genuine ring-tail defect during independent review: undersized
  candidate windows must survive for later gap accumulation. The corrected
  algorithm passes the [independent three-mode regression](../analysis/campaign-native-ai-actions-20261030/route-resource-allocator-independent-review/report.json);
  preserved buggy source fails all three. This remains a private DATA profile
  with ready callbacks. Actual asynchronous retirement, caller/Original proof,
  production activation and the Armory crash remain open.
- [x] The separate camera-v2 candidate passes its three CMake tests and an
  actual New Game/Normal replay through tick5300 with subtitles enabled.
  All6,533 native camera frames preserve the source pose; the within-animation
  cut and Earth right-edge regressions pass. Original animation cadence and
  wider-PC framing limits are explicit in
  [the camera report](CAMPAIGN_CAMERA_CUTS_20261030.md). The normal selected
  runtime remains unchanged.
- [x] **Original-mode Regret hologram and test delivery:** final-v7 passes
  thirteen CTests and the natural opening through tick5302 with zero Vulkan
  errors. All4,427 camera frames preserve the source pose; the Earth gate
  passes and the actual tick3400 frame visibly shows Regret's authored
  hologram. The source-ordered ledger covers1,095 scene copies and12,045 raw
  overlay draws, retaining source ranges separately from submitted triangles.
  `Halo2-Campaign-Test.sh` and the original bare launcher select this immutable
  candidate; its real menu/Normal/1453 replay observes the exact ELF and passes
  camera/overlay checks. [Delivery](../analysis/campaign-camera-20261030/final-v7-delivery.json)
  records that historical selection. The newer
  [V12 delivery](../analysis/campaign-camera-20261030/current-v12-delivery.json)
  selects immutablebc46f8e2 through the same `.sh` entry point. All30 CTests
  and35 shader guards pass; the complete natural5302 replay preserves5,800
  source camera frames, Earth framing and the ordered overlay ledger. Actual
  `.sh`/Normal launch independently observes that ELF through1455 with zero
  camera mismatches or Vulkan errors. The newer
  [V13 delivery](../analysis/campaign-camera-20261030/current-final-v13-delivery.json)
  selects immutable064c1d54:32 CTests/35 guards, actual36-clip audio output
  through Reset,4,057 full camera/Earth comparisons and1,317 actual `.sh`
  camera comparisons pass. Its owned source/build and unselected A/B packages
  are removed; the requested V13 package and required references are retained
  with paths/sizes in the cleanup receipt. Whole campaign/audio/native-only/Xbox
  framebuffer gates remain open.
- [x] **pStencil cold backing and resident implementation boundary:** final-v7 and pre-hologram
  camera-v2 both fail before frame1 while reading49219C. Pinned TU1.5 proves
  four omitted loader-zero writable banks totaling180bytes. V8 adds only
  those exact per-world owners and passes fourteen CTests, including fresh
  ownership, writes, checkpoints, isolation and24 foreign-overlap controls.
  Actual V9 clears that read and exposes a logical Cache/native resident
  implementation-pointer comparison in projected lighting; all306 resident
  bytes match. V11's fresh current-World pass-owner correction and captured
  tuple regression pass the coherent16-test cohort with no skips. Its
  [actual pStencil replay](../analysis/campaign-camera-20261030/pstencil-v11-resident-positive/capture.json)
  uses real Normal menu selection and renders86 campaign frames through40
  ticks, with zero script faults/Vulkan errors and the exact packaged image
  observed. Both former pre-frame failures are cleared. Full pStencil opening,
  hologram image quality and Armory progression remain separate gates. V13
  is now the delivered Original-mode test build; full V13 pStencil playback
  is not established by the bounded V11 check.

## Current finite work queue

| ID | Capability / exact original owner | Implementation | Original proof | Caller integration | Next action |
| --- | --- | --- | --- | --- | --- |
| K01 | Actor behavior callback `1BE5B0`, required Marine boundary | Native module implemented | 171 combined AI cases; GCC/Clang and root normal/sanitizers pass | Actual MCC Marine/Grunt encounters reach native callback and continue to next missing owner | Area closure passed; close actual next `278460` behavior initializer |
| K02 | Actor/frame reset `1BAE40`, required 563-tick boundary | Native module implemented | Same combined proof, all 42 original sites exercised | Actual MCC Johnson/Elite/Grunt encounters pass the old boundary into BSP5 | All three passed; close actual `278460` / `1C9F10` boundaries |
| K03 | Authored `ai_living_count` / evaluator `2AF6A0` and count owner | Native query and script caller implemented | 146 physical count/evaluator/prefix cases; GCC/Clang, sanitizers, mutation and caller compile pass | Script host wired; global HS FP policy/first-dispatch and live acceptance still open | Preserve explicit policy; authenticate broader ownership before adding state |
| K04 | Sound-class fades `228310` | Native typed 54-record module implemented | Combined 262 physical audio calls; GCC/Clang ASan/UBSan and mutations pass | Campaign registry remains closed | Verify callers as remaining audio group closes |
| K05 | Voice routing masks `225D70` | Native typed routing module implemented | Same proof; 14,632 combined ordered writes | Campaign registry remains closed | Same; do not reconstruct dead caller scratch |
| K06 | Cinematic screen stop/reset `1FCB0` and capture ack `1FCE0` | Native module, direct caller and entry transport implemented | 58 physical calls/prefixes; GCC/Clang ASan/UBSan; reset/ack and transport mutations pass | Executor/publication guards retained; original-address routes select native kernels | Full timed-screen/DOF/controller and composed world acceptance remain open |
| K07 | Native screen stop/ack execution journal | Implemented; IDs 33/34 append without changing earlier IDs | Normal and sanitizer tests pass with real kernels; malformed/reordered/untracked work rejected | Actual world scopes and decoder wired | Composed whole-world replay acceptance remains open |
| K08 | Campaign ducking clock `129500` and native dependencies `33B590`/`12E9A0` | Implemented as typed clock/ducking/activity algorithms | 1,024 physical original calls; GCC/Clang ASan/UBSan, interleaved reads/writes and three mutations pass | Candidate registered; global campaign group remains closed | Continue missing owners and qualify actual caller graph |
| K09 | Authored swarm/nonswarm counts and strength through `27DC40` | Implemented with living-count shared native traversal | All four names: 492 cases, physical repeats, GCC/Clang/sanitizers/mutations and caller compile pass | All four native ScriptVm host routes wired | First-dispatch/full HS and mission acceptance remain open |
| K10 | Authored `ai_fighting_count`, `27DDA0` / `1EA090` / `1AFBD0` | Native combat counters and actor predicate implemented | 246 original query/evaluator/fault cases; exact repeat, GCC/Clang ASan/UBSan and actual boundary mutations pass | ScriptVm host route and actual caller objects compiled | Current source encounter and full HS acceptance remain open |
| K11 | Actor/order script association `26C600` and retained hash ownership | Native erase/find/insert/hash/equality implemented | 90 physical transition/fault cases; exact repeat, GCC/Clang ASan/UBSan and free-list mutation pass | Included in build; complete orders remains unavailable | Close actual actor-plan/behavior/script reset dependency frontier |
| K12 | Full timed-screen update `1FD00` and DOF setup `1FC10` | Native semantic controllers, bindings and compatibility API implemented | 844 physical calls/prefixes; native/transport/API, GCC/Clang, real ASan/UBSan and five mutations pass | Guarded native routes and current caller objects compile; old four-body screen module removed from production graph | Qualify composed screen/sequence runtime |
| K13 | Playback fade request/sample `129900` → `12D320` → `12DB00` / amplitude conversion | Native closed dependency implemented; old `12DB00` register body removed | 888 physical calls; ordered reads/stores, GCC/Clang ASan/UBSan, three mutations and focused CMake tests pass | Four required audio entries qualified; full playback drivers and campaign audio gate remain closed | Continue actual `12AB30` / `12ADE0` dependency frontier |
| K14 | Required order-reset camouflage `1EB590` → `D3340` / `B7E50` | Native actor/unit control and actual dirty-entity publication implemented | 174 physical cases, all83 sites, GCC/Clang ASan/UBSan, original repeat and dirty-publication mutant pass | Compiled dependency; complete actor reset/orders remains unavailable | Compose actual reset after unit actions, combat and behavior dependencies close |
| K15 | MCC Marine/Grunt boundary `1B6F20` | Native `1C5870` area-query closure and behavior selector implemented | 480 original cases,337 profile-reachable sites; input timelines, AX16, GCC/Clang ASan/UBSan and five mutants pass | Guarded handler/counter wired and caller objects compile | Actual v5 passage observed; close next required owner |
| K16 | MCC Johnson boundary `1C9CE0` | Native record reset and thin binding implemented | 78 physical cases, all21 sites; GCC/Clang ASan/UBSan and four mutations pass | Main/world handler wired and current caller objects compile | Actual v5 passage observed; close next required owner |
| K17 | MCC Elite retirement boundary `272EC0` | Native intrusive teardown/free and all four registered invalidation callbacks implemented | 48 whole calls,128 callbacks,2,083 fault prefixes; GCC/Clang ASan/UBSan, mutants, original repeat and shared-library instrumentation pass | Guarded live route/counter wired and current caller objects compile | Actual v5 passage observed; close next required owner |
| K18 | Playback listener selection `12A810` / `12A930` / `2287B0` / `253070` / `2531E0` | All five existing listener bodies replaced with typed native algorithms | 672 physical calls, all312 sites; exact FP/live80-bit operands, GCC/Clang sanitizers and four mutation controls pass | Actual listener table and AL/AX/EAX result-width contracts wired; focused registered tests pass | Close remaining playback stream/provider dependencies `129E30` / `12AAD0` |
| K19 | Playback source/controller retirement `129EA0` → `220690` → shared `16E8A0` | Native release and shared salted-pool deletion implemented | 116 original calls, all114 sites; exact alias/read/store/FP, GCC/Clang sanitizers and three mutants pass | Qualified dependency records/shared library wired; campaign audio gate remains closed | Continue actual provider and backend stop dependencies |
| K20 | Order-reset combat/perception `1EC7B0` / `267220` / `267290` / `266490` | Native marking, retirement, first-match clearing and policy reads implemented | 212 whole calls +450 faults; GCC/Clang ASan/UBSan, two mutants and byte-exact original repeat pass | Main/world handler wired; actual caller objects compile | Compose native actor reset after remaining unit-action/behavior/HS dependencies close |
| K21 | Authored `ai_living_fraction` / `2AF700`, MCC329 | Native full-width counts, conversion and division implemented | 223 new original cases;961 shared query regressions, GCC/Clang ASan/UBSan and five new mutants pass | Script host and authenticated MCC layout row wired; actual callers compile | Global HS FP ownership/first dispatch and mission acceptance remain open |
| K22 | Playback stop callback `18F6B0` | Typed callback and independent qualified route implemented | 119 original cases,69 sites,10 actual indirect caller frames; compiler/sanitizer/three mutants pass | Actual manager route and low8/gate checks pass; whole audio group remains closed | Close actual backend stop/stream/provider ownership |
| K23 | Unit-action support `B9390`, admitted `E8F10`, `1EFA70`/`1EEC30` | Native marking, cancellation and history implemented | 168 whole calls,8 unsupported prefixes,8 actual empty callbacks,44 fault cases; all95 sites and three mutants pass | Register-only admission and world route wired; actual caller objects compile | Close genuine `E8E10`/`E8E50`/`ED110`/seat-detach dependency graph |
| K24 | Point/frame geometry `219030` / `145070` / `218780` / `218B20` / `218D00` | Native permutation, affine transform, frame projection and distance implemented | 778 original cases,213 sites; hardware FP/alias/fault and five mutants pass | Main build module wired; typed navigation dependency | Close actual navigation search/cost/filter owners |
| K25 | Character lookups `1ECD00` / `1ED700` / `27C6C0` / `27C710` / `1ED100` | Native tag/order lookup and inherited movement-block traversal implemented | 423 original cases,194 sites; exact repeat, GCC/Clang ASan/UBSan and five mutants pass | Main build module wired; shared typed dependency | Use actual lookups in navigation/query closure |
| K26 | Navigation heap `27BB60` / `27BC00` / `27BCE0` | Native signed-priority sift/insert implemented | 72 original calls,113 sites, four actual caller prefixes; compiler/sanitizer/five mutants and module integration pass | Main module wired; typed dependency only | Close real `27B340`/`200D30` search; heap alone is not pathfinding acceptance |
| K27 | Navigation filter `201500` with real character/order/vehicle lookups | Native filter masks and output preservation implemented | 496 original cases,247 closure sites, exact repeat; GCC/Clang ASan/UBSan and five mutants pass | Main module wired; typed dependency | Compose actual navigation selector/search without guessed pathability |
| K28 | Salted data-array allocation `16E6A0` | Native signed scan, occupancy, header, clear and salt issuance implemented | 238 original cases,78 sites, exact repeat; GCC/Clang ASan/UBSan and four mutants pass | One shared pool-helper library, main module wired; typed dependency | Use in real query/resource allocation and qualify complete consumers |
| K29 | Query support `278650` / `2789D0` / `278DD0` | Native query allocation/member initialization, attachment/refcount and signed live-slot iteration implemented | 379 original cases,190 closure sites, physical repeat; GCC/Clang ASan/UBSan and five mutants pass | Main module wired; typed dependency only | Close actual `1BD310` getter and descriptor cancellation before enabling `278460` |
| K30 | Perception references `2673A0` / `267450` / `267410` | Native policy-controlled position and optional tracking resolution implemented | 238 original cases,56 sites, exact repeat; GCC/Clang ASan/UBSan and five mutants pass | Main module wired; typed dependency | Use in actual getter/filter ownership without added bounds or null defaults |
| K31 | Navigation workspace initialization `27B010` | Native workspace clear, descriptors and BSP records implemented | 303 original cases,53 sites, repeat; compiler/sanitizer/five mutants pass | Main module wired; typed dependency | Compose proved lease/arena lifetime with actual navigation query/search |
| K32 | Query scheduler/retirement `278930` / `278720` / `278A50` / `2787F0` / `278B50` / `2786D0` | Native priority slots, member reservation, broadcasting and last-reference teardown implemented | 69 original calls,343 closure sites, seven typed-request profiles; compiler/sanitizer/six mutants and module integration pass | Main module wired; typed address/value seams | Start is proved underK36; close full1BD8F0 speech cancellation before behavior activation |
| K33 | Object→actor `1EB730` | Native object-kind/type12-extension ownership implemented | 315 original cases,29 sites, all256 kinds and real wrapped-null branch; compiler/sanitizer/five mutants/repeat pass | Main module wired; typed dependency | Lookup is composed underK53; close actual1BD310/266D30 parent |
| K34 | Animation-channel reset `116920` | Native primary/secondary overlay invalidation/playback reset implemented | 135 whole calls +99 prefixes,40 sites; actual callers/transport/RET, compiler/sanitizer/three mutants/repeat pass | Main/world native handler wired; fresh actual caller objects compile | Compose actual action13/seat/begin closure |
| K35 | Perception availability/reset `267750` / `2661F0` | Native actor-state predicate and real tracking/group resets implemented | 228 original cases,77 sites, exact repeat; compiler/sanitizer/five mutants pass | Main module wired; typed dependencies | Compose actual `266D30` with remaining real position/state/speech/geometry owners |
| K36 | Behavior query start `1BD9D0` | Native linked-member broadcast/reservation count and SIMD-result obligation implemented | 35 original calls,186 closure sites; compiler/sanitizer/five mutants and module integration pass | Main module wired; typed dependency | Compose full `278460` after actual getter/cancellation closure |
| K37 | Navigation query start `27B100` and exact numeric/heap closure | Native admission/priority conversion, geometry values and workspace publication implemented | 910 original cases,117 root/430 reachable closure sites; compiler/sanitizer/six mutants/repeat pass | Main module wired; typed dependency | Compose proved arena/lease lifetime with real query expansion/search |
| K38 | Perception storage `274E30` / `274AE0` / `2660F0` / `2745E0` | Native lookup/active/reset/property allocation implemented | 108 whole+268 faults,164 owner sites; compiler/sanitizer/four mutants/repeat pass | Main module and single shared allocator wired | Group creation/pressure is composed underK45/K50/K53; close actual perception update parent |
| K39 | Cancel support `16EA60` / `6CA40` / `C1E90` / `1D0430` / `275AB0` | Native pool iteration, campaign bitset, timer lookup and group-head publication implemented | 108 original calls,107 sites,22 native iterator/six head profiles; compiler/sanitizer/seven mutants/module integration pass | Main module and single shared helper library wired | Track admission is composed underK44/K46; close full1BD8F0 speech cancellation |
| K40 | Object world point `BBE00` | Native direct/parent transform, raw lazy publications and typed privateXYZ implemented | 237 original cases,76 sites,141 private profiles; compiler/sanitizer/five mutants/repeat pass | Main module wired; typed dependency | Compose actual perception/classification; no guessed object coordinates |
| K41 | Perception init/team `274020` / `1E7810` / `1E78F0` | Native initializer and qualified campaign team policy implemented | 779 whole+440 faults,187 owner sites; compiler/sanitizer/five mutants/repeat pass | Main module wired; existing474CE8 owner access corrected; actual cold-data/reset/restore validation passes | Cold-world ownership and group composition pass; close real1BD310/266D30 consumers |
| K42 | Perception actor link `274670` / `274750` | Native fanout/position state using real object-world point and allocation implemented | 166 whole+391 persistent faults,116 owner sites; compiler/sanitizer/five mutants/repeat pass | Main module wired; typed dependency | Creation is composed underK50/K53; qualify broader perception parent and numeric caller transport |
| K43 | Vector math `30BD0`, composed `3EA20`→`266EF9` | Native normalize/magnitude and actual distance/spill publication implemented | 421 original cases,58 sites, six pending quantities and three true unmapped spills; compiler/sanitizer/five mutants/repeat pass | Main module wired; typed pending-error result, no virtual FP stack | Compose real position/path consumers and preserve rejected-publication outcomes |
| K44 | Cancel tracks `275610` / `2755B0` / `274F40` / admitted `217310` | Native real-track/group policy scan implemented for authenticated campaign profiles | 73 original calls,231 admitted sites; actual caller, compiler/sanitizer/eight mutants/module integration pass | Main module wired; mode2 and concurrent inactive0→1 remain unavailable | 275770 is composed underK46; close real constant34 speech/full cancellation |
| K45 | Pressure allocation/prune/construct `274200` / `274190` / `274490` | Native real shared allocation, signed scoring, retirement callbacks and initialization implemented | 111 whole+635 faults; all reachable sites, eight unreachable fallback sites authenticated; compiler/sanitizer/five pressure mutants/repeat pass | Main GroupLink module includes shared Pressure module | 267520 is composed underK53; close broader perception update and numeric caller transport |
| K46 | Cancel admission `275770` and actual caller | Native campaign admission/policy/timer/track iteration implemented | 108 original calls,450 admitted sites; real1BD8F0 prefix, compiler/sanitizer/eight mutants/module integration pass | Main module wired; mode2 remains unavailable | Close actual constant34 speech and full1BD8F0 cancellation |
| K47 | Forced creation classifier `273B70` under actual274450 profile | Native object/attachment/generation/team classification implemented | 491 whole+153 faults,191of233 root sites/all256 object bytes; compiler/sanitizer/five mutants/repeat pass | Main module wired; previous-group/force=false generic branches remain open | Creation is composed underK50/K53; close broader perception update profile |
| K48 | Native path arena/lease supported profile | Native lease, primary teardown and arena reconstruction implemented | 263 original cases,126 admitted sites; compiler/sanitizer/five mutants/repeat pass | Main module wired; secondary CPU release is proved underK49 with genuine protection service | Compose proved secondary resource lifetime with actual navigation search |
| K49 | CPU resource retirement `130030` / `140310` / actual `12FD10` / `2DB4CA`, composed arena leases | Native metadata, genuine typed page protection and secondary lifetime implemented | 327 resource+312 composed+263 prior arena cases; real faults/protection, compiler/sanitizer/mutations/repeats pass | Arena module includes resource owner; actual platform service required | Wire real arena service with native navigation consumers; GPU-associated retirement/search stay open |
| K50 | Campaign group creation `2749F0` / `274450` | Native find/create, first property match, real allocation and actor fanout implemented | 178 whole+1,068 faults; all95 new-owner sites; compiler/sanitizer/five link mutants/repeat pass | Main module wired; actual267520 parent is composed underK53 | Close actual1BD310/266D30 behavior initializer; generic multiplayer policy remains open |
| K51 | Speech weighted chooser `217370` | Native threshold/filter/sum, actual LCG publication and typed candidate profile implemented | 221 original calls,76 reachable sites; one algebraically unreachable site; six mutants/compiler/sanitizer/module pass | Main module wired; actual retained RNG owner required | Finish real constant34 collector/publication and composed213D30 speech cancellation |
| K52 | Navigation candidates `27C410` / `27C450` | Native signed node/hash wrap and original segment projection implemented | 353 original cases,74 sites; seven mutants/compiler/sanitizer/repeat pass | Main module wired | Finish actual ballistic jump/search owners; no successful search route yet |
| K53 | Perception lookup `267520` | Native first-object/attached-owner match, lazy admission, real2749F0 creation and signed-clock deadline implemented | 223 whole+516 faults;111 root/1,315 closure sites; six mutants/compiler/sanitizer/exact repeat pass | Main module wired; numeric caller binding remains separate | Compose real1BD310/266D30; private numeric fault transport/generic multiplayer remain open |
| K54 | Model-marker transforms `1E11B0` / `1E1200` / `144910` / `145560` | Native marker lookup/variant filter, signed node mapping, quaternion basis and source matrix composition implemented | 362 whole+323 faults;280 sites; seven mutants/compiler/sanitizer/exact repeat pass | Main module wired | Perception point is composed underK63; qualify actual2669E0/266D30 |
| K55 | Tracking allocation `266280` / `267870` / `266150` / `1EC970` / `2677D0` | Native priority/age pressure scan, real retirement/free, iterator/reset and shared allocator implemented | 172 whole+1,193 faults;452 new sites+45 standalone retirement sites; seven mutants/compiler/sanitizer/repeat pass | Main module wired;266490 wrapper-only export has full previous combat regression | Compose actual265F40/266D30; ABI/runtime and full unit-action begin remain open |
| K56 | Object marker owner `BACD0` / `FC510`; current velocities `BC200` / `1D8C80` / `1D8D80` | Native tag inheritance/pickup policy, root traversal, signed component admission and nullable vector publication implemented | 417 whole+117 faults;214 sites/all256 kind bytes; six mutants/compiler/sanitizer/repeat pass | Main module wired | Snapshots are composed underK63; qualify actual2669E0/266D30; simulation/wake remain separate |
| K57 | Object marker request `BAD60` | Native actual inheritance/model/marker request, signed offsets/capacity and original fallback pose implemented | 116 whole+389 faults;118 root/453 closure sites; six mutants/compiler/sanitizer/repeat pass; one exact pending MOVSD read admitted on rejected copy | Main module wired; real retained-output API | Private value and point parent pass; qualify actual2669E0; rejected-copy provider side effects/numeric ABI remain open |
| K58 | Speech queue `217970` / `217870` / `218070` / `2180F0` | Native queue admission/postponement, transitive cancellation and real salted-pool reclamation implemented | 176 physical calls,384 closure sites; six mutants/compiler/sanitizer/module/repeat pass | Main module wired; two actual217812 publication-block continuations qualified | Finish full2175A0/213D30/request34; no sound/backend success supplied |
| K59 | Perception policies `2676C0` / `1ED240` / `1ED450` / `1EA1D0` / `1EA2E0` / `1ED4F0` / `1ED530` / `27C7E0` / private `1C1D70` | Native actual authored character/vehicle/perception lookups and private optional output implemented | 753 whole+131 faults;331 new/402 composed sites; nine mutants/compiler/sanitizer/repeat pass | Main module wired; prior character/tracking source unchanged | Compose real26FA40 scoring;264B50/26E040/virtual160A60 remain open |
| K60 | Jump fallback `FF580` | Native trajectory arithmetic, actual publication/pending quantities and private-value seam implemented | 261 original cases,82 sites;145 private-value completions; six mutants/compiler/sanitizer/repeat pass | Main module wired; original default vector/pointer already owned | Compose trueFF1A0 high-arc solver and278E60; actual search remains open |
| K61 | Speech reservation `116C00` / `116B80` / `213890` / `21FE40` / `140EA0` | Native unit/source/permutation eligibility and private resolved-sound output implemented | 202 completed+one real address4 fault;220 sites/133 local-output profiles; six mutants/compiler/sanitizer/module pass | Main module wired; three actual117240 entry/callee continuations qualified | Finish actual117240 submission/whole speech request34; no missing owner defaults |
| K62 | Perception score `26FA40`, actual266D30/26E040 scalar callers | Native campaign/direct-policy score, real tag/vehicle/team bitset and original x87/SSE implemented | 334 whole+one true mode2 boundary+479 faults;384 admitted root/752 composed sites; eight mutants/compiler/sanitizer/repeat pass | Main module wired; scalarXMM0 caller liveness authenticated | Compose actual266D30/26E040; mode2 virtual160A60 and private mid-confidence fault transport remain open |
| K63 | Perception points `1EBCB0` / `26DDD0` / `11B4F0` / `299770` / `26DED0` | Native real object/root/marker/motion snapshots, attachment distance selection and actor fallback copy implemented | 778 whole+348 prefixes+one real null fault;307 root/833 composed sites; seven mutants/compiler/sanitizer/repeat pass | Main module wired; actual private BAD60 profile has no synthesized guest stack | Compose actual2669E0/265F40/266D30; rejected-copy provider side effects/numeric ABI remain unqualified |
| K64 | Speech follow-up `215B40` / `1110D0` / `1494F0` | Native unit-channel publication and actual tick conversion/timer follow-up implemented | 298 original calls,95 reachable sites/one mathematically dead site; seven mutants/compiler/sanitizer/module/caller receipts pass | Main module wired; three actual215840 follow-up blocks qualified | Finish actual215840/203D80/117240 submission and request34 |
| K65 | Reaction support `11E850` / `BCF70` / `1D2CC0` / `BCFB0` / `F34F0` / `275C20` / `275BA0` | Native plane/sector output, real object roots and state/cache predicates implemented | 864 whole+102 prefixes+seven true missing header faults;186 sites; eight mutants/compiler/sanitizer/repeat pass | Main module wired;132 optional native-value plane profiles qualified | Compose actual2669E0; real275E90/275CB0 location-query graph stays open |
| K66 | Vision support `26F940` / `299860` / `26DF70` / `CDCC0` / `111FD0` | Native timestamp/rescore, attachment selection, facing, raw/private unit vector and signed extension query implemented | 981 whole+two genuine mode2 boundaries+one real missing read+140 prefixes;193 new/931 composed sites; seven mutants/compiler/sanitizer/repeat pass | Main module wired; seven actual core/helper objects instrumented | Compose actual26E040;213D30/203D80/action and documented partial-private-fault limits remain open |
| K67 | Authored jump `278E60` / actual high-arc `FF1A0` profile | Native original ballistic solver, lazy retained/private points and named pending quantities implemented | 410 originals,440 profile sites/428 canonical,284 private points/48 pending boundaries; seven mutants/compiler/sanitizer/repeat pass | Main module wired;0–3 live original operands; optional/alternate FF1A0 profile remains outside contract | Compose real27BD30/27B340 search and direction/frame dependencies |
| K68 | Speech cluster mask `203C00` / `252F30` | Native actual mutable BSP cache/mask and existing typed cluster geometry implemented | 121 originals,201 sites; seven mutants/compiler/sanitizer/module/caller receipts pass | Main module wired; existing cache/literal ownership contracts preserved | Compose actual203D80/117240; arbitrary SIMD scratch ABI remains outside typed contract |
| K69 | Vision records `26EEF0` / `1ECCC0` / `1D0400` / `272970` | Native attachment match/index, inherited character block, player association and clump clock publication implemented | 404 whole+53 prefixes;99 sites; seven mutants/compiler/sanitizer/caller/repeat pass | Main module wired; source consumers authenticated | Compose actual26E040 with remaining alert/speech/control owners |

### Next finite dependency targets

1. The three v3 boundaries (`1B6F20`, `1C9CE0`, `272EC0`) passed in the coherent v5 client. Elite reaches900 ticks; Marine/Grunt now require `278460`, Johnson requires `1C9F10`. Implement their actual dependencies before another expensive cohort. The original failed CTest report remains retained alongside the documented correction to the obsolete final-actor-survival assertion; real retirement satisfies the lifetime check. This partial conversion run does not qualify the whole engine as native-only.
2. Implement authored orders: evaluator `2AF970` calls `27F9C0`, which selects squad `20B150` or group `20B3E0`. The original squad path needs actor-plan reset `1EC820`, squad transition `20C710`, `29B450`, `20CFF0`, `274FA0`, and group helper `20B0F0`. [The authenticated original graph](../analysis/campaign-native-kernels-20261028/orders-original-owner-graph.json) records ten initial owners/1,053 instruction sites; their further direct/indirect dependencies remain explicit. This is static planning evidence, not implementation or acceptance of orders.
3. Close original AI safety `22CB90` and continue the decision/navigation closure. Its first `22CC70` clock gate is call-free; the ten-dependency owner also requires actor, player, object and unit safety predicates. Preserve the native count family and the now-native combat-count query; neither closes save safety or AI decisions.
4. Continue campaign audio from the 21 missing candidate owners and semantically qualify/replace the 15 remaining pre-existing register-level candidates. K13 supplies actual fade request/sample dependencies for `12AB30`; full playback drivers remain open. Candidate presence must never enable the live group by itself.
5. Screen update `1FD00` and DOF setup `1FC10` are converted. Qualify the resulting native screen module through composed authored consumers, then continue remaining cinematic control/camera/animation algorithms.

Checked work must link the exact report, source hashes, command, limitations and scratch removal below. An ABI adapter may marshal verified arguments/results at the external seam; the algorithm must not simulate a CPU/register file or execute instructions one at a time. Candidate source presence is not parity proof.

## Remaining engine features

- [ ] **AI decisions/combat:** complete perception, targeting, allegiance/alertness, orders/tactics, aiming/cover/melee, damage/death, species/difficulty and actor lifecycle native closure. Preserve existing proven structured controls. Close safety owner `22CB90` and each newly reached dependency explicitly.
- [ ] **Navigation:** original search/costs, hints, obstacle/movement admission, replan/blocked routes, ground/crouch/jump, flying/swarm, vehicle and cross-BSP traversal. Host geometric A* is not completion.
- [ ] **HS runtime:** original expression evaluation/coercion, script/thread/command scheduling, sleep/wake, RNG, object lists and globals; compare actual authored programs and save/revert continuity.
- [ ] **Mission builtins/commands:** orders/counts/awareness, boarding/exiting, fly-to/by, scripted shooting/jumping and all other actual authored requirements. Implement generic capabilities, not mission-specific bypasses.
- [ ] **World/player/object scheduling and physics:** remaining native scheduler, collision/contact, controller/listener, allocation/lifetime, teleport and integration closure; share the existing physics work without duplicating or reverting it.
- [ ] **Map/BSP lifecycle:** whole `13A780`/original callback ordering and readiness, streaming/retention, object/actor/HS links and repeated map/BSP loads. Static callback bodies do not prove execution.
- [ ] **Save/revert and progression:** original `226190` save safety, full actor/path/command/HS/RNG/clock restoration, repeated death/revert and cross-BSP checkpoint, completion/restart and frontend resume.
- [ ] **Local campaign co-op:** real additional participants/bipeds, independent input/camera/HUD/first-person, participant-aware death/respawn, one script/AI world and shared BSP/checkpoint/pause authority.
- [ ] **Network campaign co-op:** authoritative campaign input/state and lifecycle transport, coupled BSP/checkpoint restoration, session recovery, LAN/loss/delay/disconnect/rejoin acceptance.
- [ ] **Campaign audio/speech:** finish all 40 campaign-group entries (after K13: 4 proved typed, 15 pre-existing unqualified, 21 absent), activate only proven complete groups, convert speech dependencies and remove production lifted/hardware models.
- [ ] **Cinematic/presentation control:** remaining camera/screen/controller/composition/animation/attachment/IK native algorithms and imported authored sequence playback; object-relative animated camera and attached-volume transform support.
- [ ] **Final acceptance:** meaningful original/native component controls, complete current-source authored encounters, all applicable species/difficulty/BSPs, mission playthroughs, solo/local/network co-op, matched presentation and launcher delivery.

## Progress discipline

1. Work from a specific missing owner or capability and a finite dependency list. Record its next missing owner instead of repeating an unchanged failing campaign sweep.
2. Authenticate original semantics, implement the native module, run focused original/native comparisons, then integrate and expand. A new failure is a new explicit work item, not permission to approximate a return.
3. Separate implementation, independent proof, caller wiring and composed acceptance. Never mark an entire feature complete from a leaf test or a source-file count.
4. Re-run a completed check only after relevant source/input changes, a failure or an unresolved concern. Keep one bounded report per useful validation; retire superseded scratch promptly.
5. Use `tools/run_tests.py` for CMake and `tools/run_test.py` for one-off probes/oracles. All generated compiler/binary/bank/frame output belongs to owned `HALO2_TEST_SCRATCH`; retain compact reports/hashes/failure evidence only.
6. Preserve unrelated dirty work, original assets, existing evidence/frozen snapshots and launcher-selected runtimes. Keep the goal active until all required work is complete.

## Evidence ledger

- October 28: persistent goal created from the audit and user implementation authorization. Initial work split into disjoint AI, query, audio and screen modules. No new build or runtime accepted yet.
- K01/K02: [aggregate AI proof](../analysis/campaign-native-ai-queries-20261028/proof.json) records source/input hashes, independent original repeat, GCC/Clang, two actual source mutations and official root normal/sanitizer results. Native algorithms preserve typed data access and original outcomes; their adapter decodes arguments/encodes owed returns without virtual CPU reconstruction. Original caller continuation bytes establish that the reached callers discard dead volatile registers/flags. Actual campaign continuation and selected-runtime qualification remain open.
- K04/K05: [GCC physical/audio proof](../analysis/campaign-native-audio-leaves-20261028/gcc-v4/report.json), [Clang ASan/UBSan proof](../analysis/campaign-native-audio-leaves-20261028/clang-sanitizers-v1/report.json), [cleanup](../analysis/campaign-native-audio-leaves-20261028/cleanup.json), and [native audio notes](NATIVE_CAMPAIGN_AUDIO_LEAVES_20261028.md). Both original routines are call-free; all 112 original instruction sites are exercised offline. Registry activation and composed live audio acceptance remain open.
- K06: [physical original proof](../analysis/campaign-native-kernels-20261028/screen-lifecycle/original.json), [root CMake normal result](../build/test-results/campaign-native-20261028-screen-lifecycle-v2/report.json). An initial test-only JSON string comparison compile error was corrected; the actual caller translation unit and native kernel test then pass. Later native world methods retain existing mutation/executor/publication guards; the followup proofs below cover those callers and their journal.
- K03: [living-count proof](../analysis/campaign-native-kernels-20261028/ai-living-count/component-proof.json) retains 96 complete count/resumed-evaluator calls and 50 read-fault prefixes across eight SSE policies, exact physical repeat, caller compilation and real source-mutation rejection. Physical original execution exposed 70 FP-status disagreements in the prior Unicorn reference. Native behavior is compared with actual original hardware execution rather than changed to match emulator omissions. The global thread/frame FP ownership and checkpoint relationship are not inferred from these cases.
- K06 followup: [Clang ASan/UBSan and source mutations](../analysis/campaign-native-kernels-20261028/screen-lifecycle/clang-sanitizers/report.json), [GCC and actual caller compilation](../analysis/campaign-native-kernels-20261028/screen-lifecycle/gcc-callers-v2/report.json). The caller proof compiles current world, screen runtime and journal decoder as objects; it is not a linked full game or composed mission. A private probe's missing SDL include was fixed in its CMake configuration, with the failure retained.
- K07: [screen journal proof](../analysis/campaign-native-screen-journal-20261028/proof.json) retains normal/sanitizer results, source hashes and cleanup. Actual native kernel memory updates capture/replay identically; IDs 1–32 remain unchanged, void calls fabricate no old CPU scratch return, and bad arguments/results/order are rejected before mutation.
- K06 transport: [original caller and transport proof](../analysis/campaign-native-screen-binding-20261028/proof.json) and [authenticated callers](../analysis/campaign-native-screen-binding-20261028/callers.json) retain two direct call sites, their actual downstream consumers, normal/sanitizer checks and deliberate SP/EAX mutation rejection. Only real RET backing/SP consumption is transported; no dead CPU scratch or flags are reconstructed. The clear-direction reset ABI is admitted explicitly.
- K09 supersedes K03's original source pins: [current count-family proof](../analysis/campaign-native-kernels-20261028/ai-count-family/component-proof.json) covers 288 whole query/resumed-evaluator cases and 204 fault prefixes across eight masked SSE policies and nine paired-NaN controls. Living v3 regenerates byte-for-byte. Required canonical inputs are 383,485 bytes (living) and 953,375 bytes (family); obsolete newly generated v1/v2 payloads were removed with their rejection diagnostics retained.
- K08: [GCC interleaved ducking proof](../analysis/campaign-native-ducking-20261028/gcc-interleaved-v4/report.json), [Clang ASan/UBSan proof](../analysis/campaign-native-ducking-20261028/clang-interleaved-sanitizers-v2/report.json), [semantic gate regression/mutation](../analysis/campaign-native-ducking-20261028/semantic-gate-v1/report.json), [table-only metadata proof](../analysis/campaign-native-ducking-20261028/gate-metadata-proof.json), [cleanup](../analysis/campaign-native-ducking-20261028/cleanup.json), and [native ducking notes](NATIVE_CAMPAIGN_DUCKING_20261028.md). The proof checks 20,960 mutable reads relative to 6,448 publications, all 145 original instruction sites and a movement mutation that preserves separate read/write lists but changes interleaving. Campaign eligibility is **3 proved typed, 16 pre-existing unqualified, 21 missing**. Forty present function pointers cannot open the group.
- K06 final aggregate: [screen lifecycle proof](../analysis/campaign-native-kernels-20261028/screen-lifecycle/proof.json) includes byte-exact physical repeat, source mutations, current caller object compilation, eight removed scratch paths and the required 30,212-byte compressed original data fixture.
- Reference-test integration: [availability guard proof](../analysis/campaign-native-kernels-20261028/offline-reference-availability.json) verifies absent owned offline inputs return 77 with `passed=false`/`validation_not_run=true`. Native numeric algorithms are unchanged; removing only the new validation guards reproduces both previously tested validator hashes. Missing references and unsupported physical-reference platforms are never parity passes.
- First implementation checkpoint: [source/evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint.json) links 13 validated proof/retention reports and current source hashes. All 53 recorded owned scratch/build paths are absent. Four required original data references total 1,678,139 bytes; no new executable, archive, object, raw runtime bank or capture is retained. The goal remains active: existing lifted engine paths, complete missions, native-only closure and launcher delivery are not qualified by this batch.
- K10: [combat-count proof](../analysis/campaign-native-kernels-20261028/ai-fighting-count/proof.json) and [implementation notes](NATIVE_CAMPAIGN_AI_FIGHTING_COUNT_TU15.md) close the original combat-counter/actor-predicate dependency graph through the actual script binding. All four core original bodies are covered; 168 full queries/resumed evaluators and 78 fault prefixes pass, as do GCC/Clang/sanitizers and two new boundary mutations. Earlier count-family reference bytes are unchanged and current regressions pass. The 754,127-byte required original data reference is retained; owned fighting-count scratch is empty. Full HS first dispatch, composed mission and whole-engine native-only acceptance remain open.
- Current-client capture corrections: v1 omitted the newer Rust source tree; v2 borrowed the small authored data tree through a symlink rejected by the standard variant bundler. Both failed before runtime and their owned builds were removed. The freezer now captures Rust sources and the 28 KiB authored data tree as hashed ordinary source inputs; the subsequent v3 result is recorded in the migration checkpoint below. No original assets or selected runtime were modified.
- K11: [actor/order-script ownership proof](../analysis/campaign-native-kernels-20261028/ai-orders/script-assignment/component-proof.json) closes `26C600` and real native erase/find/insert/hash/equality for the installed four-byte actor payload profile: 48 full transitions plus 42 read/store prefixes, exact original repeat, GCC/Clang/sanitizers and compiled free-list mutation rejection. The 150,126-byte reference is required data; scratch is removed. The [expanded orders frontier](../analysis/campaign-native-kernels-20261028/ai-orders/expanded-owner-frontier.json) retains 36 original owners and 15 further direct targets, plus actual indirect behavior-retirement callbacks. Full authored orders and live campaign acceptance remain open.
- AI safety planning: [authenticated static frontier](../analysis/campaign-native-kernels-20261028/ai-safety-required-owner-graph.json) records `22CB90` and its ten direct owners (1,242 original sites total). It supplies dependency planning only; no native safety implementation or semantic acceptance is claimed.
- K13: [fade closure and retention proof](../analysis/campaign-native-playback-20261028/cleanup.json) and [scope/implementation notes](NATIVE_CAMPAIGN_AUDIO_FADES_20261028.md) cover 888 whole unchanged original calls, 6,504 reads ordered relative to 1,760 stores and 88,840 original instructions, with GCC/Clang sanitizers, three source mutations and focused registered tests. The existing `12DB00` register-level body was removed. The audio group remains closed: four of 40 required owners qualify, plus two qualified nonrequired dependencies; 15 legacy candidates and 21 missing owners remain. Earlier three typed audio algorithms are unchanged, all recorded scratch/build paths are gone, and no large artifacts are retained.
- Orders role correction from actual disassembly: `274FA0` selects clump combat status using 13 bins and four policy rows; it is not generic behavior retirement. The actual behavior-reset callback fanout is `1EA580`, reached through `277930`. The expanded frontier now records these corrected roles.
- Coherent current-source MCC checkpoint: [migration report](../analysis/campaign-native-kernels-20261028/current-encounters/migration-checkpoint.json) records a complete successful client build, unchanged frozen source snapshot, original activation and both session-seed tests passing. Each previous native callback is observed in the actual game process. The four encounter tests fail at newly exposed real owners; no original executable/map outside the MCC cache is opened and no cache write is attempted. All owned client/source/build storage is removed. This snapshot predates the final timed-screen conversion and later helpers; it is neither a latest-tree/full-campaign nor native-only qualification.
- K14: [native order-reset camouflage proof](../analysis/campaign-native-kernels-20261028/ai-orders/camouflage/component-proof.json) closes `1EB590`, `D3340` and actual `B7E50` dirty-entity gating: 132 whole calls plus 42 faults, all83 sites, complete retained banks/boolean/SSE, GCC/Clang/sanitizers, physical repeat and compiled missing-dirty-publication mutation rejection. The 173,817-byte required data reference is retained and owned scratch is removed. Full orders remains open.
- K12: [final timed-screen proof](../analysis/campaign-native-screen-timed-20261028/proof.json) and [controller/ABI scope](CAMPAIGN_NATIVE_SCREEN_TIMED_TU15.md) cover 505 whole original calls and 339 instruction-boundary prefixes, with 51,432 ordered reads, 19,024 stores and 181,902 original instructions. All 488 reachable sites are exercised; four mathematically dominated clamp branches are documented, and six unsupported intra-instruction CMPSD prefix labels were removed rather than asserted. Mutable vector-pointer slots stay lazy. Core, numeric binding and production compatibility API pass GCC, Clang, actual ASan/UBSan and five meaningful source mutations. The 180,546-byte required compressed reference is retained and all recorded scratch is removed. [Final world/wrapper/decoder object compilation](../analysis/campaign-native-kernels-20261028/screen-timed-final-caller-compile-v2/report.json) passes after dispatcher integration and removal of the old screen fallback; this is a compile-only check, not a new linked game or mission.
- K18: [listener conversion and cleanup proof](../analysis/campaign-native-listeners-20261028/cleanup.json) and [scope notes](NATIVE_CAMPAIGN_LISTENERS_20261028.md) cover all five actual listener owners and all312 original sites: 672 physical calls, 10,616 reads, 264 stores and 62,508 instructions. GCC, Clang ASan/UBSan, three algorithm mutations, the real manager low8-contract omission mutation and focused registered tests pass. Native bindings retain actual 80-bit caller operands; appended low8 result obligations preserve existing full/low16 contracts. Prior leaf/duck/fade numeric algorithms and adapters are unchanged. All recorded scratch/build paths are removed, with no large artifacts retained. The campaign audio gate remains four of40 required owners, with 15 legacy and21 absent.
- Second implementation checkpoint: [current evidence/source manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-2.json) links eight validated component reports and the actual failed-at-new-boundary migration report. Native AI callback source hashes match both independent component proof and the frozen client. Completed component scratch/build trees and the full client snapshot are removed. Ten required original input/metadata references total 2,965,504 bytes; no new executable, object, archive, raw memory bank or capture is retained. Existing lifted engine paths and all full mission/native-only/launcher acceptance gates remain open; the persistent goal is active.
- K16 component ready for the next integration step: [native behavior-reset proof](../analysis/campaign-native-ai-behavior-reset-20261028/proof.json) covers 26 whole calls plus52 fault prefixes, all21 sites, 292 ordered reads/528 stores, GCC, Clang, real ASan/UBSan and four mutants. Original caller evidence limits its result obligation to AL. This was the initial isolated component receipt; its later routing and actual v5 passage are recorded below.
- K15: [area/candidate proof](../analysis/campaign-native-ai-area-20261028/proof.json) independently executes 480 original cases (210 whole, 270 read faults), including 63 controlled external input edits. All337 sites reachable under the actual flags5/mode0-or1 call profile are covered; 27 general-helper prescan/mode2 sites lie outside this caller. AX16 is a behavior identifier, with score assigned by the caller. GCC/Clang, actual ASan/UBSan, five compiled mutants and exact original repeat pass. The 786,933-byte required compressed reference is retained; owned scratch is removed.
- K17 full closure supersedes prefix-only staging: [clump-retirement proof](../analysis/campaign-native-kernels-20261028/ai-clump-retirement/component-proof.json) covers twelve native owners, 48 complete original calls, 128 actual registered invalidation calls and2,083 read/store fault prefixes, plus two explicit service-stop controls. All four admitted callback implementations are native. GCC/Clang, actual shared-helper ASan/UBSan instrumentation, two compiled mutants and both exact physical repeats pass. Both required data fixtures total1,068,321 bytes; scratch is removed.
- K19: [source/controller retirement proof](../analysis/campaign-native-retirement-20261028/cleanup.json) and [provider/backend boundaries](NATIVE_CAMPAIGN_AUDIO_RETIREMENT_20261028.md) close `129EA0`/`220690` using the same single native `16E8A0` library as clump retirement. All114 sites and116 calls pass with ordered reads/stores, alias-induced input changes, GCC/Clang sanitizers and three mutants. Actual qualified records/shared-static-library linkage pass focused registered tests. The campaign gate remains four of40 required owners; callbacks and native backend stream operations remain separate.
- K20: [combat/perception reset proof](../analysis/campaign-native-kernels-20261028/ai-orders/combat-reset/component-proof.json) covers212 complete calls and450 faults, exact original repeat, two actual mutations and GCC/Clang/ASanUBSan. The existing policy storage was authenticated against original bytes; no new guessed constants were added. Its kernel retains signed-indexed policy reads. The 566,933-byte required reference is retained and scratch is removed. Main/world routing is staged after the v5 snapshot; current parent/mission acceptance remains open.
- Coherent v5 capture: the v4 guard rejected two audio headers changing during capture, before any build; its owned snapshot was removed. A brief source-write hold produced a stable11,164-file v5 snapshot. [Native source admission](../analysis/campaign-native-kernels-20261028/current-encounters/v5/native-source-admission.json) confirms each of the three reached native implementations matches its independent proof and the captured world routes match the compiled caller. The completed v5 runtime result is recorded below; admission alone is not gameplay acceptance.

- Completed v5 passage: [migration checkpoint](../analysis/campaign-native-kernels-20261028/current-encounters/v5/migration-checkpoint.json) retains the complete client build, actual four-squad results and explicit native activity. Elite reaches900 ticks with actual clump retirement; Marine/Grunt fail at278460 and Johnson at1C9F10. Original activation and both session-seed tests pass. The original reports remain unchanged; [lifetime-criterion proof](../analysis/campaign-native-kernels-20261028/current-encounters/v5/lifetime-criterion-proof.json) qualifies real retirement and rejects six failure mutations. No original-reference opens/cache writes occurred; owned build and source snapshot were removed.
- K21: [living-fraction proof](../analysis/campaign-native-ai-fraction-20261028/proof.json) records223 independent original cases and961 shared query regressions, real hardware FP authority, five new mutants, actual caller compilation and authenticated MCC329 layout admission. The86,921-byte compressed reference is required input; scratch is removed. Full HS scheduling/FP ownership remains open.
- K22: [stop-callback proof and cleanup](../analysis/campaign-native-stop-callback-20261028/cleanup.json) records119 original cases including10 genuine indirect-caller frames, all69 sites, GCC/Clang sanitizers and three mutants. The actual manager owns the independently qualified entry; the forty-owner gate remains closed.
- K23: [unit-action support proof](../analysis/campaign-native-kernels-20261028/ai-orders/unit-action-support/component-proof.json) closes the admitted cancellation/mark/history graph through actual native callbacks; full action begin is explicitly unimplemented. The206,624-byte data fixture is required input and scratch is removed.
- K24: [point/frame geometry proof](../analysis/campaign-native-ai-point-geometry-20261028/proof.json) records778 original cases, all213 sites, hardware FP/association/alias/fault checks and five rejected mutants. The existing projection-axis storage is the authenticated original table; no guessed duplicate was added. The160,299-byte compressed fixture is required input; scratch is removed.
- K25: [shared character proof](../analysis/campaign-native-ai-character-20261028/proof.json) records423 original cases, all194 sites, exact physical repeat, five mutants and compiler/sanitizer checks. Real movement inheritance and fullDWORD squad sentinel behavior are preserved. The498,540-byte compressed fixture is required input; scratch is removed.
- K26: [path-heap integration proof](../analysis/campaign-native-path-heap-20261028/module-integration-v2/report.json) and [cleanup](../analysis/campaign-native-path-heap-20261028/cleanup.json) retain72 actual original calls and four real caller prefixes, all113 sites, exact priority/tie/store behavior, five rejected mutations, compiler/sanitizer and missing-reference checks. Seven owned scratch/build roots are removed, with no raw banks or binaries retained. Full navigation remains open.
- K20/K23 caller integration: [current caller compilation](../analysis/campaign-native-kernels-20261028/orders-action-caller-compile/report.json) passes all three actual objects with unchanged captured source. This is compile-only; it does not qualify a linked runtime or composed reset.
- K27: [navigation-filter proof](../analysis/campaign-native-ai-navigation-filter-20261028/proof.json) records291 complete calls,203 fault prefixes and two true original faults; all247 sites, real padding/mask/alias behavior, five mutants and compiler/sanitizer checks pass. The939,778-byte compressed fixture is required input; scratch is removed.
- K28: [allocation proof](../analysis/campaign-native-data-array-allocate-20261028/proof.json) records162 complete calls and76 fault prefixes, all78 sites, exact repeat and four compiled mutation rejections. Both shared pool algorithms are actually sanitizer-instrumented. Capstone omitted the read on originalTEST852C8B; authenticated observer metadata was repaired without changing original instructions or the native algorithm. Superseded unqualified data fixtures were disposed; the217,326-byte final original fixture is required input and all recorded scratch is removed.
- K29: [query-support proof](../analysis/campaign-native-query-support-20261028/proof.json) records149 complete calls and230 genuine read/store prefixes, all190 sites including unchanged16E6A0. Salted allocation, actor admission, real member/slot/refcount stores, deadline equality/sign and alias-induced fresh datum reads are verified. Five compiled mutants and GCC/Clang/real ASan/UBSan pass; the independent physical repeat matches exactly. The860,943-byte compressed fixture is required input, all owned scratch is removed. No numeric binding or full278460 acceptance is claimed.
- K30: [perception-reference proof](../analysis/campaign-native-perception-refs-20261028/proof.json) records213 complete calls and25 read-fault prefixes, all56 sites; signed negative policy indices, salt masking, source fallback and wrapped-null pointer rules are preserved. GCC/Clang/real ASan/UBSan and five mutants pass; independent physical repeat is exact. The726,282-byte compressed data reference is required input and owned scratch is removed.
- K31: [path-workspace proof](../analysis/campaign-native-ai-path-workspace-20261028/proof.json) records53 complete calls and250 prefixes, all53 sites, exact repeat, five mutants and compiler/sanitizer checks. The48,191-byte compressed fixture is required input; scratch is removed. Original1498E0/149680 perform allocator teardown/arena reconstruction, so their ownership remains an explicit dependency rather than a guessed lock seam.
- K32: [query scheduler proof/cleanup](../analysis/campaign-native-query-scheduler-20261028/cleanup.json) records69 unchanged original calls, all343 sites,1,759 interleaved reads/448 stores and six compiled mutants. Address and actual disjoint native-local request profiles share one semantic loop; no emulated private stack is required. GCC/Clang ASan/UBSan, real module linkage, missing-reference77 withpassed=false and wrong-pin rejection pass. Six owned scratch roots are removed and prior heap/audio algorithms remain unchanged. Full278310/1BD8F0 cancellation and278460 stay open.
- K33: [object-to-actor proof](../analysis/campaign-native-object-actor-20261028/proof.json) records298 complete calls and17 read prefixes, all29 sites, all256 object-kind bytes, signed type12 extension offsets and an actual high-address mapped original wrapped-null branch. Five mutants and GCC/Clang/real ASan/UBSan pass; independent repeat is exact. The396,631-byte compressed reference is required input and owned scratch is removed.
- K34: [unit-animation reset proof](../analysis/campaign-native-kernels-20261028/ai-orders/unit-animation-reset/component-proof.json) records135 whole calls and99 faults, all40 sites, authenticated ordinary void caller epilogues, real transport/RET failure, three mutants and compiler/sanitizer checks. The210,515-byte original data fixture is required input; scratch is removed. Main/world route is added; [fresh caller compilation](../analysis/campaign-native-kernels-20261028/unit-animation-caller-compile/report.json) passes all three actual objects with unchanged source. Full action begin remains open.
- Third implementation checkpoint: [bounded component/source/reference manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-3.json) links20 passed reports, the original unchanged v5 results and current caller compilation. Thirty required original observation/site inputs total9,870,722 bytes;35 recorded component-owned scratch paths are removed, with no new binaries/raw banks/captures retained. Current geometry header/core additions for the next navigation query are explicitly unqualified until their new regression proof closes; previous point-geometry evidence is historical. Matching newer proofs explain audio/build-module pin changes. The native-only/full mission/launcher gates remain open.
- K35: [perception-state proof](../analysis/campaign-native-perception-state-20261028/proof.json) records149 whole calls and79 fault prefixes, all77 sites, actor-state/none and double wrapped-null behavior, exact store widths/order and actual post-reset group-datum rereads under aliasing. Five compiled mutants and GCC/Clang/real ASan/UBSan pass; independent original repeat is byte-exact. The431,166-byte compressed reference is required input and scratch is removed.
- K36: [query-start proof/cleanup](../analysis/campaign-native-query-start-20261028/cleanup.json) records35 complete original starts through their real three helper bodies, all186 sites,1,019 interleaved reads/135 stores, five compiled mutants and GCC/Clang ASan/UBSan. Member identity uses actual row address, rows are reread after reservations, and next links are captured before their side effects. Native result carries count and the real optional SIMD publication obligation. Real module/missing-reference/pin checks pass; four owned scratch roots are removed.
- K37: [native path-query-start proof](../analysis/campaign-native-ai-path-query-start-20261028/proof.json) records910 physical comparisons, all117 root and430 reachable closure sites,169,947 original instructions, six compiled mutants and GCC/Clang/real ASan/UBSan. Canonical exact-literal cases are separated from37 adversarial literal-bank cases; NaN/integer-indefinite admission retains the original signed low32 check. The216,081-byte compressed reference is required input and scratch is removed. [Current geometry regression](../analysis/campaign-native-ai-path-query-start-20261028/geometry-regression.json) passes all778 prior cases and requalifies the added typed projection API, superseding checkpoint3 pending geometry drift.
- K38: [perception-group storage proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/storage-component-proof.json) records108 whole calls and268 faults, all164 owner sites, ordered read values/store interleaving, exact original repeat, four mutants and compiler/sanitizer checks. Shared allocation exercises75 of78 allocator sites; the remaining three are covered by its complete generic proof. The2,368,378-byte compressed fixture is a required original reference; owned scratch is removed. Full2749F0 and pressure eviction remain open.
- K39: [cancel-support proof/cleanup](../analysis/campaign-native-cancel-support-20261028/cleanup.json) records108 whole original calls, all107 sites,1,164 interleaved reads/75 stores, seven actual mutants, GCC/Clang ASan/UBSan and real single-library module checks. Twenty-two native-local iterator profiles preserve field reads and publication phases; six native head profiles avoid synthetic source stack. Three owned scratch roots are removed. Mode2 virtual160A60 and full cancellation/speech remain unqualified.
- K40: [object world-point proof](../analysis/campaign-native-object-world-point-20261028/proof.json) records160 complete calls and77 true prefixes, all76 sites,141 disjoint native private-value profiles, exact repeat and five compiled mutation rejections. The strengthened two-NaN corpus rejects reversed source row-sum operands. Signed node/offset, lazy scale/matrix reads, alias publications and real MXCSR/x87 behavior pass GCC/Clang/ASan/UBSan. The259,158-byte final compressed reference is required input; all owned scratch and superseded own unqualified fixture are removed.
- K41: [perception initialization/team proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/initialize-component-proof.json) records779 whole calls and440 faults, all187 new-owner sites, exact repeat, five compiled mutants and instrumented compiler/sanitizer checks. The7,732,690-byte compressed reference is required original input. Actual mode2 virtual callbacks remain unqualified. The original474CE8→44ABEC default-point pointer already belongs to Source12560 in the renderer cache-pool data helper. The first full production build linked successfully, but its ownership test rejected my duplicate mount; that duplicate was removed. The actual existing owner is corrected fromReadOnly to original.data ReadWrite. A focused real cold-data constructor test now validates native reset and checkpointed pointer changes before this gate is checked.
- K42: [perception actor-link proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/actor-component-proof.json) records166 whole calls and391 persistent-memory fault prefixes, all116 new-owner sites and313 composition sites. Completed FP bits/status and ambient live x87 preservation pass, as do five mutants, exact repeat and instrumented compiler checks. Private numeric fault temporary/TOP transport is explicitly outside its profile. The5,015,891-byte compressed reference is required original input; scratch is removed.
- K43: [native vector proof](../analysis/campaign-native-ai-vector-math-20261028/proof.json) and [pending store-boundary evidence](../analysis/campaign-native-ai-vector-math-20261028/pending-boundaries.json) record362 completed calls,56 prefixes and three real unmapped266EF9 FSTP faults. All58 sites and six exact pending80-bit quantities are verified; rejected publication retains the typed quantity/cause and actual pre-store FP state. Five mutants, compiler/sanitizer checks and original repeat pass. The33,389-byte compressed reference is required input; owned scratch is removed.
- K44: [campaign track-cancellation proof/cleanup](../analysis/campaign-native-cancel-tracks-20261028/cleanup.json) records73 original cases, all231 admitted sites,1,426 ordered reads and no engine stores. Actual275770 entry runs42 unchanged steps into the real275949 call, full275610 and natural27594E return. Eight mutants, compiler/sanitizer/module checks and explicit unsupported-boundary guards pass. Six owned scratch roots are removed. Six mode2 policy instructions and inactive0→1 private-input ambiguity remain unqualified; no invented result is supplied.
- Fourth implementation checkpoint: [current bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-4.json) links32 passed reports and297 matching current pins. Forty required original observations/site inputs total25,952,613 bytes;42 recorded component scratch paths are removed and no new binaries/raw banks/captures are retained. Current geometry additions are requalified. The real production cold-world build remains in progress; later changes to the shared world/journal consumers are explicitly unqualified until a fresh applicable compile/composition receipt. The appended274800 export has its own regression receipt; no native-only/full mission/launcher gate closes.
- Cold-world integration correction: [first coherent production result](../analysis/campaign-native-kernels-20261028/current-ownership-v1/current-source-cohort.json) compiled/linked the complete Systems/physics/test graph with an unchanged11,528-file snapshot; the actual test rejected a duplicate474CE8 mount. Its original reports remain unchanged and owned storage is removed. The duplicate edit is reverted; the existing renderer data owner is corrected to original writable.data access, with its readonly12-byte zero backing preserved. [Primary owner receipt v2](../analysis/campaign-native-perception-default-20261028/original-data-owner-v2.json) authenticates exact bytes/permissions. The next focused test uses the existing standalone cold-data target and the same actual native reset/checkpoint-restore helper; no new full client run is claimed.
- K41 ownership gate: [actual constructor/reset/restore integration proof](../analysis/campaign-native-perception-default-20261028/integration-proof.json) passes the real standalone cold-data target against an unchanged11,665-file snapshot. It reuses the single existing Source12560 pointer owner with corrected originalReadWrite access and sharedReadonly12-byte zero backing. Dynamic pointer changes are consumed by native2660F0 after checkpoint restore. Both first/full-build and second/focused-build owned roots are removed. The first failed duplicate-mount report remains unchanged; no full client/gameplay or native-only qualification is claimed.
- K46: [native admission proof/cleanup](../analysis/campaign-native-cancel-admission-20261028/cleanup.json) records108 whole original calls, all450 admitted sites,6,696 reads and eight rejected mutants. Actual1BD8F0 executes41 source prefix steps including its real1BD957 property flag publication, then actual1BD973 call/275770/natural1BD978 return. This authenticates the caller input without claiming full callback/speech completion. Three owned scratch roots are removed; ten mode2 sites stay unqualified.
- K47: [forced-classification proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/classify-component-proof.json) records491 whole calls and153 faults, all256 object-kind byte values,191of233 root sites in the actualforce1/groupNONE/private-output caller profile, and full attachment/generation helper paths. Five mutants include unused-distance overflow status; compiler/sanitizer checks and exact repeat pass. The4,426,572-byte compressed fixture is required input; scratch is removed. Generic prior-group/force=false behavior stays open.
- K48: [native arena profile proof](../analysis/campaign-native-ai-path-arena-20261028/proof.json) records263 physical cases and126 admitted sites, five rejected mutations, compiler/sanitizer checks and exact repeat. The25,861-byte compressed reference is required input and scratch is removed. Secondary130030 freeing remains an explicit required owner; real native resource metadata unlink/page protection is being implemented without a fake callback completion.
- K49: [resource-retirement proof](../analysis/campaign-native-ai-resource-retirement-20261028/proof.json) and [composed arena proof](../analysis/campaign-native-ai-path-arena-retirement-20261028/proof.json) pass902 shared original regressions across297 unique sites, real page-protection actions and partial-unlink read faults. Six resource mutants and six composed mutants are rejected; original repeats are byte-exact and all owned scratch is removed. The828,821-byte resource and934,446-byte arena fixtures are required inputs. Source130030 changes permissions/metadata without unmapping its retained pages. Typed protection is a genuine required platform action; missing services and associated GPU records remain explicit required-owner boundaries. Current arena overloads/earlier profile are both requalified.
- K45/K50: [pressure proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/pressure-component-proof.json) and [group-link proof](../analysis/campaign-native-kernels-20261028/ai-perception-group/link-component-proof.json) cover746 pressure and1,246 whole-parent original comparisons, genuine capacity refusal, real callback eviction and the first matching actor property. Shared GCC2/2, Clang12/12 including ten compiled mutants and actual12-core/helper ASan/UBSan2/2 pass; both physical corpora repeat exactly. The eight unreachable pressure fallback sites have an authenticated original gate receipt. Main GroupLink/Pressure modules and both required fixtures are admitted;3,750,035 bytes of original observation inputs are retained and scratch is removed. Actual267520 integration, private numeric fault transport and unrestricted multiplayer policy remain open.
- K51: [speech-selection cleanup/proof index](../analysis/campaign-native-speech-choice-20261028/cleanup.json) and [implementation notes](NATIVE_CAMPAIGN_SPEECH_CHOICE_20261028.md) qualify221 physical calls, all76 reachable sites, six rejected mutations and219 typed-local candidate profiles. Count1 performs no numeric/RNG/data access; other calls consume the real retained RNG state and preserve original numeric controls/caller-live quantities. No seed/default is supplied. Main module is included, all three scratch roots are removed, and only compact reports/site metadata are retained. Actual constant34 collector/publication and whole speech cancellation remain open.
- K52: [navigation candidate proof](../analysis/campaign-native-ai-path-candidates-20261028/proof.json) qualifies353 original cases and all74 sites, including a full4096-slot hash wrap, negative IDs, real missing-memory faults, source NaNs and alias/fault order. Seven real mutants are rejected and GCC/Clang/ASan/UBSan plus exact independent repeat pass. The142,472-byte compressed original fixture is required input; all owned scratch is removed. The segment calculation preserves the actual authored numerator. Main module is included; real jump feasibility and path search remain open.
- K53: [whole perception-lookup proof](../analysis/campaign-native-perception-lookup-20261028/proof.json) records223 complete calls+516 memory-fault prefixes, all111 root and1,315 closure sites,72,062 ordered reads and61,078 stores. Real native2749F0 creation, pressure/refusal and signed tickrate deadline updates are exercised. Six compiled mutants are rejected, all14 actual core/shared-helper objects are sanitizer-instrumented, and independent physical repeat is byte-exact. The1,795,694-byte observation fixture is required input; all three owned scratch roots are removed. Main module is included; real1BD310/266D30/live caller and private numeric fault transport remain open.
- Fifth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-5.json) aggregates43 passed reports and379 matching current pins. Fifty required original observation/site inputs total37,881,998 bytes;58 recorded component scratch paths are removed and no binaries/raw runtime banks/captures are retained. Cold-world ownership and actual reset/checkpoint restore now pass. The vector-value extension currently being developed remains explicitly unqualified until its new regression receipt; shared consumer drift also remains explicit. This does not qualify a fresh full client, mission, native-only engine or launcher runtime.
- K43 value extension: [current vector regression](../analysis/campaign-native-ai-vector-value-20261028/proof.json) requalifies all421 original cases and191 complete normalization calls through the native private-value API. Five mutants and compiler/sanitizer checks pass. Prior distance/pending-publication behavior and canonical original input remain unchanged; scratch is removed. This supersedes checkpoint5's explicitly pending vector pins.
- K55: [tracking-allocation proof](../analysis/campaign-native-kernels-20261028/ai-tracking-allocation/component-proof.json) covers172 complete calls and1,193 fault prefixes, all452 new-owner sites and45 standalone266490 retirement sites. Native pressure/reset/iterator/allocation paths pass exact original repeat, seven actual mutants and compiler/sanitizer checks on six core/shared-helper objects. [Retirement-export regression](../analysis/campaign-native-kernels-20261028/ai-tracking-allocation/combat-retirement-export-proof.json) requalifies the narrow wrapper export with the previous complete combat corpus. The1,379,682-byte compressed reference is required input, scratch is removed and main module is included. Parent265F40/266D30 composition and live runtime remain open.
- K54: [object-marker proof](../analysis/campaign-native-object-markers-20261028/proof.json) covers362 whole+323 fault comparisons, all280 sites, real signed node/variant policies and source quaternion/transform arithmetic. All seven compiled mutants, compiler/sanitizer checks and byte-exact original repeat pass without native algorithm changes. A physical-bank contradiction exposed four incorrectly marked MOVHPS store operands in the offline metadata; its rejection diagnostics remain and the discarded newly generated payload was removed. The853,748-byte canonical fixture is required input; all owned scratch is removed. Main module is included, while fullBAD60/perception-point and private numeric ABI composition remain open.
- K56: [object-owner/current-motion proof](../analysis/campaign-native-object-motion-refs-20261028/proof.json) covers417 whole+117 fault comparisons, all214 sites and all256 object-kind bytes. Real tag inheritance, root/component motion, signed admission, nullable outputs, wrapped NONE and source/destination aliases pass six compiled mutants, compiler/sanitizer checks and exact original repeat without native algorithm changes. The537,890-byte canonical fixture is required input; scratch is removed and main module is included. ParentBAD60/1EBCB0 composition remains open.
- Sixth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-6.json) includes the completed vector-value regression, tracking/retirement export, marker geometry and object-motion references. The current vector and narrow combat-retirement exports are requalified; previous changed consumer pins remain explicit. Required references are test observation/site data, and completed component scratch/build trees are removed. Full current-client/mission/native-only/launcher gates remain open.
- K58: [speech-queue cleanup/proof index](../analysis/campaign-native-speech-queue-20261028/cleanup.json) and [implementation notes](NATIVE_CAMPAIGN_SPEECH_QUEUE_20261028.md) qualify176 physical original calls, all384 closure sites,4,844 ordered reads and506 stores. Six real mutants, Clang/ASan/UBSan, the actual GCC CMake module and exact independent repeat pass. Two authentic217812 publication blocks reach217837→217970→21783C through the real policy call. Eight scratch/build roots are removed; only compact reports/site metadata remain and38 prior semantic sources are unchanged. Main module is included. Whole2175A0, request34, speech backend and live campaign gates remain open.
- K59: [perception-policy proof](../analysis/campaign-native-kernels-20261028/ai-tracking-update/policy-component-proof.json) covers753 whole+131 fault comparisons, all331 new and402 composed sites,9,905 ordered reads and no retained stores. Nine actual mutants, GCC/Clang, both core objects under ASan/UBSan and exact physical repeat pass without algorithm correction. The350,204-byte canonical fixture is required input; scratch is removed and prior character/tracking source stays unchanged. Main module is included. The private optional output profile is bounded; full26FA40 scoring,264B50 alerts,26E040 vision and virtual160A60 remain open.
- K60: [jump-fallback proof](../analysis/campaign-native-ai-jump-fallback-20261028/proof.json) and [notes](CAMPAIGN_NATIVE_AI_JUMP_FALLBACK_TU15.md) qualify261 original cases, all82 sites,108 prefixes, two real source faults and21 exact pending quantities;145 completed calls also pass through the private-value API. Six actual mutants, GCC/Clang/sanitizers and exact repeat pass; scratch is removed. The26,665-byte canonical fixture is required input. A declared zero-epsilon adversarial case is distinguished from canonical literal input. Main module is included; source return follows real trajectory work, whileFF1A0/278E60/search stay open.
- K57: [marker-request proof](../analysis/campaign-native-object-marker-request-20261028/proof.json) covers116 whole+389 fault comparisons, all118 root/453 closure sites, real inherited model/marker requests and original fallback pose/status. Six mutants, GCC/Clang/three-core ASan/UBSan and byte-exact original repeat pass; the semantic core stayed unchanged. A rejected BAEB0 MOVSD store admits only its verified exact pending source read; memory-provider side effects on that rejected copy remain unqualified. The399,391-byte reference is required input and all five scratch roots, including failed attempts, are removed. Main module is included; genuine private output and1EBCB0/numeric ABI remain open.
- Seventh implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-7.json) adds speech queue admission/reclamation, perception policy prerequisites, jump fallback and whole retained-outputBAD60. All remain bounded component results with their stated private-output, callback, numerical and caller limits. Resolved next actions are updated; actual278460/1C9F10 parents, complete missions/native-only/launcher acceptance stay unchecked.
- K61: [speech-reservation cleanup/proof index](../analysis/campaign-native-speech-reservation-20261028/cleanup.json) and [implementation notes](NATIVE_CAMPAIGN_SPEECH_RESERVATION_20261028.md) qualify202 complete calls and the real address4 null-owner fault, all220 sites,5,708 ordered reads and236 stores.133 native local-output profiles match at every read, and three full117240 entry prefixes naturally reach116C00 and return. Six mutants, compiler/sanitizer/production-module checks and three matching physical fixture hashes pass. Five scratch/build roots are removed and40 earlier sources remain unchanged. Main module is included; whole117240 submission/request34 remain open.
- Historical pre-extension status (superseded by the completed private-storage receipt below): K54/K57 original raw evidence is historical while shared source/header refactoring is independently requalified. No current private-value or new1EBCB0 composition is counted complete. Likewise the jump lazy-coordinate interface is being requalified before replacing K60's current pins.
- K62: [perception-score proof](../analysis/campaign-native-kernels-20261028/ai-perception-score/component-proof.json) and [actual caller/profile receipt](../analysis/campaign-native-kernels-20261028/ai-perception-score/callers-and-profile.json) qualify334 complete calls, the genuine26FE94→160A60 required-owner boundary and479 read-fault prefixes. All384 admitted root/752 composed sites,64,158 ordered reads, scalar bits and applicable FP controls/status match; eight mutants, six actual core/helper sanitizer objects and exact repeat pass. Existing literal ownership is authenticated; no new mount is added. The332,289-byte fixture is required input and scratch is removed. Main module is included; three mode2 sites, one primary-proven unreachable site and six private mid-confidence fault states have explicit scopes.
- K60 lazy-input extension: [updated fallback proof](../analysis/campaign-native-ai-jump-fallback-20261028/proof.json) requalifies261 original cases,145 private outputs and145 private point pairs with the same six mutants/compiler/sanitizer checks. Its26,665-byte canonical fixture is unchanged and scratch is removed; the new arc/278E60 draft remains unqualified.
- K54/K57 private storage extension: [current marker regression](../analysis/campaign-native-object-markers-20261028/storage-factor-proof.json), [request regression](../analysis/campaign-native-object-marker-request-20261028/storage-factor-proof.json) and [native value proof](../analysis/campaign-native-object-marker-request-value-20261028/proof.json) requalify all1,190 prior raw cases plus113 unchanged-original capacity1 calls at the actual1EBCB0 private stack layout. All28 meaningful output fields,6,726 engine reads and FP match; native memory has no private output bank and rejects writes. All13 existing plus six private-storage mutants, compiler/sanitizer checks and exact physical repeat pass without algorithm correction. The148,110-byte new reference is required input, old fixtures/oracles stay byte-identical and all five owned scratch roots are removed. This supersedes the pending marker source pins above; actual whole1EBCB0/numeric ABI remains separate.
- K63: [perception-point proof](../analysis/campaign-native-perception-object-point-20261028/proof.json) qualifies778 whole calls,348 fault prefixes and the real address18 null fault, all307 new/833 composed sites,51,665 reads and11,667 stores. It covers all256 kinds, distinct orientation/component-versus-stored-velocity behavior, real private marker output, aliases, signed extension and first ties. Seven mutants, GCC/Clang/five-core ASan/UBSan and exact repeat pass without algorithm correction. The1,503,642-byte canonical reference is required input; all four scratch roots are removed. Main module is included. The rejected26DF15 pending source read retains its narrow provider-side-effect scope; numeric ABI and2669E0/265F40/266D30/runtime remain open.
- Eighth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-8.json) adds actual unit speech reservation, campaign perception scoring, the requalified retained/private marker interface and full typed perception-point parents. Existing raw references are preserved, the source extensions have current regression receipts, and all recorded completed scratch paths are removed. Actual mission/native-only/launcher gates and broader callback/numeric ABI boundaries remain open.
- K64: [speech-follow-up proof](../analysis/campaign-native-speech-followup-20261028/proof.json) qualifies298 physical calls,95 reachable sites,1,574 ordered reads and2,176 stores, with one primary-proven dead conversion site. Seven mutants, all FP/SIMD outcomes, compiler/sanitizer/production-module checks, matching fixture hashes and three real215840 caller blocks pass. Four scratch roots are removed and48 earlier semantic sources remain unchanged; no binaries/raw banks are retained. Main module is included; full215840/203D80/117240 submission and request34 remain open.
- Historical initial reaction-support status (superseded by K65 below):11E850 transition-plane/optional sector output, BCF70/1D2CC0/BCFB0 root selection, F34F0 definition flag and275C20/275BA0 state/cache gates. Native core/API are authored but not compared yet.275E90/275CB0 are authenticated as real cached-position/location query owners with additional geometry dependencies; neither is counted complete and no query result is substituted.
- K66: [vision-support proof](../analysis/campaign-native-kernels-20261028/ai-vision/support-component-proof.json) covers981 complete calls, two real nested score-policy boundaries, the original unmapped address14 read and140 fault prefixes, all193 new/931 composed sites,58,739 reads and1,345 stores. Timestamp publication invokes actual native rescoring. Seven mutants, all seven core/helper sanitizer objects and exact original repeat pass; existing constants have genuine owners and no new mount is added. The813,281-byte reference is required input; owned scratch is removed. Main module is included. Full26E040, speech/action owners, three mid-expression x87 states and partial private-vector rejection outputs retain explicit limits.
- K65: [reaction-support proof](../analysis/campaign-native-perception-reaction-20261028/support-component-proof.json) qualifies864 whole calls,102 prefixes and seven actual010C0FF7 missing-header faults, all186 sites,9,492 reads and64 stores.132 optional native-value profiles compare the real original sector word with no engine writes. Eight mutants, compiler/sanitizer checks and exact repeat pass without algorithm correction. The586,963-byte reference is required input and completed scratch is removed. Main module is included;2669E0 and the real275E90/275CB0 location graph remain open.
- K67: [jump-arc proof](../analysis/campaign-native-ai-jump-arc-20261028/proof.json) and [notes](CAMPAIGN_NATIVE_AI_JUMP_ARC_TU15.md) qualify410 originals,440 profile sites/428 canonical,284 private-point variants and48 exact pending boundaries. Seven mutants, compiler/sanitizer checks and independent repeat pass; scratch is removed. The91,909-byte reference is required input and main module is included. Actual278E60 uses the complete native fixed high-arc profile;45 optional/alternate-input FF1A0 sites remain outside that caller contract, with five free original temporary slots required. No numeric/live route or full path search is qualified.
- Ninth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-9.json) includes speech follow-up, vision support, reaction prerequisites and the actual authored jump-link profile. All new component comparisons/mutations/sanitizers/repeats and their completed storage cleanup are linked. Real cached-position/location queries, remaining search, speech/action, parent composition, mission/native-only and launcher gates stay open.
- K68: [speech-cluster mask proof](../analysis/campaign-native-speech-cluster-mask-20261028/proof.json) qualifies121 originals, all201 sites,26,198 reads and2,317 stores. Seven mutants, compiler/sanitizer/module checks, three matching physical hashes and two actual203D80 caller blocks pass. Cache backing is genuinely owned; no key is synthesized. Existing listener compiled-literal contracts remain intact and arbitrary SIMD scratch ABI is outside the typed result contract. Five scratch roots are removed and50 prior semantic sources are unchanged. Main module is included; full203D80/117240 remains open.
- K69: [vision-record proof](../analysis/campaign-native-kernels-20261028/ai-vision/records-component-proof.json) qualifies404 complete calls+53 prefixes, all99 sites,2,978 reads and325 stores. Actual attachment match reread, inherited first-positive block, player duplicates/none and captured clock/output aliases pass seven mutants, compiler/sanitizer checks, exact repeat and authenticated consumers. The476,563-byte reference is required input; scratch is removed and main module is included. Full26E040/alert/speech composition remains open.
- New Game status inspection: frontend_state.cpp creates a campaign launch after difficulty selection; main.cpp takes that launch, creates the campaign runtime/Simulation and calls start_level(). Actual prepared MCC metadata confirms a valid Heretic player start. The verified gaps are stale mission selection on New Game and exiting on authored completion instead of loading the next map. Corrections and linked validation are in progress. Full introduction/transition/gameplay, native camera closure and launcher qualification remain unchecked.
- K70: [alert permission proof](../analysis/campaign-native-kernels-20261028/ai-vision/alert-permission-component-proof.json) qualifies700 whole calls,78 read-fault prefixes, three genuine nested-speech boundaries and two actual null policy-map faults. Seven mutations, both sanitizer core objects and exact original repeat pass; the187,275-byte original reference is required input and completed scratch is removed. Main module is included. Six original closure sites and four private x87 temporary fault states remain explicitly outside the admitted component.
- K71: [campaign selection proof](../analysis/campaign-native-new-game-20261028/level-selection-component-proof.json) qualifies188 complete original calls and141 exact read faults,135 of137 original sites and174 decoded-catalog profiles. Six mutations, compiler/sanitizer and independent repeat pass. The56,368-byte original reference is required input and completed scratch is removed. Main module is included. Two sites require concurrent mutation of immutable metadata; actual completion/profile/save/request and full New Game acceptance remain open.
- K72: [camera command proof](../analysis/campaign-native-new-game-camera-20261028/commands-proof.json) qualifies440 original cases, all138 sites,5,988 ordered reads and9,429 stores, including150 read-fault prefixes and three actual source faults. GCC, actual Clang ASan/UBSan, seven compiled mutations and an independent original repeat pass. Typed commands and ordinary void ABI bindings are covered. The205,090-byte required original reference is retained, owned scratch is removed, and production dispatch precedes the existing generated cinematic/camera routes. Startup/control/animation/director composition and full New Game remain open.
- K73: [scenario lookup proof](../analysis/campaign-native-new-game-20261028/scenario-lookup-component-proof.json) qualifies402 complete original calls and190 read-fault prefixes,131 of132 sites and114 decoded-catalog profiles. Seven mutations, actual-core sanitizers and independent repeat pass. The1,281,035-byte required original reference is retained; completed scratch is removed. Native cold session lookup preserves the original case-sensitive basename and first-match behavior. The remaining site requires concurrent immutable-header mutation. Full startup/mission/transition acceptance remains open.
- K74: [camera startup proof](../analysis/campaign-native-new-game-camera-20261028/startup-proof.json) qualifies287 original cases, all141 sites and989,841 unchanged instructions, including140 read-fault prefixes and three actual source faults. Eight mutations, actual Clang ASan/UBSan, GCC, independent repeat and review pass. The930,911-byte required reference is retained and owned scratch is removed. Shared native CRC uses its genuine existing table/flag backing and private typed constructor size words; no new data mount or arbitrary CRC ABI is claimed. The ordinary forward-direction caller profile is explicit. Production constructor dispatch now precedes generated input/cinematic/camera owners. Full controller/control/frame and mission acceptance remain open.
- K75: [camera control proof](../analysis/campaign-native-new-game-camera-20261028/control-proof.json) qualifies621 original cases, all267 sites and220,631 unchanged instructions, including419 read-fault prefixes and three actual source faults. Eight mutations, actual-core sanitizers, GCC and exact repeat pass. Enabling control publishes the genuine scripted selectors; disabling uses actual salted/type-checked objects, parents and action IDs. The1,181,982-byte required original reference is retained and owned scratch is removed. Existing whitelist memory remains its real mutable source input. Production dispatch precedes generated owners and status exposes native control activity. Full animation/observer/cinematic frame and mission acceptance remain open.
- New Game v1: [coherent build](../analysis/campaign-native-new-game-frontend-20261028/cohort-v1/current-source-cohort.json) links the actual client and frontend from12,022 unchanged captured files. The real-MCC campaign menu regression passes; [natural startup](../analysis/campaign-native-new-game-frontend-20261028/cohort-v1/new-game-startup.json) reaches Heretic tick120/BSP0 with four running scripts,74 scene objects and zero script faults. No executable/reference cache opens or cache writes occur. Owned source/build storage is removed and no large artifacts are retained. The20 changed checkout inputs remain explicit: v1 predates the new native cold lookup/camera/reset path and headless presentation clocks, and does not qualify full progression/rendering/native-only execution.
- Tenth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-10.json) links70 passed component/reference reports,551 matching current source pins and73 required original inputs totaling48,099,628 bytes. All241 recorded completed scratch paths are absent. Eight changed consumer/storage/helper files remain explicitly unqualified by a current component source pin; the coherent v2 runtime check is in progress. Existing generated engine paths and full mission/native-only/launcher gates remain open.
- K76: [original release void contract](../analysis/campaign-native-new-game-20261028/release-void-contract-proof.json) qualifies16 full original2AC980→211DA0→RET12 calls, all four evaluator sites,288 ordered reads and32 genuine HS completion stores. Save/revert/request banks and FP remain unchanged; independent exact repeat passes. The14,010-byte required reference is retained and owned scratch is removed. [Linked v2 cold receipt](../analysis/campaign-native-new-game-frontend-20261028/cohort-v2/iteration-1/cold-receipt.json) records the passing actual host continuation regression as well as menu and natural startup.
- New Game v2 iteration1: [cold receipt](../analysis/campaign-native-new-game-frontend-20261028/cohort-v2/iteration-1/cold-receipt.json) records all three focused tests passing from an unchanged12,063-file source manifest. Heretic starts at tick0/scripts1 and reaches tick120/scripts4 with zero faults; native reset/FOV/control activity reaches1/1/1. The earlier missing-include compile failure remains retained as iteration0; the current header correction is captured explicitly. No reference executable/cache opens or cache writes occur. The owned worker remains active only for the declared longer progression check; its cleanup is pending and no launcher runtime is promoted.
- Natural headless progression: [failed opening run](../analysis/campaign-native-new-game-frontend-20261028/cohort-v2/opening-progression.json) reaches tick12,000 with real cinematic time400s and fails at the genuine captured-frame GPU consumer required by `cinematic_screen_effect_set_crossfade`. No Armory handoff is accepted. The ordinary graphical loop already installs that consumer and acknowledges real renderer captures; the next acceptance run must use that loop. The headless guard remains intact and no capture outcome is supplied.
- K77: [transition publication proof](../analysis/campaign-native-new-game-20261028/transition-publication-component-proof.json) qualifies116 complete original returns,36 exact required-UI boundaries and5,521 read/store/clock-rejection cuts, with all43 executable pre-boundary sites. Raw forward overlapping copies,1,163 genuine private-value cases and mandatory clock producer timing pass seven mutations, actual-core sanitizers and independent repeat. The3,277,649-byte original reference is required input and completed scratch is removed. Main build includes the isolated helper; runtime clock authority, complete pending-request storage and full completion/UI composition remain open, with no new mount or host adapter call.
- K78: [ordinary director reset proof](../analysis/campaign-native-new-game-camera-20261028/reset-proof.json) qualifies124 original cases,60 admitted sites,53,516 unchanged instructions and four genuine mode2 controller entries without supplied outcomes. Eight mutations, actual-core sanitizers, GCC and exact repeat pass; the fake-controller-success mutation is rejected. The353,340-byte required reference is retained and owned scratch is removed. No new backing is added. Production dispatch selects the native reset and status distinguishes its activity from cinematic reset. Full mode2 controller ownership, five post-call sites and whole observer/frame composition remain open.
- K79: [original scalar ownership](../analysis/campaign-native-new-game-20261028/graphics-view-angle-data-owner.json) proves original `.rdata` flags6 and exact bytesC3F5483F at45C55C, consumed by the sole exact text reference2F806 within2F7E0. Four readonly bytes are added beside the genuine adjacent data owner; no initializer or zero bank is invented. [Actual cold-data receipt](../analysis/campaign-native-new-game-frontend-20261028/cohort-v2/iteration-2/cold-data-receipt.json) passes exact extent/bytes/owner, write rejection and checkpoint restoration in the owned production build. Graphics and the whole camera algorithm remain separately unqualified.
- K80: [native packet proof](../analysis/campaign-native-new-game-camera-20261028/packet-proof.json) qualifies277 originals, all74 observer/common sites,167 fault prefixes, eight genuine IDIV traps and two actual I/O faults. Seven mutations, actual-core sanitizers, GCC and exact repeat pass. The56,248-byte original reference is required and owned scratch is removed. [Retirement proof](../analysis/campaign-native-new-game-camera-20261028/packet-retirement-proof.json) compiles the changed producer and passes both admission/fixture regressions. Both actual inline calls229B95/229C30 retain instruction notification and real return-word publication, then use the native service. The old helper body, declaration, dispatch and public admission are removed;24 other bodies and the selected-address map remain byte-identical. No new lowering is generated. The35-site null/default camera branch and remaining parent229A90 stay unqualified.
- GPU iteration2: [actual hardware failure](../analysis/campaign-native-new-game-frontend-20261028/cohort-v2/graphics-cold-iteration2.json) proves45C55C is now available and stops at229BB4 on genuine mutable4F2968. [Original arena-byte proof](../analysis/campaign-native-new-game-20261028/graphics-view-reset-data-owner.json) authenticates the exact one-byte loader-zero `.data` field and five whole-byte references. [Prepared producer audit](../analysis/campaign-native-new-game-20261028/graphics-producer-prepared-profile.json) validates all33 required absolute data pairs using45 real production cold owners; no further gap exists in that bounded profile. The wider37-pair closure retains its explicit missing default-camera5320AC field. The arena fix and mutable/checkpoint regression are staged for the new GPU batch. The v2 finite worker expired while these gates were being finished; its owned source/build were removed and its passing cold receipts and failing graphics reports remain. No rendered frame or Armory handoff is claimed.
- Eleventh implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-11.json) links80 passed component/reference/bounded-runtime reports,583 matching current source pins and77 required original inputs totaling52,968,847 bytes. All251 recorded completed component scratch paths are absent; [checkpoint runner cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-11-cleanup.json) independently verifies its own scratch removal. Nine changed consumer/storage/test-helper files remain explicitly unmatched by a current component pin. The earlier coherent v2 cold receipts remain bounded historical successes; its failing graphics and expired final status are preserved separately. Coherent v3 now builds12,156 captured files with explicit owned cold-data/GPU acceptance before initial CTest cleanup. Whole renderer, Armory progression, native-only and launcher gates remain open.
- K81/v3: [actual graphics gate](../analysis/campaign-native-new-game-frontend-20261028/cohort-v3/graphics-batch.json) records a fresh passing cold-data build/execution, including the one-byte arena owner and mutable/checkpoint regression. [GPU failure](../analysis/campaign-native-new-game-frontend-20261028/cohort-v3/graphics-cold.json) clears both missing-source-data boundaries, then stops before the first frame because native-resolution texture-camera preparation has no readable scene attachment. The long run is correctly omitted; v3 does not execute/qualify its three subsequent CTests after the failed pre-test hook. All owned v3 source/build/GPU scratch is removed. The staged renderer fix requests actual offscreen color images before realization, retains the existing guards and has [independent lifecycle review](../analysis/campaign-native-new-game-20261028/graphics-readable-scene-integration-review.json); real GPU acceptance remains pending.
- K82/v4: [actual GPU regression](../analysis/campaign-native-new-game-frontend-20261028/cohort-v4/iteration-0/renderer-framebuffer-alias.json) passes the independent native-resolution/no-camera-material case before any forced double buffering, plus the existing940,800-pixel depth/stencil and alias/resize/retirement checks, with zero validation messages. Texture-camera requests latch the genuine readable-attachment requirement before swapchain realization; the native renderer allocates its real color images and preserves guards, epochs and real capture acknowledgments. Raw PPM outputs are removed by the child owner. Repeated-image equality is not full pixel/visual parity, and inactive target18/20 does not qualify active copy/sampling. [Actual campaign GPU failure](../analysis/campaign-native-new-game-frontend-20261028/cohort-v4/iteration-0/graphics-cold.json) clears this attachment boundary and reaches the missing `2B8EC→156E40` screen-flash sampler before frame1. The long progression test is correctly omitted. V4's finite worker expired and its owned source/build storage is removed; no launcher runtime is promoted.
- K83: [screen-flash proof](../analysis/campaign-native-new-game-camera-20261028/flash-proof.json) qualifies519 physical originals,294 complete calls and223 prefixes through all actual read/store boundaries in six representative profiles. Native global fades and per-player flashes preserve lazy/overlapping publications, actual curve/classification/descriptor data and hardware SIMD/x87 policy. Both mutable CRT policies pass; seven compiled mutations, GCC/Clang/actual-core sanitizers and independent exact repeat pass. The197,949-byte required original reference is retained and all completed component scratch is removed. [Independent review](../analysis/campaign-native-new-game-20261028/screen-flash-integration-review.json) records271 of370 sites, explains99 uncovered sites and verifies the actual void caller/24-byte output/RET transport. Other computed CRT handlers and the real3334F0 error callback retain explicit required-owner boundaries; no outcome is supplied. Thirty-two data pairs use existing authentic owners, with no new mount. The early live World binding and status activity counter are wired; coherent v5 actual client/GPU validation remains in progress and full New Game stays open.
- Twelfth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-12.json) links85 passed component/reference/bounded-GPU reports,643 matching current source pins and78 required original inputs totaling53,166,796 bytes. All260 recorded completed component scratch paths are absent; [runner cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-12-cleanup.json) also verifies the proof/checkpoint owners. Four changed storage/call/test/helper files remain explicitly unmatched by a current component pin. Coherent New Game v5 captures12,209 current files and builds the actual client with the native screen-flash binding; this active owned build is separate from completed component cleanup. Full rendered intro, Armory handoff, native-only and launcher gates remain open.
- K84/v5 iteration1: [original owner proof](../analysis/campaign-native-new-game-20261028/graphics-4922e2-data-owner.json) authenticates the independent one-byte loader-zeroed mutable widescreen field and actual platform-flags producer123B0/123E8. [Fresh cold receipt](../analysis/campaign-native-new-game-20261028/graphics-4922e2-cold-receipt.json) passes exact extent/access, separate neighbors, real native mutation and checkpoint preservation. The [actual GPU gate](../analysis/campaign-native-new-game-frontend-20261028/cohort-v5/iteration-1/graphics-cold.json) clears4922E2 and reaches the missing sky-model composition owner3EAFF→170820 before frame1. No preset widescreen outcome or completed settings initializer is supplied. The finite consumer preflight also identifies the separate4920D8 render-target word; its exact backing/regression is staged with K85 for v5's final declared refresh.
- K85: [model hierarchy proof](../analysis/campaign-native-new-game-camera-20261028/model-pose-proof.json) qualifies576 physical originals,194 complete calls and379 prefixes, all271 sites and401,083 unchanged instructions. Private13-word affine values, real parent/output storage, lazy tolerance initialization, actual sibling/child queue order and the255-cell original capacity are preserved. The shared product exposes a real numeric Y/Z contribution; [discarding it](../analysis/campaign-native-new-game-20261028/model-pose-carry-negative/report.json) loses the original invalid sticky bit before the next tolerance read. All seven other mutations, GCC/Clang/actual-core sanitizers and exact repeat pass. Existing marker and private-marker-value regressions are requalified after the shared input seam. The423,987-byte original reference is required input. [Independent caller review](../analysis/campaign-native-new-game-20261028/model-pose-integration-review.json) admits actual3EAFF's valid disjoint argument frame and verifies retired outputs; arbitrary corrupt private-stack ABI and physical Xbox RCPSS estimate parity remain outside this proof. No local matrix acquires a fabricated source address. Completed component scratch is removed.
- K86/v5 iteration2: [target storage receipt](../analysis/campaign-native-new-game-20261028/graphics-4920d8-cold-receipt.json) links the original four-byte owner and fresh production cold/checkpoint execution. [Actual hardware GPU gate](../analysis/campaign-native-new-game-frontend-20261028/cohort-v5/iteration-2/graphics-cold.json) clears the model owner and target storage, then stops before frame1 at473628. Independent renderer regressions pass again; long progression and the three later CTests are correctly omitted. All three captured source iterations remain unchanged; the final owned source/build/raw GPU trees are removed. No first frame, crossfade, Armory handoff or launcher delivery is claimed. Next work is the finite view-prefix data-owner preflight, starting with actual473628.
- Thirteenth implementation checkpoint: [bounded evidence manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-13.json) links92 passed component/reference/bounded-GPU reports,654 then-current source pins and79 required original inputs totaling53,590,783 bytes. All269 recorded completed component scratch paths are absent; [final v5/proof cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-13-cleanup.json) separately verifies the failed metadata probe, refresh coordinators and final source/build owner. Three then-current changed storage/call/helper files remain unmatched. This checkpoint predates staged473628 and its test/helper changes; those are not qualified by the older source pins. [Finite473628 preflight](../analysis/campaign-native-new-game-20261028/graphics-473628-preflight.json) records exact original1.0 bytes, mutable HS registration,27 typed consumer spans and the sole missing field. V6 builds12,343 captured files with that fix and new main-view contract; its active owned storage is separate from completed cleanup. Whole native engine, introduction, Armory and launcher gates remain open.
- K87/v6: [storage receipt](../analysis/campaign-native-new-game-20261028/graphics-473628-cold-receipt.json) records fresh production cold/checkpoint ownership,133 portable main-view checks/49 negatives and independent renderer GPU success. The actual introduction clears473628 and stops before frame1 at inactive-camera renewal. [Minimized GPU diagnosis](../analysis/campaign-native-new-game-20261028/texture-camera-renewal/diagnosis-proof.json) repeats that exact guard twice with current, unsubmitted targets; omitting only renewal allows one genuine presentation/readback. Main now renews only frames with a rendered secondary view, preserving the renderer guard. V6's attempted source refresh rejects a concurrent unrelated test edit before execution; its source/build and all six probe owners are removed. Production acceptance of the caller fix remains pending.
- K88: [BSP-selection proof](../analysis/campaign-native-new-game-camera-20261028/bsp-selection-proof.json) qualifies268 physical originals, all76 sites,95 prefixes, four actual faults and exact repeat, with GCC/Clang/sanitizers and eight rejected mutations. Signed packed24-bit BSP traversal, source floating-point operation order and lazy previous-leaf/cluster repair are native algorithms. Only actual2B7E6 is ABI-admitted; arbitrary14CDB0 numeric routing remains absent. The59,889-byte original reference is required input; completed scratch is removed.
- K89: [view-rectangle proof](../analysis/campaign-native-new-game-20261028/view-rectangle/component-proof.json) qualifies550 physical originals, all124 sites,270 prefixes and215 disjoint native-value profiles, with exact repeat, compiler/sanitizer checks and eight rejected mutations. Both actual2B8B4 and13C8A callers use the native helper. Private rectangle values share the semantic algorithm without fabricated guest storage. The134,333-byte original reference is required input; completed scratch is removed.
- K90: [three-leaf retirement validation](../analysis/campaign-native-new-game-camera-20261028/three-leaf-retirement-validation/report.json) compiles the23-body legacy producer and all six sealed packet/BSP/rectangle core/binding units, then passes four fixture/admission regressions. It authenticates all five native CALL bridges,20 retained public admissions,21 byte-identical unrelated bodies and the two bridge-only parents by inverse edit. The selected-address mapping is unchanged. No generator/lowerer was run, no fixture was changed and the35-site null-camera boundary remains explicit. Whole native camera/framebuffer/campaign qualification stays open.
- Fourteenth implementation checkpoint: [bounded manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-14.json) links98 passed component/bounded diagnostic reports,685 then-current source pins and81 required original inputs totaling53,785,005 bytes. All317 recorded completed scratch paths are absent, including nested task owners now checked by the aggregator. An explicitly retired unsealed rectangle capture is recorded separately; the sealed v2 and all prior required references remain intact. [Cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-14-cleanup.json) confirms v6 source/build, refresh and retirement/checkpoint owners removed. Four current changed source files remain unqualified by a matching component pin. Fresh v7 captures12,433 files with the actual inactive-renewal fix and both native camera leaves; its owned build/production GPU validation is active and separate from completed cleanup.
- Refresh lifecycle: [transaction proof](../analysis/campaign-native-new-game-20261028/refresh-transaction/proof.json) preserves the deterministic old partial-copy failure and passes12 focused regressions. Declared inputs stage and authenticate before publication; failed capture preserves frozen bytes/mtimes and permits an explicit same-sequence replacement within the original deadline and three-rejection bound. Partial I/O publication rolls back and iteration registration consumes frozen CMake. Independent read-only review found no material issue. This controller validation supplies no gameplay result.
- Completion dependency recovery: [primary root inventory](../analysis/campaign-native-new-game-20261028/completion-primary-frontier.json) authenticates96 sites/15 calls in191750 and the real191580→5A080 tail. [Dependency inventory](../analysis/campaign-native-new-game-20261028/completion-dependencies-primary-frontier.json) identifies call-free98-site154F80 options/participant preparation,248-site155190 fixup and the genuine session/UI/request dependencies. These are read-only original CFG observations, not implemented kernels or supplied callbacks; actual clocks, options, participant storage and original continuation remain required before native lifecycle acceptance.
- K91: [awareness proof](../analysis/campaign-native-ai-awareness-20261029/proof.json) qualifies216 complete original calls,700 fault prefixes and eight actual memory faults through576 reachable sites of the590-site gameplay/support graph, with14 primary-gated exclusions. GCC/Clang/actual-core ASan/UBSan, nine meaningful mutations and exact physical repeat pass; the core remains unchanged. The signed-tickrate/unsigned variant is retained as an observed modulo-WORD equivalence, with a meaningful negative-rate-clamp control separately rejected. Native commands traverse real clumps/perception groups/properties and publish awareness/deadlines without inventing perception, visibility or allocation. Actual CW/SW/TAG,80 x87 value bytes and MXCSR are preserved; independently observed asynchronous host FIP/FOP/FDP metadata changes are outside that preservation claim. The3,829,263-byte required original fixture is retained and completed scratch is removed. World active-ABI bindings and shared HS context integration remain pending.

- Fifteenth implementation checkpoint: [bounded manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-15.json) links100 passed component/bounded diagnostic reports,692 matching current pins and82 required original inputs totaling57,614,268 bytes. All326 recorded completed component scratch paths are absent; [checkpoint cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-15-cleanup.json) also verifies completed delivery/aggregation owners. The active v7 source/build is separate; this manifest does not qualify rendered introduction, Armory handoff, native-only or launcher behavior.

- K92: [native layout proof](../analysis/campaign-native-new-game-camera-20261028/view-layout-proof.json) qualifies775 physical cases through all173 original sites,120 prefixes, eight actual signed divide traps and three actual unmapped faults. GCC/Clang/actual-core sanitizers, eight meaningful mutations and exact original repeat pass. Six original ADD-WORD before-write cuts authenticate only the one pending source read by site/address/width/value; arbitrary side-effecting device output and write-fault equivalence are outside this ordinary-RAM caller profile. The135,170-byte v2 original fixture is required input and completed scratch is removed. [Six-leaf retirement proof](../analysis/campaign-native-new-game-camera-20261028/six-leaf-retirement-proof.json) passes all five combined tests, verifies20 retained legacy bodies/17 public entries/eight original CALL bridges and preserves the canonical packet/null guard. Only229A90 changes in this retirement step, by three verified native bridges;19 other bodies and the address mapping remain unchanged. No generator/lowerer runs. Fresh v7 iteration1 captures these changes for composed GPU validation.
- K93 primary/preflight: [literal ownership](../analysis/campaign-native-new-game-20261028/graphics-projection-literal-ownership.json) authenticates exactly8988083B at46A260 and0000103F at46A670, both four-byte read-only `.rdata` fields consumed by2FB50 at2FCA6/2FC86. [Finite cold preflight](../analysis/campaign-native-new-game-20261028/graphics-projection-fixed-data-preflight.json) links a private probe to unchanged borrowed v7 object/archive inputs and finds these two gaps among11 original2FB50/2FD70 fixed extents. Its expected failed status is retained. Both exact mounts and extent/write-rejection/checkpoint regressions are captured in refresh1; fresh execution is pending. No dynamic pointer result, projection value or completed frame is supplied.

- K93/rendered startup: [actual production receipt](../analysis/campaign-native-new-game-20261028/graphics-projection-literals-cold-receipt.json) records fresh cold/checkpoint/write-rejection and six real RTX5080 frames from v7 iteration2, with zero script faults and native packet/BSP/rectangle/flash/model counts6 and layout18. The longer run reaches authored scene02 then fails at the transparent descriptor ownership join. All three v7 source snapshots remain unchanged and its expired finite owner is removed; no whole-introduction or Armory acceptance is inferred.
- K94: [null-camera component proof](../analysis/campaign-native-new-game-20261028/packet-default/component-proof.json) covers683 new originals/503 prefixes/all35 null+three initializer sites, exact repeat, compiler/sanitizer checks and eight negative controls, while preserving the old277 reference byte-for-byte. [Data/activation receipt](../analysis/campaign-native-new-game-20261028/packet-default/data-activation-receipt.json) validates all14 actual cold extents/bytes and the new5320AC zero/RW/native backing/checkpoint owner; bootstrap380450 is authenticated at CRT index79, before381710 at222. Root routes only the actual null packet or initializer through this new kernel. Combined production activation remains pending in v8.
- K95: [active-ABI binding proof](../analysis/campaign-native-ai-awareness-20261029/binding-proof.json) transports924 canonical cases through1812 checks and35 physical caller packets, with GCC/Clang/sanitizers, seven binding mutations and exact original repeat. Held GP aliases eliminate argument-memory prepeek; actual retained FP and the late real return word are preserved. Root dispatch is staged; no public HS fresh policy is manufactured and shared script-host context remains open.
- Transparent ownership diagnosis: [private actual GPU capture](../analysis/campaign-native-new-game-20261028/transparent-implementation/diagnostic-2/capture.json) reproduces the scene02 guard with unchanged borrowed engine inputs. [Minimal real-World replay](../analysis/campaign-native-new-game-20261028/transparent-implementation/resident-replay/report.json) proves Cache1D4325C maps to captured resident9137B1DC and all306 implementation bytes match. [Independent review](../analysis/campaign-native-new-game-20261028/transparent-implementation/independent-owner-review.json) confirms the candidate fresh typed owner for each descriptor, unchanged selection/descriptor values, strict foreign/missing/damaged-owner rejection and preserved OG canonical path. The new native contract is a v8 pre-GPU gate; syntax/review alone do not qualify that runtime path. [Debug retirement](../analysis/campaign-native-new-game-20261028/transparent-implementation/debug-retirement.json) records removal of private instrumented executables/worker copies and supersession of the one-off logging helper by the persistent regression.
- Sixteenth implementation checkpoint: [bounded manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-16.json) links109 passed component/bounded diagnostic reports,722 matching current pins and84 required original inputs totaling57,932,538 bytes. All409 recorded completed generated paths are absent; [cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-16-cleanup.json) verifies the three unchanged v7 snapshots and its removed source/build. Fresh v8 captures12,527 files including the transparent fix, native default-camera activation, awareness bindings and genuine transparent-owner contract before GPU. Its explicitly owned active storage is separate from completed cleanup. Full introduction, Armory, native-only and launcher gates remain open.

- Seventeenth implementation checkpoint: [bounded manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-17.json) links117 passed component/bounded reports,773 matching pins and88 required original inputs totaling67,697,894 bytes. [Cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-17-cleanup.json) verifies423 completed generated paths absent, including the expired v8 source/build and private BSP diagnostic. [V8 bounded startup](../analysis/campaign-native-new-game-20261028/v8-bounded-startup-receipt.json) records actual cold/main-view/transparent contracts and six RTX5080 frames: projection12, policy6 and default initializer1. The later natural run remains failed at real BSP admission; no complete introduction is claimed. Fresh v9 is an explicitly owned temporary build, including the full-log parser correction and sealed save-metadata module.
- Latest component evidence: [projection and FP ordering](../analysis/campaign-native-new-game-camera-20261028/projection-proof.json), [safe frame-policy retirement](../analysis/campaign-native-new-game-20261028/frame-policy/retirement-proof.json), [participants](../analysis/campaign-native-transition-participants-20261029/proof.json), [save metadata](../analysis/campaign-native-transition-save-metadata-20261029/proof.json), and [full-log parsing regression](../analysis/campaign-native-new-game-20261028/graphics-log-parser/report.json). Remaining target-query/shadow/host FP/platform lifecycle gates stay explicit.

- Live menu evidence: [all four continuous cases](../analysis/campaign-native-new-game-20261028/live-menu/continuous-menu-acceptance.json). The same frozen v9 iteration1 client reaches the [actual Armory shader boundary](../analysis/campaign-native-new-game-frontend-20261028/cohort-v9/iteration-1/graphics-progression.json); both the unresolved PRT ownership and the absence of launcher delivery remain explicit. [Native ownership review](../analysis/campaign-native-new-game-camera-20261028/armory-material-native-ownership-review.json) rules out a shader default or residency omission as a valid fix.

- Eighteenth implementation checkpoint: [bounded manifest](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-18.json) links130 passed component/bounded reports,1103 current matching pins and92 required canonical original inputs totaling68,189,602 bytes. [Cleanup](../analysis/campaign-native-kernels-20261028/implementation-checkpoint-18-cleanup.json) verifies585 completed generated paths absent, including every v9 source/build/private display. [Live menu](../analysis/campaign-native-new-game-20261028/live-menu/continuous-menu-acceptance.json) passes all4 difficulties; [bounded Heretic handoff](../analysis/campaign-native-new-game-20261028/v9-heretic-handoff-milestone.json) records2154 hardware frames and fresh Armory startup. V9 final full client build and580 Armory resource checks passed; legacy MP fixture-path admission stopped that batch before GPU. The test now explicitly resolves genuine DLC/disc references while production link rejection remains unchanged. V10 is an explicitly owned active build; its first snapshot predates this final test-only resolver and will require declared refresh. Campaign PRT packet, affected MP behavior, actual Armory rendering, full native-only closure and launcher delivery remain open.
