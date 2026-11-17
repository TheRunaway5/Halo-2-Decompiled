# Native 1BBEE0 area perform (behavior row 109) — status and continuation

Updated 2026-11-13. Implements the previously missing 01a (The Armory) behavior-row
109 "perform" callback reached after the opening cutscene. The original
failure was:

```
TU1.5 live world owner missing at 1b0ad9 -> 1bbee0 return 1b0adb
```

Current source: 26C3C0 through 200D30 now binds native 2013B0 and
27A640/27AAA0. The route's 2CBEE0/2CE030/279100/1B0E10 dependencies remain
explicit required owners. The 2026-11-13 continuation below supersedes older
staged-owner and assignment notes in this document.

Scope background is in [NATIVE_CAMPAIGN_SUBSYSTEM_SCOPE.md](NATIVE_CAMPAIGN_SUBSYSTEM_SCOPE.md).
Later goal-evaluation owners remain incomplete; this is not a claim that
the full Armory campaign crash is fixed.

The production 26A380 caller now supplies the native 2013B0 path query,
including 1ED700 and 275E90. The standalone legacy prefix entry still has
an empty query callback for its explicitly retained prefix tests; it does
not describe the production 1BBEE0 binding.

## What is native now

| Owner | What | Binding |
|---|---|---|
| 1BBEE0 | Row 109 perform: inactive return, 200C00 remaining distance and COMISS, 26C510 + 1BBBD0 area choice, 1D2D90 lease, the 0x758-byte type-4 request on the real stack, NONE handling (clear +40, 1D2E30), 26C3C0 publication call, failure-count 26C510 | `1B0AD9 CALL EAX`, return `1B0ADB`, RET 8 |
| 26A2E0 root | 32A660 0xF000 probe, actor +7 early NONE, 26B220/26BA30/26A380 calls with their real frames | inside 1BBEE0 |
| 26B220 type-4 | Radius choice, +1C equality copy, 1D2CF0 aim origin (0BAD60 marker request, +30 copy; BBF20 refuses), 1EA1D0, 1ED530 facing vector, prop timing, 1ECD00/1ECC40 flags, +274..+278 clears, +754 | inside 26A2E0 |
| 26BA30 (complete) | Firing-position candidate collection: 26BBD0 squad/pairs/order/definition walk with order redirects, 26BE90 area gates and candidate marks, 26C0B0 admission (26C740 → 140DB0 owner hash with its 267A30/267A40 callbacks, 3EA20 x87 owner/self distances), 26C1E0 candidate init with the 218B20 projection, 2A78A0/202AB0 cluster list, the 52D87C dynamic positions and the held-squad gate | inside 26A2E0 |
| 26A380 entry | No candidates: clear `*flag_out`, `[5020A8]+3 = 1`, history reset, NONE. With candidates: original prologue and native 2013B0 through `context.path_query` | actual 26A2E0 caller; legacy prefix-only API remains separate |
| 26A380 remainder (26A712..26AF82) | Query goal fields (+46/+618 target or actor +358 perception), native 201500/27B010/27B2B0 with the 26A841 `*flag_out = 1`, the candidate loop (27A460 path cost/clearance/direction, or the +58/+5A straight-line branch with x87 roots), +5E8 retry count, 269E70 random pick (4F3CD8 LCG) or 269D70 scoring + 140910 sort + 45687C evaluators, `*candidate_out`, the 140DB0 `*owner_out` copy over its frame window, the +3F2 pin clear, and the +56 tail | after 2013B0 returns; `native_campaign_ai_goal_evaluation_tu15.cpp` |
| 26A380 scoring callees | 269D70 table (267A60 goal history / +51 target sweep with 1209C0 / +70 avoid points / +698 / +5A / +5B facing with native BBE00 / +692; 2689A0 cost falloff and +68C kinds), 26C5A0, 269870 and the 269DB0 walk, 140910/140BC0 with 269DF0, 269E70 | `native_campaign_ai_goal_scoring_tu15.cpp` |
| 1BC0A0 | Row 109 +48: +4AE/+484 flags and +4B4 scalar | `1B0F56 CALL ECX`, return `1B0F58` |
| 1ECD40 (+1ECC40/1ECD00 family) | Character tag property walk (+B4/+B8) | `270696 CALL`, return `27069B` |
| 1ED700 | Actor character's object-specific property lookup (+E4/+E8, B4-byte records, exact object-definition key +4), including parent fallback and actual call ABI | Native child of the bound 2013B0 query |
| 2013B0 / 275E90 | Retained 0x50-byte actor query, character/object radius, original action and behavior tests, cached or uncached framed position, and shared native 1D2CF0 aim origin | Actual 26A380 and 200D30 callers; `native_campaign_ai_actor_path_query_tu15.cpp` |
| 275BA0 / 275CB0 | Complete parent-chain navigation eligibility; point-owner parent lookup, biped cached-point publication, absent-scenario return, and explicit vehicle/scenario boundaries | Uncached 275E90; `native_campaign_ai_object_navigation_tu15.cpp` |
| E2300 / E04B0 / E22C0 | Biped navigation cache, dynamic-sector test, invalid-object reset, signed update-tick gate, optional aliased outputs and shared BBE00 world-point owner; collision refresh E20F0 and sector lookup 202660 remain required | Actual 275CB0 call at 275DC9 |
| 2186F0 prefix | World-space framed-point copy; actual saved words and boundary to moving-frame lookup 2185E0 | Actual 275CB0 call at 275DE9 |
| 27AAA0 / 27A640 | Parent-chain route reconstruction and materialization, including retained frame bytes and pending x87 stores | Actual 200D30 calls at 201226/2012F4; named child refusals remain |
| 27B2B0 path search (+27B340, 27BD30, 202970, 186E30) | Search reset (+AE, +110B0, the 0x2000-byte +120B6 hash, +90/+94/+98), native 27B100 start, the 27B340 expansion (heap pop/27BC00, goal and node-bound exits, 27BD30 neighbor rows, parent/exclusion/device/clearance gates, midpoint and 218EB0 goal slide, 218C40 x87 roots, 27C520 influence, kind penalties, CVTTSS2SI priority, hash probe, node publication, 27BB60 insert/decrease-key, best-goal tracking) and the +70 retry counter | Qualified native component in `native_campaign_ai_path_search_tu15.cpp`; bound at 26A380's `26A831 CALL` through the native query (see "27B2B0 path search" below) |
| 26C3C0 publication (+1FC540, 1FC9B0, 200D30, 200960, 200A10, E9600, 27B0D0) | Goal publication: 26C510 history, owner reset, 26C600 assignment, +3F0/+3F1, 26C850 mode 4/5/6 selection; 1FC9B0 resume/restart; 1FC540 reset with the 218D00 distance; 200D30 gates, E9600 kind, modes 2..7, the 2010C7 route gate, the lease path (27B0D0) and the own-search tail (201500, 1D2D90, 27B010, 27B0D0, 27B2B0, 1D2E30); 200A10 failure count | Native at 1BBEE0's `1BC054 CALL`, `native_campaign_ai_publication_tu15.cpp`; binds native `27A640` and `2013B0`; deeper route and other non-01a callees remain named refusals |

Files: `src/campaign/ai/native_campaign_ai_area_perform_tu15{,_binding}.cpp`,
`native_campaign_ai_candidate_collection_tu15.cpp` (26BA30),
`native_campaign_ai_goal_evaluation_tu15.cpp` (26A380),
`native_campaign_ai_goal_scoring_tu15.cpp` (26A380's scoring callees, own
replay module `cmake/CampaignNativeAiGoalEvaluation.cmake`),
`native_campaign_ai_character_property_tu15.cpp`,
`native_campaign_ai_path_search_tu15.cpp` (27B2B0, own module
`cmake/CampaignNativeAiPathSearch.cmake`), their headers in
`include/halo2/`, `cmake/CampaignNativeAiAreaPerform.cmake`, and three dispatch
lines in `PhysicsLiveWorldTu15::Impl::required_entry_original()` (unchanged by
the 26BA30/26A380 work, which lives inside the 1BBEE0 owner).

