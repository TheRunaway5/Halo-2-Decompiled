# Full game through the launcher: OG Halo 2 / TU1.5

Requested goal: launch the complete native game, including menus, every map and
mission, and all sandbox behavior, matching original Xbox Halo 2 English TU1.5
one to one. This is the maintained checklist for
[the existing parity goal](SANDBOX_TU15_PARITY_GOAL.md), not a replacement goal.

> **Current user instruction — MCC runtime assets only (2026-10-19).** Every
> runtime/menu/map test and launcher delivery must use the port-owned imported /
> converted MCC cache. **Xbox ISO may be used only for offline validation of
> converted MCC tags.** Decompiled TU1.5 code remains the behavior authority;
> Xbox/XBE/Vista banks are never onboarding or runtime dependencies. This rule
> authorizes no other ISO use. OG Halo 2/TU1.5 behavior remains the
> one-to-one target; this correction changes runtime asset authority, not scope.
> All earlier Xbox-cache startup/smoke/visual results below are historical
> reference/component evidence only and cannot qualify MCC runtime delivery.

The active asset/frontend/gameplay integration dependency is the canonical
[MCC gameplay checklist](/home/eric/Developer/Halo-2-Decompiled/docs/MCC_GAMEPLAY_GOAL_CHECKLIST.md).
Reuse its checked milestones and qualified artifacts; do not duplicate or reset
them here. Update verified MCC milestones there before dependent work, as its
working rule requires. Preserve the accepted cache/import, shaders, mods and saves;
do not repeat the full import or broad generated-engine builds without need.

Current status: **active, incomplete; 0/102 whole sandbox elements accepted**.
The goal service reports `active` on the 2026-10-19 automatic continuation. The
earlier `usageLimited` checkpoint remains historical; no full-game gate is
closed by that service-state change.
The tool interface cannot edit an unfinished goal's objective, so this checklist
is attached through the maintained goal document and continuation record.

## Current work and order — MCC runtime

1. Continue the canonical MCC checklist's current frontend priority: use the
   imported cache for title/menu graphics, text/fonts, audio and navigation;
   follow its actual current caller/integration and selected-runtime gates using
   existing code, shaders and compatible inputs. Bounded private frontend /
   unmuted/input candidate passes and selected published capabilities are separate;
   consult that checklist instead of repeating or copying its changing milestones.
   Make normal Play use the qualified MCC
   frontend, with the accepted native gameplay test kept separately available.
2. Prove runtime provenance for each test: pin the selected MCC import/conversion
   manifest, menu/map/dependency paths and hashes, actual runtime/launcher, and
   opened-file evidence. Reject Xbox/Vista reference banks and an original-XBE
   runtime requirement. Bundled intro/credits retain the user's explicit movie
   exception recorded by the MCC checklist; other assets follow MCC authority.
3. Enumerate menus, campaign and MP maps from the actual imported MCC inventory.
   Validate converted tags against original Xbox data offline; then test normal
   menu/map/gameplay/lifecycle paths with MCC data. The old 23-Xbox-map inventory
   is reference evidence, not the new runtime input selection or completeness
   proof. Preserve all 102 sandbox IDs and their original six acceptance gates.
4. Recover source/hardware owners only where the reused MCC gameplay path has a
   demonstrated dependency. Historical MCPX `38E8D3 / FECTL=0x88` and GP/EP work
   remain bounded offline evidence; do not make that startup reconstruction an
   automatic prerequisite for the working MCC path. Preserve original gameplay,
   timing, precision, effects/audio and output acceptance through real owners.
5. Deliver only an actually qualified MCC-only launcher/runtime/cache combination.
   The Xbox-asset r10 build was canceled through its outer storage owner, exit
   143; scratch `halo2-test-q5h6cwi5` was verified removed, candidate absent, and
   no r10 asset/source-backed tests, stage or installation completed. Its v528
   post-install updater is retired and refuses execution. No replacement MCC
   qualification or delivery result is claimed by this documentation update.

## Superseded Xbox-asset work plan — historical reference/component evidence

> **SUPERSEDED:** the following former priority/r10 plan is retained only to
> explain existing evidence. Do not execute it as the current runtime/delivery
> plan. The MCC-only instruction and active dependency above govern new work.

1. **G08/G10, active:** admit Tombstone/highplains2's next actual original shape
   dispatch owner, TU1.5 `2F75F0`, and its bounded reached closure. r9 now gets
   through `1F7330` and GPU initialization, then fails this missing target. The
   diagnostic carries retail site `2EF6BB`; the actual TU1.5 dispatch is
   `2F317B`, return `2F317D`, with retained shape vtable `461488`. The collision
   source port is now complete and frozen; preserve original virtual/shared
   owners, fields and ABI. Do not replace unknown callbacks with empty success.
   The source audit now follows actual type20/type23 nested dispatch to `3040D0`,
   both published agent lifecycles and the actual type20 helpers.
   The corrected independent census is **23 missing bodies / 1,586 sites**,
   superseding the earlier ten-body/292-site stage. Bounded original CPU
   comparison now passes 35 cases covering all 23 entries and 541/1,586 sites:
   1,131 observations (1,129 executed instructions plus two pre-RDTSC stops),
   2,163 byte writes, 12 returns, 21 external and two timestamp boundaries
   without supplied results. Full production build and ordinary live tests
   remain pending. Port manifest `tombstone-collision-dispatch-port-v527.json`,
   SHA-256 `01b0b1d7f4d23dbbcd5327e4ffa103181abf0f72d506e5e02d1347b6ed0bbde6`;
   CPU report `native-collision-dispatch-cpu-report-v526.json`, SHA-256
   `c112a8f896b65b3c70d0f38843386da24ac5ce38a7232907b79b793c731b89b5`.
   A disposable **injected native-module diagnostic only** completes six frames /
   15 ticks (v527 report SHA-256
   `988f24f80ddc0ad22cbc5b099c6205612d1115123aa294590bb0223badffaba6`).
   This does not establish untouched-r9 or fresh/uninjected-r10 acceptance.
   Evidence: `tombstone-child-convex-welder-lifecycle-v525.json`, SHA-256
   `94a30251e62acc7ab5647ec6843da6ec4431e9fa0ec0b2d8e3548a9dcc8b2a51`.
   Corrected wrapper audit: `tombstone-transform-wrapper-independent-v524.json`,
   SHA-256 `9f16128ed2c4e4854a1700d49e9cb26286c03f5a55061aeb6bc95a02328d1de9`;
   its earlier revision had reversed registry columns and is superseded. Derive
   entry ownership from the corrected evidence, never guessed column meaning.
   Read-only native-bank metadata:
   `tombstone-r9-native-bank-v524.json`, SHA-256
   `51cd741aec12250e987c4bfe1b9480d178bb14dc232ed4d47782990c36bb7e4a`.
   It is native metadata, not original runtime execution or a supplied result.
