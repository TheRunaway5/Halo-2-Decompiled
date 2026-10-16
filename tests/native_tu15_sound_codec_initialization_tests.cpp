#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/native_tu15_sound_codec_initialization_data.hpp"
#include "halo2/source_runtime_sound_codec_route.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_codec_initialization_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Unowned native codec request was accepted");}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.write(p,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(8*i);return value;}
std::vector<std::byte> unhex(const std::string& text){auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> r(text.size()/2);for(std::size_t i=0;i<r.size();++i)r[i]=std::byte((n(text[2*i])<<4)|n(text[2*i+1]));return r;}
void equal_bytes(const SourceRuntimeMemory& m,std::uint32_t p,const std::string& hex,const char* message){check(std::ranges::equal(m.view(p,hex.size()/2),unhex(hex)),message);}
void equal_stack(const SourceRuntimeMemory& m,const Json& prefix){const auto expected=unhex(prefix.at("stack_hex"));const auto actual=m.view(0x2010000,expected.size());const auto scratch=prefix.at("hardware_private_stack_bytes").get<std::vector<unsigned>>();for(unsigned i=0;i<expected.size();++i)if(!std::ranges::binary_search(scratch,0x2010000+i))check(actual[i]==expected[i],"Original retained caller stack differs outside declared native leaf scratch");}
struct Done{};struct Interrupted{};
Json run(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output,const std::shared_ptr<SdlSoundPlatform>& distinct_output,const std::filesystem::path& config,unsigned interrupt,bool failure_result=false){
    constexpr unsigned fs=0x60200000,stack=0x2008000,bounded_stack=0x2018000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    mount_native_tu15_sound_codec_initialization_data(memory);const auto immutable=memory.native_address(0x41b7a8,4);
    mount_native_tu15_sound_codec_initialization_data(memory);check(memory.native_address(0x41b7a8,4)==immutable,"Codec immutable reuse replaced actual backing");
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"actual SDK plus declared bounded codec callers");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual native passive audio executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    SourceRuntimeSoundResetResources reset(memory,output,fs);std::unique_ptr<SourceRuntimeSoundCodecRoute> codec;
    SourcePhysicsWorldRuntimeAbi factory{};factory.fs_base=fs;factory.gp[4]=stack;factory.eflags=0x202;factory.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);factory.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,factory,*factory.floating_point,services);}
    Json evidence;unsigned frontiers{},foreign_rejections{},action_unknowns{};bool queued{},interrupted{};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){
            ++frontiers;check(call.instruction==0x38eca3&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,"Codec test did not retain genuine preceding SDK/reset/16DMA constructors");
            if(failure_result){
                // Explicit component input at the native ABI. This verifies
                // genuine caller failure propagation, not forced SDL OOM.
                std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(reference.at("fill").get<unsigned>()));
                SourcePhysicsWorldRuntimeAbi bounded{};bounded.gp=reference.at("initial_gp").get<decltype(bounded.gp)>();bounded.fs_base=fs;bounded.eflags=0x202;bounded.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
                put(memory,bounded_stack,0x2100000);put(memory,bounded_stack+4,1);
                Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices cpu;
                cpu.instruction=[&](auto pc){instructions.push_back(pc);};
                cpu.required_owner=[&](auto&,const auto& request,auto& input,const auto&){check(request.instruction==0x38f46c&&request.target==0x38ed2c&&word(memory,input.gp[4])==0x38f471,"Failure component reached a different native ABI");input.gp[0]=0;input.gp[4]+=4;};
                SourcePhysicsContactRecordObserver record=[&](const auto& row){std::uint64_t value{};for(unsigned i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);stores.push_back({row.instruction,row.address,row.written.size(),value});};
                source_run_sound_codec_initialization_tu15(memory,reference.at("entry"),bounded,*bounded.floating_point,cpu,record);
                const auto& expected=reference.at("prefix");check(instructions==expected.at("instructions")&&stores==expected.at("source_stores")&&bounded.gp==expected.at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(expected.at("returned_eflags").get<unsigned>()&0xcc5)&&bounded.gp[0]==0x88780078,"Original codec caller failed to propagate exact native acquisition failure HRESULT");
                equal_stack(memory,expected);equal_bytes(memory,reference.at("controller"),expected.at("controller_hex"),"Original codec failure controller stores differ");
                for(const auto& row:reference.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original failed codec caller raw data differs");
                for(const auto& row:reference.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original failed codec caller DMA data differs");
                unsigned released_claims{};for(unsigned attempt=0;attempt<2;++attempt)try{SourceRuntimeSoundCodecRoute invalid(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);}catch(const std::invalid_argument& error){if(std::string(error.what()).find("uninitialized source controller")!=std::string::npos)++released_claims;}
                check(released_claims==2,"Failed codec constructor retained its exclusive controller claim");
                evidence={{"entry",reference.at("entry")},{"retained_source_instructions",instructions.size()},{"source_stores",stores.size()},{"native_abi_failure_result_input",0},{"exact_source_returned_hresult",bounded.gp[0]},{"actual_native_allocation_failure_forced",false},{"failed_constructor_claim_release_probes",released_claims}};throw Done{};
            }
            const auto checkpoint=memory.checkpoint();SourceRuntimeMemory foreign_bank;
            rejected([&]{SourceRuntimeSoundCodecRoute wrong(foreign_bank,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);});
            rejected([&]{SourceRuntimeSoundCodecRoute wrong(memory,distinct_output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);});
            rejected([&]{reset.control_lease().require_output(*distinct_output);});
            check(memory.checkpoint()==checkpoint,"Foreign codec construction changed source data");
            codec=std::make_unique<SourceRuntimeSoundCodecRoute>(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);
            check(codec->diagnostics().pipes.empty()&&!codec->diagnostics().source_codec_ready&&codec->diagnostics().exclusive_controller_claimed,"Codec route was preset or lacked exclusive controller ownership before genuine source constructor");
            auto duplicate=[&]{const auto before=memory.checkpoint();bool rejected_claim=false;try{SourceRuntimeSoundCodecRoute wrong(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);}catch(const std::invalid_argument& error){rejected_claim=std::string(error.what()).find("already owned")!=std::string::npos;}check(rejected_claim&&before==memory.checkpoint(),"Duplicate actual codec controller owner was admitted or mutated source state");};
            duplicate();
            std::thread foreign([&]{auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_rejections;}};
                SourcePhysicsWorldRuntimeAbi bad{};bad.gp[4]=0xffffffffu;
                guard([&]{codec->diagnostics();});guard([&]{codec->require_bank(foreign_bank);});guard([&]{codec->run_cpu(0x38f45f,bad,{0x27f,0x1f80},{});});
                guard([&]{codec->run_command_component(bad,{0x27f,0x1f80},{});});guard([&]{codec->convert_native(SourceRuntimeSoundCodecRoute::Kind::Analog,{});});
            });foreign.join();check(foreign_rejections==5,"Native codec APIs read foreign mutable state or invalid source stack");
            const auto control=reset.control_lease();bool foreign_output=false;std::thread foreign_control([&]{try{control.require_output(*distinct_output);}catch(const std::exception& error){foreign_output=std::string(error.what()).find("foreign owner thread")!=std::string::npos;}});foreign_control.join();check(foreign_output,"Control output authentication read a foreign mutable output");
            std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(reference.at("fill").get<unsigned>()));
            SourcePhysicsWorldRuntimeAbi bounded{};bounded.gp=reference.at("initial_gp").get<decltype(bounded.gp)>();bounded.fs_base=fs;bounded.eflags=0x202;bounded.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            put(memory,bounded_stack,0x2100000);put(memory,bounded_stack+4,1);
            Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices native;
            const std::array<std::int16_t,14> samples{-32768,-32768,-16384,-16384,-1,-1,0,0,1,1,16384,16384,32767,32767};
            auto unity=[&]{for(auto kind:{SourceRuntimeSoundCodecRoute::Kind::Analog,SourceRuntimeSoundCodecRoute::Kind::Spdif}){
                const auto converted=codec->convert_native(kind,samples);check(converted.size()==samples.size(),"Real native stereo converter changed frame count");for(unsigned i=0;i<samples.size();++i)check(converted[i]==float(samples[i])/32768.0f,"Actual native WM9709 reference profile changed unity signed16 conversion");
            }};
            native.instruction=[&](auto pc){instructions.push_back(pc);if(interrupt==2&&pc==0x38eda2)throw Interrupted{};if(pc==0x38f471){unity();for(auto kind:{SourceRuntimeSoundCodecRoute::Kind::Analog,SourceRuntimeSoundCodecRoute::Kind::Spdif})codec->queue_native_probe(kind,samples);
                const auto routes=codec->diagnostics();check(routes.pipes.size()==2&&routes.pipes[0].queued_float_bytes==samples.size()*4&&routes.pipes[1].queued_float_bytes==samples.size()*4,"Actual native codec queues did not retain independent probe PCM");queued=true;
            }};
            SourcePhysicsContactRecordObserver record=[&](const auto& row){std::uint64_t value{};for(unsigned i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);stores.push_back({row.instruction,row.address,row.written.size(),value});
                if(row.address>=0xfec00000){check(row.before.empty(),"Pure original codec MOV invented a hardware preimage/readback");++action_unknowns;if(interrupt==1&&row.instruction==0x38eda2)throw Interrupted{};}
            };
            native.required_owner=[&](auto&,const auto& request,auto& cpu,const auto&){
                check(request.instruction==0x38f4c4&&request.target==0x8000002c,"Codec prefix passed its real IRQ6 frontier");
                const auto& expected=reference.at("prefix");
                check(instructions==expected.at("instructions")&&stores==expected.at("source_stores"),"Original retained codec constructor instructions/action/ordinary stores differ");
                check(cpu.gp==expected.at("frontier").at("gp").get<decltype(cpu.gp)>()&&(cpu.eflags&0xcc5)==(expected.at("frontier").at("eflags").get<unsigned>()&0xcc5),"Original codec constructor frontier GP/defined flags differ");
                equal_stack(memory,expected);equal_bytes(memory,reference.at("controller"),expected.at("controller_hex"),"Original source codec controller differs");
                for(const auto& row:reference.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original codec retained raw/refcount graph differs");
                for(const auto& row:reference.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original codec retained DMA/storage graph differs");
                auto actual=codec->diagnostics();check(queued&&actual.pipes.size()==2&&actual.commands.size()==1&&actual.commands[0].index==0x13&&actual.commands[0].value==0&&actual.commands[0].ignored_by_reference_profile&&actual.completed_transactions==1&&!actual.command_acquired&&!actual.faulted&&!actual.source_codec_ready&&!actual.source_channel_ready&&!actual.producer_ready,"Native codec prefix supplied readiness or lost genuine power command transaction");
                duplicate();check(codec->diagnostics().exclusive_controller_claimed&&codec->diagnostics().pipes.size()==2,"Rejected retained-prefix duplicate lost actual converter/controller ownership");
                for(const auto& pipe:actual.pipes)check(pipe.native_converter.value>0xffffffffu&&pipe.completed_resets==1&&!pipe.queued_float_bytes,"Source reset did not clear actual native64 converter queue");
                if(interrupt==3){std::set_terminate([]{std::_Exit(77);});std::thread other([owner=std::move(codec)]()mutable{owner.reset();});other.join();throw std::runtime_error("Foreign codec destructor bypassed its creating owner thread");}
                unity();unsigned components{};
                for(const auto& component:reference.at("components")){
                    Json component_instructions=Json::array(),component_stores=Json::array();SourcePhysicsWorldRuntimeServices probe;
                    probe.instruction=[&](auto pc){component_instructions.push_back(pc);};
                    SourcePhysicsContactRecordObserver component_record=[&](const auto& row){std::uint64_t value{};for(unsigned i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);component_stores.push_back({row.instruction,row.address,row.written.size(),value});if(row.address>=0xfec00000)check(row.before.empty(),"Bounded command invented physical preimage");};
                    SourcePhysicsWorldRuntimeAbi helper{};helper.gp=component.at("input_gp").get<decltype(helper.gp)>();helper.fs_base=fs;helper.eflags=component.at("input_eflags");helper.floating_point=bounded.floating_point;
                    put(memory,0x201b000,component.at("returned"));put(memory,0x201b004,component.at("index"));put(memory,0x201b008,component.at("value"));
                    codec->run_command_component(helper,*helper.floating_point,probe,component_record);
                    check(component_instructions==component.at("instructions")&&component_stores==component.at("source_stores")&&helper.gp==component.at("returned_gp").get<decltype(helper.gp)>()&&(helper.eflags&0xcc5)==(component.at("returned_eflags").get<unsigned>()&0xcc5),"Original bounded master/PCM command caller CPU/result propagation differs");
                    equal_bytes(memory,0x201b000,component.at("stack_hex"),"Original bounded codec command argument stack differs");unity();++components;
                }
                actual=codec->diagnostics();check(actual.commands.size()==3&&actual.completed_transactions==3&&!actual.command_acquired,"Actual serialized source codec commands did not commit");
                evidence={{"entry",reference.at("entry")},{"fill",reference.at("fill")},{"retained_source_instructions",instructions.size()},{"source_stores",stores.size()},{"unknown_action_preimages",action_unknowns},{"declared_private_leaf_stack_bytes",expected.at("hardware_private_stack_bytes").size()},{"foreign_thread_rejections",foreign_rejections},{"foreign_bank_rejected",true},{"same_control_distinct_output_rejections",2},{"foreign_control_output_rejected_before_mutable_access",true},{"actual_native64_converters",actual.pipes.size()},{"actual_converter_resets",2},{"actual_command_transactions",3},{"bounded_command_components",components},{"unity_s16_to_float_samples_checked",samples.size()*2*4},{"real_irq6_frontier",true}};
                auto repeated=state;repeated.gp[4]=0x201d000;repeated.gp[1]=control.source_apu()+8;repeated.gp[6]=repeated.gp[1];
                put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
                SourceRuntimeSoundResetResources distinct_worker(memory,output,fs);
                check(distinct_worker.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Distinct actual worker did not execute genuine whole APU reset");
                check(distinct_worker.control_lease().generation()==control.generation(),"Distinct-worker codec guard fixture did not reach equal real generations");
                rejected([&]{codec->require_control(memory,distinct_worker.control_lease());});
                check(codec->diagnostics().pipes.size()==2&&!codec->diagnostics().faulted,"Rejected distinct real worker mutated current codec resource owner");
                repeated.gp[4]=0x201d000;repeated.gp[1]=control.source_apu()+8;repeated.gp[6]=repeated.gp[1];
                put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
                check(reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Genuine whole APU reset did not advance codec control generation");
                rejected([&]{codec->diagnostics();});
                const auto stale=codec->diagnostics();check(stale.faulted&&stale.pipes.empty()&&!stale.command_acquired&&reset.state().faulted,"Failed codec generation inspection retained pipes or shared partial transaction");
                evidence["same_bank_output_equal_generation_distinct_worker_rejected"]=true;
                evidence["genuine_whole_reset_invalidates_diagnostics_and_releases_routes"]=true;
                evidence["duplicate_controller_claim_rejections_before_and_after_prefix"]=2;
                check(!stale.exclusive_controller_claimed,"Codec cleanup retained its exclusive controller ownership after real resource fence");
                throw Done{};
            };
            try{codec->run_cpu(reference.at("entry"),bounded,*bounded.floating_point,native,record);}catch(const Interrupted&){interrupted=true;}
            if(interrupt){const auto actual=codec->diagnostics();check(interrupted&&actual.faulted&&actual.pipes.empty()&&!actual.command_acquired&&!actual.exclusive_controller_claimed&&actual.commands.size()==(interrupt==1?1u:0u)&&reset.state().faulted,"Observer interruption retained converter/lock/claim resources or failed to abort shared control");
                evidence={{"observer_interrupted_after_committed_action",interrupt==1},{"instruction_observer_interrupted_with_command_acquired",interrupt==2},{"actual_routes_released",true},{"actual_command_lock_released",true},{"exclusive_controller_claim_released_after_fence",true},{"shared_control_faulted",true},{"committed_diagnostic_commands_retained",actual.commands.size()}};throw Done{};
            }
            throw std::runtime_error("Codec test did not stop at its genuine frontier");
        }
        if(reset.try_call(bank,call,state,*state.floating_point,services,observer))return;
        if(dma->dispatch(bank,call,state))return;if(sdk.dispatch(bank,call,state,observer))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        throw std::runtime_error("Unowned actual SDK prerequisite");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);factory.gp[4]=stack;
    bool reached=false;try{SourceRuntimeSoundSdkResources::CallScope sdk_scope(&sdk);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());SourceRuntimeSoundResetResources::CallScope reset_scope(&reset);source_run_sound_sdk_tu15(memory,0x387877,factory,*factory.floating_point,services);}catch(const Done&){reached=true;}
    check(reached&&frontiers==1,"Codec test failed to retain the original factory prerequisite frontier");preference.retire();return evidence;
}
}
int main(int argc,char**argv){try{
    const bool foreign_destructor=argc==5&&std::string(argv[1])=="--foreign-destructor-probe";const unsigned shift=foreign_destructor?1:0;
    if(argc!=int(4+shift))throw std::runtime_error("usage: native-tests [--foreign-destructor-probe] original-report private-dir output-report");
    std::ifstream stream(argv[1+shift]);const auto reference=Json::parse(stream);check(reference.at("passed"),"Original codec reference failed");
    const std::filesystem::path private_dir=argv[2+shift];std::filesystem::create_directories(private_dir);const auto config=private_dir/"menu.cfg";
    {std::ofstream file(config);file<<"pc_speakers=0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,256,{}});
    auto distinct_output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,256,{}});
    check(output.get()!=distinct_output.get()&&output->device_id()!=distinct_output->device_id(),"Distinct-output guard fixture did not open an independent actual device");
    if(foreign_destructor){run(reference.at("cases").at(0),output,distinct_output,config,3);throw std::runtime_error("Foreign codec destructor did not enforce exact-exit77 contract");}
    Json cases=Json::array();for(const auto& row:reference.at("cases"))cases.push_back(run(row,output,distinct_output,config,false));
    Json interruptions=Json::array();for(unsigned phase:{1u,2u})interruptions.push_back(run(reference.at("cases").at(0),output,distinct_output,config,phase));
    Json failures=Json::array();for(const auto& row:reference.at("failure_result_cases"))failures.push_back(run(row,output,distinct_output,config,false,true));
    Json report={{"passed",true},{"cases",cases},{"failure_result_components",failures},{"observer_failures",interruptions},{"actual_sdl_driver",SDL_GetCurrentAudioDriver()},{"actual_output_device",output->device_id()},{"actual_output_channels",output->output_channels()},{"native_abi_leaf_entries",{0x38ed2c,0x38ecff}},{"runtime_register_bank",false},{"whole38f45f_successfully_returned",false},{"irq6_result_supplied",false},{"source_codec_ready",false},{"source_channel_ready",false},{"producer_ready",false},{"full_startup_completed",false}};
    std::ofstream result(argv[3+shift]);result<<report.dump(2)<<'\n';std::cout<<"Native codec prefix2 original cases +4 bounded commands +2 source failure results + observer/ownership cleanup passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
