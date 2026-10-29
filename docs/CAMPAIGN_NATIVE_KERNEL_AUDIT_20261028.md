# Campaign native-kernel audit — October 28, 2026

Implementation is now tracked in [the native campaign goal checklist](CAMPAIGN_NATIVE_ENGINE_GOAL_CHECKLIST.md). The findings below are the pre-implementation audit baseline; use the checklist's per-kernel evidence ledger for current implementation, proof, integration and acceptance status.

This audit examines the current working tree and retained primary-source evidence. It does not run or install a new campaign build. A native semantic kernel must implement the TU1.5 algorithm with verified behavior; compiled instruction lifting is an intermediate implementation, and a host approximation is a different remaining gap. Offline executable/oracle execution does not mean a binary emulator ships in gameplay. The repository makes this distinction explicitly in [the migration audit](PHYSICS_NATIVE_KERNEL_MIGRATION.md), lines 18–23.

The user's intended end state is a complete native campaign engine: authored cutscenes, AI configurations and mission sequences should run through the engine's supported services. This is an engine implementation audit, not a request to hardcode individual missions. The working-tree HEAD at inspection was `ec3fdb77161886514b67bcdfd6bc7ec5d6659c88`; substantial pre-existing concurrent changes were present. Retained test cohorts qualify their captured inputs, not every later working-tree edit or the currently selected launcher runtime.

## What belongs in tags and what belongs in the engine

Tags and scenario resources provide parameters and authored programs; engine code implements their meaning. [Content declarations](../include/halo2/content.hpp), lines 198–241, distinguish character definitions, actor/squad placement and the script program's expression nodes, script declarations and globals. [Character import](../src/assets/content.cpp), lines 856–896, reads inherited biped, vitality, perception and general settings; lines 1215–1304 read scenario squads, starts, placement-script names and the compiled script program. [Squad import](../src/campaign/ai/source_ai_import.cpp), lines 5–17, delivers authored team/group/start-location data rather than an AI decision algorithm. [Campaign script construction](../src/gameplay/simulation.cpp), lines 745–757, passes `content.scripts` into `ScriptVm` and binds engine builtin calls to the host. [The VM](../src/campaign/scripting/scripts.cpp), lines 398–421 and 671–675, reads authored nodes and dispatches builtin calls.

| Authored data or script supplies | Native engine must supply |
| --- | --- |
| Character settings, species assets, squad composition, encounter locations and orders | Perception, behavior selection, targeting, navigation, movement, combat, vehicle/seat actions and actor lifecycle |
| Script expressions, function calls, globals, sleep/wake conditions and command sequences | A faithful Halo script runtime, scheduling, typing/coercion, builtin evaluators and command execution |
| Animation graphs/clips, camera tracks, dialogue, effects and authored timing | Animation/controller evaluation, attachment/IK, cinematic cameras, speech/audio, effects and synchronized presentation |
| BSP references, triggers and mission progression requests | Streaming/lifecycle, spatial queries, activation, checkpoints/revert, mission completion and cooperative world authority |

A native VM interpreting Halo's authored script expression data is appropriate. The excluded backend is Xbox machine-instruction emulation or instruction-lifted gameplay execution. Merely having a native host `ScriptVm` also does not establish that its scheduling, evaluation and builtin semantics match TU1.5. Engine behavior may be fixed code while still being parameterized by tags; mission-specific hardcoded substitutions are not a completion strategy.

## Findings at a glance

| Feature family | Missing work under the native-kernel requirement |
| --- | --- |
| AI decisions/combat | Concrete absent callbacks plus conversion of the remaining lifted decision/frame/behavior pipeline |
| Navigation | Original search/cost/hint/obstacle/movement algorithms; existing geometric host router is not parity |
| Mission and command scripts | Required unsupported builtins/commands and full HS semantic scheduling/evaluation proof |
| World/player/object scheduling and physics | Remaining lifted scheduler/object owners and collision/contact/listener/lifetime residuals |
| Map startup and BSP changes | Full original lifecycle ordering/integration; current selected callbacks cover only part of it |
| Checkpoints and mission progression | Original save-safety integration, complete state replay and completion/restart lifecycle qualification |
| Local/network co-op | Additional participant birth/input/views, session launch and shared campaign authority/restore/transport |
| Campaign audio and speech | 24 of the campaign group's 40 native routine implementations, group activation, and speech dependencies |
| Cinematic/presentation control | Remaining lifted control/camera/screen/composition owners and specific unsupported camera/spatial cases |

