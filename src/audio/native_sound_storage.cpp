#include "halo2/native_sound_storage.hpp"
#include <limits>
#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint64_t source_limit = std::uint64_t{1} << 32;
constexpr std::size_t alias_page = 4096;
std::size_t rounded_extent(std::size_t extent, std::size_t granularity) {
  if (!extent || std::uint64_t(extent) > source_limit ||
      extent > std::numeric_limits<std::size_t>::max() - (granularity - 1))
    throw std::invalid_argument("Native sound storage extent is unavailable");
  return (extent + granularity - 1) & ~(granularity - 1);
}
std::size_t host_page_size() {
#ifdef _WIN32
  SYSTEM_INFO information;
  GetSystemInfo(&information);
  const auto size = std::size_t(information.dwPageSize);
#else
  const auto result = sysconf(_SC_PAGESIZE);
  if (result <= 0) throw std::runtime_error("Native sound storage page size unavailable");
  const auto size = std::size_t(result);
#endif
  if (!size || size > source_limit || (size & (size - 1)))
    throw std::runtime_error("Native sound storage page size unsupported");
  return size;
}
struct Region {
  std::size_t extent, host_extent;
  std::byte *data;
  Region(std::size_t size, std::size_t page)
      : extent(size), host_extent(rounded_extent(size, page)), data(nullptr) {
#ifdef _WIN32
    data = static_cast<std::byte *>(VirtualAlloc(nullptr, host_extent, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
    const auto result = mmap(nullptr, host_extent, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result != MAP_FAILED) data = static_cast<std::byte *>(result);
#endif
  }
  ~Region() {
    if (!data) return;
#ifdef _WIN32
    VirtualFree(data, 0, MEM_RELEASE);
#else
    munmap(data, host_extent);
#endif
  }
  Region(const Region &) = delete;
  Region &operator=(const Region &) = delete;
};
}

struct NativeSoundStorage::State {
  const NativeSoundStorageConfiguration configuration;
  const std::size_t page;
  std::uint64_t next_alias;
  std::map<std::uint32_t, std::unique_ptr<Region>> regions;

