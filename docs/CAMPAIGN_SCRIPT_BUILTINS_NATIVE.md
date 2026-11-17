# Native campaign HS builtins (TU1.5)

Started 2026-11-11. User direction: wire the campaign's system-driven functions so
MCC-imported tags, triggers and scripts run as authored, and "make sure they're
all native code and not emulated". A builtin counts as native only when its effect
is computed by C++ on the live world bank. Calls into the TU1.5 world's original
code (`run_actual`, `call_actual`, generated `run_XXXX` units) do not count.

## Inventory (15 MCC campaign scenarios)

`halo2_native --inspect-systems` over every campaign map lists **381 distinct
builtins**. The campaign script host is `Simulation::host`
(`src/gameplay/simulation.cpp`), with VM intrinsics in
`src/campaign/scripting/scripts.cpp`. A static trace of each host branch gives:

| Class | Count | Notes |
| --- | --- | --- |
| VM intrinsics and natively hosted | ~165 | `begin`, `sleep_until`, `volume_test_*`, `list_*`, objectives, training, prediction, AI queries (K03/K09/K10) |
| Hosted through emulated TU1.5 code | ~40 | `ai_place`/`ai_erase` (`place_ai`), `sound_impulse_start` (scripted sound point), `object_teleport`, `object_set_function_variable`/`clear`, `object_set_permutation`, `object_set_scale`, `ai_allegiance`, the camera/cinematic family, `texture_camera_*`, `render_lights_enable_cinematic_shadow`, `unit_set_enterable_by_player` |
| No handler: thread faults with `unsupported builtin` | 175 before this work, **130 after** | 85 are AI/command-script builtins (`ai_*`, `cs_*`), which need a native AI owner |

## Batch 1: 38 builtins, native and parity-proven

37 builtins were previously unhandled; `random_range` was hosted but drew from
the wrong seed.

`include/halo2/native_campaign_script_builtins_tu15.hpp` and
`src/campaign/scripting/native_campaign_script_builtins_tu15.cpp` replicate each
original evaluator's tail after argument evaluation. They read the live world
through `SourcePhysicsContactMemory` and publish every store through the world's
journaled `write_source_call_input`, with the original store width and order.
`src/campaign/scripting/native_campaign_script_host_tu15.cpp` holds the dispatch
table: arity, declared argument types, scalar coercion, the player alias and
return types. `Simulation::host` consults it first whenever the campaign world is
attached.

| Builtins | Original |
| --- | --- |
| `pin`, `random_range`, `real_random_range` | 2AA9A0, 2AC6E0, 2AC760 |
| `garbage_collect_now` / `_unsafe` | 2ABD70 / 2ABD90 |
| `physics_set_gravity`, `physics_set_velocity_frame`, `physics_disable_character_ground_adhesion_forces` | 2AC840, 2AC890, 2AC8F0 |
| `player_effect_set_max_rotation`, `player_effect_start`, `player_effect_stop` | 2B49D0, 2B4A90, 2B4B10 |
| `pvs_set_object`, `pvs_clear` | 2B2B20, 2B2BD0 |
| `object_get_parent`, `object_set_phantom_power`, `object_set_deleted_when_deactivated`, `object_set_shield_stun_infinite`, `object_get_shield`, `object_clear_all_function_variables` | 2AB8E0, 2AB560, 2ABB30, 2AC370, 2AB7A0, 2AB4D0→10FD90 |
| `unit_kill`, `unit_kill_silent` (with B9390 update marking) | 2AD040, 2AD0B0 |
| `device_operates_automatically_set`, `device_closes_automatically_set`, `device_one_sided_set`, `device_set_never_appears_locked`, `device_group_change_only_once_more_set` | 10C640, 10C690, 2AE740, 2AE3D0, 10C6E0 (BCEE0 device iteration) |
| Device-group arguments (type 18) | 10C180: authored ordinal to full salted group handle (scenario `+0xA0` clamp, current salt, NONE for rows flagged deleted) |
| `unit_in_vehicle`, `unit_only_takes_damage_from_players_team`, `unit_doesnt_drop_items`, `unit_is_emitting`, `vehicle_driver` | 2ADA00, 11DB40, 11CD60, 11D040, 2ADF90 |
| `list_count_not_dead` | 1E7260 (members come from the VM list bank, stopping at NONE like 1E7130) |
| `interpolator_start` | 137BF0/137800 (returns the started interpolator index or -1) |
| `cheat_active_camouflage_by_player`, `time_code_show`, `time_code_start` | 2AC980: void with no state change |
| `game_save_no_timeout`, `game_save_immediate` | 2B39F0/2B3A30 differ from `game_save` only in request priority; routed to the native checkpoint owner, which commits any pending request at the tick boundary |

