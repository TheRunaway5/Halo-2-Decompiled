#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <memory>
#include <mutex>
#include <set>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t page = 0x1000;
[[noreturn]] void unsupported() {
  throw std::runtime_error("Required original physical-memory request adapter unavailable");
}
struct PhysicalProtection {
  std::mutex mutex;
  std::set<std::uint32_t> read_only;
};
std::mutex physical_protection_mutex;
std::map<const SourceRuntimeHostPhysicalMemory *,std::shared_ptr<PhysicalProtection>> physical_protections;
std::shared_ptr<PhysicalProtection> protection_of(const SourceRuntimeHostPhysicalMemory *owner) {
  std::lock_guard lock(physical_protection_mutex);return physical_protections.at(owner);
}
}
SourceRuntimeHostPhysicalMemory::SourceRuntimeHostPhysicalMemory(SourceRuntimeMemory &memory)
    : memory_(memory) {
#if defined(__linux__)
  if (::sysconf(_SC_PAGE_SIZE) != page)
    throw std::runtime_error("Reached physical storage requires4096-byte host pages");
#else
  unsupported();
#endif
  auto state=std::make_shared<PhysicalProtection>();
  {
    std::lock_guard lock(physical_protection_mutex);physical_protections.emplace(this,state);
  }
  try {
    source_runtime_register_write_guard(memory_,this,[state](std::uint32_t address,std::size_t bytes) {
      const auto end=std::uint64_t(address)+bytes;
      std::lock_guard lock(state->mutex);
      const auto first=state->read_only.lower_bound(address&~(page-1));
      if(first!=state->read_only.end() && *first<end)
        throw std::runtime_error("Original runtime write to read-only physical page");
    });
  } catch(...) {
    std::lock_guard lock(physical_protection_mutex);physical_protections.erase(this);throw;
  }
}
SourceRuntimeHostPhysicalMemory::~SourceRuntimeHostPhysicalMemory() {
  source_runtime_unregister_write_guard(memory_,this);
  {std::lock_guard lock(physical_protection_mutex);physical_protections.erase(this);}
  for (const auto &[base, allocation] : allocations_) {
#if defined(__linux__)
    ::munmap(allocation.host, allocation.bytes);
#endif
    try { memory_.release(base); } catch (...) {}
  }
}
std::uint32_t SourceRuntimeHostPhysicalMemory::allocate(SourcePhysicalAllocationRequest q) {
  if (!q.bytes || q.protection != 4 || q.lowest > q.highest ||
      q.highest >= 0x80000000u || (q.alignment && (q.alignment & (q.alignment - 1))))
    unsupported();
  const std::uint64_t alignment = std::max(q.alignment, page);
  const auto bytes = (std::uint64_t(q.bytes) + page - 1) & ~std::uint64_t(page - 1);
  auto physical = (std::uint64_t(q.lowest) + alignment - 1) & ~(alignment - 1);
  // Native first-fit resource policy. The reached original12CB70 request
  // fills its entire allowed range, so only one physical token can satisfy it.
  for (const auto &[token, allocation] : allocations_) {
    const std::uint64_t lo = token & 0x7fffffffu, hi = lo + allocation.bytes;
    if (physical < hi && physical + bytes > lo)
      physical = (hi + alignment - 1) & ~(alignment - 1);
  }
  if (physical + bytes > std::uint64_t(q.highest) + 1 || bytes > UINT32_MAX)
    return 0; // Actual native resource namespace exhaustion.
  const auto token = std::uint32_t(physical) | 0x80000000u;
#if defined(__linux__)
  auto *host = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (host == MAP_FAILED)
    throw std::system_error(errno, std::generic_category(), "Host source physical allocation");
  try {
    memory_.mount(token, {static_cast<const std::byte *>(host), std::size_t(bytes)},
        SourceRuntimeMemory::Access::ReadWrite, "actual host physical allocation pages");
    try { allocations_.emplace(token, Allocation{host, std::uint32_t(bytes)}); }
    catch (...) { memory_.release(token); throw; }
  } catch (...) { ::munmap(host, bytes); throw; }
  return token;
#else
  unsupported();
#endif
}
void SourceRuntimeHostPhysicalMemory::protect(std::uint32_t address,
                                              std::uint32_t bytes,
                                              std::uint32_t protection) {
  // Actual TU1.5 12ED5E requests PAGE_READWRITE | PAGE_WRITECOMBINE.
  // This native process uses anonymous host pages and the host cache policy;
  // mprotect owns their real read/write access. Xbox GPU cache coherency is
  // a separate platform contract, not a result supplied by this adapter.
  // https://learn.microsoft.com/en-us/windows/win32/memory/memory-protection-constants
  if (!bytes || (protection != 2 && protection != 4 && protection != 0x404)) unsupported();
  auto found = allocations_.upper_bound(address);
  if (found == allocations_.begin()) unsupported();
  --found;
  const auto offset = std::uint64_t(address) - found->first;
  if (offset + bytes > found->second.bytes) unsupported();
  // MmSetAddressProtect consumes byte ranges: the pinned kernel compatibility
  // owner visits GetPteAddress(address)..GetPteAddress(address+bytes-1).
  // Actual11F0A2/11F1C4 metadata requests are neither page-aligned nor page-
  // sized. Preserve that source request and round only the Linux syscall's
  // host range, after validating it against this actual allocation.
  const auto protected_begin = offset & ~std::uint64_t(page - 1);
  const auto protected_end = (offset + bytes + page - 1) & ~std::uint64_t(page - 1);
  const auto state=protection_of(this);std::lock_guard lock(state->mutex);
  auto next=state->read_only;
  for(auto p=std::uint64_t(found->first)+protected_begin;
      p<std::uint64_t(found->first)+protected_end;p+=page) {
    if(protection==2)next.insert(std::uint32_t(p));else next.erase(std::uint32_t(p));
  }
#if defined(__linux__)
  if (::mprotect(static_cast<std::byte *>(found->second.host) + protected_begin,
                 std::size_t(protected_end - protected_begin),
                 protection==2?PROT_READ:PROT_READ | PROT_WRITE))
    throw std::system_error(errno, std::generic_category(), "Host source physical protection");
#else
  unsupported();
#endif
  state->read_only.swap(next);
}
bool source_try_native_death_physical_call_1_1(
    SourceRuntimeMemory &memory, const SourceUnitAnimationRuntimeCall &call,
    SourceUnitAnimationRuntimeAbi &abi, SourceRuntimeHostPhysicalMemory &physical) {
  const bool allocation = call.instruction == 0x2d79d3u &&
      call.target == 0x800000a6u && !call.tail;
  const bool protection = call.instruction == 0x2d7a21u &&
      call.target == 0x800000b6u && call.tail;
  if (!allocation && !protection) return false;
  if (!physical.backs(memory))
    throw std::invalid_argument("Physical resource and original frame banks differ");
  const auto word = [&](std::uint32_t address) {
    std::array<std::byte,4> raw;
    memory.read(address, raw);
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i)
      value |= std::to_integer<std::uint32_t>(raw[i]) << (8*i);
    return value;
  };
  const auto sp = abi.gp[4];
  if (allocation) {
    if (word(0x417ba0) != call.target || word(sp) != 0x2d79d9u)
      throw std::invalid_argument("Original physical allocation call frame differs");
    const SourcePhysicalAllocationRequest request{word(sp+4), word(sp+8),
        word(sp+12), word(sp+16), word(sp+20)};
    abi.gp[0] = physical.allocate(request);
    abi.gp[4] = sp + 24;
  } else {
    if (word(0x417ba8) != call.target)
      throw std::invalid_argument("Original physical protection import differs");
    physical.protect(word(sp+4), word(sp+8), word(sp+12));
    abi.gp[4] = sp + 16;
  }
  return true;
}
} // namespace halo2
