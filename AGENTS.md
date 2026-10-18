# Test storage

Use `tools/run_tests.py` for CMake validation; it removes its private build tree
on success, failure, and cancellation. For one-off compiler, probe, emulator,
or capture scripts, read [the storage workflow](docs/TEST_STORAGE.md) and run
them through `tools/run_test.py`, with generated outputs under
`HALO2_TEST_SCRATCH`. New test helpers must use the same lifecycle or an owned
temporary-directory context.

Keep small reports, commands, source hashes, and failure diagnostics. Retain
large binaries, object/archive copies, raw memory banks, frames, or render
captures only when the user requests them or they are required reference inputs;
use explicit artifact retention and document the retained paths and size.
Preserve original assets, existing reference evidence, frozen source snapshots,
launcher-selected runtimes, and unrelated active work. Before finishing test
work, verify the generated scratch/build files were removed and report any
explicitly retained large artifacts.