These are feature families, not a percentage or exhaustive count of missing functions. Existing kernels, lifted bodies, integration gaps and unproved branches are separated below.

## AI, decisions and combat

**The complete native semantic AI decision/behavior pipeline is still missing.** The live world dispatches conditions, object decisions/callbacks, actor behavior, and the AI frame through register/memory ABI owners: [live dispatcher](../src/physics/physics_live_world_tu15.cpp), lines 2278–2336. [Actor behavior](../src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp), lines 1781–1835, is a concrete example: each original instruction manipulates `WorldState` registers, flags, virtual stack and fixed-address memory, then dispatches an original address. This is statically compiled instruction lifting, not a closed typed semantic behavior kernel. The generated header's statement that bodies compile to native arithmetic/static calls does not close this conversion requirement.

Preserve the structured foundations already present: [actor control](../src/campaign/ai/source_ai_actor_control.cpp), lines 75–127, implements actual activation checks and state publication over typed pool/services interfaces. They do not supply the full perception, targeting, alertness, order selection, tactical behavior, aiming, cover, melee, navigation or vehicle/seat dependency closure. [The feature checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md), lines 1188–1221, keeps these composed behaviors open. Species, difficulty, encounter activation/reinforcement and despawn coverage also remain open.

### Concrete blockers in the retained 900-tick encounter cohort

The checklist's statement that v10 is still running is stale. [The completed v10 cohort](../analysis/campaign-ai-coop-20261020/current-source-v10/current-source-cohort.json) has `phase: completed`, `passed: false`, an unchanged captured snapshot, and successful build, original activation, session-seed, component and cold-world ownership checks. Its four 900-tick authored MCC probes all stop at explicit missing world owners:

| Actual authored Cairo case | Last completed source tick | Missing call | Classification |
| --- | ---: | --- | --- |
| Three floor Marines, `1st_hum_floor_mar` | 288 | `1B0195 -> 1BE5B0`, descriptor pointer `48ADC0` in the retained GP state | Concrete absent actor-behavior callback owner in the build containing the recovered descriptor27 owner. The semantic behavior name is not established by this log. |
| Johnson, `1st_hum_johnson` | 563 | `1EA652 -> 1BAE40` | Concrete absent actor/frame dependency, actual root `1CFE60`; semantic role still needs authenticated recovery. |
| Elite, `1st_cov_wv1_elt` | 563 | `1EA652 -> 1BAE40` | Same actual boundary. |
| Three Grunts, `1st_cov_wv1_grt` | 563 | `1EA652 -> 1BAE40` | Same actual boundary. |

Evidence: [Marine log](../analysis/campaign-ai-coop-20261020/current-source-v10/runtime-1.json), [Johnson log](../analysis/campaign-ai-coop-20261020/current-source-v10/runtime-2.json), [Elite log](../analysis/campaign-ai-coop-20261020/current-source-v10/runtime-3.json), [Grunt log](../analysis/campaign-ai-coop-20261020/current-source-v10/runtime-4.json), each lines 54–73. All retain zero script faults, initial checkpoint existence, no original-reference input opens and no cache write attempts. This is bounded headless integration; it does not prove combat, rendered traversal, checkpoint replay or co-op. The cohort runtime SHA-256 is `fbf89be63ffac80a043939eeadc03012218e8d64b32f38cfa66b7ac26eb26c8e`; source-input manifest SHA-256 is `e93148f92829b34647342a642b379c75a985753539ffb6576650772b0c251d5c`. Later current-tree changes are not qualified by that frozen build.

