# Original bitmap request and resident-partition lookup

The standalone `source_bitmap_residency` helper reproduces the original
texture request arithmetic without selecting recorded mip levels. It is not
connected to the native renderer or a native streaming scheduler.

The authority is the unchanged original disc executable, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The [oracle](../tools/differential_source_bitmap_residency.py) verifies every
executed instruction against that executable and uses explicit x87 control
`027F` and MXCSR `1F80`. No original routine, arithmetic operation, resident
resource result, or asynchronous completion is replaced.

## Request composition

`16F60` writes a64-byte parameter record from the authored bitmap binding:
datum, image index as a float, scalar, and initial minimum3. `18A90` copies
that record into the four-stage bank at context+1320. `1B0A0` reads the
enabled-stage mask at+1420, the coverage adjustment at+2C, and the actual
`4BA019` view modifier. For each enabled non-NONE binding, it wraps the
authored frame with the original x87 remainder/floor path and passes the
low16-bit image index to `143C0`.

The requested fraction is `scalar + coverage_adjustment - .5`, in the
original separately rounded order, clamped0..4. Authored flagbit0 or the
view modifier forces0. Its integer bucket is capped at2. A request is
submitted only when that bucket is below the stage's retained minimum;
the minimum changes after the call regardless of the returned resource.
Increasing a later request does not undo the retained minimum. Requests
retain original stage order.

`143C0` resolves the bitmap record through unchanged `137550` and calls
`12310`. Its binding dimensions come from the authored record, even when
the physical resident partition begins at a lower mip. BitmapNONE and an
invalid signed image index use the original default-bitmap service; that
service is not replaced by this helper.

The4,234 request fixtures execute the complete unchanged
`1B0A0→143C0→137550→12310` path against explicit resident cache rows with
future deadlines. They observe5,422 ordered resource calls. The literal
energy-blade binding scalars are included as authored inputs; original8787
recorded mip offsets are never read by the generator.

## Hardware resident lookup

The separate2,324 fixtures execute unchanged `12310/D15E0→12CCF0` with explicit
bitmap, image-pool, and LRU-pool rows. `12310` first accepts a cached texture
whose signed deadline is strictly later than the actual cache clock.
Otherwise `12CCF0` applies the actual optional byte/global bias, selects
partition0/1/2 at request thresholds1/2 below the bitmap cutoff, touches its
LRU record, and accepts only a ready image. A ready partition0 sets the
cached pointer and deadline to clock+2. The final cached-pointer override
also accepts deadline equality. The fixtures include pending and absent
partitions, bias edges, exact/equal/expired deadlines, signed deadline
overflow, and software-mode inputs that the original direct or fast-cache
path never consumes. The114 CPU-prefix cases also pin `D15E0`'s literal
flags0; `D30D0` explicitly supplies request0. Hardware `12310` supplies
flags3 instead.

If the lookup returns0, the original entrypoint calls `12CE00`, with its
actual flags0 or3. The oracle stops exactly at that entry; the native helper
reports `requires_request=true`. That service can return a software fallback
or process hardware resources, so entering it is not itself proof of an
allocation or I/O. No service result or substitute image is supplied.

The82 explicit software-mode cases execute complete
`12CCB0→12C960` for existing inline headers, disabled cache state, and category
fallback. The helper requires the actual category at bitmap+17, header control
at+5C, pending size at+64, inline-header resource, and global `4E6468`.
Categories1..11/17/18 use the software branch when `4E647B` is enabled;
other categories continue to hardware lookup. Missing `12C820` header
construction remains a rejected input of this helper.

The6,558 fixtures check59,126 output words exactly. The frozen standalone
test also rejects unsupported stage masks, missing frame counts, invalid
minimum state, other cache request flags, unavailable software state,
unproved header construction, and missing image/header tokens. Normal and fully instrumented ASAN/UBSAN runs
with leak detection pass. The report is
`analysis/multiplayer-shading/source-bitmap-residency-original.json`; the
fixture and provenance are `retail_source_bitmap_residency.*`.

## Required live inputs

The caller must supply the actual completed stage bank, enabled-stage mask,
coverage adjustment, view modifier, bitmap cutoff/bias, resident salted
handles/readiness, cache clock/deadline, and LRU clock. The coverage
adjustment's `16B90` x87 logarithm producer is not replaced by a generic
logarithm. `12C530` initializes the cache clock to4; `12C5B0` advances it,
resets on wrap, and advances the separate LRU clock. `12C600` invokes that
maintenance from normal render setup `12A50` and from other allocation
pump callers. Thus a simulation tick or retained object snapshot is not
automatically the original cache clock.

The independent [partition proof](SOURCE_BITMAP_PARTITION_PARITY.md)
establishes the literal partition0/1/2 D3D headers and authored BC mip
tails, plus the allocation prefix up to asynchronous I/O. It does not
complete I/O, budget/pressure policy, eviction history, or readiness.
The retained8787 source views begin at mip0/1/2/2, but that observation
does not authorize choosing those levels in current native rendering.
Shader clock, original same-phase cache state, derivative sampling, and
final framebuffer appearance remain separate inputs and proofs.

The independent whole `D1E10` MAP oracle's retained floor bitmap
`F4F91373` has no hardware partition handles or cached pointer. Its CPU
`D15E0` request therefore continues into `12CE00`, which returns the inline
software header at bitmap+5C. Actual `4E647B=0`, so this is distinct from
the optional software-mode lookup cases above. Its header describes8×8 BC3
and its stored pixels differ from the authored512×512 texture's8×8 mip.
Those observations do not authorize substituting that mip. The separate
[low-detail asset proof](SOURCE_BITMAP_LOW_DETAIL_PARITY.md) now establishes
authored storage, complete original header initialization, and the explicit
raw/Texture API across all 21 maps. It does not select a resident image:
the complete fallback caller remains separate before a native sampler change.

The request helper and frozen fixture are registered in CMake as
`retail_source_bitmap_residency`. Neither this helper nor the independent
asset API activates a native residency scheduler.

Reproduce the standalone CPU proof without touching shared build outputs:

```sh
g++ -std=c++20 -O2 -fno-fast-math -ffp-contract=off -Iinclude \
  src/source_bitmap_residency.cpp tests/source_bitmap_residency_tests.cpp \
  -o analysis/multiplayer-shading/source_bitmap_residency_tests
.tools/python/bin/python tools/differential_source_bitmap_residency.py \
  --binary analysis/multiplayer-shading/source_bitmap_residency_tests
analysis/multiplayer-shading/source_bitmap_residency_tests \
  tests/fixtures/retail_source_bitmap_residency.tsv
```
