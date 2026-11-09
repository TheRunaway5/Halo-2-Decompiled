# 1262B0 animation resource wait

Primary and AI-frame now default to the typed native wait controller. The
retained reference bodies remain available through
`HALO2_REFERENCE_ANIMATION_RESOURCE_WAIT`. Original complete-body hashes are
`76d199ab47e38470231e32978c00913689a0cfcf79bf0d2c5ade99c4801febe0`
and `afa7b68951178086147b2bfd2449571955095c989516a7789651c4f5f4905f5a`;
the exact identity-resolved literal AI body is
`93fa87bc82b6e49240abb2ec7f10a4c0411612902c52d05396b486e513cdd1d9`.

The algorithm waits on the actual request's readiness byte, promotes a pending
job through genuine 1236A0, yields through genuine 2DBF74, and clears the live
tracking record only after readiness. There is no timeout or supplied
completion. The adapter preserves partial AL/CL results, every authored lazy
read, post-child DI, comparison flags and saved-SI/return-stack aliases.

The official CMake sanitizer and Release wait gates pass, alongside current
AnimationRead and ResourcePage reference-integration tests: 4/4, zero skips.
Each wait gate checks 24 inputs across the native controller and both unchanged
source-copy bodies (72 comparisons), plus 72 forced-reference comparisons.
Sixty unchanged-original i386 segments cover all 24 source sites and compare
GP/full flags/XMM/MXCSR/x87 controls/status/live 80-bit values, full banks and
ordered accesses. Six branch families cover absent jobs, existing readiness,
one/three yields, completion during promotion, and completion between readiness
reads. Two inputs alias the job word with saved SI. Sixteen body/dispatcher/
prefix mutations and seven admission refusals fail closed. Independent Clang
sanitizer CPU parity also passes; its receipt predates final assertion-only
fixture strengthening.

Completion comes from the existing SourceAnimationAsyncWorker and
SourceAsyncOwnedFiles reading an actual scratch-owned file, with computed job
IDs, NativeThreads and NativeSynchronization. Explicit scheduling controls
release the worker at declared yield/promotion/read boundaries and wait for
real retirement. The worker alone writes completion; no readiness/status/result
lists are supplied. The 24-input default-native corpus performs 16 promotions,
16 yields and 172 file callbacks, reading 30,792 bytes. These counters exclude
the repeated retained and forced-reference routes. The final fixture also
checks actual file payload/cumulative counts and released synchronization
ownership after stop/join.

Receipts are under
`build/test-results/native-animation-resource-wait-official-20261109/`.
The sanitizer receipt is 5,862 bytes, SHA
`b2002a94666bae36359c755b3f3957e71c2a1c75320c3645c970f3b1b834461a`;
Release is 5,795 bytes, SHA
`2ea0b4ba5bcfa199d4685b33dce6ca5f3a74d6af97208d9d74d786daea08f7ef`.
Exact literal install/idempotence/drift rejection has its separate
`native-animation-resource-wait-literal-binding-20261109` receipt. Model's
fresh Primary/AI-frame/Death raw/specialized generation and syntax reports
support regeneration; those executed commands did not explicitly assert the
wait prefix, and are not relabelled as that assertion.

This qualifies the CPU controller around the declared real host providers.
World's current animation provider remains synchronous. World pending-worker
integration, the genuine promotion mutex-owner handoffs at 2E0478/2E0342,
Xbox scheduling, real-asset acceptance, and page/write/observer-fault domains
remain separate. The native port preserves those actual owner boundaries and
failures; it does not establish complete runtime resource closure. All owned
build/reference/file storage was removed; no large artifacts were retained.
