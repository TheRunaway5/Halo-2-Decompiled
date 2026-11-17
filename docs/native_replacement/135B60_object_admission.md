# 135B60 render-object admission

`135B60` admits one render object into the portal-visibility object queue.
`134D30` calls it four times. It is now native by default in the generated
Shadow owner (`src/rendering/shadows/source_shadow_frame_tu15_generated.cpp`).
`HALO2_REFERENCE_OBJECT_ADMISSION=1` restores the translated body for
comparison.

## What is native

- Policy: `include/halo2/native_tu15_object_admission_kernel.hpp`
  (`admit_object`). It owns only control flow:
  - Already stamped this frame: find the object's queue row (the source keeps
    the last probed cursor, -1 when nothing matched), merge its
    cluster-association bit, then OR the primary (4) and secondary (2) row
    flags. An exhausted secondary search returns through the source's second
    epilogue.
  - Not yet stamped: run the genuine `135970` geometry admission on the
    selected bounding sphere. A visible (or force-admitted) object classifies
    its plane sides, packs its record flags, allocates a visibility
    association unless contained, appends its queue row, and publishes the
    frame stamp only when the row was appended.
- View: `src/rendering/native_tu15_object_admission.hpp`. It owns every
  ordered access, the 0x14 frame, the four saved registers, the argument
  pushes, and the four pure leaves, which are inlined:
  `135AE0` (plane sides and |distance| < radius, two x87 slots), `135620`
  (row flags), `166B60` (bounded four-stream queue append) and `C0CA0` (frame
  stamp). The `135970` and `136E10` children stay genuine and go through
  their own native bridges via `scope_.child`; a refusing child falls back to
  the translated child through the unit's real dispatcher.
- Binding: `tools/native_object_admission_replacements.py` pins the complete
  body, resolver and dispatcher. Registered in `tools/native_source_prefixes.py`.

Admission: TU1.5 image, dense masked x87 depth at most 3, no MMX, and the
exact live instruction-cursor contract (as `136EA0`). With
`HALO2_CENSUS_NATIVE_BRIDGES=1` the uncursored census callbacks are also
admitted (diagnostic only); the native closure holds no service PC.

## Proof

`tools/validate_native_object_admission_tu15.py` runs the unchanged TU1.5
instructions of `135B60`, the four leaves and the genuine
`135970`/`167EF0`/`184690`/`166C00`/`136E10` tree single-stepped on physical
i386, and compares the native route at every ordered read and store (complete
GP/EFLAGS/XMM/MXCSR/x87 packet), at genuine child entries, at the final ABI and
over every backed byte.

Run 2026-11-15 (`--prefixes --require-binding --semantic-mutants`, GCC):

- 1,410 physical cases: 248 normal (31 families x 8 variants) and 1,162
  read/write/after-write/instruction fault prefixes; 181,874 original
  instructions.
- All 314 owned sites executed (209 in `135B60`, the rest in the leaves).
- 6,900 comparisons, 444,487 fully compared pre-I/O packets, 1,216 genuine
  child entries, 496 bound/prepared default calls.
- Production access route (silent hook, direct pages, read views): 248
  comparisons, 14,731 direct-page reads, 100 view reads, 0 provider reads.
- Census route: 248 comparisons, 23,584 packets; census guards 7/7.
- Guards: 50 untouched retained fallbacks; reference guards 2; forced
  reference 496 comparisons.
- 12/12 compiled mutants rejected, including `census-without-switch`.
- The same corpus passes under ASan/UBSan (no mutants), and the normal
  corpus passes with Clang and `--require-binding` after the `136A80` binding
  landed in the same unit.

## Runtime

See `PORTAL_VISIBILITY_CONTINUATION.md` for the frame/trace parity, census and
timing measured on the shared build.
