# Native contact key sorting and entry-buffer ownership

The native candidate-buffer module implements actual English TU1.5 `2F33B0`
(59 sites), `2F3430` (50), `2F34D0` (54) and `317170` (17). The independently
authenticated [source listing and ownership maps](../analysis/physics/native-contact-candidate-cleanup-ownership-tu15-20261026/report.json)
use the original XBE hash
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The sort compares keys unsigned and indices signed, using source-width wrapping
arithmetic, the actual partition read/store order, left recursion and right tail
iteration. It forwards the opaque fourth argument, which the source never uses
for a comparison. Its typed value ABI excludes aliases between key storage and
the original caller's argument/control cells. It creates no GP file or virtual
source stack.

The buffer owns 12-byte opaque entries and a 20-byte header including original
stack-block backing and count. Its constructor initializes the verified header
fields and publishes capacity, original count, data and original backing in
that source order. It never initializes entry contents. Its destructor releases
or rewinds the original stack block, then releases independently grown owned
backing. Base cleanup `317170` reads the data pointer before the allocator
method table, as the source does.

The [two-compiler physical matrix](../analysis/physics/native-contact-candidate-buffers-tu15-20261026-matrix/report.json)
matches physics bytes, opaque packets and FP state in all 59 cases per compiler.
Fifty-two strict whole-bank cases pass. Seven interrupted constructor/destructor
prefixes differ only in two captured FS:[0] bytes; those strict raw results
remain red and appear in the acceptance calculation. No virtual FS registration
or source exception dispatcher is implemented.

Cases include keys created by an actual unchanged original query, declared
reversal, extrema, duplicate, singleton and subrange inputs, sort read/prewrite/
committed-observer prefixes, true stack exhaustion, actual owned reserve/free,
32-bit size wrapping and four rounding modes crossed with DAZ/FTZ. These are
declared leaf inputs; original and native owners independently calculate their
outputs. Wrong entry stride, unsigned ordering and constructor publication-order
controls are rejected.

The [independent cleanup proof](../analysis/physics/native-contact-candidate-unwind-tu15-20261026-first/report.json)
passes 16 cases per compiler with GCC16 and Clang23. It executes actual original
prefixes and cleanup action `37E600` on real pool backing, then compares them
with native C++ exceptions and guards. Each compiler records 44 genuine original
reference calls, 20 real pool calls and 14 injected native exceptions. Ordered
cleanup-read addresses match as well as persistent physics/header bytes and FP.
Early constructor arm, early destructor disarm and late destructor disarm
mutations are rejected. Original live FS differences remain explicitly raw red.

FuncInfo `46B70C` and map `46B704` select action `37E600`, which invokes base
cleanup `317170`. Constructor state0 arms at `2F3469` after allocator inputs.
Destructor state0 arms at `2F3507`, and disarms at `2F353A` after reading capacity
and before owned release. The native guards advance/disarm before cleanup.

The [full candidate composition refresh](../analysis/physics/native-contact-candidates-helpers-composed-tu15-20261026-qualified/aggregate.json)
passes 42 cases and three negative controls with both compilers. Native sort,
entry construction/destruction, inverse transform, dispatch, reserve and byte
copy compose under the native candidate parent. Per compiler, 38,630 actual
instructions and 9,915 retained data events match in address, width and bytes,
including the full 20-byte temporary buffer owner. All 3,795,642,138 persistent
bytes, FP and independently recorded remaining service contexts match.

Broader caller aliases, secondary exceptions/platform unwind, production
allocator ownership, visible native multiplayer and Xbox hardware identity
remain separate gates. These proofs do not close the producer's copied
opaque-byte or future-reuse failures.

Owned compiler/reference products were removed; bounded reports and cleanup
receipts remain. The GCC and Clang matrix owners removed 295,432,192 and
295,436,288 measured allocated bytes. The cleanup owner removed 295,428,096.
Measurements exclude nested CMake products already removed by their own runner.
The full composition owner and both compiler build trees are also absent,
removing another 857,657,344 measured allocated bytes. No large artifacts from
these tests are retained; protected reference inputs remain intact.
