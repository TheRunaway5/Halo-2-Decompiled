#include "halo2/source_runtime_sound_pcm_prepared_range.hpp"
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

Json prepared(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config,bool pattern){
    check(reference.at("cases").size()==2,"Prepared range requires both original default PCM binding cases");
    Json evidence;std::shared_ptr<SourceRuntimeSoundEpOutputResources::PreparedRangeLease> survivor;
    prefix(output,config,[&](auto& memory,auto& sdk,const auto& dma,const auto& ep,const auto& pages,const auto& channels,auto& reset){
        auto buffers=std::make_shared<SourceRuntimeSoundPcmBuffers>(memory,dma,ep,pages,channels,reset.control_lease());
        SourcePhysicsWorldRuntimeAbi cpu{};cpu.fs_base=0x60200000;cpu.eflags=0x202;cpu.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        SourcePhysicsWorldRuntimeServices scoped;scoped.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){
            if(pages->dispatch(bank,call,abi)||sdk.dispatch(bank,call,abi,observer))return;throw std::runtime_error("Unowned prepared PCM constructor resource");};
        constexpr std::uint32_t stack=0x2018000;
        for(unsigned index=0;index<2;++index){
            put(memory,stack,0x2100000);put(memory,stack+4,index);put(memory,stack+8,0);cpu.gp[1]=reset.control_lease().source_apu()+0x1c;cpu.gp[4]=stack;
            channels->run_cpu(0x38eef1,cpu,*cpu.floating_point,scoped);check(cpu.gp[0]==0,"Original PCM channel constructor failed");
            const auto channel=word(memory,reset.control_lease().source_apu()+0x1c+index*4);channels->bind_channel(channel);
            put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0);put(memory,stack+12,0);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38f104,cpu,*cpu.floating_point,scoped);}
            const auto pc=index?0x38e524u:0x38e4d1u;put(memory,stack,pc+5);put(memory,stack+4,ep->diagnostics().layout.data.physical);put(memory,stack+8,0x2000);cpu.gp[1]=channel;cpu.gp[4]=stack;
            {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());check(buffers->try_bind({pc,0x38ef59,false},cpu,*cpu.floating_point,scoped),"Original default shared PCM binding was absent");}
        }
        const auto analog=buffers->buffer(Kind::Analog),spdif=buffers->buffer(Kind::Spdif);check(analog.binding.native_buffer==spdif.binding.native_buffer&&analog.binding.buffer_physical==spdif.binding.buffer_physical,"Default analog/SPDIF did not mirror one actual physical domain");
        const auto kind=pattern?Kind::Spdif:Kind::Analog,other_kind=pattern?Kind::Analog:Kind::Spdif;
        SourceRuntimeMemory foreign;const auto wrong_checkpoint=memory.checkpoint();rejected([&]{SourceRuntimeSoundPcmPreparedRange wrong(foreign,buffers,kind);});check(memory.checkpoint()==wrong_checkpoint,"Foreign prepared-range bank changed source bytes");
        rejected([&]{SourceRuntimeSoundPcmPreparedRange missing(memory,buffers,kind);});
        ep->apply_pending_cursor(0x38e7c4,0xfe80402c,0x600);ep->apply_pending_cursor(0x38e7ca,0xfe80403c,0x3800);
        {auto generic=ep->acquire_native_byte_transport();rejected([&]{SourceRuntimeSoundPcmPreparedRange wrong(memory,buffers,kind);});}
        if(pattern){auto bytes=memory.mutable_view(analog.binding.buffer_source,0x600);for(unsigned i=0;i<bytes.size();++i)bytes[i]=std::byte((i*17+9)&255);}
        const auto expected=std::vector<std::byte>(memory.view(analog.binding.buffer_source,0x600).begin(),memory.view(analog.binding.buffer_source,0x600).end());
        SourceRuntimeSoundPcmPreparedRange owner(memory,buffers,kind);auto lease=owner.borrow();const auto initial=lease->state();
        check(initial.offset==0&&initial.bytes==0x600&&initial.native==analog.binding.native_buffer&&!initial.cancelled&&!initial.source_pcm_available&&!initial.source_started,"Prepared byte resource claimed source PCM availability/start");
        const auto original_bank=memory.checkpoint();std::vector<std::byte> copy(0x600);lease->copy(0,copy);check(copy==expected,"Prepared borrower did not copy its actual retained EP bytes");
        rejected([&]{lease->copy(0x600,std::span<std::byte>(copy).first(1));});rejected([&]{lease->copy(std::size_t(-1),{});});rejected([&]{lease->copy(0,memory.mutable_view(analog.binding.buffer_source,1));});
        rejected([&]{ep->acquire_native_byte_transport();});rejected([&]{SourceRuntimeSoundPcmPreparedRange mirror(memory,buffers,other_kind);});rejected([&]{(void)pages->cpu_memory();});
        check(memory.checkpoint()==original_bank,"Prepared byte admission/copy changed source state");
        const auto rings=ep->diagnostics().rings;check(rings[0].read_cursor==0x600&&rings[0].write_cursor==0x600&&!rings[0].queued&&!rings[0].native_bytes_written&&!rings[0].native_bytes_read&&
            rings[1].read_cursor==0x3800&&rings[1].write_cursor==0x3800&&!rings[1].queued&&ep->diagnostics().callback_leases==1&&!ep->diagnostics().producer_ready,"Prepared byte copies modified or claimed generic queued/source PCM");
        unsigned foreign_guards{};std::thread foreign_thread([&]{auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign_guards;}};
            guard([&]{owner.borrow();});guard([&]{owner.diagnostics();});guard([&]{owner.retire();});guard([&]{lease->cancel();});guard([&]{SourceRuntimeSoundPcmPreparedRange wrong(memory,buffers,kind);});
            std::vector<std::byte> callback(0x600);lease->copy(0,callback);check(callback==expected&&!owner.quiescent(),"Foreign prepared callback copied source metadata or incorrect actual bytes");});foreign_thread.join();check(foreign_guards==5,"Foreign prepared ownership operations touched source state");
        std::atomic<bool> started{};std::atomic<unsigned> successful{},cancelled{};
        std::thread worker([&]{std::array<std::byte,0x600> bytes{};for(;;){try{lease->copy(0,bytes);check(std::ranges::equal(bytes,expected),"Active prepared copy changed during cancellation");++successful;started=true;}catch(const std::exception&){++cancelled;break;}}});
        const auto retired_kind=pattern?kind:other_kind,remaining_kind=pattern?other_kind:kind;
        while(!started.load())std::this_thread::yield();buffers->retire(retired_kind);worker.join();
        check(successful&&cancelled==1&&lease->state().cancelled&&!ep->diagnostics().callback_leases&&!ep->diagnostics().active_copies&&
            !source_runtime_sound_pcm_descriptor_bound((retired_kind==Kind::Analog?analog:spdif).binding.native_descriptors),"PCM retirement dropped descriptor ownership before actual borrower cancellation/fence");
        rejected([&]{lease->copy(0,copy);});owner.retire();check(owner.quiescent(),"Prepared owner failed creating-thread retirement");
        SourceRuntimeSoundPcmPreparedRange source_scope(memory,buffers,remaining_kind);survivor=source_scope.borrow();bool first_source_write{};
        {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());check(survivor->state().cancelled&&!ep->diagnostics().active_copies&&!ep->diagnostics().callback_leases,"Source scope did not revoke and fence prepared bytes before source mutation");
            SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());put(memory,stack,0x2100000);put(memory,stack+4,0);cpu.gp[1]=(remaining_kind==Kind::Analog?analog:spdif).binding.channel;cpu.gp[4]=stack;
            channels->run_cpu(0x38f05c,cpu,*cpu.floating_point,scoped,[&](const auto&){check(survivor->state().cancelled,"Actual source store raced live prepared byte borrower");first_source_write=true;});}
        check(first_source_write,"Prepared cancellation proof skipped actual original source stop/reset");
        source_scope.retire();buffers->retire(remaining_kind);buffers->retire();
        {auto generic=ep->acquire_native_byte_transport();check(!generic->state(0).queued&&generic->state(0).read_cursor==0x600,"Prepared inspection changed generic empty-at600 probe");}
        for(const auto& endpoint:channels->diagnostics().endpoints){const auto status=channels->endpoint(endpoint.kind)->status();check(status.paused&&!status.pending_packets&&!status.completed_packets,"Prepared lease submitted or started output PCM");}
        auto stale_generic=ep->acquire_native_byte_transport();ep->retire();
        rejected([&]{stale_generic->state(0);});
        auto replacement=std::make_shared<SourceRuntimeSoundEpOutputResources>(memory,dma,output);
        replacement->apply_pending_cursor(0x38e7c4,0xfe80402c,0x600);replacement->apply_pending_cursor(0x38e7ca,0xfe80403c,0x3800);
        std::thread old_generic([lease=std::move(stale_generic)]()mutable{rejected([&]{lease->state(0);});lease.reset();});old_generic.join();
        // Old Core handle destruction cannot remove the replacement's exact
        // native backing registry, while the old prepared handle is retained.
        rejected([&]{SourceRuntimeSoundEpOutputResources duplicate(memory,dma,output);});
        {auto current=replacement->acquire_native_byte_transport();check(!current->state(0).queued,"Fresh EP backing owner inherited stale queued state");}
        replacement->retire();check(replacement->quiescent()&&ep->quiescent(),"Actual prepared Core did not release DMA refs on creating executor");
        evidence={{"passed",true},{"native_test_pattern",pattern},{"range_bytes",0x600},{"actual_single_mirror_domain",true},{"foreign_owner_rejections",foreign_guards},{"actual_callback_copies",successful.load()},{"pcm_retirement_cancels_and_fences_before_registry_release",true},{"source_scope_revokes_before_actual_source_stores",true},{"generic_empty_at600_preserved",true},{"retired_core_stale_leases_do_not_block_or_erase_replacement",true},{"source_pcm_available",false},{"source_started",false},{"dsp_or_scheduler_ready",false}};
    });
    std::thread stale([&]{std::array<std::byte,1> byte{};rejected([&]{survivor->copy(0,byte);});survivor.reset();});stale.join();evidence["stale_callback_after_bank_teardown_rejected_before_access"]=true;return evidence;
}
}
int main(int argc,char** argv){try{check(argc==4,"Supply prior PCM original reference, private config directory and report");std::ifstream in(argv[1]);Json reference;in>>reference;check(reference.at("passed")==true&&reference.at("instruction_patches").empty(),"Invalid original PCM prerequisite reference");std::filesystem::create_directories(argv[2]);const auto config=std::filesystem::path(argv[2])/"menu.cfg";{std::ofstream out(config);out<<"pc_speakers 0\n";}
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});Json cases=Json::array();cases.push_back(prepared(reference,output,config,false));cases.push_back(prepared(reference,output,config,true));Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"cases",cases},{"paused_prepared_byte_resource_only",true},{"source_pcm_validity_or_output_start_claimed",false},{"initializer45_complete",false}};std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save prepared byte resource report");std::cout<<report.dump()<<'\n';return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
