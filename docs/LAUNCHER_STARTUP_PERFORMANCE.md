# Launcher startup performance — October 29, 2026

Both root launchers are rebuilt and installed. Ordinary Play still opens the
selected MCC intro, title screen and menus. The selected engine remains
`bin/engine-builds/4bf0c86b3c41485193b431cf4f7da756/halo2_native`; its bytes,
capabilities, imported assets and runtime selections were preserved.

## Changes

- Draw the launcher immediately and check readiness on a background thread.
  Play stays disabled until the check passes. Tk widgets are updated only by
  the UI thread; closing the launcher never starts the game.
- Audit large shader packages with up to four spawned CPU processes, with at
  most twice that many module payloads outstanding. Small packages and hosts
  with one available CPU use serial admission. Each worker performs the same
  stage, executable-read and interface checks. Hashes, direct ownership and
  final file identity checks remain in the parent process.
- Decode each shader's instructions once. Reuse the authenticated frontend
  owner during its resource check instead of auditing its shader bank twice.
- Defer native readiness until after the home window's Play action. For an
  independently selected frontend, skip unrelated multiplayer admission and
  perform one frontend admission before dispatching the game. Setup, updates,
  diagnostics and explicit gameplay retain their respective admission paths.

No persistent validation cache or shader-quality reduction is introduced.
Authored intro duration and native game initialization remain unchanged.

## Measured results

Linux host, Xvfb at1280x720, dummy audio, OS and driver caches retained. Other
development builds and an existing game session were active. The baseline is
one pre-change run; installed values are medians of three new, serial launcher
processes. Times include20ms observation granularity.

| Measurement | Before | Installed |
| --- | ---: | ---: |
| Invoke launcher to visible, responsive home |28.743s |0.076s |
| Invoke launcher to available Play |Approximately28.7s |3.319s |
| Press Play to native executable dispatch |28.217s |3.058s |
| Press Play to game window |30.764s |5.775s |

Play-to-window time fell81.2% in these observations. Window creation is a
separate milestone from a rendered frame. A separate installed direct-launch
smoke (`--launch-game --frames 1 --mute --screenshot …`) completed successfully
and produced the first intro PPM in7.729s. That first-frame measurement has no
pre-change comparison and overlapped a startup-window probe.

On the actual3,079-module production package, three interleaved serial/four
worker comparisons returned identical admission results. Median package audit
time was3.457s with one worker and1.395s with four workers, a59.7% reduction.

These are local startup measurements, not disk-cold benchmarks, cross-platform
acceptance or new full-game/visual parity evidence.

## Validation and retained evidence

The selected-frontend regression first failed with two full owner audits
instead of one, then passed. Ten compiled CTest groups pass, covering all three
launcher variants, first run, frontend/gameplay routing, capability admission,
owner changes and the asynchronous home. Additional32 AOT tests and11 renderer
audit tests pass, including parallel result equivalence, changed bytes and
forged executable instruction contracts.

Small reports and logs remain under
`build/test-results/launcher-startup-20261029/`:

- [Summary and installed measurements](../build/test-results/launcher-startup-20261029/summary.json).
- [Installed hashes, source hashes and exact CMake command](../build/test-results/launcher-startup-20261029/delivery.json).
- [Diagnosis, original launcher hash and regression commands](../build/test-results/launcher-startup-20261029/diagnosis.json).
- [Compiled checks](../build/test-results/launcher-startup-20261029/cmake/report.json).
- [One/four worker timings](../build/test-results/launcher-startup-20261029/shader-workers.json).
- [First-frame receipt and discarded capture hash](../build/test-results/launcher-startup-20261029/first-frame.json).

All probes ran through `tools/run_test.py`; CMake validation used
`tools/run_tests.py --build-worker` within that owned scratch lifetime. The
private build and10.1MiB PPM were removed, and the temporary probe scripts were
retired. No large test artifacts were retained. Installed launcher binaries
are deliverables; existing runtime snapshots, reference evidence and unrelated
active work remain preserved.

## Forever Launcher and onboarding — October29

The shared home now uses **Forever Launcher** in the website's Arial type and
letter spacing, its navy/ice palette and the exact two website-authored ring
SVGs. The home contains Play, readiness and keyboard hints. Taglines and the old
Halo wordmark are removed. The setup window uses the same identity and artwork.
The static background is169,279 bytes; it adds no browser or animation runtime.

Onboarding now sends bounded, newline-framed UTF-8 progress records from the
owned CLI child. Percentages describe the **named task**, based on completed
bytes or items, rather than elapsed time or an estimated overall countdown.
Copying includes both the original write and independent destination readback.
Inspection, conversion, menu/font checks, presentation, publication and cache
selection have their own counters. A phase cannot emit100% before its final
checks; selection completion follows the registry's atomic replacement. Tasks
whose work is not yet known, including discovery, remain explicitly pending.

A native Canvas progress bar makes the fill agree with the number on both
platforms. The former themed bar drew no visible fill even with value37; a
pixel regression first failed, then passed with cyan at20% and the empty track
at80% for a37% update. Fragmented UTF-8 pipe records, invalid counters, failed
readback and cancellation during parallel writes have separate controls.

