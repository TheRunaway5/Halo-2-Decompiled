#pragma once
#include "halo2/physics_pool_startup_runtime.hpp"
#include "halo2/source_runtime_host_storage.hpp"
#include <memory>

namespace halo2 {
struct SourcePhysicsPoolResourceEvent {
  bool allocation;
  std::array<std::uint32_t, 5> request;
  std::array<std::uint32_t, 3> result;
};
using SourcePhysicsPoolResourceObserver =
    std::function<void(const SourcePhysicsPoolResourceEvent &)>;

// Owns fresh raw host pages for the selected TU1.5 static allocator imports.
// This explicit native stdcall ABI preserves incoming GP/FP/flags except EAX
// and consumed stack words. It is not recovered Xbox kernel clobber behavior.
// It does not supply any pool header, salt, validity or constructed payload.
class SourcePhysicsPoolHostStorage {
public:
  explicit SourcePhysicsPoolHostStorage(SourceRuntimeMemory &,
      std::uint32_t first_token = 0x10000000,
      const SourcePhysicsPoolResourceObserver & = {});
  ~SourcePhysicsPoolHostStorage();
  SourcePhysicsPoolHostStorage(const SourcePhysicsPoolHostStorage &) = delete;
  SourcePhysicsPoolHostStorage &operator=(const SourcePhysicsPoolHostStorage &) = delete;
  // Unsupported source calls return false unchanged. Host resource failures
  // throw without guessed Xbox NTSTATUS/TLS error state or successful return.
  bool try_call(SourcePhysicsContactMemory &,
      const SourcePhysicsContactRuntimeCall &, SourcePhysicsWorldRuntimeAbi &);
  bool backs(const SourceRuntimeMemory &memory) const { return &memory == &memory_; }
  std::size_t live_allocations() const { return allocations_.size(); }
  struct OwnershipCheckpoint {
    std::uint32_t version=1;
    SourceRuntimeHostStorage::CursorCheckpoint cursor;
    std::map<std::uint32_t,std::uint32_t> allocations;
    bool operator==(const OwnershipCheckpoint &) const = default;
  };
  class PreparedOwnership {
  public:
    ~PreparedOwnership();
    PreparedOwnership(PreparedOwnership &&) noexcept;
    PreparedOwnership &operator=(PreparedOwnership &&) noexcept;
    bool for_owner(const SourcePhysicsPoolHostStorage &) const noexcept;
  private:
    friend class SourcePhysicsPoolHostStorage;
    PreparedOwnership();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  OwnershipCheckpoint ownership_checkpoint() const;
  // Quiescent subowner import. Validate each retained raw allocation against
  // the complete incoming bank and stage metadata before bank publication.
  // The pool's resource label is distinct from arena scratch/static storage;
  // every incoming pool region must have exactly one registry entry.
  PreparedOwnership prepare_ownership(const OwnershipCheckpoint &,
                                      const SourceRuntimeMemory::Checkpoint &) const;
  // After whole-bank import, swap registries without releasing imported bytes.
  // Native observers stay bound to this local process and are not serialized.
  bool commit_ownership(PreparedOwnership &&) noexcept;
private:
  SourceRuntimeMemory &memory_;
  SourceRuntimeHostStorage raw_;
  SourcePhysicsPoolResourceObserver observe_;
  std::map<std::uint32_t, std::uint32_t> allocations_;
};

void source_run_physics_pool_startup_with_native_storage(SourceRuntimeMemory &,
    std::uint32_t canonical_entry, SourcePhysicsWorldRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint, const SourcePhysicsWorldRuntimeServices &,
    SourcePhysicsPoolHostStorage &,
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
