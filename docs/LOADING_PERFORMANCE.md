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
