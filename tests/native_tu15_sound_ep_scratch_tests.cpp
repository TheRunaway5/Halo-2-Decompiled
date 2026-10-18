#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include "halo2/source_sound_ac97_output_tu15.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include <SDL.h>
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"Invalid native EP scratch operation was accepted");}
void put(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.write(at,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t at){auto b=m.view(at,4);std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<unsigned>(b[i])<<(8*i);return value;}
constexpr std::uint32_t stack=0x2008000,fs=0x200a000;
struct Fixture {
    SourceRuntimeMemory memory;
    std::shared_ptr<SdlSoundPlatform> output;
    std::unique_ptr<SourceRuntimeSoundDmaResources> pages;
    std::unique_ptr<SourceRuntimeSoundSdkResources> sdk;
    std::uint32_t gp{},ep{};
    std::set<std::uint32_t> sites;
    struct Memory final:SourcePhysicsContactMemory,SourceSoundDspImageMmioTu15 {
        Fixture& f;explicit Memory(Fixture& value):f(value){}
        void read(std::uint32_t at,std::span<std::byte> out)const override {
            if(at>=0xfe800000)throw std::runtime_error("EP scratch attempted an unowned MMIO preimage/read");
            f.memory.read(at,out);
        }
        void write(std::uint32_t at,std::span<const std::byte> in)override {
            if(at>=0xfe800000)throw std::runtime_error("EP scratch bypassed its typed table publication");
            f.memory.write(at,in);
        }
        void write_sound_dsp_mmio(std::uint32_t pc,std::uint32_t at,std::uint32_t value)override {
            if(at==0xfe802048u||at==0xfe8020dcu) {
                check(f.ep&&word(f.memory,f.ep),"EP publication precedes its original scratch constructor");
                f.pages->publish_ep_scratch(word(f.memory,f.ep),pc,at,value);return;
            }
            check(f.gp!=0,"GP table publication precedes its constructor");
            f.pages->publish_gp_scratch(word(f.memory,f.gp+8),pc,at,value);
        }
    } view{*this};
    explicit Fixture(const std::filesystem::path& xbe)
        :memory(load_source_runtime_image(xbe,SourcePhysicsContactImage::TitleUpdate15)) {
        memory.mount(stack-0x1000,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"declared EP initializer caller stack");
        memory.mount(fs,std::vector<std::byte>(0x100),SourceRuntimeMemory::Access::ReadWrite,"actual passive native sound executor");
        output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
        const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","EP scratch requires an actual native audio backend");
        pages=std::make_unique<SourceRuntimeSoundDmaResources>(memory,output);
        sdk=std::make_unique<SourceRuntimeSoundSdkResources>(memory,fs);
        call(0x387bb5,0,{});call(0x387baa,0,{});
        gp=call(0x387a32,0,{0x626f5344,0x3c,1}).gp[0];
        call(0x388275,gp,{});
        allocate(0x391420,0xc000);allocate(0x391430,0x1060);
        call(0x388294,gp,{});
        allocate(0x391440,0x13000);allocate(0x391450,0x98);allocate(0x391410,0x1000);
        ep=call(0x387a32,0,{0x626f5344,12,1}).gp[0];call(0x38f5e2,ep,{});
    }
    void allocate(std::uint32_t descriptor,std::uint32_t bytes){
        put(memory,descriptor+8,bytes);check(call(0x38e71c,0,{descriptor,0x4000,4}).gp[0]==0,"Native EP descriptor allocation failed");
    }
    SourcePhysicsWorldRuntimeAbi call(std::uint32_t entry,std::uint32_t self,std::initializer_list<std::uint32_t> arguments){
        SourcePhysicsWorldRuntimeAbi state{};state.gp[1]=self;state.gp[4]=stack;state.fs_base=fs;state.eflags=2;
        state.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        put(memory,stack,0x2100000);unsigned index=1;for(auto value:arguments)put(memory,stack+4*index++,value);
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){sites.insert(pc);};
        services.required_owner=[&](auto& bank,const auto& call,auto& abi,const auto& observer){
            check(&bank==&view,"EP initializer changed its retained native memory view");
            if(pages->dispatch(memory,call,abi)||sdk->dispatch(memory,call,abi,observer))return;
            if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(view,call.target,abi,*abi.floating_point,services,observer);return;}
            if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(view,call.target,abi,*abi.floating_point,services,observer);return;}
            if(source_sound_dsp_image_tu15_owns_entry(call.target)){source_run_sound_dsp_image_tu15(view,call.target,abi,*abi.floating_point,services,observer);return;}
            throw std::runtime_error("Original EP initializer dependency has no native owner at "+std::to_string(call.instruction)+" target "+std::to_string(call.target));
        };
        SourceRuntimeSoundDmaResources::CallScope dma_scope(pages.get());SourceRuntimeSoundSdkResources::CallScope sdk_scope(sdk.get());
        if(source_sound_sdk_tu15_owns_entry(entry))source_run_sound_sdk_tu15(view,entry,state,*state.floating_point,services);
        else if(source_sound_dma_storage_tu15_owns_entry(entry))source_run_sound_dma_storage_tu15(view,entry,state,*state.floating_point,services);
        else if(source_sound_ac97_output_tu15_owns_entry(entry))source_run_sound_ac97_output_tu15(view,entry,state,*state.floating_point,services);
        else source_run_sound_dsp_image_tu15(view,entry,state,*state.floating_point,services);
        check(state.gp[4]==stack+4+arguments.size()*4,"EP source caller lost original stack cleanup");
        return state;
    }
};
}
int main(int argc,char** argv)try {
    check(argc==2,"Supply owned original TU1.5 executable");if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Fixture f(argv[1]);std::array<std::byte,4> tiny{};
    check(!f.pages->ep_scratch_binding(),"Fresh native EP owner invented a table binding");
    rejected([&]{f.pages->read_ep_scratch(0,tiny);});
    f.call(0x38f6f4,f.ep,{f.gp});
    const auto child=word(f.memory,f.ep),ep_source=word(f.memory,0x391440),ep_physical=word(f.memory,0x391444);
    check(child&&word(f.memory,f.ep+4)==f.gp&&word(f.memory,child+8)==0&&
        word(f.memory,child+0xc)==0x391450&&word(f.memory,child+0x10)==0x391440,
        "Original EP initializer did not construct its own scratch context");
    check(word(f.memory,child)==19&&word(f.memory,child+4)==0,"Original EP scratch table count/reservation differs");
    const auto table=word(f.memory,0x391450);
    for(unsigned i=0;i<19;++i)check(word(f.memory,table+i*8)==ep_physical+i*4096&&word(f.memory,table+i*8+4)==0,"Original EP table lost actual descriptor pages");
    check(word(f.memory,ep_source+0x438)==0x4000&&word(f.memory,ep_source+0x43c)==0x600,
        "Original EP initializer did not publish its three-page metadata");
    check(f.pages->gp_scratch_binding()->entries==11&&word(f.memory,word(f.memory,f.gp+8)+4)==513,
        "Original EP initializer did not append its actual three GP pages");
    const auto binding=f.pages->ep_scratch_binding();
    check(binding&&binding->context==child&&binding->entries==19&&binding->table_source==table,
        "EP native view did not retain the original table publication");
    std::vector<std::byte> actual(0x13000);f.pages->read_ep_scratch(0,actual);
    check(std::ranges::equal(actual,f.memory.view(ep_source,actual.size())),"EP SGE view differs from actual retained native bytes");
    std::array<std::byte,8> pattern{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{8}},back{};
    f.pages->write_ep_scratch(0x10ffc,pattern);f.pages->read_gp_scratch(0x8ffc,back);
    check(back==pattern,"GP and EP views do not share their actual appended native pages");
    const auto original=word(f.memory,table+8);put(f.memory,table+8,0xdead000);
    rejected([&]{f.pages->write_ep_scratch(4092,pattern);});
    check(std::ranges::equal(f.memory.view(ep_source+4092,4),std::span(actual).subspan(4092,4)),
        "Rejected EP cross-page write changed its valid first page");
    put(f.memory,table+8,original);put(f.memory,table+4,1);
    rejected([&]{f.pages->read_ep_scratch(0,tiny);});put(f.memory,table+4,0);
    rejected([&]{f.pages->publish_ep_scratch(child,0x38ddf4,0xfe802048,binding->table_physical+4096);});
    rejected([&]{f.pages->publish_ep_scratch(child,0x38ddfb,0xfe8020dc,18);});
    check(f.pages->ep_scratch_binding()->entries==19,"Rejected EP publication changed the native count");
    rejected([&]{f.pages->read_ep_scratch(0x13000-2,tiny);});
    bool foreign=false;std::thread thread([&]{try{f.pages->read_ep_scratch(0,tiny);}catch(const std::exception&){foreign=true;}});thread.join();
    check(foreign,"Foreign executor accessed creating-thread-only EP scratch");
    check(f.sites.contains(0x38f544)&&f.sites.contains(0x38f5f3)&&!f.sites.contains(0x38e2e0)&&!f.sites.contains(0x38846e),
        "EP initializer skipped a real software producer or claimed whole hardware startup/upload");
    std::cout<<"PASS original EP software initializer: "<<f.sites.size()<<" original instruction addresses; actual SDK-owned EP context,19 EP SGEs and three EP-tail GP pages\n";
    std::cout<<"LIMIT caller-provided descriptor requests; upstream PIO/device factory, EP execution/output clock and audible parity remain open\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
