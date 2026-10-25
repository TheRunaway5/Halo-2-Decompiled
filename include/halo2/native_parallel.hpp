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
}
