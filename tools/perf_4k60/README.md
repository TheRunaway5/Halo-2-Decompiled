# 4K60 split-screen performance tools

Support for [the native 4K60 goal](../../docs/NATIVE_4K60_SPLITSCREEN_GOAL.md).
Run everything through `tools/run_test.py`: builds of the preload libraries,
captures and timing files stay in `HALO2_TEST_SCRATCH` and are removed
afterwards. Only the small reports you direct elsewhere remain.

| Script | Purpose |
|---|---|
| `profile_run.sh` | One 4K-internal multiplayer run; timing summary, optional main-thread profile (`SAMPLE_DELAY` argument) or CPU ms/frame (`H2_CPUMON=1`) |
| `ab.sh`, `ab_env.sh` | Interleaved A/B of two runtimes, or one runtime with/without one or more environment assignments; reports main-thread CPU ms/frame (`H2_PERF_REPORTS` = report directory) |
| `parity.py` | Byte comparison of deterministic 3840×2160 captures between two runtimes (`VAR=VALUE,...@runtime` adds role environment; `--keep-lines REGEX` keeps matching log lines of every run beside the report and compares their digests too) |
| `census_roots.py`, `census_map.py` | Rank translated TU1.5 entry points / routines by executed instructions per frame from two `HALO2_INSTRUCTION_CENSUS` runs of different length (see docs/NATIVE_REPLACEMENT_GOAL.md) |
| `selflines.py` | Source-line attribution of one function's self samples (`PREFIX BINARY PATTERN [TOP]`); needs a binary configured with `-DHALO2_PROFILE_LINE_INFO=ON` (line tables only, no codegen change) |
| `analyze.py`, `buckets.py`, `callees.py`, `callers.py`, `callers_inc.py`, `children.py`, `diffprof.py` | Self/inclusive profiles, cost buckets, caller/child breakdowns and profile diffs from `sampler.c` output |

```bash
python3 tools/run_test.py -- tools/perf_4k60/profile_run.sh "$PWD/build/halo2_native" /tmp/reports base 4 300 ""
H2_PERF_REPORTS=/tmp/reports python3 tools/run_test.py -- tools/perf_4k60/ab.sh name 3 "$OLD/halo2_native" "$NEW/halo2_native"
python3 tools/run_test.py -- python3 tools/perf_4k60/parity.py /tmp/reports/parity.json \
    "HALO2_REFERENCE_MAIN_CULL=1@$PWD/build/halo2_native" "$PWD/build/halo2_native"
```

Run runtimes from their build directory (the EOS worker must sit beside the
binary). Compare parity only within one session: the Vulkan pipeline cache
changes 4p output between sessions. `parity.py` forces X11 under `xvfb-run`
and runs each absolute runtime from its own directory. Parity requires complete
3840×2160 RGB captures, successful capture processes and a deterministic first
reference repeat. Set `H2_REPEAT_CANDIDATE=1` to require a candidate repeat too.
Any invalid capture, timeout, repeat mismatch or reference/candidate mismatch
writes a failed report and returns a nonzero exit status. Raw frames are removed;
the report retains hashes, pixel differences and bounded failure log tails.
Vulkan and driver shader caches stay inside the same capture scratch owner.
Both tools also use private `XDG_DATA_HOME` and `XDG_CONFIG_HOME` directories
inside scratch and explicitly request Original graphics. This keeps saved user
upscaling preferences from changing the workload; fresh preferences use
Upscaler Off. The user's configuration is not modified.

Select the prepared MCC map directory explicitly when validating a runtime
against another imported cache. `parity.py --maps /absolute/path/to/h2_maps_win64_dx11`
overrides `H2_MAPS`; both parity and `profile_run.sh` use `H2_MAPS` when set and
otherwise retain the legacy `mcc-dire-valley-polish-20261023` cache. `H2_MAP`
selects the map name (default `coagulation`). The parity report pins the resolved
map directory, selected map path and map SHA-256. These options select inputs
for validation and do not change launcher cache selection or asset admission.

Profiling requests 3840×2160 internal rendering with a 1280×720 presentation
window. Before publishing timing summaries it requires every recorded timing
row, including warmup rows, to report those exact render and presentation sizes.
A missing dimension or a size mismatch returns failure with bounded diagnostics;
command line dimensions alone do not establish a 4K workload. Parity retains its
full 3840×2160 capture viewport. The CPU monitor excludes the final five frames
to keep its endpoint away from renderer destruction.

