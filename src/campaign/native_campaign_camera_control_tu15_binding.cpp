#include "halo2/native_campaign_camera_control_tu15_binding.hpp"
#include "halo2/native_campaign_camera_control_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
class Argument final:public NativeCampaignCameraBooleanArgumentTu15 {
    const SourcePhysicsContactMemory &memory_;std::uint32_t stack_;
public:
    Argument(const SourcePhysicsContactMemory &m,std::uint32_t stack):memory_(m),stack_(stack){}
    std::uint8_t value()const override{std::byte value;memory_.read(stack_+4u,std::span(&value,1));return std::to_integer<std::uint8_t>(value);}
};
}
bool native_campaign_camera_control_tu15_owns_entry(std::uint32_t entry){return entry==0x158560u;}
void native_campaign_camera_control_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_camera_control_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native camera control entry");
    if(!abi.floating_point)throw std::invalid_argument("Native camera control requires its actual retained FP policy");
    NativeCampaignCameraCommandFpTu15 fp{abi.floating_point->mxcsr};Argument argument(memory,abi.gp[4]);
    calls.fetch_add(1,std::memory_order_relaxed);
    try{native_campaign_camera_control_tu15(memory,argument,fp,observe);}
    catch(...){abi.floating_point->mxcsr=fp.mxcsr;throw;}
    abi.floating_point->mxcsr=fp.mxcsr;
    std::array<std::byte,4> returned;memory.read(abi.gp[4],returned);abi.gp[4]+=8u;
}
std::uint64_t native_campaign_camera_control_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
