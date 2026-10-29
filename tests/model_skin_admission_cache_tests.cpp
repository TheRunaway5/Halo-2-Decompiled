#include "halo2/model_skin_submission.hpp"
#include "halo2/model_skin_gpu_admission.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include "rendering/models/model_frame_pose_cache.hpp"
#include <atomic>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string_view>
#include <thread>
#include <type_traits>

using namespace halo2;
static_assert(std::is_aggregate_v<RenderModel>);
namespace {
struct AllocationTrace {
    std::size_t bytes{},allocations{},releases{};
    void *summary{};bool enabled{};
};
thread_local AllocationTrace allocation_trace;
void release_allocation(void *pointer)noexcept {
    if(pointer && pointer==allocation_trace.summary) {
        ++allocation_trace.releases;allocation_trace.summary=nullptr;
    }
    std::free(pointer);
}
}
void *operator new(std::size_t bytes) {
    if(auto *pointer=std::malloc(bytes?bytes:1)) {
        if(allocation_trace.enabled && bytes==allocation_trace.bytes) {
            ++allocation_trace.allocations;allocation_trace.summary=pointer;
        }
        return pointer;
    }
    throw std::bad_alloc();
}
void *operator new[](std::size_t bytes) {return ::operator new(bytes);}
void operator delete(void *pointer)noexcept {release_allocation(pointer);}
void operator delete[](void *pointer)noexcept {release_allocation(pointer);}
void operator delete(void *pointer,std::size_t)noexcept {release_allocation(pointer);}
void operator delete[](void *pointer,std::size_t)noexcept {release_allocation(pointer);}
namespace {
void need(bool value,const char *message) {if(!value)throw FormatError(message);}
struct Fp {
    std::fenv_t env{};unsigned mxcsr{};int error{};
    static Fp get() {Fp out;out.error=errno;std::fegetenv(&out.env);out.mxcsr=_mm_getcsr();return out;}
    void restore()const {std::fesetenv(&env);_mm_setcsr(mxcsr);errno=error;}
    bool same(const Fp &other)const {
        return mxcsr==other.mxcsr && error==other.error && !std::memcmp(&env,&other.env,sizeof(env));
    }
};
struct RestoreFp {Fp original=Fp::get();~RestoreFp(){original.restore();}};
void admitted_host() {_mm_setcsr(0x1fa0u);errno=ERANGE;}
RenderModel fixture() {
    RenderModel model;model.checksum=0x12345678u;model.bones.resize(1);
    model.bones[0].inverse={1,0,0,0,1,0,0,0,1,0,0,0};
    model.mesh.vertices.resize(3);model.skin.resize(3);
    for(auto &vertex:model.mesh.vertices) {
        vertex.position={.5f,.25f,.125f};vertex.normal={1,0,0};
        vertex.tangent={0,1,0};vertex.binormal={0,0,1};
    }
    return model;
}
void check_pose(const RenderModel &model,bool expected_admission) {
    const std::array nodes{Transform{}};
    admitted_host();const auto palette=model_skin_world_palette(model,nodes);
    need(model_skin_gpu_arithmetic_admitted(model.mesh.vertices,model.skin,palette)==expected_admission,
        "Fixture did not reach the intended full-predicate admission domain");
    admitted_host();const auto initial=Fp::get();
    const auto expected=skin_model_world_nodes(model,nodes);const auto expected_effects=Fp::get();
    initial.restore();auto actual=model_skin_world_pose(model,nodes,true);const auto actual_effects=Fp::get();
    need(actual.gpu==expected_admission,"Owned summary disagrees with the full arithmetic predicate");
    need(expected_effects.same(actual_effects),"Cached pose changed complete SSE/x87/errno effects");
    if(actual.gpu)skin_model_vertices(actual.vertices,model.skin,actual.palette,true);
    need(actual.vertices.size()==expected.size() && !std::memcmp(actual.vertices.data(),expected.data(),expected.size()*sizeof(Vertex)),
        "Cached pose changed complete original vertex output");
}
void lazy_summary_allocation() {
    constexpr std::size_t count=4096;
    allocation_trace={count*sizeof(ModelSkinAdmissionStatic::VertexSummary)};
    std::weak_ptr<const ModelSkinAdmissionCache> final_summary;
    {
        auto model=fixture();
        const auto vertex=model.mesh.vertices.front();
        model.mesh.vertices.resize(count,vertex);model.skin.resize(count);
        const auto pose=[&] {
            allocation_trace.enabled=true;
            try {check_pose(model,true);}catch(...){allocation_trace.enabled=false;throw;}
            allocation_trace.enabled=false;
        };
        pose();
        std::weak_ptr<const ModelSkinAdmissionCache> pending=model.skin_admission_cache.load();
        need(!pending.expired(),"First model use did not retain a small pending key");
        need(allocation_trace.allocations==0,"Single-use model allocated a per-vertex admission summary");
        pose();
        std::weak_ptr<const ModelSkinAdmissionCache> ready=model.skin_admission_cache.load();
        need(pending.expired(),"Second model use did not retire the pending key");
        need(allocation_trace.allocations==1,"Second model use did not build exactly one per-vertex summary");
        pose();
        need(ready.lock()==model.skin_admission_cache.load() && allocation_trace.allocations==1,
            "Warm model use rebuilt its immutable per-vertex summary");
        // An independently replaced immutable bank must become cold again.
        auto prior=std::move(model.mesh.vertices);model.mesh.vertices=prior;
        need(prior.data()!=model.mesh.vertices.data(),"Bank-change fixture reused its preceding storage");
        pose();
        need(ready.expired() && allocation_trace.allocations==1 && allocation_trace.releases==1,
            "Changed bank retained a stale summary or allocated one on its first use");
        pose();
        need(allocation_trace.allocations==2,"Changed bank did not build its own summary on second use");
        pose();
        need(allocation_trace.allocations==2,"Changed bank rebuilt its warm summary");
        final_summary=model.skin_admission_cache.load();
    }
    need(final_summary.expired() && allocation_trace.releases==2,
        "Model destruction retained per-vertex summary storage");
    std::cout<<"PASS lazy summary: cold0/warm1/reuse0 allocations, bank replacement cold reset, complete summary release\n";
    allocation_trace={};
}
void reference_cache_mode() {
    constexpr std::size_t count=4096;
    allocation_trace={count*sizeof(ModelSkinAdmissionStatic::VertexSummary)};
    auto model=fixture();const auto vertex=model.mesh.vertices.front();
    model.mesh.vertices.resize(count,vertex);model.skin.resize(count);
    for(unsigned use=0;use<3;++use) {
        allocation_trace.enabled=true;
        try {check_pose(model,true);}catch(...){allocation_trace.enabled=false;throw;}
        allocation_trace.enabled=false;
        need(!model.skin_admission_cache.load() && allocation_trace.allocations==0,
            "Reference cache switch retained a pending key or static summary");
    }
    allocation_trace={};
    std::cout<<"PASS reference cache mode: three full-predicate poses, no pending/static cache storage\n";
}
void reconstructed_owner() {
    alignas(RenderModel) std::byte storage[sizeof(RenderModel)];
    auto *model=std::construct_at(reinterpret_cast<RenderModel*>(storage),fixture());
    check_pose(*model,true);
    std::weak_ptr<const ModelSkinAdmissionCache> old_summary=model->skin_admission_cache.load();
    need(!old_summary.expired(),"Opt-in pose did not publish its owned summary");
    auto vertices=std::move(model->mesh.vertices);auto bindings=std::move(model->skin);
    const auto *vertex_address=vertices.data();const auto *binding_address=bindings.data();
    const auto checksum=model->checksum;const auto count=vertices.size();
    std::destroy_at(model);
    need(old_summary.expired(),"Destroyed RenderModel retained its admission summary");
    model=std::construct_at(reinterpret_cast<RenderModel*>(storage),fixture());
    model->mesh.vertices=std::move(vertices);model->skin=std::move(bindings);
    need(!model->skin_admission_cache.load(),"Fresh stack temporary inherited another lifetime's summary");
    need(model->checksum==checksum && model->mesh.vertices.size()==count &&
         model->mesh.vertices.data()==vertex_address && model->skin.data()==binding_address,
         "Reconstruction did not repeat every old raw-address cache key");
    model->mesh.vertices.front().position.x=std::bit_cast<float>(0x4a800000u);
    check_pose(*model,false);
    std::weak_ptr<const ModelSkinAdmissionCache> next_summary=model->skin_admission_cache.load();
    std::destroy_at(model);
    need(next_summary.expired(),"Reconstructed RenderModel retained its admission summary");
    std::cout<<"PASS same-address/checksum/count/bank reconstruction, changed admission domain and summary retirement\n";
}
void copied_owner() {
    auto original=fixture();check_pose(original,true);
    auto copy=original;
    const auto original_summary=original.skin_admission_cache.load();
    need(!copy.skin_admission_cache.load(),"Copied model inherited its source owner's cache slot");
    copy.mesh.vertices.front().position.x=std::bit_cast<float>(0x4a800000u);
    check_pose(copy,false);
    need(copy.skin_admission_cache.load()!=original_summary,
        "Copied model failed to rebind its independently owned bank");
    check_pose(original,true);
    std::cout<<"PASS copied banks rebind independently and preserve the original owner\n";
}
void concurrent_cold_readers() {
    auto model=fixture();std::atomic<unsigned> failures{};
    std::array<std::thread,8> readers;
    for(auto &reader:readers)reader=std::thread([&] {
        try {for(unsigned i=0;i<32;++i)check_pose(model,true);}catch(...){++failures;}
    });
    for(auto &reader:readers)reader.join();
    need(failures==0,"Concurrent immutable model readers lost admission, output or FP ownership");
    need(bool(model.skin_admission_cache.load()),"Cold readers did not publish an owned summary");
    std::cout<<"PASS eight concurrent cold/warm readers, 256 actual pose and FP comparisons\n";
}
void concurrent_copies() {
    const auto original=fixture();std::atomic<unsigned> failures{};
    std::thread poses([&] {
        try {for(unsigned i=0;i<128;++i){original.skin_admission_cache.publish({});check_pose(original,true);}}
        catch(...){++failures;}
    });
    std::thread copies([&] {
        try {for(unsigned i=0;i<128;++i) {
            auto copy=original;
            need(!copy.skin_admission_cache.load(),"Concurrent copy read another owner's mutable cache slot");
            check_pose(copy,true);
        }}catch(...){++failures;}
    });
    poses.join();copies.join();need(failures==0,"Concurrent copies and cache publication lost owner isolation");
    std::cout<<"PASS 128 immutable model copies concurrent with cold cache publication\n";
}
void factory_pose_cache() {
    RestoreFp saved;const auto model=fixture();
    std::array nodes{Transform{}};nodes[0].position={.13f,.27f,.41f};nodes[0].scale=1.1f;
    unsigned cases{},hits{},rebuilds{},gpu{};
    const auto equal=[&](const ModelSkinWorldPose &expected,const ModelSkinWorldPoseView &actual) {
        need(actual.gpu==expected.gpu && actual.vertices.size()==expected.vertices.size() &&
            actual.palette.size()==expected.palette.size(),"Factory cache changed CPU/GPU mode or complete bank extent");
        need(!std::memcmp(actual.vertices.data(),expected.vertices.data(),actual.vertices.size_bytes()) &&
             !std::memcmp(actual.palette.data(),expected.palette.data(),actual.palette.size_bytes()),
             "Factory cache changed complete vertex/palette bytes");
    };
    // Every cache request gets a fresh factory oracle from that request's own
    // incoming state. A previous call's terminal state may change admission.
    const auto checked=[&](const RenderModel &owner,std::span<const Transform> world,
                           bool allow_gpu,auto &&invoke) {
        const auto before=Fp::get();
        const auto expected=model_skin_world_pose(owner,world,allow_gpu);
        const auto expected_effects=Fp::get();
        before.restore();const auto actual=invoke();const auto actual_effects=Fp::get();
        need(expected_effects.same(actual_effects),"Factory cache changed fresh-call FP/errno effects");
        equal(expected,actual.pose);return actual;
    };
    // Exact identity palette does not set PE. CPU deformation does, so the
    // next identical request must switch from CPU output to deferred GPU data.
    auto precision_model=fixture();precision_model.mesh.vertices.resize(1);precision_model.skin.resize(1);
    precision_model.mesh.vertices[0].position={.7f,.25f,.125f};precision_model.skin[0].weights={.3f,0,0,0};
    const std::array identity_nodes{Transform{}};
    for(unsigned route=0;route<3;++route) {
        ModelWorldPoseCache cache;
        saved.original.restore();need(std::fesetround(FE_TONEAREST)==0,"Set precision transition rounding");
        std::feclearexcept(FE_ALL_EXCEPT);_mm_setcsr(0x1f80u);errno=EDOM;
        const auto invoke=[&] {
            return route==0?cache.update_factory(precision_model,identity_nodes,true):
                cache.update_deferred(precision_model,identity_nodes,route==1);
        };
        const auto cold=checked(precision_model,identity_nodes,true,invoke);
        need(!cold.reused&&!cold.pose.gpu&&_mm_getcsr()==0x1fa0u,
            "Precision fixture did not reach clear-PE CPU then set-PE state");
        const auto switched=checked(precision_model,identity_nodes,true,invoke);
        need(!switched.reused&&switched.pose.gpu,
            "Factory/deferred cache reused CPU output after incoming precision admitted GPU");
        const auto warm=checked(precision_model,identity_nodes,true,invoke);
        need(warm.reused&&warm.pose.gpu,"Precision-key GPU result did not become reusable");
    }
    for(bool allow_gpu:{false,true})for(int rounding:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})
    for(unsigned denorm:{0u,0x40u,0x8000u,0x8040u})for(unsigned flags:{0u,0x20u,0x3fu}) {
        saved.original.restore();need(std::fesetround(rounding)==0,"Set factory-cache rounding mode");
        _mm_setcsr((_mm_getcsr()&~0x807fu)|denorm|flags|0x1f80u);errno=EDOM;
        const auto initial=Fp::get();ModelWorldPoseCache cache;
        const auto invoke=[&]{return cache.update_factory(model,nodes,allow_gpu);};
        const auto cold=checked(model,nodes,allow_gpu,invoke);
        need(!cold.reused,"Cold factory result was marked reused");
        initial.restore();const auto strict=checked(model,nodes,allow_gpu,invoke);
        if(strict.reused)++hits;else ++rebuilds;
        const auto repeated=checked(model,nodes,allow_gpu,invoke);
        if(repeated.reused)++hits;else ++rebuilds;
        const auto steady=checked(model,nodes,allow_gpu,invoke);
        need(steady.reused,"Factory cache failed an effect-complete steady repeat");
        ++cases;if(steady.pose.gpu)++gpu;
    }
    need(cases&&hits&&rebuilds&&gpu,"Factory cache omitted strict hit/rebuild/GPU coverage");
    admitted_host();ModelFramePoseCache frame;
    for(unsigned view=0;view<4;++view)
        checked(model,nodes,false,[&]{return frame.update_factory(0x10001,2,model,nodes,false);});
    need(frame.builds()==1&&frame.reuses()==3,"Factory cache did not retain one owner/LOD across four views");
    checked(model,nodes,false,[&]{return frame.update_factory(0x20001,2,model,nodes,false);});
    checked(model,nodes,false,[&]{return frame.update_factory(0x10001,3,model,nodes,false);});
    checked(model,nodes,true,[&]{return frame.update_factory(0x10001,2,model,nodes,true);});
    auto changed=nodes;changed[0].position.x+=.125f;
    checked(model,changed,true,[&]{return frame.update_factory(0x10001,2,model,changed,true);});auto copy=model;
    checked(copy,changed,true,[&]{return frame.update_factory(0x10001,2,copy,changed,true);});
    need(frame.builds()==6,"Factory cache mixed full owner/LOD/model/WORLD/GPU identities");
    frame.clear();need(!frame.builds()&&!frame.reuses(),"Frame clear retained cache counters");
    need(!checked(model,nodes,false,[&]{return frame.update_factory(0x10001,2,model,nodes,false);}).reused,
        "Frame clear retained a prior lifetime's pose");
    for(const auto *name:{"HALO2_REFERENCE_SCALAR_SKINNING","HALO2_VERIFY_SIMD_SKINNING"}) {
        setenv(name,"1",1);ModelWorldPoseCache cache;
        for(unsigned view=0;view<2;++view) {
            const auto result=checked(model,nodes,true,[&]{return cache.update_factory(model,nodes,true);});
            need(!result.reused&&!result.pose.gpu,"Factory cache bypassed active scalar/VERIFY diagnostics");
        }
        unsetenv(name);
    }
    std::cout<<"PASS factory/deferred clear-PE CPU to set-PE GPU transition; "<<cases
             <<" complete factory-cache CPU/GPU/FP cases, "<<hits<<" strict hits, "<<rebuilds
             <<" effect rebuilds; fresh per-call state oracle, owner/LOD/model/nodes/GPU identity and diagnostics\n";
}
}
int main(int argc,char **argv)try {
    RestoreFp saved;
    setenv("HALO2_VERIFY_SKIN_ADMISSION","1",1);
    unsetenv("HALO2_REFERENCE_SCALAR_SKINNING");unsetenv("HALO2_VERIFY_SIMD_SKINNING");
    if(argc==2 && std::string_view(argv[1])=="--reference-cache") {reference_cache_mode();factory_pose_cache();return 0;}
    need(argc==1,"Supply only --reference-cache or no arguments");
    lazy_summary_allocation();reconstructed_owner();copied_owner();concurrent_cold_readers();concurrent_copies();factory_pose_cache();return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
