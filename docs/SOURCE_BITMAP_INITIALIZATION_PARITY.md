# Original texture-cache initialization

The standalone helper in `source_bitmap_initialization.hpp/.cpp` reproduces
the raw storage constructors used by original `12C0D0` and the physical-stage
reservation arithmetic before `12C1E0` enters protection. It takes explicitly
allocated storage and actual stage bounds. It does not allocate, protect,
activate a runtime allocator, submit a read, or mark an image ready.

## Constructor ownership and storage

Original `12560` calls `12D8B0` then `12C0D0` during graphics initialization.
`12C0D0` loads allocator interface `468758`, invokes its allocation entry, and
constructs these banks:

| Bank | Allocation bytes | Rows | Stride | Initial state |
| --- | ---: | ---: | ---: | --- |
| `4E6454`, xbox texture | `508C` | 512 | 40 | Flag 4, inactive |
| `4E6458`, xbox predicted texture | `6A8` | 200 | 8 | Flag 4, inactive |
| `4E6464`, xbox texture cache | `30FC` | 512 | 24 | Embedded active physical pool; page shift 12; capacity 0; state 2 |

The first two use complete original `16B5F0 → 16B650`. Their headers and
occupied bits are initialized, while row payloads and salt words remain
unwritten. The helper preserves those bytes. The manager uses complete
`13D170`, including `16B7A0` on its embedded pool at `+70`: it initializes
every row's first two bytes, the occupied bits, the pool salt, and the manager
callback/age/list fields. Its callbacks are literal `12D200`, `12D1A0`, and
`12D160`. The embedded pool allocator is actual `46875C`; the manager's own
allocator at `+6C` comes from its caller's `468758`. These are distinct
retained pointers (`479864` and `479860`).

The retained interface resolves through vtable `453308` to `11CA20`.
That routine calls `VirtualAlloc3312CC` with NULL address, actual size,
allocation type `101000` and protection 4. The complete unchanged
`12C0D0` prefix stops at actual `NtAllocateVirtualMemory8001EEAC`, reached
through IAT `4115F0`. No allocation return is supplied. Constructor fixtures
start at source post-allocation suffixes with explicit admitted storage;
they are not whole-kernel allocation replays.

`12C190` disposes the image pool and manager through their allocator's free
entry. That body does not dispose the predicted pool. This audit does not
invent missing teardown or host allocator policy.

## Physical-stage boundary

Original `12B400` zeroes the 52-byte physical-stage bank at `4E6420`, sets
stage 0 bounds to `61000..3145000`, then requests `30E4000` bytes through
`XPhysicalAlloc2D156A`. The unchanged prefix stops at actual kernel
`MmAllocateContiguousMemoryEx8001DC6C`. A fixed physical address in this
request does not establish a corresponding native allocation or budget.

Stage pushes, including `18EE60` and `18F100`, copy the current lower/upper
bounds to the next stage before advancing the index. Original `12C1E0`
uses signed wrapping difference and truncation toward zero to derive its
4096-byte page count. It rounds the requested bytes, subtracts them from
the upper bound, and performs the signed lower-bound test. The helper
preserves these operations, including zero, negative and overflow controls.
It returns the changed upper bound, page count, candidate physical address,
byte count and whether the source would call the protection wrapper.

The reservation corpus stops before `XPhysicalProtect2D15DA` when called,
or before the following manager-resize path at `12C253`. Original protection
uses `404` (read/write plus write-combine). A nonzero-size diagnostic reaches
actual `MmSetAddressProtect` without executing it. Later `13D8B0` can evict
blocks through original callbacks, then the source activates and resets image
and predicted pools. Those steps are outside the helper's reservation result.

The retained current stage is already consumed: stage 3 lower and upper are
both `66C000`, while the existing manager owns 4,737 pages at `8066C000`.
Repeating the complete source activation on this state derives zero pages,
passes the zero-byte protection wrapper without a kernel call, and reaches
the actual work-mutex wait during resize/eviction. It is a diagnostic repeated
activation, not proof of original first activation. Even the separately
supplied retained parent upper `1928000` yields 4,796 pages, not 4,737.
Intervening stage charges and source chronology therefore remain necessary
inputs; this helper does not reconstruct them from the final bank.

## Executed proof and activation limits

`tools/oracle_source_bitmap_initialization.py` executes unchanged disc code
against disposable clones of the immutable restored Zanzibar banks. The
frozen fixture contains **4,246 cases / 160,446 exact words**: 48 constructor
suffixes over independently poisoned admitted storage, and 4,198 stage
reservation inputs. All original called constructor/math bodies execute
without hooks that replace their results or instruction patches. The 547
distinct verified instructions, literal banks, service boundaries, source
hashes and explicit FPCW `023F` / MXCSR `1F80` are retained in
`analysis/multiplayer-shading/source-bitmap-initialization-original.json`.
Normal and fully instrumented ASAN/UBSAN tests pass, including six rejection
controls for missing allocation addresses/interfaces, undersized storage,
address overflow and unknown bank kind. Restored Unicorn banks are checked
against their independent initial bytes after all diagnostics.

The [async ownership audit](SOURCE_ASYNC_HANDLE_LIFECYCLE.md) separately proves
two owned mutexes, a 0/150 counting semaphore and one persistent worker.
Real native synchronization can represent those objects; observed signal
state cannot substitute for an actual acquisition result. File I/O callbacks,
completion, contention and worker teardown remain explicit services. The
[allocation/eviction prefixes](SOURCE_BITMAP_ALLOCATOR_BOUNDARY.md) and
[CPU image selector](SOURCE_BITMAP_CPU_SELECTION_PARITY.md) remain separate.
No CMake, runtime, renderer, GPU, guest or residency activation is changed by
this initializer proof.
