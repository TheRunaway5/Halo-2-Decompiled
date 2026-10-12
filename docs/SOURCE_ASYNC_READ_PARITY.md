# Original asynchronous request and read protocol

`source_async_jobs.hpp/.cpp` represents the title's 150-node queue, request
binding, read-task arithmetic and callback completion decisions. Actual list
ownership, semaphore waits/signals and file I/O remain caller services. The
module is standalone and does not activate native bitmap residency.

The authority is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
and the immutable
`analysis/multiplayer-shading/registration-snapshot-restore-20261008` memory
export. The oracle verifies every executed title instruction against the
executable, restores modified cloned pages between cases, and checks all
fourteen loaded original pages after the run. It performs no guest I/O or
guest advancement and supplies no kernel return values.

## Request construction and leased-node binding

Generic request `120BA0` increments `4E28F0` and clears the actual nonnull done
byte before it calls free-node acquisition `1208B0`. Read request `1A0F10`
constructs a zeroed forty-byte task and performs the same increment/clear.
Its register arguments are buffer in `EDX` and done pointer in `EBX`; the six
stack arguments are file handle, size, offset, category, priority and optional
cumulative-byte-output pointer.

| Task word | Read meaning |
| --- | --- |
| `0` | Actual file handle |
| `1` | Buffer address |
| `2` | Total requested bytes |
| `3` | Current file offset |
| `4` | Optional cumulative-byte-output pointer |
| `5` | Bytes processed so far, initially zero |
| `6..9` | Initially zero; retained by this read callback |

`source_async_begin_request` returns the done token that the owner must clear
before acquiring a node. It increments the source counter with 32-bit wrap,
and does not acquire storage or publish a task. Null done storage is rejected:
both original request prefixes dereference it unconditionally.

With an explicitly leased node, complete binding cores `120BB5` and `1A0F75`
copy all ten task words into node `+08..2F`, then write priority `+00`, category
`+30`, callback `+34` and done token `+38`. They preserve node ID `+04` and link
`+3C`. The read binding uses literal callback `1A0E70`. The oracle enters these
as distinct admitted ownership phases and stops at `1209C0`; it does not make
`1208B0` return. Queue publishing, request-ID generation and priority order are
proved separately in [SOURCE_ASYNC_QUEUE_PARITY.md](SOURCE_ASYNC_QUEUE_PARITY.md).

## Read callback and supplied I/O outcomes

`1A0E70` computes unsigned remaining bytes as `total - processed`, selects the
minimum with actual `46E488`, and always calls `SetFilePointer`. The retained
chunk limit is `262144` bytes. The source supplies the current low offset,
zero high offset and `FILE_BEGIN=0`, even for a zero-size request.

`source_async_read_begin` yields these exact file/offset/buffer/size inputs.
The buffer address is `base + processed`, with 32-bit wrap. The owner must use
the prepared step and its actual I/O outcome for `source_async_read_finish`.
The ReadFile Boolean return is not consumed by the source callback. The source
uses the bytes-output DWORD instead.

If the low seek result is `FFFFFFFF`, no `ReadFile` call occurs. The callback
does not change the task's processed count, current offset or external byte
output. A supplied nonzero read count on that branch is rejected. Otherwise,
the returned byte count is added to processed and offset, and is written to
the optional cumulative-output location.

The callback returns zero exactly when `total != processed` and the requested
chunk is no larger than the supplied byte count. Every other branch returns
one. Consequently, a short read or a failed seek can complete the job; the
done byte is not evidence that all requested bytes were transferred. These
are literal arithmetic/control results, not invented success statuses.

## Worker completion and ownership order

Original `120C40` waits on the semaphore, takes the work mutex, selects the
current work head, releases that mutex, stores the selected pointer at
`510810`, then invokes its callback outside both list mutexes. The pointer is
an actual selected node, not an independently reconstructed priority choice.

The complete post-callback decision at `120C7C` is:

- Result zero: retain the node and request a semaphore increment of one.
- Result exactly one with nonnull done storage: write byte one before unlink.
- Any nonzero result: unlink under the work mutex, then return the node to the
  free list under the free mutex. Nonzero results other than one do not write
  readiness.

`source_async_callback_effect` preserves these decisions. The owner must apply
them in this order with actual services. `source_async_unlink` clears the job
ID while retaining the removed node's link, and `source_async_return_free`
then replaces that link with the current free head. The native admission
checks require the node to be absent from both lists before free-list return;
an ID of `NONE` alone cannot prove removal because pure insertion does not
assign an ID. Queued-head and queued-interior controls prove that invalid
return requests are rejected before either list changes.

## Original execution and frozen tests

`tools/oracle_source_async_read.py` generates
`tests/fixtures/retail_source_async_read.tsv`, its adjacent provenance file,
and `analysis/multiplayer-shading/source-async-read-original.json`. There are
8,706 cases and 112,058 exact original output words:

- 680 read request prefixes, including 168 literal allocator requests, and
  their 680 distinct leased-node binding controls.
- 256 generic request prefixes and 256 generic leased-node binding controls,
  preserving unknown payload bits and node ID/link words.
- 1,132 complete read-step argument producers, including zero size, unsigned
  wrap and the retained chunk limit.
- 5,660 explicit post-I/O arithmetic controls for full, partial, zero and
  failed-seek outcomes. These outcomes are supplied controls, not observed
  original I/O timing.
- 42 complete post-callback decision controls for zero, one, other nonzero,
  signed edge values and null/non-null done tokens.

The proof verifies 205 title instruction addresses. Floating-point execution
controls are explicitly `FPCW=023F`, `MXCSR=1F80`; these integer routines do
not infer a live thread control state. The normal fixture test and fully
instrumented AddressSanitizer/UndefinedBehaviorSanitizer test, with leak
detection, also check eleven independent lifetime/ownership controls. No
service callee is replaced and no original instruction is patched.

Whole-caller diagnostics remain separate from the completed arithmetic:
`1A0F10` reaches real `NtWaitForSingleObjectEx` entry `80020F5A` for the free
mutex handle `4`; `120C40` reaches that entry for semaphore handle `16`.
Their retained arguments are WaitMode `1` (`UserMode`), alertable false and
null timeout. `1A0E70`, constructed from an actual allocator request, reaches
real file kernel entry `80016F61` through its unchanged `SetFilePointer`
wrapper. Each diagnostic stops before the first kernel instruction.

The missing native activation service must own real image/cache allocations,
file-handle mapping, completed reads, list synchronization and semaphore
notification. It must also connect actual pressure/budget/coverage requests to
bitmap manager state. The proved title protocol permits testing those owned
services; it does not establish their scheduling or make current bitmap
readiness appear automatically. See
[SOURCE_ASYNC_HANDLE_LIFECYCLE.md](SOURCE_ASYNC_HANDLE_LIFECYCLE.md),
[SOURCE_BITMAP_INITIALIZATION_PARITY.md](SOURCE_BITMAP_INITIALIZATION_PARITY.md)
and [SOURCE_BITMAP_ALLOCATOR_BOUNDARY.md](SOURCE_BITMAP_ALLOCATOR_BOUNDARY.md)
for the independently retained construction and allocator boundaries.