2. `1F7330` is **resolved only as a bounded component**: its whole 224-instruction
   owner and live dispatch compile in the production library and match seven
   independent original CPU contexts. r9 reaches the next owner, but Tombstone
   does not complete its requested six frames. v519 passes seven selected CTests
   in 1.95 seconds; v520 passes 30 admission/cache cases, then fails case 31.
   Immutable report: `xbox-menu-smoke-v520-first-attempt.json`, SHA-256
   `32ea30f68ffe4372116069b9038ec2e1ed0d7958195ef745234f98a84cea5142`.
3. Fresh **r10 qualification is planned**: build v524, native smoke v525 and
   installation v526; none has run at this checkpoint. r8/r9 remain unselected,
   no candidate installation is claimed, and installed r6 remains preserved.
   Planned v525 executes **six actual startup frames on all 23 owned MP maps**,
   not cache inspection alone: **24 map/player cases**, including Lockout with
   four local players. Order: Tombstone, Lockout one/four, Zanzibar, Ivory Tower
   (`cyclotron`), then the remaining canonical inventory. Desolation is `derelict`.
   Require each real native summary and
   executed inode hash; six startup frames do not establish full map lifecycle,
   gameplay, rendered parity or a playthrough. v525 also plans one native frame
   per case (24 total) and a contact sheet for visual review, all in disposable
   scratch; no frame retention is requested. These cases/captures have not run.
   Frontend checks may proceed independently of the actual physics closure;
   record new outcomes only after they run. No post-install updater was executed.
4. **G02/G11, queued:** implement current-map Xbox MP pause screen **195 / C3h**
   in its own gameplay-cache namespace: authenticated TU1.5 factory
   `14A09B -> 14A0D9 -> 239104`, input `2390CA`, dispatch `2392B1`. v517 is static
   correspondence only, not a port, original CPU execution or live lifecycle.
   Inspect actual screen/list/channel/session owners and request consumers;
   require world disposal and fresh shell creation before return-to-menu.
   Solo pause remains separate: `238B15 -> 238AC2`, global `51DF54`, dispatch
   `238A67`, and actual `1285A0` resource/audio effects. Continue G05/G06's real
   menu-to-map-to-gameplay composition without forced readiness or parallel
   substitute gameplay. Actual Xbox screen 13 lifecycle remains open.
5. Resolve G03/G04's original audio admission and processing dependencies, then
   complete every campaign/MP map, sandbox, multiplayer and presentation gate.
   The audio frontier below is historical evidence, not a new failure this turn.

> End of the superseded Xbox-asset priority/r10 plan. No passing item in that
> plan establishes MCC runtime, menu, map or delivery acceptance.

## Whole-game checklist

Every item remains unchecked until its stated acceptance is met. Partial work
belongs in the evidence ledger below. Paths in backticks are relative to the
workspace root, including when reading the private source copy of this document.

| ID | Required result | Current evidence, source owner and next acceptance |
| --- | --- | --- |
| G01 | [ ] Launcher starts the coherent tested runtime | Qualify the actual MCC-only selected runtime, launcher and imported-cache manifest together; record hashes and opened files with no Xbox/Vista/XBE runtime dependency. Existing MCC checklist owns accepted artifacts and pending launcher integration. Xbox r6/r8/r9/r10 evidence does not pass this gate. |
| G02 | [ ] Original frontend and menus through MCC assets | Follow the canonical MCC checklist's actual current frontend caller/integration and selected-runtime gates; reuse checked title/navigation/font/movie work and bounded private unmuted/input results. A private candidate pass does not publish or accept the selected frontend. Complete profile/settings/campaign/MP/pause/confirmation and real game-to-menu lifecycle with MCC data and OG/TU1.5 logic. Old Xbox sweeps/static chains are offline component evidence only. |
| G03 | [ ] Ordinary MCC map startup and audio admission | Admit normal converted MCC maps with real native audio ownership and lifecycles; reject original-reference banks as runtime prerequisites. Historical `38E8D3 / FE801100=0x88` remains unresolved offline MCPX evidence. Reconstruct that hardware/startup path only for a demonstrated dependency of the reused MCC gameplay path, never by guessed ready/device returns. |
| G04 | [ ] Complete gameplay audio and PCM through MCC resources | Use MCC-owned sound/music/codec/effect inputs and real voice/mixer/event/resource lifetimes, preserving original behavior and required PCM comparisons. Prior GP/EP preparation, command `38846E`, IRQ/codec research remain bounded reference evidence; hardware reconstruction is conditional on an actual MCC-path dependency. No opaque original DSP execution or invented ACK. |
| G05 | [ ] Complete MCC map/BSP loading and transitions | Reuse qualified MCC Cache/conversion/loader work from the canonical dependency. Complete converted dependency/postprocess, BSP activation, unload/reload and required async ownership in normal frontend-selected MCC maps. Original `191DF0`/`191E2E` findings remain offline source evidence, not permission to load original runtime banks or force readiness. |
| G06 | [ ] Original full game frame, players and objects | Compose the reused MCC gameplay path with correct source update order, players/objects/scripts, input, contacts, death/respawn and retirement. Preserve the working compatible systems; recover additional owners only for demonstrated gaps. Component source/CPU evidence alone does not establish complete live gameplay. |
| G07 | [ ] Every authored campaign mission through MCC | Maintain the complete mission/BSP inventory from the imported MCC cache and converted metadata. Include encounters/AI, scripts/objectives, cinematics, difficulty, checkpoints/save/load, death/restart and progression. Offline original tag comparisons support conversion; accept complete normal mission progression through MCC data. |
| G08 | [ ] Every multiplayer map and interaction through MCC | Enumerate every imported MCC MP map and validate its converted tags against original references offline. Require real menu/select/spawn/play/return and authored interaction coverage using MCC inputs. Old 23-Xbox-map plans, collision component probes and injected bridges cannot qualify MCC map gameplay, lifecycle or presentation. |
| G09 | [ ] All 63 weapon definitions | Preserve every existing weapon census ID, including AI, vehicle/turret, objective and unused authored variants. Complete input/ownership, triggers/barrels, both hands, reload/charge/special behavior, events and applicable presentation. Accept all six gates for each definition in the canonical matrix. |
| G10 | [ ] All 39 shared sandbox requirements | Preserve all powers/vehicles/projectiles/damage/world/AI/objectives/presentation IDs and six acceptance gates through converted MCC data and real runtime ownership. Existing collision/physics/audio CPU evidence retains its bounded logic scope; prove actual MCC composition separately. No shared element is fully accepted. |
| G11 | [ ] Local play, split screen and LAN | Complete normal MCC-backed matches, source identities/events/lifecycles, independent views/hands, join, loss/duplication/reordering, death/reset and recreation. Original MP195/solo/screen13 investigations remain offline evidence; qualify the actual converted MCC UI/session lifecycle and real gameplay. |
| G12 | [ ] Original animation, rendering, HUD and audio presentation | Render and play only MCC-owned converted resources, validating tag semantics against Xbox references offline. Preserve both species/hands, poses/IK/root motion, materials/lighting/shadows/weather/effects, UI/HUD/scopes and sound lifetimes. Require original-behavior/output acceptance; old Xbox-cache native sweeps are component/reference evidence, not MCC presentation delivery. |
| G13 | [ ] MCC-only tested launcher build delivered for user testing | Deliver a qualified coherent MCC launcher/runtime/cache combination, pin provenance/hashes/opened-file evidence and record actual tested limits. Preserve checked MCC delivery work, mods/saves/shaders and imported caches. Xbox-asset r10 was canceled before tests/stage/install; v528 updater is retired. No old Xbox runtime result passes this gate. |
| G14 | [ ] Full requested game accepted | All applicable G01-G13 gates, all missions/maps and all six acceptance gates for every one of the 102 sandbox elements must pass. No component milestone, rendered frame or successfully opened menu certifies full parity. |

