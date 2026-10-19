# Opening cinematic performance — 2026-10-19

Performance is the first priority following the user's slideshow report. The
remaining visual/audio reports are still open: fades, missing Arbiter “Pillar
of Autumn” dialogue, flicker, planet/High Charity ordering, all-character shader
and mouth checks, interpolation, completion/handoff and the finished recording.

The current coherent V651 cohort passes18 selected CMake checks and the
separate guarded original/native camera comparison. V658's full visible
1280x720 run records8408 frames across202.96 seconds: interval median/p95/max
21.28/36.52/140.97ms, renderer CPU median/p957.84/13.19ms and vertex packing
median1.13ms. Its first60 cumulative rendered seconds contain2609 frames with
interval median/p95/max20.64/35.43/80.85ms. Still captures, a16-second external
recording and concurrent validation are disclosed. This is materially more
useful pacing evidence than a full-run average, but it is not a matched,
isolated before/after benchmark or uniformly smooth60FPS acceptance.

The ring GPU readback and debugger probes V657/V660/V662 intentionally
perturb frame timing and are excluded from performance claims. V651's
four-job compiler uses8GiB/zero swap; native GPU diagnostic runs use6GiB/zero
swap. Original/native source comparison bounds remain unchanged.

The isolated reverse blast is a presentation interpolation defect: V673/V675
hold source poses and requests fixed while changing only the render fraction
at the authored terminal scale reset. The new metadata is prepared with the
model resources; the draw loop performs only exact reset comparisons and
continues ordinary pose interpolation. V681/V682 establish the focused
regression and old-behavior control. V683 passes20 CMake checks and the
unchanged guarded camera comparison. Its V684 forced-fraction GPU regression
and V686 normal visible full run both pass the ring reset acceptance.

V686's197.12-second visible run reports9750 measured1280x720 intervals with
median/p95/max19.46/31.13/130.98ms, renderer CPU median/p956.73/13.53ms and
packing median0.82ms. Its first60.02 cumulative rendered seconds contain2861
intervals at19.78/31.05/130.98ms. Window-title observation and a0.25-second
metadata audit are disclosed in the first60 sample; the full run also includes
a16-second external recording. No debugger or own compiler runs concurrently.
These are useful current pacing measurements, not a matched isolated speedup
or uniform60FPS acceptance. No FPS gain is inferred from the ring GPU controls.

V628 measures the complete v621 cohort at1280x720:657 sampled frames in60.06
clean playback seconds, with interval median/p95/max65.67/250.30/463.53ms.
Renderer CPU median is8.22ms. A concurrent compiler is disclosed; debugger
samples follow the clean interval. This remains poor and establishes no
isolated page-index speedup. The later visible v629 full run has median/p95
28.55/56.54ms over6396 frames, but includes resizing from1280x720 to2560x1378
and reference captures. It is not a controlled comparison with the clean run.

The writable memory owner now reuses its already located region node rather
than searching the map again. Validation, access/commit checks, guards and
write-observer ordering remain unchanged. V634 passes7 focused memory checks.
Its12-million-write/read benchmark has equal checksums and measures202.44ms
versus507.36ms when only the redundant write lookup is restored. Concurrent
work is disclosed. This benchmark does not establish a game FPS improvement.
The coherent v636 build compiles but fails its new material test's incorrect
placement-count assertion; no runtime is delivered. The corrected v639 build
must pass before its pacing can be measured.

The later v599 O1 cohort passes14 checks and its clean v613 measurement gives
42.86/89.62/151.76ms median/p95/max intervals,1260 frames in60.04 seconds and
about1.87GiB memory peak. The v612 O2 cohort passes15 checks; clean v615 gives
34.76/65.27/133.07ms,1507 frames in60.04 seconds and2,263,269,376 bytes peak.
Renderer CPU median is5.19ms in v615. Debugger samples follow the clean interval:
nine of12 involve translated simulation/address resolution, one catches sound
decoding and one catches renderer work. They identify hot paths without
establishing exact sample percentages. Both candidates still fail the desired
frame pacing. The visible O2 v616 run reaches all13 scenes and completion;
full-run averages do not establish uniformly smooth playback.