A read-only scan of all current `src/**/*.cpp` finds no `run_1be5b0`, `run_1bae40` or `run_22cb90` body. `22CB90` remains an explicit required safety dependency of [conditions](../src/campaign/ai/source_campaign_ai_conditions_tu15_generated.cpp), line 809, and [AI frame](../src/campaign/ai/source_ai_frame_tu15_generated.cpp), line 64998; the original/native post-call conditions remain unqualified per [the checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md), lines 157–177. These three addresses are concrete recovery targets, not a complete inventory of all unavailable dependencies.

Do **not** report `1BD1E0`, `1ED100`, or `200C00` as absent bodies. All exist in the current [actor behavior module](../src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp), with live dispatcher selection at lines 2293–2294 of the world module. Descriptor27/character-data comparisons pass 372 complete component calls, bringing combined component evidence to 16,319 cases, per [the component proof](../analysis/campaign-ai-coop-20261020/behavior-27/component-proof-v1.json). They remain lifted implementations under the stricter semantic-kernel requirement. Their separate `27C710` squad fallback and the transformed distance-path call/return are not fully qualified by those focused fixtures; see [the checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md), lines 1135–1187.

Later evidence records further bounded gains. [Actor-death integration](../analysis/sword-lunge-tu15-20261025/campaign-actor-death-integration-final-seal/report.json) seals five additional lifted AI-frame bodies (`C9460`, `106980`, `106AE0`, `106DF0`, `1EAD40`) with 333 component cases, while explicitly retaining unexecuted root/descriptor callbacks. [Three sword controls](../analysis/sword-lunge-tu15-20261025/fresh-campaign-three-ai-death-observer-v2/execution/report.json) pass against pinned reference archives, with a scope that expressly excludes current-source/selected-delivery equivalence. [Spawn-gate preparation](../analysis/sword-lunge-tu15-20261025/campaign-spawn-gate-production-preparation-attempt3/report.json) records 522 physical original comparisons for `20BBC0` and no production instruction decoder or original-code execution. These gains do not supply `1BE5B0`/`1BAE40` or complete semantic AI conversion.

## Navigation and NPC traversal

**The original navigation/search/movement pipeline is not replaced by a 1:1 native semantic implementation.** The current public [navigation interface](../include/halo2/navigation.hpp), lines 5–7, explicitly calls its cost the host's geometric cost and leaves retail actor movement, jump hints and dynamic obstacle costs separate. [The implementation](../src/campaign/ai/navigation.cpp), lines 57–108, performs A* using sector-center Euclidean distance, portal midpoints and a crouch admission filter. Authored sectors/portals alone do not supply the TU1.5 search, cost, hint or movement algorithms.

Remaining feature families are original path costs/search, authored hints, movement admission and dynamic obstacles; grounded/crouch/jump, flying/swarm and vehicle routes; blocked/replanned paths and cross-BSP traversal. The precise closure should come from authenticated owners and actual reached calls, not replacement steering rules. [The checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md), lines 1223–1231, keeps all three navigation acceptance gates open.

The host command path still calls this router at [simulation.cpp](../src/gameplay/simulation.cpp), lines 958–984, and its host AI path at lines 2100–2162 uses view-cone/raycast nearest-target selection, a simple awareness timer, and route following. These are additional non-parity implementations, not proof they execute for source-owned actors: the same source contains a separate source-AI branch before this host loop, and the mover returns early when source physics owns the actor (lines 986–994). Conversion must preserve the single source authority and prove the actual path selected; do not count these host algorithms as a TU1.5 semantic kernel.

## Mission scripting and command execution

The engine must execute authored mission programs and their builtin calls, rather than replace them with per-mission host logic. The current [script VM](../src/campaign/scripting/scripts.cpp), lines 293–339, 591–675 and 691–714, owns host expression frames, threads, sleep/wake and command state. Some helpers have original-comparison evidence; complete TU1.5 scheduling/evaluation equivalence is not established by that foundation.