## Acceptance authority and preserved inventory

**Runtime asset authority: imported/converted MCC cache only.** Xbox ISO may be
used only for offline validation of converted MCC tags; no other ISO use is
authorized. Decompiled TU1.5 code remains the behavior authority. Original Xbox,
XBE or Vista banks are never runtime/onboarding requirements. Preserve the
decompiled original TU1.5 behavior target and every original acceptance gate.

Original Xbox English TU1.5 XBE SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
This hash pins the decompiled original behavior authority; it authorizes no
runtime XBE requirement or additional ISO use. ISO access stays limited to
offline validation of converted MCC tags. Guessed tuning/service returns do not
establish OG parity. Unknown source owners and unexecuted branches remain explicit.

The canonical inventory remains
`analysis/sandbox-tu15-goal-20261013/coverage.json`: **63 weapons plus 39 shared
requirements; all 102 incomplete**. Retain every existing ID and evidence entry.
Its `latest_checkpoint` field still names historical `accepted-checkpoint-v14.json`;
do not treat that pointer as the latest source state or erase its older receipts.
Its Xbox authored-occurrence paths are reference locations, not runtime inputs.
Map each converted MCC definition to the applicable original contracts without
copying cache-local identities or inventing missing semantics.

Each element retains all six gates: authenticated reference; independently
compared original/native logic; real live composition; local/split/LAN publication;
matched original presentation; coherent tested launcher delivery. The broader
menus/maps/missions requirements above remain in addition to that sandbox matrix.

## Reuse ledger: bounded checkpoints already established

Checked entries below describe only their named milestones, not whole-game gates.

Current bounded MCC admission/source milestones, recorded before shortcut install:

- [x] **v530 synthetic shortcut admission/routing:** 13 reviewed tests pass for
  `tools/mcc_sandbox_launcher.py`, SHA-256
  `18e71ab5d5e3713db0e1761ee440a4334b4daf2c9694ed70bde2bf7719f4dcda`;
  test SHA-256 `70f2e51f6f3dfd644362d0c0876c3ac12911884b25873d6d9580d25d19ec4b8c`.
  The old regular ELF lacks explicit native-test support. Its `--launch-game`
  compatibility route is admitted only with native capability, no frontend and
  Play selecting exact Lockout; supported newer runtimes use explicit native
  selection. These are synthetic policy/routing controls, not installed-wrapper
  or actual launcher-through-gameplay acceptance.
- [x] **v530 bounded MCC runtime asset-source check:** selected native runtime
  exits 0 after six hidden muted Lockout frames / eight ticks, one player and
  one connected player, without `--gameplay-xbe`. Successful-open tracing records
  1,696 paths, no forbidden Xbox/XBE/ISO/Vista paths, no unknown assets and zero
  unmatched traces; owners remain unchanged and scratch is removed. Evidence:
  `work/sandbox-primary-v15/selected-mcc-runtime-file-opens-v530.json`.
  Report SHA-256 `edc1cc7ec4533950070fbef5036aba26b65dee1ffaf51cb862a21cd0fc0243cb`.
  This is not shortcut-through-runtime, visible/unmuted/audio, all-map,
  selected-frontend, original presentation or full parity acceptance. Actual
  shortcut-through-runtime execution is not established by that direct probe.
- [x] **v530 bounded MCC shortcut delivery:** actual `Halo2-Sandbox-Test` now uses
  the MCC helper, GUI label `MCC sandbox test  ›`. Installed `--check` passes for
  cache v13/native capability true/frontend false; seven installed CLI guard /
  info cases and all 18 existing real Tk launcher tests pass. Unsupported
  Xbox/XBE/Vista/map/runtime/installation overrides reject; `--menu` refuses the
  unqualified frontend. Eight protected files remain exact, test scratch root
  empty, no new large artifact. Receipt
  `work/sandbox-primary-v15/mcc-sandbox-shortcut-delivery-v530.json`, SHA-256
  `2725075b6d56d4930b7c7f7ca40f15c95ff7126fed1d3fb54ec84c148f9c9736`
  (5,270 bytes). This checks only shortcut delivery/admission. Direct native
  six-frame source evidence stays separate; installed-shortcut-through-runtime,
  visible/audio, menus, all-map gameplay and original/full parity stay open.

The canonical MCC checklist contains the same uniquely identified prerequisite
milestones; its other checked integration work remains authoritative there.

> **Historical reference/component ledger only.** Every Xbox-cache native startup,
> menu sweep, map/player smoke, injected bridge and candidate plan below predates
> the user's MCC-only runtime correction. None certifies MCC cache/menu/map or
> launcher delivery. Preserve valid offline logic/conversion-reference evidence,
> but do not execute superseded Xbox-runtime plans. Current MCC integration and
> checked milestones are owned by the linked canonical MCC checklist above.

Historical r9 bounded progress and its next actual failure:

- [x] **v519 `1F7330` production-library/original comparison:** all seven selected
  CTests pass in 1.95 seconds. The whole 224-instruction source body matches seven
  original contexts: 768 executed instructions, 1,248 byte stores, four natural
  returns and three stops before unsupplied external results. This closes that
  component seam; it does not establish a completed world or full gameplay.
