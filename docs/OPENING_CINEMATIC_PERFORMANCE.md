# Opening cinematic performance — 2026-10-18

Performance is the first priority following the user's slideshow report. The
remaining visual/audio reports are still open: fades, missing Arbiter “Pillar
of Autumn” dialogue, flicker, planet/High Charity ordering, all-character shader
and mouth checks, interpolation, completion/handoff and the finished recording.

The visible v543 preview uses the previously verified full-cutscene diagnostic
runtime, not the newest live DSP chain. It runs on the RTX5080 Vulkan device,
not a software renderer. It reached the3GiB memory ceiling, returned status0
after200 seconds and4570 rendered frames, and its temporary directory was
removed. A full-run average does not describe frame pacing or prove the absence
of the user's reported stalls.

The first two sampling probes retain CPU stack samples and creating-cgroup
memory counters. The moving probe v549 samples translated simulation calls,
`SourceRuntimeMemory::read/validate`, scalar stores, visibility, skinning and
material updates. The fixed tick972 probe v548 is much faster than moving
playback. Both encounter the memory ceiling. Their debugger attachments stop
the process briefly; frame-interval outliers in those reports include that
overhead and must not be presented as clean FPS measurements. These probes
do not prove sustained0.5FPS or explain every reported stall.

One concrete redundant operation is fixed in `physics_step_internal.hpp`:
unobserved scalar stores read their destination solely to construct a preimage
that nobody consumes. The original MOV writes the destination. The native
helper now performs the real write once without that synthetic read. Bounds,
permissions and write guards remain in the memory owner. An observed store
still reads its preimage, performs the write, then publishes the unchanged
ordered trace. Source arithmetic, authored sleeps and simulation clocks are
unchanged. This removes one memory-interface call per unobserved scalar store;
it is not a measured end-to-end FPS gain.

The updated scalar regression fails on the previous helper (v553). It verifies
write-only destinations, exact widths/bytes, no unobserved allocations, rejected
writes and ordered observed failures. Clang/GCC v557/v558 pass, together with
the EP software initializer and79 unchanged-original caller cases, each run
observed/unobserved:158 comparisons and196617 ordered writes. All70 previous
case files remain byte-identical. These comparisons cover the selected callers,
not every consumer of the shared store helper or complete game behavior.

A read-fastpath preview experiment v550–v552 could not be linked because the
historical build's dependent archives are absent. Its temporary outputs were
removed. It was not installed, shown to the user or counted as a speedup.
Current production `SourceRuntimeMemory::read` already has a single-owner fast
path. A coherent runtime rebuild and clean before/after visible measurements
are required to deliver and assess the new store change. The earlier diagnostic
executable remains unchanged.

The clean visible v560 probe records frame-preparation cadence, actual GPU
utilization and memory counters without debugger attachments. It is a bounded
preview of the existing runtime, not the finished cutscene or a recording.
Its results and the remaining delivery gate are recorded in
`analysis/opening-cinematic-20261015/opening-performance-visible-v560`.
The60-second clean probe records2128 preparation samples: median23.45ms,
95th percentile66.49ms and maximum408.45ms. Across30 device samples the GPU
median is17.5% and maximum40%; sampled game threads use approximately one full
CPU core. This supports a CPU-side bottleneck in this bounded segment, while
not reproducing sustained0.5FPS. These timestamps are collected before draw
submission, not physical display scanout. Memory-limit hits occur, but measured
memory-pressure stall time totals only9.5ms; they do not explain the long frame
gaps by themselves. This probe still uses the unchanged historical executable.

Every new CMake check uses `tools/run_tests.py`; one-off probes use
`tools/run_test.py` and generated binaries/banks stay in owned scratch.
Source comparisons retain60MiB hard/64MiB measured/zero swap. Earlier serial
builds used1GiB/zero swap; the v586 delivery build uses2GiB/zero swap after
measured compiler exhaustion. GPU previews retain3GiB/zero swap. Small logs, commands,
hashes and reports remain. No new large binary, raw-bank, frame or video artifact
is intentionally retained by these runs. Existing reference assets, frozen
snapshots and installed runtimes remain untouched.

The hidden v564 baseline uses the same diagnostic executable at1280x720 with
Original graphics and dummy audio. Its first60 seconds have2278 frame rows;
after the first10, median interval is22.68ms,95th percentile62.53ms and maximum
180.46ms. Renderer CPU submission has median3.31ms/95th percentile10.17ms;
these are not GPU timestamps. There are57 intervals above66.67ms and12 above
100ms. A concurrent serial compile is disclosed. The bulk of interval cost
is upstream of renderer submission in this sample; this does not identify
every camera-cut stall. Small logs/TSV/report are in
`analysis/opening-cinematic-20261015/cutscene-frame-baseline-v564`; its private
scratch was removed and it retains no frame captures.

The current changes retain completed NPC pose history across slow draws and
avoid a second legacy camera-animation decode when the source observer owns
the camera. Focused interpolation checks fail on the old history and pass
with Clang/GCC (v562). Source-camera controls pass (v580); the coherent v586 runtime,
clean candidate cadence and visible acceptance remain pending. Building this
runtime also required partitioning the generated physics-support module's2025
unchanged function bodies into32 translation units after the monolith hit
the1GiB build cap. Compiler peak/linked-helper checks are v569/v570; no gameplay
speedup follows from partitioning alone.

The v571 build passed the32 physics partitions but later exhausted its1GiB
limit in the authority archive serializer. Its reader and writer now compile
separately without changing field traversal, public function bodies or the
archive format. ClangO1 compiles both in14.8 seconds with972.3MiB peak (v578);
the compile-only project's missing CTest registration remains a recorded runner
failure, not a codec functional pass. Actual nested codec checks are part of
v579. Failed runtime scratch was removed; no new runtime or video was delivered
by v565/v571.

The v579 build passed the serializer but hit the1GiB cap compiling the final
multiplayer module. Normal ClangO1 compiles that unchanged module in11.75
seconds under a2GiB bound, with1276220KiB compiler peak (v585). The delivery
build now uses2GiB, one compiler job, zero swap and LLD. AGENTS.md specifies
owned cleanup but no compiler RAM cap. Original-data comparison guards retain
their existing60/64MiB limits. Compiler memory measurements are not game FPS.

The real source-camera control v580 passes on00a/03b; restoring the duplicate
legacy decode triggers the exact regression assertion under the same source
comparison limits. Cinematic variant names are now retained with model
resources instead of reparsing full definitions on every character draw. The
current source variant index remains live. These remove known repeated work;
clean candidate pacing and visible NPC/camera acceptance remain pending.
