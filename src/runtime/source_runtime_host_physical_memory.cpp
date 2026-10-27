#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "runtime/source_runtime_page_copy.hpp"
#include "runtime/source_runtime_host_physical_pages.hpp"
#include <algorithm>
#include <atomic>
#include <array>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <memory>
#include <mutex>
#include <set>
#include <cstring>
#include <cstdlib>

namespace halo2 {
namespace {
constexpr std::uint32_t page = 0x1000;
constexpr auto allocation_owner="actual host physical allocation pages";
[[noreturn]] void unsupported() {
  throw std::runtime_error("Required original physical-memory request adapter unavailable");
}
struct PhysicalProtection {
  std::mutex mutex;
  std::set<std::uint32_t> read_only;
  // Lock-free mirror of read_only for the per-write guard: one bit per
  // upper-half 4KiB page (allocate() publishes only upper-half tokens).
  // Writers hold mutex and change one page bit at a time, so the guard's
  // acquire loads see each page either before or after its change; it never
  // takes the mutex for a write that starts inside the namespace.
  std::unique_ptr<std::array<std::atomic<std::uint64_t>,8192>> bits=
      std::make_unique<std::array<std::atomic<std::uint64_t>,8192>>();
  void mark(std::uint32_t address,bool on) noexcept {
    if(address<0x80000000u)return;
    const auto index=(address-0x80000000u)>>12;const auto bit=std::uint64_t{1}<<(index&63);
    if(on)(*bits)[index>>6].fetch_or(bit,std::memory_order_release);
    else (*bits)[index>>6].fetch_and(~bit,std::memory_order_release);
  }
  // Any read-only page overlapping [first,end); first>=0x80000000, end>first.
  bool protected_in(std::uint64_t first,std::uint64_t end) const noexcept {
    for(auto index=(first-0x80000000ull)>>12,last=(end-1-0x80000000ull)>>12;index<=last;++index)
      if((*bits)[index>>6].load(std::memory_order_acquire)&(std::uint64_t{1}<<(index&63)))return true;
    return false;
  }
  void erase_range(std::set<std::uint32_t>::iterator first,std::set<std::uint32_t>::iterator last) noexcept {
    for(auto at=first;at!=last;++at)mark(*at,false);
    read_only.erase(first,last);
  }
  // Swap in next, changing exactly the pages that differ.
  void replace(std::set<std::uint32_t> &next) noexcept {
    auto a=read_only.begin();auto b=next.begin();
    while(a!=read_only.end() || b!=next.end()) {
      if(b==next.end() || (a!=read_only.end() && *a<*b)){mark(*a,false);++a;}
      else if(a==read_only.end() || *b<*a){mark(*b,true);++b;}
      else{++a;++b;}
    }
    read_only.swap(next);
  }
};
struct StagedPhysicalProtection {
  std::set<std::uint32_t> read_only;
  bool active=true;
  void validate(std::uint32_t address,std::size_t bytes) const {
    if(!active || !bytes)return;
    const auto first=read_only.lower_bound(address&~(page-1));
    if(first!=read_only.end() && *first<std::uint64_t(address)+bytes)
      throw std::runtime_error("Original runtime write to read-only staged physical page");
  }
};
std::shared_ptr<SourceRuntimeMemoryBacking> physical_backing(std::uint32_t bytes,
    SourceRuntimeMemoryBacking::WriteGuard guard={}) {
  auto *host=source_runtime_detail::physical_pages::allocate(bytes);
  auto lifetime=std::shared_ptr<void>(host,[bytes](void *p){source_runtime_detail::physical_pages::release(p,bytes);});
  return std::make_shared<SourceRuntimeMemoryBacking>(std::move(lifetime),
      std::span(static_cast<std::byte*>(host),bytes),std::move(guard));
}
std::mutex physical_protection_mutex;
std::map<const SourceRuntimeHostPhysicalMemory *,std::shared_ptr<PhysicalProtection>> physical_protections;
std::shared_ptr<PhysicalProtection> protection_of(const SourceRuntimeHostPhysicalMemory *owner) {
  std::lock_guard lock(physical_protection_mutex);return physical_protections.at(owner);
}
}
SourceRuntimeHostPhysicalMemory::SourceRuntimeHostPhysicalMemory(SourceRuntimeMemory &memory)
    : memory_(memory) {
  (void)source_runtime_detail::physical_pages::native_alignment();
  auto state=std::make_shared<PhysicalProtection>();
  {
    std::lock_guard lock(physical_protection_mutex);physical_protections.emplace(this,state);
  }
  try {
    const auto *reference=std::getenv("HALO2_REFERENCE_ALL_PHYSICAL_WRITE_GUARDS");
    const bool all_ranges=reference && *reference;
    const auto guard=[state,all_ranges](std::uint32_t address,std::size_t bytes) {
      const auto end=std::uint64_t(address)+bytes;
      // allocate() publishes only upper-half physical tokens; restored
      // ownership enforces the same namespace. Ordinary writes ending at
      // that boundary cannot intersect any physical protection row. Keep
      // spanning writes, native aliases and every physical write checked.
      if(!all_ranges && end<=0x80000000ull)return;
      // A non-empty write starting inside the namespace reads the page
      // mirror; spanning and empty writes keep the checked set under the mutex.
      if(address>=0x80000000u && bytes) {
        if(state->protected_in(address,end))
          throw std::runtime_error("Original runtime write to read-only physical page");
        return;
      }
      std::lock_guard lock(state->mutex);
      const auto first=state->read_only.lower_bound(address&~(page-1));
      if(first!=state->read_only.end() && *first<end)
        throw std::runtime_error("Original runtime write to read-only physical page");
    };
    // The same boundary lets dispatch skip ordinary low writes without a call.
    if(all_ranges)source_runtime_register_write_guard(memory_,this,guard);
    else source_runtime_register_write_guard(memory_,this,guard,0x80000000ull);
  } catch(...) {
    std::lock_guard lock(physical_protection_mutex);physical_protections.erase(this);throw;
  }
}
SourceRuntimeHostPhysicalMemory::~SourceRuntimeHostPhysicalMemory() {
  source_runtime_unregister_write_guard(memory_,this);
  {std::lock_guard lock(physical_protection_mutex);physical_protections.erase(this);}
  for (const auto &[base, allocation] : allocations_) {
    try { if(memory_.has_backing(base,allocation.backing))memory_.release(base); } catch (...) {}
  }
}
std::uint32_t SourceRuntimeHostPhysicalMemory::allocate(SourcePhysicalAllocationRequest q) {
  // Both reached SDK requests own writable native pages. As in protect(),
  // anonymous host storage retains the host cache policy; GPU coherency is
  // a separate contract rather than a successful graphics result.
  if (!q.bytes || (q.protection != 4 && q.protection != 0x404) || q.lowest > q.highest ||
      (q.highest >= 0x80000000u && q.highest != UINT32_MAX) ||
      q.lowest >= 0x80000000u || (q.alignment && (q.alignment & (q.alignment - 1))))
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
  // The original SDK also requests an unrestricted highest address. Keep the
  // native physical-token namespace bounded even when that sentinel is used.
  if (physical + bytes > std::min(std::uint64_t(q.highest) + 1,
                                 std::uint64_t{0x80000000u}) || bytes > UINT32_MAX)
    return 0; // Actual native resource namespace exhaustion.
  const auto token = std::uint32_t(physical) | 0x80000000u;
  auto backing=physical_backing(std::uint32_t(bytes));
  memory_.mount(token,backing,
        SourceRuntimeMemory::Access::ReadWrite, allocation_owner);
  try { allocations_.emplace(token, Allocation{std::move(backing), std::uint32_t(bytes)}); }
  catch (...) { memory_.release(token); throw; }
  return token;
}
void SourceRuntimeHostPhysicalMemory::release(std::uint32_t address) {
  const auto state=protection_of(this);
  std::unique_lock lock(state->mutex,std::try_to_lock);
  if(!lock.owns_lock())
    throw std::runtime_error("Native physical release cannot cross staged ownership or protection");
  const auto found=allocations_.find(address);
  if(found==allocations_.end() || !memory_.has_backing(address,found->second.backing))
    throw std::invalid_argument("Native physical release requires its exact unchanged owned backing");
  const auto end=std::uint64_t(address)+found->second.bytes;
  memory_.release(address);
  const auto first=state->read_only.lower_bound(address);
  auto last=first;while(last!=state->read_only.end() && *last<end)++last;
  state->erase_range(first,last);
  allocations_.erase(found);
}
void SourceRuntimeHostPhysicalMemory::protect(std::uint32_t address,
                                              std::uint32_t bytes,
                                              std::uint32_t protection) {
  // Actual TU1.5 12ED5E requests PAGE_READWRITE | PAGE_WRITECOMBINE.
  // This native process uses anonymous host pages and the host cache policy;
  // Source guards and platform page protections own read/write access. Xbox GPU cache coherency is
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
  // sized. Preserve the source request and its exact4KiB permission rows;
  // the platform owner applies them to the actual allocation's host granules.
  const auto protected_begin = offset & ~std::uint64_t(page - 1);
  const auto protected_end = (offset + bytes + page - 1) & ~std::uint64_t(page - 1);
  const auto state=protection_of(this);std::lock_guard lock(state->mutex);
  auto next=state->read_only;
  for(auto p=std::uint64_t(found->first)+protected_begin;
      p<std::uint64_t(found->first)+protected_end;p+=page) {
    if(protection==2)next.insert(std::uint32_t(p));else next.erase(std::uint32_t(p));
  }
  source_runtime_detail::physical_pages::protect_range(found->second.backing->bytes(),
      found->first,protected_begin,protected_end,protection==2,state->read_only,next);
  state->replace(next);
}
struct SourceRuntimeHostPhysicalMemory::PreparedOwnership::Impl {
  const SourceRuntimeHostPhysicalMemory *owner;
  std::shared_ptr<PhysicalProtection> protection;
  // Acquire before staging. Publication only swaps already allocated maps;
  // it cannot fail while trying to acquire a protection lock after bank import.
  std::unique_lock<std::mutex> lock;
  std::map<std::uint32_t,Allocation> allocations;
  std::set<std::uint32_t> read_only;
  std::shared_ptr<StagedPhysicalProtection> staged_protection=std::make_shared<StagedPhysicalProtection>();
  Impl(const SourceRuntimeHostPhysicalMemory *owner,std::shared_ptr<PhysicalProtection> state)
      :owner(owner),protection(std::move(state)),lock(protection->mutex) {}
};
SourceRuntimeHostPhysicalMemory::PreparedOwnership::PreparedOwnership()=default;
SourceRuntimeHostPhysicalMemory::PreparedOwnership::~PreparedOwnership()=default;
SourceRuntimeHostPhysicalMemory::PreparedOwnership::PreparedOwnership(PreparedOwnership &&) noexcept=default;
SourceRuntimeHostPhysicalMemory::PreparedOwnership &SourceRuntimeHostPhysicalMemory::PreparedOwnership::operator=(PreparedOwnership &&) noexcept=default;
bool SourceRuntimeHostPhysicalMemory::PreparedOwnership::for_owner(const SourceRuntimeHostPhysicalMemory &owner) const noexcept {
  return impl_ && impl_->owner==&owner;
}
bool SourceRuntimeHostPhysicalMemory::PreparedOwnership::matches_memory(const SourceRuntimeMemory &bank) const noexcept {
  if(!impl_)return false;
  for(const auto &[base,allocation]:impl_->allocations)
    if(!bank.has_backing(base,allocation.backing))return false;
  return true;
}
SourceRuntimeHostPhysicalMemory::OwnershipCheckpoint SourceRuntimeHostPhysicalMemory::ownership_checkpoint() const {
  const auto state=protection_of(this);std::lock_guard lock(state->mutex);
  OwnershipCheckpoint saved;saved.allocations.reserve(allocations_.size());
  for(const auto &[base,allocation]:allocations_) {
    AllocationCheckpoint copy{base,allocation.bytes,{}};copy.read_only.reserve(allocation.bytes/page);
    for(std::uint64_t at=base;at<std::uint64_t(base)+allocation.bytes;at+=page)
      copy.read_only.push_back(state->read_only.contains(std::uint32_t(at))?1:0);
    saved.allocations.push_back(std::move(copy));
  }
  return saved;
}
SourceRuntimeHostPhysicalMemory::PreparedOwnership SourceRuntimeHostPhysicalMemory::prepare_ownership(
    const OwnershipCheckpoint &saved,const SourceRuntimeMemory::Checkpoint &bank,
    SourceRuntimeMemory &replacement) const {
  if(&replacement==&memory_)throw std::invalid_argument("Physical staging requires a separate quiescent memory bank");
  SourceRuntimeMemory::validate_checkpoint(bank);
  if(saved.version!=1 || std::count_if(bank.regions.begin(),bank.regions.end(),
      [](const auto &region){return region.owner==allocation_owner;})!=std::ptrdiff_t(saved.allocations.size()))
    throw std::invalid_argument("Physical ownership differs from the incoming source bank");
  PreparedOwnership prepared;prepared.impl_=std::make_unique<PreparedOwnership::Impl>(this,protection_of(this));
  std::uint64_t previous_end{};
  for(const auto &allocation:saved.allocations) {
    const auto limit=std::uint64_t(allocation.base)+allocation.bytes;
    if(!(allocation.base&0x80000000u) || allocation.base%page || !allocation.bytes || allocation.bytes%page ||
        allocation.base<previous_end || limit>(std::uint64_t{1}<<32) ||
        allocation.read_only.size()!=allocation.bytes/page ||
        std::any_of(allocation.read_only.begin(),allocation.read_only.end(),[](auto value){return value>1;}))
      throw std::invalid_argument("Invalid native physical allocation checkpoint");
    previous_end=limit;
    const auto region=std::lower_bound(bank.regions.begin(),bank.regions.end(),allocation.base,
        [](const auto &region,auto address){return region.address<address;});
    if(region==bank.regions.end() || region->address!=allocation.base || region->owner!=allocation_owner ||
        region->bytes.size()!=allocation.bytes || region->access!=SourceRuntimeMemory::Access::ReadWrite ||
        region->page_bytes || !region->committed.empty())
      throw std::invalid_argument("Physical allocation differs from the incoming source bank");
    const auto policy=prepared.impl_->staged_protection;
    const auto guard=[policy](auto p,auto n){policy->validate(p,n);};
    auto backing=replacement.prepare_snapshot_backing(*region,
        source_runtime_detail::physical_pages::native_alignment(),guard);
    const bool retained=bool(backing);
    if(!backing)backing=physical_backing(allocation.bytes,guard);
    auto *host=backing->bytes().data();
    prepared.impl_->allocations.emplace(allocation.base,Allocation{backing,allocation.bytes});
    if(retained) {
      // A canceled preparation may have protected these same staging pages.
      // Reset the real mapping before applying this incoming protection set.
      source_runtime_detail::physical_pages::reset(backing->bytes());
    } else source_runtime_detail::restore_zero_filled_pages(
        std::span(host,allocation.bytes),std::span(region->bytes),page);
    for(std::size_t index=0;index<allocation.read_only.size();++index) {
      if(allocation.read_only[index]) {
        const auto address=allocation.base+std::uint32_t(index*page);
        prepared.impl_->read_only.insert(address);
        policy->read_only.insert(address);
      }
    }
    source_runtime_detail::physical_pages::restore_protection(backing->bytes(),allocation.base,allocation.read_only);
    replacement.bind_backing(*region,std::move(backing));
  }
  return prepared;
}
bool SourceRuntimeHostPhysicalMemory::commit_ownership(PreparedOwnership &&prepared) noexcept {
  if(!prepared.for_owner(*this) || !prepared.matches_memory(memory_))return false;
  allocations_.swap(prepared.impl_->allocations);
  prepared.impl_->protection->replace(prepared.impl_->read_only);
  prepared.impl_->staged_protection->active=false;
  prepared.impl_->staged_protection->read_only.clear();
  prepared.impl_.reset();return true;
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
