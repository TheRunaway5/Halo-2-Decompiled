#include "halo2/native_tu15_draw_units.hpp"
#include "halo2/effect_light_receiver.hpp"
#include "halo2/source_object_admission.hpp"
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
#ifdef _WIN32
    _putenv_s("HALO2_REFERENCE_BANK_DRAW_UNITS","");
    _putenv_s("HALO2_REFERENCE_LIGHT_PARENT_BIPED","");
#else
    unsetenv("HALO2_REFERENCE_BANK_DRAW_UNITS");
    unsetenv("HALO2_REFERENCE_LIGHT_PARENT_BIPED");
#endif
    check(argc==3,"Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Draw-unit World test requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr auto d=0x1000000u,bsp=d,clusters=d+0x1000u,instances=d+0x2000u,definitions=d+0x3000u,
        queue=d+0x4000u,instance_queue=d+0x4100u,flags=d+0x4200u,ids=d+0x4300u,instance_flags=d+0x4400u,
        instance_ids=d+0x4500u,associations=d+0x4600u,physical=d+0x5000u,pool=d+0x6000u,records=d+0x7000u;
    check(m.find_unowned_range(d,d+0x10000u,0x10000u,4096)==d,"Draw-unit fixture overlaps another owner");
    m.mount(d,std::vector<std::byte>(0x10000),SourceRuntimeMemory::Access::ReadWrite,"native-bank-draw-declared-inputs");
    const auto initial=m.checkpoint();
    struct Restore {PhysicsLiveWorldTu15& world;const SourceRuntimeMemory::Checkpoint& initial;
        ~Restore(){restore_bytes(world,initial);world.memory().release(0x1000000u);}} restore{world,initial};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
        world.write_source_call_input(at,std::span(bytes).first(size));
    };
    const auto f=[&](auto at,float v){put(at,std::bit_cast<std::uint32_t>(v));};
    put(0x4ecc18,bsp);put(0x4ecc14,0);put(bsp+0xa0,clusters);put(bsp+0x144,instances);put(bsp+0x13c,definitions);
    put(instances+0x34,0,2);f(instances+0x44,4);f(instances+0x48,1);
    for(auto section:{clusters,definitions}) {
        put(section+0x14,2,2);put(section+0x24,40,2);put(section+0x50,d+0x8000);
        put(section+0x28+0x1e,1,1);put(section+0x28+0x20,1);
    }
    for(unsigned i=0;i<6;++i)f(clusters+0x54+4*i,std::array{-1.f,1.f,-1.f,1.f,3.f,5.f}[i]);
    put(0x4f2d6c,physical);put(physical+0x64,pool);put(pool+0x44,records);put(physical+0x38,0xdeadbeef);
    for(auto q:{queue,instance_queue}){put(q+4,2,2);put(q+0x14,associations);}
    put(queue+8,flags);put(queue+0xc,ids);put(instance_queue+8,instance_flags);put(instance_queue+0xc,instance_ids);
    put(flags+2,0x800,2);put(instance_flags+2,0x800,2);put(associations,0x1234,2);put(associations+2,0x5678,2);
    put(0x4c6710,0);f(0x4c6640,0);f(0x4c664c,0);f(0x4c6658,1);f(0x4c6664,0);f(0x4c66f0,50);
    const auto base=m.checkpoint();unsigned cases{};
    for(auto context:{0x556e18u,0x5598f8u})for(unsigned profile=0;profile<6;++profile) {
        restore_bytes(world,base);put(0x4cd3a0,context);put(context+0xc,queue);put(context+0x18,instance_queue);
        put(context+0x2a78,context==0x556e18u?0:1);put(0x4cd398,7);put(0x4cd39c,7);
        put(context+0xa5c,profile==2?511:0,2);
        if(profile==1){put(clusters+0x50,0);put(definitions+0x50,0);}
        if(profile==3){put(clusters+0x24,16385,2);put(definitions+0x24,16385,2);}
        if(profile==4)f(0x4c66f0,.01f);
        if(profile==5){put(queue+4,0xffff,2);put(instance_queue+4,0xffff,2);}
        const auto before=m.checkpoint();NativeTu15PhysicsFrameFp fp{0x27f,0x4500,0x1f80};
        const auto invoke=[&](auto entry,auto& abi) {
            abi.x87_status=fp.status;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
            world.run_actual(entry,abi);fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        };
        auto call=world.prepare_source_call();invoke(0x2cb10u,call);
        call=world.prepare_source_call();call.gp[3]=context;invoke(0x135ea0u,call);
        call=world.prepare_source_call(std::array{context});invoke(0x136030u,call);
        const auto expected=m.checkpoint();const auto expected_fp=fp;restore_bytes(world,before);fp={0x27f,0x4500,0x1f80};
        check(world.collect_render_bsp_units_tu15(fp)==context,"Native bank collector pinned the wrong context");
        equal(expected,m.checkpoint());check(fp.control_word==expected_fp.control_word&&fp.status==expected_fp.status&&fp.mxcsr==expected_fp.mxcsr,"Native bank collector World FP differs");++cases;
    }
    std::cout<<"PASS "<<cases<<" World bank BSP admission histories, full persistent banks and FP\n";
    restore_bytes(world,base);
    constexpr auto tags=d+0xa000u,model=d+0xb000u,sections=d+0xc000u,center=d+0xd000u;
    constexpr auto section=sections+92u*5u,rectangle=0x4cdd5cu+36u*5u;
    put(0x4f0414,tags);put(tags+24,model);put(model+0x28,sections);
    put(section+4u+0x14u,2,2);put(section+4u+0x24u,0,2);put(section+0x34u,d+0x8000u);
    put(0x4cd3a0,0x5598f8u);put(0x5598f8u+0x2a78u,0);
    const auto camera=0x4c65c0u,projection=0x4c6634u;
    for(unsigned i=0;i<3;++i)f(camera+4u*i,std::array{1.f,2.f,3.f}[i]);
    for(unsigned i=0;i<4;++i)put(camera+0x30u+2u*i,std::array{0u,0u,720u,1280u}[i],2);
    f(projection,1);for(unsigned i=0;i<9;++i)f(projection+4u+4u*i,i%4u==0?1.f:0.f);
    f(projection+0x78u,.9f);f(projection+0x8cu,-1.6f);f(projection+0x98u,.05f);f(projection+0x9cu,-.02f);
    for(unsigned i=0;i<3;++i)f(center+4u*i,std::array{2.f,2.5f,-7.f}[i]);
    // Required original reference: Opus's physical whole2CBD0 row44, also
    // re-run by native_tu15_sphere_rectangle_cases.py as rectangle-parent-reached.
    constexpr std::array<std::uint32_t,4> bounded{0x44280000u,0x444483ceu,0x43befb57u,0x43854ec3u};
    unsigned rectangle_cases{};
    for(unsigned profile=0;profile<3;++profile) {
        put(0x4cd398,7);put(0x4cd39c,3);put(0x4cdd48,0);
        for(unsigned i=0;i<8;++i)put(0x4c6958u+32u*7u+4u*i,0xa5a5a5a5u);
        for(unsigned i=0;i<4;++i)put(rectangle+4u*i,0x5a5a0000u+i);
        if(profile==1)f(center+8u,13.f);else f(center+8u,-7.f);
        std::array<std::uint32_t,21> args{2,0xabcd0001u,0x11112222u,7,0,0x1234,0xc8,0x22223333u,0x77,
            std::bit_cast<std::uint32_t>(3.5f),5,0xabcd,std::uint32_t(profile==2),0,1,
            std::bit_cast<std::uint32_t>(.5f),std::bit_cast<std::uint32_t>(.25f),std::bit_cast<std::uint32_t>(12.5f),9,center,
            std::bit_cast<std::uint32_t>(1.f)};
        auto call=world.prepare_source_call(args);const auto sp=call.gp[4];call.gp[0]=5;call.gp[1]=3;call.gp[2]=0x12;
        call.x87_status=0;call.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        std::vector<SourcePhysicsContactRecordStore> stores;
        world.run_actual(0x2cbd0u,call,[&](const auto& event){if(event.instruction>=0x30270u&&event.instruction<=0x306eau)stores.push_back(event);});
        check(std::uint16_t(call.gp[0])==7&&call.gp[4]==sp+0x58u,"Translated2CBD0 lost its real native30270 return frame");
        check(call.x87_status==(profile?0:0x20)&&call.floating_point->x87_control_word==0x27f&&call.floating_point->mxcsr==0x1fa0,
            "World2CD49 bridge differs from original rectangle FP");
        for(unsigned i=0;i<4;++i) {
            const auto expected=profile==0?bounded[i]:profile==1?(i%2u?0xff7fffffu:0x7f7fffffu):(i%2u?0x7f7fffffu:0xff7fffffu);
            check(Bytes(m.view(rectangle+4u*i,4)).u32(0)==expected,"World2CD49 rectangle differs from physical whole2CBD0");
        }
        check(stores.size()==(profile==2?0u:4u),"Native30270 lost original store observation");
        for(unsigned i=0;i<stores.size();++i) {
            check(stores[i].address==rectangle+4u*i&&stores[i].before.size()==4&&stores[i].written.size()==4,"Native30270 observer lost exact store bytes");
            const auto pc=profile==0?std::array{0x304e4u,0x3055cu,0x3064bu,0x30697u}[i]:std::array{0x306d2u,0x306d6u,0x306dbu,0x306e0u}[i];
            check(stores[i].instruction==pc,"Native30270 observer changed its original store PC");
        }
        ++rectangle_cases;
    }
    std::cout<<"PASS "<<rectangle_cases<<" World translated model2CBD0 histories through native30270, original rectangle bytes/FP/store PCs\n";
    restore_bytes(world,base);
    constexpr auto parent_pool=d+0xa000u,headers=d+0xb000u,bodies=d+0xc000u,parent_queue=d+0xd000u,
        parent_definition=d+0xd100u,shadow_byte=d+0xd104u,parent_flags=d+0xe000u,parent_objects=d+0xe100u,
        parent_values=d+0xe200u,parent_associations=d+0xe300u,context=0x5598f8u;
    put(0x4ecbd0u,parent_pool);put(parent_pool+0x44u,headers);put(context+0x14u,parent_queue);
    for(unsigned index=1;index<=3;++index) {
        const auto body=bodies+(index-1u)*256u;put(headers+12u*index+8u,body);
        put(body+0xaau,index==2?0u:index==1?2u:1u,1);
        put(body+0x14u,index==3?0xffffffffu:0x80010000u+index+1u);
    }
    put(parent_queue,5);put(parent_queue+8u,parent_flags);put(parent_queue+0xcu,parent_objects);
    put(parent_queue+0x10u,parent_values);put(parent_queue+0x14u,parent_associations);
    const auto parent_base=m.checkpoint();unsigned parent_cases{};
    for(unsigned profile=0;profile<7;++profile)for(unsigned shadow:{0u,255u}) {
        restore_bytes(world,parent_base);put(parent_definition,profile==4?0u:0x400000u);put(shadow_byte,shadow,1);
        put(parent_queue+4u,profile==5?4u:profile==6?1u:0u,2);
        if(profile==6){put(parent_objects,0x80010002u);put(parent_flags,0x4400,2);put(parent_values,0xabcdef);put(parent_associations,6,2);}
        const auto owner=profile==3?0xffffffffu:profile==1?0x80010002u:profile==2?0x80010003u:0x80010001u;
        const auto expected_object=profile==2?0x80010003u:0x80010002u;
        if(owner!=0xffffffffu)check(world.render_light_parent_biped(owner)==expected_object,"Light parent walk used the root instead of the nearest biped");
        const auto before=m.checkpoint();
        if(profile!=4&&owner!=0xffffffffu) {
            auto call=world.prepare_source_call(std::array{expected_object,shadow?0x3000u:0x1000u,0u,0xffffffffu});
            call.gp[0]=profile==2?0xffffffffu:bodies+256u;call.gp[1]=parent_queue;world.run_actual(0x166b60u,call);
        }
        const auto expected=m.checkpoint();restore_bytes(world,before);
        SourceObjectAdmission receivers;
        const auto original_count=Bytes(m.view(parent_queue+4u,2)).u16(0);
        for(unsigned n=0;n<original_count;++n)receivers.objects.push_back({Bytes(m.view(parent_objects+4u*n,4)).u32(0),
            Bytes(m.view(parent_flags+2u*n,2)).u16(0),Bytes(m.view(parent_associations+2u*n,2)).u16(0),Bytes(m.view(parent_values+4u*n,4)).u32(0)});
        if(!shadow)source_effect_light_append_parent_biped(receivers,{profile==4?0u:0x400000u,0x400u,5,
            profile==3?0x80010001u:owner,std::int16_t(profile==3?-1:0),0},
            [&](auto id){return world.render_light_parent_biped(id);});
        world.append_light_parent_biped_tu15(parent_definition,shadow_byte,owner,context);equal(expected,m.checkpoint());++parent_cases;
        if(!shadow) {
            check(receivers.objects.size()==Bytes(m.view(parent_queue+4u,2)).u16(0),"Sphere parent-biped append changed the original reserved-slot rule");
            for(unsigned n=0;n<receivers.objects.size();++n) {
                const auto& row=receivers.objects[n];check(row.id==Bytes(m.view(parent_objects+4u*n,4)).u32(0)&&
                    row.flags==Bytes(m.view(parent_flags+2u*n,2)).u16(0)&&row.other==Bytes(m.view(parent_values+4u*n,4)).u32(0)&&
                    row.association==Bytes(m.view(parent_associations+2u*n,2)).u16(0),"Sphere parent-biped rows differ from genuine166B60");
            }
        }
    }
    std::cout<<"PASS "<<parent_cases<<" World native31C10 histories against genuine166B60, nearest-biped/none/refusal/shadow cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
