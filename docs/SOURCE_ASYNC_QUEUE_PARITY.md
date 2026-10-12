# Original asynchronous queue ordering

The queue proof executes complete original `120900` and `1A1720` without
platform services. It also executes the complete queue mutation/search bodies
inside five callers, starting at their literal instructions after mutex
acquisition and stopping before mutex release. Those bodies admit an explicitly
owned node bank. They do not establish that an original wait succeeded.
Three further phases cover the complete node-bank initialization prefix and
explicitly owned free-list take/return bodies. Initialization stops before the
first mutex creation entry. The free-list bodies stop before mutex release.

The original executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
Memory comes from the immutable
`analysis/multiplayer-shading/registration-snapshot-restore-20261008` export.
The oracle verifies every reached title instruction against the executable,
restores dirty Unicorn pages between cases, and finally checks the current
emulator's node bank, head and counter against the original snapshot. It
replaces no callees and supplies no kernel status, wait result, release result,
read result or worker completion.

## Node fields and retained bank

The 150 nodes start at `4E0368`, each occupying `40` hexadecimal bytes.

| Offset | Meaning |
| --- | --- |
| `00` | Signed 32-bit priority |
| `04` | Raw 32-bit request ID/state |
| `08..2F` | Forty-byte task payload |
| `30` | Category |
| `34` | Callback pointer |
| `38` | Done pointer |
| `3C` | Next node pointer |

For a read task, the file handle is at node `+08`, and the current offset is at
node `+14`. The work head is `4E28EC`; request counter `4E0364` is `1A16` in
the retained bank. The free head is `4E28E8`, and the requests-added counter is
`4E28F0`. All 150 retained node states are `FFFFFFFF`; the work head
is null. There are 69 literal `1A0E70` read callbacks and 81 zero callbacks.
Their task payloads are retained even though their states mark them free.
Insertion controls explicitly lease these retained records and supply queue
membership; they do not claim that a new acquisition happened in the snapshot.

## Exact ordering

`1A1720` receives incoming priority in `EDX`, compared priority in `ECX`,
incoming task pointer in `EDI`, and compared task pointer in `EBX`. Its three
stack arguments are the insertion-state pointer, incoming callback and
compared callback. It returns the Boolean in `AL` and uses `RET C` hexadecimal.

Priorities compare as signed 32-bit integers in descending order. At equal
priority, the literal read callback `1A0E70` precedes other callbacks. Two
nonread tasks do not reorder through this comparator. Two reads compare only
when the state valid byte is nonzero. Their file-handle differences from the
state file are unsigned 32-bit differences, including wraparound. Equal file
differences compare unsigned offset differences from the state offset. Equal
distances return false.

After every comparison against a read callback, the state file and offset
become that compared read's values and the low byte of the third state word
becomes one. Its upper three bytes remain untouched. This update happens even
when the priorities differ. These routines contain no floating-point ordering;
controls carrying floating-point NaN encodings are signed/raw integer inputs.

`120900` initializes all 12 insertion-state bytes to zero on each call. An
empty queue accepts the new head, and a strictly higher priority can pass an
existing head. Otherwise the first comparison is against `head.next`. The
existing head is never passed by an equal-priority task, and it does not seed
the rotating read state. A comparator result true inserts before the compared
node; otherwise traversal continues and eventually appends the incoming node.

For equal-priority reads of the same file with existing offsets
`900 -> 100 -> 300`, incoming offset `200` produces
`900 -> 100 -> 200 -> 300`. Incoming `50` and `400` both append after `300`.
The report freezes the complete original comparison sequence and state before
each comparison for these controls.

## Request identity and owned critical sections

| Fixture kind | Original slice | Result |
| --- | --- | --- |
| `0` | Complete `1A1720` | Boolean and all 12 state bytes |
| `1` | Complete `120900` | Head and complete node bank |
| `2` | `1209D1` to before `120A08` | ID, head, counter and complete node bank |
| `3` | `120CF0` to before `120D3C` | Match Boolean, head, counter and complete node bank |
| `4` | `120C00` to before `120C18` | Count and unchanged bank |
| `5` | `120B60` to before release entry `2D62C4` | Category Boolean and unchanged bank |
| `6` | `120A3D` to before release entry `2D62C4` | Updated head/bank and cleared removed ID |
| `7` | `120A90` to before first CreateMutex entry `2D626C` | Complete initialized bank, both heads and both preserved counters |
| `8` | `1208C0` to before release entry `2D62C4` | Taken-node index or availability boundary, both heads/counters and full bank |
| `9` | `120CA2` to before release entry `2D62C4` | Updated free head/returned next and complete bank/heads/counters |

The enqueue core receives the node pointer in `ESI`, invokes complete `120900`,
stores `(counter << 8) | node_ordinal`, increments the counter with 32-bit wrap,
and leaves the ID in `ESI`. For admitted ordinals `0..149`, the high counter
byte is discarded by the shift. No semaphore notification or whole-caller
return is represented by this slice.

The priority core receives the ID in `ESI`, starts with `BL=0`, and reads the
new priority at `ESP+8`. The low eight ID bits select the node; its complete
32-bit state must match the supplied ID. A mismatch returns false before queue
traversal. A match unlinks that queued node, updates its priority, calls complete
`120900`, and returns true. It preserves the ID, counter, task, category,
callback and done fields. No semaphore count change occurs in this body.

