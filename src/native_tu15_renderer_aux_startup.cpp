#include "halo2/native_tu15_renderer_aux_startup.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
constexpr std::uint32_t first_cell=0x492304,first_handle=0xf9000401;
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
unsigned index(std::uint32_t value,std::uint32_t first,std::uint32_t stride) {
    if(value<first||(value-first)%stride||(value-first)/stride>=3)
        throw std::invalid_argument("Native auxiliary texture identity is outside this publication");
    return (value-first)/stride;
}
}
NativeTu15RendererAuxStartup::NativeTu15RendererAuxStartup(SourceRuntimeMemory& memory,NativeTu15GraphicsDevice& device,RendererAuxTextureFactory factory)
    :memory_(memory),device_(device),factory_(std::move(factory)){}
bool NativeTu15RendererAuxStartup::initialized()const {
    if(std::this_thread::get_id()!=thread_||!resources_||!resources_->valid()||!device_.initialized())return false;
    for(unsigned i=0;i<3;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)return false;return true;
}
bool NativeTu15RendererAuxStartup::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x1d0c0)return false;
    if(&bank!=&memory_||std::this_thread::get_id()!=thread_||call.tail||call.instruction!=0x127ba||word(memory_,abi.gp[4])!=0x127bf||resources_)
        throw std::invalid_argument("Native auxiliary texture call has a foreign caller or duplicate cold owner");
    if(!device_.initialized()||!factory_.valid()||factory_.device_address()!=device_.native_address())
        throw std::runtime_error("Native auxiliary texture call lacks its live renderer owner");
    memory_.validate_mutable_view(first_cell,12);
    for(auto byte:memory_.view(first_cell,12))if(byte!=std::byte{})
        throw std::invalid_argument("Native auxiliary texture creation would replace existing identities");
    auto prepared=factory_.create();std::array<NativeTu15RendererAuxBinding,3> records{};
    for(unsigned i=0;i<3;++i)records[i]={first_cell+i*4,4,prepared.image(i)};
    std::array<std::byte,12> bytes{};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte((first_handle+i)>>(j*8));
    memory_.write(first_cell,bytes);resources_.emplace(std::move(prepared));records_=records;
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]+=4;return true;
}
std::uint32_t NativeTu15RendererAuxStartup::handle(std::uint32_t cell)const {
    if(!initialized())throw std::runtime_error("Native auxiliary texture publication is unavailable or retired");return first_handle+index(cell,first_cell,4);
}
const NativeTu15RendererAuxBinding& NativeTu15RendererAuxStartup::resolve(std::uint32_t value)const {
    if(!initialized())throw std::runtime_error("Native auxiliary texture publication is unavailable or retired");return records_[index(value,first_handle,1)];
}
NativeRuntimeAddress NativeTu15RendererAuxStartup::native_address(std::uint32_t value)const {
    return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&resolve(value)))};
}
std::vector<std::byte> NativeTu15RendererAuxStartup::pixels(std::uint32_t value)const {
    (void)resolve(value);return resources_->pixels(index(value,first_handle,1));
}
void NativeTu15RendererAuxStartup::release() {
    if(std::this_thread::get_id()!=thread_||!resources_)throw std::invalid_argument("Native auxiliary texture cleanup has no owned publication");
    for(unsigned i=0;i<3;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)
        throw std::invalid_argument("Native auxiliary texture cleanup would replace another owner's publication");
    memory_.validate_mutable_view(first_cell,12);memory_.write(first_cell,std::array<std::byte,12>{});resources_.reset();records_={};
}
void mount_native_tu15_renderer_aux_data(SourceRuntimeMemory& memory) {
    memory.mount(first_cell,std::array<std::byte,12>{},SourceRuntimeMemory::Access::ReadWrite,"source1D0C0 native2D/volume/cube opaque identities");
}
}
