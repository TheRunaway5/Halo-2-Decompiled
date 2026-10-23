# Selective LTO execution trial

2026-10-22. Completed private experiment following the
[three-unit eligibility audit](MCC_PERFORMANCE_SELECTIVE_LTO_20261022.md).
The 4K120 one-player / 4K60 four-player goal remains open. No production
compiler settings, ordinary objects/archives or launcher selection were changed
by this experiment. **This configuration is rejected for production: it did
not establish a consistently positive CPU result.**

The complete checked-memory, live-world and build-specialized shadow units
were compiled with their original GCC 16.2.1 commands, redirected object and
dependency outputs, `-flto=3` and strict FP options. Their replacement objects
link ahead of the unchanged ordinary archives. Only the private frozen CMake
copy has the trial rules; its native target writes to a separate output folder.
The existing owned native build was reused; the experiment recompiles only
the three selected units.

## Link and code-generation admission

The official CMake-worker build and link-closure CTest pass. The replacement
objects have exactly the original public strong-definition sets: 67 memory,
144 live-world and 2 shadow entries. The link map contains none of the three
old archive members. All original source/object/archive hashes, original
compiler commands and the normal executable hash remain unchanged. No ODR,
type-mismatch or duplicate-definition diagnostics were emitted; existing
indentation/class-memaccess warnings remain visible in the build log.

| Runtime | SHA-256 |
| --- | --- |
| Ordinary motion+vector reference | `6c31c802d97015086e9884bebd6d8e5717c433d288897650030a78907e34f312` |
| Private three-unit LTO candidate | `fa4e458970efd9a7addc89819e53a31748109a46f722993d75095a4ece932b97` |

GCC reports speculative guarded devirtualization to checked memory read,
write and vector-read methods. Actual disassembly confirms changed shadow
code generation, but this does not establish lower CPU cost. For example,
`run_2e6290` grows from 123,950 to 147,144 bytes, about 18.7%. Its static call
instruction count changes from 2,345 to 2,361; indirect call sites increase
from 139 to 392 as helpers/fallbacks are inlined into that body. Static counts
do not measure dynamic execution frequency or executed CPU time. Checked
memory and generic adapter paths remain substantive costs.

[Preparation and exact commands](../build/test-results/mcc-performance-goal-20261022/selective-lto/preparation.json),
[build](../build/test-results/mcc-performance-goal-20261022/selective-lto/build-validation/report.json),
[link closure](../build/test-results/mcc-performance-goal-20261022/selective-lto/link-closure.json)
and [bounded disassembly findings](../build/test-results/mcc-performance-goal-20261022/selective-lto/hot-codegen.json)
retain the evidence. The approximately 20 MB full link map, complete IPA report
and generated objects were removed when the private trial closed.

## Trial gates and decision

- [x] Run semantic tests actually linked to the replacement objects. All twelve
      CTests pass, with no skips. Private
      memory-only clones cover vector fallback/wrap/device ordering, write
      dispatch/observers/concurrency, transfer aliases/boundaries, policy
      publication and the existing cursor contract compiled with LTO. Complete
      three-unit clones cover retained memory/heap continuation, the 146-case
      original shadow fixture and live-world continuation. The shadow fixture
      does not exercise the main plane-volume MOPP query.
- [x] Compare full 4K neutral/fire images with one/four players, keeping
      validation enabled, complete image bytes and both runtime hashes pinned.
      All four complete RGB comparisons pass, with zero validation errors.
- [ ] Measure ordinary unsampled frame time with alternating controls and
      recorded host contention. This was not started after the fixed-work CPU
      diagnostic failed to establish a consistent benefit; this candidate is
      not admitted for production or final FPS acceptance.
- [x] Recheck normal binary/object/archive preservation and remove private
      trial products when they are no longer required. Keep bounded reports.

