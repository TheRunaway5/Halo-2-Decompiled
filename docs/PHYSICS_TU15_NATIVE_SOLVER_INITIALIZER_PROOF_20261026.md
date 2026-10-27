# TU1.5 native constraint solver initializer and hash

The new typed kernel implements actual initializer `3268D0` (125 original
sites) and its closed client hash `326640` (152 sites). The authority is the
English TU1.5 image with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No TU1.1 physics code, CPU register file, instruction interpreter, supplied
ready result, or emulator math is used by the native algorithm.

`native_tu15_constraint_solver_client_hash(memory, key_pointer)` copies the
source's bounded input bytes and public salt, preserves signed-byte mixing,
uses the authenticated private coefficient pattern, and retains unsigned
32-bit multiplication, division and wrap. Its original signature is one
cdecl key-pointer argument. It has no meaningful incoming GP/XMM parameter
and performs no FP operation.

`native_tu15_initialize_constraint_solver(memory, fp, services, observer,
stats)` has explicit real time and failure-logger dependencies. Its original
entry consumes no arguments. It reads the client hash, hashes the actual key,
and rereads the client hash before comparison. It never checks the ready byte
as an early return. The source's empty/matching-prefix platform acceptance,
signed-byte date decoding, read order, signed IDIV remainder and wrapped
32-bit comparisons are preserved.

The retained result is
`build/test-results/native-constraint-solver-initializer-tu15/report.json`.
GCC 16.2.1 and Clang 23.1.4 both passed:

| Check | Count |
| --- | ---: |
| Actual original cases | 113 |
| Complete original calls | 40 |
| Genuine logger entry stops | 34 |
| Genuine time entry stops | 1 |
| Read-fault prefixes | 38 |
| Genuine original clock returns | 61 |
| Ordered component reads | 2,957 |
| Ordered original stores | 13 |
| Physical original instructions | 810,466 |

The checks compare ordered reads, stores and store sites, all retained bank
bytes, actual hash results, exact dependency arguments, logical CW/SW/MXCSR,
and preservation of host FP state. Cases include the real private source key
without printing it, bounded/long/signed input bytes, integer payloads,
future/past/invalid dates, platform prefixes, existing ready values, and
repeated key/salt fault occurrences. Hash-seed and date-predicate mutations
are rejected for their specific hash and dependency/ready differences by
the unchanged original.

All 152 hash sites and 111 of 125 initializer sites were visited. The only
14 unvisited initializer sites are stack cleanup and RET instructions after
the still-unowned logger returns. The oracle stops at genuine `2E3850` entry
and supplies no logger return. The native initializer's source algorithm is
complete; execution of the actual logger body remains a separate dependency.

For date qualification the reference ELF preserves the original `.text` and
the encoded `41B668 -> 80000080` import cell. An explicitly declared native
i386 Linux `gettimeofday` bridge implements only the authenticated
`KeQuerySystemTime(PLARGE_INTEGER)` platform contract: real wall-clock input
as eight-byte FILETIME in 100-nanosecond units since 1601 GMT. The original
`37C5C0 -> 32BBAE -> 32AE00` code computes its actual seconds result. A
read-only physical observer records that genuine return; the native
initializer's declared time callback consumes only that service output.
The old initializer is never executed behind the native kernel.

The clock's original import read belongs to that explicit dependency and is
recorded separately from the initializer component's ordered reads. No live
world memory comparison or exclusion rule is changed. This is a component
composition proof with an actual host platform input, not Xbox oscillator
continuity or a production binding claim. The fixed time-divisor path and
NULL time-output argument used by `37C5C0` were covered; other general
division paths, non-NULL time output, and negative/overflowed clock values
not produced by the current host are outside this proof.

Validation used `tools/run_test.py` around the validator and
`tools/run_tests.py` for both owned CMake builds. The 828,781-byte private
fixture, raw key/reference banks, clock/reference ELF files, object files,
test binaries and private build trees were removed. The final owner
`/tmp/halo2-test-cawv94qi` and both private compiler builds were verified
absent. Only small result/command/source-hash reports remain; no large
artifact is retained.

Production initializer/time/logger binding, coherent runtime validation and
visible multiplayer acceptance remain with the parent physics integration
work. This proof does not close the complete physics parity goal.
