# Native model-region state, TU1.5

`native_tu15_model_region_state` owns the complete `1704C0` control flow. Its
legacy boundary preserves the original caller formals, stack DATA, partial AL
return, registers, flags and live FP/SIMD state. The TU1.5 death-cycle route
selects the kernel and retains the guarded original literal body.

The implementation preserves signed state comparisons and count reads, the
low-byte force test, property shifts masked to five bits, the signed16 region
mask, and filter/NONE short circuits. It rereads the model's count after every
region. Candidate records and effect packets stay in their actual source stack
locations. Fields read after a callee remain live; neither packets nor selected
records are replaced with cached host objects or prepared outcomes.

Five remaining callees retain their genuine execution: candidate collection
`170930`, weighted choice `170B40`, effect stop `17A0F0`, effect preparation
`178D80`, and effect creation `178E30`. This change does not claim those owners
have all become semantic native kernels.

## Independent original execution

Authority is the English Xbox TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The required Warthog and power-module native BEFORE archives from
[BC720 qualification](NATIVE_REGION_TRANSITION_TU15.md) are reused. They are
actual current MCC/native inputs, not original Xbox hardware captures.

The offline harness executes the original `BC720` prefix to obtain the real
`1704C0` call frame. It independently compares the native parent and model-state
kernel on a separate mutable bank. Every remaining callee executes unchanged
original i386 instructions on that side's current bank and real ABI. Captured
native AFTER bytes, candidate answers, RNG draws, effect results and allocation
handles are never supplied. Instruction bytes are not patched.

The retained positive set has 45 complete comparisons, 14,153 physical model
instructions, all 154 model instruction sites, all five callee boundaries, and
4,051,739,015 compared bytes. It checks GP state and defined flags at every
model instruction, ordered operands/stores, call inputs/returns, FP controls,
logical x87/TOP/tags, SIMD, the complete parent return and all captured banks.
Linux/debugger IF/RF bits and unknown dead x87 words are excluded explicitly.

| Inputs | Evidence |
| --- | --- |
| Actual Warthog and power-module frames | Full original/native returns; module separately checked with Clang |
| Twenty Warthog cases with GCC and Clang | Count zero/negative; NONE/default and decreasing state; force low byte; filters; property set/clear and shifts 31/32/negative; absent variant/mapping; all regions; authored effect transition |
| Authored effect with two live x87 values | Real stop, preparation, creation, returned source handle and exact incoming/outgoing80-bit values |
| 2, 5 and 32 candidates with two live x87 values | Copies of an actual authored permutation in an explicit temporary input arena; real collector and weighted selector; real RNG-state advancement |

The 32-candidate case chooses index 19 and advances the original RNG from
3,669,832,197 to 2,624,945,056. This exercises record-address multiplication
beyond index zero, the source candidate capacity, weighted selection and the
actual x87 calculation. The input arena is compared too and is removed with
the test owner.

Reports: [GCC](../build/test-results/vehicle-destruction-model-region-state-20261101/gcc-model-final/report.json),
[Clang](../build/test-results/vehicle-destruction-model-region-state-20261101/clang-model-final/report.json),
[actual module](../build/test-results/vehicle-destruction-model-region-state-20261101/actual-power-v1/report.json),
[effect/live x87](../build/test-results/vehicle-destruction-model-region-state-20261101/authored-effect-live-x87/report.json),
[32 candidates](../build/test-results/vehicle-destruction-model-region-state-20261101/weighted-32-live-x87/report.json),
[five candidates](../build/test-results/vehicle-destruction-model-region-state-20261101/clang-weighted-5-live-x87/report.json),
[two candidates](../build/test-results/vehicle-destruction-model-region-state-20261101/clang-weighted-2-live-x87/report.json).

## Authored effects and negative controls

The [input inventory](../build/test-results/vehicle-destruction-model-region-state-20261101/authored-effect-inputs.json)
reads 155 model tags and 756 authored states from the verified BEFORE. All 221
non-NONE state-effect references have matching resident effect-tag bindings.
This is an input inventory, not sound playback acceptance.

The effect case requests Warthog variant0, region1, state1 with force enabled.
Its live authored state identifies effect `3853600420` and parameter
`100670144`; the harness authenticates these fields and the full effect-tag
binding before executing the original code. No effect data is invented. The
original creation owner allocates handle `0xE681000C`, and native execution
matches every resulting bank byte. The request/formals are explicit component
inputs; this particular case is not a claim about a naturally occurring shot.

