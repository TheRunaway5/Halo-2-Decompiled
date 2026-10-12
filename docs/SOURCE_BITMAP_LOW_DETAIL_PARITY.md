# Original shared low-detail bitmap storage

The five audited Zanzibar bitmaps have independently authored low-detail
images in the local map's shared pixel blob. They are separate from the
three hardware resource partitions. All five compressed payloads differ
from the same-dimension mip tails in partition 0; those tails cannot be
substituted for the shared images.

`tools/oracle_source_bitmap_low_detail.py` freezes ten complete original
`12C820` headers, 50 output words, raw descriptions, and every stored
payload byte. Its report is
`analysis/multiplayer-shading/source-bitmap-low-detail-original.json`;
its fixture is `tests/fixtures/retail_source_bitmap_low_detail.tsv`.
Every executed instruction is checked against the pinned disc executable.
The separate asset APIs and CPU tests are now registered in CMake. They do
not change rendering or the current bitmap cache-selection service.

## Disc fields and the original map-header copy

The original `1228F0` loads the cache header into `547700` with a
512-word copy at `122911`–`12291B`. The oracle executes this unchanged
copy on the actual 2,048 disc bytes and stops at `12291D`. Consequently:

| Disc map-header offset | Source global | Actual Zanzibar value |
| --- | --- | --- |
| `158` | `547858` | Shared pixel file offset `01C3B000` |
| `15C` | `54785C` | Shared pixel byte count `0002A600` |

These fields are not the map-header `2C4` / `2C8` bank. The source
`12C640` reads precisely the `158` / `15C` fields.

The immutable 116-byte bitmap record contains this description before
its inline hardware header is built:

| Record offset | Meaning used by the original producer |
| --- | --- |
| `5C` | Initial Common, zero for these five images |
| `60` | Byte offset into the map's shared pixel blob |
| `64` | Stored byte count; 128 for each audited image |
| `68`, `6A` | Signed 16-bit cache format and width |
| `6C`, `6E` | Signed 16-bit height and depth |

The literal `energy_blade_bottom` shader bitmap bank supplies the four
sword bitmap datums. The floor datum is the named source CPU-sampler
case. No retained graphics capture is used to discover their fields.

| Bitmap | Shared offset | Format | Dimensions | Admitted compressed bytes |
| --- | --- | --- | --- | --- |
| `F4F91373` floor_base | `28F80` | 16 / BC3 | 8×8×1 | 64 |
| `E2E1016C` energy_blade_noise1 | `3400` | 14 / BC1 | 8×8×1 | 32 |
| `E2E2016D` jackal_shield_noise | `3480` | 16 / BC3 | 8×8×1 | 64 |
| `E2E3016E` blade_shield_offset | `3500` | 16 / BC3 | 8×2×1 | 32 |
| `E2E4016F` blade_shield_mask | `3580` | 16 / BC3 | 8×2×1 | 32 |

Each stored slot is 128 bytes. The record's dimensions and format admit
224 compressed bytes across the five images. Their remaining slot bytes
are retained in the report too; no extra mip chain is inferred from them.

## Original shared-blob initialization boundary

The unchanged `12C640` walks all 806 actual bitmap tags through original
`122C70`, `122D00`, `13DDD0` and the source group comparator. Only host
pointers to copied tag/group metadata are relocated. No iterator result
or group membership is supplied.

It rounds the authored blob size up to `2B000` for its physical
allocation and to `2A600` for sector I/O. The physical stage index and
stage bounds are declared caller inputs. The source itself subtracts
`2B000` from the stage's upper bound, checks its lower bound, records
the new upper bound, and applies the `80000000` CPU alias. It then
stores that pointer to `4E646C` at `12C72C`, stores availability 1
to `4E6468` at `12C732`, and calls `213760` with:

| Input | Original value |
| --- | --- |
| ECX file location | `01C3B000` |
| Read size | `0002A600` |
| Buffer | The source-computed aliased physical allocation |
| Bytes-read pointer | NULL |
| Done pointer | Original local stack byte |
| Category / priority | 3 / 7 |

Execution stops at that I/O entry. No disk-read completion, readiness,
or asynchronous result is supplied. The actual authored blob is read
independently from the disc file for payload provenance. The only supplied
service is `2D15DA`'s external `XPhysicalProtect` storage-permission call;
its unused return cannot affect the allocation arithmetic or any header.
No source instruction is patched.

Two declared stage bounds produce physical bases `08100000` and
`018ED000`, proving that relocation is an input. This does not recover
the whole-title physical-memory budget. Three absent-blob controls
(zero offset, zero size, both zero) execute complete `12C640`: they
clear `4E6468` and preserve the prior `4E646C` pointer.

## Complete original inline header construction

The unchanged `12C820` reads the authored description, adds `4E646C`
to its shared byte offset, obtains the original format mapping from
`450768`, and calls `359E0` with maximum-levels zero. That source call
returns zero additional mip levels for these descriptions. The resulting
header advertises one level, not the hardware partition mip chain.

