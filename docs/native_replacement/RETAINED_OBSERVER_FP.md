# Retained observer FP restoration

`NativeTu15RetainedStoreObserver` deliberately gives its callback the host
register file captured when the observer was constructed, then restores the
native numerical register file after the callback. It retains the existing
read-only/non-reentrant observer policy and ordinary store event semantics.

The old destructor used a bare `FRSTOR`. A callback could raise a masked x87
exception, then unmask it before returning or throwing. The restoration itself
then raised pending `#MF`, replacing normal return or the original C++ exception
with `SIGFPE`. A physical pre-fix probe reproduces exit `-8` from the exact old
header SHA `c8858215b8e136a3f7a38ddb43b200a198bcb0a4c94a25065e822b7815ab9f22`.

The fix is a single `FNINIT` before that destructor's `FRSTOR`. This discards
the callback's pending exception before the waiting restore instruction, then
restores the saved native 80-bit values, tags, TOP, complete status/control word
and MXCSR. Source/native numerical results and store protocol are unchanged.
This does not grant callbacks permission to mutate backing data or source ABI.

Current GCC ASan/UBSan and independent Clang ASan/UBSan each pass 15,360 exact
hardware transfers with native/callback-host depths 0/1/4/8, all TOP positions,
24/53/64-bit precision controls, rounding modes, NaNs, denormals and sticky bits.
Callbacks return or throw after ordinary masked clobbers and pending unmasked
invalid, divide-by-zero or stack exceptions. A further 6,720 transactions use
the actual current `joint_constraint_numeric::Memory` DWORD/FSTP writers and
check complete FP poststate, backing bytes, one preimage/read, one completed
write and the original observer exception. The exact historical old-header
negative still terminates with `SIGFPE`.

The registered paired CMake gate passes 2/2 with zero skips. Target:
`native_retained_observer_fp_tests`; filter:
`^native_tu15_retained_observer_fp(_release)?$`; reports:
`native-tu15-retained-observer-fp{,-release}.json`.

Evidence:

- [Registered GCC pair](../../build/test-results/native-retained-observer-fp-official-offline-20261109/report.json).
- [Sanitizer hardware/negative receipt](../../build/test-results/native-retained-observer-fp-official-offline-20261109/evidence/native-tu15-retained-observer-fp.json).
- [Independent Clang receipt](../../build/test-results/native-retained-observer-pending-final-clang-20261109/report.json).
- [Pre-fix hardware failure](../../build/test-results/native-retained-observer-pending-pre-fix-20261109/report.json).

An initial CMake attempt failed fetching an unrelated online dependency before
tests. The passing invocation archives the existing pinned datachannel and its
three pinned subrepositories into disposable scratch, with exact commit/archive
provenance retained. Original dependency working trees were not patched. An
attempted older Simplex private-stage regression was inapplicable to the current
ROOT public API; its failure diagnostic is retained, and it supplies no pass
claim. The later current numeric-Memory transactions provide the stated
integration check.

This correction follows the r12 frozen inventory and does not amend its bytes.
It changes no raw-ray controller or default binding. Sparse/MMX source owners,
provider FP policies, physical memory-fault atomicity and the existing outer
host C0/C1/C2/C3 limitation remain separate domains. All generated binaries,
build trees, dependency copies and private inputs were removed; no large
artifacts are retained.
