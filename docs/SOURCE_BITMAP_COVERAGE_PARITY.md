# Original bitmap coverage input

`source_bitmap_coverage(actual_metric)` provides the complete arithmetic that
original `16B90` stores in render context `+28` and `+2C`. It is a standalone
module; no native residency scheduler, renderer, shared build, or GPU path is
changed.

The actual ordinary part caller is `4D9C0`. Its metric is the ordered SSE
expression `((float(collector_byte) * resolved_part_scale) * 4670E4) * 4`.
This is the existing `source_shader_part_metric` expression. The collector
byte and authored part scale remain inputs from their proved source producers;
a projected-radius approximation or retained mip cannot replace them.
`4670E4` is the actual shader multiplier, independently produced by `2C490`.
The geometry-detail multiplier is a different input.

`16B90` uses `DIVSS 1 / metric` when the original comparison is positive.
Zero, signed zero, negative, and unordered outcomes use literal `100000`
(`45DE10`). The reciprocal is stored as float at `+28`. The following
`FLDLN2`, `FYL2X`, and `FMUL [44A04C]` store the adjustment at `+2C`:

```
adjustment = ln(float_reciprocal) * float_from_bits(3F317218)
```

This is the literal natural logarithm multiplied by the stored float ln2.
It is not a generic log2 or a fitted mip bias. The helper preserves the
separate reciprocal spill, double logarithm/scaling, and final float store.
Its frozen corpus independently checks the resulting float words against the
unchanged original x87 instructions. This does not establish every physical
CPU transcendental result on every host.

## Original execution and regression

`tools/oracle_source_bitmap_coverage.py` reads the immutable
`registration-snapshot-restore-20261008` RAM through its original page tables.
It executes complete `4D9C0 → 1BD50 → 1BBF0 → 16B90` up to `16CB3`, immediately
after both coverage stores. The actual tag, pass, D3D state and pointer banks
are retained; there are no instruction patches or substituted callees.
The authored shader is `E2DA0165`, template `E2DB0166`, LOD 0 / layer 15 /
pass 0 / implementation 0. All ten literal energy-blade model parts contribute
four distinct proved scale values. The caller cases span every coverage byte
and the actual retained `.85` plus explicitly supplied `.3` / `.25` player
policy multipliers. Direct original `16B90` cases cover adjacent exponent
boundaries, raw float controls, signed zeros, negative values, subnormals,
NaNs, and reciprocal overflow.

Normal and fully instrumented ASAN/UBSAN/leak tests pass **7,942 cases /
23,826 words exactly**. The fixture is
`tests/fixtures/retail_source_bitmap_coverage.tsv`; its provenance and complete
report are `retail_source_bitmap_coverage.provenance.json` and
`analysis/multiplayer-shading/source-bitmap-coverage-original.json`.
Supplied x87 control `023F` / MXCSR `1F80` are execution inputs, not inferred
live scheduling. The report verifies 2,645 distinct original instructions,
including the further diagnostic probes described below.

The positive-infinity metric produces a zero reciprocal. The unchanged
emulator's zero-argument `FYL2X` result differs from the host logarithm result.
That exceptional path is retained as a separate report control and explicitly
rejected by the native helper. No zero-log value is fitted, and no actual
finite authored part requires it.

## Further whole-caller boundaries

Three diagnostic calls (coverage bytes 0 / 1 / 255 and the literal section-0
part-0 scale) continue through all material setup, animation, extern and stage
binding functions to the actual `1B0A0` entry. Their complete stage banks and
contexts are retained. They independently produce four active stages, authored
bitmap scalars, animated frames/transforms, and the original minimum state.
The clock and packed render context are the literal snapshot `4858A0` and
context ordinal 0. This is an explicit supplied caller environment, not proof
of original draw 8787's owner or clock phase.

A fourth diagnostic continues through `1B0A0 → 143C0 → 137550 → 12310`.
Stage 0's request completes through its actual ready cache and commits the
minimum. Stage 1 then reaches original `12CE00` with flags 3 and request 0,
record `8026C174`. Execution stops at that service entry. No allocation result,
readiness, or completion is supplied. This keeps stage production separate
from the cache activation history.

Native activation still needs the actual ordered binding/context lifecycle.
`16F60` runs when the selected implementation pointers at context `+20/+24`
change or the incoming shader datum differs from context `+0`; it writes 3
to each mapped parameter minimum. `18A90` is called on every `16B90` invocation
and copies all 64 bytes into the stage bank only for mappings whose parameter
byte is not FF. That copy includes the parameter minimum. `1B207` commits the
clamped floor `0..2` only after actual `143C0` returns. A per-material guess
cannot replace this global binding/copy history. See
[SOURCE_BITMAP_CPU_SELECTION_PARITY.md](SOURCE_BITMAP_CPU_SELECTION_PARITY.md)
for the explicit current-cache selector and missing allocator/maintenance
services.
