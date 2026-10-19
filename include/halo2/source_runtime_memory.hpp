#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/native_runtime_address.hpp"
#include "halo2/source_runtime_snapshot_bytes.hpp"
#include <array>
#include <map>
#include <string>
#include <atomic>
#include <memory>
#include <functional>
#include <span>

namespace halo2 {
class SourceRuntimeMemory;
// One retained host allocation. The producer supplies its real lifetime and
// complete span; one memory region may publish it at a time. Platform owners
// may retain the same lease to protect/release the actual source pages.
class SourceRuntimeMemoryBacking final {
public:
  using WriteGuard = std::function<void(std::uint32_t,std::size_t)>;
  SourceRuntimeMemoryBacking(std::shared_ptr<void> lifetime,
      std::span<std::byte> bytes, WriteGuard = {});
  std::span<std::byte> bytes() const noexcept { return bytes_; }
private:
  friend class SourceRuntimeMemory;
  std::shared_ptr<void> lifetime_;
  std::span<std::byte> bytes_;
  WriteGuard write_guard_;
  std::atomic<bool> mounted_{false};
};
// Host backing for retained original32-bit banks. Addresses are data tokens,
// never host pointers. The allocation/constructor owners choose every region
// and its initial bytes; mounting does not construct an object, initialize a
// pool, admit physics or produce an animation pose.
class SourceRuntimeMemory final : public SourcePhysicsContactMemory {
public:
  enum class Access { ReadOnly, ReadWrite };
  struct CheckpointRegion;
  SourceRuntimeMemory() = default;
  SourceRuntimeMemory(const SourceRuntimeMemory &) = delete;
  SourceRuntimeMemory &operator=(const SourceRuntimeMemory &) = delete;
  SourceRuntimeMemory(SourceRuntimeMemory &&);
  SourceRuntimeMemory &operator=(SourceRuntimeMemory &&);
  // Publish prevalidated backing without moving this owner's identity or its
  // registered resource guards. Both banks/users must be quiescent; the world
  // transaction commits staged resource owners immediately afterward. Every
  // borrowed view/native address must then be rebound.
  void swap_backing(SourceRuntimeMemory &);

  // Own a copy of the actual initial backing. Overlap/wrap/empty regions are
  // rejected. Regions stay at their chosen address until explicitly released.
  void mount(std::uint32_t address, std::span<const std::byte> initial, Access,
             std::string owner);
  void mount(std::uint32_t address, std::shared_ptr<SourceRuntimeMemoryBacking>,
             Access, std::string owner);
  // Quiescent staging only. Require identical complete bytes/extent before
  // replacing a copied region with its prepared platform lease. Existing
  // views/native addresses for this region must be rebound afterward.
  void bind_backing(const CheckpointRegion &, std::shared_ptr<SourceRuntimeMemoryBacking>);
  // Quiescent staging: retain an exclusive, page-isolated restored snapshot
  // mapping after checking its complete bytes/metadata. Its native address
  // stays fixed and the supplied platform guard follows the lease. Returns
  // null when this region needs a separate aligned platform allocation.
  std::shared_ptr<SourceRuntimeMemoryBacking> prepare_snapshot_backing(
      const CheckpointRegion &, std::size_t alignment,
      SourceRuntimeMemoryBacking::WriteGuard);
  bool has_backing(std::uint32_t address,
                   const std::shared_ptr<SourceRuntimeMemoryBacking> &) const noexcept;
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
  // other regions. Releasing/rebinding the region or replacing the bank
  // invalidates them.
  std::span<const std::byte> view(std::uint32_t, std::size_t) const;
  bool is_read_only(std::uint32_t,std::size_t) const;
  std::span<std::byte> mutable_view(std::uint32_t, std::size_t);
  // Admit a future mutable borrower without handing out a writable span.
  // Includes region/commit/access and platform-resource write guards.
  // Does not hand out a span or notify actual-write observers.
  void validate_mutable_view(std::uint32_t, std::size_t) const;
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
    SourceRuntimeSnapshotBytes bytes;
    std::vector<std::uint8_t> committed;
    bool operator==(const CheckpointRegion &) const = default;
  };
  struct Checkpoint {
    std::uint32_t version=1;
    std::vector<CheckpointRegion> regions;
    bool operator==(const Checkpoint &) const = default;
  };
  Checkpoint checkpoint() const;
  // Compare only immutable regions after whole-checkpoint structural admission.
  // Checks complete bytes and every region field in source-address order,
  // without allocating snapshots/lists or changing either memory owner.
  bool matches_read_only_checkpoint(const Checkpoint &) const noexcept;
  // Allocation-free structural admission, also used by staged native resource
  // owners before the enclosing quiescent world transaction publishes bytes.
  static void validate_checkpoint(const Checkpoint &);
  void restore(const Checkpoint &);

private:
  struct Region {
    std::vector<std::byte> bytes;
    Access access;
    std::string owner;
    std::size_t page_bytes = 0;
    std::vector<bool> committed;
    std::shared_ptr<SourceRuntimeMemoryBacking> shared;
    Region(std::vector<std::byte> data, Access mode, std::string name,
           std::size_t page_size=0, std::vector<bool> pages={})
        :bytes(std::move(data)),access(mode),owner(std::move(name)),
         page_bytes(page_size),committed(std::move(pages)) {}
    Region(std::shared_ptr<SourceRuntimeMemoryBacking> backing, Access mode,
           std::string name):access(mode),owner(std::move(name)),shared(std::move(backing)) {}
    Region(Region &&) noexcept = default;
    ~Region() { if(shared)shared->mounted_.store(false); }
    std::span<std::byte> storage() noexcept { return shared?shared->bytes():std::span(bytes); }
    std::span<const std::byte> storage() const noexcept { return shared?shared->bytes():std::span(bytes); }
  };
  std::map<std::uint32_t, Region> regions_;
  std::map<std::uint64_t,std::uint32_t> native_regions_;
  using Position = std::map<std::uint32_t, Region>::const_iterator;
  // Full source pages have exactly one region owner. Index only those pages;
  // partial pages and gaps retain the checked interval lookup. Published
  // entries are read-only between topology changes, so concurrent readers do
  // not mutate a cache. Map nodes remain stable across bank moves/swaps.
  using SourcePageBlock = std::array<Position,1024>;
  using SourcePageIndex = std::array<std::unique_ptr<SourcePageBlock>,1024>;
  std::unique_ptr<SourcePageIndex> source_pages_;
  void index_source_region(Position);
  void unindex_source_region(Position) noexcept;
  Position locate(std::uint32_t) const;
  void check_extent(std::uint32_t, std::size_t) const;
  void validate(std::uint32_t, std::size_t, bool writable) const;
  void validate(std::uint32_t, std::size_t, bool writable,bool observe) const;
  void index_native_region(std::uint32_t);
  void index_native_span(std::uint32_t,std::span<const std::byte>);
  void rebuild_native_index();
};
} // namespace halo2
