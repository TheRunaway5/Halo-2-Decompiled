# Native vehicle rate kernels

The scalar vehicle acceleration and target-approach owners now have structured
native implementations at canonical `208D80` / `208E70`, corresponding to
English TU1.5 `20D680` / `20D770`. They execute native SSE and strict x87
arithmetic with native branches. Production does not decode or execute an XBE.
The generated complete vehicle-controller graph dispatches these two owners to
the native kernels; the generator preserves that dispatch when regenerated.

The [`generation round trip`](../analysis/vehicles-tu15-goal-20261023/native-generation-v1.json)
reproduces the workspace C++ byte for byte from the pinned original
correspondence, including both native dispatches.

These kernels retain the actual controller ABI: argument/scratch and repeated
value stores, upper SSE lanes, defined flags, x87 stack placement, target
clamping and the real return stack. Scalar-only public helpers do not own that
ABI and could not simply replace the original callees.

The live owner normally provides an authenticated instruction cursor. Native
execution admits its exact callback type, executor and three service PCs only
when both full traces and store observation are inactive and the required
ordinary memory ranges are backed. Neither kernel contains a live service PC.
The kernel publishes the original write/fault and final return cursor. Other
observers, missing range capabilities, replaced callbacks and different
executors retain the original instruction path. Process-local diagnostic
counts demonstrate native execution; they do not enter gameplay or replay
state.

The real TU1.5 phantom/world composition wraps canonical source addresses.
The initial direct-cursor implementation therefore declined the actual Ghost
rate calls, as the read-only
[`dispatch observation`](../analysis/vehicles-tu15-goal-20261023/rate-dispatch-ghost-v2.json)
shows. The composition now uses `PhysicsInstructionRemapTu15`, a named table
adapter with the same original callback and lookup behavior. Native admission
checks the underlying live cursor and **all 94 instruction mappings**, including
unexecuted branches. Arbitrary wrappers still receive complete delivery. Native
stores and final PCs pass through the original mapping and callback; the
general instruction-cursor contract remains unchanged.

The mapped regression was
[`red on both original-image fixtures`](../analysis/vehicles-tu15-goal-20261023/native-rates-mapped-v1-red/validation/tests.log)
before the dispatch fix. The
[`corrected comparison`](../analysis/vehicles-tu15-goal-20261023/native-rates-mapped-v2/report.json)
passes the same 4,316 original cases, plus **2,136 mapped native calls** and
**14,400 mapped refusals**. Refusals cover active traces, callback/executor
changes, missing service/cursor contracts, absent unexecuted branch mappings,
changed mappings and an executed missing mapping. They compare complete
modeled state, memory, delivered traces and actual fault prefixes. This is
component evidence; the next integrated build must establish native admission
through the actual composition.

## Qualification

Run through the normal scratch owner:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python -B tools/validate_vehicle_native_rates_tu15.py \
  --output analysis/vehicles-tu15-goal-20261023/native-rates-v8
```

The retained
[`native-rates-v8/report.json`](../analysis/vehicles-tu15-goal-20261023/native-rates-v8/report.json)
and
[`LastTest.log`](../analysis/vehicles-tu15-goal-20261023/native-rates-v8/validation/evidence/Testing/Temporary/LastTest.log)
record the current source hashes and actual original comparisons. Untouched
original 1.1 and English TU1.5 execute offline on declared raw inputs. Native
output never supplies expected results or original call returns. Across both
images, 4,316 cases compare 94,130 ordered writes and 575,086 retained output
bytes. They include the enclosing vehicle-controller cases and scalar callers
under all declared x87 precision/rounding modes and nonempty stack placements.

The cursor comparisons additionally execute 2,136 admitted native rate calls,
exercise refusals for both trace flags, wrong executor/type, nonliteral service
descriptors, absent ordinary views and store observation, and compare 8,764
scalar write-rejection and 23,660 read-rejection prefixes with the validated
instruction path. Those failures compare complete modeled ABI and memory;
the admitted live-cursor tests separately compare normal completion and final
cursor. V8 also compares the propagated x87 control word and every MXCSR bit
between the native and instruction paths, including rejection boundaries.
Explicit
padded ordinary test backing is identical on both paths; its immutable `1.0`
operand is read from the original image. It supplies no gameplay result.

[`native-rates-v2`](../analysis/vehicles-tu15-goal-20261023/native-rates-v2/validation/tests.log)
records an initial cursor test refusal: the sparse original fixture omitted
unread memory, so it did not own the complete range required by the native
capability. The revised test explicitly declares its backing and checks both
admission and refusal, without relaxing the production guard.

The read-rejection extension was red in
[`v5`](../analysis/vehicles-tu15-goal-20261023/native-rates-v5/validation/tests.log)
and [`v6`](../analysis/vehicles-tu15-goal-20261023/native-rates-v6/validation/tests.log).
Combined C++ operands changed read order, and the target-approach temporary
became visible later than the original on a failed read. Explicit ordered
arithmetic and register publication fix both prefixes; v7 passes the same
cases. These fixes preserve ordinary successful results as well.

The
[`negative control`](../analysis/vehicles-tu15-goal-20261023/native-rates-negative-v1/report.json)
changed addition to subtraction only inside a private kernel copy. Both image
comparisons rejected the resulting memory mismatch. Original instructions and
the production source remained unchanged by that control.

The integrated [Ghost boost run](../analysis/vehicles-tu15-goal-20261023/native-ghost-boost-v2.json)
reaches four native acceleration calls through four authenticated live-cursor
admissions. Ordinary right-click input raises the original boost state from
zero to one; release returns it to zero. This confirms actual gameplay routing
for that case. It does not qualify every controller, vehicle or boost behavior.

All completed component owners have been removed, including raw original
banks, source copies and compiler output. Only bounded hashes/reports/logs
remain. The main vehicle source/build owner is still active separately.

Qualification covers two complete native scalar owners. The full native game rebuild,
actual driving/firing/destruction validation and launcher promotion remain in
[`VEHICLES_TU15_GOAL_CHECKLIST.md`](VEHICLES_TU15_GOAL_CHECKLIST.md).
