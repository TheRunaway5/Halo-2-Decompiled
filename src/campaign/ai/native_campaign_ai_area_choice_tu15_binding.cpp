#include "halo2/native_campaign_ai_area_choice_tu15_binding.hpp"
#include "halo2/native_campaign_ai_area_choice_tu15.hpp"
#include <atomic>
#include <cstring>

namespace halo2 { namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
void publish_argument(SourcePhysicsContactMemory &memory,std::uint32_t address,std::uint32_t value,
    const SourcePhysicsContactRecordObserver &observe) {
    SourcePhysicsContactRecordStore event{0x1bbecdu,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
struct Inputs final:NativeCampaignAiAreaChoiceInputsTu15 {
    const SourcePhysicsContactMemory &memory;std::uint32_t sp;
    Inputs(const SourcePhysicsContactMemory &m,std::uint32_t p):memory(m),sp(p){}
    std::uint32_t actor()const override{return read(memory,sp+4u);}
    std::uint32_t selection()const override{return read(memory,sp+8u);}
};
void retain_pending(SourcePhysicsWorldRuntimeAbi &abi,const NativeCampaignAiAreaScorePendingTu15 &pending) {
    // These are the real mathematical quantities carried by the typed kernel
    // on a rejected retained read. Prepend their exact80-bit values to the
    // retained caller values; no instruction/private arithmetic state is made.
    static_assert(sizeof(long double)>=10);
    if(pending.count>2u || abi.x87_depth+pending.count>abi.x87.size())
        throw std::runtime_error("Authored area score pending values exceed the admitted caller capacity");
    for(unsigned n=abi.x87_depth;n;--n)
        std::memcpy(&abi.x87[n-1u+pending.count],&abi.x87[n-1u],sizeof(long double));
    for(unsigned n=0;n<pending.count;++n) {
        std::memset(&abi.x87[n],0,sizeof(long double));
        std::memcpy(&abi.x87[n],pending.quantities[n].data(),10);
    }
    abi.x87_depth+=pending.count;
}
}
bool native_campaign_ai_area_choice_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x1bbec0u && call.instruction==0x1b0d1cu && !call.tail;
}
void native_campaign_ai_area_choice_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_ai_area_choice_tu15_owns_call(call))
        throw std::invalid_argument("Authored area-choice binding requires its actual1B0D1C descriptor call");
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>6u)
        throw std::invalid_argument("Authored area-choice binding requires its retained FP policy and two free temporaries");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi &abi;NativeTu15PhysicsFrameFp &fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    calls.fetch_add(1,std::memory_order_relaxed);
    const auto sp=abi.gp[4];
    const auto request=read(memory,sp+8u);
    const auto selection=native_campaign_ai_prepare_area_choice_tu15(memory,request,observe);
    // The wrapper publishes its actual retained argument before the tail.
    // Actor and each later selection pointer remain lazy in the shared core.
    publish_argument(memory,sp+8u,selection,observe);
    Inputs inputs(memory,sp);
    bool result;
    try {result=native_campaign_ai_choose_area_tu15(memory,inputs,fp,abi.x87_depth,observe);}
    catch(const NativeCampaignAiAreaScorePendingErrorTu15 &error){retain_pending(abi,error.pending());throw;}
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|unsigned(result);
    if(read(memory,sp)!=0x1b0d1eu)
        throw std::runtime_error("Authored area-choice binding lost its actual1B0D1E return");
    abi.gp[4]=sp+12u;
}
std::uint64_t native_campaign_ai_area_choice_calls_tu15(){return calls.load(std::memory_order_relaxed);}
}
