#include "halo2/native_campaign_screen_lifecycle_tu15.hpp"
#include "halo2/source_execution_campaign_screen.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <ranges>

namespace {
using namespace halo2;
using Kind=SourceExecutionCallKind;
static_assert(unsigned(Kind::MatchStepViews)==32 && unsigned(Kind::AdmittedCamera)==31);
static_assert(unsigned(Kind::StopCampaignScreen)==33 && unsigned(Kind::AcknowledgeCampaignScreenCapture)==34);
static_assert(!source_execution_call_has_result(Kind::StopCampaignScreen));
static_assert(!source_execution_call_has_result(Kind::AcknowledgeCampaignScreenCapture));
void check(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
template<class F>void rejects(F &&function) {
    bool failed=false;try{function();}catch(const std::exception &){failed=true;}
    check(failed,"Invalid campaign screen journal work was accepted");
}
std::array<std::byte,4> word(std::uint32_t value) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8u));
    return bytes;
}
void prepare(SourceRuntimeMemory &memory) {
    memory.mount(0x515ec0,word(0x2000000),SourceRuntimeMemory::Access::ReadWrite,"Actual cinematic interface pointer");
    memory.mount(0x4c1eb0,std::vector<std::byte>(16),SourceRuntimeMemory::Access::ReadWrite,"Actual texture capture target state");
    memory.mount(0x2000000,std::vector<std::byte>(0x3fc,std::byte{0x55}),SourceRuntimeMemory::Access::ReadWrite,"Actual cinematic interface record");
    memory.write(0x4c1eb0,word(1));
}
void invoke(SourceRuntimeMemory &memory,Kind kind) {
    // Same empty-operand root scopes as the actual PhysicsLiveWorld methods.
    SourceExecutionCallScope journal(&memory,kind,[]{return std::vector<std::byte>{};});
    SourceExecutionCall call;call.kind=kind;
    check(source_execution_campaign_screen_call(call,{
        [&]{native_campaign_screen_stop_tu15(memory);},
        [&]{native_campaign_screen_capture_complete_tu15(memory);}}),"Unknown native screen operation");
}
SourceExecutionCampaignScreenTarget target(SourceRuntimeMemory &memory) {
    return {[&]{invoke(memory,Kind::StopCampaignScreen);},
            [&]{invoke(memory,Kind::AcknowledgeCampaignScreenCapture);}};
}
}
int main()try {
    SourceRuntimeMemory authority,replay;prepare(authority);prepare(replay);
    const std::array kinds{Kind::AcknowledgeCampaignScreenCapture,Kind::StopCampaignScreen,Kind::AcknowledgeCampaignScreenCapture};
    SourceExecutionInputs tape;tape.track_calls();
    {SourceExecutionInputBinding binding(authority,tape);
        invoke(authority,kinds[0]);
        const auto interface=authority.view(0x2000000,0x3fc);
        for(unsigned i=0;i<interface.size();++i)
            check(interface[i]==(i==0xa0u?std::byte{0}:std::byte{0x55}),"Capture acknowledgement dispatched the wrong native kernel");
        check(authority.view(0x4c1ebd,1)[0]==std::byte{0},"Capture acknowledgement invalidated the target");
        invoke(authority,kinds[1]);
        check(std::ranges::all_of(authority.view(0x2000000,0x3fc),[](auto byte){return byte==std::byte{0};}),"Stop did not reset the real screen bank");
        invoke(authority,kinds[2]);}
    check(tape.finish().empty(),"Native screen work fabricated an external input");
    check(tape.calls().size()==kinds.size(),"Native screen calls were not captured exactly once");
    for(unsigned i=0;i<kinds.size();++i) {
        const auto &call=tape.calls()[i];
        check(call.kind==kinds[i] && call.arguments.empty() && call.result.empty() &&
            !call.input_begin && !call.input_end,"Native screen call fabricated operands/results");
        check(source_execution_campaign_screen_call(call),"Native screen schema validation failed");
    }
    SourceExecutionInputs transcript({},tape.calls());
    {SourceExecutionInputBinding binding(replay,transcript);
        for(const auto &call:tape.calls())check(source_execution_campaign_screen_call(call,target(replay)),"Native screen replay dispatch failed");}
    transcript.finish();
    for(const auto [address,bytes]:std::array{std::pair{0x2000000u,0x3fcu},std::pair{0x4c1eb0u,16u}})
        check(std::ranges::equal(authority.view(address,bytes),replay.view(address,bytes)),"Native screen replay state differs");
    check(replay.view(0x4c1ebd,1)[0]==std::byte{1} && replay.view(0x20000a0,1)[0]==std::byte{0},"Native screen kernels did not mutate their actual state");

    unsigned mutations{};
    const SourceExecutionCampaignScreenTarget counted{[&]{++mutations;},[&]{++mutations;}};
    for(const auto kind:kinds) {
        SourceExecutionCall bad;bad.kind=kind;bad.arguments={std::byte{1}};
        rejects([&]{source_execution_campaign_screen_call(bad,counted);});
        bad.arguments.clear();bad.result={std::byte{0}};
        rejects([&]{source_execution_campaign_screen_call(bad,counted);});
    }
    check(!mutations,"Malformed void call mutated native state before schema rejection");
    SourceExecutionCall unknown;unknown.kind=static_cast<Kind>(255);
    check(!source_execution_campaign_screen_call(unknown,counted),"Screen decoder claimed an unknown call kind");
    rejects([&]{SourceExecutionInputs invalid({},std::span(&unknown,1));});
    SourceExecutionInputs unknown_capture;unknown_capture.track_calls();
    {SourceExecutionInputBinding binding(authority,unknown_capture);
        SourceExecutionCallScope call(&authority,unknown.kind,[]{return std::vector<std::byte>{};});}
    rejects([&]{unknown_capture.finish();});

    SourceRuntimeMemory wrong;prepare(wrong);const auto before=std::vector<std::byte>(wrong.view(0x2000000,0x3fc).begin(),wrong.view(0x2000000,0x3fc).end());
    SourceExecutionInputs reordered({},tape.calls());
    {SourceExecutionInputBinding binding(wrong,reordered);rejects([&]{invoke(wrong,Kind::StopCampaignScreen);});}
    rejects([&]{reordered.finish();});
    check(std::ranges::equal(before,wrong.view(0x2000000,0x3fc)),"Reordered replay mutated native state before rejection");
    SourceExecutionInputs trailing({},tape.calls());
    {SourceExecutionInputBinding binding(wrong,trailing);invoke(wrong,kinds[0]);}
    rejects([&]{trailing.finish();});
    SourceExecutionInputs invented_result;invented_result.track_calls();
    {SourceExecutionInputBinding binding(wrong,invented_result);
        SourceExecutionCallScope call(&wrong,Kind::StopCampaignScreen,[]{return std::vector<std::byte>{};});
        call.complete([]{return std::vector<std::byte>{std::byte{0}};});}
    rejects([&]{invented_result.finish();});
    SourceExecutionInputs raw_capture;raw_capture.track_calls();
    {SourceExecutionInputBinding binding(wrong,raw_capture);native_campaign_screen_stop_tu15(wrong);}
    rejects([&]{raw_capture.finish();});
    SourceExecutionInputs raw_replay({},std::span<const SourceExecutionCall>{});
    {SourceExecutionInputBinding binding(wrong,raw_replay);rejects([&]{native_campaign_screen_stop_tu15(wrong);});}
    rejects([&]{raw_replay.finish();});
    std::cout<<"PASS native screen journal capture/replay with real kernels; empty void schema, unknown/reorder/trailing/result/mutation gates\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
