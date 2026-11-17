#include "halo2/native_campaign_ai_target_link_tu15.hpp"
#include "halo2/native_campaign_ai_perception_group_actor_tu15.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
void native_campaign_ai_target_link_call_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &) {
    if(entry!=0x274670u)throw std::invalid_argument("Unknown native target-link entry");
    if(!abi.floating_point||abi.active_mmx)
        throw std::invalid_argument("Native target-link requires retained x87 FP");
    const auto g=abi.gp;
    std::array<std::byte,4> bytes{};memory.read(g[4]+4u,bytes);
    std::uint32_t group{};for(unsigned i=0;i<4;++i)group|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    const auto publish=[&]{abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;};
    // The existing semantic owners publish through the real memory provider.
    // They do not tag child stores or materialize private CPU frames.
    try{native_campaign_ai_perception_group_link_actors_tu15(memory,g[0],group,fp,abi.x87_depth);}
    catch(...){publish();throw;}
    publish();abi.gp=g;abi.gp[4]=g[4]+8u;
}
}
