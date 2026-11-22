#include "halo2/pstencil/projected_lights.hpp"
#include "campaign_lighting_timing.hpp"
#include "halo2/appearance.hpp"
#include "halo2/pstencil/projected_light_material.hpp"
#include "halo2/pstencil/native_light_constants_tu15.hpp"
#include "halo2/pstencil/native_light_shape_tu15.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/projected_constants_2003.hpp"
#include "halo2/pstencil/caster.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_live_projected_model_palette_tu15.hpp"
#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_shadow_raw_draw.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/menu_world_depth.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <optional>
#include <sstream>
#include <set>
#include <tuple>

namespace halo2::pstencil {
namespace {
// A shadowed light's projection block, original vertex constants c40..48, and
// the viewport it renders inside the 512x512 depth target.
struct LightShadowBlock {
    std::array<ShaderVector,9> constants{};
    std::array<std::uint16_t,2> extent{};
};
// TU1.5 48ED0 for a dynamic light (feature bit 4 clear): the reciprocal range
// plane, the target size from the light's projected footprint, the depth
// planes and the gel basis the generation/application programs consume.
LightShadowBlock light_shadow_block(const SourceRuntimeMemory& memory,std::uint32_t record,std::uint32_t root,
    const NativeLightShape& shape,const EffectLightUniforms& uniforms) {
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto real=[&](std::uint32_t at){return std::bit_cast<float>(word(at));};
    const auto part=[&](unsigned index){return std::bit_cast<float>(shape.words.at(index));};
    const Bytes definition(memory.view(root,228));
    const Vec3 position{real(record+0x84u),real(record+0x88u),real(record+0x8cu)};
    const Vec3 forward{real(record+0xacu),real(record+0xb0u),real(record+0xb4u)};
    const float aspect=part(2),range=part(25);
    const float origin=definition.u16(0x8e)==1?part(10):0.f;
    const float reciprocal=range!=0.f?1.f/range:-std::numeric_limits<float>::max(); // 44AAC8
    LightShadowBlock out;auto& c=out.constants;
    c[0]={forward.x*reciprocal,forward.y*reciprocal,forward.z*reciprocal,0.f-reciprocal*origin};
    c[1]={0.f-forward.x,0.f-forward.y,0.f-forward.z,0.f};
    // 492260/492254 are the shape's volume and specular fades (record+108/+FC).
    const bool layer10=(word(0x4ce2a0u)>>10)&1u;
    const unsigned edge_mode=layer10?(real(record+0x108u)>0.f && definition.u16(0x14)==0?1u:0u):2u;
    double footprint=part(1);
    if(part(3)>0.f) {
        const double tangent=std::tan(double(part(3))*.5);
        footprint=tangent*double(range)+tangent*double(range)+double(part(1));
    }
    constexpr float minimum=128.f,maximum=512.f; // 46A270, feature bit 4 clear.
    const double scaled=(double(maximum)-double(minimum))*
        (double(definition.f32(0x10))*.125+std::sqrt(footprint*footprint*double(aspect))*double(std::bit_cast<float>(0x3eaaaaabu)))*
        double(real(record+0xfcu))+double(minimum);
    const float size=scaled<double(minimum)?minimum:float(scaled)<=maximum?float(scaled):maximum;
    const auto round_up=[](float value){return (std::uint32_t(std::ceil(value))+0x3fu)&0xffffffc0u;};
    auto width=round_up(size/std::sqrt(aspect)),height=round_up(size*std::sqrt(aspect));
    constexpr std::uint32_t area=512u*512u;
    if(width*height>area) {
        if(height<width)width=(area/height)&0xffffffc0u;
        else height=(area/width)&0xffffffc0u;
    }
    if(!width || !height || width>512 || height>512)throw FormatError("Light shadow target escaped its 512x512 surface");
    out.extent={std::uint16_t(width),std::uint16_t(height)};
    c[2]={float(width),float(height),16777215.f,1.f};
    const float edge=edge_mode?float(width)*.75f/maximum:0.f;
    c[3]={edge+.5f,.5f-edge,0.f,0.f};
    constexpr float bias_distance=std::bit_cast<float>(0x3c23d70au); // 46A18C
    c[4]={0.f-forward.x*100.f,0.f-forward.y*100.f,0.f-forward.z*100.f,
        ((0.f-forward.z)*(forward.z*bias_distance+position.z)+(0.f-forward.y)*(forward.y*bias_distance+position.y)+
         (0.f-forward.x)*(forward.x*bias_distance+position.x))*-100.f};
    const float near_plane=definition.f32(0x2c),far_plane=definition.f32(0x30);
    const float depth_scale=near_plane<far_plane?1.f/(far_plane-near_plane):1000.f;
    c[5]={forward.x*depth_scale,forward.y*depth_scale,forward.z*depth_scale,
        0.f-(forward.z*(forward.z*near_plane+position.z)+forward.y*(forward.y*near_plane+position.y)+
             forward.x*(forward.x*near_plane+position.x))*depth_scale};
    c[6]=uniforms.gel_basis_u;c[7]=uniforms.gel_basis_v;
    c[8]={1.f,1.f,0.f,0.f};
    return out;
}
struct CinematicLightShadow {
    std::uint32_t object{};
    ShaderVector projection{}; // c48 after 494A0.
};
// TU1.5 30EB0: each `render_lights_enable_cinematic_shadow` object this light
// receives gets the whole 512x512 buffer zoomed onto the sphere around its
// marker (167F80 in the light camera, clamped to the light frustum, 494A0).
std::vector<CinematicLightShadow> cinematic_light_shadows(PhysicsLiveWorldTu15& world,const NativeLightReceiver& receiver) {
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    const auto half=[&](std::uint32_t at){return Bytes(memory.view(at,2)).u16(0);};
    const auto real=[&](std::uint32_t at){return std::bit_cast<float>(word(at));};
    std::vector<CinematicLightShadow> out;
    const auto cinematic=word(0x51df50u);
    if(!cinematic || memory.view(cinematic+5u,1)[0]==std::byte{})return out;
    const auto count=std::bit_cast<std::int32_t>(word(0x4c6954u));
    if(count<=0)return out;
    if(count>16)throw FormatError("Cinematic shadow table exceeds its sixteen rows");
    const auto queue=word(receiver.context+0x14u);const unsigned objects=half(queue+4u);
    const auto camera=word(receiver.context)+4u;
    for(std::int32_t row=0;row<count;++row) {
        const auto entry=0x4c6894u+std::uint32_t(row)*12u;
        const auto object=word(entry);
        unsigned index=objects;
        for(unsigned n=0;n<objects;++n)if(word(word(queue+0xcu)+n*4u)==object){index=n;break;}
        if(index==objects || !(half(word(queue+8u)+index*2u)&0x1000u))continue;
        // BAD60(object, marker, 1): the marker's world position is +60 of its row.
        auto call=world.prepare_source_call();const auto caller=call.gp[4],output=caller-0x800u;
        const std::array<std::byte,0x200> empty{};world.write_source_call_input(output,empty);
        call.gp[4]=caller-0x3000u;
        const std::array arguments{word(caller),object,word(entry+4u),1u};
        world.write_source_call_input(call.gp[4],std::as_bytes(std::span(arguments)));
        call.gp[1]&=0xffffff00u;call.gp[3]=output;
        world.run_actual(0xbad60u,call);
        if(std::bit_cast<std::int16_t>(std::uint16_t(call.gp[0]))<=0)continue;
        Vec3 point{real(output+0x60u),real(output+0x64u),real(output+0x68u)};
        const float radius=real(entry+8u),scale=real(camera+4u);
        if(scale!=1.f)point={scale*point.x,scale*point.y,scale*point.z};
        const auto row3=[&](unsigned offset) {
            return real(camera+0x20u+offset)*point.z+real(camera+0x14u+offset)*point.y+
                real(camera+8u+offset)*point.x+real(camera+0x2cu+offset);
        };
        const std::array<float,2> lateral{row3(0),row3(4)};const float depth=row3(8);
        if(depth>radius)continue; // 167F80 rejects a sphere behind the light.
        std::array<float,4> bounds{};
        for(unsigned axis=0;axis<2;++axis) {
            const float distance=std::sqrt(lateral[axis]*lateral[axis]+depth*depth);
            if(!(radius<distance)){bounds[axis*2]=-10000.f;bounds[axis*2+1]=10000.f;continue;}
            const float spread=float(std::asin(double(radius)/double(distance)));
            const float centre=float(std::atan2(double(lateral[axis]),-double(depth)));
            const float low=centre-spread,high=centre+spread;
            bounds[axis*2]=std::bit_cast<float>(0xbfc90fdbu)>low?-10000.f:float(std::tan(double(low)));
            bounds[axis*2+1]=high>std::bit_cast<float>(0x3fc90fdbu)?10000.f:float(std::tan(double(high)));
        }
        // 30F8C..31014: clamp to the light camera's own frustum bounds (+A0).
        const std::array<float,4> frustum{real(camera+0xa0u),real(camera+0xa4u),real(camera+0xa8u),real(camera+0xacu)};
        if(!(bounds[0]>frustum[0]))bounds[0]=frustum[0];
        if(bounds[1]>frustum[1])bounds[1]=frustum[1];
        if(!(bounds[2]>frustum[2]))bounds[2]=frustum[2];
        if(bounds[3]>frustum[3])bounds[3]=frustum[3];
        // 494A0.
        const float width=frustum[1]-frustum[0],height=frustum[3]-frustum[2];
        const float left=1.f-(bounds[0]-frustum[0])/width,right=1.f-(bounds[1]-frustum[0])/width;
        const float bottom=(bounds[2]-frustum[2])/height,top=(bounds[3]-frustum[2])/height;
        const float scale_x=1.f/(right-left),scale_y=1.f/(top-bottom);
        if(!std::isfinite(scale_x) || !std::isfinite(scale_y))continue;
        out.push_back({object,{scale_x,scale_y,scale_x*left*-512.f,scale_y*bottom*-512.f}});
    }
    return out;
}
}
struct ProjectedLights::Impl {
    const Cache& cache;unsigned bsp;
    SourceBspAdmissionBsp geometry;
    std::shared_ptr<SharedLightMaterials> shared;
    PointCasterBank casters;
    MenuWorldSourceBindingCache bindings;
    std::map<std::uint32_t,EffectLightDefinition> lights;
    const Tag* cairo_master_guns_light{};
    const Tag* cairo_master_guns_face{};
    const Tag* cairo_helmet_prop{};
    // A resident slot owns immutable textures/vertex family/raster state.
    // Current light program/constants are supplied independently per packet.
    // Combined implementations do not bind a gel; other implementations share
    //residency only when their literal authored gel bitmap identity is equal.
    using Key=std::tuple<std::uint32_t,unsigned,unsigned,std::uint32_t>;
    struct Entry {unsigned slot;MenuWorldSourcePass pass;};
    std::map<Key,std::vector<Entry>> materials;
    // Source layer 11 implementation 2: the gel-to-alpha pass that precedes a
    // DST_ALPHA light response. Keyed like its layer-12 companion.
    std::map<Key,std::vector<Entry>> alpha_materials;
    // Source layer 10 casters into a light's shadow buffer (implementations
    // 1 and 2), keyed by shader, level and implementation only.
    std::map<Key,std::vector<Entry>> caster_materials;
    // Layer 14 rebuilds the texture target before the native light response.
    // Its authored texture mapping is independent of the particular light.
    std::map<Key,std::vector<Entry>> texture_materials;
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceBspRangeGeometry ranges;
    };
    using SectionKey=std::tuple<MenuWorldSourceSectionKind,std::uint32_t,unsigned>;
    std::map<SectionKey,Section> sections;
    std::vector<std::string> gaps;
    std::optional<std::array<ShaderVector,9>> previous;
    Impl(const Cache& c,Mesh& mesh,unsigned index,std::shared_ptr<SharedLightMaterials> bank)
      :cache(c),bsp(index),geometry(c.load_source_bsp_admission(index)),
       shared(bank?std::move(bank):std::make_shared<SharedLightMaterials>(c,mesh)),casters(c,index),bindings(c) {
        if(cache.scenario()=="scenarios\\solo\\00a_introduction\\00a_introduction") {
            cairo_master_guns_light=cache.query_tag({.group=TagGroup::from_string("ligh"),
                .name="scenarios\\objects\\lights\\cinematics\\x01_light_master_guns",.require_address=true});
            cairo_master_guns_face=cache.query_tag({.group=TagGroup::from_string("shad"),
                .name="objects\\characters\\marine\\shaders\\head_morgan",.require_address=true});
            cairo_helmet_prop=cache.query_tag({.group=TagGroup::from_string("mode"),
                .name="objects\\characters\\masterchief\\helmet\\helmet",.require_address=true});
        }
        for(const auto& tag:cache.tags())if(tag.group=="ligh") {
            const auto raw=cache.load_source_tag_metadata(tag.datum);
            const auto bytes=raw.bytes_at(raw.source_address(),228);const Bytes d(bytes);
            if(d.u16(4)!=2 || (d.u32(0)&(0x20u|0x80000u)) || d.u32(0xe0)!=0xffffffffu ||
               d.u32(0xc4) || d.u32(0xcc) || d.u32(0xd4))continue;
            EffectLightDefinition value;std::copy(bytes.begin(),bytes.end(),value.bytes.begin());
            value.flags=d.u32(0);value.radius=d.f32(0x18);value.specular_radius=d.f32(0x1c);
            value.duration=d.f32(0xb0);value.lens=d.u32(0x94);lights.emplace(tag.datum,std::move(value));
        }
        std::set<std::uint32_t> shaders;
        for(const auto& m:mesh.materials)if(m.source_shader_datum!=0xffffffffu)shaders.insert(m.source_shader_datum);
        EffectLightUniforms residency{}; // Residency seed only, never a live draw result.
        for(const auto shader:shaders) {
            try {
                const auto count=shared->authored(shader).lods.size();
                for(unsigned lod=0;lod<count;++lod) {
                    const auto authored=shared->selected(shader,lod);
                    if(!lights.empty())try {
                        auto prepared=prepare_projected_light_texture_material(cache,*authored,residency,true);
                        std::vector<Entry> entries;
                        for(unsigned pass=0;pass<prepared.size();++pass) {
                            auto selected=load_menu_world_source_pass(cache,shader,{lod,14,pass,0});
                            const unsigned slot=unsigned(mesh.materials.size())+pass;
                            (void)source_effect_light_draw(slot,prepared[pass],{});
                            entries.push_back({slot,std::move(selected)});
                        }
                        if(!entries.empty()) {
                            for(auto& material:prepared)mesh.materials.push_back(std::move(material));
                            texture_materials.emplace(Key{shader,lod,0u,0u},std::move(entries));
                        }
                    }catch(const FormatError& e) {
                        gaps.push_back(authored->name+"/"+std::to_string(lod)+"/layer 14: "+e.what());
                    }
                    for(const auto& [identity,definition]:lights) {
                    if(supports_projected_light_response_2003(definition))try {
                        shared->preload_specular(shader,lod,identity,definition);
                    }catch(const FormatError& e) {
                        gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/2003 response: "+e.what());
                    }
                    if(const Key k{shader,lod,2u,Bytes(definition.bytes).u32(0x7c)};!alpha_materials.contains(k))try {
                        auto prepared=prepare_projected_light_alpha_material(cache,*authored,definition,residency,2);
                        std::vector<Entry> entries;
                        for(unsigned pass=0;pass<prepared.size();++pass) {
                            auto selected=load_menu_world_source_pass(cache,shader,{lod,11,pass,2});
                            (void)source_effect_light_draw(unsigned(mesh.materials.size()),prepared[pass],{});
                            entries.push_back({unsigned(mesh.materials.size()),std::move(selected)});
                            mesh.materials.push_back(std::move(prepared[pass]));
                        }
                        alpha_materials.emplace(k,std::move(entries));
                    }catch(const FormatError& e) {
                        alpha_materials.emplace(k,std::vector<Entry>{});
                        gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/layer 11: "+e.what());
                    }
                    // A shadowed light's layer 11 comparison passes replace the
                    // gel pass; its casters use layer 10. Only lights that
                    // may cast (ligh flag bit 3 clear) need them resident.
                    if(!(definition.flags&8u)) {
                        for(unsigned implementation:{0u,1u,6u}) {
                            const Key k{shader,lod,implementation,Bytes(definition.bytes).u32(0x7c)};
                            if(alpha_materials.contains(k))continue;
                            try {
                                auto prepared=prepare_projected_light_shadow_material(cache,*authored,definition,residency,11,implementation);
                                std::vector<Entry> entries;
                                for(unsigned pass=0;pass<prepared.size();++pass) {
                                    auto selected=load_menu_world_source_pass(cache,shader,{lod,11,pass,implementation});
                                    (void)source_effect_light_draw(unsigned(mesh.materials.size()),prepared[pass],{});
                                    entries.push_back({unsigned(mesh.materials.size()),std::move(selected)});
                                    mesh.materials.push_back(std::move(prepared[pass]));
                                }
                                alpha_materials.emplace(k,std::move(entries));
                            }catch(const FormatError& e) {
                                alpha_materials.emplace(k,std::vector<Entry>{});
                                gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/layer 11 shadow "+std::to_string(implementation)+": "+e.what());
                            }
                        }
                        for(unsigned implementation:{1u,2u}) {
                            const Key k{shader,lod,implementation,0u};
                            if(caster_materials.contains(k))continue;
                            try {
                                auto prepared=prepare_projected_light_shadow_material(cache,*authored,definition,residency,10,implementation);
                                std::vector<Entry> entries;
                                for(unsigned pass=0;pass<prepared.size();++pass) {
                                    auto selected=load_menu_world_source_pass(cache,shader,{lod,10,pass,implementation});
                                    (void)source_effect_light_draw(unsigned(mesh.materials.size()),prepared[pass],{});
                                    entries.push_back({unsigned(mesh.materials.size()),std::move(selected)});
                                    mesh.materials.push_back(std::move(prepared[pass]));
                                }
                                caster_materials.emplace(k,std::move(entries));
                            }catch(const FormatError& e) {
                                caster_materials.emplace(k,std::vector<Entry>{});
                                gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/layer 10 "+std::to_string(implementation)+": "+e.what());
                            }
                        }
                    }
                    for(unsigned features:{0u,2u,4u,6u}) {
                        const auto implementation=projected_light_implementation(definition.flags,features,authored->shader_switches[4]!=0);
                        const auto k=key(shader,lod,implementation,definition);
                        if(materials.contains(k))continue;
                        try {
                            auto prepared=prepare_projected_light_material(cache,*authored,definition,residency,implementation,features);
                            std::vector<Entry> entries;
                            for(unsigned pass=0;pass<prepared.size();++pass) {
                                auto selected=load_menu_world_source_pass(cache,shader,{lod,12,pass,implementation});
                                (void)source_effect_light_draw(unsigned(mesh.materials.size()),prepared[pass],{});
                                entries.push_back({unsigned(mesh.materials.size()),std::move(selected)});
                                mesh.materials.push_back(std::move(prepared[pass]));
                            }
                            materials.emplace(k,std::move(entries));
                        }catch(const FormatError& e) {
                            gaps.push_back(std::to_string(identity)+"/"+authored->name+"/"+std::to_string(lod)+"/"+std::to_string(implementation)+": "+e.what());
                        }
                    }
                    }
                }
            }catch(const FormatError& e){gaps.push_back(std::to_string(shader)+": "+e.what());}
        }
    }
    static Key key(std::uint32_t shader,unsigned lod,unsigned implementation,const EffectLightDefinition& definition) {
        const auto gel=(implementation==2 || implementation==3)?Bytes(definition.bytes).u32(0x7c):0u;
        return {shader,lod,implementation,gel};
    }
    const Section& section(const SourceCullDrawUnit& unit) {
        MenuWorldSourceSectionKind kind;std::uint32_t model;unsigned ordinal;
        if(unit.words[1]!=0xffffffffu) {
            kind=MenuWorldSourceSectionKind::render_model;model=unit.words[1];ordinal=unit.words[3]>>9&511u;
        }else {
            model=0xffffffffu;const bool instance=unit.words[0]&0x1000u;
            kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            ordinal=instance?unsigned(geometry.instances.at(unit.words[3]>>18&2047u).definition):(unit.words[3]>>9&511u);
        }
        const SectionKey k{kind,model,ordinal};auto found=sections.find(k);
        if(found==sections.end()) {
            auto raw=std::make_shared<const MenuWorldSourceSection>(model!=0xffffffffu?
                load_menu_world_source_model_section(cache,model,ordinal):load_menu_world_source_bsp_section(cache,bsp,kind,ordinal));
            auto ranges=model!=0xffffffffu?source_model_range_geometry(*raw):source_bsp_range_geometry(*raw);
            found=sections.emplace(k,Section{std::move(raw),std::move(ranges)}).first;
        }
        return found->second;
    }
};
ProjectedLights::ProjectedLights(const Cache& cache,Mesh& mesh,unsigned bsp,std::shared_ptr<SharedLightMaterials> bank)
    :impl_(std::make_unique<Impl>(cache,mesh,bsp,std::move(bank))){}
