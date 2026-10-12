# Source weather particle simulation

The helper reproduces the supplied ready-particle branch of original `38450`.
It is registered in the CPU-only `halo2_weather_simulation` library through
`cmake/SourceWeatherSimulation.cmake` and does not activate multiplayer weather. Resource
publication, view admission, clocks, wind ownership and the reciprocal-square-root
instruction's implementation remain explicit caller inputs.

## Independent actual original call

The immutable original Lockout snapshots under
`analysis/multiplayer-shading/original-weather-20261009/session1/` contain an
actual `38450` entry, its complete return, and a before/after pair for literal
`RSQRTSS` at `38B02`. The original title code and assets were unchanged. Hardware
breakpoints paused the xemu0.8.136 guest; these are emulator observations rather
than physical Xbox measurements.

The entry has system `8026E92C` in EBX, owning `weat` root `8026E87C` at stack
argument one, portal distance `BF800000` in XMM0, and original stack return
`36E99`. The return resumes there with ESP advanced eight bytes and AL=1. Group
`819C6008` owns 1,024 render rows at `819C603C` and state rows at `819CB040`.
Their strides are 20 and 32 bytes. Actual control words are FCW `023F` and MXCSR
`1FA0`. FCW selects 53-bit x87 precision; it does not select binary32 arithmetic.

`tools/audit_source_weather_particle_simulation_original.py` executes every
called title instruction unchanged over a separately owned copy of this RAM.
No ready result, random value, arithmetic result or service return is supplied.
The complete call executes 506,927 instructions at 856 distinct source
addresses. All 41,120 nonstack bytes written match the independent actual
return. Final GPRs, all eight SSE registers, flags and recorded floating control
state also match. The comparison excludes dead stack scratch, inactive x87
payloads, privileged CPU state and unrelated RAM; it is not a full-machine
emulation assertion.

The real `12DE70` linked-ready call still runs. It refreshes physical resource
row `7FE601D8` from age `00020CF8` to `00020CF9`; bypassing the service would
lose that source state. The auxiliary random stream advances 3,168 times:
1,012 initially nonnegative rows take three draws each and 12 negative rows
respawn with eleven draws each. Ten actual exclusion volumes and their plane
banks are read. Five previously active rows finish excluded.

The directly observed instruction has bytes `F3 0F 52 44 24 40`. Its operand
`4012FB04` produces `3F28ED69` at the actual next PC `38B08`. The same pair is
exact in unchanged Unicorn execution. The complete replay records 1,009 operand
and result pairs; its resulting particle banks independently match the actual
return. Only the first pair has its own direct guest instruction snapshot.
The remaining pairs are corroborated full-call replay observations. Neither
set defines a general physical Xbox RSQRT implementation.

## Native interface and phase ownership

`source_weather_advance_particle_elapsed(system, elapsed)` represents the
literal `384E5..384FD` store before `12DE70`. The caller invokes it once even
when the resource is not ready. It neither queries nor declares readiness.

`source_weather_step_ready_particles(system, render, state, input, auxiliary)`
requires a completed actual ready service and an already advanced system. Its
system span is read-only. Its input carries the actual 244-byte view bank,
owning weather root through offset AC, exclusion centers/radii/planes,
signed view count, elapsed, intensity, portal distance, and observed controls.
The helper clamps view count to 1..4, partitions by signed integer quotient,
and preserves undistributed remainder rows.

Every velocity-limit operation calls a required `SourceWeatherParticleRsqrt`
producer with the actual particle ordinal and squared-speed word. There is no
default reciprocal, square root, SSE estimate, cached answer or fitted result.
The native tests provide the frozen literal-source operand/results and verify
each requested operand and ordinal before returning an answer.

The port preserves PC53 random/interpolation intermediates until each source
float store, separately rounded SSE operations, Z/X/Y perturbation draw order,
rotated respawn extents, source matrix order and wrap branches, source opacity
conversion, and the different active `>` versus respawn `<` count gates. It
calls the separately proved `source_weather_point_in_exclusion` for `38390`.

Mutable row banks and auxiliary state commit only after successful completion.
Missing/invalid ISA answers, thrown producers, nonfinite stored scalars or
squared speeds,
incomplete or aliased banks, and unsupported controls leave those banks and
auxiliary unchanged. A prior successful pre-gate elapsed store is a separate
completed phase. External callback side effects are not rolled back.

This implementation checks binary32/binary64 widths, `FLT_EVAL_METHOD==0`, and
the actual native x86 SSE environment: nearest rounding, masked exceptions,
no DAZ and no FTZ. It checks again after each external ISA call and never sets
the caller's controls. Other host control inspection is unsupported. Build with
contraction disabled and no fast math. Exceptional source floating inputs and
more than 32 selected exclusion scratch entries are explicitly rejected.

## Validation and reproduction

The combined fixture has one actual call and 45 declared controlled calls over
small owned copies of the same ready banks. Every callee executes unchanged;
the source service reads the genuine retained manager/cache/resource rows.
Controls cover view-count clamps and partitions, remainder and zero-quotient
rows, elapsed/intensity/portal limits, mass recovery, respawn admission, rain
opacity exclusion, near volumes, velocity limits, box dimensions and supplied
transform scales. They are input experiments, not inferred title history.

Normal and fully instrumented ASAN/UBSAN runs with leak detection pass all
33,306 output/control words, 2,134 explicit ISA pairs and 7,479 auxiliary draws.
There are 378 ownership, input, failed-producer and native-control rejection
checks, including finite-input overflow and a producer that changes MXCSR.
These private proofs involve no GPU, guest access, resource allocator or runtime
activation. The separate CMake registration exposes the actual and controlled
particle fixtures alongside the plate program and its admission test.

- `tools/audit_source_weather_particle_simulation_original.py` reproduces the
  independent actual-call comparison and raw entry/replay/return banks.
- `tools/freeze_source_weather_particle_simulation_fixture.py` freezes the
  actual call's fixture and authority distinctions.
- `tools/differential_source_weather_particle_simulation.py` produces the
  combined unchanged-source control fixture and its report.
- `tools/build_source_weather_particle_simulation.sh <private-dir> normal`
  or `asan` builds only the new helper, existing exclusion arithmetic and test.
- `source_weather_particle_simulation_tests <fixture>` verifies the actual or
  combined fixture; it has no GPU or live mode.

The frozen inputs are
`tests/fixtures/retail_source_weather_particle_simulation{,_controls}.tsv`.
Reports, raw banks, build logs and durable hashes are under
`analysis/multiplayer-shading/weather-particle-simulation-original/`.

Future activation still needs the actual `36B50` visible-palette and portal
admission, first-view clock and wind sequence, published initialized resource
ownership, complete `12DE70` service, and an independently supported ISA
producer. The existing isolated point renderer consumes draw packets; it does
not supply these simulation inputs or an original scheduling timeline.