Concrete unsupported authored services include:

| Service | TU1.5 evaluator identities | Current boundary |
| --- | --- | --- |
| AI orders/living counts | `ai_set_orders: 2AF970`, `ai_living_count: 2AF6A0` | No current handler/complete evaluator closure found |
| Authored AI awareness | `ai_magically_see: 2AF0C0`, `ai_magically_see_object: 2AF110` | Same |
| AI vehicle boarding/exiting | `2AFC20`, `2AFC70`, `2AFCC0`, `2AFE00` | Same |
| Flying command scripts | `cs_fly_to: 2B0890/2B0920`, `cs_fly_by: 2B0AD0/2B0B60` | Unsupported actor-command branch |
| Scripted shooting/jumping | `cs_shoot: 2B1450/2B14B0`, `cs_shoot_point: 2B14F0`, `cs_jump: 2B1640` | Unsupported actor-command branch |

Names/signatures are admitted by [the converted script layout](../src/campaign/scripting/mcc_script_layout.inc), lines 176–250; actual authored use is recorded by [the campaign inventory](../analysis/campaign-ai-coop-20261020/inventory/campaign-v2.json). The limited [command dispatcher](../src/gameplay/simulation.cpp), lines 2515–2553, implements host goal/route behavior for `cs_go_to` and throws for remaining commands. Other unhandled builtins throw at line 3272. [The evaluator inventory](../analysis/campaign-ai-coop-20261020/mission-functions/evaluator-body-inventory-v1.json) gives original address provenance. Its 348 admitted rows without generated bodies are **not** a missing-feature count because handwritten owners are outside that count. `ai_actors` already has a lifted evaluator; do not classify its body as absent.

## World scheduling and shared physics

Campaign uses shared player/object/physics systems, so their remaining lifted execution also prevents a complete native campaign engine. [The live tick](../src/physics/physics_live_world_tu15.cpp), lines 3510–3542, enters `14FEC0` player/input scheduling, `1D0670` AI, `15EBA0` game-engine work, `13B6F0` PVS and `B8F40` object updates through the original-address ABI. `1CC8D0` already selects a typed native frame; its [binding](../src/physics/physics_live_world_tu15.cpp), lines 2640–2675, explicitly calls `run_actual` for remaining residuals.

Preserve the substantial native world/island, solver, forces, motion, vehicle-rate and other kernels. Remaining semantic conversion is visible in [world residual binding](../src/physics/native_tu15_world_integration_residuals.cpp): unhandled shape bounds at lines 151–155, sweep/pair consumption at 163–170, agent processing and contact motion at 188–197, and queue/listener/action/lifetime owners at 231–301. These are explicit residual calls, not a claim that every listed semantic operation is wholly absent. [Object-motion admission](../include/halo2/native_tu15_object_motion.hpp), lines 18–19 and 47–56, also identifies unported teleport placement `B95D0` and refused inactive-island/listener paths.

The latest located [v1462 physics matrix](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-producers-owners-original-v1462/matrix.json) has `passed: false`, `native_only_physics_closure: false` and `error: SystemExit(143)` during build/model qualification. It cannot supply completed integration proof for its pending refresh. Earlier successful multiplayer/native component comparisons remain bounded reference evidence, not campaign-wide native-only acceptance. Exact Xbox reciprocal-square-root/transcendental behavior also retains separately stated limits in [the physics audit](PHYSICS_NATIVE_KERNEL_MIGRATION.md), lines 58–86.

## Map/BSP lifecycle, checkpoints and mission progression

