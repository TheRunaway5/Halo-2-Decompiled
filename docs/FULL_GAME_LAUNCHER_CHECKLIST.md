# Full game through the launcher: OG Halo 2 / TU1.5

Requested goal: launch the complete native game, including menus, every map and
mission, and all sandbox behavior, matching original Xbox Halo 2 English TU1.5
one to one. This is the maintained checklist for
[the existing parity goal](SANDBOX_TU15_PARITY_GOAL.md), not a replacement goal.

Status on the resumed 2026-10-19 turn: **incomplete; 0/102 whole sandbox elements
accepted**. The goal service still reported `usageLimited` after the explicit
resume request. Its status is host controlled; this document records the
authorized continuation and checklist without claiming the service resumed.
The tool interface cannot edit an unfinished goal's objective, so this checklist
is attached through the maintained goal document and continuation record.

## Current work and order

1. **G02, active:** make the explicit Xbox menu route independent of mandatory
   Vista menu/loading assets, and connect it to the sandbox launcher. Source
   owners: private `FrontendOptions::xbox_only` in `include/halo2/frontend.hpp`,
   `src/main.cpp` (`--menu-xbox`), `src/frontend.cpp` (`run_frontend`), and
   `tools/sandbox_test_launcher.py`. This host route does not establish original
   menu lifecycle or composed game-world behavior. The first candidate compiled
   the runtime and passed five of six selected frontend checks. The all-pane
   `retail_frontend` sweep stopped at deliberately retired screen 13. An immutable
   authored-capture API is being added to preserve all 84-pane/150-state render
   checks and retain retirement in ordinary navigation. Actual Xbox screen 13
   lifecycle remains open under G02/G11. Next: fresh v513 build/checks after this
   fix; `xbox-menu-validation-v510/tests.log` preserves the intermediate failure.
2. Validate the actual native Xbox menu with owned original assets and the
   launcher route. Package a coherent, identifiable user-test runtime only after
   the relevant checks pass; update G01/G13 with its real installed hashes.
3. Continue the source menu-to-map-to-gameplay path under G05/G06. Record each
   reached original owner and the next failing boundary; no forced map-ready,
   supplied successful callback or parallel gameplay substitute. The next pause
   source boundary is authenticated TU1.5 factory `238B15 -> 238AC2`, game-time
   global `51DF54`, and action dispatch `238A67`. The v512 correspondence audit
   identifies 15 original bodies; it is not a port, CPU comparison or live proof.
4. Resolve G03/G04's original audio admission and processing dependencies, then
   complete every campaign/MP map, sandbox, multiplayer and presentation gate.
   The audio frontier below is historical evidence, not a new failure this turn.

## Whole-game checklist

Every item remains unchecked until its stated acceptance is met. Partial work
belongs in the evidence ledger below. Paths in backticks are relative to the
workspace root, including when reading the private source copy of this document.

