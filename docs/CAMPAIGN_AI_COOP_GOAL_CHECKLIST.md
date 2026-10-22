# Campaign AI, navigation, mission scripting and co-op

Updated: October 20, 2026. Goal status: **active; incomplete**.

Latest verified engine increment: descriptor27's whole1BD1E0 callback and
1ED100 character-data owner now join the actor module (93 bodies /7,112
original instruction sites), preserving all91 earlier bodies and212 immutable
bytes. All16,319 combined component cases pass normally and with ASan/UBSan
in an unchanged snapshot. The new372 complete original comparisons execute
every instruction in both new bodies and the reused278DD0 iterator, using real
lookup owners without supplied dependency returns. The separate squad-style
fallback remains unqualified. Owned component source/builds are removed;
the extractor's later opening-docstring-only change is authenticated in its
proof. Complete v10 composition is running with900-tick encounter requests.

Complete v9 composition passes cold-world ownership, activation, both seed
checks and the prior15,947-case component target. Three Marines reach288
ticks before the callback now recovered above. V8 was rejected before building
when a concurrent test edit changed during capture; its scratch and all
completed v9 source/builds are removed.

The complete v7 client qualifies the preceding480-byte child-head publication
and unchanged original list builder, including722 whole original comparisons.
Its cold-world ownership, activation, both seed checks and15,847-case component
target pass. Johnson, three-Grunt and Elite Cairo controls each pass300 source
ticks with zero script faults and initial checkpoints. Three Marines now reach
283 ticks before the distance owner addressed above; v9 progresses to288.
Owned v7 builds
are removed. Complete mission, navigation/combat, live checkpoint replay and
actual split-screen/network co-op acceptance remain open.

Complete original Xbox TU1.5 actor behavior, navigation/combat and mission/command
scripts in the native campaign, then support real split-screen and network
campaign co-op with shared world, script, BSP and checkpoint authority. Prepared
MCC content supplies runtime assets; authenticated TU1.5 code and original
execution supply behavioral evidence. Never infer success from a menu, a decoded
asset, an allocated actor or a passing isolated routine.

User clarification, October 20: tags are imported from MCC and converted to
TU1.5 equivalents. **Engine parity is the priority.** Concentrate on original
actor decisions, navigation, HS/command scheduling, player/object lifecycle,
checkpoints and cooperative session behavior. Change tag conversion only for a
demonstrated mismatch that blocks those engine paths; imported data does not
replace implementation or verification of the original runtime owners.

This checklist owns this combined goal. Update it immediately after each
verified milestone, recording commands, source/input hashes, remaining blockers
and removed scratch. Existing broader MCC, physics, weapons and rendering
checklists retain their own acceptance gates. Preserve unrelated dirty work,
reference inputs and launcher-selected runtimes.

## Baseline and tracking

- [x] Create the persistent goal for this chat and this repository checklist.
- [x] Inspect current campaign source ownership, AI placement, frame ordering,
  script host, co-op respawn services and storage requirements.
- [x] Retain existing foundations: authored/counted/packed AI placement, bounded
  original AI/object ticks, whole-world checkpoint components and original
  co-op camera/respawn kernels. These are component evidence, not new gameplay
  acceptance. See [source world](CAMPAIGN_SOURCE_WORLD.md),
  [campaign progress](CAMPAIGN_PROGRESS.md) and
  [co-op components](CAMPAIGN_COOP_DEATH_IMPLEMENTATION.md).
- [x] Record an MCC-only selected-runtime baseline and actual authored NPC
  placement attempt, retaining cache/runtime hashes and opened-file audits.
  Cairo advances four ticks with zero NPC actors/script faults; Armory stops
  at `13B6F0 -> 2F7A38` reading `41CD1C`. Actual Cairo squad
  `1st_hum_johnson` stops at `27D190 -> 2098A6` reading `209D04` before placement
  completes. No original XBE/external-map reads or cache writes are observed.
  These are baseline findings, not acceptance of the new query code or AI.
  [Map baseline](../analysis/campaign-ai-coop-20261020/baseline/selected-runtime.json),
  [NPC attempt](../analysis/campaign-ai-coop-20261020/baseline/cairo-authored-ai.json).
- [ ] Establish a reproducible current MCC campaign baseline with actual player
  and NPC birth, normal controls, source AI decisions and script execution.
- [ ] Inventory all campaign maps/BSPs, actor/species/difficulty families,
  command-script owners and script builtins; preserve every unsupported path.
- [x] Inventory the15 prepared MCC campaign scenarios through the native
  content consumer:65 declared BSPs,1,807 authored squads,5,244 script
  definitions and380 distinct builtin names. Squad names/counts/starting-
  location counts, teams and authored BSP fields are retained per map; parsed
  squad/script totals match the native declarations. This inspects BSP0 data
  and scenario-wide metadata; it does not qualify every BSP admission or any
  script/AI behavior. Species/difficulty and original command/builtin owner
  coverage remain open. The inspection runtime is preserved, with cache and
  runtime hashes and opened-file audits in the
  [campaign inventory](../analysis/campaign-ai-coop-20261020/inventory/campaign-v2.json).

## A01 — One campaign world and lifecycle

- [x] Reproduce actual authored Johnson birth through the coherent C55 MCC
  client, without tag conversion changes, supplied actors or source input
  overrides. The new engine probe reports one actor and zero script faults at
  tick 0, then exposes `1CC8D0 -> 32AB71 -> 32ABD8`: the portal module intercepts
  the shared CRT copy routine before the complete Process owner. Current source
  already excludes that interception; qualify the narrow existing route in this
  same client. [C55 source-world attempt](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-cairo.json).
- [x] Qualify that existing engine-owner correction in a private, ABI-consistent
  C55 client: the unchanged authored Johnson path advances through the first
  complete source world tick with one actor, zero script faults and an actual
  checkpoint. Only the CRT dispatch guard changes; all 355 original provider
  files remain unchanged. The new object/client and private source are
  removed after validation, with no runtime selection or tag change.
  [Coherent build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-existing-route.json),
  [Actual MCC frame](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-existing-route-runtime.json).
  This closes the reached copy-owner failure, not AI/navigation/co-op parity.
- [x] Extend the same authored Johnson path to four complete source ticks,
  retaining one actor, zero script faults and a checkpoint with the same
  original provider pins. This is bounded source-frame integration, not an
  observation of combat decisions or original HS scheduling.
  [Four-tick build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-johnson-four-ticks.json),
  [Four-tick MCC result](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-johnson-four-ticks-runtime.json).
- [ ] Use one retained authority for players, objects, actors, commands, HS
  threads, RNG, clocks, navigation and physics; eliminate competing host updates
  on every source-owned path without replacing missing behavior.
- [ ] Complete map admission, original initialization/update ordering, actor
  birth/activation/death/retirement and object/actor/script linkage.
- [ ] Verify salt reuse, parent/child ownership and actual player/NPC lifetimes
  across BSP changes, restart, death, mission exit and repeated map loads.

- [ ] Integrate the complete admitted original map-start lifecycle13A780,
  including all54 non-null callbacks in its68 rows, and let the original
  driver publish loading/ready flags only after real completion. All54
  callback entries have static source bodies, but current session composition
  does not invoke the complete driver. Body presence and bounded tick probes
  do not qualify callback execution, required services or map-start readiness.
  No current native ready-byte state is inferred from this static inventory.
  [Original lifecycle and static ownership inventory](../analysis/campaign-ai-coop-20261020/contrail-init/full-map-start-gate-v1.json).

## A02 — Original actor behavior and combat

- [x] Reproduce authored Cairo Grunt placement's missing `1E8BBF -> 20B980`
  owner and independently execute all 74 original instructions of its settings
  selector and inherited-character lookup (`1ECC80`) across 272 cases. These
  cover all four difficulties, the complete switch, inherited/absent records,
  non-campaign default, source override and positive/zero/negative/NaN settings.
  No instruction patches or supplied dependency returns are used. Native and
  actual campaign qualification of this new owner follows separately.
  [Reached failure](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-four-ticks-runtime.json).
- [x] Port both complete placement-settings owners and match all 272 original
  cases, complete GPR/SIMD state, defined flags, instruction order, ordered
  writes/call-stack footprint and untouched output bytes. The actual root CMake
  normal and ASan/UBSan tests pass, including five absent-owner controls; the
  original fixture repeats byte-for-byte. Wire the leaf into the retained world
  and publish its exact original 16-byte switch as read-only engine constants,
  without supplying tag values or actor records.
  [Root result](../analysis/campaign-ai-coop-20261020/placement-settings/root-v3/report.json),
  [Sanitizers](../analysis/campaign-ai-coop-20261020/placement-settings/sanitizers-v2/report.json),
  [Source pins](../analysis/campaign-ai-coop-20261020/placement-settings/component-proof.json),
  [Reference and regeneration](../tests/fixtures/campaign_ai_placement_settings_tu15.md).
- [x] Compose the new complete settings leaf and its immutable table in the
  private C55 client, using the exact selected World/factory layouts: actual
  MCC `1st_cov_wv1_grt` places three actors and completes four source ticks,
  zero script faults and a checkpoint. All original provider files remain
  unchanged. This closes the reached birth/settings gap; visible tactics,
  navigation, combat and longer continuation remain separate gates.
  [Composed build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-settings-v3.json),
  [Actual authored squad](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-settings-v3-runtime.json).
- [x] Recover whole `29AFD0` and the missing `20B500`/`20B570`/`2802F0`
  predicates: 393 instructions, original switch/translation tables, six reused
  AI-frame owners and two reused campaign iterator owners. All 210 original
  component paths match native GPR/SIMD state, defined flags, instruction order
  and ordered stores. Of these, 202 complete and eight stop at the actual
  unavailable `22CB90` safety-call boundary without a supplied return. Coverage
  executes 391/393 new instructions; the two post-safety-call instructions
  remain unverified. The focused root CMake test passes.
  [Generation](../analysis/campaign-ai-coop-20261020/ai-conditions/generation/generation.json),
  [Root test](../analysis/campaign-ai-coop-20261020/ai-conditions/root-v1/report.json),
  [Sanitizers](../analysis/campaign-ai-coop-20261020/ai-conditions/sanitizers-v1/report.json).
- [x] Compose these original conditions and tables in the coherent C55 client.
  The real three-Grunt run passes the former `29ACAF -> 29AFD0` failure and
  reaches a different missing AI-frame owner at `20AF85 -> 203B60`, while
  retaining a grounded player and zero script faults through tick 10. Original
  providers and MCC cache inputs stay unchanged; no decision or result is
  supplied.
  [Composed build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-conditions-sixty-v1.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-conditions-sixty-v1-runtime.json).
- [ ] Complete `22CB90` and its reached safety dependencies, then verify both
  post-call condition instructions with actual original/native execution.
- [x] Extend the condition module with whole original `203B60` request
  admission, reusing actual `C1E90` and `213D30` owners. The 330-case original
  fixture repeats exactly, and the root normal test passes (307 complete
  cases, 23 explicit dependency-boundary prefixes; 450/453 new instructions).
  Fifteen prefixes enter existing multiplayer `160A60` at its real call ABI;
  they do not supply its result or qualify a live campaign/co-op session.
  [Request generation](../analysis/campaign-ai-coop-20261020/ai-conditions/request-generation-v2/generation.json),
  [Root normal](../analysis/campaign-ai-coop-20261020/ai-conditions/root-v2/report.json),
  [Request sanitizers](../analysis/campaign-ai-coop-20261020/ai-conditions/sanitizers-v2/report.json).
- [x] The composed original request path advances the three real Grunts
  through tick 30, with a grounded player, zero script faults and a checkpoint.
  It passes the former request-owner failure and next reaches missing immutable
  operand `2728F9 -> 46A6C8`. No tag or actor state is substituted.
  [Actual authored continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-requests-sixty-v2-runtime.json).
- [x] Qualify the existing immutable physics-data owner in the coherent client,
  then extend it with the compiled AI frame's exact fixed readonly reads:
  818 original reads, 76 records and 724 bytes. Preserve indexed/writable state,
  tags and relocated clock imports as separate owners. This supplies whole
  `2728C0`'s original double operands (`46A6B8`/`46A6C0`/`46A6C8`) rather than
  introducing individual guessed values. The real authored run passes both
  operand failures and next reaches an unprovided mutable constructor
  destination at `1B07AC -> 56AE84`.
  [Audited literal profile](../analysis/campaign-ai-coop-20261020/ai-conditions/ai-readonly-literals.json),
  [Actual MCC run](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-literals-sixty-v5-runtime.json).
- [x] Pass normal and ASan/UBSan immutable-owner tests after adding the AI
  profile: exact original double bytes, repeat installation, compatible partial
  owners and rejection of wrong/writable/uncommitted aliases without partial
  publication. No clock-import binding is replaced.
  [Normal ownership](../analysis/campaign-ai-coop-20261020/ai-conditions/readonly-owner-normal-v1/report.json),
  [Sanitized ownership](../analysis/campaign-ai-coop-20261020/ai-conditions/readonly-owner-sanitizers-v1/report.json),
  [Qualified source pins](../analysis/campaign-ai-coop-20261020/ai-conditions/component-v2-proof.json).