**RNG parity fix.** In campaign, `random_range` used to draw from
`Simulation::random_state_`. The original (2AC6E0) advances the primary seed at
`*[0x4F3CD8]`, the same stream `begin_random` already consumed. `random_range` and
`real_random_range` now share that seed, so checkpoints restore it with the world.

**Device groups.** The native VM keeps authored device-group ordinals. Original
evaluators such as 10C6E0 and 10C290 compare device `+0x130`/`+0x13C` against
the full salted handle. Every type-18 argument therefore goes through a native
10C180 before the kernel runs. The getter and immediate setter now use this
resolver (see "Device position scripts" below). The delayed `device_group_set`
path still reaches 10C180 through emulation.

**COMISS and DAZ.** COMISS honors MXCSR.DAZ, so `pin` compares a denormal bound
as a signed zero but returns the original operand bits. The oracle found this:
the case was `-0` pinned to `[denormal, -49.5]` with DAZ set.

**Floating point.** MULSS, ADDSS, CVTSI2SS, FISTP and the 2AC760 x87 sequence each
run in one `asm volatile` block that loads the world's live MXCSR or x87 control
word, performs the operation, and restores the host state. GCC's
`-frounding-math` can still move an inlined conversion across `fesetround`; that
produced a 1-ulp error under round-down, which the oracle caught. Non-x86 and MSVC
builds keep a software fallback.

### Proof

The validator now streams 2,000-case batches through two independent physical
observer lanes, then compares them in generator order. It keeps at most four
batches in flight and accumulates one comparison digest across the whole run.
`--shard K/N` selects every Nth batch without changing case generation or RNG
order. Existing track wrappers still run on each batch; adding, dropping or
reordering cases is rejected, including an in-place reorder.

CTest `native_campaign_script_streaming` checks out-of-order observer
completion, ordered digests, shard coverage, and failure propagation. A fresh
strict-write old/new control matched 5,172 physical cases, 10,344 comparisons,
217,895 stores, exact ordered input hash and final comparison digest. The old
report counted instructions from only its first internal observer chunk;
the streaming report correctly sums all chunks (391,645 versus 1,595,448 for
this control). That accounting correction is documented separately from the
identical semantic outputs in
`build/test-results/resume-mcc-streaming-20261112/paired-control.json`.
The unsharded full-cohort run completed successfully: **136,162 physical
cases, 272,324 native comparisons and 2,414,484 ordered stores**, over 385
kernels in 69 batches. `full.json` and `full-session.json` in that directory
pin that tested tree; they do not cover subsequent native additions. The
parent process peaked at 9,997,492 KiB RSS and the sampled process tree at
12,171,672 KiB. Its owned build and scratch were removed. Timings from the
paired run share a busy host and are not a controlled speedup measurement.

`tools/validate_native_campaign_script_builtins_tu15.py`
(CTest `original_tu15_native_campaign_script_builtins`) authenticates every
descriptor against `tools/mcc-script-layout.json` and the pinned XBE. For each
case it:

1. Enters the unchanged evaluator right after `212010`.
2. Executes under the existing ptrace observer, with the transitive callee closure
   as the authenticated sites.
3. Stops at the `211DA0` hs_return call.
4. Compares the result, every ordered store, and the final banks against the
   native harness on both the direct and journaled-writer paths.

Results with GCC `-O2` plus ASan/UBSan:

- 2,672 physical cases, 5,344 comparisons, 4,033 ordered original stores, zero
  differences. The cases include 48 standalone 10C180 cases.
- The cases include a deterministic grid of every FP kernel against eight
  x87/MXCSR controls (round-to-nearest/down/up/zero, precision 24/53/64, DAZ/FTZ)
  and denormal, tie, boundary, infinite and NaN operands.
