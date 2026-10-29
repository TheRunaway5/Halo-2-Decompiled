# Native TU1.5 trajectory fallback

`src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp` implements actual
FF580 as mathematical direction, distance, speed and duration outputs. The
memory API publishes them in the original order; the private-value API returns
a typed value for native arc composition without creating a retained temporary
address. One shared calculation owns both. No CPU register packet, virtual FP
stack, instruction loop, captured result or lifted callback is used. The
original FF580 boolean is always true after its calculation/publication; no
pathability boolean is supplied by this component.

The norm deliberately differs from ordinary normalization. X and Y differences
are spilled to REAL. The Z difference remains extended while also being spilled,
then contributes `dz_extended*dz_REAL` to the x87 norm:
`sqrt(((dz_extended*dz_REAL)+(dy_REAL*dy_REAL))+(dx_REAL*dx_REAL))`.
The REAL magnitude spill retains its extended comparison quantity. Epsilon
uses the original strict x87 comparison; normalization uses the original SSE
operand priority and reciprocal. Zero/small magnitudes select the actual
indirect default vector through474D10. Duration is computed only when the
original speed comparison exceeds epsilon. All globals remain lazy source
reads. Direction XYZ publishes before optional distance, speed and duration;
aliased destinations preserve those exact stores.

A rejected source read can leave one real mathematical quantity pending:
the first X/Y/Z coordinate or absolute distance. The typed error retains its
kind, exact80-bit value and original failure cause. Materializing it preserves
the actual pre-failure native FP state/TOP. It does not invent a successful
input, discard the quantity or construct a guest stack. The native arithmetic
owner restores unrelated live host x87 values/environment and MXCSR. Masked
exceptions and two available original temporary x87 slots are the qualified
profile. Unmasked traps/other pressure remain separate ownership cases.

`tools/oracle_native_campaign_ai_jump_fallback_tu15.py` executes unchanged
physical i386 English TU1.5, image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The261 cases cover all82 source sites,14,561 original instructions,2,201
ordered reads and1,074 stores. They include108 semantic fault prefixes, two
actual unmapped operand faults and21 exact pending quantities. Rounding,
precision, DAZ/FTZ, epsilon boundaries, exceptional/paired NaNs, overflow,
unaligned/overlapping outputs, optional-output combinations and partial
publications are checked. Original caller x87 values remain intact through
both completed and pending boundaries. No source instruction, callee return
or resumed failure result is supplied.

One declared adversarial epsilon=0 data case reaches the five zero-magnitude
sites that the literal positive epsilon cannot reach. It is identified in the
report and is not a production constant change. Canonical constants and
source data remain unchanged; existing native owners already provide the
literal default vector44AC40 and indirect pointer474D10.

GCC normal and ASan/UBSan pass1/1 each; Clang passes7/7. Six actual-source
mutations are rejected: prematurely rounded Z, omitted pending quantity,
discarded pre-fault FP state, inverted duration division, missing default
vector and early optional publications. The private-value entry additionally
matches completed original direction/scalar/FP results and the full source
read order while leaving retained memory unchanged. The original repeat is
byte-identical. Current pins, commands and verified cleanup are in
`analysis/campaign-native-ai-jump-fallback-20261028/proof.json`.

The required fixture is
`tests/fixtures/native_campaign_ai_jump_fallback_tu15.json.gz`,26,665 bytes,
SHA-256 `f68f85da73baa87cb83a65e4bee63b940bc4c4d795c64d03cef0dcd2da330d0b`.
All owned original ELF, raw banks and compiler/build outputs were removed.
`cmake/CampaignNativeAiJumpFallback.cmake` adds the native source and focused
test. Numeric dispatch, full FF1A0/278E60 jump feasibility, graph search/update
and generic native campaign acceptance remain open.

The point inputs now also have explicit lazy storage: real retained fields use
`NativeCampaignAiJumpPointMemoryTu15`, and native projected vectors use
`NativeCampaignAiJumpPointValueTu15`. Both implement coordinate access without
a source-private stack. The current v3 proof reruns all261 original cases,145
private output values and145 private input-point pairs under GCC, sanitizers
and Clang mutation controls. Private inputs preserve vector/scalar/FP results
and actual constant read order while leaving retained memory unchanged.
