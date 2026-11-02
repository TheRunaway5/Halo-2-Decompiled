#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* s){if(!v)throw FormatError(s);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Native command walk changed World topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        if(a.regions[i].address==0x60000000u)continue;
        if(a.regions[i]!=b.regions[i]) {
            for(std::size_t n=0;n<a.regions[i].bytes.size();++n)if(a.regions[i].bytes[n]!=b.regions[i].bytes[n])
                throw FormatError("Native command walk differs at "+std::to_string(a.regions[i].address+n)+" in "+a.regions[i].owner);
        }
    }
}
void restore_bytes(PhysicsLiveWorldTu15& world,const SourceRuntimeMemory::Checkpoint& saved) {
    // Keep every actual native allocation/lease in place. Replacing World
    // topology with a bare memory checkpoint would retire those owners.
    const auto current=world.memory().checkpoint();check(saved.regions.size()==current.regions.size(),"Command services changed fixture topology");
    for(std::size_t i=0;i<saved.regions.size();++i) {
        const auto& a=saved.regions[i];const auto& b=current.regions[i];
        check(a.address==b.address&&a.owner==b.owner&&a.access==b.access&&a.page_bytes==b.page_bytes&&a.committed==b.committed&&a.bytes.size()==b.bytes.size(),
            "Command services replaced a native fixture lease");
        if(a.bytes==b.bytes)continue;
        check(a.access==SourceRuntimeMemory::Access::ReadWrite,"Command services changed a readonly fixture bank");
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
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Command walk requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto data=0x1000000u,ctx=data,pool=data+0x4000u,rows=data+0x4800u,
        tags=data+0x5000u,model=data+0x6000u,sections=data+0x7000u,geom=data+0x8000u,
        parts=data+0x9000u,materials=data+0xa000u,body=data+0xb000u,mask=data+0xc000u,units=0x4c6958u;
    check(m.find_unowned_range(data,data+0x20000u,0x20000u,4096)==data,"Command fixture overlaps another owner");
    m.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native-command-declared-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(data);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    put(0x4cd3a0,ctx);put(ctx+0x14,ctx+0x3000);put(0x4f0414,tags);put(tags+8,model);
    put(0x4ecbd0,pool);put(pool+0x44,rows);put(rows+8,body);put(0x4f3218,model);put(model+8,3);
    put(model+0x28,sections);put(model+0x64,materials);put(sections+0x34,geom);put(sections+0x2c,0xffff,2);
    put(geom,3);put(geom+4,parts);put(materials+0xc,0);
    put(body+0x50,std::bit_cast<std::uint32_t>(1.f));put(body+0x60,std::bit_cast<std::uint32_t>(2.f));
    put(body+0x70,std::bit_cast<std::uint32_t>(3.f));
    for(unsigned i=0;i<3;++i) {
        put(parts+72*i+0xa,i*31,2);put(parts+72*i+0xc,2,2);
        put(parts+72*i+0x2c,std::bit_cast<std::uint32_t>(i?3.f:0.f));
    }
    for(unsigned i=0;i<4;++i) {
        put(units+32*i+4,0);put(units+32*i+8,0);put(units+32*i+12,31u<<4);
        put(units+32*i+0x14,mask);put(units+32*i+0x18,1u<<16);
    }
    unsigned compared{};
    for(unsigned mode:{0u,1u})for(unsigned visible:{0u,1u})for(unsigned selector:{0u,1u,2u}) {
        put(ctx+0x2a78,mode);put(0x4cd39c,mode?1:4);put(0x4cd398,4);
        put(0x515f1c,0);put(0x515f20,0);put(0x4d2f1c,0,2);
        for(unsigned i=0;i<4;++i) {
            put(units+32*i,i==3?0x400u:(0x71u));put(0x4cd6f6u+i,255,1);put(0x4cd3a4u+i,13+i*60,1);
        }
        for(unsigned i=0;i<3;++i)put(mask+4*i,visible?0xffffffffu:0u);
        const auto before=m.checkpoint();auto original=world.prepare_source_call();original.gp[0]=selector;
        world.run_actual(0x40e80,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        const auto actual=world.run_native_command_frame_tu15(selector);equal(expected,m.checkpoint());
        check(actual.control_word==original.floating_point->x87_control_word&&actual.status==original.x87_status&&actual.mxcsr==original.floating_point->mxcsr,
            "Native command frame changed complete World FP controls/status");++compared;
    }
    std::cout<<"PASS "<<compared<<" whole40E80 non-stack World-bank and FP comparisons through genuine41C70/449D0/45CD0/41030 and later producers\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