- **Map initialization:** whole `13A780` exists as a lifted body at [activation](../src/campaign/source_campaign_activation_tu15_generated.cpp), lines 1731–1755. [Current initialization](../src/physics/physics_live_world_tu15.cpp), lines 3593–3614 and 4153–4174, manually stages callbacks. [The static map-start inventory](../analysis/campaign-ai-coop-20261020/contrail-init/full-map-start-gate-v1.json) finds bodies for all 54 non-null callback entries but explicitly records `configure_calls_complete_original_driver: false`. Missing work is semantic conversion/full integration and readiness evidence, not 54 absent bodies.
- **BSP transitions:** [the current world](../src/physics/physics_live_world_tu15.cpp), lines 3616–3636, runs selected gameplay load/unload callbacks; [the callback table](../src/campaign/campaign_bsp_callbacks_tu15.hpp), lines 8–17, contains nine rows. Renderer notification after activation/restore exists at [campaign runtime](../src/campaign/campaign_source_runtime.cpp), lines 110–120 and 144–148. Full original lifecycle, authored transition/retention behavior and all-BSP qualification remain open.
- **Save admission:** [the script host](../src/gameplay/simulation.cpp), lines 2827–2832, checks an alive player and no pending revert/BSP request, explicitly leaving `226190` Havok/animation/effect danger checks to a separate service. `226190` already exists as a lifted [death/startup body](../src/gameplay/units/death_startup_runtime_generated.cpp), line 29142. Its campaign binding and semantic conversion remain.
- **State restoration:** [coupled world/pool commit](../src/campaign/campaign_source_runtime.cpp), lines 134–148, and [aggregate simulation/VM snapshots](../src/gameplay/simulation.cpp), lines 480–537, exist. Complete actor/path/command/HS/RNG/clock replay, repeated death/revert and cross-BSP acceptance remain unproved. An initial checkpoint is not proof of successful replay.
- **Mission completion/restart:** `game_won`/`map_reset` currently set host flags and suspend scripts at [simulation](../src/gameplay/simulation.cpp), lines 2836–2838. Full original completion/restart lifecycle remains unqualified. Frontend save/resume is explicitly unavailable at [frontend state](../src/ui/frontend_state.cpp), lines 1830–1832; this is a missing delivery path in addition to in-session checkpoint acceptance gaps.

## Cooperative campaign

Real campaign participant/session integration remains missing. [Campaign state](../include/halo2/campaign_source_runtime.hpp), lines 58–72, retains one player and checkpoint player. [Birth](../src/campaign/campaign_source_runtime.cpp), lines 175–205, allocates index 0/controller 0/local slot 0; controls at lines 345–374 target that player. [Frontend start](../src/ui/frontend_state.cpp), lines 1815–1829, launches solo mode 0 or multiplayer modes 1/2, while co-op mode 4 reaches unavailable handling; online co-op is explicitly unavailable at lines 1563–1567.

Required engine work is additional real player/biped birth, independent input, camera/HUD/first-person views, one shared script/AI world, participant-aware save/revert/BSP transitions and campaign network authority/transport/recovery. Multiplayer multi-slot APIs alone do not establish a campaign path.

Do not report the respawn scheduler as absent. The live tick enters lifted `14FEC0`, and [the frame-owner inventory](../analysis/campaign-ai-coop-20261020/coop-session/current-frame-owners-v1.json) records `14F8F0`, `CE650`, `1D2920`, `151DA0` and `151490` bodies while declaring `adapter_additional_participants_implemented: false` and `live_respawn_branch_qualified: false`. A separate semantic [co-op scheduler](../src/campaign/source_campaign_coop.cpp), lines 60–120, exists but is not the completed live campaign integration.

## Campaign audio, speech and cinematic presentation

**Campaign audio:** [the required group](../src/audio/native_tu15_sound_kernels_campaign_audio.cpp), lines 33–47, has 40 entry identities. [Playback](../src/audio/native_tu15_sound_kernels_campaign_playback.cpp), lines 796–809, provides 11 candidate routines; [mix](../src/audio/native_tu15_sound_kernels_campaign_mix.cpp), lines 1698–1699, provides five; [channels](../src/audio/native_tu15_sound_kernels_campaign_channels.cpp), line 6, is empty. Thus 24 group routines lack a native implementation. The group is withheld until complete (lines 61–62), and unconverted calls explicitly use lifted fallback (lines 54–56). This count describes source candidates, not 16 fully qualified shipped routines.

