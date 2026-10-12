# Original bitmap allocation boundary

The independent unchanged-executable audit is
`tools/audit_source_bitmap_allocator_snapshot.py`, with report
`analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json`.
It uses the immutable restored Zanzibar RAM, original page tables, exact
physical/image manager pools, occupied bits, linked lists, bitmap records,
asynchronous task banks, and retained fence RAM. Caller wait/force/priority
and a disposable stack are explicit inputs. No busy callback, allocation,
I/O, kernel result, readiness, or instruction is replaced.

The 64 literal CPU-sampler records contain 42 eligible missing hardware
partitions: four tier 0, twenty tier 1, eighteen tier 2. Each is called with
all four wait/force combinations, giving 168 diagnostic `12D5A0` prefixes.
These are requests on retained state, not proof of their original scheduling.

All 168 execute complete `13D370` physical reservation and its original
candidate/age/overlap/eviction logic. Original busy callbacks `12D1A0 →
3FB710` use the retained GPU fence RAM; no hardware kick/wait entry or MMIO
access is reached. Across the probes, 432 original `12D200` deletions finish.
Two independent direct `13D370` controls request one byte beyond the actual
retained capacity and return NONE, including the source age-limit update.
The separately registered state callback `12D160` is not called by these
verified `13D370` paths; its registration is not execution evidence.

`12D5A0` then constructs pending image state and the complete `12D6D0`
header. It writes the actual salted handle into bitmap `+28`, addresses at
`+40` (and `+54` for partition 0), and clears image readiness `+3`.
The physical address is `(block+8 << manager+34) + 4E6460`. The manager's
actual 4,096-byte page size, capacity, ages and lists are retained input;
they are not a native budget recommendation. The reservation age is 2 when
wait or force is true, otherwise 5. The separate timed-pump allocator
`12D400` is not called.

Every probe reaches `213760 → 1A0F10 → 1208B0`, then stops at kernel entry
`80020F5A`. Pinned IAT `411654` is export ordinal EA, independently named
`NtWaitForSingleObjectEx`; original wrapper `2D62E4` supplies mutex handle 4,
WaitMode 1 (UserMode), alertable 0, timeout NULL. The kernel supplies wait
reason 6 separately. This is the free-job-list mutex wait,
before a queue job node is consumed. The task prefix has incremented the
source task counter and cleared done; no queued job ID or successful return
is invented. Final priority/load-counter stores and the `12D5A0` return have
not happened.

All 168 newly constructed readiness bytes remain zero. Original asynchronous
worker `120C79` calls the job callback; only its return 1 with a nonnull done
pointer stores done=1 at `120C8C`. The actual read callback `1A0E70` depends
on file seek/read kernel services and worker scheduling. An allocated image
or matching header cannot stand in for completed bytes.

The report verifies 1,034 original instructions, ordered busy outcomes,
source writes, pending headers, 170 call results/boundaries, restored bank
hashes and exact ABIs. Supplied FPCW `023F` / MXCSR `1F80` are explicit CPU
execution inputs. No GPU replay or guest advancement is performed.

The minimum port seam is physical reservation/eviction over explicit manager
and pool banks plus the actual busy producer, followed by pending image/header
state. Async job submission and readiness completion remain separate services.
The current native renderer's memory budget, residency pressure, fence state,
file completion order and cache maintenance are not inferred from this one
retained manager. This audit installs no native allocator or image-selection
policy. The complete [CPU selector](SOURCE_BITMAP_CPU_SELECTION_PARITY.md) and
[coverage input](SOURCE_BITMAP_COVERAGE_PARITY.md) remain independently tested
standalone services.

The later [startup/handle audit](SOURCE_ASYNC_HANDLE_LIFECYCLE.md) establishes
the actual owned mutexes, counting semaphore and persistent worker. The
[storage/physical-stage initializer](SOURCE_BITMAP_INITIALIZATION_PARITY.md)
proves constructor bodies after explicit allocation admission. Neither closes
this allocation caller's kernel wait, file completion or readiness boundary.
The later [queue](SOURCE_ASYNC_QUEUE_PARITY.md) and
[request/read protocol](SOURCE_ASYNC_READ_PARITY.md) prove title-side decisions
over explicit ownership and I/O outcomes. They do not manufacture the kernel
returns or original completion timeline missing from this whole-caller audit.