- [x] **v520 bounded admission/cache cases:** 30 successful cases retained. The
  real Tombstone process uses the qualified r9 inode, passes the former `1F7330`
  boundary and initializes the GPU, then fails the next actual shape target.
  No six-frame completion, later menu sweep or X11/input survival is claimed.
- [ ] **r9 complete smoke/delivery:** attempted case 31 fails target `2F75F0`,
  actual TU call `2F317B`, return `2F317D`; diagnostic retains retail site
  `2EF6BB`, shape vtable `461488`. Runtime SHA-256
  `e8920d27cd38863a0c7b3137eec6877a22065da29e952efd08594267388f486f`.
  Immutable `work/sandbox-primary-v15/xbox-menu-smoke-v520-first-attempt.json`
  SHA-256 `32ea30f68ffe4372116069b9038ec2e1ed0d7958195ef745234f98a84cea5142`.
  Original closure audit: `tombstone-callee-closure-v524.json` and its retained
  listings. This is a new real missing owner, not a harness timeout or permission
  to supply an empty virtual return. r9 is not installed.
- [x] **G08/G10 bounded collision source/CPU checkpoint:** port complete/frozen;
  35 original cases cover all 23 entries, 541/1,586 sites, 1,131 observations
  (1,129 executed instructions and two pre-RDTSC stops), 2,163 byte writes,
  12 natural returns, 21 external plus two timestamp unsupplied boundaries.
  Manifest v527 SHA-256 `01b0b1d7f4d23dbbcd5327e4ffa103181abf0f72d506e5e02d1347b6ed0bbde6`;
  v526 CPU report SHA-256 `c112a8f896b65b3c70d0f38843386da24ac5ce38a7232907b79b793c731b89b5`.
- [x] **v527 diagnostic bridge only:** injected native module in a disposable
  diagnostic completes six frames / 15 ticks, report SHA-256
  `988f24f80ddc0ad22cbc5b099c6205612d1115123aa294590bb0223badffaba6`.
  It proves neither untouched-r9 behavior nor a fresh/uninjected-r10 pass;
  original live/full-game acceptance and installed runtime are unchanged.
- [x] **Superseded Xbox-asset r10 plan canceled:** outer `tools/run_test.py`
  owner stopped the build with exit 143; `halo2-test-q5h6cwi5` scratch was verified
  removed, r10 candidate absent, and no asset/source-backed CTests, native smoke,
  stage or installation completed. The old 23-map/24-case/frame/contact-sheet
  plan is retired. v528 post-install helper is explicitly disabled. Cancellation
  cleanup is verified; no MCC runtime or full-goal acceptance follows.
- [x] **v525 corrected static closure census:** independent actual type20/type23
  nested `3040D0`, both published lifecycle closures and helper ownership audited
  in `tombstone-child-convex-welder-lifecycle-v525.json` (SHA-256 above). Corrected
  v524 wrapper audit (SHA-256 above) supersedes its reversed-registry-column
  revision. This static/source-generation checkpoint claims no original CPU,
  production build, live success or complete entry-logic coverage.
- [x] **v524 bounded native metadata:** read-only stopped-r9 metadata identifies
  actual shape objects/table inputs; `tombstone-r9-native-bank-v524.json`, SHA-256
  `51cd741aec12250e987c4bfe1b9480d178bb14dc232ed4d47782990c36bb7e4a`.
  This is not original execution, a native port, successful callback or gameplay
  acceptance. Reuse it with authenticated source to resolve actual dispatch.

Historical r8 evidence remains valid only within its recorded scope. The full
candidate failed native startup and is **not installed or accepted**:

- [x] **v515 bounded frontend validation:** all six selected CTests pass in
  1.26 seconds; the separate authored check renders all 84 Xbox definitions and
  150 panes, with retired navigation rejected. Complete before/after source
  fingerprints and actual native guard matched for that r8 build. Evidence:
  `work/sandbox-primary-v15/xbox-menu-validation-v515/`.
- [x] **v516 retained successful cases:** 39 of 40 attempted native smoke cases
  pass, including actual Xbox menu launch, import rejection, owned cache
  inspections, limited launcher map/local-player cases and **279 GPU capture
  rows**. GPU execution is not original visual parity. The failure interrupts
  qualification before the later real X11 idle/combat cases; do not claim them.
  Evidence: `work/sandbox-primary-v15/xbox-menu-smoke-v516.json`.
- [ ] **r8 full native qualification and delivery:** failed at
  `Actual-launcher-highplains2-1` (Tombstone). Exact same-runtime reproduction:
  `tombstone-r8-reproduction-v518.json`; native diagnostic: missing original live
  callee `1F7330`, return `1F7D4F`. r8 SHA-256
  `bb255d2739b35c890272a688ec0a5b8a25efc6fbd7d94cd538d8f9897ea6c0c5`.
  No installation receipt exists; r6 remains selected.
- [x] **G08/G10 bounded `1F7330` correction:** whole source owner and live dispatch
  compile and pass the v519 production-library/original contexts above. r9 now
  reaches the next original shape target. Full Tombstone admission and candidate
  installation remain unverified; the old component failure is not repeated.
- [x] **v511 owned-input inventory and source launcher checks:** 23 candidate MP
  maps authenticated against original catalogs/manifests, with nine passing
  source-only launcher UI checks. Reuse `owned-multiplayer-launcher-maps-v511.json`,
  `owned-map-catalog-v511.json` and `sandbox-launcher-titles-v511.log`; none
  establishes an installed package or complete native map gameplay.
- [x] **v517 static MP/solo correspondence:** 23 authenticated retail/TU1.5
  body correspondences identify the MP screen 195 chain above; solo screen 18
  remains distinct. Reuse `xbox-pause-next-implementation-v517.md` and
  `xbox-pause-source-correspondence-v517.json`; no port/CPU/live result is claimed.
  Earlier v512's 15-body solo audit and v510/v513/r7 diagnostics remain historical.

- [x] **v508 EP CPU and resource preparation:** eight whole routines/242 static
  sites; real three EP append pages plus the fourth separate descriptor;
  GP/EP shared-page aliasing and ownership/bounds/high-word/count guards.
  Seven focused checks passed in 3.67 seconds; 26 independent original EP CPU
  cases. All 5,090 static sites across ten audio CPU modules authenticate to
  original code. Source only; no full factory/DSP/audio/delivery claim.
- [x] **v504 GP CPU preparation/upload and resources:** real GP pool allocation,
  growth/retirement, source decoding/relocation and all 15 effect-data comparisons.
  Historical coherent regression: 111/114 checks passed; only the three campaign
  FECTL failures above remained. The focused v508 run did not rerun these 114.
