#include "halo2/native_tu15_texture_pair_startup.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t first_cell=0x515e54,first_handle=0xf9000301;
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
unsigned index(std::uint32_t value,std::uint32_t first,std::uint32_t stride) {
    if(value<first||(value-first)%stride||(value-first)/stride>=5)
        throw std::invalid_argument("Native texture-pair identity is outside this publication");
    return (value-first)/stride;
}
}
NativeTu15TexturePairStartup::NativeTu15TexturePairStartup(SourceRuntimeMemory& memory,NativeTu15GraphicsDevice& device,
    NativeTu15FramebufferStartup& framebuffers,RendererTexturePairFactory factory)
    :memory_(memory),device_(device),framebuffers_(framebuffers),factory_(std::move(factory)){}
bool NativeTu15TexturePairStartup::initialized()const {
    if(std::this_thread::get_id()!=thread_||!resources_||!resources_->valid()||!device_.initialized()||!framebuffers_.initialized())return false;
    for(unsigned i=0;i<5;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)return false;
    return true;
}
bool NativeTu15TexturePairStartup::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x14830)return false;
    if(&bank!=&memory_||std::this_thread::get_id()!=thread_||call.tail||call.instruction!=0x127a8||
       word(memory_,abi.gp[4])!=0x127ad||resources_)
        throw std::invalid_argument("Native texture-pair call has a foreign caller or duplicate cold owner");
    if(!device_.initialized()||!framebuffers_.initialized()||!factory_.valid()||factory_.device_address()!=device_.native_address())
        throw std::runtime_error("Native texture-pair call lacks its live renderer/framebuffer owner");
    memory_.validate_mutable_view(first_cell,20);
    for(auto byte:memory_.view(first_cell,20))if(byte!=std::byte{})
        throw std::invalid_argument("Native texture-pair creation would replace existing identities");
    auto prepared=factory_.create();std::array<NativeTu15TexturePairBinding,5> records{};
    for(unsigned i=0;i<4;++i) {
        const auto& image=prepared.image(i/2);
        records[i]={first_cell+i*4,6,64,64,image.image,image.view,image.image_backing,image.view_backing,false};
    }
    records[4]={first_cell+16,0x12,512,512,0,0,NativeRuntimeAddress{},NativeRuntimeAddress{},true};
    std::array<std::byte,20> bytes{};
    for(unsigned i=0;i<5;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte((first_handle+i)>>(j*8));
    memory_.write(first_cell,bytes);resources_.emplace(std::move(prepared));records_=records;
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]+=4;return true;
}
std::uint32_t NativeTu15TexturePairStartup::handle(std::uint32_t cell)const {
    if(!initialized())throw std::runtime_error("Native texture-pair publication is unavailable or retired");
    return first_handle+index(cell,first_cell,4);
}
const NativeTu15TexturePairBinding& NativeTu15TexturePairStartup::resolve(std::uint32_t value)const {
    if(!initialized())throw std::runtime_error("Native texture-pair publication is unavailable or retired");
    return records_[index(value,first_handle,1)];
}
NativeRuntimeAddress NativeTu15TexturePairStartup::native_address(std::uint32_t value)const {
    return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&resolve(value)))};
}
void NativeTu15TexturePairStartup::release() {
    if(std::this_thread::get_id()!=thread_||!resources_)throw std::invalid_argument("Native texture-pair cleanup has no owned publication");
    for(unsigned i=0;i<5;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)
        throw std::invalid_argument("Native texture-pair cleanup would replace another owner's publication");
    memory_.validate_mutable_view(first_cell,20);memory_.write(first_cell,std::array<std::byte,20>{});resources_.reset();records_={};
}
void mount_native_tu15_texture_pair_data(SourceRuntimeMemory& memory) {
    memory.mount(first_cell,std::array<std::byte,20>{},SourceRuntimeMemory::Access::ReadWrite,
        "source14830 native texture/surface opaque identities");
}
}
