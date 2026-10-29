#include "halo2/native_campaign_camera_commands_tu15_binding.hpp"
#include "halo2/native_campaign_camera_commands_tu15.hpp"
#include <atomic>
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> resets{},stops{},fovs{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width) {
    std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
    std::uint32_t value{};for(unsigned i=0;i<width;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);return value;
}
class Arguments final:public NativeCampaignCameraFovArgumentsTu15 {
    const SourcePhysicsContactMemory &memory_;std::uint32_t stack_;
public:
    Arguments(const SourcePhysicsContactMemory &m,std::uint32_t stack):memory_(m),stack_(stack){}
    std::int16_t ticks()const override{return std::bit_cast<std::int16_t>(std::uint16_t(read(memory_,stack_+8u,2)));}
    std::uint32_t degrees_bits()const override{return read(memory_,stack_+4u,4);}
};
}
bool native_campaign_camera_commands_tu15_owns_entry(std::uint32_t entry){return entry==0x13ea40u||entry==0x13f560u||entry==0x16f5a0u;}
void native_campaign_camera_commands_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_camera_commands_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native camera command");
    if(entry==0x13f560u && (abi.eflags&0x400u))throw std::invalid_argument("Native cinematic stop requires the ordinary clear-direction caller");
    if(entry==0x13ea40u){resets.fetch_add(1,std::memory_order_relaxed);native_campaign_cinematic_reset_tu15(memory,observe);}
    else if(entry==0x13f560u){stops.fetch_add(1,std::memory_order_relaxed);native_campaign_cinematic_stop_tu15(memory,observe);}
    else {
        if(!abi.floating_point)throw std::invalid_argument("Native camera FOV needs its actual floating-point policy");
        NativeCampaignCameraCommandFpTu15 fp{abi.floating_point->mxcsr};Arguments arguments(memory,abi.gp[4]);
        fovs.fetch_add(1,std::memory_order_relaxed);
        try{native_campaign_camera_fov_tu15(memory,arguments,fp,observe);}
        catch(...){abi.floating_point->mxcsr=fp.mxcsr;throw;}
        abi.floating_point->mxcsr=fp.mxcsr;
    }
    (void)read(memory,abi.gp[4],4);abi.gp[4]+=entry==0x16f5a0u?12u:4u;
}
NativeCampaignCameraCommandActivityTu15 native_campaign_camera_command_activity_tu15(){return{resets.load(std::memory_order_relaxed),stops.load(std::memory_order_relaxed),fovs.load(std::memory_order_relaxed)};}
}