Count starts with `ESI=0`; category search starts with `ESI=category, BL=0`.
Unlink receives the known queued node in `ESI`, changes the work head or its
predecessor's next pointer, and clears its ID. The removed node's own next
pointer is preserved. Matching IDs for detached nodes, corrupted links and
out-of-bank ordinals are unsupported source inputs; the native helper rejects
them instead of fabricating a traversal outcome.

Initialization zeroes all 9,600 node bytes, sets each state to `FFFFFFFF`, links
nodes in increasing ordinal order, and clears the final next pointer. It sets
the free head to node zero and clears the work head. Neither the ID counter nor
the requests-added counter is initialized by this prefix. It stops at
`2D626C` with three zero arguments for the first `CreateMutex` call; it does not
create a mutex, supply its handle, or reach the later semaphore/worker creation.

The free-list take body starts at `1208C0` with `ESI=0`, after the source wait
instruction. With a nonnull free head, it takes that node into `ESI`, advances
the free head to its next pointer, and clears the taken node's next pointer.
With an empty free list, it leaves `ESI=0` and the complete bank unchanged. The
fixture reports that empty phase as `FFFFFFFF`, an availability boundary. The
whole original `1208B0` would release its mutex, yield and retry; the fixture
does not claim it returns an empty result.

Return-free starts at `120CA2` with `ESI` pointing to an explicitly unlinked
node. The source worker reaches this phase only after `120A30` removed that
node and cleared its ID, followed by the free-list wait. The admitted node has
state `FFFFFFFF` and is absent from both lists. The body stores the old free
head in its next pointer and publishes it as the new free head. All other node
fields, the work list and both counters remain unchanged. Its old next pointer
may still point into the work list because original unlink preserves that
field; return-free overwrites it. Native admission checks list membership as
well as state before mutation. These checks require the explicit exclusive
bank supplied to this proof; original worker concurrency is not reconstructed.

Whole caller prefixes `1209C0`, `120CE0`, `120BF0`, `120B50` and `120A30` all
reach actual kernel entry `80020F5A`, `NtWaitForSingleObjectEx`. The retained
arguments are handle `8`, WaitMode `1` (`UserMode`), alertable false, and null
timeout. The oracle stops before executing the first kernel instruction. The
work-mutex release and enqueue semaphore release remain separate services.
The whole free-list caller `1208B0` independently stops at the same kernel wait
with retained handle `4` and the same three remaining arguments.

## Frozen validation

`tools/oracle_source_async_queue.py` generates
`tests/fixtures/retail_source_async_queue.tsv`, its adjacent provenance JSON,
and `analysis/multiplayer-shading/source-async-queue-original.json`.
There are 6,483 cases with 3,623,481 original output words:

- 3,322 complete comparator controls, including signed edges, equal priorities,
  callback ties, valid-byte/padding preservation and unsigned modular distances.
- 673 complete insertion controls, including all 150 retained records, the
  protected head and rotating read order, and explicit descending-priority banks.
- 313 enqueue-ID controls, covering all node ordinals and counter truncation/wrap.
- 512 priority controls, including stale generations; 256 unlink controls.
- 151 count controls from zero through 150 nodes and 453 category controls.
- 200 initialization prefixes, including poisoned node bytes, all 150 incoming
  work-head positions and a seven-by-seven counter-preservation edge matrix.
- 302 owned free-list takes, including empty availability, the retained bank,
  and every node ordinal as both a full-chain and a single-node free head.
- 301 post-unlink returns, covering every returned ordinal and free-head
  position, empty free lists and disjoint nonempty work/free lists.

The fixture normalizes only next/head pointers to node indices or `FFFFFFFF`.
All other node words retain their original bits, so unknown task bytes and done
tokens are compared as well. The first output word for void slices is an
artifact placeholder zero, not a synthesized original return register.
`tests/source_async_jobs_tests.cpp` compares the fixture through
`source_async_before`, `source_async_insert`, `source_async_publish`,
`source_async_reprioritize`, `source_async_count`, `source_async_has_category`
and `source_async_unlink`, plus `source_async_initialize`,
`source_async_take_free` and `source_async_return_free`. The three ownership
phase kinds use a separate eight-word input header:
`kind/workHead/nodeIndex/freeHead/counter/150/requestsAdded/0`, followed by all
150 raw node records. Their output header is
`result/workHead/counter/freeHead/requestsAdded`, followed by all 150 records.
Twenty-one separate admission controls cover missing nodes, cycles, missing
queue membership, free-list leases, queued returns with state `NONE`, active-ID
returns, empty availability and the stale-ID shortcut. The proof covers 312
verified title instruction addresses. Supplied
floating-point controls are `FPCW=023F`, `MXCSR=1F80`; they are execution
configuration, not an inference about live thread controls.

Normal fixture mode additionally requires exactly 6,483 cases and 3,623,481
checked words before reporting success. Independent file controls verify that
a missing fixture, an empty/comment-only fixture, and the actual fixture with
its final row removed are rejected. The last case has 6,482 matching rows and
3,621,076 words, and therefore cannot silently pass as the complete corpus.
Probe mode remains available for the original oracle's explicit input rows.

Queue helpers require actual exclusive ownership from their caller. This proof
does not activate a queue, execute file I/O, start a worker, establish a kernel
notification timeline, or prove bitmap readiness. Startup and handle resolution
have their separate boundary in `SOURCE_ASYNC_HANDLE_LIFECYCLE.md`.