- [x] Authenticate actual `1B08D9`/`1B08F1` working-mask delivery and whole
  `1B0720`: the destination is exactly 20 bytes in the original loader-zero tail.
  Twenty unchanged-original runs cover squad, property and fallback mask inputs,
  inherited masks and guarded target handling. All five words are written
  before a destination read; four prior-content poisons produce identical
  results per profile. No code patches or supplied dependency returns are used.
  Publish only this bounded cold destination through the existing startup-data
  owner and its exporter; native/campaign qualification follows separately.
  [Original working-mask contract](../analysis/campaign-ai-coop-20261020/ai-working-mask/original.json).
- [x] Match all 20 original working-mask cases through the actual production
  AI/data modules, including GPR/SIMD/defined flags, ordered stores and caller
  stack. The root normal check also verifies exact cold ownership, adjacent
  holes, whole-bank checkpoint/restore and repeated-cold rejection. The real
  three-Grunt run passes the old mask failure and next reaches missing decision
  bitmap storage at `1B0902 -> 4FD524`.
  [Root working-mask check](../analysis/campaign-ai-coop-20261020/ai-working-mask/root-v2/report.json),
  [Working-mask sanitizers](../analysis/campaign-ai-coop-20261020/ai-working-mask/sanitizers-v1/report.json),
  [Actual MCC run](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-working-mask-sixty-v1-runtime.json).
- [x] Authenticate the next gathering-phase workspace from unchanged original
  execution: context word, 131-kind/three-lane/24-byte matrix, 131 count bytes
  and five-word bitmap. Eighteen prefix cases cover high/cross-word IDs,
  repeats, inactive entries and poisoned old contents. Source writes precede
  every watched read; packed records preserve their original padding. This is
  original-only evidence; the complete later decision phase remains open.
  [Original scratch contract](../analysis/campaign-ai-coop-20261020/ai-working-mask/decision-scratch-original.json),
  [Reference details](../tests/fixtures/campaign_ai_working_mask_tu15.md).
- [x] Publish the bounded decision workspace through the existing cold-data
  owner/exporter and match all 18 unchanged-original gathering prefixes in the
  production native frame. Verify GPR/SIMD/defined flags, every ordered store,
  complete workspace bytes, exact extents, poisoned old contents, packed padding
  and whole-bank checkpoint/restore. The focused normal and ASan/UBSan tests
  pass all 38 mask/gathering cases through the actual production AI module.
  [Native workspace qualification](../analysis/campaign-ai-coop-20261020/ai-working-mask/root-v3/report.json).
- [x] Compose all five bounded cold destinations (9,607 bytes) in the coherent
  MCC client. The three real Grunts pass the former workspace failure and
  reach `1B061B -> 47C1AC` during original selection. The canonical exporter
  agrees with the declarations and all five extents are authenticated original
  loader-zero storage. No mask, count, candidate or context result is supplied.
  [Cold ownership](../analysis/campaign-ai-coop-20261020/ai-working-mask/cold-owner-proof.json),
  [Actual workspace continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-decision-scratch-sixty-v2-runtime.json).
- [x] Extend the existing AI descriptor metadata owner with the original
  behavior sentinel operands. Thirty-five compiled-frame reads consume exactly
  ten bytes; the first DWORD already has an owner, so only six missing bytes
  are published. Do not borrow the last two unread file bytes. The normal
  38-case production-module test also checks readonly ownership, the adjacent
  hole and rejection of conflicting/writable owners. The actual MCC run passes
  the former sentinel failure and next reaches an incomplete descriptor child
  record at `1B00A6 -> 47BE0C`; three actors and zero script faults remain
  reported through tick 30. This is no completed 60-tick run.
  [Original sentinel reads](../analysis/campaign-ai-coop-20261020/ai-working-mask/behavior-sentinels-generation.json),
  [Normal qualification](../analysis/campaign-ai-coop-20261020/ai-working-mask/root-v5/report.json),
  [38-case sanitizer qualification](../analysis/campaign-ai-coop-20261020/ai-working-mask/sanitizers-v2/report.json),
  [Actual descriptor frontier](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-behavior-sentinels-sixty-v1-runtime.json).
- [x] Authenticate the original child-list constructor `1AFA90` across all 16
  kind-1 parent lists and 105 authored 20-byte rows. The 96 independent original
  cases exercise all 65 constructor instructions: 95 complete and one retains
  the original empty/null-table opcode-2 failure at `1AFB1C`. No instruction or
  service return is replaced, and regeneration repeats byte-for-byte. Publish
  the selector's 1,050 bytes of ID/flag/timer/seconds fields and the complete
  original 20-byte five-command switch; preserve unread row padding/tails and
  existing owners. Native and actual MCC qualification follow separately.
  [Original constructors](../analysis/campaign-ai-coop-20261020/ai-children/original-constructor-v1.json),
  [Typed field generation](../analysis/campaign-ai-coop-20261020/ai-children/generation-v2.json),
  [Reference details](../tests/fixtures/campaign_ai_children_tu15.md).
- [x] Match all 96 original child-constructor cases through the existing
  production Actor module, alongside the 38 mask/gathering cases. The focused
  normal test passes all 134 comparisons, including complete GPR/SIMD state,
  defined flags, instruction paths, ordered stores/caller stack, untouched
  output bytes, checkpoint restoration and original failure behavior. Verify
  all 1,050 child field bytes/read-only access while leaving row gaps unowned.
  [Normal qualification](../analysis/campaign-ai-coop-20261020/ai-children/root-v2/report.json).
- [x] Repeat all 134 comparisons under normal and ASan/UBSan builds of one
  immutable 67-file current-source cohort. Preserve the concurrent Frame and
  weapon-query-symbol changes by including their real source and exact headers;
  both builds use the production CMake target. No copied source or checkout
  input changes during qualification. This supersedes post-build root hashes
  that could not identify a concurrent edit's compile boundary.
  [Current normal](../analysis/campaign-ai-coop-20261020/ai-children/current-cohort-v1/normal/report.json),
  [Current sanitizers](../analysis/campaign-ai-coop-20261020/ai-children/current-cohort-v1/sanitizers/report.json),
  [Frozen input receipt](../analysis/campaign-ai-coop-20261020/ai-children/current-cohort-v1/source-cohort.json),
  [Qualified component](../analysis/campaign-ai-coop-20261020/ai-children/component-proof-v2.json).
- [x] Compose these exact fields and complete constructor table in the same
  coherent C55 client. The real three-Grunt run passes `1B00A6 -> 47BE0C` and
  next reaches a property-command-inserted child at `1B02AF -> 47D3CC`, still
  reporting three actors and zero script faults through tick 30. Original
  provider files remain unchanged. Preserve the concurrent teleporter work:
  this candidate reconstructs the previously qualified immutable-data publisher
  exactly by its retained source hash, without modifying the root publisher.
  [Coherent child owner](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-child-inputs-sixty-v2.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-child-inputs-sixty-v2-runtime.json).
- [x] Publish the 15 original property-command child rows' 150 selector-input
  bytes through the same literal-data owner. The descriptor and ordinary-child
  profiles remain unchanged. Independently execute all 14 authored property
  lists and whole `1AF970` under zero/A5/FF prior directory contents: 17 cases,
  all 89 producer instructions and complete pointer/header outputs, preserving
  compact-header padding. The original fixture repeats byte-for-byte.
  [Property field generation](../analysis/campaign-ai-coop-20261020/ai-property-children/generation-v1.json),
  [Original directory cases](../analysis/campaign-ai-coop-20261020/ai-property-children/original-v1.json),
  [Required 4,463,734-byte reference](../tests/fixtures/campaign_ai_child_directory_tu15.md).
- [x] Match those 17 cases through the existing production Actor module under
  normal and ASan/UBSan checks in one immutable cohort. Verify complete
  GPR/SIMD state, modeled arithmetic flags (AF excluded), instruction order,
  every ordered store/caller stack, all pointer/header bytes, preserved padding
  and whole-bank restoration. Exact readonly fields/holes and rejection of
  conflicting aliases also pass. Native whole `1AF970` is now qualified for
  these inputs; actual campaign continuation and later callbacks follow.
  [Normal](../analysis/campaign-ai-coop-20261020/ai-property-children/current-cohort-v3/normal/report.json),
  [Sanitizers](../analysis/campaign-ai-coop-20261020/ai-property-children/current-cohort-v3/sanitizers/report.json),
  [Exact source snapshot](../analysis/campaign-ai-coop-20261020/ai-property-children/current-cohort-v3/source-cohort.json).
- [x] Pass the actual authored property-child reads, then publish all 49
  original bounded switch tables consumed by the compiled AI frame: 1,836
  immutable bytes, original guarded extents and same-body branch targets.
  The reached six-lane `1FC0E8` switch has ten independent original guard
  prefixes; these prefixes do not qualify the whole behavior callback.
  All 17 directory comparisons and table ownership/conflict controls pass in
  one normal/ASan/UBSan source cohort. The actual MCC run passes both former
  failures and next reaches the absent scalar `2A144E -> 56E08C` at tick 30.
  [Original table audit](../analysis/campaign-ai-coop-20261020/ai-dispatch/original-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/ai-dispatch/current-cohort-v1/source-cohort.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-dispatch-sixty-v1-runtime.json).
- [x] Implement complete original `_cinit` callbacks `3826A0`, `3826C0` and
  `3826F0` in table `46DD00` order before AI storage/geometry startup. Publish
  only authored immutable operands, identities and four cold destinations;
  the callbacks produce all scalar values. All 11 instructions match across
  36 independent original cases with full GPR/SIMD/flags, NaN payloads, signed
  zero/infinity, poisoned destinations, ordered stores and real caller RET.
  Mounted-producer/checkpoint and missing-source/destination/stack controls
  also pass normal and ASan/UBSan checks in one immutable source cohort.
  [Original callbacks](../analysis/campaign-ai-coop-20261020/ai-scalar-defaults/original-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/ai-scalar-defaults/current-cohort-v2/source-cohort.json).
- [x] Compose those original scalar producers in the same frozen MCC client.
  The real three-Grunt run clears `56E08C` and reaches tick 31, three actors,
  zero script faults and a retained checkpoint. Its next failure is kind2
  callback metadata `1B0EFD -> 48A82C`; 60 ticks remains failing.
  [Coherent build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-scalars-sixty-v1.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-scalars-sixty-v1-runtime.json).
- [x] Complete all 55 kind2 descriptors' two guarded callback identities at
  offsets `44`/`48`: 440 immutable bytes, preserving following records and
  padding ownership. The existing Frame module matches 230 original cases:
  155 complete selectors and 75 prefixes stopped before callback execution.
  All 42 non-call parent instructions execute; both call sites are observed
  at the declared stopping boundaries. Complete ABI, modeled flags (AF
  excluded), instruction paths, ordered stack stores, exact field extents and
  alias controls pass normal and ASan/UBSan checks. The latest immutable
  cohort below reruns these cases with no checkout/source drift.
  [Original selectors](../analysis/campaign-ai-coop-20261020/ai-descriptor-callbacks/original-repeat-v1.json),
  [Authored field regeneration](../analysis/campaign-ai-coop-20261020/ai-descriptor-callbacks/generation-repeat-v2.json).
- [x] Publish the nine authored object-tracking type rows' four consumed WORD
  fields (72 bytes), retaining their IDs and unread tails as separate storage.
  Audit twelve indexed original consumers plus `267320`'s row-pointer reads.
  Match 54 whole `267320` predicates and 18 `274020` constructor prefixes,
  including all 28 predicate instructions, 14/100 constructor instructions,
  poisoned prior storage, complete ABI/stores and preserved output padding.
  This supplies no new index bounds rule and does not qualify the rest of
  the constructor. Also publish the four tracking predicate/handler rows'
  exact callback fields (32 bytes), proving the original descending and
  fallback strides. Handler `2753C0` is absent from the compiled Frame and
  remains an explicit required owner, not a supplied callback result.
  [Original tracking cases](../analysis/campaign-ai-coop-20261020/ai-object-tracking/original-v1.json),
  [Original callback table](../analysis/campaign-ai-coop-20261020/ai-object-tracking/dispatch-original-v2.json),
  [Normal](../analysis/campaign-ai-coop-20261020/ai-object-tracking/current-cohort-v1/normal/report.json),
  [ASan/UBSan](../analysis/campaign-ai-coop-20261020/ai-object-tracking/current-cohort-v1/sanitizers/report.json),
  [Same immutable 230+72-case cohort](../analysis/campaign-ai-coop-20261020/ai-object-tracking/current-cohort-v1/source-cohort.json).
