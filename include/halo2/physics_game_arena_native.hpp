#pragma once
#include "halo2/physics_game_arena_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include "halo2/native_synchronization.hpp"
#include <filesystem>
#include <memory>
#include <stop_token>
namespace halo2 {
struct SourcePhysicsGameArenaSchedule {
  // Optional declared host schedule, used for original differential execution.
  // Hooks do not grant a permit, complete source work or alter source state.
  std::function<void(std::uint32_t,std::stop_token)> before_worker_wait;
  std::function<void()> before_main_yield;
};
struct SourcePhysicsGameArenaConfiguration {
  std::filesystem::path savegame_directory;
  std::uint32_t worker_stack_address;
  SourcePhysicsFactoryFloatingPoint floating_point;
  SourcePhysicsGameArenaSchedule schedule;
};
// Fresh TU1.5 arena resources: native physical pages, mutexes/semaphore,
// immortal source worker and real savegame files. Source instructions own all
// queue links, job completion, CRC, offsets and arena/allocator construction.
// Native ABI/token/stack/priority/scheduling policies are explicit adaptation,
// not Xbox kernel equivalence. The calling bank and main stack are retained
// caller inputs. Stop/join this owner before releasing the bank or callbacks.
class SourcePhysicsGameArenaNative {
public:
  SourcePhysicsGameArenaNative(SourceRuntimeMemory &,
      SourcePhysicsGameArenaConfiguration,const SourcePhysicsWorldRuntimeServices & = {},
      const SourcePhysicsContactRecordObserver & = {});
  ~SourcePhysicsGameArenaNative();
  SourcePhysicsGameArenaNative(const SourcePhysicsGameArenaNative &)=delete;
  SourcePhysicsGameArenaNative &operator=(const SourcePhysicsGameArenaNative &)=delete;
  SourcePhysicsContactMemory &memory();
  void run(std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &);
  void stop();
  // Protect an existing retained physical allocation; source cache callbacks
  // own the requested address and extent after startup has joined its worker.
  void protect_pages(std::uint32_t address,std::uint32_t bytes,std::uint32_t protection);
  // Native converted-cache allocation policy supplies its explicit token
  // bounds. The same real page owner handles original metadata protection
  // calls and coupled authority checkpoint/restore for this backing.
  std::uint32_t allocate_native_pages(SourcePhysicalAllocationRequest);
  struct FileCheckpoint {
    std::uint32_t alias{},position{};
    std::string name;
    SourceRuntimeSnapshotBytes bytes;
    bool operator==(const FileCheckpoint &) const = default;
  };
  struct OwnershipCheckpoint {
    std::uint32_t version=1,worker{},worker_stack{},worker_semaphore{},next_file{};
    SourceRuntimeHostStorage::CursorCheckpoint scratch;
    SourceRuntimeHostPhysicalMemory::OwnershipCheckpoint physical;
    NativeSynchronization::Checkpoint synchronization;
    std::vector<FileCheckpoint> files;
    bool operator==(const OwnershipCheckpoint &) const = default;
  };
  class PreparedOwnership {
  public:
    ~PreparedOwnership();
    PreparedOwnership(PreparedOwnership &&) noexcept;
    PreparedOwnership &operator=(PreparedOwnership &&) noexcept;
    bool for_owner(const SourcePhysicsGameArenaNative &) const noexcept;
    bool matches_memory(const SourceRuntimeMemory &) const noexcept;
  private:
    friend class SourcePhysicsGameArenaNative;
    PreparedOwnership();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // The current live game retires its startup worker before gameplay. Export
  // requires actual join at the original idle wait, an empty source queue and
  // released mutexes. It cannot checkpoint an executing source job/context.
  OwnershipCheckpoint ownership_checkpoint() const;
  // The replacement must have completed and retired its own matching startup
  // worker. Keep that real local thread's terminal alias; never serialize a
  // native thread ID/entry closure. Stage private real files, pages and sync
  // objects without altering the current bank or the savegame-directory files.
  // The already restored replacement bank receives those actual page leases;
  // publish it by swap_backing before committing the resource owners.
  // Both owners must be quiescent. Commit/cancel on the same barrier thread.
  // The enclosing complete-world owner authenticates the immutable image/map
  // and budgets incoming byte/file data before constructing this subowner.
  PreparedOwnership prepare_ownership(const OwnershipCheckpoint &,
                                      const SourceRuntimeMemory::Checkpoint &,
                                      SourceRuntimeMemory &replacement) const;
  // After admitted whole-bank publication. Callback/schedule bindings remain
  // local, and run() remains stopped; only existing resource operations resume.
  bool commit_ownership(PreparedOwnership &&) noexcept;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace halo2
