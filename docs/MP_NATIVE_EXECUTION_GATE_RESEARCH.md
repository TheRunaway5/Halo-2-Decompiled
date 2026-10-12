# Native source CPU execution gate and concurrent cache worker

`NativeExecutionGate` supplies an explicit native serialization policy for the cooperating source CPU owners. The complete cache request and async worker now compose with **concurrent, gate-protected completion polling**, real retained threads/synchronization and actual owned shared-map reads. This is host synchronization, not a claim about original Xbox atomic instructions, scheduler interleavings or TLS.

Evidence is separate under `analysis/multiplayer/integration/spawn-source/native-execution-gate`. The [quiescent actual-thread checkpoint](MP_ANIMATION_ASYNC_NATIVE_WORKER_RESEARCH.md), complete [worker/file CPU](MP_ANIMATION_ASYNC_WORKER_RESEARCH.md), constructor and [cache request](MP_SOUND_CACHE_REQUEST_NATIVE_INTEGRATION.md) freezes remain immutable. Their original instruction/output comparisons remain the source authority. This extension changes no original CPU implementation or evidence.

## Gate contract

Copies of `NativeExecutionGate` retain the same real host mutex. `execute(void entry)` establishes one thread-bound, nonrecursive lease for a complete CPU entry. A thread already inside any entry cannot enter the same or another gate, including while its entry is suspended. `require_owned()` rejects memory/IO access outside the matching, acquired, non-revoked lease. `require_outside_entry()` rejects teardown/join inside any entry, including a suspended one.

`suspend(void provider)` drops the gate **before** the actual native wait/yield. Only a normal provider return permits reacquiring the gate. The bound wait provider itself must return only after genuine native acquisition; internal finite polling timeouts never return to Source. If a provider throws because of failure or cancellation, the entry stays unlocked and revoked. `require_owned`, another suspension and any nested entry all reject attempts to resume Source through caught-exception user code. An entry that swallows the provider exception and returns is also rejected. Unwinding clears its thread-local entry marker without claiming a wait result.

All CPU reads/writes and actual file-byte writes in the composition occur within this lease. The private memory callbacks check it on every invocation. Source cache spans are immediate local reads/writes; none escape the CPU call or are exported to another owner. The worker's file destination remains local to its callback while the gate covers the actual read and following source task/count stores. Join and final owner teardown occur outside entries. This policy requires every cooperating consumer to use the same gate; it does not make an exposed ordinary byte span inherently thread-safe.

The complete queue owners support this lock order. Submit releases the free-list mutex before work publication. The worker waits on the semaphore with no list mutex, captures/releases the work mutex before its callback, then unlinks under work exclusion and recycles under free exclusion separately. Each actual wait suspends the gate before acquiring that native object and reacquires afterward. Native mutex release stays inside the acquired CPU lease. Empty-free-list yield must also suspend. No nested list mutex or reentrant Source entry is introduced.

## Actual concurrent composition

The test consumes the three immutable source-produced `127750→21A960` states (`ready`, `reload`, `empty_reload`) from the frozen cache suite. It uses the literal pre-constructor inputs and genuinely owned storage; it does not install final queue or cache completion bytes. The complete CPU constructor creates the 150-node bank, two actual mutexes and semaphore. Its real native worker creation stores actual alias `F5100001`, then the constructor stops explicitly at unsupported priority1. Source TLS/SEH/stack/NTSTATUS/priority translation is still unavailable. The host thread runs the complete frozen immortal worker inside a gate entry.

The main thread calls the complete cache request with flags2 under an entry. Source allocates its actual page/cache entry and submits the complete async request through `19D8C0/1A4870`; return1 and all publication stores/regions match the original capture. In that same entry, flags0 observes pending1. Flags1 then enters the original blocking done-byte loop. Its real yield suspends the gate, admitting the worker. No done/result is supplied to break the loop.

The worker's real semaphore/mutex acquisitions and actual `SourceAsyncOwnedFiles` read drive source task advancement, terminal result1, done-byte write, unlink and free-list retirement. Flags1 rereads the done byte while holding the common gate and returns2 **before** native stop/join. Source flags4/5 subsequently return6/6. Only afterward is stop requested; the actual no-permit semaphore wait throws before source post-wait stores. The final full-region comparison proves cancellation added no stores or byte changes.

Each of the three cases checks:

- All 2,709 original constructor-prefix stores plus the separately identified actual native worker-slot store.
- All 59 original publication stores and eight original worker completion stores.
- One extra pending flags0 touch store, equal to the original page-touch instruction/value; flags1's two completion stores equal the original completed flags0 stores. Flags1 is an explicit native caller control of the complete verified body, not a recovered original caller/scheduler producer.
- The original two-store flags4 and flags5 results.
- Six complete comparisons of the original 3,654,420-byte region set, totaling 21,926,520 checked bytes per case. The sole expected original-buffer substitution is the genuine native worker alias at `004E7924`.

That is 2,784 ordered stores per case, 8,352 across the three cases, and three actual shared-map reads. The 53,760-byte regular-file requests and file bytes are the original captured requests/outcomes. Their terminal result1 is not generalized into full-transfer/resource-loaded success; the prior worker suite retains genuine short/failed-seek terminal cases unchanged. Extra polling iterations and host scheduling opportunities are variable native behavior; no fixed original interleaving or wall-clock timing is asserted.

Focused gate tests also reject missing/wrong/recursive ownership, reject post-failure reacquisition, admit a real competing entry during a blocked suspended provider, retain gate ownership across copies, and serialize 2,000 writes across four real threads. The positive cache test rejects outside-entry raw memory access and checks complete data both before and after cancellation.

## Boundary and use

Native code can retain one gate with its private raw backing, supply memory callbacks that require its lease, and execute each complete cache/worker entry through `execute`. Bind native waits and yields through `suspend`; keep actual file-byte writes inside the acquired callback; stop/join only after unwinding all entries. The complete frozen CPU callbacks still own queue/cache transitions and result decisions. Missing file, synchronization, metadata or other owners must still fail explicitly.

No live runtime consumer or Unit/Q bridge is installed here. Original file path/open/token production, complete resource residency/current-transform owners, Source platform priority/TLS/SEH/stack/status and live audio admission remain separate. Source data residency or this synchronization policy cannot substitute a resource-active byte. New runtime u5 execution is not claimed; prior complete u5 body mappings are protected. No renderer/shader/release/GPU work occurs.

Normal, GCC ASan/UBSan and GCC ThreadSanitizer checks pass. They compile the exact new source plus previously frozen provider source, with no Cache/Systems archive. Independent run/repeat outputs, compiler/dependency pins, commands and the read-only audit are retained in this scope. ThreadSanitizer coverage is these cooperating owners and fixtures, not arbitrary future raw-span consumers. The first isolated CMake configuration used a reserved variable name and stopped before compilation; its diagnostic is preserved under `drafts`.