- [x] **v490/v477 bounded kernel-string/section and download/pool owners:** reuse
  authenticated whole CPU owners, their original comparisons and resource guards.
  These do not complete the original factory or native processor installation.
- [x] **Earlier narrow source/resource/gameplay checkpoints:** preserve existing
  APU reset/control/DMA, numerical DSP/output, map-loader/callback, source weapon
  input/manual reload and lunge evidence under their original limits. Consult
  the linked goal record before repeating their investigation or implementation.

Current integration authority and status are the linked canonical MCC checklist.
Its existing native Lockout test is the validated default gameplay test; private
frontend/unmuted/input candidate results do not automatically accept the selected
runtime or publish menus. Reuse those bounded MCC results and separately verify
actual launcher/cache/runtime selection. No checked MCC milestone is duplicated here.

All earlier v519/v524 Xbox candidate source fingerprints remain historical
component evidence. The r10 build is canceled, with no candidate/test/stage/install
result. No prepared post-install updater ran; r10 v528 is retired and refuses
execution. New MCC-only tests require their actual imported-cache/conversion
provenance and runtime/opened-file receipts, without Xbox/Vista/XBE dependencies.

Earlier authoritative component receipt:
`work/sandbox-primary-v15/native-EP-source-progress-v508.json` (389 source and
13 then-installed fingerprints). Earlier receipts:
`native-gp-scratch-progress-v504.json`, `native-dsp-lookup-progress-v490.json`,
`native-sound-download-progress-v477.json` in the same directory. v508's current
test executables were disposable and not installed; missing historical v504 binary
paths are recorded, and their older hashes are not current candidate evidence.

## Continuation rules

1. Before each implementation task, record the checklist ID, actual source owner,
   actual MCC cache/conversion provenance, concrete expected behavior, relevant
   existing evidence and required acceptance.
   Work on the next unresolved boundary rather than restating the whole history.
2. After each task, update its row/receipt with changed files, source/runtime
   hashes, actual results, scope limits and next unresolved owner. Separate
   implemented, independently compared, live composed, presentation matched and
   launcher delivered claims.
   For verified MCC integration milestones, update the canonical MCC checklist
   before starting dependent work; link its evidence rather than duplicating boxes.
3. Reuse valid authenticated references and passing checks. Repeat validation
   when dependencies changed, failures remain unresolved or new concerns justify
   it; do not rerun unchanged investigations merely to regenerate activity.
4. Reuse the qualified MCC systems and compatible build inputs identified by the
   canonical dependency; record the actual source cohort and affected consumers.
   Do not require the old private Xbox runtime or broad generated-engine rebuild.
   Preserve dirty/concurrent work, imported MCC caches, mods/saves/shaders,
   reference evidence, frozen snapshots and selected runtimes. Append historical
   goal logs; maintain this checklist as the concise current view.
5. Follow `AGENTS.md` and `docs/TEST_STORAGE.md`: CMake through `tools/run_tests.py`,
   one-off probes/captures through `tools/run_test.py`, outputs under owned
   `HALO2_TEST_SCRATCH`, selected small evidence retained, and cleanup verified.
   Retain a tested launcher runtime as an explicit deliverable with its path/size;
   do not retain routine test builds or large generated banks.

All G01-G14 whole-game gates remain open, **0/102**. OG Halo 2/TU1.5 logic stays
the target; only the imported/converted MCC cache may supply runtime assets.
Prior Xbox-runtime evidence cannot accept MCC integration/delivery. Preserve the
canonical MCC checklist's bounded progress and selected-capability distinctions.
No new runtime test, installation or large artifact is created by this doc update.


### v531 MCC menu catalogue component (2026-10-19)

- [x] Replace the twelve-name menu filter with authored catalogue identity, without changing MenuState layout. The standalone production CMake test passes 12 frozen original metadata rows and 25 literal MCC rows, preserving sort/unlock, preview/launch identity and exact scenario leaves. Two blank-title DLC placeholders remain explicitly unqualified; no alias or full map acceptance is claimed. Private CMake scratch is removed. [Component receipt](../work/sandbox-primary-v15/mcc-menu-catalog-component-v531.json).
- [x] Verify the changed MenuState through the actual imported MCC Cache and compatible native link; completed by v532 below. Renderer/launcher delivery remains its separate unchecked gate.


### v532 actual MCC catalogue/MenuState integration (2026-10-19)

- [x] Verify actual MCC Cache → native profile/sign-in → Split Screen → map list → focused preview → accepted lobby identity → selected-cache launch routing. The native CPU test passes 48 previews and 40 launch/header routes across original and reversed 25-record catalogues; four blank-title and four existing energy-weapon rejections remain explicit. All316 parent link inputs and230 frozen dependency files match before/after. No Xbox/XBE/ISO/Vista assets are opened; owned scratch is removed and no executable or raw trace is retained. [Integration receipt](../work/sandbox-primary-v15/mcc-map-picker-integration-v532.json), SHA-256 `4f983c985d3fba34f833cd04821340e82bd54cfb216dcc0dd340750e8887178c`.
- [ ] Qualify and deliver the changed frontend through the selected renderer/launcher. CPU map routing does not prove gameplay, displayed frames, audio, pause, source session return or full map parity. The two source identities and native CPU routes are resolved by v544 below; fresh-preparation inventory and selected delivery remain separate. No highplains/highplains2 alias or full-map equivalence is inferred.


### v533 MCC source-world weapon composition (2026-10-19)

- [x] Migrate the source weapon-effects fixture and its three active registrations to selected MCC/native construction, rejecting executable arguments and non-MCC headers. Seven Python admission/routing controls, three actual native rejection controls and the focused production CTest pass. Ordinary native MCC SMG input produces60→53 rounds, seven accepted positive effect requests and12 contact effects with no parallel adapter shots. All316 parent providers remain exact, successful-open tracing finds no Xbox/XBE/ISO/Vista inputs, and compiler/native scratch is removed. [Frozen migration receipt](../work/sandbox-primary-v15/mcc-source-weapon-migration-v533.json), SHA-256 `5bd70c61198ad372462c91f23a915d82b1e4d36c8394a2e4ddff3dd2eaffe93b`.
- [ ] Complete the original plasma-pistol charge/release lifecycle. The first-trigger `104698` failure is resolved by v542. Actual MCC execution reaches release/141 effect frames before missing vtable slot `45F8B4`; its complete method-table owner is now authenticated. Correct the fixture to the original instant damage/player-response effect: positive allocation, event execution and natural retirement before returning NONE. The whole-qualified preparation profile now projects the authenticated flag word2→6 (v550/v552); actual native allocation/event/player-response/retirement acceptance remains pending. Persistent particle-graph acceptance belongs to an actor whose authored charging effect actually has such a graph. Preserve the failed strict receipts; do not supply targets/results or invented effects. [Original effect contract](WEAPONS_TU15_PARITY_GOAL.md), [latest native frontier](../work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json).


