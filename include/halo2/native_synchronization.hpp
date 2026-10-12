#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <system_error>

namespace halo2 {
struct NativeSynchronizationConfiguration {
  std::uint32_t first_handle_alias;
  std::uint64_t handle_alias_limit;
};
struct NativeSynchronizationCreation {
  std::uint32_t handle{};
  std::error_code error;
};
enum class NativeSynchronizationWaitState { Acquired, TimedOut, Failed };
struct NativeSynchronizationWait {
  NativeSynchronizationWaitState state;
  std::error_code error;
};
struct NativeSemaphoreRelease {
  // Present only after a real successful release. Failed releases preserve
  // the retained count and do not supply a previous-count output.
  std::optional<std::int32_t> previous_count;
  std::error_code error;
};
struct NativeSynchronizationServices {
  std::function<NativeSynchronizationCreation(bool)> create_mutex;
  std::function<NativeSynchronizationCreation(std::int32_t,std::int32_t)> create_semaphore;
  std::function<NativeSynchronizationWait(std::uint32_t,std::uint32_t)> wait;
  std::function<std::error_code(std::uint32_t)> release_mutex;
  std::function<NativeSemaphoreRelease(std::uint32_t,std::int32_t)> release_semaphore;
};
// Owns real native recursive mutexes and bounded counting semaphores. Explicit
// 32-bit aliases retain these objects; aliases are never native pointers or
// prescribed Xbox handles. Services retain backing beyond facade lifetime.
// Quiesce all callers and release acquired mutexes before dropping the final
// service owner. Source NtClose/thread/TLS/error mapping are separate owners.
class NativeSynchronization {
public:
  explicit NativeSynchronization(const NativeSynchronizationConfiguration &);
  NativeSynchronizationServices services() const;
  std::size_t object_count() const;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