- The FP kernels also pass when built with Clang.
- Nine targeted mutations of the shipped code are each rejected.
  - Five logic mutations: device-flag polarity, the `pin` comparison, the
    device-iteration type mask, list termination at NONE, and the B9390
    already-marked test.
  - Four FP mutations, each dropping one live control load: MULSS MXCSR,
    CVTSI2SS MXCSR, FISTP control word, and the 2AC760 x87 control word.
- The software fallback was mutation-tested before the asm path replaced it on
  x86 (tie rounding, DAZ, FTZ, x87 precision). It is not the shipped x86 path.

Known oracle artifact: x87 control-word bit 6 is reserved and reads back as 1, so
`0x023F` becomes `0x027F` after any x87 instruction. The fixture therefore uses
`0x027F`, which has the same precision and rounding.

`native_campaign_script_builtins_tu15` (CTest) runs the self-test, which needs no
original executable.

## Device position scripts (2026-11-13)

Four previously emulated campaign calls now dispatch through the native host:

| Builtin | Native owner |
| --- | --- |
| `device_get_position` | 2AE570 evaluator tail, device `+140`, zero for NONE |
| `device_group_get` | 2AE630 evaluator tail, group `+4` |
| `device_set_position_immediate` | 10C420, group update and device `+12C` bit 7 |
| `device_group_set_immediate` | 10C4E0, both power and position memberships |

Group arguments use the existing native 10C180 ordinal-to-salted-handle
resolver. Immediate changes clamp under the live MXCSR, preserve NaNs and
signed-zero/denormal operand bits where the original does, update every
matching device, clear its corresponding speed, and recursively refresh its
child hierarchy through native 10C5D0. Stores go through the retained world's
journaled writer, including the original repeated stores and B9390 wake order.
The group getter retains the original's unchecked resolved-handle contract.

The complete builtins harness also compiles the production host and verifies
authored group resolution with a high-bit salt, scalar conversion, both
memberships, the writer callback, FP controls and all four registrations.
Isolated factory-track builds retain their leaf-only harness.

Physical-original receipts and source pins are in
`build/test-results/resume-native-device-scripts-20261113/`. They cover the
four evaluator tails plus the existing group resolver and change-once setter,
with acyclic child/sibling hierarchies, mixed object types, empty memberships,
NONE cases where supported, and a grid of FP controls and edge values.
Both seeds passed: **3,840 original cases, 7,680 native comparisons and 29,864
ordered stores**. Six out-of-tree mutations were rejected: omitted clamping,
omitted child refresh, uncleared speed, ignored power membership, omitted
immediate-position flag and the wrong group getter field. The all-track
CMake harness and streaming tests passed. `closure.json` records the source
hashes and verified removal of both owned build/scratch trees; no large
artifacts were retained.
These are native-owner and host checks; a complete campaign mission and
launcher promotion are not established by them. Delayed `device_group_set`,
`device_set_position` and `device_set_power` retain their previous paths.

## Headless mission baseline (before batch 1)

Command: `halo2_native --map <mcc map> --simulate 900 --campaign`, launcher-selected
runtime `b7b64e92…`, cache `mcc-82114d7d…`.

| Mission | Script faults in 900 ticks |
| --- | --- |
| 03a Old Mombasa | `player_effect_set_max_rotation` x3, `player_effect_stop` (kills `mission_start`), `object_set_velocity`, `sound_impulse_start_effect` x2 |
| 08b Delta Control | `pvs_set_object` (`e7_scarab_main`), `cinematic_clone_players_weapon` (`mission_start`) |
| 04a Gas Giant | `interpolator_start` (kills `mission`), `ai_combat_status` |
| 07a High Charity | `interpolator_start` (`blurry_vision`) |
| 04b, 05b, 06b, 07b | `cinematic_clone_players_weapon` kills `mission_start`/`mission` |
| 03b New Mombasa | `objects_detach`; `custom_animation_relative_loop` hits a missing emulated owner (bbab0→cea70) |
| 05a Delta Approach | `sound_impulse_start_effect` x3 |
| 06a Sentinel Walls | external global 798 (`ai_current_actor`) |
| 00a Introduction | `cinematic_screen_effect_set_crossfade` needs its GPU consumer (expected when headless) |
| 01a Tutorial | stops at tick 10 on the AI owner gap `1B0AD9 -> 1BBEE0` (tracked in the engine checklist) |
| 01b Cairo | `Native memory pressure requires actual1CD130 trim owner` before tick 0 |
| 08a Delta Cliffs | clean |

