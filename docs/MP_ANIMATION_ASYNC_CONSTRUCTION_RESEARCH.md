# Original animation async constructor and platform contract

The unchanged positive Unit/selector/resource-capacity prefix reaches complete `1221B0`, initializes all150×64 request records and stops before the first actual `NtCreateMutant` at encoded ordinal192. No handle, wait, release, worker, priority, TLS or file completion is supplied. This separate extension preserves the immutable capacity/residency freeze and its observation264 errata.

Current original evidence: `analysis/multiplayer/integration/spawn-source/animation-async-construction/measured/report.json`, SHA `e1c8420ce6fc4abf6bcd9626cb0294a66847594a57fd68c72db21bc064509304`. The first four resource stages correlate byte for byte after removing only new async observations. Its full bank and surrounding state are retained before/after. The new original phase records2calls and2713stores, including stack stores. Its actual stop is `2DC862→ordinal192`, return `2DC868`, with `(output_handle_slot021FCFFC, object_attributesNULL, initial_owner0)`. Independent repeat/final pins are recorded in the scope manifest once complete.

## Complete constructor storage and call order

`1221B0` clears9600 bytes at4E7938, sets every node+4 toFFFFFFFF, links node+3C in ascending64-byte stride, then terminates the final next pointer with zero. It stores free head4E9EB8=4E7938 and work head4E9EBC=0. It preserves request-ID counter4E7934 and requests-added counter4E9EC0. No completed handle store occurs in the current source run.

The remainder of the complete original body calls, in order:

| Wrapper | Literal startup request | Output store |
| --- | --- | --- |
| `2DC83A` | security0, initial_owner0, nameNULL | EAX→4E7928 free-list mutex |
| `2DC83A` | same | EAX→4E792C work-list mutex |
| `2DC7B9` | security0, initial_count0, maximum150, nameNULL | EAX→4E7930 work semaphore |
| `2D8606` | security0, stack0x4000 (16384 bytes), start122350, contextNULL, flags0, thread_id_outNULL | EAX→4E7924 worker handle |
| `2D83F6` | actual stored worker handle, relative_priority1 | Return ignored |

Before CreateThread, the constructor stores5098A0=0,5098A4=5018A0,5098A8=0x8000. These source stores do not establish a completed thread or TLS producer. Start122350 calls122360 then INT3; the worker body122360 loops indefinitely and normally does not return.

## Complete XAPI creation wrappers

These are32-bit stdcall stack arguments in call order. Source fields/outputs are full32-bit values unless a width is stated.

`2DC83A(security,initial_owner,name)` returns EAX handle or0 and RET12. Security is unused by this body. NonNULLname requires complete2DC6E2 attribute formatting; ordinary startup usesNULL and passesNULLattributes. Kernel192 receives `(address_of_local_output_handle, attributes, initial_owner)`. It must write the output only according to its actual creation outcome and return its actual status. Signed status<0 calls2D81D4 and returns0. Nonnegative status calls2D81A6 with183 only for40000000, otherwise0, then returns the actual output handle. No fallback handle or successful status is inserted by the wrapper.

`2DC7B9(security,initial_count,maximum_count,name)` returns EAX handle or0 and RET16. Security is unused. Name formatting and status/last-error rules match the mutex wrapper. Kernel193 receives `(address_of_local_output_handle, attributes, initial_count,maximum_count)`.

`2D8606(security,stack_bytes,start,context,flags,thread_id_out)` returns EAX and RET24. Security is unused. If stack_bytes is0 it reads the actual XBE header word10130; otherwise it retains the supplied stack. Kernel255 `PsCreateSystemThreadEx` receives exactly:

`(out_handle, extension_bytes0, stack_bytes, actual[566EF8]TLS_bytes, thread_id_out, start, context, (flags>>2)&FFFFFF01, debugger_thread0, system_routine2D856E)`.

The output handle slot aliases the wrapper's incoming stack-size slot. Signed kernel status>=0 returns that actual slot. On negative status, only actual FS24 byte<2 **and** actual `[FS28]+28` nonzero calls2D81D4 and returns0; otherwise it returns the actual slot, which may still contain its original stack-size value. This unusual error branch must be retained rather than universally mapping failure to0. No default value is established for runtime566EF8 by its image zero.