### v535 complete CTF/Assault initializer closure (2026-10-19)

- [x] Port the complete original initializer and marker/query/region owners (`249460`, `24CE00`, `24CEF0`, `1A8850`). Independent execution of all three original versions passes 879 calls, normalized to 293 cases; optimized native and ASan/UBSan comparisons match the full 520-byte bank and conditional outputs. Five cases use authored marker data from imported MCC maps. The production CMake registration passes through `tools/run_tests.py`; its private build and oracle/compiler scratch are removed. [Milestone receipt](../work/sandbox-primary-v15/ctf-initializer-milestone-v535.json).
- [ ] Compose the original activation/object-birth, registry and factory owners with the real selected native world, then complete CTF/Assault rules and lifecycle acceptance. The initializer does not admit either mode or establish live gameplay, presentation, LAN or launcher delivery. Three unreachable zero-distance replacement instructions remain outside executed coverage. All 14 whole-game gates and all 102 complete-element acceptances remain open.


### v541 physics branch-owner compile fix (2026-10-19)

- [x] Fix three invalid memory-API calls in the new physics branch-data owner, preserving the authenticated table payload. Focused production CMake validation passes installation/idempotence, partial-owner retention and four atomic rejection cases, including an uncommitted reserved owner. Private build scratch is removed. [Adapter receipt](../work/sandbox-primary-v15/physics-branch-owner-adapter-v541.json).
The dependent actual MCC charge/release lifecycle remains tracked at v533 above. This compile/ownership fix supplies no runtime result, gameplay rule change, selected-runtime installation or full parity acceptance.


### v542 complete weapon program tables and MCC charge frontier (2026-10-19)

- [x] Restore all five bounded dispatch tables of the complete original `1030F0` owner: 26 words/104 bytes, including eight newly missing words. All 51 independent original guarded-selector cases and the coherent native cold-factory comparison pass. The original guards, instructions and branch targets are unchanged. The separate current-header cold executable also passes; its aggregate receipt correctly records concurrent source drift and does not qualify the subsequently changed workspace.
- [x] Verify the coherent source-world build against actual imported MCC inputs. Ordinary SMG firing passes again (60→53 rounds, seven positive effects, 11 contact effects), with all 316 parent providers unchanged and no Xbox/XBE/ISO/Vista runtime reads. The strict plasma-pistol case now passes the former `104698` failure, reaches charge/release and 141 effect frames, then fails at original `2F8B27` reading vtable slot `45F8B4`. No positive charge graph was observed. All owned build/runtime scratch is removed. [Progress receipt](../work/sandbox-primary-v15/mcc-weapon-program-tables-progress-v542.json).

The remaining charge lifecycle is the unchecked task at v533. Its persistent-graph expectation was subsequently disproved by the authenticated original instant-effect contract; that raw flag difference is now covered by the whole-qualified preparation profile in v550/v552, with actual native acceptance still pending. Failed receipts are preserved. No charged-weapon acceptance or runtime installation is claimed.


### v544 authored Tombstone/Desolation catalogue and native menu routing (2026-10-19)

- [x] Resolve both inherited blank MCC menu records from their complete authored scenario Level Data: Tombstone1402/order1500 and Desolation1400/order1400. Preserve destination bitmap identity and registration; redirect only their exact owned MCC preview partitions. The six planned stores total6,360 bytes. Eleven focused conversion controls and actual v13 reopen/idempotence/unrelated-byte checks pass. Offline converted-tag validation also finds both authored MCC preview pixel banks identical to the current Xbox LVL reference previews. [Conversion receipt](../work/sandbox-primary-v15/mcc-dlc-catalog-conversion-v533.json).
- [x] Verify the converted catalogue through actual native MCC Cache/MenuState construction: 48 previews, 44 selected-cache launch/header routes, zero blank-record rejections and four preserved energy-combat gates, including reversed authored ordering. Both new identities, localized metadata, team counts, exact MCC scenario paths and destination preview owners are checked. All316 parent link inputs remain exact, no Xbox/XBE/ISO/Vista runtime inputs are opened, and the scratch mainmenu/object/executable/profile tree is removed. The selected cache remains unchanged. [Native integration](../work/sandbox-primary-v15/mcc-map-picker-converted-integration-v544.json), SHA-256 `5ff9750cbaef09c81669a679cebf54ee20594beb8d45251d9c54099dc71d5745`.
- [x] Qualify the fresh-preparation hook, strict final DDS inventory and complete menu-asset assembly; completed by v550/v552 below. Coherent selected frontend/launcher delivery remains the separate v532 gate. CPU routing and offline preview-tag validation do not prove displayed native pixels, gameplay, transitions or complete map parity.


### v550 strict preparation inventory and original instant-charge profile (2026-10-19)

- [x] Qualify both converted preview pages through the actual full MCC inventory: all648 page owners pass,646 remain exactly unchanged, and the two authored replacements produce563 unique DDS pages. The default stock verifier rejects the changed pages; only the complete authenticated projection admits them. Original stock digests/comparisons remain recorded. [Inventory qualification](../work/sandbox-primary-v15/mcc-catalog-inventory-v534.json).
- [x] Integrate catalogue projection before private bitmap extraction, preserve all six stores/counters, record the post-serialization mainmenu hash, and pass the projection into strict final menu inventory. Production CMake preparation/conversion/inventory tests pass; a further preparation/instant-charge registration run passes after the new scalar hook. Both private builds are removed. [Catalogue CMake](../work/sandbox-primary-v15/mcc-catalog-preparation-cmake-v547/report.json), [charge-profile CMake](../work/sandbox-primary-v15/mcc-instant-charge-preparation-cmake-v550/report.json).
- [x] Validate the named instant-charge effect's complete160-byte source tuple, root marker and salted damage reference before projecting only its stored flag word2→6 to the original raw tag value. Eight profile controls and a real selected-MCC memory-only plan/idempotence check pass. Unknown custom definitions stay unchanged; this is no generic effect-bit heuristic. The current-header cold factory also passes the complete reached contact method tables. [Frozen source checkpoint](../work/sandbox-primary-v15/mcc-instant-charge-source-checkpoint-v549.json).

The copied-MCC preparation/bitmap/menu-asset pipeline subsequently passed v552. Actual converted-MCC instant charge remains the v533 gate; the selected cache/runtime remains unchanged. Retained idle-loop graph acceptance stays a separate task for an actor authored to emit that graph.


