#include "halo2/native_campaign_ai_behavior_reset_tu15_binding.hpp"
#include "halo2/native_campaign_ai_behavior_reset_tu15.hpp"
#include <atomic>
#include <stdexcept>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
class Arguments final:public NativeCampaignAiBehaviorResetArgumentsTu15 {
    const SourcePhysicsContactMemory &memory_;std::uint32_t sp_;
    std::uint32_t get(unsigned offset)const{
        std::array<std::byte,4> data;memory_.read(sp_+offset,data);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(data[n])<<(n*8u);
        return value;
    }
public:
    Arguments(const SourcePhysicsContactMemory &memory,std::uint32_t sp):memory_(memory),sp_(sp){}
    std::uint32_t actor()const override{return get(4);}
    std::uint32_t action()const override{return get(8);}
};
}
bool native_campaign_ai_behavior_reset_tu15_owns_entry(std::uint32_t entry){return entry==0x1c9ce0u;}
void native_campaign_ai_behavior_reset_tu15_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe){
    if(!native_campaign_ai_behavior_reset_tu15_owns_entry(entry))throw std::invalid_argument("Unowned native actor behavior reset");
    calls.fetch_add(1,std::memory_order_relaxed);Arguments arguments(memory,abi.gp[4]);
    const auto success=native_campaign_ai_behavior_reset_tu15(memory,arguments,observe);
    std::array<std::byte,4> return_word;memory.read(abi.gp[4],return_word);
    abi.gp[4]+=12u;
    // Actual callers consume boolAL. Preserve the dead upper word instead of
    // reconstructing the old implementation's private actor-address scratch.
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|std::uint32_t(success);
}
std::uint64_t native_campaign_ai_behavior_reset_activity_tu15(){return calls.load(std::memory_order_relaxed);}
}
