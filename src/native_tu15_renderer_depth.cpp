#include "halo2/native_tu15_renderer_depth.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
NativeTu15RendererDepth::NativeTu15RendererDepth(SourceRuntimeMemory& memory,
    NativeTu15GraphicsDevice& device,RendererDepthMode control)
    :memory_(memory),device_(device),control_(std::move(control)),thread_(std::this_thread::get_id()){}
const RendererDepthConfiguration& NativeTu15RendererDepth::configuration() const {
    if(!configuration_ || !device_.initialized() || !control_.valid())
        throw std::runtime_error("Native source depth configuration has retired");
    return *configuration_;
}
bool NativeTu15RendererDepth::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x3ff5d0u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_ ||
       call.instruction!=0x1265au || word(memory_,abi.gp[4])!=0x1265fu ||
       word(memory_,abi.gp[4]+4)!=2u || configuration_)
        throw std::invalid_argument("Native cold depth-mode caller or mode differs");
    if(!device_.initialized() || !control_.valid() || control_.device_address()!=device_.native_address())
        throw std::runtime_error("Native depth mode lacks its live renderer owner");
    configuration_=control_.select(2);
    abi.gp[4]+=8; // Native void graphics ABI; remove return and one argument.
    return true;
}
}