Named fatal refusals (`NativeCampaignAiPerformRequiredOwnerTu15`, site → entry):

- `27A7A2 → 2CBEE0`, `27A882 → 2CE030`, `27AD0E → 279100`,
  `27A8E6 → 1B0E10`; the native route preserves each actual CALL frame.
- Uncached biped point: `E240A → E20F0`, `E246C → 202660`,
  `218719 → 2185E0`. Vehicle/scenario points still stop at
  `275D8B → F35D0` and `275D66 → 276E10`.
- Other 26A380 request shapes: `26A6FB → 26A846` (request +56 set:
  alert or vision). `26A438 → 26A43E` (+618),
  `26A5B5 → 26A5B7` (+54) and `26A628 → 26A62E` (+55) cover request shapes
  1BBEE0 never builds (26B220 type 4 clears +618 and never sets +54/+55).
- `1D2D65 → BBF20` and 26B220 request types
  other than 4 (`26B2DE → 26B2EC`).
- Inside 26BA30, never a substituted result: a [56AEB0] owner table whose
  callbacks are not 267A30/267A40 (`140DB8`/`140DD9`), and original stack
  corruption the native cannot reproduce: a 26C740 value size above 4, a
  2A78A0 cluster below -160 or above 63 (its unbounded 64-bit frame mask
  would reach saved EBP or the return address), a zero bucket count (#DE),
  order redirects deeper than 4096.

Design notes for 26BA30:

- Every retained read and store keeps the original order and width. The
  callees' frames lie below 26A2E0's ESP and are dead on return; their locals
  are host values. The count is 26BA30's dword local: 26BE90 increments its
  low word, the dynamic loop the dword.
- 3EA20 runs as the original x87 sequence (raw opcodes, including FST to
  single, FCOMIP and the unordered-accepting JBE) inside
  `detail::NativeTu15ArithmeticEnvironment` with the caller's control word
  and status, so C1/precision flags and the 53-bit default precision are the
  original's. 218B20 uses the existing native projection under the caller's
  MXCSR.
- 2A78A0 indexes a two-dword "seen" mask with the signed cluster >> 5 and no
  bound. The native models the seven frame words that index can reach with a
  known meaning: saved EBX (the request) and EDI (26BE90's area record), the
  counter, geometry and area-record locals (a corrupted local changes the
  original's later loads, and the native follows it), and the mask.

## 2026-11-13 biped navigation continuation

The newer frozen runtime receipt at
`build/test-results/resume-mcc-native-ai-20261113/armory-preinstall.json`
already passed the old 2013B0 boundary and stopped at tick 25 at
`275DC9 -> E2300`. This increment therefore extends the biped navigation
owner reached by the real authored Armory startup.

The native E2300 owns cached/reset paths and exact prefixes to collision
refresh E20F0 and spatial lookup 202660. E04B0 and E22C0 are complete.
BCDF0 uses the shared salted-object lookup kernel with its lazy retained
reads and real saved/argument words. BBE00 reuses the existing numeric owner;
its new frame-aware wrapper also preserves the conditional saved EDI word
for attached bipeds. No child result or translated fallback is supplied.
The 275CB0 caller consumes the resulting cached position and either copies
a world-space point or refuses at 2185E0 when a moving frame is required.

The unchanged physical-i386 navigation reference has 558 cases: 348 complete,
47 named refusals and 163 fault prefixes. It reaches 128/139 E2300 sites,
19/19 E04B0 sites, 13/13 E22C0 sites, 107/122 275CB0 sites and the admitted
17/54 2186F0 sites. Missing sites lie beyond the named child boundaries.
The 186-case actor-query reference was regenerated through this caller.
Both fixtures reproduce byte for byte in independent original runs.
Navigation, actor-query, area-perform and character-object CTests pass
with ASan/UBSan. The unmutated control passes and ten independent semantic
mutants are rejected, including update-tick equality, dynamic-sector flags,
cache reset, coordinate publication and the attached-biped saved word.
Evidence is under
`build/test-results/resume-ai-biped-point-20261113/`.

The replay now distinguishes each actual PUSH/CALL word from explicit
stores in the nested frame. In particular, 275CB0's SP-9C is an implicit
vehicle argument but an explicit E2300 local on the biped path. Those words
still participate in complete retained-bank comparison; only implicit
stores are excluded from explicit-operand event counts.

Required regenerated references are
`tests/fixtures/native_campaign_ai_object_navigation_tu15.json.gz`
(108,433 bytes) and `tests/fixtures/native_campaign_ai_actor_path_query_tu15.json.gz`
(41,755 bytes). Prior references remain in the evidence directory.
All recorded scratch/build paths are removed; no generated executable,
object, raw bank or capture is retained. The cleanup receipt records the
removed duplicate generated reference copies and their hashes.
This is component and composed-call qualification. A new natural Armory
run, 900-tick acceptance and launcher delivery have not been established
for this increment.

## Earlier query and route qualification (2026-11-13)

The actual 26A380 and 200D30 callers now enter the native query with their
real stack frames. The uncached 275E90 path uses native 275BA0 and the
admitted 275CB0 branches. This earlier cohort stopped at E2300, F35D0 or
276E10; the biped continuation above advances E2300. The shared aim-origin
owner still refuses at BBF20 where required.
There is no substituted child result or new translated production fallback.

| Reference cohort | Evidence |
|---|---|
| Actor query and position | 186 cases; all 97/97 2013B0 and 59/59 275E90 instruction sites; byte-identical independent original repeat; 11 query/caller mutants rejected |
| Object navigation | 342 cases: 299 whole/refusal cases and 43 fault prefixes; 275BA0 38/38 sites, admitted 275CB0 85/122; independent original repeat and native replay passed |
| Area perform | 931 cases: 372 complete, 16 required-owner refusals, 543 fault prefixes; eleven former query stops now execute native 2013B0 to its first consumer read |
| Goal evaluation | 718 cases, including 24 complete actual 26A380 entries consuming the native query; independent original repeat passed |
| Route | 866 cases: 317 completed direct/component entries, 66 named refusals, 483 fault prefixes; 27AAA0 364/364 sites, 27A640 332/336; 38 meaningful mutants rejected |
| Publication | 1,266 cases: 256 complete, 65 named refusals, 945 fault prefixes; actual query and route callers included; independent original repeat byte-identical |

Query, area, character-object, object-navigation and goal CTests passed with
ASan/UBSan. Route sanitizer validation passed its 864-case cohort before two
additional fixture-only retry witnesses; the final 866-case Release replay
and those two mutation controls passed with unchanged production code.

Route component entries 27A7A7/27A887 use independently declared post-call
input frames. They do not qualify 2CBEE0 or 2CE030. All authored route frame
bytes are compared, including the full count DWORD at 27ABD2 and its FFFF
carry. Rejected FSTP/FISTP stores retain the original pending 80-bit value
and stack state. Implicit PUSH/CALL words are checked in final banks and
callee ESP/return-word witnesses, rather than explicit-operand event counts.
The four unexecuted 27A640 sites are three unreachable signed-quotient
instructions under the guarded positive retry divisor, and the instruction
after the named 1B0E10 boundary.

Extending publication exposed previously unconsumed test inputs: a zero
cyclic nearest-node fallback, poisoned actor policy fields, and a 5A5A frame
index in actor +4F8. Original-instruction observations identified each
consumption. The reference now declares valid empty-workspace/policy inputs
and a default world-space frame WORD, retaining the high half and explicit
frame overrides. Expected outputs were regenerated from the unchanged
original; the native production implementation was not changed to accept
those poisoned inputs.

Receipts are under `build/test-results/resume-ai-actor-path-query-20261113/`,
`build/test-results/resume-mcc-object-navigation-20261112/`,
`build/test-results/route-27a640-20261113/`, and
`build/test-results/resume-ai-route-20261112/`. The required compressed
original fixtures remain under `tests/fixtures/`. Generated observer banks,
mutation objects and completed component build trees were removed.

The fresh frozen engine build and natural 900-tick Armory probe are separate
delivery gates, recorded under
`build/test-results/resume-mcc-native-ai-20261113/`. Their result must be
checked before treating this source qualification as launcher delivery or
Armory acceptance.

## Earlier staged proof

Physical unchanged-i386 oracle `tools/oracle_native_campaign_ai_area_perform_tu15.py`
(authenticated English TU1.5, SHA-256 `21b207e7…`). It single-steps the
original closure over declared banks: 1BBEE0/1BC0A0/1ECD40, 200C00, 26C510,
1BBBD0, 1D2D90/1D2E30, 26A2E0, 26B220, 0BAD60, the whole 26BA30 closure and
26A380. Stop entries are the unported calls (2013B0, 26C3C0, BBF20) and the
26A380 branch PCs listed above; the original halts there, where the native
refuses. No instruction is patched and no callee outcome is supplied. Direct
26BA30 and 26A380 cases pass their extra stack words through the shared
observer's optional `stack_arguments` (backward compatible).

Fixture `tests/fixtures/native_campaign_ai_area_perform_tu15.json.gz`
(3.78 MB, SHA-256 `a2f579e1…`): 932 cases.

- 399 whole cases: 1BBEE0 208, 26BA30 162 (direct), 26A380 18 (direct),
  1ECD40 8, 1BC0A0 3. 372 complete, 27 named refusals (13 at 2013B0, 3 at
  26A846, 3 at 26A43E, 2 at 26A5B7, 3 at 26A62E, 3 at BBF20).
- 533 read/store/observer fault prefixes.
- 108,203 ordered reads, 214,773 ordered stores, 4,464 register checks;
  927,454 original instructions.
- 15 composed 1BBEE0 → 26A2E0 → 26BA30 cases with real candidates (zone
  firing positions, dynamic positions, alert, targeted, MXCSR variants).
  Count-0 1BBEE0 cases now complete through the NONE path natively.
- Coverage: all 946 sites of the 13 collection functions are reached
  (26BA30 127/127, 26BBD0 240/240, 26BE90 158/158, 26C0B0 98/98, 26C1E0
  64/64, 26C740 37/37, 140DB0 34/34, 267A30, 267A40, 218B20, 3EA20, 2A78A0
  123/123, 202AB0). 26A380: 59 of 769 sites (the prefix).
- Independent repeat (`--verify`) reproduces the fixture byte for byte. The
  replay is clean under ASan/UBSan.

Harness rule added for read-modify-write stores: 2A7A0F `INC WORD` faults as
a whole before its store, so the observer records neither half; the native
publication reads its operand first. On a before-write prefix, exactly one
trailing native read equal to the boundary operand (address and width) is
accepted. XMM is compared only on paths that avoid 26B220 (0BAD60 and 218B20
own unmodeled XMM scratch; XMM is caller-saved in the original ABI).

Mutation controls (compiled into the replay test, run against the fixture):

- 26BA30 closure: 59 mutants, 56 killed. The 3 survivors are equivalent:
  26BBD0's AL result is dead at both callers (2 mutants), and an inserted
  `FLD1; FSTP ST(0)` is a no-op. Boundary cases were added for the 3
  initially surviving real mutants: kind values where `&31` and `&15`
  differ, d_owner exactly 1.0, and 2·d_self exactly d_owner.
- 26A380 prefix: 14 mutants, 14 killed (16-bit count test, history value,
  count and stride, flag, `[5020A8]+3`, reread order, branch order, exit
  ECX/ESP).
- Earlier: 27 1BBEE0 kernel mutants plus 8 for 1ECD40, all rejected except
  one equivalent (ECX restore after the 32A660 probe).

## Resumed validation and delivery state (2026-11-12)

The interrupted Claude build did not establish a live result for the new
26BA30/26A380 source. Its retained build `halo2-test-73c86odf` failed to link
`native_campaign_ai_evaluate_goals_tu15`. The parallel launcher attempt's
shell notification reported exit 0, but its actual
`analysis/launcher-engine-build/fp-body-jitter-20261112/report.json` says
`passed: false` and `runtime_installed: false`: goal-evaluation source was
absent from that frozen cohort's runtime graph. The selected runtime was
not replaced by that attempt.

A fresh official CMake build of the current area-perform target passed its
932-case replay. The current `CampaignNativeAiAreaPerform.cmake` includes
goal evaluation in both the runtime and test targets; its source hash differs
from the failed launcher cohort. Rechecking the fresh retained engine graph
with `build_launcher_engine.wiring` found no unexpected unwired sources.
These checks establish current source registration and component behavior,
not an Armory live-run or launcher-delivery result. The 900-tick native
Armory goal remains open.

The next complete leaf, 1ED700, is now native and independently qualified.
`tools/oracle_native_campaign_ai_character_object_tu15.py` runs its unchanged
physical original with 32 whole cases and all 342 retained-read fault
prefixes: all 47 instruction sites, 7,518 original instructions. Cases cover
salted actor and character handles, child-before-parent precedence, signed
counts, exact 32-bit keys, first/middle/last/missing entries and parent
fallback. The replay checks all output registers, caller-preserved registers,
XMM/FP state, saved stack bytes and ordered reads. It failed before the
native owner existed and passes afterward. The new CTest and the inherited
area-perform CTest both pass with ASan/UBSan. Reports are under
`build/test-results/resume-ai-character-object-20261112/`.
An independent physical rerun reproduced the new fixture byte for byte;
all 13 compiled mutation controls were rejected. Mutants changed actor and
character masking, actor/record strides, character/key offsets, count sign,
key width, parent traversal, result identity, RET cleanup and EDX. Mutations
were confined to owned scratch copies; production source hashes stayed
unchanged during the screen.

The new permanent original fixture is
`tests/fixtures/native_campaign_ai_character_object_tu15.json.gz` (27,734
bytes, SHA-256 `f45dff4f9f03c896736eb9ba883d366265d04267bcad764b07a4824a52db8234`).
It supplements the existing 932-case fixture without replacing it. This
leaf is not dispatched into production independently: 2013B0 must supply
its real call inputs before the complete parent path can be admitted.

The resumed 2026-11-12 production comparison used the same MCC Armory map
with neutral input and a 900-tick request. The previously selected runtime
stopped at tick 10 on the missing 1BBEE0 owner. The newly built executable
(`c42f71be0ac585121cbc3e2e545809978c4e179c87d0a9a67e26026816c06013`)
reached tick 25, then refused `26A70D → 2013B0`. Both exited 1 without a
timeout; neither completed the mission or 900 ticks. Neither run opened
reference executables or maps outside the prepared MCC cache, and both
owned probe directories were removed. The bounded comparison and exact
commands are in `build/test-results/resume-armory-live-20261112/comparison.json`
and its two referenced run reports. This is progress to the next native
owner, not campaign acceptance or evidence of launcher promotion.

Subsequent package qualification passed all 145 CTests, 27 Original map
checks and 4 pStencil map checks. Both launchers now select that executable
in `bin/engine-builds/8148d6485b17478b95f92ab357e37d17/`. Publication and
independent selection/hash verification are recorded under
`analysis/launcher-engine-build/mcc-triggers-resume-20261112/`. These checks
qualify the delivered multiplayer package; the Armory refusal above remains.

### Live differentials (kept-tree build `halo2-test-73c86odf`, relinked)

After the CMake registration, the kept tree was rebuilt incrementally. Its
CTest `native_campaign_ai_area_perform_tu15` passes. Runs: 01a,
`--simulate 900 --campaign`, `SDL_VIDEODRIVER=dummy`,
`HALO2_NATIVE_AI_PERFORM_TRACE=1`.

- **Production:** the same result as above. 15 inactive calls; the first
  active call collects 3 candidates through native 26BA30; native 26A380
  refuses at `26A70D → 2013B0`; tick 25.
- **`differential-26ba30`:** all 35 active 26BA30 calls in 900 ticks match
  their translated body (0xF000-byte candidate array, request, count, FP
  controls, ESP, preserved registers). The run reaches tick 900.
- **`differential-26a380`:** all 35 active calls in 01a have candidates, so
  all 35 take the prefix refusal and keep the translated result.
  The no-candidate path never occurs live in 01a and is proven by the oracle
  only. The run reaches tick 900.
- **`translated`:** native 26BA30 results feed the translated 26A380/26C3C0.
  Tick 900, 35 active calls, no script faults.

## 27B2B0 path search (native component, 2026-11-12)

`src/campaign/ai/native_campaign_ai_path_search_tu15.cpp` ports the whole
27B2B0 closure that has no earlier native owner: 27B2B0 (40 sites), 27B340
(504), 27BD30 (471), 202970 (41) and 186E30 (13). It composes the existing
qualified natives for everything else: 27B100 query start (with 218C40,
32CC0C and the 27BCE0 insert), the 27BB60/27BC00 heap, 218B20/218780
projection and frame resolution, 218EB0 framed difference, 27C520 influence
(value form) and 278E60/FF1A0 jump links. Nothing is transported; no
translated or original code runs. 218C40 is reproduced in the new file
because the existing copy is private to query start: equal frames subtract
locally, otherwise both points go through native 218B20. Its FSQRT result
stays extended for the following `FST m32 / FADD m32 / FSTP m32`, under the
caller's x87 control word.

The private candidates in `analysis/campaign-native-ai-actions-20261030/`
(`path-search-candidate`, `whole-search-candidate`) were reviewed and not
promoted. They depend on a web of other private candidates and authored
adapters. They have zero original comparisons and deliberately leave the
padding and stack-history questions open. The port is fresh,
instruction-ordered code checked against the physical original.

### Entry shape (for 26A380 at 26A831)

```cpp
#include "halo2/native_campaign_ai_path_search_tu15.hpp"
bool native_campaign_ai_path_search_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace, std::uint32_t stack, NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe = {});
```

- `workspace` is 26A380's seventh argument (`call.workspace`, `[esp+8CC]`
  at 26A81A). It is EAX at 26A831 because 27B010 never writes EAX.
- `stack` is ESP at 27B2B0's first instruction: 26A380's entry ESP − 0x8B4.
  At 26A831, 26A380 has `SUB ESP,8A0` plus four pushes, so ESP is entry −
  0x8B0, and the CALL pushes 26A836. `NativeCampaignAiGoalEvaluationTu15`
  carries no stack pointer today; the caller must supply it from the
  `_call_` ABI (`abi.gp[4]`).
- `fp` is the caller's live x87 CW/SW and MXCSR, threaded through. Masked
  exceptions are required (`std::invalid_argument` otherwise), and the x87
  stack must be empty (the call ABI refuses a live stack).
- Return value: the original AL. Then 26A836..26A844 does
  `if(found) *flag_out = 1` (store `26A841`: byte 1 through the pointer
  loaded from `[esp+8D0]`).
- Like the original, the search uses its real frame below `stack`. The
  candidate point is at `stack-0xC4C` (16 bytes; its frame word is a WORD
  store read back as a DWORD), the goal distance at `stack-0xC3C`, the
  218EB0 difference at `stack-0xC20` and the 64 neighbor rows (0x30 bytes
  each) at `stack-0xC14`. 27BD30's resolved jump points are at
  `stack-0xCA4` and `stack-0xC98`. All accesses lie in
  `[stack-0xCD0, stack)`, below 26A380's frame. The other 27B340 locals are
  host values.
- `native_campaign_ai_path_search_call_tu15(memory, abi, observe)` is the
  same entry with the original call ABI. EAX holds the workspace and
  `[ESP]` the return address. On exit, AL is the result (EAX
  zero-extended); EBX/EBP/ESI/EDI are preserved and ESP += 4. ECX, EDX and
  XMM are caller-saved scratch and are left as they were. 26A836 consumes
  only AL. ECX is rewritten at 26A861 and 26AB7C before use; EDX is not read
  in the 26A870 loop head.
- 27B340 and 27BD30 are also exported (`native_campaign_ai_path_expand_tu15`,
  `native_campaign_ai_path_neighbors_tu15` and their `_call_` ABIs) for
  direct qualification.
- Test-target sources for a composed caller: the new file plus
  `native_campaign_ai_{path_query_start,path_heap,point_geometry,
  frame_geometry,path_influence,jump_arc,jump_fallback,vector_math}_tu15.cpp`.
  The runtime already links all of them.

### Named refusal

`NativeCampaignAiPathSearchOverrunTu15(site 27C32D, row)`: 27BD30's ring
stage would write neighbor row 64. Its linked stage stops at 64 rows, but
its ring stage checks only `== 64` after each row. Row 64 overwrites
27B340's return address, its argument and 27B2B0's saved registers, so the
original crashes afterward. The native publishes rows 0..63 and every
earlier store, then refuses before row 64's first store. Direct 27BD30 calls
have no 64-row cap; the original writes rows 64+ into whatever buffer it is
given.

### Stack history (residual, documented)

27B340 reads two kinds of bytes the original never initializes:

- Row +A/+B: 27BD30 stores the row kind as a WORD, and 27B340 copies the
  DWORD to node +10.
- The candidate frame's upper word: node +24 is a DWORD copy.

The native reads both from the real guest stack at the original addresses,
which is exact when the guest stack holds the original's history. In
production, history left by natively owned predecessors is not the
original's, because native owners keep their locals in host storage. One
case is inside this closure: the original 27B100 → 218C40 → 218B20 →
218780/219030 and 32CC0C frames reach about `stack-0x80`, which covers
row +A/+B of rows 62–63. Native 27B100 leaves the older history there.
Only node +12/+13 and node +26/+27 can differ, and only when such a row is
published. The oracle therefore declares the 27B2B0 row-history bank only
up to `stack-0x100`. Direct 27B340 cases declare all 64 rows with nonzero
history.

### Proof

Oracle `tools/oracle_native_campaign_ai_path_search_tu15.py` drives the
physical unchanged i386 (English TU1.5 `21b207e7…`) through the shared
area-perform machinery. It declares banks for the workspace (node table,
heap and hash), a synthetic navigation world (sectors with closed
boundary-edge rings, edges, vertices, links), frames/objects/bodies,
device tables, the jump scenario and gravity, and the actual `.rdata`
constants and permutation tables. It single-steps all 26 closure functions
with no stop entries. Cases enter at 27B2B0 (172 whole searches), at 27B340
(28, with pre-populated node/heap/hash state: the 0x400 limits, probe
wrap at 0xFFF, found better/worse/closed/special nodes, node-bound exits,
the exact 0x7FFF priority gate) and at 27BD30 (15, with a declared 66-row
array).

Coverage spans corridors and grids with and without goals, unreachable
goals and every threshold outcome, 27B100 refusals with retry-counter
wrap, exclusions, devices (gated/open/indexed/list order), influence,
penalties, the +45 bound, clearance and the goal slide, framed points
(valid/missing object, out-of-range node), kinds 0–7 with masks and
policies, jumps (free/arc/masked/exhausted/only-kind-5/rise limit, gravity
and speed sweep), unflagged targets, sector flags, a NaN vertex, six
x87/MXCSR profiles (precision, rounding, FTZ/DAZ, preset sticky flags,
TOP 7), and stack-history banks.

Fixture `tests/fixtures/native_campaign_ai_path_search_tu15.json.gz`
(2,269,730 bytes, SHA-256 `fe67828e6137b46f8516b039a28d66f782786a023c5a8a3cf5b50ac99cab6d6f`;
27B2B0's REP STOSD stores as counted runs):

- 1,045 cases: 215 whole (172 / 28 / 15) and 830 fault prefixes (one
  before-read per read site, one before-write and one after-write per store
  site, each from the cheapest case reaching it); 1,771,742 original
  instructions.
- Owned coverage: all 1,069 sites (27B2B0 40/40, 27B340 504/504, 27BD30
  471/471, 202970 41/41, 186E30 13/13). Whole closure 2,340/2,480. The
  composed natives are reached only as far as this caller drives them; for
  example 278E60 never gets secondary endpoints from 27BD30.
- Replay (`tests/native_campaign_ai_path_search_tu15_tests.cpp`, CTest
  `native_campaign_ai_path_search_tu15`) compares every bank, ordered read
  and ordered store with its PC, AL/AX, EBX/ESP/EBP/ESI/EDI and the x87
  CW/SW and MXCSR: 139,536 reads, 1,064,394 stores, 3,010 register checks.
  One 64-row control replays the original 27BD30 state that writes rows
  64–65. With a 64-row cap the native must refuse at row 64 after exactly
  the original's stores for rows 0..63.
- An independent physical rerun reproduced the fixture byte for byte
  (`build/test-results/path-search-27b2b0-20261112/repeat.json`; the
  original report is `original.json` there).
- The official `tools/run_tests.py --target
  native_campaign_ai_path_search_tu15_tests --filter
  '^native_campaign_ai_path_search_tu15$' --jobs 4` CTest passes in Release
  (`green/`) and with `--sanitizers` (ASan/UBSan, `sanitized/`). The replay
  prints: "1045 original 27B2B0/27B340/27BD30 cases (215 complete: 172
  27B2B0, 28 27B340, 15 27BD30; 830 fault prefixes; 1 64-row refusal
  controls); 139536 reads; 1064394 stores; 3010 register checks passed". The
  production source also compiles cleanly with GCC and Clang `-Wall -Wextra`
  under the runtime's FP flags. This is component qualification only. There
  is no 01a live run yet, because 26A380 still refuses at 2013B0 first.
- Mutation screen (`mutations.py` and `mutations.json` in the same
  folder): 71 compiled mutants, each an out-of-tree copy of the production
  source linked with the unchanged replay. The unmutated control copy
  passes, and all 71 mutants are rejected. They cover:
  - 27B2B0: reset count and value, retry bounds, the expansion frame offset.
  - 27B340: clamp floor, pop sift threshold, goal-bound terms; the parent,
    blocked, exclusion, device and clearance gates; midpoint and slide math
    (threshold, clamps, dot axis); an SSE replacement of the extended
    accumulation; influence keep/one; penalty kinds; goal-distance store;
    the 0x7FFF gate and truncation; the +45 bound operand; hash slot, wrap
    and special match; found priority and closed node; node and heap
    limits; DWORD-versus-WORD copies of the candidate frame and kind word;
    flag bits 12 and 3C0; depth steps; the decrease-key slot; goal
    tracking; the exit comparison and its default.
  - 27BD30: link limit; portal side and flag; jump admission sign,
    proximity, free bit, kind range, direction, flags and special kind;
    kind-2 mode and special; kind-5 policy and flags; ring root, next side,
    limit, portal flag and edge vector.
  - 202970 kind and gate; both 186E30 offsets.
  - Two value-equivalent read-order mutants.

  Eight survivors of a trial screen were all test gaps, and each was closed
  with new cases before this run: exact 0x7FFF, a threshold equal to best,
  bit-12 targets, a nonzero +98, a bound with penalty, a jump admitted only
  at kind 5, and jump/kind-5 targets without flag bit 0. The production
  source SHA-256
  `13c9327ffd64d52ec0e2bb9b6f067b6551bfef0fbb7c65664b8c8c6c77db1698` was
  verified unchanged before and after.

## 26A380 remainder (native component, 2026-11-12)

`src/campaign/ai/native_campaign_ai_goal_evaluation_tu15.cpp` now runs
26A380 from 26A712 (after 2013B0 returns) to its RET 20, and
`native_campaign_ai_goal_scoring_tu15.cpp` ports its unported callees:
269D70 with 267A60 and 2689A0, 26C5A0, 1209C0, 269870 and the 269DB0 walk,
140910/140BC0 with 269DF0, and 269E70. It composes the existing native
201500 (navigation filter), 27B010 (workspace), 27B2B0 (path search), 27A460
(path result) and BBE00 (object world point); 140DB0 is the same walk as
26BA30's private copy. Shared primitives (retained port, one SSE instruction
per call under the caller's MXCSR, x87 sequences under the caller's control
word) are in the private header `src/campaign/ai/native_campaign_ai_goal_port_tu15.hpp`.

### Executed slice (01a)

The 01a census (`HALO2_REFERENCE_AI_PERFORM_CALLEES=translated`, 900
ticks, selected runtime `c42f71be…`, `HALO2_INSTRUCTION_CENSUS`) slices
26A380 to 60 functions and 4,168 executed instructions; the indirect
tables add 267A60, 2689A0 (45681C) and 269870 (45687C) for request type 4.
Everything outside 2013B0's own subtree (275E90, 275CB0, 0E2300, 16C000 and
their leaves) is native now. The census and slice are retained in
`build/test-results/goal-evaluation-26a380-20261112/`. On 01a: request
+46/+51/+56/+58/+5A and +618 are clear, +15 is set and a candidate is always
within reach (sorted path), candidate_out is 0, owner_out is set, and
269870 never needs 2A7D60. So once 2013B0 is wired, 26A380 completes
natively for every 01a call shape observed; the next 1BBEE0 refusal is then
26C3C0's (step 4).

