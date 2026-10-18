# Native opening recording presentation

The active Xwayland server reported no outputs. A minimal SDL/Vulkan clear and
the actual native renderer both reproduced roughly one-second FIFO image waits
without campaign assets or video encoding. Changing only presentation mode to
an advertised immediate mode removed that wait. This isolates the presentation
path; it does not establish the compositor's internal cause.

The renderer now accepts the recording-only
`HALO2_PSTENCIL_CAPTURE_PRESENT_MODE=immediate` override. It checks the surface's
advertised modes and rejects an unavailable request. Unset/default and explicit
`fifo` retain the existing FIFO behavior. No game assets, shaders, simulation
clocks, material parameters, shadow rules, or graphics-menu options change.

`tools/pstencil/record_campaign.py --present-mode immediate` applies the override
only to its child game. It still records only that PID's window and isolated
game audio. `--fullscreen-window` verifies a 1920×1080 client before capture;
ordinary window capture reads the actual client dimensions.

## Evidence and limits

- `analysis/pstencil/presentation-probe-v1/native-before/`: actual renderer,
 12 frames in 6 seconds, steady-state median image acquisition 1000.400ms.
- `analysis/pstencil/capture-presentation-native-v6/native-default/`: rebuilt
 renderer with no override, 12 frames in 6 seconds; default behavior retained.
- `native-immediate/`: same renderer, 29,964 frames in 6 seconds with a diagnostic
 triangle, median acquisition 0.002224ms; zero Vulkan validation messages.
 These counts describe an asset-free presentation test, not game performance.
- Full production opening recording: 1,642 native frames, zero script faults
 and Vulkan validation messages, acquisition median0.025488ms. The uncut
1920×1080 H264444/FLAC file and game-only audio remain under
 `analysis/pstencil/campaign-live-v6/recording-window-native/`.
- `opening-presentation.mp4` is the 208.7385-second H264420/AAC compatibility
 copy. It removes 54.850541 seconds before the first native scene draw, retaining
 a half-second preroll. The trace stream opens immediately before that draw;
 its creation timestamp and the recorder's first input timestamp anchor the
 cut. Audio before the cut is verified silent. The uncut source is retained;
 no simulation timing or video frame interpolation changes.
- The actual F1/mouse selector passes four switches in 115.55 seconds. An
 analysis-only projected-material timing leaf using the same renderer/SPVs
 passes 20 exact Original roundtrips and 14 positive light/shadow pairs, with no
 shadow brightening or Vulkan validation errors.

Native frame interval median remains 54.374ms, p95 345.162ms, maximum 7078.09ms.
The video has repeated presented frames and must not be called 30 distinct native
frames per second. Initial preparation and other frame hitches remain visible.
Audio contains 17,920,762 nonzero samples, peak 0.68573 and RMS 0.05290; final mix
and dialogue synchronization remain unverified.

Projected companion completion was measured before considering another cache:
15,041 completions across 9,904 distinct per-light shader/LOD keys consumed
727.23ms across the full instrumented run. This does not explain the large
hitches, so that speculative optimization was not implemented. The profile is
retained at `analysis/pstencil/projected-response-profile-v1/native-highres/`.

The fast acquisition check is
`python tools/pstencil/check_presentation_wait.py TIMINGS.tsv`; small probes
use `--skip 6 --minimum-frames 3` to exclude initial swapchain queue filling.
This checks the isolated wait only; it is not full presentation acceptance.
Full 2003 material/normal accumulation, asset/face coverage, original GPU
comparison and native pacing gates remain active.
