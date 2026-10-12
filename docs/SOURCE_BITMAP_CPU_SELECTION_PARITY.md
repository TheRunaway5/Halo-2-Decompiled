# Original CPU bitmap image selection

The standalone `source_bitmap_cpu_select` reproduces `D15E0` image selection
from explicit current bitmap, handle, readiness, cache, and global banks. It
does not select a captured mip, initialize a native residency history, or
change the current renderer/sampler. The helper is registered in `halo2_cache`
with strict floating-point operations. CTest `retail_source_bitmap_cpu_selection`
checks the frozen original fixture; the earlier 299-test capture build predates
this registration.

The independent oracle is `tools/oracle_source_bitmap_cpu_selection.py`.
It executes unchanged `D15E0 → 12CCF0 → 12CE00 → 12CB10 / 12C990` against
the pinned disc executable. Original instructions, lookups, floating arithmetic,
and returns are never replaced. Actual construction/allocation/pump entries
are explicit stopping boundaries; no service result is supplied.

## Exact CPU composition

`D15E0` consumes the bitmap record in EAX and raw request in XMM2. It returns
an existing nonzero cached image immediately when the signed deadline `+70`
is strictly greater than the actual cache clock `4E6488`. Otherwise it calls
`12CCF0` with flags zero and the raw request in XMM1. That lookup applies
the separately rounded byte bias and global bias, handles the literal
software-category mode, and looks up hardware tiers using the actual handle
bank. Touching a present pending image still updates its LRU row. Ready tier 0
updates the cached image and the signed, wrapping `clock + 2` deadline.
Its final cached override accepts deadline equality.

A zero lookup result enters complete `12CE00` with flags zero. The source:

1. Computes its own adjusted request, then applies `46883D` zero and `46883C`
   one overrides in that order.
2. Obtains the inline image if shared storage is available and the current
   Common/pending-size description admits it. A missing Common enters
   original `12C820`; the helper reports `InlineHeaderConstruction` there.
3. Uses the **raw XMM1 request** for its cutoff gate, separately from the
   **adjusted XMM0 request** consumed by `12CB10`.
4. Lets `12CB10` choose from literal authored resource pointers `+1C` and
   sizes `+34`: pointer is not NONE, size is nonzero, and tiers 1/2 additionally
   require signed size greater than 1,024. These fields are independent of
   currently allocated handles at `+28`.
5. Calls `12C990` with CL zero. Its `12CABD` branch only checks the requested
   current handle/readiness and touches LRU; it does not recheck authored
   resource eligibility. A ready requested image returns immediately.
6. On a missing/pending requested image, scans current handles 0, 1, 2. Each
   ready image replaces the candidate, so the **last ready image wins** over
   the inline fallback. A skipped request with an admitted inline candidate
   does not perform that scan.

Filtered positive `4E6470` force modes and `4E6474` category selection affect
the source wait/default local. Mode 1 can select the actual type-specific
default image when all candidates are absent, and marks `4E647A`; an already
set marker is preserved. CPU flags zero nevertheless keep CL zero throughout.
This path cannot reach `12D5A0`, `120CE0`, `SwitchToThread`, or `125D60`.

All three lookup sites index the source pool by the handle's low 16 bits,
without a salt check. The typed input preserves that behavior. If two
supplied handles alias the same row, their resolved image/readiness/LRU data
must agree; touching the row updates both aliases. The helper rejects
contradictory supplied rows rather than manufacturing pool state.

## Literal retained state and independent controls

The immutable snapshot is
`analysis/multiplayer-shading/registration-snapshot-restore-20261008`.
The whole MAP report independently supplies 64 record identities and five
additional entry-state versions produced by earlier unchanged MAP calls.
These versions are not invented initial state. Two snapshot records,
`8038E248` and `80637158`, still have Common zero / stored size 128 and reach
`12C820`; their later observed versions already contain the built headers.
Sixty-two snapshot cases and all five later versions return completely.

Actual retained inputs include software-selection mode `4E647B=0`, shared
storage availability `4E6468=1`, shared base `818ED000`, bias enable 1,
global bias zero, cache clock 12,474, both request overrides zero, and both
force globals zero. Consequently inline fallback is not evidence that the
optional software-category lookup mode was enabled.

