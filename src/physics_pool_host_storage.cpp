#include "halo2/physics_pool_host_storage.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
#if defined(__linux__)
#include <unistd.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &m, std::uint32_t p) {
  std::array<std::byte, 4> b;
  m.read(p,b);
  std::uint32_t v=0;
  for(unsigned i=0;i<4;++i) v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return v;
}
void write(SourcePhysicsContactMemory &m,std::uint32_t p,std::uint32_t v) {
  std::array<std::byte,4> b;
  for(unsigned i=0;i<4;++i) b[i]=std::byte(v>>(8*i));
  m.write(p,b);
}
}
SourcePhysicsPoolHostStorage::SourcePhysicsPoolHostStorage(SourceRuntimeMemory &memory,
    std::uint32_t first,const SourcePhysicsPoolResourceObserver &observe)
    :memory_(memory),raw_(memory,first),observe_(observe) {}
SourcePhysicsPoolHostStorage::~SourcePhysicsPoolHostStorage() {
  for(const auto &[base,bytes]:allocations_) {
    // Only this owner's banks may be released. Do not throw while unwinding.
    try { memory_.release(base); } catch(...) {}
  }
}
struct SourcePhysicsPoolHostStorage::PreparedOwnership::Impl {
  const SourcePhysicsPoolHostStorage *owner;
  SourceRuntimeHostStorage::PreparedCursor cursor;
  std::map<std::uint32_t,std::uint32_t> allocations;
  Impl(const SourcePhysicsPoolHostStorage *owner,SourceRuntimeHostStorage::PreparedCursor cursor)
      :owner(owner),cursor(std::move(cursor)) {}
};
SourcePhysicsPoolHostStorage::PreparedOwnership::PreparedOwnership()=default;
SourcePhysicsPoolHostStorage::PreparedOwnership::~PreparedOwnership()=default;
SourcePhysicsPoolHostStorage::PreparedOwnership::PreparedOwnership(PreparedOwnership &&) noexcept=default;
SourcePhysicsPoolHostStorage::PreparedOwnership &SourcePhysicsPoolHostStorage::PreparedOwnership::operator=(PreparedOwnership &&) noexcept=default;
bool SourcePhysicsPoolHostStorage::PreparedOwnership::for_owner(const SourcePhysicsPoolHostStorage &owner) const noexcept {
  return impl_ && impl_->owner==&owner;
}
SourcePhysicsPoolHostStorage::OwnershipCheckpoint SourcePhysicsPoolHostStorage::ownership_checkpoint() const {
  return {1,raw_.cursor_checkpoint(),allocations_};
}
SourcePhysicsPoolHostStorage::PreparedOwnership SourcePhysicsPoolHostStorage::prepare_ownership(
    const OwnershipCheckpoint &saved,const SourceRuntimeMemory::Checkpoint &bank) const {
  SourceRuntimeMemory::validate_checkpoint(bank);
  if(saved.version!=1)throw std::invalid_argument("Unsupported physics pool resource checkpoint");
#if defined(__linux__)
  const auto page=::sysconf(_SC_PAGE_SIZE);
  if(page<=0)throw std::runtime_error("Host virtual-memory page size unavailable");
#else
  throw std::runtime_error("Required physics pool resource import platform unavailable");
  constexpr auto page=0x1000;
#endif
  PreparedOwnership prepared;
  prepared.impl_=std::make_unique<PreparedOwnership::Impl>(this,raw_.prepare_cursor(saved.cursor));
  std::uint64_t previous_end{};
  for(const auto &[base,bytes]:saved.allocations) {
    const auto limit=std::uint64_t(base)+bytes;
    if(!bytes || base%page || bytes%page || base<previous_end || limit>saved.cursor.next)
      throw std::invalid_argument("Invalid physics pool resource extent or allocation cursor");
    previous_end=limit;
    const auto region=std::lower_bound(bank.regions.begin(),bank.regions.end(),base,
        [](const auto &region,auto address){return region.address<address;});
    if(region==bank.regions.end() || region->address!=base || region->bytes.size()!=bytes ||
        region->owner!="actual host raw virtual-memory allocation" ||
        region->access!=SourceRuntimeMemory::Access::ReadWrite || region->page_bytes || !region->committed.empty())
      throw std::invalid_argument("Physics pool resource differs from incoming source bank");
  }
  prepared.impl_->allocations=saved.allocations;
  return prepared;
}
bool SourcePhysicsPoolHostStorage::commit_ownership(PreparedOwnership &&prepared) noexcept {
  if(!prepared.for_owner(*this))return false;
  if(!raw_.commit_cursor(std::move(prepared.impl_->cursor)))return false;
  allocations_.swap(prepared.impl_->allocations);prepared.impl_.reset();return true;
}
bool SourcePhysicsPoolHostStorage::try_call(SourcePhysicsContactMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi) {
  // Lowered TU1.5 owners can reach the actual XAPI sites directly; the pool
  // module retains the corresponding canonical IDs. Both require the same
  // authenticated TU1.5 return frame below and this owner's real resources.
  const bool allocate=(call.instruction==0x33786fu || call.instruction==0x33b3bfu) && call.target==0x800000b8u;
  const bool release=(call.instruction==0x3378abu || call.instruction==0x33b3fbu) && call.target==0x800000c7u;
  if(!allocate && !release) return false;
  if(&m!=&memory_ || call.tail)
    throw std::runtime_error("Original physics pool storage/call owner mismatch");
  const auto sp=abi.gp[4];
  if(read(m,sp)!=(allocate?0x33b3c5u:0x33b401u))
    throw std::runtime_error("Original TU1.5 virtual-memory return owner mismatch");
  const auto base_cell=read(m,sp+4);
  if(allocate) {
    const auto bytes_cell=read(m,sp+12);
    const SourceVirtualAllocationRequest request{read(m,base_cell),read(m,sp+8),
        read(m,bytes_cell),read(m,sp+16),read(m,sp+20)};
    const auto result=raw_.allocate(request);
    try { allocations_.emplace(result.base,result.bytes); }
    catch(...) {memory_.release(result.base);throw;}
    write(m,base_cell,result.base);write(m,bytes_cell,result.bytes);
    if(observe_) observe_({true,{request.base,request.zero_bits,request.bytes,
        request.allocation_type,request.protection},{result.status,result.base,result.bytes}});
    abi.gp[0]=result.status;abi.gp[4]+=24u;
  } else {
    const auto bytes_cell=read(m,sp+8),base=read(m,base_cell),
        bytes=read(m,bytes_cell),type=read(m,sp+12);
    const auto found=allocations_.find(base);
    if(bytes || type!=0x8000u || found==allocations_.end())
      throw std::runtime_error("Unowned original physics pool release request");
    memory_.release(base);allocations_.erase(found);
    // These by-reference input cells are dead in the complete source caller.
    // This native ABI leaves them unchanged; no Xbox output-cell claim.
    if(observe_) observe_({false,{base,bytes,type,0,0},{0,base,bytes}});
    abi.gp[0]=0;abi.gp[4]+=16u;
  }
  return true;
}
} // namespace halo2
