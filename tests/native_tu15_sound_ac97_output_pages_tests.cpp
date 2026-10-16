#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/native_tu15_sound_ac97_output_data.hpp"
#include "halo2/source_runtime_sound_ac97_output_pages.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/source_sound_ac97_output_tu15.hpp"
#include "halo2/source_sound_hardware_storage_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Unowned/invalid AC97 page request was accepted");}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(i*8));m.write(p,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return value;}
std::vector<std::byte> unhex(const std::string& text){auto n=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};std::vector<std::byte> r(text.size()/2);for(std::size_t i=0;i<r.size();++i)r[i]=std::byte((n(text[2*i])<<4)|n(text[2*i+1]));return r;}
void equal_bytes(const SourceRuntimeMemory& memory,std::uint32_t p,const std::string& hex,const char* message){check(std::ranges::equal(memory.view(p,hex.size()/2),unhex(hex)),message);}
struct SourceFrontier {};
struct ChannelResetFrontier {};
void with_foreign_dma_ep(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,
    const std::function<void(const std::shared_ptr<SourceRuntimeSoundDmaResources>&,
        const std::shared_ptr<SourceRuntimeSoundEpOutputResources>&)>& inspect){
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"independent genuine foreign-bank SDK caller");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"independent genuine foreign-bank executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    SourceRuntimeSoundResetResources reset(memory,output,fs);SourcePhysicsWorldRuntimeAbi abi{};abi.fs_base=fs;abi.gp[4]=stack;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);abi.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,abi,*abi.floating_point,services);}
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(call.target==0x38e8c9){
            auto ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);inspect(dma,ep);
            SourceRuntimeSoundAc97OutputPages old_pages(memory,dma,ep,output,reset.control_lease());auto& old_view=old_pages.cpu_memory();
            auto repeated=state;repeated.gp[4]=0x201a000;repeated.gp[6]=repeated.gp[1];
            put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
            check(reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Genuine repeated native reset was not owned");
            bool stale_read=false,stale_write=false;std::array<std::byte,1> byte{};
            try{old_view.read(0xffffffffu,byte);}catch(const std::exception& error){stale_read=std::string(error.what()).find("retired-generation")!=std::string::npos;}
            try{old_view.write(0xffffffffu,byte);}catch(const std::exception& error){stale_write=std::string(error.what()).find("retired-generation")!=std::string::npos;}
            check(stale_read&&stale_write,"Borrowed AC97 CPU view accessed source bank after genuine reset generation changed");
            throw SourceFrontier{};
        }
        if(reset.try_call(bank,call,state,*state.floating_point,services,observe))return;
        if(dma->dispatch(bank,call,state))return;
        if(sdk.dispatch(bank,call,state,observe))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observe);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observe);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observe);return;}
        throw std::runtime_error("Unowned actual foreign-bank SDK resource");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);abi.gp[4]=stack;
    bool reached=false;try{SourceRuntimeSoundSdkResources::CallScope sdk_scope(&sdk);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());SourceRuntimeSoundResetResources::CallScope reset_scope(&reset);source_run_sound_sdk_tu15(memory,0x387877,abi,*abi.floating_point,services);}catch(const SourceFrontier&){reached=true;}
    check(reached,"Independent foreign-bank SDK resource fixture did not reach its genuine frontier");preference.retire();
}
Json run(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config){
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000,channel_stack=0x2018000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    mount_native_tu15_sound_ac97_output_data(memory);const auto immutable=memory.native_address(0x4212f8,4);
    mount_native_tu15_sound_ac97_output_data(memory);check(memory.native_address(0x4212f8,4)==immutable,"Immutable AC97 reuse replaced original backing");
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared factory and independent bounded channel callers");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual native passive audio executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    auto reset=std::make_unique<SourceRuntimeSoundResetResources>(memory,output,fs);
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep;std::unique_ptr<SourceRuntimeSoundAc97OutputPages> pages;
    SourcePhysicsWorldRuntimeAbi factory{};factory.fs_base=fs;factory.gp[4]=stack;factory.eflags=0x202;factory.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);factory.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,factory,*factory.floating_point,services);}
    Json evidence;unsigned frontiers{},foreign_rejections{};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){
            ++frontiers;check(call.instruction==0x38eca3&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,"AC97 prerequisite did not retain real preceding SDK/DMA/reset graph");
            ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);
            pages=std::make_unique<SourceRuntimeSoundAc97OutputPages>(memory,dma,ep,output,reset->control_lease());
            check(pages->quiescent()&&pages->diagnostics().controller==reference.at("controller"),"AC97 page owner lacks its real source controller");
            const auto checkpoint=memory.checkpoint();unsigned bank_rejections{};
            with_foreign_dma_ep(output,config,[&](const auto& foreign_dma,const auto& foreign_ep){
                auto reject_bank=[&](const auto& candidate_dma,const auto& candidate_ep){
                    rejected([&]{SourceRuntimeSoundAc97OutputPages wrong(memory,candidate_dma,candidate_ep,output,reset->control_lease());});++bank_rejections;
                    check(memory.checkpoint()==checkpoint,"Rejected same-output foreign-bank AC97 resources changed the current bank");
                };
                reject_bank(foreign_dma,foreign_ep);reject_bank(dma,foreign_ep);reject_bank(foreign_dma,ep);
            });check(bank_rejections==3,"Same-output foreign/hybrid bank resources were not rejected");
            std::thread foreign([&]{
                check(!pages->quiescent(),"Foreign AC97 diagnostic claimed quiescence");
                auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_rejections;}};
                guard([&]{pages->diagnostics();});guard([&]{SourceRuntimeSoundAc97OutputPages::CallScope scope(pages.get());});
                guard([&]{pages->cpu_memory();});
                SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffffu;
                guard([&]{pages->dispatch(memory,{0x3879dc,0x800000a6,false},invalid);});
                guard([&]{pages->try_exclusion({0x383d88,0x800000a0,false},invalid);});
            });foreign.join();check(foreign_rejections==5,"AC97 page owner read a foreign mutable source frame");
            std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(reference.at("fill").get<unsigned>()));
            SourcePhysicsWorldRuntimeAbi bounded{};bounded.gp=reference.at("initial_gp").get<decltype(bounded.gp)>();bounded.fs_base=fs;bounded.eflags=0x202;bounded.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices scoped;
            scoped.instruction=[&](auto pc){if(pc==0x38f007)throw ChannelResetFrontier{};instructions.push_back(pc);};
            SourcePhysicsContactRecordObserver observe=[&](const auto& row){std::uint64_t value{};for(unsigned i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(i*8);stores.push_back({row.instruction,row.address,row.written.size(),value});};
            scoped.required_owner=[&](auto& channel_bank,const auto& request,auto& cpu,const auto& record){
                if(pages->dispatch(memory,request,cpu)){
                    if(request.instruction==0x3879dc){const auto rows=pages->diagnostics().allocations;const auto at=std::ranges::find_if(rows,[&](const auto& row){return row.source==cpu.gp[0];});check(at!=rows.end(),"Real AC97 allocation is not owned");std::ranges::fill(memory.mutable_view(at->source,at->capacity),std::byte{0xa5});}
                    return;
                }
                if(pages->try_exclusion(request,cpu)){
                    if(request.instruction==0x383d88)check(!pages->quiescent(),"AC97 page owner advertised quiescence while real source exclusion was held");
                    return;
                }
                if(reset->try_call(memory,request,cpu,*cpu.floating_point,scoped,record))return;
                if(sdk.dispatch(memory,request,cpu,record))return;
                if(source_sound_ac97_output_tu15_owns_entry(request.target)){source_run_sound_ac97_output_tu15(channel_bank,request.target,cpu,*cpu.floating_point,scoped,record);return;}
                if(source_sound_hardware_storage_tu15_owns_entry(request.target)){source_run_sound_hardware_storage_tu15(channel_bank,request.target,cpu,*cpu.floating_point,scoped,record);return;}
                if(source_sound_dma_storage_tu15_owns_entry(request.target)){source_run_sound_dma_storage_tu15(channel_bank,request.target,cpu,*cpu.floating_point,scoped,record);return;}
                if(source_sound_sdk_tu15_owns_entry(request.target)){source_run_sound_sdk_tu15(channel_bank,request.target,cpu,*cpu.floating_point,scoped,record);return;}
                throw std::runtime_error("Unowned AC97 channel constructor request");
            };
            put(memory,channel_stack,0x2100000);put(memory,channel_stack+4,reference.at("index"));put(memory,channel_stack+8,0);
            source_run_sound_ac97_output_tu15(pages->cpu_memory(),0x38eef1,bounded,*bounded.floating_point,scoped,observe);
            check(instructions==reference.at("constructor").at("instructions")&&stores==reference.at("constructor").at("source_stores"),"Original AC97 small-pool channel allocation/refcount/store order differs");
            check(bounded.gp==reference.at("constructor").at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(reference.at("constructor").at("returned_eflags").get<unsigned>()&0xcc5),"Original AC97 channel constructor registers/defined flags differ");
            const auto channel=word(memory,reference.at("controller").get<unsigned>()+reference.at("index").get<unsigned>()*4);
            check(channel==reference.at("channel"),"Genuine shared-pool channel identity differs");
            instructions=Json::array();stores=Json::array();put(memory,channel_stack,0x2100000);put(memory,channel_stack+4,reference.at("flags"));put(memory,channel_stack+8,0);put(memory,channel_stack+12,0);
            bounded.gp[1]=channel;bounded.gp[4]=channel_stack;
            check(bounded.gp==reference.at("allocation_input_gp").get<decltype(bounded.gp)>()&&bounded.eflags==reference.at("allocation_input_eflags"),"Declared AC97 allocation caller differs");
            bool reset_frontier=false;try{
                SourceRuntimeSoundEpOutputResources::CallScope ep_scope(ep.get());
                SourceRuntimeSoundAc97OutputPages::CallScope page_scope(pages.get());
                source_run_sound_ac97_output_tu15(pages->cpu_memory(),0x38f104,bounded,*bounded.floating_point,scoped,observe);
            }catch(const ChannelResetFrontier&){reset_frontier=true;}
            check(reset_frontier&&instructions==reference.at("instructions")&&stores==reference.at("source_stores"),"Original AC97 descriptor allocation/clear prefix differs");
            check(bounded.gp==reference.at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(reference.at("returned_eflags").get<unsigned>()&0xcc5),"AC97 reset frontier registers/defined flags differ");
            equal_bytes(memory,0x2010000,reference.at("stack_hex"),"AC97 complete declared caller stack differs");
            equal_bytes(memory,reference.at("controller"),reference.at("controller_hex"),"Original AC97 source controller differs");
            for(const auto& row:reference.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"AC97 retained raw object/refcount bytes differ");
            for(const auto& row:reference.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"AC97 original shared-pool list/split/channel bytes differ");
            check(word(memory,0x390bc0)==reference.at("raw_statistics")&&word(memory,0x390bc4)==reference.at("physical_statistics")&&word(memory,0x390bcc)==reference.at("pool_statistics")&&pages->diagnostics().source_exclusion_level==reference.at("fs_level")&&memory.view(fs+0x24,1)[0]==std::byte{},"AC97 source allocation/statistics/exclusion domain differs");
            const auto actual=pages->diagnostics();check(actual.faulted&&!actual.channel_ready&&!actual.producer_ready&&actual.allocations.size()==reference.at("physical_allocations").size(),"Partial AC97 native pages supplied channel/producer success");
            unsigned i=0;for(const auto& row:actual.allocations){const auto& expected=reference.at("physical_allocations").at(i++);check(row.source==expected.at("source")&&row.physical==expected.at("physical")&&row.requested==expected.at("requested")&&row.capacity==expected.at("capacity")&&row.channel==channel&&row.index==expected.at("index")&&row.field==expected.at("field")&&row.native.value>0xffffffffu&&row.native_channel.value>0xffffffffu&&memory.source_address(row.native,row.capacity)==row.source,"Actual AC97 source/physical/native64 page binding differs");equal_bytes(memory,row.source,expected.at("hex"),"Original AC97 CPU did not clear complete real capacity");}
            rejected([&]{SourceRuntimeSoundAc97OutputPages::CallScope again(pages.get());});rejected([&]{memory.view(reference.at("first_unsupported_address"),1);});
            evidence={{"fill",reference.at("fill")},{"index",reference.at("index")},{"flags",reference.at("flags")},{"constructor_instructions",reference.at("constructor").at("instructions").size()},{"allocation_instructions",instructions.size()},{"allocation_stores",stores.size()},{"actual_native64_channel_pages",actual.allocations.size()},{"foreign_thread_rejections",foreign_rejections},{"same_output_foreign_bank_rejections",bank_rejections},{"first_unsupported_instruction",0x38f007},{"first_unsupported_address",reference.at("first_unsupported_address")}};
            throw SourceFrontier{};
        }
        if(reset->try_call(bank,call,state,*state.floating_point,services,observer))return;
        if(dma->dispatch(bank,call,state))return;
        if(sdk.dispatch(bank,call,state,observer))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        throw std::runtime_error("Unowned full SDK resource before bounded AC97 probe");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);factory.gp[4]=stack;
    bool stopped=false;try{
        SourceRuntimeSoundSdkResources::CallScope sdk_scope(&sdk);SourceRuntimeSoundDmaResources::CallScope dma_scope(dma.get());SourceRuntimeSoundResetResources::CallScope reset_scope(reset.get());
        source_run_sound_sdk_tu15(memory,0x387877,factory,*factory.floating_point,services);
    }catch(const SourceFrontier&){stopped=true;}
    check(stopped&&frontiers==1&&pages&&pages->diagnostics().faulted&&ep->diagnostics().faulted&&word(memory,0x2001000)==0,"Bounded AC97 prerequisite bypassed full SDK frontend or published device success");
    const auto retained=pages->diagnostics().allocations;for(const auto& row:retained)check(memory.native_address(row.source,row.capacity)==row.native,"Partial AC97 diagnostic pages disappeared before creating-thread teardown");
    pages.reset();for(const auto& row:retained)rejected([&]{memory.view(row.source,row.capacity);});
    ep->retire();ep.reset();reset.reset();preference.retire();preference.require_quiescent();
    check(SDL_GetAudioDeviceStatus(output->device_id())==SDL_AUDIO_PLAYING,"AC97 page cleanup closed its shared actual output");
    evidence["passed"]=true;evidence["held_exclusion_not_quiescent"]=true;evidence["borrowed_view_rejected_after_actual_reset_generation_change"]=true;evidence["source_reset_or_ready_supplied"]=false;evidence["source_descriptor_pages_released_on_creating_thread"]=true;return evidence;
}
}
int main(int argc,char** argv){try{
    check(argc==4,"Supply bounded original AC97 reference, private config directory and native report");std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&reference.at("instruction_patches").empty()&&reference.at("supplied_game_or_codec_results").empty(),"Original AC97 reference differs");
    const std::filesystem::path directory(argv[2]);std::filesystem::create_directories(directory);const auto config=directory/"menu.cfg";{std::ofstream file(config);file<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    Json cases=Json::array();for(const auto& row:reference.at("cases"))cases.push_back(run(row,output,config));
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"cases",cases},{"real_shared_sdk_dma_ep_reset_owners",true},{"channel_codec_interrupt_reset_or_program_ready",false},{"whole38f45f_or38e484_return_supplied",false},{"source_sdk_output_published",false},{"full_startup_completed",false}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save AC97 page evidence");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
