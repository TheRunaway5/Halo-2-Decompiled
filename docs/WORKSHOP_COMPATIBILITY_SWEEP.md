# Installed Halo 2 Workshop compatibility sweep

The normal launcher selection has subsequently advanced to runtime
`de12ea8c5ba846db8e7280ee11165ee8` through concurrent physics/water work.
The installed-mod results below are the latest complete Workshop sweep of
`7d8dac7632ae45459da0efe3e94f2e49`; they do not automatically qualify the
newer runtime. Its complete14593-file production inventory is independently
verified and preserved for the next coherent Workshop build. The existing
source archive's embedded JSON names its older ancestor, so preparation uses
the independently verified selected-package inventory and the whole-archive
hash instead. No archive or launcher is replaced by this verification.
[Selected-source preservation](../build/test-results/workshop-goal-20261101/source-archive-selected-de12.json).

November1 current result: delivered runtime`7d8dac7632ae45459da0efe3e94f2e49` passes all84
required CTests,14 stock-map/local-player frame cases and four frontend cases.
Its full installed sweep covers10 packages and101 map files: all96 declared
missions prepare and inspect,35 complete at least90 native ticks,35 submit four
preview frames, and none submits four gameplay frames. All source/resource,
per-map scratch, executable, frozen-source and3283-module shader checks pass.
Five undeclared resource maps remain explicit. The stricter final-tick rule
rejects early clean exits. No mission is qualified as fully playable.

Compared with the previous published Workshop runtime7a93, every passing
inspection, simulation and preview is preserved. The complete six-file
physics cohort from the intervening selected32273 runtime is also preserved;
its stock-map/player cases pass in the new qualification. These comparisons
cover the recorded cohorts and do not qualify complete missions.

| Installed package | Declared missions | Native inspection | At least90 ticks | Four gameplay frames | Four preview frames |
| --- | ---: | ---: | ---: | ---: | ---: |
| DIGSITE - ALPHA MOON | 1 | 1 | 0 | 0 | 1 |
| E3 2003 EARTH CITY | 3 | 3 | 0 | 0 | 3 |
| Earth Ark | 1 | 1 | 0 | 0 | 0 |
| Halo2 E3 2004 Multiplayer Demo | 1 | 1 | 0 | 0 | 0 |
| Halo2 Forge | 1 | 1 | 0 | 0 | 0 |
| Halo2 Grifball | 1 | 1 | 0 | 0 | 0 |
| Halo2 Reformation | 40 | 40 | 25 | 0 | 16 |
| Halo2 Restored Lighting | 13 | 13 | 0 | 0 | 8 |
| Rampancy Map Pack | 3 | 3 | 0 | 0 | 0 |
| Digtacular Collection | 32 | 32 | 10 | 0 | 7 |

The native contact/filter work now includes the complete collection filter,
wrapped and transformed factories, and Headlong's reversed transformed factory.
All five installed Reformation controls—containment, headlong, elongation, dune
and coagulation—complete91 ticks for90-tick requests. Physical-original,
compiler, fault/unwind, mutation and sanitizer proofs remain separate from
mission acceptance. The actual SMG/Warthog replay and original loop-audio trace
also pass the complete current qualification.

The completed Workshop8 publication selected both root launchers to
`7d8dac7632ae45459da0efe3e94f2e49`, preserving the selected32273 physics
cohort and prior networking/water changes. All84 required CTests,14
packaged stock cases and four frontend cases pass without waivers. The exact
packaged v9 installed sweep retains all results in the current table with no
passing-check losses. After private build removal, both installed launchers
report that owner and render the actual Mods screen. Real key input on an
isolated X11 display also follows Mods → E3 → Play Campaign → difficulty →
asynchronous preparation, visually verifies the24-feature unavailable dialog,
then returns through Continue → Back → Explore Map. It renders322 native
preview frames and exits cleanly.
All input/selector hashes remain unchanged; temporary profiles, prepared data
and captures are removed. These are menu/preview delivery results; main E3
still has24 requirements and no mod passes the gameplay-render gate.
[Guarded publication](../build/test-results/workshop-goal-20261101/publication-candidate-8.json),
[actual installed E3 handoff](../build/test-results/workshop-goal-20261101/candidate8-root-e3-flow.json),
[both installed launcher routes](../build/test-results/workshop-goal-20261101/candidate8-root-launcher-routes.json).

