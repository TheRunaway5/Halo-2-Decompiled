# Remaining Translated Execution Checklist (2026-11-19)

This document is the actionable `.MD` checklist for agents continuing the Halo 2 TU1.5 translated-to-native replacement effort. See [REMAINING_TRANSLATED_20261115.md](REMAINING_TRANSLATED_20261115.md) for the historical progress log across earlier waves.

## November 20 particle motion and hierarchy update

The x86-64 Release performance binary `630d7fe83b6f432f900cfa815cb1f07038c941435e26787df8121784850d6074` runs the TU1.5 `2C2AD0` particle motion owner and binds the existing `BBD10` hierarchy kernel in live support. The particle kernel preserves the authored jump table, gravity and drag calculations, particle traversal, and the original child calls. The component comparison exercised 199 original instruction sites across 8,000 cases per mode, including 1,143 injected interruptions. All nine compiled mutations were detected.

| Workload | Owner | Translated callbacks with reference switches | Translated callbacks with native defaults |
|---|---|---:|---:|
| Coagulation, four players, 600 ticks | `2C2AD0` | 11,524,444 | 0 |
| Coagulation, four players, 600 ticks | `BBD10` | 1,144,899 | 0 |
| Armory, 300 frames | `2C2AD0` | 23,861 | 0 |
| Armory, 300 frames | `BBD10` | 129,029 | 0 |

The reference runs used the same binary with `HALO2_REFERENCE_EFFECT_OWNERS=2c2ad0` and `HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT=1`. The latter restores the translated walk in every bound unit. Both workloads matched their reference frames, and Armory matched its reference trace. Coagulation verification replayed 16,093 particle motion calls. The largest remaining translated routine there was `1770C0` (17,919 callbacks per tick), followed among the unconverted routines by `1772C0` (10,776 per tick). The largest in Armory was `30E930` (4,750 per frame). Component and live results are in `build/test-results/particle-motion-20261120/`.

Nine scripted 600-tick multiplayer cases across eight maps matched their translated reference captures byte for byte. Four-player Lockout reached tick 600 on all four runs. Its two translated-reference frames differed from each other, as did its two native-verification frames. Both native runs matched their per-call reference replay. Lockout frame parity remains open. The first comparison and repeat evidence are in `live/summary.json` and `repeats/summary.json` under the same results directory.

---

## 1. Current Baseline & Summary of Completed Work (as of 2026-11-19)

- **Shared Performance Binary**: `build-perf-4k60/halo2_native` (`5b7ab5790a0de084b8ad8d36d451bf691237d1dc5e93c605132379eecccbaff2`)
- **Armory Reference Frame / Trace Hashes**: `5627a51badf47762` / `8a9b58b38bef` (300 frames, Original mode, `1280x720`)
- **Translated Instruction Census Progression (Armory 01a, `HALO2_CENSUS_NATIVE_BRIDGES=1`)**:
  - **2026-11-18 Reference Baseline**: `343,892.0` translated instructions/frame
  - **2026-11-19 Reference (15 recent switches set to `=1`)**: `205,891.0` translated instructions/frame (`61,767,151` total)
  - **2026-11-19 Native Default**: **`123,204.3` translated instructions/frame** (`36,961,290` total — **-64.2% vs 343.9k**, **-40.2% vs 205.9k**)
  - Raw census comparison: `build/test-results/shadow-frame-20261119/census/compare.txt`

### Recently Completed, Proven & Bound Owners (2026-11-18 to 2026-11-19)

