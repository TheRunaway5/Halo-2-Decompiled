#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char* s){if(!v)throw FormatError(s);}
void equal(const SourceRuntimeMemory::Checkpoint& a,const SourceRuntimeMemory::Checkpoint& b) {
    check(a.regions.size()==b.regions.size(),"Native44710 changed World topology");
    for(std::size_t i=0;i<a.regions.size();++i) {
        if(a.regions[i].address==0x60000000u)continue; // distinct real caller stacks
        if(a.regions[i]!=b.regions[i])throw FormatError("Native44710 differs in complete World bank "+a.regions[i].owner);
    }
}
}
int main(int argc,char** argv)try {
    check(argc==3,"Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1])||!std::filesystem::is_regular_file(argv[2]))return 77;
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Native44710 requires owned scratch");
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    PhysicsLiveWorldTu15 world(cache,argv[1],std::filesystem::path(scratch)/"saves");auto& m=world.memory();
    constexpr std::uint32_t data=0x1000000u,pool=data,rows=data+0x800,tags=data+0x1000,model=data+0x2000,
        inverse=data+0x2800,sections=data+0x3000,body=data+0x4000,geometry=data+0x6000,subpart=data+0x6800,
        bound=data+0x7000,draw=data+0x8000,mask=data+0x9000,transform=data+0xa000,ctx=data+0x10000,
        collect=data+0x14000,units=0x4c6958u;
    check(m.find_unowned_range(data,data+0x20000u,0x20000u,4096)==data,"Native44710 fixture data owner overlaps");
    m.mount(data,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"native44710-declared-geometry");
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> saved;
    const auto save=[&](auto at,auto size){const auto bytes=m.view(at,size);saved.emplace_back(at,std::vector(bytes.begin(),bytes.end()));};
    for(auto at:{0x4ecbd0u,0x4f0414u,0x4ecc18u,0x4cd3a0u,0x4cd39cu,0x4cd398u})save(at,4);
    save(units,128);
    struct Restore {PhysicsLiveWorldTu15& world;decltype(saved)& entries;~Restore(){for(const auto& [at,bytes]:entries)world.write_source_call_input(at,bytes);world.memory().release(data);}} restore{world,saved};
    const auto put=[&](std::uint32_t at,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4> b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8));world.write_source_call_input(at,std::span(b).first(size));
    };
    const auto floats=[&](auto at,std::initializer_list<float> values){for(auto v:values){put(at,std::bit_cast<std::uint32_t>(v));at+=4;}};
    put(0x4ecbd0,pool);put(pool+0x44,rows);put(rows+8,body);put(0x4f0414,tags);put(tags+8,model);
    put(model+0x28,sections);put(model+0x4c,inverse);put(sections+0x34,geometry);
    put(body+0x114,52,2);put(body+0x116,0x200,2);
    for(auto at:{body+0x200,inverse+0x28,transform})floats(at,{1,1,0,0,0,1,0,0,0,1,0,0,0});
    put(geometry+8,1);put(geometry+0xc,subpart);put(geometry+0x14,bound);
    floats(bound,{0,0,-2,.5});put(draw,0);put(0x4cd3a0,ctx);put(ctx,collect);
    put(collect,1,2);put(collect+0xa70,1,2);put(collect+0xa7c,0,2);
    const auto frustum=collect+0x1974;floats(frustum+0x60,{-5,5,-5,5,-5,5});
    for(unsigned i=0;i<6;++i) {
        const unsigned axis=i/2;std::array<float,4> p{};p[axis]=i%2?-1.f:1.f;p[3]=5;
        for(unsigned k=0;k<4;++k)put(frustum+0x78+16*i+4*k,std::bit_cast<std::uint32_t>(p[k]));
    }
    unsigned compared{};
    for(bool sphere:{false,true})for(unsigned mode:{0u,1u})for(unsigned initial_mask:{0u,1u}) {
        const auto setup=[&] {
            put(0x4cd39c,mode?1:4);put(0x4cd398,4);put(ctx+0x2a78,mode);put(ctx+0x2a7c,sphere?1:0);
            floats(ctx+0x2a68,{.1f,.2f,-2,1});put(transform,std::bit_cast<std::uint32_t>(sphere?3.f:1.f));
            put(mask,initial_mask);put(draw,sphere?0xffffffffu:0);
            for(unsigned i=0;i<4;++i) {
                put(units+32*i,(i==0&&!sphere)?0x48:0x40);put(units+32*i+4,0);put(units+32*i+0x10,draw);
                put(units+32*i+0x14,mask);put(units+32*i+0x18,0);put(units+32*i+0x1c,sphere?transform:0);
            }
            put(ctx+0xa60,1); // original per-unit association path
        };
        setup();auto original=world.prepare_source_call();world.run_actual(0x44710,original);const auto expected=m.checkpoint();
        const auto first=mode?1u:0u;
        check(Bytes(m.view(units+32*first,4)).u32(0)==((first==0&&!sphere?0x48u:0x40u)|(initial_mask?0u:1u)),"Genuine original44710 did not exercise its nonempty shared mask chronology");
        setup();const auto actual_fp=source_native_cull_walk_tu15(world);equal(expected,m.checkpoint());
        check(actual_fp.control_word==original.floating_point->x87_control_word &&
            actual_fp.mxcsr==original.floating_point->mxcsr && actual_fp.status==original.x87_status,
            "Native44710 changed World FP controls/status");++compared;
    }
    // Real native BSP entry + real residual posed-model entries, both orders,
    // with a shared mask and a separately admitted model. No service outcome
    // is supplied; the whole literal44710 computes the independent gold bank.
    constexpr auto bsp_root=data+0xb000u,bsp_section=data+0xc000u,
        bsp_geometry=data+0xd000u,bsp_tree=data+0xe000u;
    put(0x4ecc18,bsp_root);put(bsp_root+0xa0,bsp_section);put(bsp_section+0x50,bsp_geometry);
    put(bsp_geometry+8,1);put(bsp_geometry+0x2c,bsp_tree);put(bsp_geometry+0x30,1);
    put(bsp_tree+12,std::bit_cast<std::uint32_t>(65536.f));put(bsp_tree+48,0x30,2);
    SourceBspRangeGeometry bsp_native;bsp_native.subpart_count=1;bsp_native.subparts.resize(1);
    bsp_native.accelerator_count=1;bsp_native.accelerator.assign(m.view(bsp_tree,49).begin(),m.view(bsp_tree,49).end());
    SourcePortalVisibilitySet visibility;visibility.groups.resize(1);visibility.clusters.resize(1);visibility.frustums.resize(1);
    visibility.clusters[0].counts[0]=1;
    std::memcpy(visibility.frustums.data(),m.view(frustum,sizeof(visibility.frustums[0])).data(),sizeof(visibility.frustums[0]));
    for(bool bsp_first:{false,true})for(unsigned initial_mask:{0u,1u}) {
        const unsigned bsp_index=bsp_first?0:1,model_index=bsp_first?1:0;
        const auto setup=[&] {
            put(0x4cd39c,3);put(0x4cd398,3);put(ctx+0x2a78,0);put(ctx+0x2a7c,0);put(draw,0);
            put(mask,initial_mask);put(mask+4,0);
            for(unsigned i=0;i<3;++i) {
                put(units+32*i,0x40);put(units+32*i+4,i==bsp_index?0xffffffffu:0);
                put(units+32*i+0x10,draw);put(units+32*i+0x14,i==2?mask+4:mask);
                put(units+32*i+0x18,0);put(units+32*i+0x1c,0);
            }
        };
        setup();auto original=world.prepare_source_call();world.run_actual(0x44710,original);const auto expected=m.checkpoint();
        check(Bytes(m.view(units+32*model_index,4)).u32(0)==(0x40u|(!bsp_first&&!initial_mask?1u:0u)),
            "Original mixed cull did not expose shared-mask source order");
        check(Bytes(m.view(units+64,4)).u32(0)==0x41u,"Original mixed cull did not admit its independent model");
        setup();unsigned native_calls{},residual_calls{};
        const auto actual=source_native_cull_walk_tu15(world,[&](auto index,auto entry,auto& arithmetic) {
            if(index!=bsp_index){++residual_calls;return false;}
            SourceCullDrawUnit unit;std::memcpy(&unit,m.view(units+32*index,sizeof(unit)).data(),sizeof(unit));
            SourceBspRangeArena arena;std::memcpy(arena.words.data(),m.view(mask,sizeof(arena.words)).data(),sizeof(arena.words));
            {detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
                source_bsp_frustum_cull_entry(unit,{&bsp_native,true,{}},arena,{0,0,0,0,mask},visibility,entry);}
            put(units+32*index,unit.words[0]);
            world.write_source_call_input(mask,std::as_bytes(std::span(arena.words)));++native_calls;return true;
        });
        equal(expected,m.checkpoint());
        check(native_calls==1&&residual_calls==2&&actual.status==original.x87_status&&actual.mxcsr==original.floating_point->mxcsr,
            "Mixed native/residual cull lost service order or FP");++compared;
    }
    // Independent physical2E79A0 row: scale3, Y bounds3..7, terminal1,
    // no planes. FSTP rounds upward: x87 PE|C1, SSE remains untouched.
    constexpr auto query_tree=data+0x18000u,query_planes=data+0x18100u,
        query_direct=data+0x18200u,query_ranges=data+0x19200u;
    put(query_tree+12,std::bit_cast<std::uint32_t>(3.f));
    const std::array<std::byte,4> query_code{std::byte{0x27},std::byte{3},std::byte{7},std::byte{0x31}};
    world.write_source_call_input(query_tree+48,query_code);
    for(auto header:{query_direct,query_ranges}){put(header,header+16);put(header+8,0x80000080u);}
    auto query=world.prepare_source_call(std::array{query_tree,query_planes,query_direct,query_ranges});
    query.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};query.x87_status=0;
    world.run_actual(0x2e79a0u,query);
    check(query.x87_status==0x220 && query.floating_point->mxcsr==0x1f80,
        "Translated accelerator does not retain physical2E79A0 PE|C1");
    std::cout<<"PASS "<<compared<<" complete non-stack World-bank comparisons with actual44710/44E80/45550, genuine pose/frustum/sphere predicates and aliased masks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
