`source_bitmap_begin_pending` composes the admitted original `12D5A0` caller
over supplied texture-manager, physical-pool, image-pool, owner and async banks.
Its pending result stops at the actual mutex service. It executes no wait,
supplies no acquired node, and returns no completed caller success.

The original ABI takes the bitmap record in ECX and wait in AL, with partition,
force and priority on the stack, then returns with `RET 0Ch`. Any nonzero wait
or force byte selects reservation age 2; both zero select age 5. Only nonzero
wait with zero force replaces priority with 7. The native input preserves the
raw byte semantics and signed priority.

Admission requires the independently proved reservation state 2, page shift
12, positive signed resource size, separate addressed 512-slot pools, actual
owner records and represented physical storage. Every consumed header branch
is preflighted by the complete
[pending-header producer](SOURCE_BITMAP_PENDING_HEADER_PARITY.md). All supplied
metadata spans and the native queue storage identity must be disjoint both in
declared address space and in native storage. The supplied physical extent
must contain the addressable physical capacity and overlap none of those
metadata banks. These checks are native storage/address admission; rejection
does not represent an original failed request and commits no changes.

The implementation stages exact copies of the admitted raw metadata. Complete
source failure commits every preceding source failure write, including fence
clears, evictions and limit writes. A real work/free boundary likewise commits
every source-produced change before that boundary. It never restores banks to
pretend that the original failed or completed. The staged copies do not hide
aliases: admission examines the original supplied spans first.

After complete physical reservation, the caller performs complete same-salt
`16B910` image binding, preserving its exact zeroing, counters and salt
consumption. It stores partition at image `+4`, owner at `+10`, and the new
handle/address in owner arrays `+28/+40`; partition zero also updates owner
`+54`. Image `+3` remains zero and image flag bit 0 is cleared. Complete
`12D6D0`, including unchanged `359E0`, `1365A0` and `13E3A0`, writes Common,
Data, Lock, Format and Size at image `+14..24`. Shifted depth zero and literal
format-table `FFFFFFFF` entries retain their exact source arithmetic. Header
admission establishes no renderer format support.

The byte count is rounded to 512 with wrapping DWORD arithmetic. Complete
`213760` routing and the `1A0F10` request prefix select the supplied current,
main-menu, shared or single-player-shared file token using the resource
location's high two bits. The low 30 bits become the file offset. The buffer
is the source physical base plus the reserved page offset shifted by 12.
The bytes-output argument is NULL; the done address is image row `+3`, and the
category is 3. The caller's temporary read of owner `+4C` does not become the
bytes-output argument.

`1A0F10` increments requests-added at `1A0F5F` with wrapping DWORD arithmetic
before calling `1208B0`, which reaches the real free-list mutex wait. The result
exposes `NeedsFreeMutexAcquire`, the
actual mutex token and explicit read/request metadata. The original job counter,
all 150 nodes, free/work heads and loading counter remain unchanged. Image
priority `+8` and job ID `+C` remain zero from binding: `12D5A0` writes those
fields and increments loading only after `213760` returns. A free node in the
supplied bank does not establish mutex acquisition. The later work-mutex
publication and semaphore release are outside this prefix.

The context's `queue_address` describes the supplied native queue storage
identity and extent for alias admission. It does not assert that the typed
`SourceAsyncQueue` has the original contiguous field layout. The fixture
serializer maps the source counter at `4E0364`, nodes at `4E0368`, free head at
`4E28E8`, work head at `4E28EC` and requests-added at `4E28F0` independently.
Source NULL links map to the typed NONE sentinel; valid node pointers map to
their source ordinal and are reencoded on output.

The work-boundary result propagates the existing manager/reservation callback
boundary with its pending deletion handle, preserving preceding changes and
requiring the actual work-mutex input at that point. This whole-caller corpus
does not reach that branch. The separate
[manager callback proof](SOURCE_BITMAP_MANAGER_CALLBACKS.md) freezes its actual
wait prefix. Neither boundary is a retry-safe continuation: restarting the
function would not restore the original reservation/deletion control state.

The unchanged original corpus contains 248 complete callers or stopped
prefixes: 168 literal arguments from the preserved allocator report and 80
explicit conditional controls. The retained arguments cover 42 authored
partitions with four wait/force combinations, independently restored; they do
not describe 168 historical requests. The controls cover completed capacity
and signed-limit failures, linear and nonlinear header metadata, file selectors
and offsets, request-counter wrap and noncanonical boolean bytes. Exactly 242
cases stop before the first instruction of `NtWaitForSingleObjectEx` at
`80020F5A`; six complete the original false return. The actual wait argument
bank contains the free mutex, WaitMode 1 (UserMode), nonalertable and NULL
timeout. No wait instruction or result is supplied.

The frozen full-bank comparison checks 18,635,712 bytes across 75,144 bytes per
case, including every unchanged owner, row, occupied bit, pool header, async
node/head/counter and loading byte. It also checks 6,944 result words. Result
word index 26 (zero-based) is the future callback literal from source instruction `1A0F90`; that
store is not reached before the wait and is explicitly labeled source-inspected.
The independent 680 owned `1A0F75` binding cases in the
[async read proof](SOURCE_ASYNC_READ_PARITY.md) establish that later store.
Other result words encode observed original arguments/task/boundary data or
explicit absent-value sentinels.

The oracle verifies 1,112 distinct reached title instructions against the
pinned XBE and restores and reads back 112 loaded original pages. All source
nonstack writes are confined to the exact raw output banks; controlled data
ranges remain disjoint from reached code. FPCW `023F` and MXCSR `1F80` are
explicit supplied execution controls. No instructions or services are replaced.
The physical extent is existing immutable snapshot storage, retained by address,
size and hash; this prefix never reads, allocates, initializes or writes its
pixels. Capacity, clock, fences, busy flags, mutexes, files and queue state are
explicit inputs, without a native budget, pressure or scheduling producer.

The normal and fully instrumented ASAN/UBSAN tests with leak detection pass the
mandatory 248-case corpus and 56 admission/phase controls. Each rejection
preserves every supplied raw bank, queue field and span identity. The test
requires the exact case/byte totals and observed phases `{6,0,242}`; missing,
empty, comment-only and last-row-truncated corpora are independently rejected
by both normal and sanitized binaries. No whole-caller work-boundary case is
invented to fill the unobserved phase.

The new API and proof remain standalone, with no renderer, GPU, runtime,
production activation or CMake integration. A separately supplied actually
owned-node continuation requires its own unchanged-source proof and is not
implemented here.

Evidence is retained in
[the whole-caller report](../analysis/multiplayer-shading/source-bitmap-pending-original.json),
[the mandatory fixture](../tests/fixtures/retail_source_bitmap_pending.tsv),
[its provenance](../tests/fixtures/retail_source_bitmap_pending.provenance.json),
and `tools/oracle_source_bitmap_pending.py`. The original pure producers and
their standalone proofs remain separate:
[reservation](SOURCE_BITMAP_RESERVATION_PARITY.md),
[image binding](SOURCE_BITMAP_IMAGE_BINDING_PARITY.md),
[pending header](SOURCE_BITMAP_PENDING_HEADER_PARITY.md), and
[read routing](SOURCE_BITMAP_ASYNC_READ_PARITY.md).