### v551 native CTF/Assault activation and registry owners (2026-10-19)

- [x] Port25 complete TU1.5 initializer/first-activation/registry owners and wire their exact entry predicates into the real World dispatcher source. Independent original/native optimized and ASan/UBSan comparisons pass278 cases:209 complete returns and69 exact unsupplied-owner stops, with1,180/1,459 production instruction sites observed. The isolated production CMake test passes with strict FP. Actual object factories, existing registry services and dirty ownership remain genuine external native owners. [Milestone receipt](../work/sandbox-primary-v15/ctf-activation-milestone-v551.json).

Real same-bank MCC factory birth, remaining CTF/Assault rules and complete lifecycle acceptance remain the unchecked v535 gate. The full World object was not rebuilt or live-qualified by the isolated test; mode admission is unchanged. FP oracle status exclusions are documented. Required text regression fixture `tests/fixtures/ctf_activation_original_tu15.json` is1,097,045 bytes; compiler/test scratch is removed, with no retained executable, raw runtime bank or capture.


### v552 actual43-map private preparation and final menu assembly (2026-10-19)

- [x] Run the real preparation path on private copies of all43 selected MCC maps, texture bank and fonts. All25 multiplayer maps are processed;1,045 ordered stores include only20 changed blocks: six complete catalogue/resource stores and14 qualified instant-charge flag words. The distinct Floodlab effect tuple remains unchanged and explicitly unqualified. Strict inventory and final asset assembly pass648 frame owners,563 unique DDS pages and40 font files. All selected assets, registry/provenance, source files and launcher owners remain exact. The4,332,372,016 generated bytes are removed with their owned scratch. [Preparation receipt](../work/sandbox-primary-v15/mcc-private-preparation-milestone-v552.json).

The combined native step did not run: its factory copy caught a concurrent extra-brace syntax error at line361, subsequently corrected in the live source. The aggregate failure remains preserved. Repeat only the native stage against fresh owned MCC copies; do not repeat this passing43-map/bitmap/menu preparation. Actual instant-charge acceptance remains the v533 gate, and selected frontend/renderer delivery remains v532.


### v555 actual prepared-MCC instant-charge caller boundary (2026-10-19)

- [x] Compile/link the corrected genuine factory with all316 retained providers unchanged, then test private MCC core copies with the qualified instant-effect conversion. The actual effective flags6/one-event/one-jpt/no-particle contract passes; SMG firing and both native input-open audits pass. No43-map/bitmap/menu preparation is repeated. All selected/frozen input guards pass and owned scratch is removed. [Native caller receipt](../work/sandbox-primary-v15/mcc-instant-charge-caller-frontier-v555.json).

The charge fixture stops at its first input tick80, original108E26 storing NONE, before any required positive damage allocation/player-response observation. Its whole108DB0 caller/control context and observer coverage are now the unresolved boundary; this is not yet a confirmed gameplay defect. Preserve that actual result and compare the complete original caller before changing expectations. The v533 charge gate remains open.


### v558 Floodlab conversion and actual CTF constructor frontier (2026-10-19)

- [x] Qualify the separate164-byte Floodlab instant-charge tuple with its authored inline root marker and complete damage graph. Fourteen focused controls, actual MCC scratch projection, production v13 serialization, independent reopen, idempotence and the registered production CMake test pass. Only the authenticated four-byte flags word changes2→6. All selected/source owners remain exact; both private scratch trees are removed. This resolves the distinct tuple preserved by v552 without repeating its passing43-map preparation. [Milestone receipt](../work/sandbox-primary-v15/mcc-floodlab-and-ctf-frontier-v558.json).

The first actual current-World MCC CTF probe compiles/links but stops during the original contrail constructor11E550/11E55D at missing backing505E90, before CTF activation. Its698-path open audit passes with no forbidden, unknown or unmatched inputs. The complete destination already has a genuine current scalar-state owner; the stale archive provider must be refreshed. Preserve the failed run and retry that dependency. No CTF birth, charge lifecycle, selected frontend or full parity gate is accepted; v533/v535/v532 remain open. No large artifact is retained.


### v559 reuse the passed corrected-menu native build (2026-10-19)

- [x] Independently review the existing V23 native menu result and preserved compiled owners. The native run exits0 after412 title/profile/keyboard/main/Settings/back frames, with finite nonzero title/menu audio; its real-input Lockout regression also exits0. Root visually inspected the actual640x480 profile and Settings frames: complete SELECT/BACK artwork and captions fit. All retained file and reference hashes match the sealed receipt; prior scratch is removed. No native test or capture is repeated. [Existing native evidence review](../work/sandbox-primary-v15/mcc-preserved-menu-native-review-v559.json).

Prepare the actual ordinary Play handoff using this coherent successor while preserving the selected cameraV3 gameplay action, runtime and shaders. Check whether its menu catalogue needs the already qualified current MenuState/content and converted data. Selected frontend delivery remains v532; native widescreen frames, complete menus/maps, original framebuffer/audio parity and full-game acceptance remain open. The118,332,732-byte compiled/reference input cohort and180,872-byte two-PNG proof are existing required retained inputs, not newly generated test outputs.


### v560 actual installed MCC sandbox route (2026-10-19)

- [x] Verify the installed shortcut through the real regular launcher and exact selected cameraV3 native runtime. It exits0 after six hidden muted MCC Lockout frames, seven ticks and one player. The1,974-open/28-trace audit has zero forbidden, unmatched or remaining unknown inputs. Only the two exact save paths are reclassified using the recorded namespace bind and independently verified scratch backing. The original mechanical-red receipt is preserved; no native rerun or broad audit exception is used. [Installed route milestone](../work/sandbox-primary-v15/mcc-installed-shortcut-milestone-v560.json).

Protected file hashes/device/inode/size/mtime, full selected-cache stable metadata, actual selection and installed saves remain unchanged. Concurrent inode ctime changes are explicitly recorded; no unchanged-ctime claim is made. Both owned scratch trees are removed and no large artifact is retained. This closes the earlier shortcut-through-runtime uncertainty only. Visible/unmuted/audio, complete maps, ordinary Play frontend delivery and full parity remain separate open gates.


### v566 actual MCC current-World initialization (2026-10-19)

- [x] Compose the genuine current World and complete updated cold-data providers with the compatible native cohort. All26 translation units compile/link; actual MCC Lockout world construction and map initialization return. The698-path input-open audit passes with no forbidden, unknown or unmatched inputs. Frozen source, all316 retained providers, selected cache/runtime and current-workspace guards pass; scratch is removed with no large artifact retained. [Initialization checkpoint](../work/sandbox-primary-v15/ctf-activation-port-20261019/current-world-initialization-checkpoint.json).

