# Ordinary object queue after admission

The retained Zanzibar main queue at original `33400` is a **render-processed
queue**, rather than the immediate output of `132220`. Its 41 owners and six
association masks can be reproduced from the actual retained spatial lists,
owner spheres, main portal/frustum bank and two later queue operations.
This is a reconstruction over supplied original state. Native successful
registration, global collection and gameplay shadow activation remain separate.

## Exact source stages

`1320F0` resets the four queues with `1348E0`, increments the BSP epoch and calls
`134390`, then increments the object/light-volume epochs and calls `132220`.
For the retained context, unchanged `134390` creates **zero prior associations**.
The actual 15 cluster entries are visited in this source construction order:

```
0 45 8 21 6 47 50 10 2 33 44 48 31 7 32
```

Within each cluster, `132220` walks the literal collidable head chain, followed
by the noncollidable chain. `133050` computes frustum/plane/fog flags and
associations. `163C80` appends the four queue columns: identity, 16-bit flags,
32-bit auxiliary value and 16-bit association. It reserves the final capacity
slot. `BEC70` stamps the visited epoch only after a successful append. Repeated
links update the already admitted row's fog/sky flags and association bits;
they do not create duplicate queue rows.

The native raw-pool overload matches this fresh original output: **190 words**
(41 complete rows, count and six four-word association masks). It uses the
actual 202 full owners, both 512-head arrays and all independent spatial-pool
records. No registration, insertion, sorting by owner identity or allocation
reconstructs these lists. With actual context flags zero, the replay also runs
four original `132A30` light-volume calls and `C3950`; those services do not alter
the resulting ordinary owner queue. Registration and spatial allocation have
zero calls. The companion frozen CTest `retail_source_object_admission_shadow`
checks this fresh 190-word output using the existing raw-pool snapshot
executable. Its complete original binary bank is local evidence and
intentionally ignored; an absent bank skips with77.

`2CBC0` subsequently calls `133720`, which begins with `134240`. That routine
uses the **current** owner definition's `+38` hlmt datum, hlmt `+4` mode datum,
and the signed PRTInfo count at mode `+74`. It does not select the parent/root
model and does not test PRT resource residency. A positive count promotes the
row by calling `163D50` to swap all four columns with the next prefix slot.
The promoted prefix retains its relative order. The displaced ordinary suffix
is **not a stable partition**.

For this queue, six PRT owners become the prefix:

```
E2840015 E27F0010 E29E002F E2830014 E2820013 E2810012
```

`133720` then walks the reordered queue through `133790`. That routine derives
its draw metric from the owner's actual cached center `+30`, the main camera
eye at `4B9DA0..A8` and scale at `4B9DCC`. Complete `4BAF0` computes both fade
bytes. `133790` at `133A51`/`133A98` adds **low bit0** to the row's flags only
when the **first** byte is zero. A nonzero first byte retains all prior bits.
The second byte is the separate shadow-cutoff input; it cannot substitute for
this first-byte marker. Exactly 17 rows receive the marker in the retained
queue. After these two stages, all **190 words** match the saved `33400` queue.

## Native helper and checks

`source_object_render_queue_order` in
`include/halo2/source_object_render_queue.hpp` implements literal current-model
lookup and full-row swaps. Explicit optional presence fields distinguish a
source null pointer from unresolved input. The original checks null owner,
definition and hlmt pointers; it does not protect a null mode pointer after a
valid mode datum. Missing native mode metadata is therefore rejected, not
treated as an unlit or ordinary model. Only fields read by the selected branch
are required. Unresolved input is rejected before any queue row is mutated.

`source_object_render_queue_fade_flags` consumes a **computed**
`SourceModelFadeBytes` value. It only performs the source conditional bit
operation; callers still need actual `133790` metric, `4BA019`, owner/root and
model input banks for the complete fade producer.

`tools/differential_source_object_render_queue.py` executes unchanged `134240`,
`163D50` and the conditional flag instruction slice for 1,024 controlled banks,
covering literal null/NONE branches, signed zero/negative/positive PRT counts,
and nontrivial flags, associations and auxiliary values. It adds one live-bank
case that starts from the independently reproduced fresh admission rows.
The native test derives all 41 metrics and complete first/second fade pairs
from actual owner/camera fields and correlated model metadata. Saved queue
flags and unchanged original `133790`-prefix metrics/fade outputs are
independent expected results; they are not fed into the native fade helper.

The frozen fixture has **1,025 cases / 261,562 exact output words**. Its live
row checks 330 words: 41 four-column rows plus count, and 41 independent
identity/metric/first-byte/second-byte groups plus count. Ten missing-input
controls verify rejection without partial mutation; four early/byte controls
verify unread fields, NONE and existing-marker behavior. CTest is
`retail_source_object_render_queue`.

## Phase and service boundaries

`tools/audit_shadow_main_collector.py` reads the immutable `33400-entry`
exports from `shadow-live-phase-20261008/capture-opengl`, clock tick12475,
frame111186, view0, with the pinned retail XBE. The source backend was
OpenGL/llvmpipe; it is neither physical Xbox evidence nor retained original20
Vulkan GPU evidence. The actual context is mode0, auxNULL, flags0, containing
cluster0, with one group, 15 clusters and 15 frusta.

The phase export did not include every global service page. Active fog/sample,
complete tag/light-volume pages and their pointers come from the earlier
immutable restored snapshot at tick12474. All 202 retained definition prefixes
agree with that earlier snapshot. Every used definition `+38`, hlmt `+4`, fade
range/cutoffs/count and mode PRT count is independently byte-identical to the
authored Zanzibar cache. Later paused `3290A` metadata corroborates two of
those model chains in the separate fade proof. Earlier/later corroboration is
explicit; no simultaneous export of the missing main-phase globals is claimed.
The `4BA019=0` replay input is also retained from that earlier bank, rather than
invented from a desired queue output.

The replay validates every phase-file size/hash and every executed instruction
against the original executable. All 202 complete bodies and both spatial-pool
states remain byte-identical after execution. The guest is not accessed,
advanced or written by either audit or fixture generator. No native lifetime
registration chronology, observer producer, cache lifetime, portal producer
or gameplay shadow integration follows from this queue reconstruction alone.

Reports are `analysis/multiplayer-shading/shadow-main-collector-replay.json`
and `object-render-queue-original.json`; the textual fixture and adjacent
provenance are `tests/fixtures/retail_source_object_render_queue.*`.
