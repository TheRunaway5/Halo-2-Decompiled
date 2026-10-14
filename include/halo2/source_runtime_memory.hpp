#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/native_runtime_address.hpp"
#include <map>
#include <string>

namespace halo2 {
// Host backing for retained original32-bit banks. Addresses are data tokens,
// never host pointers. The allocation/constructor owners choose every region
// and its initial bytes; mounting does not construct an object, initialize a
// pool, admit physics or produce an animation pose.
class SourceRuntimeMemory final : public SourcePhysicsContactMemory {
public:
  enum class Access { ReadOnly, ReadWrite };
  SourceRuntimeMemory() = default;
  SourceRuntimeMemory(const SourceRuntimeMemory &) = delete;
  SourceRuntimeMemory &operator=(const SourceRuntimeMemory &) = delete;
  SourceRuntimeMemory(SourceRuntimeMemory &&) = default;
  SourceRuntimeMemory &operator=(SourceRuntimeMemory &&) = default;

  // Own a copy of the actual initial backing. Overlap/wrap/empty regions are
  // rejected. Regions stay at their chosen address until explicitly released.
  void mount(std::uint32_t address, std::span<const std::byte> initial, Access,
             std::string owner);
  // Reserve one stable bank without making any bytes source-accessible.
  // Commit imports actual initial page bytes only for previously uncommitted
  // pages; committing again preserves writes made by the source owners.
  void reserve(std::uint32_t address, std::size_t bytes, std::size_t page_bytes,
               std::string owner);
  void commit(std::uint32_t address, std::span<const std::byte> initial);
  void release(std::uint32_t region_address);
  void read(std::uint32_t, std::span<std::byte>) const override;
  void write(std::uint32_t, std::span<const std::byte>) override;
  // Span adapters for original heap, constructors and packed animation
  // owners. Each view must fit a single real region. Views survive mounting
  // other regions and are invalidated only by releasing their own region.
  std::span<const std::byte> view(std::uint32_t, std::size_t) const;
  std::span<std::byte> mutable_view(std::uint32_t, std::size_t);
  // Live engine access uses native addresses. Original tokens are resolved
  // only at this owner boundary; host pointers are never truncated or saved.
  NativeRuntimeAddress native_address(std::uint32_t source,std::size_t) const;
  std::uint32_t source_address(NativeRuntimeAddress,std::size_t) const;
  std::span<const std::byte> view(NativeRuntimeAddress,std::size_t) const;
  std::span<std::byte> mutable_view(NativeRuntimeAddress,std::size_t);
  void read(NativeRuntimeAddress,std::span<std::byte>) const;
  void write(NativeRuntimeAddress,std::span<const std::byte>);
  // Read-only diagnostic export of accessible storage. Uncommitted pages are
  // omitted. Borrowed spans remain valid only during the synchronous callback.
  void visit_committed(const std::function<void(std::uint32_t,
      std::span<const std::byte>)> &) const;
  // Complete memory-owner checkpoint, including reservation holes and access
  // policy. This is one subowner of a world continuation, not a process or
  // physics-world save. The caller must quiesce execution and rebind all
  // borrowed views after a successful restore. Rejected restores are atomic.
  // Native resource access guards belong to their platform owners and must
  // be restored separately; mutable views cannot span protection changes.
  // Network imports additionally require authenticated image/map bindings and
  // complete runtime-resource validation by the enclosing world owner.
  struct CheckpointRegion {
    std::uint32_t address{};
    Access access{};
    std::string owner;
    std::size_t page_bytes{};
    std::vector<std::byte> bytes;
    std::vector<std::uint8_t> committed;
    bool operator==(const CheckpointRegion &) const = default;
  };
  struct Checkpoint {
    std::uint32_t version=1;
    std::vector<CheckpointRegion> regions;
    bool operator==(const Checkpoint &) const = default;
  };
  Checkpoint checkpoint() const;
  void restore(const Checkpoint &);

private:
  struct Region {
    std::vector<std::byte> bytes;
    Access access;
    std::string owner;
    std::size_t page_bytes = 0;
    std::vector<bool> committed;
  };
  std::map<std::uint32_t, Region> regions_;
  std::map<std::uint64_t,std::uint32_t> native_regions_;
  using Position = std::map<std::uint32_t, Region>::const_iterator;
  Position locate(std::uint32_t) const;
  void check_extent(std::uint32_t, std::size_t) const;
  void validate(std::uint32_t, std::size_t, bool writable) const;
  void index_native_region(std::uint32_t);
  void rebuild_native_index();
};
} // namespace halo2