CTF session configuration now reaches original159FF0/15A0EA and requires its real static engine object48BFA0/interface466190. Configuration, activation and object birth have not completed. Audit and restore that complete authored object/interface/registration owner before continuing; no ready-state seeding or mode admission. The v535 lifecycle and all whole-game gates remain open.


### v573 compiled independent launcher routes and complete CTF static data (2026-10-19)

- [x] Implement and production-test independent frontend selection while keeping the native gameplay runtime unchanged. Six focused CTests pass, including five actual compiled-launcher routing cases, eight selection-admission cases,14 shortcut cases and18 real launcher UI cases. Missing selection preserves prior behavior; stale explicit selection blocks ordinary Play while the native test remains available. Newly compiled normal/title launchers both retain the exact cameraV3 native owner; no selection or installed launcher is changed. [Combined milestone](../work/sandbox-primary-v15/mcc-separated-launcher-and-ctf-milestone-v573.json).
- [x] Restore the authenticated complete CTF/Assault static object/interface/publication destination:212 bytes, all51 original callback identities, correct original sections/access, duplicate/partial/reserved-owner rejection and adjacent-owner preservation. Optimized and sanitized component comparisons pass69 cases; the registered production CMake test passes. The real factory now mounts this data before imported tags, with existing source activation/callback owners unchanged. Actual configured activation and object birth remain the v535 gate.

The initial launcher target's unnecessary native dependency build was canceled before tests; its229,970,142 generated bytes are removed. The launcher-only configuration builds the actual routing binaries without a native rebuild, preserving normal source-build defaults. Its179,704,999-byte private tree is also removed after retaining552,190 bytes of requested working launcher inputs/frozen source. Source-only reproduction cohorts for the current World and charge test are retained separately; no raw bank or disposable engine binary is retained. Actual ordinary Play publication, expanded native map-picker adoption and full parity remain open.


### v574 actual expanded MCC map picker and bounded cold-table correction (2026-10-19)

- [x] Adopt the qualified catalogue MenuState over the preserved V23 client, reusing its identical qualified content producer. The coherent325-provider V24 run exits0 after818 native frontend frames: real title/profile/main/Split Screen/lobby/setup/map-picker input,15 Down inputs and return. Root visually reviewed the actual Backwash selection, authored preview/description and surrounding DLC rows. Selected MCC cache, parent inputs and frozen dependencies stay exact; no forbidden asset input is opened. [Actual native picker](../work/sandbox-primary-v15/mcc-expanded-menu-native-v572/report.json).
- [x] Correct the cold weapon-table unit's owner-label assumption for the two already-authenticated185111 interface words. The native unit passes26 original words/104 bytes,51 guarded selectors and10 contact methods, retaining exact bytes/read-only/unique-owner/write-rejection checks. All316 providers and frozen inputs remain exact. [Cold unit result](../work/sandbox-primary-v15/test-cold-weapon-ownership-v571.json).

V24 retains111,479,920 bytes of requested executable/object inputs plus its small frozen source/cohort and102,933-byte required picker reference; both native-test scratch and cold-test scratch are removed. The charge diagnostic was stopped after automatic review flagged possible cybersecurity risk; the completed follow-up ran ordinary native unit tests only. Charge remains unaccepted. Two unconverted blank catalogue identities remain gated, as do energy-combat paths; native widescreen, map gameplay, ordinary Play installation and full parity are not accepted by the map-picker run. Reuse this V24 result for staging; do not repeat the passing menu flow.


### v576 final launcher consistency and CTF configuration owners (2026-10-19)

- [x] Match launcher status and controls to actual stale-selector behavior. Three focused production CTests pass: five compiled routing cases,22 real Tk UI cases and576 original/native CTF configuration cases. A changed explicit menu build disables only ordinary Play and reports that condition accurately; native test, sandbox and preview remain independent. Full-ready valid frontend selection and missing-selection fallback remain intact. Both newly compiled launchers preserve the exact cameraV3 native owner. [Milestone](../work/sandbox-primary-v15/mcc-frontend-and-ctf-configuration-milestone-v576.json).
- [x] Port and wire complete original24B390/249440 configuration owners: all18 instruction sites and576 cases compare full defined ABI, FP state and banks with optimized and sanitized native execution. The real MCC session now passes its class dispatch and249460 initializer, then reaches further callback24A6C0. Configuration, objective birth and lifecycle acceptance remain open. The old25-owner activation module stays unchanged.

The successful private CMake tree is removed after retaining the requested small normal/title launcher successors and frozen source. No selector, root launcher, selected cache or runtime is installed by this checkpoint. The2,745,863-byte declared synthetic CTF configuration fixture is a required regression input, not a captured runtime bank. V24's passing native menu proof is reused for the pending frontend staging/delivery.


### v577 qualified isolated frontend staging (2026-10-19)

- [x] Stage and independently recheck the passed V24 frontend with its own97-file shader bank and complete52-resource MCC capability. The99-file F24 owner totals107,082,951 bytes. Only22 bytes inside the authenticated64-byte read-only shader-path field change; game instructions remain exact. The existing capability checker and every retained-file hash/size pass. All protected325 inputs, old cameraV3 runtime/106 shaders, installed launchers, selected cache/registry and absent-selector state remain unchanged. [Stage receipt](../work/sandbox-primary-v15/mcc-frontend-stage-v573.json).

Owned staging scratch is removed. F24 is an explicitly retained requested working runtime; this filesystem milestone starts no game and publishes no selector. Actual normal/title launcher adoption and installed Play→native menu acceptance are the next separate delivery step. Basic menus/raw expanded catalogue only: two DLC placeholders, unfinished gameplay and full parity remain open.


### v579 installed independent MCC title/menu route (2026-10-19)

- [x] Install the qualified normal/title launcher pair and independent F24 frontend selection. Both actual installed `--launcher-info` calls identify cameraV3 for gameplay and F24 for Play/title/menus. The installed MCC sandbox `--check` passes both capabilities and still selects the exact original cameraV3 native-test owner. Selected cache/registry/provenance, sandbox wrapper, old runtime and all106 old shaders remain unchanged. Both prior launchers are retained under the explicit rollback owner. [Installation receipt](../work/sandbox-primary-v15/mcc-frontend-install-v578.json).

This is local route installation/admission only; no game is executed by the installer. The actual installed six-frame CLI route and actual GUI Play-button→title handoff are being tested separately. F24's earlier818-frame menu/map-picker proof is reused. Full menus, two converted DLC catalogue records, all-map gameplay and full parity remain open. Installer scratch is removed; working installed launchers and436,224-byte rollback launcher pair are explicitly retained.
