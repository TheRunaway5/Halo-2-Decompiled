# Source texture-cache physical reservation

`source_bitmap_reservation.hpp/.cpp` implements complete `13D370` over
explicitly owned raw texture-manager banks. It reuses the independently
proved image busy/deletion and complete manager-deletion callbacks. The
result is a physical metadata reservation, not an allocated native texture,
completed read, bound image, or reconstructed pressure history.

## Inputs and admission

The literal ABI is stack `[manager, signed bytes, signed age]`, `RET 0Ch`,
with EAX containing a salted physical handle or `FFFFFFFF`. The native API
takes `SourceBitmapManagerBanks`, `SourceBitmapReservationInput{bytes,age}`,
and the explicit busy/fence context. Admission requires ranking state 2,
page shift 12, positive signed bytes, ages 2 or 5, and the registered literal
texture callbacks. Other ranking states, shifts, nonpositive requests and
age branches are admission errors, distinct from a completed source failure.

The manager's raw fields include ranking `+2C`, capacity in pages `+30`,
shift `+34`, clock `+38`, head/tail `+3C/+40`, eight limits `+44..60`, and
physical pool `+64`. Each 24-byte physical row contains salt `+0`, pages
`+4`, page offset `+8`, next/previous handles `+C/+10`, and timestamp `+14`.
All pool counters, salt words, occupied bits, row payloads and owner bytes
are retained. Paired banks must have equal occupied bits and matching salts;
the complete physical address list must be finite, ordered, nonoverlapping
and within capacity. The next physical salt must have the admitted source
producer range `8000..FFFE`. Debug-poison pools remain excluded.

The source age clamp reaches index 8, which aliases the pool pointer at
manager `+64`; there is no ninth limit. This module admits the image read
caller's ages 2/5. The same manager's separate loan producer `12D2F0` uses
age 1 and `bytes+2023h`; that caller is outside this API's proved scope.

## Exact selection, mutation, and failure

For positive admitted bytes, source arithmetic rounds to 4,096-byte pages.
A signed `pages >= limit[age]` rejects the request immediately. The remaining
algorithm scans the address list and uses a separate 256-entry ring of
16-byte candidate records. Ring capacity is unrelated to the 512-slot pool.
When the ring is full, the source skips adding a new candidate; it does not
discard an older one to admit that start position.

The busy callback executes before the unsigned `block.time+age >= now`
protection test. Busy segments discard all open candidates. Even protected
blocks may therefore produce a legitimate completed-fence clear during the
preceding query. Candidate timestamps use unsigned maximum, candidate pages
use wrapping DWORD addition and signed qualification, and ties retain the
earlier candidate. Ranking state 2 compares signed low DWORD products:
`best.pages*(now-candidate.time) > candidate.pages*(now-best.time)`.
No division, floating-point ratio or widened multiplication is substituted.

After selection, the source scans occupied slots by ascending absolute index,
deleting signed-overlap rows through complete `13D830`. This order differs
from the preceding address-list scan. If the pool still has its maximum
occupied count, the source may delete the oldest eligible slot outside the
chosen interval. When that slot is the saved predecessor, it saves the old
previous link before deleting it.

Complete `16B840` searches occupied bits from first-free to high-water,
falls back to high-water when space remains, updates occupancy and counters,
zeros the entire new physical row, assigns its salt, and advances the salt
WORD. Next salt `FFFF` is skipped by setting it to `8000`. The new row is
inserted into the source list and receives the selected offset, requested
pages, and the supplied manager clock. All other raw bytes remain exact.

No candidate, exhausted pool, or another completed allocation failure returns
`Complete` with handle `FFFFFFFF`. The source does not roll back preceding
queries or deletions. On the admitted ages, its final failed-search path
stores the signed minimum of the current age limit and requested pages.
The retained oversized controls lower limits `+4C/+58` to 4,738 pages.