[LTO-linked semantic validation](../build/test-results/mcc-performance-goal-20261022/selective-lto/semantic-validation/report.json)
and [complete 4K capture retry](../build/test-results/mcc-performance-goal-20261022/selective-lto/runtime-captures-retry/cohort.json)
record the successful checks. The initial cold-cache reference capture hit its
180-second limit before a frame while host load was about 44. That failure is
preserved in [the initial cohort](../build/test-results/mcc-performance-goal-20261022/selective-lto/runtime-captures/cohort.json).
The source-identical retry used a 360-second finite limit and a read-only copy
of an existing 23,589,027-byte same-driver pipeline cache. Normal cache header/
vendor/device/UUID validation and compilation fallback remained enabled.
All eight retry executions completed. Both completed capture owners, including
copied cache and raw images, are removed.

An additional private fixed-work diagnostic measured presenting-thread CPU
time between gameplay presents, with identical input/cadence/seed and separately
recorded wall intervals. It authenticates the scene callsite and excludes
other presents, matches the probe's complete 4K image with the admitted
uninstrumented image, aligns scene-present/native timing rows, and preserves
the FP environment. Its first admission incorrectly required the tick count
to equal rendered frames; tick3 in this path renders two frames. That failed
probe produced no accepted timing. The follow-up symbol lookup also rejected an
incorrect function name before launching a game. These probe failures do not
change the native candidate's existing correctness or image results.

Thread CPU time excludes descheduled time but still reflects CPU frequency,
cache and SMT contention, driver host work and probe overhead. Fixed-cadence
instrumented results are diagnostic CPU evidence, not ordinary FPS acceptance.
See [Linux clock semantics](https://man7.org/linux/man-pages/man3/clock_gettime.3.html).

Each timed run requests tick120 and produces119 complete gameplay frames.
Discarding30 warmup leaves89 main-thread CPU intervals per run. Three pairs
alternate execution order for each view count; all six final presentation
images match exactly. Internal rendering is3840×2160, presentation1280×720.

| Players / pair | Ordinary main CPU per frame | LTO main CPU per frame | CPU speed ratio |
| --- | ---: | ---: | ---: |
| One / 1 | 29.77 ms | 27.25 ms | 1.092x |
| One / 2 | 34.37 ms | 35.89 ms | 0.958x |
| One / 3 | 30.31 ms | 37.76 ms | 0.803x |
| Four / 1 | 70.39 ms | 91.85 ms | 0.766x |
| Four / 2 | 113.59 ms | 91.13 ms | 1.247x |
| Four / 3 | 68.47 ms | 92.49 ms | 0.740x |

Four of six pairs have higher candidate CPU cost. Residual host contention
and one slow unchanged-reference run prevent a precise regression percentage,
but these results do not justify admitting LTO. Fixed one-tick-per-frame work
differs from ordinary real-time render/tick scheduling; these CPU intervals
must not be converted into playable FPS. Exact clocks, callsites, commands,
source and per-frame tables are in the
[CPU diagnostic](../build/test-results/mcc-performance-goal-20261022/selective-lto/fixed-work-cpu/cohort.json).

The [storage/restoration receipt](../build/test-results/mcc-performance-goal-20261022/selective-lto/storage-and-restoration.json)
verifies removal of864,709,601 generated bytes: the trial candidate, objects,
maps and eight private test executables/object directories. All private helpers
are retired; their small texts/hashes remain in the
[experiment source report](../build/test-results/mcc-performance-goal-20261022/selective-lto/experiment-sources.json).
The frozen CMake file was restored byte-for-byte. Official worker validation
reports no native build work needed, a passing vector regression, and the
same ordinary binary/object/archive hashes. No large trial artifact is retained.

The outer owner `.local/test-scratch/halo2-test-0yux_h0i` remains active for
the next source optimization; its ordinary runtime is still required.
Normal source flags, all callbacks, fault publication, generic adapters,
instruction `noipa`, memory policies and FP controls are preserved.
