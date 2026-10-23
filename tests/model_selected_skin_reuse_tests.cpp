#include "halo2/source_live_model_geometry_tu15.hpp"
#include "rendering/models/model_frame_pose_cache.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <ctime>
#include <iostream>
#include <optional>
#include <string>

using namespace halo2;
namespace {
void need(bool value,const char *message) {if(!value)throw FormatError(message);}
void equal(std::span<const Vertex> a,std::span<const Vertex> b) {
    need(a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size_bytes())),
         "Selected skin changed complete vertex bytes/order");
}
struct Environment {
    const char *name;std::optional<std::string> previous;
    explicit Environment(const char *n):name(n) {if(const auto *p=std::getenv(name))previous=p;set(false);}
    void set(bool on) {
#ifdef _WIN32
        _putenv_s(name,on?"1":"");
#else
        if(on)setenv(name,"1",1);else unsetenv(name);
#endif
    }
    ~Environment() {
#ifdef _WIN32
        _putenv_s(name,previous?previous->c_str():"");
#else
        if(previous)setenv(name,previous->c_str(),1);else unsetenv(name);
#endif
    }
};
struct Fp {
    std::fenv_t env{};unsigned mxcsr{};std::uint16_t control{},status{};int error{};
    static Fp get() {
        Fp out;out.error=errno;std::fegetenv(&out.env);out.mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status)::"memory");
        return out;
    }
    void restore()const {std::fesetenv(&env);_mm_setcsr(mxcsr);errno=error;}
    bool same(const Fp &b)const {return mxcsr==b.mxcsr && control==b.control && status==b.status && error==b.error;}
};
struct RestoreFp {Fp original=Fp::get();~RestoreFp(){original.restore();}};
RenderModel model(unsigned count=24,unsigned special=0) {
    RenderModel m;m.bones.resize(1);m.bones[0].inverse={1,0,0,0,1,0,0,0,1,0,0,0};
    for(unsigned n=0;n<count;++n) {
        Vertex v{};v.position={.1f+float(n)*.03125f,.2f,.3f};v.normal={.33f,.66f,.99f};
        v.tangent={.25f,.75f,.125f};v.binormal={.375f,.625f,.875f};
        v.color={.7f,.4f,.2f};v.uv={.3f,.6f};v.lightmap_uv={.1f,.9f};
        v.material=n%5;v.source_section=n%3;v.source_part=n;v.lightmap=n%7;
        v.bsp_cluster=-1;v.incident={.3f,.2f,.7f};v.source_local_position=v.position;
        if(special==1)v.position.x=std::bit_cast<float>(0x7f800001u);
        if(special==2)v.position.x=std::bit_cast<float>(0x00000001u);
        m.mesh.vertices.push_back(v);m.skin.emplace_back();m.skin.back().weights={.37f,0,0,0};
    }
    return m;
}
std::array<Transform,1> nodes() {
    Transform n;n.scale=1.1f;n.position={.13f,.27f,.41f};return {n};
}
void fp_contract() {
    RestoreFp saved;unsigned cases{},hits{},rebuilds{};
    for(unsigned special=0;special<3;++special) {
        const auto m=model(24,special);const auto world=nodes();
        for(int round:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})
        for(unsigned mode:{0u,0x40u,0x8000u,0x8040u})
        for(unsigned flags:{0u,1u,2u,4u,8u,16u,32u,63u}) {
            saved.original.restore();need(std::fesetround(round)==0,"Set skin rounding control");
            std::feclearexcept(FE_ALL_EXCEPT);_mm_setcsr((_mm_getcsr()&~0x8040u&~0x3fu)|mode|flags|0x1f80u);errno=EDOM;
            const auto initial=Fp::get();ModelWorldPoseCache cache;cache.update(m,world);
            initial.restore();const auto expected=skin_model_world_nodes(m,world);const auto expected_fp=Fp::get();
            initial.restore();const auto actual=cache.update_preserving_effects(m,world);const auto actual_fp=Fp::get();
            equal(expected,actual.vertices);need(expected_fp.same(actual_fp),"Strict skin changed terminal SSE/x87/errno state");
            if(actual.reused)++hits;else ++rebuilds;
            const auto repeated=cache.update_preserving_effects(m,world);const auto repeated_fp=Fp::get();
            need(repeated.reused,"Effects already present did not admit exact skin reuse");
            equal(expected,repeated.vertices);need(actual_fp.same(repeated_fp),"Strict hit changed existing FP effects");
            ++cases;
        }
    }
    need(hits && rebuilds,"FP admission did not exercise both reuse and forced fresh work");
    std::cout<<"PASS "<<cases<<" complete vertex/SSE/x87/errno cases, "<<hits<<" admitted hits, "<<rebuilds<<" forced builds\n";
}
void identities_and_diagnostics(Environment &scalar,Environment &verify) {
    RestoreFp saved;const auto m=model();auto world=nodes();ModelFramePoseCache frame;
    frame.update_preserving_effects(0x10001,2,m,world);
    frame.update_preserving_effects(0x10001,2,m,world);
    need(frame.builds()==1 && frame.reuses()==1,"Identical full skin was not reused");
    frame.update_preserving_effects(0x20001,2,m,world);
    frame.update_preserving_effects(0x10001,3,m,world);
    auto other=m;frame.update_preserving_effects(0x10001,2,other,world);
    world[0].position.x+=.125f;frame.update_preserving_effects(0x10001,2,m,world);
    need(frame.builds()==5,"Changed generation/LOD/model/WORLD bytes reused stale geometry");
    ModelWorldPoseCache cache;cache.update(m,world);
    for(auto *mode:{&scalar,&verify}) {
        mode->set(true);const auto before=Fp::get();
        const auto expected=skin_model_world_nodes(m,world);const auto expected_fp=Fp::get();
        before.restore();const auto actual=cache.update_preserving_effects(m,world);const auto actual_fp=Fp::get();
        need(!actual.reused,"Strict cache bypassed active scalar/verification diagnostics");
        equal(expected,actual.vertices);need(expected_fp.same(actual_fp),"Diagnostic fresh path changed FP/errno effects");
        mode->set(false);
    }
    std::vector<Transform> wrong;
    bool failed{};try{cache.update_preserving_effects(m,wrong);}catch(const FormatError &){failed=true;}
    need(failed,"Changed node count did not preserve the fresh failure");
    need(cache.update_preserving_effects(m,world).vertices.size()==m.mesh.vertices.size(),"Failed fill poisoned the prior complete bank");
    std::cout<<"PASS full owner/LOD/model/pose identities, diagnostic fresh paths and failed-fill recovery\n";
}
void selection(Environment &scalar,Environment &verify) {
    RestoreFp saved;const auto m=model();const auto world=nodes();const std::array sections{2u,0u,2u,1u};
    const auto expected=source_live_model_selected_vertices_tu15(0x10001,2,m,world,sections);
    ModelFramePoseCache frame;unsigned calls{};
    SourceLiveModelPoseProviderTu15 provider=[&](std::uint32_t owner,unsigned lod,const RenderModel &actual,std::span<const Transform> pose) {
        need(owner==0x10001 && lod==2 && &actual==&m,"Selected provider lost its full request identity");
        ++calls;return frame.update_preserving_effects(owner,lod,actual,pose);
    };
    for(unsigned view=0;view<4;++view)for(unsigned section=0;section<3;++section)
        equal(expected,source_live_model_selected_vertices_tu15(0x10001,2,m,world,sections,provider));
    need(calls==12 && frame.builds()==1 && frame.reuses()==11,
         "Selected WORLD geometry still rebuilds the same complete skin");
    for(auto *mode:{&scalar,&verify}) {
        mode->set(true);const auto count=calls;
        equal(source_live_model_selected_vertices_tu15(0x10001,2,m,world,sections),
              source_live_model_selected_vertices_tu15(0x10001,2,m,world,sections,provider));
        need(calls==count,"Selected geometry bypassed a fresh diagnostic skin");mode->set(false);
    }
    const std::array absent{999u};
    for(bool cached:{false,true}) {
        bool failed{};
        try{(void)source_live_model_selected_vertices_tu15(0x10001,2,m,world,absent,cached?provider:SourceLiveModelPoseProviderTu15{});}
        catch(const FormatError &e){failed=std::string(e.what())=="Third-person collector selected an unprepared original section";}
        need(failed,"Selected absent section changed its original error");
    }
    std::cout<<"PASS twelve selected requests, one full build/eleven reuses, complete ordered duplicate sections and diagnostics\n";
}
void benchmark() {
    const auto m=model(16384);const auto world=nodes();const std::array sections{2u,0u,2u,1u};
    std::vector<double> fresh,cached;std::uint64_t checksum{};
    auto measure=[&](bool reuse) {
        ModelFramePoseCache frame;SourceLiveModelPoseProviderTu15 provider;
        if(reuse)provider=[&](auto owner,auto lod,const auto &model,auto pose){return frame.update_preserving_effects(owner,lod,model,pose);};
        const auto begin=std::clock();
        for(unsigned repeat=0;repeat<24;++repeat) {
            const auto vertices=source_live_model_selected_vertices_tu15(0x10001,2,m,world,sections,provider);
            checksum+=vertices.size()+std::bit_cast<std::uint32_t>(vertices.front().position.x);
        }
        return double(std::clock()-begin)/CLOCKS_PER_SEC;
    };
    for(unsigned pair=0;pair<7;++pair) {
        if(pair%2){cached.push_back(measure(true));fresh.push_back(measure(false));}
        else{fresh.push_back(measure(false));cached.push_back(measure(true));}
    }
    std::sort(fresh.begin(),fresh.end());std::sort(cached.begin(),cached.end());
    const auto speedup=fresh[3]/cached[3];
    std::cout<<"selected_fresh_cpu_seconds="<<fresh[3]<<" selected_cached_cpu_seconds="<<cached[3]
             <<" selected_speedup="<<speedup<<" checksum="<<checksum<<'\n';
    need(checksum && speedup>=1.10,"Selected skin has not met the component 1.10x gate");
}
}
int main(int argc,char **argv)try {
    RestoreFp fp;Environment scalar("HALO2_REFERENCE_SCALAR_SKINNING"),verify("HALO2_VERIFY_SIMD_SKINNING");
    fp_contract();identities_and_diagnostics(scalar,verify);selection(scalar,verify);
    if(argc==2 && std::string(argv[1])=="--require-speedup")benchmark();
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
