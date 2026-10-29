#include "halo2/native_campaign_screen_timed_tu15_binding.hpp"
#include "halo2/native_campaign_screen_timed_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> setups{},updates{};
struct Arguments final:NativeCampaignScreenDofArgumentsTu15 {
    SourcePhysicsContactMemory &memory;std::uint32_t sp;
    Arguments(SourcePhysicsContactMemory &m,std::uint32_t s):memory(m),sp(s){}
    std::uint32_t word(unsigned index)const override {
        std::array<std::byte,4> bytes;memory.read(sp+4u+index*4u,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
        return value;
    }
};
struct StatusPublication {
    SourcePhysicsWorldRuntimeAbi &abi;NativeCampaignScreenTimedFpTu15 &fp;
    ~StatusPublication(){abi.x87_status=std::uint16_t((fp.status&~0x3800u)|(abi.x87_status&0x3800u));abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};}
};
}
bool native_campaign_screen_timed_tu15_owns_entry(std::uint32_t entry){return entry==0x1fc10u || entry==0x1fd00u;}
void native_campaign_screen_timed_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_screen_timed_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native timed cinematic entry");
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>5 || (abi.eflags&0x400u))
        throw std::invalid_argument("Native timed cinematic requires ordinary FP stack, retained policy and clear direction");
    NativeCampaignScreenTimedFpTu15 fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    StatusPublication publication{abi,fp};
    std::optional<std::uint32_t> result;
    if(entry==0x1fc10u){setups.fetch_add(1,std::memory_order_relaxed);Arguments arguments(memory,abi.gp[4]);native_campaign_screen_dof_setup_tu15(memory,abi.xmm[0][0],arguments,fp,observe);}
    else {updates.fetch_add(1,std::memory_order_relaxed);result=native_campaign_screen_timed_update_tu15(memory,fp,observe);}
    std::array<std::byte,4> return_word;memory.read(abi.gp[4],return_word);
    abi.gp[4]+=entry==0x1fc10u?28u:4u;
    if(result)abi.gp[0]=*result;
}
NativeCampaignScreenTimedActivityTu15 native_campaign_screen_timed_activity_tu15(){return{setups.load(std::memory_order_relaxed),updates.load(std::memory_order_relaxed)};}
}