The current inspection reports58 distinct script requirements and179 shader
templates with recorded gaps across this cohort. Main E3 retains24 requirements;
its first reported simulation requirement is`scenery_animation_start`, while
the sorted unavailable dialog lists`bitmap_predict` first. The complete native
cast table and authenticated MCC-only`rasterizer_blur` contract are delivered.

The main E3 campaign and its low-spec variant have the same24 remaining script
requirements in delivered build7d8dac76. These are unqualified native runtime
contracts; an existing host handler or passing isolated component does not
remove the mission gate.

| Area | Remaining E3 script requirements |
| --- | --- |
| Resource prediction | `bitmap_predict`, `shader_predict`, `sound_looping_predict` |
| Cameras and titles | `camera_set_animation`, `cinematic_set_title_delayed`, `pvs_set_camera`, `unit_set_prefer_tight_camera_track` |
| Animation | `scenery_animation_start`, `unit_custom_animation_at_frame` |
| AI command scripts | `cs_die`, `cs_formation`, `cs_go_by`, `cs_play_sound`, `cs_set_style`, `cs_vehicle_speed_instantaneous` |
| Unit control and input | `player_action_test_jump`, `unit_set_actively_controlled` |
| Vehicles | `unit_exit_vehicle`, `vehicle_count_bipeds_killed`, `vehicle_hover` |
| Sound timing | `sound_impulse_time` |
| Damage, objects and inspection | `damage_objects`, `object_set_shadowless`, `inspect` |

The complete58-name installed-cohort list, with the number of affected maps
for each requirement, is the `script_requirements` object in the
[current sweep summary](../build/test-results/workshop-goal-20261101/installed-candidate-4-summary.json).
The other34 requirements include movie playback, navigation breadcrumbs,
position-distance queries, additional AI and unit/input commands, save and
loading presentation, object operations, rendering and audio controls.

Alpha Moon first needs delayed
title scheduling. All27 maps referencing `cinematic_start_movie`, the breadcrumb
and distance helpers, missing authored starts, incomplete multiplayer LevelData,
materials, combat/AI, audio playback and mission/co-op/network flows remain
explicit work. Static readers do not qualify those effects. Two unit/input
native evaluators and reset now pass150 original/physical-reference component
cases and60 complete original HS dispatcher/parser/completion compositions; they remain standalone
until World/input consumers and MCC admission are qualified.

The installed cohort is not the full MCC capability set. The authenticated
registry contains918 builtin declarations; the current410-entry profile is
metadata admission, not proof of410 complete effects.508 declarations,
remaining globals/value types and broader content behavior still need native
qualification. Root's separate eight-argument metadata-capacity fix passes
GCC/Clang/sanitizer checks but is not in the delivered candidate8 binary and
admits neither of the affected commands.

[Exact packaged sweep](../build/test-results/workshop-goal-20261101/installed-candidate-4/runtime/sweep.json),
[strict per-package totals, every requirement/template gap and comparisons](../build/test-results/workshop-goal-20261101/installed-candidate-4-summary.json),
[package/source integrity](../build/test-results/workshop-goal-20261101/installed-candidate-2/candidate-route.json),
[selected-source preservation](../build/test-results/workshop-goal-20261101/selected-preservation-refresh.json),
[current goal checklist](MCC_GAMEPLAY_GOAL_CHECKLIST.md).

The following baseline and earlier published-build sections retain their
historical results; they do not supersede the current table above.

Baseline runtime: `9f29a0ffd4a6951358f86293cbc5e40a8246cb9ecd8b13cdad7526df81eaea9b`. All101 map files in10 installed Halo2 packages are inventoried. Production preparation is attempted for every declared mission. Native checks are90 simulation ticks, four Original gameplay frames and four preview frames; these do not qualify a complete mission.

Follow-up fixes have been qualified and published from a frozen cohort. The
first table preserves the original baseline, including every failure. Current gates
and evidence are maintained in
[MCC_GAMEPLAY_GOAL_CHECKLIST.md](MCC_GAMEPLAY_GOAL_CHECKLIST.md).

The verified follow-up component changes are:

- E3's low-spec physical filename may retain the main mission's internal
  scenario name. Actual preparation/reuse and four Original preview frames pass.
- Empty named placements return `NONE` through the script creation factory,
  matching48 unchanged original C1120/D75A0 cases. The ScriptVm/Simulation/SceneObjects
  regression fails before the change and passes afterward.
- Digtacular, Reformation and Rampancy retain their own matching shared maps
  and DAT banks. Their sampled native inspections decode5487 bitmap frames,
  6814 first audio permutations and153 facial blocks without a resource error.
  This is resource-reader acceptance, not mission gameplay acceptance.
