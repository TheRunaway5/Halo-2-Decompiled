#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/native_tu15_sound_ac97_output_data.hpp"
#include "halo2/source_runtime_sound_ac97_output_pages.hpp"
#include "halo2/source_runtime_sound_ac97_channel_reset.hpp"
#include "halo2/source_runtime_sound_ac97_page_release.hpp"
#include "halo2/source_runtime_sound_pcm_buffers.hpp"
#include "halo2/native_tu15_sound_ac97_retire_data.hpp"
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
void with_foreign_dma_ep(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,const Json& retirement_reference,
    const std::function<void(const SourceRuntimeMemory&,const std::shared_ptr<SourceRuntimeSoundDmaResources>&,
        const std::shared_ptr<SourceRuntimeSoundEpOutputResources>&)>& inspect){
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    mount_native_tu15_sound_ac97_output_data(memory);mount_native_tu15_sound_ac97_retire_data(memory);
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"independent genuine foreign-bank SDK caller");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"independent genuine foreign-bank executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    SourceRuntimeSoundResetResources reset(memory,output,fs);SourcePhysicsWorldRuntimeAbi abi{};abi.fs_base=fs;abi.gp[4]=stack;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);abi.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,abi,*abi.floating_point,services);}
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(call.target==0x38e8c9){
            auto ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);inspect(memory,dma,ep);
            auto old_pages=std::make_shared<SourceRuntimeSoundAc97OutputPages>(memory,dma,ep,output,reset.control_lease());auto& old_view=old_pages->cpu_memory();
            SourceRuntimeSoundAc97ChannelReset native_owner(memory,old_pages,output,reset.control_lease());
            SourcePhysicsWorldRuntimeAbi retiring{};retiring.gp=retirement_reference.at("initial_gp").get<decltype(retiring.gp)>();retiring.fs_base=fs;retiring.eflags=0x202;retiring.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            Json cpu_instructions=Json::array(),cpu_stores=Json::array();SourcePhysicsWorldRuntimeServices native_services;
            std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> generation_lease;
            auto probe=std::make_shared<const std::vector<float>>(64,0.125f);
            native_services.instruction=[&](auto pc){cpu_instructions.push_back(pc);if(pc==0x38f3bd){
                generation_lease=native_owner.endpoint(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);
                check(generation_lease->submit_native({probe,0,32,901})==SdlSoundPlatform::Submission::Accepted,"Actual pre-retirement native endpoint could not retain PCM");
            }};
            SourcePhysicsContactRecordObserver record=[&](const auto& row){std::uint64_t value{};for(unsigned i=0;i<row.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);cpu_stores.push_back({row.instruction,row.address,row.written.size(),value});};
            native_services.required_owner=[&](auto& actual,const auto& request,auto& cpu,const auto& observe){
                if(old_pages->dispatch(actual,request,cpu))return;if(sdk.dispatch(actual,request,cpu,observe))return;
                throw std::runtime_error("Unowned native resource in genuine pre-page channel retirement");
            };
            constexpr std::uint32_t retire_stack=0x201c000;
            put(memory,retire_stack,0x2100000);put(memory,retire_stack+4,0);put(memory,retire_stack+8,0);
            native_owner.run_cpu(0x38eef1,retiring,*retiring.floating_point,native_services,record);
            check(cpu_instructions==retirement_reference.at("constructor").at("instructions")&&cpu_stores==retirement_reference.at("constructor").at("source_stores")&&retiring.gp==retirement_reference.at("constructor").at("returned_gp").get<decltype(retiring.gp)>(),"Original pre-page channel constructor differs");
            native_owner.bind_channel(retirement_reference.at("channel"));
            cpu_instructions=Json::array();cpu_stores=Json::array();put(memory,retire_stack,0x2100000);put(memory,retire_stack+4,0);retiring.gp[1]=retirement_reference.at("controller");retiring.gp[4]=retire_stack;
            native_owner.run_cpu(0x38f395,retiring,*retiring.floating_point,native_services,record);
            check(cpu_instructions==retirement_reference.at("instructions")&&cpu_stores==retirement_reference.at("source_stores")&&retiring.gp==retirement_reference.at("returned_gp").get<decltype(retiring.gp)>()&&(retiring.eflags&0xcc5)==(retirement_reference.at("returned_eflags").get<unsigned>()&0xcc5),"Original channel teardown/shared-pool return CPU differs");
            equal_bytes(memory,0x2010000,retirement_reference.at("stack_hex"),"Original pre-page channel retirement stack differs");
            for(const auto& row:retirement_reference.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original channel retirement raw/refcount bytes differ");
            for(const auto& row:retirement_reference.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original channel retirement shared-pool bytes differ");
            check(!word(memory,retirement_reference.at("controller"))&&!word(memory,0x390bcc),"Genuine channel retirement did not clear its slot/return shared-pool accounting");
            check(generation_lease&&native_owner.diagnostics().endpoints[0].admission_closed,"Genuine pre-page source retirement left native admission open");
            rejected([&]{native_owner.endpoint(SourceRuntimeSoundAc97ChannelReset::Kind::Analog);});
            rejected([&]{generation_lease->submit_native({probe,0,32,903});});
            auto repeated=state;repeated.gp[4]=0x201a000;repeated.gp[6]=repeated.gp[1];
            put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
            check(reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Genuine repeated native reset was not owned");
            bool stale_submit=false;try{generation_lease->submit_native({probe,0,32,902});}catch(const std::exception& error){stale_submit=std::string(error.what()).find("Whole native APU reset")!=std::string::npos;}
            check(stale_submit&&generation_lease->status().pending_packets==0&&generation_lease->completion()&&generation_lease->completion()->status==SdlSoundCompletionStatus::Flushed,"Actual whole reset did not invalidate/flush old native endpoint generation");
            generation_lease->acknowledge(generation_lease->completion()->submission);generation_lease->retire();
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
Json run(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,const Json& retirement_reference){
    const auto& allocated=reference.at("allocation");
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000,channel_stack=0x2018000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);
    mount_native_tu15_sound_ac97_output_data(memory);mount_native_tu15_sound_ac97_retire_data(memory);mount_native_tu15_sound_ac97_page_release_data(memory);const auto immutable=memory.native_address(0x4212f8,4);
    mount_native_tu15_sound_ac97_output_data(memory);check(memory.native_address(0x4212f8,4)==immutable,"Immutable AC97 reuse replaced original backing");
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared factory and independent bounded channel callers");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual native passive audio executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);
    auto reset=std::make_unique<SourceRuntimeSoundResetResources>(memory,output,fs);
    std::shared_ptr<SourceRuntimeSoundEpOutputResources> ep;std::shared_ptr<SourceRuntimeSoundAc97OutputPages> pages;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset> channel_reset;
    std::shared_ptr<SourceRuntimeSoundAc97ChannelReset::EndpointLease> retained_endpoint;
    SourcePhysicsWorldRuntimeAbi factory{};factory.fs_base=fs;factory.gp[4]=stack;factory.eflags=0x202;factory.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);factory.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,factory,*factory.floating_point,services);}
    Json evidence;unsigned frontiers{},foreign_rejections{};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){
            ++frontiers;check(call.instruction==0x38eca3&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,"AC97 prerequisite did not retain real preceding SDK/DMA/reset graph");
            ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);
            pages=std::make_shared<SourceRuntimeSoundAc97OutputPages>(memory,dma,ep,output,reset->control_lease());
            check(pages->quiescent()&&pages->diagnostics().controller==reference.at("controller"),"AC97 page owner lacks its real source controller");
            channel_reset=std::make_shared<SourceRuntimeSoundAc97ChannelReset>(memory,pages,output,reset->control_lease());
            channel_reset->require_bank(memory);
            const auto checkpoint=memory.checkpoint();unsigned bank_rejections{};
            with_foreign_dma_ep(output,config,retirement_reference,[&](const auto& foreign_bank,const auto& foreign_dma,const auto& foreign_ep){
                rejected([&]{channel_reset->require_bank(foreign_bank);});
                check(memory.checkpoint()==checkpoint,"Foreign channel-reset bank authentication changed source data");
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
            scoped.instruction=[&](auto pc){instructions.push_back(pc);};
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
            channel_reset->require_bank(memory);
            const auto reset_bank_checkpoint=memory.checkpoint();SourceRuntimeMemory wrong_reset_bank;
            rejected([&]{channel_reset->require_bank(wrong_reset_bank);});
            check(memory.checkpoint()==reset_bank_checkpoint,"Foreign channel-reset bank authentication changed source data");
            channel_reset->bind_channel(channel);
            const auto kind=reference.at("index")==0?SourceRuntimeSoundAc97ChannelReset::Kind::Analog:SourceRuntimeSoundAc97ChannelReset::Kind::Spdif;
            retained_endpoint=channel_reset->endpoint(kind);
            check(retained_endpoint->kind()==kind&&retained_endpoint->source_channel()==channel&&retained_endpoint->status().paused,"Actual native endpoint lost analog/SPDIF source identity");
            unsigned foreign_reset{};std::thread foreign_channel([&]{
                auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_reset;}};
                guard([&]{channel_reset->diagnostics();});guard([&]{channel_reset->bind_channel(0xffffffffu);});guard([&]{channel_reset->endpoint(kind);});guard([&]{channel_reset->require_bank(wrong_reset_bank);});
                SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffffu;
                guard([&]{channel_reset->run_cpu(0x38efea,invalid,{0x27f,0x1f80},{});});guard([&]{retained_endpoint->retire();});guard([&]{retained_endpoint->submit_native({});});
            });foreign_channel.join();check(foreign_reset==7,"Foreign channel/reset API touched native source or output state");
            auto pcm=std::make_shared<const std::vector<float>>(64,0.25f);
            check(retained_endpoint->submit_native({pcm,0,32,111})==SdlSoundPlatform::Submission::Accepted&&retained_endpoint->submit_native({pcm,0,32,222})==SdlSoundPlatform::Submission::Accepted&&
                retained_endpoint->submit_native({pcm,0,32,333})==SdlSoundPlatform::Submission::Backpressure&&retained_endpoint->status().pending_packets==2,"Actual paused endpoint did not retain its two native PCM packets");
            SourceRuntimeSoundAc97ChannelReset::CallScope channel_scope(channel_reset.get());
            instructions=Json::array();stores=Json::array();put(memory,channel_stack,0x2100000);put(memory,channel_stack+4,reference.at("flags"));put(memory,channel_stack+8,0);put(memory,channel_stack+12,0);
            bounded.gp[1]=channel;bounded.gp[4]=channel_stack;
            check(bounded.gp==reference.at("allocation_input_gp").get<decltype(bounded.gp)>()&&bounded.eflags==reference.at("allocation_input_eflags"),"Declared AC97 allocation caller differs");
            {
                SourceRuntimeSoundEpOutputResources::CallScope ep_scope(ep.get());
                SourceRuntimeSoundAc97OutputPages::CallScope page_scope(pages.get());
                channel_reset->run_cpu(0x38f104,bounded,*bounded.floating_point,scoped,observe);
            }
            check(instructions==allocated.at("instructions")&&stores==allocated.at("source_stores"),"Original whole AC97 channel constructor/reset CPU differs");
            check(bounded.gp==allocated.at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(allocated.at("returned_eflags").get<unsigned>()&0xcc5),"AC97 reset frontier registers/defined flags differ");
            equal_bytes(memory,0x2010000,allocated.at("stack_hex"),"AC97 complete declared caller stack differs");
            equal_bytes(memory,reference.at("controller"),allocated.at("controller_hex"),"Original AC97 source controller differs");
            for(const auto& row:allocated.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"AC97 retained raw object/refcount bytes differ");
            for(const auto& row:allocated.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"AC97 original shared-pool list/split/channel bytes differ");
            check(word(memory,0x390bc0)==allocated.at("raw_statistics")&&word(memory,0x390bc4)==allocated.at("physical_statistics")&&word(memory,0x390bcc)==allocated.at("pool_statistics")&&pages->diagnostics().source_exclusion_level==allocated.at("fs_level")&&memory.view(fs+0x24,1)[0]==std::byte{},"AC97 source allocation/statistics/exclusion domain differs");
            const auto actual=pages->diagnostics();check(!actual.faulted&&!actual.channel_ready&&!actual.producer_ready&&actual.allocations.size()==allocated.at("physical_allocations").size(),"AC97 native reset supplied channel/producer start readiness");
            unsigned i=0;for(const auto& row:actual.allocations){const auto& expected=allocated.at("physical_allocations").at(i++);check(row.source==expected.at("source")&&row.physical==expected.at("physical")&&row.requested==expected.at("requested")&&row.capacity==expected.at("capacity")&&row.channel==channel&&row.index==expected.at("index")&&row.field==expected.at("field")&&row.native.value>0xffffffffu&&row.native_channel.value>0xffffffffu&&memory.source_address(row.native,row.capacity)==row.source,"Actual AC97 source/physical/native64 page binding differs");equal_bytes(memory,row.source,expected.at("hex"),"Original AC97 CPU did not clear complete real capacity");}
            const auto native=channel_reset->diagnostics();check(!native.faulted&&!native.source_channel_started&&!native.producer_ready&&native.endpoints.size()==1&&native.endpoints[0].completed_resets==1&&native.endpoints[0].descriptor_table_bound&&
                !native.endpoints[0].resetting&&native.endpoints[0].native_descriptors==actual.allocations[0].native,"Actual endpoint reset did not complete worker/quiescence/descriptor binding");
            check(native.endpoints[0].pcm48_profile_bound==(kind==SourceRuntimeSoundAc97ChannelReset::Kind::Spdif)&&native.endpoints[0].iec958_channel_status==(kind==SourceRuntimeSoundAc97ChannelReset::Kind::Spdif?0x02000000u:0),"Native endpoint profile lost exact source consumerPCM48k identity");
            check(retained_endpoint->status().pending_packets==0&&retained_endpoint->status().completed_packets==2&&retained_endpoint->completion()->status==SdlSoundCompletionStatus::Flushed,"Native channel reset lost or falsely consumed real PCM receipts");
            rejected([&]{retained_endpoint->submit_native({pcm,0,32,444});});
            const auto constructor_instruction_count=instructions.size(),constructor_store_count=stores.size();
            for(const auto context:{111u,222u}){const auto receipt=retained_endpoint->completion();check(receipt&&receipt->packet_context==context,"Constructor reset PCM receipt changed");retained_endpoint->acknowledge(receipt->submission);}
            retained_endpoint=channel_reset->endpoint(kind);
            std::unique_ptr<SourceRuntimeSoundPcmBuffers> pcm_owner;
            if(!reference.at("pcm_binding").is_null()){
                const auto& expected=reference.at("pcm_binding");pcm_owner=std::make_unique<SourceRuntimeSoundPcmBuffers>(memory,dma,ep,pages,channel_reset,reset->control_lease());
                bounded.gp[1]=channel;bounded.gp[4]=0x201a000;put(memory,bounded.gp[4],expected.at("returned"));put(memory,bounded.gp[4]+4,ep->diagnostics().layout.data.physical);put(memory,bounded.gp[4]+8,0x2000);
                check(bounded.gp==expected.at("input_gp").get<decltype(bounded.gp)>()&&bounded.eflags==expected.at("input_eflags"),"Declared genuine PCM caller differs");instructions=Json::array();stores=Json::array();
                check(pcm_owner->try_bind({expected.at("instruction"),0x38ef59,false},bounded,*bounded.floating_point,scoped,observe),"Actual source default PCM binding was not owned");
                check(instructions==expected.at("instructions")&&stores==expected.at("source_stores")&&bounded.gp==expected.at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(expected.at("returned_eflags").get<unsigned>()&0xcc5),"Original pre-teardown PCM descriptor binding differs");
                check(source_runtime_sound_pcm_descriptor_bound(actual.allocations[0].native),"Actual native PCM descriptor binding was not retained");
            }
            check(retained_endpoint->submit_native({pcm,0,32,111})==SdlSoundPlatform::Submission::Accepted&&retained_endpoint->submit_native({pcm,0,32,222})==SdlSoundPlatform::Submission::Accepted,"Actual native pre-teardown PCM was not retained");
            SourceRuntimeSoundAc97PageRelease release_owner(memory,pages,channel_reset,reset->control_lease());
            bounded.gp[1]=reference.at("controller");bounded.gp[4]=0x201c000;put(memory,bounded.gp[4],0x2100000);put(memory,bounded.gp[4]+4,reference.at("index"));
            check(bounded.gp==reference.at("retirement_input_gp").get<decltype(bounded.gp)>()&&bounded.eflags==reference.at("retirement_input_eflags"),"Declared initialized source teardown caller differs");
            instructions=Json::array();stores=Json::array();unsigned registry_rejections{};
            if(pcm_owner){const auto bank_before=memory.checkpoint();const auto cpu_before=bounded.gp;
                rejected([&]{release_owner.run_retirement(bounded,*bounded.floating_point,scoped,observe);});++registry_rejections;
                check(instructions.empty()&&stores.empty()&&memory.checkpoint()==bank_before&&bounded.gp==cpu_before,"Live PCM binding rejection changed source CPU or bytes");
                pcm_owner->retire(kind);check(pcm_owner->channel_quiescent(kind)&&!source_runtime_sound_pcm_descriptor_bound(actual.allocations[0].native),"Actual PCM retirement did not unregister its descriptor page");
            }
            ep->apply_pending_cursor(0x38e7c4,0xfe80402c,0x600);ep->apply_pending_cursor(0x38e7ca,0xfe80403c,0x3800);
            {auto callback=ep->acquire_native_byte_transport();const auto bank_before=memory.checkpoint();const auto cpu_before=bounded.gp;
                rejected([&]{release_owner.run_retirement(bounded,*bounded.floating_point,scoped,observe);});
                check(instructions.empty()&&stores.empty()&&memory.checkpoint()==bank_before&&bounded.gp==cpu_before,"Live EP callback rejection changed source CPU or bytes");}
            unsigned foreign_release{};std::thread foreign_teardown([&]{auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_release;}};
                guard([&]{release_owner.diagnostics();});SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffff;guard([&]{release_owner.run_retirement(invalid,{0x27f,0x1f80},{});});});foreign_teardown.join();check(foreign_release==2,"Foreign teardown API read mutable source state");
            {SourceRuntimeSoundEpOutputResources::CallScope ep_scope(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope page_scope(pages.get());
                release_owner.run_retirement(bounded,*bounded.floating_point,scoped,observe);}
            check(instructions==reference.at("instructions")&&stores==reference.at("source_stores"),"Original whole initialized channel cleanup/free instruction/store sequence differs");
            check(bounded.gp==reference.at("returned_gp").get<decltype(bounded.gp)>()&&(bounded.eflags&0xcc5)==(reference.at("returned_eflags").get<unsigned>()&0xcc5),"Original initialized teardown ABI/defined flags differs");
            equal_bytes(memory,0x2010000,reference.at("stack_hex"),"Original initialized teardown complete caller stack differs");
            for(const auto& row:reference.at("retained_raw_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original initialized teardown raw/refcount bytes differ");
            for(const auto& row:reference.at("retained_dma_hex"))equal_bytes(memory,row.at("source"),row.at("hex"),"Original initialized teardown shared-pool bytes differ");
            check(word(memory,0x390bc0)==reference.at("raw_statistics")&&word(memory,0x390bc4)==reference.at("physical_statistics")&&word(memory,0x390bcc)==reference.at("pool_statistics")&&!word(memory,reference.at("controller")),"Original initialized teardown did not own all statistics/channel clears");
            const auto released=release_owner.diagnostics();check(!released.faulted&&!released.source_started&&!released.producer_ready&&released.released.size()==reference.at("released_allocations").size()&&pages->diagnostics().allocations.empty(),"Actual initialized teardown lost physical ownership or claimed producer readiness");
            for(const auto& row:released.released){rejected([&]{memory.view(row.source,row.capacity);});rejected([&]{memory.source_address(row.native,row.capacity);});}
            const auto retired=channel_reset->diagnostics();check(retired.endpoints[0].admission_closed&&!retired.endpoints[0].descriptor_table_bound&&retained_endpoint->status().paused&&retained_endpoint->status().pending_packets==0&&retained_endpoint->status().completed_packets==2,"Actual initialized teardown lost admission closure or PCM receipts");
            rejected([&]{channel_reset->endpoint(kind);});rejected([&]{retained_endpoint->submit_native({pcm,0,32,444});});
            unsigned control_rejections{};
            {SourceRuntimeSoundResetResources other_reset(memory,output,fs);auto repeated=state;repeated.gp[4]=0x201b000;repeated.gp[6]=repeated.gp[1];put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
                check(other_reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Independent same-bank control worker did not execute genuine cold reset");
                const auto other=other_reset.control_lease();check(other.generation()==reset->control_lease().generation()&&!other.same_owner(reset->control_lease()),"Same-bank control fixture did not retain equal-generation distinct workers");
                const auto before=memory.checkpoint();auto deny=[&](auto action){rejected(action);++control_rejections;check(memory.checkpoint()==before,"Rejected distinct-worker composition changed source bytes");};
                deny([&]{pages->require_control(memory,other);});deny([&]{channel_reset->require_control(memory,other);});
                deny([&]{SourceRuntimeSoundAc97ChannelReset wrong(memory,pages,output,other);});
                deny([&]{SourceRuntimeSoundAc97PageRelease wrong(memory,pages,channel_reset,other);});
            }
            // Negative corruption fixture, separate from the exact original
            // comparison: an empty source retirement returns, then its slot
            // is inconsistent. No implementation cleanup/clear is supplied.
            auto inconsistent=bounded;inconsistent.gp[1]=reference.at("controller");inconsistent.gp[4]=0x201d000;put(memory,inconsistent.gp[4],0x2100000);put(memory,inconsistent.gp[4]+4,reference.at("index"));
            auto mismatch_services=scoped;bool injected=false;mismatch_services.instruction=[&](auto pc){if(pc==0x38f3d1){put(memory,reference.at("controller").get<unsigned>()+reference.at("index").get<unsigned>()*4,0xffffffff);injected=true;}};
            bool mismatched=false;try{release_owner.run_retirement(inconsistent,*inconsistent.floating_point,mismatch_services);}catch(const std::exception& error){mismatched=std::string(error.what()).find("did not finish its owned page/channel cleanup")!=std::string::npos;}
            check(injected&&mismatched&&release_owner.diagnostics().faulted&&!channel_reset->diagnostics().faulted,"Post-source-return cleanup mismatch did not reach the release owner guard");
            rejected([&]{reset->control_lease().generation();});check(retained_endpoint->status().completed_packets==2,"Post-return mismatch abort lost native PCM receipts");
            evidence={{"fill",reference.at("fill")},{"index",reference.at("index")},{"flags",reference.at("flags")},{"channel_constructor_instructions",reference.at("constructor").at("instructions").size()},{"allocation_reset_instructions",constructor_instruction_count},{"allocation_reset_stores",constructor_store_count},{"source_teardown_instructions",instructions.size()},{"source_teardown_stores",stores.size()},{"actual_native_pages_released",released.released.size()},{"live_pcm_binding_rejections",registry_rejections},{"live_ep_callback_rejected_before_stores",true},{"foreign_release_api_rejections",foreign_release},{"source_channel_admission_closed",true},{"source_owned_field_and_statistics_cleanup",true}};
            evidence["same_bank_output_equal_generation_different_worker_rejections"]=control_rejections;
            evidence["post_source_return_cleanup_mismatch_aborts_actual_control"]=true;
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
    check(stopped&&frontiers==1&&pages&&channel_reset&&channel_reset->diagnostics().faulted&&word(memory,0x2001000)==0,"AC97 reset bypassed full SDK frontend or published device success");
    const auto retained=pages->diagnostics().allocations;for(const auto& row:retained)check(memory.native_address(row.source,row.capacity)==row.native,"Partial AC97 diagnostic pages disappeared before creating-thread teardown");
    std::weak_ptr<SourceRuntimeSoundAc97OutputPages> weak_pages=pages;channel_reset.reset();pages.reset();check(!weak_pages.expired(),"Unaccounted actual PCM lease lost its retained native pages");
    for(const auto context:{111u,222u}){const auto receipt=retained_endpoint->completion();check(receipt&&receipt->packet_context==context&&receipt->status==SdlSoundCompletionStatus::Flushed,"Actual endpoint receipts were lost/reordered after graph teardown");retained_endpoint->acknowledge(receipt->submission);}
    retained_endpoint->retire();check(weak_pages.expired(),"Creating-thread endpoint retirement retained its last actual page lease");
    rejected([&]{retained_endpoint->status();});retained_endpoint.reset();for(const auto& row:retained)rejected([&]{memory.view(row.source,row.capacity);});
    ep->retire();ep.reset();reset.reset();preference.retire();preference.require_quiescent();
    check(SDL_GetAudioDeviceStatus(output->device_id())==SDL_AUDIO_PLAYING,"AC97 page cleanup closed its shared actual output");
    evidence["passed"]=true;evidence["genuine_prepage_control_generation_path_and_initialized_admission_closed"]=true;evidence["actual_pcm_receipts_preserved_after_graph_teardown"]=true;evidence["source_start_or_producer_ready_supplied"]=false;evidence["source_descriptor_pages_released_on_creating_thread"]=true;return evidence;
}
}
int main(int argc,char** argv){try{
    check(argc==5,"Supply original AC97 reset/retirement references, private config directory and native report");std::ifstream input(argv[1]);Json reference;input>>reference;
    check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&reference.at("instruction_patches").empty()&&reference.at("supplied_game_or_codec_results").empty(),"Original AC97 reference differs");
    std::ifstream retirement_input(argv[2]);Json retirement;retirement_input>>retirement;check(retirement.at("passed")==true&&retirement.at("original_xbe_sha256")==reference.at("original_xbe_sha256")&&retirement.at("instruction_patches").empty(),"Original channel retirement reference differs");
    const std::filesystem::path directory(argv[3]);std::filesystem::create_directories(directory);const auto config=directory/"menu.cfg";{std::ofstream file(config);file<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    Json cases=Json::array();for(const auto& row:reference.at("cases"))cases.push_back(run(row,output,config,retirement.at("cases").at(0)));
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"cases",cases},{"real_shared_sdk_dma_ep_reset_owners",true},{"actual_native_initialized_page_release",true},{"channel_codec_interrupt_start_or_program_ready",false},{"whole38f45f_or38e484_return_supplied",false},{"source_sdk_output_published",false},{"full_startup_completed",false}};
    std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save AC97 reset evidence");std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