- [x] Pass all preceding metadata/scalar boundaries in the same frozen MCC
  client. The authored three-Grunt run reaches tick 34 with three actors,
  zero script faults and a retained checkpoint. It next requires original
  owner `26E60E -> 26EFA0`; 60 ticks remains failing. The original 355 linked
  provider files stay unchanged; no selected runtime or tag conversion changes.
  [Composed build](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-tracking-dispatch-sixty-v1.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-tracking-dispatch-sixty-v1-runtime.json).
- [x] Port whole `26EFA0` (162 instructions), `26EEF0` (56), `265620` (69)
  and `2753C0` (11), preserving every branch: 298 original instructions.
  Reuse existing Frame `1E7810`, `1ECD00`, `213D30` and `25980` in the same
  bank; no successful nested result is supplied. Mount only their five
  authenticated immutable operands (20 bytes), retaining existing owners.
  The production module matches 244 independent original component cases
  in the first normal build: 234 whole calls and ten prefixes stopped before
  the HS handoff. All query/handler instructions and 276/298 new instruction
  sites execute; later team/HS boundaries and uncovered branches stay open.
  Exact GP/SIMD/modeled flags, paths, every ordered write and prior byte,
  actual RNG mutation, produced actor state, padding and checkpoint restore
  pass normal and ASan/UBSan checks in the same immutable source cohort.
  [Whole-body generation](../analysis/campaign-ai-coop-20261020/ai-object-decisions/generation-v2/generation.json),
  [Independent original cases/coverage](../analysis/campaign-ai-coop-20261020/ai-object-decisions/original-v1.json),
  [Normal native check](../analysis/campaign-ai-coop-20261020/ai-object-decisions/current-cohort-v1/normal/report.json).
  [ASan/UBSan](../analysis/campaign-ai-coop-20261020/ai-object-decisions/current-cohort-v1/sanitizers/report.json),
  [Exact component cohort](../analysis/campaign-ai-coop-20261020/ai-object-decisions/current-cohort-v1/source-cohort.json).
- [x] Pass the reached `26EFA0` owner in the composed MCC client. The three
  Grunts now reach tick 45, with zero script faults and a retained checkpoint.
  The next actual read requires the original scratch-lease latch at
  `1D2D90 -> 52BE58`; 60 ticks remains failing.
  [Composed decision owner](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-object-decisions-sixty-v1.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-object-decisions-sixty-v1-runtime.json).
- [x] Authenticate and publish the complete two-slot scratch-lease storage
  consumed by existing whole `1D2D90`/`1D2E30`: two 12-byte descriptors,
  four-byte heap-use flag and one-byte latch, exactly 29 loader-zero bytes.
  Original producers retain pointer, capacity, occupancy, padding and reset
  ownership. Four independent caller sequences cover 20 whole acquire/release
  calls and two nonnull-heap lock prefixes. Native callers release their own
  returned pointers; no captured workspace pointer or successful lock result
  is supplied. The 244 decision cases and 22 lease cases pass together under
  normal and ASan/UBSan checks. The actual MCC run passes the latch, original
  lease producer and reached heap path, then stops at callback mask `456818`.
  [Original lease lifecycle](../analysis/campaign-ai-coop-20261020/ai-object-decisions/leases-original-v1.json),
  [Native normal check](../analysis/campaign-ai-coop-20261020/ai-object-decisions/leases-cohort-v1/normal/report.json).
  [Sanitizers](../analysis/campaign-ai-coop-20261020/ai-object-decisions/leases-cohort-v1/sanitizers/report.json),
  [Actual MCC lease continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-workspace-leases-sixty-v1-runtime.json).
- [x] Publish all eleven object-callback masks/identities and their original
  null terminator: 70 readonly bytes, preserving unread padding. Validate
  signed-mask extension and x86 shift counts across 36 original/native
  prefixes stopped before callback execution. These cases and the 244
  decisions/22 lease calls pass together under normal and ASan/UBSan checks
  in one immutable 302-case source cohort. The actual MCC client clears
  `456818` and next stops on original context write `26ACAF -> 52D874`,
  still at tick45 with three actors and zero script faults.
  [Original table and prefixes](../analysis/campaign-ai-coop-20261020/ai-object-decisions/callbacks-original-v1.json),
  [Same normal/sanitizer cohort](../analysis/campaign-ai-coop-20261020/ai-object-decisions/callbacks-cohort-v1/source-cohort.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-object-callbacks-sixty-v1-runtime.json).
- [x] Publish the original `26A380` context destinations `52D874` (WORD) and
  `52D878` (DWORD), exactly six cold bytes with the two-byte gap preserved.
  Source producers still supply both values before their iterator/callback
  calls. Cold/publication storage and checkpoint controls pass in the normal
  component build; original `269DF0` remains the existing Frame owner.
- [x] Port both complete callback families at `456818` (11 entries) and
  `456878` (six entries): all 14 absent whole bodies (1,503 instructions) and
  seven absent direct helpers (516), totaling 21 functions/2,019 instructions.
  Reuse existing Frame callees and require existing physical services; no
  callback result is supplied. Authenticate the second table's typed fields,
  bounded switches and operands: 256 readonly bytes, preserving other owners.
  [Complete generation/closure](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/generation-v1/generation.json).
- [x] Match 610 independent original callback cases in the normal build:
  592 whole calls and 18 prefixes stopped before the first `1EA1D0` dependency.
  Cover signed counts, candidate flags/type switches, null candidate paths,
  weighting/filtering, output padding and actual ordered writes. The original
  coverage report lists each function separately; several helpers and later
  branches remain unexecuted, so this does not close full callback behavior.
  All earlier 302 cases also pass in the same normal/ASan/UBSan 912-case
  immutable source cohort. No source snapshot or checkout drift is recorded.
  [Original cases and precise coverage](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/original-v1.json),
  [Native normal check](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/current-cohort-v1/normal/report.json).
  [Sanitizers](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/current-cohort-v1/sanitizers/report.json),
  [Same immutable source cohort](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/current-cohort-v1/source-cohort.json).
- [x] Pass the context declarations and reached callback bodies in the
  composed MCC encounter. The next actual stop is an existing numeric
  dependency at `2CCA66 -> 44ACD8`, still with three actors, zero script
  faults and a retained checkpoint at tick45; 60 ticks remains failing.
  [Composed callback increment](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-callback-bodies-sixty-v1.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-callback-bodies-sixty-v1-runtime.json).
- [x] Audit all 380 existing `2CC8F0` instruction sites and its fifteen fixed
  readonly accesses. Publish the full five operand ranges (24 bytes), including
  the reached eight-byte FLD operand. Three ranges already belong to previous
  immutable declarations; preserve their bytes/access and add the two missing
  ranges (12 bytes). The existing numeric algorithm is unchanged and remains
  separately qualified; no orientation or successful result is supplied.
  [Original operand audit](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/numeric-data-original-v1.json).
- [x] Verify the numeric operands in the composed MCC client. The original
  callback path passes its former numeric-data failure and next stops at the
  registered identity query `140DD9 -> 267A40`, tick46, with three actors,
  zero script faults and the initial checkpoint. The 60-tick gate still fails.
  [Actual continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-callback-numeric-sixty-v1-runtime.json).
- [x] Recover all six original `267A40` instructions, registered by `1E7E00`,
  including full DWORD equality and the actual `RET8`. All 36 independent
  original cases cover every instruction and repeat byte-for-byte, without
  patched instructions or supplied returns. All 948 combined original/native
  cases pass in one immutable normal/ASan/UBSan source cohort without drift.
  [Original query](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/query-identity-original-repeat-v1.json),
  [Complete source generation](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/generation-v2/generation.json),
  [Native query cohort](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/query-cohort-v1/source-cohort.json).
- [x] Pass the registered query in the actual authored MCC continuation.
  The run reaches tick48 with three actors, zero script faults and its initial
  checkpoint, then requires a different original table read at
  `272164 -> 4568F6`. Sixty ticks is still failing.
  [Actual continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-identity-query-sixty-v1-runtime.json).
- [x] Authenticate the complete `272100` selector's original 61 instructions
  and its unchanged Frame owner. Execute964 whole original calls through the
  actual actor/property lookup and `26DA40` classifier, reaching all 61 sites,
  all 14 classifier sites and all twelve consumed WORD cells. Preserve exactly
  24 immutable bytes; the adjacent row remains unowned. Cases include null
  context, guarded kinds, all three produced classes, four column controls,
  target presence and zero/negative/positive/NaN response fields. Original
  references repeat exactly. All 1,912 combined cases pass normal and
  ASan/UBSan checks in one immutable cohort without drift, reusing the existing
  original type-field reference for the nested lookup. The public CMake target
  also passes. These do not qualify every branch in the two existing helpers.
  [Whole original selector](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/prop-selector-original-repeat-v1.json).
  [Native cohort](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/prop-selector-cohort-v2/source-cohort.json),
  [Public target](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/root-v1/report.json).
- [x] Continue the actual three-actor AI frame to 60 ticks using authored MCC
  placement and the original matrix. The run retains three actors, reports
  zero script faults and its initial checkpoint, and performs no reference
  input opens or cache writes. This is bounded headless engine evidence;
  visible combat, checkpoint replay and full mission/co-op remain open.
  [Coherent client](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-prop-selector-sixty-v1.json),
  [Actual sixty ticks](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-prop-selector-sixty-v1-runtime.json).
- [x] Extend the actual authored encounter beyond60 ticks. The300-tick
  attempt reaches tick95 with three actors, zero script faults and its initial
  checkpoint, then requires `1FBED6 -> 1FB3C0`. The 300-tick gate is failing.
  [Reached actor-state boundary](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-prop-selector-threehundred-v1-runtime.json).
- [x] Port all 43 original `1FB3C0` actor-state instructions, reusing existing
  whole Frame `1FB2A0` and `200A40`. All 168 whole original cases reach all 43
  sites, with real nested waypoint/vector and fallback-state publication,
  signed count/index controls, prior-byte poison and full actor/stack outputs.
  Nested helpers' remaining branches stay explicit. No new live or immutable
  data is seeded. All 2,080 combined original/native cases pass normal and
  ASan/UBSan checks in one immutable source cohort without drift.
  [Original actor-state cases](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/actor-state-original-v1.json),
  [Whole source generation](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/actor-state-generation-v1/generation.json).
- [x] Pass the actor-state increment in the actual authored MCC continuation.
  The run reaches109 ticks with three actors, zero script faults and its
  initial checkpoint before requiring the registered query partner
  `140E08 -> 267A30`. The300-tick gate still fails.
  [Actor-state source cohort](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/actor-state-cohort-v1/source-cohort.json),
  [Actual continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-actor-state-threehundred-v1-runtime.json).
- [x] Authenticate all four existing Frame `267A30` instructions and the
  adjacent `1E7E00` registration. Add the missing runtime dispatch route by
  forwarding to this whole existing owner, without generating a duplicate.
  All 286 whole original calls cover every low-byte bucket plus high-word and
  signed key controls, real `RET4`, registers, flags and unchanged caller stack.
  Original fixtures and complete source generation repeat exactly.
  [Original bucket query](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/query-bucket-original-repeat-v1.json),
  [Qualified source reuse](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/generation-repeat-v3/generation.json).
- [x] Qualify the registered query route in the native component and actual
  authored MCC encounter. All 2,366 combined original/native cases pass normal
  and ASan/UBSan in one immutable cohort without drift; the main CMake target
  passes. The encounter completes300 source ticks with three actors, zero
  script faults and its initial checkpoint. All 355 original C55 providers
  retain their pins; no original reference input is opened and no cache write
  is attempted at runtime. No rendered combat or cooperative session is proven.
  [Native source cohort](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/query-bucket-cohort-v1/source-cohort.json),
  [Public CMake target](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/root-v3/report.json),
  [Actual three hundred ticks](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-ai-query-bucket-threehundred-v1-runtime.json).
- [ ] Extend qualification across additional authored actor/species/encounter
  families and longer mission progression. Cover remaining callback
  helpers, branches and physical/lock/team/HS dependencies with original
  owners rather than substituted service results.
- [x] Extend the same coherent current engine increment to three separately
  isolated authored Cairo cases. `1st_cov_wv1_elt` passes300 source ticks with
  one actor and zero script faults. `1st_hum_floor_mar` stops during actual
  birth at `284137 -> 284300`; `1st_hum_johnson` reaches16 ticks before
  `E57EE -> 1EA360`. All355 original providers remain unchanged. These are
  separate actor cases, not combined combat or full encounter acceptance.
  [Complete family cohort](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-authored-families-threehundred-v1.json),
  [Elite](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-authored-families-threehundred-v1-runtime.json),
  [Marine birth boundary](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-authored-families-threehundred-v1-runtime-2.json),
  [Johnson update boundary](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-authored-families-threehundred-v1-runtime-3.json).
