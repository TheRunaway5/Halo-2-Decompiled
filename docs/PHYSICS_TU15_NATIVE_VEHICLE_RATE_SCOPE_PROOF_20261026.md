# Native TU1.5 vehicle rate scope qualification, 2026-10-26

The native scalar rate kernels for actual TU1.5 `20D680` and `20D770` now qualify against both the exact frozen live-state headers and the current state headers. The live consumer scope admits native math without importing new machine-state capabilities into the frozen runtime. All calculations remain typed native x87/SSE arithmetic; original Xbox instructions execute only in the offline reference test.

The final source pins, proof links and measured storage receipts are in [the handoff report](../analysis/physics/native-vehicle-rate-final-handoff-tu15-20261026/report.json). This component result does not establish live activation, full vehicle/contact parity, or the multiplayer camera gates.

## Physical authority

The unchanged English TU1.5 XBE has SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The test runs its two actual scalar bodies on physical i386 hardware under ptrace, with no paired instruction patch and no unported-owner capture stub. Explicit input cases cover distinct quiet/signaling NaNs, operand order, precision and rounding controls, FTZ/DAZ, denormals and aliased input storage.

Both GCC and Clang passed [1,382 original calls](../analysis/physics/native-vehicle-rate-physical-tu15-20261026-final/report.json): 46,053 unchanged original instructions and 6,680 exact ordered stores, complete retained banks, GP/XMM values, defined integer flags, and complete CW/SW/MXCSR. The observer collected 6,252 actual CPU snapshots before and after the four FMUL and two FSTP sites. Each of 2,084 FSTP publications also passed two independent native failure projections: a destination write rejection equals the physical pre-instruction state; an observer rejection after the write equals the physical post-instruction state. Six publications exercise C1 rounding upward.

The initial physical test exposed two real differences hidden by the older Unicorn component comparison: x87 FSTP C1 and the NaN selected by FMUL m32. The native math now uses the original operand form and captures defined C1 immediately after each multiplication. Publication commits its real destination write, logical pop and C1 before delivering the observer. A rejected destination restores the pre-instruction numeric exception flags and leaves logical depth/C1 unchanged. The same two FSTP/four FMUL primitives were corrected narrowly in the optional literal trace route and its generator.

A [private negative mutation](../analysis/physics/native-vehicle-rate-physical-negative-tu15-20261026/report.json) moves pop/C1 after observer delivery. Both compilers compile successfully and the physical comparison rejects this mutation at the completed-store prefix. No production source is mutated by the negative test.

## Consumer scope

The [frozen-context proof](../analysis/physics/native-vehicle-rate-scope-tu15-20261026-final/report.json) and [current-context proof](../analysis/physics/native-vehicle-rate-scope-modern-tu15-20261026-final/report.json) each pass 2,230 original cases and 47,505 original stores on GCC and Clang. They retain full ABI/banks, ordered stores, operand-read failures and completed-store observer failures. Scope lookup is local to the constructing executor, selects the nearest matching memory owner for nested scopes, and restores the previous owner after destruction.

An active live scope requires TU1.5, ordinary retained caller stack bounds, a phase publisher, disabled full instruction traces, and read-only store observation. Unsupported active contexts refuse explicitly and increment their per-world refusal count. The scope does not silently enter the literal fallback. The canonical store event is kept canonical; only the actual phase publisher adds the authenticated `0x4900` TU1.5 instruction displacement, because the existing vehicle composition already maps store events.

The frozen qualification copies the existing frozen headers unchanged. Its only vehicle CPP changes are the native include, two entry seams, the diagnostic activity accessor, and the six source FP primitive corrections. Current headers are tested separately.

## Storage

Every fixture, hardware executable, compiler output and private source tree is owned by `tools/run_test.py`; CMake builds run through `tools/run_tests.py`. All four final owners were verified absent. Their measured remaining scratch allocations before removal total **146,665,472 bytes**; compiler build trees had already been removed by the inner runner. No large artifact is retained. Small source hashes, commands, bounded failure diagnostics and reports remain.
