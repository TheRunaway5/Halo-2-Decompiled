// Offline photo staging in an actual loaded Xbox level. Native constructors,
// live selected sections/palettes, current materials and pStencil renderer.
// Placements, camera and steady point-light inputs are deliberately declared.
#include "../capture_live_model_common.hpp"
#include "../../src/source_object_geometry.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/pstencil/geometry.hpp"
#include "halo2/pstencil/environment.hpp"
#include "halo2/pstencil/adaptation.hpp"
#include "native_variant_photo.hpp"
#include "native_photo_inputs.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/world_material.hpp"
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include "halo2/collision.hpp"
#include "halo2/object_light_query.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Actor {
    std::string name;std::uint32_t definition,model,object;
    Vec3 position;std::vector<Vertex> vertices;
    std::vector<RenderModel::VertexOrigin> origins;
    ObjectLightingState lighting;ObjectColors colors;
    std::vector<SourcePrtMaterialBank> prt;
    unsigned first{},count{};
    std::optional<Vec3> incident;
    std::uint32_t variant{};
    ShaderInput shader_input;
};
}
int run(int argc,char**argv) {
    require(argc>=5 && argc<=9,"Supply XBE MAP NEW_OUTPUT group|chief|elite|arbiter|visor [spawn index] [front-light|side-light] [elite-variant:NAME] [elite-definition:PATH]");
    const std::filesystem::path output=argv[3];require(!std::filesystem::exists(output),"output exists");
    std::filesystem::create_directories(output);
    Cache cache(argv[2]);const unsigned spawn=argc>=6?std::stoul(argv[5]):0;
    const bool front_light=argc>=7 && std::string_view(argv[6])=="front-light";
    require(argc<7 || front_light || std::string_view(argv[6])=="side-light","unknown photo light setup");
    std::string elite_variant,elite_definition;
    for(int n=7;n<argc;++n) {
        const std::string value=argv[n];
        if(value.starts_with("elite-variant:"))elite_variant=value.substr(14);
        else if(value.starts_with("elite-definition:"))elite_definition=value.substr(17);
        else throw FormatError("unknown photo actor input: "+value);
    }
    const auto origin=cache.spawns().at(spawn).position;
    CollisionWorld collision(cache.load_bsp_content(0).collision);
    std::optional<Vec3> floor_center;float best=1.e20f;
    const auto surface_lighting=cache.load_bsp_surface_lighting(0);
    const auto lighting_definition=cache.load_object_lighting_definition();
    for(const auto& triangle:collision.triangles()) {
        const auto n=normalized(cross(triangle.b-triangle.a,triangle.c-triangle.a));
        if(std::abs(n.z)<.99f)continue;
        const auto candidate=(triangle.a+triangle.b+triangle.c)/3.f;
        const auto delta=candidate-origin;const auto score=dot(delta,delta);
        if(score>=best || std::abs(delta.z)>4 || score>1600)continue;
        bool clear=true;
        // Original level triangles select a flat stage; this host query does
        // not supply any native physical result or alter the level geometry.
        for(float x:{-.3f,.3f,1.f,2.2f})for(float y:{-1.f,-.72f,-.3f,0.f,.3f,.72f,1.f}) {
            const Vec3 p=candidate+Vec3{x,y,.05f};
            const auto hit=collision.raycast(p+Vec3{0,0,.15f},{0,0,-1},.4f);
            if(!hit || std::abs(hit->position.z-candidate.z)>.025f || collision.overlaps(p,1.35f,.2f))clear=false;
        }
        const auto camera=candidate+Vec3{2.2f,1.2f,1.6f};
        if(collision.overlaps(camera,.15f,.12f))clear=false;
        for(float y:{-.72f,0.f,.72f}) {
            const auto d=candidate+Vec3{0,y,.6f}-camera;
            if(collision.raycast(camera,normalized(d),std::sqrt(dot(d,d))-.15f))clear=false;
        }
        for(float y:{-.72f,0.f,.72f}) {
            const auto sample=query_object_lightmap(surface_lighting,collision,candidate+Vec3{0,y,.6f});
            if(sample.sample) {
                const auto state=object_lighting(lighting_definition,*sample.sample,0);
                const auto c=state.ambient+state.primary_color;
                if(c.x>c.y*1.3f || c.z>c.y*1.4f)clear=false;
            }
        }
        if(clear){best=score;floor_center=candidate;}
    }
    require(bool(floor_center),"no nearby stage fits all three characters and camera");
    const auto center=*floor_center+Vec3{0,0,.015f};
    std::cout<<"Scene anchor "<<center.x<<','<<center.y<<','<<center.z<<" spawn="<<spawn<<'\n';
    PhysicsLiveWorldTu15 world(cache,argv[1],output/"savegame");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);
    std::vector<Actor> actors;std::vector<std::uint32_t> render_models;
    const std::array<std::pair<const char*,const char*>,3> names{{
        {"chief","objects\\characters\\masterchief\\masterchief"},
        {"arbiter","objects\\characters\\dervish\\dervish"},
        {"elite","objects\\characters\\elite\\elite"}}};
    for(unsigned i=0;i<names.size();++i) {
        const auto requested=i==2 && !elite_definition.empty()?elite_definition:std::string(names[i].second);
        const auto tag=std::ranges::find_if(cache.tags(),[&](const auto&t){return t.group=="bipd" && t.name==requested;});
        require(tag!=cache.tags().end(),"level does not contain all three authored characters");
        const auto model=cache.load_initial_model_definition(tag->datum).render_model;
        world.preload_render_model(model);render_models.push_back(model);
        actors.push_back({names[i].first,tag->datum,model,null_datum,center+Vec3{0,(float(i)-1)*.72f,0}});
    }
    world.initialize_map();
    for(auto& a:actors) {
        const auto definition=cache.load_initial_model_definition(a.definition);a.variant=definition.default_variant;
        if(a.name=="elite" && !elite_variant.empty()) {
            const auto selected=std::ranges::find_if(definition.variants,[&](const auto& v){return cache.string_id(v.name)==elite_variant;});
            require(selected!=definition.variants.end(),"requested Elite model variant is absent from the actual definition");
            a.variant=selected->name;
            a.object=pstencil::create_variant_photo_biped(world,a.definition,a.position,{1,0,0},a.variant);
            require(a.object!=null_datum,"native variant biped birth failed");
            const auto actual=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(world.memory().view(world.source_object_body(a.object)+0xb1u,1)[0]));
            require(actual==source_initial_model_variant(definition,a.variant),"native biped selected another model variant");
            std::cout<<"Actual Elite variant: "<<elite_variant<<" string_id="<<a.variant<<" body_index="<<int(actual)<<'\n';
        } else a.object=world.create_biped(a.definition,a.position,{1,0,0});
        require(a.object!=null_datum,"native biped birth failed");
        a.shader_input=pstencil::native_photo_inputs(cache,world,a.object,a.definition);
        // Declared offline photo-quality input. Native4BCC0 itself selects
        // authored LOD5 for bit19; no mesh is invented or changed on disk.
        const auto body=world.source_object_body(a.object);
        const auto flags=Bytes(world.memory().view(body+4,4)).u32(0)|0x80000u;
        world.write_source_call_input(body+4,std::as_bytes(std::span(&flags,1)));
        std::cout<<"Native actor "<<a.name<<" definition="<<a.definition<<" object="<<a.object
            <<" position="<<a.position.x<<','<<a.position.y<<','<<a.position.z<<'\n';
    }
    const std::string framing=argv[4];Vec3 target=center+Vec3{0,0,.6f};Vec3 eye=center+Vec3{1.85f,1.1f,1.5f};float fov=53.f;
    if(framing!="group") {
        const auto name=framing=="visor"?"chief":framing;
        const auto actor=std::ranges::find_if(actors,[&](const auto&a){return a.name==name;});
        require(actor!=actors.end(),"unknown camera framing");
        target=actor->position+Vec3{0,0,.6f};eye=actor->position+Vec3{1.45f,.15f,.8f};fov=58.f;
        if(framing=="visor") {
            const auto hierarchy=cache.load_model_hierarchy(actor->definition);
            const auto head=std::ranges::find_if(hierarchy.bones,[&](const auto& b){return cache.string_id(b.name).find("head")!=std::string::npos;});
            require(head!=hierarchy.bones.end(),"authored head node absent");
            const auto body_address=world.source_object_body(actor->object);const Bytes body(world.memory().view(body_address,0x11c));
            std::vector<Transform> nodes(body.u16(0x114)/52u);std::memcpy(nodes.data(),world.memory().view(body_address+body.u16(0x116),nodes.size()*52u).data(),nodes.size()*52u);
            const auto head_position=nodes.at(unsigned(head-hierarchy.bones.begin())).position;
            target=head_position+Vec3{.05f,0,.025f};eye=head_position+Vec3{.55f,-.24f,.08f};fov=35.f;
            std::cout<<"Authored head="<<head_position.x<<','<<head_position.y<<','<<head_position.z<<'\n';
        }
    }
    const auto forward=normalized(target-eye);constexpr unsigned width=2560,height=1920;
    const std::array<std::int16_t,4> viewport{0,0,height,width};
    SourceFrustumProjectionInput pi;pi.projection_angle=fov*3.14159265358979323846f/180;pi.outer=pi.safe=viewport;
    const auto projection=source_frustum_projection(pi);
    const auto camera=source_view_projection({eye,forward,{0,0,1},.015625f,1024.f,{},projection.projection_program});
    const auto atmosphere=cache.load_bsp_atmosphere(0);const auto fog=atmosphere.sample(eye,forward);
    const auto frustum=source_frustum_camera({eye,forward,{0,0,1},.015625f,1024.f,projection.clipping_rectangle()});
    std::array<std::uint32_t,16> open;open.fill(~0u);
    const auto visibility=source_portal_visibility(cache.load_source_portal_bsp(0),std::span(&frustum,1),fog.cluster,open);
    require(!visibility.clusters.empty(),"staged camera outside level BSP");
    const auto containing_fog=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,0});
    const auto geometry=cache.load_source_bsp_admission(0);
    SourceBspAdmissionInput bsp_input;bsp_input.containing_fog=containing_fog;
    const auto main_bsp=source_bsp_admission(geometry,visibility,bsp_input);
    Mesh mesh=cache.load_bsp(0);SourceBspResidentPlan bsp_shaders(cache,mesh,0);
    const auto bsp_plan=bsp_shaders.select({camera,main_bsp,0,height,1,false});
    const auto& bsp_parts=bsp_plan.selected_parts;
    require(!bsp_parts.empty(),"staged scene has no visible original BSP geometry");
    const auto light_tag=std::ranges::find_if(cache.tags(),[](const auto&t){return t.group=="ligh" && t.name=="effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion";});
    require(light_tag!=cache.tags().end(),"authored sphere light material absent");
    const std::array lights{light_tag->datum};SourceBspEffectLights owner(cache,mesh,0,lights,render_models);
    const auto definition=load_effect_light(cache,light_tag->datum);
    SourceEffectLightView light_view;SourceEffectLightViewRecord light{};light.definition=&definition;light.definition_identity=light_tag->datum;
    light.runtime.identity=0xe96c0000u;light.runtime.owner=null_datum;
    // Side lighting exposes the cast silhouettes beside the actors instead
    // of hiding them behind the bodies from the declared group camera.
    constexpr float light_size=1.4f,light_radius=2.f*light_size;
    light.runtime.position=light.runtime.spatial_center=center+
        (front_light?Vec3{.95f,.1f,.95f}:Vec3{.6f,-.6f,1.4f});
    light.runtime.spatial_radius=light_radius;light.runtime.forward={1,0,0};light.runtime.up={0,0,1};
    light.runtime.state={light_size,1,{2.f,2.f,2.f},{1.f,1.f,1.f},1};light.fades={1,1,1,1,1};light_view.records.push_back(light);
    const auto shape=source_effect_light_sphere(definition,light.runtime.position,{1,0,0},{0,0,1},light_size);
    const auto spatial=cache.load_object_light_spatial_bsp(0);
    auto receiver_view=source_effect_light_receiver_view({spatial,geometry,atmosphere,0},
        std::int16_t(atmosphere.cluster_at(light.runtime.position)),light.runtime.spatial_center,light_radius,shape,containing_fog,&main_bsp);
    WorldEffectLightReceiver receiver{light.runtime.identity,1,1,std::move(receiver_view)};
    SourceLiveModelDrawContextTu15 context;
    context.entry.mode=context.draw.draw.mode=1;context.entry.distance_scale=0;
    context.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
    context.entry.eye=eye;context.forward=forward;context.draw.eye=eye;context.draw.viewport=viewport;
    context.draw.rendering=camera;context.entry.coverage=source_shader_coverage_camera(camera);context.draw.draw.mask_arena_address=0x71001000u;
    SourceLiveModelBaseSelectionTu15 selected;context.built=[&](const auto& collector){selected.remember(collector);};
    std::vector<SourceLiveModelBaseDrawTu15> base_draws;
    std::map<std::pair<std::uint32_t,unsigned>,std::shared_ptr<const VertexProgramBank>> prt_banks;
    SourceObjectAdmissionInput object_input;object_input.containing_fog=containing_fog;
    SourceEffectLightModelViewTu15 objects{world,world.admit_render_objects(visibility,object_input,main_bsp.associations),context,containing_fog};
    objects.main_draws=[&](const auto& units){
        base_draws=selected.copy(world,units);
        std::erase_if(base_draws,[&](const auto& draw){return std::ranges::none_of(actors,[&](const auto&a){return a.object==draw.object;});});
        for(const auto& draw:base_draws)if(const auto prt=cache.load_source_prt_definition(draw.unit.words[1])) {
            auto bank=source_live_model_prt_bank_tu15(world,draw.unit,*prt);require(bool(bank),"missing native PRT bank");
            prt_banks.emplace(std::pair{draw.object,draw.unit.words[3]>>9&511u},std::move(bank));
        }
    };
    objects.model_available=[&](const SourceCullDrawUnit& unit){
        const auto object=Bytes(world.memory().view(unit.words[4],4)).u32(0);
        return std::ranges::any_of(base_draws,[&](const auto& draw){return draw.object==object;});
    };
    world.begin_model_frame();world.begin_model_view();
    std::vector<pstencil::PointCaster> point_casters;
    auto packets=owner.draw(light_view,std::span(&receiver,1),main_bsp,camera,fog,eye,viewport,{width,height},1,&objects,&point_casters);
    require(!packets.empty(),"staged point light produced no receivers");
    // Photo light deliberately isolates the independently collected character
    // casters. BSP remains a real receiving surface with its ordinary shading.
    for(auto& packet:packets)packet.pstencil_casts_shadow=false;
    std::erase_if(point_casters,[&](const auto& caster){
        const auto& owner=caster.source.owner();
        return !owner.generation || std::ranges::none_of(actors,[&](const auto& actor){return actor.object==*owner.generation;});
    });
    require(!point_casters.empty(),"native character-only shadow query produced no caster parts");
    for(const auto& packet:packets)for(const auto& caster:point_casters)
        if(packet.source.owner()==caster.source.owner() && packet.source.draw().part==caster.source.draw().part &&
           packet.source.draw().geometry->section()==caster.source.draw().geometry->section()) {
            require(packet.source.draw().topology==caster.source.draw().topology,"native caster changed source receiver topology");
            require(packet.source.draw().object_rows==caster.source.draw().object_rows,"native caster changed current source pose");
        }
    for(auto& a:actors) {
        std::vector<SourceLiveModelBaseDrawTu15> draws;
        for(const auto& draw:base_draws)if(draw.object==a.object)draws.push_back(draw);
        // Close-up views may legitimately cull the other two actors.
        if(draws.empty()){std::cout<<"Actor outside camera: "<<a.name<<'\n';continue;}
        const auto model=cache.load_initial_model_definition(a.definition);SourceInitialModelSelection selection;
        selection.variant_index=source_initial_model_variant(model,a.variant);
        selection.permutations.resize(model.regions.size());selection.states.resize(model.regions.size());
        const auto record=draws.front().unit.words[4]-0x44u;
        const Bytes source_light(world.memory().view(record,0x30));
        if(source_light.u16(2)==0x101u) {
            const Vec3 incident{source_light.f32(0x1c),source_light.f32(0x20),source_light.f32(0x24)};
            if(dot(incident,incident)>=1.e-8f)a.incident=incident;
        }
        if(!a.incident) {
            // Current PRT owners may use a resident probe without a dynamic
            // cache sample. Adapt that case using the game's current BSP
            // lightmap-query module at the actual native owner sphere.
            const auto query=query_object_lightmap(surface_lighting,collision,draws.front().sphere.center);
            if(query.sample && dot(query.sample->incident,query.sample->incident)>=1.e-8f) {
                a.incident=query.sample->incident;
                std::cout<<"Current BSP incident adaptation: "<<a.name<<'\n';
            }
        }
        std::memcpy(selection.permutations.data(),world.memory().view(record+0xc0u,selection.permutations.size()).data(),selection.permutations.size());
        const Bytes body(world.memory().view(world.source_object_body(a.object),0x11c));
        std::vector<Transform> nodes(body.u16(0x114)/52u);
        std::memcpy(nodes.data(),world.memory().view(world.source_object_body(a.object)+body.u16(0x116),nodes.size()*52u).data(),nodes.size()*52u);
        a.lighting=draws.front().lighting;
        a.colors=source_live_model_base_colors_tu15(draws.front(),cache.load_object_colors(a.definition,a.variant));
        a.first=unsigned(mesh.materials.size());
        const auto lod=draws.front().lod;
        if(cache.load_source_prt_definition(a.model)) {
            SourceObjectGeometry base;ScenarioObject placement;placement.definition=a.definition;placement.variant=a.variant;
            base.load(cache,placement,model,selection,a.model);
            base.pose_world([&](const auto&m){return skin_model_world_nodes(m,nodes);});
            const auto prt_lighting=cache.load_source_prt_lighting(0);
            base.prepare(cache,cache.load_object_colors(a.definition,a.variant),nodes,prt_lighting,mesh);
            a.vertices=base.at(lod).vertices;a.prt=base.owner_banks(cache,nodes,prt_lighting,lod);
            a.origins=base.at(lod).model.vertex_origins;
            for(auto& bank:a.prt)bank.bank=prt_banks.at({a.object,bank.section});
        } else {
            auto base=cache.load_model(a.definition,true,a.variant,selection,lod);
            for(auto& material:base.mesh.materials) {
                if(!material.lods.empty())require(prepare_dynamic_material(cache,material,{},true,0),"missing native dynamic base material");
            }
            SourceDynamicModelShaders shaders(cache,base,a.model,2,0.f);
            a.vertices=skin_model_world_nodes(base,nodes);
            SourceModelShaderView shader_view;
            shader_view.camera=source_shader_coverage_camera(camera);
            shader_view.shader=source_shader_lod_view(viewport[0],viewport[2],false,false);
            shader_view.object_coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
            shader_view.part_shader_multiplier=source_shader_lod_player_multiplier(1);
            shader_view.view_modifier=context.view_modifier;
            for(const auto& draw:draws)require(draw.coverage==draws.front().coverage,"photo actor sections changed native owner coverage");
            const auto vertex_count=a.vertices.size();
            shaders.select(a.vertices,{0,draws.front().coverage},shader_view);
            require(a.vertices.size()==vertex_count,"native photo shader selection declined a visible base part");
            for(auto& material:base.mesh.materials)mesh.materials.push_back(std::move(material));
            for(auto& v:a.vertices)v.material+=a.first;
            a.origins=base.vertex_origins;
        }
        a.count=unsigned(mesh.materials.size())-a.first;
        require(a.vertices.size()==a.origins.size(),"native color pose lost source vertex ordinals");
        std::size_t position_words{},different_words{};float maximum_delta{};
        for(const auto& caster:point_casters)if(caster.source.owner().generation==a.object) {
            const auto& source=caster.source;const auto points=pstencil::world_positions(source);
            for(unsigned n=0;n<a.vertices.size();++n)if(a.origins[n].section==source.draw().geometry->section() && a.origins[n].part==source.draw().part) {
                const auto raw=points.at(a.origins[n].vertex),base=a.vertices[n].position;
                const std::array<float,3> x{raw.x,raw.y,raw.z},y{base.x,base.y,base.z};
                for(unsigned c=0;c<3;++c){++position_words;different_words+=std::bit_cast<std::uint32_t>(x[c])!=std::bit_cast<std::uint32_t>(y[c]);maximum_delta=std::max(maximum_delta,std::abs(x[c]-y[c]));}
            }
        }
        require(maximum_delta<1.e-4f,"native point caster and current color pose differ materially");
        std::cout<<"Current base/source position comparison: "<<a.name<<" words="<<position_words<<" different="<<different_words
            <<" max_world_delta="<<maximum_delta<<" matching original vertex ordinals\n";
        // Declared photo-stage fill replaces the colored level probe for this
        // isolated showcase only. The game and its level lightmaps are untouched.
        a.lighting={};a.lighting.ambient={.18f,.18f,.18f};
        std::cout<<"[armor-inputs] "<<a.name<<" colors="<<a.colors[0].x<<','<<a.colors[0].y<<','<<a.colors[0].z
            <<" ambient="<<a.lighting.ambient.x<<','<<a.lighting.ambient.y<<','<<a.lighting.ambient.z
            <<" primary="<<a.lighting.primary_color.x<<','<<a.lighting.primary_color.y<<','<<a.lighting.primary_color.z<<'\n';
        std::cout<<"Actor base: "<<a.name<<" vertices="<<a.vertices.size()<<" lod="<<lod<<" units="<<draws.size()<<'\n';
        Vec3 low{1.e20f,1.e20f,1.e20f},high{-1.e20f,-1.e20f,-1.e20f};
        for(const auto& v:a.vertices) {
            low={std::min(low.x,v.position.x),std::min(low.y,v.position.y),std::min(low.z,v.position.z)};
            high={std::max(high.x,v.position.x),std::max(high.y,v.position.y),std::max(high.z,v.position.z)};
        }
        std::cout<<"Actor world bounds: "<<a.name<<" low="<<low.x<<','<<low.y<<','<<low.z
            <<" high="<<high.x<<','<<high.y<<','<<high.z<<'\n';
        if(a.incident)std::cout<<"Native lightmap incident: "<<a.name<<' '<<a.incident->x<<' '<<a.incident->y<<' '<<a.incident->z<<'\n';
    }
    nlohmann::json actor_bindings=nlohmann::json::array();
    for(const auto& a:actors) {
        std::map<std::string,unsigned> materials,shader_lods;
        for(const auto& v:a.vertices)++materials[mesh.materials.at(v.material).name];
        for(const auto& v:a.vertices)++shader_lods[mesh.materials.at(v.material).name+" / lod "+std::to_string(mesh.materials.at(v.material).selected_shader_lod)];
        nlohmann::json colors=nlohmann::json::array();for(const auto& c:a.colors)colors.push_back({c.x,c.y,c.z});
        nlohmann::json inputs=nlohmann::json::object();
        for(unsigned m=a.first;m<a.first+a.count;++m)for(const auto& overlay:mesh.materials[m].animation.overlays)
            if(overlay.input)inputs[cache.string_id(overlay.input)]=a.shader_input(overlay.input);
        actor_bindings.push_back({{"actor",a.name},{"definition",cache.query_tag({.handle=TagHandle{a.definition}})->name},
            {"model_variant",cache.string_id(a.variant)},{"variant_string_id",a.variant},{"current_colors",colors},
            {"object_function_inputs",inputs},
            {"selected_shader_lod_vertex_counts",shader_lods},
            {"visible_material_vertex_counts",materials},{"posed_vertices",a.vertices.size()}});
    }
    std::ofstream(output/"variant-bindings.json")<<actor_bindings.dump(2)<<'\n';
    if(SDL_Init(SDL_INIT_VIDEO)!=0)throw FormatError(std::string("SDL video initialization: ")+SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("pStencil armor in level",0,0,width,height,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    unsigned environment_responses{};
    std::function<void(const Material&)> inspect_environment=[&](const Material& m) {
        if(pstencil::supports_combined_environment_strength(m))
            std::cout<<"Combined reflection strength: "<<m.name
                <<" original_static_values="<<bool(pstencil::reference_environment_parameters(m.name))
                <<" polished_armor="<<pstencil::arbiter_metal(m.name)<<'\n';
        if(pstencil::supports_environment_response(m)) {
            ++environment_responses;std::cout<<"2003 resident environment response: "<<m.name
                <<" original_static_values="<<bool(pstencil::reference_environment_parameters(m.name))<<'\n';
        }
        if(m.environment_pass)inspect_environment(*m.environment_pass);
        for(const auto& child:m.world_overlays_before)inspect_environment(*child);
        for(const auto& child:m.world_overlays_after)inspect_environment(*child);
    };
    for(const auto& material:mesh.materials)inspect_environment(material);
    require(environment_responses>0,"armor scene has no matching independent environment response");
    std::cout<<"2003 resident environment response material count="<<environment_responses<<'\n';
    if(!window)throw FormatError(std::string("SDL capture window: ")+SDL_GetError());auto renderer_owner=std::make_unique<Renderer>(window.get(),mesh,true,std::span<const SourceShadowPlanMaterial>{},&bsp_shaders);auto& renderer=*renderer_owner;renderer.scene_bsp_atmosphere(atmosphere);
    DisplaySettings settings;settings.anti_aliasing=1;renderer.debug_panel().display_settings(settings);
    ModelView view;view.camera.position=eye;view.camera.forward=forward;view.camera.up={0,0,1};view.camera.oriented=true;view.camera.field_of_view=fov;
    view.width=640;view.height=480;view.full_surface=true;view.atmosphere=fog;view.atmosphere_source=&atmosphere;
    view.source_render_policy=source_geometry_lod_policy(1);view.source_bsp_admission=&main_bsp;
    view.source_bsp_resident_plan=&bsp_shaders;view.source_bsp_bindings=bsp_plan.bindings;
    const auto camera_projection=source_camera_projection(pi.projection_angle,{0,0,width,height},{0,0,width,height},false);
    view.camera.projection_clip_scale=camera_projection.clip_scale;view.camera.projection_clip_offset=camera_projection.clip_offset;
    std::vector<ObjectLightingBinding> bindings;std::vector<ModelObjectDraw> bodies;
    for(const auto& a:actors)if(!a.vertices.empty()){bindings.push_back({a.first,a.count,a.lighting});bodies.push_back({a.vertices,a.lighting,a.colors,a.shader_input,a.prt});}
    view.object_lighting=bindings;view.objects=bodies;view.raw_effect_lights=packets;
    std::vector<pstencil::LightmapCaster> lightmap_casters;
    for(const auto& a:actors)if(a.incident && !a.vertices.empty())lightmap_casters.push_back({a.vertices,*a.incident});
    view.pstencil_lightmap_casters=lightmap_casters;
    view.pstencil_point_casters=point_casters;view.pstencil_native_point_casters=true;
    const auto bank=source_view_shader_bank(camera,1024.f,eye,{0,0,width,height},false);
    view.source_view_shader_constants.emplace();std::copy_n(bank.begin(),7,view.source_view_shader_constants->world.begin());
    view.source_view_shader_constants->reflection=view.source_view_shader_constants->world;
    const auto capture=[&](const char* name){for(unsigned i=0;i<5;++i)if(renderer.draw(view.camera,output/name,{},false,{},{},{},std::span(&view,1)))return;throw FormatError("capture did not submit");};
    view.raw_effect_lights={};capture("base-no-point-light.ppm");view.raw_effect_lights=packets;
    settings.graphics_mode=pstencil::GraphicsMode::Original;renderer.debug_panel().display_settings(settings);capture("Original.ppm");
    settings.graphics_mode=pstencil::GraphicsMode::pStencil;renderer.debug_panel().display_settings(settings);capture("pStencil.ppm");
    auto ordinary_armor_specular=packets;unsigned polish_controls{};
    for(auto& packet:ordinary_armor_specular)if(packet.pstencil_specular &&
        pstencil::arbiter_metal(mesh.materials.at(packet.material_slot).name)) {
        const auto& original=*packet.pstencil_specular;
        const auto stride=1u+pstencil::arbiter_specular_copies;
        require(original.size()%stride==0,"polished armor lost its authored specular groups");
        auto filtered=std::make_shared<std::vector<EffectLightDraw>>();
        for(unsigned n=0;n<original.size();n+=stride)filtered->push_back(original[n]);
        polish_controls+=unsigned(original.size()/stride)*pstencil::arbiter_specular_copies;packet.pstencil_specular=std::move(filtered);
    }
    require(polish_controls>0 || framing=="visor" || framing=="chief" || framing=="elite",
        "staged Arbiter armor has no polished specular companion");
    view.raw_effect_lights=ordinary_armor_specular;capture("pStencil-armor-original-specular.ppm");view.raw_effect_lights=packets;
    std::cout<<"Polished armor companion controls="<<polish_controls<<'\n';
    view.pstencil_point_casters={};capture("pStencil-no-point-shadows.ppm");view.pstencil_point_casters=point_casters;
    view.pstencil_point_casters={};view.pstencil_native_point_casters=false;capture("pStencil-CPU-caster-control.ppm");
    view.pstencil_point_casters=point_casters;view.pstencil_native_point_casters=true;
    view.pstencil_lightmap_casters={};capture("pStencil-no-baked-shadow.ppm");view.pstencil_lightmap_casters=lightmap_casters;
    auto dynamic_only=packets;for(auto& packet:dynamic_only)packet.pstencil_casts_shadow=false;
    view.raw_effect_lights=dynamic_only;capture("pStencil-dynamic-shadows-only.ppm");
    view.raw_effect_lights=packets;
    auto diffuse=packets;for(auto& p:diffuse)p.pstencil_specular.reset();view.raw_effect_lights=diffuse;capture("pStencil-diffuse.ppm");
    view.raw_effect_lights=packets;
    settings.graphics_mode=pstencil::GraphicsMode::Original;renderer.debug_panel().display_settings(settings);capture("Original-restored.ppm");
    const auto file_bytes=[](const auto& path){std::ifstream f(path,std::ios::binary);return std::vector<char>{std::istreambuf_iterator<char>(f),{}};};
    require(file_bytes(output/"Original.ppm")==file_bytes(output/"Original-restored.ppm"),"Original changed after live high-resolution armor/shadow round trip");
    const auto full=file_bytes(output/"pStencil.ppm"),unshadowed=file_bytes(output/"pStencil-no-point-shadows.ppm");
    require(full.size()==unshadowed.size(),"shadow-control image shape changed");
    std::size_t darkened{},brightened{};
    for(unsigned n=0;n<full.size();++n) {
        darkened+=static_cast<unsigned char>(full[n])<static_cast<unsigned char>(unshadowed[n]);
        brightened+=static_cast<unsigned char>(full[n])>static_cast<unsigned char>(unshadowed[n]);
    }
    require(darkened>10 && brightened==0,"native point-shadow control produced no darkening or added light");
    std::cout<<"Native point-shadow control: darkened_channels="<<darkened<<" brightened_channels="<<brightened<<'\n';
    settings.graphics_mode=pstencil::GraphicsMode::pStencil;renderer.debug_panel().display_settings(settings);
    if(framing=="visor") {
    const auto visor=std::find_if(mesh.materials.begin(),mesh.materials.end(),[](const auto& m){return m.name.find("visor")!=std::string::npos;});
    if(visor!=mesh.materials.end()) {
        Mesh unreflected=mesh;unsigned cubes=0;
        const auto clone=[&](auto&& self,const Material& source)->Material {
            auto m=source;
            const auto child=[&](std::shared_ptr<Material>& p){if(p)p=std::make_shared<Material>(self(self,*p));};
            child(m.environment_pass);child(m.planar_fog_pass);
            for(auto& p:m.world_overlays_before)child(p);for(auto& p:m.world_overlays_after)child(p);
            for(auto& p:m.world_bloom_passes)child(p);for(auto& p:m.world_planar_passes)child(p);
            if(m.forward_pass && m.forward_pass_index<m.passes.size()) {
                Texture* textures[]{&m.diffuse,&m.detail,&m.bump,&m.auxiliary};
                for(const auto& binding:m.passes[m.forward_pass_index].textures)if(!binding.external && textures[binding.stage]->layers==6) {
                    Texture black;black.width=black.height=1;black.layers=6;black.format=11;black.pixels.resize(24);
                    for(unsigned face=0;face<6;++face) {black.pixels[face*4+3]=std::byte{255};black.mips.push_back({1,1,1u*face*4u,4});}
                    *textures[binding.stage]=black;
                    for(auto& frame:m.animated_textures[binding.stage])frame=black;
                    ++cubes;
                }
            }
            return m;
        };
        for(unsigned n=0;n<unreflected.materials.size();++n)if(unreflected.materials[n].name.find("visor")!=std::string::npos)
            unreflected.materials[n]=clone(clone,unreflected.materials[n]);
        require(cubes>0,"visor has no bound authored environment cube");
        renderer_owner.reset();
        Renderer control_renderer(window.get(),unreflected,true,{},&bsp_shaders);control_renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
        control_renderer.debug_panel().display_settings(settings);view.raw_effect_lights=packets;
        for(unsigned i=0;i<5;++i)if(control_renderer.draw(view.camera,output/"pStencil-visor-no-reflection.ppm",{},false,{},{},{},std::span(&view,1)))break;
        std::cout<<"Authored visor environment control: cubes="<<cubes<<'\n';
    }
    }
    std::ofstream report(output/"scene.txt");report<<"Staged native TU1.5 level capture; not an original matched frame or campaign event.\n"
        <<"Map="<<argv[2]<<"\nFraming="<<framing<<"\nQuality: native LOD5, 2560x1920 output, 5120x3840 stencil/color/depth targets with 2x SSAA per axis.\nNative actors="<<actors.size()<<"\nBSP parts="<<bsp_parts.size()<<"\nRaw point-light ranges="<<packets.size()
        <<"\nPoint light: authored frag sphere program, steady declared diffuse RGB2, specular RGB1, neutral ambient fill RGB.18, character-only stencil casters, radius2.8, position "
        <<light.runtime.position.x<<' '<<light.runtime.position.y<<' '<<light.runtime.position.z
        <<"\nBaked-light shadow casters="<<lightmap_casters.size()<<"; native completed raw incident or declared current BSP query for resident PRT owners; one RGB152/255 multiply before environment/emission/dynamic lights.\n"
        <<"Native independent point-caster parts="<<point_casters.size()<<"; actual current light-query sections and palettes before color availability; character-only isolation.\n"
        <<"\nCamera="<<eye.x<<' '<<eye.y<<' '<<eye.z<<" fov="<<fov<<"\n";
    std::cout<<"PASS staged level capture with "<<bsp_parts.size()<<" BSP parts, "<<bodies.size()<<" native character owners, "<<packets.size()<<" light ranges\n";return 0;
}
int main(int argc,char**argv){std::cout<<std::unitbuf;std::ostringstream diagnostics;auto* prior=std::cerr.rdbuf(diagnostics.rdbuf());try{const auto result=run(argc,argv);std::cerr.rdbuf(prior);require(diagnostics.str().find("Vulkan validation:")==std::string::npos,"Vulkan validation diagnostics");std::ofstream(std::filesystem::path(argv[3])/"diagnostics.txt")<<diagnostics.str();return result;}catch(const std::exception&e){std::cerr.rdbuf(prior);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}}