The producer writes only the 20 bytes at record `5C` through `6F`:
Common `00040001`, Data `(shared_base + authored_offset) & 0FFFFFFF`,
Lock zero, the NV2A Format, and Size zero. It encodes two dimensions,
NV2A format `0C` for BC1 or `0F` for BC3, and the source floor-log2
dimension fields. All other record bytes remain unchanged.

| Bitmap | Original Format |
| --- | --- |
| `F4F91373` | `03310F29` |
| `E2E1016C` | `03310C29` |
| `E2E2016D` | `03310F29` |
| `E2E3016E`, `E2E4016F` | `01310F29` |

With declared aliased shared base `818ED000`, the source floor header is
exactly `00040001 / 01915F80 / 00000000 / 03310F29 / 00000000`.
These values are computed from immutable disc fields and original
instructions. No retained header is copied into the oracle.

The `4E647B` software-selection mode is a separate input from shared
blob availability `4E6468`. Original `12CE00` can build and return this
inline header through its fallback even when `4E647B` is zero. Its
complete request/readiness logic and the source `D15E0` sampling path
belong to the separate sampler proof; this audit does not infer them
from category `+17` or from this header producer.

## All-map immutable resource API

`Cache::load_bitmap_low_detail_resource(datum, index)` returns an optional
`BitmapLowDetailResource` copied from the record and the map-header
`158` / `15C` blob. It retains the authored dimensions, type, format,
flags, relative shared offset, and every padded payload byte. A missing
blob or zero-sized descriptor stays absent. Invalid tag/index, mutated
inline Common, invalid dimensions/type, and out-of-range storage throw.
The API does not mutate the independent hardware bitmap partitions.

`Cache::load_bitmap_low_detail(datum, index)` separately decodes the proved
one-level descriptions. Planar BC1/BC2/BC3 images retain their compressed
bytes. The supplied 4×4 RGBA cubemaps use the original padded face layout
and the existing NV6 Morton/BGRA-to-RGBA conversion. The source `12C820`
format mapping determines that layout; the full hardware image's flag
bit 3 does not. Format-11 volumes remain available as raw resources;
requesting their unproved Texture decode throws.

Source flag `0x20` changes formats 10/11 to NV format `0x33` in `12C820`.
That layout is outside the proved NV6 cube decoder and is explicitly rejected,
while the raw resource retains its description and bytes. None of the 23,157
authored present records has that flag. Four unchanged original calls over
the actual Zanzibar `E17D0008` record freeze both ordinary and special
format-10/11 headers in
`tests/fixtures/retail_source_bitmap_low_detail_cube_formats.tsv` (20 words).
For format 11 the header changes from `0221062D` to `0221332D`.

`tools/audit_source_bitmap_low_detail_assets.py` inventories all 21 maps
and executes unchanged `12C820` / `359E0` for every present descriptor.
Its immutable fixture contains 35,442 records: 23,157 present and 12,285
absent, with 115,785 literal original header words. The present set has
22,486 planar images, 524 cubemaps, and 147 volumes. All present Common
fields are zero and all dimensions have the admitted power-of-two layout.
The complete report is
`analysis/multiplayer-shading/source-bitmap-low-detail-assets.json`; raw
descriptions and padded payloads are frozen in
`tests/fixtures/retail_source_bitmap_low_detail_assets.tsv` with adjacent
provenance. The original five-bitmap fixture remains separate and unchanged.

For all 524 cubes, unchanged XDK `3FDBC0` independently locks every face:
face N starts at the actual aliased shared base plus the record's offset
plus `N × 128`, with row pitch 16 and slice pitch 64. The 3,144 complete
calls retain 15,720 output words in
`tests/fixtures/retail_source_bitmap_low_detail_cube_layouts.tsv`. No face stride is
inferred from the native cubemap implementation. The expanded RGBA pixel
expectation uses the specified NV6 format conversion; it is not a GPU
sampling or framebuffer comparison.

`source_bitmap_low_detail_tests` checks all raw descriptions and padded
payloads against `retail_source_bitmap_low_detail_assets.tsv`, compares
1,200,056 admitted decoded bytes across the planar/cube descriptions,
checks every original header/face-layout row, and explicitly rejects all
147 volume decodes. The raw slots retain 3,318,272 bytes; their descriptions
admit 1,237,688 bytes, including the raw volumes. Twenty-five controls cover
absence, corrupt storage, unsupported decode, padding, and unchanged full
hardware images, including copied-map special-format rejection. The five Zanzibar low-detail images remain distinct from
the same-dimension hardware mip tails.

The registered CPU checks are `retail_source_bitmap_low_detail`,
`retail_source_bitmap_residency`, and `retail_source_bitmap_partitions`.
The API makes the independent authored images available to an explicit
caller. Selecting them as the current source `D15E0` resident/fallback
image still requires the actual cache handles, readiness, clocks, and
complete `12CE00` service. No default image or forced lower mip is installed.
