# 136A80 model section draw-unit submission

`136A80` submits the draw units of a model render record. `1365C0` (under
`1362A0`, light geometry) calls it with the record in EAX and one byte
argument. It is native by default in the generated Shadow owner;
`HALO2_REFERENCE_MODEL_DRAW_SUBMISSION=1` restores the translated body.

It is not a pure leaf: every part calls `2CBD0` (the dynamic model draw
unit) through the State's required boundary at `136CE0`. In the live World
that call is the native draw unit (`physics_live_world_tu15.cpp`, "sole
dynamic-model site"), so `2CBD0` never appears in the census.

## What is native

- Policy: `include/halo2/native_tu15_model_draw_submission_kernel.hpp`
  (`submit_model`). For every section: an optional "same first part" argument
  skips sections whose first part differs from section 0's; a single-owner
  record of a unit with shadow geometry is first copied (`rep movsd`, 0x79
  dwords) into the caller frame with its override fields patched, and the
  copy is used for the rest of the section. Each part packs its flags and 21
  arguments and submits one draw unit.
- View: `src/rendering/native_tu15_model_draw_submission.hpp`. It owns every
  ordered access, the 8-aligned 0x214 frame, the saved registers and the
  `2CBD0` required call (return address pushed first, then
  `State::required`). The body runs no x87/MMX instruction, so there is no
  raw x87 scope and provider calls need no FP parking. Details that matter:
  - `rep movsd` honours DF, and each iteration retires (ESI/EDI/ECX advance)
    before its completed store is observed, as on the CPU.
  - `AND`+`SHL 4` before an access uses hardware flags (`owner_row`), since
    SHL's OF/AF reach the next packet.
  - Everything is re-read after `2CBD0` returns (part count, cursor, EDI).
- Binding: `tools/native_model_draw_submission_replacements.py` (complete
  body, resolver and dispatcher pins), registered in
  `tools/native_source_prefixes.py`.

Admission: TU1.5 image, no MMX, the exact live instruction-cursor contract,
and a certified required owner
(`native_required_owner_policy_detail::preserves_instruction_policy`, see
`REQUIRED_OWNER_POLICY.md`): the native parent skips the remaining
instruction callbacks after `2CBD0` returns, so that owner must preserve the
caller's callback, cursor and trace policy. The live World owner is certified
(`WorldRequiredOwner`). `HALO2_CENSUS_NATIVE_BRIDGES=1` also admits the
uncursored census callbacks (diagnostic only).

## Proof

`tools/validate_native_model_draw_submission_tu15.py` runs the unchanged
TU1.5 instructions of `136A80` single-stepped on physical i386. Each run stops
before `2CBD0`; a deterministic stub then supplies that callee's ABI and
storage effects and the original resumes at `136CE5`. The same stub is the
harness's certified required owner (`Stub` in
`tests/native_tu15_model_draw_submission_tests.cpp`). It pops the return
address and 0x54 argument bytes, clobbers EAX/ECX/EDX/EFLAGS/XMM0-7, and per
mode also sets an MXCSR flag, changes EDI (callee-saved), decrements the part
count, or advances the caller's part cursor. The native route is compared at
every ordered read and store (complete GP/EFLAGS/XMM/MXCSR/x87 packet), at
every `2CBD0` entry (complete ABI), at the final ABI and over every backed
byte.

Run 2026-11-15 (`--prefixes --semantic-mutants`, GCC):

- 1,419 physical cases: 128 normal (16 families x 8 variants) and 1,291
  read/write/after-write/instruction fault prefixes; 185,973 original
  instructions in up to 10 resumed segments per case; 331 stubbed `2CBD0`
  calls in the normal corpus.
- All 201 owned sites executed (DF=0 and DF=1 copies, skip/no-skip,
  override 0xFF, side bytes, negative counts, x87 depth 0-3).
- 5,523 comparisons, 885,312 fully compared pre-I/O packets, 3,197 compared
  `2CBD0` entries, 256 bound/prepared default calls.
- Production access route (silent hook, direct pages): 128 comparisons,
  19,703 direct-page reads, 0 provider reads.
- Census route: 128 comparisons; census guards 7/7.
- Guards (46 untouched fallbacks, including uncertified/foreign/wrapped
  required owners), reference guards and forced reference (256 comparisons)
  pass.
- 12/12 compiled mutants rejected: `ignore-skip-argument`,
  `submit-skipped-section`, `forward-only-copy`, `copy-observed-before-retire`,
  `narrow-override`, `swapped-side-bytes`, `part-cursor-stride`,
  `call-before-return-address`, `software-owner-shift`, `copy-keeps-record`,
  `uncertified-required-owner`, `census-without-switch`.

After installing the default, the full corpus with prefixes passes again
with `--require-binding` under ASan/UBSan, and the normal corpus passes with
Clang.

The first full run found one real difference: an observer exception after a
`rep movsd` store left ESI/EDI/ECX un-advanced, while the CPU retires the
iteration first. The translated body has the same order; the native view now
follows the CPU.

## Runtime

See `PORTAL_VISIBILITY_CONTINUATION.md` for parity, census and timing.
