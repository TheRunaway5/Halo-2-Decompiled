#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/source_bsp_draw_collector.hpp"
#include "halo2/source_runtime_write_protection.hpp"
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
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Lighting World test requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto d=0x1000000u,lm=d+0x100u,group=d+0x200u,refs=d+0x400u,scenery=d+0x500u,
        checksums=d+0x600u,buckets=d+0x800u,bsp=d+0x1000u,other=d+0x2000u,instances=d+0x3000u,
        alternate=d+0x4000u,tags=d+0x5000u,model=d+0x6000u,physical=d+0x7000u,pool=d+0x8000u,
        records=d+0x9000u,flight=d+0xa000u,rows=d+0xb000u,objects=d+0xc000u,headers=d+0xd000u,body=d+0xe000u;
    check(m.find_unowned_range(d,d+0x20000u,0x20000u,4096)==d,"Lighting fixture overlaps another owner");
    m.mount(d,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native-lighting-declared-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(0x1000000u);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    const auto word=[&](std::uint32_t at){return Bytes(m.view(at,4)).u32(0);};
    const auto arithmetic=[](const auto& abi){return NativeTu15PhysicsFrameFp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};};
    const auto same_fp=[&](const auto& abi,const auto& fp){const auto expected=arithmetic(abi);
        check(expected.control_word==fp.control_word&&expected.status==fp.status&&expected.mxcsr==fp.mxcsr,"Native lighting changed World FP");};
    put(0x4ecc14,lm);put(0x4ecc18,bsp);put(lm+0x80,1);put(lm+0x84,group);put(group+4,0x12345678);put(bsp+8,0x12345678);
    put(group+0x54,refs);put(group+0x5c,checksums);put(group+0x64,scenery);put(group+0x44,buckets);
    put(bsp+0x144,instances);put(other+0x144,alternate);put(alternate+88+0x56,1,2);
    put(0x4f0414,tags);put(tags,0x6c746d70);put(tags+24,model);put(model+8,0x87654321);put(checksums+20,0x87654321);
    put(0x4ecbd0,objects);put(objects+0x38,16);put(objects+0x24,12);put(objects+0x44,headers);
    put(headers+24,0x8001,2);put(headers+32,body);put(body+7,1,1);
    put(0x4f2d6c,physical);put(physical+0x64,pool);put(pool+0x44,records);put(physical+0x38,0x1234abcd);
    put(0x4f2d5c,flight);put(flight+0x44,rows);put(0x4f2d64,d+0xf000);put(rows+14,1,1);put(rows+15,255,1);
    const auto base=m.checkpoint();unsigned lighting_cases{},factory_cases{};
    for(unsigned profile=0;profile<16;++profile)for(bool dynamic:{false,true}) {
        restore_bytes(world,base);const auto resource=buckets+0xc;put(resource+8,16);put(resource+0x1c,0x24,2);
        put(resource+0x1e,1,1);put(resource+0x20,1);put(instances+88+0x56,1,2);
        std::uint32_t owner=0x80010002u,index=1;
        if(profile==0)put(0x4ecc14,0);
        else if(profile==1)put(lm+0x80,0);
        else if(profile==2)put(lm+0x80,0x80000000);
        else if(profile==3)put(0x4ecc18,0);
        else if(profile==4)put(group+0x1c,0xffffffffu);
        else if(profile==5)put(group+4,0);
        else if(profile==6)index=0x7ff;
        else if(profile==7)put(model+8,0);
        else if(profile==8){put(resource+0x1e,0,1);put(resource+8,0);}
        else if(profile==9){put(resource+0x1e,0,1);put(resource+0xc,0xffffffffu);}
        else if(profile==10)put(resource+0x1e,0,1);
        else if(profile==11){put(resource+0x1e,0,1);put(0x4751ac,1,1);put(rows+14,0,1);}
        else if(profile==12){put(0x4ecc14,0);put(headers+24,0x8002,2);}
        else if(profile==13){put(0x4ecc14,0);put(body+7,0,1);}
        else if(profile==14){put(0x4ecc14,0);owner=0xffffffffu;}
        else if(profile==15){
            put(resource+0x1e,0,1);put(resource+0x1c,bsp+0x140-resource,2);
            put(instances+88+0x56,0,2);put(0x4f2d64,alternate-8);
        }
        const auto before=m.checkpoint();auto original=world.prepare_source_call();
        original.gp[0]=owner;original.gp[1]=index;original.gp[2]=dynamic?0xabcd0001u:0xffffffffu;
        world.run_actual(0x2cfe0,original);const auto expected=m.checkpoint();restore_bytes(world,before);
        auto fp=arithmetic(world.prepare_source_call());
        check(world.classify_render_lighting_tu15(owner,dynamic?0xabcd0001u:0xffffffffu,index,fp)==original.gp[0],"World native2CFE0 result differs");
        equal(expected,m.checkpoint());same_fp(original,fp);++lighting_cases;
    }
    restore_bytes(world,base);
    constexpr auto clusters=d+0x10000u,definitions=d+0x11000u;
    put(bsp+0xa0,clusters);put(bsp+0x13c,definitions);put(clusters+0x14,2,2);put(definitions+0x14,1,2);
    put(clusters+0x24,1,2);put(definitions+0x24,1,2);put(instances+0x34,0,2);put(instances+88+0x34,0,2);
    put(instances+0x56,1,2);
    for(auto section:{clusters,definitions}) {
        put(section+0x28+8,16);put(section+0x28+0x1e,1,1);put(section+0x28+0x20,section==clusters?0:2);put(section+0x50,d+0x12000);
    }
    const auto initial_abi=world.prepare_source_call();auto fp=arithmetic(initial_abi);
    auto services=source_draw_resource_services_tu15(world,fp);
    for(unsigned instance:{0u,1u}) {
        SourceCullDrawUnit unit;unit.words[1]=0xffffffffu;unit.words[3]=instance<<18;
        const auto selector=instance;world.write_source_call_input(0x4c6958u,std::as_bytes(std::span(unit.words)));
        auto original=world.prepare_source_call(std::array{selector});original.gp[0]=0;world.run_actual(0x44930,original);
        const auto section=original.gp[0];const auto metadata=services.geometry(0,selector,unit);
        check(metadata.classification==Bytes(m.view(section+0x14,2)).u16(0)&&metadata.subpart_count==Bytes(m.view(section+0x24,2)).u16(0),"Native live44930 metadata differs");
        const auto before=m.checkpoint();original=world.prepare_source_call(std::array{selector});original.gp[0]=0;original.gp[2]=1;
        world.run_actual(0x449d0,original);const auto expected=m.checkpoint();restore_bytes(world,before);fp=arithmetic(initial_abi);
        check(services.resident(0,selector,true,unit)==bool(original.gp[0]),"Native live449D0 admission differs");
        equal(expected,m.checkpoint());same_fp(original,fp);++factory_cases;
    }
    const auto h=buckets+0xc;put(h+8,16);put(h+0x1e,1,1);put(h+0x20,1);
    SourceBspAdmissionBsp geometry;geometry.clusters.resize(1);geometry.definitions.resize(1);geometry.instances.resize(2);
    geometry.clusters[0].bounds={-1,1,-1,1,3,5};
    geometry.instances[0].definition=geometry.instances[1].definition=0;geometry.instances[0].center=geometry.instances[1].center={0,0,4};
    geometry.instances[0].radius=1;geometry.instances[1].radius=0;
    std::vector<SourceBspDrawGeometry> prepared_clusters{{3,9,false}},prepared_definitions{{3,9,false}};
    std::vector<SourceBspDrawInstance> prepared_instances(2);
    SourceBspDrawWorld draw_world{geometry,prepared_clusters,prepared_definitions,prepared_instances,false};draw_world.services=&services;
    SourceBspAdmission admission;admission.clusters.push_back({0});admission.instances.push_back({0});admission.instances.push_back({1});
    SourceBspDrawState state;state.arena.used=1;SourceBspDrawContext context;context.mask_arena_address=0x71000000;
    context.camera.inverse_view_z={0,0,1,0};context.camera.viewport_height_metric=200;
    std::vector<unsigned> stamps;int observer;
    source_runtime_register_write_observer(m,&observer,[&](std::uint32_t at,std::size_t size) {
        for(unsigned i=0;i<3;++i)if(at==records+24*i+0x14&&size==4)stamps.push_back(i);
    });
    struct Unobserve {SourceRuntimeMemory& memory;const void* owner;~Unobserve(){source_runtime_unregister_write_observer(memory,owner);}} unobserve{m,&observer};
    source_bsp_collect_draw_units(state,admission,draw_world,context);
    check(stamps==std::vector<unsigned>{0,1,2},"Live BSP requests changed cluster / bucket / definition order or requested the zero-alpha instance");
    check(state.draw_count==2&&state.main_prefix==2&&(admission.instances[1].flags&1u),"Live admission lost accepted units or the alpha gate");
    check(state.units[0].words[3]>>29==2&&state.units[1].words[3]>>29==1&&state.units[1].words[0]>>29==1,
        "Live admission consumed prepared metadata or cached lighting");
    for(unsigned refused=0;refused<3;++refused) {
        for(unsigned n=0;n<3;++n) {
            const auto header=n==0?clusters+0x28u:n==1?h:definitions+0x28u;
            put(header+0x1e,n==refused?0:1,1);put(rows+12*n+2,n==refused?0:1,1);put(rows+12*n+3,255,1);
        }
        put(0x4751ac,1,1);put(0x4f2d78,0);stamps.clear();
        state={};state.arena.used=1;admission.instances[1].flags=0;
        source_bsp_collect_draw_units(state,admission,draw_world,context);
        check(stamps==std::vector<unsigned>{0,1,2},"Pressure changed the original cluster/bucket/definition resource order");
        check(word(0x4f2d78)==1,"A refused resource changed the original special-resource counter more than once");
        check(state.draw_count==(refused==1?2u:1u)&&state.main_prefix==state.draw_count,
            "Pressure dropped the wrong unit or skipped a later admission");
        if(refused==1)check(state.units[1].words[0]>>29==3,"Refused lightmap did not choose original dynamic classification3");
        check(admission.instances[1].flags&1u,"Pressure bypassed the original zero-alpha skip");
    }
    std::cout<<"PASS "<<lighting_cases<<" whole2CFE0 World bank/FP comparisons, "<<factory_cases<<
        " live44930/449D0 service cases and actual cluster/bucket/definition request order with zero-alpha skip, plus3 pressure/refusal histories\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
