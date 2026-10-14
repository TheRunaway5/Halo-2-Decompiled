#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <system_error>
#include <vector>

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
  enum class ObjectKind { Mutex,Semaphore };
  struct ObjectCheckpoint {
    std::uint32_t handle{};
    ObjectKind kind{};
    std::int32_t count{},limit{};
    bool operator==(const ObjectCheckpoint &) const = default;
  };
  struct Checkpoint {
    std::uint32_t version=1,first{};
    std::uint64_t next{},limit{};
    std::vector<ObjectCheckpoint> objects;
    bool operator==(const Checkpoint &) const = default;
  };
  class PreparedRestore {
  public:
    ~PreparedRestore();
    PreparedRestore(PreparedRestore &&) noexcept;
    PreparedRestore &operator=(PreparedRestore &&) noexcept;
    bool for_owner(const NativeSynchronization &) const noexcept;
  private:
    friend class NativeSynchronization;
    PreparedRestore();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Call only at a world barrier: pending waits/calls and any acquired mutex
  // reject export/import. Thread ownership and waiters cannot be transferred.
  Checkpoint checkpoint() const;
  // Stages real unlocked mutexes/semaphores and holds the local service gate
  // until publication/cancellation. All captured services keep their identity
  // and resolve imported handles afterward. Do not call services/object_count
  // from the staging thread while PreparedRestore owns that gate.
  PreparedRestore prepare_restore(const Checkpoint &) const;
  bool commit_restore(PreparedRestore &&) noexcept;
private:
  struct State;
  std::shared_ptr<State> state_;
};
} // namespace halo2
