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

Existing build directories need reconfiguration to pick up the CTest wrapper
and default-build exclusion. This change does not sweep historical directories.
On platforms without POSIX process groups, normal completion cleans scratch;
forced process termination cannot guarantee cleanup.
