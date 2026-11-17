#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_sphere_cull_45550.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_native_store_oracle_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace halo2 {
namespace {
struct Stats {
    std::atomic<std::uint64_t> calls{},frustum{},sphere{},native{},residual{};
    std::atomic<std::uint64_t> sphere_native{},sphere_reference{},sphere_residual{},sphere_comparisons{},sphere_mismatches{},
        sphere_queries{},sphere_grown{},sphere_parts{},sphere_already{},sphere_unbounded{},sphere_bsp{},sphere_no_object{},
        sphere_same_node{},sphere_no_nodes{},sphere_transforms{},sphere_hits{},sphere_misses{};
    std::atomic<std::uint64_t> site_native{},site_reference{},site_comparisons{},site_mismatched_calls{},site_mismatches{},
        site_fp_mismatches{},site_native_bytes{},site_original_bytes{};
    ~Stats() {
        const auto* enabled=std::getenv("HALO2_NATIVE_CULL_WALK_STATS");if(!enabled || !*enabled)return;
        std::cerr<<"Native44710 stats: calls="<<calls<<" frustum_calls="<<frustum<<" sphere_calls="<<sphere<<
            " native_entries="<<native<<" residual_entries="<<residual<<'\n';
        std::cerr<<"Sphere45550 stats: native="<<sphere_native<<" reference="<<sphere_reference<<" residual="<<sphere_residual<<
            " comparisons="<<sphere_comparisons<<" mismatches="<<sphere_mismatches<<" queries="<<sphere_queries<<
            " grown_arrays="<<sphere_grown<<" parts="<<sphere_parts<<" already="<<sphere_already<<" unbounded="<<sphere_unbounded<<
            " bsp="<<sphere_bsp<<" no_object="<<sphere_no_object<<" same_node="<<sphere_same_node<<" no_nodes="<<sphere_no_nodes<<
            " transforms="<<sphere_transforms<<" hits="<<sphere_hits<<" misses="<<sphere_misses<<'\n';
        std::cerr<<"Site44710 stats: native="<<site_native<<" reference="<<site_reference<<" comparisons="<<site_comparisons<<
            " mismatched_calls="<<site_mismatched_calls<<" mismatched_bytes="<<site_mismatches<<" fp_mismatches="<<site_fp_mismatches<<
            " native_bytes="<<site_native_bytes<<" original_bytes="<<site_original_bytes<<'\n';
    }
};
Stats& stats(){static Stats s;return s;}
bool enabled(const char* flag){const auto* p=std::getenv(flag);return p&&*p&&*p!='0';}
void invoke(PhysicsLiveWorldTu15& world,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,NativeTu15PhysicsFrameFp& arithmetic) {
    abi.x87_status=arithmetic.status;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
    world.run_actual(entry,abi);
    arithmetic={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
}
// [48C938]+14 for a grown 2E7C40 array, through the existing native
// 149E00 -> 233420 owners (45550's 456D6/45703 release calls).
constexpr std::uint32_t havok_allocator=0x48c938u,array_release=0x149e00u,pool_release=0x233420u;
void release_array(SourceRuntimeMemory& memory,std::uint32_t data,std::uint32_t bytes,NativeTu15PhysicsFrameFp& fp) {
    NativeTu15ArrayAllocatorServices heap;
    heap.release_owner=[&memory](const NativeTu15ArrayReleaseRequest& r,NativeTu15PhysicsFrameFp&) {
        NativeTu15PoolMemory pool(memory,r.receiver);
        if(r.owner!=pool_release || std::bit_cast<std::int32_t>(r.bytes)>0x2000 || !native_tu15_pool_release(pool,r.data,r.bytes))
            throw FormatError("Native45550 grown query array has no native pool release owner");
    };
    heap.default_heap=[](NativeTu15PhysicsFrameFp&)->std::uint32_t {
        throw FormatError("Native45550 grown query array requires the lazy default heap");
    };
    NativeTu15ArrayAllocatorMemory allocator(memory);
    native_tu15_route_array_release(allocator,data,bytes,0x12u,fp,heap);
}
void translated_sphere(PhysicsLiveWorldTu15& world,std::uint32_t unit,const std::array<std::uint32_t,3>& center,
    std::uint32_t radius,NativeTu15PhysicsFrameFp& arithmetic) {
    // Caller-owned arguments above ESP survive the actual 45550 frame.
    // prepare_source_call checks the full extent for nested callers too.
    auto abi=world.prepare_source_call(std::array{0u,radius,center[0],center[1],center[2]});
    const auto point=abi.gp[4]+12u;
    world.write_source_call_input(abi.gp[4]+4u,std::as_bytes(std::span(&point,1)));
    abi.gp[1]=unit;invoke(world,0x45550u,abi,arithmetic);
}
void native_sphere(PhysicsLiveWorldTu15& world,std::uint32_t unit,const std::array<std::uint32_t,3>& center,
    std::uint32_t radius,NativeTu15PhysicsFrameFp& arithmetic) {
    auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    NativeTu15SphereCullServices services;
    // 2E7C40 and its 2E6ED0 traversal belong to the projection executor;
    // until that owner is native they run here, in 45550's own frame shape:
    // {code, &sphere, &leaves, &ranges} and two 512-row inline arrays.
    services.query_2e7c40=[&](std::uint32_t code,const std::array<std::uint32_t,4>& sphere,
        NativeTu15SphereQueryRows& rows,NativeTu15PhysicsFrameFp& fp) {
        ++stats().sphere_queries;
        constexpr std::uint32_t capacity=512u,inline_flag=0x80000000u;
        const auto caller=world.prepare_source_call().gp[4];
        std::uint32_t first=4u;while((caller+4u+4u*first)&15u)++first;
        const auto leaves_at=first+4u,ranges_at=leaves_at+3u+capacity;
        std::vector<std::uint32_t> args(ranges_at+3u+capacity);
        const auto address=[&](std::uint32_t index){return caller+4u+4u*index;};
        args[0]=code;args[1]=address(first);args[2]=address(leaves_at);args[3]=address(ranges_at);
        std::copy(sphere.begin(),sphere.end(),args.begin()+first);
        args[leaves_at]=address(leaves_at+3u);args[leaves_at+2u]=inline_flag|capacity;
        args[ranges_at]=address(ranges_at+3u);args[ranges_at+2u]=inline_flag|capacity;
        auto abi=world.prepare_source_call(args);
        invoke(world,0x2e7c40u,abi,fp);
        const auto read_rows=[&](std::uint32_t header,std::vector<std::uint32_t>& out,bool pairs)->std::uint32_t {
            const auto data=word(header),count=word(header+4u);
            const auto rows_read=std::bit_cast<std::int32_t>(count)>0?count+(pairs&&(count&1u)):0u;
            out.resize(rows_read);
            for(std::uint32_t n=0;n<rows_read;++n)out[n]=word(data+4u*n);
            return std::bit_cast<std::int32_t>(count)>0?count:0u;
        };
        rows.range_count=read_rows(args[3],rows.ranges,true);
        read_rows(args[2],rows.leaves,false);
        for(const auto header:{args[2],args[3]}) {
            const auto stored=word(header+8u);
            if(std::bit_cast<std::int32_t>(stored)<0)continue;
            ++stats().sphere_grown;
            release_array(memory,word(header),(stored&0x7fffffffu)<<2u,fp);
        }
    };
    NativeTu15SphereCullCoverage seen;
    native_tu15_sphere_cull_45550(memory,unit,center,radius,arithmetic,services,&seen);
    auto& s=stats();
    s.sphere_parts+=seen.parts;s.sphere_already+=seen.already;s.sphere_unbounded+=seen.unbounded;s.sphere_bsp+=seen.bsp_parts;
    s.sphere_no_object+=seen.no_object;s.sphere_same_node+=seen.same_node;s.sphere_no_nodes+=seen.no_nodes;
    s.sphere_transforms+=seen.transforms;s.sphere_hits+=seen.hits;s.sphere_misses+=seen.misses;
}
// Refusals are decided before any effect: the parts branch needs masked FP
// exceptions and a grown query array needs the native allocator owners.
bool sphere_admitted(PhysicsLiveWorldTu15& world,const NativeTu15PhysicsFrameFp& fp) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) || (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))return false;
    const auto allocator=word(havok_allocator);
    return allocator && word(word(allocator)+0x14u)==array_release;
}
}
NativeTu15PhysicsFrameFp source_native_cull_walk_tu15(PhysicsLiveWorldTu15& world,const NativeTu15CullFrustumEntry& native_entry) {
    const auto initial=world.prepare_source_call();const auto controls=initial.floating_point.value();
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,initial.x87_status,controls.mxcsr};
    NativeTu15CullWalkServices services;
    services.frustum=[&](auto unit,auto entry,auto& arithmetic) {
        ++stats().frustum;
        if(native_entry && native_entry(unit,entry,arithmetic)){++stats().native;return;}
        ++stats().residual;
        auto abi=world.prepare_source_call(std::array{unit,entry});invoke(world,0x44e80u,abi,arithmetic);
    };
    static const bool sphere_reference=enabled("HALO2_REFERENCE_SPHERE_CULL");
    static const bool sphere_oracle=enabled("HALO2_SPHERE_CULL_ORACLE");
    services.sphere=[&](auto unit,const auto& center,auto radius,auto& arithmetic) {
        ++stats().sphere;
        if(sphere_reference) {++stats().sphere_reference;translated_sphere(world,unit,center,radius,arithmetic);return;}
        if(!sphere_admitted(world,arithmetic)) {++stats().sphere_residual;translated_sphere(world,unit,center,radius,arithmetic);return;}
        if(!sphere_oracle) {++stats().sphere_native;native_sphere(world,unit,center,radius,arithmetic);return;}
        auto native_fp=arithmetic;
        SourceNativeStoreOracleTu15 compare;
        compare.run(world.memory(),[&]{native_sphere(world,unit,center,radius,native_fp);},
            [&]{translated_sphere(world,unit,center,radius,arithmetic);});
        ++stats().sphere_comparisons;
        if(compare.mismatches || native_fp.control_word!=arithmetic.control_word || native_fp.status!=arithmetic.status ||
           native_fp.mxcsr!=arithmetic.mxcsr) {
            if(++stats().sphere_mismatches<=8) {
                std::cerr<<"Sphere45550 oracle mismatch unit="<<unit<<" bytes="<<compare.mismatches<<std::hex<<
                    " fp native="<<native_fp.control_word<<'/'<<native_fp.status<<'/'<<native_fp.mxcsr<<
                    " original="<<arithmetic.control_word<<'/'<<arithmetic.status<<'/'<<arithmetic.mxcsr;
                for(const auto& m:compare.first)std::cerr<<" ["<<m.address<<' '<<unsigned(m.native)<<'/'<<unsigned(m.original)<<']';
                std::cerr<<std::dec<<'\n';
            }
        }
    };
    native_tu15_cull_walk_44710(world.memory(),fp,services);
    ++stats().calls;
    return fp;
}
void source_cull_walk_site_tu15(PhysicsLiveWorldTu15& world,SourcePhysicsWorldRuntimeAbi& call) {
    static const bool reference=enabled("HALO2_REFERENCE_SECONDARY_CULL");
    static const bool oracle=enabled("HALO2_SECONDARY_CULL_ORACLE");
    if(reference) {++stats().site_reference;world.run_actual(0x44710u,call);return;}
    const auto publish=[&](const NativeTu15PhysicsFrameFp& fp) {
        call.x87_status=fp.status;call.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    };
    if(!oracle) {++stats().site_native;publish(source_native_cull_walk_tu15(world));return;}
    // Both runs start from the same fresh caller arithmetic state; the
    // original's completed state and stores persist.
    NativeTu15PhysicsFrameFp native_fp{};
    auto original=world.prepare_source_call();
    SourceNativeStoreOracleTu15 compare;
    compare.run(world.memory(),[&]{native_fp=source_native_cull_walk_tu15(world);},[&]{world.run_actual(0x44710u,original);});
    auto& s=stats();
    ++s.site_comparisons;s.site_native_bytes+=compare.native_bytes;s.site_original_bytes+=compare.original_bytes;
    const auto& controls=original.floating_point.value();
    const bool fp_differs=native_fp.control_word!=controls.x87_control_word||native_fp.status!=original.x87_status||
        native_fp.mxcsr!=controls.mxcsr;
    if(fp_differs)++s.site_fp_mismatches;
    if(compare.mismatches||fp_differs) {
        s.site_mismatches+=compare.mismatches;
        if(++s.site_mismatched_calls<=8) {
            std::cerr<<"Site44710 oracle mismatch: bytes="<<compare.mismatches<<std::hex<<
                " fp native="<<native_fp.control_word<<'/'<<native_fp.status<<'/'<<native_fp.mxcsr<<
                " original="<<controls.x87_control_word<<'/'<<original.x87_status<<'/'<<controls.mxcsr;
            for(const auto& m:compare.first)std::cerr<<" ["<<m.address<<' '<<unsigned(m.native)<<'/'<<unsigned(m.original)<<']';
            std::cerr<<std::dec<<'\n';
        }
    }
    call.x87_status=original.x87_status;call.floating_point=original.floating_point;
}
}
