# Native TU1.5 shape bounds proof

The independent physical qualification covers the source box projection,
radius wrapper, transformed and forwarding wrappers, motion sweep, centered
projection and centered radius wrapper. The original authority is English
TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Actual complete body counts are `306690=48`, `306760=13`, `2E8870=25`,
`2E9600=3`, `2E5E80=48`, `1D64C0=61`, `307E30=15`, plus native affine
geometry `2E44D0=40`. `306690` is a cdecl four-argument helper and `1D64C0`
a cdecl five-argument helper; the shape wrappers have true `RET12`.

`build/test-results/native-shape-bounds-final-tu15/report.json` passes on
GCC and Clang: 1,195 physical unchanged-original cases, including 627 complete
calls/chains, 356 read-fault prefixes, 102 rejected-store prefixes, 102
post-store observer prefixes, and eight physical alignment faults. Comparison
covers 23,544 ordered reads, 5,787 ordered stores, all retained bytes, complete
FP control/status and genuine incoming live x87 values. The native host's
complete 108-byte legacy floating state and MXCSR are guarded independently.

The native test service recursively calls only qualified native children.
Original hardware executes each complete unmodified chain. The known chains
include forwarding → transformed → radius → box and motion → centered radius
→ centered box. No child return, bound, success/admission, contact result or
captured after image supplies expected physics. The component storage is
declared input; authenticated vtables and constants come from the TU1.5 image.
The separate MCC shape census establishes which of these owner chains occur
in actual imported-cache before banks; it does not establish live activation.

The suite exercises every SIMD rounding mode and DAZ/FTZ combination, paired
quiet/signaling NaNs, all four vector lanes, output/source aliases, scratch
aliases, motion-null paths, radius additions and lazy rereads. Four private
mutations are rejected: swapped MAX operands, reassociated extent additions,
a cached second translation, and reading the center before publication. An
explicit extent-association boundary distinguishes `(33554432+2)-33554432`
from `33554432+(2-33554432)` under source float rounding.

Transformed scratch is the actual source address
`((entrySP-4)&~15)-0x40`, remains live across its genuine child, and the child
entry SP is `((entrySP-4)&~15)-0x60`. A motion wrapper enters its child at
`((entrySP-4)&~15)-0x20`; forwarding preserves child entry SP. Tests follow
these exact identities and lifetimes. The historical first centered-motion
fixture retained obsolete transformed scratch after changing the child;
that storage had become a private compiler frame. The corrected fixture
removes that obsolete packet and excludes the old scratch-as-matrix case.
This changes test ownership, without changing any module or semantic output.
The historical red report is preserved.

Twelve radius-read fault prefixes at `306768`/`307E38` remain explicitly
unqualified: margin is then a live private x87 value, and the current typed FP
API does not transport that temporary value after an exception. Their source
status must not be silently restored or their private value fabricated.
Unmasked platform traps, source exception unwind, raw public GP ABI consumers,
and complete world/live activation remain separate gates.

The production modules were never edited by this independent proof:

- Shape CPP `117d1459b133857217fe6ea61551e2fd7e061f3e698faf404db01300a558b217`.
- Shape HPP `af451b0c502a0b4dd2ec8cb962a3005378c4fffdf1867eafaed0bc6dacb3d2b4`.
- Centered CPP `f87bb2fbe3b84a8b97ac25e0a381e65c6de597372ace8bcd00394370b6660014`.
- Centered HPP `217a37e0e976a8f92f7c3162f06c24d05612e8e455146c2a868d5b32af765563`.
- Geometry CPP `6ddbdb31c2f109ca8748bec3edc74556d0ff72bc95b4120a603a7ccac889dc79`.

The final owned scratch `halo2-test-77tsd7i5` was removed, freeing
199,217,152 allocated bytes. `handoff-cleanup.json` verifies absence and current
source pins. No new binaries, object files, ELF copies, raw banks or frames
are retained. Existing original assets, MCC caches and reference evidence
remain intact. Small commands, hashes and failure diagnostics are preserved.
