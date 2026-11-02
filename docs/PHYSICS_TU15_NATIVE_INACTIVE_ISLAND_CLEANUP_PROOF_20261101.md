# TU1.5 native inactive-island cleanup

`native_tu15_cleanup_inactive_island` implements the complete75-site282AB0
parent using typed memory, callbacks and FP context. It contains no instruction
decoder or guest register frame. It preserves short-circuit body tests, the
manager identity cached before agent invalidation, fresh pair backing after
callbacks, and fresh body backing after the kind query.

[Original qualification](../build/test-results/native-inactive-island-cleanup-tu15-20261101-qualified/report.json)
passes176 physical cases under GCC and Clang:121 complete returns, three real
31B730 entry stops and52 fault prefixes. It compares4,223 ordered reads,
163 stores, persistent DATA and FP state; original RET4 is checked. Three
semantic mutations are rejected. Genuine31D400 invalidators and AD740/31D520
kind getters execute in the oracle. No child outcome or original postimage is
supplied.

The [world binding](../analysis/physics/native-inactive-island-cleanup-world-binding-20261101/source-pins.json)
selects this kernel for the actual island-deactivation notification. Existing
native31B730 owns default-policy replacement. Its model publishes declared
callback DATA before notification, then exercises one agent invalidation, one
manager invalidation, one pair clear and one kind2-to-default replacement.
Neither282AB0,31B730 nor31D400 executes through the old owner in that model.

v1466/06 now exercises11 native cleanup calls,12 pair clears,12 native policy
selections and8 native1D9C80 destructions with zero cleanup refusal or policy
residual. Strict original frame450 agrees over114,363,573 persistent bytes and
FP. This is selected live coverage; custom invalidators remain genuine.

Custom invalidation, destructor and base-lock children remain separately named
residual services. Callback-mutated cohort qualification, broader positive authored inputs and full native-only closure remain
open. A component or binding pass does not close these gates.

Both component owners and the model build are removed; only small reports,
commands and source pins remain. No large artifact was retained for this proof.