- [x] Port the complete original `284300` selector and four birth-query
  variants, retaining their shared code, indirect callback boundaries and
  original breakpoint. There are1,012 distinct original instruction sites
  (1,210 across per-entry CFGs). Reuse the existing writable `.data` sign-mask
  owner; no query context, callback result or successful birth is supplied.
  Compare1,308 whole original calls and420 prefixes stopped before real
  indirect calls. All4,094 combined cases pass normal/ASan/UBSan in one
  immutable cohort without drift; uncovered callback/estimate paths remain
  explicit. [Original variants](../analysis/campaign-ai-coop-20261020/birth-query/original-v1.json),
  [Native source cohort](../analysis/campaign-ai-coop-20261020/birth-query/cohort-v1/source-cohort.json).
- [x] Qualify those birth variants in the actual Marine path: three Marines
  are now born. The next failure is object update `2843A7 -> 2912A0` at tick0,
  before the initial checkpoint. The same private client retains the Elite's
  passing300 ticks. [Actual Marine continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-birth-query-families-threehundred-v1-runtime.json),
  [Elite control](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-birth-query-families-threehundred-v1-runtime-2.json).
- [x] Recover the complete four-target update family of the existing
  `284390` selector: `290B50`, `2912A0`, `291740` and `291E30`,1,373 further
  original sites. Keep all four rather than one reached selector outcome,
  preserving source context/callback ownership and existing scalar/mask data.
  Native comparisons and actual Marine update qualification follow.
  [Complete source generation](../analysis/campaign-ai-coop-20261020/birth-query/generation-v2/generation.json).
- [x] Match 760 whole original update calls and 80 prefixes at real indirect
  callbacks. All 4,934 combined cases pass in one immutable normal/ASan/UBSan
  cohort and the public CMake target. The actual Marine run reaches 30 ticks
  with three actors, no script faults and its initial checkpoint, then needs
  `284372 -> 28CC70`. [Original update cases](../analysis/campaign-ai-coop-20261020/birth-query/update-original-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/birth-query/update-cohort-v1/source-cohort.json),
  [Public CMake result](../analysis/campaign-ai-coop-20261020/birth-query/root-v1/report.json),
  [Actual Marine continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-update-query-families-threehundred-v1-runtime.json).
- [x] Audit all 11 existing query selectors and their 65 static targets.
  Recover the remaining 12 absent bodies together, 3,716 further original
  per-entry sites. The query module now has 21 entries, 6,101 distinct original
  sites /6,299 per-entry sites, and reuses every other existing target body.
  No data/callback result is seeded. This closes the static missing-body
  inventory, not all 65 targets' behavioral acceptance.
  [Complete target inventory](../analysis/campaign-ai-coop-20261020/birth-query/complete-selector-inventory-v1.json),
  [Complete generation](../analysis/campaign-ai-coop-20261020/birth-query/generation-v3/generation.json).
- [x] Execute 1,100 whole original calls and 160 prefixes across those 12
  remaining bodies, preserving the precise uncovered-site inventory. All
  6,194 combined cases pass normal/ASan/UBSan in one immutable source cohort
  without drift. The actual Marine run now reaches 34 ticks with three actors,
  no script faults and its initial checkpoint before AI owner
  `1B0128 -> 1B7240`. The same private client still passes the Elite's 300 ticks.
  [Remaining original calls](../analysis/campaign-ai-coop-20261020/birth-query/remaining-original-repeat-v1.json),
  [Complete native cohort](../analysis/campaign-ai-coop-20261020/birth-query/complete-cohort-v1/source-cohort.json),
  [Actual Marine continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-complete-query-families-threehundred-v1-runtime.json),
  [Elite control](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-complete-query-families-threehundred-v1-runtime-2.json).
- [x] Pass the current complete query source through the main CMake target,
  with all6,194 original/native cases. Original references and complete source
  generation repeat byte-for-byte. No original reference is opened or cache
  write attempted during the actual MCC runs; all355 C55 providers remain
  unchanged. [Current CMake](../analysis/campaign-ai-coop-20261020/birth-query/root-v2/report.json),
  [Source repeat](../analysis/campaign-ai-coop-20261020/birth-query/generation-repeat-v3/generation.json).
- [ ] Complete original `1B7240` and its required owners for the next Marine
  AI path, and original `1EA360` for Johnson's later biped update. Keep the
  real character/global inputs and existing owners; do not supply an actor
  decision or scalar result. [Next original owners](../analysis/campaign-ai-coop-20261020/birth-query/next-ai-owners-v1.json).
- [x] Recover the complete original actor-behavior/scalar direct closure and
  six missing nested Frame dependencies:18 bodies,1,680 original instructions.
  Reuse existing Frame/physics owners and publish only72 authenticated
  read-only operand bytes. Execute808 independent original component cases:
  802 whole returns and six prefixes before the actual `275FE0` object-update
  call. All42 scalar `1EA360` instructions and all42 inherited behavior lookup
  `1ED690` instructions execute; `1B7240` executes99/109 instructions. The
  remaining generated bodies/branches retain explicit coverage limits; native
  comparison and real campaign continuation follow separately.
  [Source generation](../analysis/campaign-ai-coop-20261020/actor-behavior/generation-v2/generation.json),
  [Independent original execution](../analysis/campaign-ai-coop-20261020/actor-behavior/original-v1.json).
- [x] Match the808 behavior/scalar component cases in native execution and
  pass all7,002 combined cases in one immutable normal/ASan/UBSan cohort.
  Declare the reused action/type tables and CRT mode as component inputs;
  initialize the original x87 tag word empty to match the native caller.
  Every original trace, store, register and output remains unchanged by
  those fixture declaration/metadata corrections. The reached Marine call
  passes and then requests `1B7000` at tick34. Johnson passes `1EA360` and
  reaches tick30 before `262590` writes an unbacked `50EEE4` context field.
  The Elite control still passes300 ticks. These results leave full behavior,
  mission, navigation/combat and actual co-op acceptance open.
  [Immutable cohort](../analysis/campaign-ai-coop-20261020/actor-behavior/cohort-v4/source-cohort.json),
  [Corrected independent reference](../analysis/campaign-ai-coop-20261020/actor-behavior/original-inputs-v3.json),
  [Context metadata correction](../analysis/campaign-ai-coop-20261020/actor-behavior/x87-input-metadata-correction-v3.json),
  [Actual four-family client](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-actor-behavior-four-families-threehundred-v1.json).
- [x] Declare the original eight-byte `50EEE4`/`50EEE8` mutable script
  context at cold startup and in the coherent C55 factory. Both DWORDs are
  outside the original section's raw bytes and inside its loader-zero virtual
  range. Original `262590` publishes actor/script IDs before `211840` and
  clears them after return; no live context is seeded. Its24 independent
  cases cover18 whole empty/busy guards and six prefixes before the actual
  callback, observing59/123 instructions and preserving exact writes and
  checkpoints. All7,026 combined original/native cases pass normal,
  ASan/UBSan and the main CMake target. The actual Johnson run passes the
  context writes and enters the real callback, then requires table `480C00`
  at `211B10 -> 211C0D`, tick30. Post-callback cleanup and full command
  execution remain unqualified by these component cases.
  [Original context proof](../analysis/campaign-ai-coop-20261020/actor-behavior/script-context-original-v1.json),
  [Complete cohort](../analysis/campaign-ai-coop-20261020/actor-behavior/cohort-context-v1/source-cohort.json),
  [Main CMake result](../analysis/campaign-ai-coop-20261020/actor-behavior/root-context-v1/report.json),
  [Actual script callback](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-actor-script-context-johnson-threehundred-v1-runtime.json).
- [x] Repeat the generated18-body module,72-byte operand declaration and both
  active original references exactly; verify removal of every owned test
  build, private client, copied source and raw output. Retain6,925,410 bytes
  of required independent reference/audit JSON, with no binaries or captures.
  Selected runtimes and all355 C55 provider pins are preserved. Source hashes,
  normal/sanitizer/root results, actual-family failures, references and cleanup
  are recorded in the
  [increment proof](../analysis/campaign-ai-coop-20261020/actor-behavior/component-proof-v1.json)
  and [reference guide](../tests/fixtures/campaign_ai_actor_behavior_tu15.md).
- [x] Add the complete missing Marine1B7000 dependency group:15 more original
  bodies /809 instructions, including command helpers and the original
  nine-way1D2100 switch. The module now contains33 bodies /2,489 instructions
  and124 authenticated read-only bytes. Its895 new independent original
  cases have880 whole returns and15 real CALL prefixes, with no supplied
  dependency results. Nine new helpers execute every source instruction;
  other bodies retain explicit unexecuted branches. All7,921 combined
  original/native cases pass one immutable normal/ASan/UBSan cohort. Actual
  MCC continuation and the main CMake target follow separately.
  [Generation](../analysis/campaign-ai-coop-20261020/marine-command/generation-v1/generation.json),
  [Original cases](../analysis/campaign-ai-coop-20261020/marine-command/original-v1.json),
  [Immutable cohort](../analysis/campaign-ai-coop-20261020/marine-command/cohort-v1/source-cohort.json).
- [x] Authenticate the complete original HS function registry:948 pointers,
  variable-sized definitions, return/parameter types, flags, evaluator
  addresses and usage literals. The original count word at45B3D0 and packed
  definition extent456B20..45B3D0 agree, and all389 admitted original MCC
  profile signatures match. Preserve the authored3,792-byte `.data` pointer
  registry as mutable; its18,892 definition/count/usage bytes stay read-only.
  Execute956 independent original211B10 cases:952 actual evaluator/script
  CALL prefixes and four whole disabled guards. These qualify70/133 dispatch
  instructions and every table slot, retaining the original null evaluator.
  Evaluator bodies and complete HS execution remain separate requirements.
  The campaign composition exposed shared startup owners; validate exact
  existing bytes/permissions and publish only missing registry ranges.
  [Registry evidence](../analysis/campaign-ai-coop-20261020/mission-functions/registry-generation-v2.json),
  [Original dispatcher](../analysis/campaign-ai-coop-20261020/mission-functions/original-registry-v1.json).
- [x] Pass all8,877 combined original/native cases plus registry ownership
  controls in one immutable normal/ASan/UBSan cohort and the main CMake target.
  The publisher validates shared registry bytes before adding holes, preserving
  existing read-only/mutable owners and rejecting conflicts before admission.
  Actual Marines pass1B7000, and Johnson passes the script-function lookup;
  both reach tick35 with no script faults and an initial checkpoint. Both now
  require the same267750 helper from26C8B0. The Elite control passes300 ticks.
  This does not qualify all948 evaluator bodies or a complete mission/co-op.
  [Final component cohort](../analysis/campaign-ai-coop-20261020/mission-functions/cohort-v2/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/mission-functions/root-v2/report.json),
  [Actual family continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-marine-commands-hs-registry-four-families-threehundred-v2.json).
- [x] Recover the reached26C8B0 parent's22 missing nested bodies /1,395
  original instructions, preserving its existing Frame implementation. The
  behavior module now contains55 bodies /3,884 instructions and156 read-only
  operand bytes. Independently exercise every39-instruction267750 path
  across192 whole original calls. All9,069 combined original/native cases
  pass one immutable normal/ASan/UBSan cohort. Actual Johnson now completes
  300 ticks with no script faults and his initial checkpoint. Marines pass
  the shared property helper and reach tick45 before204AA0 requires105F70.
  Other new branches and callbacks retain their explicit qualification limits.
  [Source closure](../analysis/campaign-ai-coop-20261020/marine-command/prop-generation-final-v1/generation.json),
  [Whole shared helper](../analysis/campaign-ai-coop-20261020/marine-command/shared-original-v1.json),
  [Immutable cohort](../analysis/campaign-ai-coop-20261020/marine-command/prop-cohort-v1/source-cohort.json),
  [Johnson300 ticks](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-prop-parent-closure-four-families-threehundred-v1-runtime-2.json),
  [Next Marine owner](../analysis/campaign-ai-coop-20261020/marine-command/next-marine-tick45-v1.json).
- [x] Pass the main CMake9,069-case target for the final55-body source and
  shared registry publisher. The pending network clone was explicitly
  cancelled after verifying local Git objects for the exact pinned dependency
  and its three submodules. Disposable committed-source snapshots, with the
  project's normal patches, avoid unrelated dirty submodule changes and
  preserve the same component target/filter. The cancelled build and all
  snapshots/private builds are removed; no selected runtime is changed.
  [Main build](../analysis/campaign-ai-coop-20261020/marine-command/prop-root-local-v1/report.json),
  [Exact dependency pins](../analysis/campaign-ai-coop-20261020/marine-command/prop-root-local-v1/dependency-inputs.json).
- [x] Repeat both generated data/code publishers and all new original
  references exactly, then verify removal of owned builds, private clients,
  dependency snapshots and raw outputs. Retain9,556,530 bytes of additional
  independent original reference JSON, with paths/sizes/hashes in the
  [increment proof](../analysis/campaign-ai-coop-20261020/marine-command/component-proof-v1.json).
  No test binaries/captures or installed-runtime changes are retained. The
  final9,069-case cohort has no snapshot or checkout drift; earlier immutable
  cohorts retain any concurrent-checkout drift explicitly in their receipts.