Six [negative controls](../build/test-results/vehicle-destruction-model-region-state-20261101/negative-controls.json)
are rejected: reversed decreasing-state force logic, an incorrect property-bit
wrap, wrong selected-record stride, wrong effect-packet constant, wrong effect
handle destination, and an omitted late count read. Rejections occur in the
instruction ABI or ordered access/store comparisons after independently
computing the original expected execution.

## Regeneration and current gameplay build

[Regeneration](../build/test-results/vehicle-destruction-model-region-state-20261101/regeneration.json)
reproduces the complete normalized function and native route. Only the exact
fixed-image folding of `selected(identity,TU15)` to the same immediate is
allowed. Both complete literal hashes are pinned; other source changes are
refused. The installer is idempotent.

The previous finite source/build owner expired and removed its tree; its
[closure receipt](../build/test-results/vehicle-destruction-model-region-state-20261101/previous-owner-closed.json)
records authoritative removal. A fresh frozen source has passed the
[26-header preflight](../build/test-results/vehicle-destruction-model-live-20261101/native-model-preflight.json).
Its [eight-case CMake/CTest validation](../build/test-results/vehicle-destruction-model-live-20261101/eight-native-v2/refresh.json)
passes all seven vehicles and the power module. The
[joined qualification](../build/test-results/vehicle-destruction-model-region-state-20261101/qualification.json)
records 138 actual target-native calls, 114 calls with changed region bytes,
106 same-step authored-response matches and 42 effect-handle field changes.
All 144 complete-world restore/replay comparisons pass. The module's native
pending due, eligibility and original blast order remains 8, 9, 10.

The fixture now records actual full-generation target calls only from the
selected native `1704C0` route. It checks the model tag and object definition,
the count and live active/state pointers, and the real `RET40` boundary. It
retains before/after region bytes and AL. Pending restore/replay calls are
excluded, and this observer is disabled before final continuation comparisons.
The journal cannot supply a model, damage, effect or allocation result.

An effect-handle field change alone does not establish allocation, stopping or
audible delivery; the original-code effect case and downstream audio acceptance
are separate evidence. The [broader thirty-case run](../build/test-results/vehicle-destruction-model-live-20261101/expanded-current/refresh.json)
completed with20 passes. Five additional cases passed after the fixture's
read-only store collectors were registered through their actual read-only API;
native contact/flip admission and every destruction assertion remain intact.
The [focused rerun](../build/test-results/vehicle-destruction-model-live-20261101/readonly-observers/refresh.json)
and [joined result](../build/test-results/vehicle-destruction-model-region-state-20261101/expanded-current-summary.json)
retain25/30 passing cases and416 complete-world comparisons. Five campaign
cases refused cold animation-resource I/O during immutable body-pose
publication. Those failures are retained; the subsequent fix and its narrower
remaining physics refusal are recorded below.

## Cold campaign body-pose resource reads

The original pinned`BF640` call graph reaches`1112E0 ->1E5DF0 ->1E5A70
->1262F0 ->1260C0 ->126139/1A3B70`. The
[original-code inspection](../build/test-results/vehicle-destruction-campaign-resources-20261101/original-body-resource-call-paths.json)
traces93 bodies with no trace failures. Both the initial campaign baseline and
the new cold-pose regression fail at that exact live read, before weapon
equipment or firing. The refusal comes from the local native publication guard.

The native World now permits only that genuine non-tail animation-read call
during immutable pose completion. Existing descriptor, owner, extent and cache
sector validation remains in place. The actual synchronous read completes and
returns its real native job. Source cache/stack bytes are undone in reverse
store order; a surrounding scope then removes the temporary completed jobs and
restores their dense identity counter. New memory backing, synchronization and
unrelated I/O remain refused. The original pose instructions and authored data
are unchanged; Xbox asynchronous queue timing is not established by this policy.

