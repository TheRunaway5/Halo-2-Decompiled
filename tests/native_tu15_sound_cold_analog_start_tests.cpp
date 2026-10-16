#include "halo2/source_runtime_sound_cold_analog_start.hpp"
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
#include <poll.h>
#include "halo2/source_runtime_sound_pcm_prepared_range.hpp"
#include <cerrno>
#include <new>
#include <cstdlib>
thread_local long codec_fail_allocation=-1;
void* operator new(std::size_t n){if(codec_fail_allocation==0){codec_fail_allocation=-1;throw std::bad_alloc();}if(codec_fail_allocation>0)--codec_fail_allocation;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
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
Json exercise(const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,unsigned fill,unsigned mode,long failure=-1){
    Json evidence;prefix(output,config,[&](auto& memory,auto& sdk,const auto& dma,const auto& ep,const auto& pages,const auto& channels,auto& reset){
        mount_native_tu15_sound_codec_initialization_data(memory);mount_native_tu15_sound_codec_event_data(memory);mount_native_tu15_sound_ac97_retire_data(memory);mount_native_tu15_sound_ac97_page_release_data(memory);
        SourceProcessShutdownRegistry registry(memory,0x60210000);auto pcm=std::make_shared<SourceRuntimeSoundPcmBuffers>(memory,dma,ep,pages,channels,reset.control_lease());
        auto codec=std::make_shared<SourceRuntimeSoundCodecRoute>(memory,output,reset.control_lease(),SourceRuntimeSoundCodecRoute::Profile::XboxWm9709Reference);
        SourceRuntimeSoundCodecEventPort port(memory,output,reset.control_lease(),codec,pcm,registry);
        constexpr auto stack=0x2018000u,fs=0x60200000u;const auto controller=reset.control_lease().source_apu()+0x1c;
        SourcePhysicsWorldRuntimeAbi cpu{};cpu.fs_base=fs;cpu.eflags=0x202;cpu.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};cpu.gp[1]=controller;cpu.gp[4]=stack;
        put(memory,stack,0x2100000);put(memory,stack+4,1);SourcePhysicsWorldRuntimeServices construction;
        construction.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){if(port.try_call(bank,call,abi,observer)||pages->dispatch(bank,call,abi)||sdk.dispatch(bank,call,abi,observer))return;throw std::runtime_error("Unowned cold-start prerequisite constructor");};
        {SourceRuntimeSoundCodecEventPort::CallScope scope(&port);codec->run_cpu(0x38f45f,cpu,*cpu.floating_point,construction);}
        for(unsigned index=0;index<2;++index){
            put(memory,stack,0x2100000);put(memory,stack+4,index);put(memory,stack+8,0);cpu.gp[1]=controller;cpu.gp[4]=stack;channels->run_cpu(0x38eef1,cpu,*cpu.floating_point,construction);
            const auto channel=word(memory,controller+index*4);channels->bind_channel(channel);put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0);put(memory,stack+12,0);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38f104,cpu,*cpu.floating_point,construction);}
            const auto site=index?0x38e524u:0x38e4d1u;put(memory,stack,site+5);put(memory,stack+4,ep->diagnostics().layout.data.physical);put(memory,stack+8,0x2000);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());check(pcm->try_bind({site,0x38ef59,false},cpu,*cpu.floating_point,construction),"Default analog descriptor prerequisite absent");}
        }
        const auto binding=pcm->buffer(Kind::Analog).binding;auto raw=memory.mutable_view(binding.buffer_source,0x2000);
        for(unsigned i=0;i<4096;++i){const std::uint16_t v=std::int16_t((i%31)-15)*8;raw[i*2]=std::byte(v);raw[i*2+1]=std::byte(v>>8);}
        auto prior=channels->endpoint(Kind::Analog);
        if(mode==2){const auto samples=std::make_shared<const std::vector<float>>(8,0.0f);check(prior->submit_native({samples,0,4,0x42})==SdlSoundPlatform::Submission::Accepted,"Old receipt lifetime fixture was not queued");}
        auto start_owner=std::make_unique<SourceRuntimeSoundColdAnalogStart>(memory,channels,pcm,codec,reset.control_lease(),port);auto& start=*start_owner;
        rejected([&]{SourceRuntimeSoundColdAnalogStart duplicate(memory,channels,pcm,codec,reset.control_lease(),port);});
        std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(fill));for(unsigned i=0;i<cpu.gp.size();++i)cpu.gp[i]=0x21000000+i*0x111111;
        cpu.gp[1]=binding.channel;cpu.gp[6]=reset.control_lease().source_apu()+8;cpu.gp[4]=stack;cpu.eflags=0x202;put(memory,stack,0x38ecf2);put(memory,stack+4,0xffffffff);
        {const auto unchanged=memory.checkpoint();put(memory,stack+4,0);rejected([&]{start.start(cpu,*cpu.floating_point,construction);});put(memory,stack+4,0xffffffff);check(memory.checkpoint()==unchanged&&!start.state().attempted,"Seek rejection changed source or native start state");}
        {const auto saved=cpu;const auto checkpoint=memory.checkpoint();cpu.fs_base=0xffffffff;rejected([&]{start.start(cpu,*cpu.floating_point,construction);});cpu=saved;cpu.gp[4]=0xffffffff;rejected([&]{start.start(cpu,*cpu.floating_point,construction);});cpu=saved;cpu.gp[1]+=4;rejected([&]{start.start(cpu,*cpu.floating_point,construction);});cpu=saved;check(memory.checkpoint()==checkpoint&&!start.state().attempted,"Cold source frame/FS rejection mutated source or start ownership");}
        const auto guarded_checkpoint=memory.checkpoint();unsigned foreign_guards{};std::thread foreign([&]{SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffff;
            for(auto action:{std::function<void()>([&]{start.start(invalid,{0x27f,0x1f80},{});}),std::function<void()>([&]{start.stop(invalid,{0x27f,0x1f80},{});}),std::function<void()>([&]{start.state();}),std::function<void()>([&]{start.previous_receipt_owner();})})try{action();}catch(const std::exception& e){if(std::string(e.what()).find("owner thread")!=std::string::npos)++foreign_guards;}
        });foreign.join();check(foreign_guards==4&&memory.checkpoint()==guarded_checkpoint,"Cold start foreign guard read source or mutated state");
        unsigned pre_start_guards{},forbidden_count{};auto forbidden=construction;forbidden.instruction=[&](auto){++forbidden_count;};
        for(const auto& [at,n,value]:std::vector<std::tuple<unsigned,unsigned,unsigned>>{{binding.channel,4,1},{binding.channel+8,4,controller+4},{0x391208,4,0x38f443},{0x39120c,4,controller+4},{controller+0xc,4,0},{binding.buffer_source,1,0x7f},{binding.descriptor_source,4,binding.buffer_physical+4},{binding.descriptor_source+4,4,0xc0000fff},{binding.channel+4,4,1},{binding.channel+0xc,4,1},{binding.channel+0x10,4,0x38f443},{binding.channel+0x14,4,1},{controller+8,4,0},{binding.channel+0x24,1,2},{binding.channel+0x25,1,1}}){
            const auto saved=std::vector<std::byte>(memory.view(at,n).begin(),memory.view(at,n).end());std::array<std::byte,4> value_bytes{};for(unsigned i=0;i<n;++i)value_bytes[i]=std::byte(value>>(i*8));memory.write(at,std::span(value_bytes).first(n));const auto checkpoint=memory.checkpoint();rejected([&]{start.start(cpu,*cpu.floating_point,forbidden);});
            check(memory.checkpoint()==checkpoint&&!forbidden_count&&!start.state().attempted,"Cold start drift was admitted before whole source execution");memory.write(at,saved);++pre_start_guards;
        }
        // The retained-byte diagnostic requires both genuine default cursor
        // observations before it can acquire its actual native range.
        ep->apply_pending_cursor(0x38e7c4,0xfe80402c,0x600);ep->apply_pending_cursor(0x38e7ca,0xfe80403c,0x3800);
        {SourceRuntimeSoundPcmPreparedRange prepared(memory,pcm,Kind::Analog);const auto checkpoint=memory.checkpoint();rejected([&]{start.start(cpu,*cpu.floating_point,forbidden);});check(memory.checkpoint()==checkpoint&&!start.state().attempted&&!forbidden_count,"Cold start replaced a live prepared-byte borrower");prepared.retire();++pre_start_guards;}
        {const auto saved_cpu=cpu;const auto saved_count=std::vector<std::byte>(memory.view(binding.channel+0x24,2).begin(),memory.view(binding.channel+0x24,2).end());
            put(memory,stack,0x2100000);put(memory,stack+4,1);put(memory,stack+8,2);put(memory,stack+12,1);put(memory,stack+16,1);cpu.gp[4]=stack;cpu.gp[1]=binding.channel;channels->run_cpu(0x38edf4,cpu,*cpu.floating_point,construction);memory.write(binding.channel+0x24,saved_count);
            cpu=saved_cpu;put(memory,stack,0x38ecf2);put(memory,stack+4,0xffffffff);const auto checkpoint=memory.checkpoint();rejected([&]{start.start(cpu,*cpu.floating_point,forbidden);});check(memory.checkpoint()==checkpoint&&!start.state().attempted&&!forbidden_count,"Cold start accepted a genuine mismatched native current index");++pre_start_guards;
            put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,1);put(memory,stack+12,1);put(memory,stack+16,1);cpu.gp[4]=stack;cpu.gp[1]=binding.channel;channels->run_cpu(0x38edf4,cpu,*cpu.floating_point,construction);cpu=saved_cpu;put(memory,stack,0x38ecf2);put(memory,stack+4,0xffffffff);
        }
        Json start_in=snapshot(memory),start_gp=cpu.gp,instructions=Json::array(),stores=Json::array();
        SourcePhysicsWorldRuntimeServices traced=construction;traced.instruction=[&](auto pc){instructions.push_back(pc);if(pc==0x38f0f3){if(mode==4)reset.control_lease().run(reset.control_lease().generation(),[&]{codec_fail_allocation=failure;});if(mode==9)codec_fail_allocation=failure;}};
        SourcePhysicsContactRecordObserver record=[&](const auto& row){if(mode==9&&row.instruction==0x38f0f3)codec_fail_allocation=-1;stores.push_back({{"instruction",row.instruction},{"address",row.address},{"before",hex(row.before)},{"written",hex(row.written)}});if((mode==3&&row.instruction==0x38f0f3)||(mode==6&&row.instruction==0x38f215))throw Interrupted{};};
        const bool locked=false;
        bool failed=false;try{start.start(cpu,*cpu.floating_point,traced,record);}catch(const Interrupted&){failed=true;}catch(const std::bad_alloc&){failed=true;}catch(const std::runtime_error&){if(mode!=2)throw;failed=true;}
        if(mode==2){check(failed&&start.state().faulted&&!start.state().source_started&&reset.state().faulted,"Unaccounted old receipt did not reject actual source run after reset");const auto receipt=prior->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Flushed,"Successor reset discarded old Flushed receipt");prior->acknowledge(receipt->submission);evidence={{"mode","old_receipt_rejection"},{"old_flushed_receipt_preserved",true},{"source_run_issued",false}};return;}
        if(mode==3||mode==4||mode==9){
            if(failed){check(start.state().faulted&&reset.state().faulted&&!port.state().pending&&word(memory,binding.channel+4)==0,"Actual start failure published source completion or left shared worker live");
                auto receipts=port.receipt_owner();const auto receipt=receipts?receipts->completion():std::optional<SdlSoundCompletion>{};if(receipt){check(receipt->status==SdlSoundCompletionStatus::Flushed,"Failed actual run did not preserve genuine cancellation receipt");receipts->acknowledge(receipt->submission);}
                if(locked)SDL_UnlockAudioDevice(output->device_id());evidence={{"mode",mode==3?"source_interruption":mode==9?"source_action_allocation_failure":"worker_allocation_failure"},{"allocation_position",failure},{"failed",true},{"actual_packet_cancelled",bool(receipt)},{"receipt_preserved",true},{"source_counter",0}};return;
            }
            codec_fail_allocation=-1;reset.control_lease().run(reset.control_lease().generation(),[]{codec_fail_allocation=-1;});
        }
        check(start.state().source_started&&start.state().native_running&&start.state().successor_generation==start.state().previous_generation+1&&word(memory,binding.channel+4)==0,"Whole source cold start did not retain actual run/successor ownership");
        {const auto checkpoint=memory.checkpoint();rejected([&]{start.start(cpu,*cpu.floating_point,construction);});check(memory.checkpoint()==checkpoint&&start.state().native_running&&!port.state().faulted,"Unsupported warm start changed actual run or source state");}
        const auto receipts=port.receipt_owner();rejected([&]{prior->submit_native({std::make_shared<const std::vector<float>>(8,0.0f),0,4,0x55});});
        if(mode>=10&&mode<=12){
            if(mode==11){const auto deadline=SDL_GetTicks64()+5000;while(!port.collect_success()){check(SDL_GetTicks64()<deadline,"Shutdown Success fixture was not consumed");SDL_Delay(1);}pollfd event{port.state().event_descriptor,POLLIN,0};check(port.state().pending&&poll(&event,1,0)==1,"Shutdown Success fixture did not publish an actual readable eventfd");}
            else check(receipts->status().pending_packets==1,"Shutdown cancellation fixture was not actually pending");
            SourceRuntimeSoundAc97PageRelease release(memory,pages,channels,reset.control_lease());SourcePhysicsWorldRuntimeServices retirement;
            retirement.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){if(call.target==0x38f395){release.run_retirement(abi,*abi.floating_point,retirement,observer);return;}if(sdk.dispatch(bank,call,abi,observer))return;throw std::runtime_error("Unowned cold-start registered shutdown resource");};
            port.request_shutdown();put(memory,stack,0x2100000);put(memory,stack+4,controller+0xc);cpu.gp[4]=stack;
            if(mode==12){bool interrupted=false;SourcePhysicsContactRecordObserver interrupt=[&](const auto& row){if(row.instruction==0x38f426)throw Interrupted{};};try{port.dispatch_shutdown(cpu,*cpu.floating_point,retirement,interrupt);}catch(const Interrupted&){interrupted=true;}check(interrupted&&start.state().faulted&&reset.state().faulted&&!start.state().source_stopped&&!start.state().source_cancelled,"Interrupted shutdown was accepted as legitimate source closure");const auto receipt=receipts->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Flushed,"Interrupted registered shutdown lost its actual Flushed receipt");receipts->acknowledge(receipt->submission);start_owner.reset();evidence={{"mode","registered_shutdown_interruption"},{"source_stopped",false},{"source_cancelled",false},{"shared_control_faulted",true},{"actual_flushed_receipt_preserved",true}};return;}
            check(port.dispatch_shutdown(cpu,*cpu.floating_point,retirement),"Cold-start registered global shutdown did not dispatch");
            check(registry.records().empty()&&!port.state().connected&&!port.state().shutdown_registered&&!port.state().pending&&!port.state().delivered&&!word(memory,0x391208)&&!word(memory,controller+0xc)&&!word(memory,controller)&&!word(memory,controller+4)&&release.diagnostics().released.size()==2,"Cold-start registered shutdown did not finish genuine source/page/route closure");
            const auto receipt=receipts->completion();check(receipt&&receipt->status==(mode==11?SdlSoundCompletionStatus::Success:SdlSoundCompletionStatus::Flushed)&&!start.state().source_stopped&&start.state().source_cancelled&&!start.state().native_running&&!start.state().faulted&&!reset.state().faulted,"Registered cancellation lost actual receipt or manufactured source-stop return");
            if(mode==11){pollfd event{port.state().event_descriptor,POLLIN,0};check(poll(&event,1,0)==0,"Registered source cancellation retained a readable Success eventfd");}
            start_owner.reset();check(!reset.state().faulted,"Witnessed registered shutdown owner destruction faulted healthy shared control");receipts->acknowledge(receipt->submission);receipts->retire();port.retire();evidence={{"mode",mode==11?"registered_shutdown_retained_success":"registered_shutdown_pending"},{"genuine_registered_shutdown_completed",true},{"physical_pages_released",2},{"source_stopped",false},{"source_cancelled",true},{"receipt_preserved_for_caller",true},{"isr_deliveries",0},{"shared_control_live_after_owner_destruction",true}};return;
        }
        Json start_evidence;if(mode==0)start_evidence={{"input_regions",start_in},{"input_gp",start_gp},{"input_eflags",0x202},{"fs_base",fs},{"instructions",instructions},{"stores",stores},{"output_gp",cpu.gp},{"output_eflags",cpu.eflags},{"output_regions",snapshot(memory)}};
        if(mode==8){std::set_terminate([]{std::_Exit(77);});std::thread other([&]{start.~SourceRuntimeSoundColdAnalogStart();});other.join();throw std::runtime_error("Foreign source-start destructor returned");}
        if(mode==7){check(output->device_removed(output->device_id()),"Source-start failure notification did not identify actual device");rejected([&]{port.collect_success();});const auto receipt=receipts->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Failure&&!start.state().native_running&&start.state().faulted&&word(memory,binding.channel+4)==0,"Source-start native device failure lost receipt or manufactured progress");receipts->acknowledge(receipt->submission);evidence={{"mode","device_notification_failure"},{"actual_failure_receipt_preserved",true},{"physical_os_unplug_tested",false},{"source_counter",0}};return;}
        if(mode!=1&&mode!=4&&mode!=9){if(locked)SDL_UnlockAudioDevice(output->device_id());const auto deadline=SDL_GetTicks64()+5000;while(!port.collect_success()){check(SDL_GetTicks64()<deadline,"Actual cold-start descriptor was not consumed");SDL_Delay(1);}
            check(!receipts->status().paused&&start.state().native_running,"Cold source start reused diagnostic auto-pause");
            if(mode==5){pollfd event{port.state().event_descriptor,POLLIN,0};check(port.state().pending&&poll(&event,1,0)==1&&(event.revents&POLLIN),"Success-stop fixture did not retain an actual readable eventfd");}
            if(mode!=5){
            for(unsigned i=0;i<cpu.gp.size();++i)cpu.gp[i]=0x21000000+i*0x111111;cpu.gp[4]=stack;cpu.eflags=0x202;put(memory,stack,0x2100000);put(memory,stack+4,0x391208);put(memory,stack+8,controller);
            auto input=snapshot(memory);auto input_gp=cpu.gp;instructions=Json::array();stores=Json::array();
            if(mode==6){bool caught=false;try{port.dispatch(cpu,*cpu.floating_point,traced,record);}catch(const Interrupted&){caught=true;}check(caught&&start.state().faulted&&!start.state().native_running&&word(memory,binding.channel+4)==0x1000,"Interrupted source-start ISR did not preserve partial store and fault actual run");const auto receipt=receipts->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Success,"Interrupted source-start ISR lost its real Success receipt");receipts->acknowledge(receipt->submission);evidence={{"mode","isr_interruption"},{"actual_success_receipt_preserved",true},{"partial_source_counter",0x1000},{"source_stop_returned",false}};return;}
            check(port.dispatch(cpu,*cpu.floating_point,traced,record),"Registered original codec ISR did not deliver actual source-start receipt");
            check(word(memory,binding.channel+4)==0x1000&&!receipts->status().paused&&start.state().native_running,"Source-start receipt did not retain running owner through exact source accounting");
            const auto unchanged=memory.checkpoint();rejected([&]{port.collect_success();});check(memory.checkpoint()==unchanged&&!port.state().faulted,"Unsupported new cycle changed source or faulted retained running owner");
            evidence={{"mode","success"},{"fill",fill},{"input_regions",input},{"input_gp",input_gp},{"input_eflags",0x202},{"fs_base",fs},{"instructions",instructions},{"stores",stores},{"output_gp",cpu.gp},{"output_eflags",cpu.eflags},{"output_regions",snapshot(memory)},{"source_channel_words",0x1000},{"native_packet_frames",2048},{"source_start",start_evidence},{"native_run_retained_after_success",true},{"foreign_guards",foreign_guards},{"pre_start_guards",pre_start_guards}};
            }
        }
        cpu.gp[1]=binding.channel;cpu.gp[4]=stack;cpu.eflags=0x202;put(memory,stack,0x38f1c3);put(memory,stack+4,0);auto stop_input=snapshot(memory);auto stop_gp=cpu.gp;instructions=Json::array();stores=Json::array();start.stop(cpu,*cpu.floating_point,traced,record);
        check(start.state().source_stopped&&!start.state().native_running&&receipts->status().paused&&!word(memory,binding.channel+0x24),"Genuine whole source stop failed actual native/descriptor closure");
        if(mode==5){const auto receipt=receipts->completion();pollfd event{port.state().event_descriptor,POLLIN,0};check(receipt&&receipt->status==SdlSoundCompletionStatus::Success&&word(memory,binding.channel+4)==0&&!port.state().pending&&poll(&event,1,0)==0&&port.state().delivered==0&&!reset.state().faulted&&port.state().connected&&port.state().shutdown_registered,"Genuine stop discarded Success, retained event readability, delivered source progress or faulted shared control");check(std::ranges::none_of(instructions,[](const auto& pc){return pc==0x38f378;}),"Success-stop delivered an ISR instruction");receipts->acknowledge(receipt->submission);evidence={{"mode","stop_retained_success"},{"actual_success_receipt_preserved",true},{"source_counter",0},{"pending_event_cancelled",true},{"actual_eventfd_readable_before_stop",true},{"actual_eventfd_drained_after_stop",true},{"isr_deliveries",0},{"shared_control_live",true}};}
        else if(mode==1||mode==4||mode==9){const auto receipt=receipts->completion();check(receipt&&receipt->status==SdlSoundCompletionStatus::Flushed&&word(memory,binding.channel+4)==0,"Pending source stop lost its actual Flushed receipt or fabricated source progress");receipts->acknowledge(receipt->submission);if(locked)SDL_UnlockAudioDevice(output->device_id());evidence={{"mode",mode==1?"pending_stop":mode==9?"source_action_allocation_success":"worker_allocation_success"},{"allocation_position",failure},{"failed",false},{"genuine_source_stop",true},{"actual_flushed_receipt_preserved",true}};}
        else evidence["source_stop"]={{"input_regions",stop_input},{"input_gp",stop_gp},{"input_eflags",0x202},{"fs_base",fs},{"instructions",instructions},{"stores",stores},{"output_gp",cpu.gp},{"output_eflags",cpu.eflags},{"output_regions",snapshot(memory)}};
        port.retire();
    });return evidence;
}
}
int main(int argc,char** argv){try{const bool probe=argc==4&&std::string(argv[1])=="--foreign-destructor-probe";const unsigned shift=probe?1:0;check(argc==int(3+shift),"Supply private configuration directory and cold-start report");const auto config=std::filesystem::path(argv[1+shift])/"menu.cfg";std::filesystem::create_directories(config.parent_path());{std::ofstream out(config);out<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});if(probe){exercise(output,config,0x5a,8);throw std::runtime_error("Foreign destructor did not exit77");}Json cases=Json::array();for(unsigned fill:{0u,0x5au,0xa5u})cases.push_back(exercise(output,config,fill,0));for(unsigned mode:{1u,2u,3u,5u,6u})cases.push_back(exercise(output,config,0x5a,mode));
    auto removed=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});cases.push_back(exercise(removed,config,0x5a,7));for(unsigned mode:{10u,11u,12u})cases.push_back(exercise(output,config,0x5a,mode));
    Json allocations=Json::array();for(long n=0;n<12;++n)allocations.push_back(exercise(output,config,0x5a,4,n));
    Json metadata=Json::array();bool metadata_completed=false;for(long n=0;n<128&&!metadata_completed;++n){auto result=exercise(output,config,0x5a,9,n);metadata_completed=!result.at("failed").get<bool>();metadata.push_back(std::move(result));}check(metadata_completed,"Source action metadata allocation sweep never completed");
    Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"cases",cases},{"worker_allocations",allocations},{"source_action_allocations",metadata},{"source_producer_admitted",false},{"spdif_mirror_admitted",false},{"initializer45_complete",false}};std::ofstream out(argv[2+shift]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save native cold-start evidence");std::cout<<"Native whole cold analog start/IRQ6/stop cases passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