### Entry and frame

`native_campaign_ai_evaluate_goals_tu15(memory, call, context, observe)`
with `NativeCampaignAiGoalEvaluationContextTu15{stack, fp, occupied_x87,
path_query}`. 26A2E0 passes its real ESP, FP and x87 depth; `path_query` is
empty, so production still refuses at `26A70D → 2013B0`. The prefix-only
overload (no context) is unchanged in behavior. The private locals
(goal, chosen index, row, offset, best score, near/far) are host values. The
query (ESP+44), the 201500 output (ESP+94) and the sort indices (ESP+B0)
are real frame bytes, because their consumers read memory; CALL argument
and return words are written where the original pushes them.
`native_campaign_ai_evaluate_goals_resume_call_tu15` is the original
continuation ABI at 26A712 (ESP = entry − 0x8B0, EBP = request; locals and
saved registers read from the frame), used for isolated original
comparison.

Details kept from the original:

- The 26AE59 owner copy is `[56AEB0]+28` bytes into `[esp+10]`; values up
  to 0x18 bytes overwrite the goal, index, row and chosen-candidate locals
  exactly as the original (a 9-byte case is in the fixture). Longer values
  are a named refusal.
- 26ABFC clears `*flag_out` on the random path; 26AB35 clears +4C of every
  candidate not within reach, before 269E70 re-admits one.