The [cold-pose qualification](../build/test-results/vehicle-destruction-campaign-resources-20261101/cold-pose-qualification.json)
passes18 complete native-bank/resource-owner comparisons across nine actual
campaign/vehicle inputs. Eight destruction cases pass128 complete-world replay
comparisons. The [ordinary render control on Lockout](../build/test-results/vehicle-destruction-campaign-resources-20261101/body-render-lockout-v8/report.json)
also passes: both player representations match the original`BF640` node output
with zero component/position error, retained-byte changes or topology changes.
Its earlier Coagulation run did not reach the required dual-held input and is
retained as a setup diagnostic. The Covenant turret now clears this read boundary and reaches a
later native convex-factory query refusal. Its genuine typed BEFORE identifies
`AD390`/class8 for the second shape, with native SoA center/support owners.
The required22,328,168-byte
[BEFORE archive](../analysis/physics/native-convex-factory-destruction-20261101-required-before/campaign_c_turret_ap/retention.json)
retains113,018,055 input bytes; every member round-trips. There is no captured
original GP frame, AFTER bank, supplied class result or constructor outcome.
The subsequent physical comparison in
`build/test-results/vehicle-destruction-campaign-resources-20261101/convex-bound-class8-control-qualified-v15/report.json`
passes the actual class23/class8 constructor composition with GCC and Clang,
three independently poisoned native scratch patterns and three rejected
constructor mutations. It compares complete captured banks, return/FP and
ordered persistent data accesses; the original immutable switch-table control
reads are authenticated separately. The binding admits only the additional
observed`AD390` owner. Live destruction acceptance remains separate.

## Actual sound caller evidence

A subsequent [eight-case run](../build/test-results/vehicle-destruction-model-live-20261101/actual-sound-requests/refresh.json)
passes with read-only original sound-request observation. It captures165 full
160-byte caller requests at the genuine completed`18CEFC ->223B20` boundary,
including source tick and resident sound-tag identity. Explicit verification
replays are excluded and observation ends before final continuation checks.
The callbacks supply no request, admission, source voice or PCM result.

The [joined journal](../build/test-results/vehicle-destruction-model-region-state-20261101/actual-sound-request-summary.json)
retains exact evidence hashes and request-stream digests. Seven sound-bank
names match the original37-definition destruction dependency inventory,
including Warthog hood/hull, turret, Ghost and Covenant damage cues. Names alone
do not establish causal target attribution. The report keeps all24 other
original dependency banks explicitly unobserved, including wreck impacts and
combined loops. The [PCM/manual-output replay](../build/test-results/vehicle-destruction-model-live-20261101/actual-sound-output-v2/qualification.json)
passes all165 exact callers with1,2 and4 coincident listeners:270 classic PCM
voices are non-silent, emit once and complete;225 authored remastered-only
routes create no voice. Coincident listener counts preserve each selected
pitch range/permutation and complete PCM hash. Original owner and world-position
fields reach the output packet unchanged. This is component replay with declared
listener points and auxiliary RNG; original timing/permutation, normal runtime,
physical device and Xbox waveform equivalence remain open.

## Remaining scope and storage

No new large reference is required: the same 25,074,613-byte pair of verified
BEFORE archives is reused. Component source copies, original-code ELF files,
expanded banks and declared input arenas are disposable test-owner data.
Existing references, assets, frozen evidence and selected launcher runtimes
are preserved.

The finite current-gameplay/PCM owner is now closed. Its
[cleanup receipt](../build/test-results/vehicle-destruction-model-region-state-20261101/owner-closure.json)
verifies removal of the complete2,689,085,440-byte allocated source/build tree
and an empty scratch parent. No new large artifact is retained; the bounded
reports, source hashes, caller plan and failure diagnostics remain available.

Proper source effect transitions now have direct original-code evidence. The
runtime fixture is muted; audible delivery, original waveform/device output,
both combined loops, exact wet-feedback format/time, full-definition visuals
and remaining roles/modes/network gates remain open. Lack of Xbox hardware
does not authorize guessed feedback behavior. All 37 full-definition gates,
D01-D13 and the complete destruction/audio goal remain active.

The [later class8/request/PCM cohort](../build/test-results/vehicle-destruction-campaign-resources-20261101/class8-and-complete-sound-qualification.json)
passes all30 registered cases on a frozen engine after the bounded native
class8 factory admission:60 cold-pose checks and480 complete-world replay
comparisons. Its sound observer also covers terrain glass and all four
reciprocal blast chains, excluding verification replays. All561 exact requests
pass component PCM accounting for1/2/4 coincident listeners:304 classic voices
and257 authored silent routes per configuration. Original admission,
permutation/waveforms, combined loops and normal live delivery remain open.
