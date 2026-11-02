# Native TU1.5 body reference compaction

Actual3174A0,3173C0 and317430 now compact the body's three reference arrays
with native semantic kernels. The root contains53 sites and its helpers47 each.
It caches the original count and first scan's data pointer, finds the first null,
then rereads the header pointer for each later source element. Nonzero entries
move in original order; stale tail entries are retained. Capacity masking,
signed comparisons, wrapping doubled capacity, reserve timing and count stores
follow the source. This owner compacts references before retirement; it does
not itself detach or destroy a body.

Authority is English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No guest CPU state, instruction dispatch or supplied compaction result is used.
The source2E31E0 reserve is a typed service, wired to its existing native owner.

`build/test-results/native-body-references2-20261101/report.json` passes323
physical original cases with GCC and Clang:231 complete calls,12 genuine
reserve-entry stops and80 read/store/observer prefixes. All retained bytes,
4,267 ordered reads,1,086 stores and FP state match through22,663 unchanged
original instructions. It covers empty/negative counts, mixed/full/null arrays,
capacity flags, growth admission and preserved x87 depths0/1/3/8. Wrong-copy,
wrong-count and wrong-reserve-stride mutations are rejected. An initial missing
header dependency caused a compiler failure; that report remains separate.

`build/test-results/native-world-body-references-binding-20261101/report.json`
passes110 original world cases,96 native force compositions, the updated world
binding model and its negative control. The model compacts a real sparse array,
changes the world's retirement entry from a store observer, and verifies the
subsequent reference decrement uses the newly published entry. All three arrays
are visited for each body; no3174A0 residual executes. The existing reserve
binding performs genuine native growth rather than providing array outputs.

Per-world counters expose root/cohort calls, moved entries and reserves.
`HALO2_REQUIRE_NATIVE_BODY_REFERENCES` requires positive native root work, all
three completed cohorts per call and absence of the old residual owner key.
Visible imported-MCC multiplayer/vehicle-camera and strict whole-frame original
qualification are staged and still pending. Mutable-observer growth composition,
unmasked platform exceptions and other retirement/destructor owners remain open.

The successful component scratch and compiler builds were removed, accounting
for80,445,440 measured allocated temporary bytes. The failed compiler owner and
successful world-model owner were also removed; their cleanup receipts retain
separate measurements. No large new artifacts remain. The live freeze is
`analysis/physics/native-body-references-live-binding-20261101/source-pins.json`.
