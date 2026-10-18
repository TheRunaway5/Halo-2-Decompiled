# Opening recording pacing

The campaign-v2 recording is functional and has captured game audio, but its
native pacing is unfinished. Across all native render intervals after the first,
the median is 57.97 ms, p95 is 215.57 ms and maximum is 1662.85 ms. The renderer
median is 9.79 ms. A 30 fps capture stream therefore includes repeated frames;
it does not establish 30 distinct native frames per second.

The reproducible receipt is
`analysis/pstencil/campaign-live-v2/recording-window/recording-verification.json`.
The retained `verify_campaign_recording.py` measures actual decoded audio,
native completion and complete timing rows. Its method differs from the earlier
v1 observation, which excluded the first 20 initialization rows.

## Profiling evidence

The initial clock-sampling run completed, but its collector reported the timer
changing from 10 ms to zero. Its function percentages are not accepted as a
whole-opening profile. A delayed-start signal then terminated only the second
instrumented run. Both attempts and their warnings remain retained; neither
changes the normal runtime or the completed recording.

The third run uses user-mode hardware cycles at a fixed 50,000,003-cycle
interval, without a trigger signal or clock profiling. It completes normally
in 226.38 seconds with 2,687 native frames, zero script faults and zero Vulkan
validation messages. Child resource usage is 211.35 user CPU seconds and
12.18 system seconds. Hardware samples extend through execution second 225.

Reports filtered to timestamps at or beyond 40 seconds exclude initial loading.
Within those collected hardware cycles, `SourceRuntimeMemory::read` accounts for
17.80% exclusive cycles; memory validation and writes are also prominent.
The public full-bank `Renderer::update_material_constants` accounts for 12.01%
inclusive cycles, with `main` as its observed caller. Inclusive costs overlap
and must not be added. The collector converts cycles to displayed seconds using
its CPU-frequency estimate; those values are not independent wall-clock times.

The retained header still reports signal-mask and interval-timer warnings even
with clock profiling disabled. The hardware sample timeline establishes late
coverage, but these reports remain hotspot evidence rather than exact timing or
performance acceptance. Validation was enabled in every retained run.

Reports, commands, resource usage and hashes are under
`analysis/pstencil/campaign-pacing-v3/`. The graphics binary and shader bank
remain the hash-verified campaign-v2 delivery throughout these experiments.

## Next implementation gate

The campaign main loop has full-bank uploads after first-person ammunition
animation and weapon-HUD preparation. The renderer already supports ordered,
unique active root slots; multiplayer uses `MaterialFrameSelection` to retain
the closure of shared child passes. A campaign adapter must collect every
actual consumer before using that path: world, objects, foreground/background,
HUD, source bindings and effect-light materials. Limiting uploads to HUD
vertices alone has not been established as correct.

Any candidate must preserve source ticks, memory validation, mutation observers,
poses and shader inputs. Compare active and full material updates on the same
completed campaign frame, retain positive light/shadow contributions and exact
Original restoration, and measure an uninterrupted run before changing the
launcher. The validation below tracks that candidate through this gate.

## Active-material candidate

The new own `CampaignMaterialUpdates` module defers only pStencil's requested
uploads until the completed world, object, HUD and light consumers are known.
It uses the existing ordered active-root upload and shared-child closure.
General graph checks retain campaign child replacements; Original keeps its
immediate full-bank upload. No shader, source tick or simulation behavior is
changed. `HALO2_PSTENCIL_CAMPAIGN_ALL_MATERIALS=1` selects the earlier full-bank
path for an uninterrupted comparison using the same candidate binary.

The candidate's 2560×1378 full opening completes in 221.85 seconds with 3,814
native frames, zero script faults and zero Vulkan validation messages. All 20
same-completed-frame active-versus-full material images are byte-identical.
All 20 Original roundtrips restore exactly; 14 lighting/shadow pairs have
positive contributions and no shadow brightening. The shared-pass closure test
also passes 2,048 changing graphs, including new links, detachments and bank
reallocation.

That control run selects 179–357 roots from a 9,126-root bank, with a median
1.39 ms for graph selection and material update. It includes extra diagnostic
draws and readbacks, so its frame count and intervals are not a normal pacing
comparison. Matched uninterrupted full-bank and active runs at 1920×1080 with
the actual UI asset pack now both complete with zero script/Vulkan validation
messages. The full-bank run renders 2,720 frames with 44.06 ms median interval;
the active run renders 3,492 frames with 31.09 ms median interval. Their p95
intervals are 191.34 and 164.67 ms. Median material update cost falls from
8.56 to 1.50 ms. These are matched local observations, not a general frame-rate
guarantee; large source stalls remain.

The campaign-v3 manifest now pins the tested candidate, unchanged renderer and
all 64 previous shaders. Its actual F1 selector passes four switches in 104.21
seconds, including resumed native shadow owners. Its fresh owned-window
recording completes with 3,255 native frames, actual game audio, zero script
faults and zero Vulkan validation messages. The median native interval is
34.33 ms and p95 is 166.30 ms, compared with 57.97/215.57 ms in the prior v2
recording. These recordings are separate local runs with capture/audio work;
the matched native runs above isolate the material policy more directly.

The untrimmed campaign-v3 preview lasts 209.54 seconds at 1920×1080 H264/AAC.
Its FLAC source and receipts remain alongside it. Actual captured audio has
17,928,570 nonzero samples, peak 0.6893 and RMS 0.05957. Final audible balance
and synchronization remain unverified, and large frame stalls remain.
Evidence is under
`analysis/pstencil/campaign-material-updates-v2/` and `campaign-live-v3/`.

## Campaign-v4 native-window recording

The source-backed utility/dependent integration is pinned by campaign-live-v4.
Its retained private-Xvfb owned-window capture completes in232.56seconds with
1,281 native frames. Median interval101.263ms, renderer46.9919ms and
presentation37.5678ms identify substantial capture-display overhead. It remains
`recording-window/` diagnostic evidence.

The selected retry runs on the actual native Xwayland display `:0` and records
only its own SDL window ID35651631, retaining all pixels/audio without frame
interpolation. It completes in235.26seconds with3,207 native frames, zero script
faults or Vulkan validation messages. Its210.0915-second H264420/AAC preview and
uncut H264444/FLAC are in `campaign-live-v4/recording-window-native/`.

| Metric | Median ms | p95 ms | Maximum ms |
| --- | ---: | ---: | ---: |
| Native interval after first | 28.60895 | 207.237 | 1622.49 |
| Renderer | 8.01604 | 45.0645 | 117.015 |
| Geometry binding | 1.48105 | 37.0612 | 95.7864 |
| Presentation | 0.088399 | 0.117745 | 0.884174 |

This is a capture-environment comparison of the same renderer, not a controlled
shader-cost or full pacing parity result. Remaining source preparation/geometry
stalls are visible in the retained timing stream. The runtime guard observed
14.8GB resident peak and13.2GB swap under concurrent host pressure; memory
and pacing are not declared solved. A live GPU observation during playback
was3179MiB of16303MiB VRAM, so that observation does not support a VRAM-exhaustion
explanation for the private-display presentation cost.
