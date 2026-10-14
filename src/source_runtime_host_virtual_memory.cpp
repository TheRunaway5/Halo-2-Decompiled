#include "halo2/source_runtime_host_virtual_memory.hpp"
#include <cerrno>
#include <algorithm>
#include <cstring>
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
constexpr auto reservation_owner="actual host process-heap virtual reservation";
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
                       reservation_owner);
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
struct SourceRuntimeHostVirtualMemory::PreparedOwnership::Impl {
  const SourceRuntimeHostVirtualMemory *owner{};
  std::uint32_t next{};
  std::map<std::uint32_t,Reservation> reservations;
  ~Impl() {
#if defined(__linux__)
    for(const auto &[base,reservation]:reservations)::munmap(reservation.host,reservation.bytes);
#endif
  }
};
SourceRuntimeHostVirtualMemory::PreparedOwnership::PreparedOwnership()=default;
SourceRuntimeHostVirtualMemory::PreparedOwnership::~PreparedOwnership()=default;
SourceRuntimeHostVirtualMemory::PreparedOwnership::PreparedOwnership(PreparedOwnership &&) noexcept=default;
SourceRuntimeHostVirtualMemory::PreparedOwnership &SourceRuntimeHostVirtualMemory::PreparedOwnership::operator=(PreparedOwnership &&) noexcept=default;
bool SourceRuntimeHostVirtualMemory::PreparedOwnership::for_owner(const SourceRuntimeHostVirtualMemory &owner) const noexcept {
  return impl_ && impl_->owner==&owner;
}
SourceRuntimeHostVirtualMemory::OwnershipCheckpoint SourceRuntimeHostVirtualMemory::ownership_checkpoint() const {
  OwnershipCheckpoint saved;saved.next=next_;saved.reservations.reserve(reservations_.size());
  for(const auto &[base,reservation]:reservations_) {
    ReservationCheckpoint copy{base,reservation.bytes,{}};
    copy.committed.reserve(reservation.committed.size());
    for(bool value:reservation.committed)copy.committed.push_back(value?1:0);
    saved.reservations.push_back(std::move(copy));
  }
  return saved;
}
SourceRuntimeHostVirtualMemory::PreparedOwnership SourceRuntimeHostVirtualMemory::prepare_ownership(
    const OwnershipCheckpoint &saved,const SourceRuntimeMemory::Checkpoint &bank) const {
  SourceRuntimeMemory::validate_checkpoint(bank);
  if(saved.version!=1 || std::count_if(bank.regions.begin(),bank.regions.end(),
      [](const auto &region){return region.owner==reservation_owner;})!=std::ptrdiff_t(saved.reservations.size()))
    throw std::invalid_argument("Virtual ownership differs from the incoming source bank");
  PreparedOwnership prepared;prepared.impl_=std::make_unique<PreparedOwnership::Impl>();
  prepared.impl_->owner=this;prepared.impl_->next=saved.next;
  std::uint64_t previous_end{};
  for(const auto &reservation:saved.reservations) {
    const auto limit=std::uint64_t(reservation.base)+reservation.bytes;
    if(!reservation.bytes || reservation.base%page_bytes || reservation.bytes%page_bytes ||
        reservation.base<previous_end || limit>saved.next ||
        reservation.committed.size()!=reservation.bytes/page_bytes ||
        std::any_of(reservation.committed.begin(),reservation.committed.end(),[](auto value){return value>1;}))
      throw std::invalid_argument("Invalid native virtual reservation checkpoint");
    previous_end=limit;
    const auto region=std::lower_bound(bank.regions.begin(),bank.regions.end(),reservation.base,
        [](const auto &region,auto address){return region.address<address;});
    if(region==bank.regions.end() || region->address!=reservation.base || region->owner!=reservation_owner ||
        region->bytes.size()!=reservation.bytes || region->access!=SourceRuntimeMemory::Access::ReadWrite ||
        region->page_bytes!=page_bytes || region->committed!=reservation.committed)
      throw std::invalid_argument("Virtual reservation topology differs from the incoming source bank");
#if defined(__linux__)
    auto *host=::mmap(nullptr,reservation.bytes,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(host==MAP_FAILED)throw std::system_error(errno,std::generic_category(),"Stage native source reservation");
    try {
      Reservation staged{host,reservation.bytes,{}};
      staged.committed.reserve(reservation.committed.size());
      for(auto value:reservation.committed)staged.committed.push_back(value!=0);
      prepared.impl_->reservations.emplace(reservation.base,std::move(staged));
    } catch(...) {::munmap(host,reservation.bytes);throw;}
    for(std::size_t page=0;page<reservation.committed.size();) {
      if(!reservation.committed[page]){++page;continue;}
      const auto first=page;
      while(page<reservation.committed.size() && reservation.committed[page])++page;
      const auto offset=first*page_bytes,bytes=(page-first)*page_bytes;
      auto *destination=static_cast<std::byte*>(host)+offset;
      if(::mprotect(destination,bytes,PROT_READ|PROT_WRITE))
        throw std::system_error(errno,std::generic_category(),"Stage native source commitment");
      std::memcpy(destination,region->bytes.data()+offset,bytes);
    }
#else
    unsupported();
#endif
  }
  return prepared;
}
bool SourceRuntimeHostVirtualMemory::commit_ownership(PreparedOwnership &&prepared) noexcept {
  if(!prepared.for_owner(*this))return false;
  reservations_.swap(prepared.impl_->reservations);std::swap(next_,prepared.impl_->next);
  prepared.impl_.reset();return true;
}
} // namespace halo2