`H2_GPU_TIMINGS=1` also records same-queue GPU timestamps. Every completed CPU
frame must have one valid GPU sample with matching resolution/player count;
missing, malformed or incomplete samples fail the run. The optional query pool
is harvested after the existing frame fence and adds no wait. Its approximate
intervals cover compute, scene rendering and final copies (including diagnostic
readbacks when requested). They do not measure display latency or occupancy.
See the [Khronos timestamp guidance](https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html).

The preload sampler observes only the main thread. Under render overlap, use
an all-thread profiler for renderer attribution, or explicitly use
`HALO2_REFERENCE_SYNC_RENDER=1` for a synchronous diagnostic sample. Keep startup
and destruction outside the sample window. `HALO2_PSTENCIL_CAPTURE_PRESENT_MODE=immediate`
is an uncapped diagnostic control; ordinary launcher presentation retains FIFO.
Report its throughput separately from ordinary presentation and physical display
qualification.

## Switches added for the goal (2026-10-24..26)

Reference (restore the translated/original route): `HALO2_REFERENCE_MAIN_CULL`,
`HALO2_REFERENCE_MAIN_COMMANDS`, `HALO2_REFERENCE_CPU_SKINNING`,
`HALO2_REFERENCE_SERIAL_NATIVE`, `HALO2_REFERENCE_SEAT_CANDIDATES`,
`HALO2_REFERENCE_OBJECT_LOD_SKINS`, `HALO2_REFERENCE_SYNC_RENDER`,
`HALO2_REFERENCE_SERIAL_POSING` (the per-view posing lane: vehicles, corpses and
bodies back in the sequential order on the main thread; `HALO2_REFERENCE_SINGLE_POSING_LANE`
keeps the lane but runs vehicles and bodies on one thread instead of two; `profile_run.sh` keeps
the startup line as `<label>.lane.txt`). `HALO2_VERIFY_POSING_LANE=1` prints a
`Posing lane lists:` line per view and frame (digests of the records that carry
world offsets) and a `Posing lane timing:` line (lane/main milliseconds) for the
lane-vs-serial list oracle: `parity.py ... --keep-lines 'Posing lane (lists|skipped)'`.
`HALO2_REFERENCE_PHANTOM_REMAP=1`, `HALO2_REFERENCE_LIVE_ENTRY_SCAN=1`,
`HALO2_REFERENCE_LOCK_SERVICES=1` and `HALO2_REFERENCE_PRESENTED_STORE_INDEX=1` restore
the per-call remap search, linear entry scan, per-call lock services and per-scope
journal vector. `HALO2_REFERENCE_EAGER_COLLISION_BUILDS=1` builds every host collision BVH when its layer view is
composed (moving devices recompose every tick) instead of on the view's first query.
`HALO2_REFERENCE_PRESENTED_RELOCATION=1` relocates every presented root through B95D0 (default:
childless, attachment-free roots within 1e-4 of their simulation placement are presented in place).
`HALO2_REFERENCE_MACHINE_LOD_SKINS=1` skins every machine LOD slot (default: skin-identical slots
share one skin); `HALO2_VERIFY_MACHINE_LOD_SKINS=1` checks every shared slot against its own skin.
`HALO2_REFERENCE_SCENERY_VIEW_SKINS=1` skins retained scenery views per section draw (default: one
exact pose-cache skin per scenery LOD).
`HALO2_RELOCATION_CHANGE_CENSUS=1` prints, every 120 frames, which memory owners the presented-pose
relocation (B95D0) actually changed; `HALO2_EXPERIMENT_RELOCATION_RESTORE=owner,...` (`*` = all) puts
the original bytes back after the relocation (keeping the scope's root and sphere words) and
`HALO2_EXPERIMENT_RELOCATION_POISON=<hex byte>` writes that byte instead, for consumption tests.
`parity.py --render-fraction F` fixes another interpolation fraction and `--motion` scripts moving,
turning, jumping, grenade-throwing players.
`HALO2_REFERENCE_MATRIX_LEAVES=1` runs the translated 145560/144CF0 bodies instead of the
native leaf kernels; `HALO2_VERIFY_MATRIX_LEAVES=1` checks every admitted call against them.
`HALO2_INSTRUCTION_CENSUS=<file>` (with `HALO2_REFERENCE_INSTRUCTION_CALLBACKS=1`) writes
executed TU1.5 instruction counts. `HALO2_REFERENCE_WORLD_MEMORY_CALLS=1` restores per-use
world accessor calls in the command walk. `HALO2_REFERENCE_CPU_VEHICLE_SKINS=1` keeps vehicle skinning on the CPU;
`HALO2_REFERENCE_COPY_DEFERRED_POSES=1` copies admitted deferred poses instead of
borrowing the mesh. `HALO2_VERIFY_SECTION_INDEX=1` makes every model section selection (vehicles,
third-person bodies, source unit bases) also run the original linear scan and
reject any difference from the per-model section index.
The model-owned skin-admission cache is enabled by default
(`HALO2_NATIVE_SKIN_ADMISSION_CACHE=0` or an empty value restores full per-pose
admission). The banded cull is opt-in (`HALO2_NATIVE_BANDED_CULL=1`): it is
frame-identical but measured a net loss at its 0.10–0.16 ms per-view
granularity (2026-10-31). A cold model uses the full predicate;
only a repeated immutable bank builds its static summary.
Opt-in native paths awaiting a frame-parity capture: `HALO2_NATIVE_SHADOW_COMMANDS`,
`HALO2_GPU_OBJECT_SKINS` (measured slower; kept for reference).
Oracles and censuses (diagnostic, off by default): `HALO2_MAIN_COMMANDS_ORACLE`,
`HALO2_MAIN_COMMANDS_CENSUS`, `HALO2_MAIN_COMMANDS_CONTAINMENT`,
`HALO2_SEAT_CANDIDATE_ORACLE`, `HALO2_VERIFY_SKIN_ADMISSION`,
`HALO2_TICK_OWNER_CENSUS=<hex>` / `HALO2_CALL_CENSUS_EVERY`,
`HALO2_TICK_OWNER_DIGEST=<hex>`, `HALO2_NESTED_OWNER_CENSUS=<hex>`,
`HALO2_LOCATE_CENSUS`. The game prints `Render overlap: enabled|disabled`
once per run; A/B the overlap with `ab_env.sh name 3 <runtime>
HALO2_REFERENCE_SYNC_RENDER=1`. Keep a copy of any profiled binary beside its
profile: `children.py`/`callers.py` symbolize against the binary you pass.

## Copy and pose optimizations (2026-10-28)

These paths are enabled by default. For an exact same-executable comparison,
set the corresponding reference control to `1`:

| Reference control | Restored work |
|---|---|
| `HALO2_REFERENCE_COPY_FP_GRAPH_METADATA` | Copy the complete animation-graph metadata before reading its header |
| `HALO2_REFERENCE_BODY_POSE_JOURNAL` | Allocate a separate undo byte vector for every body-pose store |
| `HALO2_REFERENCE_DYNAMIC_OBJECT_POSE_CACHE` | Rebuild complete dynamic-object skins instead of reusing an identical pose |
| `HALO2_REFERENCE_FP_CONTEXT_MEMORY` | Walk ownership one byte at a time and copy private ranges again when repinning |
| `HALO2_REFERENCE_SKIN_PLAN_SCRATCH` | Allocate a fresh GPU-skin grouping planner for every rendered frame |

`HALO2_NATIVE_DYNAMIC_OBJECT_POSE_CACHE=0` and
`HALO2_NATIVE_FP_CONTEXT_MEMORY=0` also disable their respective paths; an empty
value disables them too. The explicit reference controls take precedence.
Pose reuse keys exact WORLD-node bytes and preserves the caller's FP/errno
state; GPU-capable keys include the incoming SSE precision flag because that
flag affects the existing admission decision. No animation rate is reduced.
`HALO2_DYNAMIC_OBJECT_POSE_DIAGNOSTICS=1` reports requests, builds and reuses.

The GPU-skin planner retains only cleared vector capacity between frames, with
an 8 MiB per-renderer limit. Frame exit, including exceptions and rejected draws,
releases every borrowed source, shared geometry owner, map node and count.
Oversized capacity is released at that boundary. `retained_bytes()` reports the
exact idle vector allocation total; the component report records the measured
43,932-record workload and its retained size. No previous frame's record or
palette values are reused, and all source/grouping checks still execute.

The launcher builder accepts repeated
`--renderer-4k-reference-control HALO2_REFERENCE_NAME=1` arguments. This requires
the packaged executable's defaults to match the supplied reference route in
four full-resolution cases before publication, with deterministic repeats of
the first case. Runtime controls inherited from the caller are removed from
both roles. The gate retains small hashes/reports and retires its raw frames.

Use `callees.py` for frame maps: `children.py` only counts parents within DEPTH frames of the leaf, so its inclusive numbers are depth-limited; `callees.py PREFIX BINARY PARENT [CHILD]` counts every sample containing the parent and breaks it down by the next frame toward the leaf (`<self>` when the parent is the leaf).
