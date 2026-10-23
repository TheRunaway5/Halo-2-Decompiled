#include "halo2/source_campaign_model.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include "rendering/renderer/frame_model_skin_plan.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <unistd.h>

using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw FormatError(message);}
void same_vertices(std::span<const Vertex> a,std::span<const Vertex> b,const char *message) {
    need(a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size_bytes())),message);
}
struct Fp {
    std::fenv_t env{};unsigned mxcsr{};std::uint16_t control{},status{};int error{};
    static Fp get() {
        Fp out;out.error=errno;std::fegetenv(&out.env);out.mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status)::"memory");return out;
    }
    void restore()const {std::fesetenv(&env);_mm_setcsr(mxcsr);errno=error;}
    bool same(const Fp &other)const{return mxcsr==other.mxcsr && control==other.control && status==other.status && error==other.error;}
};
struct RestoreFp {Fp state=Fp::get();~RestoreFp(){state.restore();}};
struct Environment {
    const char *name;std::optional<std::string> previous;
    explicit Environment(const char *n):name(n){if(const auto *p=std::getenv(n))previous=p;unsetenv(name);}
    void set(bool value){if(value)setenv(name,"1",1);else unsetenv(name);}
    ~Environment(){if(previous)setenv(name,previous->c_str(),1);else unsetenv(name);}
};
std::vector<Vertex> materialize(const SourceCampaignDrawGeometry &geometry) {
    auto result=geometry.vertices;
    for(const auto &submission:geometry.deferred_skins) {
        need(bool(submission.geometry),"Campaign deferred range lost its immutable owner");
        const auto &skin=*submission.geometry;
        need(submission.first<=result.size() && skin.vertices.size()<=result.size()-submission.first,
            "Campaign deferred range escapes its output bank");
        skin_model_vertices(std::span{result}.subspan(submission.first,skin.vertices.size()),skin.bindings,skin.palette);
    }
    return result;
}
void run(const std::filesystem::path &map) {
    RestoreFp saved;Environment scalar("HALO2_REFERENCE_SCALAR_SKINNING"),verify("HALO2_VERIFY_SIMD_SKINNING");
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch && *scratch,"Use tools/run_test.py for campaign model handoff");
    auto path=(std::filesystem::path(scratch)/"campaign-model-skin-world-XXXXXX").string();
    need(bool(::mkdtemp(path.data())),"Campaign model handoff source storage unavailable");
    struct Cleanup {std::string path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}}cleanup{path};
    Cache cache(map);need(cache.is_mcc(),"Campaign model handoff requires a genuine prepared MCC map");
    MultiplayerObjectRuntime objects(std::filesystem::path{},cache,0,path);
    auto &world=objects.tu15_world();auto &memory=world.memory();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678);
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    const auto scenario=word(0x4ecc20u),count=word(scenario+0x160u),squads=word(scenario+0x164u);
    const auto atmosphere=cache.load_bsp_atmosphere(0);std::uint32_t selected=null_datum;
    // Same authored ordinary-Cairo squad admission as the existing armed case.
    for(unsigned i=0;i<count;++i) {
        const auto row=squads+i*0x74u,starts=word(row+0x48u),rows=word(row+0x4cu);
        if(!starts || std::bit_cast<std::int16_t>(Bytes(memory.view(row+0x2cu,2)).u16(0))<=0)continue;
        bool admitted=true;
        for(unsigned n=0;n<starts;++n) {
            const Bytes start(memory.view(rows+n*0x64u,0x64));
            if(start.u16(0x10)!=0xffff || atmosphere.cluster_at({start.f32(4),start.f32(8),start.f32(12)})<0)admitted=false;
        }
        if(admitted){selected=i;break;}
    }
    need(selected!=null_datum,"No authored ordinary Cairo squad admitted in BSP0");world.place_ai(selected,1);
    const auto live=objects.live_objects();
    const auto actor=std::ranges::find_if(live,[&](const auto &object) {
        return object.type==0 && word(objects.source_object_body(object.object)+0x12cu)!=null_datum;
    });
    need(actor!=live.end(),"Original Cairo ai_place produced no actual actor");
    const auto root=objects.object_transform(actor->object);const auto eye=root.position;
    SourceFrustumProjectionInput projection;projection.projection_angle=1.f;
    projection.outer=projection.safe={0,0,480,640};projection.scenario_present=true;projection.scenario_type=0;
    const auto projected=source_frustum_projection(projection);
    const auto rendering=source_view_projection({eye,root.basis[0],root.basis[2],.015625f,1024.f,{},projected.projection_program});
    const SourceModelShaderView shader_view{source_shader_coverage_camera(rendering),
        source_shader_lod_view(0,480,false,false),1.f,source_shader_lod_player_multiplier(1),false};
    SourceCampaignViewInputTu15 input;input.rendering=rendering;
    input.clipping=source_frustum_camera({eye,root.basis[0],root.basis[2],.015625f,1024.f,projected.clipping_rectangle()});
    input.eye=eye;input.forward=root.basis[0];input.viewport={0,0,480,640};
    input.distance_scale=input.coverage_multiplier=1.f;input.view_modifier=projected.view_modifier;
    SourceCampaignViewTu15 view(cache,0);SourceCampaignViewFrameTu15 frame;
    objects.render_model_view([&](auto &owner){frame=view.collect(owner,input,atmosphere,atmosphere.sample(eye,root.basis[0]));});
    need(frame.drawn.contains(actor->object),"Campaign view did not admit its actual actor");
    std::vector<SourceLiveModelBaseDrawTu15> ordinary;
    for(const auto &unit:frame.base_draws)if(unit.object==actor->object && unit.lighting_class==2 &&
        unit.override_slot==255 && (unit.unit.words[0]&1u))ordinary.push_back(unit);
    need(ordinary.size()>1,"Campaign handoff lacks actual multi-unit ordinary body coverage");
    const auto nodes=objects.rendered_world_nodes(actor->object);need(nodes && !nodes->empty(),"Campaign actor lost completed WORLD nodes");
    const auto rng=word(0x4f3cd8u),rng_before=word(rng);
    const auto source_clock=memory.view(objects.source_clock(),0x24);
    const std::vector<std::byte> clock_before(source_clock.begin(),source_clock.end());
    SourceCampaignModel body(cache,actor->definition);Mesh resident;body.prepare_materials(cache,resident);
    unsigned cases{},gpu_cases{},packets{},mapped_records{};
    const auto compare=[&](std::span<const SourceLiveModelBaseDrawTu15> draws,bool allow_gpu) {
        const auto initial=Fp::get();const auto expected=body.draw(actor->object,draws,*nodes,shader_view);
        const auto terminal=Fp::get();initial.restore();
        const auto actual=body.draw_deferred(actor->object,draws,*nodes,shader_view,nullptr,allow_gpu);
        const auto effects=Fp::get();need(terminal.same(effects),"Campaign deferred draw changed terminal SSE/x87/errno effects");
        same_vertices(expected,materialize(actual),"Campaign deferred draw changed complete selected vertex bytes/order/materials");
        need(allow_gpu || actual.deferred_skins.empty(),"Disabled campaign handoff returned GPU ranges");
        if(!actual.deferred_skins.empty()) {
            ++gpu_cases;packets+=unsigned(actual.deferred_skins.size());
            FrameModelSkinPlan plan;plan.add_bank(actual.vertices,actual.deferred_skins);
            // Real group reordering: reverse the contiguous part runs, preserving
            // each run's exact source span and final copied destination.
            std::vector<std::span<const Vertex>> runs;
            for(std::size_t first=0;first<actual.vertices.size();) {
                auto end=first+1;const auto &v=actual.vertices[first];
                while(end<actual.vertices.size() && actual.vertices[end].material==v.material &&
                    actual.vertices[end].source_section==v.source_section && actual.vertices[end].source_part==v.source_part)++end;
                runs.push_back(std::span{actual.vertices}.subspan(first,end-first));first=end;
            }
            std::reverse(runs.begin(),runs.end());plan.append_parts(runs,7);plan.validate_complete();
            need(plan.mapped_vertices()==plan.expected_vertices(),"Campaign grouped destination mapping omitted a packet");
            mapped_records+=unsigned(plan.records().size());
            for(const auto &submission:actual.deferred_skins)need(submission.geometry->owner==actor->object &&
                submission.geometry->definition==actor->definition && submission.geometry->render_model==draws.front().unit.words[1] &&
                submission.geometry->lod==draws.front().lod,"Campaign packet lost full owner/definition/model/LOD identity");
        }
        ++cases;
    };
    for(int rounding:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})
    for(unsigned denorm:{0u,0x40u,0x8000u,0x8040u})for(unsigned precision:{0u,32u}) {
        saved.state.restore();need(std::fesetround(rounding)==0,"Campaign test cannot select rounding");
        std::feclearexcept(FE_ALL_EXCEPT);_mm_setcsr((_mm_getcsr()&~0x807fu)|denorm|precision|0x1f80u);errno=EDOM;
        compare(ordinary,true);compare(ordinary,false);
    }
    saved.state.restore();_mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);errno=EDOM;
    auto repeated=ordinary;repeated.push_back(ordinary.front());compare(repeated,true);
    auto coverage=ordinary;coverage.front().coverage=0;compare(coverage,true);coverage.front().coverage=255;compare(coverage,true);
    auto unresident=ordinary;for(auto &unit:unresident)unit.geometry.geometry.resident=false;compare(unresident,true);
    compare({},true);
    for(auto *diagnostic:{&scalar,&verify}) {diagnostic->set(true);compare(ordinary,true);diagnostic->set(false);}
    const auto rejects=[&](const auto &draws,std::span<const Transform> pose) {
        const auto initial=Fp::get();std::string expected,actual;
        try{(void)body.draw(actor->object,draws,pose,shader_view);}catch(const FormatError &error){expected=error.what();}
        const auto terminal=Fp::get();initial.restore();
        try{(void)body.draw_deferred(actor->object,draws,pose,shader_view,nullptr,true);}catch(const FormatError &error){actual=error.what();}
        need(!expected.empty() && actual==expected && terminal.same(Fp::get()),"Campaign deferred failure changed original diagnostic/FP prefix");
    };
    for(unsigned mutation=0;mutation<8;++mutation) {
        auto changed=ordinary;
        if(mutation==0)changed.front().object^=0x10000u;
        if(mutation==1)changed.front().definition^=0x10000u;
        if(mutation==2)changed.front().unit.words[1]^=0x10000u;
        if(mutation==3)changed.front().lighting_class=3;
        if(mutation==4)changed.front().override_slot=0;
        if(mutation==5)changed.front().unit.words[0]&=~1u;
        if(mutation==6)changed.front().lod=6;
        if(mutation==7) {changed.front().lighting_class=1;changed.front().unit.words[0]=(changed.front().unit.words[0]&0x1fffffffu)|(1u<<29);}
        rejects(changed,*nodes);
    }
    rejects(ordinary,{});
    need(gpu_cases && packets && mapped_records,"Actual multi-unit MCC campaign body never executed deferred handoff");
    need(word(rng)==rng_before && std::ranges::equal(clock_before,memory.view(objects.source_clock(),0x24)),
        "Campaign resource/handoff mutated source RNG or clock");
    std::cout<<"PASS genuine MCC native Cairo actor: "<<ordinary.size()<<" actual main units, "<<cases
        <<" CPU/deferred byte+effect cases, "<<gpu_cases<<" GPU cases, "<<packets<<" ranged packets, "
        <<mapped_records<<" grouped destination records; no Vulkan/pixel or matched-Xbox-frame claim\n";
}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply a prepared MCC Cairo map");run(argv[1]);return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
