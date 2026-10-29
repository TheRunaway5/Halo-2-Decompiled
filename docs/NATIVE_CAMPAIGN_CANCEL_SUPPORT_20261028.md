# Campaign cancellation prerequisites, TU1.5

This semantic module closes five actual dependency owners: `16EA60` next
occupied data-array index (30 original sites), `6CA40` salted iterator advance
(33), `C1E90` signed bit predicate (11), `1D0430` lookup in the two actual actor
timer records (24), and `275AB0` perception-group head publication (9).
The 107-site set contains no callbacks, CPU/register interpreter, generated
fallback, original binary execution or supplied service result at runtime.

Generic iteration and bit lookup live in
`native_tu15_data_array_iteration.hpp/.cpp`. The guarded
`cmake/NativeTu15DataArrayIteration.cmake` appends only the new source to the
existing single `halo2_native_tu15_data_array_delete` library. Previously
proved deletion/allocation source and CMake modules are unchanged. Timer and
group operations live in `native_campaign_ai_cancel_support_tu15.hpp/.cpp`;
`cmake/CampaignNativeAiCancelSupport.cmake` links the shared helper owner.

Next-index scanning rejects a negative start before any header read, captures
signed highwater once, and reads the actual bitmap word for every candidate.
Iteration reads cursor before pool, preserves the full returned index when
forming the salted datum, publishes cursor before reading the record salt,
then publishes datum and current row. Failure publishes the actual capacity
as cursor, datum NONE, and current row zero. Bit lookup retains signed
arithmetic word indexing and the original low-five-bit mask. No helper clamps
indices, fabricates a salt, treats a missing owner as empty, or admits a policy.

The raw retained iterator API preserves field/header/record aliases. The
typed `NativeTu15DataArrayIterator` overload uses the same structured algorithm
and lets actual private-local callers own four native fields. Twenty-two
disjoint profiles compare its fields at **every engine-memory read** against
the original field publications, plus final field values and unchanged engine
banks. The publication-before-salt-read alias is separately covered by the
raw iterator. Six group-head profiles also compare the direct typed result.
`native-iterator-caller-profile.json` authenticates the actual `275770` private
iterator producers and its real `275884 -> 6CA40` call. It does not claim
execution of the complete unclosed `275770` caller.

Independent offline reference execution uses unchanged physical i386 TU1.5
instructions with image SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The reference includes real `6CA40 -> 16EA60` calls, with no instruction patch,
register/state edit, supplied return or resumed unknown owner. GCC and Clang
ASan/UBSan pass 108 calls, 11,777 instructions, 1,164 ordered mutable reads,
75 stores, and all 107 authenticated sites. Complete retained banks,
before/after stores, read/store interleaving, actual return/stack/nonvolatile
contracts, original SIMD/occupied 80-bit x87 values and FP controls/status are
compared. Native integer calls preserve the host FP environment. Mapped data
profiles are covered; unmapped faults, dead scratch values and unmasked FP
traps are not claimed.

Seven semantic mutants are rejected: truncating negative bit-word division,
reading salt before cursor publication, masking the returned full index,
publishing the wrong failure cursor, changing scan-limit read admission,
using the wrong timer stride, and using the wrong group stride. Physical
cases include negative bit indices, bitmap word transitions, negative/boundary
starts, exhausted scans, salt boundaries, index `10000`, duplicate timers,
and publication aliases.

Reports are under `analysis/campaign-native-cancel-support-20261028/gcc-v1`,
`clang-sanitizers-v1`, and `module-integration-v1`. The actual CMake module
build verifies deletion, allocation and iteration remain one shared target.
It also tests the provided native executable, missing-reference skip77 with
`passed:false`/`validation_not_run:true`, and wrong-pin failure. Official
disposable wrappers remove original ELF, banks, mutation sources and builds;
the cleanup receipt preserves source hashes and small reports only.

The actual cancellation `1BD8F0`, composite `275770`/`275610`, virtual game
policy `160A60`, and speech/object dependencies remain open. These complete
prerequisites supply no substitute for those owners and enable no global
campaign gate.
