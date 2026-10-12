#pragma once
#include "halo2/native_synchronization.hpp"
#include <exception>
#include <stop_token>

namespace halo2 {
struct NativeThreadConfiguration {
  std::uint32_t first_handle_alias;
  std::uint64_t handle_alias_limit;
};
struct NativeThreadCreation {
  std::uint32_t handle{};
  std::error_code error;
};
enum class NativeThreadState { Running, Returned, Stopped, Failed };
struct NativeThreadOutcome {
  NativeThreadState state;
  std::exception_ptr failure;
};
// Host teardown exception, never a returned original wait status or completed
// source task. Throw only before acquiring a native permit/exclusion.
class NativeWorkerStopped final : public std::exception {
public:
  const char *what() const noexcept override;
};
using NativeThreadTask = std::function<void(std::stop_token)>;
struct NativeThreadServices {
  std::function<NativeThreadCreation(NativeThreadTask)> create;
  std::function<std::error_code(std::uint32_t)> request_stop;
  std::function<std::error_code(std::uint32_t)> join;
  std::function<NativeThreadOutcome(std::uint32_t)> outcome;
};
// Retains real std::jthread workers behind an explicit alias namespace.
// Services retain workers beyond facade lifetime. Dropping the final owner
// requests stop and joins; do so outside its workers, after quiescing callers.
// Tasks must cooperate with stop. Native default stack/TLS/scheduling apply;
// Xbox CreateThread/priority/NTSTATUS/CloseHandle are separate adaptations.
class NativeThreads {
public:
  explicit NativeThreads(const NativeThreadConfiguration &);
  NativeThreadServices services() const;
  std::size_t worker_count() const;
private:
  struct State;
  std::shared_ptr<State> state_;
};
// Actual finite native waits implement an interruptible host wait. Timeouts
// retry internally until the requested deadline; they never grant source
// access. An acquisition always returns ownership, even when stop is requested
// concurrently. Stop/failure/finite timeout throw before source post-wait stores.
void acquire_native_synchronization(const NativeSynchronizationServices &,
    std::uint32_t handle,std::uint32_t milliseconds,std::stop_token,
    std::uint32_t polling_milliseconds);
} // namespace halo2
