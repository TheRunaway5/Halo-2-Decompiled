# Test storage

Normal game builds exclude test executables by default. Build a specific test
target when using a persistent development build, or use `halo2_tests` to build
the complete suite. `-DHALO2_TESTS_IN_ALL=ON` restores the previous default-build
behavior when explicitly needed.

## Disposable CMake validation

```bash
python3 tools/run_tests.py --target movie_tests --filter '^frontend_movie_routing$'
python3 tools/run_tests.py --sanitizers --target movie_tests --filter '^frontend_movie_routing$'
```

Without `--target` and `--filter`, the runner builds `halo2_tests` and runs the
full suite. Focused builds should supply both: a filter alone does not reduce
the build targets. Release builds and two jobs are the default to reduce the
size of linked test programs. Use `--config Debug` when debugging requires
symbols. Extra configure options use
`--cmake-arg=-DOPTION=VALUE`.

For launcher-only validation against an existing native runtime, pass
`--cmake-arg=-DHALO2_BUILD_LAUNCHERS_WITH_NATIVE=OFF` and
`--cmake-arg=-DHALO2_LAUNCHER_RUNTIME_DIR=/absolute/path/to/runtime-directory`.
This builds the launcher targets without their default native-game dependency;
normal source builds retain that dependency by default.

Use `--test-jobs` to limit concurrent CTests independently of build `--jobs`,
for example `--jobs 6 --test-jobs 2` for fixtures that own several native worlds.
Without that option, both phases use `--jobs` as before.

The private build directory, created under `.local/test-scratch` on disk,
is removed after success, failure, or cancellation. Override its parent with
`--scratch-root`; using the system `/tmp` can exhaust its small RAM filesystem.
The latest `configure.log`, `build.log`, `tests.log`, and `report.json` remain in
`build/test-results`; each log is bounded to 8 MiB. Choose a different `--output`
directory for concurrent runs. Failures preserve their real exit status.
`--keep-artifacts` explicitly retains the private build and prints its path.

Use repeatable `--retain-report relative/path.json` options to keep selected
small build/test reports before the private build is removed. Each must stay
inside that build and is limited to 1 MiB, with an 8 MiB total limit. Supported
report suffixes are JSON, log, text, TSV, CSV, Markdown and XML; executable
content and links outside the private build are rejected. Exact bytes, SHA-256 hashes, missing
outputs and size-limit rejections are recorded in the runner report. Copies go
under the report directory's `evidence` folder. This does not retain binaries,
raw banks or the build tree.

## CTest scratch and one-off validation

Live physics acceptance uses the prepared imported MCC cache through
`tools/test_physics_multiplayer_visible_tu15.py --mcc-root PATH`. Xbox TU1.5
execution and assets remain offline references. Runtime acceptance rejects
Xbox/Vista cache inputs and keeps raw outputs under `HALO2_TEST_SCRATCH`.
Use `--report-output PATH` for bounded results and add `--retain-video` only
when retaining the requested visible proof. The enclosing `tools/run_test.py`
removes the private outputs after success, failure, or cancellation.

Native input/body reports and authority phantom/velocity/machine traces between1 and8MiB
may be retained losslessly as `.xz` when the compressed report fits the1MiB
individual and8MiB total report budgets. Retention receipts record both hashes
and sizes, and decompression is checked before raw scratch removal. This keeps
long input/body diagnostics without retaining raw banks or build products.

`tools/test_physics_multiplayer_vehicle_seats_tu15.py` uses the same
`--mcc-root`, optional `--map`, scratch `--output`, bounded `--report-output`
and explicit `--retain-video` contract. It records every requested seat case
before returning a failing suite result; one red case does not hide the rest.
Xbox executable and Vista UI arguments are not accepted by either live wrapper.

Object-field acceptance requires `HALO2_TU15_PHANTOM_TRACE`,
`HALO2_TU15_PHANTOM_OBJECT_TRACE` and the actual field definition in
`HALO2_TU15_LIFT_DEFINITION`. The wrapper rejects missing diagnostics before
launching, so an empty member trace cannot masquerade as a force regression.

The visible physics wrapper bounds each native case to180 seconds by default
(`--timeout-seconds` can declare a different finite budget). Expiry stops only
that owned native process and records a failed result with `timeout_errors` in
`execution.json`; the cohort can continue and retain its diagnostic recording.
It never advances a stalled physics world or supplies an admission result.

