#include "halo2/source_geometry_resource_request.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* s){if(!v)throw FormatError(s);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Native resource request changed World topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        if(a.regions[i].address==0x60000000u)continue;
        if(a.regions[i]!=b.regions[i]) {
            for(std::size_t n=0;n<a.regions[i].bytes.size();++n)if(a.regions[i].bytes[n]!=b.regions[i].bytes[n])
                throw FormatError("Native resource request differs at "+std::to_string(a.regions[i].address+n)+" in "+a.regions[i].owner);
        }
    }
}
void restore_bytes(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& saved) {
    // Keep every actual native allocation/lease in place. Replacing World
    // topology with a bare memory checkpoint would retire those owners.
    const auto current=world.memory().checkpoint();check(saved.regions.size()==current.regions.size(),"Resource services changed fixture topology");
    for(std::size_t i=0;i<saved.regions.size();++i) {
        const auto& a=saved.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.page_bytes==b.page_bytes&&a.committed==b.committed&&a.bytes.size()==b.bytes.size(),
            "Resource services replaced a native fixture lease");
        if(a.bytes==b.bytes)continue;
        check(a.access==SourceRuntimeMemory::Access::ReadWrite,"Resource services changed a readonly fixture bank");
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
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Resource World test requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto data=0x1000000u,header=data+0x100u,frame=data+0x1000u,meta=data+0x2000u,records=data+0x3000u,
        flight=data+0x4000u,rows=data+0x5000u,tags=data+0x6000u,allocation=data+0x8000u,desc=data+0xa000u;
    check(m.find_unowned_range(data,data+0x20000u,0x20000u,4096)==data,"Resource fixture overlaps another owner");
    m.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native-resource-declared-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(data);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    const auto arithmetic=[](const auto& abi){return NativeTu15PhysicsFrameFp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};};
    const auto same_fp=[&](const auto& abi,const auto& actual){const auto expected=arithmetic(abi);
        check(expected.control_word==actual.control_word&&expected.status==actual.status&&expected.mxcsr==actual.mxcsr,"Native130980 changed final World FP");};
    const auto g=SourceGeometryResourceGlobals::title_update_1_5();
    put(g.tags,tags);put(g.physical,frame);put(frame+0x64,meta);put(meta+0x44,records);put(frame+0x38,0xabcd1234);
    put(g.inflight,flight);put(flight+0x44,rows);put(g.backing,allocation);unsigned request_cases{},publication_cases{};
    for(unsigned flags:{0u,1u,2u,3u,6u,8u,10u,15u})for(unsigned phase=0;phase<6;++phase)
        for(auto group:{0x65646f6du,0x73627370u,0x6c746d70u}) {
        put(tags,group);put(g.policy,phase==4?1:0);put(g.policy_filter,1);put(g.disable_blocking,phase==5?1:0,1);
        put(header+8,phase==1?0:16);put(header+0xc,phase==2?0xffffffffu:0u);put(header+0x10,0);put(header+0x14,desc);
        put(header+0x18,0,2);put(header+0x1c,0x24,2);put(header+0x1e,phase==0?1:0,1);
        put(header+0x20,phase==3?0xffffffffu:1u);put(rows+14,phase==5?0:1,1);put(rows+15,255,1);
        put(g.missing_special,7);put(g.pending_failure,0,1);
        const auto actual_flags=phase==3?flags&~2u:flags;
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=actual_flags;original.gp[1]=header;
        world.run_actual(0x130980,original);const auto expected=m.checkpoint();const auto answer=bool(original.gp[0]&255u);
        restore_bytes(world,before);auto fp=arithmetic(world.prepare_source_call());
        check(world.request_render_geometry_tu15(actual_flags,header,fp)==answer,"Native130980 result differs");equal(expected,m.checkpoint());same_fp(original,fp);
        restore_bytes(world,before);check(world.request_render_geometry(actual_flags,header)==answer,"Native public resource result differs");
        equal(expected,m.checkpoint());++request_cases;
    }
    for(unsigned kind:{0u,1u,2u,3u,255u})for(unsigned stride:{1u,7u,255u})for(bool indexed:{false,true}) {
        put(g.disable_blocking,0,1);put(header+8,0x100);put(header+0xc,0);put(header+0x10,1);put(header+0x14,desc);
        put(header+0x1c,0x24,2);put(header+0x1e,0,1);put(header+0x20,1);put(rows+14,1,1);
        put(desc,kind,1);put(desc+4,0x80,2);put(desc+6,kind<=1?stride:indexed?3:65535,2);
        put(desc+8,103);put(desc+12,0x200);put(allocation+8+0x84,allocation+0x800);
        const auto buffer=indexed?allocation+0x800+3*32:allocation+8+0x80;
        put(buffer+1,stride,1);put(buffer+4,0xe1234567);put(buffer+8,7);
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=3;original.gp[1]=header;
        world.run_actual(0x130980,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        auto fp=arithmetic(world.prepare_source_call());check(world.request_render_geometry_tu15(3,header,fp),"Native resource publication refused");
        equal(expected,m.checkpoint());same_fp(original,fp);++publication_cases;
    }
    unsigned acquire_failures{};
    for(bool limit_refusal:{true,false}) {
        put(tags,0x73627370);put(header+4,0x3000);put(header+8,16);put(header+0xc,0);put(header+0x10,0);
        put(header+0x18,0,2);put(header+0x1e,0,1);put(header+0x20,0xffffffffu);put(g.disable_blocking,0,1);put(g.policy,0);
        put(frame+0x30,limit_refusal?0x40:0);put(frame+0x34,12);put(frame+0x3c,0xffffffffu);put(frame+0x40,0xffffffffu);
        for(unsigned i=0;i<8;++i)put(frame+0x44+4*i,i==1&&limit_refusal?3:0x100);
        put(frame+0x64,meta);put(meta+0x20,16);put(meta+0x24,24);put(meta+0x44,records);put(meta+0x48,meta+0x80);
        put(g.missing_special,0);put(g.pending_failure,0,1);
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=3;original.gp[1]=header;
        world.run_actual(0x130980,original);check(!(original.gp[0]&255u),"Genuine130EB0/13FE50 did not refuse declared allocation");
        const auto expected=m.checkpoint();restore_bytes(world,before);auto fp=arithmetic(world.prepare_source_call());
        check(!world.request_render_geometry_tu15(3,header,fp),"Native130980 accepted refused genuine acquisition");
        equal(expected,m.checkpoint());same_fp(original,fp);++acquire_failures;
    }
    std::cout<<"PASS "<<request_cases<<" whole130980 World request cases through both public entry points and "<<publication_cases<<
        " whole130980/22A680 descriptor cases, complete non-stack banks and FP\n";
    std::cout<<"PASS "<<acquire_failures<<" whole130980 World acquisition failures through genuine130EB0/13FE50\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