- 140910's comparator reads `[52D878]` on every comparison; the sort's 30
  partition slots are the original's (overflow refuses; unreachable).
- x87: 26AA9B/26AAA4 roots, 267B8B/267BC8/267C41/267E42 and the 267FD3
  projection run as raw opcodes (register forms as bytes), with memory
  operands read beforehand in original order.

### Proof

Oracle `tools/oracle_native_campaign_ai_goal_evaluation_tu15.py` single-steps
the physical unchanged i386 (English TU1.5 `21b207e7…`) through the shared
area-perform machinery and the path-search world builders. 26A712 cases
install the real 26A380 frame as stack words (saved registers, prologue
locals, the query 2013B0 leaves, return and arguments) and run the original
26A712..26AF82 with real 201500/27B010/27B2B0 over a declared grid
navigation world, 27A460, the scoring callees and 140DB0. Direct cases enter
26C5A0, 1209C0, 269D70, 267A60, 2689A0, 269870, 269DB0, 140910 and 269E70.
Stop entries: 2013B0, 2694C0, 2A7D60, 120AF0 and the other request types'
45681C/45687C callbacks. No instruction is patched; no outcome supplied.

Fixture `tests/fixtures/native_campaign_ai_goal_evaluation_tu15.json.gz`
(2,537,519 bytes, SHA-256
`e21f2443ed60e37c559f25fe973d4fdd8af38f0e7ec31a434fe084739233ba10`):

