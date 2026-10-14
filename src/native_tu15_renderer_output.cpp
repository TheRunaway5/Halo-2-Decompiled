#include "halo2/native_tu15_renderer_output.hpp"
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t at) {
    const auto bytes=memory.view(at,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
constexpr std::array<std::uint32_t,8> sites{0x1269e,0x126b0,0x126c5,0x126db,0x126ed,0x12702,0x12718,0x1272d};
constexpr std::array<std::uint32_t,8> methods{0x40338,0x40300,0x4033c,0x40340,0x40304,0x40344,0x40348,0x40350};
}
NativeTu15RendererOutput::NativeTu15RendererOutput(SourceRuntimeMemory& memory,
    NativeTu15GraphicsDevice& device,NativeTu15RendererDepth& depth,RendererOutputState control)
    :memory_(memory),device_(device),depth_(depth),control_(std::move(control)),thread_(std::this_thread::get_id()){}
const RendererOutputConfiguration& NativeTu15RendererOutput::configuration() const {
    if(!configuration_ || !device_.initialized() || !control_.valid())
        throw std::runtime_error("Native source output configuration has retired");
    return *configuration_;
}
bool NativeTu15RendererOutput::try_call(SourcePhysicsContactMemory& bank,
    const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x3ffdd0u)return false;
    if(&bank!=&memory_ || call.tail || std::this_thread::get_id()!=thread_ || calls_>=sites.size())
        throw std::invalid_argument("Native cold output caller has a foreign bank, thread or exhausted sequence");
    if(call.instruction!=sites[calls_] || word(memory_,abi.gp[4])!=sites[calls_]+5 ||
       abi.gp[1]!=methods[calls_] || abi.gp[2]!=renderer_cold_output_values[calls_] || depth_.state_calls()!=3)
        throw std::invalid_argument("Native cold output caller, value or order differs");
    if(!device_.initialized() || !control_.valid() || control_.device_address()!=device_.native_address())
        throw std::runtime_error("Native output state lacks its live renderer owner");
    (void)depth_.configuration();
    configuration_=control_.select(RendererOutputSetting(calls_),abi.gp[2]);
    ++calls_;abi.gp[4]+=4; // Native void API retains other GP/flags/FP state.
    return true;
}
void mount_native_tu15_renderer_output_data(SourceRuntimeMemory& memory) {
    // Exact virtual-tail scalar caches touched directly by original12560.
    // These are not an SDK driver bank or imported GPU/host pointers.
    for(auto address:std::array<std::uint32_t,12>{0x4116bc,0x411668,0x411660,0x41166c,
            0x411664,0x411670,0x411674,0x4116a0,0x411570,0x4116e8,0x411710,0x411714})
        memory.mount(address,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,
            "source12560 inline output-state scalar cache");
}
}
