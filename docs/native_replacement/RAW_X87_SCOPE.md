# Private raw x87 ownership

`src/physics/runtime/native_tu15_raw_x87_scope.hpp` provides the private
`native_raw_x87_detail::Scope<State>` boundary for native numerical bodies that
use the retained contact/world `State` ABI. It is qualified as an ownership
utility. It does not bind `30AA70`, replace a controller, or establish a raw ray
default route.

The numerical body uses native arithmetic. This utility imports and exports
its register file; it does not decode or emulate arithmetic instructions.
The caller must keep compiler-owned x87 temporaries outside the scope and use
only bounded, non-overflowing source operations. `require_capacity(extra)` is
available before a numerical step that pushes values.

## Source and host ownership

The source register file is a dense logical stack of zero through eight real
80-bit values, with independent physical TOP. The utility imports its values,
TOP and status from `State.fp`, and its current source control word and MXCSR
from the hardware environment. `State.fp.word()` deliberately supplies the
exception bits from the current host x87 status word.

An `io(void_callback)` boundary publishes the complete source logical values,
TOP and status to `State`, then parks the hardware stack empty while carrying
the current source CW, status/exception bits and MXCSR. The callback can obtain
a complete `State.save()` packet without consuming the native live stack.
After success or failure, the utility restores the native source snapshot.
This callback has a read-only source-FP contract; genuine callee FP changes
must use `child(void_callback)` instead.

`child` publishes the actual source inputs and resumes from the actual returned
retained-State values and hardware controls/status. This includes partial
throwing continuations. It does not supply a callee outcome. Nested ownership
is supported. If a child leaves an unsupported FP domain, the utility releases
its host registers and preserves the retained child outcome. A normal return
reports the unsupported domain; an already-throwing child retains its original
exception. Native resumption of that unsupported outcome is not qualified.

Destruction publishes the latest source state and restores the caller's host
register payloads, tags and coherent host TOP. It intentionally leaves the
current **source** CW, exception/status bits and MXCSR for the enclosing
runtime's subsequent `State.save()`. Restoring the entry environment at this
point would discard newly raised source exceptions and changed controls.
Source TOP and condition bits remain in `State.fp` independently of the restored
host register-file TOP.

The runtime ordering is:

1. Enter the existing outer `physics_step_detail::FpScope`.
2. Load the retained source `State`, then enter the private native scope.
3. Publish I/O/child boundaries through that scope.
4. Destroy the native scope, then save the returned/throwing source `State`.
5. Destroy the outer runtime FP scope.

There is a separate pre-existing host-environment limitation: the existing
outer `FpScope` uses `fesetenv`, which restores standard controls and exception
flags but does not restore caller-saved x87 C0/C1/C2/C3 on the qualified glibc
backend. A directed case observed host SW `5D00` to `1B00` with the same TOP,
tags and payloads. The private utility still delivers the **complete source
SW** to `State.save()` before that outer destruction. No source SW bits are
masked to accommodate this host limitation. Full restoration of those outer
host condition bits remains a separate integration item; the common scope was
not changed.

## Staged virtual spills

A native conversion to private owned storage is speculative until its virtual
provider accepts the actual write. FST/FSTP conversion, exception bits, C1 and
stack/TOP changes must not become visible to the pre-write provider packet.

```cpp
const auto pending = scope.stage_single(/* pop = */ true);
scope.io([&] { provider.write(pending.bits); });
scope.commit(pending);
scope.io([&] { completed_observer(); });
```

`stage_single` executes the real conversion and retains the rounded bits plus
pre/post hardware snapshots, then restores the prestate. A provider failure
therefore leaves the original source FP state. Successful `commit` installs
the post-conversion state before the completed-write observer. An observer
failure retains the completed store and its committed FP state. Foreign,
replayed, pre-child and numerically stale tokens are rejected.

The failure authority here is actual physical i386 write atomicity, not the
incidental exception ordering of a retained C++ `w(...)` expression.

## Qualification

The registered paired test is
`^original_tu15_native_raw_x87_scope(_release)?$`, target
`native_raw_x87_scope_tests`, defined by
`cmake/NativeTu15RawX87Scope.cmake`. It requires an explicit authenticated
English TU1.5 XBE for the genuine child tests.

Current-source GCC ASan/UBSan, GCC Release and independent Clang ASan/UBSan
checks each pass:

- 3,552 independent physical i386 arithmetic/control fixtures and 56,832 native
  ownership transfers, with incoming/host depths 0/1/4/8, all TOP values,
  24/53/64-bit precision controls, all rounding modes, sticky status including
  stale masked SF, changed source controls, signed zero, denormals, infinities
  and NaNs; logical 80-bit values, full tags, TOP, C1, full SW/CW and MXCSR agree.
- 576 actual unmapped FST/FSTP write faults with unchanged pre-store FP state.
- 60 unchanged TU1.5 i386 TickConversion/integer child fixtures and 240 genuine
  `State.required` handoffs, including nested scopes, full throwing results and
  partial `22A615` continuations with two live intermediate values; complete
  GP/flags/XMM/FP, backing banks and ordered stores agree.
- Four actual outer-runtime save-order cases and fifteen admission, capacity,
  unsupported-child, sparse-tag and stale-token controls.

The registered sanitizer test also rejects five semantic mutants: early spill
commit, wrong provider restoration, discarded source status, discarded actual
child continuation, and truncated 80-bit values.

Evidence:

- [Registered GCC sanitizer and Release reports](../../build/test-results/native-raw-x87-scope-official-bound-input-20261109/report.json).
- [Sanitizer hardware receipt](../../build/test-results/native-raw-x87-scope-official-bound-input-20261109/evidence/native-tu15-raw-x87-scope.json).
- [Independent Clang receipt](../../build/test-results/native-raw-x87-scope-final-clang-20261109/report.json).

The first registered invocation had no XBE configured and skipped both tests;
it is not execution acceptance. The later explicit-XBE invocation passes both
with zero skips. All generated binaries, private build trees, raw inputs and
oracles were removed. Only bounded reports, hashes and diagnostics remain.

## Follow-through checklist

- [x] Dense native ownership and complete source-state publication.
- [x] Physical staged-spill write-fault authority and completed observer order.
- [x] Genuine retained child suspension/resumption and original-exception preservation.
- [x] Source status/control handoff through the actual outer-runtime save order.
- [x] GCC sanitizer/Release, independent Clang, semantic controls and scratch cleanup.
- [ ] Integrate and qualify a raw numerical owner, including its complete caller
  ABI, lazy source reads, genuine recursive continuations and production route.
- [ ] Supply separate owners for sparse tags/MMX, unmasked or pending exceptions,
  stack overflow/underflow and asynchronous FP signals.
- [ ] Qualify any required restoration of the outer host C0/C1/C2/C3 bits.

FIP/FDP/opcode and dead source-register payloads have no retained-State ABI
fields and are outside its packet claim. Cross-thread scope use and
compiler-owned x87 temporaries spanning the scope are outside its contract.