  explicit State(const NativeSoundStorageConfiguration &input)
      : configuration(input), page(host_page_size()), next_alias(input.first_allocation_alias) {
    if (!input.allocator_identity || !next_alias || next_alias % alias_page || input.allocation_alias_limit % alias_page ||
        next_alias >= input.allocation_alias_limit || input.allocation_alias_limit > source_limit)
      throw std::invalid_argument("Native sound allocation alias range is unavailable");
  }
  std::size_t rounded(std::size_t extent) const {
    return rounded_extent(extent, alias_page);
  }
  void interval(std::uint32_t address, std::size_t extent) const {
    if (!address || address % alias_page || std::uint64_t(address) + extent > source_limit)
      throw std::invalid_argument("Native sound storage alias interval is unavailable");
  }
  bool disjoint(std::uint32_t address, std::size_t extent) const {
    const auto following = regions.lower_bound(address);
    if (following != regions.end() && std::uint64_t(address) + extent > following->first) return false;
    if (following != regions.begin()) {
      const auto &prior = *std::prev(following);
      if (std::uint64_t(prior.first) + prior.second->extent > address) return false;
    }
    return true;
  }
  bool allocate_at(std::uint32_t address, std::size_t extent) {
    interval(address, extent);
    if (!disjoint(address, extent))
      throw std::invalid_argument("Native sound storage aliases overlap owned backing");
    auto backing = std::make_unique<Region>(extent, page);
    if (!backing->data) return false;
    regions.emplace(address, std::move(backing));
    return true;
  }
  std::pair<const Region *, std::size_t> resolve(std::uint32_t address, std::size_t extent) const {
    // A source address can be unaligned. The entire nonempty interval must
    // belong to one actual allocation, rather than a stitched implicit region.
    if (!extent || std::uint64_t(extent) > source_limit ||
        std::uint64_t(address) + std::uint64_t(extent) > source_limit)
      throw std::out_of_range("Native sound storage read extent is unavailable");
    auto entry = regions.upper_bound(address);
    if (entry == regions.begin())
      throw std::out_of_range("Native sound storage address is unavailable");
    --entry;
    const auto offset = std::uint64_t(address) - entry->first;
    if (offset + extent > entry->second->extent)
      throw std::out_of_range("Native sound storage interval crosses owned backing");
    return {entry->second.get(), std::size_t(offset)};
  }
  std::span<std::byte> memory(std::uint32_t address, std::size_t extent) const {
    const auto [region, offset] = resolve(address, extent);
    return {region->data + offset, extent};
  }
  std::uint32_t allocate(std::uint32_t owner, std::uint32_t requested) {
    if (owner != configuration.allocator_identity)
      throw std::runtime_error("Native sound raw allocator identity is unavailable");
    const auto extent = rounded(requested);
    auto candidate = next_alias;
    for (const auto &[address, backing] : regions) {
      const auto end = std::uint64_t(address) + backing->extent;
      if (end <= candidate) continue;
      if (std::uint64_t(address) >= candidate + extent) break;
      candidate = end;
    }
    if (candidate + extent > configuration.allocation_alias_limit)
      throw std::runtime_error("Native sound raw allocator alias space is unavailable");
    const auto alias = std::uint32_t(candidate);
    if (!allocate_at(alias, extent)) return 0;
    next_alias = candidate + extent;
    return alias;
  }
  void protect(std::uint32_t address, std::uint32_t extent, std::uint32_t mode) const {
    // The verified21A6E0 request is PAGE_READWRITE4. Other modes have no
    // represented cache constructor owner and cannot silently succeed here.
    if (mode != 4 || !extent || address % alias_page || extent % alias_page)
      throw std::invalid_argument("Native sound protection request is unsupported");
    const auto [region, offset] = resolve(address, extent);
    // Only read/write is admitted. Round its real host operation within this
    // independently allocated region; host padding is never an exposed alias.
    const auto first = offset & ~(page - 1);
    const auto last = rounded_extent(offset + extent, page);
    if (last > region->host_extent)
      throw std::out_of_range("Native sound protection escapes owned host backing");
    const auto data = region->data + first;
    const auto bytes = last - first;
#ifdef _WIN32
    DWORD previous;
    if (!VirtualProtect(data, bytes, PAGE_READWRITE, &previous))
      throw std::runtime_error("Native sound storage VirtualProtect failed");
#else
    if (mprotect(data, bytes, PROT_READ | PROT_WRITE) != 0)
      throw std::runtime_error("Native sound storage mprotect failed");
#endif
  }
};

NativeSoundStorage::NativeSoundStorage(const NativeSoundStorageConfiguration &configuration)
    : state_(std::make_shared<State>(configuration)) {}
void NativeSoundStorage::map_zeroed(std::uint32_t address, std::size_t extent) {
  if (!state_->allocate_at(address, state_->rounded(extent)))
    throw std::runtime_error("Native sound storage OS allocation failed");
}
std::span<std::byte> NativeSoundStorage::memory(std::uint32_t address, std::size_t extent) const {
  return state_->memory(address, extent);
}
std::size_t NativeSoundStorage::page_size() const { return state_->page; }
std::size_t NativeSoundStorage::alias_page_size() const { return alias_page; }
std::size_t NativeSoundStorage::region_count() const { return state_->regions.size(); }
SoundCacheStartupServices NativeSoundStorage::cache_services() const {
  const auto owner = state_;
  return {
      [owner](std::uint32_t address, std::size_t extent) { return owner->memory(address, extent); },
      [owner](std::uint32_t identity, std::uint32_t extent) { return owner->allocate(identity, extent); },
      [owner](std::uint32_t address, std::uint32_t extent, std::uint32_t mode) {
        owner->protect(address, extent, mode);
      }};
}
} // namespace halo2
