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
    if(call.target!=0x3ff5d0u && call.target!=0x3ffdd0u && call.target!=0x3ffa90u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_)
        throw std::invalid_argument("Native cold depth caller has a foreign bank or thread");
    unsigned operation=0,cleanup=0;
    if(call.target==0x3ff5d0u && call.instruction==0x1265au &&
       word(memory_,abi.gp[4])==0x1265fu && word(memory_,abi.gp[4]+4)==2u && !configuration_) {
        operation=1;cleanup=8;
    } else if(configuration_ && call.target==0x3ffdd0u && call.instruction==0x12669u &&
       word(memory_,abi.gp[4])==0x1266eu && abi.gp[1]==0x4035cu && abi.gp[2]==1 && !state_calls_) {
        operation=2;cleanup=4;
    } else if(configuration_ && call.target==0x3ffdd0u && call.instruction==0x12682u &&
       word(memory_,abi.gp[4])==0x12687u && abi.gp[1]==0x40354u && abi.gp[2]==0x203 && state_calls_==1) {
        operation=3;cleanup=4;
    } else if(configuration_ && call.target==0x3ffa90u && call.instruction==0x12692u &&
       word(memory_,abi.gp[4])==0x12697u && word(memory_,abi.gp[4]+4)==0 && state_calls_==2) {
        operation=4;cleanup=8;
    } else throw std::invalid_argument("Native cold depth caller, value or order differs");
    if(!device_.initialized() || !control_.valid() || control_.device_address()!=device_.native_address())
        throw std::runtime_error("Native depth mode lacks its live renderer owner");
    switch(operation) {
        case 1:configuration_=control_.select(2);break;
        case 2:configuration_=control_.write_enable(1);++state_calls_;break;
        case 3:configuration_=control_.function(0x203);++state_calls_;break;
        case 4:configuration_=control_.bias(0);++state_calls_;break;
    }
    abi.gp[4]+=cleanup; // Native void graphics ABI; retain other GP/FP/flags.
    return true;
}
}