| Entry | Group | Name / Role | Systems & Adapter Headers | Reference Switch | Proof Report | Census Delta (Armory) |
|---|---|---|---|---|---|---|
| `1362A0` | S | Render-object submission | `src/rendering/native_tu15_object_submit_systems.hpp` + `native_tu15_object_submit.hpp` | `HALO2_REFERENCE_OBJECT_SUBMIT` | `build/test-results/shadow-frame-20261119/object_submit.json` (963 physical cases, 180/180 sites, 16/16 mutants) | `6,706.5` -> **`0.0/frame`** |
| `134D30` | S | Cluster contents collector | `src/rendering/native_tu15_cluster_contents_systems.hpp` + `native_tu15_cluster_contents.hpp` | `HALO2_REFERENCE_CLUSTER_CONTENTS` | `build/test-results/shadow-frame-20261118/cluster-contents-sanitizers.json` | `5,686.5` -> **`0.0/frame`** |
| `4BAE0` | S | Object distance fade & LOD | `src/rendering/native_tu15_object_fade_systems.hpp` + `native_tu15_object_fade.hpp` | `HALO2_REFERENCE_OBJECT_FADE` | `build/test-results/shadow-frame-20261118/object_fade_sanitizers.json` (296 physical cases, 127/127 sites, 15/15 mutants) | `3,652.1` -> **`0.0/frame`** |
| `251A60` / `250450` | E | Particle location & instance update | `src/rendering/effects/native_tu15_particle_update_systems.hpp` + `native_tu15_particle_update.hpp` | `HALO2_REFERENCE_EFFECT_OWNERS` | `build/test-results/effect-owners-20261118/validator.json` (60/60 + 59/59 branches, 153/153 mutants) | `2,758.2` -> **`0.0/frame`** |
| `153BA0` | F | Player action builder (`153BA0`/`BD080`/`1540D0`/`153F20`/`154270`/`1541B0`) | `src/physics/runtime/native_tu15_player_action_systems.hpp` + `native_tu15_player_action.hpp` | `HALO2_REFERENCE_PLAYER_ACTION` | `build/test-results/frame-misc-20261118/final-153ba0.json` (8,000 cases, 19/19 branches, 0 surviving mutants) | `2,415.6` -> **`0.0/frame`** |
| `1365C0` | S | Render-model hierarchy & layer splitter (self-recursive) | `src/rendering/native_tu15_model_hierarchy_systems.hpp` + `native_tu15_model_hierarchy.hpp` | `HALO2_REFERENCE_MODEL_HIERARCHY` | `build/test-results/shadow-frame-20261119/model_hierarchy.json` (833 physical cases, 265/265 sites, 16/16 mutants) | `2,376.6` -> **`0.0/frame`** |
| `E8D80` | H | Vehicle update (`physics_vehicle_update_generated.cpp`) | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `2,204.2` -> **`0.0/frame`** |
| `136D50` | S | PRT lighting partition | `src/rendering/native_tu15_prt_partition_systems.hpp` + `native_tu15_prt_partition.hpp` | `HALO2_REFERENCE_PRT_PARTITION` | `build/test-results/shadow-frame-20261118/prt_partition_sanitizers.json` (220 physical cases, 65/65 sites, 13/13 mutants) | `1,845.0` -> **`0.0/frame`** |
| `C3AB0` | S/E | Light volume shape builder | `src/rendering/native_tu15_light_shape_systems.hpp` + `native_tu15_light_shape.hpp` | `HALO2_REFERENCE_LIGHT_SHAPE` | `build/test-results/shadow-frame-20261119/light_shape.json` (829 physical cases, 194/194 sites, 13/13 mutants) | `1,683.4` -> **`1.7/frame`** |
| `1D7260` | H | Havok broadphase / world leaf | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `1,494.3` -> **`0.0/frame`** |
| `135E00` | S | Object draw flags | `src/rendering/native_tu15_object_draw_flags_systems.hpp` + `native_tu15_object_draw_flags.hpp` | `HALO2_REFERENCE_OBJECT_DRAW_FLAGS` | `build/test-results/shadow-frame-20261119/object_draw_flags.json` (147 physical cases, 47/47 sites, 14/14 mutants) | `1,011.5` -> **`0.0/frame`** |
| `1373F0` | S | Render-view queue reset | `src/rendering/native_tu15_view_reset_systems.hpp` + `native_tu15_view_reset.hpp` | `HALO2_REFERENCE_VIEW_RESET` | `build/test-results/shadow-frame-20261119/view_reset.json` (156 physical cases, 35/35 sites, 10/10 mutants) | `707.5` -> **`0.0/frame`** |
| `136230` | S | Render-object submission loop | `src/rendering/native_tu15_object_pass_systems.hpp` + `native_tu15_object_pass.hpp` | `HALO2_REFERENCE_OBJECT_PASS` | `build/test-results/shadow-frame-20261119/object_pass.json` (338 physical cases, 42/42 sites, 12/12 mutants) | `605.7` -> **`0.0/frame`** |
| `131610` | F | Campaign environment fog mixers (`131610`/`137AC0`) | `src/physics/runtime/native_tu15_fog_mixers_systems.hpp` + `native_tu15_fog_mixers.hpp` | `HALO2_REFERENCE_FOG_MIXERS` | `build/test-results/frame-misc-20261118/final-131610.json` (8,000 cases, 26/26 branches, 30/30 mutants) | `561.0` -> **`0.0/frame`** |
| `BBD10` | S/L | Object hierarchy root walk (bound in `source_shadow_frame` & `source_ai_frame`) | `src/rendering/native_tu15_object_hierarchy_root_systems.hpp` + `native_tu15_object_hierarchy_root.hpp` | `HALO2_REFERENCE_OBJECT_HIERARCHY_ROOT` | `build/test-results/shadow-frame-20261119/object_hierarchy_root.json` (138 physical cases, 19/19 sites, 11/11 mutants) | `430.1` -> **`259.1/frame`** (`0.0` in shadow/AI) |
| `135690` | S | Render-view visibility setup | `src/rendering/native_tu15_view_setup_systems.hpp` + `native_tu15_view_setup.hpp` | `HALO2_REFERENCE_VIEW_SETUP` | `build/test-results/shadow-frame-20261119/view_setup.json` (902 physical cases, 120/120 sites, 12/12 mutants) | `396.0` -> **`0.0/frame`** |
| `2E4760` | H | Rotation into world (`2E0C70` canonical) | `src/physics/runtime/native_tu15_world_leaves_systems.hpp` + `native_tu15_world_leaves.hpp` | `HALO2_REFERENCE_PHYSICS_WORLD_LEAVES` | `build/test-results/world-leaves-20261118/validator.json` | `215.7` -> **`0.0/frame`** |

