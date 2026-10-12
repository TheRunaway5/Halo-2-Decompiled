# Multiplayer frame preparation and measurement

The native renderer preserves the existing source-selected geometry, materials,
stipple patterns and ordered passes. A frame-local body/attached-weapon cache
avoids rebuilding the same pose for each split view. View-specific selection
and lighting still run independently. The existing source stipple helper is
evaluated once per selected part; consecutive identical draw keys reuse their
map iterator. The dynamic vertex staging vector reserves the complete validated
frame count once and reuses its allocation on later frames. Every frame clears
the staging contents before rebuilding its groups. No shader program, resolution,
visibility admission, animation event or gameplay clock was simplified.

The separate [initial CPU comparison](MP_RENDER_CPU_PROFILE_SUMMARY.md) retains
the original source/executable and byte comparisons. Body/node/attached-weapon
outputs match2,520 complete comparisons; stipple selection matches256 patterns
and80 complete BSP selections; grouping matches258 complete vertex/draw-request
outputs. A later [complete grouping-input audit](MP_VERTEX_RUN_PACK_RESEARCH.md)
retains19 actual calls and368,076 vertices. Its554 complete comparisons and1,082
failure-state controls also pass under sanitizers. The proposed run-append
alternative remains an isolated experiment and is not the production packer.

`Renderer::measure_frames` enables read-only host elapsed-time measurements.
`RendererFrameTiming` separates fence, global constants, per-view constants,
geometry packing/upload, image acquisition, command encoding, submission,
presentation and screenshot work. Profiling is disabled for ordinary play.
These are host/driver timings, not GPU timestamps or physical scanout intervals.
The standalone profiler records actual drawable dimensions and rejects external
gameplay input or admitted physical controllers. It does not inject a pose or
alternate simulation.

## Measured limits

The initial controlled1280×720 X11/RTX5080 comparison improves one-view active
frame medians38.70→27.00ms and four-view103.82→60.38ms. Later four-view profiling
separates geometry16.16ms from presentation21.38ms. Reserving the complete frame
reduces geometry to9.19ms. Retained staging then measures packing5.77ms,
upload2.62ms, per-view constants3.08ms and presentation20.79ms, with a49.23ms
active-frame median across90 post-warmup frames. Different measurements and
continued source revisions are retained separately; isolated byte comparisons
establish the transformations, not all possible runtime scenes.

A short actual desktop/Xwayland run records1280×720,150 post-warmup frames,
42.96ms active-frame median and0.21ms presentation median. Its inverse mean
active-frame indication is22.86/s; it is not full refresh. The one-view desktop
attempt is rejected because real external gameplay input triggered the probe's
explicit guard. It is not a comparable result. The initial offscreen display's
presentation overhead must not be described as the game's GPU execution cost.

There is no native30FPS render cap: simulation remains30Hz, rendering follows
the existing frame loop and FIFO surface. Full split-screen refresh remains
unachieved. Resident selected-BSP reuse, full current-pose ownership, broader
sandbox logic and actual second-machine LAN still require integration and
measurement. Fractional animation sampling does not establish full rendering
performance or exact original packed-codec arithmetic.

## Validated checkpoint

`build-kbm-20261008-201923` is a source-frozen Linux checkpoint. Its launcher
starts at the title screen using the previously verified frontend module while
the separate frontend work continues. Real SDL local one/four-view fire/reload,
native UDP one/four-view fire/reload, RMB scope, F1 roster, KBM delivery and the
original clock/registration/UDP tests pass. The four-view local probe records
260 fractional FP submissions and792 fractional body submissions; four-view UDP
records263 and468. Those are actual posed draw observations, not FPS benchmarks.

The UDP animation fraction uses the local fixed-step remainder independently
of the snapshot arrival window. Missing third-person intervals still cut;
contained FP pairs are accepted only after a callback-free source-clock step
reproduces their complete channel state. This fixes the observed fraction0
reset when every sub30FPS render receives a newer snapshot. It does not advance
events or replay damage, ammunition, sounds or effects.

The broad multiplayer goal remains active. Wheel/Tab, F and Q remain recorded
host requests until their complete source gameplay owners are connected. Live
authored muzzle effects are a separate ongoing integration, not claimed by this
checkpoint. Source assets remain external imports; the checkpoint records the
owned1.5 executable's data path/hash without copying or executing it.

Timing evidence is preserved in
`analysis/multiplayer/integration/render-cpu-profile-20261008/inner-phase-summary.json`
and the separately frozen `render-frame-timing-20261008` report. The checkpoint's
`validation/checks.json`, captures, source manifest and release checksums retain
its exact validation boundary.