- 675 cases: 262 whole (99 at 26A712, 40 267A60, 21 2689A0, 45 269870,
  14 1209C0, 11 140910, 10 269D70, 10 269DB0, 8 269E70, 4 26C5A0), 45 of
  them stopping at a named-refusal entry (29 2A7D60, 3 2694C0, 2 120AF0,
  11 other-type callbacks), and 413 fault prefixes (one per owned read site
  and before/after each owned store site, from the cheapest case);
  1,339,938 original instructions, 166,666 reads, 39,052 stores.
- Coverage: 26A380 remainder 496/540 sites (the 44 others are the 26A846
  label and the 26AEBC tail, which only follows 2A7D60), 267A60 425/439
  (after 120AF0), 269870 55/67 (after 2A7D60); 269D70, 26C5A0, 2689A0,
  269DB0, 140910, 140BC0, 269DF0, 269E70 and 1209C0 complete.
- Cases cover sorted and random paths (LCG states, admitted and rejected
  picks, invalid clusters), unreachable sectors (the +5E8 count with node
  counts 4..12, including exactly 8), the +358 perception and +46/+618
  query fields, straight-line distances with 3D directions, exact
  reach/radius boundaries, twin candidates (equal totals and equal scores),
  owner hits/misses/self and value sizes 0/2/7/9, the +3F2 pin, six
  x87/MXCSR profiles and live x87 depth; for the callees, the +51 sweep,
  +70 avoid points (radius ≤ 0), +698/+5A/+5B/+692, kinds with bit 15 or
  bit 7, request types 0..8, 0x14, 0x24, 0x30, and sorts of 0..64 indices.
