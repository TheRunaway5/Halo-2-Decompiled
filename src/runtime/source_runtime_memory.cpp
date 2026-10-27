#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <vector>
#include <map>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

namespace halo2 {
SourceRuntimeMemoryBacking::SourceRuntimeMemoryBacking(std::shared_ptr<void> lifetime,
    std::span<std::byte> bytes, WriteGuard guard)
    :lifetime_(std::move(lifetime)),bytes_(bytes),write_guard_(std::move(guard)) {
  if(!lifetime_ || bytes_.empty())
    throw std::invalid_argument("Source backing requires its actual allocation lifetime and bytes");
}
SourceRuntimeMemory::SourceRuntimeMemory(SourceRuntimeMemory &&other) {
  // A pre-retained alias can move the real bank without going through World.
  // Check both owner identities before either backing or receipt is changed.
  source_runtime_validate_topology(*this,0,0);source_runtime_validate_topology(other,0,0);
  source_runtime_observe_topology(*this,0,0);source_runtime_observe_topology(other,0,0);
  regions_=std::move(other.regions_);native_regions_=std::move(other.native_regions_);
  source_pages_=std::move(other.source_pages_);small_pages_=std::move(other.small_pages_);direct_index_=std::move(other.direct_index_);
  publish_direct_index();other.publish_direct_index();
}
SourceRuntimeMemory &SourceRuntimeMemory::operator=(SourceRuntimeMemory &&other) {
  if(this==&other)return *this;
  source_runtime_validate_topology(*this,0,0);source_runtime_validate_topology(other,0,0);
  source_runtime_observe_topology(*this,0,0);source_runtime_observe_topology(other,0,0);
  regions_=std::move(other.regions_);native_regions_=std::move(other.native_regions_);
  source_pages_=std::move(other.source_pages_);small_pages_=std::move(other.small_pages_);direct_index_=std::move(other.direct_index_);
  publish_direct_index();other.publish_direct_index();
  return *this;
}
void SourceRuntimeMemory::swap_backing(SourceRuntimeMemory &other) {
  if(this==&other)return;
  source_runtime_validate_topology(*this,0,0);source_runtime_validate_topology(other,0,0);
  source_runtime_observe_topology(*this,0,0);source_runtime_observe_topology(other,0,0);
  regions_.swap(other.regions_);native_regions_.swap(other.native_regions_);
  source_pages_.swap(other.source_pages_);small_pages_.swap(other.small_pages_);direct_index_.swap(other.direct_index_);
  publish_direct_index();other.publish_direct_index();
}
namespace {
const bool reference_small_transfers=[] {
  const auto *p=std::getenv("HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS");return p && *p;
}();
const bool reference_scalar_vector_reads=[] {
  const auto *p=std::getenv("HALO2_REFERENCE_SCALAR_VECTOR_READS");return p && *p;
}();
// Reference transfer modes keep every access on the original checked route.
const bool direct_pages_enabled=!reference_small_transfers && !reference_scalar_vector_reads;
inline void transfer(void *destination,const void *source,std::size_t bytes) {
  // Generated scalar instructions overwhelmingly transfer four bytes. Keep
  // memmove's complete overlap contract, with fixed extents the compiler can
  // lower to unaligned loads/stores instead of a variable-size library call.
  if(!reference_small_transfers) {
    if(bytes==4){std::memmove(destination,source,4);return;}
    if(bytes==8){std::memmove(destination,source,8);return;}
    if(bytes==2){std::memmove(destination,source,2);return;}
    if(bytes==1){std::memmove(destination,source,1);return;}
  }
  std::memmove(destination,source,bytes);
}
struct RestoredSnapshotDelete {
  void operator()(SourceRuntimeSnapshotBytes *bytes) const noexcept { delete bytes; }
};
std::uint64_t end(std::uint32_t p, std::size_t n) {
  if (n > (std::uint64_t{1} << 32) - p)
    throw std::out_of_range("Original runtime memory extent wraps32 bits");
  return std::uint64_t(p) + n;
}
[[noreturn]] void missing(std::uint32_t p) {
  std::ostringstream message;
  message << "Required original runtime backing unavailable at0x" << std::hex
          << p;
  throw std::out_of_range(message.str());
}
template<class Position>
void validate_committed(Position found,std::uint64_t current,std::uint64_t limit) {
  const auto &region=found->second;
  if(region.page_bytes && current<limit) {
    const auto first=(current-found->first)/region.page_bytes;
    const auto last=(limit-1-found->first)/region.page_bytes;
    for(auto page=first;page<=last;++page)
      if(!region.committed[page])
        missing(found->first+std::uint32_t(page*region.page_bytes));
  }
}
} // namespace
void SourceRuntimeMemory::mount(std::uint32_t p,
                                std::span<const std::byte> initial,
                                Access access, std::string owner) {
  if (initial.empty() || owner.empty())
    throw std::invalid_argument(
        "Original runtime region requires backing and an owner");
  check_extent(p, initial.size());
  source_runtime_validate_topology(*this,p,initial.size());
  source_runtime_observe_topology(*this,p,initial.size());
  regions_.emplace(
      p, Region{std::vector<std::byte>(initial.begin(), initial.end()), access, std::move(owner)});
  bool native_indexed=false;
  try{index_native_region(p);native_indexed=true;index_source_region(regions_.find(p));}
  catch(...){unindex_source_region(regions_.find(p));if(native_indexed)native_regions_.erase(reinterpret_cast<std::uintptr_t>(regions_.at(p).storage().data()));regions_.erase(p);throw;}
}
void SourceRuntimeMemory::mount(std::uint32_t p,
    std::shared_ptr<SourceRuntimeMemoryBacking> backing, Access access, std::string owner) {
  if(!backing || owner.empty())throw std::invalid_argument("Shared source region requires backing and an owner");
  check_extent(p,backing->bytes().size());
  source_runtime_validate_topology(*this,p,backing->bytes().size());
  if(backing->mounted_.exchange(true))throw std::invalid_argument("Source backing is already published by a region");
  source_runtime_observe_topology(*this,p,backing->bytes().size());
  try {
    regions_.emplace(p,Region{backing,access,std::move(owner)});
    bool native_indexed=false;
    try{index_native_region(p);native_indexed=true;index_source_region(regions_.find(p));}
    catch(...){unindex_source_region(regions_.find(p));if(native_indexed)native_regions_.erase(reinterpret_cast<std::uintptr_t>(regions_.at(p).storage().data()));regions_.erase(p);throw;}
  }catch(...){backing->mounted_.store(false);throw;}
}
void SourceRuntimeMemory::bind_backing(const CheckpointRegion &expected,
    std::shared_ptr<SourceRuntimeMemoryBacking> backing) {
  const auto p=expected.address;
  auto found=regions_.find(p);
  if(found==regions_.end() || !backing || expected.page_bytes || !expected.committed.empty() ||
      found->second.page_bytes || !found->second.committed.empty() ||
      found->second.owner!=expected.owner || found->second.access!=expected.access ||
      backing->bytes().size()!=expected.bytes.size() ||
      !std::ranges::equal(found->second.storage(),expected.bytes) ||
      !std::ranges::equal(backing->bytes(),expected.bytes))
    throw std::invalid_argument("Prepared source backing differs from its complete staged region");
  auto &region=found->second;
  if(region.shared==backing)return;
  source_runtime_validate_topology(*this,p,expected.bytes.size());
  if(backing->mounted_.exchange(true))throw std::invalid_argument("Prepared source backing is already published");
  try{index_native_span(p,backing->bytes());}catch(...){backing->mounted_.store(false);throw;}
  source_runtime_observe_topology(*this,p,expected.bytes.size());
  native_regions_.erase(reinterpret_cast<std::uintptr_t>(region.storage().data()));
  if(region.shared)region.shared->mounted_.store(false);
  region.shared=std::move(backing);
  std::vector<std::byte>().swap(region.bytes);
  refresh_direct_region(found);
}
bool SourceRuntimeMemory::has_backing(std::uint32_t p,
    const std::shared_ptr<SourceRuntimeMemoryBacking> &backing) const noexcept {
  const auto found=regions_.find(p);
  return backing && found!=regions_.end() && found->second.shared==backing;
}
std::shared_ptr<SourceRuntimeMemoryBacking> SourceRuntimeMemory::prepare_snapshot_backing(
    const CheckpointRegion &expected,std::size_t alignment,
    SourceRuntimeMemoryBacking::WriteGuard guard) {
  if(!alignment || (alignment&(alignment-1)))
    throw std::invalid_argument("Snapshot backing alignment must be a power of two");
  const auto found=regions_.find(expected.address);
  if(found==regions_.end() || !found->second.shared || found->second.shared.use_count()!=1)
    return {};
  auto &backing=found->second.shared;
  // Only this owner's restored byte containers may become platform mappings.
  // A vector-backed buffer is not safe to protect, even if malloc happened to
  // align its address: freeing it can write allocator metadata into its pages.
  if(!std::get_deleter<RestoredSnapshotDelete>(backing->lifetime_) ||
      !static_cast<SourceRuntimeSnapshotBytes*>(backing->lifetime_.get())->mapped_ ||
      reinterpret_cast<std::uintptr_t>(backing->bytes().data())%alignment ||
      backing->bytes().size()%alignment)
    return {};
  // Same-backing bind is intentionally a topology no-op; replacing its access
  // policy still requires admission through destruction of the old captures.
  SourceRuntimePolicyChange policy(*this);
  bind_backing(expected,backing); // Full admission before changing a guard.
  backing->write_guard_=std::move(guard);
  refresh_direct_region(found);
  return backing;
}
void SourceRuntimeMemory::check_extent(std::uint32_t p, std::size_t bytes) const {
  const auto limit = end(p, bytes);
  const auto next = regions_.lower_bound(p);
  const auto overlap=[&](const auto &region) {
    std::ostringstream message;
    message<<"Original runtime regions overlap: requested [0x"<<std::hex<<p<<",0x"<<limit
        <<") existing [0x"<<region.first<<",0x"
        <<std::uint64_t(region.first)+region.second.storage().size()<<") owner="<<region.second.owner;
    return std::invalid_argument(message.str());
  };
  if (next != regions_.end() && next->first < limit)
    throw overlap(*next);
  if (next != regions_.begin()) {
    const auto previous = std::prev(next);
    if (std::uint64_t(previous->first) + previous->second.storage().size() > p)
      throw overlap(*previous);
  }
}
void SourceRuntimeMemory::reserve(std::uint32_t p, std::size_t bytes,
                                  std::size_t page_bytes, std::string owner) {
  if (!bytes || !page_bytes || p % page_bytes || bytes % page_bytes ||
      owner.empty())
    throw std::invalid_argument("Original reservation requires aligned pages and an owner");
  check_extent(p, bytes);
  source_runtime_validate_topology(*this,p,bytes);
  source_runtime_observe_topology(*this,p,bytes);
  regions_.emplace(p, Region{std::vector<std::byte>(bytes), Access::ReadWrite,
                             std::move(owner), page_bytes,
                             std::vector<bool>(bytes / page_bytes, false)});
  bool native_indexed=false;
  try{index_native_region(p);native_indexed=true;index_source_region(regions_.find(p));}
  catch(...){unindex_source_region(regions_.find(p));if(native_indexed)native_regions_.erase(reinterpret_cast<std::uintptr_t>(regions_.at(p).storage().data()));regions_.erase(p);throw;}
}
void SourceRuntimeMemory::commit(std::uint32_t p,
                                 std::span<const std::byte> initial) {
  end(p, initial.size());
  const auto found = locate(p);
  auto &region = regions_.at(found->first);
  const auto offset = std::size_t(p - found->first);
  if (!region.page_bytes || initial.empty() || offset % region.page_bytes ||
      initial.size() % region.page_bytes ||
      initial.size() > region.storage().size() - offset)
    throw std::invalid_argument("Original commit must fit aligned reserved pages");
  source_runtime_validate_topology(*this,p,initial.size());
  const std::vector<std::byte> staged(initial.begin(), initial.end());
  source_runtime_observe_topology(*this,p,initial.size());
  for (std::size_t done = 0; done < staged.size(); done += region.page_bytes) {
    const auto page = (offset + done) / region.page_bytes;
    if (!region.committed[page]) {
      std::memcpy(region.storage().data() + offset + done, staged.data() + done,
                  region.page_bytes);
      region.committed[page] = true;
    }
  }
  refresh_direct_region(found);
}
void SourceRuntimeMemory::release(std::uint32_t p) {
  const auto found=regions_.find(p);
  if(found==regions_.end())
    throw std::out_of_range("Original runtime release has no region owner");
  source_runtime_validate_topology(*this,p,found->second.storage().size());
  source_runtime_observe_topology(*this,p,found->second.storage().size());
  unindex_source_region(found);
  native_regions_.erase(reinterpret_cast<std::uintptr_t>(found->second.storage().data()));
  regions_.erase(found);
}
void SourceRuntimeMemory::visit_committed(const std::function<void(std::uint32_t,
    std::span<const std::byte>)>&visit) const {
  for(const auto&[address,region]:regions_) {
    if(!region.page_bytes) {visit(address,region.storage());continue;}
    for(std::size_t page=0;page<region.committed.size();) {
      if(!region.committed[page]) {++page;continue;}
      const auto first=page;
      while(page<region.committed.size() && region.committed[page])++page;
      const auto offset=first*region.page_bytes,bytes=(page-first)*region.page_bytes;
      visit(address+std::uint32_t(offset),region.storage().subspan(offset,bytes));
    }
  }
}
SourceRuntimeMemory::Checkpoint SourceRuntimeMemory::checkpoint() const {
  Checkpoint result;
  result.regions.reserve(regions_.size());
  for(const auto &[address,region]:regions_) {
    const auto bytes=region.storage();
    CheckpointRegion copy{address,region.access,region.owner,region.page_bytes,{bytes.begin(),bytes.end()},{}};
    copy.committed.reserve(region.committed.size());
    for(bool page:region.committed)copy.committed.push_back(page?1:0);
    result.regions.push_back(std::move(copy));
  }
  return result;
}
bool SourceRuntimeMemory::matches_checkpoint_region(const CheckpointRegion &saved) const noexcept {
  const auto found=regions_.find(saved.address);
  if(found==regions_.end())return false;
  const auto &region=found->second;
  return region.access==saved.access && region.owner==saved.owner &&
      region.page_bytes==saved.page_bytes &&
      std::ranges::equal(region.committed,saved.committed) &&
      std::ranges::equal(region.storage(),saved.bytes);
}
bool SourceRuntimeMemory::matches_read_only_checkpoint(const Checkpoint &saved) const noexcept {
  if(saved.version!=1)return false;
  auto live=regions_.begin();
  for(const auto &incoming:saved.regions) {
    if(incoming.access!=Access::ReadOnly)continue;
    while(live!=regions_.end() && live->second.access!=Access::ReadOnly)++live;
    if(live==regions_.end())return false;
    const auto &[address,region]=*live;
    const auto bytes=region.storage();
    if(address!=incoming.address || region.access!=incoming.access ||
        region.owner!=incoming.owner || region.page_bytes!=incoming.page_bytes ||
        region.committed.size()!=incoming.committed.size() ||
        !std::ranges::equal(region.committed,incoming.committed) ||
        !std::ranges::equal(bytes,incoming.bytes))
      return false;
    ++live;
  }
  while(live!=regions_.end() && live->second.access!=Access::ReadOnly)++live;
  return live==regions_.end();
}
void SourceRuntimeMemory::validate_checkpoint(const Checkpoint &saved) {
  if(saved.version!=1)throw std::invalid_argument("Unsupported source memory checkpoint version");
  std::uint64_t previous_end{};
  for(const auto &region:saved.regions) {
    if(region.bytes.empty() || region.owner.empty() || region.address<previous_end ||
        (region.access!=Access::ReadOnly && region.access!=Access::ReadWrite))
      throw std::invalid_argument("Invalid source memory checkpoint region");
    previous_end=end(region.address,region.bytes.size());
    if(!region.page_bytes) {
      if(!region.committed.empty())
        throw std::invalid_argument("Mounted checkpoint region has reservation pages");
    } else {
      if(region.access!=Access::ReadWrite || region.address%region.page_bytes ||
          region.bytes.size()%region.page_bytes ||
          region.committed.size()!=region.bytes.size()/region.page_bytes ||
          std::any_of(region.committed.begin(),region.committed.end(),[](auto page){return page>1;}))
        throw std::invalid_argument("Invalid source memory checkpoint commitment map");
    }
  }
}
void SourceRuntimeMemory::restore(const Checkpoint &saved) {
  validate_checkpoint(saved);
  source_runtime_validate_topology(*this,0,0);
  SourceRuntimeMemory replacement;
  for(const auto &region:saved.regions) {
    auto bytes=std::shared_ptr<SourceRuntimeSnapshotBytes>(
        new SourceRuntimeSnapshotBytes(region.bytes),RestoredSnapshotDelete{});
    auto backing=std::make_shared<SourceRuntimeMemoryBacking>(bytes,std::span(*bytes));
    replacement.mount(region.address,std::move(backing),region.access,region.owner);
    auto &restored=replacement.regions_.at(region.address);
    restored.page_bytes=region.page_bytes;
    restored.committed.reserve(region.committed.size());
    for(auto page:region.committed)restored.committed.push_back(page!=0);
    replacement.refresh_direct_region(replacement.regions_.find(region.address));
  }
  replacement.rebuild_native_index();
  source_runtime_observe_topology(*this,0,0);
  regions_.swap(replacement.regions_);native_regions_.swap(replacement.native_regions_);
  source_pages_.swap(replacement.source_pages_);small_pages_.swap(replacement.small_pages_);direct_index_.swap(replacement.direct_index_);
  publish_direct_index();replacement.publish_direct_index();
}
void SourceRuntimeMemory::index_source_region(Position found) {
  const auto base=std::uint64_t(found->first),size=found->second.storage().size();
  const auto first=(base+4095)&~std::uint64_t{4095};
  const auto limit=(base+size)&~std::uint64_t{4095};
  if(first<limit) {
    if(!source_pages_)source_pages_=std::make_unique<SourcePageIndex>();
    for(auto page=first>>12;page<limit>>12;++page) {
      auto &block=(*source_pages_)[page>>10];
      if(!block)block=std::make_unique<SourcePageBlock>();
      (*block)[page&1023]=found;
    }
  }
  // Pages this region covers only partly.
  for(auto page=base>>12;page<=(base+size-1)>>12;++page) {
    if(first<limit && page>=(first>>12) && page<(limit>>12))continue;
    if(!small_pages_)small_pages_=std::make_unique<SourceSmallIndex>();
    auto &block=(*small_pages_)[page>>10];
    if(!block)block=std::make_unique<SourceSmallBlock>();
    auto &entries=(*block)[page&1023];
    const auto at=std::upper_bound(entries.begin(),entries.end(),found->first,
        [](std::uint32_t b,const SourceSmallEntry &e){return b<e.base;});
    entries.insert(at,SourceSmallEntry{found->first,found});
  }
  if(first<limit)refresh_direct_region(found);
}
void SourceRuntimeMemory::unindex_source_region(Position found) noexcept {
  const auto base=std::uint64_t(found->first),size=found->second.storage().size();
  if(small_pages_)
    for(auto page=base>>12;page<=(base+size-1)>>12;++page)
      if(auto &block=(*small_pages_)[page>>10])
        std::erase_if((*block)[page&1023],[&](const SourceSmallEntry &e){return e.found==found;});
  if(!source_pages_)return;
  const auto first=(base+4095)&~std::uint64_t{4095};
  const auto limit=(base+size)&~std::uint64_t{4095};
  for(auto page=first>>12;page<limit>>12;++page) {
    if(auto &block=(*source_pages_)[page>>10])(*block)[page&1023]=Position{};
    if(direct_index_)if(auto &block=direct_index_->blocks[page>>10])(*block)[page&1023]={};
  }
}
std::uint64_t SourceRuntimeMemory::restore_storage_overhead(const Checkpoint &saved) {
  validate_checkpoint(saved);
  std::array<bool,1024> full{},partial{};std::uint64_t edges{};
  for(const auto &region:saved.regions) {
    const auto base=std::uint64_t(region.address),size=std::uint64_t(region.bytes.size());
    const auto first=(base+4095)&~std::uint64_t{4095},limit=(base+size)&~std::uint64_t{4095};
    if(first<limit)for(auto block=(first>>22);block<=((limit-1)>>22);++block)full[block]=true;
    const auto begin=base>>12,last=(base+size-1)>>12;
    for(auto page:std::array{begin,last}) {
      if(first<limit&&page>=(first>>12)&&page<(limit>>12))continue;
      // A single partial page is deliberately counted twice for growth slack.
      partial[page>>10]=true;++edges;
    }
  }
  std::uint64_t bytes=sizeof(SourcePageIndex)+sizeof(SourceSmallIndex)+sizeof(SourceDirectPageIndex);
  for(unsigned block=0;block<1024;++block){
    if(full[block])bytes+=sizeof(SourcePageBlock)+sizeof(SourceDirectPageIndex::Block);
    if(partial[block])bytes+=sizeof(SourceSmallBlock);
  }
  // Two red-black maps, shared snapshot/backing objects/control nodes, and
  // one page of alignment slack for mapped snapshot storage.
  bytes+=saved.regions.size()*(sizeof(Region)+sizeof(SourceRuntimeSnapshotBytes)+sizeof(SourceRuntimeMemoryBacking)+512+4096);
  bytes+=2*edges*sizeof(SourceSmallEntry);return bytes;
}
SourceRuntimeMemory::Position SourceRuntimeMemory::small_lookup(std::uint32_t p) const noexcept {
  if(!small_pages_)return {};
  const auto &block=(*small_pages_)[p>>22];
  if(!block)return {};
  const auto &entries=(*block)[(p>>12)&1023];
  auto at=std::upper_bound(entries.begin(),entries.end(),p,
      [](std::uint32_t a,const SourceSmallEntry &e){return a<e.base;});
  if(at==entries.begin())return {};
  --at;
  if(std::uint64_t(p)-at->base>=at->found->second.storage().size())return {};
  return at->found;
}
void SourceRuntimeMemory::publish_direct_index() noexcept {
  direct_pages_=direct_pages_enabled?direct_index_.get():nullptr;
}
void SourceRuntimeMemory::refresh_direct_region(Position found) {
  if(!direct_pages_enabled)return;
  const auto base=std::uint64_t(found->first);
  const auto &region=found->second;
  const auto first=(base+4095)&~std::uint64_t{4095};
  const auto limit=(base+region.storage().size())&~std::uint64_t{4095};
  if(first>=limit)return;
  if(!direct_index_){direct_index_=std::make_unique<SourceDirectPageIndex>();publish_direct_index();}
  // Allocate before publishing, so a failure cannot leave a stale backing.
  for(auto page=first>>12;page<limit>>12;++page)
    if(auto &block=direct_index_->blocks[page>>10];!block)block=std::make_unique<SourceDirectPageIndex::Block>();
  const bool writable=region.access==Access::ReadWrite && !(region.shared && region.shared->write_guard_);
  // The owner's bank is mutable; Position is only a const map iterator.
  auto *const bytes=const_cast<std::byte*>(region.storage().data());
  for(auto page=first>>12;page<limit>>12;++page) {
    const auto offset=(page<<12)-base;
    bool committed=true;
    if(region.page_bytes)
      for(auto commit=offset/region.page_bytes;commit<=(offset+4095)/region.page_bytes;++commit)
        if(!region.committed[commit]){committed=false;break;}
    (*direct_index_->blocks[page>>10])[page&1023]=committed?
        SourceDirectPageIndex::Page{bytes+offset,writable}:SourceDirectPageIndex::Page{};
  }
}
namespace {
// HALO2_LOCATE_CENSUS=1: which 1MiB regions still reach the slow locate path
// (direct page index missed), fast page-table hits vs region-map lookups.
struct LocateCensus {
  const bool enabled=[]{const auto *p=std::getenv("HALO2_LOCATE_CENSUS");return p && *p;}();
  std::map<std::uint32_t,std::pair<std::uint64_t,std::uint64_t>> regions;
  ~LocateCensus() {
    if(!enabled || regions.empty())return;
    std::vector<std::pair<std::uint32_t,std::pair<std::uint64_t,std::uint64_t>>> top(regions.begin(),regions.end());
    std::sort(top.begin(),top.end(),[](const auto &a,const auto &b){return a.second.first+a.second.second>b.second.first+b.second.second;});
    std::uint64_t total=0;for(const auto &r:top)total+=r.second.first+r.second.second;
    std::fprintf(stderr,"Locate census: total=%llu\n",(unsigned long long)total);
    for(std::size_t i=0;i<top.size() && i<16;++i)
      std::fprintf(stderr,"  region %08x: table=%llu map=%llu\n",top[i].first<<20,(unsigned long long)top[i].second.first,(unsigned long long)top[i].second.second);
  }
} locate_census;
}
SourceRuntimeMemory::Position
SourceRuntimeMemory::locate(std::uint32_t p) const {
  if(source_pages_)
    if(const auto &block=(*source_pages_)[p>>22])
      if(const auto found=(*block)[(p>>12)&1023];found!=Position{}){if(locate_census.enabled)++locate_census.regions[p>>20].first;return found;}
  if(const auto small=small_lookup(p);small!=Position{}){if(locate_census.enabled)++locate_census.regions[p>>20].first;return small;}
  if(locate_census.enabled)++locate_census.regions[p>>20].second;
  auto found = regions_.upper_bound(p);
  if(locate_census.enabled && found!=regions_.begin()) {
    // Describe the enclosing region of the first few map-path addresses per 1MiB.
    static std::map<std::uint32_t,unsigned> shown;
    if(shown[p>>20]++<1) {
      auto it=std::prev(found);const auto &r=it->second;
      std::size_t committed=0;for(const auto c:r.committed)committed+=c?1:0;
      std::fprintf(stderr,"Locate map-path %08x: region %08x size=%zu access=%d page_bytes=%zu committed=%zu/%zu shared=%d guard=%d owner=%s\n",
        p,it->first,r.storage().size(),int(r.access),std::size_t(r.page_bytes),committed,r.committed.size(),int(bool(r.shared)),
        int(r.shared && r.shared->write_guard_),r.owner.c_str());
    }
  }
  if (found == regions_.begin())
    missing(p);
  --found;
  if (std::uint64_t(p) - found->first >= found->second.storage().size())
    missing(p);
  return found;
}
void SourceRuntimeMemory::validate(std::uint32_t p, std::size_t n,
                                   bool writable) const {
  validate(p,n,writable,true);
}
void SourceRuntimeMemory::validate(std::uint32_t p,std::size_t n,bool writable,bool observe) const {
  const auto limit = end(p, n);
  auto current = std::uint64_t(p);
  while (current < limit) {
    const auto found = locate(std::uint32_t(current));
    if (writable && found->second.access != Access::ReadWrite)
      throw std::runtime_error("Original runtime write to read-only owner " +
                               found->second.owner);
    const auto &region = found->second;
    const auto region_end = std::min(limit, std::uint64_t(found->first) +
                                               region.storage().size());
    validate_committed(found,current,region_end);
    if(writable && region.shared && region.shared->write_guard_)
      region.shared->write_guard_(std::uint32_t(current),std::size_t(region_end-current));
    current = region_end;
  }
  if(writable)source_runtime_validate_write(*this,p,n,observe);
}
bool SourceRuntimeMemory::is_read_only(std::uint32_t p,std::size_t n) const {
  validate(p,n,false);const auto found=locate(p);
  if(n>found->second.storage().size()-std::size_t(p-found->first))throw std::out_of_range("Read-only resource query spans region owners");
  return found->second.access==Access::ReadOnly;
}
std::optional<std::uint32_t> SourceRuntimeMemory::find_unowned_range(
    std::uint32_t first, std::uint64_t limit, std::size_t bytes,
    std::uint32_t alignment) const {
  if (!bytes || !alignment || (alignment & (alignment - 1)) ||
      limit > (std::uint64_t{1} << 32) || first >= limit)
    throw std::invalid_argument("Original runtime gap query has invalid extent/alignment");
  const auto align = [alignment](std::uint64_t value) {
    return (value + alignment - 1) & ~std::uint64_t(alignment - 1);
  };
  auto at = align(first);
  if (at >= limit || bytes > limit - at) return std::nullopt;
  // Owner regions are disjoint (mount rejects overlap), so the first region
  // whose end lies past the candidate is either the one containing it or the
  // first one above it; regions below the candidate cannot change the answer.
  auto it = regions_.upper_bound(std::uint32_t(std::min<std::uint64_t>(at, 0xffffffffu)));
  if (it != regions_.begin()) {
    const auto previous = std::prev(it);
    if (std::uint64_t(previous->first) + previous->second.storage().size() > at) it = previous;
  }
  for (; it != regions_.end(); ++it) {
    const auto address = it->first;
    const auto region_end = std::uint64_t(address) + it->second.storage().size();
    if (region_end <= at) continue;
    if (std::uint64_t(address) >= at + bytes) return std::uint32_t(at);
    at = align(region_end);
    if (at >= limit || bytes > limit - at) return std::nullopt;
  }
  return std::uint32_t(at);
}
void SourceRuntimeMemory::read(std::uint32_t p,
                               std::span<std::byte> output) const {
  if(const auto *page=direct_page(p,output.size())) {
    transfer(output.data(),page->bytes+(p&4095u),output.size());return;
  }
  const auto limit=end(p,output.size());
  if (output.empty()) return;
  const auto first = locate(p);
  const auto &region = first->second;
  const auto offset = std::size_t(p - first->first);
  if (output.size() <= region.storage().size() - offset) {
    validate_committed(first,p,limit);
    transfer(output.data(), region.storage().data() + offset, output.size());
    return;
  }
  validate(p,output.size(),false);
  // Cross-owner transfers stage before publishing to preserve aliases.
  // Whole-extent validation above also keeps rejected output untouched.
  std::vector<std::byte> staged(output.size());
  std::size_t done = 0;
  while (done < staged.size()) {
    const auto found = locate(p + std::uint32_t(done));
    const auto offset = std::size_t(std::uint64_t(p) + done - found->first);
    const auto count =
        std::min(staged.size() - done, found->second.storage().size() - offset);
    std::memcpy(staged.data() + done, found->second.storage().data() + offset,
                count);
    done += count;
  }
  if (!staged.empty())
    std::memmove(output.data(), staged.data(), staged.size());
}
void SourceRuntimeMemory::read_vector(std::uint32_t p,std::span<std::byte,16> output) const {
  // Ordinary retained reads have no device callbacks. Validate and transfer
  // their complete operand once. The scalar source addresses wrap separately;
  // preserve that path rather than rejecting a legal wrap as a16-byte extent.
  if(reference_scalar_vector_reads || p>std::numeric_limits<std::uint32_t>::max()-15u) {
    SourcePhysicsContactMemory::read_vector(p,output);return;
  }
  read(p,output);
}
std::optional<std::span<const std::byte>> SourceRuntimeMemory::try_read_view(
    std::uint32_t p,std::size_t n) const noexcept {
  if(reference_small_transfers || reference_scalar_vector_reads ||
     n>(std::uint64_t{1}<<32)-p)return std::nullopt;
  Position found{};
  if(source_pages_)
    if(const auto &block=(*source_pages_)[p>>22])found=(*block)[(p>>12)&1023];
  if(found==Position{})found=small_lookup(p);
  if(found==Position{}) {
    found=regions_.upper_bound(p);
    if(found==regions_.begin())return std::nullopt;
    --found;
  }
  const auto &region=found->second;
  const auto bytes=region.storage();
  const auto offset=std::uint64_t(p)-found->first;
  if(offset>=bytes.size() || n>bytes.size()-offset)return std::nullopt;
  if(region.page_bytes && n) {
    const auto first=offset/region.page_bytes,last=(offset+n-1)/region.page_bytes;
    if(last>=region.committed.size())return std::nullopt;
    for(auto page=first;page<=last;++page)
      if(!region.committed[page])return std::nullopt;
  }
  return bytes.subspan(std::size_t(offset),n);
}
void SourceRuntimeMemory::write(std::uint32_t p,
                                std::span<const std::byte> input) {
  // A writable direct page already proves the extent, region, ReadWrite
  // access, commitment and absent backing guard that validate() checks.
  // Registry guards and observers still run before the store, in order.
  if(const auto *page=direct_page(p,input.size()); page && page->writable) {
    source_runtime_validate_write(*this,p,input.size(),true);
    // Policies may change topology; then store exactly as the checked route.
    if(const auto *after=direct_page(p,input.size()); after && after->writable) {
      transfer(after->bytes+(p&4095u),input.data(),input.size());return;
    }
  } else {
    validate(p, input.size(), true);
    if (input.empty()) return;
  }
  const auto found = locate(p);
  // locate already established this stable map node after every write guard.
  // This method owns mutable memory; its const lookup iterator does not make
  // the underlying region const. Do not search the same map again per store.
  auto &region = const_cast<Region&>(found->second);
  const auto offset = std::size_t(p - found->first);
  if (input.size() <= region.storage().size() - offset) {
    transfer(region.storage().data() + offset, input.data(), input.size());
    return;
  }
  const std::vector<std::byte> staged(input.begin(), input.end());
  std::size_t done = 0;
  while (done < staged.size()) {
    const auto found = locate(p + std::uint32_t(done));
    auto &region = const_cast<Region&>(found->second);
    const auto offset = std::size_t(std::uint64_t(p) + done - found->first);
    const auto count =
        std::min(staged.size() - done, region.storage().size() - offset);
    std::memcpy(region.storage().data() + offset, staged.data() + done, count);
    done += count;
  }
}
std::span<const std::byte> SourceRuntimeMemory::view(std::uint32_t p,
                                                     std::size_t n) const {
  // A complete committed direct page already proves the extent, single
  // owner and commitment the checked route validates for this span.
  if(const auto *page=direct_page(p,n))return {page->bytes+(p&4095u),n};
  const auto limit=end(p,n);
  const auto found = locate(p);
  const auto offset = std::size_t(p - found->first);
  if (n > found->second.storage().size() - offset)
    throw std::out_of_range("Original runtime view crosses its region owner");
  validate_committed(found,p,limit);
  return found->second.storage().subspan(offset, n);
}
std::span<std::byte> SourceRuntimeMemory::mutable_view(std::uint32_t p,
                                                       std::size_t n) {
  // As write(): a writable direct page proves ReadWrite access and the
  // absent backing guard; registry guards and observers still run first.
  if(const auto *page=direct_page(p,n); page && page->writable) {
    source_runtime_validate_write(*this,p,n);
    if(const auto *after=direct_page(p,n); after && after->writable)return {after->bytes+(p&4095u),n};
  }
  const auto bytes = view(p, n);
  const auto found = locate(p);
  if (found->second.access != Access::ReadWrite)
    throw std::runtime_error(
        "Original runtime mutable view of read-only owner " +
        found->second.owner);
  source_runtime_validate_write(*this,p,n);
  if(found->second.shared && found->second.shared->write_guard_)
    found->second.shared->write_guard_(p,n);
  return const_cast<Region&>(found->second).storage()
      .subspan(p - found->first, bytes.size());
}
void SourceRuntimeMemory::validate_mutable_view(std::uint32_t p,std::size_t n) const {
  (void)view(p,n);validate(p,n,true,false);
}
void SourceRuntimeMemory::index_native_region(std::uint32_t source) {
  static_assert(sizeof(std::uintptr_t)==8,"Live runtime memory requires a 64-bit host");
  index_native_span(source,regions_.at(source).storage());
}
void SourceRuntimeMemory::index_native_span(std::uint32_t source,std::span<const std::byte> bytes) {
  const auto address=reinterpret_cast<std::uintptr_t>(bytes.data());
  const auto limit=(NativeRuntimeAddress{address}+bytes.size()).value;
  const auto next=native_regions_.lower_bound(address);
  if(next!=native_regions_.end() && next->first<limit)
    throw std::logic_error("Native runtime allocations overlap their backing");
  if(next!=native_regions_.begin()) {
    const auto previous=std::prev(next);
    if(previous->first+regions_.at(previous->second).storage().size()>address)
      throw std::logic_error("Native runtime allocations overlap their backing");
  }
  if(!native_regions_.emplace(address,source).second)
    throw std::logic_error("Native runtime allocations alias their backing");
}
void SourceRuntimeMemory::rebuild_native_index() {
  native_regions_.clear();for(const auto&[source,region]:regions_){(void)region;index_native_region(source);}
}
NativeRuntimeAddress SourceRuntimeMemory::native_address(std::uint32_t source,std::size_t bytes)const {
  return NativeRuntimeAddress{reinterpret_cast<std::uintptr_t>(view(source,bytes).data())};
}
std::uint32_t SourceRuntimeMemory::source_address(NativeRuntimeAddress address,std::size_t bytes)const {
  (void)(address+bytes);
  auto found=native_regions_.upper_bound(address.value);
  if(found==native_regions_.begin())throw std::out_of_range("Native runtime address has no owner");
  --found;const auto& region=regions_.at(found->second);
  const auto offset=address.value-found->first;
  if(offset>=region.storage().size() || bytes>region.storage().size()-offset ||
      offset>std::numeric_limits<std::uint32_t>::max()-found->second)
    throw std::out_of_range("Native runtime address escapes its owner");
  const auto source=found->second+std::uint32_t(offset);
  // This also enforces commitment policy and source extent limits. Mutable
  // access checks the region's write policy separately.
  const auto original=view(source,bytes);
  if(reinterpret_cast<std::uintptr_t>(original.data())!=address.value)
    throw std::logic_error("Native runtime address mapping differs from its backing");
  return source;
}
std::span<const std::byte> SourceRuntimeMemory::view(NativeRuntimeAddress address,std::size_t bytes)const {
  return view(source_address(address,bytes),bytes);
}
std::span<std::byte> SourceRuntimeMemory::mutable_view(NativeRuntimeAddress address,std::size_t bytes) {
  return mutable_view(source_address(address,bytes),bytes);
}
void SourceRuntimeMemory::read(NativeRuntimeAddress address,std::span<std::byte> output)const {
  const auto input=view(address,output.size());
  if(!output.empty())std::memmove(output.data(),input.data(),output.size());
}
void SourceRuntimeMemory::write(NativeRuntimeAddress address,std::span<const std::byte> input) {
  const auto output=mutable_view(address,input.size());
  if(!input.empty())std::memmove(output.data(),input.data(),input.size());
}
} // namespace halo2
#include "runtime/source_runtime_write_protection.inc"
