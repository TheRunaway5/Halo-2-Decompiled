#include "halo2/native_object_lifetimes.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
NativeObjectLifetimes::NativeObjectLifetimes(std::int32_t capacity)
    :pool_("object",capacity,12){}
NativeObjectLifetimes::NativeObjectLifetimes(SourceRuntimeMemory &memory,std::uint32_t p):pool_(memory,p) {
    if(pool_.stride()!=12 || pool_.capacity()!=0x800)
        throw FormatError("Original object lifetime view requires its source header pool");
}
NativeObjectLifetimes::NativeObjectLifetimes(SourceRuntimeMemory &memory,NativeRuntimeAddress p):pool_(memory,p) {
    if(pool_.stride()!=12 || pool_.capacity()!=0x800)
        throw FormatError("Original object lifetime view requires its source header pool");
}
void NativeObjectLifetimes::observe_source_birth(std::uint32_t handle) {
    if(!pool_.source_memory() || owners_.contains(handle))
        throw FormatError("Source birth observation requires a new raw source identity");
    const auto header=pool_.lookup(handle);
    if(header.size()!=12 || header[3]>12 || (unsigned(header[6])|(unsigned(header[7])<<8))<4)
        throw FormatError("Source birth observation has no constructed object body");
    std::uint32_t address=0;for(unsigned i=0;i<4;++i)address|=std::uint32_t(header[8+i])<<(8*i);
    if(!address)throw FormatError("Source birth observation has no original body backing");
    const auto body=pool_.source_memory()->view(address,4);std::uint32_t definition=0;
    for(unsigned i=0;i<4;++i)definition|=std::to_integer<std::uint32_t>(body[i])<<(8*i);
    NativeObjectLifetime owner;owner.handle=handle;owner.definition=definition;owner.type=header[3];
    owner.allocation_ordinal=ordinal_+1;record(NativeObjectLifetimeEventKind::reserve,owner);
    owner.published=true;owner.publication_ordinal=ordinal_+1;
    record(NativeObjectLifetimeEventKind::publish,owner);owners_.emplace(handle,owner);
}
void NativeObjectLifetimes::observe_source_retirement(std::uint32_t handle) {
    const auto found=owners_.find(handle);
    if(!pool_.source_memory() || found==owners_.end() || !pool_.lookup(handle).empty())
        throw FormatError("Source retirement observation requires an actually retired full identity");
    record(NativeObjectLifetimeEventKind::release,found->second);
    owners_.erase(found);
}
void NativeObjectLifetimes::observe_source_retirements() {
    if(!pool_.source_memory())throw FormatError("Source retirement sweep requires its retained source pool");
    for(auto at=owners_.begin();at!=owners_.end();) {
        if(!pool_.lookup(at->first).empty()){++at;continue;}
        record(NativeObjectLifetimeEventKind::release,at->second);at=owners_.erase(at);
    }
}
std::uint64_t NativeObjectLifetimes::record(NativeObjectLifetimeEventKind kind,
    const NativeObjectLifetime& owner) {
    if(ordinal_==std::numeric_limits<std::uint64_t>::max())
        throw FormatError("native object lifetime ordinal exhausted");
    const auto next=ordinal_+1;
    events_.push_back({next,kind,owner});ordinal_=next;return next;
}
std::uint32_t NativeObjectLifetimes::reserve(std::uint8_t type) {
    if(type>12)throw FormatError("native object type outside original header table");
    return reserve_header(type,true);
}
std::uint32_t NativeObjectLifetimes::reserve_storage() {
    return reserve_header(0xff,false);
}
std::uint32_t NativeObjectLifetimes::reserve_header(std::uint8_t type,bool write_type) {
    const auto handle=pool_.allocate();if(handle==DatumPool::none)return handle;
    if(write_type)pool_.lookup(handle)[3]=type;
    NativeObjectLifetime owner;owner.handle=handle;owner.type=type;
    owner.allocation_ordinal=ordinal_+1;
    record(NativeObjectLifetimeEventKind::reserve,owner);
    if(!owners_.emplace(handle,owner).second)
        throw FormatError("native object pool allocated a duplicate full identity");
    return handle;
}
const NativeObjectLifetime& NativeObjectLifetimes::owner(std::uint32_t handle) const {
    const auto found=owners_.find(handle);
    if(found==owners_.end() || pool_.lookup(handle).empty())
        throw FormatError("stale native object lifetime identity");
    return found->second;
}
bool NativeObjectLifetimes::contains(std::uint32_t handle) const {
    return owners_.contains(handle) && !pool_.lookup(handle).empty();
}
void NativeObjectLifetimes::publish_native(std::uint32_t handle,std::uint32_t definition) {
    const auto &current=owner(handle);
    if(current.published || current.type>12)
        throw FormatError("invalid native object construction publication");
    auto next=current;next.published=true;next.definition=definition;
    next.publication_ordinal=ordinal_+1;
    record(NativeObjectLifetimeEventKind::publish,next);owners_.at(handle)=next;
}
void NativeObjectLifetimes::release(std::uint32_t handle) {
    const auto backing=header(handle);
    if(backing.size()!=12 || std::any_of(backing.begin()+8,backing.end(),[](auto b){return b!=0;}))
        throw FormatError("native object body requires preceding source storage release");
    record(NativeObjectLifetimeEventKind::release,owner(handle));
    pool_.release_unchecked(handle);owners_.erase(handle);
}
std::span<std::uint8_t> NativeObjectLifetimes::header(std::uint32_t handle) {
    return pool_.lookup(handle);
}
std::span<const std::uint8_t> NativeObjectLifetimes::header(std::uint32_t handle) const {
    return pool_.lookup(handle);
}
}
