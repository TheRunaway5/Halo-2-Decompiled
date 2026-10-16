#include "halo2/native_tu15_framebuffer_startup.hpp"
#include "halo2/source_weather_depth_alias.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
// Keep framebuffer identities disjoint from the palette's F9000101.
constexpr std::uint32_t first_cell=0x515e34,first_handle=0xf9000201;
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
unsigned index(std::uint32_t value,std::uint32_t first,std::uint32_t stride) {
    if(value<first || (value-first)%stride || (value-first)/stride>=8)
        throw std::invalid_argument("Framebuffer identity is outside this native publication");
    return (value-first)/stride;
}
}
NativeTu15FramebufferStartup::NativeTu15FramebufferStartup(SourceRuntimeMemory& memory,NativeTu15GraphicsDevice& device,NativeTu15RendererCoordinates& coordinates)
    :memory_(memory),device_(device),coordinates_(coordinates),thread_(std::this_thread::get_id()){}
bool NativeTu15FramebufferStartup::initialized()const {
    if(std::this_thread::get_id()!=thread_ || !resources_ || !resources_->valid() || !device_.initialized())return false;
    for(unsigned i=0;i<8;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)return false;
    return true;
}
bool NativeTu15FramebufferStartup::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x145e0u)return false;
    if(&bank!=&memory_ || std::this_thread::get_id()!=thread_ || call.tail || call.instruction!=0x1279fu ||
       word(memory_,abi.gp[4])!=0x127a4u || resources_ || coordinates_.calls()!=4 ||
       memory_.view(0x494330,1)[0]!=std::byte{1})
        throw std::invalid_argument("Native framebuffer publication has a foreign caller, order or duplicate owner");
    if(!device_.initialized())throw std::runtime_error("Native framebuffer publication lacks its actual renderer device");
    (void)coordinates_.configuration();memory_.validate_mutable_view(first_cell,32);
    for(const auto byte:memory_.view(first_cell,32))if(byte!=std::byte{})
        throw std::invalid_argument("Native framebuffer publication would replace existing resource identities");
    auto prepared=device_.framebuffer_aliases();
    for(unsigned i=0;i<8;++i) {
        const auto& binding=prepared.resolve(first_cell+i*4);
        if(!binding.image || !binding.view || !binding.width || !binding.height || !prepared.native_address(first_cell+i*4).value)
            throw std::runtime_error("Native framebuffer publication has an incomplete GPU resource owner");
    }
    std::array<std::byte,32> bytes{};
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)bytes[i*4+j]=std::byte((first_handle+i)>>(j*8));
    memory_.write(first_cell,bytes);resources_.emplace(std::move(prepared));
    // Declared native bool ABI. Only AL is the game return; the following
    // original TEST consumes it. Internal SDK/allocator clobbers stay private.
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]+=4;
    return true;
}
std::uint32_t NativeTu15FramebufferStartup::handle(std::uint32_t cell)const {
    if(!initialized())throw std::runtime_error("Native framebuffer publication is unavailable or retired");
    return first_handle+index(cell,first_cell,4);
}
const NativeTu15FramebufferBinding& NativeTu15FramebufferStartup::resolve(std::uint32_t value)const {
    if(!initialized())throw std::runtime_error("Native framebuffer publication is unavailable or retired");
    return resources_->resolve(first_cell+index(value,first_handle,1)*4);
}
NativeRuntimeAddress NativeTu15FramebufferStartup::native_address(std::uint32_t value)const {
    if(!initialized())throw std::runtime_error("Native framebuffer publication is unavailable or retired");
    return resources_->native_address(first_cell+index(value,first_handle,1)*4);
}
void NativeTu15FramebufferStartup::release() {
    if(std::this_thread::get_id()!=thread_ || !resources_)
        throw std::invalid_argument("Native framebuffer cleanup has no owned publication on this thread");
    for(unsigned i=0;i<8;++i)if(word(memory_,first_cell+i*4)!=first_handle+i)
        throw std::invalid_argument("Native framebuffer cleanup would replace another owner's publication");
    if(resources_->valid() && resources_->depth_producer().has_pending_submission())
        throw std::runtime_error("Native framebuffer cleanup requires its outstanding depth submission to retire");
    memory_.validate_mutable_view(first_cell,32);memory_.write(first_cell,std::array<std::byte,32>{});resources_.reset();
}
void mount_native_tu15_framebuffer_publication_data(SourceRuntimeMemory& memory) {
    memory.mount(first_cell,std::array<std::byte,32>{},SourceRuntimeMemory::Access::ReadWrite,
        "source145E0 native framebuffer opaque identity cells");
}
}
