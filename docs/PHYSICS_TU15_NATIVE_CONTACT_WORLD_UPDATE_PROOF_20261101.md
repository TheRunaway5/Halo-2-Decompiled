# TU1.5 native contact world update and collector info

English TU1.5 XBE SHA256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Both new modules are native semantic C++ over typed current DATA. The original
i386 process is an offline comparison oracle; it is not compiled into either
kernel. No original GP/SIMD postimage, allocation result or control frame is
supplied to native code.

## World update

`native_tu15_contact_world_update.hpp/.cpp` implements all44 sites of316AA0 and
46 sites of3223D0. Deferred updates grow world+FC with the existing qualified
native2E3250/149DA0/149E00/233250/233420 services, append the current collector,
increment its word reference and write a zero result byte. Immediate updates
select the first body's island, or the second when the first island index is
minus one; they cache that identity before current world listener callbacks,
compact null listeners stably, invoke current island virtual+4 and write one.

`NativeTu15ContactWorldUpdateArguments` provides ordinary result/collector
formal DATA at the actual late source phases. It supports a collector argument
changed during growth and a result address changed during a callback, without
using a source GP file. The simple value overload retains the same behavior
when the input DATA is stable.

Qualification:
`analysis/physics/native-contact-world-update-tu15-20261101-qualified/report.json`.
Both GCC and Clang pass59 declared source-valid cases,981 original reads,
193 stores,83 genuine service entries,14 fault prefixes,five phase mutation
controls,two unbound callback prefixes and three rejected mutants. All90 parent
sites and the entire reused grower/copy are exercised. Independently qualified
allocator modules supply their wider branch domain; this composition does not
claim every allocator branch was exercised by these59 parents.

`analysis/physics/native-contact-world-update-tu15-20261101-first/report.json`
retains the earlier source atomic INC prewrite-fault failure. Native normal
execution matched; a requested write-fault at an atomic source INC is observed
before the source instruction's read, while the split native read/write has
already read. That instruction-atomic fault prefix is outside the qualified
domain, explicitly stated in the final report. No persistent normal byte is
waived, changed or supplied as an expected result.

The physical parent cases execute genuine2E4D60 RET4 callbacks as declared
component services. Production current world-listener and island virtual+4
owners remain typed requirements. Authenticated island table41F080 selects
3201B0 at+4; the activation agent owns that68-site detach conversion. These
component results do not authorize an unknown callback or claim a complete
live final-point path.

Frozen hashes:

- Header:`5eabb6e71b03ea63f15166e3389707700fd2c2848f11a23f5761f59f22a030a7`.
- CPP:`bdb1597838f8fe27ed28ffc7829cfd7a81550e9e2320f2e898973aaa9f4caa74`.

## Collector constraint-info fill

`native_tu15_contact_collector_info.hpp/.cpp` implements source325E90, the
authenticated41F478 virtual+28 owner. Table+2C is string DATA and is not admitted
as another method. The existing shared `NativeTu15IslandConstraintInfo` names
the16-byte output consumed by island metadata and detach logic.

The function initializes info0/+4/+C to zero and+8 to24, then reads current
collector+94. For32-bit count n it writes
`{48*n+128,48*(n+2),4*n+44,n+2}` with unsigned32 wrapping arithmetic. Signed n
at least two adds`{0,32,4,1}`. Reading count after the initial four stores is
observable when the packet aliases that field or an observer changes it at a
source store. The implementation preserves that phase.

`analysis/physics/native-contact-collector-info-tu15-20261101-first/report.json`
qualifies both compilers on111 cases,all31 sites,106 reads,992 stores,16 aliases,
12 fault prefixes,three phase mutation controls and four rejected mutants.
Counts cover signed boundaries and arithmetic wrapping. The early-read mutant
is rejected specifically by the packet/count alias cases.

325E90 does not read the output packet or store a retained pointer. Source EAX
remains the supplied output pointer on return. Actual3201B0 overwrites it at
320206 before any use, so that caller's native fill service has a typed void
result. This is not a fabricated GP postimage or a blanket claim about a
different caller. All16 packet bytes are fully initialized before this callback
returns. Parent scratch preinitialization raw ledgers retain their separate
obligations.

Frozen hashes:

- Header:`dfb01e6a461d750106386c5094578ac68af56cddf2902fdfcfb92cc310bd5ffc`.
- CPP:`140b388ca5e55c15089bab23804caa5a9d5a134b397f0bf8487607ba15947b21`.

## Context and retention

`analysis/physics/native-contact-update-input-context-tu15-20261101-final/report.json`
audits all local GP/XMM paths of316AA0,3223D0 and325E90. There are no semantic
incoming volatile GP or XMM dependencies beyond current typed receiver/formals.
The earlier audit result is retained: an exact six-byte `LEA EBX,[EBX]` alignment
NOP caused a syntactic false input dependency, corrected by authenticating its
bytes and classifying its zero data/control effect. No meaningful instruction
or dependency is skipped.

All original ELF/bank/compiler files and nested CMake builds use owned
`run_test.py`/`run_tests.py` storage. Required original archives are preserved.
Only bounded reports/source hashes/diagnostics remain, and the native contact
storage ledger verifies each completed measured owner was removed. These
standalone modules have not changed shared World,CMake or generated bindings.