Large verified copies now use up to four threads with bounded1MiB buffers.
Small copies remain serial. Source identity checks and independent output
hashes remain required, and cancellation retires writers before stage cleanup.
Known incompatible presentation families are rejected before hashing both
entire MCC trees; compatible candidates still receive full verification.

Three-run medians on the actual copy/verification seam, using eight synthetic
128MiB files and warm caches, fell from0.996s to0.502s:49.6% less elapsed time.
The retained import metadata describes883 files /31.25GB, but a new complete
import was not run or timed. This is a bounded copy-stage result.

Linux validation passes139 cases with one skip. Native Windows Python/Tk under
Wine passes13 progress/protocol/pixel controls and ten copy/integrity controls.
An installed-package Windows preview also passes using actual counters from a
private64MiB copy. This is not physical-Windows or whole-game qualification.

The existing Windows local package at `dist/builds/windows-x86_64` receives the
same eleven UI/onboarding/artwork resources, with its package inventory updated.
Its engine and shader package hashes are unchanged. Linux uses the updated
shared source immediately. No imported assets or game selections were changed.

[Onboarding validation and exact commands](../build/test-results/onboarding-progress-20261029/validation.json)
and [delivery/source hashes](../build/test-results/onboarding-progress-20261029/delivery.json)
retain the bounded evidence. Final design previews are explicitly retained for
review: `build/test-results/launcher-redesign-20261029/{linux-ready,windows-ready}.png`
(372,458 bytes total) and
`build/test-results/onboarding-progress-20261029/{linux-progress,windows-packaged-progress}.png`
(302,404 bytes total). Other captures, temporary helpers, copy fixtures and Wine
prefixes are removed through their owned test lifetimes; no large test artifacts
are retained.

## Readiness repair — October29

An unavailable launcher now exposes Repair on Linux and Windows. Its owned
child verifies the private MCC cache and uses normal onboarding to rebuild it
when verification fails. Progress uses the shared measured counters. Errors
remain visible; users can choose their MCC folder and retry. Completion always
rechecks admission and returns to the launcher without automatically playing.
Missing or changed Windows package files are named explicitly and require
re-extracting the package; Repair does not invent replacement binary bytes.

The reported Proton failure reproduced with a valid imported cache and no
`halo2_native.exe.frontend-test.json`. For that case, Windows Repair verifies
the package inventory before running native menu qualification. The exact PE
executable uses SDL's executable-adjacent shader directory, so a fixed relative
Windows binding now authenticates that layout, executable SHA-256, bounded
read-only PE shader literal, complete shader package and frontend resources.
It retains the existing strict resource/readmission checks. Canonical binary
UTF-8/LF capability writes avoid Windows newline translation invalidating the
newly generated record.

Qualification distinguishes absent optional supplied movies from required menu
assets. Existing or declared supplied movie banks still require their movie
check. The older Windows executable implements MCC V3 loading; optional kit
files in a newer cache do not make it implement a different presentation. Its
qualification therefore pins and witnesses genuine MCC V3, preserving those
other files. The default supplied-movie qualification remains available.

The actual installed Windows package changed from missing validation to ready
through GE-Proton11-7 with native bundled Windows Python. Title, main-menu and
online-sign-in each rendered four nonuniform 960x720 frames after the actual
source-map intro cleared, with native clear-clock witnesses. Each native case
took8.6–9.3s on this run. Full package/cache verification plus qualification and
readmission took110.0s; this is a deliberate repair operation, not normal startup
latency. The passed capability pins48 frontend resources and the2150-module
shader package. Real sign-in, online gameplay, whole-game correctness and
physical Windows execution are outside this validation.

[Proton repair result](../build/test-results/launcher-repair-proton-20261029/repair-proton.json)
records the actual unavailable-to-ready transition. The installed bounded
receipt is `dist/builds/windows-x86_64/diagnostics/frontend-startup-check.json`;
its bytes match the capability's report hash. Existing runtime and shader
payloads were preserved. Failed diagnostic attempts are retained as small
reports; their private captures and state are retired by their owners.

The Windows UI passes10 regression cases under Proton. A controlled unavailable
state exposes Repair, then invokes the real owned repair child against the
installed package; completion enables Play without launching automatically.
The delivered C++ launcher independently reports an available frontend, and its
Play route exits successfully after four actual post-intro menu frames. That
separate probe took79.5s for launcher-info and114.2s for Play plus four frames;
these results establish routing/readiness, not a Windows startup speedup.
[UI result](../build/test-results/launcher-repair-proton-20261029/ui-proton.json),
[C++ launcher result](../build/test-results/launcher-repair-proton-20261029/launcher-proton.json)
and [focused checks/source hashes](../build/test-results/launcher-repair-proton-20261029/validation.json)
record the scope. The retained `windows-repair.png` preview is180,905 bytes and
shows the controlled unavailable state for review. Actual game captures,
private builds, temporary helpers and repair state were removed; no large
validation artifacts are retained.