---

## 2. Mandatory Guardrails for Every Agent

### A. 64 GB RAM Memory Cap (Strict User Requirement)
The host has **64 GB of RAM** and will OOM-crash if multiple heavy C++ builds, sanitizer harnesses, or live game instances run in parallel.
1. **Run at most ONE heavy build, validator, or live check at a time.** Never launch concurrent `g++`/`cmake` compilations or parallel `halo2_native` instances.
2. **Cap parallel jobs**:
   - Shared perf binary build: `JOBS=4 tools/build_perf.sh`
   - Launcher engine build: `python3 tools/build_launcher_engine.py --jobs 2` (or `./Halo2-Launcher --build-engine`)
   - Validator mutation screens: `--jobs 2`
   - Live multiplayer sweep: `--workers 1` (`tools/perf_4k60/live_check.py sweep ... --workers 1`)
3. **Enforce `RLIMIT_AS` (1 GiB)** and ordered-store vector caps in every test/mutant harness so a bad pointer or infinite loop in a mutant cannot exhaust host RAM.

### B. Test Storage Lifecycle (`AGENTS.md` / `docs/TEST_STORAGE.md`)
- Use `tools/run_tests.py` for CMake validation targets.
- Use `tools/run_test.py` (with `--scratch-root .local/test-scratch` for live game runs) for one-off compiler, probe, oracle, or live-check scripts, writing temporary outputs under `$HALO2_TEST_SCRATCH`.
- Verify all temporary `/tmp/halo2-test-*` and `.local/test-scratch/halo2-test-*` directories are removed before finishing your turn.

