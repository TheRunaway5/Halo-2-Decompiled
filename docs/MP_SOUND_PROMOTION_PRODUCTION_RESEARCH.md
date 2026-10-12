# Production promotion owner and native synchronization

The production Cache target now includes the complete original promotion
selector and clock owner. `OriginalData` imports the three verified immutable
arithmetic words for owned update1.1 and1.5. The base image has no verified
mapping and supplies no constants. Mutable clock origins, counters, cooldowns,
options and source availability are not imported or assigned.

The prior complete original comparison is
`sound-promotion-native/final-manifest.json`, SHA
`eb053b6680b4917f9d03eec27b078c725932a9b4e4cb369e13ebbc2a7c11940e`.
[Its integration contract](MP_SOUND_PROMOTION_NATIVE_INTEGRATION.md) records
the full body, unchanged original execution, clock replay and required mutable
map ownership. Fresh production normal and GCC ASan/UBSan comparisons pass
74,318 calls,249,740 ordered writes and19,211,104 complete region bytes over
23maps and both images. The constant test independently compares24bytes read
from both actual executables; the new exporter requires the immutable paired
operand provider SHA
`3b96cff25fd62c0a462dbbc22b9c9c5e7409acc1211cb64c9bbfe78328ff2fe0`.

The native synchronization target supplies real retained recursive mutexes
and bounded counting semaphores. It exists for the exact creation/wait/release
contracts recovered from the original animation async loader. The original
kernel boundary remains an unavailable request, recorded in
`spawn-source/animation-async-construction/measured/report.json` SHA
`e1c8420ce6fc4abf6bcd9626cb0294a66847594a57fd68c72db21bc064509304`.
The complete wrapper/worker contract export is SHA
`32fdef27ca343207a873e59fd27c38e0203c2b84a0f623af82fa6363d2df7b49`.
This host adapter is explicitly at the named native/XAPI boundary; it does
not execute or invent Xbox NTSTATUS mapping, TLS, SEH, worker startup, file
completion or `NtClose` behavior.

The caller explicitly supplies a nonzero32-bit alias namespace. Each published
alias owns a real native object; no original numeric handle is prescribed.
Mutex acquisition uses the real recursive timed mutex. Release validates the
retained owner and recursion depth before unlocking. A bounded semaphore uses
a native mutex and condition variable around its retained count. Waits consume
one actual permit, and a failed release leaves the count unchanged and supplies
no previous-count output. Native errors remain native error codes. Missing alias
space is an explicit configuration boundary, not a made-up Source allocation
failure. Captured services retain backing after facade destruction. The final
owner requires quiescent callers and no acquired mutexes; thread shutdown and
original handle-close semantics remain separate.

Fresh normal and ASan/UBSan native platform tests prove actual recursive
ownership, contention, wrong-thread rejection,800 concurrent protected writes,
one genuine blocked waiter,151 permit consumptions, maximum150 enforcement,
retained facade lifetime, missing/kind validation and alias exhaustion.
These are native platform tests, not original kernel outcome comparisons.
No injected successful creation, wait, callback or I/O result is used.

The first standalone test compile rejected an incorrectly const future. That
test-only qualifier was corrected before successful standalone/production
checks; no platform or Source behavior changed. The actual production runs
are retained under `sound-promotion-production-20261009/validation/`.

No launcher or live gameplay path is changed here. Reload/grenade audio,
animation loading, canister collision/explosion and complete all-map audible
playback remain unfinished. Promotion needs one persistent mutable owned map
bank; native synchronization still needs the genuine Source worker and file
callback before a resource can become ready.
