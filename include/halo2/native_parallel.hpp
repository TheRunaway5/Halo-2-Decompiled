#pragma once
#include <cstddef>
#include <functional>

namespace halo2 {
// Deterministic fork-join for independent native work. Each task runs under
// the caller's SSE/x87 control words with clear sticky exception flags; after
// every task finishes, the caller's sticky flags gain each task's flags and
// errno takes the last change in task order, exactly as running the tasks
// inline would leave them. Tasks must not share mutable state. Exceptions are
// rethrown in task order. Nested calls, single tasks and
// HALO2_REFERENCE_SERIAL_NATIVE run the tasks inline in order.
void native_parallel_for(std::size_t count,const std::function<void(std::size_t)>& task);
// Worker threads available to native_parallel_for, excluding the caller.
unsigned native_parallel_workers()noexcept;
// Run job on a persistent dedicated thread while overlap runs on the calling
// thread, then join. The job inherits the caller's SSE/x87 control state and
// errno; its sticky FP flags are merged back and its errno change applied
// unless overlap changed errno itself (the later change in the serial order).
// The job's exception is rethrown first, else overlap's. With
// HALO2_REFERENCE_SERIAL_NATIVE both run in order on the calling thread.
// A nested call from the job or from overlap throws std::logic_error.
void native_overlap_run(const std::function<void()>& job,const std::function<void()>& overlap);
// Two jobs on two persistent overlap threads while overlap runs on the caller.
// Same rules as native_overlap_run; exceptions are rethrown in the order
// first, second, overlap. HALO2_REFERENCE_SERIAL_NATIVE runs all three in order.
void native_overlap_run2(const std::function<void()>& first,const std::function<void()>& second,
                         const std::function<void()>& overlap);
}
