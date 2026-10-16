# Original lens request continuation

The native TU1.5 world dispatcher now owns 14 complete original lens request,
effect-phase and request-pool bodies, containing 1,276 original instructions. Generation
uses the pinned original executable, with no opcode interpreter in production.
Actual calls at `179ABE/179C38` authenticate the effect request entry;
`17CBA6/17CBB3` and `41B0A/41B13` authenticate its shared helpers and the
direct-object path. The caller retains its actual source ABI and bank.

| Original entry | Responsibility | Instructions |
| --- | --- | ---: |
| `17C710` | Effect-part request branch | 376 |
| `3E9B0` | Owner/view test | 38 |
| `2DB80` | Lens request admission | 106 |
| `2E210` | Packed authored request construction | 108 |
| `20990` | Retained request lookup/allocation | 140 |
| `20B20` | Retained request linkage | 42 |
| `20E30` | Request-slot reset | 32 |
| `32CC0C` | Original float-to-integer helper | 37 |
| `134A50` | Original packed scalar helper | 32 |
| `179A40` | Ordinary effect-part request phase | 90 |
| `179B40` | Mode-three effect-part request phase | 125 |
| `17CD10` | Completed effect-part bookkeeping | 90 |
| `20680` | Original frame admission/reset gates | 36 |
| `20700` | Original view selection/retirement gates | 24 |

The request pool is distinct from the particle draw queue. Its record range is
`52DFDC..531FBC`, with 32-byte records and **511 slots**; `531FBC` is the
high-water word immediately following the records. The implementation does
not allocate a substitute pool or force its enable flag. Whole `20680/20700`
now supply admission from the actual retained display state and submitting
view. At the 14-body delivery, the display-stamp publisher and asynchronous
GPU query lifecycle were missing and the observed stamp remained zero. The
later [query continuation](SOURCE_MAP_LENS_QUERIES_TU15.md) advances this stamp
on actual native submissions and consumes real GPU results in bounded
ordinary one/four-view Elongation checks. Full presentation/network history
and final flare bitmap rendering retain their separate boundaries.

## Original/native comparison

`tools/oracle_lens_request_tu15.py` executes whole untouched `2DB80` and all
reached callees. It surveys all 21 Xbox caches and obtains nine distinct
map/definition pairs from their direct `lens` attachments and effect parts.
Its 72 cases use those actual definitions with declared owner/view, scale,
fade and retained-pool inputs. The original code admits 23 requests. It
rejects the NONE-view, NONE-tag, zero-scale, disabled-pool and full-pool cases.
Five actual definitions admit zero fade through their authored flag gate;
the native implementation preserves that distinction.

`original_tu15_lens_requests` compares complete instruction paths, all GP/SIMD
outputs, defined flags, retained request-pool and caller-argument bytes,
MXCSR and x87 status/depth. All 72 cases pass. The corpus reaches seven of
the nine bodies and 436 distinct instruction sites. It does not independently
execute whole `17C710` or `3E9B0`; registering those authenticated complete
bodies does not establish every caller's producer state.

The fixture uses explicit retained-pool inputs. It does not prove original
pool construction, frame reset, asynchronous query completion or GPU
occlusion. No instruction or external service is replaced in the original
oracle. The private tight loop uses a declared memory-bank adapter; the CMake
test uses the real `SourceRuntimeMemory` owner.

`tools/oracle_lens_frame_tu15.py` independently executes unchanged `20680/20700`
and reached `20E30` retirement. Its 42 cases cover negative, equal, advancing
and regressing frame stamps; ordinary and special modes; NONE and real views;
disabled/suppressed admission; and occupied/empty retirement. All 42 native
cases match complete instruction paths, GP/SIMD, defined flags, full retained
banks, MXCSR and x87 state. Logic instructions leave AF undefined, so these
cases exclude AF from the comparison. No original service or instruction is
replaced. Together the two fixtures contain 114 cases.
They reach nine of the 14 bodies and 496 distinct original instruction sites;
the effect-phase and owner/view callers retain separate live evidence below.

## Live boundary

The normal runtime links this module through `PhysicsLiveWorldTu15` and
preserves the existing original dispatch conditions. The accepted runtime and
live regression bank are recorded in
[normal launcher delivery](SOURCE_WORLD_LAUNCHER_DELIVERY.md).

This is a request-path continuation, not visible lens-flare acceptance.
This 14-body receipt preceded the separate completed query geometry and
GPU occlusion connection documented above. Final flare geometry/material
consumption and complete scheduling histories remain open. Elongation's spark
particles are separately visible through the
[marker correction](SOURCE_MAP_EFFECT_MARKERS_TU15.md), while its lens part
remains explicitly incomplete. Complete attachment-graph admission stays
93/96. The 12 direct lens attachment rows also remain outside that count.

The ordinary Headlong diagnostic reaches neither the public lens dispatcher
nor its `41C70/41970` parent callbacks during 15 ticks. All nine body counts
are zero; the separate parent/public probe excludes an optimized-body-only
explanation. Both diagnostic images and simulation/RNG records are exact.
That earlier nine-body module is therefore not evidence of ordinary lens
callback scheduling.

The restored `40F14/40F31` phase requests completed effect parts between
`136EA0` admission and the ordinary `1770C0` particle producer, retaining the
actual mode-owner branch and the separate first-person particle pass.
Actual Elongation gameplay reaches `179A40` 150 times, `17C710` 314 times and
both `3E9B0/2DB80` 14 times. The earlier 130-tick probe was before the original
spark's 4.556-second delay expired: its actual source clock was 4.333 seconds,
with the spark instance and location present. Neither delay nor RNG was
overridden to obtain those requests. A read-only probe then identified disabled
pool admission. Running whole `20680/20700` creates a genuine retained request
record by tick 150, with no forced pool fields or GPU visibility result.
Those private diagnostics were removed from the source before promotion.

The shared particle phase now runs once, after game ticks and before the
visibility reset. Original `12E142 -> 13AA50` precedes `12E151 -> 13A9B0`, whose
`13AA28` calls `179DF0`. The previous native swarm setup also advanced this
phase before game ticks; queue construction is now separate from advancement.
The explicit standalone `advance` API remains available to bounded fixtures.
Direct-object `41C70/41970` scheduling, display-stamp progression, GPU queries
and visible flare rendering remain open. The selected normal runtime's read-only observer sees 150 frame gates, 150
view gates, 150 ordinary effect phases, 314 effect-part callbacks and 14
requests/lookups/linkage calls over 150 frames. Its particle update count is
150, and its image and simulation/RNG record exactly match the ordinary run.
Current normal-build and live regression acceptance are recorded in the
[delivery document](SOURCE_WORLD_LAUNCHER_DELIVERY.md).

Evidence: `analysis/map-world-features/lens-request-20261015/`.
The frozen original/native fixture is
`tests/fixtures/lens_request_tu15_original.json`; generation provenance and
observed instruction coverage are retained alongside the original reports.