ProjectedLights::~ProjectedLights()=default;
const std::vector<std::string>& ProjectedLights::unavailable()const{return impl_->gaps;}
ProjectedLightFrame ProjectedLights::draw(PhysicsLiveWorldTu15& world,const SourceEffectLightViewRecord& light,
    float selected_fade,const NativeLightReceiver& receiver,const NativeLightGeometry& geometry,
    const SourceViewProjectionBank& camera,Vec3 eye,std::array<std::int16_t,4> viewport,
    std::array<std::uint32_t,2> surface,std::vector<PointCaster>& point_casters,std::uint32_t domain,
    const std::array<ShaderVector,12>* submitting_constants,const PresentedModelPalette& presented_palette,
    bool original_alpha_layer,unsigned* shadow_targets,bool materialize_draws,bool materialize_point_casters,
    CampaignLightingPassTimings* timing) {
    using Phase=CampaignLightPhase;
    CampaignLightPhaseTimer setup_time(timing,Phase::projected_setup);
    auto& i=*impl_;const auto found=i.lights.find(light.definition_identity);
    const auto& definition=found->second;
    if(!i.previous) {
        i.previous.emplace();
        // Literal4A3BD destination. Preserve the real caller's unwritten words
        //in this optional renderer's own consecutive light-uniform bank.
        std::memcpy(i.previous->data(),world.memory().view(0x49219cu,sizeof(*i.previous)).data(),sizeof(*i.previous));
    }
    const auto uniforms=native_light_constants_tu15(world,light.runtime.identity,light.fades,selected_fade,*i.previous);
    i.previous=uniforms.constants;
    const auto shape=native_light_shape_tu15(world,light.runtime.identity);
    std::optional<EffectLightUniforms> response;
    static const bool reference_companions=[] {
        for(const auto* name:{"HALO2_REFERENCE_ORIGINAL_PROJECTED_SPECULAR",
            "HALO2_REFERENCE_PROJECTED_LIGHT_PREPARATION",
            "HALO2_REFERENCE_PROJECTED_SPECULAR_MATERIALS",
            "HALO2_PSTENCIL_CAMPAIGN_MATERIAL_TRACE"})
            if(const auto* value=std::getenv(name); value && *value)return true;
        return false;
    }();
    // These companion passes are consumed only by the pStencil renderer.
    // The ordinary path still builds its complete native light layers below.
    if((!original_alpha_layer || reference_companions) && supports_projected_light_response_2003(definition)) {
        const auto word=[&](unsigned p){return Bytes(world.memory().view(p,4)).u32(0);};
        const auto pool=word(0x4ecbdcu),records=word(pool+0x44u);
        const auto record=records+(light.runtime.identity&65535u)*272u;
        if(Bytes(world.memory().view(record,2)).u16(0)!=std::uint16_t(light.runtime.identity>>16))
            throw FormatError("2003 projected response lost the actual native light generation");
        std::array<float,16> frame;
        std::memcpy(frame.data(),world.memory().view(record+0x84u,sizeof(frame)).data(),sizeof(frame));
        std::array<ShaderVector,6> previous;
        std::copy_n(uniforms.constants.begin(),previous.size(),previous.begin());
        const auto adapted=projected_constants_2003(shape.words,frame,Bytes(definition.bytes).u16(0x8e),
            uniforms.constants[5][3],previous);
        response=uniforms;
        std::copy(adapted.begin(),adapted.end(),response->constants.begin());
    }
    const Vec3 origin{std::bit_cast<float>(shape.words[11]),std::bit_cast<float>(shape.words[12]),std::bit_cast<float>(shape.words[13])};
    MenuWorldSourceConstants constants{};
    const auto bank=submitting_constants?*submitting_constants:source_view_shader_bank(camera,1024.f,eye,
        {float(viewport[1]),float(viewport[0]),float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0])},false);
    std::copy(bank.begin(),bank.end(),constants.begin());
    setup_time.finish();
    if(materialize_draws && materialize_point_casters) {
        const CampaignLightPhaseTimer casters_time(timing,Phase::projected_casters);
        auto stencil_light=light.runtime;stencil_light.position=origin;
        i.casters.collect(geometry.retained.state,&world,stencil_light,constants,domain,{},point_casters,presented_palette);
    }
    // 31150: a light whose 31650 caller byte survives and whose caster bank
    // submitted layer 10 owns a shadow buffer (feature bit 2). The ordinary
    // renderer draws that buffer when the caller supplies target slots;
    // otherwise it selects both layers without the bit. The stencil mode
    // keeps the full mask for its own volumes.
    static const bool light_shadows=[]{const auto* value=std::getenv("HALO2_REFERENCE_NO_LIGHT_SHADOWS");return !(value && *value);}();
    const bool shadowed=original_alpha_layer && light_shadows && shadow_targets && *shadow_targets<shadow_target_limit &&
        receiver.shadow_enabled && (Bytes(world.memory().view(0x4ce2a0u,4)).u32(0)&0x400u);
    const auto select_layer=[&](unsigned layer,bool shadow,std::uint32_t extra_features=0) {
        return time_campaign_light_phase(timing,Phase::projected_layer_selection,[&] {
            return native_projected_light_selection_tu15(world,light.runtime.identity,receiver,layer,shadow,extra_features);
        });
    };
    const auto selected=select_layer(12,!original_alpha_layer || shadowed);
    ProjectedLightFrame out;out.features=selected.features;out.selected_commands=selected.records.size();out.color_shift=uniforms.color_shift;
    const auto& state=geometry.retained.state;
    const bool cairo_master_guns=i.cairo_master_guns_light &&
        i.cairo_master_guns_light->datum==light.definition_identity;
    static const bool reference_preparation=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTED_LIGHT_PREPARATION");
        return value && *value;
    }();
    static const bool reference_model_palettes=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTED_MODEL_PALETTES");
        return value && *value;
    }();
    static const bool reference_binding_inputs=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTED_BINDING_INPUTS");
        return value && *value;
    }();
    // Layer 12 is the response, 14 the texture write, 11 the alpha pass and
    // 10 the casters. block carries c40..48 for a shadowed light; target is its
    // depth target slot; only restricts cinematic re-application to one object.
    const auto emit=[&](const ProjectedLightSelection& selected,const unsigned layer,
        const LightShadowBlock* block=nullptr,const std::uint16_t target=0,const std::optional<std::uint32_t> only={}) {
    if(!materialize_draws)return;
    const CampaignLightPhaseTimer packet_time(timing,Phase::projected_packets);
    const bool alpha=layer==11,generate=layer==10,texture=layer==14;
    const auto& commands=geometry.commands.banks[generate?2:1];
    std::map<unsigned,EffectLightRawDraw> bindings;
    // This drain only reads the completed World. Reuse its pass and geometry
    // inputs here, before the next native selection can change current state.
    struct PreparedPass {
        SourceShadowRawPassOwnerTu15 owner;
        std::optional<MenuWorldSourceConstants> constants;
    };
    struct PreparedUnit {
        const Impl::Section* section{};
        std::optional<SourceModelCommandGeometry> live_model;
        std::optional<std::array<float,12>> instance_rows;
        // All parts of this completed unit use its own current c50 range.
        // Its palette and owner stay fixed during this read-only drain.
        std::optional<SourceLiveProjectedModelPaletteTu15> palette;
        std::optional<ObjectColors> colors;
    };
    std::map<const Impl::Entry*,PreparedPass> prepared_passes;
    std::map<unsigned,PreparedUnit> prepared_units;
    std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const std::vector<EffectLightDraw>>> specular_passes;
    std::shared_ptr<const std::vector<EffectLightDraw>> covered_response;
    std::vector<const SourceOpaqueSortRecord*> first_records;
    if(!reference_preparation) {
        first_records.resize(commands.storage.size());
        for(const auto& record:selected.records) {
            const auto command=std::uint16_t(record.words[0]);
            if(command<first_records.size() && !first_records[command])first_records[command]=&record;
        }
    }
    // Commands for different parts can share the same shader/LOD/pass. Keep
    // one completed metadata bank per light, with image residency independent.
    std::map<Impl::Key,std::vector<Material>> completed_materials;
    const auto ranges=source_effect_light_ordered_ranges(commands,state.units,selected.records,state.arena,0x559b54u,
        [&](unsigned command,unsigned unit_index,unsigned vertex_class) {
            const auto& c=commands.storage.at(command).words;const auto& unit=state.units.at(unit_index);
            PreparedUnit fresh_unit;
            auto& prepared_unit=reference_preparation?fresh_unit:prepared_units[unit_index];
            if(!prepared_unit.section)prepared_unit.section=&i.section(unit);
            const auto& section=*prepared_unit.section;const auto part=c[3]&255u;
            const auto* descriptor=[&]()->const SourceOpaqueSortRecord* {
                if(!reference_preparation)return first_records.at(command);
                const auto found=std::find_if(selected.records.begin(),selected.records.end(),
                    [&](const auto& r){return std::uint16_t(r.words[0])==command;});
                return found==selected.records.end()?nullptr:&*found;
            }();
            if(!descriptor)throw FormatError("Projected range lost its native selected command");
            const auto implementation=descriptor->words[1]&255u;
            const auto lod=c[5]>>8&255u,pass_index=c[5]&255u;
            if(only && (unit.words[1]==0xffffffffu || Bytes(world.memory().view(unit.words[4],4)).u32(0)!=*only))
                return SourceOpaqueRangeGeometry{&section.ranges,false};
            const auto& bank=texture?i.texture_materials:generate?i.caster_materials:alpha?i.alpha_materials:i.materials;
            const auto material=bank.find((generate || texture)?Impl::Key{c[2],lod,implementation,0u}:
                alpha?Impl::Key{c[2],lod,implementation,Bytes(definition.bytes).u32(0x7c)}:
                Impl::key(c[2],lod,implementation,definition));
            // An alpha implementation outside the admitted gel branch stays
            // undrawn; its light response then keeps the existing target alpha.
            if((alpha || generate) && (material==bank.end() || pass_index>=material->second.size()))
                return SourceOpaqueRangeGeometry{&section.ranges,false};
            if(material==bank.end() || pass_index>=material->second.size())
                throw FormatError("Native projected shader/LOD/pass was not admitted during residency: "+std::to_string(c[2])+"/"+std::to_string(lod)+"/"+std::to_string(implementation));
            const auto& slot=material->second[pass_index];
            // The Cache owns logical tag addresses; the completed native
            // descriptor owns this World's resident mapping. Authenticate the
            // complete current implementation and pass lineage at consumption.
            std::optional<PreparedPass> fresh_pass;
            auto& prepared_pass=[&]()->PreparedPass& {
                if(reference_preparation)return fresh_pass.emplace(PreparedPass{
                    source_shadow_raw_pass_owner_tu15(world,i.cache,slot.pass),{}});
                auto found=prepared_passes.find(&slot);
                if(found==prepared_passes.end())found=prepared_passes.emplace(&slot,PreparedPass{
                    source_shadow_raw_pass_owner_tu15(world,i.cache,slot.pass),{}}).first;
                return found->second;
            }();
            if(!prepared_pass.owner.matches(slot.pass,descriptor->words[4])) {
                std::ostringstream detail;
                detail<<"Native projected descriptor differs from its actual compiled implementation pointer"
                    <<" wanted={cache_mcc:"<<i.cache.is_mcc()<<",light:"<<light.runtime.identity
                    <<",light_definition:"<<light.definition_identity<<",features:"<<selected.features
                    <<",command:"<<command<<",unit:"<<unit_index<<",part:"<<part
                    <<",vertex_class:"<<vertex_class<<",shader:"<<slot.pass.shader()
                    <<",lod:"<<lod<<",layer:"<<layer<<",pass:"<<pass_index<<",implementation:"<<implementation
                    <<",template:"<<slot.pass.shader_template()<<",shader_pass:"<<slot.pass.shader_pass()
                    <<",vertex_shader:"<<slot.pass.vertex_shader()<<",template_pass:"<<slot.pass.template_pass()
                    <<",template_remap:"<<slot.pass.template_remap()<<",lod_address:"<<slot.pass.lod_address()
                    <<",layer_address:"<<slot.pass.layer_address()<<",template_pass_address:"<<slot.pass.template_pass_address()
                    <<",remap_address:"<<slot.pass.template_remap_address()
                    <<",pass_postprocess_address:"<<slot.pass.pass_postprocess_address()
                    <<",logical_implementation_address:"<<slot.pass.implementation_address()
                    <<",actual_implementation_address:"<<descriptor->words[4]<<"}";
                try {
                    const auto &expected=slot.pass.implementation_record();
                    const auto native=world.resident_cache_address(i.cache,slot.pass.implementation_address(),expected.size());
                    const auto &memory=world.memory();const auto address=memory.source_address(native,expected.size());
                    detail<<" resident={address:"<<address<<",matches_descriptor:"<<(address==descriptor->words[4])
                        <<",complete_preimage_equal:"<<std::ranges::equal(memory.view(native,expected.size()),expected)<<"}";
                }catch(const std::exception &error){detail<<" resident_error={"<<error.what()<<"}";}
                detail<<" descriptor={";
                for(unsigned n=0;n<descriptor->words.size();++n)detail<<(n?",":"")<<descriptor->words[n];
                detail<<"} command={";
                for(unsigned n=0;n<c.size();++n)detail<<(n?",":"")<<c[n];
                detail<<"} unit={";
                for(unsigned n=0;n<unit.words.size();++n)detail<<(n?",":"")<<unit.words[n];
                detail<<"}";
                throw FormatError(detail.str());
            }
            std::optional<SourceLiveProjectedModelPaletteTu15> fresh_palette;
            std::span<const std::array<float,12>> palette_rows;
            MenuWorldSourceOwner owner;
            if(unit.words[1]!=0xffffffffu) {
                auto& palette=reference_model_palettes?fresh_palette:prepared_unit.palette;
                if(reference_model_palettes) {
                    if(!prepared_unit.live_model)
                        prepared_unit.live_model=source_live_model_command_geometry_tu15(world,unit);
                    const auto& live=*prepared_unit.live_model;
                    if(!live.geometry.resident)return SourceOpaqueRangeGeometry{&section.ranges,false};
                    const auto object=Bytes(world.memory().view(unit.words[4],4)).u32(0);
                    (void)world.source_object_body(object);
                    auto rows=source_live_model_palette_rows_tu15(world,unit,live,
                        presented_palette?presented_palette(object,unit.words[1]):std::span<const std::array<float,12>>{});
                    palette.emplace(SourceLiveProjectedModelPaletteTu15{true,object,std::move(rows)});
                } else if(!palette) {
                    palette=source_live_projected_model_palette_tu15(world.memory(),unit,
                        [&](std::uint32_t object,std::uint32_t model) {
                            (void)world.source_object_body(object);
                            return presented_palette?presented_palette(object,model):std::span<const std::array<float,12>>{};
                        });
                }
                if(!palette->resident)return SourceOpaqueRangeGeometry{&section.ranges,false};
                palette_rows=palette->rows;
                owner={MenuWorldSourceOwnerKind::dynamic_object,section.raw->owner_datum(),
                    palette->object&65535u,palette->object};
            }else {
                const bool instance=unit.words[0]&0x1000u;const auto ordinal=instance?(unit.words[3]>>18&2047u):section.raw->section();
                owner={instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,section.raw->owner_datum(),ordinal,{}};
                if(instance) {
                    if(!prepared_unit.instance_rows)
                        prepared_unit.instance_rows=load_menu_world_source_bsp_owner(i.cache,i.bsp,ordinal).rows;
                    palette_rows=std::span(&*prepared_unit.instance_rows,1);
                }
            }
            auto completed_at=completed_materials.find(material->first);
            if(completed_at==completed_materials.end()) {
                const CampaignLightPhaseTimer material_time(timing,Phase::projected_material);
                const auto authored=i.shared->selected(c[2],lod);
                completed_at=completed_materials.emplace(material->first,complete_projected_light_packet_materials(
                    i.cache,*authored,definition,uniforms,layer,implementation,selected.features)).first;
            }
            const auto& completed=completed_at->second;
            if(pass_index>=completed.size())throw FormatError("Native projected material lost its selected pass");
            if(!prepared_pass.constants) {
                prepared_pass.constants=source_effect_light_raw_constants(constants,completed[pass_index]);
                if(block)std::copy(block->constants.begin(),block->constants.end(),prepared_pass.constants->begin()+40);
            }
            const auto topology=reference_preparation?
                source_effect_light_part_ranges(unit,section.ranges,part,state.arena,0x559b54u).topology:
                source_effect_light_part_topology(unit,section.ranges,part);
            auto draw=section.raw->kind()==MenuWorldSourceSectionKind::bsp_cluster?
                menu_world_source_cluster_draw(section.raw,part,topology):menu_world_source_draw(section.raw,part,topology,palette_rows);
            auto source=reference_binding_inputs?
                menu_world_source_binding_class(i.cache,owner,std::move(draw),slot.pass.vertex_shader(),
                    vertex_class,*prepared_pass.constants):
                i.bindings.bind(owner,std::move(draw),slot.pass.vertex_shader(),vertex_class,*prepared_pass.constants);
            MenuWorldRasterContext raster;
            raster.viewport_reset(generate?std::array<std::uint32_t,2>{512u,512u}:surface);raster.game_initialize_offset();
            if(generate)raster.shadow_begin(); // 44360 case 10 -> 495D0.
            raster.layer_begin(texture?MenuWorldRasterLayer::light_texture_14:generate?MenuWorldRasterLayer::shadow_10:
                alpha?MenuWorldRasterLayer::shadow_apply_11:MenuWorldRasterLayer::sphere_diffuse_12);
            raster.entry_begin(menu_world_raster_entry_from_source_flags(unit.words[0],false,false));
            auto packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
            packet.window_exclusive=false;for(auto& rect:packet.regions)rect={0,0,0,0};
            packet.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
            if(generate)packet.regions[0]={0,0,block->extent[0],block->extent[1]};
            const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
            auto bound=source_effect_light_raw_draw(slot.slot,completed[pass_index],slot.pass,std::move(source),packet,
                {0,16777215.f,depth.minimum_w,depth.maximum_w},{1,1});
            if(texture && owner.kind==MenuWorldSourceOwnerKind::dynamic_object) {
                if(!prepared_unit.colors) {
                    // 33980 gets the current owner's packed colors from the
                    // completed collector context. Unwritten externs use 468710.
                    const auto context=world.memory().view(unit.words[4],0x75u);
                    SourceLiveModelBaseDrawTu15 color_bank;
                    color_bank.color_count=std::to_integer<std::uint8_t>(context[0x74u]);
                    if(color_bank.color_count>color_bank.packed_colors.size())
                        throw FormatError("Projected texture colors exceed the original four-channel bank");
                    for(unsigned n=0;n<color_bank.color_count;++n)
                        color_bank.packed_colors[n]=Bytes(context).u32(0x64u+n*4u);
                    constexpr Vec3 white{1,1,1};
                    prepared_unit.colors=source_live_model_base_colors_tu15(color_bank,{white,white,white,white});
                }
                const auto& authored=completed[pass_index];
                apply_object_render_color_bindings(bound.program,bound.constants,
                    authored.passes.at(authored.forward_pass_index),*prepared_unit.colors,authored.shader_switches);
            }
            bound.light_identity=light.runtime.identity;bound.light_definition=light.definition_identity;
            bound.point_light_position=origin;bound.pstencil_light_domain=domain;
            bound.pstencil_casts_shadow=shadow_casting_part(section.raw->parts().at(part).type);
            if(generate){bound.shadow_generate=target;bound.shadow_extent=block->extent;}
            else if(alpha && implementation!=2)bound.shadow_sample=target;
            // Under this cinematic gel the recovered response overdrives the
            // face and adds a strong purple specular pattern to the visor.
            // Keep the authored native layer 12 sequence for these receivers.
            const bool authored_response=cairo_master_guns &&
                ((i.cairo_master_guns_face && c[2]==i.cairo_master_guns_face->datum) ||
                 (i.cairo_helmet_prop && unit.words[1]==i.cairo_helmet_prop->datum));
            if(layer==12 && response && !authored_response) {
                const auto complete_response=[&]{
                    const CampaignLightPhaseTimer material_time(timing,Phase::projected_material);
                    return i.shared->specular(c[2],lod,light.definition_identity,*response);
                };
                std::shared_ptr<const std::vector<EffectLightDraw>> response_passes;
                if(reference_preparation)response_passes=complete_response();
                else {
                    const auto key=std::pair{c[2],lod};
                    auto found=specular_passes.find(key);
                    if(found==specular_passes.end())found=specular_passes.emplace(key,complete_response()).first;
                    response_passes=found->second;
                }
                // A native combined implementation has a separate gel pass.
                // The recovered dependent7 already includes that gel, so
                // replace the entire native sequence exactly once.
                if(response_passes) {
                    if(!pass_index)bound.pstencil_specular=std::move(response_passes);
                    else if(reference_preparation)bound.pstencil_specular=std::make_shared<const std::vector<EffectLightDraw>>();
                    else {
                        if(!covered_response)covered_response=std::make_shared<const std::vector<EffectLightDraw>>();
                        bound.pstencil_specular=covered_response;
                    }
                }
            }
            bindings.emplace(command,std::move(bound));return SourceOpaqueRangeGeometry{&section.ranges,true};
        });
    static const bool reference_binding=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTED_LIGHT_RANGE_BINDING");
        return value && *value;
    }();
    // Retain the exact D24 depth candidates, including the base draw last.
    static const int original_depth_steps=[]{const auto* value=std::getenv("HALO2_REFERENCE_LIGHT_DEPTH_STEPS");
        return value && *value?std::clamp(std::atoi(value),0,8):2;}();
    const int depth_steps=original_alpha_layer && !generate && !texture?original_depth_steps:0;
    static const bool expand_depth_neighbours=[] {
        const auto* value=std::getenv("HALO2_REFERENCE_PROJECTED_DEPTH_NEIGHBOURS");return value && *value;
    }();
    if(!reference_preparation) {
        std::size_t packet_count=0;
        for(const auto& range:ranges)
            packet_count+=range.selected.ranges.size()*std::size_t(expand_depth_neighbours?1+2*depth_steps:1);
        const auto required=out.draws.size()+packet_count;
        if(required>out.draws.capacity())out.draws.reserve(std::max(required,out.draws.capacity()*2));
    }
    for(const auto& range:ranges) {
        const auto& original=bindings.at(range.command);
        for(const auto& indices:range.selected.ranges) {
            auto packet=original;
            if(reference_binding || packet.source.declaration().class_index!=range.vertex_class) {
                auto draw=packet.source.draw();draw.range=indices;
                auto bind=reference_binding_inputs?
                    menu_world_source_binding_class(i.cache,packet.source.owner(),std::move(draw),
                        packet.source.vertex_shader(),range.vertex_class,packet.source.constants()):
                    i.bindings.bind(packet.source.owner(),std::move(draw),packet.source.vertex_shader(),
                        range.vertex_class,packet.source.constants());
                packet.source=std::move(bind);
            } else packet.source.set_index_range(indices);
            // The raw pass tests EQUAL against separately rasterized base
            // depth. Neighbouring D24 steps recover its rounding differences.
            if(expand_depth_neighbours)for(int step=-depth_steps;step<=depth_steps;++step)if(step) {
                auto neighbour=packet;neighbour.raster.depth_factor_offset[1]+=float(step);
                out.draws.push_back(std::move(neighbour));
            }
            if(!expand_depth_neighbours)packet.depth_neighbour_steps=unsigned(depth_steps);
            out.draws.push_back(std::move(packet));
        }
    }
    };
    // 31150: the light's alpha and texture writes precede its response.
    if(shadowed) {
        CampaignLightPhaseTimer shadow_time(timing,Phase::projected_shadow_setup);
        const auto word=[&](unsigned p){return Bytes(world.memory().view(p,4)).u32(0);};
        const auto record=word(word(0x4ecbdcu)+0x44u)+(light.runtime.identity&65535u)*272u;
        const auto definition_tag=word(0x4f0414u)+(word(record+4u)&65535u)*16u;
        auto block=light_shadow_block(world.memory(),record,word(definition_tag+8u),shape,uniforms);
        shadow_time.finish();
        // Casters into this light's buffer, then its comparison into alpha.
        const auto target=std::uint16_t(++*shadow_targets);
        emit(select_layer(10,true),10,&block,target);
        emit(select_layer(11,true),11,&block,target);
        // 30EB0: a cinematic object's own buffer and three-tap comparison
        // replace the alpha just written on that object's commands.
        std::vector<CinematicLightShadow> cinematic;
        try {
            const CampaignLightPhaseTimer cinematic_time(timing,Phase::projected_shadow_setup);
            cinematic=cinematic_light_shadows(world,receiver);
        }
        catch(const FormatError& error) {
            if(i.gaps.empty() || i.gaps.back().find("cinematic shadow")==std::string::npos)
                i.gaps.push_back(std::string("cinematic shadow: ")+error.what());
        }
        for(const auto& object:cinematic) {
            if(*shadow_targets>=shadow_target_limit)break;
            auto zoomed=block;zoomed.constants[2]={512.f,512.f,16777215.f,1.f};zoomed.constants[8]=object.projection;
            zoomed.extent={512,512};
            const auto object_target=std::uint16_t(++*shadow_targets);
            emit(select_layer(10,true),10,&zoomed,object_target);
            emit(select_layer(11,true,0x48u),11,&zoomed,object_target,object.object);
        }
    } else if(original_alpha_layer && ((selected.features&6u) || (definition.flags&0x20u)))
        emit(select_layer(11,false),11);
    if(original_alpha_layer && materialize_draws && (geometry.commands.banks[1].header[2]&(1u<<14)))
        emit(select_layer(14,shadowed),14);
    emit(selected,12);
    return out;
}
}
