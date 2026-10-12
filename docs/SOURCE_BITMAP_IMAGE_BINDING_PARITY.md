# Source texture-cache image-pool binding

`source_bitmap_image_binding.hpp/.cpp` implements complete `16B910`, including
its complete `16BA00` row constructor, over separate explicitly addressed
image-pool banks. This supplies the image-binding phase that follows physical
reservation in `12D5A0`. It does not construct the pending D3D header, submit a
read, produce a job ID, or establish ready images.

The original ABI is EDX = pool, EAX = supplied salted handle, plain `RET`.
EAX returns the supplied handle on success or `FFFFFFFF` on invalid low-16
index or nonzero row salt. The native API preserves that DWORD distinction;
zero is a valid successful handle. Original EBX/ESI/EDI/EBP preservation is
also checked by the oracle.

## Raw inputs and exact mutation

`SourceBitmapImageBindingBank` contains the addressed pool header, row bank,
and occupied-bit bank. The admitted actual layout is a 76-byte header,
512 rows of 40 bytes, and 64 occupied-bit bytes. Capacity and stride are
pool `+20/+24`, first-free is `+34`, signed high-water is `+38`, wrapping
count is `+3C`, generated next-salt WORD is `+40`, and rows/bits pointers are
`+44/+48`. The unused WORD at `+42` and all other raw bytes are preserved.

`16B910` masks the handle to its low 16 bits and checks the index against
capacity. It then tests the selected row's salt WORD. A nonzero row salt
returns `FFFFFFFF`; the supplied salt is not compared with that row, and
occupied bits are not used as the admission test. Invalid indices return
before consuming rows or bits. Occupied rows return before consuming bits.
The helper therefore leaves unconsumed row and bit banks optional on these
early paths; each supplied bank that is consumed still needs its full admitted
storage shape.

On success, the source ORs the occupied bit, increments the DWORD count with
wrapping arithmetic, and updates high-water to `index+1` only when the signed
comparison permits it. Complete `16BA00` zeros all 40 row bytes, writes the
generated next-salt WORD, and advances that WORD. If the incremented value is
`FFFF`, it becomes `8000`; initial `FFFF` instead wraps to zero. `16B910`
then overwrites the row salt with the supplied upper 16 bits. First-free
remains unchanged even when that value points at the newly occupied slot.

Supplied salts zero and `FFFF` are accepted at a valid zero-salt row. With
supplied zero, the row remains salt-zero after success, so another call can
succeed even though its occupied bit is already set. Both calls consume a
generated salt and increment the raw count. No count/popcount, first-free,
generation, occupied-bit, or unused-flag coherence condition is invented.
The pool's debug flag does not affect `16B910/16BA00` zeroing.

Admission errors cover unavailable or inconsistent addressed storage,
unsupported capacity/stride, overlapping addressed bank ranges, and aliased
native storage for different banks. These are distinct from a completed
source failure and occur before mutation. The overlap restriction is
material: with bits aliased to header `+24`, the original occupied-bit OR
can change the stride before `16BA00` rereads it. That layout is outside the
actual separate-bank proof; the helper rejects it instead of pretending the
40-byte constructor remains valid.

## Original evidence and validation

[The oracle](../tools/oracle_source_bitmap_image_binding.py) executes only
unchanged complete `16B910/16BA00`. Every reached instruction is checked
against the literal disc XBE on every execution. All nonstack writes must
fall within the three full raw banks, disjoint from code. Each reset restores
and reads all loaded original pages. The supplied execution configuration
is FPCW `023F`, MXCSR `1F80`; it is not inferred live floating-point state.

The mandatory frozen corpus contains 787 cases, 789 complete source calls,
and 16,227,940 compared bank bytes:

- 168 exact frozen post-reservation inputs from the completed unchanged
  `13D370` proof. Their entire producer bank hashes are verified against the
  reservation report before extracting the image banks. Binding restores
  image/physical occupied-bit equality, matching count, and matching salts
  across every occupied slot for these actual physical reservations. The
  reservation producer is not reexecuted by this
  oracle, and complete `12D5A0` is not claimed.
- 512 immutable retained slot inputs, covering occupied and free rows with
  explicit supplied handles. These are callee controls, not reconstructed
  historical binding events.
- 107 explicit raw controls for invalid indices, independent supplied and
  generated salts, signed high-water and wrapping count, occupied-bit
  disagreement with zero row salt, occupied-row generation aliases, unused
  debug flags, repeated supplied-zero salt, and successful EAX zero.

The frozen test compares the whole 20,620-byte bank set per case, including
all bytes unchanged by the source. The native probe and normal fixture
validation match every original output patch. Seventeen independent native
controls reject unsupported storage or check unconsumed-storage early
returns. Missing, empty, and truncated fixture files must fail; normal mode
requires the exact 787-case/16,227,940-byte corpus.

Normal and fully instrumented ASAN/UBSAN execution with leak detection both
pass that mandatory corpus and all seventeen controls. The original proof
checks 74 distinct instructions and restores eight original pages.

The report and its compact provenance retain XBE, snapshot, frozen producer,
fixture, tool, native helper/header/test, and comparison binary hashes:

- [Original report](../analysis/multiplayer-shading/source-bitmap-image-binding-original.json)
- [Frozen fixture](../tests/fixtures/retail_source_bitmap_image_binding.tsv)
- [Fixture provenance](../tests/fixtures/retail_source_bitmap_image_binding.provenance.json)
- [Native test](../tests/source_bitmap_image_binding_tests.cpp)
- [Physical-reservation authority](SOURCE_BITMAP_RESERVATION_PARITY.md)

The reservation oracle's historical comparison binary identity is preserved;
this proof uses its frozen output and does not rerun it merely to update a
durable executable hash. The helper and mandatory corpus are registered in CMake. No
renderer/runtime activation, original pressure history,
kernel outcome, I/O completion, or residency timeline is asserted.

## Next caller boundary

The source caller next fills image ready `+3=0`, flags bit 0 clear, partition
`+4`, owner `+10`, and the 20-byte resource header at `+14`, then stores owner
handle `+28[partition]`, address `+40[partition]`, and primary address `+54`
for partition zero. Complete `12D6D0` header production is a separate
prerequisite: the existing 168 allocator prefixes cover 42 authored bitmap
partitions with their actual non-linear header branches, formats 9/14/16,
type zero, and depth one. Admission should follow proved metadata branches,
not weapon identity.

At entry `213760`, pending row job `+C` and priority `+8` remain zero from
binding. Its actual read location, sector-rounded byte count, physical
address, done pointer, category, and priority are explicit source inputs.
Final job stores, loading-counter increments, caller success, mutex results,
and actual completion remain outside this binding module.