### After batch 1

Same command with a kept-tree `halo2_native` built from this change. Both CTests
pass in that tree.

- Total faults: 23 → 18.
- 07a High Charity and 08a Delta Cliffs are clean.
- 04a Gas Giant's main `mission` thread no longer faults; only `ai_combat_status`
  remains.
- 03a Old Mombasa drops from 7 faults to 4. `mission_start` survives and the
  mission reaches further dialogue (`c03_1060_cor`), which then faults on
  `sound_impulse_start_effect`.
- 08b Delta Control's `e7_scarab_main` gets past `pvs_set_object` and now stops
  at `device_set_position_track`.
- 01a Tutorial and 01b Cairo stop exactly where they did before (AI owner gap,
  memory-pressure owner).

## Batch 2: emulated owners replaced, device animation added (2026-11-11)

These production wrappers previously ran original code through `run_actual` or
`call_actual`. They now call native kernels in
`src/campaign/scripting/native_campaign_script_presentation_tu15.cpp`, through
`native_campaign_script_bank(world)` (`native_campaign_script_world_tu15.cpp`).
Every kernel passes the same physical-original oracle as batch 1.

| Builtin (script calls) | Original owner | Native kernel |
| --- | --- | --- |
| `sound_impulse_start` scripted sound point (871) | BAD60 → BACD0, FC510, 1E11B0, 1E1200, 144910, 145560 | `object_get_markers` into a private scratch extent; no world writes |
| `unit_set_emotional_state` (210) | 111080 → 1110D0 | `unit_set_emotion` |
| `camera_control` (192) | 158560 → 158340, 111CD0, C9550, 158900 | `camera_control` |
| `object_set_function_variable` (173), function-value sampling | 10FC30, 10FB70 | `object_set_function_variable`, `object_function_value` |
| `cinematic_stop` (64; the stop half of start/stop) | 13F560 | `cinematic_stop` |
| `cinematic_screen_effect_set_depth_of_field` (64) | 1FC10 | `cinematic_depth_of_field` |
| `object_clear_function_variable` (26) | 10FD50 | `object_clear_function_variable` |
| `object_cinematic_lod` (561) | BDF70 | `object_cinematic_lod` |
| `camera_set_field_of_view` (105) | 16F5A0 | `camera_set_field_of_view` |
| `render_lights_enable_cinematic_shadow` (12) | 30E10 | `cinematic_shadow_control` |
| `texture_camera_set_object_marker` (9), `texture_camera_off` (7) | 255E0 (+ strncpy 32A4E0), 255C0 | `texture_camera_set_object_marker`, `texture_camera_off`, `original_strncpy` |
| `unit_set_enterable_by_player` (12) | 11DB00 | `unit_set_enterable_by_player` |
| New: `device_animate_position` (119), `device_animate_overlay` (27) | 10D6B0/10D720 → 10CB60 | `device_animate_position`, `device_animate_overlay` |

Oracle additions for standalone routines:

- Register, XMM and stack inputs.
- A third data bank plus absolute banks for the BSS globals these routines touch.
- Capstone mislabels x87 `FST`/`FSTP`/`FISTP` and `MOVHPS`/`MOVLPS` memory
  destinations as reads. The metadata now records them as writes, so they enter
  the ordered-store comparison.
- Case counts are larger for the copy routine and the marker and camera
  closures (400/400/200).

## Batch 3: spatial queries, view tests and AI references (2026-11-11)

Native kernels in `native_campaign_script_presentation_tu15.cpp` (spatial and
view) and the new `src/campaign/scripting/native_campaign_script_ai_tu15.cpp`
(AI), all through the same physical-original oracle.