### C. Whole-File SHA256 Pins (`tools/export_native_tu15_physics_callable_data.py`)
**CRITICAL**: `tools/export_native_tu15_physics_callable_data.py` pins the whole-file SHA256 of these **7 canonical `.cpp` files**:
1. `src/physics/runtime/physics_live_support_tu15_generated.cpp`
2. `src/physics/runtime/physics_world_runtime_generated.cpp`
3. `src/physics/runtime/physics_contact_runtime_generated.cpp`
4. `src/physics/vehicle/physics_vehicle_motion_math_generated.cpp`
5. `src/world/source_map_world_support_tu15_generated.cpp`
6. `src/campaign/death_cycle_runtime_tu15_generated.cpp`
7. `src/combat/source_combat_action_tu15_generated.cpp`

- **NEVER modify those 7 `.cpp` files directly in `src/`!** Doing so breaks `native_tu15_physics_callable_data_export` in CTest.
- Instead, bind native replacements into those 7 units **at build time** in their specialized copies via `spatial_owner_bindings` in `tools/specialize_native_operands.py` (using `tools/native_live_support_leaves_replacements.py`, `tools/native_world_leaves_replacements.py`, `tools/native_frame_misc_replacements.py`, or `tools/native_effect_owner_replacements.py`).
- Other generated units (such as `source_shadow_frame_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`, `native_light_shape_tu15_generated.cpp`, `source_effect_particle_draw_tu15_generated.cpp`) are **not** whole-file pinned by `export_native_tu15_physics_callable_data.py`; their prefixes are installed directly in `src/` and registered in `tools/native_source_prefixes.py` (verified by `native_source_membership_tests`).

### D. Code Standard & Proof Requirements
1. **Systems Logic (`*_systems.hpp`)**: Named structs (`static_assert(offsetof(...))`), named bitflags/constants, clean control flow, and an `enum class Access` naming every memory access role. No raw hex offsets, register arrays (`gp[]`), or processor flags in the systems header.
2. **Thin Boundary Adapter (`*.hpp`)**: Maps `Access` roles to exact TU1.5 instruction PCs, register updates, stack pushes/pops, EFLAGS, and x87/SSE state. Includes `HALO2_REFERENCE_*` switch and `HALO2_CENSUS_NATIVE_BRIDGES=1` uncursored admission (`native_census_detail::uncursored`).
3. **Proof**:
   - Physical i386 single-stepped oracle (or 8,000-case randomized oracle + boundary/NaN/denormal corpus) covering 100% of reachable owned instructions/branches.
   - Out-of-tree semantic mutation screen (`0` surviving mutants) + guard mutants (`uncertified-required-owner`, `census-without-switch`).
   - ASan + UBSan clean.
   - Live Armory parity (`5627a51badf47762` / `8a9b58b38bef`), live verify (`HALO2_VERIFY_*=1`), Armory census drop, and 8-map MP verify sweep.

---

## 3. Prioritized Checklist of Remaining Translated Routines by Group

Per-frame instruction counts below come directly from the 2026-11-19 Armory census (`build/test-results/shadow-frame-20261119/census/compare.txt`, `123,204/frame` total).

### Group H — Havok Physics World & Vehicle/Arena Runtime

> **Binding mechanism**: `physics_world_runtime_generated.cpp` is whole-file SHA256 pinned; bind via `tools/native_world_leaves_replacements.py` (or a sibling specializer binder in `tools/specialize_native_operands.py`). Note that `physics_world_runtime_generated.cpp` uses canonical (disc) function names (`run_<disc>`) and `ObservedWorldState` with `selected(0x..., s.image)`.