`tests/fixtures/retail_source_bitmap_cpu_selection.tsv` freezes 1,629 CPU
cases and 16,290 exact output words. Thirty stop at original `12C820`; the
remaining 1,599 run through the complete return. Controls cover all ready/
present combinations, requested-ready versus last-ready precedence, raw
cutoff versus adjusted selection, sequential overrides, category filters,
resource-size boundaries, signed deadlines/overflow, default types, persistent
marker state, and 24 low-16 handle alias cases. Eight native admission controls
also reject missing source inputs and preserve the cached shortcut's inactive
inputs.

The broader original report has 2,025 cases: 1,905 complete returns and 120
actual service-entry boundaries (30 inline construction, 54 allocation,
36 priority-pump). The allocation/pump controls are general `12CE00` request
flags, **not CPU `D15E0` activation**. Every executed instruction is checked
against the disc image; the report retains 483 unique verified instructions,
all call registers/arguments, and ordered LRU writes. Supplied FPCW `023F`
and MXCSR `1F80` are explicit execution inputs, not inferred scheduling state.
No GPU replay or guest advancement is performed.

The report and provenance are
`analysis/multiplayer-shading/source-bitmap-cpu-selection-original.json` and
`tests/fixtures/retail_source_bitmap_cpu_selection.provenance.json`. Normal
and fully instrumented ASAN/UBSAN/leak runs pass the frozen CPU fixture.

## Missing native activation and allocation producers

The separate [low-detail asset API](SOURCE_BITMAP_LOW_DETAIL_PARITY.md)
establishes the stored images and complete original `12C820` header arithmetic.
The selector still needs an explicit actual initialized inline resource; a raw
descriptor is not silently treated as an initialized/ready image. Connecting
its construction boundary requires the native shared-image initialization
service, retaining original `12C640` storage availability and actual completed
image identity. The existing asset proof stops the allocation/I/O prefix at
`213760`; it does not infer asynchronous completion from an authored blob.

Connecting **per-view hardware requests** additionally requires:

- The actual ordered `16B90` binding context, literal stage mask/bitmap
  scalar/frame/flags, and `1B0A0` minimum state. The standalone
  [coverage producer](SOURCE_BITMAP_COVERAGE_PARITY.md) now proves context
  `+28/+2C`, and further unchanged diagnostic calls reach the complete stage
  bank and actual allocation-request entry. Its retained context/clock is
  explicit; native binding/lifetime history is still required. Neither a
  retained mip nor a generic logarithm replaces those inputs.
- Real initialized image/cache pools (`12C0D0`) and physical stage bounds
  (`4E6420 / 4E642C / 4E6440`), followed by the actual `12D5A0 → 13D370 /
  16B910` allocation and eviction producers. The proved allocation prefix
  does not determine the whole-title memory budget or pressure history.
  The separate [retained allocator audit](SOURCE_BITMAP_ALLOCATOR_BOUNDARY.md)
  completes reservation/eviction and pending headers, then stops at the actual
  async free-list kernel wait without inventing a completed job or ready bytes.
- The literal `12CE00` allocation/wait flags, `4E28F0` task pressure,
  `4E6484` per-maintenance load count, authored source partition sizes, and
  actual pending/ready image rows. General flags 3 can allocate and wait;
  CPU flags zero cannot stand in for that service.
- Resource I/O `213760`, source completion bytes/handles, and `120CE0`
  priority changes / `125D60` pumping. Header/byte equality alone does not
  provide completion order or readiness.
- Original cache-clock/LRU maintenance: render setup `12A50` calls
  `12C600 → 12C400 / 12C450 / 12C5B0`. This processes services/predictions,
  advances the separate cache and LRU clocks with their wrap behavior,
  resets per-maintenance load count, and performs timed eviction work.
  A simulation tick or retained source time is not a substitute clock.

Until those caller services are represented, eager native texture loading
cannot be labeled the original residency history. The safe output of this
slice is the independently tested selector over explicit state, with current
renderer/runtime activation unchanged.
