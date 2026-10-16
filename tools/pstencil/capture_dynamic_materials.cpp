#include "../capture_live_model_common.hpp"
#include "../../src/source_object_geometry.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/world_material.hpp"
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <unistd.h>
using namespace halo2;
namespace {
void require(bool b,const char* why){if(!b)throw FormatError(why);}
struct Picture {unsigned w,h;std::vector<unsigned char> rgb;};
Picture read(const std::filesystem::path& path) {
    std::ifstream f(path,std::ios::binary);Picture p{};std::string magic;unsigned maximum;
    f>>magic>>p.w>>p.h>>maximum;f.get();require(magic=="P6" && maximum==255,"capture PPM schema differs");
    p.rgb.resize(std::size_t(p.w)*p.h*3);f.read(reinterpret_cast<char*>(p.rgb.data()),p.rgb.size());
    require(bool(f),"capture PPM truncated");return p;
}
}
int run(int argc,char**argv) {
    require(argc==4 || argc==5,"Supply TU15_XBE MAP NEW_OUTPUT_DIRECTORY [mode:FRAGMENT | bipd:FRAGMENT]");
    const std::filesystem::path output=argv[3];require(!std::filesystem::exists(output),"capture output already exists");
    std::filesystem::create_directories(output);
    Cache cache(argv[2]);
    const std::string selector=argc==5?argv[4]:"bipd:objects\\characters\\masterchief\\masterchief_mp";
    const bool scenario_model=selector.starts_with("mode:");
    const auto fragment=selector.substr(selector.find(':')+1);
    const auto selected_tag=std::ranges::find_if(cache.tags(),[&](const auto&t){
        return t.group==(scenario_model?"mode":"bipd") && t.name.find(fragment)!=std::string::npos;
    });
    require(selected_tag!=cache.tags().end(),"requested authored model absent");
    const auto light_tag=std::ranges::find_if(cache.tags(),[](const auto&t){return t.group=="ligh" && t.name=="effects\\objects\\weapons\\grenade\\frag_grenade\\grenade_frag_explosion";});
    require(light_tag!=cache.tags().end(),"authored frag light absent");
    auto definition_datum=selected_tag->datum;
    const auto model=scenario_model ? selected_tag->datum : cache.load_initial_model_definition(definition_datum).render_model;
    PhysicsLiveWorldTu15 world(cache,argv[1],output/"savegame");
    world.configure_session(source_game_variant_default(0),1,30,0x12345678u);world.preload_render_model(model);world.initialize_map();
    Vec3 position;std::uint32_t object;
    if(scenario_model) {
        const auto placement=original_model_placement(world,model);
        definition_datum=placement.definition;object=world.create_scenario_object(placement);
    } else {
        position=cache.spawns().at(0).position;
        object=world.create_biped(definition_datum,position,{1,0,0});
    }
    require(object!=null_datum,"original constructor declined selected owner");
    const auto model_definition=cache.load_initial_model_definition(definition_datum);
    const auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
    const auto body=word(word(word(0x4ecbd0u)+0x44u)+(object&65535u)*12u+8u);
    const Bytes sphere(world.memory().view(body+0x30u,16));
    if(scenario_model)position={sphere.f32(0),sphere.f32(4),sphere.f32(8)};
    const float framing=scenario_model?sphere.f32(12):1.f;
    require(framing>0,"owner has no positive framing sphere");
    SourcePortalVisibilitySet visibility;
    const auto clusters=cache.load_source_bsp_admission(0).clusters.size();std::vector<std::uint8_t> fog(clusters);
    for(unsigned n=0;n<std::min<std::size_t>(clusters,128);++n){SourceClusterFrustumRanges r;r.source_cluster=std::int16_t(n);visibility.clusters.push_back(r);}
    SourceLiveModelDrawContextTu15 context;
    context.entry.mode=context.draw.draw.mode=1;context.entry.distance_scale=1;
    context.entry.coverage_multiplier=source_geometry_lod_policy(1).coverage_multiplier;
    context.entry.eye={position.x+framing*(scenario_model?2.f:1.f),position.y+framing*(scenario_model?2.f:1.f),position.z+framing*.9f};
    context.forward=scenario_model?Vec3{-2.f,-2.f,-.9f}:Vec3{-1.f,-1.f,-.45f};
    const float inverse_length=1.f/std::sqrt(dot(context.forward,context.forward));
    context.forward={context.forward.x*inverse_length,context.forward.y*inverse_length,context.forward.z*inverse_length};
    context.draw.eye=context.entry.eye;context.draw.viewport={0,0,720,960};
    SourceFrustumProjectionInput projection;projection.projection_angle=1.2217304706573486f;projection.outer=projection.safe=context.draw.viewport;
    context.draw.rendering=source_view_projection({context.entry.eye,context.forward,{0,0,1},.015625f,1024.f,{},source_frustum_projection(projection).projection_program});
    context.entry.coverage=source_shader_coverage_camera(context.draw.rendering);context.draw.draw.mask_arena_address=0x71001000u;
    Mesh mesh;const std::array models{model},lights{light_tag->datum};SourceBspEffectLights owner(cache,mesh,0,lights,models);
    for(const auto& gap:owner.unavailable())std::cout<<"Material gap: "<<gap<<'\n';
    const auto definition=load_effect_light(cache,light_tag->datum);
    SourceEffectLightView light_view;SourceEffectLightViewRecord light{};light.definition=&definition;light.definition_identity=light_tag->datum;
    light.runtime.identity=0xe96c0000u;light.runtime.owner=0xffffffffu;
    light.runtime.position=light.runtime.spatial_center={position.x+.45f*framing,position.y+.45f*framing,position.z+.5f*framing};
    light.runtime.spatial_radius=2*framing;light.runtime.forward={1,0,0};light.runtime.up={0,0,1};
    light.runtime.state={1,1,{.8f,.5f,.25f},{.8f,.5f,.25f},1};light.fades={1,1,1,1,1};light_view.records.push_back(light);
    WorldEffectLightReceiver receiver;receiver.identity=light.runtime.identity;receiver.selected_fade=1;receiver.view.visibility=visibility;
    const auto prt_definition=cache.load_source_prt_definition(model);
    SourceLiveModelBaseSelectionTu15 base_selection;
    std::vector<SourceLiveModelBaseDrawTu15> base_draws;
    std::map<unsigned,std::shared_ptr<const VertexProgramBank>> retained_prt;
    SourceInitialModelSelection selection;
    selection.variant_index=source_initial_model_variant(model_definition,model_definition.default_variant);
    selection.permutations.resize(model_definition.regions.size());selection.states.resize(selection.permutations.size());
    context.built=[&](const auto& collector) {
        base_selection.remember(collector);
    };
    const auto camera=source_frustum_camera({context.entry.eye,context.forward,{0,0,1},
        .015625f,1024.f,source_frustum_projection(projection).clipping_rectangle()});
    visibility.groups.push_back(camera);visibility.frustums.push_back(camera.full_frustum);
    for(auto& cluster:visibility.clusters)cluster.counts[0]=1;
    SourceObjectAdmissionInput main_input;main_input.containing_fog=fog;
    SourceEffectLightModelViewTu15 objects{world,world.admit_render_objects(visibility,main_input),context,fog};
    objects.main_draws=[&](const auto& units) {
        base_draws=base_selection.copy(world,units);
        std::erase_if(base_draws,[&](const auto& draw){return draw.object!=object || draw.unit.words[1]!=model;});
        require(!base_draws.empty(),"main camera admitted no selected original model units");
        for(const auto& draw:base_draws)
            std::cout<<"Completed main unit: owner="<<draw.object<<" class="<<draw.lighting_class
                <<" override="<<unsigned(draw.override_slot)<<" lod="<<draw.lod
                <<" section="<<(draw.unit.words[3]>>9&511u)<<'\n';
        const auto record=base_draws.front().unit.words[4]-0x44u;
        std::memcpy(selection.permutations.data(),world.memory().view(record+0xc0u,selection.permutations.size()).data(),selection.permutations.size());
        if(prt_definition)for(const auto& draw:base_draws) {
            const auto section=draw.unit.words[3]>>9&511u;
            auto bank=source_live_model_prt_bank_tu15(world,draw.unit,*prt_definition);
            require(bool(bank),"main-view PRT unit has no completed original coefficient bank");
            retained_prt.emplace(section,std::move(bank));
        }
    };
    world.begin_model_frame();world.begin_model_view();
    auto packets=owner.draw(light_view,std::span(&receiver,1),{},context.draw.rendering,{},context.entry.eye,
        context.draw.viewport,{960,720},1,&objects);require(!packets.empty(),"live model yielded no raw illumination packets");
    std::set<unsigned> selected;for(const auto& packet:packets)selected.insert(packet.source.draw().geometry->section());
    const std::vector<unsigned> sections(selected.begin(),selected.end());
    for(const auto& packet:packets) {
        const auto base=std::find_if(base_draws.begin(),base_draws.end(),[&](const auto& draw){
            return draw.object==*packet.source.owner().generation &&
                (draw.unit.words[3]>>9&511u)==packet.source.draw().geometry->section();
        });
        require(base!=base_draws.end() && base->palette_rows==packet.source.draw().object_rows,
            "base and grenade light do not share the completed source section/palette");
    }
    // Explicit selected-section base geometry. Its packed ordinary path is
    // intentionally measured against the raw light path, never a parity oracle.
    const Bytes body_data(world.memory().view(body,0x11c));
    const unsigned node_count=body_data.u16(0x114)/52u,node_offset=body_data.u16(0x116);
    std::vector<Transform> nodes(node_count);
    std::memcpy(nodes.data(),world.memory().view(body+node_offset,node_count*52u).data(),node_count*52u);
    const auto actual_lighting=base_draws.front().lighting;
    const auto lod=base_draws.front().lod;
    for(const auto& draw:base_draws)require(draw.lod==lod,"one source model selected inconsistent base LODs");
    const auto base_sections=source_initial_model_sections(model_definition,selection,lod);
    const std::set<unsigned> visible_sections(base_sections.begin(),base_sections.end());
    // A visible authored section can have no dynamic-light pass. Illumination
    // must use the current base selection, but need not illuminate all of it.
    require(std::includes(visible_sections.begin(),visible_sections.end(),selected.begin(),selected.end()),
        "illumination selects a section outside the current base LOD/permutations");
    const unsigned offset=unsigned(mesh.materials.size());
    std::vector<Vertex> vertices;
    std::vector<SourcePrtMaterialBank> original_prt_banks;
    if(prt_definition) {
        // Reuse the resident source section/LOD/PRT path. It samples the
        // authored BSP SH using the actual created object's WORLD nodes.
        SourceObjectGeometry base;ScenarioObject placement;placement.definition=definition_datum;
        base.load(cache,placement,model_definition,selection,model);
        base.pose_world([&](const RenderModel& m){return skin_model_world_nodes(m,nodes);});
        base.prepare(cache,cache.load_object_colors(definition_datum,model_definition.default_variant),
            nodes,cache.load_source_prt_lighting(0),mesh);
        vertices=base.at(lod).vertices;
        original_prt_banks=base.owner_banks(cache,nodes,cache.load_source_prt_lighting(0),lod);
        for(auto &binding:original_prt_banks) {
            binding.bank=retained_prt.at(binding.section);
        }
    } else {
        auto base=cache.load_model(definition_datum,true,0,selection,lod);
        for(auto material:base.mesh.materials) {
            const auto classification=base_draws.front().lighting_class;
            if(!material.lods.empty() && !(classification==2?prepare_dynamic_material(cache,material,{},true,0.f):
                classification==3?prepare_default_object_material(cache,material,0.f):false))
                throw FormatError("selected base material lacks original dynamic pass: "+material.name);
            mesh.materials.push_back(std::move(material));
        }
        vertices=skin_model_world_nodes(base,nodes);for(auto& vertex:vertices)vertex.material+=offset;
    }
    const auto material_count=unsigned(mesh.materials.size())-offset;
    mesh.vertices.resize(3);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());struct Sdl{~Sdl(){SDL_Quit();}} sdl;
    auto window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("Live TU1.5 model illumination",0,0,960,720,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(bool(window),SDL_GetError());auto renderer_owner=std::make_unique<Renderer>(window.get(),mesh,true);auto& renderer=*renderer_owner;renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
    ModelView view;view.camera.position=context.entry.eye;view.camera.forward=context.forward;view.camera.up={0,0,1};view.camera.oriented=true;view.camera.field_of_view=70;
    const std::array bindings{ObjectLightingBinding{offset,material_count,actual_lighting}};
    view.object_lighting=bindings;
    const ModelObjectDraw prt_draw{vertices,actual_lighting,{},{},original_prt_banks};
    view.width=640;view.height=480;view.full_surface=true;
    view.objects=std::span(&prt_draw,1);
    view.atmosphere=BspFogSample{};view.atmosphere->bloom={1,.9f,0,0};
    view.source_view_shader_constants.emplace();std::copy_n(packets.front().source.constants().begin(),7,view.source_view_shader_constants->world.begin());
    view.source_view_shader_constants->reflection=view.source_view_shader_constants->world;
    const auto capture_renderer=[&](Renderer& target,const char* name) {
        const auto path=output/name;
        for(unsigned n=0;n<5;++n)if(target.draw(view.camera,path,{},false,{},{},{},std::span(&view,1)))return read(path);
        throw FormatError("live model Vulkan capture did not submit");
    };
    const auto capture=[&](const char* name){return capture_renderer(renderer,name);};
    const auto dark=capture("baseline.ppm");view.raw_effect_lights=packets;const auto lit=capture("illumination.ppm");
    DisplaySettings mode;mode.graphics_mode=pstencil::GraphicsMode::pStencil;renderer.debug_panel().display_settings(mode);
    const auto shadowed=capture("pStencil-specular.ppm");
    std::size_t specular_packets=0;for(const auto& packet:packets)if(packet.pstencil_specular)specular_packets+=packet.pstencil_specular->size();
    require(specular_packets>0,"pStencil model lacks authored specular companions");
    std::map<std::uint32_t,std::string> specular_shaders;
    for(const auto& packet:packets)if(packet.pstencil_specular)
        specular_shaders.emplace(mesh.materials.at(packet.material_slot).source_shader_datum,mesh.materials.at(packet.material_slot).name);
    for(const auto& [shader,name]:specular_shaders) {
        auto without=packets;
        for(auto& packet:without)if(mesh.materials.at(packet.material_slot).source_shader_datum==shader)packet.pstencil_specular.reset();
        view.raw_effect_lights=without;
        const auto filename="without-specular-"+std::to_string(shader)+".ppm";
        const auto control=capture(filename.c_str());std::size_t affected=0;
        for(unsigned n=0;n<control.rgb.size();++n)affected+=int(shadowed.rgb[n])>int(control.rgb[n])+1;
        std::cout<<"Authored specular: "<<name<<" positive_channels="<<affected<<'\n';
        // Authored passes can be zero for this angle/mask; retain their measured result.
    }
    auto diffuse_only=packets;for(auto& packet:diffuse_only)packet.pstencil_specular.reset();
    view.raw_effect_lights=diffuse_only;const auto stencil_diffuse=capture("pStencil-diffuse.ppm");
    std::size_t highlights=0,occluded=0;
    for(unsigned n=0;n<lit.rgb.size();++n) {
        highlights+=int(shadowed.rgb[n])>int(stencil_diffuse.rgb[n])+1;
        occluded+=int(lit.rgb[n])>int(stencil_diffuse.rgb[n])+1;
    }
    require(highlights>100,"pStencil authored specular produced no visible highlights");
    require(occluded>100,"pStencil dynamic model received no visible stencil shadow");
    view.raw_effect_lights=packets;mode.graphics_mode=pstencil::GraphicsMode::Original;renderer.debug_panel().display_settings(mode);
    require(capture("Original-restored.ppm").rgb==lit.rgb,"dynamic model Original round trip changed pixels");
    mode.graphics_mode=pstencil::GraphicsMode::pStencil;
    // Isolate the independent dynamic-caster path: no raw receiver is
    // allowed to supply a caster. Base geometry still supplies its complete
    // current triangles, including parts with no dynamic-light pass.
    auto dynamic_only=diffuse_only;for(auto& packet:dynamic_only)packet.pstencil_casts_shadow=false;
    view.raw_effect_lights=dynamic_only;renderer.debug_panel().display_settings(mode);
    const auto independent=capture("pStencil-independent-dynamic-casters.ppm");
    Mesh no_casters=mesh;
    for(auto& material:no_casters.materials)for(auto& lod:material.lods)lod.available_layers&=~(1u<<10);
    renderer_owner.reset();
    std::size_t dynamic_shadows=0;
    {
        Renderer control_renderer(window.get(),no_casters,true);control_renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
        control_renderer.debug_panel().display_settings(mode);
        const auto control=capture_renderer(control_renderer,"pStencil-no-dynamic-casters.ppm");
        for(unsigned n=0;n<control.rgb.size();++n)dynamic_shadows+=int(control.rgb[n])>int(independent.rgb[n])+1;
    }
    require(dynamic_shadows>100,"independent current dynamic casters made no visible shadow on the dynamic receiver");
    std::cout<<"Independent dynamic caster shadows: channels="<<dynamic_shadows<<'\n';
    // Remove only authored environment bitmap bindings from a deep private
    // visor copy. The extern normalization cube and all authored combiners,
    // geometry, lighting and specular remain identical.
    const auto visor=std::find_if(mesh.materials.begin()+offset,mesh.materials.end(),[](const auto& m){return m.name.find("visor")!=std::string::npos;});
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
        for(unsigned n=offset;n<unreflected.materials.size();++n)if(unreflected.materials[n].name.find("visor")!=std::string::npos)
            unreflected.materials[n]=clone(clone,unreflected.materials[n]);
        require(cubes>0,"visor has no bound authored environment cube");
        Renderer control_renderer(window.get(),unreflected,true);control_renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
        control_renderer.debug_panel().display_settings(mode);view.raw_effect_lights=packets;
        const auto control=capture_renderer(control_renderer,"pStencil-visor-no-reflection.ppm");std::size_t reflected=0;
        for(unsigned n=0;n<control.rgb.size();++n)reflected+=control.rgb[n]!=shadowed.rgb[n];
        require(reflected>0,"pStencil visor reflection bitmap made no visible contribution");
        std::cout<<"Authored visor environment: cubes="<<cubes<<" changed_channels="<<reflected<<'\n';
    }
    std::cout<<"pStencil dynamic model: highlights="<<highlights<<" shadowed_channels="<<occluded<<" specular_packets="<<specular_packets<<'\n';

    std::size_t channels=0;unsigned maximum=0;
    for(unsigned n=0;n<dark.rgb.size();++n){const auto delta=unsigned(std::abs(int(lit.rgb[n])-int(dark.rgb[n])));channels+=delta!=0;maximum=std::max(maximum,delta);}
    std::ofstream report(output/"report.txt");report<<"Controlled native diagnostic, not an original matched frame.\n"
        <<"Fresh TU1.5 owner: "<<selected_tag->name<<"; native live collector and palettes.\n"
        <<"Authored frag material with declared camera and light inputs; sphere-framed scenario diagnostics scale the declared query.\n"
        <<"Original depth EQUAL retained. Ordinary base uses current native lighting and packed model renderer.\n"
        <<"PRT owner banks read from the completed native TU1.5 cache: "<<original_prt_banks.size()<<".\n"
        <<"Main source units retained before light queries: "<<base_draws.size()<<"; exact base/light c50 palettes.\n"
        <<"independent_dynamic_shadow_channels="<<dynamic_shadows<<'\n'
        <<"pStencil highlights="<<highlights<<" shadowed_channels="<<occluded<<" specular_packets="<<specular_packets<<'\n'
        <<"packets="<<packets.size()<<" sections="<<sections.size()<<" changed_channels="<<channels<<" max_delta="<<maximum<<'\n';
    std::cout<<"Native live model light: packets="<<packets.size()<<" changed_channels="<<channels<<" max_delta="<<maximum<<'\n';
    require(channels>100,"live model light made no visible contribution through original depth EQUAL");return 0;
}
int main(int argc,char**argv){std::cout<<std::unitbuf;std::ostringstream diagnostics;auto* old=std::cerr.rdbuf(diagnostics.rdbuf());try{
    const auto result=run(argc,argv);std::cerr.rdbuf(old);
    require(diagnostics.str().find("Vulkan validation:")==std::string::npos,"live model Vulkan validation diagnostics");return result;
}catch(const std::exception&e){std::cerr.rdbuf(old);std::cerr<<diagnostics.str()<<e.what()<<'\n';return 1;}}