- Forge's Warthog child bank is deduplicated into the golden Warthog's resident
  allocation. The content reader now follows that checked cache owner while
  literal tag-payload views retain their original boundary. The installed-map
  regression creates the authored vehicle attachments and passes.

The expanded native-resource sweep is complete: all96 declared missions now
prepare, all201 BSP content readers succeed, and native readers accept203463
bitmap frames,270794 first audio permutations and76215 facial blocks with zero
resource errors. Source map/resource hashes remain unchanged and all per-map
scratch owners are removed. The five undeclared files are three Reformation
resource maps plus Digtacular's martian/overboard caches. Shader/script metadata
gaps remain explicit; resource decoding does not qualify their engine effects.
[Complete resource sweep](../build/test-results/workshop-complete-20261031/installed-resources-1/sweep.json).

The new runtime is selected by both root launchers after62 required CTests,
13 packaged stock-map/player frame smokes and four frontend startup captures
pass without waivers. The installed Mods catalog also renders after removal of
the private build. Full mission branches, checkpoints, mission transitions, authored
multiplayer settings, combat, cinematics, audio playback and network content
agreement remain separate acceptance gates.

| Package | Map files | Native checks failed | Preparation failed | Not declared |
| --- | ---: | ---: | ---: | ---: |
| DIGSITE - ALPHA MOON | 1 | 1 | 0 | 0 |
| E3 2003 - "EARTH CITY" DEMO | 3 | 2 | 1 | 0 |
| Earth Ark | 1 | 1 | 0 | 0 |
| Halo 2 E3 2004 Multiplayer Demo | 1 | 1 | 0 | 0 |
| Halo 2 Forge | 1 | 1 | 0 | 0 |
| Halo 2 Grifball | 1 | 1 | 0 | 0 |
| Halo 2 Reformation | 43 | 15 | 25 | 3 |
| Halo 2 Restored Lighting | 13 | 13 | 0 | 0 |
| Rampancy Map Pack - Halo 2's 20th Anniversary Celebration | 3 | 0 | 3 | 0 |
| The Digtacular Collection | 34 | 0 | 32 | 2 |

## Published-build runtime sweep

Runtime SHA256: 199b52313e5d2aa430c2a025836a846bd3abbddce27487b7eaae3375528663c1. All96 declared missions prepare and inspect;35 pass the bounded simulation check and19 submit four preview frames. No mission submits the required four gameplay frames. Each runtime/source hash stays unchanged and per-map scratch is removed. All96 retain failed checks; no mission or package is qualified as fully playable.

| Package | Declared missions | Simulation passes | Gameplay frame passes | Preview frame passes |
| --- | ---: | ---: | ---: | ---: |
| DIGSITE - ALPHA MOON | 1 | 0 | 0 | 0 |
| E3 2003 - "EARTH CITY" DEMO | 3 | 0 | 0 | 3 |
| Earth Ark | 1 | 0 | 0 | 0 |
| Halo 2 E3 2004 Multiplayer Demo | 1 | 0 | 0 | 0 |
| Halo 2 Forge | 1 | 0 | 0 | 0 |
| Halo 2 Grifball | 1 | 0 | 0 | 0 |
| Halo 2 Reformation | 40 | 25 | 0 | 9 |
| Halo 2 Restored Lighting | 13 | 0 | 0 | 4 |
| Rampancy Map Pack - Halo 2's 20th Anniversary Celebration | 3 | 0 | 0 | 0 |
| The Digtacular Collection | 32 | 10 | 0 | 3 |

[Complete per-map commands/results](../build/test-results/workshop-complete-20261031/installed-runtime-1/sweep.json), [summary and all failure categories](../build/test-results/workshop-complete-20261031/installed-runtime-summary.json), [actual reached sky shaders](../build/test-results/workshop-complete-20261031/observed-sky-materials.json).

The largest reached rendering blocker is the sky kernel guard (77 previews). Other actual failures include unresolved BSP shader LOD passes, missing/native-null unit backing, missing player starts, source prop-tag ownership, invalid navigation edge cycles, unregistered AI callbacks and MCC script owners. These are retained individually across68 observed failure categories; the60-declaration inventory also covers script branches hidden behind earlier failures.

## Unpublished sky-candidate runtime sweep

