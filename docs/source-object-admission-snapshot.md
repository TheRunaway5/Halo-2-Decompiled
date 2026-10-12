# Ordinary admission over a complete original snapshot

`tools/differential_source_object_admission_snapshot.py` runs unchanged retail
`132220`/`133050` and their fog, frustum, association and queue callees over the
complete paused Zanzibar object and spatial pools. The native comparison uses
the `SourceObjectSpatialPool` overload directly. It never reconstructs those
pools through registration, insertion, owner sorting or authored placement order.

The evidence is the independently copied and restored
`zanzibar-shading-baseline` internal snapshot, clock tick **12474**, CPU
`EIP=0012B33E`, `CR3=0000F000`. Its camera and root cached sphere match the
retained player reference. Its animation banks differ from later capture20;
it is not labeled capture20. The private guest remains paused. The replay uses
only immutable RAM and file exports and performs no QMP or guest operation.

The local binary fixture preserves all original2048 object handle records,
occupancy words and headers, all **202 complete live object bodies**
(127296bytes), both **512-word head arrays**, and all2048 records, occupancy
words and headers in each of the four independent spatial data arrays.
There are **187 collidable and37 noncollidable entries**, and the same counts
of allocated owner references. Generations, unused bytes and linked head order
are retained. Native admission does not require the reference arrays to traverse
cluster entries, but the fixture retains both independently.

The original `547F88` context points at `7FD80000`. Its retained visibility set
has one camera group,15 cluster entries and15 frusta. It has ordinary mode0
and auxiliaryNULL. Two replay cases use this exact bank, with empty or explicitly
retained prior association masks. Additional cases vary supported filtering,
clipping, force-admission, queue capacity and explicitly synthetic valid
frustum banks. All cases use a fresh visited epoch and reset queue.

Every case explicitly supplies flag`10` to omit the separate light-volume
services, and disables the sky-owner bypass. The latter can write a negative
association ordinal outside the bounded native seam. Original`16E210` computes
fog for the55 authored clusters using captured BSP, scenario and active fog
state. The unqueried capacity55..511 is explicitly zero. None of these inputs
is presented as a recovered complete live-frame producer.

The oracle maps required virtual pages through the captured page tables. It
rejects absent pages and uncaptured MMIO. Every executed instruction is checked
against the pinned original XBE; no target lookup, allocator, registration,
frustum, fog, queue or association callee is hooked. After each pass, every
original object and spatial bank is checked byte for byte against the snapshot.
Both `1CAC60` registration and `16B840` allocation have **zero calls**.

The frozen comparison covers **39 cases and8066 output words**: exact full
handle order, flags, auxiliary words, association indices and bit masks. Native
and unchanged original outputs match. The existing saved collector has41
records, but its production phase has not been established at the paused
instruction. It is diagnostic evidence only, not the expected output of this
fresh pass or a successful registration chronology.

The target `source_object_admission_snapshot_tests` and CTest
`retail_source_object_admission_snapshot` read
`tests/fixtures/retail_source_object_admission_snapshot.bin` and the adjacent
TSV. The original binary evidence is intentionally ignored; the test skips with
code77 when it is absent. The adjacent provenance JSON pins its hash, every raw
bank and complete owner record, snapshot metadata, original context/visibility
hashes, and all case labels. This is an admission replay over supplied original
state; it does not close native registration, camera/door production,
light-volume, sky-bypass or shadow-candidate lifecycle boundaries.