- [x] Recover204AA0's21 missing dependencies /1,260 original instructions,
  including105F70/FF7B0 scalar lookups, byte counters, action guards and
  original CRT branches. The module now has76 bodies /5,144 original sites
  and188 read-only operand bytes. Add the actual32ECE1 FNCLEX operation to
  its existing FP backend/model, preserving condition codes and TOP.
  Independently execute2,232 original component cases:2,227 whole returns
  and five real physical/action CALL prefixes. Of these,840 check status
  capture/clear over exception bits, condition codes, TOP and busy inputs.
  All11,301 combined original/native cases pass normal and ASan/UBSan in one
  immutable cohort without snapshot or checkout drift. The coherent MCC
  Marine run passes105F70 and next stops at203C00's missing64-byte cache
  buffer. Complete actor/CRT behavior and full FP/environment parity remain
  unqualified.
  [Generation](../analysis/campaign-ai-coop-20261020/actor-scalars/generation-v2/generation.json),
  [Independent original cases](../analysis/campaign-ai-coop-20261020/actor-scalars/original-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/actor-scalars/cohort-v1/source-cohort.json).
- [x] Declare203C00's64-byte mutable BSP audibility cache, preserving its
  separate two-byte cluster key and original1CFB79 initialization. Recover no
  duplicate production query bodies and seed no cached mask/key. Independently
  execute105 whole original cache cases across hits/misses, audibility present
  or absent, and counts0 through512. Observe every instruction of203C00,
  252F30 and253070; the nested2531E0 equal-cluster return remains unexecuted
  because its real parents handle equal clusters before calling it.
  The focused target extracts exact existing production bodies into temporary
  build-only source. GP/SIMD/flags, instruction order, prior bytes, ordered
  writes, untouched cache tail and checkpoint restoration agree.
  [Original cache cases](../analysis/campaign-ai-coop-20261020/bsp-audibility/original-v1.json).
- [x] Pass all11,406 combined original/native cases normally, with ASan/UBSan
  and through main CMake using the exact pinned dependency archives. The
  coherent MCC Marine run passes the cache builder and next reaches1F19D0's
  original table read at1F1A3C ->450214. Johnson, Elite and Grunt300-tick
  controls remain separate from complete mission or co-op acceptance.
  [Normal/sanitizer cohort](../analysis/campaign-ai-coop-20261020/bsp-audibility/cohort-v2/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/bsp-audibility/root-local-v1/report.json),
  [Actual authored continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-bsp-audibility-cache-four-families-threehundred-v1.json).
- [x] Recover the complete35-WORD allied-team difficulty-remapping table at
  450210, using the typed source declaration for its bound and checking every
  byte against TU1.5. Retain the existing1F19D0/1F1950/C1E90 bodies. All343
  independent whole original cases cover every instruction in all three
  functions across35 rows, allied/enemy and invalid teams, game modes,
  difficulty bounds and missing global data. All11,749 combined original/native
  cases pass normal, ASan/UBSan and main CMake. The coherent MCC Marine run
  passes this owner and next stops at1578C9 ->47524E, in the existing1578A0
  owner. The table's extent and producer still require original evidence.
  [Authenticated table](../analysis/campaign-ai-coop-20261020/ai-difficulty/generation-v1.json),
  [Whole original cases](../analysis/campaign-ai-coop-20261020/ai-difficulty/original-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/ai-difficulty/cohort-v1/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/ai-difficulty/root-local-v1/report.json),
  [Next actual Marine boundary](../analysis/campaign-ai-coop-20261020/ai-difficulty/next-marine-v1.json).
- [x] Repeat scalar code/operands, cold-storage declarations, difficulty-table
  generation and all three new original fixtures exactly. All16 recorded
  owned source/build/client/dependency paths are removed and the scratch
  parent is empty. Retain14,391,125 bytes of required independent original
  reference JSON; no large binary, memory bank, capture or installed runtime
  is retained or promoted. The final11,749-case immutable cohort has no
  snapshot or checkout drift. Johnson, Elite and three-Grunt controls each
  pass300 source ticks with zero script faults and initial checkpoints;
  Marines retain the explicit45-tick engine-data boundary above.
  [Source pins, comparisons, runtime boundaries and cleanup](../analysis/campaign-ai-coop-20261020/ai-difficulty/component-proof-v1.json).
- [x] Identify47524C as the original seven-WORD camera-effect request type
  table, rather than a live actor/mission result. The recovered typed source
  supplies its bound and writable declaration; all14 bytes match TU1.5 .data.
  Preserve its following two-byte gap and separate director-name table.
  Independently execute204 whole1578A0 request calls, covering all33 source
  instructions,18 overlapping-copy cases and18 mutable type overrides. Timer,
  priority, raw float and exact copy/store behavior come from original source
  execution. Native comparison and actual MCC continuation follow separately.
  [Original authored storage](../analysis/campaign-ai-coop-20261020/camera-effect/generation-v1.json),
  [Whole original requests](../analysis/campaign-ai-coop-20261020/camera-effect/original-v1.json).
- [x] Observe the missing-table regression, then publish only its14 authored
  writable bytes. The whole native request owner agrees in204 cases, including
  full MXCSR checked against unchanged whole original execution on host i386.
  All6,091 original instructions across those hardware cases execute without
  instruction patches or supplied returns. GP/SIMD/flags, normalized trace,
  ordered stores and output bytes also agree with the unchanged emulator
  fixture. Its98 omitted sticky-status cases are preserved explicitly, with
  a separate23,455-byte hardware-status sidecar; native status is not masked.
  Physical Xbox CPU and visual acceptance remain separate.
  All11,953 combined cases pass normal, ASan/UBSan and main CMake. Actual MCC
  Marines pass1578A0 and next stop atCBEEB ->46AB2C. Johnson, Elite and Grunt
  controls each pass300 ticks with zero script faults and initial checkpoints.
  [Original whole-CPU status](../analysis/campaign-ai-coop-20261020/camera-effect/hardware-v1.json),
  [Native cohort](../analysis/campaign-ai-coop-20261020/camera-effect/cohort-v2/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/camera-effect/root-local-v1/report.json),
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-camera-effect-four-families-threehundred-v1.json).
- [x] Inventory the existing damage-delivery owner's fixed read-only operands
  from8,976 authenticated selected-image sites. Its125 fixed reads consume
  128 original .rdata bytes across24 exact regions, includingCBEEB's46AB2C.
  Export only those exact reads. Four indexed operands still require separate
  bounds/ownership evidence; writable fields and live source bindings are
  excluded. This supplies no damage result or original service return.
  [Complete fixed-operand family](../analysis/campaign-ai-coop-20261020/damage-operands/generation-v1.json).
- [x] Publish the complete128-byte fixed damage-operand family, preserving
  exact existing immutable owners. All11,953 combined comparisons and every
  fixed operand/access check pass normal, ASan/UBSan and main CMake. The
  camera hardware sidecar and damage byte inventory add verification, not
  additional native execution cases. Actual MCC Marines now passCBEEB's
  fixed read and next stop at11266F ->44AA04, within the existing112620 damage
  owner. The three indexed tables44A9E0/44AA00/44AA20 require original bounds
  and ownership evidence. The fourth indexed read,450210, already has its
  separately qualified35-WORD difficulty owner. Full damage/mission behavior
  and co-op remain unqualified.
  [Native fixed-data checks](../analysis/campaign-ai-coop-20261020/damage-operands/cohort-v1/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/damage-operands/root-local-v1/report.json),
  [Next actual Marine boundary](../analysis/campaign-ai-coop-20261020/damage-operands/next-marine-v1.json).
- [x] Repeat the camera type, original request/hardware references and fixed
  damage operands exactly. All18 recorded owned source/build/client/dependency
  paths are removed and scratch is empty, including original CPU images,
  objects, raw banks and traces. Retain532,301 bytes of required independent
  original reference JSON; no large binary/capture or promoted runtime remains.
  Final immutable validation finished without snapshot/checkout drift. A later
  concurrent edit changed the large shared physics file; all five extracted
  production bodies and their shared sign-extension helper remain unchanged.
  Preserve that edit and record the whole-file drift rather than treating the
  entire current client as validated.
  [Source pins, scoped evidence and cleanup](../analysis/campaign-ai-coop-20261020/damage-operands/component-proof-v1.json),
  [Post-validation body/hash audit](../analysis/campaign-ai-coop-20261020/damage-operands/post-validation-drift-v1.json).
- [x] Establish the three indexed damage-animation extents from original
  CBC50 selection and CBB90 range writes, not from neighboring padding.
  The selector emits0/1/2/3/5/6/7, and the lower range writes only0/1/2.
  Index7 with DWORD reads requires eight rows in each of44A9E0/44AA00/44AA20:
  96 immutable bytes. Their classes index four authored animation-name IDs at
  47C1F0:16 writable bytes. Row4 retains original bytes but has no claimed
  natural selector reachability. Independently execute446 original cases:
  412 whole returns and34 real actor-limit/animation CALL prefixes. Preserve
  unexecuted branches and callbacks; native/actual MCC qualification follows.
  [Original selection/preparation](../analysis/campaign-ai-coop-20261020/damage-indexed/original-v1.json),
  [Authored indexed data](../analysis/campaign-ai-coop-20261020/damage-indexed/generation-v2.json).
- [x] Qualify all446 original reaction cases against the existing production
  damage module and publish its96 immutable indexed bytes and16 authored
  name-ID bytes. All12,399 combined cases pass normal, ASan/UBSan and the
  main CMake target. Existing read-only/writable owners retain their bytes,
  access and native backing; conflicting metadata fails before missing name
  ranges are admitted. The original C55 factory already owns47C1F8 read-only;
  preserve it rather than repeating the failed overlapping16-byte mount.
  Actual MCC Marines pass11266F's indexed read and next stop at
  274A40 ->274E30, still at45 ticks with three actors, zero script faults and
  an initial checkpoint. No longer Marine run or full damage/animation outcome
  is claimed. The fixture retains its unexecuted sites and34 real CALL
  boundaries; row4 is an explicit ABI control, not a natural selector result.
  [Native cohort](../analysis/campaign-ai-coop-20261020/damage-indexed/cohort-v2/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/damage-indexed/root-local-v1/report.json),
  [Earlier overlap failure](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-damage-indexed-four-families-threehundred-v1-runtime.json),
  [Next actual boundary](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-damage-indexed-four-families-threehundred-v2-runtime.json),
  [Reference guide](../tests/fixtures/campaign_damage_reaction_tu15.md).
- [x] Repeat the original reaction fixture and composed data exactly. The
  final immutable cohort has no snapshot or checkout drift; all355 frozen
  C55 provider pins remain unchanged. Johnson, Elite and three-Grunt controls
  each pass300 ticks with zero script faults and initial checkpoints. No
  original-reference opens or cache writes occur in any of the four probes.
  All14 recorded owned source/build/client/dependency paths are removed and
  scratch is empty. Retain the1,611,598-byte original JSON reference; no binary,
  raw bank, capture or promoted runtime remains. The active goal still
  requires full mission/navigation/combat and real co-op acceptance.
  [Current source pins, scope and cleanup](../analysis/campaign-ai-coop-20261020/damage-indexed/component-proof-v1.json).
- [x] Execute the missing274E30 property-owner lookup against720 independent
  whole original cases, covering every one of its56 instructions. Preserve
  full object-handle equality, related-owner eligibility, signed relative-part
  offsets, the zero-result relative guard and actual property-chain traversal
  through match/rejection/exhaustion. No callback returns or runtime pool
  snapshots are supplied. The native regression first fails at this exact
  missing entry. Generate its complete body in the existing object-decision
  module, preserving all five earlier bodies and their20 immutable operand
  bytes. Native/composed qualification follows separately.
  [Whole original lookup](../analysis/campaign-ai-coop-20261020/prop-owner/original-v1.json),
  [Missing-owner regression](../analysis/campaign-ai-coop-20261020/prop-owner/cohort-red-v1/normal/report.json),
  [Whole-body publication](../analysis/campaign-ai-coop-20261020/prop-owner/publication-v1.json).
- [x] Qualify all720 lookup cases against the recovered whole native owner.
  All13,119 combined cases pass normal, ASan/UBSan and main CMake. The actual
  MCC Marine path now passes274E30 and reaches274A4E ->274450, still at45
  ticks, with three authored actors, zero script faults and an initial
  checkpoint. Preserve the actual no-match result and enter original
  property creation; do not force a matching property handle. Complete
  creation/pool lifetime remains a separate requirement.
  [Immutable native cohort](../analysis/campaign-ai-coop-20261020/prop-owner/cohort-v1/source-cohort.json),
  [Main CMake](../analysis/campaign-ai-coop-20261020/prop-owner/root-local-v1/report.json),
  [Actual lookup continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-prop-owner-four-families-threehundred-v1-runtime.json),
  [Reference and coverage](../tests/fixtures/campaign_ai_prop_owner_lookup_tu15.md).