Candidate SHA256: `a90f4efc6dd4425fe6e87e02920654fde707f9070e202aaf5e21ca9d0ff73d50`.
All101 files in10 packages are inventoried and all96 declared missions prepare
and inspect. Source map/resource hashes and the runtime stay unchanged; every
per-map scratch owner and the temporary shader fallback link are removed.
The3283-module shader bank exactly matches the candidate's qualification input.
No resources are reauthored and no renderer or script failure is waived.

| Package | Declared missions | Simulation passes | Gameplay frame passes | Preview frame passes |
| --- | ---: | ---: | ---: | ---: |
| DIGSITE - ALPHA MOON | 1 | 0 | 0 | 1 |
| E3 2003 - "EARTH CITY" DEMO | 3 | 0 | 0 | 3 |
| Earth Ark | 1 | 0 | 0 | 0 |
| Halo 2 E3 2004 Multiplayer Demo | 1 | 0 | 0 | 0 |
| Halo 2 Forge | 1 | 0 | 0 | 0 |
| Halo 2 Grifball | 1 | 0 | 0 | 0 |
| Halo 2 Reformation | 40 | 21 | 0 | 16 |
| Halo 2 Restored Lighting | 13 | 0 | 0 | 8 |
| Rampancy Map Pack | 3 | 0 | 0 | 0 |
| The Digtacular Collection | 32 | 10 | 0 | 7 |
| **Total** | **96** | **31** | **0** | **35** |

Previews gain16 and lose none against the earlier published Workshop build.
Four Reformation simulations regress: containment, elongation and headlong
reach the shape-class dispatch guard; dune reaches the contact-manager cleanup
profile guard. The stock SMG/Warthog replay also fails that cleanup guard,
leaving66/67 qualification checks passing and this candidate unpublished.
There are61 remaining sky-guard preview failures and every mission retains at
least one failed check. These bounded probes do not qualify full missions.
[Per-map evidence](../build/test-results/workshop-goal-20261101/installed-candidate-1/runtime/sweep.json),
[Comparison and all reached errors](../build/test-results/workshop-goal-20261101/installed-candidate-summary.json),
[Exact shader package and route cleanup](../build/test-results/workshop-goal-20261101/installed-candidate-1/candidate-route.json).

The following targeted private cohort adds the original-validated type10 getter
and the independently qualified absent-base-lock release. Dune and the
coagulation control each complete91 ticks for a90-tick request. Containment and
headlong now identify factory2F81F0; elongation identifies2F1CC0. Their complete
original constructors are inventoried for implementation. These targeted
results do not replace the pinned all-installed table above or qualify delivery.
The five prepared-map copies and temporary shader staging have been removed;
the active source/build remains owned for this continuing diagnosis.
[Targeted probes](../build/test-results/workshop-goal-20261101/shape-class-live-3.json),
[Finished probe cleanup](../build/test-results/workshop-goal-20261101/completed-probe-cleanup.json).

## Mission metadata beyond map loading

All96 declared mission descriptors were inspected. The Mods bridge currently
reads the scenario filename, title, description and campaign/multiplayer type.
It does not route the following authored fields:

| Descriptor feature | Installed coverage | Remaining acceptance |
| --- | ---: | --- |
| Default primary weapon and Forge default | 56 multiplayer maps | Honor the authored damage-reporting identity through native weapon selection. Grifball requests a gravity hammer; E32004 requests a battle rifle. A coincidentally matching tag default is insufficient. |
| Maximum teams by game category | 56 multiplayer maps | Enforce each category's authored team limit in the lobby and native session. |
| Mission images | 96 missions | Use the correct authored loading/thumbnail images with their package owner. |
| Campaign metagame/scoring fields | 40 campaign missions | Explicit native scoring policy and displayed results. |
| Saved-film flags | 96 missions | Every installed descriptor disables saved films; no Workshop film-policy acceptance exists. |
| Insertion points | 96 empty lists | No installed fixture supplies a selectable insertion point; support remains unqualified. |
| Custom font table/atlas | AlphaMoon | Its modified Conduit9 file and table are dropped by preparation. Native font authentication currently accepts a finite stock set, so a custom font needs an explicit owner and glyph/layout acceptance. |

[Exact descriptor inventory](../build/test-results/workshop-complete-20261031/mission-metadata-feature-inventory.json).