| ID | Required result | Current evidence, source owner and next acceptance |
| --- | --- | --- |
| G01 | [ ] Launcher starts the coherent tested runtime | Source v508 is not delivered. Last verified installed selection is r6, SHA-256 `5900bf2c7902976132a4eb17a709dd999f8ecaaa1f6904e0bc64b9c1c70b3884`. Owner: launcher selection/runtime packaging. Accept only after actual selected executable/assets/shaders and source hashes match the tested candidate, and ordinary launch succeeds. |
| G02 | [ ] Original frontend and menus | **Active host-route work** in `FrontendOptions::xbox_only`, `main.cpp`, `frontend.cpp` and the sandbox launcher. Initial frontend validation is 5/6; the authored all-pane sweep reaches retired screen 13, and a separate immutable capture path is being added before fresh v513 validation. Screen 13's actual Xbox lifecycle remains open. Next pause owners: authenticated TU1.5 `238B15 -> 238AC2`, `51DF54`, dispatch `238A67` (static evidence only). Complete source input, profiles/settings, campaign/MP selection, pause and back/quit transitions; accept original behavior/appearance with normal menu-to-game-to-menu lifecycle evidence. |
| G03 | [ ] Native startup completes ordinary map audio admission | Historical v504 campaign path reaches `127EE0 -> 224EC0 -> SDK/APU -> 38E8C9`, then rejects `38E8D3 -> FE801100 = 0x88`: FECTL bit3 remains unverified. Owner: `SourceRuntimeSoundConfiguration` and original MCPX configuration/IRQ callers. Establish all requested semantics and producers from original evidence; accept when actual configuration returns and ordinary admission proceeds without invented device results. |
| G04 | [ ] Complete GP/EP audio installation, execution and PCM | CPU preparation/descriptors are partial verified work. Original `38846E` command publication still rejects; complete `38E2E0` hardware/factory, native command consumer/install/ACK, processing/status/supervisor/clock/voices/GP-EP transport, IRQ, codec and asynchronous resource retirement. Accept actual source-produced audio through normal gameplay and independent original PCM/lifetime comparisons. |
| G05 | [ ] Complete map/BSP loading and transitions | Reuse authenticated source resource/loader/callback work. Source admission root `191DF0` and actual readiness owner `191E2E` must finish; complete new-BSP activation, dependency/postprocess, unload/reload and asynchronous cache/animation/resource ownership. Accept normal frontend-selected maps and transitions without forced readiness or synchronous substitutes for required async owners. |
| G06 | [ ] Original full game frame, players and objects | Source world, frontend, Simulation, player/object and authored script owners remain incompletely composed. Follow actual owners/update order through spawn, input, simulation, death, respawn and map retirement. Accept ordinary gameplay with real downstream allocations, contacts, state and lifecycle. |
| G07 | [ ] Every authored campaign mission | Build/maintain the complete original mission/BSP inventory; include encounters/AI, scripts/objectives, cinematics, difficulty, checkpoints/save/load, death/restart and progression. Accept complete normal mission progression across the inventory; component/map fixtures are narrower evidence. |
| G08 | [ ] Every authored multiplayer map and interaction | The v511 inventory authenticates 23 owned candidate maps (12 retail plus 11 DLC) against original catalogs/manifests; native acceptance remains pending for every map. Use `owned-multiplayer-launcher-maps-v511.json` and `owned-map-catalog-v511.json` rather than rebuilding the inventory. Complete normal select/spawn/play/return flow, props, breakage, machinery, doors/lifts/trains, supports/riding/crushing, effects and authored interactions. Accept all map contexts and lifecycles; catalog admission, UI titles and selected Lockout/world tests do not complete them. |
| G09 | [ ] All 63 weapon definitions | Preserve every existing weapon census ID, including AI, vehicle/turret, objective and unused authored variants. Complete input/ownership, triggers/barrels, both hands, reload/charge/special behavior, events and applicable presentation. Accept all six gates for each definition in the canonical matrix. |
| G10 | [ ] All 39 shared sandbox requirements | Preserve the matrix's powers (3), vehicles (4), projectiles (6), damage (4), world (5), AI (6), objectives (5) and presentation (6) IDs. Complete original owners and applicable branches for every row. Accept all six gates per row; selected movement, damage, vehicle, grenade or objective successes remain partial. |
| G11 | [ ] Local play, split screen and LAN | Original Xbox screen 13 lifecycle remains open alongside G02; its authored capture must not enable retired navigation. Complete applicable normal matches, source identity/events/lifecycles, independent views/hands, join, loss/duplication/reordering, death/reset and recreation. Accept rendered normal sessions and required transport/lifecycle cases, with no parallel substitute gameplay. |
| G12 | [ ] Original animation, rendering, HUD and audio presentation | Complete both species/hands, poses/overlays/blends/IK/root motion, materials/lighting/shadows, weather/effects, UI/HUD/scopes and sound lifetimes. Accept matched original TU1.5 poses, frames and PCM in menus, campaign and MP. Numeric/GPU/component checks alone do not pass this gate. |
| G13 | [ ] Tested launcher build delivered for user testing | Build a coherent candidate, run appropriate regressions, inspect its actual launcher-selected hashes, and exercise ordinary launch/menu/map/gameplay paths. Record exact tested source/runtime/assets, limitations and user launch commands. Deliver the tested files, with generated test scratch removed. |
| G14 | [ ] Full requested game accepted | All applicable G01-G13 gates, all missions/maps and all six acceptance gates for every one of the 102 sandbox elements must pass. No component milestone, rendered frame or successfully opened menu certifies full parity. |