- [x] Audit the complete next creation family before admitting new owners.
  Only274450 and274490 are absent:50 original instructions. The parent also
  calls274670, whose complete body already exists in Frame; preserve it.
  The creation family reuses existing273B70/274020/274200 and the parent's
  existing274670 path reuses2745E0/274750. The counter/latch fields at4EABCC
  and4EABC8 are four-plus-one original loader-zero bytes; their live storage
  and original producers remain unqualified. Do not publish surrounding
  padding, fabricate an allocation result or duplicate the existing siblings.
  [Original family and owner audit](../analysis/campaign-ai-coop-20261020/prop-owner/next-complete-prop-family-v1.json).
- [x] Repeat the lookup fixture, generated six-body module and immutable
  data exactly. The final coherent snapshot and checkout match. All355 frozen
  C55 provider pins remain unchanged; Johnson, Elite and three-Grunt controls
  each pass300 ticks, with zero script faults and initial checkpoints. None
  of the four probes opens original-reference inputs or writes MCC caches.
  All10 recorded owned source/build/client/dependency paths are removed and
  scratch is empty. Retain the4,900,620-byte original JSON reference; no
  binary, raw bank, capture or promoted runtime remains. Full mission,
  navigation/combat, checkpoint replay and actual co-op are still open.
  [Current validation and cleanup](../analysis/campaign-ai-coop-20261020/prop-owner/component-proof-v1.json).
- [x] Independently execute576 whole original property-creation calls through
  274450/274490 and real existing273B70/274200/274020 classification,
  allocation and initialization. Both missing bodies cover all50 original
  instructions. Exercise first/last object slots, fresh/recycled/full pools,
  priority rejection/admission, salt rollover, epoch wrap and duplicate stamps.
  No allocation or service return is supplied. Dependency branches and full
  reclamation remain explicitly unqualified. Generate the two complete bodies
  in the existing module; all six earlier bodies and20 immutable bytes remain
  unchanged. The native regression first fails at the exact missing274450.
  [Whole original creation with consistent live counts](../analysis/campaign-ai-coop-20261020/prop-create/original-admissible-v2.json),
  [Missing owner regression](../analysis/campaign-ai-coop-20261020/prop-create/cohort-red-v1/normal/report.json),
  [Complete publication](../analysis/campaign-ai-coop-20261020/prop-create/publication-v1.json).
- [x] Establish exact creation storage from original constructor/lookup bounds:
  one latch byte, four epoch bytes and2,048 four-byte per-object stamps,
  totaling8,197 loader-zero writable bytes. OriginalB87F0 supplies2,048 object
  slots; generic header/allocator and valid-object lookup bound stamp indexing.
  Preserve prior mutable values and lifetimes; validate prior access before
  admitting missing fields. Three alignment bytes remain separate. This backs
  original producers without supplying a classified property or allocation.
  Native and MCC qualification follows separately.
  [Object-pool constructor/bounds](../analysis/campaign-ai-coop-20261020/prop-create/object-pool-bounds-v1.json),
  [Exact storage publication](../analysis/campaign-ai-coop-20261020/prop-create/storage-v1.json).
- [x] Correct the creation fixture's recycled-pool live count to match its
  occupied bitmap: two existing live records, not three. Retain the earlier
  original fixture unchanged as a declared-count diagnostic. The corrected
  fixture independently repeats all576 whole original calls and all50 new
  sites; only that corrected576-case fixture executes in the current target.
  This qualifies consumed component fields, not full pool construction or
  every record's gameplay state. Native requalification follows separately.
  [Corrected original input/result record](../analysis/campaign-ai-coop-20261020/prop-create/original-admissible-v2.json).
- [x] Qualify the recovered creation family in the coherent MCC client.
  Three authored Marines pass creation and advance from45 to51 source ticks,
  with zero script faults,11 scripts reported and an initial checkpoint.
  The next stop is21942E ->505E88 in existing219330, under17EAD0. Johnson,
  Elite and three-Grunt controls each retain300 ticks. No shots/hits are
  reported, so changed shield state does not establish combat acceptance.
  The corrected13,695-case target passes normal, ASan/UBSan and main CMake.
  [Actual MCC continuation](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-prop-create-four-families-threehundred-v1.json),
  [Corrected native cohort](../analysis/campaign-ai-coop-20261020/prop-create/cohort-v2/source-cohort.json),
  [Corrected main CMake](../analysis/campaign-ai-coop-20261020/prop-create/root-local-v2/report.json).
- [x] Identify the next field's existing original producer instead of
  supplying zero or a constant:219300 derives505E84/505E88 from52BE68's
  +4/+8 fields and the original46A2FC multiplier. The whole producer already
  exists in campaign activation, and the consumer in contrail geometry.
  Genuine producer inputs, startup/lifecycle ordering and native backing
  remain to be qualified; do not duplicate those owners or fabricate results.
  [Next original producer/consumer](../analysis/campaign-ai-coop-20261020/prop-create/next-contrail-parameters-v1.json).
- [x] Finish the corrected13,695-case normal, ASan/UBSan and main CMake
  checks. All192 recycled-pool controls now have bitmap-consistent live counts
  in their inputs and original outputs; the earlier reference is preserved and
  excluded from execution totals. Both original references and generated
  module/data/storage repeat exactly. The final snapshot/checkout match,
  all355 C55 provider pins remain unchanged, and none of four runtime probes
  opens original references or writes MCC caches. All16 recorded owned paths
  are removed and scratch is empty. Retain11,547,164 bytes of original JSON
  references (current plus diagnostic); no binary, raw bank, capture or
  promoted runtime remains. Full engine/co-op acceptance is still open.
  [Current corrected proof and cleanup](../analysis/campaign-ai-coop-20261020/prop-create/component-proof-v2.json),
  [Creation reference guide](../tests/fixtures/campaign_ai_prop_creation_tu15.md).
- [x] Revalidate the51-tick contrail stop against current production source.
  Current scalar storage already declares505E84/505E88 and the complete noise
  field, and current world initialization already executes real1F3830,
  11E550 and219300. Authenticated lifecycle rows24/25/42
  confirm those owners; the later session-order audit below identifies the
  constructor/session seed ordering defect. The older C55 world omits the two new-map calls; its
  stop is historical client evidence, not a newly proven current-source bug.
  Preserve the existing implementations instead of adding duplicate owners
  or supplying scalar/noise outputs.
  [Current-source/older-client audit](../analysis/campaign-ai-coop-20261020/contrail-init/current-versus-c55-v1.json).
- [ ] Build and exercise the complete current-source native client in an
  immutable disposable snapshot, with exact committed dependencies and the
  existing whole original activation checks. Run all four MCC actor cases
  against that same newly linked client, retaining startup/source/runtime
  failures without substituting older providers or readiness results.
  The first full native build succeeds and original activation passes. Johnson,
  Elite and three-Grunt cases separately pass300 ticks; three Marines reach63
  with zero script faults and an initial checkpoint before1B8612 calls missing
  1B7D20. That snapshot is unchanged; recorded later checkout edits are outside
  its qualification.
  [First complete current-source results](../analysis/campaign-ai-coop-20261020/current-source-v1/current-source-cohort.json).
- [x] Establish original session admission before noise/RNG initialization:
  whole13A8B0 transfers the admitted seed, followed by registered new-map
  rows24/25/42 (11E550/1F38D0/219300). Independent original cases for two seeds
  compare all3,340 noise bytes and eight scalar bytes. Replaying admission
  afterward loses24 actual RNG draws. Preserve original direction/scalar
  inputs and algorithms; move existing initialization into configure_session
  after clock/engine/world setup and before cinematic/HS/AI initialization.
  [Original session order](../analysis/campaign-ai-coop-20261020/contrail-init/session-seed-order-v1.json),
  [Repeatable original two-seed reference](../analysis/campaign-ai-coop-20261020/contrail-init/seed-reference-repeat-v1.json).
- [ ] Qualify the corrected session order in the complete native world for both
  original seeds, then rerun original activation and all four actor cases.
  A concurrent source edit rejected capture v2 before compilation; its owned
  snapshot is removed. V3 builds and six of seven tests pass: activation, both
  seeds and three300-tick controls. Marine stops at125 at missing1B7D20. The
  immutable snapshot is unchanged and its owned source/builds are removed.
  Recorded checkout drift includes the later actor extension and concurrent
  work; it does not change this snapshot qualification.
  [Post-fix current-source qualification](../analysis/campaign-ai-coop-20261020/current-source-v3/current-source-cohort.json).
- [x] Match the complete native campaign world's noise/scalar outputs for both
  original seeds after the source-order correction: each compares all3,340
  noise bytes and eight scalar bytes, with no supplied runtime outputs. The
  full v3 client builds; its actual Marine encounter advances125 ticks with
  three actors, zero script faults and an initial checkpoint before the same
  missing1B7D20 owner. Original activation and Johnson/Elite/three-Grunt
  controls each pass300 ticks in the same completed batch. Its overall result
  remains failed because Marine is incomplete. The new90-body module is being
  composed separately in v4.
  [Original reference and native test contract](../tests/fixtures/campaign_session_seed_order_tu15.md),
  [Full v3 batch](../analysis/campaign-ai-coop-20261020/current-source-v3/current-source-cohort.json).
  [Complete next families](../analysis/campaign-ai-coop-20261020/ai-object-decisions/next-complete-callback-families.json).
  Preserve the older missing-owner reports
  as regression evidence, including the
  [initial 60-tick attempt](../analysis/campaign-ai-coop-20261020/ai-runtime/c55-grunts-sixty-ticks-v1-runtime.json).
- [x] Authenticate both complete original six-row projection/inverse tables
  used by existing218FE0/219030,72 immutable bytes at44AC8C/44ACB0. The
  existing native world owns the36-byte forward table; add only the inverse. Original
  123210 returns axes0..2 and14CEA0 forms Boolean orientation, establishing
  the six-row family. Independently execute1,152 whole calls covering both
  complete17-instruction bodies, all selectors, raw float bit patterns,
  poisoned upper selectors and overlapping input/output. Preserve malformed
  metadata as unqualified; supply no path/coordinate/query outcome.
  [Original reference](../analysis/campaign-ai-coop-20261020/decision-geometry/original-v1.json),
  [Reached original owner](../analysis/campaign-ai-coop-20261020/decision-geometry/discovery-v1.json).
- [ ] Qualify the two unchanged geometry owners and their shared72-byte read-only
  publication in the expanded15,125-case native target and actual MCC actor
  encounters. V5 passes the original activation and15,125-case isolated
  component target, but all six native startup/actor cases fail because the
  new forward publication duplicates the existing world provider. The source
  guard reports exactly the declared helper correction; its exact compiled
  slice is audited without overriding those failures. V5 is not runtime
  acceptance and its owned source/builds are removed. The corrected publisher
  now reuses that forward owner and adds only36 inverse bytes. The component
  test composes the exact existing world publication with final startup to
  catch duplicate ownership; corrected component-v4 normal checks pass and
  ASan/UBSan also passes in the same unchanged snapshot and captured checkout.
  Its owned component source/builds are removed. Full immutable v6 also runs
  cold-world ownership.
  [V5 failure and helper audit](../analysis/campaign-ai-coop-20261020/current-source-v5/declared-refresh-and-failure-audit.json),
  [Corrected original/data reference](../analysis/campaign-ai-coop-20261020/decision-geometry/original-reused-forward-v2.json),
  [Composed component checks](../analysis/campaign-ai-coop-20261020/decision-geometry/component-v4/source-cohort.json),
  [Full v6 client](../analysis/campaign-ai-coop-20261020/current-source-v6/current-source-cohort.json).
- [x] Pass all15,125 combined native component cases normally and with
  ASan/UBSan in the corrected immutable component-v2 cohort. All1,152 original
  geometry calls and the72-byte data include repeat exactly; both original
  geometry bodies remain unchanged. The component snapshot is unchanged;
  concurrent callable-data edits are recorded separately. Owned component
  source/builds are removed. Retain the required1,815,380-byte original
  reference and no binary, raw bank or capture. This earlier isolated v2
  result does not qualify the native world composition: v5 subsequently
  exposes duplicate forward ownership. The corrected composition is tracked
  above and the original1,152-case reference remains unchanged.
  [Component evidence and cleanup](../analysis/campaign-ai-coop-20261020/decision-geometry/component-proof-v2.json).
- [x] Qualify the corrected provider composition with all15,125 cases normal
  and ASan/UBSan. Preserve the exact existing forward mount/encoder and both
  geometry bodies; publish only the36 inverse bytes. The original1,152 cases
  and inverse-only include repeat exactly. Snapshot and captured checkout
  match, and the owned component source/builds are removed. No new reference
  is added by this correction; the existing1,815,380-byte original fixture is
  preserved. Full v6 runtime/cold-world acceptance remains pending.
  [Corrected composition proof](../analysis/campaign-ai-coop-20261020/decision-geometry/component-proof-corrected-v4.json).
