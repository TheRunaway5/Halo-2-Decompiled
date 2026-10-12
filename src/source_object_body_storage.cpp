#include "halo2/source_object_body_storage.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <string_view>

namespace halo2 {
namespace {
constexpr auto none=DatumPool::none;
std::uint32_t read(std::span<const std::byte> data,std::size_t at,unsigned size=4) {
    if(at>data.size() || size>data.size()-at)throw FormatError("source body storage field unavailable");
    std::uint32_t value=0;
    for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(data[at+i])<<(8*i);
    return value;
}
}
SourceObjectBodyStorage::SourceObjectBodyStorage(std::uint32_t capacity,std::byte initial)
    :capacity_(capacity) {
    if(!capacity || capacity>0x100000)throw FormatError("source body arena capacity unavailable");
    arena_.assign((capacity+0x53u)&~3u,initial);handles_.fill(none);
    constexpr std::string_view name="objects";
    for(std::size_t i=0;i<name.size();++i)scratch_[16+i]=std::byte(name[i]);
    // No header/pool provider is needed by actual 18FC80. Its stores alone
    // initialize the heap; the caller-specified payload bytes are preserved.
    SourceObjectBodyHeapServices s;
    s.read=[this](auto a,auto n)->std::span<const std::byte>{
        if(a>=heap_address && std::uint64_t(a)-heap_address+n<=arena_.size())
            return std::span<const std::byte>(arena_).subspan(a-heap_address,n);
        if(a>=scratch_address && std::uint64_t(a)-scratch_address+n<=scratch_.size())
            return std::span<const std::byte>(scratch_).subspan(a-scratch_address,n);
        throw FormatError("source body heap initialization read unavailable");};
    s.memory=[this](auto a,auto n)->std::span<std::byte>{
        if(a>=heap_address && std::uint64_t(a)-heap_address+n<=arena_.size())
            return std::span<std::byte>(arena_).subspan(a-heap_address,n);
        throw FormatError("source body heap initialization write unavailable");};
    initialize_source_object_body_heap(heap_address,capacity_,name_address,0,s);
}
std::uint32_t SourceObjectBodyStorage::pointer_cell(std::uint32_t handle) const {
    return headers_address+(handle&65535)*12+8;
}
bool SourceObjectBodyStorage::contains(std::uint32_t handle) const {
    return handle!=none && (handle&65535)<handles_.size() && handles_[handle&65535]==handle;
}
void SourceObjectBodyStorage::require_owner(const NativeObjectLifetimes& lifetimes,std::uint32_t h) const {
    if(!contains(h) || !lifetimes.contains(h))throw FormatError("source body storage lifetime absent");
}
std::span<std::byte> SourceObjectBodyStorage::memory(NativeObjectLifetimes& lifetimes,
    std::uint32_t address,std::size_t size) {
    const auto fits=[&](std::uint32_t base,std::size_t extent){
        const auto offset=std::uint64_t(address)-base;
        return address>=base && offset<=extent && size<=extent-offset;};
    if(fits(heap_address,arena_.size()))
        return std::span<std::byte>(arena_).subspan(address-heap_address,size);
    auto headers=std::as_writable_bytes(lifetimes.source_headers());
    if(fits(headers_address,headers.size()))
        return headers.subspan(address-headers_address,size);
    if(fits(scratch_address,scratch_.size()))
        return std::span<std::byte>(scratch_).subspan(address-scratch_address,size);
    throw FormatError("source body storage address unavailable");
}
SourceObjectBodyHeapServices SourceObjectBodyStorage::services(NativeObjectLifetimes& lifetimes) {
    return {[this,&lifetimes](auto a,auto n)->std::span<const std::byte>{return memory(lifetimes,a,n);},
        [this,&lifetimes](auto a,auto n){return memory(lifetimes,a,n);},{}};
}
std::span<std::byte> SourceObjectBodyStorage::backing(NativeObjectLifetimes& lifetimes,std::uint32_t h) {
    require_owner(lifetimes,h);
    const auto pointer=read(std::as_bytes(lifetimes.header(h)),8);
    if(pointer<heap_address+0x50 || std::uint64_t(pointer)-heap_address>arena_.size())
        throw FormatError("source body pointer unavailable");
    const auto block=pointer-16;
    const auto size=read(memory(lifetimes,block,16),0);
    if(size<16 || size>arena_.size()-(block-heap_address))
        throw FormatError("source body block extent unavailable");
    return std::span<std::byte>(arena_).subspan(pointer-heap_address,size-16);
}
std::span<std::byte> SourceObjectBodyStorage::body(NativeObjectLifetimes& lifetimes,std::uint32_t h) {
    const auto bytes=read(std::as_bytes(lifetimes.header(h)),6,2);auto result=backing(lifetimes,h);
    if(bytes>result.size())throw FormatError("source body logical extent exceeds backing");
    return result.first(bytes);
}
UnitConstructorObjectPool SourceObjectBodyStorage::objects(NativeObjectLifetimes& lifetimes,
    const ObjectConstructionObserver& observer) {
    if(std::size_t(lifetimes.pool().capacity())>handles_.size())
        throw FormatError("source body header capacity exceeds original shared pool");
    return {std::as_writable_bytes(lifetimes.source_headers()),std::uint32_t(lifetimes.pool().capacity()),
        [this,&lifetimes](std::uint16_t slot){
            if(slot>=handles_.size())throw FormatError("source body index unavailable");
            return backing(lifetimes,handles_[slot]);},
        [this,&lifetimes,observer](std::uint32_t h,std::int32_t bytes){
            require_owner(lifetimes,h);
            return resize_source_object_body(heap_address,pointer_cell(h),std::bit_cast<std::uint32_t>(bytes),
                0,0,{scratch_address},services(lifetimes),observer);}};
}
std::uint32_t SourceObjectBodyStorage::allocate(NativeObjectLifetimes& lifetimes,std::int16_t extent,
    const UnitActionObserver& observer,const ObjectConstructionObserver& heap_observer) {
    SourceObjectBodyAllocationServices s;s.objects=objects(lifetimes,heap_observer);
    s.allocate_header=[this,&lifetimes]{
        const auto h=lifetimes.reserve_storage();
        if(h!=none)handles_.at(h&65535)=h;
        return h;};
    s.allocate_body=[this,&lifetimes,&heap_observer](auto h,std::int32_t bytes){
        return allocate_source_object_body(heap_address,pointer_cell(h),std::bit_cast<std::uint32_t>(bytes),
            0,0,services(lifetimes),heap_observer);};
    s.release_header=[this,&lifetimes](auto h){lifetimes.release(h);handles_[h&65535]=none;};
    return allocate_source_object_body(extent,s,observer);
}
bool SourceObjectBodyStorage::append(NativeObjectLifetimes& lifetimes,std::uint32_t h,
    std::int16_t descriptor,std::int16_t bytes,std::uint8_t alignment,
    const UnitConstructorObserver& observer,const ObjectConstructionObserver& heap_observer) {
    require_owner(lifetimes,h);
    return append_source_unit_component(h,descriptor,bytes,alignment,objects(lifetimes,heap_observer),observer);
}
void SourceObjectBodyStorage::release(NativeObjectLifetimes& lifetimes,std::uint32_t h,
    const ObjectConstructionObserver& observer) {
    require_owner(lifetimes,h);
    auto header=std::as_writable_bytes(lifetimes.header(h));
    // Actual BD1F0 order: header flags0, free body, header pointer0, then
    // header-pool retirement. Its complete pool primitive is DatumPool's owner.
    const auto store=[&](std::uint32_t pc,unsigned at,unsigned n,std::uint32_t value){
        const auto before=read(header,at,n);
        for(unsigned i=0;i<n;++i)header[at+i]=std::byte(value>>(8*i));
        if(observer)observer({pc,headers_address+(h&65535)*12+at,n,before,value});};
    store(0xbd215,2,1,0);
    free_source_object_body(heap_address,pointer_cell(h),services(lifetimes),observer);
    store(0xbd250,8,4,0);
    lifetimes.release(h);handles_[h&65535]=none;
}
}