## Acceptance authority and preserved inventory

Original Xbox English TU1.5 XBE SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Owned original Xbox assets and authenticated correspondence to retail establish
reference behavior. Vista/MCC and guessed tuning/service returns do not establish
OG parity. Unknown source owners and unexecuted branches remain explicit.

The canonical inventory remains
`analysis/sandbox-tu15-goal-20261013/coverage.json`: **63 weapons plus 39 shared
requirements; all 102 incomplete**. Retain every existing ID and evidence entry.
Its `latest_checkpoint` field still names historical `accepted-checkpoint-v14.json`;
do not treat that pointer as the latest source state or erase its older receipts.

Each element retains all six gates: authenticated reference; independently
compared original/native logic; real live composition; local/split/LAN publication;
matched original presentation; coherent tested launcher delivery. The broader
menus/maps/missions requirements above remain in addition to that sandbox matrix.

## Reuse ledger: bounded checkpoints already established

Checked entries below describe only their named milestones, not whole-game gates.

Current-turn intermediate evidence, with no candidate/delivery acceptance yet:

- `work/sandbox-primary-v15/xbox-menu-validation-v510/tests.log`: first runtime
  candidate compiled; five of six selected frontend checks passed. The remaining
  authored-render sweep rejected retired screen 13. Fix and fresh v513 validation
  are pending; do not delete retirement or reduce the 84-pane/150-state sweep.
- `work/sandbox-primary-v15/owned-multiplayer-launcher-maps-v511.json` and
  `owned-map-catalog-v511.json`: 23 owned candidate MP maps authenticated against
  original catalogs/manifests. All native map acceptance remains pending.
  `sandbox-launcher-titles-v511.log` records nine passing source-only launcher UI
  checks; these do not validate an installed package or native gameplay.
- `work/sandbox-primary-v15/xbox-pause-next-boundary-v512.md` and
  `xbox-pause-source-correspondence-v512.json`: static correspondence for 15
  original bodies, including the authenticated TU1.5 pause factory/constructor,
  global and dispatch above. No port, original CPU execution or live integration
  is claimed. The separate multiplayer pause factory remains unauthenticated.

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

Latest authoritative source receipt:
`work/sandbox-primary-v15/native-EP-source-progress-v508.json` (389 source and
13 installed fingerprints). Earlier receipts:
`native-gp-scratch-progress-v504.json`, `native-dsp-lookup-progress-v490.json`,
`native-sound-download-progress-v477.json` in the same directory. v508's current
test executables were disposable and not installed; missing historical v504 binary
paths are recorded, and their older hashes are not current candidate evidence.

## Continuation rules

1. Before each implementation task, record the checklist ID, actual source owner,
   concrete expected behavior, relevant existing evidence and required acceptance.
   Work on the next unresolved boundary rather than restating the whole history.
2. After each task, update its row/receipt with changed files, source/runtime
   hashes, actual results, scope limits and next unresolved owner. Separate
   implemented, independently compared, live composed, presentation matched and
   launcher delivered claims.
3. Reuse valid authenticated references and passing checks. Repeat validation
   when dependencies changed, failures remain unresolved or new concerns justify
   it; do not rerun unchanged investigations merely to regenerate activity.
4. Implementation changes stay in `.local/sandbox-tu15-source-20261014-v15`;
   preserve shared root/concurrent source, original assets, references, frozen
   snapshots and launcher-selected runtimes. Update historical goal logs only
   by appending; maintain this checklist as the concise current view.
5. Follow `AGENTS.md` and `docs/TEST_STORAGE.md`: CMake through `tools/run_tests.py`,
   one-off probes/captures through `tools/run_test.py`, outputs under owned
   `HALO2_TEST_SCRATCH`, selected small evidence retained, and cleanup verified.
   Retain a tested launcher runtime as an explicit deliverable with its path/size;
   do not retain routine test builds or large generated banks.

All whole-game gates remain open. Current-turn entries record intermediate
evidence only; final menu/launcher validation, acceptance and delivery outcomes
must be added when available. This documentation work retains no large artifact.
