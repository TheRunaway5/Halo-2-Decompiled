#include "halo2/source_runtime_sound_pcm_buffers.hpp"
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
Json run(const Json& reference,const std::shared_ptr<SdlSoundPlatform>& output,const std::filesystem::path& config){
    Json evidence;prefix(output,config,[&](auto& memory,auto& sdk,const auto& dma,const auto& ep,const auto& pages,const auto& channels,auto& reset){
        SourceRuntimeSoundPcmBuffers buffers(memory,dma,ep,pages,channels,reset.control_lease());const auto checkpoint=memory.checkpoint();unsigned bank_rejections{};
        prefix(output,config,[&](auto&,auto&,const auto& foreign_dma,const auto& foreign_ep,const auto& foreign_pages,const auto& foreign_channels,auto& foreign_reset){
            auto wrong=[&](const auto& d,const auto& e,const auto& p,const auto& c,auto control){rejected([&]{SourceRuntimeSoundPcmBuffers invalid(memory,d,e,p,c,control);});++bank_rejections;check(memory.checkpoint()==checkpoint,"Foreign PCM bank owner changed source storage");};
            wrong(foreign_dma,foreign_ep,foreign_pages,foreign_channels,reset.control_lease());wrong(dma,foreign_ep,pages,channels,reset.control_lease());
            wrong(dma,ep,foreign_pages,channels,reset.control_lease());wrong(dma,ep,pages,foreign_channels,reset.control_lease());wrong(dma,ep,pages,channels,foreign_reset.control_lease());
        });
        unsigned control_rejections{};
        {SourceRuntimeSoundResetResources other_reset(memory,output,0x60200000);SourcePhysicsWorldRuntimeAbi repeated{};
            repeated.fs_base=0x60200000;repeated.eflags=0x202;repeated.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            repeated.gp[1]=reset.control_lease().source_apu()+8;repeated.gp[6]=repeated.gp[1];repeated.gp[4]=0x201b000;
            put(memory,repeated.gp[4],0x38ec94);put(memory,repeated.gp[4]+12,0x389389);
            check(other_reset.try_call(memory,{0x38ec8f,0x38ec0e,false},repeated,*repeated.floating_point,{}),"Independent same-bank native control worker did not execute genuine reset");
            const auto other=other_reset.control_lease();check(other.generation()==reset.control_lease().generation()&&!other.same_owner(reset.control_lease()),"PCM control fixture did not establish equal-generation distinct actual workers");
            const auto before=memory.checkpoint();rejected([&]{SourceRuntimeSoundPcmBuffers invalid(memory,dma,ep,pages,channels,other);});
            check(memory.checkpoint()==before,"Rejected same-bank equal-generation distinct PCM worker changed source bytes");++control_rejections;
        }
        const auto& prior=reference.at("channel_reference");std::ranges::fill(memory.mutable_view(0x2010000,0x10000),std::byte(prior.at("fill").get<unsigned>()));
        SourcePhysicsWorldRuntimeAbi cpu{};cpu.gp=prior.at("initial_gp").get<decltype(cpu.gp)>();cpu.fs_base=0x60200000;cpu.eflags=0x202;cpu.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        Json instructions=Json::array(),stores=Json::array();SourcePhysicsWorldRuntimeServices scoped;
        scoped.instruction=[&](auto pc){instructions.push_back(pc);};SourcePhysicsContactRecordObserver observe=[&](const auto& row){std::uint64_t v{};for(unsigned i=0;i<row.written.size();++i)v|=std::uint64_t(std::to_integer<unsigned>(row.written[i]))<<(8*i);stores.push_back({row.instruction,row.address,row.written.size(),v});};
        scoped.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){
            if(pages->dispatch(bank,call,abi)){if(call.instruction==0x3879dc){const auto rows=pages->diagnostics().allocations;const auto at=std::ranges::find_if(rows,[&](const auto& row){return row.source==abi.gp[0];});check(at!=rows.end(),"Native PCM page allocation disappeared");std::ranges::fill(memory.mutable_view(at->source,at->capacity),std::byte{0xa5});}return;}
            if(sdk.dispatch(bank,call,abi,observer))return;throw std::runtime_error("Unowned real PCM constructor platform resource");};
        constexpr std::uint32_t stack=0x2018000;put(memory,stack,0x2100000);put(memory,stack+4,prior.at("index"));put(memory,stack+8,0);
        channels->run_cpu(0x38eef1,cpu,*cpu.floating_point,scoped,observe);
        check(instructions==prior.at("constructor").at("instructions")&&stores==prior.at("constructor").at("source_stores")&&cpu.gp==prior.at("constructor").at("returned_gp").get<decltype(cpu.gp)>(),"Original prior PCM channel constructor differs");
        channels->bind_channel(prior.at("channel"));instructions=Json::array();stores=Json::array();put(memory,stack,0x2100000);put(memory,stack+4,0);put(memory,stack+8,0);put(memory,stack+12,0);cpu.gp[1]=prior.at("channel");cpu.gp[4]=stack;
        {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38f104,cpu,*cpu.floating_point,scoped,observe);}
        check(instructions==prior.at("instructions")&&stores==prior.at("source_stores")&&cpu.gp==prior.at("returned_gp").get<decltype(cpu.gp)>(),"Original prior whole PCM channel allocation/reset differs");
        const auto kind=reference.at("index")==0?Kind::Analog:Kind::Spdif;rejected([&]{buffers.buffer(kind);});
        put(memory,stack,reference.at("caller_return"));put(memory,stack+4,reference.at("buffer_physical"));put(memory,stack+8,reference.at("buffer_bytes"));cpu.gp[1]=prior.at("channel");cpu.gp[4]=stack;
        check(cpu.gp==reference.at("initial_gp").get<decltype(cpu.gp)>()&&cpu.eflags==reference.at("initial_eflags"),"Native PCM binding caller ABI differs");
        const SourcePhysicsContactRuntimeCall call{reference.at("caller_pc"),0x38ef59,false};const auto ready=memory.checkpoint();
        auto bad=cpu;rejected([&]{buffers.try_bind({call.instruction,0x38efea,false},bad,*bad.floating_point,scoped,observe);});check(memory.checkpoint()==ready,"Rejected PCM caller changed source bytes");
        check(!buffers.try_bind({0x38e512,0x38ef59,false},bad,*bad.floating_point,scoped,observe),"PCM binding accepted encoded SPDIF caller");
        instructions=Json::array();stores=Json::array();{SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());SourceRuntimeSoundAc97ChannelReset::CallScope c(channels.get());check(buffers.try_bind(call,cpu,*cpu.floating_point,scoped,observe),"Exact original default PCM buffer binding was not owned");}
        check(instructions==reference.at("instructions")&&stores==reference.at("source_stores")&&cpu.gp==reference.at("returned_gp").get<decltype(cpu.gp)>()&&(cpu.eflags&0xcc5)==(reference.at("returned_eflags").get<unsigned>()&0xcc5),"Original PCM binding instructions/stores/GP/defined flags differ");
        equal(memory,0x2010000,reference.at("stack_hex"),"Original complete PCM binding caller stack differs");
        for(const auto key:{"retained_raw_hex","retained_dma_hex","physical_allocations"})for(const auto& row:reference.at(key))equal(memory,row.at("source"),row.at("hex"),"Original PCM binding retained source/DMA/physical bytes differ");
        const auto view=buffers.buffer(kind);const auto layout=ep->diagnostics().layout;
        check(view.bytes.size()==0x2000&&view.binding.buffer_physical==layout.data.physical&&view.binding.native_buffer==layout.data.native&&view.binding.native_buffer.value>0xffffffffu&&std::ranges::equal(view.bytes,memory.view(layout.data.source,0x2000)),"Native PCM view lost its actual EP0 backing");
        check(source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors)&&!buffers.channel_quiescent(kind),"Actual PCM descriptor binding was absent from native teardown admission");
        unsigned foreign{};std::thread other([&]{check(!buffers.quiescent()&&!buffers.channel_quiescent(kind)&&source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors),"Foreign PCM ownership query touched source state or claimed quiescence");auto guard=[&](auto action){try{action();}catch(const std::exception& error){if(std::string(error.what()).find("owner thread")!=std::string::npos)++foreign;}};SourcePhysicsWorldRuntimeAbi invalid{};invalid.gp[4]=0xffffffff;guard([&]{buffers.try_bind(call,invalid,{0x27f,0x1f80},{},{});});guard([&]{buffers.buffer(kind);});guard([&]{buffers.diagnostics();});guard([&]{buffers.retire();});guard([&]{buffers.retire(kind);});});other.join();check(foreign==5,"Foreign PCM resource API touched mutable source or native state");
        const auto before_repeat=memory.checkpoint();rejected([&]{auto again=cpu;buffers.try_bind(call,again,*again.floating_point,scoped,observe);});check(memory.checkpoint()==before_repeat,"Repeated PCM bind changed source bytes");
        const auto state=buffers.diagnostics();check(state.bindings.size()==1&&!state.source_started&&!state.producer_ready,"PCM resource supplied source start/producer readiness");
        evidence={{"passed",true},{"index",reference.at("index")},{"original_binding_instructions",instructions.size()},{"original_binding_stores",stores.size()},{"actual_buffer_bytes",view.bytes.size()},{"native_buffer",view.binding.native_buffer.value},{"foreign_bank_rejections",bank_rejections},{"foreign_thread_rejections",foreign},{"same_bank_output_equal_generation_distinct_worker_rejections",control_rejections},{"source_descriptor_index_published",true},{"source_started",false},{"producer_ready",false}};
        // A genuine same-channel reset invalidates the old endpoint epoch and
        // descriptor/count binding, without replacing the whole APU generation.
        put(memory,0x201a000,0x2100000);cpu.gp[1]=prior.at("channel");cpu.gp[4]=0x201a000;
        {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38efea,cpu,*cpu.floating_point,scoped,observe);}
        rejected([&]{buffers.buffer(kind);});evidence["genuine_channel_reset_rejected_old_buffer"]=true;
        check(source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors),"Reset implicitly discarded native PCM ownership before explicit retirement");
        //38EFEA preserves the source count. The genuine stop/reset body
        //38F05C then calls EDF4 with count0, retaining the actual table.
        put(memory,0x201a000,0x2100000);put(memory,0x201a004,0);cpu.gp[1]=prior.at("channel");cpu.gp[4]=0x201a000;
        {SourceRuntimeSoundEpOutputResources::CallScope e(ep.get());SourceRuntimeSoundAc97OutputPages::CallScope p(pages.get());channels->run_cpu(0x38f05c,cpu,*cpu.floating_point,scoped,observe);}
        check(memory.view(prior.at("channel").get<unsigned>()+0x24,2)[0]==std::byte{},"Genuine channel stop/reset did not clear source descriptor count");
        SourceRuntimeSoundPcmBuffers conflict(memory,dma,ep,pages,channels,reset.control_lease());
        cpu.gp=reference.at("initial_gp").get<decltype(cpu.gp)>();put(memory,stack,reference.at("caller_return"));put(memory,stack+4,reference.at("buffer_physical"));put(memory,stack+8,reference.at("buffer_bytes"));
        const auto conflict_checkpoint=memory.checkpoint();const auto conflict_gp=cpu.gp;const auto conflict_instructions=instructions.size();const auto conflict_stores=stores.size();
        std::string conflict_error;try{conflict.try_bind(call,cpu,*cpu.floating_point,scoped,observe);}catch(const std::exception& error){conflict_error=error.what();}
        check(conflict_error=="Native PCM descriptor page already has a live binding owner","Genuine-reset conflict did not exercise prior descriptor registry ownership");
        check(memory.checkpoint()==conflict_checkpoint&&cpu.gp==conflict_gp&&instructions.size()==conflict_instructions&&stores.size()==conflict_stores&&
            source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors)&&!buffers.diagnostics().faulted&&!conflict.diagnostics().faulted,
            "Reset then conflicting PCM binding mutated source/native assembly or replaced its prior registry owner");
        reset.control_lease().require_bank(memory);conflict.retire();
        check(source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors),"Conflicting owner retirement removed the prior descriptor registration");
        evidence["reset_then_second_owner_conflict_rejected_before_source"]=true;
        buffers.retire(kind);check(buffers.channel_quiescent(kind)&&!source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors),"Per-channel PCM retirement failed to release real descriptor ownership");evidence["physical_free_requires_explicit_binding_retirement"]=true;
        buffers.retire();check(buffers.quiescent(),"Native PCM buffer did not retire on its creator");
        SourceRuntimeSoundPcmBuffers failing(memory,dma,ep,pages,channels,reset.control_lease());
        bool actual_source_store=false;SourcePhysicsContactRecordObserver interrupt=[&](const auto&){actual_source_store=true;throw std::runtime_error("declared actual PCM source-store interruption");};
        std::string interrupted_error;try{failing.try_bind(call,cpu,*cpu.floating_point,scoped,interrupt);}catch(const std::exception& error){interrupted_error=error.what();}
        if(!actual_source_store)throw std::runtime_error("PCM interruption probe did not reach an actual source store: "+interrupted_error);
        check(actual_source_store&&failing.diagnostics().faulted&&!source_runtime_sound_pcm_descriptor_bound(view.binding.native_descriptors),
            "Interrupted PCM commit retained a descriptor reservation or missed its actual source store");
        rejected([&]{reset.control_lease().require_bank(memory);});
        evidence["interrupted_source_commit_rolls_back_reservation_and_aborts_shared_control"]=true;
        failing.retire();
    });return evidence;
}
}
int main(int argc,char** argv){try{check(argc==4,"Supply original PCM binding reference, private config directory and report");std::ifstream input(argv[1]);Json reference;input>>reference;check(reference.at("passed")==true&&reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&reference.at("instruction_patches").empty(),"Wrong original PCM binding reference");std::filesystem::create_directories(argv[2]);const auto config=std::filesystem::path(argv[2])/"menu.cfg";{std::ofstream out(config);out<<"pc_speakers 0\n";}auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::DeviceCallback,64,{}});Json cases=Json::array();for(const auto& row:reference.at("cases"))cases.push_back(run(row,output,config));Json report={{"passed",true},{"native_backend",SDL_GetCurrentAudioDriver()},{"cases",cases},{"real_retained_default_pcm_binding",true},{"source_codec_start_or_dsp_ready",false},{"opaque_scheduler_words_admitted",false},{"initializer45_complete",false}};std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot save native PCM binding evidence");std::cout<<report.dump()<<'\n';return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