The native memory owner now indexes fully covered source pages in a sparse
two-level table. Partial pages, gaps, cross-owner transfers, commitment, access
policy and write observers retain their existing checks. Entries move with the
actual region nodes; release/restore/swap and failed alias publication are
covered. V620's matching-checksum12-million-read benchmark measures100.1ms
indexed versus645.4ms interval lookup under concurrent work. V623 catches a
rollback defect for identical shared aliases; v624 passes after correcting it.
This is a lookup benchmark, not a claimed game FPS gain. The coherent O2 v621
build uses8GiB/zero swap and two compiler jobs, with source hashes and controlled
pre-consumer source amendments recorded. Original/native comparison guards
remain60MiB hard/64MiB measured/zero swap. The clean v628 result above measures
the complete resulting cohort, including other frozen renderer work.

The reported first fade is specifically space-to-council. The v622 read-only
native observer confirms the first capture already has the incoming camera.
The caller now submits a render at the source capture request and carries
unused admitted ticks/elapsed into the following frame. The old-loop negative
control fails; new scheduling and existing original clock fixtures pass
ASan/UBSan. This is a source-clock-preserving native presentation change;
actual corrected transition pixels and end-to-end pacing remain to be checked.

Latest actual candidate v594: the3GiB startup trial v595 still fails. Under a
separately declared6GiB diagnostic bound, v600 starts rendering after17.23s
and records907 rows across60 clean playback seconds. After the first10 rows,
frame interval median/p95/max are64.59/109.85/473.56ms; renderer CPU submission
median/p95/max are5.65/8.38/23.62ms. There are66 intervals over100ms. This is
still about15–16FPS at the median, confirmed by the user's15–20FPS report in
the visible v574 run. The memory bound and source cohort differ from the old
v564 baseline, so this is not an isolated before/after speedup comparison.
The serial v599 compile is concurrent and disclosed. The debugger attaches
only after the clean sample; it observes actual fractional moving Prophet
poses across two-tick draw gaps, without contributing to these cadence values.

V574 runs visibly with PulseAudio, executes all13 authored scenes in order,
reports completion around tick5731 and exits0 with no script/audio faults.
The user reports stable audio pacing, but the Arbiter reply remains inaudible.
Its observed computed volume begins at-3306 hundredths dB; all157850 packet
frames are consumed. Final direct/spatial output mixing remains unresolved.

New startup samples v596 still land in full MaterialPass-bank copies. The
indexed bank now shares unchanged records and copies only edited records,
preserving value semantics for escaped references and moves. The actual light
fan-out regression fails on old storage (v597), passes on new storage (v598)
and passes ASan/UBSan (v601). V599 is rebuilding the complete ABI cohort and
includes14 selected checks plus separate caption-layout validation. Its actual
memory and pacing improvements have not yet been established.

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
with Clang/GCC (v562). Source-camera controls pass (v580). The coherent v586
runtime built and passed its seven selected checks. Clean candidate cadence
and visible acceptance remain pending. Building this
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

The coherent v586 runtime failed before its first frame under both3GiB (v573)
and8GiB (v588) startup bounds. The latter grew roughly600MiB per second.
Neither run is an FPS sample. Six bounded stack samples (v589) locate repeated
full Material copies and bitmap decodes while preparing model/BSP effect lights.
Material image bytes now share readonly residency with independent writable
copies; completed native bitmap decodes are memoized by datum, image index and
exact palette bytes. PC/MCC decode validation is preserved. The real light-pass
fan-out regression fails before the storage change (v590) and passes afterward
(v591), including pointer/iterator escape, move and concurrent-copy boundaries.
ASan/UBSan passes (v593). Actual intro bitmap residency and existing cache/mip
bounds pass (v592). Full v594 runtime startup and clean playback remain pending.

The v586 executable/shaders are retained as a required startup-comparison
input:97,302,183 bytes in
`analysis/opening-cinematic-20261015/current-cutscene-runtime-v586/runtime`.
Their source/build scratch was removed. The in-progress v594 delivery build
explicitly retains its executable/shaders for the requested visible preview;
the outer test owner removes its private source/build on every exit. No finished
video has been produced.