- [x] Validate the corrected inverse-only publication in the complete v6
  native client: cold-world ownership, original activation, the15,125-case
  main component target, both native seed regressions and three300-tick actor
  controls pass. Marines progress to199 ticks with three actors, zero script
  faults and an initial checkpoint, then original1AFDE0 stores through1AFE1B
  into missing child-row link-head47BE14. Snapshot is unchanged; later checkout
  edits are recorded. All four probes use prepared MCC caches, open no original
  references and write no MCC caches. Owned v6 source/builds are removed.
  [Full native v6 proof and cleanup](../analysis/campaign-ai-coop-20261020/decision-geometry/full-native-proof-v6.json).
- [x] Recover exact mutable child-row link-head storage and compare whole
  original1AFDE0 command-list construction/sorting in722 complete original
  calls. The unchanged native Frame owner passes the15,847-case combined
  component target normally and with ASan/UBSan. The snapshot is unchanged;
  owned component source/builds are removed. Full v7 checks also pass this
  target; the actual Marine path progresses to283 ticks.
  Publication preserves existing mutable values/backing, rejects read-only
  conflicts before admitting missing heads, and retains authored headers,
  timers and unowned row gaps. Exact stores, registers, flags, instruction
  order and checkpoint restoration agree. Original execution and the include
  repeat exactly;137 of142 static sites execute, with five unobserved sites
  explicitly unqualified. Retain the required2,596,841-byte original reference.
  [Original reference](../analysis/campaign-ai-coop-20261020/behavior-links/original-v1.json),
  [Independent repeat](../analysis/campaign-ai-coop-20261020/behavior-links/original-repeat-v2.json),
  [Component cohort](../analysis/campaign-ai-coop-20261020/behavior-links/component-v1/source-cohort.json),
  [Component proof and cleanup](../analysis/campaign-ai-coop-20261020/behavior-links/component-proof-v1.json),
  [Reference guide](../tests/fixtures/campaign_ai_child_links_tu15.md).
- [x] Compose the recovered child heads and whole list owner in the complete
  current client and continue the199-tick Marine path.1AFE1B is a write, not
  a missing immutable scalar. Preserve
  authored row headers/timers and source-produced lists; do not seed links.
  The authenticated family is120 unique rows (105 authored children plus15
  authored property commands),480 exact writable head bytes. All original
  head inputs are zero;1AFDE0 still owns clearing and linking. Do not include
  unread row tails or overwrite existing headers/timers.
  [Original writer and complete body](../analysis/campaign-ai-coop-20261020/descriptor-generation/discovery-v1.json),
  [Complete authored head family](../analysis/campaign-ai-coop-20261020/behavior-links/authored-head-family-v1.json).
- [x] Qualify those heads in the complete unchanged v7 client: cold-world
  ownership, original activation,15,847 component cases, both seed regressions
  and all three300-tick controls pass. Three Marines reach283 ticks with zero
  script faults and an initial checkpoint, then1B8938 requires the missing
  whole200C00 path-distance owner (84 instructions; existing218780 transform
  dependency). No lists, path distances or successful queries are supplied.
  All probes use prepared MCC input, open no original references and write no
  MCC caches. Later checkout drift is recorded; owned source/builds are removed.
  [Full v7 proof and cleanup](../analysis/campaign-ai-coop-20261020/behavior-links/full-native-proof-v7.json),
  [Next complete original body](../analysis/campaign-ai-coop-20261020/actor-current/after-283-discovery-v1.json).
- [x] Recover whole200C00, preserving all90 earlier actor bodies and all212
  immutable bytes. The module now has91 bodies /6,964 original sites. Original
  execution independently repeats100 cases:96 whole finite world-space calls
  and four prefixes before real218780. All15,947 combined native component
  cases pass normally and with ASan/UBSan in the same unchanged snapshot;
  owned component source/builds are removed. Full v9 passes this target after
  the v8 capture was rejected before building. Original
  x87 ordering, guards, resulting float bits, registers, modeled flags, exact
  stores and checkpoint restoration agree.81 of84 sites execute; the actual
  transform call/return guard remains unqualified here, without supplied
  transform results. Retain the required296,037-byte original reference.
  [Generation preservation](../analysis/campaign-ai-coop-20261020/path-distance/generation-v1/preservation.json),
  [Repeated generation](../analysis/campaign-ai-coop-20261020/path-distance/generation-repeat-v2/generation.json),
  [Original execution repeat](../analysis/campaign-ai-coop-20261020/path-distance/original-repeat-v2.json),
  [Component cohort](../analysis/campaign-ai-coop-20261020/path-distance/component-v1/source-cohort.json),
  [Component proof and cleanup](../analysis/campaign-ai-coop-20261020/path-distance/component-proof-v1.json),
  [Reference guide](../tests/fixtures/campaign_ai_path_distance_tu15.md).
- [ ] Compose whole200C00 in the actual Marine path beyond283 ticks; qualify
  transformed-path completion, live checkpoint replay and wider navigation.
- [x] Build and validate the complete unchanged v9 client including200C00.
  Cold-world ownership, original activation,15,947 component cases, both seed
  regressions and the three300-tick controls pass. Three Marines reach288
  ticks with zero script faults and an initial checkpoint, then original
  1B0195 indirectly calls missing1BD1E0. Descriptor48ABE0 is original kind1,
  directory index27; its+0x10 callback field supplies that exact identity.
  The source snapshots include concurrent work and are not a controlled
  binary A/B; a separate live trace of200C00 is not captured. All four probes
  remain MCC-only with no cache writes. Later checkout drift is recorded and
  all owned v9 source/builds are removed. No full Marine case, transformed
  path, mission/combat or cooperative session is qualified.
  [Full v9 proof and cleanup](../analysis/campaign-ai-coop-20261020/path-distance/full-native-proof-v9.json),
  [Original callback and caller](../analysis/campaign-ai-coop-20261020/path-distance/after-288-discovery-v1.json).
- [x] Recover whole1BD1E0 and its missing1ED100 callee (148 original sites
  across two bodies), preserving the existing267450,278DD0 and27C710 owners.
  Qualify callback guards and real dependency prefixes/returns before
  continuing the actual288-tick Marine path; supply no behavior result.
  [Complete static missing closure](../analysis/campaign-ai-coop-20261020/path-distance/after-288-closure-v1.json).
- [x] Qualify372 complete original/native callback, character-data and iterator
  calls. Every new1BD1E0/1ED100 site and every existing278DD0/267450 site
  executes;27C710's squad fallback remains unqualified (34/50 sites execute).
  Preserve all91 previous bodies and212 immutable bytes; generation and
  original execution repeat exactly. All16,319 combined cases pass normally
  and with ASan/UBSan in one unchanged snapshot. Exact registers, modeled
  flags, instruction order, every store/prior byte and checkpoint restoration
  agree. Owned source/builds are removed; the only captured checkout drift is
  a later extractor opening docstring, proven against its tested hash. Retain
  the required1,510,016-byte original reference and no binary/raw bank.
  [Generation preservation](../analysis/campaign-ai-coop-20261020/behavior-27/generation-v1/preservation.json),
  [Original repeat](../analysis/campaign-ai-coop-20261020/behavior-27/original-repeat-v2.json),
  [Component proof and cleanup](../analysis/campaign-ai-coop-20261020/behavior-27/component-proof-v1.json),
  [Reference guide](../tests/fixtures/campaign_ai_behavior_27_tu15.md).
- [ ] Compose the recovered descriptor27 callback in the complete current
  client and continue beyond288 ticks. V10 requests900 ticks per encounter;
  probe tools now permit finite requests up to1,800 without changing their
  existing defaults or the engine tick rate. Longer runs do not qualify full
  mission/navigation/combat, checkpoint replay or a cooperative session.
- [ ] Complete and connect perception, targeting, allegiance, alertness,
  decisions, orders and command ownership using authenticated original owners.
- [x] Recover the authenticated14-body/1,736-site static closure of reached
  1B7D20. Preserve all76 previous bodies and188 immutable bytes exactly;
  export only24 additional original read-only bytes. Independently repeat278
  original component cases (230 whole calls,48 service prefixes), including
  every instruction in277860/2778E0. No planning, placement, collision or
  allocation result is supplied. Other paths remain explicitly unqualified.
  [Original coverage](../analysis/campaign-ai-coop-20261020/actor-current/reference-repeat-v1.json),
  [Preservation](../analysis/campaign-ai-coop-20261020/actor-current/generation-v2/preservation.json).
- [x] Qualify the90-body/6,880-site module in native normal and ASan/UBSan
  checks. All13,973 combined component cases pass in one unchanged snapshot;
  the checkout also matches its captured input closure. Module/data generation
  repeats exactly. The owned component source/builds are removed. Retain the
  required1,123,199-byte original reference, no component binary/raw bank.
  [Component proof and cleanup](../analysis/campaign-ai-coop-20261020/actor-current/component-proof-v1.json).
- [ ] Compose the actual Marine decision path in a complete current client.
  The immutable v4 full client builds and passes the production13,973-case
  component target, both native seed regressions, original activation and
  three300-tick actor controls. Marines enter1B7D20 and reach219041 in existing
  219030, missing44ACD2. This qualifies entry/composition, not complete
  1B7D20 return or the whole Marine case. Its owned source/builds are removed. V3/V4 capture concurrent work
  independently; their recorded source delta is not a controlled binary A/B.
  Full navigation/combat remains open regardless of these component results.
  [Component checks](../analysis/campaign-ai-coop-20261020/actor-current/component-v1/source-cohort.json),
  [Repeat generation](../analysis/campaign-ai-coop-20261020/actor-current/generation-repeat-v2.json),
  [Complete v4 client](../analysis/campaign-ai-coop-20261020/current-source-v4/current-source-cohort.json),
  [Reference guide](../tests/fixtures/campaign_ai_decision_workspace_tu15.md),
  [Complete-client proof and next actual boundary](../analysis/campaign-ai-coop-20261020/actor-current/full-client-proof-v4.json).

- [ ] Integrate original unit controls, weapons, aiming, damage/hit regions,
  melee, cover, tactical animation and applicable vehicle/seat behavior.
- [ ] Exercise applicable species, difficulties, encounter activation and
  reinforcement/despawn paths through actual authored MCC encounters.

## A03 — Navigation

- [ ] Recover original path costs, search, hints, movement admission and dynamic
  obstacles; the host geometric `NavigationWorld` route is not retail parity.
- [ ] Integrate grounded/crouching/jumping, flying/swarm and vehicle navigation
  where authored, including blocked/replanned routes and BSP transitions.
- [ ] Compare complete original/native routes and movement decisions, then
  validate visible NPC traversal and combat without supplied paths or outcomes.

## A04 — Mission and command scripts

- [x] Authenticate TU1.5 `players`/cooperative evaluator definitions and repeat
  unchanged original execution: 25 cooperative predicates and 161 whole player
  lists, including actual original allocation, with no instruction patches or
  supplied dependency returns. The retained
  [fixture](../tests/fixtures/campaign_player_queries_tu15.json) repeats exactly
  with SHA256 `c3b9c88bd0cda3be956622f71c9f2b3520adbc85777a8b9d10567c6c92fd2ea7`.
- [x] Match the native source-bank query component against those 186 original
  cases, complete allocated header/reference bytes, snapshot/restore and missing-
  owner controls under ASan/UBSan. The isolated source cohort and private build
  are removed. [Sanitizer result](../analysis/campaign-ai-coop-20261020/player-queries/isolated-sanitizers-v1/report.json).
  This is component evidence; caller qualification follows below and actual
  MCC campaign execution remains a separate gate.
- [x] Wire both queries into the campaign Simulation host, retain full local
  player unit identities through unit/center/hide/teleport consumers, and reuse
  the existing HS list checkpoint/GC owner. The isolated coherent Simulation
  object compiles and normal component CTest passes; the actual root CMake
  target also passes. [Caller compilation](../analysis/campaign-ai-coop-20261020/player-queries/isolated-normal-v4/report.json),
  [root result](../analysis/campaign-ai-coop-20261020/player-queries/root-v2/report.json).
- [ ] Complete original HS scheduling, expression evaluation, sleep/wake and
  command-script ownership; preserve original namespace, types and timing.
- [ ] Verify source player-list and cooperative-query consumers in an actual
  two-player MCC session, including dead/retired players and checkpoint restore.
  The source-bank component/caller increment above does not create that session.
- [ ] Complete required mission builtins and actor commands, triggers, object
  controls, dialogue/cinematics and progression; fail on unsupported owners.
- [ ] Verify startup, encounter progression, mission completion and restart
  through normal authored scripts, without manually setting progress/readiness.

## A05 — Checkpoints and mission progression

- [ ] Restore players, objects, actor decisions, paths, commands/HS stacks,
  globals, RNG and clocks together through the existing authority transaction.
- [ ] Rebind borrowed pools only after publication; reject foreign/stale input
  and preserve the previous world after failed preparation/publication.
- [ ] Verify repeated save/revert, death/restart, cross-BSP checkpoints and
  continued encounter decisions against original behavior.

