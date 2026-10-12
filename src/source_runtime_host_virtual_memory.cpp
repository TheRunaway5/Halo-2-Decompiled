#include "halo2/source_runtime_host_virtual_memory.hpp"
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <system_error>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t page_bytes = 0x1000;
[[noreturn]] void unsupported() {
  throw std::runtime_error("Required original reserve/commit request adapter unavailable");
}
}
SourceRuntimeHostVirtualMemory::SourceRuntimeHostVirtualMemory(
    SourceRuntimeMemory &memory, std::uint32_t first)
    : memory_(memory), next_(first) {
#if defined(__linux__)
  if (::sysconf(_SC_PAGE_SIZE) != page_bytes)
    throw std::runtime_error("Reached source reservations require a4096-byte host page");
#else
  throw std::runtime_error("Required original reserve/commit platform adapter unavailable");
#endif
}
SourceRuntimeHostVirtualMemory::~SourceRuntimeHostVirtualMemory() {
  for (const auto &[base, reservation] : reservations_) {
#if defined(__linux__)
    ::munmap(reservation.host, reservation.bytes);
#endif
    // Destruction cannot throw. External release of these banks violates the
    // owner's lifetime contract, but must not terminate exception unwinding.
    try { memory_.release(base); } catch (...) {}
  }
}
SourceVirtualAllocationResult SourceRuntimeHostVirtualMemory::allocate(
    SourceVirtualAllocationRequest request) {
  if (request.zero_bits || request.protection != 4 || !request.bytes ||
      request.bytes % page_bytes)
    unsupported();
#if defined(__linux__)
  if (request.allocation_type == 0x2000 && !request.base) {
    const auto base = (std::uint64_t(next_) + page_bytes - 1) &
                      ~std::uint64_t(page_bytes - 1);
    const auto limit = base + request.bytes;
    if (limit > std::numeric_limits<std::uint32_t>::max())
      throw std::runtime_error("Host original-address reservation space exhausted");
    auto *host = ::mmap(nullptr, request.bytes, PROT_NONE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (host == MAP_FAILED)
      throw std::system_error(errno, std::generic_category(), "Host source reservation");
    try {
      memory_.reserve(std::uint32_t(base), request.bytes, page_bytes,
                       "actual host process-heap virtual reservation");
      try {
        reservations_.emplace(std::uint32_t(base),
            Reservation{host, request.bytes,
                         std::vector<bool>(request.bytes / page_bytes, false)});
      } catch (...) { memory_.release(std::uint32_t(base)); throw; }
    } catch (...) { ::munmap(host, request.bytes); throw; }
    next_ = std::uint32_t(limit);
    return {0, std::uint32_t(base), request.bytes};
  }
  if (request.allocation_type != 0x1000 || !request.base ||
      request.base % page_bytes)
    unsupported();
  auto found = reservations_.upper_bound(request.base);
  if (found == reservations_.begin()) unsupported();
  --found;
  auto &reservation = found->second;
  const auto offset = std::uint64_t(request.base) - found->first;
  if (offset + request.bytes > reservation.bytes) unsupported();
  auto *host = static_cast<std::byte *>(reservation.host) + offset;
  if (::mprotect(host, request.bytes, PROT_READ | PROT_WRITE))
    throw std::system_error(errno, std::generic_category(), "Host source page commitment");
  try {
    memory_.commit(request.base, {host, request.bytes});
  } catch (...) {
    // Restore the previous host protection if importing the real page bytes
    // failed before publishing a new commitment to SourceRuntimeMemory.
    for (std::size_t done = 0; done < request.bytes; done += page_bytes)
      if (!reservation.committed[(offset + done) / page_bytes])
        ::mprotect(host + done, page_bytes, PROT_NONE);
    throw;
  }
  for (std::size_t done = 0; done < request.bytes; done += page_bytes)
    reservation.committed[(offset + done) / page_bytes] = true;
  // Native resource success; this does not recover Xbox kernel register or
  // status behavior for any failed or differently shaped request.
  return {0, request.base, request.bytes};
#else
  unsupported();
#endif
}
void SourceRuntimeHostVirtualMemory::release(std::uint32_t base,
                                             std::uint32_t bytes,
                                             std::uint32_t type) {
  const auto found = reservations_.find(base);
  if (type != 0x8000 || found == reservations_.end() ||
      bytes != found->second.bytes)
    unsupported();
#if defined(__linux__)
  if (::munmap(found->second.host, bytes))
    throw std::system_error(errno, std::generic_category(), "Host source reservation release");
  memory_.release(base);
  reservations_.erase(found);
#else
  unsupported();
#endif
}
} // namespace halo2
