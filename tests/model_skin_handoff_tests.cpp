#include "halo2/model_skin_submission.hpp"
#include "halo2/model_skin_gpu_admission.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/object_lighting.hpp"
#include "rendering/models/model_skin_vertices.hpp"
#include "rendering/models/model_frame_pose_cache.hpp"
#include <algorithm>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <optional>
#include <set>

using namespace halo2;
namespace {
void need(bool value,const char *message) {if(!value)throw FormatError(message);}
void same_vertices(std::span<const Vertex> a,std::span<const Vertex> b,const char *message) {
    need(a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size_bytes())),message);
}
struct Environment {
    const char *name;std::optional<std::string> previous;
    explicit Environment(const char *n):name(n) {if(const auto *p=std::getenv(n))previous=p;set(false);}
    void set(bool enabled) {if(enabled)setenv(name,"1",1);else unsetenv(name);}
    ~Environment(){if(previous)setenv(name,previous->c_str(),1);else unsetenv(name);}
};
struct Fp {
    std::fenv_t env{};unsigned mxcsr{};std::uint16_t control{},status{};int error{};
    static Fp get() {
        Fp out;out.error=errno;std::fegetenv(&out.env);out.mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status)::"memory");return out;
    }
    void restore()const {std::fesetenv(&env);_mm_setcsr(mxcsr);errno=error;}
    bool same(const Fp &other)const {
        return mxcsr==other.mxcsr && control==other.control && status==other.status && error==other.error;
    }
};
struct RestoreFp {Fp original=Fp::get();~RestoreFp(){original.restore();}};
RenderModel fixture(unsigned special=0) {
    RenderModel m;m.bones.resize(2);
    for(auto &bone:m.bones)bone.inverse={1,0,0,0,1,0,0,0,1,.125f,-.25f,.5f};
    m.bones[1].inverse_scale=.75f;
    m.compound_nodes.push_back({{0,1,255,255},{.25f,.75f,0}});
    for(unsigned n=0;n<25;++n) {
        Vertex v{};v.position={.1f+float(n)*.03125f,.2f,.3f};v.normal={.33f,.66f,.99f};
        v.tangent={.25f,.75f,.125f};v.binormal={.375f,.625f,.875f};
        v.color={std::bit_cast<float>(0x7fc12345u),-.4f,.2f};v.uv={.3f,.6f};v.lightmap_uv={.1f,.9f};
        v.material=n%5;v.source_section=n%3;v.source_part=n;v.lightmap=n%7;v.bsp_cluster=-1;
        v.incident={.3f,.2f,.7f};v.source_local_position=v.position;v.source_stipple_level=n%17;
        if(special==1)v.position.x=std::bit_cast<float>(0x7f800001u);
        if(special==2)v.position.x=std::bit_cast<float>(1u);
        if(special==3)v.position.x=std::bit_cast<float>(0x4a800000u);
        if(n%7==0)v.normal={-0.f,0.f,-0.f};
        m.mesh.vertices.push_back(v);SkinBinding b;b.bones={2,0,1,2};b.weights.fill(0);
        for(unsigned j=0;j<n%5;++j)b.weights[j]=.125f*float(j+1);
        if(n%5>=3)b.weights[1]=-0.f; // This interior lane must retain its bone lookup.
        m.skin.push_back(b);
    }
    return m;
}
std::array<Transform,2> world_nodes() {
    std::array<Transform,2> out;out[0].scale=1.1f;out[0].position={.13f,.27f,.41f};
    out[1].scale=.9f;out[1].position={-.3f,.7f,.2f};
    out[1].basis={Vec3{0,1,0},Vec3{-1,0,0},Vec3{0,0,1}};return out;
}
void palette_contract() {
    RestoreFp saved;const auto m=fixture();const auto nodes=world_nodes();
    std::vector<Transform> inverse;
    for(const auto &bone:m.bones) {
        Transform bind;bind.scale=bone.inverse_scale;
        for(unsigned c=0;c<3;++c)bind.basis[c]={bone.inverse[c*3],bone.inverse[c*3+1],bone.inverse[c*3+2]};
        bind.position={bone.inverse[9],bone.inverse[10],bone.inverse[11]};inverse.push_back(bind);
    }
    auto expected=source_model_skin_palette(nodes,inverse);append_source_model_compounds(expected,m.compound_nodes);
    const auto actual=model_skin_world_palette(m,nodes);
    need(actual.size()==3 && actual.size()==expected.size() &&
        !std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(actual.front())),
        "Exported WORLD palette changed original inverse-bind/compound row bytes");
    const auto posed=skin_model_world_nodes(m,nodes);
    for(bool simd:{false,true}) {
        auto result=m.mesh.vertices;skin_model_vertices(result,m.skin,actual,simd);
        same_vertices(posed,result,"Exported palette changed complete original scalar/SSE vertex bytes");
    }
    std::cout<<"PASS original WORLD/inverse-bind/compound palette, complete scalar/SSE metadata bytes\n";
}
void admission_contract() {
    RestoreFp saved;unsigned cases{};
    for(unsigned rounding:{0u,0x2000u,0x4000u,0x6000u})
    for(unsigned denorm:{0u,0x40u,0x8000u,0x8040u})
    for(unsigned exception_masks=0;exception_masks<64;++exception_masks)
    for(bool precision:{false,true}) {
        // No floating operation may run while these individual masks are
        // removed; this directly tests admission, without provoking a trap.
        const auto csr=rounding|denorm|(exception_masks<<7)|(precision?0x20u:0u);
        _mm_setcsr(csr);const bool admitted=model_skin_gpu_host_effects_admitted();const auto after=_mm_getcsr();
        _mm_setcsr(saved.original.mxcsr);
        need(admitted==(rounding==0 && exception_masks==63 && precision),
            "GPU host gate accepted an unmasked exception, changed rounding or absent precision effect");
        need(after==csr,"GPU host admission changed FP flags/control");++cases;
    }
    const auto model=fixture();const auto nodes=world_nodes();const auto palette=model_skin_world_palette(model,nodes);
    for(unsigned field=0;field<3;++field) {
        // The original static rectangle remains admitted. Its lower
        // neighboring normal values also fit this fixture's adaptive lattice;
        // upper/nonfinite violations still reject without FP side effects.
        const std::uint32_t low=field==0?0x33800000u:field==1?0x3a800000u:0x2b800000u;
        const std::uint32_t high=field==1?0x3f800000u:0x49800000u;
        for(const auto bits:{0u,0x80000000u,low,low|0x80000000u,high,high|0x80000000u,
                             low-1,high+1,0x7f800000u,0x7f800001u}) {
            auto vertices=model.mesh.vertices;auto bindings=model.skin;auto rows=palette;
            if(field==0)vertices[1].position.x=std::bit_cast<float>(bits);
            if(field==1)bindings[1].weights[0]=std::bit_cast<float>(bits);
            if(field==2)rows[0][0]=std::bit_cast<float>(bits);
            saved.original.restore();_mm_setcsr((_mm_getcsr()&~0x3fu)|0x1f80u);errno=ERANGE;
            const auto initial=Fp::get();const bool admitted=model_skin_gpu_arithmetic_admitted(vertices,bindings,rows);
            const auto terminal=Fp::get();const auto magnitude=bits&0x7fffffffu;
            need(admitted==(magnitude==0 || (magnitude>=0x00800000u && magnitude<=high)),"GPU numeric gate changed a proved endpoint");
            need(initial.same(terminal),"Integer numeric admission raised FP effects or changed errno");++cases;
        }
    }
    std::cout<<"PASS "<<cases<<" pure host/numeric admission cases, including every exception mask and signed endpoints\n";
}
void factory_effects(Environment &scalar,Environment &verify) {
    RestoreFp saved;unsigned cases{},admitted{},fallbacks{};const auto nodes=world_nodes();
    for(unsigned special=0;special<4;++special) {
        const auto model=fixture(special);
        for(int round:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})
        for(unsigned denorm:{0u,0x40u,0x8000u,0x8040u})
        for(unsigned flags=0;flags<64;++flags) {
            saved.original.restore();need(std::fesetround(round)==0,"Set handoff rounding control");
            std::feclearexcept(FE_ALL_EXCEPT);
            _mm_setcsr((_mm_getcsr()&~0x807fu)|denorm|flags|0x1f80u);errno=EDOM;
            const auto initial=Fp::get();const auto expected=skin_model_world_nodes(model,nodes);const auto terminal=Fp::get();
            for(bool allow_gpu:{false,true}) {
                initial.restore();const auto actual=model_skin_world_pose(model,nodes,allow_gpu);const auto effects=Fp::get();
                need(terminal.same(effects),"WORLD handoff changed terminal SSE/x87/errno effects");
                if(actual.gpu) {
                    need(allow_gpu && special==0 && round==FE_TONEAREST,"WORLD handoff admitted an unsupported numeric/host case");
                    same_vertices(model.mesh.vertices,actual.vertices,"GPU handoff executed CPU deformation or changed local metadata");
                    // This reconstruction is only an API boundary test. The real GPU
                    // dispatch/readback test separately qualifies the shader kernel.
                    auto reconstructed=actual.vertices;skin_model_vertices(reconstructed,model.skin,actual.palette,true);
                    same_vertices(expected,reconstructed,"GPU handoff rows cannot reconstruct the original complete CPU output");++admitted;
                } else {same_vertices(expected,actual.vertices,"Fallback handoff changed original complete CPU output");++fallbacks;}
                ++cases;
            }
        }
    }
    need(admitted && fallbacks,"WORLD handoff did not exercise both GPU and CPU paths");
    const auto model=fixture();
    for(auto *diagnostic:{&scalar,&verify}) {
        diagnostic->set(true);_mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);const auto initial=Fp::get();
        const auto expected=skin_model_world_nodes(model,nodes);const auto terminal=Fp::get();
        initial.restore();const auto actual=model_skin_world_pose(model,nodes,true);const auto effects=Fp::get();
        need(!actual.gpu && terminal.same(effects),"Active scalar/VERIFY diagnostic was bypassed or changed its effects");
        same_vertices(expected,actual.vertices,"Active scalar/VERIFY diagnostic changed complete output");diagnostic->set(false);
    }
    std::cout<<"PASS "<<cases<<" factory complete-output/SSE/x87/errno cases, "<<admitted<<" GPU handoffs, "<<fallbacks<<" exact CPU fallbacks; scalar/VERIFY controls\n";
}
void error_contract() {
    RestoreFp saved;const auto nodes=world_nodes();unsigned cases{};
    for(unsigned fault=0;fault<5;++fault) {
        auto model=fixture();auto actual_nodes=std::vector<Transform>(nodes.begin(),nodes.end());
        if(fault==0)actual_nodes.pop_back();
        if(fault==1)model.skin.pop_back();
        if(fault==2){model.skin[1].bones[0]=65535;model.skin[1].weights={1,0,0,0};}
        if(fault==3){model.skin[3].bones[1]=65535;model.skin[3].weights={.25f,0,.75f,0};}
        if(fault==4)model.compound_nodes[0].nodes[0]=254;
        _mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);errno=ERANGE;const auto initial=Fp::get();
        std::string expected;try{(void)skin_model_world_nodes(model,actual_nodes);}catch(const FormatError &e){expected=e.what();}
        const auto terminal=Fp::get();need(!expected.empty(),"Original malformed skin did not throw");
        for(bool allow_gpu:{false,true}) {
            initial.restore();std::string message;
            try{(void)model_skin_world_pose(model,actual_nodes,allow_gpu);}catch(const FormatError &e){message=e.what();}
            const auto effects=Fp::get();need(message==expected && terminal.same(effects),"WORLD handoff changed the original error or terminal FP/errno effects");++cases;
        }
    }
    std::cout<<"PASS "<<cases<<" original mapping/node/active-zero-bone/compound failures and exact terminal effects\n";
}
std::vector<Vertex> materialize(const RenderModel &model,const ModelSkinWorldPoseView &pose) {
    std::vector<Vertex> out(pose.vertices.begin(),pose.vertices.end());
    if(pose.gpu)skin_model_vertices(out,model.skin,pose.palette,true);return out;
}
void deferred_cache(Environment &scalar,Environment &verify) {
    RestoreFp saved;unsigned cases{},hits{},rebuilds{},gpu{};const auto nodes=world_nodes();
    for(unsigned special=0;special<4;++special) {
        const auto model=fixture(special);
        for(int round:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO})
        for(unsigned denorm:{0u,0x40u,0x8000u,0x8040u})
        for(unsigned flags:{0u,1u,2u,4u,8u,16u,32u,63u}) {
            saved.original.restore();need(std::fesetround(round)==0,"Set deferred-cache rounding control");
            std::feclearexcept(FE_ALL_EXCEPT);
            _mm_setcsr((_mm_getcsr()&~0x807fu)|denorm|flags|0x1f80u);errno=EDOM;
            const auto initial=Fp::get();const auto expected=skin_model_world_nodes(model,nodes);const auto terminal=Fp::get();
            ModelWorldPoseCache cache;
            initial.restore();const auto cold=cache.update_deferred(model,nodes,true);const auto cold_effects=Fp::get();
            need(!cold.reused && terminal.same(cold_effects),"Deferred cache cold fill changed original effects");
            same_vertices(expected,materialize(model,cold.pose),"Deferred cache cold fill changed complete materialized vertices");
            initial.restore();const auto strict=cache.update_deferred(model,nodes,true);const auto strict_effects=Fp::get();
            need(terminal.same(strict_effects),"Deferred cache strict hit/rebuild changed terminal SSE/x87/errno effects");
            same_vertices(expected,materialize(model,strict.pose),"Deferred cache strict hit/rebuild changed complete materialized vertices");
            if(strict.reused)++hits;else ++rebuilds;if(strict.pose.gpu)++gpu;
            terminal.restore();const auto repeated=cache.update_deferred(model,nodes,true);const auto repeated_effects=Fp::get();
            need(repeated.reused && terminal.same(repeated_effects),"Deferred cache failed to reuse existing exact effects");++cases;
        }
    }
    need(hits && rebuilds && gpu,"Deferred cache missed admitted GPU, strict hit or required rebuild coverage");
    const auto model=fixture();ModelWorldPoseCache cache;
    _mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);
    need(!cache.update(model,nodes).reused,"Normal cache cold fill reused absent storage");
    const auto deferred=cache.update_deferred(model,nodes);need(!deferred.reused && deferred.pose.gpu,"Cache mode change reused posed CPU records as local GPU records");
    const auto normal=cache.update(model,nodes);need(!normal.reused,"Cache mode change reused local GPU records as posed CPU records");
    same_vertices(skin_model_world_nodes(model,nodes),normal.vertices,"Mode change lost original complete CPU output");
    for(auto *diagnostic:{&scalar,&verify}) {
        diagnostic->set(true);const auto initial=Fp::get();const auto expected=skin_model_world_nodes(model,nodes);const auto terminal=Fp::get();
        initial.restore();const auto first=cache.update_deferred(model,nodes);const auto first_effects=Fp::get();
        need(!first.reused && !first.pose.gpu && terminal.same(first_effects),"Deferred diagnostic did not execute the original fresh CPU fill");
        same_vertices(expected,first.pose.vertices,"Deferred diagnostic changed complete original CPU output");
        need(!cache.update_deferred(model,nodes).reused,"Deferred nonstrict hit bypassed active scalar/VERIFY diagnostic");diagnostic->set(false);
    }
    _mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);cache.update_deferred(model,nodes);
    bool failed{};try{(void)cache.update_deferred(model,std::span<const Transform>{});}catch(const FormatError &e){failed=std::string(e.what())=="posed model world bank count changed";}
    need(failed,"Deferred cache swallowed original failed-fill diagnostic");
    const auto retained=cache.update_deferred(model,nodes);
    need(retained.reused,"Failed deferred fill replaced the preceding complete owner/key");
    ModelFramePoseCache frame;
    for(unsigned view=0;view<4;++view)for(unsigned section=0;section<3;++section) {
        const auto pose=frame.update_deferred(0x10001,2,model,nodes,true);
        same_vertices(skin_model_world_nodes(model,nodes),materialize(model,pose.pose),"Frame deferred cache changed complete original CPU result");
    }
    need(frame.builds()==1 && frame.reuses()==11,"Frame deferred cache lost one full owner across repeated sections/views");
    frame.update_deferred(0x20001,2,model,nodes);frame.update_deferred(0x10001,3,model,nodes);
    auto other=model;frame.update_deferred(0x10001,2,other,nodes);
    auto changed=nodes;changed[0].position.x+=.125f;frame.update_deferred(0x10001,2,other,changed);
    need(frame.builds()==5,"Frame deferred cache reused a changed owner/LOD/model/WORLD bank");
    std::cout<<"PASS "<<cases<<" deferred-cache complete-output/effect cases, "<<hits<<" strict hits, "<<rebuilds
             <<" required rebuilds, "<<gpu<<" GPU banks; mode identity, diagnostics, failed-fill recovery and twelve-view/section reuse\n";
}
void describe_numeric_admission(const RenderModel &model,std::span<const Transform> nodes) {
    RestoreFp saved;
    auto palette=model_skin_world_palette(model,nodes);auto vertices=model.mesh.vertices;
    const auto csr=_mm_getcsr();const bool host=model_skin_gpu_host_effects_admitted();
    const bool numeric=model_skin_gpu_arithmetic_admitted(vertices,model.skin,palette);
    struct Statistics {
        std::uint32_t minimum=0x7fffffffu,maximum{};std::size_t values{},rejected{},printed{};
        void observe(float value,std::uint32_t low,std::uint32_t high,const std::string &identity) {
            const auto literal=std::bit_cast<std::uint32_t>(value),magnitude=literal&0x7fffffffu;
            ++values;if(magnitude){minimum=std::min(minimum,magnitude);maximum=std::max(maximum,magnitude);}
            if(magnitude && (magnitude<low || magnitude>high)) {
                ++rejected;if(printed++<6)std::cout<<"  rejected "<<identity<<" bits=0x"<<std::hex<<literal<<std::dec
                    <<" value="<<std::setprecision(9)<<value<<'\n';
            }
        }
        void print(const char *name)const {
            std::cout<<"  "<<name<<" count="<<values<<" rejected="<<rejected<<" min_nonzero_abs_bits=0x"<<std::hex<<minimum
                <<" max_abs_bits=0x"<<maximum<<std::dec<<" min_nonzero_abs="<<std::setprecision(9)
                <<(minimum==0x7fffffffu?0.f:std::bit_cast<float>(minimum))<<" max_abs="<<std::bit_cast<float>(maximum)<<'\n';
        }
    } attributes,weights,rows;
    std::cout<<"MCC GPU admission "<<model.name<<" host_after_palette="<<host<<" mxcsr_after_palette=0x"<<std::hex<<csr<<std::dec
        <<" numeric="<<numeric<<" vertices="<<vertices.size()<<" bindings="<<model.skin.size()<<" palette="<<palette.size()<<'\n';
    for(std::size_t i=0;i<vertices.size();++i) {
        const std::array<const Vec3*,4> banks{&vertices[i].position,&vertices[i].normal,&vertices[i].tangent,&vertices[i].binormal};
        constexpr std::array names{"position","normal","tangent","binormal"};
        for(unsigned a=0;a<4;++a) {
            const std::array values{banks[a]->x,banks[a]->y,banks[a]->z};
            for(unsigned c=0;c<3;++c)attributes.observe(values[c],0x33800000u,0x49800000u,
                "vertex["+std::to_string(i)+"]."+names[a]+"["+std::to_string(c)+"]");
        }
    }
    std::size_t invalid_bones{};
    for(std::size_t i=0;i<model.skin.size();++i) {
        int last=-1;
        for(unsigned j=0;j<4;++j) {
            weights.observe(model.skin[i].weights[j],0x3a800000u,0x3f800000u,
                "binding["+std::to_string(i)+"].weight["+std::to_string(j)+"]");
            if(std::bit_cast<std::uint32_t>(model.skin[i].weights[j])&0x7fffffffu)last=int(j);
        }
        for(int j=last;j>=0;--j)if(model.skin[i].bones[unsigned(j)]>=palette.size()) {
            if(invalid_bones++<6)std::cout<<"  invalid binding["<<i<<"].bone["<<j<<"]="<<model.skin[i].bones[unsigned(j)]<<'\n';
        }
    }
    for(std::size_t r=0;r<palette.size();++r)for(unsigned c=0;c<12;++c)
        rows.observe(palette[r][c],0x2b800000u,0x49800000u,"palette["+std::to_string(r)+"]["+std::to_string(c)+"]");
    attributes.print("attributes");weights.print("weights");rows.print("palette");
    std::cout<<"  invalid_active_bones="<<invalid_bones<<'\n';
}
void prepared_assets(const std::filesystem::path &path) {
    RestoreFp saved;Cache cache(path);need(cache.is_mcc(),"Handoff integration requires an explicit prepared MCC map");
    unsigned models{},gpu_owners{},selected_vertices{},indexed_cases{};
    for(const char *name:{"objects\\characters\\masterchief\\masterchief","objects\\characters\\elite\\elite",
                          "objects\\weapons\\rifle\\smg\\smg"}) {
        const auto *tag=cache.query_tag({.group=tag_groups::render_model,.name=name,.require_address=true});
        need(tag,"Prepared MCC handoff owner is absent");auto model=cache.load_model(tag->datum,true);
        need(!model.mesh.vertices.empty() && model.skin.size()==model.mesh.vertices.size(),"Prepared MCC owner has no complete skin bank");
        for(auto &material:model.mesh.materials)if(!material.lods.empty())
            need(prepare_dynamic_material(cache,material,{},true,0.f),"Prepared MCC ordinary shader needs unresolved source inputs");
        SourceDynamicModelShaders shaders(cache,model,tag->datum,2,0.f);
        Transform root;root.position={3.125f,-.07f,.625f};root.scale=.625f;
        const auto nodes=pose_model_nodes_world(model,root);const auto posed=skin_model_world_nodes(model,nodes);
        const auto palette=model_skin_world_palette(model,nodes);
        for(bool simd:{false,true}) {
            auto result=model.mesh.vertices;skin_model_vertices(result,model.skin,palette,simd);
            same_vertices(posed,result,"Prepared MCC exported palette differs from original complete CPU skin");
        }
        std::set<unsigned> bank;for(const auto &v:model.mesh.vertices)bank.insert(v.source_section);
        std::vector<unsigned> sections(bank.rbegin(),bank.rend());const auto repeated_sections=sections;
        sections.insert(sections.end(),repeated_sections.begin(),repeated_sections.end());
        // Duplicate whole sections deliberately: equal position/material bytes
        // cannot identify which expanded source ordinal supplied a record.
        std::vector<Vertex> expanded;std::vector<std::uint32_t> ordinals;
        for(auto section:sections)for(unsigned i=0;i<model.mesh.vertices.size();++i)
            if(model.mesh.vertices[i].source_section==section){expanded.push_back(model.mesh.vertices[i]);ordinals.push_back(i);}
        for(unsigned players:{1u,4u})for(std::uint8_t coverage:{std::uint8_t(0),std::uint8_t(255)}) {
            SourceModelShaderView view{{{0,0,-1,0},players==1?240.f:120.f},
                source_shader_lod_view(0,players==1?480:240,false,false),1.f,source_shader_lod_player_multiplier(players),false};
            const SourceModelShaderCollector collector{0,coverage};auto expected=expanded;shaders.select(expected,collector,view);
            auto actual=expanded;auto indices=ordinals;shaders.select_indexed(actual,indices,collector,view);
            same_vertices(expected,actual,"Indexed selection changed complete original selected/shaded vertex bytes");
            std::vector<std::uint32_t> expected_indices;bool accepted{};
            for(unsigned i=0;i<expanded.size();++i) {
                // The old selection is the independent identity/material oracle.
                // Cache its one-record result for each literal authored part.
                if(!i || expanded[i].source_section!=expanded[i-1].source_section || expanded[i].source_part!=expanded[i-1].source_part) {
                    std::vector<Vertex> single{expanded[i]};shaders.select(single,collector,view);accepted=!single.empty();
                }
                if(accepted)expected_indices.push_back(ordinals[i]);
            }
            need(indices==expected_indices,"Indexed selection lost expanded source ordinals through repeated sections");++indexed_cases;
        }
        std::array<RenderModel,6> lods;lods[5]=model;
        constexpr std::uint32_t owner=0x10001,definition=0x20002;
        std::vector<SourceLiveModelBaseDrawTu15> draws;
        for(auto section:sections) {
            SourceLiveModelBaseDrawTu15 draw;draw.object=owner;draw.definition=definition;draw.lod=5;
            draw.lighting_class=2;draw.override_slot=255;draw.coverage=255;
            draw.unit.words[0]=1;draw.unit.words[1]=tag->datum;draw.unit.words[3]=section<<9;draws.push_back(draw);
        }
        const SourceModelShaderView view{{{0,0,-1,0},240.f},source_shader_lod_view(0,480,false,false),1.f,.85f,false};
        const auto expected=source_live_model_base_vertices_tu15(owner,definition,tag->datum,draws,lods,shaders,nodes,view);
        need(expected && !expected->empty(),"Prepared MCC selected CPU owner is absent");
        _mm_setcsr((_mm_getcsr()&~0x6000u)|0x1fa0u);
        describe_numeric_admission(model,nodes);
        std::shared_ptr<const ModelSkinGeometry> deferred;
        auto actual=source_live_model_base_vertices_tu15(owner,definition,tag->datum,draws,lods,shaders,nodes,view,{},&deferred);
        need(actual.has_value(),"Prepared MCC deferred owner was lost");
        const bool owner_gpu=bool(deferred);
        if(deferred) {
            need(deferred->owner==owner && deferred->definition==definition && deferred->render_model==tag->datum && deferred->lod==5,
                "Deferred owner/definition/render-model/LOD identity changed");
            same_vertices(*actual,deferred->vertices,"Deferred immutable handoff differs from selected local records");
            need(deferred->vertices.size()==deferred->bindings.size() && deferred->vertices.size()==deferred->source_vertices.size(),
                "Deferred selected bindings/ordinals lost ordered extent");
            for(unsigned i=0;i<deferred->vertices.size();++i) {
                const auto ordinal=deferred->source_vertices[i];need(ordinal<model.skin.size(),"Deferred source ordinal escaped its original owner");
                const auto &binding=deferred->bindings[i];const auto &reference=model.skin[ordinal];
                need(binding.bones==reference.bones && std::bit_cast<std::array<std::uint32_t,4>>(binding.weights)==std::bit_cast<std::array<std::uint32_t,4>>(reference.weights),
                    "Deferred repeated section selected another source skin binding");
            }
            skin_model_vertices(*actual,deferred->bindings,deferred->palette,true);++gpu_owners;
        }
        same_vertices(*expected,*actual,"Deferred selected owner cannot reproduce complete CPU vertices/materials/order");
        auto rejected=draws;rejected.back().override_slot=0;
        need(!source_live_model_base_vertices_tu15(owner,definition,tag->datum,rejected,lods,shaders,nodes,view,{},&deferred) && !deferred,
            "Late unsupported owner override published a partial deferred bank");
        auto malformed=model.mesh.vertices;std::vector<std::uint32_t> short_indices(malformed.size()-1);bool failed{};
        try{shaders.select_indexed(malformed,short_indices,{},view);}catch(const FormatError &e){failed=std::string(e.what())=="dynamic model source ordinals differ from vertex extent";}
        need(failed,"Indexed shader selection accepted a mismatched ordinal extent");
        malformed=model.mesh.vertices;malformed.front().source_part=0xffffffffu;
        std::vector<std::uint32_t> identity(malformed.size());std::iota(identity.begin(),identity.end(),0);failed=false;
        try{shaders.select_indexed(malformed,identity,{},view);}catch(const FormatError &e){failed=std::string(e.what())=="dynamic model vertex lost its authored shader part";}
        need(failed,"Indexed shader selection swallowed original missing-part diagnostic");
        selected_vertices+=unsigned(expected->size());++models;
        std::cout<<"MCC "<<name<<": "<<expected->size()<<" ordered selected records, deferred="<<owner_gpu<<'\n';
    }
    need(models==3 && gpu_owners==3,"Prepared MCC body/held owners did not execute the deferred skin route");
    std::cout<<"PASS "<<models<<" prepared MCC body/held owners, "<<selected_vertices<<" complete selected records, "<<indexed_cases
             <<" per-view indexed cases; atomic owner rejection and original shader diagnostics\n";
}
}
int main(int argc,char **argv)try {
    RestoreFp saved;Environment scalar("HALO2_REFERENCE_SCALAR_SKINNING"),verify("HALO2_VERIFY_SIMD_SKINNING");
    palette_contract();admission_contract();factory_effects(scalar,verify);error_contract();deferred_cache(scalar,verify);
    if(argc==2)prepared_assets(argv[1]);else need(argc==1,"Supply only an explicit prepared MCC map");
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