## C01 — Cooperative participants, input and views

- [x] Establish original cooperative-option admission rather than infer an
  engine-produced readiness flag. Whole13A8B0 copies the supplied0x1118-byte
  options record to game+8, preserving input byte12C at game+134. Eight
  independent original admission calls (four byte values/two prior patterns)
  complete, followed by eight real2B3790 evaluator prefixes stopping before
  211DA0. Zero selects false; nonzero selects true only with campaign kind1.
  This proves an admitted option's data flow, not a cooperative session.
  Native admission must couple that request to actual participants, source
  birth and view/script/checkpoint ownership; setting the byte alone closes
  none of those gates. Candidate opcode scans remain unqualified.
  [Original admission and query prefixes](../analysis/campaign-ai-coop-20261020/coop-session/option-admission-original-v1.json).
- [ ] Create real additional campaign players/bipeds through original birth
  owners and retain full salted identities in the shared campaign world.
- [ ] Deliver independent per-player controls, player lists, camera/HUD/first-
  person views and local view ownership; preserve existing solo behavior.
- [ ] Launch an actual cooperative session from the frontend and return cleanly.

## C02 — Cooperative death and safe respawn

- [x] Locate the existing live frame route: step_objects calls whole14FEC0,
  whose original cooperative branch calls14F8F0. The current complete native
  bodies for14FEC0,14F8F0,CE650,1D2920,151DA0 and151490 retain their full
  original instruction-site sets. Reuse these owners; absence of a call to
  the separate Map Pack component API does not mean the live scheduler is
  missing. This is static ownership evidence, not live co-op acceptance.
  [Original calls and current body hashes](../analysis/campaign-ai-coop-20261020/coop-session/current-frame-owners-v1.json).
- [ ] Exercise that original cooperative branch with real campaign players,
  action/AI-combat/biped safety, source birth and safe-placement services.
- [ ] Verify living/dead player combinations, cinematics, unsafe survivors,
  failed birth/placement, difficulty rules and all-dead checkpoint reversion.
- [ ] Retain corpses and camera ownership through death, respawn, vehicle
  exit, participant replacement and checkpoint restoration.

## C03 — Shared scripts, BSPs and checkpoints

- [ ] Run mission/command scripts once against the actual cooperative player
  set, including player-dependent trigger and cinematic behavior.
- [ ] Synchronize BSP admission/transition, encounter activation, pause,
  checkpoint creation/revert and mission completion across participants.
- [ ] Restore the complete shared world and participant-local views without
  duplicate script events, stale actor references or independent RNG divergence.

## C04 — Network co-op

- [ ] Carry authoritative campaign inputs/state, player/object birth/death,
  actor/command/script results and mission events through the native session.
- [ ] Deliver coupled BSP/checkpoint world restoration with participant/session
  identity and rejection of stale, incomplete or foreign transfers.
- [ ] Verify actual LAN play, loss/delay/recovery, disconnect/rejoin and the
  original applicable session rules; camera snapshots alone do not qualify.

## V01 — Acceptance and delivery

- [ ] Run focused original/native comparisons and meaningful failing controls
  for each changed behavior using disposable test storage.
- [ ] Pass composed player/NPC/script/checkpoint tests using current MCC data.
- [ ] Verify complete authored mission playthroughs in solo, local co-op and
  network co-op, covering the inventory rather than one bounded encounter.
- [ ] Verify visible actor navigation/combat, independent player views and
  synchronized audio/presentation with original evidence; keep shader/audio
  dependencies explicit rather than weakening these checks.
- [ ] Deliver the coherently built runtime through the actual launchers and
  exercise normal start, death/revert, transitions, completion and menu return.
- [ ] Verify generated build/scratch removal; retain only bounded reports and
  explicitly required reference artifacts with paths and sizes.

## Current work and evidence ledger

1. Continue the coherent MCC actor/frame qualification across additional
   authored encounters and original engine paths. C55 Johnson, Grunt and
   Elite cases separately pass300 source ticks; other families and maps remain
   open. Current
   source has `209D04` in
   `src/native_tu15_campaign_runtime_tables.inc` and `41CD1C` within the complete
   `41CD08` table in `src/native_tu15_physics_callable_data.inc`. The selected
   runtime baseline predates these qualifications; do not duplicate the ports
   or install a mixed-layout client. Real player controls, decision execution
   and mission scheduling are still required before closing the baseline gate.
2. Continue the actual Marine decision path beyond288 ticks. V9 passes
   cold-world ownership, original activation, the15,947-case main component
   target, both seed regressions and the three300-tick controls. The original
   forward provider is preserved and only36 inverse bytes are added; normal
   and sanitizer component checks also pass. V9 snapshot is unchanged and
   owned source/builds are removed. At199, existing1AFDE0 reaches1AFE1B's
   real DWORD store into child-row+0xC (47BE14). The complete480-byte mutable
   head family and unchanged list owner now pass722 original comparisons and
   all15,847 combined normal/sanitizer component cases. Complete v7 composition
   now passes and advances to283, where1B8938 requires200C00. That complete
   84-instruction distance owner is now ported; all15,947 component
   cases pass normal and sanitizer checks. Complete v9 composition reaches
   288, then1B0195 invokes descriptor27's missing1BD1E0 callback. That body
   and1ED100 (148 original sites) are now ported, reusing existing dependencies.
   All16,319 component cases pass normal/sanitizer checks; complete v10
   composition requests900 ticks per encounter.
   Complete map-start callbacks, navigation/
   combat, command/HS scheduling, checkpoints and co-op remain open.
   [Full v6 proof](../analysis/campaign-ai-coop-20261020/decision-geometry/full-native-proof-v6.json),
   [Full v7 proof](../analysis/campaign-ai-coop-20261020/behavior-links/full-native-proof-v7.json),
   [Full v9 proof](../analysis/campaign-ai-coop-20261020/path-distance/full-native-proof-v9.json),
   [Next original writer](../analysis/campaign-ai-coop-20261020/descriptor-generation/discovery-v1.json).
3. Qualify original player birth/placement/scheduler against live campaign
   records, then add real participants and shared checkpoints/BSPs/views.
   Whole `151DA0`, `151490`, `14F8A0`, `14F8F0`, `1D2920` and `CE650` bodies
   already exist in `physics_live_support_tu15_generated.cpp`; do not duplicate
   those ports or treat their presence as a qualified cooperative session.

Validation commands and source hashes are retained with the normal/root and
sanitizer reports linked above. `tools/test_campaign_player_queries_tu15.py`
reproduces the small source cohort through `tools/run_tests.py`; the
[fixture instructions](../tests/fixtures/campaign_player_queries_tu15.md) record
original regeneration. Generated source/build trees and raw baseline traces are
removed; no installed runtime is retained or promoted by this work.
The working-storage/sentinel increment retains
[qualified source pins](../analysis/campaign-ai-coop-20261020/ai-working-mask/workspace-component-pins.json)
and [cleanup verification](../analysis/campaign-ai-coop-20261020/ai-working-mask/cleanup.json).
Its two required independent original reference inputs total 495,540 bytes;
the owned scratch directory is empty and unrelated active builds are preserved.
The child constructor increment adds one 356,738-byte required original input;
its temporary source cohort, private clients, objects, builds and raw traces
are removed. [Child increment cleanup](../analysis/campaign-ai-coop-20261020/ai-children/cleanup.json).
The property/directory, switch, scalar and tracking increments retain six
required independent original JSON references totaling 4,963,651 bytes. The
largest is the 4,463,734-byte whole-directory reference. Their exact paths,
sizes and hashes, four passing component cohorts, composed MCC boundaries
and removal of every owned source/build/client directory are recorded in the
[current proof and cleanup receipt](../analysis/campaign-ai-coop-20261020/ai-object-tracking/component-proof-v1.json).
No new large binary, raw memory bank, render capture or installed runtime is
retained. The public CMake scalar and whole-directory targets also
[pass together](../analysis/campaign-ai-coop-20261020/ai-scalar-defaults/root-v1/report.json).
The decision/lease/object-callback increment adds three required original
references totaling 1,285,830 bytes; the nine references across these two
increments total 6,249,481 bytes. Its 302-case normal/ASan/UBSan source cohort
and public CMake target pass. All generated source/build/client paths are
removed, with source hashes, reference paths/sizes and scoped runtime failures
recorded in the [decision increment proof and cleanup](../analysis/campaign-ai-coop-20261020/ai-object-decisions/component-proof-v1.json).
The owned scratch directory is empty; unrelated active builds are preserved.

The complete callback/context/numeric/selector/actor-state/query increment
retains six additional required original references totaling 12,114,625 bytes.
The largest is the 8,321,491-byte callback reference. Their paths, hashes,
source pins, 2,366-case passing normal/ASan/UBSan cohort, public CMake results,
scoped failures and passing 300-tick actual MCC encounter are recorded in the
[callback increment proof and cleanup](../analysis/campaign-ai-coop-20261020/ai-object-callbacks/component-proof-v1.json).
Every owned copied-source/build/client directory and receipt wrapper is removed;
the owned scratch parent is empty. No binary, raw memory bank, render capture
or promoted runtime is retained. Earlier references and unrelated active work
remain intact. Full mission, navigation/combat and real co-op remain open.

The complete query-group increment adds three required independent original
references totaling 24,447,076 bytes, with paths/sizes/hashes in the
[query proof and cleanup](../analysis/campaign-ai-coop-20261020/birth-query/component-proof-v1.json).
That receipt links the 6,194-case passing component and main CMake checks,
the 15-map metadata inventory, scoped authored-family runs and the remaining
Marine/Johnson owners. Every owned copied source/build/client and receipt
wrapper is removed; the owned scratch parent is empty. No binary, raw bank,
render capture or promoted runtime is retained. Required references, original
assets and unrelated active work remain intact.

The actor-scalar, BSP-cache and difficulty increment adds three required
independent original references totaling14,391,125 bytes, with exact paths,
sizes and hashes in the [current proof and cleanup](../analysis/campaign-ai-coop-20261020/ai-difficulty/component-proof-v1.json).
All11,749 combined comparisons pass normal, ASan/UBSan and main CMake. The
final immutable cohort has no snapshot or checkout drift. All16 recorded
owned source/build/client/dependency paths are removed and scratch is empty;
no test binary, raw bank, capture or promoted runtime is retained.

The camera-effect and fixed damage-operand increment adds three required
independent references totaling532,301 bytes. All11,953 combined native cases,
full camera-request MXCSR against unchanged i386 execution, and exact fixed
operand/access checks pass normal, ASan/UBSan and main CMake. Its18 recorded
owned paths are removed and scratch is empty. No test binary, original CPU
image, raw bank, capture or promoted runtime is retained. The
[current proof](../analysis/campaign-ai-coop-20261020/damage-operands/component-proof-v1.json)
records the three300-tick controls, remaining45-tick Marine boundary and later
concurrent whole-file drift with all tested bodies unchanged.

The indexed reaction increment adds one required1,611,598-byte original
reference. All12,399 combined cases pass normal, ASan/UBSan and main CMake,
with no snapshot/checkout drift. Original fixture and generated data repeat
exactly. The14 recorded owned paths are removed and scratch is empty; no
binary, raw bank, capture or promoted runtime remains. Its
[source pins and cleanup](../analysis/campaign-ai-coop-20261020/damage-indexed/component-proof-v1.json)
record the unchanged355 C55 provider pins, three300-tick controls and the next
Marine call274A40 ->274E30 at tick45.

The whole property-owner increment adds one required4,900,620-byte original
reference. All13,119 combined cases pass normal, ASan/UBSan and main CMake;
the final snapshot and checkout match. Original fixture, generated module
and unchanged immutable data repeat exactly. Its10 recorded owned paths
are removed and scratch is empty. No binary, raw bank, capture or promoted
runtime remains. The [current source pins and cleanup](../analysis/campaign-ai-coop-20261020/prop-owner/component-proof-v1.json)
record the unchanged355 provider pins, three300-tick controls and the next
Marine creation call274A4E ->274450 at tick45.

The property-creation increment retains two original references totaling
11,547,164 bytes, including the earlier declared-count diagnostic. Only the
corrected576-case reference executes in the13,695-case current target; all
normal, ASan/UBSan and main CMake checks pass. Both original references and
generated module/data/storage repeat exactly, with no final source drift.
All16 recorded owned paths are removed and scratch is empty. No binary, raw
bank, capture or promoted runtime remains. The [corrected source pins and cleanup](../analysis/campaign-ai-coop-20261020/prop-create/component-proof-v2.json)
record unchanged355 provider pins, three300-tick controls and the new51-tick
Marine boundary21942E ->505E88.

Initial source review found one source player, a fixed `players` list and a
hard-coded cooperative false. The first increment replaces the last two with
authenticated source-state queries; actual participant birth and views remain
single-player. Authored AI placement and original AI/object frame calls already
exist. The host `NavigationWorld` explicitly uses geometric costs. No full
client delivery or live campaign/co-op acceptance follows from these component
and caller-compilation results.