- Replay `tests/native_campaign_ai_goal_evaluation_tu15_tests.cpp` (CTest
  `native_campaign_ai_goal_evaluation_tu15`) compares every bank, ordered
  read and store with its PC, the goal/AL/XMM0 result, EBX/ESP/EBP/ESI/EDI
  and x87 CW/SW and MXCSR; at a stop it requires the same named refusal.
  It prints: "675 original 26A380 remainder cases (217 complete: 81 26A712,
  38 267A60, 11 140910, 87 other callees; 45 required-owner refusals; 413
  fault prefixes); 155966 reads; 493486 stores; 3472 register checks
  passed". ECX/EDX/XMM at exit are caller-saved scratch (26A2E0 and
  1BBEE0 never read them) and are not compared.
- Harness rule (as for RMW): a before-write fault at a MOVS store
  (26ADFD, 26AF30, 26AF6D, 26AF6F) faults as a whole; exactly one
  trailing native source read of the same width is accepted.
- An independent physical rerun reproduced the fixture byte for byte
  (`build/test-results/goal-evaluation-26a380-20261112/repeat.json`; the
  original report is `original.json`).
- Mutation screen (`mutations.py`/`mutations.json` there): 84 compiled
  mutants, each an out-of-tree copy of one production source linked with
  the unchanged replay; both unmutated controls pass; 82 rejected. The two
  survivors are equivalent: `-0.0 − t` for `0.0 − t` at 26A9FE feeds only
  squares, and dropping 269DB0's early exit cannot matter because 269870 is
  the last 45687C entry and every other entry refuses for type 4. Ten
  first-screen survivors were test gaps and were closed with cases (exact
  boundaries, distinct y/z deltas, node count 8, twins, kind bits, type
  bit 4, radius ≤ 0). Production source SHA-256s were unchanged before
  and after (`1ff15c3b…` goal evaluation, `ce41e99a…` scoring).
- Official `tools/run_tests.py --target native_campaign_ai_goal_evaluation_tu15_tests
  --target native_campaign_ai_area_perform_tu15_tests --filter
  '^native_campaign_ai_(goal_evaluation|area_perform|character_object)_tu15$'
  --jobs 4` passes 3/3 in Release (`green/`) and with `--sanitizers`
  (ASan/UBSan, `sanitized/`). The area-perform and character-object
  fixtures are unchanged and still pass with the extended goal evaluation.
  GCC and Clang `-Wall -Wextra` are clean for the new sources.
- Runtime registration: `native_campaign_ai_goal_scoring_tu15.cpp` is in
  `halo2_physics_world_runtime` (`CampaignNativeAiAreaPerform.cmake`, which
  also lists the goal-evaluation closure for the 1BBEE0 test); the replay
  is `cmake/CampaignNativeAiGoalEvaluation.cmake`. The launcher wiring
  audit (`build_launcher_engine.wiring`) on a fresh configure graph
  (`wiring/evidence/engine-targets.txt`) passes with no unexpected unwired
  sources, and the new source is in the engine closure. halo2_native was
  not relinked: production behavior is unchanged until 2013B0 is wired.

### Residuals (documented)

- Stack history: 201500 leaves byte +0xD of its 7-word output unwritten,
  and 27B010 copies the whole dword into workspace +140C4. The native reads
  the real frame byte; in production it holds whatever native owners left
  there, not the original's history. Only workspace +140C5 can differ.
  The 27B2B0 row-history note above applies unchanged.
- The area-perform fixture is unchanged: its 26A380 cases still refuse at
  26A70D (no path query) and 26A6FB.

## 26C3C0 publication and movement (native, 2026-11-12)

Status note (2026-11-13): this section records the 2026-11-12 increment.
Since then, 2013B0 and 27A640/27AAA0 have become native, and the publication
fixture has been regenerated at 1,266 cases (see the 2026-11-13 sections
above). The `2013B0`/`27A640` refusals, the 1,105-case fixture figures and
the mutation screen below describe that increment's source
(`084d47ad…`). They do not describe the current file.

Live differential (2026-11-12 source, kept-tree build linked 2026-11-12
22:16, since removed): on 01a, `HALO2_REFERENCE_AI_PERFORM_CALLEES=translated`,
900 ticks. Native 26C3C0 ran all 54 publications, with 2013B0/27A640
transported in that reference mode. The run reached tick 900 with 560
perform calls. Its `HALO2_NATIVE_AI_PERFORM_TRACE` lines (actor, +418 goal,
+504 path state, AL, active flag) were byte-identical to the selected
runtime's translated-26C3C0 runs, which are identical to each other run to
run. Production on that snapshot still stopped at `26A70D → 2013B0`, tick 25.
Receipt: `build/test-results/publication-26c3c0-20261112/live.json`.

`src/campaign/ai/native_campaign_ai_publication_tu15.cpp` (header
`include/halo2/native_campaign_ai_publication_tu15.hpp`, module
`cmake/CampaignNativeAiPublication.cmake`) ports 26C3C0 and the movement
owners it reaches: 1FC540 (action reset), 1FC9B0 (begin in mode 4/5/6),
200D30 (movement and path update), 200960, 200A10, E9600 and 27B0D0. It
composes the existing natives for 26C510, 26C600, 26C850/277D40, 16E940,
1ECC40/1ECD00, 218D00 (`native_campaign_ai_point_distance_squared_tu15`),
201500, 1D2D90/1D2E30, 27B010 and 27B2B0. 1BBEE0's `1BC054` call now runs
it natively (`native_campaign_ai_area_perform_tu15.cpp`; the 26C3C0
transport is gone). Production behavior on 01a is unchanged for now,
because 26A380 still refuses at `26A70D → 2013B0` before 1BBEE0 reaches
1BC054.

