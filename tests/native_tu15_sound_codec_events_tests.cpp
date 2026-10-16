#include "halo2/source_runtime_sound_codec_event_port.hpp"
#include "halo2/source_runtime_sound_ac97_page_release.hpp"
#include "halo2/native_tu15_sound_codec_initialization_data.hpp"
#include "halo2/native_tu15_sound_ac97_retire_data.hpp"
#include <atomic>
#include "halo2/native_tu15_audio_preference.hpp"
#include "halo2/native_tu15_sound_sdk_prefix_data.hpp"
#include "halo2/native_tu15_sound_ac97_output_data.hpp"
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
#include <fcntl.h>
#include <sys/epoll.h>
#include <cerrno>
#include <new>
#include <cstdlib>
thread_local long codec_fail_allocation=-1;
void* operator new(std::size_t n){if(codec_fail_allocation==0)throw std::bad_alloc();if(codec_fail_allocation>0)--codec_fail_allocation;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace halo2;
namespace {
using Json=nlohmann::json;using Kind=SourceRuntimeSoundPcmBuffers::Kind;
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Invalid native PCM binding was accepted");}
void put(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));m.write(p,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t p){const auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
std::vector<std::byte> unhex(const std::string& s){std::vector<std::byte> b(s.size()/2);for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));return b;}
void equal(const SourceRuntimeMemory& m,std::uint32_t p,const Json& hex,const char* message){const auto b=unhex(hex.get<std::string>());check(std::ranges::equal(m.view(p,b.size()),b),message);}
struct Frontier{};
using Inspect=std::function<void(SourceRuntimeMemory&,SourceRuntimeSoundSdkResources&,
    const std::shared_ptr<SourceRuntimeSoundDmaResources>&,const std::shared_ptr<SourceRuntimeSoundEpOutputResources>&,
    const std::shared_ptr<SourceRuntimeSoundAc97OutputPages>&,const std::shared_ptr<SourceRuntimeSoundAc97ChannelReset>&,
    SourceRuntimeSoundResetResources&)>;
