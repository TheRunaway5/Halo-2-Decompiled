#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* s){if(!v)throw FormatError(s);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Native render prepare changed World topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        if(a.regions[i].address==0x60000000u)continue;
        if(a.regions[i]!=b.regions[i]) {
            for(std::size_t n=0;n<a.regions[i].bytes.size();++n)if(a.regions[i].bytes[n]!=b.regions[i].bytes[n])
                throw FormatError("Native render prepare differs at "+std::to_string(a.regions[i].address+n)+" in "+a.regions[i].owner);
        }
    }
}
void restore_bytes(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& saved) {
    // Keep every actual native allocation/lease in place. Replacing World
    // topology with a bare memory checkpoint would retire those owners.
    const auto current=world.memory().checkpoint();check(saved.regions.size()==current.regions.size(),"Preparation services changed fixture topology");
    for(std::size_t i=0;i<saved.regions.size();++i) {
        const auto& a=saved.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.page_bytes==b.page_bytes&&a.committed==b.committed&&a.bytes.size()==b.bytes.size(),
            "Preparation services replaced a native fixture lease");
        if(a.bytes==b.bytes)continue;
        check(a.access==SourceRuntimeMemory::Access::ReadWrite,"Preparation services changed a readonly fixture bank");
        for(std::size_t n=0;n<a.bytes.size();) {
            if(a.bytes[n]==b.bytes[n]){++n;continue;}
            const auto first=n;while(n<a.bytes.size()&&a.bytes[n]!=b.bytes[n])++n;
            world.write_source_call_input(a.address+std::uint32_t(first),std::span(a.bytes).subspan(first,n-first));
        }
    }
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Preparation tests require owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto data=0x1000000u,pool=data,rows=data+0x1000u,tags=data+0x2000u,model=data+0x3000u,
        sections=data+0x4000u,bsp=data+0x5000u,clusters=data+0x6000u,definitions=data+0x7000u,
        instances=data+0x8000u,body=data+0x9000u,queue=data+0xa000u,ctx=data+0xb000u,
        frame=data+0xc000u,meta=data+0xd000u,stamps=data+0xe000u,units=0x4c6958u;
    check(m.find_unowned_range(data,data+0x20000u,0x20000u,4096)==data,"Preparation fixture overlaps another owner");
    m.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native-render-prepare-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(data);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    const auto arithmetic=[](const auto& abi){return NativeTu15PhysicsFrameFp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};};
    const auto same_fp=[&](const auto& expected,const auto& actual){const auto gold=arithmetic(expected);
        check(gold.control_word==actual.control_word&&gold.status==actual.status&&gold.mxcsr==actual.mxcsr,"Preparation changed final World FP");};
    put(0x4ecbd0,pool);put(pool+0x44,rows);put(0x4f0414,tags);put(tags+8,model);put(model+0x28,sections);
    put(0x4ecc18,bsp);put(bsp+0xa0,clusters);put(bsp+0x13c,definitions);put(bsp+0x144,instances);
    put(0x4f2d6c,frame);put(frame+0x64,meta);put(frame+0x38,0xabcd1234);put(meta+0x44,stamps);
    unsigned geometry_cases{},prepare_cases{};
    for(unsigned kind:{0u,1u,2u})for(unsigned request:{0u,1u})for(bool resident:{false,true}) {
        put(units+4,kind==0?0u:0xffffffffu);put(units+12,0);put(instances+0x34,0,2);
        const auto section=kind==0?sections:kind==1?clusters:definitions;
        const auto header=section+(kind==0?0x38u:0x28u),geometry=section+(kind==0?0x34u:0x50u);
        put(geometry,0x56781234);put(header+0x1e,resident?1:0,1);put(header+0x20,0xbeef0001);put(header+8,0);
        const auto before=m.checkpoint();auto original=world.prepare_source_call(std::array{kind==2?1u:0u});
        original.gp[0]=0;original.gp[2]=request;world.run_actual(0x449d0,original);const auto expected=m.checkpoint();
        check(original.gp[0]==((request&&!resident)?0u:0x56781234u),"Genuine449D0 did not exercise its declared request path");
        if(request&&resident)check(Bytes(m.view(stamps+24+0x14,4)).u32(0)==0xabcd1234,"Genuine resident resource did not publish its stamp");
        restore_bytes(world,before);const auto base=world.prepare_source_call();auto fp=arithmetic(base);
        const auto actual=source_native_render_geometry_tu15(world,0,std::uint8_t(request),kind==2?1:0,fp);
        check(actual==original.gp[0],"Native449D0 changed geometry result");equal(expected,m.checkpoint());same_fp(original,fp);++geometry_cases;
    }
    for(unsigned kind:{0u,1u,2u})for(unsigned path:{0u,1u,2u}) {
        put(units+4,kind==0?0u:0xffffffffu);put(units+12,0);put(instances+0x34,0,2);
        const auto section=kind==0?sections:kind==1?clusters:definitions;
        const auto header=section+(kind==0?0x38u:0x28u),geometry=section+(kind==0?0x34u:0x50u);
        put(geometry,0);put(header+8,16);put(header+0xc,path==0?0xffffffffu:0u);put(header+0x10,0);
        put(header+0x18,0,2);put(header+0x1c,kind==0?0xfff8u:0x24u,2);put(header+0x1e,0,1);put(header+0x20,0xabcd0001);
        put(tags,kind==0?0x65646f6du:0x73627370u);put(0x4751ac,path==2?1:0,1);put(0x4f2d70,0);put(0x4f2d78,7);
        constexpr auto flight=data+0xf000u,flight_rows=flight+0x100u,allocation=data+0x18000u;
        put(0x4f2d5c,flight);put(flight+0x44,flight_rows);put(flight_rows+12+2,path==1?1:0,1);put(flight_rows+12+3,7,1);
        put(0x4f2d64,allocation);put(frame+0x34,0);put(stamps+24+8,0);
        const auto before=m.checkpoint();auto original=world.prepare_source_call(std::array{kind==2?1u:0u});
        original.gp[0]=0;original.gp[2]=1;world.run_actual(0x449d0,original);const auto expected=m.checkpoint();
        check(original.gp[0]==(path==1?allocation+8u:0u),"Genuine449D0 failed its publication/refusal fixture");
        if(path==1)check(Bytes(m.view(geometry,4)).u32(0)==allocation+8u,"Genuine request did not publish geometry during the call");
        if(path==2&&kind!=0)check(Bytes(m.view(0x4f2d78,4)).u32(0)==8,"Genuine in-flight refusal missed its sbsp counter");
        restore_bytes(world,before);auto fp=arithmetic(world.prepare_source_call());
        const auto actual=source_native_render_geometry_tu15(world,0,1,kind==2?1:0,fp);
        check(actual==original.gp[0],"Native449D0 changed publication/refusal result");equal(expected,m.checkpoint());same_fp(original,fp);++geometry_cases;
    }
    for(unsigned i=0;i<7;++i) {
        put(rows+12*i+8,body+0x100*i);put(body+0x100*i+0xc,0xffffffffu);put(body+0x100*i+0x10,0xffffffffu);
        put(body+0x100*i+0xdc,0xffffffffu); // genuine D7230 has no active attachment
    }
    put(ctx+0x14,queue);put(queue+8,queue+0x100);put(queue+12,queue+0x200);
    put(queue+0x100,0x8001,2);put(queue+0x102,0xffff,2);put(queue+0x200,0xabcd0000);put(queue+0x204,0xbeef0006);
    for(unsigned mode:{0u,1u,2u,0x10000u})for(unsigned shape:{0u,1u,2u}) {
        put(queue+4,2,2);put(body+0x10,shape?0xaaaa0001u:0xffffffffu);put(body+0xc,shape?0xbbbb0002u:0xffffffffu);
        put(body+0x100+0x10,shape==2?0xcccc0003u:0xffffffffu);put(body+0x100+0xc,shape==2?0xdddd0004u:0xffffffffu);
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[7]=ctx;original.gp[3]=mode;
        world.run_actual(0x41c70,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        auto fp=arithmetic(world.prepare_source_call());source_native_render_prepare_tu15(world,ctx,mode,fp);
        equal(expected,m.checkpoint());same_fp(original,fp);++prepare_cases;
    }
    std::cout<<"PASS "<<geometry_cases<<" complete449D0 World comparisons with genuine resident/unavailable130980 and "<<prepare_cases<<
        " complete41C70/41C10 World hierarchy comparisons with genuineD7230/41970, all non-stack bytes and FP\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
