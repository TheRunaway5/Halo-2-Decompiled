#include "halo2/native_tu15_renderer_constants.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
NativeTu15RendererConstants::NativeTu15RendererConstants(SourceRuntimeMemory& memory,
    NativeTu15GraphicsDevice& device,RendererConstantMode control)
    :memory_(memory),device_(device),control_(std::move(control)),thread_(std::this_thread::get_id()){}
const RendererConstantConfiguration& NativeTu15RendererConstants::configuration() const {
    if(!configuration_ || !device_.initialized() || !control_.valid())
        throw std::runtime_error("Native source constant configuration has retired");
    return *configuration_;
}
bool NativeTu15RendererConstants::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x400ef0u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_ ||
       call.instruction!=0x12653u || word(memory_,abi.gp[4])!=0x12658u || abi.gp[0]!=0x11u || configuration_)
        throw std::invalid_argument("Native cold constant-mode caller or mode differs");
    if(!device_.initialized() || !control_.valid() || control_.device_address()!=device_.native_address())
        throw std::runtime_error("Native constant mode lacks its live renderer owner");
    configuration_=control_.select(abi.gp[0]);
    abi.gp[4]+=4; // Native void graphics ABI; preserve other GP/FP/flags.
    return true;
}
}