| Builtin (script calls) | Original owner | Notes |
| --- | --- | --- |
| `volume_return_objects` / `_by_type` (139) | 11ED20 → 11E600 (BSP leaf 14CDB0), BD080 cluster flood (14D0E0/14CEA0/123210/243450) | Allocates first, then appends every trigger-test hit, through the VM list bank |
| `objects_can_see_flag` (138), `objects_can_see_object` | 2A9680 / 2A9560→2A94B0 → CE790 (BAD60 head marker, 30BD0 normalize, aim dot > FCOS) | x87 subtract/length/compare in single asm blocks |
| `ai_vehicle_get_from_starting_location` (715), `ai_vehicle_get` (218), `object_get_ai` (8) | 27C9A0, 27FB30 (+27C880), 27CD00 | `object_get_ai` returns the original `0xC3E703E7` null reference |
| `ai_spawn_count` (119), `ai_combat_status` (88) | 27FA80, 27FD60 | 20D070/20D0D0 group walk, 27CAA0/27CB30 actor walk |
| `ai_disposable` (292), `ai_set_blind` (29), `ai_set_deaf` (8), `ai_suppress_combat` (1) | 27D5B0, 27D6E0, 27D610, 27DC00 | Squad flag bits, actor bytes `+C`/`+223`/`+228`, prop `+3C` |
| `ai_disregard` (49), `ai_prefer_target` (1) | 27EE70, 27EF80 | Object `+134` bits `0x80`/`0x100` |
| `ai_magically_see_object` (46) | 27DA40 | Squad-mate perception rows: `+1C = 3`, `+1E = FISTP(ticks/s × 5.0)` |

`ai_current_actor` / `ai_current_squad` (external globals 798/797) and the AI
enumeration constants (805..821, e.g. `ai_combat_status_uninspected`) now read
and store their original backing (`0x50EF00`/`0x50EF04`, `.data` shorts at
`0x47C244`) in the world image. The VM publishes the command context around
each command-thread step like the original runner (262590 sets actor
`index | 0x80000000` and its `+30` squad; 26264B clears only the actor).
`ScriptVm::command_context` is the hook.

One documented divergence: 27DA40 keeps its visited-prop mask in a single
32-bit stack word, so prop indices of 32 or more alias the frame's float and
iterator temporaries (undefined in the original). The native kernel keeps the
intended full mask; behaviour is identical for indices below 32, which is
what the oracle exercises.

Oracle additions: a fourth bank for the actor (0x888), squad (0x98),
encounter-group (0x38), prop (0x50), knowledge (0xC4) and perception (0x3C)
data arrays plus scenario squad and starting-location rows; bounded switch
tables (`JUMP_TABLES`) and tail calls (27CA60 → 20D070) in the callee
closure. Mutation checks confirm the group walk, starting-location
resolution, actor salt check, single-actor path, AI-disabled path and the
perception refresh are each exercised.

## Batch 4: more AI, seats, physics velocity and player/HUD owners (2026-11-11)

All through the same physical-original oracle (the full run passes; per-kernel
mutation checks reject every seeded fault).

| Builtin / owner (script calls) | Original | Notes |
| --- | --- | --- |
| `ai_vitality_pinned` (10) | 27CFF0 | Model damage-info flag or object +10A bit 14, `0.0002 >= vitality` by COMISS, swarm chains |
| `ai_renew` (105) | 27F5C0 + 1ED690 | Vitality flags, character equipment rows (parent chain), primary RNG count that only raises +23E |
| `vehicle_test_seat` (32), `vehicle_test_seat_list` (220), `ai_vehicle_reserve` (89), `ai_vehicle_reserve_seat` (11) | 11D240, 11D310, 27FCD0, 27FBC0 over CAF20 | Recursive seat walk (children with +4 bit 26), BCEE0 occupant search |
| `object_set_velocity` (24) | 1100C0 | `native_tu15_object_set_velocity` in the shared motion module: local-frame velocity, then the native B9800 owners (B9770, 1CD6A0, wake). Wake test keeps 1100C0's `(z²+y²)+x²` order. A sleeping Havok island is a named refusal, never emulation |
| `weapon_hold_trigger` (45) | 107AD0 | Trigger row +4 bit 0x80, B9390 |
| `activate/deactivate_team_nav_point_object/flag` (52) | 25B6AF/25B77A → 25B5DA/25B709 | Player iterator (bitmap +48), per-user table at `[0x50EED0]`; FLD/FSTP quiets a signaling-NaN offset |
| `unit_set_active_camo` (2), `biped_ragdoll` (3), `units_set_maximum_vitality` (1) | 11CF10/D3340, 2B4280, 11C900 | Fade rate `1/(max(s,tick)*0.25)*0.25`, B7E50 network mark |
| `player_flashlight_on` (1), `ice_cream_flavor_available` (1) | 1518A0, 2B5F80 | The flavor evaluator returns the raw byte |
| Campaign player birth profile | 14E7B0 | Was `run_actual`; now native over a private record copy |
| Environment view leaf | 14CDB0 | Was `run_actual`; now the native `bsp_leaf` |