01a census (selected runtime, `HALO2_REFERENCE_AI_PERFORM_CALLEES=translated`,
900 ticks): 54 calls to 26C3C0, all with a goal whose definition selects
mode 4. 1FC9B0 resumes 47 times and restarts 7 times. 200D30 takes path mode
0 every time: 26 calls with the 1BBEE0 lease (27B0D0, then 27A640) and 28
calls without one (2013B0, 201500, 1D2D90, 27B010, 27B0D0, 27B2B0, then
27A640 on 25 found searches; 3 searches fail and copy +70 → +5B4). The
executed slice under 26C3C0 is 121 functions and 9,839 instructions. Without
2013B0's subtree and the existing natives, 66 functions and 6,224
instructions remain. 50 of those functions (5,578 instructions) lie under
27A640. 1FC540, 2A8440 and 279FB0 never execute on 01a.

Named refusals (`NativeCampaignAiPerformRequiredOwnerTu15`, site → entry),
each after every preceding source read and store:

- `201226 → 27A640` and `2012F4 → 27A640`: route smoothing, the next owner
  on 01a.
- `201245 → 2013B0`: the path query, owned by the other session. The code
  after it (20124A..20133D) is native and qualified as a component; see below.
- `200D95 → 1EBBD0` (+504 == 2 resume, then 1FB4F0/200A40),
  `201022 → 201840` (mode 6 test), `201058 → 275FE0` (mode 7, then 2673A0),
  `201127 → 201750` (path mode 2), `201178 → BCFB0` and `2011A5 → 1D2CF0`
  (path mode 1, before 2A8440), and `20128C → 27A2D0`.

The 1D2CF0 refusal is conservative. 1D2CF0 already has a native, but it is
private to `native_campaign_ai_area_perform_tu15.cpp`, and 2A8440 comes
right after it. Export it when 2A8440 is ported.

`services.callee` (reference modes only) transports these callees with
their original registers and stack. The continuation code after each one is
native. Natives keep their locals on the real guest stack at the original
addresses wherever a callee or a later read consumes them: 200D30's F+12
result byte, F+14 path mode or lease, F+24 filter, F+40 query; 1FC540's
framed point at entry−10.

Integration notes:

- The area-perform oracle still lists 0x26C3C0 as a stop entry. No case
  reaches it today, because 26A380 refuses first. Once 26A380 completes with
  candidates, composed 1BBEE0 cases will reach 1BC054, and the native now
  runs 26C3C0 instead of refusing there. Replace that stop entry with this
  closure's `ENTRIES` and `STOP_ENTRIES` from
  `tools/oracle_native_campaign_ai_publication_tu15.py`, and declare its
  scene banks.
- `cmake/CampaignNativeAiPublication.cmake` adds this closure's sources to
  the area-perform test target. It builds the publication replay from that
  target's deduplicated `SOURCES`, so new 26A380 dependencies reach both
  tests.

Stack history (residual, documented): 1FC540 writes only the low word of its
local frame dword, then copies the whole dword into +4C4. The native reads
it from the real stack, which is exact when the stack holds the original's
history. Natively owned predecessors keep their locals in host storage, so
+4C6/+4C7 can differ in production. This happens only when 1FC540 restarts
the action of a flying actor in mode 4 with +5D0 set. The direct 1FC540
cases declare that stack with nonzero history and compare it.

### Proof

Physical unchanged-i386 oracle `tools/oracle_native_campaign_ai_publication_tu15.py`
(English TU1.5 `21b207e7…`). It reuses the area-perform and path-search
machinery. The scene declares the actor pool (three actors), the 26C600 hash
table and free list, scenario zone and dynamic goal definitions, the
path-search world (objects/frames, and a 3×1 corridor for the mid-entry
search), character tags, the 1D2D90 descriptors and full-size workspaces.
It single-steps 26C3C0, 1FC540, 1FC9B0, 200D30, 200960, 200A10, E9600,
27B0D0, every composed native's original closure, 201500, 1D2D90/1D2E30,
27B010 and the 26-function 27B2B0 closure. The unported callees above are
the stop entries. No instruction is patched and no callee outcome is
supplied.

Mid-entry component cases resume 200D30 from a declared post-call state:

- 20124A, after 2013B0: the native 201500/1D2D90/27B010/27B0D0/27B2B0
  composition over a real search. It covers found (stops at 27A640),
  unreachable, a +4E4 restriction, +358/+35E (27A2D0) and both lease
  descriptors.
- 2012DF: AL from 27B2B0.
- 201342: the route owner's result byte.

ESP is the frame; the saved registers, locals, return word and arguments are
declared stack words. This qualifies the 01a own-search path ahead of 2013B0
and 27A640. It is not a production claim.

Fixture `tests/fixtures/native_campaign_ai_publication_tu15.json.gz`
(3,540,095 bytes, SHA-256
`c5275d045b3fe6d9225f1aab434c7f571a9b87adcf095fcea0751f680cb18170`):

- 1,105 cases: 321 whole and 784 fault prefixes.
  - Whole cases: 148 at 26C3C0, 60 at 1FC9B0, 46 at 200D30, 40 at 1FC540,
    12 at 20124A, 9 at 2012DF and 6 at 201342.
  - Of those, 197 complete and 124 stop at named refusals: 52 at 27A640,
    48 at 1D2CF0, 14 at 2013B0, 4 at 275FE0, 3 at 201840, 2 at 27A2D0 and
    1 at 1EBBD0.
- 884,242 original instructions.
- The replay checks 47,605 ordered reads and 619,677 ordered stores, 197,674
  of them with their PCs, plus 2,758 register checks.
- Coverage:
  - Fully reached: 26C3C0 120/120, 1FC540 124/124, 1FC9B0 69/69,
    200960 15/15, 200A10 9/9, E9600 9/9, 27B0D0 16/16.
  - 200D30 reaches 320/426. The 106 unreached sites are the tails behind
    the unported callees listed above, plus the dead `default` arm
    200F91..200F9D (ECX is 0, 1 or 2 there).
- An independent repeat (`--verify`) reproduces the fixture byte for byte.

The replay is `tests/native_campaign_ai_publication_tu15_tests.cpp`, CTest
`native_campaign_ai_publication_tu15`. It compares every bank, every ordered
read and store, the refusal entry, EAX (26C3C0) or AL, EBX/ESP/EBP/ESI/EDI
and the x87 CW/SW and MXCSR, including at refusals. Rules specific to this
replay:

- Native 26C600 reports no store PCs. Its stores must still match in order,
  address and bytes, and come from the 26C600/140D50..140E80 closure.
- The observer checks a MOVS destination before its source. At a
  before-write boundary on a MOVS site (the fixture's `movs_sites`, computed
  from the authenticated metadata), exactly one trailing source read is
  accepted.
- An after-write prefix stops inside its boundary store before the PC event.

Mutation screen (`build/test-results/publication-26c3c0-20261112/mutations.py`,
results in `mutations.json`): 65 compiled out-of-tree mutants of the
production source, each linked with the unchanged replay. The unmutated
control passes and 64 are rejected. The one survivor, `update-path-range`
(the `> 2` path-mode bound), is equivalent: F+14 only ever holds 0, 1 or 2.
An earlier screen also left `begin-count-sign` alive. That was a test gap,
closed by same-goal cases at a firing position with a real sector. The
source SHA-256
`084d47adf0617669db3d877c9fd5e122a9992ad4526c49bc233b661ebb7db781` was
unchanged before and after.

The private `movement-reset-candidate` (200960) is superseded by this
physically compared owner and was not promoted. The route candidates
(2762A0, 2029E0, 276090) belong to the 27A640 subtree and have not been
compared against the original. `route-edge-iterator-candidate` (2029E0, 70
sites) was inspected and is compact. Before it can be compared it needs
store PCs and the shared observer memory, in place of its `CursorMemory`
increment hook, plus a direct-entry oracle family.

