# Retained native worker threads

`NativeThreads` owns real C++ native threads behind caller-selected, nonzero
32-bit aliases. Captured services retain the workers after facade destruction.
The native creation result is an actual thread or an actual `std::system_error`;
missing tasks and exhausted alias namespaces throw rather than manufacturing
an original creation failure. Final owner destruction requests cooperative
stop and joins every worker. Final owner destruction must occur outside its
workers, after callers have quiesced. Tasks must not retain their own final
thread owner or ignore stop indefinitely.

`acquire_native_synchronization` uses actual finite waits on the retained native
synchronization provider. Timeouts remain internal until the caller's deadline.
Stop and failure throw before any original post-wait stores. If a wait acquires
ownership concurrently with a stop request, it returns the actual acquisition;
it does not discard a consumed permit or skip that iteration's original writes.
The next acquisition boundary observes stop. `NativeWorkerStopped` identifies
host teardown, separately from an ordinary returned task or retained exception.
It never represents successful source task completion or an original wait status.

Fresh standalone and production normal/ASan/UBSan checks cover six real workers,
two consumed permits, blocked semaphore/mutex shutdown, native invalid-handle
failure, finite timeout, retention beyond facade lifetime, final-owner join and
the acquisition/stop race. The existing real synchronization checks also pass.
No forced native resource-exhaustion failure or thread-sanitizer claim is made.

This supplies a native worker owner for the original async CPU loop. It does
not implement Xbox thread creation/TLS, native priority translation, original
NTSTATUS/last-error fields, CloseHandle semantics, file-path/token producers,
source task execution or readiness. Native default stack, TLS and scheduling
apply. The async CPU/file adapter and its actual bindings remain separate.

Logs and immutable source/binary snapshots are retained under
`analysis/multiplayer/integration/native-worker-threads-20261009/frozen`.
No launcher or live gameplay result is published by this checkpoint.