| `ai_set_active_camo` (32), `ai_berserk` (5) | 27DBC0→1EB590/D3340, 27F830→1EC900 | Per walked actor; B7E50 marks only network mode 4 |
| `flock_start`/`flock_stop` (2) | 2AFF60/2AFFD0 → 29F5B0 | Flock data array by bitmap, scenario `+354` names |
| `ai_magically_see` (39) | 27D7D0 (27C950, 20D590) | Knowledge `+1C` member of B, holder member of A; squad-in-group parent walk |
| `object_model_targets_destroyed` (16), `object_at_marker` (5) | DAE40, 10FF10 over BAD60 | Marker record in a private overlay; 10FF10's x87 `1.0 > sqrt((dz²+dx²)+dy²)` |

Also: `ai_combat_status_*`/`ai_movement_*` globals (805..821) return the
authenticated TU1.5 `.data` constants, because the world image does not map
`0x47C244`.

Sweep (15 missions, 900 ticks): 18 → 15 fault lines. Gas Giant, Sentinel
Walls, High Charity and Delta Cliffs now run clean. What is left is listed
below; every item needs a whole original subsystem.

## Audio effect stage: `sound_impulse_start_effect` (2026-11-11)

The builtin now plays natively. The path, each step taken from original code:

1. **Resolve the name (1903A0).** `native_campaign_script_tu15::sound_effect_reference`
   (oracle-proven against the original, 400 cases). `0x300012A` is the null
   effect; scenario `+0x324` rows give `0x80000000|row`, then the globals'
   collection (`[+0xC4]+0xC`) gives `0x40000000|row`.
   `sound_effect_collection` returns the collection tag. Unknown names and the
   null effect play with the sound's own playback, like 18FE80.
2. **Carry it to the mixer.** `Simulation` handles the builtin through the same
   path as `sound_impulse_start` (both call 18CAC0). `SoundEvent` carries
   `effect_collection`/`effect_row`; `main.cpp` passes them to
   `MultiplayerAudio::schedule_impulse(_at)`. The scene `Cache` is now a
   `shared_ptr`, so the scripted audio retains it.
3. **Select the instance (2290C0/224740).**
   `MultiplayerAudio::Core::effect_parameters` takes the row's first DSP
   instance. Its template must be `sfx_mono_distortion`, which 1974B0 maps to
   mixbin 13 (original string ID `0x1300012D`; MCC uses `0x13000131`, so the
   match is by resolved name). It converts the instance's override through
   `CompiledMonoDefinition` (the 228CD0 → 385B4F conversion) under CW `027F`.
   Rows without a DSP instance play dry. Other templates, and
   function-driven overrides (`radio_3d`), report an `Audio fault` instead of
   playing a guess.
4. **Route through the DSP (SdlSoundPlatform).**
   - The voice keeps its dry routes and adds a 0 dB mono send to mixbin 13
     (2290C0 sets voice `+0x90`/flag `0x200`, 128CA2 copies it to the driver
     request `+0x12`, and 2B96B0 appends the auxiliary bin at 0 dB).
   - Creating the voice installs its record-1 parameters.
   - The bus runs native DSP record 1 (`tu15_sound_mono_distortion::process`)
     in place, in 32-sample Word24 blocks, under the traced linear,
     non-saturating controls. It starts from `authored_initial_parameters()`
     and zeroed state, as `GraphStorage::fresh_literal` does.
   - The 1973E0 → 2275A0 mix-in voice returns mixbin 13 to speakers 0–5 at
     0 dB, so the front pair gets it at unity, one block later.
   - The bus is inactive until an effect voice exists, so other output is
     unchanged. Checkpoints carry its state.

