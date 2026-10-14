#include "halo2/datum_pool.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset,unsigned size=4) {
    std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
DatumPool::SourceBinding binding(const SourceRuntimeMemory &memory,std::uint32_t address,bool writable) {
    const auto header=memory.view(address,0x4c);
    DatumPool::SourceBinding result;result.pool=address;
    result.capacity=word(header,0x20);result.stride=word(header,0x24);
    if(!address || word(header,0x29,1)!=1 || !result.capacity || result.capacity>65535 ||
       result.stride<2 || std::uint64_t(result.capacity)*result.stride>UINT32_MAX ||
       word(header,0x34)>result.capacity || word(header,0x38)>result.capacity ||
       word(header,0x3c)>word(header,0x38))
        throw std::invalid_argument("Source continuation pool has invalid shape or allocator bounds");
    std::copy_n(header.begin(),result.name.size(),result.name.begin());
    if(writable) {
        memory.validate_mutable_view(address,0x4c);
        memory.validate_mutable_view(word(header,0x44),std::size_t(result.capacity)*result.stride);
        memory.validate_mutable_view(word(header,0x48),((std::size_t(result.capacity)+31)/32)*4);
    }
    // Completed original reset/allocate/retire/bind-existing owners maintain
    // the same salt/bitmap occupancy and count/high-water frontier. Check it
    // without normalizing or reconstructing a single allocator byte. Cursor
    // and next-salt remain the actual imported values, not derived values.
    const auto elements=memory.view(word(header,0x44),std::size_t(result.capacity)*result.stride);
    const auto bitmap=memory.view(word(header,0x48),((std::size_t(result.capacity)+31)/32)*4);
    std::uint32_t count=0,high=0;
    for(std::uint32_t i=0;i<result.capacity;++i) {
        const bool occupied=word(elements,std::size_t(i)*result.stride,2)!=0;
        if(occupied!=bool(word(bitmap,(i/32)*4)&(1u<<(i&31u))))
            throw std::invalid_argument("Source continuation pool salt/bitmap occupancy differs");
        if(occupied){++count;high=i+1;}
    }
    if(count!=word(header,0x3c) || high!=word(header,0x38))
        throw std::invalid_argument("Source continuation pool allocation frontier differs from its records");
    return result;
}
void validate_lifetimes(const NativeObjectLifetimes::SourceAuthorityCheckpoint &saved,
                        const SourceRuntimeMemory &memory) {
    if(saved.version!=1 || saved.pool.capacity!=0x800 || saved.pool.stride!=12 ||
       saved.ordinal!=saved.events.size() || saved.owners.size()>saved.pool.capacity)
        throw std::invalid_argument("Source lifetime continuation has invalid shape or event frontier");
    std::map<std::uint32_t,NativeObjectLifetime> replayed;
    for(std::size_t i=0;i<saved.events.size();++i) {
        const auto &event=saved.events[i];const auto &owner=event.owner;
        if(event.ordinal!=i+1 || owner.handle==DatumPool::none ||
           (owner.handle&65535u)>=saved.pool.capacity || !(owner.handle>>16) ||
           (owner.type>12 && owner.type!=0xff))
            throw std::invalid_argument("Source lifetime continuation has an invalid ordered identity");
        const auto current=replayed.find(owner.handle);
        switch(event.kind) {
        case NativeObjectLifetimeEventKind::reserve:
            if(current!=replayed.end() || owner.published || owner.publication_ordinal ||
               owner.allocation_ordinal!=event.ordinal)
                throw std::invalid_argument("Source lifetime continuation has an invalid reservation");
            replayed.emplace(owner.handle,owner);break;
        case NativeObjectLifetimeEventKind::publish:
            if(current==replayed.end() || current->second.published || !owner.published ||
               owner.type>12 || owner.type!=current->second.type ||
               owner.allocation_ordinal!=current->second.allocation_ordinal ||
               owner.publication_ordinal!=event.ordinal)
                throw std::invalid_argument("Source lifetime continuation has an invalid publication");
            current->second=owner;break;
        case NativeObjectLifetimeEventKind::release:
            if(current==replayed.end() || current->second!=owner)
                throw std::invalid_argument("Source lifetime continuation has an invalid retirement");
            replayed.erase(current);break;
        default:throw std::invalid_argument("Source lifetime continuation has an unknown event kind");
        }
    }
    if(replayed!=saved.owners)
        throw std::invalid_argument("Source lifetime continuation sidecars differ from their mutation history");
    const SourceRawDatumPoolServices storage{.read=[&](auto address,auto bytes){return memory.view(address,bytes);}};
    for(const auto &[handle,owner]:saved.owners) {
        const auto address=source_raw_datum_lookup(saved.pool.pool,handle,storage);
        if(handle!=owner.handle || !address)
            throw std::invalid_argument("Source lifetime continuation lost its full allocated salt");
        const auto header=memory.view(address,12);
        if(owner.published) {
            const auto body=word(header,8);
            if(word(header,3,1)!=owner.type || word(header,6,2)<4 || !body ||
               word(memory.view(body,4),0)!=owner.definition)
                throw std::invalid_argument("Source lifetime continuation differs from its constructed body");
        } else if(owner.type!=0xff && word(header,3,1)!=owner.type)
            throw std::invalid_argument("Source lifetime continuation differs from its reserved header type");
    }
}
}

