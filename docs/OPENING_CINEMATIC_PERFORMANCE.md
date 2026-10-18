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
Source comparisons retain60MiB hard/64MiB measured/zero swap; serial builds
retain1GiB/zero swap; GPU previews retain3GiB/zero swap. Small logs, commands,
hashes and reports remain. No new large binary, raw-bank, frame or video artifact
is intentionally retained by these runs. Existing reference assets, frozen
snapshots and installed runtimes remain untouched.
