#include "halo2/source_render_object_startup_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
void cold(SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes) {
    memory.mount(address,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,
        "source startup33..44 loader-zeroed constructor destination");
}
template<std::size_t N>void literal(SourceRuntimeMemory& memory,std::uint32_t address,const std::array<std::uint8_t,N>& bytes) {
    memory.mount(address,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,
        "source startup33..44 immutable constructor metadata");
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}
struct Binding {std::uint32_t cell,bytes;};
constexpr std::array<Binding,24> bindings{{
    {0x4ce27c,0xf000},{0x4ce294,0x6000},{0x4ce2ac,0x3000},
    {0x515f14,0x1006c},{0x515f0c,0x18},{0x4f9b50,0x4204},
    {0x4f5504,4},{0x51df6c,0x78},{0x51df70,0x14},
    {0x4f7220,0x100cc},{0x4f721c,0x3c2c},{0x515f24,0xc},{0x515f28,0x613c},
    {0x4f7214,0x486c},{0x4f7210,0xe0cc},{0x4f720c,0x1906c},{0x4f7208,0x788c},
    {0x51df74,0x2a5c},{0x52d714,0x100cc},{0x52d718,0x4c6c},{0x52d71c,0x346c},
    {0x50ebf0,0x360},{0x4d4fb8,0x1000},{0x491878,0x6c}}};
// Last two are statically owned banks rather than pointer cells.
NativeTu15RenderObjectStartupStorage::Region region(const SourceRuntimeMemory& memory,Binding binding) {
    const auto source=(binding.cell==0x4d4fb8||binding.cell==0x491878)?binding.cell:word(memory,binding.cell);
    if(!source)throw std::invalid_argument("Source startup33..44 storage binding is absent");
    memory.validate_mutable_view(source,binding.bytes);
    const auto native=memory.native_address(source,binding.bytes);
    if(native.value<=0xffffffffu)throw std::invalid_argument("Source startup33..44 backing is not native64");
    return {binding.cell,source,binding.bytes,native};
}
void validate_constructor_storage(const SourceRuntimeMemory& memory,std::uint32_t entry) {
    std::size_t first=0,count=0;
    switch(entry) {
    case 0x2b480:first=0;count=5;break;
    case 0x186cb0:first=5;count=1;break;
    case 0x157e10:first=6;count=2;break;
    case 0x171e10:first=8;count=1;break;
    case 0x1800b0:first=9;count=4;break;
    case 0x17e640:first=13;count=2;break;
    case 0x1789f0:first=15;count=6;break;
    case 0x228260:first=21;count=1;break;
    case 0x31d400:return;
    default:throw std::invalid_argument("Source startup33..44 entry is not an initializer");
    }
    for(std::size_t i=first;i<first+count;++i)(void)region(memory,bindings[i]);
    if(entry==0x1800b0) {
        const auto resource=word(memory,0x515f24);
        if(word(memory,resource)!=1||word(memory,resource+8))
            throw std::invalid_argument("Source decal vertex resource header differs");
        const auto data=word(memory,resource+4)|0x80000000u;
        memory.validate_mutable_view(data,0x14000);
        if(memory.native_address(data,0x14000).value<=0xffffffffu)
            throw std::invalid_argument("Source decal vertex resource data lacks native64 backing");
    }
}
}
void mount_native_tu15_render_object_startup_data(SourceRuntimeMemory& memory) {
#include "native_tu15_render_object_startup_data.inc"
}
bool source_render_object_startup_tu15_is_initializer(std::uint32_t entry) {
    switch(entry) {case 0x2b480:case 0x31d400:case 0x186cb0:case 0x157e10:case 0x171e10:
        case 0x1800b0:case 0x17e640:case 0x1789f0:case 0x228260:return true;default:return false;}
}
void source_run_render_object_startup_with_native_storage(SourceRuntimeMemory& memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,SourcePhysicsPoolHostStorage& host,
    const SourcePhysicsContactRecordObserver& observer) {
    if(!host.backs(memory)||!source_render_object_startup_tu15_is_initializer(entry))
        throw std::invalid_argument("Source startup33..44 storage owner or initializer differs");
    auto joined=services;
    joined.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe) {
        if(host.try_call(bank,call,state))return;
        if(!services.required_owner)throw std::runtime_error("Source startup33..44 has an unowned dependency");
        services.required_owner(bank,call,state,observe);
    };
    source_run_render_object_startup_tu15(memory,entry,abi,fp,joined,observer);
    validate_constructor_storage(memory,entry);
}
NativeTu15RenderObjectStartupStorage native_tu15_render_object_startup_views(const SourceRuntimeMemory& memory) {
    for(const auto entry:std::array<std::uint32_t,8>{0x2b480,0x186cb0,0x157e10,0x171e10,0x1800b0,0x17e640,0x1789f0,0x228260})
        validate_constructor_storage(memory,entry);
    NativeTu15RenderObjectStartupStorage result;result.regions.reserve(bindings.size()+1);
    for(const auto binding:bindings)result.regions.push_back(region(memory,binding));
    const auto resource=word(memory,0x515f24),data=word(memory,resource+4)|0x80000000u;
    result.regions.push_back({resource+4,data,0x14000,memory.native_address(data,0x14000)});
    return result;
}
}
