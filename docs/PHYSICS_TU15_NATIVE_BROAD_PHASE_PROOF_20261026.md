# TU1.5 native broad phase and temporary pair-array lifetime

`native_tu15_update_broad_phase` implements the owned `321700` scheduling,
workspace, bounds dispatch, sweep dispatch, pending-pair append, consume and
lifetime algorithm. `3177E0`/`317870` temporary-array construction/destruction
are native helpers in the same module. Their authenticated source bodies have
278/49/52 instruction sites; these counts describe listings, not all-path proof.

The module uses named physics objects and callbacks. Its two pair-array headers
are explicit caller-owned 20-byte semantic packets, with data/count/capacity/
allocation-base/reserve-count fields. They remain in shared memory so genuine
callbacks and aliases can affect subsequent reads. They are not a virtual CPU
frame or register file. The implementation retains source arena allocation and
release branches, wrapping and signed comparisons, original x87 gravity-margin
order, the cached profile cursor on the zero-body path, current shape/dispatcher
owners, pending-pair array/data/count rereads and actual temporary lifetime.
The source's configuration argument is unused within this owned body.

Shape bounds, dispatcher sweep, pair growth, pair consumption and custom
allocator operations remain explicit actual services. Missing owners stop at
their real phase. No recorded geometry, collision result or pointer outcome is
used as a production implementation.

## Physical original qualification

Unchanged original i386 code from the English TU1.5 XBE
(`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`)
ran with genuine Linux i386 FS setup. The comparison covers retained physics
memory and the two declared semantic pair packets. Platform-private SEH links
and other private CPU-frame bytes are outside this component comparison; no
such bytes are manufactured by the native implementation. The whole-frame raw
comparison remains unchanged.

The final GCC and Clang proof passed:

- 236 physical cases: 15 complete temporary-array lifetimes, 116 genuine
  allocator/shape/sweep entry stops, 67 operand-read fault prefixes and 38 integer
  store-fault prefixes.
- 4,696 ordered reads, 3,647 ordered stores, 29,488 original instructions and all
  retained banks/semantic packets, with complete CW/SW/MXCSR comparisons.
- All four rounding modes and 24/53/64-bit x87 precision controls for the margin,
  including finite values, signed zero, denormals, infinities and quiet/signaling
  NaNs. Original read-only numeric constants were preserved.
- Zero, negative and wrapping counts, borrowed/owned array flags, real virtual
  allocation/release stops, a source packet/arena alias and actual callback
  argument packets observed at the original boundary.
- A private mutation removing the real borrowed-array high bit was rejected by
  the original publication/bank comparison on both compilers.

The raw virtual-boundary fixtures establish dispatch and argument preservation;
they do not establish that those objects are admitted gameplay geometry.
Prepared-MCC multiplayer execution must establish actual object admission and
real callback execution.

A separate continuation model exercises a mutable sweep, actual pair-header
updates, capacity growth, pending-pair append, consume ordering, the collision
publication and return of native arena storage. It is not original execution
proof of those remaining callback owners. A complete original `321700` return
after its real remaining owners, platform exception-unwind behavior, unmasked
traps, full profiling timestamps and live multiplayer composition remain open.

## Evidence and storage

The final source/listing hashes, commands, fixture hash, bounded GCC/Clang logs
and correctly rejected negative control are in
`analysis/physics/native-broad-phase-tu15-20261026-final/`. The small authenticated
listing is in `analysis/physics/native-broad-phase-tu15-20261026/original-listing.txt`.
The first attempt records a test-only explicit JSON integer-conversion fix; no
physics-kernel correction or relaxed comparison was needed.

Both compiler builds and all three enclosing owners were verified removed.
Immediately before the final owner ended, its remaining files occupied
89,919,488 allocated bytes (89,886,897 logical bytes), including the
78,143,323-byte original fixture. Those files were removed; the compiler builds
had already been deleted. No large artifact is retained. The final bounded
reports occupied 11,474 logical bytes before the cleanup receipt was added.
`cleanup.json` records removal checks for every attempt.

Integration source pins:

| Source | SHA-256 |
| --- | --- |
| `native_tu15_broad_phase.cpp` | `f1099b8647d6b9b724f2083f1edb03896545e489fb45d17173d432dc585cf1bc` |
| `native_tu15_broad_phase.hpp` | `ba72556a73d7628bccf1f125fb8cc5132e288eb151548ea0b1d8cd24346f1c13` |
