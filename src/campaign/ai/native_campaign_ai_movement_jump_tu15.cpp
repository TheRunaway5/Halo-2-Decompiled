#include "halo2/native_campaign_ai_movement_jump_tu15.hpp"
#include "halo2/native_campaign_ai_jump_arc_tu15.hpp"
namespace halo2 {
void native_campaign_ai_movement_jump_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const NativeCampaignAiPerformServicesTu15 &,const SourcePhysicsContactRecordObserver &observe) {
    if(entry!=0xff1a0u)throw std::invalid_argument("Unknown native movement jump entry");
    if(!abi.floating_point||abi.active_mmx||abi.x87_depth)
        throw std::invalid_argument("Native movement jump requires retained FP and empty x87");
    const auto g=abi.gp;std::array<std::uint32_t,11> args{};
    for(unsigned n=0;n<args.size();++n){std::array<std::byte,4> b{};memory.read(g[4]+4u+n*4u,b);
        for(unsigned k=0;k<4;++k)args[n]|=std::to_integer<std::uint32_t>(b[k])<<(k*8u);}
    if(args[1]!=0x3f800000u||args[3]||args[4]||args[5]!=1u||args[6]||args[7]||args[8])
        throw std::invalid_argument("Native movement jump has an unowned FF1A0 option contract");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    bool result;
    try{result=native_campaign_ai_jump_arc_movement_profile_tu15(memory,g[0],g[1],args[0],g[6],args[2],args[9],args[10],fp,observe);}
    catch(...){publish();throw;}publish();abi.gp=g;abi.gp[0]=(g[0]&0xffffff00u)|std::uint32_t(result);abi.gp[4]=g[4]+0x30u;
}
}
