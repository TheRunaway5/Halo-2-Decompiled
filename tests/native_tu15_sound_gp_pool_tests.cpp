#include "halo2/source_runtime_sound_dma_resources.hpp"
#include "halo2/source_sound_dsp_image_tu15.hpp"
#include "halo2/source_sound_sdk_tu15.hpp"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught,"Native GP pool accepted an invalid resource request");}
void put(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t word){std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(word>>(i*8));m.write(at,b);}
void constant(SourceRuntimeMemory& m,std::uint32_t at,std::uint32_t word){std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(word>>(i*8));m.mount(at,b,SourceRuntimeMemory::Access::ReadOnly,"authenticated GP caller native import identity");}
constexpr std::uint32_t context=0x2001000,stack=0x2008000;
struct Fixture {
    SourceRuntimeMemory memory;
    std::shared_ptr<SdlSoundPlatform> output;
    std::unique_ptr<SourceRuntimeSoundDmaResources> owner;
    explicit Fixture(std::shared_ptr<SdlSoundPlatform> device,unsigned context_bytes=0x24):output(std::move(device)){
        // The original four MOVSD reads from context14 reach context23.
        // Give the caller exactly that observed36-byte footprint.
        memory.mount(context,std::vector<std::byte>(context_bytes),SourceRuntimeMemory::Access::ReadWrite,"declared exact GP allocation caller context");
        memory.mount(stack-0x1000,std::vector<std::byte>(0x2000),SourceRuntimeMemory::Access::ReadWrite,"declared GP caller stack");
        memory.mount(context+0x100,std::vector<std::byte>(12),SourceRuntimeMemory::Access::ReadWrite,"declared separate DMA descriptor");
        for(auto [slot,id]:{std::pair{0x41b79cu,0x8000000fu},{0x41b7ecu,0x80000017u},{0x41b7bcu,0x800000adu},{0x41b7c0u,0x800000afu},{0x41b798u,0x80000011u}})constant(memory,slot,id);
        owner=std::make_unique<SourceRuntimeSoundDmaResources>(memory,output);
    }
    SourcePhysicsWorldRuntimeAbi abi(){
        SourcePhysicsWorldRuntimeAbi a{};
        for(unsigned i=0;i<a.gp.size();++i)a.gp[i]=0x12340000+i;
        for(unsigned i=0;i<a.xmm.size();++i)for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x100*i+j;
        a.gp[3]=context;a.gp[4]=stack;a.eflags=0x246;a.fs_base=0x12340000;
        a.x87_depth=1;a.x87[0]=1.25L;a.x87_status=0x3800;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;
    }
    bool call(SourcePhysicsContactRuntimeCall c,SourcePhysicsWorldRuntimeAbi& a,unsigned consumed,bool result){
        const auto before=a;const bool owned=owner->dispatch(memory,c,a);
        if(!owned){check(a.gp==before.gp&&a.eflags==before.eflags,"Unowned GP call changed CPU state");return false;}
        for(unsigned i=0;i<a.gp.size();++i)if(i!=4&&(i!=0||!result))check(a.gp[i]==before.gp[i],"GP native service changed a preserved register");
        check(a.gp[4]==before.gp[4]+consumed&&a.xmm==before.xmm&&a.eflags==before.eflags&&a.fs_base==before.fs_base&&a.x87_depth==before.x87_depth&&a.x87_status==before.x87_status&&a.x87==before.x87,"GP native service changed its retained ABI or stack extent");return true;
    }
    std::uint32_t allocate(std::uint32_t bytes){
        auto a=abi();put(memory,context+0x1c,bytes);put(memory,stack,0x38797b);put(memory,stack+4,bytes);put(memory,stack+8,0x61645344);
        put(memory,stack+16,0x38ded0);put(memory,stack+20,0x61645344);put(memory,stack+24,bytes);put(memory,stack+28,1);
        check(call({0x387975,0x8000000f,false},a,12,true),"Missing native GP pool allocation owner");return a.gp[0];
    }
    void capacity(std::uint32_t source,std::uint32_t expected,bool freeing=false){
        auto a=abi();put(memory,stack,freeing?0x3879bd:0x387988);put(memory,stack+4,source);
        put(memory,stack+(freeing?8:12),freeing?0x38df50:0x38ded0);a.gp[6]=source;
        check(call({freeing?0x3879b7u:0x387982u,0x80000017,false},a,8,true)&&a.gp[0]==expected,"GP pool capacity differs from its actual page extent");
    }
    void pin(std::uint32_t source,std::uint32_t bytes,bool unlock){
        auto a=abi();a.gp[7]=0x800000af;put(memory,stack,unlock?0x38df42:0x38deec);put(memory,stack+4,source);put(memory,stack+8,bytes);put(memory,stack+12,unlock?1:0);
        check(call({unlock?0x38df40u:0x38deeau,0x800000af,false},a,16,false),"Missing actual GP page lock owner");
    }
    std::uint32_t address(std::uint32_t pointer){auto a=abi();a.gp[0]=pointer;put(memory,stack,0x38df10);put(memory,stack+4,pointer);check(call({0x38df0a,0x800000ad,false},a,8,true),"Missing native GP physical query");return a.gp[0];}
    void release(std::uint32_t source){auto a=abi();put(memory,stack,0x38df50);put(memory,stack+4,source);check(call({0x3879c5,0x80000011,true},a,8,false),"Missing actual GP pool release owner");}
};
void storage(const std::shared_ptr<SdlSoundPlatform>& output,std::uint32_t bytes){
    Fixture f(output);check(f.owner->quiescent(),"Fresh GP page owner is not quiescent");
    const auto source=f.allocate(bytes),capacity=(bytes+0xfffu)&~0xfffu;check(source!=0,"Actual GP page allocation failed");
    const auto rows=f.owner->gp_pool_allocations();check(rows.size()==1,"GP allocation was not retained");const auto& row=rows[0];
    check(row.source==source&&row.physical==(source&0x7fffffffu)&&row.requested==bytes&&row.capacity==capacity&&row.context==context&&!row.locked&&row.native.value>0xffffffffu&&!(row.native.value&0xfff),"GP page/native64 binding differs");
    check(f.owner->allocations().empty(),"GP pool was reported as an initialized DMA descriptor");f.capacity(source,capacity);
    std::array<std::byte,4> data{std::byte{0x31},std::byte{0x42},std::byte{0x53},std::byte{0x64}},out{};
    rejected([&]{f.address(source);});rejected([&]{f.owner->read_gp_pool(row.physical,out);});
    put(f.memory,context+0x14,source);f.pin(source,bytes,false);
    check(f.owner->gp_pool_allocations()[0].locked,"Native GP page lock did not retain its resource");
    rejected([&]{f.pin(source,bytes,false);});rejected([&]{f.release(source);});
    for(std::uint32_t offset=0;offset<capacity;offset+=0x1000)check(f.address(source+offset)==row.physical+offset,"GP physical page lookup lost its actual offset");
    rejected([&]{f.address(source+1);});rejected([&]{f.address(source+capacity);});
    f.owner->write_gp_pool(row.physical+capacity-4,data);f.memory.read(source+capacity-4,out);check(out==data,"GP physical write did not reach retained source pages");
    put(f.memory,source,0x88776655);f.owner->read_gp_pool(row.physical,out);check(out==std::array{std::byte{0x55},std::byte{0x66},std::byte{0x77},std::byte{0x88}},"GP physical read copied a shadow buffer");
    rejected([&]{f.owner->read_gp_pool(row.physical+capacity-1,out);});
    bool foreign=false;std::thread t([&]{try{f.owner->gp_pool_allocations();}catch(const std::runtime_error&){foreign=true;}});t.join();check(foreign,"Foreign GP resource thread was admitted");
    f.pin(source,bytes,true);rejected([&]{f.owner->write_gp_pool(row.physical,data);});rejected([&]{f.pin(source,bytes,true);});
    f.capacity(source,capacity,true);f.release(source);check(f.owner->quiescent()&&f.owner->gp_pool_allocations().empty(),"GP release left native page ownership");
    rejected([&]{f.memory.view(source,capacity);});rejected([&]{f.address(source);});
}
void resize_and_failure(const std::shared_ptr<SdlSoundPlatform>& output){
    Fixture f(output);const auto old=f.allocate(0x2000);put(f.memory,context+0x14,old);f.pin(old,0x2000,false);
    const auto next=f.allocate(0x4000);check(next!=old,"Live GP allocations alias the same namespace");put(f.memory,context+0x14,next);f.pin(next,0x4000,false);
    f.pin(old,0x2000,true);f.capacity(old,0x2000,true);f.release(old);check(f.owner->gp_pool_allocations().size()==1,"GP growth released the new backing");
    f.pin(next,0x4000,true);f.release(next);check(f.owner->quiescent(),"GP growth retirement is incomplete");
    Fixture partial(output);std::uint32_t source{};
    rejected([&]{SourceRuntimeSoundDmaResources::CallScope scope(partial.owner.get());source=partial.allocate(0x2000);throw std::runtime_error("declared source failure after actual GP allocation");});
    check(source&&partial.memory.native_address(source,0x2000).value>0xffffffffu&&!partial.owner->quiescent(),"Failed GP transaction lost diagnostic pages or became quiescent");
    rejected([&]{partial.owner->gp_pool_allocations();});rejected([&]{SourceRuntimeSoundDmaResources::CallScope scope(partial.owner.get());});
}
void rejected_frames(const std::shared_ptr<SdlSoundPlatform>& output){
    Fixture f(output);auto a=f.abi();put(f.memory,stack,0x38797b);put(f.memory,stack+4,0x2000);put(f.memory,stack+8,0x61645344);put(f.memory,stack+16,0x38ded0);put(f.memory,stack+20,0x61645344);put(f.memory,stack+24,0x2000);put(f.memory,stack+28,1);put(f.memory,context+0x1c,0x2000);
    const auto before=a;rejected([&]{f.owner->dispatch(f.memory,{0x387975,0x8000000f,true},a);});check(a.gp==before.gp&&f.owner->quiescent(),"Rejected GP call changed source state or allocated storage");
    put(f.memory,stack+8,0x626f5344);rejected([&]{f.owner->dispatch(f.memory,{0x387975,0x8000000f,false},a);});check(f.owner->quiescent(),"Foreign GP tag allocated native pages");
    put(f.memory,stack+16,0x38835f);check(!f.owner->dispatch(f.memory,{0x387975,0x8000000f,false},a)&&a.gp==before.gp,"General SDK pool caller was intercepted by GP storage");
}
void shared_namespace(const std::shared_ptr<SdlSoundPlatform>& output){
    Fixture f(output);constant(f.memory,0x41b640,0x800000a6);
    const auto descriptor=context+0x100;put(f.memory,descriptor+8,0x2000);
    auto a=f.abi();a.gp[7]=descriptor;put(f.memory,stack,0x3879e2);put(f.memory,stack+4,0x2000);
    put(f.memory,stack+8,0);put(f.memory,stack+12,0xffffffff);put(f.memory,stack+16,0x1000);put(f.memory,stack+20,4);put(f.memory,stack+28,0x38e734);
    check(f.call({0x3879dc,0x800000a6,false},a,24,true),"Existing descriptor page allocation was lost");
    const auto descriptor_pages=f.owner->allocations().at(0);const auto source=f.allocate(0x3000);const auto pool=f.owner->gp_pool_allocations().at(0);
    check(pool.physical>=descriptor_pages.physical+descriptor_pages.capacity&&source!=descriptor_pages.source,"GP and existing DMA resource namespaces overlap");
    f.release(source);check(f.owner->allocations().size()==1&&!f.owner->quiescent(),"GP pool release destroyed a descriptor allocation");
}
void original_caller(const std::shared_ptr<SdlSoundPlatform>& output){
    Fixture f(output);constexpr std::uint32_t bytes=774144,pages=189,prefix=7,table=0x2002000;
    f.memory.mount(0x390bc0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared source pool statistics input");
    f.memory.mount(0x3912a4,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"original statistics-constructor output");
    f.memory.mount(0x391430,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared actual GP table resource binding");
    f.memory.mount(table,std::vector<std::byte>((prefix+pages)*8,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"actual native SGE backing; prefix explicitly unconfigured");
    put(f.memory,0x391430,table);put(f.memory,context,prefix+pages);put(f.memory,context+4,pages);put(f.memory,context+0x20,0xa5a5a5a5);
    auto a=f.abi();put(f.memory,stack,0x2100000);
    source_run_sound_sdk_tu15(f.memory,0x387baa,a,{0x27f,0x1f80},{});
    struct Frontier{};std::set<std::uint32_t> sites;unsigned clears{},pairs{};
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){if(pc==0x38df31)throw Frontier{};sites.insert(pc);};
    services.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(f.owner->dispatch(bank,call,state))return;
        if(source_sound_sdk_tu15_owns_entry(call.target)){source_run_sound_sdk_tu15(bank,call.target,state,{0x27f,0x1f80},services,observe);return;}
        throw std::runtime_error("Unowned original GP allocation caller dependency");
    };
    SourcePhysicsContactRecordObserver observe=[&](const auto& store){if(store.instruction==0x3879a3)++clears;if(store.instruction==0x38df10||store.instruction==0x38df1b)++pairs;};
    a=f.abi();a.gp[1]=context;put(f.memory,stack,0x2100000);put(f.memory,stack+4,bytes);bool reached=false;
    {
        SourceRuntimeSoundDmaResources::CallScope scope(f.owner.get());
        try{source_run_sound_dsp_image_tu15(f.memory,0x38de81,a,{0x27f,0x1f80},services,observe);}catch(const Frontier&){reached=true;}
    }
    const auto allocations=f.owner->gp_pool_allocations();check(reached&&allocations.size()==1&&allocations[0].locked,"Original GP caller did not reach its genuine publication boundary with locked pages");
    const auto& row=allocations[0];check(clears==bytes/4&&pairs==2*pages&&a.gp[0]==prefix+pages,"Original GP clear/table/cursor instruction work differs");
    const auto word=[&](std::uint32_t at){const auto raw=f.memory.view(at,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(raw[i])<<(8*i);return v;};
    for(unsigned i=0;i<pages;++i)check(word(table+(prefix+i)*8)==row.physical+i*4096&&word(table+(prefix+i)*8+4)==0,"Original CPU SGE writer lost the actual retained GP page");
    check(std::ranges::all_of(f.memory.view(table,prefix*8),[](auto b){return b==std::byte{0xa5};}),"GP test invented or overwrote its unconfigured prefix");
    check(word(0x390bc0)==row.capacity&&word(context+0x14)==row.source&&word(context+0x1c)==bytes,"Original pool counter/context stores differ");
    std::cout<<"Original38DE81 native composition: "<<sites.size()<<" instruction addresses, "<<clears<<" original zero stores, "<<pages<<" actual SGEs; stopped before38DF31 MMIO publication\n";
}
void replacement_and_short_context(const std::shared_ptr<SdlSoundPlatform>& output){
    Fixture short_context(output,0x20);rejected([&]{short_context.allocate(0x2000);});check(short_context.owner->quiescent(),"GP owner accepted a caller missing the fourth MOVSD word");
    Fixture f(output);const auto source=f.allocate(0x2000);put(f.memory,context+0x14,source);f.pin(source,0x2000,false);
    const auto row=f.owner->gp_pool_allocations().at(0);f.memory.release(source);f.memory.mount(source,std::vector<std::byte>(row.capacity),SourceRuntimeMemory::Access::ReadWrite,"foreign replacement at an old GP token");
    rejected([&]{f.owner->gp_pool_allocations();});std::array<std::byte,4> out{};rejected([&]{f.owner->read_gp_pool(row.physical,out);});
}
}
int main()try{
    auto output=std::make_shared<SdlSoundPlatform>(SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,64,{}});
    const auto* driver=SDL_GetCurrentAudioDriver();check(driver&&std::string(driver)!="dummy"&&std::string(driver)!="disk","GP ownership test requires an actual native output backend");
    for(auto bytes:{0x1000u,0x1001u,774144u})storage(output,bytes);resize_and_failure(output);rejected_frames(output);shared_namespace(output);replacement_and_short_context(output);original_caller(output);
    std::cout<<"PASS native GP pool allocation/capacity, locked real-page address/read/write, growth/release, frame/thread rejection and fault lifetime; backend="<<driver<<'\n';
    std::cout<<"LIMIT native resource/ABI contract; original GP SGE construction, processor/device lifecycle and audible playback remain separate gates\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