Checks:
- `sdl_sound_platform_tests` adds `effect_send_bus`. Output without an effect
  voice is exactly dry. With one, it equals dry plus an independent record-1
  run of the send, block-delayed.
- A hidden Delta Approach run with dummy audio now plays every effect line:
  45 script audio packets against 33 before, and no `Audio fault`.

Still open in this stage:
- The effect row's filter block (row `+0x18`: DLS2/parametric EQ per voice,
  e.g. `radio_default`'s 1000–1600 Hz, +13 dB). This is the Xbox per-voice
  hardware filter, set through DirectSound `SetFilter`. Its evaluation lives in
  the hardware, not the XBE, and the native mixer does not model it for any
  sound yet. `radio_default` has no DSP instance in MCC, so it plays dry.
- Pitch and filter LFOs, also row data.
- `radio_3d` parameter functions (time, range and speaker inputs).

## Status of the remaining items (2026-11-11)

Each remaining item depends on an original subsystem that is still emulated as a
whole. These are full ports, not single kernels: each call tree exceeds about
24,000 original instructions across hundreds of functions with indirect
(vtable) dispatch.

1. `cinematic_clone_players_weapon` (2B5E60→13ED30): object construction
   (B7B40 and per-type constructors) plus attach (10FFD0). No native object
   factory exists; `object_creation_request` stops at the request (B8820).
   Every campaign creation path (scripted `object_create`, `ai_place`, item
   and weapon births) still reaches `call_actual(0xb9b70)` in
   `physics_live_world_tu15.cpp`, so a native clone needs that factory first.
   Measured scope (2026-11-11). B9B70's static callee closure is 2,208
   functions and 180,399 instructions. An instruction census
   (`HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1 HALO2_INSTRUCTION_CENSUS=...`)
   over 300 ticks of 04b with real campaign births shows the executed slice:
   699 functions and 36,525 instructions. B9B70 ran 549 of its 614
   instructions, 428 times. The heaviest executed owners are 1E3420/1E3F30
   (about 690 each), 1D46A0, 1CB8E0, 31A260, BF0C0, 1DE170, BB3E0 and 1D5320.
   This is a dedicated native-replacement project, best ported per object
   type behind refusals, with the weapon path that the clone needs first.
2. `device_set_position_track` (10CF80): animation-graph sampling
   (1D4200/1D4090) and the node rebuild BF050. `device_animate_*` are now
   native. `device_set_overlay_track` (10D5E0, 2.3k instructions,
   animation-graph lookup 10D870) is next.
3. Done (see "Audio effect stage" above), apart from the per-voice hardware
   filter and LFOs. Original notes: `sound_impulse_start_effect` (2B3C30 → 18CAC0 with the effect name): the
   name resolves through 1903A0 (`0x300012A` is the null effect
   `0xC0000000`; else scenario `+0x324` rows of 0x38 give `0x80000000|i`,
   then globals `+0xC4` collection `0x40000000|i`). The effect index rides on
   the playing sound into 18CDD0/18C130 and the title DSP effect graph.
   Campaign maps use `radio_default_in/loop/out` rows: radio-static in/loop/out
   sounds plus a distortion filter. Native DSP for the filter exists
   (`native_tu15_sound_mono_distortion`, `mcc_mono_distortion_definition`), but
   `MultiplayerAudio`/`SdlSoundPlatform` voices have no effect stage. This needs
   an audio-engine feature: effect resolution, auxiliary in/loop/out playback,
   and per-voice DSP.
4. `objects_detach` (BBAC0): object placement and Havok rebuild owners
   BB910 (24k instructions, 287 virtual calls), B92C0, BA630 and BA8C0.
   `object_set_velocity` is done (batch 4).
   Engine-level stops in the same class: Tutorial's 1BBEE0 (55,692 static
   instructions) and Delta Approach's 29D840, reached at tick 388 in a
   hidden-window run through AI root 1D0670 (39,126).
