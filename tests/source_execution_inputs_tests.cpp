#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/simulation.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_execution_model_inputs.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include <bit>
#include <atomic>
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid native input transcript accepted");}
}
int main()try {
    using K=SourceExecutionInputKind;SourceRuntimeMemory first,second;
    check(source_execution_input(first,K::Milliseconds,0,[]{return 13;})==13,"Unbound native input changed");
    SourceExecutionInputs capture;
    const std::array<SourceExecutionInput,9> expected{{{K::Milliseconds,0,UINT32_MAX},{K::TimestampCounter,0,UINT64_MAX},
        {K::ReciprocalSquareRoot,0x40800000,0x3f000000},{K::UtcFiletime,0,132000000000000000ull},{K::ResultWallSeconds,0,1700000000},
        {K::GraphicsRead1,0xf7100000u,255},{K::GraphicsRead2,0xf7100000u,65535},
        {K::GraphicsRead4,0xf7100000u,UINT32_MAX},{K::GraphicsRead8,0xf7100000u,UINT64_MAX}}};
    {SourceExecutionInputBinding binding(first,capture);
        rejects([&]{SourceExecutionInputBinding duplicate(first,capture);});
        rejects([&]{SourceExecutionInputBinding cross(second,capture);});
        rejects([&]{capture.finish();});
        for(const auto &entry:expected)check(source_execution_input(first,entry.kind,entry.operand,[&]{return entry.value;})==entry.value,"Capture replaced a real external input");
    }
    const auto entries=capture.finish();check(std::ranges::equal(entries,expected),"Captured external input order or bits changed");
    rejects([&]{SourceExecutionInputBinding closed(first,capture);});
    SourceExecutionInputs replay(entries);unsigned actual_reads=0;
    {SourceExecutionInputBinding binding(second,replay);for(const auto &entry:expected)
        check(source_execution_input(second,entry.kind,entry.operand,[&]{++actual_reads;return 0;})==entry.value,"Replay changed producer input bits");}
    replay.finish();check(!actual_reads,"Replay sampled the replacement machine's inputs");
    SourceExecutionInputs wrong(entries);rejects([&]{wrong.sample(K::Milliseconds,1,[]{return 0;});});rejects([&]{wrong.finish();});
    SourceExecutionInputs reordered(entries);rejects([&]{reordered.sample(K::TimestampCounter,0,[]{return 0;});});rejects([&]{reordered.finish();});
    SourceExecutionInputs short_phase(entries);short_phase.sample(K::Milliseconds,0,[]{return 0;});rejects([&]{short_phase.finish();});
    SourceExecutionInputs empty(std::span<const SourceExecutionInput>{});rejects([&]{empty.sample(K::Milliseconds,0,[]{return 0;});});
    for(auto bad:std::array{SourceExecutionInput{K::Milliseconds,1,0},SourceExecutionInput{K::ResultWallSeconds,0,UINT64_MAX},
        SourceExecutionInput{K::ReciprocalSquareRoot,1,UINT64_MAX},SourceExecutionInput{static_cast<K>(255),0,0},
        SourceExecutionInput{K::GraphicsRead1,0,256},SourceExecutionInput{K::GraphicsRead2,0,65536},
        SourceExecutionInput{K::GraphicsRead4,0,UINT64_MAX},SourceExecutionInput{K::GraphicsRead2,UINT32_MAX,0},
        SourceExecutionInput{K::GraphicsRead4,UINT32_MAX,0},SourceExecutionInput{K::GraphicsRead8,UINT32_MAX,0}})
        rejects([&]{SourceExecutionInputs invalid(std::span(&bad,1));});
    SourceExecutionInputs overflow(1);unsigned count=0;
    check(overflow.sample(K::Milliseconds,0,[&]{return ++count;})==1,"First bounded capture changed input");
    check(overflow.sample(K::Milliseconds,0,[&]{return ++count;})==2,"Overflow interrupted the real host input");
    check(overflow.sample(K::Milliseconds,0,[&]{return ++count;})==3,"Failed capture stopped the host provider");rejects([&]{overflow.finish();});
    SourceExecutionInputs unavailable;bool original_error=false;
    try{unavailable.sample(K::UtcFiletime,0,[]()->std::uint64_t{throw std::runtime_error("actual clock unavailable");});}
    catch(const std::runtime_error &e){original_error=std::string(e.what())=="actual clock unavailable";}
    check(original_error,"Capture changed the original provider failure");rejects([&]{unavailable.finish();});
    std::atomic<bool> denied=false;SourceExecutionInputs owned;
    {std::jthread worker([&]{try{owned.sample(K::Milliseconds,0,[]{return 0;});}catch(const std::logic_error &){denied=true;}});}
    check(denied,"Another executor mutated a live transcript");
    using C=SourceExecutionCallKind;const std::array<std::byte,4> word{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    first.mount(0x1000,word,SourceRuntimeMemory::Access::ReadWrite,"journal caller storage");
    second.mount(0x1000,word,SourceRuntimeMemory::Access::ReadWrite,"journal replay caller storage");
    unsigned permission_checks{},write_observations{};
    source_runtime_register_write_guard(first,&permission_checks,[&](auto,auto){++permission_checks;});
    source_runtime_register_write_observer(first,&write_observations,[&](auto,auto){++write_observations;});
    SourceExecutionInputs validation;validation.track_calls();
    {SourceExecutionInputBinding binding(first,validation);first.validate_mutable_view(0x1000,4);}
    validation.finish();check(validation.calls().empty()&&permission_checks==1&&!write_observations,
        "Permission-only admission was recorded as a write or skipped its access policy");
    check(std::ranges::equal(first.view(0x1000,4),word),"Permission-only admission changed source storage");
    SourceExecutionInputs validation_replay({},validation.calls());validation_replay.track_calls();
    {SourceExecutionInputBinding binding(second,validation_replay);second.validate_mutable_view(0x1000,4);}
    validation_replay.finish();
    (void)first.mutable_view(0x1000,4);
    check(write_observations==1,"A real mutable borrower skipped its write observer");
    source_runtime_register_write_guard(first,&validation,[](auto,auto){throw std::logic_error("Protected source resource");});
    rejects([&]{first.validate_mutable_view(0x1000,4);});
    check(write_observations==1,"A denied permission-only query emitted a write observation");
    source_runtime_unregister_write_guard(first,&validation);
    SourceExecutionInputs borrow_replay({},std::span<const SourceExecutionCall>{});borrow_replay.track_calls();
    {SourceExecutionInputBinding binding(first,borrow_replay);rejects([&]{first.mutable_view(0x1000,4);});}
    rejects([&]{borrow_replay.finish();});
    source_runtime_unregister_write_observer(first,&write_observations);
    source_runtime_unregister_write_guard(first,&permission_checks);
    std::uint64_t mutations{};SourceExecutionInputs journal;journal.track_calls({&mutations,nullptr});unsigned encodings{};
    {SourceExecutionInputBinding binding(first,journal);
        SourceExecutionCallScope call(&first,C::BeginModelFrame,[&]{++encodings;return std::vector<std::byte>{};});
        SourceExecutionCallScope nested(&first,C::BeginModelView,[&]{++encodings;return std::vector<std::byte>{};});
        ++mutations;first.write(0x1000,word);}
    journal.finish();check(encodings==1&&journal.calls().size()==1,"Nested native calls escaped their complete root operation");
    SourceExecutionInputs call_replay({},journal.calls());call_replay.track_calls({&mutations,nullptr});
    {SourceExecutionInputBinding binding(second,call_replay);
        SourceExecutionCallScope call(&second,C::BeginModelFrame,[]{return std::vector<std::byte>{};});++mutations;second.write(0x1000,word);}
    call_replay.finish();
    SourceExecutionInputs unknown;unknown.track_calls({&mutations,nullptr});
    {SourceExecutionInputBinding binding(first,unknown);++mutations;}rejects([&]{unknown.finish();});
    SourceExecutionInputs raw;raw.track_calls();
    {SourceExecutionInputBinding binding(first,raw);first.write(0x1000,word);first.mount(0x2000,word,SourceRuntimeMemory::Access::ReadWrite,"unrecorded caller bank");}
    rejects([&]{raw.finish();});check(first.view(0x2000,4).size()==4,"Unrecorded capture interrupted real caller allocation");
    SourceExecutionInputs raw_replay({},std::span<const SourceExecutionCall>{});raw_replay.track_calls();
    {SourceExecutionInputBinding binding(second,raw_replay);
        rejects([&]{second.write(0x1000,word);});
        rejects([&]{second.reserve(0x3000,4096,4096,"unrecorded replay reserve");});}
    rejects([&]{raw_replay.finish();});rejects([&]{second.view(0x3000,1);});second.write(0x1000,word);
    SourceExecutionInputs denied_write;denied_write.track_calls();denied=false;
    {SourceExecutionInputBinding binding(first,denied_write);std::jthread thread([&]{try{first.write(0x1000,word);}catch(const std::logic_error &){denied=true;}});}
    check(denied,"A foreign executor changed journaled native memory");denied_write.finish();
    SourceExecutionInputs failed_arguments;failed_arguments.track_calls();unsigned real_calls{};
    {SourceExecutionInputBinding binding(first,failed_arguments);
        SourceExecutionCallScope call(&first,C::BeginModelFrame,[]()->std::vector<std::byte>{throw std::bad_alloc();});++real_calls;}
    rejects([&]{failed_arguments.finish();});check(real_calls==1,"Call recording failure interrupted or repeated actual host work");
    SourceExecutionInputs instrumentation;instrumentation.track_calls();unsigned instrumentation_work{};
    {SourceExecutionInputBinding binding(first,instrumentation);
        SourceExecutionCallScope call(&first,C::BeginModelFrame,[]{return std::vector<std::byte>{};});
        instrumentation.invalidate_recording();++instrumentation_work;first.write(0x1000,word);}
    rejects([&]{instrumentation.finish();});check(instrumentation_work==1,"Recorder instrumentation failure interrupted actual host work");
    SourceExecutionInputs instrumentation_replay(std::span<const SourceExecutionInput>{});
    rejects([&]{instrumentation_replay.invalidate_recording();});rejects([&]{instrumentation_replay.finish();});
    const auto nan=std::bit_cast<float>(0x7fc00001u);
    SourceExecutionCall position{C::ObserverPosition,source_execution_arguments(3u,Vec3{nan,-0.f,std::bit_cast<float>(1u)})};
    validate_source_execution_calls(std::span(&position,1));
    check(position.arguments[4]==std::byte{1}&&position.arguments[7]==std::byte{0x7f}&&position.arguments[11]==std::byte{0x80}&&position.arguments[12]==std::byte{1},"Native call float bits were normalized");
    auto trailing=position;trailing.arguments.push_back(std::byte{});rejects([&]{validate_source_execution_calls(std::span(&trailing,1));});
    SourceExecutionCall controls{C::MatchStep,source_execution_arguments(std::array<Controls,16>{})};controls.arguments.back()=std::byte{2};
    rejects([&]{validate_source_execution_calls(std::span(&controls,1));});
    SourceExecutionCall invalid{static_cast<C>(255),{}};rejects([&]{SourceExecutionInputs bad({},std::span(&invalid,1));});
    SourceExecutionCall stray{C::BeginModelFrame,{},0,1};rejects([&]{SourceExecutionInputs bad({},std::span(&stray,1));});
    const SourceExecutionInput clock{K::Milliseconds,0,17};
    SourceExecutionInputs boundary(std::span(&clock,1),std::span(&stray,1));boundary.track_calls();
    {SourceExecutionInputBinding binding(first,boundary);SourceExecutionCallScope call(&first,C::BeginModelFrame,[]{return std::vector<std::byte>{};});}
    rejects([&]{boundary.finish();});
    SourceExecutionInputs returns;returns.track_calls();
    const auto call_args=source_execution_arguments(0x335b0u,std::span<const std::uint32_t>{});
    {SourceExecutionInputBinding binding(first,returns);SourceExecutionCallScope call(&first,C::CallActualSource,[&]{return call_args;});
        ++real_calls;call.complete([]{return source_execution_result(17u);});}
    returns.finish();check(returns.calls().front().result==source_execution_result(17u),"Actual native result was not sealed");
    SourceExecutionInputs result_replay({},returns.calls());result_replay.track_calls();
    {SourceExecutionInputBinding binding(second,result_replay);SourceExecutionCallScope call(&second,C::CallActualSource,[&]{return call_args;});
        ++real_calls;rejects([&]{call.complete([]{return source_execution_result(18u);});});}
    rejects([&]{result_replay.finish();});check(real_calls==3,"Result verification skipped actual native work");
    SourceExecutionInputs missing_result;missing_result.track_calls();
    {SourceExecutionInputBinding binding(first,missing_result);SourceExecutionCallScope call(&first,C::CallActualSource,[&]{return call_args;});}
    rejects([&]{missing_result.finish();});
    SourceExecutionInputs failed_result;failed_result.track_calls();
    {SourceExecutionInputBinding binding(first,failed_result);SourceExecutionCallScope call(&first,C::CallActualSource,[&]{return call_args;});
        ++real_calls;call.complete([]()->std::vector<std::byte>{throw std::bad_alloc();});}
    rejects([&]{failed_result.finish();});check(real_calls==4,"Result recording failure interrupted actual host work");
    auto oversized=returns.calls().front();oversized.result.resize(SourceExecutionInputs::maximum_call_bytes);
    rejects([&]{SourceExecutionInputs bad({},std::span(&oversized,1));});rejects([&]{validate_source_execution_calls(std::span(&oversized,1));});
    auto void_result=position;void_result.result={std::byte{1}};rejects([&]{validate_source_execution_calls(std::span(&void_result,1));});
    SourcePhysicsWorldRuntimeAbi abi{};abi.eflags=2;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x1f80};
    abi.x87_depth=2;abi.x87_status=0x1000;abi.x87[0]=1.0000000000000000001L;abi.x87[1]=-0.L;
    SourceExecutionCall prepared{C::PrepareSourceCall,source_execution_arguments(std::span<const std::uint32_t>{}),0,0,source_execution_result(abi)};
    validate_source_execution_calls(std::span(&prepared,1));
    const auto live_bits=prepared.result;abi.x87[7]=1234.L;
    check(source_execution_result(abi)==live_bits,"Dead x87 lanes leaked into the native ABI journal");
    abi.x87_depth=9;rejects([&]{source_execution_result(abi);});abi.x87_depth=0;abi.x87_status=0;
    abi.active_mmx.emplace();abi.active_mmx->known_mask=2;abi.active_mmx->physical[1][0]=std::byte{0x15};
    const auto mmx=source_execution_result(abi);abi.active_mmx->physical[7][3]=std::byte{0x44};
    check(source_execution_result(abi)==mmx,"Unknown physical MMX lanes leaked into the journal");
    prepared.result=mmx;validate_source_execution_calls(std::span(&prepared,1));
    abi.x87_depth=1;rejects([&]{source_execution_result(abi);});abi.x87_depth=0;abi.x87_status=0x800;
    rejects([&]{source_execution_result(abi);});
    auto truncated_abi=prepared;truncated_abi.result.pop_back();rejects([&]{validate_source_execution_calls(std::span(&truncated_abi,1));});
    std::vector<std::uint32_t> arguments(0x1000,0xffffffffu);
    prepared.arguments=source_execution_arguments(std::span<const std::uint32_t>(arguments));validate_source_execution_calls(std::span(&prepared,1));
    arguments.push_back(0);rejects([&]{source_execution_arguments(std::span<const std::uint32_t>(arguments));});
    SourceExecutionCall caller{C::WriteSourceCallInput,source_execution_arguments(0x1000u,std::span<const std::byte>(word))};
    validate_source_execution_calls(std::span(&caller,1));caller.arguments[4]=std::byte{5};rejects([&]{validate_source_execution_calls(std::span(&caller,1));});
    rejects([&]{source_execution_arguments(0xffffffffu,std::span<const std::byte>(word));});
    abi.active_mmx.reset();abi.x87_status=0;abi.floating_point->mxcsr=0x80001f80u;
    rejects([&]{source_execution_result(abi);});abi.floating_point->mxcsr=0x1f80u;
    prepared.arguments=source_execution_arguments(std::span<const std::uint32_t>{});prepared.result=source_execution_result(abi);
    // Reserved MXCSR high bits must never reach the CPU's LDMXCSR path.
    prepared.result[prepared.result.size()-2]=std::byte{0x80};rejects([&]{validate_source_execution_calls(std::span(&prepared,1));});
    SourceExecutionInputs observed;observed.track_calls({&mutations,nullptr});
    {SourceExecutionInputBinding binding(first,observed);SourceExecutionCallScope root(&first,C::BeginModelFrame,[]{return std::vector<std::byte>{};});
        {SourceExecutionObserverScope observer(first);check(first.view(0x1000,4).size()==4,"Read-only observer could not inspect source bytes");}}
    observed.finish();
    SourceExecutionInputs observer_write;observer_write.track_calls();
    {SourceExecutionInputBinding binding(first,observer_write);SourceExecutionCallScope root(&first,C::BeginModelFrame,[]{return std::vector<std::byte>{};});
        SourceExecutionObserverScope observer(first);first.write(0x1000,word);}
    rejects([&]{observer_write.finish();});
    SourceExecutionInputs observer_replay({},observed.calls());observer_replay.track_calls();
    {SourceExecutionInputBinding binding(second,observer_replay);SourceExecutionCallScope root(&second,C::BeginModelFrame,[]{return std::vector<std::byte>{};});
        SourceExecutionObserverScope observer(second);rejects([&]{second.write(0x1000,word);});
        rejects([&]{SourceExecutionCallScope nested(&second,C::BeginModelView,[]{return std::vector<std::byte>{};});});
        rejects([&]{source_execution_input(second,K::Milliseconds,0,[]{return 9;});});}
    rejects([&]{observer_replay.finish();});
    SourceExecutionInputs observer_counter;observer_counter.track_calls({&mutations,nullptr});
    {SourceExecutionInputBinding binding(first,observer_counter);SourceExecutionCallScope root(&first,C::BeginModelFrame,[]{return std::vector<std::byte>{};});
        SourceExecutionObserverScope observer(first);++mutations;}
    rejects([&]{observer_counter.finish();});
    SourceObjectAdmission admission;SourceDrawUnitState draw_state;SourceExecutionModelInputTu15 model_input;
    SourceExecutionCall model_call{C::LiveModelDraws,source_execution_arguments(admission,draw_state,model_input),0,0,source_execution_model_result(admission,draw_state)};
    validate_source_execution_calls(std::span(&model_call,1));check(model_call.result.size()==32,"Live-model output seal exceeds its digest size");
    draw_state.units.back().words[0]^=1;check(source_execution_model_result(admission,draw_state)!=model_call.result,"Draw-bank result digest omitted retained tail words");
    draw_state.draw_count=851;rejects([&]{source_execution_arguments(admission,draw_state,model_input);});draw_state.draw_count=0;
    model_input.view=4;rejects([&]{source_execution_arguments(admission,draw_state,model_input);});model_input.view=0;
    admission.objects.resize(source_object_main_queue_capacity+1);rejects([&]{source_execution_arguments(admission,draw_state,model_input);});admission.objects.clear();
    auto bad_model=model_call;bad_model.result.pop_back();rejects([&]{validate_source_execution_calls(std::span(&bad_model,1));});
    bad_model=model_call;bad_model.arguments.push_back(std::byte{});rejects([&]{validate_source_execution_calls(std::span(&bad_model,1));});
    SourceBspPartCommandState part_state;for(auto &bank:part_state.banks){bank.header[0]=2;bank.storage.resize(2);}
    part_state.calls.resize(129,0x41c70u);
    SourceExecutionCall attachment{C::RenderAttachments,source_execution_arguments(part_state,admission,1u),0,0,source_execution_attachment_result(part_state)};
    validate_source_execution_calls(std::span(&attachment,1));part_state.banks[0].storage.back().words[5]=0x55550000u;
    check(source_execution_attachment_result(part_state)!=attachment.result,"Attachment digest omitted the source-retained final two bytes");
    rejects([&]{source_execution_arguments(part_state,admission,0u);});part_state.banks[0].header[1]=3;
    rejects([&]{source_execution_arguments(part_state,admission,1u);});part_state.banks[0].header[1]=0;
    part_state.banks[0].storage.pop_back();rejects([&]{source_execution_attachment_result(part_state);});
    auto bad_attachment=attachment;bad_attachment.result.pop_back();rejects([&]{validate_source_execution_calls(std::span(&bad_attachment,1));});
    SourceExecutionCall geometry{C::RenderGeometry,source_execution_arguments(2u,0x12340000u),0,0,source_execution_result(1u)};
    validate_source_execution_calls(std::span(&geometry,1));geometry.result=source_execution_result(2u);rejects([&]{validate_source_execution_calls(std::span(&geometry,1));});
    std::cout<<"PASS native external input bit/order/width binding, complete consumption, failure isolation and executor ownership\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
