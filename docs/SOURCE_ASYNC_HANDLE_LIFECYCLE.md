# Original async wait-object ownership

The allocator's `NtWaitForSingleObjectEx` boundary belongs to the title's
asynchronous job queue. Original `120A90` creates two mutexes, one counting
semaphore and one persistent worker. These are concrete owned objects, rather
than an unexplained ready flag. A native provider can own corresponding real
synchronization objects and a worker, while preserving the title's queue and
callback protocol. This audit does not execute that provider or establish host
scheduling parity.

The unchanged audit is
`tools/audit_source_async_handle_lifecycle.py`. Its report and the two newly
inspected pinned compatibility files are in
`analysis/multiplayer-shading/source-async-handle-lifecycle/`.

## Startup and retained ownership

`12000` calls `120A90` at `12040` during title initialization. `120A90` clears
150 nodes of 64 bytes at `4E0368`, sets every node state at `+4` to NONE,
links their `+3C` fields, terminates the final link and sets the free/work heads
at `4E28E8` / `4E28EC`. It does not reset the task-ID counter at `4E0364`.
The next source calls are:

| Global | Original producer | Literal creation inputs | Retained handle |
| --- | --- | --- | --- |
| `4E0358` | `2D626C` / `NtCreateMutant` | Unnamed, initially unowned | 4 |
| `4E035C` | `2D626C` / `NtCreateMutant` | Unnamed, initially unowned | 8 |
| `4E0360` | `2D61EB` / `NtCreateSemaphore` | Unnamed, initial count 0, maximum 150 | 16 |
| `4E0354` | `2D21C6` / `PsCreateSystemThreadEx` | `120C30`, stack `4000`, NULL parameter, not suspended | 24 |

`120B4A` then calls the thread-priority wrapper with relative priority 1.
The thread wrapper also passes the retained XAPI TLS-size global `55F138`
(literal value `0x94`) and
system routine `2D212E`. That routine initializes thread TLS, calls the supplied
start routine and would terminate a returning thread. The actual worker never
returns from its source loop.

The retained kernel handle table resolves handles 4 / 8 to KMUTANT objects
`D00084D8` / `D0008538`, with dispatcher type 2, signal state 1 and no owner.
Handle 16 resolves to KSEMAPHORE `D00081D8`, dispatcher type 5, count 0 and
limit 150. Handle 24 resolves to thread object `D0067358`. The report retains
their exact headers, type pointers, dispatcher addresses and bytes. These
numeric handles are captured inputs; future native handles are not prescribed.

## Queue and worker protocol

The free-list mutex protects node removal in `1208B0` and node return in
`120C40`. When the free list is empty, the producer releases the mutex,
yields through `2D2084`, and retries. The work-list mutex protects insertion,
removal, category/count checks and priority changes. `1209C0` creates the job
ID from `(counter << 8) | node_index`, increments the counter, releases the
work mutex and releases one semaphore count. Ordering uses original
`120900` and its `1A1720` comparator; a generic host priority queue does not
by itself recover the comparator's tie rules.

Worker `120C30 → 120C40` repeatedly waits on the semaphore, acquires the work
mutex, takes the current work head, releases the mutex, and calls the node's
callback with its copied task at node `+8`. The callback runs outside both
list mutexes. Callback result 0 releases one semaphore count and retries.
Any nonzero result removes the node, sets its state to NONE, and returns it
under the free-list mutex. Only result 1 with a nonnull node `+38` pointer
stores done=1 at `120C8C`; another nonzero result does not manufacture done.
File callback execution and the bitmap readiness producer remain separate
from synchronization-object ownership.

The wait wrapper supplies `[Handle, WaitMode=1, Alertable=0, Timeout=NULL]`.
The second argument is **KPROCESSOR_MODE / UserMode**, not a wait reason.
Retained kernel `80020F5A` separately passes wait reason 6 / UserRequest to its internal
`KeWaitForSingleObject` call. No acquired status follows merely from a retained
mutex signal state or an existing node.

## Shutdown and compatibility boundary

Original `12080` performs other title cleanup, then closes worker, semaphore,
work mutex and free mutex in that order at `120D0..120F9`, using
`2D1F3A → NtClose`. That body does not clear these four handle globals,
signal cancellation, join the worker or terminate the persistent loop.
A native shutdown/join policy therefore needs its own stated ownership
boundary; it cannot be attributed to this title cleanup body.

Pinned primary Cxbx compatibility source uses real native NT objects for
mutants and semaphores. Its worker construction owns an ETHREAD plus a native
proxy thread; the wait implementation maps object-manager thread handles to
native handles and handles wait mode/APCs. This supports the representation
seam, with distinct limits: these files were inspected, not executed; they are
compatibility implementations, not the captured Xbox kernel. The pinned
`NtReleaseMutant` body returns a constant success despite invoking the native
release, so that return cannot be adopted as proof of an actual successful
release. See the pinned
[synchronization implementation](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlNt.cpp)
and
[thread construction](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/585c49a50af1255ab155099e06f24505f9c5a800/src/core/kernel/exports/EmuKrnlPs.cpp).

## Executed evidence

The audit executes **15 unchanged title prefixes**, verifying 118 distinct
disc instructions. Startup initializes all 150 nodes and stops before the
first real `NtCreateMutant` instruction. Independent literal constructor
wrappers, retained owner waits, releases, priority and closes stop at their
actual kernel entries. No handle store from a completed constructor, kernel
status, wait success, release success, queued job ID or readiness is supplied.

A separate **19-case** run executes only the captured kernel's pure,
nonreferencing handle lookup `800215C4`, verifying 16 distinct instructions.
It resolves the four actual handles and their low-two-bit aliases, and returns
NULL for three unused/out-of-bank controls. It performs no writes, object
reference, interrupt-level change, acquisition or scheduling. The actual
protected lookup and wait bodies are retained as disassembly, not executed.

The RAM, manifest, source regions, imports, tool and primary-source hashes
are frozen in the report. Every mutated title bank is restored and checked.
FPCW `023F` / MXCSR `1F80` are supplied execution inputs. No GPU replay,
guest advancement or production/CMake change is made.

The minimum implementation seam is an owned synchronization/worker provider
with real statuses and source queue/callback ordering. Async file services,
callback completion, object creation failures, contention and teardown stay
explicit. Cache memory budgets, physical reservation pressure, fence state
and maintenance are independently supplied services; none is inferred from
this worker lifecycle. The earlier
[allocator boundary](SOURCE_BITMAP_ALLOCATOR_BOUNDARY.md) remains intact.
