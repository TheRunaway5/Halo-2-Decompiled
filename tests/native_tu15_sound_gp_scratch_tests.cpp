#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_sound_dma_storage_tu15.hpp"
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include "halo2/sound_gp_image_tu15.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Invalid native GP scratch resource was accepted");}
void put(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.write(at,b);}
std::uint32_t word(const SourceRuntimeMemory& m,std::uint32_t at){auto b=m.view(at,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);return v;}
void constant(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.mount(at,b,SourceRuntimeMemory::Access::ReadOnly,"authenticated scratch caller import identity");}
constexpr std::uint32_t context=0x2001000,stack=0x2008000;
struct Fixture {
    SourceRuntimeMemory memory;
    std::shared_ptr<SdlSoundPlatform> output;
    std::unique_ptr<SourceRuntimeSoundDmaResources> pages;
    std::set<std::uint32_t> sites;
    struct Publication {std::uint32_t pc,address,value;};
    std::vector<Publication> publications;
    struct Memory final:SourcePhysicsContactMemory,SourceSoundDspImageMmioTu15{
        Fixture& f;explicit Memory(Fixture& v):f(v){}
        void read(std::uint32_t at,std::span<std::byte> out)const override{
            if(at>=0xfe800000)throw std::runtime_error("Native GP publication attempted an unowned MMIO preimage read");
            f.memory.read(at,out);
        }
        void write(std::uint32_t at,std::span<const std::byte> in)override{
            if(at>=0xfe800000)throw std::runtime_error("GP MMIO store bypassed typed publication");
            f.memory.write(at,in);
        }
        void write_sound_dsp_mmio(std::uint32_t pc,std::uint32_t at,std::uint32_t value)override{
            f.pages->publish_gp_scratch(context,pc,at,value);f.publications.push_back({pc,at,value});
        }
    } view{*this};
    explicit Fixture(std::shared_ptr<SdlSoundPlatform> device):output(std::move(device)){
        memory.mount(context,std::vector<std::byte>(0x24),SourceRuntimeMemory::Access::ReadWrite,"declared GP context retained through source constructor");
        memory.mount(stack-0x1000,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"declared native scratch caller stack");
        memory.mount(0x391420,std::vector<std::byte>(0x20),SourceRuntimeMemory::Access::ReadWrite,"real GP descriptor constructor outputs");
        memory.mount(0x2002000,std::vector<std::byte>(0x40),SourceRuntimeMemory::Access::ReadWrite,"declared extra descriptor resources for append branch");
        memory.mount(0x390bc0,std::vector<std::byte>(8),SourceRuntimeMemory::Access::ReadWrite,"source loader-zeroed allocation counters");
        memory.mount(0x3912a0,std::vector<std::byte>(8),SourceRuntimeMemory::Access::ReadWrite,"original statistics constructor outputs");
        constant(memory,0x41b640,0x800000a6);mount_native_tu15_sound_dma_imports(memory);
        for(auto [slot,id]:{std::pair{0x41b79cu,0x8000000fu},{0x41b7ecu,0x80000017u},{0x41b7c0u,0x800000afu},{0x41b798u,0x80000011u}})constant(memory,slot,id);
        pages=std::make_unique<SourceRuntimeSoundDmaResources>(memory,output);
        run(0x387bb5,{});run(0x387baa,{});
    }
    SourcePhysicsWorldRuntimeAbi run(std::uint32_t entry,std::initializer_list<std::uint32_t> args,std::uint32_t self=context){
        SourcePhysicsWorldRuntimeAbi a{};a.gp[1]=self;a.gp[4]=stack;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        put(memory,stack,0x2100000);unsigned index=1;for(auto value:args)put(memory,stack+4*index++,value);
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){sites.insert(pc);};
        services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
            check(&bank==&view,"Scratch CPU dispatch lost its original memory view");
            if(pages->dispatch(memory,call,state))return;
            if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(view,call.target,state,{0x27f,0x1f80},services,observe);return;}
            if(source_sound_dma_storage_tu15_owns_entry(call.target)){source_run_sound_dma_storage_tu15(view,call.target,state,{0x27f,0x1f80},services,observe);return;}
            throw std::runtime_error("Original native GP scratch dependency is unowned");
        };
        SourceRuntimeSoundDmaResources::CallScope scope(pages.get());
        if(source_sound_dma_storage_tu15_owns_entry(entry))source_run_sound_dma_storage_tu15(view,entry,a,{0x27f,0x1f80},services);
        else if(source_sound_sdk_tu15_owns_entry(entry))source_run_sound_sdk_tu15(view,entry,a,{0x27f,0x1f80},services);
        else source_run_sound_dsp_image_tu15(view,entry,a,{0x27f,0x1f80},services);
        return a;
    }
    void allocate(std::uint32_t descriptor,std::uint32_t bytes,std::uint32_t alignment){
        put(memory,descriptor+8,bytes);auto a=run(0x38e71c,{descriptor,alignment,4});
        check(a.gp[0]==0&&word(memory,descriptor)&&word(memory,descriptor+4),"Original DMA descriptor constructor did not publish real pages");
    }
    void initialize(){
        // These are the exact two GP requests from original38E996, followed
        // by original388294's516-entry reservation. The full factory's other
        // allocations and consumers are not supplied by this declared context.
        allocate(0x391420,0xc000,0x4000);allocate(0x391430,0x1060,0x4000);
        run(0x38dd0c,{1});run(0x38dd54,{0x204});
    }
};
void run(const std::shared_ptr<SdlSoundPlatform>& output,const SoundGpImageTu15& image){
    Fixture f(output);std::array<std::byte,4> tiny{};
    check(!f.pages->gp_scratch_binding(),"Fresh GP owner invented a table base/count");
    rejected([&]{f.pages->read_gp_scratch(0,tiny);});f.initialize();
    auto binding=f.pages->gp_scratch_binding();check(binding&&binding->entries==8&&f.publications.size()==2,"Original GP initialization did not publish its actual8-entry prefix");
    check(word(f.memory,context)==524&&word(f.memory,context+4)==516,"Original GP table reservation differs");
    // Original38F5F3 appends the last three pages of the19-page EP scratch
    // allocation; the fourth page is the parent's separate391410 descriptor.
    // Software EP/GP caller contexts remain declared here. Their complete
    // device/PIO constructor order is not supplied by this component.
    constexpr std::uint32_t gp_object=0x2003000,ep_object=0x2003080,ep_context=0x2003100;
    f.memory.mount(gp_object,std::vector<std::byte>(0x200),SourceRuntimeMemory::Access::ReadWrite,"declared retained GP/EP software append caller contexts");
    f.memory.mount(0x391410,std::vector<std::byte>(0x10),SourceRuntimeMemory::Access::ReadWrite,"actual parent descriptor11 resource fields");
    f.memory.mount(0x391440,std::vector<std::byte>(0x10),SourceRuntimeMemory::Access::ReadWrite,"actual parent EP scratch descriptor14 resource fields");
    f.allocate(0x391440,0x13000,0x4000);f.allocate(0x391410,4096,0x4000);
    put(f.memory,gp_object+8,context);put(f.memory,ep_object,ep_context);put(f.memory,ep_object+4,gp_object);put(f.memory,ep_context+0x10,0x391440);
    f.run(0x38f5f3,{0x10000},ep_object);
    const auto ep_source=word(f.memory,0x391440),ep_physical=word(f.memory,0x391444),gp_table=word(f.memory,0x391430);
    check(word(f.memory,ep_source+0x438)==0x4000&&word(f.memory,ep_source+0x43c)==0x600&&
          word(f.memory,context+4)==513&&f.pages->gp_scratch_binding()->entries==11,"Original EP bridge did not retain its exact metadata/three-page append");
    for(unsigned i=0;i<3;++i)check(word(f.memory,gp_table+(8+i)*8)==ep_physical+0x10000+i*4096&&
          word(f.memory,gp_table+(8+i)*8+4)==0,"Original EP bridge did not publish the actual last three physical pages");
    put(f.memory,stack+0x100,0);f.run(0x38de07,{0x391410,1,stack+0x100});
    check(word(f.memory,stack+0x100)==0xb000&&word(f.memory,context+4)==512&&f.pages->gp_scratch_binding()->entries==12&&
          word(f.memory,gp_table+11*8)==word(f.memory,0x391414),"Original separate page append lost its actual reservation/cursor");
    auto a=f.run(0x38de81,{image.user_scratch_bytes});check(a.gp[0]==0,"Original GP data-pool allocation did not complete");
    const auto pool=f.pages->gp_pool_allocations().at(0);
    check(pool.locked&&pool.requested==774144&&f.pages->gp_scratch_binding()->entries==201,"Original GP publication lost its189 actual data pages");
    f.memory.mount(0x3000000,image.decoded_resource,SourceRuntimeMemory::Access::ReadOnly,"authenticated immutable image preparation output");
    std::vector<std::byte> control(image.control_program.size()*4);
    for(std::size_t i=0;i<control.size();++i)control[i]=std::byte(image.control_program[i/4]>>(8*(i%4)));
    f.memory.mount(0x3010000,control,SourceRuntimeMemory::Access::ReadOnly,"authenticated original GP control bytes");
    f.run(0x38de59,{0,0x3000000,std::uint32_t(image.decoded_resource.size())});
    f.run(0x38de59,{0,0x3010000,std::uint32_t(control.size())});
    std::vector<std::byte> expected(image.user_scratch_base+image.user_scratch_bytes),actual(expected.size());
    std::ranges::copy(image.decoded_resource,expected.begin());std::ranges::copy(control,expected.begin());
    f.pages->read_gp_scratch(0,actual);check(actual==expected,"Real native SGE backing differs from immutable GP boot input");
    check(std::to_integer<unsigned>(actual[0x810])==3,"Native scratch owner supplied a GP request acknowledgement");
    std::array<std::byte,37> pattern{},back{};for(unsigned i=0;i<pattern.size();++i)pattern[i]=std::byte(3*i+1);
    f.pages->write_gp_scratch(0xbff8,pattern);f.pages->read_gp_scratch(0xbff8,back);check(back==pattern,"SGE write/read lost its cross-resource bytes");
    check(std::ranges::equal(f.memory.view(pool.source,29),std::span(pattern).subspan(8)),"GP scratch write used shadow pool storage");
    const auto count=f.pages->gp_scratch_binding()->entries;
    rejected([&]{f.pages->publish_gp_scratch(context,0x38df31,0xfe8020d4,*count+1);});
    check(f.pages->gp_scratch_binding()->entries==count,"Rejected publication changed the actual GP count");
    rejected([&]{f.pages->read_gp_scratch(*count*4096-2,tiny);});
    bool foreign=false;std::thread t([&]{try{f.pages->read_gp_scratch(0,tiny);}catch(const std::exception&){foreign=true;}});t.join();check(foreign,"Foreign scratch executor was admitted");
    // Preflight an entire transfer. An invalid later SGE must not partially
    // mutate the first real page before the missing ownership is discovered.
    const auto table=word(f.memory,0x391430);
    const auto first=f.memory.view(word(f.memory,0x391420)+4092,4);
    std::array<std::byte,4> before{};std::ranges::copy(first,before.begin());const auto saved=word(f.memory,table+8);
    put(f.memory,table+8,0xdead000);rejected([&]{f.pages->write_gp_scratch(4092,pattern);});
    check(std::ranges::equal(f.memory.view(word(f.memory,0x391420)+4092,4),before),"Failed SGE preflight partially changed an earlier page");put(f.memory,table+8,saved);
    put(f.memory,table+4,1);rejected([&]{f.pages->read_gp_scratch(0,tiny);});put(f.memory,table+4,0);
    const auto grown=image.user_scratch_bytes+4096;
    a=f.run(0x38de81,{grown});
    check(a.gp[0]==0&&f.pages->gp_pool_allocations().size()==1&&
          f.pages->gp_scratch_binding()->entries==202&&word(f.memory,0x390bc0)==grown,
          "Original GP growth/release did not retire the old real pages and source statistics");
    rejected([&]{f.memory.view(pool.source,pool.capacity);});
    f.pages->read_gp_scratch(0xc000,tiny);
    check(std::ranges::all_of(tiny,[](auto b){return b==std::byte{};}),"Original grown GP pool was not cleared by source code");
    f.run(0x38dd26,{});
    check(f.pages->gp_pool_allocations().empty()&&word(f.memory,context+0x14)==0&&word(f.memory,0x390bc0)==0,
          "Original GP context destructor did not unlock/free the actual pool and clear its pointer/statistics");
    rejected([&]{f.pages->read_gp_scratch(0xc000,tiny);});
    check(!f.sites.contains(0x38846e),"Scratch test claimed to execute the actual upload/acknowledgement caller");
    std::cout<<"PASS original native GP scratch: "<<f.sites.size()<<" instruction addresses; table524/reserve516, actual8+3 EP tail+1 separate prefix/189 data SGEs, "<<actual.size()<<" actual backing bytes; request3 retained; source growth to190 pages and context destructor retire actual pools\n";
}
}
int main(int argc,char** argv)try{
    check(argc==2,"Supply owned TU1.5 executable");if(!std::filesystem::exists(argv[1]))return 77;
    const auto image=load_sound_gp_image_tu15(argv[1]);
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
    const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","GP scratch test requires an actual native output backend");
    run(output,image);std::cout<<"LIMIT declared upstream descriptor inputs and prepared immutable image; full factory/upload, processor/device/clock, and audible parity remain open; backend="<<driver<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