- [ ] **`30E930`** (`4,750/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Currently the #1 largest translated routine in the Armory census. Havok narrowphase/solver hot path. Check canonical entry address via `selected(..., s.image)` mapping in `physics_world_runtime_generated.cpp`.
- [ ] **`125280`** (`3,489/frame`) — `physics_game_arena_runtime_generated.cpp`
  - **Notes**: Load-time / initial-tick arena table initializer (~1M instructions amortized over 300 frames). Pure integer/table loop.
- [ ] **`1D9EC0`** (`1,645/frame`) — `physics_phantom_world_tu15_generated.cpp`
  - **Notes**: Havok phantom AABB / broadphase overlap update.
- [ ] **`2E2E30`** (`1,319/frame`) — `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`
  - **Notes**: Multi-unit Havok transform/constraint helper; bind across both owning units.
- [ ] **`1F6610`** (`1,070/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Havok collision agent / shape query helper in `physics_world_runtime_generated.cpp`.
- [ ] **`1F7000`** (`780/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Pairs with already-native `1F76A0` / `1F7C10` / `1F6D10` in `native_tu15_world_leaves_systems.hpp`.
- [ ] **`315C90`** (`722/frame`) — `physics_phantom_force_tu15_generated.cpp`, `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`
  - **Notes**: Phantom force / impulse accumulator; shared across 3 units.
- [ ] **`3077F0`** (`720/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`30CD00`** (`711/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Adjacent to already-native `30CF40` (`10,203 calls` in Armory verify).
- [ ] **`2E5DB0`** (`706/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`233250`** (`634/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`2E44D0`** (`520/frame`) — `physics_vehicle_environment_state_tu15_generated.cpp`, `physics_world_runtime_generated.cpp`, `source_collision_...`
  - **Notes**: Note comment in `tools/native_world_leaves_replacements.py` line 25 mentioning `2E44D0`/`2E46E0` explicit-destination packed SSE (`ordered_mul_ps`, `ordered_add_ps`, `ordered_sub_ps`).
- [ ] **`B9770`** (`491/frame`) — `physics_vehicle_update_generated.cpp`
  - **Notes**: Sibling of `E8D80` (`2,204/frame` -> `0.0/frame`) in `physics_vehicle_update_generated.cpp`.
- [ ] **`1D64C0`** (`447/frame`) — `physics_vehicle_motion_generated.cpp`, `physics_world_runtime_generated.cpp`
- [ ] **`1D8B20`** (`427/frame`) — `physics_world_runtime_generated.cpp`
- [ ] **`2E4F30`** (`418/frame`) — `physics_world_runtime_generated.cpp`
  - **Notes**: Adjacent to already-native `2E4FE0` (`9,571 calls` in Armory verify).
- [ ] **`2E3250`** (`407/frame`) — `physics_world_runtime_generated.cpp`, `source_scene_device_constructor_tu15_generated.cpp`

---

### Group F — Frame & Gameplay Misc

> **Binding mechanism**: Specializer binding via `tools/native_frame_misc_replacements.py` (invoked by `tools/specialize_native_operands.py`). `1306B0`, `D7230`, `1EEE80`, `41970`, `133A70`, `153BA0`, and `131610` are already bound there!

- [ ] **`131AC0` (+ `133B40` + `1342B0`)** (`1,962/frame`) — `source_campaign_environment_tu15_generated.cpp`
  - **Notes**: #3 largest remaining routine overall. Pure integer/SSE (`0` x87 instructions). Calls `133B40`, `1342B0`, `32A5C0`, `30BD0`, `1211C0`.
- [ ] **`D8330`** (`1,826/frame`) — `source_damage_delivery_runtime_generated.cpp`
  - **Notes**: #4 largest remaining routine overall. Object/material setup loop in damage delivery runtime.
- [ ] **`1EFD60`** (`1,314/frame`) — `source_multiplayer_engines_tu15_generated.cpp`
  - **Notes**: Direct child of already-native `1EEE80` (`native_tu15_engine_player_update.hpp`). Has `0` x87 instructions! Porting `1EFD60` and `1F0670` completes the `1EEE80` multiplayer engine player update closure.
- [ ] **`14D410`** (`664/frame`) — `physics_live_support_tu15_generated.cpp`, `source_combat_action_tu15_generated.cpp`, `source_model_render_tu15_generated.cpp`
  - **Notes**: Multi-unit leaf/helper across `live_support`, `combat_action`, and `model_render`.
- [ ] **`2B9F0`** (`640/frame`) — `source_texture_camera_producer_tu15_generated.cpp`
  - **Notes**: Texture camera producer frame update.
- [ ] **`1F0670`** (`438/frame`) — `source_multiplayer_engines_tu15_generated.cpp`
  - **Notes**: Second direct child of already-native `1EEE80` (`native_tu15_engine_player_update.hpp`).

---

### Group L — Live Support Remainder (`physics_live_support_tu15_generated.cpp` & Multi-Copy Leaves)

> The project pins the SHA256 of the complete live-support source. The operand specializer binds native owners in its build-local copy. The shared leaves use `tools/native_live_support_leaves_replacements.py`, and `BBD10` uses `tools/native_object_hierarchy_root_replacements.py`.

- [x] **Bind `BBD10` in `physics_live_support_tu15_generated.cpp`.** The build-time operand specializer installs the existing hierarchy-root bridge after checking the complete literal and prepared bodies. The native census counted zero callbacks for the walk in Armory and four-player Coagulation on November 20.
- [ ] **`1D6FB0`** (`1,601/frame`) — `physics_live_support_tu15_generated.cpp`
  - **Notes**: Largest remaining routine in `physics_live_support`.
- [ ] **`2F8B60`** (`842/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`BCDF0`** (`790/frame`) — `physics_vehicle_environment_state_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`, `source_damage_delivery_runtime_generated.cpp`
  - **Notes**: Multi-unit object/hierarchy leaf.
- [ ] **`2E5320`** (`789/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`E4A40`** (`758/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`303800`** (`635/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DEAE0`** (`628/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1F8B30`** (`622/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`C85A0`** (`604/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`113D30`** (`596/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`30A020`** (`569/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`314A30`** (`538/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`10E1F0`** (`536/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`1EDE60`** (`524/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`DF880`** (`507/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`D05A0`** (`475/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`E9530`** (`457/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`B9460`** (`432/frame`) — `physics_live_support_tu15_generated.cpp`
- [ ] **`113190`** (`388/frame`) — `physics_live_support_tu15_generated.cpp`, `source_ai_frame_tu15_generated.cpp`

---

### Group E — Effects & Lighting

> **Binding mechanism**: `death_cycle_runtime_tu15_generated.cpp` and `source_combat_action_tu15_generated.cpp` are whole-file SHA256 pinned; bind via `tools/native_effect_owner_replacements.py` (or `tools/native_live_support_leaves_replacements.py` for shared leaves). `source_effect_particle_draw_tu15_generated.cpp` and `native_light_constants_tu15_generated.cpp` can use direct source prefixes registered in `tools/native_source_prefixes.py`.

- [ ] **`30BD0` Census-Only Admission** (`1,575/frame` in census; **already native in production**)
  - **Notes**: `src/physics/runtime/native_tu15_normalize_vector_bridge.hpp` is already native in production, and is guarded by 22 mutation tests in `tools/validate_native_tu15_normalize_mutations.py` that pin exact line occurrences. It only falls back in the census when `cursor == nullptr`. If updating `native_tu15_normalize_vector_bridge.hpp` to admit `native_census_detail::uncursored(s, 0x30bd0u, 0x30c84u)`, update `validate_native_tu15_normalize_mutations.py` in lockstep.
- [ ] **`17C370`** (`978/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0` (`CHILD_CALLS` in `tools/native_live_support_leaves_replacements.py`).
- [ ] **`49790`** (`755/frame`) — `native_light_constants_tu15_generated.cpp`
  - **Notes**: Light constants table/shader packaging routine.
- [ ] **`17C140`** (`743/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `native_weapon_effect_advance_tu15_internal.hpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0`.
- [ ] **`178070` Census-Only Admission** (`693/frame` in census; **already native in production**)
  - **Notes**: `src/rendering/effects/native_tu15_effect_reference_element_admission.hpp` is already bound and native in production across 5 units; it refuses in the census only because `admitted(s)` requires `s.instruction_cursor == &s.services.instruction_cursor` when `s.services.instruction` is set and does not check `native_census_detail::uncursored`.
- [ ] **`1772C0`** (`543/frame`) — `source_effect_particle_draw_tu15_generated.cpp`
  - **Notes**: Effect particle draw routine.
- [ ] **`251C80`** (`530/frame`) — `death_cycle_runtime_tu15_generated.cpp`
  - **Notes**: Particle state / location sibling of `251A60`/`250450`.
- [ ] **`250780`** (`528/frame`) — `death_cycle_runtime_tu15_generated.cpp`
  - **Notes**: Particle state / location sibling of `251A60`/`250450`.
- [ ] **`17C5C0`** (`433/frame`) — `death_cycle_runtime_tu15_generated.cpp`, `physics_live_support_tu15_generated.cpp`
  - **Notes**: Child of already-native `179CD0`.

---

### Group S — Shadow Frame Remainder (`source_shadow_frame_tu15_generated.cpp`)

> **Status**: 10 routines in `source_shadow_frame_tu15_generated.cpp` (`134D30`, `136D50`, `4BAE0`, `135E00`, `136230`, `1365C0`, `1362A0`, `C3AB0`, `1373F0`, `BBD10`, `135690`) are now proven on physical i386 and bound! Only two larger routines (`166C90`, `C3DC0`) and the small `2CBD0` callers remain:

- [ ] **`166C90`** (`1,268/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: 260 lines, 54 x87 lines; calls `1845C0`, `184690`, `134D30`, `166E60`, `17C840`, `166C00`, `136030`.
- [ ] **`C3DC0`** (`894/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: 487 lines, 84 x87 lines; calls `C3A20`, `144090`, `C1E90`, `C3AB0` (now native), `C1BA0`, `C1FB0`, `167790`.
- [ ] **`135EA0` & `136030` (+ `2CBD0`)** (`<350/frame`) — `source_shadow_frame_tu15_generated.cpp`
  - **Notes**: `135EA0` (104 lines, 0 x87) and `136030` (83 lines, 0 x87) are short visibility/draw helpers, but unlike `136A80` (which calls `s.required(0x136ce0u, 0x2cbd0u, false)`), their generated bodies call `run_2cbd0(s)` directly (`2CBD0` is a 904-line routine inside `source_shadow_frame_tu15_generated.cpp`). Either port `2CBD0` first or stub/route `run_2cbd0(s)` through a boundary callback validated by `native_source_membership_tests`.

---

### AI & Campaign Runtime (`source_ai_frame_tu15_generated.cpp`, `source_campaign_runtime_tu15_generated.cpp`)

- [ ] **`1B0020`** (`1,519/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`1B01E0`** (`1,475/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`16E510`** (`806/frame`) — `source_campaign_runtime_tu15_generated.cpp`
- [ ] **`2AA6D0`** (`806/frame`) — `source_campaign_runtime_tu15_generated.cpp`
- [ ] **`209F40`** (`487/frame`) — `source_ai_frame_tu15_generated.cpp`
- [ ] **`1B08B0`** (`464/frame`) — `source_ai_frame_tu15_generated.cpp`

---

### Legibility Refactor Backlog (Already Native, Needs `*_systems.hpp` + Adapter Split)

- [ ] **`BD080`, `1532A0`, `14DD70`** matrix leaves (`src/physics/runtime/native_tu15_matrix_leaves.hpp`)
- [ ] **`135B60` (`native_tu15_object_admission.cpp`) & `136A80` (`native_tu15_model_draw_submission.cpp`)** renderer closures

---

## 4. Known Hardware & Physical-i386 Oracle Pitfalls (Read Before Porting!)

1. **RMW and x87 Store Pre-I/O Timing (`at(pc)` before store)**:
   - For read-modify-write instructions (`or [mem], reg`, `and [mem], imm`, `inc [mem]`), call `at(pc)` **once** before the read, then use `store(..., true)` (`positioned=true`) for the write so the instruction callback `before_(s_, pc)` fires only once and sees the pre-read register/flag state.
   - For `fstp dword ptr [mem]` / `fst dword ptr [mem]`, the hardware instruction callback `at(pc)` fires **before** the x87 store/pop changes `status`/`top`/`c1`. Always call `at(pc)` first, execute the x87 store (`fstp_f32` / `fst_f32`), and then record the memory write with `positioned=true`.
2. **C++ Function-Argument Evaluation Order**:
   - GCC evaluates function arguments right-to-left! Never write `store32(Access::store_foo, addr, load32(Access::load_bar, src))` or `call_child(read_arg(a), read_arg(b))` if both arguments touch `Access` roles or mutate registers/stack. Always bind each `load`/`read_arg` to a local `const auto` variable on its own statement in exact execution order.
3. **`comiss` vs `ucomiss` on QNaN**:
   - `comiss` signals `#IA` (setting `MXCSR.IE`, bit 0) on **both** QNaN and SNaN, whereas `ucomiss` only sets `MXCSR.IE` on SNaN. Pass `quiet=false` to `s_.flags.scalar_compare(a, b, false)` for `comiss` and `quiet=true` for `ucomiss`.
4. **Zero-Count `REP MOVSD` / `REP STOSD`**:
   - When `ECX == 0`, hardware `rep movsd` / `rep stosd` still executes the instruction once at `pc` as a no-op (advancing EIP without reading or writing memory). If the trace records instruction PCs or faults, ensure `ECX` is masked/initialized the same way as the caller (`movzx ebx, bx` vs full 32-bit `ebx`).
5. **Capstone Metadata Quirks (`tools/native_shadow_frame_oracle.py`)**:
   - Capstone reports `operand.access == 0` (`read=False, write=False`) on the memory operand of `fstp`/`fst` and `test [mem], reg` (even though `test [mem], imm` has `CS_AC_READ`). `tools/native_shadow_frame_oracle.py` now normalizes `fst`/`fstp` to `write=True` and any `(not read and not write)` memory operand to `read=True`.

---

## 5. Verification & Launcher Promotion Commands

### 1. Run Single-Owner Physical-i386 Validator
```bash
python3 tools/run_test.py -- .tools/python/bin/python -B tools/validate_native_<owner>_tu15.py \
  --xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe" \
  --compiler g++ --report build/test-results/shadow-frame-20261119/<owner>.json \
  --prefixes --require-binding --sanitizers --semantic-mutants
```

### 2. Verify Source-Prefix Membership (`tools/native_source_prefixes.py`)
```bash
python3 tools/run_tests.py --target native_source_membership_tests --filter '^native_source_nonplaying_membership$'
```

### 3. Build Shared Perf Binary & Run Live Armory + Census + MP Sweep
```bash
# 1. Build shared perf binary (capped at 4 jobs for 64 GB RAM safety)
JOBS=4 tools/build_perf.sh

# 2. Run Armory parity + live verify
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py armory build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/armory \
  default \
  reference:HALO2_REFERENCE_<OWNER>=1 \
  verify:HALO2_VERIFY_<OWNER>=1

# 3. Run Armory census
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py census build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/census \
  --reference HALO2_REFERENCE_<OWNER>=1 --owner <entry_hex>

# 4. Run 8-map Multiplayer verify sweep (--workers 1 for 64 GB RAM safety)
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  python3 tools/perf_4k60/live_check.py sweep build-perf-4k60/halo2_native \
  --out build/test-results/shadow-frame-20261119/sweep --workers 1 \
  mp:coagulation:1 mp:coagulation:4 mp:headlong:4 mp:zanzibar:1 \
  mp:containment:4 mp:burial_mounds:4 mp:turf:1 mp:waterworks:4
```

### 4. Promote a New Qualified Build to `Halo2-Launcher`
```bash
./Halo2-Launcher --build-engine
./Halo2-Launcher --launcher-info
```
