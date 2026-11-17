#include "halo2/native_campaign_ai_area_perform_tu15_binding.hpp"
#include "halo2/native_campaign_ai_area_perform_tu15.hpp"
#include "halo2/native_campaign_ai_resource_retirement_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace halo2 { namespace {
std::atomic<std::uint64_t> perform_calls{},release_calls{};
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> bytes{};memory.read(address,bytes);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);
    return value;
}
struct Protection final:NativeCampaignAiResourceProtectionTu15 {
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> &owner;
    explicit Protection(const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> &o):owner(o){}
    void protect_pages(std::uint32_t address,std::uint32_t bytes,std::uint32_t protection)const override {
        if(!owner)throw std::runtime_error("Native 1BBEE0 workspace requires its real page owner");
        owner(address,bytes,protection);
    }
};
// Reference census only: unported callees run their translated AI-frame body
// so a census can measure the executed slice. Never the production path.
// "translated-with-26b220" also routes the native 26B220 slice to its body,
// for a live digest differential against "translated"; likewise
// "translated-with-26ba30" for native 26BA30. "differential-26b220" and
// "differential-26ba30" compare each native call against its body.
unsigned translated_callees() {
    static const unsigned selected=[]{
        const auto *p=std::getenv("HALO2_REFERENCE_AI_PERFORM_CALLEES");
        if(!p)return 0u;
        if(std::strcmp(p,"translated")==0)return 1u;
        if(std::strcmp(p,"translated-with-26b220")==0)return 2u;
        if(std::strcmp(p,"differential-26b220")==0)return 3u;
        if(std::strcmp(p,"translated-with-26ba30")==0)return 4u;
        if(std::strcmp(p,"differential-26ba30")==0)return 5u;
        if(std::strcmp(p,"differential-26a380")==0)return 6u;
        return 0u;
    }();
    return selected;
}
}
bool native_campaign_ai_area_perform_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x1bbee0u && call.instruction==0x1b0ad9u && !call.tail;
}
void native_campaign_ai_area_perform_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> &protect,
    const std::function<void(std::uint32_t,SourcePhysicsWorldRuntimeAbi &)> &translated,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_ai_area_perform_tu15_owns_call(call))
        throw std::invalid_argument("Native 1BBEE0 perform requires its actual 1B0AD9 descriptor call");
    const auto sp=abi.gp[4];
    if(read(memory,sp)!=0x1b0adbu)throw std::runtime_error("Native 1BBEE0 perform lost its actual 1B0ADB return");
    perform_calls.fetch_add(1,std::memory_order_relaxed);
    Protection protection(protect);
    NativeCampaignAiPerformServicesTu15 services{&protection,{}};
    services.reference_candidate_preparation=translated_callees()==2u;
    static std::uint64_t differential_matches{},collection_matches{};
    if(translated_callees()==3u)services.differential_candidate_preparation=&differential_matches;
    services.reference_candidate_collection=translated_callees()==4u;
    if(translated_callees()==5u)services.differential_candidate_collection=&collection_matches;
    // Every reference mode except the 26A380 differential keeps the whole
    // translated 26A380 body, so a census still measures its executed slice.
    static std::array<std::uint64_t,2> evaluation_outcomes{};
    services.reference_goal_evaluation=translated_callees() && translated_callees()!=6u;
    if(translated_callees()==6u)services.differential_goal_evaluation=&evaluation_outcomes;
    if(translated_callees())services.callee=[&](std::uint32_t,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &state) {
        if(!translated)throw std::runtime_error("Native 1BBEE0 reference census has no translated owner");
        translated(entry,state);
    };
    static const bool trace=[]{const auto *p=std::getenv("HALO2_NATIVE_AI_PERFORM_TRACE");return p && *p;}();
    std::uint32_t actor{},row{},before{};
    if(trace) {
        actor=read(memory,sp+4u);
        row=read(memory,read(memory,0x5020c8u)+0x44u)+(actor&0xffffu)*0x888u;
        before=read(memory,row+0x40u)&0xffu;
    }
    try {native_campaign_ai_area_perform_tu15(memory,abi,services,observe);}
    catch(const std::exception &error) {
        if(trace)std::fprintf(stderr,"native 1BBEE0 actor=%08x active=%u refused: %s\n",actor,before,error.what());
        throw;
    }
    if(trace)std::fprintf(stderr,"native 1BBEE0 actor=%08x active=%u goal=%08x mode=%u excluded=%u -> al=%u active=%u 26b220_differential=%llu 26ba30_differential=%llu 26a380_differential=%llu/%llu\n",
        actor,before,read(memory,row+0x418u),read(memory,row+0x504u)&0xffffu,read(memory,row+7u)&0xffu,
        abi.gp[0]&0xffu,read(memory,row+0x40u)&0xffu,static_cast<unsigned long long>(differential_matches),
        static_cast<unsigned long long>(collection_matches),static_cast<unsigned long long>(evaluation_outcomes[0]),
        static_cast<unsigned long long>(evaluation_outcomes[1]));
    if(abi.gp[4]!=sp+12u)throw std::runtime_error("Native 1BBEE0 perform returned an unbalanced frame");
}
bool native_campaign_ai_area_perform_release_tu15_owns_call(const SourcePhysicsContactRuntimeCall &call) {
    return call.target==0x1bc0a0u && call.instruction==0x1b0f56u && !call.tail;
}
void native_campaign_ai_area_perform_release_tu15_run(SourcePhysicsContactMemory &memory,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) {
    if(!native_campaign_ai_area_perform_release_tu15_owns_call(call))
        throw std::invalid_argument("Native 1BC0A0 requires its actual 1B0F56 descriptor call");
    const auto sp=abi.gp[4];
    if(read(memory,sp)!=0x1b0f58u)throw std::runtime_error("Native 1BC0A0 lost its actual 1B0F58 return");
    release_calls.fetch_add(1,std::memory_order_relaxed);
    native_campaign_ai_area_perform_release_tu15(memory,abi,observe);
}
std::uint64_t native_campaign_ai_area_perform_calls_tu15(){return perform_calls.load(std::memory_order_relaxed);}
std::uint64_t native_campaign_ai_area_perform_release_calls_tu15(){return release_calls.load(std::memory_order_relaxed);}
}
