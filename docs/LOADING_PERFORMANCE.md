# Native loading performance

The native renderer uploads startup textures in bounded batches rather than
submitting, waiting and retiring staging allocations separately for every image.
Each batch retains at most 128 staging allocations or 32 MiB of pixel payload;
an individual larger image is transferred on its own. Staging buffers stay alive
until the batch completes. Initialization finishes all transfers before creating
the swapchain and rendering the first frame. Later movie-frame updates retain
their existing transfer path.

This changes native transfer scheduling only. Original image bytes, dimensions,
mip/face/depth ranges, decoded formats, samplers and layout/access barriers are
retained. No gameplay timing, asset quality, source admission rule or authored
movie duration is shortened. Existing strict floating-point compiler options
are retained.

Fresh single-config CMake builds default to `RelWithDebInfo` when no build type
is supplied. Explicit configurations, including `Debug`, are respected. Use
`Release` for the fastest ordinary launcher builds; the README includes the
build and installation commands. `--launcher-info` identifies the executable
actually selected by each launcher.

## Measurements, 2026-10-10

Measured on Linux with the NVIDIA GeForce RTX 5080, using the actual hidden
SDL/Vulkan renderer and original disc assets. Each value below is the median
process-start-to-first-captured-frame time across three new processes. OS file
caches and driver caches were retained; audio and validation were disabled for
the timing comparison. Other development workloads were present, so these are
observed host timings rather than isolated cold-disk benchmarks.

| Path | Optimized build, serial uploads | Batched uploads | Reduction |
| --- | ---: | ---: | ---: |
| Title prompt, installed Vista 2D art | 16.483 s | 6.767 s | 58.9% |
| Lockout Slayer, one local player | 7.575 s | 2.724 s | 64.0% |
| Lockout Slayer, four local players | 6.928 s | 3.435 s | 50.4% |
| First intro movie frame | 11.476 s | 6.556 s | 42.9% |

The older installed Debug runtime initially took 48.209 s for the title prompt
and 28.724 s for the one-player Lockout frame (one run each). Those initial
measurements also include build/source differences and are not an isolated
measurement of upload batching. The serial baseline above was linked with the
pre-change renderer; its reports and captured frames are retained separately.

## Repeating the measurement

`tools/benchmark_loading.py` runs scenarios serially, requires a successful
complete PPM frame, records stage times and executable/capture hashes, enforces
a timeout and writes `report.json` plus timestamped logs. Its fixed native fog
seed makes title captures repeatable. It does not change saved game settings.
The `boot` scenario samples the first movie frame, not the complete intro.

```bash
python3 tools/benchmark_loading.py --runtime build-release/halo2_native \
  --output analysis/loading/baseline --scenario title --scenario match \
  --scenario split --scenario boot --repetitions 3
# After rebuilding the changed runtime:
python3 tools/benchmark_loading.py --runtime build-release/halo2_native \
  --output analysis/loading/after --scenario title --scenario match \
  --scenario split --scenario boot --repetitions 3 \
  --compare analysis/loading/baseline/report.json --max-ratio .85
```

Use `--audio` to include actual audio startup, or `--validation` with installed
Vulkan validation layers to check transfers and cleanup. Compare reports with
the same audio/validation settings. The measured match scenarios use the
authored SMG variant and sample startup at tick zero; they do not establish a
full match or campaign playthrough, disk-cold performance, or another platform.

Session evidence is retained under `analysis/loading-20261010/`.

## Validation

- Title, intro, one-player and four-player startup pass Vulkan synchronization
  validation. The existing HUD framebuffer, special-texture GPU and renderer
  device-contract tests all pass. These exercise original cube/volume/mip
  resources, shader sampling, four HUD views and swapchain resizing.
- Controlled pre/post captures are byte-identical for the fixed-seed title,
  one-player Lockout and first intro frame. Four-player startup is captured
  and validated; a byte-identical whole-frame comparison is not claimed.
- Real SDL audio startup passes for the title, intro and match. Both installed
  launchers also render successfully with audio enabled: 5.903 s for the title
  launcher and 5.812 s for the normal launcher's first intro frame (one run
  each, using the launchers' imported game/UI sources).
