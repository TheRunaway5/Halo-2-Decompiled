#include "halo2/source_runtime_host_storage.hpp"
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace halo2 {
SourceRuntimeHostStorage::SourceRuntimeHostStorage(SourceRuntimeMemory &memory,
                                                   std::uint32_t first)
    : memory_(memory), next_(first) {}

SourceRuntimeHostStorage::PreparedCursor::PreparedCursor(const SourceRuntimeHostStorage *owner,std::uint32_t next)
    :owner_(owner),next_(next) {}
SourceRuntimeHostStorage::PreparedCursor::PreparedCursor(PreparedCursor &&other) noexcept
    :owner_(std::exchange(other.owner_,nullptr)),next_(other.next_) {}
SourceRuntimeHostStorage::PreparedCursor &SourceRuntimeHostStorage::PreparedCursor::operator=(PreparedCursor &&other) noexcept {
  if(this!=&other){owner_=std::exchange(other.owner_,nullptr);next_=other.next_;}return *this;
}
bool SourceRuntimeHostStorage::PreparedCursor::for_owner(const SourceRuntimeHostStorage &owner) const noexcept {
  return owner_==&owner;
}
SourceRuntimeHostStorage::CursorCheckpoint SourceRuntimeHostStorage::cursor_checkpoint() const noexcept {
  return {1,next_};
}
SourceRuntimeHostStorage::PreparedCursor SourceRuntimeHostStorage::prepare_cursor(const CursorCheckpoint &saved) const {
  if(saved.version!=1)throw std::invalid_argument("Unsupported raw host allocation cursor checkpoint");
  return {this,saved.next};
}
bool SourceRuntimeHostStorage::commit_cursor(PreparedCursor &&prepared) noexcept {
  if(!prepared.for_owner(*this))return false;
  next_=prepared.next_;prepared.owner_=nullptr;return true;
}

SourceVirtualAllocationResult
SourceRuntimeHostStorage::allocate(SourceVirtualAllocationRequest request) {
  return allocate(request,"actual host raw virtual-memory allocation");
}
SourceVirtualAllocationResult
SourceRuntimeHostStorage::allocate(SourceVirtualAllocationRequest request,std::string owner) {
  if(owner.empty())throw std::invalid_argument("Raw host storage requires an explicit resource owner");
  // Authenticated main12008 reserves its sound arena with a null-base
  // MEM_COMMIT request(1000); the other retained source constructors request
  // 101000. Both bind fresh committed native pages here. Existing-address
  // commits still belong to SourceRuntimeHostVirtualMemory, not this owner.
  if (request.base || request.zero_bits ||
      (request.allocation_type != 0x101000 && request.allocation_type != 0x1000) ||
      request.protection != 4 || !request.bytes)
    throw std::runtime_error("Required original virtual-memory request adapter unavailable");
#if defined(__linux__)
  const auto page = ::sysconf(_SC_PAGE_SIZE);
  if (page <= 0)
    throw std::runtime_error("Host virtual-memory page size unavailable");
  const auto bytes = (std::uint64_t(request.bytes) + page - 1) / page * page;
  const auto base = (std::uint64_t(next_) + page - 1) / page * page;
  if (bytes > std::numeric_limits<std::uint32_t>::max() ||
      base + bytes > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error("Host original-address storage exhausted");
  auto *raw = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (raw == MAP_FAILED)
    throw std::system_error(errno, std::generic_category(),
                            "Host original-address allocation");
  // SourceRuntimeMemory retains its own allocation. Copy only the actual
  // host page contents; no pool header, payload field or salt is initialized.
  try {
    memory_.mount(std::uint32_t(base),
                   {static_cast<const std::byte *>(raw), std::size_t(bytes)},
                   SourceRuntimeMemory::Access::ReadWrite,
                   std::move(owner));
  } catch (...) {
    ::munmap(raw, bytes);
    throw;
  }
  ::munmap(raw, bytes);
  next_ = std::uint32_t(base + bytes);
  // This success follows the actual host allocation. Failure is reported
  // above; no guessed Xbox NTSTATUS is returned for a host error.
  return {0, std::uint32_t(base), std::uint32_t(bytes)};
#else
  throw std::runtime_error("Required original virtual-memory platform adapter unavailable");
#endif
}
} // namespace halo2
