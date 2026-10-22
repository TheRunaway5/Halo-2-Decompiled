#include "halo2/native_tu15_renderer_coordinates.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
constexpr std::array<std::uint32_t,4> sites{0x1276b,0x12777,0x12783,0x1278f};
}
NativeTu15RendererCoordinates::NativeTu15RendererCoordinates(SourceRuntimeMemory& memory,
    NativeTu15GraphicsDevice& device,NativeTu15RendererOutput& output,RendererTextureCoordinates control)
    :memory_(memory),device_(device),output_(output),control_(std::move(control)),thread_(std::this_thread::get_id()){}
const RendererTextureCoordinateConfiguration& NativeTu15RendererCoordinates::configuration() const {
    if(!configuration_ || !device_.initialized() || !control_.valid())
        throw std::runtime_error("Native source texture-coordinate configuration has retired");
    return *configuration_;
}
bool NativeTu15RendererCoordinates::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x3ffe00u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_ || calls_>=sites.size())
        throw std::invalid_argument("Native coordinate caller has a foreign bank, thread or exhausted sequence");
    if(call.instruction!=sites[calls_] || word(memory_,abi.gp[4])!=sites[calls_]+5 ||
       abi.gp[6]!=calls_ || word(memory_,abi.gp[4]+4)!=calls_ || output_.calls()!=8)
        throw std::invalid_argument("Native coordinate caller, stage, input or order differs");
    if(!device_.initialized() || !control_.valid() || control_.device_address()!=device_.native_address())
        throw std::runtime_error("Native coordinates lack their live renderer owner");
    (void)output_.configuration();configuration_=control_.select(calls_,calls_);
    ++calls_;abi.gp[4]+=8; // Native void ABI; SDK internal clobbers are not imported.
    return true;
}
}