Every test registered by the main project runs under `tools/run_test.py`. The
wrapper supplies unique `TMPDIR`, `TMP`, `TEMP`, and `HALO2_TEST_SCRATCH` values.
It preserves the test's original working directory, arguments, stdout/stderr,
skip status, and CTest properties. Scratch is removed after passes, failures,
and skips. On POSIX systems a separate supervisor also cleans up if CTest times
out or the invoking process is killed; child processes in the test's process
group are stopped before storage is removed. Core dumps are disabled there.
Camera continuation, sound-prefix, and memory-budget test outputs use this
scratch directory too. Contained memory tests forward a bounded failure-log
tail and measured controls to CTest before their files are removed.

One-off scripts and private compiler probes must put generated outputs inside
the owned directory, including compile/link `.tmp` files and raw memory/frame
dumps. For example:

```bash
python3 tools/run_test.py --scratch-cwd -- python3 /absolute/path/to/probe.py
python3 tools/run_test.py -- python3 tools/probe.py --output '@HALO2_TEST_SCRATCH@/capture'
```

The second command illustrates the output-path convention; use the actual
probe's output option. The wrapper replaces `@HALO2_TEST_SCRATCH@` in arguments
and environment values. Python helpers can instead use `tempfile` or
`os.environ['HALO2_TEST_SCRATCH']`. Keep small receipts outside scratch using an
explicit report path. Files intentionally written elsewhere are not swept.

`--keep-artifacts` or `HALO2_KEEP_TEST_ARTIFACTS=1` retains scratch for an explicit
investigation. Print the retained path and size in the investigation's report.
Retain required original/reference captures with their provenance; rebuildable
test binaries and duplicate libraries are temporary. Existing assets, reference
banks, source snapshots, and installed runtimes are never deletion candidates
of this lifecycle.

Retire one-off test helpers when they are superseded and no remaining check
uses them. Keep the small result, commands and source hashes needed to explain
their findings, then remove their disposable builds and duplicate captures.
Do this as each investigation finishes; do not accumulate obsolete probes for
a later cleanup. Regression tests still used by the project remain source.

An active incremental weapon investigation may use the snapshot validator's
`--owned-build-worker --release-only` mode from an enclosing `tools/run_test.py`
coordinator. It invokes the existing `tools/run_tests.py --build-worker` inside
that same owner's `HALO2_TEST_SCRATCH/build`; the coordinator can then reuse it
through that official worker while the investigation is active. Record each
declared source refresh and its hashes, keep bounded reports per iteration,
and document the active path and size. Finish the enclosing owner as soon as
the build is no longer needed. Its supervisor removes both source and build on
completion, failure or cancellation. Never use this mode for an unowned build,
a launcher runtime or a separate persistent checkout; verify removal afterward.

When refreshing an existing snapshot, give changed files fresh modification
times or explicitly rebuild their outputs. Preserving an older source timestamp
can make Ninja reuse an object compiled before the refresh, even when the source
hash changed. Retain the refresh hashes and verify the affected units rebuilt.

The same owned coordinator may add `--prepare-only` to freeze/register a full
cohort before running focused targets through the official worker. That result
is `prepared`, with `passed: false` and `validation_not_run: true`; it is not
test acceptance. Keep the effective source hashes and each subsequent worker
command/result. The outer owner still removes the entire source/build context.

Mac dependency investigations can use `tools/macos/test_port.py
--owned-build-worker` from an enclosing `tools/run_test.py` owner. This keeps
the active build at that owner's `HALO2_TEST_SCRATCH/build` for subsequent
`tools/run_tests.py --build-worker` commands. `tools/macos/test_session.py`
provides a finite JSON-argv session inside that owner, records command results,
and expires after twenty idle minutes by default. Declare and authenticate
each source/reference refresh before rerunning. Send `{"quit": true}` as soon
as the investigation finishes, then verify removal of the owner, SDKs, reference
copies and binaries. Do not leave the session open between completed tasks.

Existing build directories need reconfiguration to pick up the CTest wrapper
and default-build exclusion. This change does not sweep historical directories.
On platforms without POSIX process groups, normal completion cleans scratch;
forced process termination cannot guarantee cleanup.
