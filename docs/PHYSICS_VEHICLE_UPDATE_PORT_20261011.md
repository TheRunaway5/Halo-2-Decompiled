# Composed original vehicle and object update

The [full physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md) remains active.
This extension connects the [retained controller component](PHYSICS_VEHICLE_ABI_RUNTIME_PORT_20261011.md)
to 22 additional original owners. It advances actual vehicle updates through
object velocity reads/writes, hierarchy dirty propagation, component delivery,
state queries and animation-state admission. The live simulation still needs
source-created bodies, shapes, world state and remaining motion/solver owners.

## Authenticated native graph

The owned executables remain pinned to
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
(Map Pack 1.1) and
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
(English TU1.5). The prior ten controller bodies are reused through their actual
native ABI. Additional entries are recovered from matching original calls or
tail jumps, with full function inventories, instructions and branch targets
checked in both executables. Addresses are never inferred from a section delta.

| Additional 1.1 entry | TU1.5 entry | Compiled / exercised instructions |
| --- | --- | ---: |
| `0B8250` | `0B9390` | 26 / 26 |
| `0B8630` | `0B9770` | 47 / 47 |
| `0B86C0` | `0B9800` | 53 / 53 |
| `0BAA80` | `0BBBC0` | 61 / 37 |
| `0BB0C0` | `0BC200` | 107 / 106 |
| `0BBE70` | `0BCFB0` | 13 / 13 |
| `0BC910` | `0BDA50` | 30 / 30 |
| `0BD6A0` | `0BE7E0` | 33 / 33 |
| `0D26C0` | `0D3750` | 34 / 34 |
| `0E7E30` | `0E8D50` | 16 / 16 |
| `0E7E60` | `0E8D80` | 51 / 43 |
| `0F0EB0` | `0F1DD0` | 174 / 155 |
| `0F11A0` | `0F20C0` | 60 / 60 |
| `0F1270` | `0F2190` | 89 / 89 |
| `0F1D00` | `0F2C20` | 8 / 8 |
| `0F2960` | `0F3880` | 24 / 24 |
| `1105E0` | `111500` | 31 / 31 |
| `115600` | `116520` | 25 / 25 |
| `1C8430` | `1CD6A0` | 114 / 56 |
| `1D43C0` | `1D8C80` | 24 / 24 |
| `1D44C0` | `1D8D80` | 24 / 24 |
| `208ED0` | `20D7D0` | 24 / 24 |

The extension adds **1,068 compiled instructions**. Together with the retained
controller component, the graph has **32 paired bodies and 2,594 compiled
instructions**. The original main vehicle update `F1420` now reaches 146 of its
583 instruction sites in the corpus, compared with 23 in the prior cohort.
This is measured progress through the update, not full update coverage.

`source_run_physics_vehicle_update_runtime` binds only authenticated native
owners. Unknown targets retain the caller's explicit service and the actual
pushed return address, selected-image target and full call state. Production
compiles static C++ branches and arithmetic; no opcode decoding, original code
execution, successful default or recorded result replay is installed.

## Shared call-state correction

The first composed native comparison failed at case 854. Original `F1420`
reached `BC910` and then stopped at the missing `BD6A0` owner after changing EAX
and other state. The native shared bridge discarded the nested callee's changed
ABI when its callback threw, retaining EAX `E6F80000` instead of original `0`.

`State::required` now reloads the callback's retained ABI before rethrowing.
This preserves actual progress through nested native owners, including GP/SSE,
flags, FS and active x87 state. Missing owners still stop at their reached call.
The initial failing log is retained as `draft-disc-v1.log`; no expected value was
changed to make this pass. The earlier controller source snapshot remains
unchanged; the new manifest records the exact shared-header overlay. The older
root validator's header equality gate is superseded by this explicit overlay
and retained original comparisons against the new build.

## Original differential evidence

The original-only oracle executes **2,505 inputs per executable**: **1,471
complete** and **1,034 required-owner prefixes**. Each native build compares
**5,010 cases**, **1,497,880 executed instructions**, **141,366 ordered writes**
and **790,008 bytes of final observed memory**. All **1,929 exercised instruction
sites** match. These counts exclude the separately retained 3,164 controller
comparisons and 67,392 scalar comparisons.

Inputs use 48 definition prefixes from the owned Coagulation, Headlong and
Zanzibar maps, with their map/datum/offset/prefix hashes retained. Additional
definition-field overlays, object hierarchies, raw component/motion records,
animation states, velocities, controls, elapsed times and FP states are explicit
test inputs. The oracle rejects a cyclic declared hierarchy; original and native
fault behavior for arbitrary corrupt bodies is outside this evidence. Neither
the bodies nor the component records are claimed to come from the original
factory or represent a live driving scenario.

The corpus exercises zero and nonzero vectors, signed/quiet-NaN scalar queries,
rate/decay boundaries, motion-backed and zero-vector velocity reads, null and
aliased output pointers, parent and child traversal, recursive dirty propagation,
animation-state admission and actual callback-table boundaries. Original memory
hooks observe first reads and pre-write bytes to seed sparse native inputs. No
instruction is patched and no missing callee supplies a fabricated result.

`tools/research_vehicle_update_20261011.py`,
`tools/generate_vehicle_update_20261011.py` and
`tools/oracle_vehicle_update_20261011.py` retain deterministic repeat checks.
`cmake/PhysicsVehicleUpdate.cmake` links the extension to the retained controller
library and `halo2_systems`. The source cohort reuses both production modules.
Release and ASan/UBSan cohorts check both original-image update fixtures and both
retained controller fixtures. Production-root checks additionally cover the
scalar component and twelve retained world/contact/listener tests affected by
the shared header. `tools/validate_vehicle_update_20261011.py` audits exact source,
input, listing, log and binary hashes under
`analysis/multiplayer/sandbox-source/vehicle-update-20261011/`.

## Remaining ownership

The newly reached original boundaries are `0B6760` / `0B7E50`,
`0E8950` / `0E9870`, `0EC9D0` / `0ED8F0`, `112E10` / `113D30`,
`1D5E90` / `1DA750`, `2095A0` / `20DEA0`, and original CRT power
`37935C` / `37CE0C`. Their motion, callback, animation and math ownership must be
recovered. The original power wrapper still has no host `pow` replacement.

The compiled graph remains partially exercised. FP exception/status flags,
sparse x87 stacks and fault behavior remain unproven; the backend requires
GCC/Clang x86 with masked FP exceptions. Native Windows/macOS, Xbox hardware,
all-map scenarios, split-screen and LAN physics remain open. The temporary
`Simulation::move_unit` terrain mover is still present. Full player movement,
vehicle motion classes and forces, source body/world creation, contacts,
constraints, ragdolls, object lifecycle and composed live physics remain required
by the goal.
