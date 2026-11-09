# Legacy image production reachability

The migration scope includes all reachable compiled production variants.
Fixed MapPack11 state construction is image evidence, not an offline-only
boundary. No game or original executable was run for this read-only audit.

The configured Linux x86_64 Release `halo2_native` File API closure includes
all nine fixed-MapPack11 generated units below. They contain 924 translated
function instances. These were omitted by the earlier inventory expression,
which did not recognize `[[gnu::noinline]]` before function definitions. The
corrected parser finds 8,794 address-named functions, agreeing with an independent
definition scan, plus three descriptive owners across all 187 generated units.
The complete 8,797 subtotal includes `producer`, `run_status`, and
`source_run_tu15_power_dpc`; 8,749 belong to the 180 units in the recorded
configured production closure. Later native bindings do not change this source
ownership count or establish execution of retained bodies.

| Fixed MapPack11 generated unit under `src/` | Function instances |
| --- | ---: |
| `gameplay/units/death_startup_runtime_generated.cpp` | 418 |
| `gameplay/units/death_object_phases_generated.cpp` | 261 |
| `gameplay/units/unit_object_runtime_generated.cpp` | 90 |
| `gameplay/units/death_world_physics_runtime_generated.cpp` | 61 |
| `physics/collision/source_segment_collision_generated.cpp` | 40 |
| `gameplay/units/death_world_scheduler_generated.cpp` | 22 |
| `gameplay/units/biped_postphysics_phase_generated.cpp` | 20 |
| `gameplay/units/biped_postphysics_runtime_generated.cpp` | 6 |
| `rendering/source_point_collision_runtime_generated.cpp` | 6 |

## Route conditions

The ordinary MCC constructor uses compiled TU1.5 data and constructs
`PhysicsLiveWorldTu15`. Campaign ownership explicitly requires TU1.5. This
qualifies only those paths, not every branch of the shipped application.

The application parser still exposes `--gameplay-xbe PATH` and
`--mp-source-object-world DIRECTORY` in `src/app/main.cpp`. Multiplayer world
construction passes that supplied `OriginalData` into `Cache`, then the
explicit-image `MultiplayerObjectRuntime` constructor. `OriginalData::load_xbe`
recognizes MapPack11; the constructor's non-TU1.5 branch constructs `Impl`,
whose state bank and startup executor are explicitly MapPack11. `source_storage`
requires explicit source-world selection when TU1.5 automatic admission is
absent. No offline-only assertion was found along that application path.

This is a static callable path. Successful full-world construction for every
asset/configuration, current launcher selection and actual execution of all
legacy functions have not been demonstrated. An unsupported later operation
does not prove that the preceding compiled startup/resource work is unreachable.

## Named remaining owners

| Owner | Production evidence | Dynamic qualification |
| --- | --- | --- |
| `12F5E0` | In the startup ownership table and exported generated switch. MapPack11 `Impl::preload_render_model` directly requests and polls it; `Impl::build_world_model` supplies it as the geometry resource callback. Public wrappers select these methods when the TU1.5 owner is absent. Seven generated startup routines also call it literally. | Real execution under the explicit MapPack11 route remains unproved by this audit. It remains required migration work. |
| `2E8F00` | In the object-phase ownership table, exported generated switch and common dynamic dispatcher. Its complete translated `State` body is compiled into `halo2_systems`. | No distinct literal caller found outside the entry switch/dispatcher; vtable or callback reachability is unresolved. Keep the owner in the ledger until its dynamic route is proved or an actual production exclusion is established. |

The new inventory records constructor image scope and gives fixed MapPack11
entries `observed_map_pack11_entry`, with `observed_tu15_entry` null. The census
now parses attributed functions and excludes a fixed other-image unit only
when analyzing a particular image. Unknown/mixed constructors stay unresolved;
the checked vehicle observation remap remains explicit.

## Evidence and source owners

- CMake: `CMakeLists.txt` registers both death generated units and their runtime
  adapters directly on `halo2_systems`; the File API confirms actual target membership.
- Branch and calls: `src/gameplay/multiplayer/multiplayer_object_runtime.cpp`,
  `src/gameplay/multiplayer/multiplayer_world.cpp`, and `src/assets/original_data.cpp`.
- Ownership/dispatch: `src/gameplay/units/death_world_runtime_entries.hpp`,
  `death_startup_runtime.cpp`, `death_object_phases.cpp`, and their generated units.
- Bounded evidence: `evidence/linux-release-production-closure-20261108-r3.json`
  and `build/test-results/native-replacement-20261108/legacy-image-reachability.json`.
  Later source/configuration changes are recorded by inventory freshness fields;
  immutable cohorts do not automatically qualify the current runtime.
