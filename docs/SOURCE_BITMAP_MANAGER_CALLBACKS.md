# Source bitmap manager callbacks and metadata release

`source_bitmap_manager_transition.hpp/.cpp` implements the bounded image
callbacks registered by `12C0D0` and the complete `13D830` deletion caller.
The interface consumes explicitly owned raw image/physical banks, owner
records, and actual readiness/fence inputs. It allocates no storage, sets no
ready byte, and represents no original kernel acquisition or GPU completion.

## Registration, lookup, and inputs

The source manager stores delete `12D200` at `+20`, busy `12D1A0` at `+24`, and
state `12D160` at `+28`. Each callback takes one DWORD handle at `ESP+4` and
returns with `RET 4`. Image lookup uses only `handle & FFFF`, with a 40-byte
row in the pool's `+44` bank. The state and busy callbacks perform no salt or
occupied-bit comparison. The fixture independently covers 108 query calls
with the same low index and different upper salts. Native queries preserve
this lookup rule. Deletion admission separately requires an actual occupied,
matching salted slot; this is an ownership boundary, not an invented check
inside the source callback.

The raw image row contains the following consumed fields:

| Offset | Source meaning |
| --- | --- |
| `+0` | Pool salt, WORD |
| `+2` | Flags byte |
| `+3` | Actual done/readiness byte |
| `+4` | Resource partition index |
| `+C` | Async job ID, or `FFFFFFFF` |
| `+10` | Owner bitmap record pointer, or zero |
| `+14` | Embedded resource header, beginning with Common |
| `+1C` | Resource Lock/fence word |

`12D160` returns AL `10` when flags bit zero is set, otherwise AL `20` when
the partition DWORD is nonzero, otherwise zero. The complete machine EAX
also retains `((index*5) & FFFFFF00)`. The oracle records and verifies that
full register formula for all 724 state cases. The native typed API returns
the registered byte contract; its fixture compares AL rather than claiming
a native machine-register ABI.

`12D1A0` reports busy for flags bit zero, or flags bit one with actual
`4E6479` equal to zero. After those branches, job ID `FFFFFFFF` reports clear;
a present job with a zero ready byte reports busy. A present job with a
nonzero ready byte executes `3FB710` on the actual resource header. Readiness
is a byte test, so explicit controls include noncanonical nonzero values.

For the proved texture header class, Common bits `780000` report busy.
Otherwise Lock zero reports clear. For a nonzero Lock, the source unsigned
comparison is `queued-Lock < queued-completed`, with DWORD wrap. A clear
result writes Lock zero; a busy result preserves it. The restored bank uses
device `00404FE0`, queued `001B082D`, completion pointer `83FDF000`, and
completed `001B0815`. These are retained RAM inputs, not inferred timing or
supplied service results. The fixture exercises the actual fence values and
all actual resource Lock values. Parent/surface resource headers and other
unproved header classes are explicitly rejected by the standalone helper.

The audited generic manager range `13D170..13D9EE` calls the busy and delete
pointers. It contains no call to the registered state pointer. Registration
does not establish an allocator state-callback invocation.

## Complete deletion and the first service boundary

`12D200` skips its busy/owner path when owner is zero. For a present owner and
a nonbusy image, it writes owner `+28[partition] = FFFFFFFF` and
`+40[partition] = 0`. Partition zero additionally clears owner `+54`, `+50`,
and `+70`. It then executes complete `16BA40` on the image pool.

`13D830` takes the actual manager in EDI and handle in EBX. It invokes
the delete callback before touching the physical list. A completed callback
is followed by updates to physical row `+C` next and `+10` previous links,
manager `+3C` head and `+40` tail, and complete `16BA40` on the physical pool.
The native helper validates the complete admitted occupied/salted physical
chain before mutation. The fixture compares all list, salt, occupied-bit,
counter, owner, resource, and otherwise untouched bytes.

For admitted nonpoison pools, `16BA40` clears the occupied bit and salt WORD,
lowers the next free index when required, shrinks the high-water mark over
zero salts when deleting its last slot, and decrements the occupied count.
It preserves the remaining row bytes. The source debug-poison pool flag
`+2A` bit three takes a separate body and is explicitly rejected here.

For an actual busy row, `12D200` reaches the work mutex at `12D277`, then
unchanged handle resolution reaches `NtWaitForSingleObjectEx` at `80020F5A`.
The retained arguments are handle 8, WaitMode 1 (`UserMode`), alertable false,
and null timeout. The oracle stops before the first kernel instruction.
The helper returns `NeedsWorkMutexAcquire` without releasing the owner,
image, or physical metadata. It does not fabricate a wait result, count the
queue under an unowned mutex, execute `3FAD50`, or claim that retrying became
nonbusy. The wait/count/release/hardware-wait/retry loop remains a real
service boundary.

## Connection to actual read completion

`12D5A0` clears image `+3` and passes that actual byte address as done storage
to `213760 -> 1A0F10`. Its return value is the async job ID stored at image
`+C`; the decompiled `hardware_format` name does not identify that field.
The worker writes one to nonnull done storage at `120C8C` only after the
actual callback returned one. The independently proved read/job arithmetic
and explicit native file outcomes do not establish an original I/O or worker
timeline. This transition helper consumes the supplied actual ready byte;
it never produces a completed image or changes that byte.

## Frozen original comparison

The original executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The immutable bank is
`analysis/multiplayer-shading/registration-snapshot-restore-20261008`, RAM
SHA-256 `ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55`.
Supplied FPCW `023F` and MXCSR `1F80` are execution controls, not a claim about
live thread settings.

`tools/audit_source_bitmap_manager_callbacks.py` runs unchanged callbacks
and the complete unchanged deletion caller. It verifies 257 title
instruction addresses, restores and checks 95 loaded pages, patches no
instructions, and replaces no services. The retained banks contain 286
occupied paired image/physical slots. Each deletion kind completes all 132
actual nonbusy rows and stops all 154 actual busy rows at the first real
wait. Two additional explicit ownerNULL cases per deletion kind prove that
conditional callback branch without claiming a historical borrowed-buffer
producer.

The mandatory fixture has 2,024 cases: 724 state, 724 busy, 288 image delete,
and 288 complete manager delete. Of these, 1,716 complete and 308 stop at the
real kernel boundary. Query controls include 768 explicit flags, partition,
job, ready, and override inputs. They prove conditional source behavior and
do not invent a readiness producer or original completion sequence.

Normal and fully instrumented AddressSanitizer/UndefinedBehaviorSanitizer
tests with leak detection compare 131,681,440 raw bank bytes and exercise
nine independent admission/shortcut/boundary controls. The non-probe test
requires both exact case and checked-byte counts. The oracle separately
verifies that missing, empty, and last-case-truncated fixtures are rejected.
Raw `B` records freeze every bank and owner; each `K` case reconstructs its
full expected bank from its actual conditional input and exact source output
patches, preserving all unchanged bytes.

The frozen corpus is
`tests/fixtures/retail_source_bitmap_manager_transition.tsv`, with adjacent
provenance and
`analysis/multiplayer-shading/source-bitmap-manager-callbacks-original.json`.
These files and the helper remain standalone. No CMake or existing production
path was modified; allocation pressure, worker ownership, genuine wait
completion, resource-parent layouts, and residency activation remain
separate boundaries.
