#include "halo2/source_renderer_transparent_storage_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
bool zero(std::span<const std::byte> bytes) {
    return std::ranges::all_of(bytes,[](auto value){return value==std::byte{};});
}
}
void mount_native_tu15_renderer_transparent_storage_data(SourceRuntimeMemory& memory) {
    // Original12560 immediately writes24 here after1E0F0 returns. The
    // authenticated image's writable initial value isFFFFFFFF, not zero.
    memory.mount(0x473658,std::array{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}},
        SourceRuntimeMemory::Access::ReadWrite,"source12560 authored renderer selector before debugger construction");
    memory.mount(0x4c2aa0,std::array<std::byte,0x12>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1E0F0 cold transparent-plane counters, bindings and flags");
    memory.mount(0x52dfd0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1E0F0 cold transparent-container binding");
    constexpr std::array<char,20> name{"transparent planes"};
    memory.mount(0x45c548,std::as_bytes(std::span(name)),SourceRuntimeMemory::Access::ReadOnly,
        "source140C80 authored transparent-container name and copy padding");
    // Original2F7E0 reads this independent .rdata scalar at2F806. It follows
    // the name's padding but is not part of the writable renderer state.
    memory.mount(0x45c55c,std::array{std::byte{0xc3},std::byte{0xf5},std::byte{0x48},std::byte{0x3f}},
        SourceRuntimeMemory::Access::ReadOnly,"source2F7E0 authored view-angle scalar");
}
NativeTu15RendererTransparentStorage native_tu15_renderer_transparent_storage_initial_views(
    const SourceRuntimeMemory& memory) {
    const auto planes=word(memory,0x4c2aa8),indices=word(memory,0x4c2aac),container=word(memory,0x52dfd0);
    if(!planes||!indices||!container||word(memory,0x4c2aa0)||word(memory,0x4c2aa4)||
        memory.view(0x4c2ab0,2)[0]!=std::byte{}||memory.view(0x4c2ab0,2)[1]!=std::byte{1})
        throw std::invalid_argument("Source transparent storage is not fully constructed and idle");
    for(const auto [address,bytes]:std::array<std::pair<std::uint32_t,std::size_t>,3>{{
        {planes,0x1c000},{indices,0x800},{container,0x1203c}}})
        memory.validate_mutable_view(address,bytes);
    if(word(memory,container+0x20)!=0x1000||word(memory,container+0x24)!=0x800||
        word(memory,container+0x28)!=0x10||word(memory,container+0x2c)!=0x1e8a0||
        word(memory,container+0x30)!=0x1e8d0||word(memory,container+0x34)!=0x486034)
        throw std::invalid_argument("Source transparent container definition differs");
    constexpr std::array<char,32> name{"transparent planes"};
    if(!std::ranges::equal(memory.view(container,32),std::as_bytes(std::span(name))))
        throw std::invalid_argument("Source transparent container name differs");
    const auto bucket_base=container+0x3c,node_base=bucket_base+0x4000;
    if(!zero(memory.view(bucket_base,0x4000))||word(memory,container+0x38)!=node_base+0x7ff*0x1c)
        throw std::invalid_argument("Source transparent initial bucket or free-list head differs");
    for(unsigned i=0;i<0x800;++i) {
        const auto node=node_base+i*0x1c;
        if(word(memory,node+8)!=(i?node-0x1c:0)||!zero(memory.view(node,8))||
            !zero(memory.view(node+12,0x10)))
            throw std::invalid_argument("Source transparent initial free-list chain differs");
    }
    NativeTu15RendererTransparentStorage result{memory.native_address(planes,0x1c000),
        memory.native_address(indices,0x800),memory.native_address(container,0x1203c),
        memory.native_address(bucket_base,0x4000),memory.native_address(node_base,0xe000),0x1000,0x800,0x10};
    for(const auto address:std::array{result.planes,result.indices,result.container,result.buckets,result.free_nodes})
        if(address.value<=0xffffffffu)
            throw std::invalid_argument("Source transparent storage lacks native64 backing");
    // Independently allocated source objects must not be aliases of one bank.
    const auto planes_end=std::uint64_t(planes)+0x1c000,indices_end=std::uint64_t(indices)+0x800,
        container_end=std::uint64_t(container)+0x1203c;
    if((planes<indices_end&&indices<planes_end)||(planes<container_end&&container<planes_end)||
        (indices<container_end&&container<indices_end))
        throw std::invalid_argument("Source transparent constructor allocations overlap");
    return result;
}
void source_run_renderer_transparent_storage_with_native_storage(SourceRuntimeMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,SourcePhysicsPoolHostStorage& host,
    const SourcePhysicsContactRecordObserver& observer) {
    if(!host.backs(memory)||!zero(memory.view(0x4c2aa0,0x12))||word(memory,0x52dfd0))
        throw std::invalid_argument("Source transparent cold storage or native allocator owner differs");
    auto joined=services;
    joined.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe) {
        if(host.try_call(bank,call,state))return;
        if(!services.required_owner)
            throw std::runtime_error("Source transparent constructor has an unowned dependency");
        services.required_owner(bank,call,state,observe);
    };
    source_run_renderer_transparent_storage_tu15(memory,0x1e0f0,abi,fp,joined,observer);
    // OriginalAL tests only the first allocation. Require all actual owned
    // spans and the complete source-produced idle container before admission.
    if((abi.gp[0]&255u)!=1u)
        throw std::runtime_error("Source transparent constructor failed");
    (void)native_tu15_renderer_transparent_storage_initial_views(memory);
}
}