## Reference and differential modes

`HALO2_REFERENCE_AI_PERFORM_CALLEES` (oracle only; production never runs
translated code):

- `translated`: 26A380 runs its translated body (26BA30 is native). 26C3C0
  is native, and only its unported callees (27A640, 2013B0, …) run
  translated. Use it for censuses.
- `translated-with-26b220`, `translated-with-26ba30`: also route that native
  owner to its translated body.
- `differential-26b220`, `differential-26ba30`: run the native owner,
  restore its outputs (26BA30: the 0xF000-byte candidate array and the
  request), run the translated body, require identical bytes, EAX, FP
  controls and preserved registers.
- `differential-26a380`: run the native prefix; when it completes (no
  candidates) require identical `*flag_out`, `[5020A8]+3`, history bytes and
  EAX against the translated body; when it refuses, count it and keep the
  translated result.

`HALO2_NATIVE_AI_PERFORM_TRACE=1` prints one line per perform call with the
differential counters. `HALO2_TICK_OWNER_DIGEST` (13B6F0) is not
deterministic run to run on 01a, so it cannot serve as a differential here.

## Continuation plan (next owners, in order)

1. **2013B0 path query (another session).** The only remaining 26A380
   refusal on 01a is `26A70D → 2013B0`. Everything after it is native
   (step 3). To wire it, set `NativeCampaignAiGoalEvaluationContextTu15::
   path_query` in the 26A2E0 `evaluate` lambda in
   `native_campaign_ai_area_perform_tu15.cpp` (one line; the context is
   already built there with the real stack, FP and x87 depth). The hook
   receives `(memory, actor, query, stack, fp, observe)`: `query` is
   26A380's entry ESP − 0x86C, `stack` is ESP at 2013B0's first
   instruction (entry − 0x8B8; `[stack]` = 26A712 and `[stack+4]` = actor
   are already in the real stack). It must leave the 0x50-byte query in
   retained memory, because 26A712 adds its fields and 27B010 copies it
   into the workspace from memory.
   Preserve 2013B0's partially initialized local: 2014A2 writes only the
   low word at its `[esp+18]`, then 2014A7/2014AF copy the complete dword
   into query +20. Its upper word comes from actual stack history. Do not
   replace it with a host-zeroed value. 275E90 also reaches 275CB0/275BA0
   on uncached paths; those need actual native owners or named refusals.
   Its 1D2CF0 call is already native, but private to
   `native_campaign_ai_area_perform_tu15.cpp` as `object_aim_origin`, with
   its BBF20 refusal. Export it rather than re-port it.
   After wiring, extend the area-perform oracle: drop 2013B0 from its
   `STOP_ENTRIES`, add 2013B0's closure and the entries/stops of
   `tools/oracle_native_campaign_ai_goal_evaluation_tu15.py`, and declare
   the navigation world/workspace banks its `world_env` builds.
2. **27B2B0 path search: native and qualified (2026-11-12).** The native
   26A380 remainder calls it at 26A831 (`native_campaign_ai_path_search_tu15(
   memory, workspace, entry_esp − 0x8B4, fp, observe)`, then the 26A841
   store); production reaches that call once step 1 is wired.
3. **26A380 remainder: native and qualified (2026-11-12).** See "26A380
   remainder" above. Still refused (none is reached on 01a):
   - `26A6FB → 26A846`, request +56 (alert/vision). Porting it is the same
     code (the candidate loop's straight-line branch, the evaluators and the
     26AEBC tail are already native); it only needs the prefix branch to
     continue, and the area-perform fixture's three 26A846 cases then need
     regeneration. Coordinate with whoever edits that oracle next.
   - `2698FF → 2A7D60`, 269870's line-of-sight test (request +56, or +5A
     with candidate +58, or +58 with candidate +59). Its static closure is
     127 functions / 8,198 sites: 16C620 collision ray (camera-query parent
     needs AI flags qualified), 1F8200, 1E6980, 24DC90, 2DE70E MOPP and the
     30AA70 components. This is the next heavy owner for missions whose
     characters set behavior bit 0x20.
   - `267D53 → 120AF0` (267A60's moving-target sweep, request +51);
     `26AD7E/269EE2 → 2694C0` (+618, never built by 1BBEE0); the 45681C and
     45687C entries of other request types; `26AEC8 → 2013B0` (only after
     2A7D60).
4. **26C3C0 publication: native and qualified down to 200D30 (2026-11-12).**
   See "26C3C0 publication and movement" above. Update 2026-11-13: 2013B0
   and 27A640/27AAA0 are now native too (see "Earlier query and route
   qualification"), so the first two bullets below are done. On 01a, the
   route's next required owners are **2CBEE0** and **2CE030**, together
   with 279100/1B0E10 off the 01a path. The 01a executed slice under 27A640
   was 50 functions and 5,578 instructions, in bottom-up order:
   - 27A640 itself (336 sites; 0x5AC frame passed by pointer to its callees;
     the tail runs 218C40 + FSTP) and **27AAA0** path reconstruction
     (364 sites; parent walk, SSE, 30BD0 native; 279100 and 218780 arms
     unexecuted on 01a).
   - The **2CBEE0** subtree: 2CBEE0, 2CC5C0, 2CC790, 2CC8F0 (2B3E0,
     32CCD8), 2CCEB0, 2761F0, 2769C0, 2768F0, 2CBAF0, and the route
     segment 2762A0 → 2029E0, 276090, 27A310. Evaluate the private
     `route-segment-traversal`, `route-edge-iterator` and
     `route-segment-contact` candidates against the original first.
   - The **2CE030** subtree: 2CE030, 2CABE0 (919 sites), 2CBE30, 2CBCF0,
     2CDEB0, 2CD780, 2CDBC0, 2CD870, 2CD500, 2CD3D0, 2CD440, 2CDB40,
     2CBC10, 2CAAC0, 2CBA80, 2CAB80, 267520, 1EB730, BD080 (BA970, BAA40),
     BBD10 and 2B3E0.

   Several leaves already have natives in the object-factory/motion
   headers: 14CEA0, 14D0E0, 123210 and 243450 (placement), 144BF0, BC190,
   BAC00, C1E90 and 32CCD8. Reuse them.

   To extend the proof, drop 27A640 from the publication oracle's
   `STOP_ENTRIES` and make the new frontier entries stop entries. The
   `after_query_found_*` mid-entry cases then drive a real 27B2B0 result
   into 27A640. A scratch probe confirmed this: with 27A640 and 27AAA0
   running, and stops at 2CBEE0, 2CE030, 279100 and 1B0E10, the two corridor
   cases reach 27A640 83/336, 27AAA0 159/364 and 27C410 17/17, then stop at
   `27A7A2 → 2CBEE0`. The rest of 27AAA0 needs richer worlds: link kinds
   1, 2, 5 and 6, framed points (218780), and paths longer than the
   10-point cap. 1FC540, 2A8440 (→ 279FB0 → 112090) and the other
   non-01a branches stay named refusals until another map reaches them.
5. Then the 01a 900-tick production run and the mission sweep.

Workflow per owner: disassemble, port into its own file under
`src/campaign/ai/`, extend the oracle (add closure entries, move stop entries,
add declared data), run `tools/run_test.py -- … oracle … --output <scratch>`,
replay with the native test, run mutants, then rebuild and rerun 01a in
production and reference modes. A quick standalone replay build (no CMake):
compile `tests/native_campaign_ai_area_perform_tu15_tests.cpp` with the
sources listed in the CMake test target, `-std=c++20 -O2 -fno-fast-math
-ffp-contract=off -frounding-math -Iinclude -Isrc`, and pass it the
gunzipped fixture.
