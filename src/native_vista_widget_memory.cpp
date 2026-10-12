#include "halo2/native_vista_widget_memory.hpp"
#include "halo2/native_vista_widget_managers.hpp"
#include "halo2/native_vista_widget_cache.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <new>
#include <utility>

namespace halo2 {
namespace {
std::uint64_t end(std::uint32_t at,std::size_t size) {
  if(size>(std::uint64_t{1}<<32)-at)
    throw FormatError("Vista source bank extent wraps32 bits");
  return std::uint64_t(at)+size;
}
void scalar(std::size_t n) {
  if(n!=1&&n!=2&&n!=4)throw FormatError("Vista registry scalar width must be1,2 or4");
}
} // namespace
struct NativeVistaWidgetMemory::Impl {
  struct Region {
    std::span<const std::byte> readable;
    std::span<std::byte> writable;
    std::shared_ptr<const void> lifetime;
    std::string owner;
    bool generic_owned{};
  };
  SourceRuntimeMemory owned;
  std::map<std::uint32_t,Region> regions;
  void admit(std::uint32_t at,std::size_t size,const std::string& owner) const {
    if(!size||owner.empty())throw FormatError("Vista source bank requires bytes and an owner");
    const auto limit=end(at,size);const auto next=regions.lower_bound(at);
    if(next!=regions.end()&&next->first<limit)
      throw FormatError("Vista source banks overlap");
    if(next!=regions.begin()) {
      const auto previous=std::prev(next);
      if(std::uint64_t(previous->first)+previous->second.readable.size()>at)
        throw FormatError("Vista source banks overlap");
    }
  }
  auto locate(std::uint32_t at) const {
    auto found=regions.upper_bound(at);
    if(found==regions.begin())throw FormatError("Required Vista source bank is absent");
    --found;
    if(std::uint64_t(at)-found->first>=found->second.readable.size())
      throw FormatError("Required Vista source bank is absent");
    return found;
  }
  void validate(std::uint32_t at,std::size_t size,bool writing) const {
    const auto limit=end(at,size);auto current=std::uint64_t(at);
    while(current<limit) {
      const auto found=locate(std::uint32_t(current));const auto& r=found->second;
      if(writing&&r.writable.empty())throw FormatError("Vista write reached a read-only source bank: "+r.owner);
      current=std::min(limit,std::uint64_t(found->first)+r.readable.size());
    }
  }
  void borrowed(std::uint32_t at,std::span<const std::byte> readable,
      std::span<std::byte> writable,std::shared_ptr<const void> lifetime,
      std::string owner,bool required_lifetime) {
    admit(at,readable.size(),owner);
    if(required_lifetime&&!lifetime)throw FormatError("Vista borrowed bank requires its retained owner");
    regions.emplace(at,Region{readable,writable,std::move(lifetime),std::move(owner),false});
  }
};
NativeVistaWidgetMemory::NativeVistaWidgetMemory():impl_(std::make_unique<Impl>()) {}
NativeVistaWidgetMemory::~NativeVistaWidgetMemory()=default;
void NativeVistaWidgetMemory::mount(std::uint32_t at,std::span<const std::byte> input,
    Access access,std::string owner) {
  impl_->admit(at,input.size(),owner);impl_->owned.mount(at,input,access,owner);
  try {
    const auto bytes=impl_->owned.view(at,input.size());
    const auto writable=access==Access::ReadWrite?impl_->owned.mutable_view(at,input.size()):std::span<std::byte>{};
    impl_->regions.emplace(at,Impl::Region{bytes,writable,{},std::move(owner),true});
  }catch(...) {impl_->owned.release(at);throw;}
}
void NativeVistaWidgetMemory::mount_aligned(std::uint32_t at,std::span<const std::byte> input,
    std::size_t alignment,Access access,std::string owner) {
  impl_->admit(at,input.size(),owner);
  if(!alignment||(alignment&(alignment-1))||std::uint64_t(at)%alignment)
    throw FormatError("Vista aligned bank requires power-of-two source and host alignment");
  const auto host_alignment=std::max(alignment,alignof(std::max_align_t));
  auto* raw=static_cast<std::byte*>(::operator new(input.size(),std::align_val_t(host_alignment)));
  std::shared_ptr<const void> lifetime(raw,[host_alignment](const void* p){::operator delete(const_cast<void*>(p),std::align_val_t(host_alignment));});
  std::memcpy(raw,input.data(),input.size());
  const std::span<std::byte> bytes(raw,input.size());
  impl_->borrowed(at,bytes,access==Access::ReadWrite?bytes:std::span<std::byte>{},std::move(lifetime),std::move(owner),true);
}
void NativeVistaWidgetMemory::borrow(std::uint32_t at,std::span<std::byte> input,
    Access access,std::shared_ptr<const void> lifetime,std::string owner) {
  impl_->borrowed(at,input,access==Access::ReadWrite?input:std::span<std::byte>{},std::move(lifetime),std::move(owner),true);
}
void NativeVistaWidgetMemory::borrow(std::uint32_t at,std::span<const std::byte> input,
    std::shared_ptr<const void> lifetime,std::string owner) {
  impl_->borrowed(at,input,{},std::move(lifetime),std::move(owner),true);
}
void NativeVistaWidgetMemory::bind_managers(NativeVistaWidgetManagers& owner) {
  auto bytes=owner.bytes(owner.source_base,owner.storage_size);
  impl_->borrowed(owner.source_base,bytes,bytes,{},"retained actual Vista UI managers",false);
}
void NativeVistaWidgetMemory::bind_managers(std::shared_ptr<NativeVistaWidgetManagers> owner) {
  if(!owner)throw FormatError("Vista manager binding requires its actual owner");
  auto bytes=owner->bytes(owner->source_base,owner->storage_size);
  impl_->borrowed(NativeVistaWidgetManagers::source_base,bytes,bytes,std::move(owner),"retained actual Vista UI managers",true);
}
void NativeVistaWidgetMemory::bind_cache(NativeVistaWidgetCache& owner) {
  auto bytes=owner.bytes(owner.source_base(),owner.storage_size());
  impl_->borrowed(owner.source_base(),bytes,bytes,{},"retained actual Vista loaded cache",false);
}
void NativeVistaWidgetMemory::bind_cache(std::shared_ptr<NativeVistaWidgetCache> owner) {
  if(!owner)throw FormatError("Vista cache binding requires its actual owner");
  const auto base=owner->source_base();auto bytes=owner->bytes(base,owner->storage_size());
  impl_->borrowed(base,bytes,bytes,std::move(owner),"retained actual Vista loaded cache",true);
}
void NativeVistaWidgetMemory::release(std::uint32_t at) {
  const auto found=impl_->regions.find(at);
  if(found==impl_->regions.end())throw FormatError("Vista release requires its exact registered bank base");
  if(found->second.generic_owned)impl_->owned.release(at);
  impl_->regions.erase(found);
}
void NativeVistaWidgetMemory::read(std::uint32_t at,std::span<std::byte> output) const {
  impl_->validate(at,output.size(),false);std::vector<std::byte> staged(output.size());
  std::size_t done{};
  while(done<staged.size()) {
    const auto found=impl_->locate(at+std::uint32_t(done));const auto& r=found->second;
    const auto offset=std::size_t(std::uint64_t(at)+done-found->first);
    const auto n=std::min(staged.size()-done,r.readable.size()-offset);
    std::memcpy(staged.data()+done,r.readable.data()+offset,n);done+=n;
  }
  if(!staged.empty())std::memmove(output.data(),staged.data(),staged.size());
}
void NativeVistaWidgetMemory::write(std::uint32_t at,std::span<const std::byte> input) {
  impl_->validate(at,input.size(),true);const std::vector<std::byte> staged(input.begin(),input.end());
  std::size_t done{};
  while(done<staged.size()) {
    const auto found=impl_->locate(at+std::uint32_t(done));const auto& r=found->second;
    const auto offset=std::size_t(std::uint64_t(at)+done-found->first);
    const auto n=std::min(staged.size()-done,r.writable.size()-offset);
    std::memcpy(r.writable.data()+offset,staged.data()+done,n);done+=n;
  }
}
std::uint32_t NativeVistaWidgetMemory::read(std::uint32_t at,std::size_t n) const {
  scalar(n);std::array<std::byte,4> bytes{};read(at,std::span(bytes).first(n));std::uint32_t value{};
  for(std::size_t i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return value;
}
void NativeVistaWidgetMemory::write(std::uint32_t at,std::uint32_t value,std::size_t n) {
  scalar(n);std::array<std::byte,4> bytes{};
  for(std::size_t i=0;i<n;++i)bytes[i]=std::byte(std::uint8_t(value>>(8*i)));
  write(at,std::span<const std::byte>(bytes).first(n));
}
std::span<const std::byte> NativeVistaWidgetMemory::view(std::uint32_t at,std::size_t n) const {
  end(at,n);const auto found=impl_->locate(at);const auto offset=std::size_t(at-found->first);
  if(n>found->second.readable.size()-offset)throw FormatError("Vista view crosses its registered bank");
  return found->second.readable.subspan(offset,n);
}
std::span<std::byte> NativeVistaWidgetMemory::mutable_view(std::uint32_t at,std::size_t n) {
  (void)view(at,n);const auto found=impl_->locate(at);
  if(found->second.writable.empty())throw FormatError("Vista mutable view reached a read-only source bank");
  return found->second.writable.subspan(at-found->first,n);
}
} // namespace halo2