void prefix(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,const Inspect& inspect){
    constexpr std::uint32_t fs=0x60200000,stack=0x2008000;
    SourceRuntimeMemory memory;mount_native_tu15_sound_sdk_prefix_data(memory);mount_native_tu15_sound_dma_imports(memory);mount_native_tu15_sound_ac97_output_data(memory);
    memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared SDK and authentic bounded PCM callers");
    memory.mount(fs,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual passive native PCM executor");
    NativeTu15AudioPreference preference(memory,output,native_tu15_load_audio_preference(config));preference.publish();
    SourceRuntimeSoundSdkResources sdk(memory,fs);auto dma=std::make_shared<SourceRuntimeSoundDmaResources>(memory,output);SourceRuntimeSoundResetResources reset(memory,output,fs);
    SourcePhysicsWorldRuntimeAbi abi{};abi.fs_base=fs;abi.gp[4]=stack;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};SourcePhysicsWorldRuntimeServices services;
    for(auto entry:{0x387baau,0x387bb5u,0x38a988u,0x38a993u}){put(memory,stack,0x2100000);abi.gp[4]=stack;source_run_sound_sdk_tu15(memory,entry,abi,*abi.floating_point,services);}
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observer){
        if(call.target==0x38e8c9){check(call.instruction==0x38eca3&&word(memory,state.gp[4])==0x38eca8&&dma->allocations().size()==16,"PCM prerequisite did not follow genuine SDK/reset/storage");
            auto ep=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);auto pages=std::make_shared<SourceRuntimeSoundAc97OutputPages>(memory,dma,ep,output,reset.control_lease());auto channels=std::make_shared<SourceRuntimeSoundAc97ChannelReset>(memory,pages,output,reset.control_lease());
            inspect(memory,sdk,dma,ep,pages,channels,reset);throw Frontier{};}
        if(reset.try_call(bank,call,state,*state.floating_point,services,observer)||dma->dispatch(bank,call,state)||sdk.dispatch(bank,call,state,observer))return;
        if(source_sound_hardware_storage_tu15_owns_entry(call.target)){source_run_sound_hardware_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,*state.floating_point,services,observer);return;}
        throw std::runtime_error("Unowned SDK prerequisite before native PCM buffer binding");
    };
    put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0x2001000);put(memory,stack+12,0);abi.gp[4]=stack;
    bool stopped=false;try{SourceRuntimeSoundSdkResources::CallScope s(&sdk);SourceRuntimeSoundDmaResources::CallScope d(dma.get());SourceRuntimeSoundResetResources::CallScope r(&reset);source_run_sound_sdk_tu15(memory,0x387877,abi,*abi.floating_point,services);}catch(const Frontier&){stopped=true;}
    check(stopped&&word(memory,0x2001000)==0,"PCM prerequisite supplied source SDK/factory success");preference.retire();
}
std::string hex(std::span<const std::byte> bytes){static const char* d="0123456789abcdef";std::string s;for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);s+=d[v>>4];s+=d[v&15];}return s;}
Json snapshot(const SourceRuntimeMemory& m){Json rows=Json::array();m.visit_committed([&](auto p,auto bytes){rows.push_back({{"address",p},{"hex",hex(bytes)}});});return rows;}
struct Interrupted{};
Json exercise(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,unsigned fill,unsigned mode){
    Json evidence;
    prefix(output,config,[&](auto& memory,auto& sdk,const auto& dma,const auto& ep,const auto& pages,const auto& channels,auto& reset){
        mount_native_tu15_sound_codec_initialization_data(memory);mount_native_tu15_sound_codec_event_data(memory);mount_native_tu15_sound_ac97_page_release_data(memory);mount_native_tu15_sound_ac97_retire_data(memory);
        SourceProcessShutdownRegistry registry(memory,0x60210000);
        auto buffers=std::make_shared<SourceRuntimeSoundPcmBuffers>(memory,dma,ep,pages,channels,reset.control_lease());
        auto codec=std::make_shared<SourceRuntimeSoundCodecRoute>(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);
        SourceRuntimeSoundCodecEventPort port(memory,output,reset.control_lease(),codec,buffers,registry);
        rejected([&]{SourceRuntimeSoundCodecEventPort duplicate(memory,output,reset.control_lease(),codec,buffers,registry);});
        SourceRuntimeMemory foreign;SourceProcessShutdownRegistry foreign_registry(foreign,0x60210000);
        rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,output,reset.control_lease(),codec,buffers,foreign_registry);});
        unsigned composition_guards{};
        if(!mode&&!fill){
            const auto original=memory.checkpoint();
            rejected([&]{SourceRuntimeSoundCodecEventPort wrong(foreign,output,reset.control_lease(),codec,buffers,foreign_registry);});++composition_guards;
            auto distinct_output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
            check(distinct_output->device_id()!=output->device_id(),"Distinct codec output fixture did not open its own actual native device");
            rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,distinct_output,reset.control_lease(),codec,buffers,registry);});++composition_guards;
            prefix(output,config,[&](auto& other_memory,auto&,const auto& other_dma,const auto& other_ep,const auto& other_pages,const auto& other_channels,auto& other_reset){
                auto other_buffers=std::make_shared<SourceRuntimeSoundPcmBuffers>(other_memory,other_dma,other_ep,other_pages,other_channels,other_reset.control_lease());
                auto other_codec=std::make_shared<SourceRuntimeSoundCodecRoute>(other_memory,output,other_reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);
                rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,output,reset.control_lease(),other_codec,buffers,registry);});++composition_guards;
                rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,output,reset.control_lease(),codec,other_buffers,registry);});++composition_guards;
                rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,output,other_reset.control_lease(),codec,buffers,registry);});++composition_guards;
            });
            check(memory.checkpoint()==original,"Rejected foreign/hybrid codec completion owner changed its current source bank");
            SourceRuntimeSoundResetResources other(memory,output,0x60200000);SourcePhysicsWorldRuntimeAbi repeated{};
            repeated.fs_base=0x60200000;repeated.eflags=0x202;repeated.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            repeated.gp[1]=reset.control_lease().source_apu()+8;repeated.gp[6]=repeated.gp[1];repeated.gp[4]=0x201d000;
            put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
            check(other.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Distinct codec worker fixture did not execute genuine whole reset");
            check(other.control_lease().generation()==reset.control_lease().generation()&&!other.control_lease().same_owner(reset.control_lease()),"Distinct codec workers do not have equal real generations and separate immutable identities");
            const auto unchanged=memory.checkpoint();rejected([&]{SourceRuntimeSoundCodecEventPort wrong(memory,output,other.control_lease(),codec,buffers,registry);});++composition_guards;
            check(memory.checkpoint()==unchanged&&!port.state().faulted,"Rejected distinct worker codec owner changed the current route or bank");
        }
        constexpr unsigned stack=0x2018000,fs=0x60200000;const auto controller=reset.control_lease().source_apu()+0x1c;
        SourcePhysicsWorldRuntimeAbi cpu{};cpu.fs_base=fs;cpu.eflags=0x202;cpu.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};cpu.gp[1]=controller;cpu.gp[4]=stack;
        put(memory,stack,0x2100000);put(memory,stack+4,1);
        SourcePhysicsWorldRuntimeServices construction;
        construction.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){
            if(port.try_call(bank,call,abi,observer)||pages->dispatch(bank,call,abi)||sdk.dispatch(bank,call,abi,observer))return;
            throw std::runtime_error("Unowned actual codec/channel constructor resource");};
        {SourceRuntimeSoundCodecEventPort::CallScope scope(&port);codec->run_cpu(0x38f45f,cpu,*cpu.floating_point,construction);}
        check(cpu.gp[0]==0&&port.state().connected&&port.state().shutdown_registered&&registry.records().size()==1,"Actual codec constructor did not complete bounded native registration");
        {const auto unchanged=memory.checkpoint();rejected([&]{SourceRuntimeSoundCodecEventPort duplicate(memory,output,reset.control_lease(),codec,buffers,registry);});check(memory.checkpoint()==unchanged&&port.state().connected&&port.state().shutdown_registered&&registry.records().size()==1,"Rejected duplicate invalidated its retained actual codec registration");}
        for(unsigned index=0;index<2;++index){
            put(memory,stack,0x2100000);put(memory,stack+4,index);put(memory,stack+8,0);cpu.gp[1]=controller;cpu.gp[4]=stack;
            channels->run_cpu(0x38eef1,cpu,*cpu.floating_point,construction);check(cpu.gp[0]==0,"Original codec test channel constructor failed");
            const auto channel=word(memory,controller+index*4);channels->bind_channel(channel);
            put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0);put(memory,stack+12,0);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38f104,cpu,*cpu.floating_point,construction);}
            const auto site=index?0x38e524u:0x38e4d1u;put(memory,stack,site+5);put(memory,stack+4,ep->diagnostics().layout.data.physical);put(memory,stack+8,0x2000);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());check(buffers->try_bind({site,0x38ef59,false},cpu,*cpu.floating_point,construction),"Authentic codec test PCM binding absent");}
        }
        const auto binding=buffers->buffer(Kind::Analog).binding;
        unsigned index_control_calls{},pre_claim_index_rejections{},pre_delivery_index_rejections{};
        auto endpoint_state=[&]{const auto rows=channels->diagnostics().endpoints;
            const auto at=std::ranges::find_if(rows,[&](const auto& row){return row.kind==Kind::Analog;});
            check(at!=rows.end(),"Native index regression lost its actual analog endpoint");return *at;};
        auto reindex=[&](unsigned current,unsigned count,unsigned publish_current,unsigned publish_last){
            // Whole original38EDF4 updates the native controls separately from
            // source count/current. Keep the pending ISR's CPU/frame intact.
            constexpr unsigned index_stack=0x2019000;auto index_cpu=cpu;
            put(memory,index_stack,0x2100000);put(memory,index_stack+4,current);put(memory,index_stack+8,count);
            put(memory,index_stack+12,publish_current);put(memory,index_stack+16,publish_last);
            index_cpu.gp[1]=binding.channel;index_cpu.gp[4]=index_stack;
            channels->run_cpu(0x38edf4,index_cpu,*index_cpu.floating_point,construction);++index_control_calls;
        };
        auto drift_native_index=[&](bool last){const auto generation=endpoint_state().generation;
            if(last)reindex(0,2,0,1);else reindex(5,32,1,0);
            reindex(0,1,0,0);const auto endpoint=endpoint_state();
            check(endpoint.source_current==(last?0:5)&&endpoint.source_last==(last?1:0)&&endpoint.generation==generation&&
                memory.view(binding.channel+0x24,2)[0]==std::byte{1}&&memory.view(binding.channel+0x24,2)[1]==std::byte{}&&
                word(memory,binding.descriptor_source)==binding.buffer_physical&&word(memory,binding.descriptor_source+4)==0xc0001000,
                "Authentic index controls did not retain nonzero native index with valid default source descriptor");
        };
        auto restore_native_indices=[&]{const auto generation=endpoint_state().generation;reindex(0,1,1,1);const auto endpoint=endpoint_state();
            check(!endpoint.source_current&&!endpoint.source_last&&endpoint.generation==generation&&
                memory.view(binding.channel+0x24,2)[0]==std::byte{1}&&memory.view(binding.channel+0x24,2)[1]==std::byte{},
                "Whole original index controls did not restore the actual default native/source indices");};
        // Explicit low-amplitude diagnostic source bytes, not DSP-produced or
        // MCC gameplay PCM. They occupy the genuinely owned descriptor span.
        auto raw=memory.mutable_view(binding.buffer_source,0x2000);
        for(unsigned i=0;i<4096;++i){const std::int16_t sample=std::int16_t((i%31)-15)*8;const auto bits=std::uint16_t(sample);raw[i*2]=std::byte(bits);raw[i*2+1]=std::byte(bits>>8);}
        const auto unrelated=channels->endpoint(Kind::Analog);
        unsigned allocation_failures{};
        if(!mode&&!fill){std::vector<std::int16_t> source(4096);for(unsigned i=0;i<source.size();++i){const std::uint16_t bits=std::to_integer<unsigned>(raw[i*2])|(std::to_integer<unsigned>(raw[i*2+1])<<8);source[i]=std::bit_cast<std::int16_t>(bits);}
            auto converted=std::make_shared<const std::vector<float>>(codec->convert_native(SourceRuntimeSoundCodecRoute::Kind::Analog,source));const auto checkpoint=memory.checkpoint();bool succeeded=false;
            for(long n=0;n<256&&!succeeded;++n){codec_fail_allocation=n;try{auto lease=channels->claim_default_analog_consumer(buffers,converted);codec_fail_allocation=-1;lease->cancel();succeeded=true;}catch(const std::bad_alloc&){codec_fail_allocation=-1;++allocation_failures;}
                check(memory.checkpoint()==checkpoint&&!channels->endpoint(Kind::Analog)->status().pending_packets&&!channels->endpoint(Kind::Analog)->status().completed_packets,"Consumer allocation failure changed source or native packet state");}
            check(succeeded&&allocation_failures>=4,"Consumer allocation-failure proof did not reach staged lease/control-block metadata");}
        if(!mode)for(bool last:{false,true}){
            drift_native_index(last);const auto checkpoint=memory.checkpoint();
            rejected([&]{port.consume_default_analog();});const auto status=unrelated->status();
            check(memory.checkpoint()==checkpoint&&!port.state().consumer&&!port.state().faulted&&!port.state().pending&&
                !port.receipt_owner()&&status.paused&&!status.pending_packets&&!status.completed_packets,
                "Nonzero native descriptor index admitted a claim or changed source/native packet state");
            ++pre_claim_index_rejections;restore_native_indices();
        }
        port.consume_default_analog();const auto receipt_owner=port.receipt_owner();check(receipt_owner&&receipt_owner->native_voice()==binding.native_voice,"Actual codec consumer did not retain its exact endpoint");
        rejected([&]{port.consume_default_analog();});
        const auto silent=std::make_shared<const std::vector<float>>(8,0.0f);rejected([&]{unrelated->submit_native({silent,0,4,0x99});});
        unsigned foreign_guards{};const auto foreign_checkpoint=memory.checkpoint();std::thread wrong([&]{
            SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffff;
            for(auto action:{std::function<void()>([&]{port.state();}),std::function<void()>([&]{port.collect_success();}),std::function<void()>([&]{port.retire();}),
                std::function<void()>([&]{port.consume_default_analog();}),std::function<void()>([&]{port.dispatch(invalid,{0x27f,0x1f80},{});}),
                std::function<void()>([&]{port.dispatch_shutdown(invalid,{0x27f,0x1f80},{});}),std::function<void()>([&]{port.request_shutdown();}),
                std::function<void()>([&]{port.receipt_owner();}),std::function<void()>([&]{port.try_call(foreign,{0,0,false},invalid);}),
                std::function<void()>([&]{port.try_call(foreign,{0x38f4c4,0x8000002c,false},invalid);}),
                std::function<void()>([&]{SourceRuntimeSoundCodecEventPort::CallScope scope(&port);})})
                try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_guards;}
        });wrong.join();check(foreign_guards==11&&memory.checkpoint()==foreign_checkpoint,"Codec event foreign guard did not run before source frame, bank or mutable state access");
        if(mode==4){std::set_terminate([]{std::_Exit(77);});std::thread foreign([&]{port.~SourceRuntimeSoundCodecEventPort();});foreign.join();throw std::runtime_error("Foreign codec event destructor returned");}
        if(mode==6){check(receipt_owner->status().pending_packets==1,"Failure fixture was not actually pending");check(output->device_removed(output->device_id()),"Explicit native removal event did not match actual output");
            rejected([&]{port.collect_success();});const auto receipt=receipt_owner->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Failure&&port.state().faulted&&!port.state().pending&&!port.state().connected&&word(memory,binding.channel+4)==0,"Failure receipt fabricated source success or was discarded");
            receipt_owner->acknowledge(receipt->submission);evidence={{"mode","failure"},{"explicit_native_device_removal_event_input",true},{"actual_failure_receipt_preserved",true},{"source_completion_published",false}};return;}
        if(mode==1){
            check(receipt_owner->status().pending_packets==1,"Cancellation fixture was not actually pending");port.retire();const auto receipt=receipt_owner->completion();
            check(receipt&&receipt->status==SdlSoundCompletionStatus::Flushed&&word(memory,binding.channel+4)==0&&!port.state().pending,"Cancelled native descriptor fabricated source completion");
            rejected([&]{port.collect_success();});receipt_owner->acknowledge(receipt->submission);
            evidence={{"mode","flushed"},{"explicitly_accounted_flushed_receipt",true},{"source_channel_words",word(memory,binding.channel+4)}};return;
        }
        if(mode==3){
            check(receipt_owner->status().pending_packets==1,"Shutdown fixture was not actually pending");
            SourceRuntimeSoundAc97PageRelease release(memory,pages,channels,reset.control_lease());SourcePhysicsWorldRuntimeServices retirement;
            retirement.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){
                if(call.target==0x38f395){release.run_retirement(abi,*abi.floating_point,retirement,observer);return;}
                if(sdk.dispatch(bank,call,abi,observer))return;throw std::runtime_error("Unowned codec shutdown channel resource");};
            port.request_shutdown();put(memory,stack,0x2100000);put(memory,stack+4,controller+0xc);cpu.gp[4]=stack;
            check(port.dispatch_shutdown(cpu,*cpu.floating_point,retirement),"Actual codec shutdown endpoint did not dispatch its registered source callback");
            check(registry.records().empty()&&!port.state().connected&&!port.state().shutdown_registered&&!word(memory,0x391208)&&!word(memory,controller+0xc)&&!word(memory,controller)&&!word(memory,controller+4),"Original codec shutdown failed genuine channel/registration cleanup");
            const auto receipt=receipt_owner->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Flushed&&release.diagnostics().released.size()==2,"Shutdown lost its actual Flushed packet or physical page cleanup");
            receipt_owner->acknowledge(receipt->submission);receipt_owner->retire();port.retire();evidence={{"mode","shutdown"},{"actual_registered_source_callback",true},{"physical_pages_released",2},{"flushed_receipt_preserved",true}};return;
        }
        const auto deadline=SDL_GetTicks64()+5000;while(!port.collect_success()){check(SDL_GetTicks64()<deadline,"Actual codec callback did not consume its descriptor");SDL_Delay(1);}
        check(port.state().pending&&receipt_owner->completion()->status==SdlSoundCompletionStatus::Success,"Actual codec Success did not produce its own event");
        {const auto unchanged=memory.checkpoint();rejected([&]{SourceRuntimeSoundCodecEventPort duplicate(memory,output,reset.control_lease(),codec,buffers,registry);});
            check(memory.checkpoint()==unchanged&&port.state().pending&&port.state().connected&&port.state().shutdown_registered&&receipt_owner->completion()->status==SdlSoundCompletionStatus::Success,"Rejected pending duplicate changed the actual codec event or retained Success receipt");}
        std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(fill));
        for(unsigned i=0;i<cpu.gp.size();++i)cpu.gp[i]=0x21000000+i*0x111111;cpu.gp[4]=stack;cpu.eflags=0x202;
        put(memory,stack,0x2100000);put(memory,stack+4,0x391208);put(memory,stack+8,controller);
        const auto before=memory.checkpoint();put(memory,stack+8,controller+4);rejected([&]{port.dispatch(cpu,*cpu.floating_point,{});});put(memory,stack+8,controller);
        check(memory.checkpoint()==before&&port.state().pending,"Invalid ISR frame consumed actual event or changed source");
        unsigned drift_rejections{};
        SourcePhysicsWorldRuntimeServices guarded;unsigned forbidden_instructions{};guarded.instruction=[&](auto){++forbidden_instructions;};
        for(bool last:{false,true}){
            drift_native_index(last);const auto checkpoint=memory.checkpoint();const auto saved_gp=cpu.gp;
            const auto saved_flags=cpu.eflags;const auto receipt=receipt_owner->completion();
            check(receipt&&receipt->status==SdlSoundCompletionStatus::Success,"Native index drift lost its pending actual Success receipt");
            rejected([&]{port.dispatch(cpu,*cpu.floating_point,guarded);});const auto retained=receipt_owner->completion();
            check(!forbidden_instructions&&memory.checkpoint()==checkpoint&&cpu.gp==saved_gp&&cpu.eflags==saved_flags&&
                port.state().pending&&!port.state().faulted&&retained&&retained->voice==receipt->voice&&
                retained->channel_context==receipt->channel_context&&retained->packet_context==receipt->packet_context&&
                retained->submission==receipt->submission&&retained->status==receipt->status&&retained->pcm_frames==receipt->pcm_frames,
                "Nonzero native descriptor index consumed an event, executed source CPU or changed its retained Success receipt");
            ++pre_delivery_index_rejections;restore_native_indices();
        }
        for(const auto& [at,n,value]:std::vector<std::tuple<unsigned,unsigned,unsigned>>{{binding.buffer_source,1,0x7f},{binding.descriptor_source,4,binding.buffer_physical+4},{binding.descriptor_source+4,4,0xc0000fff},{binding.channel,4,1},{binding.channel+0xc,4,1},{binding.channel+0x10,4,0x38f443},{binding.channel+0x14,4,0x99},{controller+8,4,0},{binding.channel+0x24,1,2},{binding.channel+0x25,1,1}}){
            const auto old=std::vector<std::byte>(memory.view(at,n).begin(),memory.view(at,n).end());std::array<std::byte,4> changed{};for(unsigned i=0;i<n;++i)changed[i]=std::byte(value>>(8*i));memory.write(at,std::span(changed).first(n));const auto checkpoint=memory.checkpoint();
            rejected([&]{port.dispatch(cpu,*cpu.floating_point,guarded);});check(!forbidden_instructions&&memory.checkpoint()==checkpoint&&port.state().pending&&!port.state().faulted&&receipt_owner->completion()->status==SdlSoundCompletionStatus::Success,"Drifted descriptor branch consumed an event or executed source CPU");memory.write(at,old);++drift_rejections;}
        const auto input=snapshot(memory);const auto input_gp=cpu.gp;Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices trace;trace.instruction=[&](auto pc){instructions.push_back(pc);};
        SourcePhysicsContactRecordObserver record=[&](const auto& row){stores.push_back({{"instruction",row.instruction},{"address",row.address},{"before",hex(row.before)},{"written",hex(row.written)}});if(mode==2&&row.instruction==0x38f215)throw Interrupted{};};
        if(mode==2){bool interrupted=false;try{port.dispatch(cpu,*cpu.floating_point,trace,record);}catch(const Interrupted&){interrupted=true;}
            check(interrupted&&port.state().faulted&&!port.state().connected&&!port.state().shutdown_registered&&reset.state().faulted&&word(memory,binding.channel+4)==0x1000,"Partial original accounting did not fault/cancel shared native route");
            const auto receipt=receipt_owner->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Success,"Interruption discarded its original Success receipt");receipt_owner->acknowledge(receipt->submission);
            evidence={{"mode","interrupted"},{"original_partial_store_retained",true},{"actual_success_receipt_preserved",true},{"shared_worker_faulted",true}};return;
        }
        check(port.dispatch(cpu,*cpu.floating_point,trace,record)&&port.state().delivered==1&&!port.state().pending&&word(memory,binding.channel+4)==0x1000&&!receipt_owner->completion(),"Whole source ISR did not account exactly one real descriptor");
        evidence={{"mode","success"},{"fill",fill},{"input_regions",input},{"input_gp",input_gp},{"input_eflags",0x202},{"fs_base",fs},{"instructions",instructions},{"stores",stores},{"output_gp",cpu.gp},{"output_eflags",cpu.eflags},{"output_regions",snapshot(memory)},{"source_channel_words",0x1000},{"native_packet_frames",2048},{"foreign_guards",foreign_guards},{"composition_guards",composition_guards},{"duplicate_domain_rejections",3},{"pre_delivery_drift_rejections",drift_rejections},{"pre_claim_native_index_rejections",pre_claim_index_rejections},{"pre_delivery_native_index_rejections",pre_delivery_index_rejections},{"whole_original_index_control_calls",index_control_calls},{"actual_metadata_allocation_failures",allocation_failures},{"source_started",false},{"dsp_pcm_producer",false}};
        port.retire();check(registry.records().empty(),"Native retirement retained a dead shutdown registration");
    });return evidence;
}
Json registration_failure(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,unsigned failure_store){
    Json evidence;prefix(output,config,[&](auto& memory,auto&,const auto& dma,const auto& ep,const auto& pages,const auto& channels,auto& reset){
        mount_native_tu15_sound_codec_initialization_data(memory);mount_native_tu15_sound_codec_event_data(memory);
        SourceProcessShutdownRegistry registry(memory,0x60210000);auto pcm=std::make_shared<SourceRuntimeSoundPcmBuffers>(memory,dma,ep,pages,channels,reset.control_lease());
        auto codec=std::make_shared<SourceRuntimeSoundCodecRoute>(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);
        SourceRuntimeSoundCodecEventPort port(memory,output,reset.control_lease(),codec,pcm,registry);
        SourcePhysicsWorldRuntimeAbi cpu{};cpu.fs_base=0x60200000;cpu.eflags=0x202;cpu.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};cpu.gp[1]=reset.control_lease().source_apu()+0x1c;cpu.gp[4]=0x2018000;
        put(memory,cpu.gp[4],0x2100000);put(memory,cpu.gp[4]+4,1);unsigned stores{};int prior_event=-1,prior_dispatcher=-1,prior_shutdown=-1;bool caught=false;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){if(!failure_store&&pc==0x38f4e5){const auto state=port.state();prior_event=state.event_descriptor;prior_dispatcher=state.dispatcher_descriptor;
            epoll_event event{};event.events=EPOLLIN;event.data.u64=1;check(epoll_ctl(prior_dispatcher,EPOLL_CTL_ADD,prior_event,&event)==0,"Could not create actual duplicate epoll registration failure fixture");}};
        services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){if(port.try_call(bank,call,abi,observer))return;throw std::runtime_error("Unowned registration-failure request");};
        SourcePhysicsContactRecordObserver observer=[&](const auto& row){if(row.instruction==0x38f52f){const auto state=port.state();prior_event=state.event_descriptor;prior_dispatcher=state.dispatcher_descriptor;prior_shutdown=state.shutdown_descriptor;if(++stores==failure_store)throw Interrupted{};}};
        try{SourceRuntimeSoundCodecEventPort::CallScope scope(&port);codec->run_cpu(0x38f45f,cpu,*cpu.floating_point,services,observer);}catch(const Interrupted&){caught=true;}catch(const std::system_error& error){check(!failure_store&&error.code().value()==EEXIST,"Codec connection failure was not actual kernel EEXIST");caught=true;}
        const auto state=port.state();check(caught&&state.faulted&&!state.connected&&!state.shutdown_registered&&reset.state().faulted&&registry.records().empty(),"Failed codec registration left an owned record linked or live route");
        check(state.event_descriptor==-1&&state.shutdown_descriptor==-1&&state.dispatcher_descriptor==-1,"Failed codec registration retained actual native FDs");
        for(int fd:{prior_event,prior_dispatcher,prior_shutdown})if(fd>=0){errno=0;check(fcntl(fd,F_GETFD)==-1&&errno==EBADF,"Failed codec registration did not close its actual native resource");}
        evidence={{"registration_store_failure",failure_store},{"actual_epoll_eexist_failure",!failure_store},{"owned_record_removed",true},{"registry_traversable",true},{"actual_fds_closed",true},{"source_diagnostic_callback_retained",word(memory,state.controller+0xc)==(failure_store?0x38f443u:0u)}};
    });return evidence;
}
}
int main(int argc,char** argv){try{const bool foreign_destructor=argc==4&&std::string(argv[1])=="--foreign-destructor-probe";const unsigned shift=foreign_destructor?1:0;check(argc==int(3+shift),"Supply private config directory and codec event report");std::filesystem::create_directories(argv[1+shift]);const auto config=std::filesystem::path(argv[1+shift])/"menu.cfg";{std::ofstream out(config);out<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});
    if(foreign_destructor){exercise(output,config,0x5a,4);throw std::runtime_error("Foreign event destructor did not exit77");}
    Json cases=Json::array();for(unsigned fill:{0u,0x5au,0xa5u})cases.push_back(exercise(output,config,fill,0));for(unsigned mode:{1u,2u,3u})cases.push_back(exercise(output,config,0x5a,mode));
    auto removed_output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});cases.push_back(exercise(removed_output,config,0x5a,6));
    Json registration_failures=Json::array();for(unsigned n=0;n<=4;++n)registration_failures.push_back(registration_failure(output,config,n));
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"registration_failures",registration_failures},{"cases",cases},{"source_configuration_admitted",false},{"source_output_start_admitted",false},{"source_dsp_pcm_producer_admitted",false},{"initializer45_complete",false}};std::ofstream out(argv[2+shift]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save codec event proof");std::cout<<"Native codec descriptor Success/Flushed/interruption/shutdown cases passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