Missing campaign-group native identities:

```text
128650 129500 12AB30 12ADE0 12BC10 12BF00 12C5B0 12CBC0
12CCC0 2206D0 2207A0 224C40 225610 225890 225D70 226490
226C00 228310 228480 2B8650 2B8B10 2B8F80 2B92E0 2B9330
```

The [playback source's ownership notes](../src/audio/native_tu15_sound_kernels_campaign_playback.cpp), lines 2–15, cover driver updates, fades/ducking, playback start/retirement, listener gathering and channel/DSP routing. [Unit-speech native registration](../src/audio/native_tu15_sound_kernels_unit_speech.cpp), lines 1–6, is also empty; [live speech dispatch](../src/physics/physics_live_world_tu15.cpp), lines 2257–2258, enters the lifted module. `18F510` already exists in the native object-loop group, so not all speech-related code is missing. Its external dependencies remain visible in [object-loop frame](../src/audio/native_tu15_sound_kernels_object_loop_frame.cpp), lines 380–394. [The native sound goal](NATIVE_SOUND_ENGINE.md), lines 20–26, also retains removal of unreached lifted/hardware code from the production build.

**Cinematic control:** [live dispatch](../src/physics/physics_live_world_tu15.cpp), lines 2233–2255, still selects lifted cinematic, camera, screen, animation-composition and unit-cinematic owners. [Screen execution](../src/campaign/source_campaign_screen_tu15_generated.cpp), lines 882–897, and [cinematic execution](../src/campaign/source_campaign_cinematic_tu15_generated.cpp), lines 724–746, load/save virtual ABI state and dispatch original-address routines. Their native semantic replacements remain even where the authored opening already has successful component evidence.

**Concrete unsupported script presentation/spatial features:** object-relative animated cameras throw at [simulation](../src/gameplay/simulation.cpp), lines 2944–2948; attached `volume_return_objects` lacks a transform owner and original cluster-registration order remains separate at lines 3041–3054.

Preserve successful renderer work. [The Johnson CPU/GPU comparison](MCC_GPU_SKIN_CAMPAIGN_CAPTURE_20261023.md), lines 84–88, passes 60 ticks at physical 1080p with zero fallbacks and matching complete frames/ordered traces. That does not establish complete cinematic startup, all animation controllers or matched TU1.5 campaign presentation. Older repaired lens/storage failures should not be relisted as current missing kernels without reproduction.

## Implementation order and completion contract

Build generic native engine capabilities consumed by tags/scripts. Recover their original algorithms and contracts rather than substitute per-mission timing or behavior. Prioritize:

1. Recover authenticated semantics and native dependency closures for the actually reached `1BE5B0` and `1BAE40` owners; retain failure evidence and compare native results, memory publication order and dependency calls against original execution.
2. Complete native world/map/actor authority and HS scheduling/evaluation, then implement required generic mission builtins and command services. Recover the full callback/readiness lifecycle rather than add more independent host updates.
3. Convert the live lifted decision/frame/behavior closure and implement original navigation/search/hints/movement, preserving callback scheduling, RNG, pool ownership and floating-point behavior. Keep existing proven foundations.
4. Finish campaign audio/speech and cinematic/controller services, including the explicit unsupported camera/spatial paths. Use authored data to exercise the engine, not prerecorded results or mission-specific bypasses.
5. Integrate complete save-safety/replay/BSP/completion and real local/network co-op, with independent participant presentation and one shared world/script authority.
6. Qualify native kernels against independent TU1.5 evidence, then composed authored scenarios and complete missions. Record executed native/residual identities per feature; require no lifted/binary backend advancing gameplay on accepted paths. Compare visible behavior, timing, state, callbacks, ordered writes and numerical edge cases. Refused/skipped/unsupported branches remain open.

This is a source/evidence audit, not a newly executed validation run or an exhaustive call-graph proof. No engine source, runtime selection, imported content or test scratch was changed. Only this report and its user-facing copy/source-hash metadata were created; no large artifacts were generated.
