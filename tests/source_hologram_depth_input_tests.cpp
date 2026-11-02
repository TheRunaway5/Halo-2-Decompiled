#include "halo2/source_hologram_depth_input.hpp"
#include "halo2/model.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/world_material.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/render_view.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>

using namespace halo2;
namespace {
unsigned controls{},attempted{};
void need(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void reject(F&& fn) {
    ++attempted;
    try{fn();}catch(const FormatError&){++controls;return;}
    throw FormatError("Foreign hologram depth input admitted at control"+std::to_string(attempted));
}
std::vector<const Material*> children(const Material& m) {
    std::vector<const Material*> out{&m};
    if(m.environment_pass)out.push_back(m.environment_pass.get());
    for(const auto& p:m.world_overlays_before)out.push_back(p.get());
    for(const auto& p:m.world_overlays_after)out.push_back(p.get());
    for(const auto& p:m.world_bloom_passes)out.push_back(p.get());
    return out;
}
}
int main(int argc,char**argv)try {
    need(argc==3,"Supply original introduction and retained original group command fixture");
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);std::ifstream file(argv[2]);const auto fixture=nlohmann::json::parse(file);
    need(fixture.at("schema")=="opening-native-group-command-join-v1","Original group fixture schema differs");
    const auto& g=fixture.at("group");SourceLiveTransparentGroupTu15 command;
    command.section=g.at("section");command.flags=g.at("flags");command.first_command=g.at("first_command");
    command.end_command=g.at("end_command");command.layers=g.at("layers");command.shader_clock=g.at("shader_clock");
    command.queue_command.callback=0x423b0u;const std::array payload{command.section,command.flags};
    std::memcpy(command.queue_command.payload.data(),payload.data(),sizeof(payload));
    for(const auto& row:g.at("commands"))command.commands.push_back({row.get<std::array<std::uint32_t,6>>()});
    for(const auto& row:g.at("ordered_layers"))command.ordered_layers.push_back({row.at("layer"),
        row.at("descriptors").get<std::vector<std::array<std::uint32_t,6>>>()});
    std::vector<SourceLiveModelBaseDrawTu15> units;std::vector<std::uint32_t> definitions;
    auto ranges=std::make_shared<SourceLiveTransparentGroupGeometryTu15>();ranges->units.resize(153);
    for(const auto& row:fixture.at("units")) {
        auto& unit=units.emplace_back();unit.unit.words=row.at("words").get<std::array<std::uint32_t,8>>();
        unit.ordinal=row.at("ordinal");unit.object=row.at("object");unit.definition=row.at("definition");
        unit.override_slot=row.at("override_slot");unit.shader_override=row.at("shader_override");
        unit.lighting_class=unit.unit.words[0]>>29;
        // The retained fixture has no rendered pose/LOD observation. These
        // explicit component operands exercise raw skin ownership, not a live
        // Xbox pose or natural geometry-LOD selection claim.
        unit.lod=0;unit.palette_rows.resize(40);
        for(unsigned n=0;n<40;++n)unit.palette_rows[n]={1,0,0,n*.03125f,0,1,0,-n*.015625f,0,0,1,n*.0078125f};
        ranges->units.at(unit.ordinal)=unit.unit;
        const auto raw=load_menu_world_source_model_section(cache,unit.unit.words[1],unit.unit.words[3]>>9&511u);
        unit.geometry.geometry.resident=true;
        for(const auto& part:raw.parts()) {
            unit.geometry.geometry.parts.push_back(part.bytes);
            if(part.material>=0) {
                if(unsigned(part.material)>=unit.geometry.materials.size())unit.geometry.materials.resize(unsigned(part.material)+1);
                unit.geometry.materials[unsigned(part.material)].shader=part.shader;
            }
        }
        definitions.push_back(unit.definition);
    }
    command.geometry=ranges;
    SourceTransparentGroupPartBindings pass_owner(cache);const auto resolved=pass_owner.resolve(command,units);
    std::ranges::sort(definitions);definitions.erase(std::unique(definitions.begin(),definitions.end()),definitions.end());
    Mesh residency;SourceHologramGroupBindings binder(cache,residency,definitions);
    MenuWorldSourceConstants constants{};constants[0]={1,0,0,0};constants[1]={0,1,0,0};
    constants[2]={0,0,1,0};constants[3]={0,0,1,1};
    // Explicit component pose. It is distinct from the captured group's
    // geometry-only fixture, which contains no live camera observation.
    const SourceHologramCameraPose submitted{{0,0,0},{0,0,1}};
    constants[7]={submitted.position.x,submitted.position.y,submitted.position.z,767.8125f};
    const auto viewport=viewport_constants({0,0,640,480});std::copy(viewport.begin(),viewport.end(),constants.begin()+8);
    MenuWorldRasterContext raster;raster.viewport_reset({640,480});raster.game_initialize_offset();
    raster.layer_begin(MenuWorldRasterLayer::hologram_21);raster.entry_begin({});
    const auto packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
        std::array<std::int32_t,2>{1,1},true});
    auto planned=binder.draw(std::span{&command,1},std::span{&resolved,1},units,constants,packet,
        {0,16777215,.01f,16777215.f},{1,1},{640,480},submitted);
    need(planned.size()==1,"Original group depth fixture lost its current group");
    auto& group=planned.front();
    reject([&]{auto pose=submitted;pose.position.x+=1;(void)binder.draw(std::span{&command,1},std::span{&resolved,1},units,
        constants,packet,{0,16777215,.01f,16777215.f},{1,1},{640,480},pose);});
    reject([&]{auto pose=submitted;pose.forward.x=std::numeric_limits<float>::quiet_NaN();
        (void)binder.draw(std::span{&command,1},std::span{&resolved,1},units,
            constants,packet,{0,16777215,.01f,16777215.f},{1,1},{640,480},pose);});
    unsigned checked_vertices{},reordered_triangles{},draw_checks{};
    for(const auto object:std::vector(group.owners)) {
        std::vector<Vertex> input;std::set<std::pair<unsigned,unsigned>> parts;
        for(const auto& p:group.parts)if(p.layer!=21 && p.object==object && parts.emplace(p.section,p.part).second) {
            need(p.body_source && p.body_geometry && &p.body_geometry->cache_owner()==&cache &&
                p.body_geometry->lod()==p.geometry_lod,"Body lost sealed Cache/LOD/origin witness");
            const auto& model=p.body_geometry->model();
            for(std::size_t n=0;n<model.vertex_origins.size();++n) {
                const auto& origin=model.vertex_origins[n];
                if(origin.section==p.section && origin.part==p.part) {
                    auto vertex=model.mesh.vertices[n];vertex.position={99,-17,22};
                    input.push_back(vertex);
                }
            }
        }
        auto depth=source_hologram_depth_input(group,object,input,{640,480});depth.validate_vertices(depth.vertices());
        need(depth.vertices().size()==input.size(),"Native group depth dropped source geometry");
        checked_vertices+=unsigned(input.size());
        for(const auto& p:group.parts)if(p.layer!=21 && p.object==object) {
            const auto& raw=*p.body_source;
            const auto binding=menu_world_source_binding_class(cache,
                {MenuWorldSourceOwnerKind::dynamic_object,raw.geometry->owner_datum(),object&65535u,object},
                raw,p.pass->vertex_shader(),p.vertex_class,constants);
            const auto native=pstencil::complete_world_positions(binding);
            std::vector<std::size_t> actual;
            for(std::size_t n=0;n<depth.vertices().size();++n)
                if(depth.vertices()[n].source_section==p.section && depth.vertices()[n].source_part==p.part)actual.push_back(n);
            std::size_t at{};
            for(std::size_t t=0;t<raw.triangle_count();++t) {
                const auto triangle=raw.triangle(t);
                if(triangle[0]==triangle[1] || triangle[0]==triangle[2] || triangle[1]==triangle[2])continue;
                for(unsigned lane=0;lane<3;++lane) {
                    const auto index=actual.at(at+lane);
                    need(depth.source_vertices()[index]==triangle[lane] &&
                        !std::memcmp(&depth.vertices()[index].position,&native.at(triangle[lane]),sizeof(Vec3)),
                        "Native group depth changed original primitive order or exact native prefix");
                }
                bool reordered=false;
                for(unsigned lane=0;lane<3;++lane) {
                    const auto& value=depth.vertices()[actual[at+lane]];
                    bool original=false;
                    for(unsigned old=0;old<3;++old) {
                        auto expected=input[actual[at+old]];expected.position=value.position;
                        if(!std::memcmp(&expected,&value,sizeof(Vertex))) {original=true;reordered|=old!=lane;break;}
                    }
                    need(original,"Native group depth changed non-position authored vertex inputs");
                }
                reordered_triangles+=reordered;at+=3;
            }
            need(at==actual.size(),"Native group depth lost raw primitive occurrences");
        }
        reject([&]{auto foreign=group;depth.validate_group(foreign);});
        reject([&]{auto altered=std::vector<Vertex>(depth.vertices().begin(),depth.vertices().end());altered.front().normal.x+=1;depth.validate_vertices(altered);});
        reject([&]{auto partial=input;partial.resize(partial.size()-3);(void)source_hologram_depth_input(group,object,partial,{640,480});});
        reject([&]{auto foreign=input;foreign.front().source_local_position.x+=1;(void)source_hologram_depth_input(group,object,foreign,{640,480});});
        reject([&]{auto foreign=input;foreign.front().source_part^=1;(void)source_hologram_depth_input(group,object,foreign,{640,480});});
        reject([&]{(void)source_hologram_depth_input(group,object^0x10000u,input,{640,480});});
        const auto bad_source=[&](auto fn) {
            const auto saved=group;
            auto part=std::find_if(group.parts.begin(),group.parts.end(),[&](const auto& p){return p.layer!=21 && p.object==object;});
            fn(*part);
            try{(void)source_hologram_depth_input(group,object,input,{640,480});}
            catch(...){group=saved;throw;}group=saved;
        };
        reject([&]{bad_source([&](auto& p){
            const auto foreign=std::find_if(group.parts.begin(),group.parts.end(),[&](const auto& q){
                return q.body_geometry && q.section!=p.section;
            });
            need(foreign!=group.parts.end(),"Foreign sealed origin control is absent");p.body_geometry=foreign->body_geometry;
        });});
        reject([&]{bad_source([&](auto& p){p.vertex_class=p.vertex_class==1?2:1;});});
        {
            const auto saved=group;
            group.raster.regions.fill({0,0,100,100});
            reject([&]{(void)source_hologram_depth_input(group,object,input,{640,480});});group=saved;
        }
        const auto mutate=[&](auto fn) {
            const auto saved=group;fn(group);
            try{depth.validate_group(group);}catch(...){group=saved;throw;}group=saved;
        };
        reject([&]{mutate([](auto& g){g.constants[0][0]+=1;});});
        reject([&]{mutate([](auto& g){g.camera_owner.reset();});});
        {
            const auto saved=group;group.camera_owner.reset();
            reject([&]{(void)source_hologram_depth_input(group,object,input,{640,480});});group=saved;
        }
        reject([&]{mutate([](auto& g){g.clip_range[2]+=1;});});
        reject([&]{mutate([](auto& g){g.raster.depth_factor_offset[1]=1;});});
        reject([&]{mutate([](auto& g){g.parts.front().body_source->object_rows.front()[3]+=1;});});
        reject([&]{mutate([](auto& g){g.parts.front().geometry_lod^=1;});});
        reject([&]{mutate([](auto& g){g.parts.front().pass=g.parts[2].pass;});});
        for(const auto& part:group.parts)if(part.layer==3 && part.object==object) {
            auto material=cache.load_material(part.pass->shader());material.selected_shader_lod=part.pass->choice().shader_lod;
            const auto base=std::find_if(units.begin(),units.end(),[&](const auto& u){return u.ordinal==part.base_ordinal;});
            need(base!=units.end(),"Retained source body unit disappeared");
            const bool prepared=base->lighting_class==1?prepare_world_material(cache,material,true):
                base->lighting_class==3?prepare_default_object_material(cache,material):
                prepare_dynamic_material(cache,material,{},true);
            need(prepared,"Actual body shader cannot produce its source lighting material");
            const Material* selected{};
            for(const auto* m:children(material))if(m->forward_pass && m->forward_pass_index<m->passes.size() &&
                m->passes[m->forward_pass_index].template_pass==part.pass->template_pass() &&
                m->passes[m->forward_pass_index].implementation==part.pass->choice().implementation) {
                need(!selected,"Actual source body material has an ambiguous pass");selected=m;
            }
            need(selected,"Actual selected body pass disappeared during source preparation");
            auto vertex=std::find_if(depth.vertices().begin(),depth.vertices().end(),[&](const auto& v) {
                return v.source_section==part.section && v.source_part==part.part;
            });
            need(vertex!=depth.vertices().end(),"Actual source part has no admitted body vertices");
            const SourceBspDrawKey key{vertex->material,vertex->lightmap,-1,vertex->source_section,vertex->source_part};
            WorldMaterialUniform uniform;std::copy_n(constants.begin(),7,uniform.source_camera_world.begin());uniform.source_camera_control[0]=1;
            const PackedVertexProgramBank bank{};
            const ShaderVector actual_forward{submitted.forward.x,submitted.forward.y,submitted.forward.z,1};
            const ShaderVector actual_eye{submitted.position.x,submitted.position.y,submitted.position.z,1};
            const auto check=[&](const auto& m,const auto& u,const auto& b,const auto& state,unsigned count,std::array<float,4> v,unsigned blend,bool stencil=false) {
                depth.validate_draw(group,part,key,count,m,u,b,actual_forward,actual_eye,v,state,blend,stencil);
            };
            const auto check_push=[&](ShaderVector forward,ShaderVector eye,ShaderVector viewport) {
                depth.validate_draw(group,part,key,part.body_vertices,*selected,uniform,bank,
                    forward,eye,viewport,selected->forward_raster,selected->forward_blend,false);
            };
            check(*selected,uniform,bank,selected->forward_raster,part.body_vertices,{0,0,640,480},selected->forward_blend);++draw_checks;
            for(float mode:{0.f,2.f,3.f})reject([&]{auto eye=actual_eye;eye[3]=mode;check_push(actual_forward,eye,{0,0,640,480});});
            reject([&]{auto forward=actual_forward;forward[3]=0;check_push(forward,actual_eye,{0,0,640,480});});
            reject([&]{auto forward=actual_forward;forward[0]+=1;check_push(forward,actual_eye,{0,0,640,480});});
            reject([&]{auto eye=actual_eye;eye[0]+=1;check_push(actual_forward,eye,{0,0,640,480});});
            reject([&]{check(*selected,uniform,bank,selected->forward_raster,part.body_vertices-3,{0,0,640,480},selected->forward_blend);});
            reject([&]{auto m=*selected;m.source_shader_datum^=0x10000u;check(m,uniform,bank,m.forward_raster,part.body_vertices,{0,0,640,480},m.forward_blend);});
            reject([&]{auto u=uniform;u.source_camera_world[3][0]+=1;check(*selected,u,bank,selected->forward_raster,part.body_vertices,{0,0,640,480},selected->forward_blend);});
            reject([&]{auto state=selected->forward_raster;state.depth_function=0x207;check(*selected,uniform,bank,state,part.body_vertices,{0,0,640,480},selected->forward_blend);});
            reject([&]{check(*selected,uniform,bank,selected->forward_raster,part.body_vertices,{0,0,639,480},selected->forward_blend);});
            reject([&]{check(*selected,uniform,bank,selected->forward_raster,part.body_vertices,{0,0,640,480},selected->forward_blend^1u);});
            reject([&]{check(*selected,uniform,bank,selected->forward_raster,part.body_vertices,{0,0,640,480},selected->forward_blend,true);});
            reject([&]{auto m=*selected;m.vertex_program.back()^=std::byte{128};check(m,uniform,bank,m.forward_raster,part.body_vertices,{0,0,640,480},m.forward_blend);});
            auto camera_bank=bank;camera_bank.range[3]=1;std::copy_n(constants.begin(),12,camera_bank.constants.begin());
            reject([&]{check(*selected,uniform,camera_bank,selected->forward_raster,part.body_vertices,{0,0,640,480},selected->forward_blend);});
            break;
        }
    }
    need(checked_vertices>0 && reordered_triangles>0 && draw_checks==2 && controls>=66,"Hologram depth controls were vacuous");
    std::cout<<"PASS "<<checked_vertices<<" native vertices; "<<reordered_triangles
        <<" exact strip-order triangles; "<<draw_checks<<" actual material/camera joins; "<<controls<<" rejected ownership controls\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
