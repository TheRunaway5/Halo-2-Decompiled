# Original vehicle controller ABI port

The [full player, vehicle and object physics goal](PHYSICS_PLAYERS_VEHICLES_OBJECTS_GOAL.md)
remains active. This component extends the earlier scalar vehicle kernels into
their original controller call graph. It preserves the actual GP, SSE, stack,
active extended precision values, physical x87 TOP, comparisons and ordered
memory writes. It does not complete live driving or the shared physics engine.

## Source ownership

The Map Pack 1.1 executable is pinned to
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The English TU1.5 executable is pinned to
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The prior sealed scalar correspondence authenticates the first six bodies.
Additional helpers are admitted through actual paired call or tail-jump sites;
TU addresses are never inferred from a section delta. Full inventory function
boundaries distinguish the power thunk from its separate wrapper body.

| Map Pack 1.1 | TU1.5 | Compiled instructions | Exercised instructions |
| --- | --- | ---: | ---: |
| `000F02D0` | `000F11F0` | 251 | 156 |
| `000F06B0` | `000F15D0` | 299 | 299 |
| `000F0B30` | `000F1A50` | 232 | 195 |
| `000F1420` | `000F2340` | 583 | 23 |
| `000F5930` | `000F6850` | 31 | 31 |
| `00208D80` | `0020D680` | 63 | 63 |
| `00208E70` | `0020D770` | 31 | 31 |
| `00326AF0` | `0032A660` | 27 | 27 |
| `00379338` | `0037CDE8` | 1 | 1 |
| `0037933A` | `0037CDEA` | 8 | 6 |

All **1,526 instructions** lower to static C++ control flow, integer/SSE/x87
arithmetic, selected-image addresses and real calls. Production has no original
executable execution, instruction decoder or recorded output replay. Generation
fails for unsupported operations or an unmatched instruction inventory.

The directly exercised steering body `F06B0` retains the original vector
normalization and fallback, direction/reversal decisions, x87 `FPATAN`, authored
turn scale and clamps, gear state, rate helper calls and float stores. Its
transcendental uses the native x87 instruction, not a replacement `atan2`.
The actual 64-bit x87 load/store operands retain their full eight-byte memory
extent. The transient state query and stack allocation probe are also exercised
through every instruction, including the probe's register/stack exchange.

## Differential evidence

`tools/oracle_vehicle_runtime_20261011.py` executes untouched original code for
both images, independently of native execution. It reads 48 vehicle definition
prefixes directly from the owned Coagulation, Headlong and Zanzibar maps and
records each map, datum, offset and prefix hash. Raw object bytes, velocities,
directions, controller state, node offset, elapsed time and FP state are declared
test inputs. The bodies are not claimed to come from the original constructor.

There are **1,582 cases per image**: 1,033 complete and 549 stopped at their next
required owner. Each native build compares **3,164 cases**, **703,324 executed
instructions**, **86,642 ordered writes** and **531,310 bytes of final observed
memory**. This includes full GP/SSE values, live 80-bit x87 values, TOP and
comparison state, stack writes, before/after write bytes and absent-owner
failure prefixes. All **832 exercised instruction sites** match. The larger
compiled inventory remains only partially exercised, as shown above.

The controlled inputs include both turn clamp sides, zero-speed fallback,
reversal, gear/long-step flags, multiple authored controller subtypes, all four
rounding modes at 24/53/64-bit x87 precision, and all eight physical TOP values
with empty, one-value and three-value caller stacks. Separate stack-probe inputs
cover zero, small allocations, page boundaries and multiple pages. These are
input comparisons, not thousands of live driving scenarios.

Original instruction and memory hooks only observe execution. First actual
reads and pre-write bytes seed sparse native memory banks; successful callee
outputs are never inserted as inputs. No instruction is patched, no unknown
owner is made to return successfully, and no native result supplies expected
data. Original fixtures, correspondence and generated code have deterministic
repeat checks.

The production CMake module is `cmake/PhysicsVehicleRuntime.cmake`. It links the
component into `halo2_systems` and registers both original-image differential
tests. `tools/validate_vehicle_runtime_20261011.py` freezes the exact component
and its transitive headers; the standalone cohort reuses the production module
with strict FP options in Release and ASan/UBSan builds. Evidence and validation
records live in
`analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/`. The initial
scalar component and its sealed evidence are retained unchanged.

## Remaining boundary

The currently reached missing targets are the original object helper
`0D26C0` / `0D3750`, vehicle helper `0F2960` / `0F3880`, and power implementation
`37935C` / `37CE0C`. Calls preserve their actual pushed return address and full
guest ABI. Absent owners stop at the reached instruction with all preceding
state preserved. The wrapper does not replace original power semantics with
host `pow` or a recorded answer.

This is a GCC/Clang x86 masked-exception backend. FP exception/status flags,
sparse x87 tag stacks, fault behavior, native Windows/macOS and Xbox hardware
equivalence remain unproven. The live `Simulation::move_unit` terrain mover and
the vehicle body/world construction, authored suspension/hover/flight forces,
contacts, constraints, solved readback, seats, destruction and composed maps,
split-screen and LAN physics remain unfinished. Complete compiled bodies and
focused original comparisons do not establish full physics parity.
