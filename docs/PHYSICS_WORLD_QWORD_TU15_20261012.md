# TU1.5 triangle contact callback load correction

The original callback `282FD0` loads the eight-byte constant at `46A688` with
`FLD qword` at `2830FE`. Its bytes are `105839b4c876e23f`, the binary64 value
`0.577`. The shared generator previously treated every x87 memory operand as
binary32 and passed width eight into a four-byte scalar reader. The first new
complete-callback test terminated the pre-fix native build with stack corruption.

The current world generator now selects a real eight-byte read and converts the
binary64 value directly to extended precision. Regeneration changes exactly one
lowered instruction in the current 477-body graph. The shared scalar reader also
rejects zero or oversized widths before accessing memory. Historical snapshots
and their original reports remain intact.

The new oracle executes only the owned English TU1.5 executable. Original shape
constructor stores bind the triangle/type24 table `4620D0` and type8 table
`461FE0` to their real slot+14 methods. The complete callback runs those methods,
computes and transforms the triangle normal, executes its actual RSQRTSS and
refinement instructions, and performs the original threshold comparison.

There are 288 declared-input cases covering both shape orders, both triangle
shapes, neither triangle, adjacent binary32 threshold values, signed directions,
zero, infinities, NaNs, subnormals, geometry scale, transform rotation, x87
precision/rounding/TOP and SSE rounding. Unchanged original instructions execute
on the host i386 CPU: 31,488 instructions and 4,512 ordered writes; 240 cases
reach the qword load. Both callback return values occur. The test-only estimate
service checks the input and returns the independently observed CPU estimate;
production still requires its real CPU owner. Every native instruction,
ordered before/after store, retained byte, defined flag, GP/SSE state and live
x87 value is compared. Missing CPU ownership must retain the exact prefix.

The original observer rejects data access outside declared retained input. No
original instruction is patched, no contact result is supplied, and no Xbox CPU
equivalence is claimed. The raw shape/pair records are procedure inputs, not
proof of source body construction or live multiplayer contact generation.

The correction is installed in the shared production world graph and is
registered with the TU1.5 component tests. Release and ASAN/UBSAN/leak checks
are recorded in `analysis/multiplayer/sandbox-source/world-qword-tu15-20261012`.
The fresh TU1.5 world/body/contact producers and actual multiplayer wiring remain
open. The existing visible lift and vehicle failures are still acceptance
failures; this component does not make them pass.