The E3 authoring references also identify custom dynamic-light/shadow shader
variants. They need their own native Vulkan behavior checks; this is an
additional coverage requirement, not a diagnosis of the observed sky failure.
The author distinguishes the E3 and retail shadow variants in the
[custom-shader README](https://github.com/777Sev777/Halo-2-Custom-Shaders/blob/main/README.md),
and the [Digsite authoring README](https://github.com/digsite/H2E3/blob/main/README.md)
links that implementation. No external tagset or shader binary was copied into
the runtime during this sweep.

The submitted-frame requests use Original graphics and muted audio with one
local player. Path Tracing, split-screen/co-op, authored team/game variants,
audio playback, network/content agreement and later mission branches remain
unqualified for this installed-mod cohort.

The private source/build, per-map preparation trees, sweep owners and menu
captures are removed, including9030787627 regular bytes from candidate8's
completed build owner. The delivered runtime/resources retain3197763388 logical
bytes; its required exact source archive retains78641876 bytes. Required
original fixtures, bounded reports, existing assets, prior runtimes and
unrelated work are preserved.
[Cleanup receipt](../build/test-results/workshop-goal-20261101/candidate8-storage-cleanup.json),
[exact source reference](../build/test-results/workshop-goal-20261101/source-archive-candidate-8.json).

## Missing declaration inventory

This table preserves the original60-declaration baseline. The delivered
`rasterizer_blur` contract is now qualified and excluded from the current58
distinct requirements. A matching TU1.5 signature is a candidate only. Some
functions already have partial host paths; their complete native behavior
still needs proof.

| Function | MCC opcode | Signature-matching TU1.5 candidates |
| --- | ---: | ---: |
| `!=` | 14 | 1 |
| `inspect` | 23 | 1 |
| `damage_objects` | 44 | 1 |
| `object_destroy_all` | 54 | 1 |
| `object_set_shadowless` | 58 | 1 |
| `object_function_set` | 59 | 1 |
| `object_wake_physics` | 65 | 1 |
| `object_model_target_destroyed` | 82 | 1 |
| `objects_distance_to_position` | 116 | 0 |
| `shader_predict` | 119 | 1 |
| `bitmap_predict` | 120 | 1 |
| `scenery_animation_start` | 185 | 1 |
| `unit_custom_animation_at_frame` | 207 | 1 |
| `unit_set_actively_controlled` | 211 | 1 |
| `unit_enter_vehicle` | 217 | 1 |
| `unit_set_prefer_tight_camera_track` | 226 | 1 |
| `unit_exit_vehicle` | 227 | 1 |
| `cheat_active_camouflage` | 275 | 1 |
| `ai_enable` | 279 | 1 |
| `ai_attach` | 287 | 1 |
| `ai_overcomes_oversteer` | 296 | 0 |
| `ai_allegiance_remove` | 312 | 1 |
| `ai_vehicle_exit` | 350 | 1 |
| `cs_go_by` | 389 | 1 |
| `cs_play_sound` | 415 | 1 |
| `cs_die` | 420 | 1 |
| `cs_formation` | 429 | 1 |
| `cs_vehicle_speed_instantaneous` | 438 | 1 |
| `cs_set_style` | 448 | 1 |
| `camera_set_animation` | 454 | 1 |
| `camera_pan` | 464 | 1 |
| `pvs_set_camera` | 473 | 1 |
| `player_action_test_jump` | 482 | 1 |
| `player_action_test_rotate_weapons` | 487 | 1 |
| `player_action_test_back` | 500 | 1 |
| `cinematic_set_title_delayed` | 563 | 1 |
| `game_save_unsafe` | 575 | 1 |
| `sound_impulse_time` | 602 | 1 |
| `sound_impulse_time` | 603 | 1 |
| `sound_impulse_start_3d` | 606 | 1 |
| `sound_looping_predict` | 607 | 1 |
| `sound_looping_stop_immediately` | 610 | 1 |
| `vehicle_hover` | 625 | 1 |
| `vehicle_count_bipeds_killed` | 626 | 1 |
| `activate_nav_point_flag` | 638 | 1 |
| `deactivate_nav_point_flag` | 642 | 1 |
| `breadcrumbs_activate_team_nav_point_position` | 646 | 0 |
| `breadcrumbs_activate_team_nav_point_flag` | 647 | 0 |
| `breadcrumbs_activate_team_nav_point_object` | 648 | 0 |
| `breadcrumbs_deactivate_team_nav_point` | 649 | 0 |
| `breadcrumbs_deactivate_team_nav_point_flag` | 650 | 0 |
| `breadcrumbs_deactivate_team_nav_point_object` | 651 | 0 |
| `player_effect_set_max_translation` | 659 | 1 |
| `rasterizer_blur` | 691 | 0 |
| `ui_transition_out_console_window` | 729 | 1 |
| `rasterizer_bloom_override_reset` | 870 | 1 |
| `loading_screen_fade_to_white` | 890 | 0 |
| `enable_render_light_suppressor` | 893 | 0 |
| `disable_render_light_suppressor` | 894 | 0 |
| `cinematic_start_movie` | 895 | 0 |

## Native failures

The examples below preserve the original baseline diagnostics. Current
per-map failures are in the
[candidate8 sweep](../build/test-results/workshop-goal-20261101/installed-candidate-4/runtime/sweep.json).

Per-map commands, output, shader names, missing native passes and all BSP content results are retained in the linked JSON detail reports. Shader metadata gaps include potential specialized/non-drawn materials; a gap alone is not proof that a gameplay draw reaches that material.

### DIGSITE - ALPHA MOON

- [alphamoon.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3366014540-22e07b4c0b6af90a.json): simulation: error: MCC script requires an explicit native owner: cinematic_set_title_delayed at syntax datum 3948939245; gameplay_render: error: MCC script requires an explicit native owner: cinematic_set_title_delayed at syntax datum 3948939245; preview_render: error: Sky material has no exact native Vulkan kernel

### E3 2003 - "EARTH CITY" DEMO

- [earthcity_e3.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3360515088-71aac34b4d70f8f6.json): simulation: error: MCC script requires an explicit native owner: rasterizer_blur at syntax datum 3881632746; gameplay_render: error: MCC script requires an explicit native owner: rasterizer_blur at syntax datum 3881632746
- [earthcity_e3_potato.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3360515088-03769cd3e0eb2d81.json): Workshop: map scenario differs from its mission metadata
- [earthcity_playground.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3360515088-64eb1a9eb26cd62d.json): simulation: Script fault: field_hospital_setup: builtin 'object_create_anew_containing': object placement has no definition; gameplay_render: error: Campaign cinematic model shader is unresolved: scenarios\shaders\human\urban\glass\window_skylight

### Earth Ark

- [earthark.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3724459012-dc9cc19c8563d569.json): simulation: error: BSP has no authored player start for simulation; gameplay_render: error: BSP has no authored player start for simulation; preview_render: error: Sky material has no exact native Vulkan kernel

### Halo 2 E3 2004 Multiplayer Demo

- [zanzibar.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3422469230-d26074974aee08a7.json): simulation: error: MCC script requires an explicit native owner: ui_transition_out_console_window at syntax datum 3872064344; gameplay_render: error: MCC script requires an explicit native owner: ui_transition_out_console_window at syntax datum 3872064344; preview_render: error: Sky material has no exact native Vulkan kernel

### Halo 2 Forge

- [mombasa_battle.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3577874470-e6efb1956ba8cd30.json): simulation: error: Source tag pointer precedes owned payload; gameplay_render: error: Source tag pointer precedes owned payload; preview_render: error: Sky material has no exact native Vulkan kernel

### Halo 2 Grifball

- [grifball.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/2997083897-3dc116a3c8e5e87a.json): simulation: error: TU1.5 live actual root 11ef70 at 11f07b: Required original runtime backing unavailable at0x14 [gp 0 90000000 1629 0 600effbc 0 90036ac8 600effec] [stack 0 0 0 600effec 14 c 1 1]; gameplay_render: error: TU1.5 live actual root 11ef70 at 11f07b: Required original runtime backing unavailable at0x14 [gp 0 90000000 1629 0 600effbc 0 90036ac8 600effec] [stack 0 0 0 600effec 14 c 1 1]; preview_render: error: Sky material has no exact native Vulkan kernel

### Halo 2 Reformation

- [00a_introduction.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-804c03846881f6cd.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [01a_tutorial.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-948f8fa421d05cb9.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [01b_spacestation.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-791a26f79837a6ce.json): simulation: error: MCC facial geometry block header/footer differs from its declared resource; gameplay_render: error: MCC facial geometry block header/footer differs from its declared resource; preview_render: error: MCC file extent is invalid or excessive
- [03a_oldmombasa.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-d5872f34f278268a.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [03b_newmombasa.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-8268b80b774e21b0.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [04a_gasgiant.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-ec6109ffc3ff7599.json): simulation: error: MCC facial geometry block header/footer differs from its declared resource; gameplay_render: error: MCC facial geometry block header/footer differs from its declared resource; preview_render: error: MCC file extent is invalid or excessive
- [04b_floodlab.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-9002bf039bba8d91.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [05a_deltaapproach.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-1f985b3a03296def.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [05b_deltatowers.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-9af90c5f98eda513.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [06a_sentinelwalls.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-966c88f0c6f6ba8d.json): simulation: error: MCC facial geometry block header/footer differs from its declared resource; gameplay_render: error: MCC facial geometry block header/footer differs from its declared resource; preview_render: error: MCC file extent is invalid or excessive
- [06b_floodzone.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-268cd4f9535b8c83.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [07a_highcharity.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-b0f69618f02c6924.json): simulation: error: MCC facial geometry block header/footer differs from its declared resource; gameplay_render: error: MCC facial geometry block header/footer differs from its declared resource; preview_render: error: Invalid MCC texture segment count
- [07b_forerunnership.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-fd2b069337ff445d.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [08a_deltacliffs.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-47baa38e9422002f.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [08b_deltacontrol.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-45104c9f43b896b6.json): simulation: error: MCC facial resource escapes its actual English DAT container; gameplay_render: error: MCC facial resource escapes its actual English DAT container; preview_render: error: MCC file extent is invalid or excessive
- [ascension.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-9bcc977ace86d59f.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [backwash.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-aa3f32f4b962981f.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [beavercreek.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-8eca6de26185b916.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [burial_mounds.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-d4b5ba2039b3203b.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [coagulation.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-23274333c9332eed.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [colossus.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-a6dfbc4431d71713.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [containment.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-f3451b0d5a57274e.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [cyclotron.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-ab1f3ca808fdc7ef.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [deltatap.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-a75924ba5a4e0d64.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [derelict.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-08f2757a557a62b5.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [dune.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-9984b44bc4a46f87.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [elongation.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-6175ebbfbe15b754.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [foundation.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-40c079ed8042b93f.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [gemini.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-5782308210f9db7f.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [headlong.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-66dddb98d299cfbd.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [highplains.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-061536308faf39f8.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [lockout.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-6c23fb3a1e773b33.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [mainmenu.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-5d60d9e7008be7e8.json): Map is inventoried statically but cannot be selected through the production Mods menu
- [midship.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-d3ced0b52f98448e.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [needle.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-2cac1c56f816f361.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [shared.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-a557224c20cb686e.json): Map is inventoried statically but cannot be selected through the production Mods menu
- [single_player_shared.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-0fa502d56735cbc8.json): Map is inventoried statically but cannot be selected through the production Mods menu
- [street_sweeper.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-33bec1591941f918.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [triplicate.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-aa83968954ea2c48.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [turf.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-41f4c1277fd79cfb.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [warlock.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-42492310cb0b8fbb.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [waterworks.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-f185f8da8aaa4611.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [zanzibar.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3361280895-d26074974aee08a7.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet

### Halo 2 Restored Lighting

- [01b_spacestation.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-791a26f79837a6ce.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3928688312; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3928688312; preview_render: error: Sky material has no exact native Vulkan kernel
- [03a_oldmombasa.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-d5872f34f278268a.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3925214851; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3925214851; preview_render: error: Sky material has no exact native Vulkan kernel
- [03b_newmombasa.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-8268b80b774e21b0.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3899131125; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3899131125
- [04a_gasgiant.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-ec6109ffc3ff7599.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3894215850; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3894215850; preview_render: error: Sky material has no exact native Vulkan kernel
- [04b_floodlab.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-9002bf039bba8d91.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3889759334; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3889759334; preview_render: error: Sky material has no exact native Vulkan kernel
- [05a_deltaapproach.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-1f985b3a03296def.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3881960431; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3881960431; preview_render: error: Sky material has no exact native Vulkan kernel
- [05b_deltatowers.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-9af90c5f98eda513.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3896902867; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3896902867; preview_render: error: Sky material has no exact native Vulkan kernel
- [06a_sentinelwalls.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-966c88f0c6f6ba8d.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3898541292; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3898541292
- [06b_floodzone.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-268cd4f9535b8c83.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3895264442; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3895264442; preview_render: error: Sky material has no exact native Vulkan kernel
- [07a_highcharity.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-b0f69618f02c6924.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3892184203; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3892184203
- [07b_forerunnership.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-fd2b069337ff445d.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3884319763; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3884319763; preview_render: error: Sky material has no exact native Vulkan kernel
- [08a_deltacliffs.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-47baa38e9422002f.json): simulation: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3879011266; gameplay_render: error: MCC script requires an explicit native owner: cinematic_start_movie at syntax datum 3879011266; preview_render: error: Sky material has no exact native Vulkan kernel
- [08b_deltacontrol.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3743458113-45104c9f43b896b6.json): simulation: error: MCC script requires an explicit native owner: breadcrumbs_activate_team_nav_point_object at syntax datum 3973319009; gameplay_render: error: MCC script requires an explicit native owner: breadcrumbs_activate_team_nav_point_object at syntax datum 3973319009

### Rampancy Map Pack - Halo 2's 20th Anniversary Celebration

- [abyss.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3164362918-4f58d4f4f0181f7c.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [amberaction.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3164362918-9a046b2c2465fe77.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet
- [jetty.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3164362918-80985557c19fe17c.json): Workshop: inherited tag metadata needs a per-resource owner and is not supported yet

### The Digtacular Collection

- [03_ascension.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-27b9b56f933a6cb7.json): Workshop: mods replacing shared resource files need additional native support
- [03_burial_mounds.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-61f6f90db58e1c7f.json): Workshop: mods replacing shared resource files need additional native support
- [03_colossus.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-06749569ab6a8f22.json): Workshop: mods replacing shared resource files need additional native support
- [03_headlong.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-5ef095fd5411d83e.json): Workshop: mods replacing shared resource files need additional native support
- [03_lockout.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-1e5512c6811d0662.json): Workshop: mods replacing shared resource files need additional native support
- [03_midship.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-149337f91de28e82.json): Workshop: mods replacing shared resource files need additional native support
- [03_waterworks.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-9c414d4d4e7d45ec.json): Workshop: mods replacing shared resource files need additional native support
- [anchor_point.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-68511753aa2748d9.json): Workshop: mods replacing shared resource files need additional native support
- [arcology.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-fa3222280050e45c.json): Workshop: mods replacing shared resource files need additional native support
- [arcology_playground.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-62f4a72af8001570.json): Workshop: mods replacing shared resource files need additional native support
- [artery.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-f99a2da51b02fe0f.json): Workshop: mods replacing shared resource files need additional native support
- [bloodgulch_test.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-e3b535981da08c86.json): Workshop: mods replacing shared resource files need additional native support
- [cloud_city.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-c2594ea9dee48211.json): Workshop: mods replacing shared resource files need additional native support
- [convergence.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-b7f16421dd41bdcc.json): Workshop: mods replacing shared resource files need additional native support
- [convict.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-25d1f46dc4a7059a.json): Workshop: mods replacing shared resource files need additional native support
- [evergreen.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-27c1bdd43398897a.json): Workshop: mods replacing shared resource files need additional native support
- [example.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-c096be60e208a5e3.json): Workshop: mods replacing shared resource files need additional native support
- [forerunner_sample.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-b5662415b0d3c883.json): Workshop: mods replacing shared resource files need additional native support
- [fourth_station.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-c32d51ca053a0a7c.json): Workshop: mods replacing shared resource files need additional native support
- [gauntlet.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-29857a9bcbedc28c.json): Workshop: mods replacing shared resource files need additional native support
- [grotto.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-bdc21668801f9125.json): Workshop: mods replacing shared resource files need additional native support
- [h2defense.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-d0d58011863804d8.json): Workshop: mods replacing shared resource files need additional native support
- [human_sample.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-10c401591968a2ea.json): Workshop: mods replacing shared resource files need additional native support
- [m1_assault.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-8c313b8cc9eac020.json): Workshop: mods replacing shared resource files need additional native support
- [martian.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-108c6e0b042ad06c.json): Map is inventoried statically but cannot be selected through the production Mods menu
- [missile_beach.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-b7f75de7657c71eb.json): Workshop: mods replacing shared resource files need additional native support
- [moonbase.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-94501064c6d04832.json): Workshop: mods replacing shared resource files need additional native support
- [overboard.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-789dbda1d336782a.json): Map is inventoried statically but cannot be selected through the production Mods menu
- [permafrost.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-f25e912e542ea205.json): Workshop: mods replacing shared resource files need additional native support
- [rodentia.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-2c483dde1e55a8df.json): Workshop: mods replacing shared resource files need additional native support
- [shaft.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-a4074935a7d4a028.json): Workshop: mods replacing shared resource files need additional native support
- [solitude.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-de081f07af7e79d6.json): Workshop: mods replacing shared resource files need additional native support
- [sunken_chamber.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-cf79f7d0b178921a.json): Workshop: mods replacing shared resource files need additional native support
- [timberland.map](/home/eric/Developer/Halo-2-Decompiled/build/test-results/workshop-complete-20261031/installed-baseline/maps/3000901245-39949be8293976b1.json): Workshop: mods replacing shared resource files need additional native support