- Current launcher setup/argument-routing checks pass, and `--launcher-info`
  confirms both installed launchers select `build-loading-20261010/halo2_native`.
- Dedicated LAN startup remains blocked by an existing empty-host lifecycle
  failure: clients receive `Match has ended`. Both the serial and batched
  executables report `players=0 finished=1` after one headless host tick,
  without constructing a renderer. That failure was reproduced with the
  pre-change renderer and is not repaired by this transfer-scheduling change.

## Movie and bitmap work, 2026-10-20

Bitmap preparation now validates the complete input span once and retains one
private writable output pointer. The pixel/channel operations, Morton address
calculation, format rejection and output remain the same. A differential probe
matched 4,942 accepted images and 5,810 rejected inputs. Its uncompressed 4K
case fell from 188.949 ms to 21.8912 ms. This CPU optimization applies across
GPU vendors; it does not establish an equivalent whole-match loading gain.

Movies now initialize and decode on one worker thread, with a bounded two-frame
prefetch queue and reusable RGBA buffers. Supported FFmpeg hardware decoding
uses CUDA, VAAPI, D3D11VA or VideoToolbox and falls back to software. VAAPI
downloads synchronize discarded frames during catch-up; skipping that transfer
reproduced incorrect AV1 output with the development machine's NVDEC VAAPI
wrapper. RGBA conversion can still be omitted for overdue intermediate frames.
See [release movie preparation](RELEASE_MOVIES.md#hardware-decoding-and-lossless-intro-playback)
for the optional lossless HEVC intro bank, supported backends and diagnostics.

Every original intro frame matches the prepared bank in decoded YUV (all 1,079
frames), including physical AMD VAAPI decoding. NVIDIA and AMD RGBA checks
compare the first 180 frames of both clips to the original software decoder,
including three fresh delayed-start/catch-up cases. Software original decoding
and its audio path remain available. Fixed 4K title and intro-frame-100 captures
match the installed frontend byte for byte. Four initial 1080p Lockout match
captures also match across original and updated texture leaves.

Physical decoder coverage is RTX 5080 CUDA and Ryzen 9800X3D integrated AMD
VAAPI. Intel, Windows and macOS hardware were unavailable. Selecting this
machine's AMD Vulkan ICD fails the renderer's existing combined D24S8,
graphics/compute and window-presentation admission; it cannot provide a
same-adapter AMD rendering benchmark. Mixing its tiny AMD
decoder with the NVIDIA renderer was slow and is recorded as a failed
performance configuration, rather than smooth AMD playback evidence.

The installed frontend update preserves the existing V24 link cohort and
shader inventory, replacing only movie and bitmap conversion leaves. Its
compatibility texture leaf omits only the newer copy-on-write publication call
when compiled against the installed vector-based texture ABI. All 325 original
link inputs were hashed before and after linking. A complete current-source
build also passed focused cache/residency validation, but was not selected
because the checkout contains unrelated active rendering/gameplay work.

Session reports, commands, hashes and retained-product inventories are under
`analysis/loading-movie-20261020/`. Raw test captures, compiler objects and
private CMake builds use the test-storage lifecycle. The selected runtime and
its lossless intro bank are retained runtime products; original assets, frozen
cohorts and the previous installed frontend are preserved.

Final process-start-to-first-frame Lockout medians were **28.872 s before and
20.860 s after** (27.8% lower), across three fresh processes per runtime at
1920x1080 with audio disabled. Median user CPU time fell from 22.138 s to
14.900 s. Runs were serial, with reversed order for the middle pair; OS/driver
caches were retained and unrelated host load varied. A separate earlier series
overlapping compiler/decoder tests was noisier and showed no median wall-time
gain. These numbers describe this measured host, rather than disk-cold or
cross-vendor loading guarantees.

The original runtime produced two first-frame variants during the final series;
all three candidate captures match an observed original variant. This is not a
claim that every match capture across the series is byte-identical. The fixed
title and intro-frame-100 comparisons are byte-identical.

The final CUDA run displayed 1,077 of the intro's 1,079 source frames on its
authored clock, with 169.2 rendered iterations/s, 9.92 ms p95 iteration interval,
and 0.006 ms mean decode wait. The encoded video remains 59.94 fps. The final
CPU fallback completed the same source-frame range and used the original
H.264 file; its mean decode wait was 0.948 ms, with occasional longer stalls.
An earlier software test of the HEVC copy performed poorly, so the compatibility
copy is now restricted to hardware decoding. A 360-iteration credits preview
averaged 92.3 rendered iterations/s; this is not a full credits playback claim.

## Further match-startup work, 2026-10-20

The next profile found repeated CPU asset processing, including complete image
hashing, scalar channel conversion, repeated MCC bitmap decoding/partition reads,
font atlas/map copies and full font authentication for every selector.
The selected frontend now uses `.local/p41/halo2_native`. Across three fresh
processes per runtime, 1920x1080 one-player Lockout startup to the first captured
frame fell from **19.219 s to 12.803 s** (33.4%). Median user CPU time fell from
**14.053 s to 8.125 s**. This compares the previously selected p36 with the new
performance leaves, in alternating order with OS/driver caches retained. Other
development work was active. It is not a disk-cold, audio-enabled, Intel/AMD GPU,
full-match, or menu-to-match timing claim. Twelve seconds remains substantial
and this work does not establish a sub-five-second loading budget.

- Linear BGRA conversion uses an unaligned-safe word shuffle with a portable
  byte fallback. Texture preparation transfers the completed base allocation
  instead of copying it again. All 4,942 accepted differential cases retain
  every pixel/mip field; 5,810 rejected inputs still reject. A focused 4K case
  measured 12.104 ms before and 1.754 ms after, separately from whole startup.
- The GPU image-sharing bucket samples 256 bytes for large images. Full image
  bytes, dimensions, format, layers, depth and mip layout still determine
  equality. An intentional sampled-hash collision must not share an image.
- Address-containment tag scans hoist unused filters, preserving exact query
  results and directory order, including overlap, zero-size and wrap cases.
- Font decoding and sampling retain one private output pointer and publish
  completed atlases for safe copy-on-write sharing in current source builds.
  MCC logical projection moves the physical glyph map before rebuilding
  logical metrics, and ordered insertions avoid repeated tree searches.
- Authentication retains only the eight fixed authenticated font payloads
  (3,874,022 bytes). Every validation still reads and compares all bytes of
  every font, including unused files; restored file dates do not hide changes.

The installed frontend retains all 325 original V24 link inputs and the
original shader inventory. Its compatibility leaves preserve the older vector
texture/Cache ABI. Completed-MCC bitmap residency has a 128 MiB / 512-entry
limit, keys full handles/subresources/palettes, and uses the frozen shader-table
control block as a cache-lifetime identity; import-time reads use the original
decoder. Returned mutable vectors stay independent. The separately compiled
partition reader and its helpers match the frozen reader byte for byte; its
existing complete-byte keys and 128 MiB / 512-entry limit are retained.
Changed files, malformed data and eviction still run the original validation.
The original gameplay link cohort and source asset/shader content are retained.

The real 4K title and intro-frame-100 comparisons are byte-identical. A seeded
one-player Lockout capture at source tick 1 also matches byte for byte. All
three timed candidate frames match an observed baseline variant. Differential
font checks cover all 11 gameplay selectors, 258 prompt glyphs and 4,196 classic
samples with identical atlas bytes, metrics and UV rectangles. Real bitmap
checks cover 40 accepted subresources and 74 matching rejections with repeated
loads and write isolation. Partition tests cover warm decode suppression,
changed input, two malformed reads, four concurrent readers, and 68 eviction
inputs. Six final current-source CMake font/value/collision tests pass; the
earlier full current-source native build and nine focused contracts also pass.
The local Vulkan validation layer is unavailable, so this round does not claim
a synchronization-validation pass. GPU movie playback and its lossless intro
bank are retained from the separately qualified movie work above.

Reports, commands, source pins, exact runtime/shader hashes and final storage
cleanup are under `analysis/match-loading-20261020/`. The qualified executable
(105,723,888 bytes), shader directory and lossless intro hard link are runtime
products; the canonical intro bank remains 1,658,954,183 bytes and is shared
with the previously installed runtime. Discarded candidates and the temporary
whole-source executable are removed after launcher validation; scratch builds,
objects, archives and raw frames are removed by their test owners.

## Five-second target, 2026-10-20

The selected frontend runtime is `.local/p53/halo2_native` (SHA-256
`c71680542642f9e9736a968b17883272e428dc9981e65d10c818adc956368437`).
Both existing launchers select it for the frontend and matches started there.
The separate native gameplay test selection remains its existing authority.
The five-second match-start target is **not met**.

This revision adds bounded CPU map decoding and reduces startup allocation and
copy work while retaining the installed gameplay cohort:

- Independent MCC zlib chunks use at most four workers per file, with workers
  reused across batches and at most 32 MiB of pending decoded output. Results
  publish in original chunk order, including short and mixed raw/zlib chunks.
  Worker creation failure falls back to the available workers and caller.
- Optional libdeflate uses runtime CPU dispatch. CMake enables it when
  libdeflate >= 1.9 is available; `HALO2_USE_LIBDEFLATE=OFF` builds the generic
  zlib path. Invalid, unsupported, empty, trailing, checksum and over-bound
  streams retain strict rejection through the reference decoder.
- Immutable tag ownership queries use a checked interval index and preserve
  original directory order and all query filters. The installed compatibility
  bank also uses metadata-only free-range searches and invalidated region
  lookup entries, avoiding whole-bank snapshots for CPU bitmap placement.
- Startup Vulkan uploads share one mapped 32 MiB staging buffer, retain the
  original transfer completion/barrier rules, and fall back to individual
  buffers when allocation fails or an image exceeds the pool. Descriptor sets
  allocate together and update in bounded groups with stable backing storage.
- Dynamic model preparation validates its authored input binding before
  copying a material. Selected slots and input transforms remain unchanged.
  Immutable authored light materials retain shared ownership where possible.

Three alternating fresh processes per runtime used the real hidden Vulkan
renderer, muted audio, 1920x1080 Lockout, one local player, and a complete first
captured frame. OS/driver caches were retained. Other development builds were
active on the AMD Ryzen 7 9800X3D / NVIDIA RTX 5080 host.

| Runtime | Process to first frame, median | User CPU, median |
| --- | ---: | ---: |
| Previously selected p41 | 12.330 s | 8.086 s |
| Selected p53 | 11.083 s | 7.320 s |

The paired wall-time reduction is 10.1%. Candidate runs were 11.083, 10.584 and
11.871 seconds. Peak startup RSS remains approximately 18.8 GiB. The decoder
microbenchmark is separate: on the same host, the four actual map payloads
matched the reference decoder byte-for-byte; `single_player_shared.map`
decoding averaged 0.249 s with the reference and 0.127 s with acceleration.
Those small decoder timings do not represent complete match startup.

The remaining profile includes substantial material/texture copies, font atlas
construction and copying, resource authentication, and GPU initialization.
The current-source completed-image sharing control failed at startup with
`Original runtime regions overlap`; it supplied no usable performance result
and was not selected. Replacing the installed image value representation
requires rebuilding its complete producer/consumer cohort consistently.

Qualification includes 100 codec admission/byte cases, eight simultaneous
callers, first/partial worker-creation failures, generic and accelerated builds,
and address/undefined sanitizers. Model checks cover 84 authored first-person
metadata banks, 470,925 expanded vertex identities, 292 multiplayer and 382
campaign program/negative-control cases. Bitmap checks cover 40 accepted
subresources, 74 matched rejections and independent writes. Existing partition
residency checks retain four-reader, changed-file and eviction controls.

Fixed 4K title and intro frame 100, plus seeded 1080p Lockout tick 1, match p41
byte-for-byte. Both launchers also produced the expected fixed frames after
selection. These focused checks do not certify a full playthrough or every
map. Vulkan synchronization validation was unavailable on this host.

Movie decoding retains the previously qualified CUDA, VAAPI, D3D11VA and
VideoToolbox paths with software fallback. The same lossless Main-profile
intro bank remains available for AMD/Intel-compatible decoding. Physical AMD
VAAPI and NVIDIA decoding were checked in the preceding movie qualification;
physical Intel hardware remains untested. See
[the movie qualification](../analysis/match-loading-20261020/summary.md).
libdeflate's dispatch and exact consumed-byte API are documented in its
[primary source](https://github.com/ebiggers/libdeflate) and
[v1.9 public header](https://github.com/ebiggers/libdeflate/blob/v1.9/libdeflate.h).

Receipts, commands, source hashes and scratch cleanup inventory are in
[`analysis/five-second-loading-20261020/`](../analysis/five-second-loading-20261020/).
Private validation builds and raw captures are disposable. The retained
installed executable is 105,743,312 bytes, plus its 1,427,092-byte shader bundle
and capability sidecar. Its 1,658,954,183-byte movie is a hardlink to the
existing canonical `resources/movie-playback/intro.hevc.mp4`, so it adds no
second movie payload. Previous p41 and p36 runtimes and original reference
providers remain preserved. This investigation's unselected candidate binaries
were retired after selection and launcher checks.

## Countdown zero to visible gameplay, 2026-10-20

The process-start measurements above do not measure this interval. Three real
menu/countdown runs on the current complete build averaged **12.89 seconds**
from countdown zero to displayed Lockout gameplay (13.63, 13.11 and 11.93 s).
The fade was estimated fully clear at 13.19 s on average. Startup and the
countdown are excluded; audio preparation remains enabled. GDB detached at zero,
and framebuffer sampling observed the gameplay image with about 0.11 s cadence.

The tested build was `.local/upscaling-menu-20261020/runtime/halo2_native`,
in Original mode with upscaling Off. The older p53 frontend selection remains
unchanged; its mixed-layout menu-to-match crash does not supply a timing sample.
The five-second target remains unmet. See the
[measurement report](../analysis/countdown-gameplay-20261020/summary.md)
for exact build identity, conditions, screenshots and cleaned scratch owners.


## Countdown loading halved, 2026-10-20

The new coherent performance build measures **5.089 s average** from real
countdown zero to first displayed Lockout gameplay, against **13.181 s** in
the matching before cohort, a **61.4% reduction**. Ordinary desktop runs were
5.195, 4.985 and 5.086 s; the original ten-second countdown is excluded. Audio
preparation/mixing stayed enabled. Original graphics, upscaling Off, one
player, 1920×1080 and the actual loading/spawn fade curves were preserved.
Equally prioritized before/after groups measured 12.613 → 5.484 s. Background
load varied; all final run receipts are retained. The strict five-second mean
remains just unmet.

Immutable map/font/HUD/resource/metadata work runs during the countdown;
world/player/RNG/clock construction stays at zero. Forced private-bank
page-out was removed, relocation lookups are indexed, completed images/fonts
share residency safely, and standard Vulkan pipeline caching and concurrent
complete texture fingerprinting reduce GPU setup. The existing loading fade
overlaps GPU preparation. GPU shader programs and gameplay logic are unchanged.

Fixed source-tick1 gameplay, title tick0 and intro frame100 match the before
build byte-for-byte. Fifteen focused CTests, 53 launcher/admission cases,
native MCC prompt checks and live functional loading handoff passed. Physical
GPU coverage here is NVIDIA; new paths have no NVIDIA-specific requirement.

The separately qualified MCC menu route selects
`.local/halve-loading-20261020/runtime/halo2_native`; use
`./Halo2-Launcher --menu`. Existing independent qualified multiplayer/default
Play and pStencil selections are preserved. This qualification grants no full
playthrough/all-map or full gameplay/visual/audio parity claim.

See [the final report](../analysis/halve-loading-20261020/summary.md) for exact
runtime/source hashes, conditions, byte evidence,retention and scratch cleanup.