5. AI family: 22 of the 85 are native (batches 3 and 4). The rest:
   - Unit actions: `vehicle_unload`, `ai_vehicle_exit`, `unit_exit_vehicle`
     request action 0x1D/0x20 through the 473D28 table; the handlers
     (ED110, 28k instructions; ED970, 32k) belong to the unit action system.
   - Command-script setters (`cs_*`, about 40): they write the command-script
     datum at `[0x50EEE0]` row `[0x50EEE8]`. `cs_run_command_script`
     (2801D0) also creates an original HS thread (2117E0/211A80) that the
     actor runner 262590 executes through 211840. The native VM owns script
     threads, so original command-script datums can only exist once the
     actor update itself is native; until then these stay on the Simulation
     command path. Tracked in A04 of the AI checklist.
   - Whole-subsystem owners: `ai_place`, `ai_migrate`, `ai_play_line*`,
     `ai_scene`, `ai_vehicle_enter*`, `ai_erase_all`, `cs_custom_animation`,
     `cs_teleport` (thousands of instructions with indirect dispatch).
6. Remaining emulated hosted builtins:
   - Animation pipeline: `custom_animation*`, `scenery_animation_start*`,
     `objects_attach`, `object_set_scale` refresh, `camera_set`,
     `camera_set_animation_relative`, and the two animation-time queries
     (resource prefetch 1E5C80).
   - Effects: `effect_new_on_object_marker`.
   - Physics: `object_teleport`.
   - AI: `ai_place`/`ai_erase`, `ai_get_object`/`ai_get_unit`, `ai_allegiance`
     (27E000, 23k instructions).
   - Cinematics: `cinematic_start` (13EAD0).
   - Permutations: `object_set_permutation` (BC440).

## Earlier next-target notes (from the baseline, by impact)

1. `cinematic_clone_players_weapon` (2B5E60→13ED30). Kills the main thread on 5
   missions. It reads the first local player's alive unit's primary and secondary
   weapons (`unit+0x212/0x213` into `+0x218`), falls back to scenario starting
   profiles, then creates and attaches a weapon object. It needs a native object
   factory and attach path.
2. `device_set_position_track` (2AE8B0, 111 calls) is now the first blocker on the 08b Scarab thread. Then `sound_impulse_start_effect` (2B3C30). Same `18CAC0` owner as
   `sound_impulse_start`, but with a sound-effect string id where the plain call
   passes -1. `SoundEvent` has no effect field yet. Routing it to the plain impulse
   would drop the effect (for example a radio filter), so the effect has to be
   carried through native sound delivery first.
3. `objects_detach` (BBAC0) and `object_set_velocity` (1100C0). Both touch physics
   and hierarchy owners (`C0F60`, `BB8C0`/`BB910`, `1CD6A0`) that still run as
   emulated code.
4. AI family (85 builtins). Needs the native actor owner; see
   [the AI checklist](CAMPAIGN_AI_COOP_GOAL_CHECKLIST.md).
5. Convert the ~40 emulated-but-hosted builtins listed above. Simple candidates:
   `object_set_function_variable`/`clear` (10FC30/10FD50), `ai_allegiance`, and
   `object_cinematic_lod`.

Other remaining non-AI builtins:

- High use: `effect_new` (564), `vehicle_test_seat_list` (220) and
  `vehicle_test_seat` (32) (they use CAF20 seat enumeration),
  `sound_impulse_trigger` (146), `volume_return_objects_by_type` (139),
  `objects_can_see_flag` (138), `device_animate_position` (119),
  `device_set_position_track` (111), `object_create_clone` (91).
- Damage: `damage_new`, `damage_object`, `damage_players`,
  `object_damage_damage_section`.
- Navigation points: `activate_team_nav_point_*`, `deactivate_team_nav_point_*`.
- Units: `unit_add_equipment`, `unit_exit_vehicle`, `unit_stop_custom_animation`,
  `unit_set_active_camo`, `units_set_current_vitality`,
  `units_set_maximum_vitality`.
- Low use: `object_dynamic_simulation_disable` (1CC310 physics), `weapon_hold_trigger`,
  `object_model_targets_destroyed`, `object_at_marker`, `object_set_region_state`,
  prediction variants, `biped_ragdoll`, `volume_teleport_players_not_inside`,
  `player_flashlight_on`, `ice_cream_flavor_*`, `play_credits`, `bink_time`.
