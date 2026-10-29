#include "halo2/native_campaign_camera_startup_tu15_binding.hpp"
#include "halo2/native_campaign_camera_startup_tu15.hpp"
#include <stdexcept>
namespace halo2 {
bool native_campaign_camera_startup_tu15_owns_entry(std::uint32_t entry) {
    return entry==0x1888a0u||entry==0x157e10u||entry==0x13e9e0u||entry==0x381710u||entry==0x381730u||entry==0x381750u;
}
void native_campaign_camera_startup_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_camera_startup_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native camera startup entry");
    if(entry==0x157e10u && (abi.eflags&0x400u))throw std::invalid_argument("Native director construction requires the ordinary clear-direction caller");
    switch(entry) {
    case 0x1888a0:native_campaign_camera_input_storage_tu15(memory,observe);break;
    case 0x157e10:native_campaign_camera_director_storage_tu15(memory,observe);break;
    case 0x13e9e0:native_campaign_cinematic_storage_tu15(memory,observe);break;
    case 0x381710:native_campaign_camera_default_tu15(memory,NativeCampaignCameraDefaultTu15::Angle,observe);break;
    case 0x381730:native_campaign_camera_default_tu15(memory,NativeCampaignCameraDefaultTu15::Minimum,observe);break;
    case 0x381750:native_campaign_camera_default_tu15(memory,NativeCampaignCameraDefaultTu15::Maximum,observe);break;
    }
    std::array<std::byte,4> returned;memory.read(abi.gp[4],returned);abi.gp[4]+=4u;
}
}
