#include "halo2/source_scenery_scene.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/content.hpp"
#include "halo2/collision.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/scenery.hpp"
#include "halo2/scene.hpp"
#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include "halo2/appearance.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

using namespace halo2;
namespace {
void require(bool value,const std::string& message){if(!value)throw FormatError(message);}
float real(std::uint32_t x){return std::bit_cast<float>(x);}
std::uint32_t word(float x){return std::bit_cast<std::uint32_t>(x);}
std::array<std::uint32_t,33> vertex_words(const Vertex&v){
    // Compare fields rather than struct padding, without heap allocation for
    // each vertex in the Debug/sanitizer all-map run.
    return {word(v.position.x),word(v.position.y),word(v.position.z),
        word(v.color.x),word(v.color.y),word(v.color.z),word(v.uv.x),word(v.uv.y),v.material,
        word(v.lightmap_uv.x),word(v.lightmap_uv.y),word(v.normal.x),word(v.normal.y),word(v.normal.z),
        word(v.tangent.x),word(v.tangent.y),word(v.tangent.z),word(v.binormal.x),word(v.binormal.y),word(v.binormal.z),
        word(v.incident.x),word(v.incident.y),word(v.incident.z),v.lightmap,
        std::bit_cast<std::uint32_t>(v.bsp_cluster),word(v.prt_cluster),
        word(v.prt_weights[0]),word(v.prt_weights[1]),word(v.prt_weights[2]),word(v.prt_weights[3]),
        v.source_section,v.source_part,v.source_stipple_level};
}
bool same_texture(const Texture&a,const Texture&b){
    if(a.width!=b.width || a.height!=b.height || a.format!=b.format || a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels || a.mips.size()!=b.mips.size())return false;
    for(unsigned i=0;i<a.mips.size();++i){const auto &x=a.mips[i],&y=b.mips[i];if(x.width!=y.width || x.height!=y.height || x.offset!=y.offset || x.size!=y.size || x.depth!=y.depth)return false;}return true;
}
void same_material(const Material&a,const Material&b){
    require(a.source_shader_datum==b.source_shader_datum && a.selected_shader_lod==b.selected_shader_lod && a.forward_pass_index==b.forward_pass_index,
            "resident shader identity/index/pass differs from independently prepared source bank");
    require(a.forward_program.words==b.forward_program.words && a.forward_program.fragment_offsets==b.forward_program.fragment_offsets &&
        a.lightmap_program.words==b.lightmap_program.words && a.lightmap_program.fragment_offsets==b.lightmap_program.fragment_offsets &&
        a.vertex_program==b.vertex_program &&
        std::bit_cast<std::array<std::uint32_t,128>>(a.program_constants)==std::bit_cast<std::array<std::uint32_t,128>>(b.program_constants) &&
        a.texture_states==b.texture_states && a.forward_raster==b.forward_raster && a.forward_blend==b.forward_blend &&
        a.forward_vertex==b.forward_vertex && a.world_pass==b.world_pass && a.water_pass==b.water_pass &&
        a.per_vertex_lighting==b.per_vertex_lighting && a.baked_lightmap_texture==b.baked_lightmap_texture && a.accumulation_texture==b.accumulation_texture,
        "resident selected shader LOD changed program/constants/sampler/raster/category bank");
    require(same_texture(a.diffuse,b.diffuse) && same_texture(a.detail,b.detail) && same_texture(a.bump,b.bump) && same_texture(a.auxiliary,b.auxiliary),
        "resident selected shader LOD changed bitmap bytes/mips/dimensions");
    require(bool(a.environment_pass)==bool(b.environment_pass),"resident selected LOD changed accumulation admission");
    if(a.environment_pass)same_material(*a.environment_pass,*b.environment_pass);
    for(const auto &list:std::array<std::pair<const std::vector<std::shared_ptr<Material>>*,const std::vector<std::shared_ptr<Material>>*>,3>{{
        {&a.world_overlays_before,&b.world_overlays_before},{&a.world_overlays_after,&b.world_overlays_after},{&a.world_bloom_passes,&b.world_bloom_passes}}}){
        require(list.first->size()==list.second->size(),"resident selected LOD changed ordered child passes");
        for(unsigned i=0;i<list.first->size();++i)same_material(*list.first->at(i),*list.second->at(i));
    }
}
struct RawOwner {
    std::uint32_t definition{},mode{};unsigned implementation{};
    std::array<std::vector<unsigned>,6> collectors;
};
struct RawPart {std::array<std::uint32_t,18> words;std::uint32_t shader{};};
struct Cold {
    std::uint32_t definition{},flags{},scale{};
    std::array<std::uint32_t,13> node;
    std::array<std::uint32_t,4> sphere;
    std::array<std::uint32_t,3> velocity;
    unsigned control{},frozen{};int variant{};
};
struct Literal {
    std::map<std::pair<std::string,unsigned>,RawOwner> owners;
    std::map<std::tuple<std::string,std::uint32_t,unsigned,unsigned>,RawPart> parts;
    std::map<std::pair<std::string,unsigned>,Cold> cold;
    std::map<std::pair<std::string,std::uint32_t>,SourceShaderLodDefinition> shaders;
};
Literal literals(){
    Literal out;const auto folder=std::filesystem::path(__FILE__).parent_path()/"fixtures";
    std::ifstream input(folder/"retail_scenery_shader_parts.tsv");require(bool(input),"missing literal SCEN collector/part fixture");
    unsigned collectors=0;
    for(std::string line;std::getline(input,line);){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream in(line);char kind;std::string map;unsigned a,b;
        in>>kind>>map>>std::hex>>a>>b;
        if(kind=='O'){RawOwner owner;owner.definition=b;in>>owner.mode>>owner.implementation;
            require(out.owners.emplace(std::pair{map,a},std::move(owner)).second,"repeated raw SCEN owner");}
        else if(kind=='C'){unsigned count;in>>count;require(b<6,"raw SCEN geometry LOD out of range");
            auto& collector=out.owners.at({map,a}).collectors[b];for(unsigned i=0;i<count;++i){unsigned section;in>>section;collector.push_back(section);}++collectors;}
        else if(kind=='P'){unsigned ordinal;in>>ordinal;RawPart p;for(auto&w:p.words)in>>w;in>>p.shader;
            require(out.parts.emplace(std::tuple{map,a,b,ordinal},p).second,"repeated raw SCEN part");}
        else throw FormatError("unknown raw SCEN fixture kind");
        require(bool(in),"truncated raw SCEN collector/part fixture");
    }
    require(out.owners.size()==274&&out.parts.size()==219&&collectors==1644,"original SCEN literal census differs");
    std::ifstream bounds(folder/"retail_scenery_initial_bounds.tsv");require(bool(bounds),"missing original SCEN constructor sphere fixture");
    std::string name;while(bounds>>name){unsigned placement;Cold c;bounds>>placement>>c.definition>>c.flags>>c.scale;
        for(auto&w:c.node)bounds>>w;
        for(auto&w:c.sphere)bounds>>w;
        for(auto&w:c.velocity)bounds>>w;
        bounds>>c.control>>c.frozen>>c.variant;require(bool(bounds),"truncated original SCEN constructor state");
        require(out.cold.emplace(std::pair{name,placement},c).second,"duplicate original SCEN constructor owner");}
    require(bounds.eof()&&out.cold.size()==278,"original SCEN constructor fixture census differs");
    std::ifstream shaders(folder/"retail_source_shader_lod_assets.tsv");require(bool(shaders),"missing original shader threshold fixture");
    for(std::string line;std::getline(shaders,line);){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream in(line);std::string name;unsigned datum,bias,count;
        in>>name>>std::hex>>datum>>bias>>count;SourceShaderLodDefinition bank;bank.bias=std::uint16_t(bias);
        for(unsigned i=0;i<count;++i){unsigned bits;in>>bits;bank.thresholds.push_back(real(bits));}
        require(bool(in),"truncated original shader threshold fixture");
        require(out.shaders.emplace(std::pair{name,datum},std::move(bank)).second,"duplicate original shader definition");
    }
    require(out.shaders.size()==6450,"original shader definition fixture census differs");return out;
}
struct CameraFixture {SourceModelShaderView shader;};
CameraFixture camera(Vec3 eye,unsigned players,unsigned index){
    const auto grid=source_view_grid(players,index,false);
    const auto angles=source_camera_angles(1.2217304706573486f,1.3333333730697632f,grid.grid,grid.span);
    const int width=640/grid.grid[0],height=480/grid.grid[1];SourceFrustumProjectionInput p;
    p.projection_angle=angles.projection_angle;p.outer={std::int16_t(grid.position[1]*height),std::int16_t(grid.position[0]*width),
        std::int16_t((grid.position[1]+grid.span[1])*height),std::int16_t((grid.position[0]+grid.span[0])*width)};
    // Explicit source features: safe==outer, feature/global overrides off.
    p.safe=p.outer;const auto projection=source_frustum_projection(p);
    const auto bank=source_view_projection({eye,{1,0,0},{0,0,1},.1f,1024.f,{},projection.projection_program});
    SourceModelShaderView shader;shader.camera=source_shader_coverage_camera(bank);
    shader.shader=source_shader_lod_view(p.outer[0],p.outer[2],false,false);
    shader.object_coverage_multiplier=source_geometry_lod_policy(players).coverage_multiplier;
    shader.part_shader_multiplier=source_shader_lod_player_multiplier(players);shader.view_modifier=projection.view_modifier;
    return {shader};
}
struct Submission {std::vector<Vertex> vertices;std::vector<ObjectLightingBinding> bindings;std::vector<SourceObjectViewSelection> draws;};
struct Stats {unsigned owners{},baked{},dynamic{},views{},parts{},lower{},high{},empty_geometry{},suppressed_parts{},banks{},different_views{},baked_vertices{};};
Stats verify(const std::filesystem::path&path,const Literal&literal){
    Cache cache(path);const auto name=cache.name();auto content=cache.load_content(0,true);
    // The untouched 274-owner corpus describes automatic initial MP2 owners.
    for(auto &o:content.objects)if(o.type==6 && (!scenery_automatic_creation_allowed(o,2) || o.origin_bsp>0))o.definition=null_datum;
    const auto lighting=cache.load_bsp_lighting(0);CollisionWorld collision(content.collision);
    std::map<std::size_t,const RawOwner*> owners;std::map<std::size_t,const Cold*> cold;
    std::map<std::size_t,SourceInitialModelSelection> selections;std::map<std::size_t,ObjectColors> colors;
    std::map<std::size_t,std::map<unsigned,std::uint32_t>> source_materials;
    for(std::size_t i=0;i<content.objects.size();++i){const auto&o=content.objects[i];
        const auto raw=literal.owners.find({name,unsigned(o.placement)});
        if(o.type!=6||!scenery_automatic_creation_allowed(o,2)||o.origin_bsp>0)continue;
        const auto selection=cache.load_initial_model_selection(o.definition,o.variant);
        if(!selection.resolved()){require(name=="zanzibar"&&o.placement>=19&&o.placement<=22,"unexpected unresolved SCEN initial state");continue;}
        require(raw!=literal.owners.end()&&raw->second.definition==o.definition,"SCEN resident owner absent from untouched collector fixture");
        const auto& state=literal.cold.at({name,unsigned(o.placement)});
        require(state.definition==o.definition&&state.scale==word(o.scale)&&state.control==0&&state.frozen==1&&
            state.velocity==std::array<std::uint32_t,3>{0,0,0}&&state.variant==selection.variant_index,
            "SCEN original constructor identity/control/variant differs");
        owners.emplace(i,&raw->second);cold.emplace(i,&state);selections.emplace(i,selection);
        const auto definition=cache.load_initial_model_definition(o.definition);
        const auto variant=selection.variant_index<0?0u:definition.variants.at(unsigned(selection.variant_index)).name;
        colors.emplace(i,source_object_initial_colors(cache.load_source_object_color_definition(o.definition),o.position,variant));
        for(unsigned lod=0;lod<6;++lod){
            require(source_initial_model_sections(definition,selection,lod)==raw->second.collectors[lod],"native SCEN selected collector differs from untouched original");
            const auto model=cache.load_model(o.definition,false,o.variant,selection,lod);
            require(model.vertex_origins.size()==model.mesh.vertices.size(),"SCEN source vertex ordinal bank missing");
            for(unsigned v=0;v<model.mesh.vertices.size();++v){const auto&origin=model.vertex_origins[v];
                const auto&p=literal.parts.at({name,raw->second.mode,origin.section,origin.part});
                require(model.mesh.vertices[v].material==(p.words[1]&0xffffu),
                    name+" owner"+std::to_string(o.placement)+" section"+std::to_string(origin.section)+" part"+std::to_string(origin.part)+
                    " SCEN source material "+std::to_string(model.mesh.vertices[v].material)+" differs from literal "+std::to_string(p.words[1]&0xffffu));
                const auto [it,fresh]=source_materials[i].emplace(model.mesh.vertices[v].material,p.shader);
                require(fresh||it->second==p.shader,"one source material table ordinal resolved to different original shaders");}
        }
    }
    Mesh mesh;SourceSceneryScene scene(cache,content.objects,0,mesh,collision,{},
        [&](std::size_t i,const ScenarioObject&o){require(!owners.contains(i)&&name=="zanzibar"&&o.placement>=19&&o.placement<=22,
            "resolved owner escaped through omission service");return true;});
    SceneObjects objects(content,0,SourceSceneGameContext{2,2});scene.synchronize(objects);std::map<std::size_t,unsigned> reads;
    scene.animation_frame([&](std::size_t i,std::uint32_t handle,std::uint64_t born,std::uint64_t tick,const AnimationClip&clip){
        require(objects.get(handle).placement==i&&born==0&&tick==0&&clip.frame_count>1,"SCEN explicit cold controller frame lost source owner");return 0.f;});
    scene.lighting_query_state([&](std::size_t i,std::uint32_t handle,const Transform&node,float scale){
        const auto&c=*cold.at(i);require(objects.get(handle).placement==i&&word(scale)==c.scale,"SCEN cached sphere service lost actual owner/scale");
        require(std::bit_cast<std::array<std::uint32_t,13>>(node)==c.node,"SCEN cold node0 differs from independently retained constructor");++reads[i];
        return SourceSceneryQueryState{{real(c.sphere[0]),real(c.sphere[1]),real(c.sphere[2])},real(c.sphere[3]),{},c.flags&0x2000u,true};
    });scene.update_pose(0,0,mesh);
    Stats stats;stats.owners=scene.stats().render_models;stats.baked=scene.stats().baked_models;stats.dynamic=scene.stats().dynamic_models;
    require(stats.owners==owners.size(),"SCEN resident rendered owner census differs");
    // Independent banks start from actual raw shader/LOD, the source owner
    // class and original-verified initial color helper, not resident materials.
    std::map<std::tuple<std::size_t,std::uint32_t,unsigned>,Material> prepared;
    std::vector<const Material*> all_expected;
    for(const auto&[i,owner]:owners){const auto&object=content.objects[i];
        const auto model=cache.load_model(object.definition,false,object.variant,selections.at(i),5);
        const auto implementation=scenery_lighting_implementation(lighting,model,object,*cache.load_scenery_definition(object.definition));
        require(unsigned(implementation)==owner->implementation,"SCEN source owner baked/dynamic class differs");
        for(const auto&[source,shader]:source_materials.at(i)){
            const auto&bank=literal.shaders.at({name,shader});(void)source;
            const auto decoded=cache.load_source_shader_lod_definition(shader);
            require(decoded.bias==bank.bias&&decoded.thresholds.size()==bank.thresholds.size(),"SCEN decoded shader definition differs from literal bank");
            for(unsigned j=0;j<bank.thresholds.size();++j)require(word(decoded.thresholds[j])==word(bank.thresholds[j]),"SCEN raw shader threshold float differs");
            for(unsigned lod=0;lod<bank.thresholds.size();++lod){auto [it,fresh]=prepared.try_emplace({i,shader,lod});
                if(fresh){it->second=cache.load_material(shader);it->second.selected_shader_lod=lod;
                    require(owner->implementation==1?prepare_world_material(cache,it->second,true):prepare_dynamic_material(cache,it->second,{},true),
                        "SCEN literal selected source material has unsupported binding");apply_object_colors(it->second,colors.at(i));}
                all_expected.push_back(&it->second);
            }
        }
    }
    require(all_expected.size()==mesh.materials.size(),"SCEN resident alternative union omitted/additional actual part material bank");
    std::vector<bool> matched(mesh.materials.size());for(const auto*want:all_expected){bool found=false;
        for(unsigned n=0;n<mesh.materials.size();++n)if(!matched[n]){const auto&actual=mesh.materials[n];
            if(actual.source_shader_datum!=want->source_shader_datum||actual.selected_shader_lod!=want->selected_shader_lod||
                actual.per_vertex_lighting!=want->per_vertex_lighting||actual.forward_program.words!=want->forward_program.words)continue;
            same_material(actual,*want);matched[n]=true;found=true;break;}
        require(found,"SCEN resident union lost an independently prepared owner/color/selected bank");++stats.banks;}
    auto submit=[&](unsigned players,unsigned view,float geometry_metric,const std::optional<SourceModelShaderView>&shader){
        Submission out;SourceObjectViewLod v;v.view_index=view;v.local_players=players;v.shader=shader;
        v.input=[&](std::size_t i,std::uint32_t handle,Vec3 center,float radius){
            const auto&state=*cold.at(i);require(objects.get(handle).placement==i&&word(center.x)==state.sphere[0]&&
                word(center.y)==state.sphere[1]&&word(center.z)==state.sphere[2]&&word(radius)==state.sphere[3],"SCEN view substituted an inferred sphere");
            return SourceObjectLodInput{geometry_metric,state.flags,0};};
        scene.append_view(out.vertices,out.bindings,v,&out.draws);return out;};
    require(!cache.spawns().empty(),"SCEN view fixture has no authored spawn");
    Vec3 eye=cache.spawns().front().position;eye.z+=.35f;
    std::set<std::tuple<std::size_t,std::uint32_t,unsigned,unsigned>> checked_banks;
    auto check=[&](const Submission&base,const Submission&actual,const SourceModelShaderView&view,unsigned players){
        require(base.draws.size()==owners.size()&&actual.draws.size()==base.draws.size(),"SCEN per-view collector owner list differs");
        std::size_t before=0,after=0;unsigned binding=0;
        for(unsigned d=0;d<base.draws.size();++d){const auto&b=base.draws[d];const auto&a=actual.draws[d];
            require(a.source_object==b.source_object&&a.lod==b.lod&&a.transition==b.transition&&a.source_distance==b.source_distance,
                "shader camera altered geometryLOD/history/source owner");const auto&owner=*owners.at(a.source_object);const auto&state=*cold.at(a.source_object);
            const auto coverage=source_shader_object_coverage({real(state.sphere[0]),real(state.sphere[1]),real(state.sphere[2])},real(state.sphere[3]),view.object_coverage_multiplier,view.camera);
            const auto collector=source_shader_coverage_byte(coverage);
            std::map<std::pair<unsigned,unsigned>,SourceShaderLodChoice> choices;
            for(unsigned j=0;j<b.vertices;++j){const auto&v=base.vertices.at(before+j);const auto key=std::pair{v.source_section,v.source_part};
                if(choices.contains(key))continue;
                const auto&p=literal.parts.at({name,owner.mode,key.first,key.second});
                float scale=real(p.words[11]);if(!(scale>0))scale=1.f;
                const auto metric=source_shader_part_metric(collector,scale,view.part_shader_multiplier);
                const auto choice=source_shader_lod_choice(literal.shaders.at({name,p.shader}).thresholds,literal.shaders.at({name,p.shader}).bias,metric,view.shader,view.view_modifier);
                choices.emplace(key,choice);++stats.parts;if(!choice.accepted||choice.index<0)++stats.suppressed_parts;else if(choice.index)++stats.lower;else ++stats.high;
            }
            unsigned emitted=0;const std::string retained_error=name+" SCEN selected shader changed retained vertex/light/part fields";
            for(unsigned j=0;j<b.vertices;++j){auto want=base.vertices.at(before+j);const auto&choice=choices.at({want.source_section,want.source_part});
                if(!choice.accepted||choice.index<0)continue;
                const auto&p=literal.parts.at({name,owner.mode,want.source_section,want.source_part});
                const auto&v=actual.vertices.at(after+emitted);const auto&m=mesh.materials.at(v.material);
                require(m.source_shader_datum==p.shader&&m.selected_shader_lod==unsigned(choice.index),"SCEN per-part selected shader identity/LOD differs from original bank composition");
                if(checked_banks.emplace(a.source_object,p.shader,unsigned(choice.index),v.material).second)
                    same_material(m,prepared.at({a.source_object,p.shader,unsigned(choice.index)}));
                want.material=v.material;
                require(vertex_words(want)==vertex_words(v),retained_error);++emitted;
            }
            require(emitted==a.vertices,"SCEN rejected/empty shader parts retained vertices");
            if(!b.vertices)++stats.empty_geometry;
            if(owner.implementation==2&&a.vertices){require(binding<actual.bindings.size(),"SCEN dynamic selected shader lost per-owner binding");
                const auto&bank=actual.bindings[binding++];
                for(unsigned j=0;j<a.vertices;++j){auto id=actual.vertices.at(after+j).material;require(id>=bank.first_material&&id<bank.first_material+bank.material_count,
                    "SCEN selected alternative escaped its owner per-view lighting bank");}
            }
            if(owner.implementation==1){const auto&object=content.objects.at(a.source_object);const auto model=cache.load_model(object.definition,false,object.variant,selections.at(a.source_object),a.lod);
                const auto ref=std::find_if(lighting.scenery.begin(),lighting.scenery.end(),[&](const auto&r){return r.unique_id==object.unique_id&&r.type==object.type&&r.source==object.source&&r.checksum==model.checksum&&(r.source!=0||r.origin_bsp==object.origin_bsp);});
                require(ref!=lighting.scenery.end(),"SCEN selected baked reference missing");const auto&bucket=lighting.buckets.at(unsigned(ref->reference.bucket));
                for(unsigned j=0;j<b.vertices;++j){const auto&origin=model.vertex_origins.at(j);const auto offset=ref->reference.section_offsets.at(origin.section);
                    require(offset!=0xffff,"SCEN original section baked offset missing");const auto source=std::size_t(offset)+origin.vertex;const auto&v=base.vertices.at(before+j);
                    const auto incident=decode_bsp_incident(bucket.incident.at(source)),rgb=decode_bsp_radiance(bucket.radiance.at(source));
                    require(word(v.incident.x)==word(incident.x)&&word(v.incident.y)==word(incident.y)&&word(v.incident.z)==word(incident.z)&&
                        word(v.color.x)==word(rgb.x)&&word(v.color.y)==word(rgb.y)&&word(v.color.z)==word(rgb.z),"SCEN literal authored section incident/RGB changed");++stats.baked_vertices;}
            }
            before+=b.vertices;after+=a.vertices;
        }
        require(before==base.vertices.size()&&after==actual.vertices.size()&&binding==actual.bindings.size(),"SCEN per-view emitted unauthored commands/bindings");
        require(base.bindings.size()==actual.bindings.size(),"SCEN shader selector changed owner lighting admission in supplied banks");
        for(unsigned j=0;j<base.bindings.size();++j)require(base.bindings[j].state.source_values()==actual.bindings[j].state.source_values(),"later shader view replaced first owner light cache");
        ++stats.views;(void)players;
    };
    for(unsigned players:{1u,2u,4u}){
        // Geometry metric is an explicit fixed consumer input, keeping actual
        // geometry collectors identical while source camera shader detail moves.
        const auto base=submit(players,0,0,{});const auto near=camera(eye,players,0).shader;auto far_eye=eye;far_eye.x-=100000;
        const auto far=camera(far_eye,players,std::min(1u,players-1)).shader;
        const auto high=submit(players,0,0,near);check(base,high,near,players);
        const auto saved=high;const auto low=submit(players,std::min(1u,players-1),0,far);check(base,low,far,players);
        const auto again=submit(players,0,0,near);check(base,again,near,players);
        require(saved.vertices.size()==again.vertices.size(),"SCEN later view mutated retained first view storage");bool differs=false;
        for(unsigned i=0;i<saved.vertices.size();++i)require(vertex_words(saved.vertices[i])==vertex_words(again.vertices[i]),"SCEN repeated first view changed shader/geometry/material bindings");
        for(unsigned i=0;i<std::min(high.vertices.size(),low.vertices.size());++i)if(high.vertices[i].material!=low.vertices[i].material){differs=true;break;}
        if(differs)++stats.different_views;
        // Empty lower geometry collector commands are checked using their
        // actual source list. No synthetic empty shader bank is introduced.
        const auto empty_base=submit(players,0,100000,{});const auto empty=submit(players,0,100000,far);check(empty_base,empty,far,players);
    }
    for(const auto&[index,count]:reads)require(count==1,"SCEN source cached sphere reread during isolated shader views");
    const auto read_snapshot=reads;for(const auto&o:objects.objects())objects.visible(o.handle,false);scene.synchronize(objects);scene.update_pose(0,1,mesh);
    const auto hidden=submit(1,0,0,camera(eye,1,0).shader);require(hidden.vertices.empty()&&hidden.bindings.empty()&&hidden.draws.empty()&&reads==read_snapshot,
        "hidden SCEN queried/submitted an alternative shader bank");
    std::cout<<name<<": "<<stats.owners<<" owners, "<<stats.banks<<" resident banks, "<<stats.views<<" shader views, "<<stats.lower
        <<" lower / "<<stats.high<<" highest selected parts, "<<stats.empty_geometry<<" empty collectors, "<<stats.suppressed_parts<<" rejected shader parts, "
        <<stats.different_views<<" distinct same-geometry view pairs, "<<stats.baked_vertices<<" literal incident/RGB vertices\n";return stats;
}
}
int main(int argc,char**argv){try{
    const auto literal=literals();Stats total;
    const auto add=[&](const std::filesystem::path&p){const auto s=verify(p,literal);total.owners+=s.owners;total.baked+=s.baked;total.dynamic+=s.dynamic;total.views+=s.views;
        total.parts+=s.parts;total.lower+=s.lower;total.high+=s.high;total.empty_geometry+=s.empty_geometry;total.suppressed_parts+=s.suppressed_parts;
        total.banks+=s.banks;total.different_views+=s.different_views;total.baked_vertices+=s.baked_vertices;};
    if(argc==2)add(argv[1]);else {
        for(const auto*name:{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"})add(std::filesystem::path("assets/disc/maps")/(std::string(name)+".map"));
        for(const auto*name:{"containment","warlock","deltatap","turf","backwash","dune","elongation","gemini","triplicate"})add(std::filesystem::path("assets/map-pack/maps")/(std::string(name)+".map"));
        require(total.owners==274&&total.baked==196&&total.dynamic==78,"all-map SCEN class/owner census differs");
        require(total.banks==789&&total.views==252&&total.lower==2796&&total.high==3012&&total.different_views==50&&
            total.baked_vertices==792954,"SCEN literal camera/part/bank fixture summary changed");
        require(total.empty_geometry==0&&total.suppressed_parts==0,"supplied nonempty SCEN source bank gained an omitted collector/part");
    }
    std::cout<<"SCEN shader union: "<<total.owners<<" owners ("<<total.baked<<" baked / "<<total.dynamic<<" dynamic), "<<total.banks<<" resident banks, "<<total.views
        <<" views, "<<total.lower<<" lower / "<<total.high<<" highest selected parts, "<<total.empty_geometry<<" empty collectors, "<<total.suppressed_parts
        <<" rejected shader parts, "<<total.different_views<<" distinct same-geometry view pairs, "<<total.baked_vertices<<" literal incident/RGB vertices\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