`2D83F6(handle,relative_priority)` returns BOOL in EAX and RET8. Kernel246 receives `(handle, actual PsThreadObjectType import259, address_of_object_output)`; output aliases the wrapper's incoming handle slot. Negative reference status calls2D81D4 and returns0. Otherwise priority+15 becomes+16 and-15 becomes-16; all other signed values pass unchanged. Kernel143 receives `(actual_referenced_thread_object, adjusted_priority)`; its return is ignored. Kernel250 receives that same actual object in **ECX**, then the wrapper returns1. The constructor ignores this BOOL. A host provider must retain reference/alias lifetime and actual failures, not use the thread handle as a fabricated object pointer.

`2D81A6(error)` writes through actual source `FS4 → TLS_index entry → +8`. `2D81D4(status)` obtains the actual kernel status-to-DOS mapping then calls that writer. Source TLS/error storage is required; a native host-thread-local error adapter is a separate named platform boundary. A supplied last-error integer is not an Xbox TLS producer.

The complete original `2D856E(start,context)` owns its SEH prologue, reads actual FS28+28 TLS backing, initializes its self-pointer, copies original TLS image418070..418074, clears actual418080 zero-fill bytes, invokes2D83BE notifications with1, invokes start(context), invokes notifications with0, then kernel-terminates the returning thread. Genuine host worker creation does not prove execution of this Xbox TLS/SEH body or its callbacks. Its actual producer and complete context must remain explicit. The contract export additionally retains its entire49-instruction contiguous extent, including the filter/handler omitted by ordinary control-flow reachability, and the exact12-byte SEH scope table for both verified executables. The SEH helper and notification/termination services remain required; these bytes do not establish their execution.

## Wait, release, queue and callback protocol

`2DC9C8(handle,milliseconds)` pushes alertable0 then calls complete `2DC8B2`; RET8. `2DC8B2(handle,milliseconds,alertable)` uses complete2DC70A for timeout storage and calls kernel234 with `(handle,WaitMode1/UserMode,alertable,timeout_pointer)`. FFFFFFFFmilliseconds producesNULLtimeout. Every other unsigned32-bit value is multiplied by10000 with the original unsigned64-bit product: the helper first stores low/high product, then stores its two-word negative value in that same output buffer. Nonnegative status returns unchanged except alertable nonzero/status101 retries. Negative status calls2D81D4 and returnsFFFFFFFF. The queue callers ignore the wait result; a native adapter still must actually own exclusive access before touching its records. Their ignored result does not authorize a successful wait substitute.

`2DC892(handle)` calls kernel221 `(handle,NULLprevious_count)`. Signed-negative status maps the actual error and returns0; otherwise returns1. RET4. `2DC814(semaphore,release_count,previous_count_out)` calls kernel222 with those same three arguments, maps negative error/returns0 or returns1, RET12. The previous-count output is actual writable caller storage. Queue publication and worker retry initialize their local previous-count word to0 before calling it; the actual release owns any replacement value.

`121FD0` waits on actual4E7928 withFFFFFFFF timeout, takes the actual free head, advances the head, clears the leased node's next pointer, releases the same mutex, and returns that node only when nonNULL. With no free node it calls2D84C4 and retries. `2D84C4` calls kernel238 `NtYieldExecution`; it returns1 unless actual status equals40000024, in which case0. The producer ignores that Boolean. There is no empty-node return in the whole original owner.

`1220E0` receives the leased node in EAX. It waits on4E792C, calls complete122020 insertion, stores node+4 first as signed arithmetic `(node-4E7938)>>6`, then overwrites it with that ordinal OR `(actual4E7934<<8)`. It increments4E7934 with32-bit wrap, preserves the full published ID, releases4E792C, releases one actual4E7930 semaphore count, and returns the ID. A real node lease, list lock, comparator/ordering owner and semaphore release are required. The old base typed async module has separately verified normalized list/queue helpers; it does not itself create these objects, translate arbitrary pointers, or admit this TU worker.

