# Original lens query continuation

The TU1.5 native dispatcher now includes 20 complete original query bodies,
containing 1,319 instructions. They complement the existing 14-body
[request and effect phase](SOURCE_MAP_LENS_REQUESTS_TU15.md).
The original executable remains pinned to SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

| Original entry | Responsibility | Instructions |
| --- | --- | ---: |
| `20B90` | Query selection, original callback and ring issue | 144 |
| `20750` | Completion, ratio clamp, smoothing and retirement | 172 |
| `20D60` | Visibility normalization and linked request readout | 62 |
| `2E520` | Authored projected query quad | 283 |
| `48AF0` | Retained source camera projection | 171 |
| `3803B0` | Original startup normalization initializer | 3 |
| `C1E90` | Retained ring bit test | 11 |
| `402650/4026B0/402710` | SDK query metadata allocation, issue and readout | 98 |
| `40B080/405560` | Original clean query pipeline flush and primitive end | 71 |
| `32B199/32ECE9/32EA95/32EAE8/32EB88/32EBC3` | Original ceiling and complete CRT callees | 235 |
| `32CC0C/20E30` | Shared original integer conversion and slot retirement | 69 |

Generation authenticates direct original callers, the `2DEC8` callback pointer,
and the static initializer pointer at `46D818`. The SDK routines are in the
original **D3D section**, which the generator reads alongside `.text`.
Production contains statically compiled bodies, with no opcode interpreter.

## Original comparisons

`oracle_lens_quad_tu15.py` executes unchanged original `2FD70` to construct its
declared camera, then unchanged `2E520`, `48AF0`, CRT rounding and SDK primitive
emission. Its 45 cases cover nine actual authored definitions with near, far,
behind, off-axis and split-screen inputs. Thirty-six emit complete 96-byte
quads; nine correctly emit none. `2FD70` is the verified TU1.5 entry. Older
disc projection fixtures use different addresses; TU1.5 `2FD90/2F970` are
inside instructions and are not function entries.

`oracle_lens_query_state_tu15.py` adds 172 unchanged-original cases: 45 full
query issues, 63 completion cases, 63 readouts and the authentic initializer.
Metadata includes actual observed GPU sample counts and explicitly declared
half, saturated, empty, pending and view-three inputs. No callee or instruction
is replaced. The readout corpus does not cover its linked-record branch.

Eighteen additional unchanged-original cases check an occupied ring identity:
ready queries are reused and pending queries skip callback/issue. Each actual
authored definition gets both branches. `oracle_lens_quad_hardware_tu15.py`
executes the unchanged instructions on host i386, single-stepping 85,433
instructions across all 235 cases. It checks
the emulator's GP/SIMD, defined flags, instruction path and retained outputs.
Hardware confirms two emulator limitations: omitted x87 precision status and
omitted fixed bit six in some `FNSTCW` stores. The C++ fixtures retain the
hardware-observed bytes and status; the native comparison does not mask them.
All 235 cases pass using the real `SourceRuntimeMemory` owner, including full
retained caller scratch. The corpus reaches 930 instruction sites in 16 of
the 20 complete bodies. Registration is not evidence of every branch.

The ignored fixture files can be regenerated with the original pinned XBE,
the request fixtures, a real GPU test report and the tools above. Provenance
and the exact commands belong to `analysis/map-world-features/lens-query-20261015/`.
This is host CPU evidence, not an Xbox hardware comparison.

## Native graphics owner

`SourceLensQueryGpu` owns real Vulkan occlusion and timestamp pools for the
original 511 requests with three query identities each. Original emitted
vertices are submitted against the existing one-sample D24S8 world depth,
before foreground drawing, with W depth, LEQUAL, and color/depth writes disabled.
Only successful queue submission admits a pending query. Availability and
samples come from `vkGetQueryPoolResults`; empty original quads still receive
real begin/end queries. Generation checks prevent stale reuse.

The GPU test executes original quad kernels and submits visible, world-occluded
and empty geometry. It obtains **4,624 / 0 / 0** actual samples on the RTX 5080,
reuses the identities, and checks unchanged color, actual D24 depth and stencil
bytes with zero Vulkan validation errors. All 45 authored quad cases also match
their original sample areas against an unoccluded target. An explicit renderer
retirement test checks safe query-owner teardown. These are native GPU checks;
they do not establish Xbox sample coverage.

`SourceLensQueryFrameTu15` connects this owner to ordinary authoritative native
matches. It runs whole `20680` once per frame, preserves whole `20700` in the
actual particle view phase, and supplies the authenticated `2DED4` arguments
to whole `20B90`. Whole `20750` follows query issue as at `2C2F7`. The original
static initializer supplies `532098`; no normalization constant is guessed.
The source presentation counter advances only on a real native submission,
following the unconditional 64-bit increment in whole `13EF0` at
`14172/1417C`. A declined submission rolls back its query bits, sample-area
fields and metadata while preserving later views' requests and prior completions.

The native adapter retains an immutable query pipeline, bounded original ABI
emission storage, and pages corresponding to the real GPU query pool. It
leaves the source world's graphics device pointer untouched. Only actual
completed GPU samples/timestamps populate ready SDK metadata. Unreached SDK
allocation/flush services remain explicit missing owners; they do not return
fabricated success. The submitting source camera and complete projection bank
are retained, with rectangles expressed in actual attachment pixels, including
internal resolution and supersampling. Higher-resolution acceptance remains
separate from the 640x480 tests.

An ordinary Elongation 150-tick run first issues a genuine request at frame 136.
It issues 14 queries and observes 13 completed sample counts of 16 before the
final submission. The original visibility byte reaches200 at frame 149.
Four local views issue56 queries and observe52 completions: visible view zero
returns 16 samples; the other three actual viewpoints return zero. All four
source view records and the presentation stamp remain distinct and continuous.
Both runs finish without Vulkan errors and retain the same final gameplay
visual-event RNG words. These are bounded live query acceptance, not final
flare bitmap rendering.

## Remaining scope

The final reflection/bitmap flare draw at `2E9C0` and the direct-object callback
path `41C70/41C10/41970` remain unimplemented. Consequently Elongation's full
attachment graph remains **2/3**, the all-map graph count remains **93/96**,
and the 12 direct lens attachment rows retain their separate boundary.
Client presentation histories, late joins, reset/import, the two Vista maps,
native MCC gameplay entry, other incomplete map interactions and Xbox framebuffer
acceptance remain open. This continuation does not complete the all-map goal.

The next draw body is recovered and authenticated: `2E16C` pushes `2E9C0`,
whose complete body has 567 instructions. Its actual dependencies include
`2E3D0`, the now-tested `20D60` visibility readout, `166A80` retained animation
input, authored function/color owners, `2F2B0`, vertex geometry, bitmap binding
and SDK drawing. That recovery is preserved under `final-flare-recovery/`;
it is not a compiled or accepted visible flare consumer.
