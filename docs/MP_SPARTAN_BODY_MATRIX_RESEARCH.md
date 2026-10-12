# Original current-body matrix delivery leaf

The isolated native `1D42D0` consumer matches ten complete original calls on the
actual newly constructed Spartan bodies. It reads each committed body's motion
transform and emits the source 0x34-byte matrix. This is a current-state delivery
leaf, not live ragdoll admission, world insertion, contact solving, hierarchy
propagation or rendering.

## Authority and source preparation

The authority is unchanged TU1.1 executable SHA256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`
and original Lockout/shared cache data. The separate
[oracle](../tools/oracle_mp_spartan_body_matrix_1_1.py) reproduces every stage,
call, read and store of the positive ten-body/nine-constraint constructor
byte-identically before making ten explicit original `1D42D0` callee calls.
No body, node, matrix, component flag, pose, contact, world/PVS, clock or success
is supplied. The output addresses are explicitly declared caller scratch;
no expected geometry is used as a guest input.
[Constructor authority and limits](MP_SPARTAN_RAGDOLL_CONSTRUCTOR_RESEARCH.md).

The inherited selected startup, command axes/heading, 30Hz timestep and
platform/audiovisual boundaries remain explicit. The ten-body constructor is an
explicit callee root with unit+34B still0 and new body world pointers NULL.
These matrix calls do not change that admission or insert a world.

The prior complete authenticated correspondence maps **1D42D0 → 1D8B90** with
identical normalized instructions and internal branches. Its one literal scale
constant is included in the paired memory operands. This new dynamic run
executes1.1; it does not relabel1.1 execution addresses as1.5 or prove a live
whole-title1.5 ragdoll.
[Immutable paired body](../analysis/multiplayer/integration/death-animation-20261010/correspondence/report.json).

## Complete owning input/output contract

Original ABI is EAX=slot index, ECX=raw component, ESI=output34; the body returns
with `RET` and ESP delta4. The measured unchanged EAX return is motion+80, while
the native memory consumer exposes the owning output matrix. The represented
algorithm and order are:

1. Resolve the real slot through component+70 + index*60h, its body through
   slot+40, and its motion through body+3C. Read the aligned motion+80 40h-byte
   transform. The original function copies four 16-byte vectors into its aligned
   local scratch at1D42F8/1D4300/1D430F/1D4318. Component+4 is read before the
   first local store.
2. If component+4 bit6 is set, call original **2E0B60** with the copied transform
   and component+30. This branch is source-listed but unexecuted in these actual
   bodies. Its native service has no success default; absent service stops after
   the four original copies and before any output store.
3. Read the original binary32 word at465030, write output+0 at1D4334, then copy
   scratch offsets `{0,4,8,10,14,18,20,24,28,30,34,38}` hexadecimal into output
   +4..+30 in original instruction order. It preserves every raw float word;
   there is no normalization, interpolation or newly computed pose.

[Complete original1D42D0](../analysis/multiplayer/integration/spartan-body-matrix-20261010/measured/instructions/001d42d0.asm),
[listed-only2E0B60](../analysis/multiplayer/integration/spartan-body-matrix-20261010/measured/instructions/002e0b60.asm).

The native API is
`source_physics_body_matrix_1_1(memory,component,index,output34,scratch40,correction,observer)`
in [physics_body_matrix.hpp](../include/halo2/physics_body_matrix.hpp) and
[physics_body_matrix.cpp](../src/physics_body_matrix.cpp). Source memory must
provide actual component/slot/body/motion backing, the literal scale word and
caller-owned aligned scratch/output. Missing backing or correction is an error,
not an identity matrix or a captured result. The adapter validates the original
MOVAPS alignment requirement. The API does not allocate a body or a pose bank.

## Measurement, independent repeat and native comparison

[Measured report](../analysis/multiplayer/integration/spartan-body-matrix-20261010/measured/report.json)
and independent repeat have SHA256
`8fad755fbc5b2264d8aa3d354006f7c1bf73d7482048119e646f4e3f71eb136d`.
The report and both complete listings compare identically. All ten original
calls return, with270 reads,220 raw memory-store callbacks and zero supplied
services. The [auditor](../tools/audit_mp_spartan_body_matrix_1_1.py) verifies75
listed instruction rows,150 unchanged full backing intervals and all520 emitted
output bytes against recorded source stores. The audit repeats identically.

The [exporter](../tools/export_mp_spartan_body_matrix_1_1.py) writes only original
captured input/output fields and ordered stores to the independent
[fixture](../analysis/multiplayer/integration/spartan-body-matrix-20261010/fixture.txt).
The fixture and metadata independently repeat exactly. Its schema is:

- `C index component output scratch`: actual source call inputs.
- `M address rawhex` / `A address rawhex`: complete captured before/after spans.
- `W instruction address beforehex writtenhex`: original owning stores.

Only the source ABI prologue `PUSH EBP` is omitted from the native owning-buffer
projection. Unicorn splits each original MOVAPS into two 8-byte callbacks. The
report retains those raw callbacks; the native test compares ordered
instruction/address/before/written **bytes**, preserving their original order
without treating emulator callback width as a different algorithm.

The [test](../tests/physics_body_matrix_tests.cpp) passes normal and actual
ASan/UBSan builds with leak detection. All complete component, slot, body,
motion, constant, scratch and output spans match; the170 logical owning stores
match1,160 ordered source bytes. A separate explicitly native-only bit6 negative
control verifies the required-service stop after four copies, unchanged output
and exact preceding stores. It is not claimed as an original positive correction
execution. No captured body or finished matrix is supplied to the native
algorithm. These are isolated source/module tests; shared runtime, renderer,
CMake and releases are unchanged by this leaf.

## Integration limit and next owner

`BDF80 → 1D7310` is the source physical-first pose delivery chain. This new leaf
can supply its real per-body matrix step once the actual current slot/node mask,
shape virtuals, optional child-frame transform and `1D7710` hierarchy owner are
provided. The existing native `source_refresh_physics_body_slot_1_1` also names
1D42D0 as a required dependency on its component-correction path; that positive
path still needs2E0B60 rather than being silently admitted here.
[Physical node contract](MP_DEATH_RAGDOLL_INTEGRATION_RESEARCH.md),
[retained slot consumer](../src/physics_body_slots.cpp).

Natural +34B contact admission, positive native biped/constraint construction,
world/island/solver updates, continuous corpse movement, corpse lifetime/network
and presentation remain separate live owners. The generated source fixtures
are validation inputs; replaying them is not a live construction backend.

```sh
.tools/python/bin/python tools/oracle_mp_spartan_body_matrix_1_1.py \
  --output NEW_DIRECTORY/report.json
.tools/python/bin/python tools/export_mp_spartan_body_matrix_1_1.py \
  --directory NEW_DIRECTORY --output NEW_DIRECTORY/fixture.txt
c++ -Iinclude -std=c++20 -O2 -ffp-contract=off \
  src/physics_body_matrix.cpp tests/physics_body_matrix_tests.cpp -o NEW_TEST
NEW_TEST NEW_DIRECTORY/fixture.txt
```