`122360` waits on the semaphore, waits on the work mutex, captures actual work head, releases the work mutex, stores current node518000 and invokes `node.callback(node+8)` **outside both list mutexes**. Callback0 releases one semaphore count and retries. Any nonzero result calls122150 to unlink/clear its ID, then waits on the free mutex and pushes the node onto the free list before release. Only callback1 with nonNULLnode+38 writes the byte done=1 before unlink. Other nonzero values remove/free without that write. It does not infer file completion from a cached resource, an allocated node or a successful semaphore acquisition.

The actual resource request builder1A4870 sets callback1A47D0. Its ten-word task carries file at+0, buffer+4, requested bytes+8, current file offset+C, optional cumulative-output pointer+10 and processed bytes+14. The callback computes the unsigned remaining count and takes its minimum with actual[475970]. It calls SetFilePointer even for zero-size requests; low resultFFFFFFFF suppresses ReadFile. Otherwise it calls ReadFile into buffer+processed, consumes its actual byte-output word and ignores its Boolean return. It advances processed/current offset and optionally writes the cumulative count. It returns0 only when processed differs from requested and transferred>=requested chunk; otherwise1. Thus result1/done is terminal callback completion, not proof of a successful full read: failed seek or short read can also terminate. Native IO must preserve those actual outcomes and the later resource consumer's own gates. A native worker needs this complete callback and genuine seek/read owners, not an arbitrary successful callback. These owners remain separate from host synchronization creation.

## Handle lifetime and selected-update evidence

32-bit handle aliases must refer to genuinely owned host objects and preserve kind, acquisition/reference and close lifetime. Numeric source handles are not prescribed. Thread close does not mean termination or join; original persistent worker shutdown requires separately declared host lifetime handling. The complete original close wrapper2D837A calls NtClose, maps negative error/returns0 or returns1; it does not clear caller globals. Complete original12080 closes worker, semaphore, work mutex and free mutex in that order at120D0..120F9; it does not clear those four handle slots, signal cancellation, terminate or join the persistent worker. This scope retains that complete body but does not execute a close or shutdown schedule.

Seventeen complete bodies have unique full raw-byte/internal-branch correspondence to authenticated u5, including all four creation/priority wrappers. The tiny122350 thunk and byte-identical2DC892 wrappers have multiple static matches; their selected later targets are established only through the actual constructor pointer and actual complete caller references in `contracts-complete/report.json`. Labels and address proximity are not authority. No u5 runtime or platform execution is asserted.

No native Unit/runtime/Simulation/body/resource-ready or Q behavior is changed by this research. Real synchronization, worker lifetime, TLS/error storage, complete file callback and read completion remain owning inputs.

## Native CPU owner and verified host composition

`construct_source_animation_async_queue` implements the complete1221B0 CPU store/call sequence over required bounded raw memory and selected-image address tokens. Its explicit `native_create_mutex`/`native_create_semaphore` callbacks adapt the called XAPI boundary and return actual owned native aliases or actual creation failure0. They do not execute or replace the original wrappers' NTSTATUS/TLS protocol. Missing worker or priority owners stop after the source's preceding stores; no successful callback exists by default. No final Unit/body/action offset is supplied.

Seven complete original initialization prefixes compare all raw bank/descriptor bytes and18921 ordered stores: the owning source case plus six explicit raw-storage/counter edge controls. Each stops before the first XAPI creation entry. The owning case's raw state and2703stores agree exactly with the genuine full source path that reaches kernel192. Both original counters and every untouched handle/padding byte are preserved.

A separate actual native composition uses the frozen NativeSynchronization provider to create two real mutexes and one real bounded semaphore, stores all three actual aliases through the CPU owner, then stops at the absent worker/TLS/callback service. Its six subsequent source stores and actual mutex/semaphore operations are checked. The native worker-storage token has genuinely owned host backing; its initial contents are declared host storage, not recovered Xbox initialization. No source handles, NTSTATUS, worker, read completion or loaded descriptor is supplied. This composition is a named native adaptation, not a completed original constructor.

The isolated comparison links only the new CPU owner and copied, hash-matched frozen native synchronization source/header; no Cache archive is linked. Normal and GCCASanUBSan tests use the complete literal ignored fixtures. The new source/test CMake fragments are guarded independently of concurrent whole-CMake changes. All earlier source/evidence/releases remain immutable.