A deletion that reaches the genuine work-mutex service instead returns
`NeedsWorkMutexAcquire` with its pending deletion handle, preserving preceding
source writes. It reports neither a completed wait nor a completed source
failure. This result is a stopped prefix, not a retry-safe continuation:
starting another request over the mutated banks would not resume the original
candidate/eviction state. The manager callback proof independently freezes
the actual mutex boundary and leaves its wait/count/hardware-retry loop
unresolved. See [SOURCE_BITMAP_MANAGER_CALLBACKS.md](SOURCE_BITMAP_MANAGER_CALLBACKS.md).

## Success phase and history boundaries

Success reserves only a physical slot. The source caller `12D5A0` next creates
the same-salt image with `16B910`, then binds owner/partition and pending
header state before submitting the read. Another paired-bank reservation
must wait for that intervening source image binding; the test explicitly
rejects a second reservation over the unbound physical phase. This helper
does not execute that binding or change any ready byte.

The retained manager has 4,737 pages, shift 12, clock 111,186 and 286 occupied
slots. These are immutable source inputs, not recommended native budgets or
an inferred timeline. `12C5B0` advances the source clock and resets eight
limits, with `13D230` handling clock wrap. `12C990` touches block timestamps.
`12C1E0` changes physical capacity through separate physical protection and
manager-resize services. None of those producers is manufactured here.

## Original fixture validation

The original executable is `assets/disc/default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The immutable source bank is
`analysis/multiplayer-shading/registration-snapshot-restore-20261008`, RAM
SHA-256 `ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55`.

The mandatory corpus freezes complete raw `B` banks and explicit context.
Each `R` row supplies conditional input patches, then reconstructs the entire
expected bank from the unchanged source output patches. Unchanged bytes are
mandatory, including owner, image, row payload and unused header fields.

The corpus contains 392 complete original calls and compares 25,503,520 bank
bytes: 168 arguments from the retained allocator report, two oversize
controls, 16 byte-count/rounding edges, 144 coherent chain/ring/capacity
controls, 18 salt controls, 24 signed-limit controls, 16 wrapping-clock
controls, two reversed-slot full-pool controls, and two protected full-pool
allocation failures. The latter prove the final age-limit store after a
chosen gap cannot obtain a slot and no eligible oldest slot exists. The 168 report cases
are 42 authored eligible partitions crossed with four supplied wait/force
combinations, restored independently; they are not 168 historical events.

Conditional chains use the original addressed pool layout and explicit raw
metadata. OwnerNULL and no-job fields select independently proved source
branches; the immutable ready bytes remain unchanged and unconsumed.
These controls establish conditional arithmetic and metadata transitions,
without claiming constructor, readiness or completion producers. The
unchanged oracle reaches 14,410 full-ring branches, ten extra-oldest deletions
with predecessor repair, and six salt-rollover branches. It also records
11,442 overlap deletions, 32 failure-limit stores and two full-pool slot
allocation failures. Every whole-caller case completes without a service
replacement; the separate callback fixture retains the genuine wait cases.

Normal and fully instrumented AddressSanitizer/UndefinedBehaviorSanitizer
tests compare the full frozen corpus, with leak detection. Fourteen native
controls distinguish unsupported inputs and the unbound
physical phase from completed source failure. The normal fixture requires
exact case and bank-byte counts; missing, empty and truncated corpora are
independently rejected. No production activation or CMake change
is part of this module.

The corpus and provenance are
`tests/fixtures/retail_source_bitmap_reservation.tsv` and its adjacent JSON;
the source report is
`analysis/multiplayer-shading/source-bitmap-reservation-original.json`.
`tools/oracle_source_bitmap_reservation.py` verifies 660 distinct reached
title instructions against the original executable and checks that all
original writes and controlled data ranges remain disjoint from reached
code. It restores and checks 92 loaded original pages, supplies FPCW `023F`
and MXCSR `1F80`,
and replaces no instructions or services. The separate
[source-only pool allocation report](../analysis/multiplayer-shading/source-pool-allocation-original.json)
corroborates complete `16B840` with
116 calls over physical/image pools, including sparse/full/first-free and
salt controls. Its hashes and byte patches retain unchanged bytes too. The
392-case whole caller fixture remains the native reservation authority; the
primitive report adds no separate portable service, native budget, pressure
history, provider, or image-binding claim.
