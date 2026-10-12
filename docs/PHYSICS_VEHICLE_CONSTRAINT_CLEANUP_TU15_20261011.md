# TU1.5 vehicle constraint cleanup and force-state port

English TU1.5 is the sole parity target. The original calls from the sealed
contact/reference graph authenticate cleanup entries `22E390`, `22FE30` and
`22FEA0`. Recovery follows their direct calls and seven reached secondary
owners. It adds 21 complete native bodies and 2,565 distinct instruction sites;
the composed graph contains 120 bodies and 13,781 sites. The already compiled
TU1.5 body `1D8C80` is reused rather than duplicated.

| TU1.5 entry | Instructions | Recovered behavior |
| --- | ---: | --- |
| `0DE8C0` | 66 | Resolve the original component association/property record. |
| `16E6A0` | 78 | Allocate and initialize an original datum-pool slot, occupancy bit and generation. |
| `178E30` | 214 | Dispatch the original object lifecycle operations and their required owners. |
| `17A0F0` | 52 | Stop a force record through its actual cleanup owners. |
| `17DEC0` | 102 | Update a force record and deliver vectors, scalars and linked 13-float transforms. |
| `18BBC0` | 54 | Convert the original float vector into packed integer fields. |
| `18C440` | 34 | Build the original activation record. |
| `18CDD0` | 137 | Process activation state and call its actual physics/object owners. |
| `18D4B0` | 24 | Create a reference record using the real datum-pool allocator. |
| `18D510` | 12 | Update reference-record flags. |
| `1903A0` | 44 | Resolve the original type/color lookup and its fallback owner. |
| `1D8B90` | 55 | Convert body pose data, retaining the optional transform owner. |
| `22E210` | 99 | Calculate the contact basis using the real retained normalization body. |
| `22E390` | 121 | Resolve the source body/record and its component association. |
| `22E4F0` | 406 | Calculate contact transforms and force inputs. |
| `22EEC0` | 86 | Process contact operations through the actual point/transform owner. |
| `22EFD0` | 123 | Dispatch original point/object transforms. |
| `22FE30` | 27 | Apply the original threshold and timer expression. |
| `22FEA0` | 775 | Process original constraint cleanup, state changes and force delivery. |
| `230A60` | 42 | Stop the source constraint/force handles. |
| `230AE0` | 14 | Evaluate the original handle predicate. |

New recovery and execution read only the owned English TU1.5 executable,
SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Earlier correspondence IDs are retained parent identifiers. Every new
instruction/store ID is the actual TU1.5 address. Generated C++ contains static
native operations and branches, with no original-code execution or runtime
opcode decoder. The TU1.5-only API is
`source_run_physics_vehicle_constraint_cleanup_runtime`.

The bounded original corpus contains 15,141 cases: 12,689 complete calls and
2,452 real required-owner prefixes. It exercises 1,351 of the 2,565 new sites.
Inputs include 120 allocator cases, 40 source-created reference records, 80
force updates, 12 composed cleanup-to-force calls, 288 contact-basis cases,
1,440 threshold cases and 264 packed-vector cases. The allocator cases cover
capacity, occupancy, cursor/high-water behavior, generation changes, clearing,
varied strides and the 31/33-slot bit boundary. Both implementations allocate
their own slots and compute their own force records and copied transforms.
No successful allocation, solver, activation or cleanup result is supplied.

The force and composed cleanup cases declare their source pool, tag, component,
pose and linked-block records as inputs. They cover the full force-update body
and both normalization calls in the basis body. They do not establish actual
world contact creation, suspension or driving. The retained 432 independently
computed builder-to-motion cases still have no vehicle solver object or
constraint list. Object lifecycle `178E30` is compiled but unexercised; the
remaining 1,214 new sites lack execution coverage.

The new floating-point paths retain x87 controls, active extended values,
sticky exceptions, MXCSR and the defined C1 result of `FLD`/`FISTP`. The latter
is checked against original host execution; the native integer spill captures
the actual x87 status. Undefined condition bits are excluded according to the
last original FP instruction, following the
[Intel instruction reference](https://cdrdv2-public.intel.com/671110/325383-sdm-vol-2abcd.pdf).
Unicorn is used to discover memory footprints; its FP precision/status behavior
is not the final oracle.

Evidence is retained under
`analysis/multiplayer/sandbox-source/vehicle-constraint-cleanup-tu15-20261011/`.
The hardware oracle single-steps unchanged original TU1.5 graph bytes on the
host Linux i386 x87/SSE backend. Test-only capture stubs at unported owners
preserve reached state without providing returns. Native comparisons check
instruction order, ordered stores, GP/SSE/x87 state, defined flags, FP controls,
retained memory and missing-owner rejection. The validator pins repeated
recovery, generation, diagnostic execution, hardware fixtures, the transitive
source cohort, reused parent archives and Release/ASan/UBSan/production logs.

Required deeper object, activation, pose/point-transform and force-cleanup
owners remain explicit boundaries. The live player mover, world/contact/solver
ownership, player/vehicle/object integration, authored scenario traces,
split-screen and LAN remain required by the active full physics goal. Bounded
host comparisons do not establish Xbox hardware equivalence or full physics
parity.