struct DatumPool::PreparedSourceView::Impl {
    const DatumPool *owner;
    SourceRuntimeMemory *bank;
    SourceBinding binding;
    NativeRuntimeAddress native;
};
DatumPool::PreparedSourceView::PreparedSourceView()=default;
DatumPool::PreparedSourceView::~PreparedSourceView()=default;
DatumPool::PreparedSourceView::PreparedSourceView(PreparedSourceView &&) noexcept=default;
DatumPool::PreparedSourceView &DatumPool::PreparedSourceView::operator=(PreparedSourceView &&) noexcept=default;
bool DatumPool::PreparedSourceView::for_owner(const DatumPool &owner) const noexcept {
    return impl_ && impl_->owner==&owner;
}
DatumPool::SourceBinding DatumPool::source_binding() const {
    if(!source_memory_)throw std::logic_error("Owned datum pool does not have a source-bank binding");
    // Current platform resource owners can already hold their publication
    // locks in the enclosing transaction. Capture is read-only; only the
    // independent replacement bank admits future mutable borrowers.
    return binding(*source_memory_,source_pool(),false);
}
DatumPool::PreparedSourceView DatumPool::prepare_source_view(const SourceBinding &saved,
                                                            const SourceRuntimeMemory &replacement) const {
    const auto current=source_binding();
    if(&replacement==source_memory_ || saved.version!=1 || saved.capacity!=current.capacity ||
       saved.stride!=current.stride || saved.name!=current.name || binding(replacement,saved.pool,true)!=saved)
        throw std::invalid_argument("Source continuation pool does not match its local owner/bank");
    PreparedSourceView prepared;
    prepared.impl_=std::make_unique<PreparedSourceView::Impl>(PreparedSourceView::Impl{
        this,source_memory_,saved,replacement.native_address(saved.pool,0x4c)});
    return prepared;
}
bool DatumPool::source_view_ready(const PreparedSourceView &prepared) const noexcept {
    if(!prepared.for_owner(*this) || source_memory_!=prepared.impl_->bank)return false;
    try {
        return source_memory_->native_address(prepared.impl_->binding.pool,0x4c)==prepared.impl_->native;
    } catch(...) {return false;}
}
bool DatumPool::commit_source_view(PreparedSourceView &&prepared) noexcept {
    if(!source_view_ready(prepared))return false;
    source_pool_=prepared.impl_->native;bitmap_view_.clear();prepared.impl_.reset();return true;
}

struct NativeObjectLifetimes::PreparedSourceAuthority::Impl {
    const NativeObjectLifetimes *owner;
    DatumPool::PreparedSourceView pool;
    std::map<std::uint32_t,NativeObjectLifetime> owners;
    std::vector<NativeObjectLifetimeEvent> events;
    std::uint64_t ordinal;
};
NativeObjectLifetimes::PreparedSourceAuthority::PreparedSourceAuthority()=default;
NativeObjectLifetimes::PreparedSourceAuthority::~PreparedSourceAuthority()=default;
NativeObjectLifetimes::PreparedSourceAuthority::PreparedSourceAuthority(PreparedSourceAuthority &&) noexcept=default;
NativeObjectLifetimes::PreparedSourceAuthority &NativeObjectLifetimes::PreparedSourceAuthority::operator=(PreparedSourceAuthority &&) noexcept=default;
bool NativeObjectLifetimes::PreparedSourceAuthority::for_owner(const NativeObjectLifetimes &owner) const noexcept {
    return impl_ && impl_->owner==&owner;
}
NativeObjectLifetimes::SourceAuthorityCheckpoint NativeObjectLifetimes::source_authority_checkpoint() const {
    SourceAuthorityCheckpoint saved;saved.pool=pool_.source_binding();saved.owners=owners_;
    saved.events=events_;saved.ordinal=ordinal_;validate_lifetimes(saved,*pool_.source_memory());return saved;
}
NativeObjectLifetimes::PreparedSourceAuthority NativeObjectLifetimes::prepare_source_authority(
    const SourceAuthorityCheckpoint &saved,const SourceRuntimeMemory &replacement) const {
    auto pool=pool_.prepare_source_view(saved.pool,replacement);
    validate_lifetimes(saved,replacement);
    PreparedSourceAuthority prepared;
    prepared.impl_=std::make_unique<PreparedSourceAuthority::Impl>(PreparedSourceAuthority::Impl{
        this,std::move(pool),saved.owners,saved.events,saved.ordinal});return prepared;
}
bool NativeObjectLifetimes::commit_source_authority(PreparedSourceAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !pool_.commit_source_view(std::move(prepared.impl_->pool)))return false;
    owners_.swap(prepared.impl_->owners);events_.swap(prepared.impl_->events);ordinal_=prepared.impl_->ordinal;
    prepared.impl_.reset();return true;
}
} // namespace halo2
