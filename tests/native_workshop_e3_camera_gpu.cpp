#define SDL_MAIN_HANDLED
#include "native_workshop_e3_camera_gpu.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_texture_camera_tu15.hpp"
#include "halo2/source_texture_camera_producer_tu15.hpp"
#include "halo2/world_material_uniform.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
void need(bool value,const std::string& why){if(!value)throw FormatError(why);}
unsigned word(const SourceRuntimeMemory& memory,unsigned at){return Bytes(memory.view(at,4)).u32(0);}
struct Image {std::vector<unsigned char> pixels;};
Image image(const std::filesystem::path& path){
    std::ifstream input(path,std::ios::binary);std::string magic;unsigned w{},h{},max{};
    input>>magic>>w>>h>>max;input.get();need(magic=="P6"&&w==640&&h==480&&max==255,"E3 camera GPU capture format differs");
    Image result;result.pixels.resize(w*h*3);input.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
    need(bool(input),"E3 camera GPU capture is truncated");return result;
}
unsigned difference(const Image& a,const Image& b){
    need(a.pixels.size()==b.pixels.size(),"E3 camera GPU control extents differ");unsigned changed{};
    for(unsigned n=0;n<a.pixels.size();n+=3)
        changed+=a.pixels[n]!=b.pixels[n]||a.pixels[n+1]!=b.pixels[n+1]||a.pixels[n+2]!=b.pixels[n+2];
    return changed;
}
}
struct E3CameraGpuConsumer::Impl {
    const Cache& cache;PhysicsLiveWorldTu15& world;
    Mesh mesh;BspAtmosphere atmosphere;SourcePortalBsp portals;SourceBspAdmissionBsp geometry;
    SourceBspResidentPlan resident;
    std::filesystem::path output;std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window{nullptr,&SDL_DestroyWindow};
    std::unique_ptr<Renderer> renderer;std::ostringstream diagnostics;std::streambuf* previous{};
    std::optional<SourceCampaignCameraViewTu15> held;unsigned current_root{},captures{},draws{},uploads{},words{},pairs{},max_motion{},limits{},native_target_frames{};
    unsigned authored_empty_samples{},nonempty_samples{},empty_controls{};
    unsigned resident_draws{},dynamic_draws{};std::size_t verified_vertex_bytes{};
    std::size_t vertices{};bool warmed{},finished{};
    nlohmann::json records=nlohmann::json::array();
    static SourceBspResidentLighting lighting(const Cache& cache){const auto light=cache.load_bsp_lighting(0);
        return light.structure_matches&&light.bitmap_datum!=0xffffffffu?SourceBspResidentLighting::static_lightmap:SourceBspResidentLighting::classification3;}
    static SourceBspResidentPlan resident_input(const Cache& cache,Mesh& mesh){
        try{return SourceBspResidentPlan(cache,mesh,0,lighting(cache),nullptr,true);}
        catch(const FormatError& error){
            for(const auto& root:mesh.materials)if(!root.name.empty() && std::string(error.what()).find(root.name+" LOD")!=std::string::npos){
                const auto raw=cache.load_material(root.source_shader_datum,false,false);
                nlohmann::json diagnostic={{"name",root.name},{"template",raw.template_name},{"shader",root.source_shader_datum},
                    {"per_vertex",root.per_vertex_lighting},{"lighting_class",unsigned(lighting(cache))},{"lods",nlohmann::json::array()},
                    {"passes",nlohmann::json::array()},{"gaps",raw.rendering_gaps}};
                for(const auto& lod:raw.lods)diagnostic["lods"].push_back({{"layers",lod.layers.packed},{"mask",lod.available_layers},{"height",lod.projected_height}});
                for(const auto& pass:raw.passes)diagnostic["passes"].push_back({{"name",pass.name},{"vertex",pass.vertex_shader},
                    {"template_pass",pass.template_pass},{"implementation",pass.implementation}});
                std::cout<<"[E3-MATERIAL] "<<diagnostic.dump()<<'\n'<<std::flush;
            }
            throw;
        }
    }
    Impl(const Cache& c,PhysicsLiveWorldTu15& w):cache(c),world(w),mesh(c.load_bsp(0,true)),
        atmosphere(c.load_bsp_atmosphere(0)),portals(c.load_source_portal_bsp(0)),geometry(c.load_source_bsp_admission(0)),
        resident(resident_input(c,mesh)){
        need(!mesh.vertices.empty()&&!resident.parts().empty(),"E3 camera GPU input has no authored BSP geometry");
        const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch&&*scratch,"E3 camera GPU requires owned scratch");
        output=std::filesystem::path(scratch)/"e3-camera-gpu";std::filesystem::create_directories(output);
        for(const auto* name:{"HALO2_PIPELINE_CACHE_DIRECTORY","XDG_CACHE_HOME","XDG_CONFIG_HOME","MESA_SHADER_CACHE_DIR","__GL_SHADER_DISK_CACHE_PATH"}){
            const auto path=output/name;std::filesystem::create_directories(path);need(SDL_setenv(name,path.c_str(),1)==0,"Cannot own E3 GPU driver/cache storage");}
        need(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
        window.reset(SDL_CreateWindow("Native E3 camera",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN));need(bool(window),SDL_GetError());
        previous=std::cerr.rdbuf(diagnostics.rdbuf());
        try {
            const auto* shaders=std::getenv("HALO2_NATIVE_GPU_SHADER_DIR");need(shaders&&*shaders,"E3 GPU test lost its pinned shader directory");
            need(SDL_setenv("HALO2_CAPTURE_BSP_DRAW_BINDINGS","1",1)==0,
                "Cannot enable the E3 renderer's actual creation/write ledger");
            renderer=std::make_unique<Renderer>(window.get(),mesh,true,std::span<const SourceShadowPlanMaterial>{},&resident,
                std::function<void()>{},nullptr,shaders);
            renderer->scene_bsp_atmosphere(atmosphere);
            for(const auto* software:{"llvmpipe","lavapipe","SwiftShader","Software Rasterizer"})
                need(renderer->device_name().find(software)==std::string::npos,"E3 camera GPU requires a hardware Vulkan device");
        }catch(...){restore();throw;}
    }
    void restore(){if(previous){std::cerr.rdbuf(previous);previous=nullptr;}}
    ~Impl(){renderer.reset();restore();if(!finished&&!diagnostics.str().empty())std::cerr<<diagnostics.str();window.reset();SDL_Quit();}
    struct Frame {
        SourceBspAdmission admission;SourceBspResidentViewPlan selected;ModelView view;
        std::array<ShaderVector,7> constants{};Vec3 eye{},forward{};
        int containing_cluster=-1;unsigned visibility_limits{};
        std::vector<int> visible_clusters;
        SourceTextureCameraPcPresentationTu15 presented;
    };
    Frame frame(const SourceCampaignCameraViewTu15& observer){
        Frame result;const auto angles=source_camera_angles(observer.horizontal_field_of_view,observer.aspect);
        const auto clip=source_campaign_clip_planes_tu15(world);
        SourceFrustumProjectionInput input;input.projection_angle=angles.projection_angle;input.outer=input.safe={0,0,480,640};
        input.scenario_present=true;input.scenario_type=0;
        const auto projection=source_frustum_projection(input);
        const auto bank=source_view_projection({observer.position,observer.forward,observer.up,clip.x,clip.y,{},projection.projection_program});
        result.presented={{observer.position,observer.forward,observer.up,clip.x,clip.y,{},{}},input,angles.lighting_distance_scale,{}};
        const auto frustum=source_frustum_camera({observer.position,observer.forward,observer.up,clip.x,clip.y,projection.clipping_rectangle()});
        const auto fog=atmosphere.sample(observer.position,observer.forward);
        need(fog.cluster>=0,"Actual E3 camera GPU observer has no containing BSP cluster");
        const auto portal_owner=word(world.memory(),0x505e7cu);need(portal_owner,"E3 camera GPU has no actual native portal-mask owner");
        std::array<unsigned,16> mask;std::memcpy(mask.data(),world.memory().view(portal_owner,sizeof(mask)).data(),sizeof(mask));
        const auto visible=source_portal_visibility(portals,std::span(&frustum,1),fog.cluster,mask);limits|=visible.exhausted_limits;
        result.containing_cluster=fog.cluster;result.visibility_limits=visible.exhausted_limits;
        for(const auto& cluster:visible.clusters)result.visible_clusters.push_back(cluster.source_cluster);
        const auto containing=source_bsp_fog_admission(atmosphere,{true,true,fog.planar_datum,word(world.memory(),0x4e64c8u)});
        SourceBspAdmissionInput admission_input;admission_input.containing_fog=containing;
        result.admission=source_bsp_admission(geometry,visible,admission_input);
        const auto aux=word(world.memory(),0x510c50u);
        const SourceBspShaderLodAuxiliary auxiliary{aux!=0,aux&&std::to_integer<unsigned>(world.memory().view(aux+5,1)[0])!=0};
        result.selected=resident.select({bank,result.admission,0,480,1,projection.view_modifier,{},{}},auxiliary);
        auto& view=result.view;view.width=640;view.height=480;view.full_surface=true;
        view.camera.position=observer.position;view.camera.forward=observer.forward;view.camera.up=observer.up;
        view.camera.oriented=true;view.camera.horizontal_field_of_view=true;
        view.camera.field_of_view=observer.horizontal_field_of_view*57.295779513f;
        view.camera.projection_vertical_angle=angles.projection_angle;
        const auto camera_clip=source_camera_projection(angles.projection_angle,{0,0,640,480},{0,0,640,480},false);
        view.camera.projection_clip_scale=camera_clip.clip_scale;view.camera.projection_clip_offset=camera_clip.clip_offset;
        result.constants=source_view_shader_constants(bank,clip.y,false);result.eye=observer.position;result.forward=observer.forward;
        view.source_view_shader_constants.emplace();view.source_view_shader_constants->world=result.constants;
        view.source_view_shader_constants->reflection=result.constants;
        const auto foreground=source_view_shader_constants(bank,clip.y,true);
        std::copy_n(foreground.begin(),4,view.source_view_shader_constants->foreground.begin());
        view.projection_clip_planes=clip;view.source_w_distance=clip.y;view.source_render_policy=source_geometry_lod_policy(1);
        view.atmosphere=fog;view.atmosphere_source=&atmosphere;view.source_bsp_resident_plan=&resident;
        view.effect_time=float(word(world.memory(),word(world.memory(),0x51df54u)+8))/30.f;
        return result;
    }
    bool authored_empty_bsp(const Frame& frame) const {
        if(frame.containing_cluster<0||frame.visible_clusters.empty()||!frame.admission.clusters.empty()||
           !frame.admission.instances.empty()||!frame.selected.selected_parts.empty()||frame.visibility_limits)return false;
        bool contains_eye=false;
        for(const auto id:frame.visible_clusters) {
            contains_eye|=id==frame.containing_cluster;
            const auto& cluster=geometry.clusters.at(id);
            if(cluster.geometry_count||!cluster.instances.empty())return false;
            for(const auto& part:resident.parts())if(part.original.geometry==id&&part.count)return false;
        }
        return contains_eye;
    }
    Image capture(Frame& frame){
        auto& view=frame.view;view.source_bsp_admission=&frame.admission;view.source_bsp_bindings=frame.selected.bindings;
        view.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload){
            ++uploads;words+=upload.uploaded_vertex_instruction_words;
            need(upload.material_uniform.size()==sizeof(WorldMaterialUniform),"E3 GPU material upload lost its actual camera-constant bank");
            const auto camera=upload.material_uniform.subspan(offsetof(WorldMaterialUniform,source_camera_world),sizeof(frame.constants));
            need(std::ranges::equal(camera,std::as_bytes(std::span(frame.constants))),"E3 GPU material camera constants differ from its native observer projection");
        };
        view.completed_bsp_draw_observer=[&](const RendererObservedBspDraw& draw){
            need(draw.source_token&&draw.resident_part&&draw.source_binding&&draw.pipeline&&draw.descriptor&&draw.count&&
                draw.source_vertices.size()==draw.count,
                "E3 camera GPU draw lost its authored BSP/pipeline/descriptor/payload owner");
            const auto& token=*draw.source_token;const auto& part=*draw.resident_part;
            const auto part_index=source_part_index(draw.resident_part);
            const auto& binding=frame.selected.bindings.at(part_index);
            need(binding.material&&*binding.material==token.key.material&&
                binding.material==draw.source_binding->material&&binding.stipple==draw.source_binding->stipple&&
                token.count==draw.count&&part.count==draw.count&&token.key.material==draw.parent_material&&
                token.key.lightmap==draw.lightmap&&part.original.lightmap==draw.lightmap&&
                token.key.geometry==draw.geometry&&token.key.section==draw.section&&token.key.part==draw.part&&
                part.original.geometry==draw.geometry&&part.original.section==draw.section&&part.original.part==draw.part&&
                token.storage==(draw.source_resident?SourceBspVertexStorage::resident:SourceBspVertexStorage::dynamic),
                "E3 BSP draw escaped its exact native selection/storage/part owner");
            const auto& raw_vertices=resident.resident_vertices();
            need(part.first<=raw_vertices.size()&&part.count<=raw_vertices.size()-part.first,"E3 BSP source part escapes its complete vertex bank");
            const auto original=std::span(raw_vertices).subspan(part.first,part.count);
            if(draw.source_resident) {
                need(!std::memcmp(original.data(),draw.source_vertices.data(),original.size_bytes()),
                    "E3 resident BSP payload differs from its complete authored vertex bank");
                ++resident_draws;
            } else {
                for(unsigned i=0;i<draw.count;++i) {
                    Vertex expected;std::memcpy(&expected,&original[i],sizeof(expected));
                    expected.material=*binding.material;if(binding.stipple)expected.source_stipple_level=*binding.stipple;
                    need(!std::memcmp(&expected,&draw.source_vertices[i],sizeof(expected)),
                        "E3 dynamic BSP payload differs from its authored vertex/selected-stipple owner");
                }
                ++dynamic_draws;
            }
            verified_vertex_bytes+=original.size_bytes();
            need(draw.camera_push.size()==112&&!std::memcmp(draw.camera_push.data()+80,&frame.eye,sizeof(Vec3))&&
                !std::memcmp(draw.camera_push.data()+64,&frame.forward,sizeof(Vec3)),
                "E3 submitted GPU camera position/direction differ from the actual native observer");
            ++draws;vertices+=draw.count;
        };
        const auto path=output/"frame.ppm";
        const auto constructed=source_texture_camera_original_view_tu15(world,0,1,0,0);
        const auto original=source_texture_camera_pc_presented_view_tu15(constructed,frame.presented);
        const auto input=source_texture_camera_inputs_tu15(world.memory());
        const auto extent=source_texture_camera_target18_extent_tu15();
        const auto clip=*view.projection_clip_planes;
        const SourceSceneCopyContext copy{frame.presented.lens.outer,extent[0],extent[1],clip.x,clip.y};
        const auto targets=renderer->prepare_texture_camera_targets(input,copy,{640,480});
        need(targets.available(18),"E3 cinematic GPU camera did not obtain its actual native target18 owner");
        auto texture_frame=source_texture_camera_frame_tu15(world,original,targets);
        need(!texture_frame.source_rendered_view(),"E3 camera-only fixture unexpectedly activated an authored secondary texture camera");
        for(unsigned attempt=0;attempt<5;++attempt)if(renderer->draw_texture_camera(view.camera,{&texture_frame,nullptr,{}},{&view,1},path)){
            ++captures;++native_target_frames;auto result=image(path);std::filesystem::remove(path);return result;}
        throw FormatError("E3 native camera GPU frame was not submitted");
    }
    std::size_t source_part_index(const SourceBspResidentPart* source) const {
        const auto& parts=resident.parts();
        const auto found=std::find_if(parts.begin(),parts.end(),[&](const auto& part){return &part==source;});
        need(found!=parts.end(),"E3 draw references an unowned resident part");
        return std::size_t(found-parts.begin());
    }
};
E3CameraGpuConsumer::E3CameraGpuConsumer(const halo2::Cache& cache,halo2::PhysicsLiveWorldTu15& world):impl_(std::make_unique<Impl>(cache,world)){}
E3CameraGpuConsumer::~E3CameraGpuConsumer()=default;
void E3CameraGpuConsumer::sample(unsigned root,unsigned index){
    auto& s=*impl_;if(index!=0&&index!=3)return;
    const auto observer=source_campaign_camera_view_tu15(s.world.memory(),0);need(observer.has_value(),"E3 GPU lost its registered native observer");
    if(index==0){s.current_root=root;s.held=*observer;}else need(s.current_root==root&&s.held.has_value(),"E3 camera GPU control lost its authored shot");
    auto actual=s.frame(*observer);
    if(!s.warmed){(void)s.capture(actual);s.warmed=true;}
    const auto checkpoint=s.renderer->bsp_postprocess_state();
    const auto before_draws=s.draws,before_resident=s.resident_draws,before_dynamic=s.dynamic_draws;
    const auto rendered=s.capture(actual);
    const bool authored_empty=s.authored_empty_bsp(actual);
    if(authored_empty&&!s.empty_controls) {
        auto missing_visibility=actual;missing_visibility.visible_clusters.clear();
        need(!s.authored_empty_bsp(missing_visibility),"Missing native visibility was accepted as an authored empty scene");
        auto truncated=actual;truncated.visibility_limits=SourcePortalVisibilitySet::nodes;
        need(!s.authored_empty_bsp(truncated),"Truncated native visibility was accepted as an authored empty scene");
        const auto nonempty=std::find_if(s.geometry.clusters.begin(),s.geometry.clusters.end(),
            [](const auto& cluster){return cluster.geometry_count||!cluster.instances.empty();});
        need(nonempty!=s.geometry.clusters.end(),"E3 empty-view control lacks an actual geometry-bearing cluster");
        auto geometry=actual;geometry.visible_clusters.push_back(int(nonempty-s.geometry.clusters.begin()));
        need(!s.authored_empty_bsp(geometry),"Geometry-bearing native visibility was accepted as an authored empty scene");
        s.empty_controls=3;
    }
    nlohmann::json record={{"root",root},{"frame",index},{"clusters",actual.admission.clusters.size()},
        {"instances",actual.admission.instances.size()},{"selected_parts",actual.selected.selected_parts.size()},
        {"actual_draws",s.draws-before_draws},{"resident_draws",s.resident_draws-before_resident},
        {"dynamic_draws",s.dynamic_draws-before_dynamic},{"authored_empty_BSP",authored_empty}};
    if(s.draws==before_draws&&!authored_empty) {
        record["eye"]={actual.eye.x,actual.eye.y,actual.eye.z};
        record["forward"]={actual.forward.x,actual.forward.y,actual.forward.z};
        record["decisions"]=nlohmann::json::object();
        for(const auto& decision:actual.selected.decisions) {
            const auto key=std::to_string(unsigned(decision.state))+":"+std::to_string(unsigned(decision.suppression));
            auto& count=record["decisions"][key];count=count.is_number_unsigned()?count.get<unsigned>()+1:1u;
        }
        const auto mask=word(s.world.memory(),0x505e7cu);
        std::array<unsigned,16> words;std::memcpy(words.data(),s.world.memory().view(mask,sizeof(words)).data(),sizeof(words));
        record["native_portal_mask"]=words;
        record["containing_cluster"]=actual.containing_cluster;record["visibility_limits"]=actual.visibility_limits;
        record["visible_clusters"]=nlohmann::json::array();
        for(const auto id:actual.visible_clusters) {
            const auto& cluster=s.geometry.clusters.at(id);unsigned parts{},vertices{};
            for(const auto& part:s.resident.parts())if(part.original.geometry==id){++parts;vertices+=part.count;}
            record["visible_clusters"].push_back({{"id",id},{"geometry_count",cluster.geometry_count},
                {"instance_memberships",cluster.instances.size()},{"resident_parts",parts},{"resident_vertices",vertices}});
        }
        std::cout<<"[E3-NO-DRAW] "<<record.dump()<<'\n'<<std::flush;
    }
    need(authored_empty?s.draws==before_draws:s.draws>before_draws,
        "E3 camera GPU draws differ from actual authored empty/nonempty BSP ownership");
    if(authored_empty)++s.authored_empty_samples;else ++s.nonempty_samples;
    if(index==3){
        if(checkpoint)s.renderer->restore_bsp_postprocess_state(*checkpoint);
        auto held=s.frame(*s.held);const auto control=s.capture(held);
        const auto changed=difference(rendered,control);++s.pairs;s.max_motion=std::max(s.max_motion,changed);
        if(checkpoint)s.renderer->restore_bsp_postprocess_state(*checkpoint);
        const auto replay=s.capture(actual);need(difference(rendered,replay)==0,"E3 camera GPU repeat changed with the same native observer/material frame");
        record["frozen_camera_changed_pixels"]=changed;record["repeat_exact"]=true;
    }
    std::cout<<"[E3-FRAME] "<<record.dump()<<'\n'<<std::flush;
    s.records.push_back(std::move(record));
}
nlohmann::json E3CameraGpuConsumer::finish(){
    auto& s=*impl_;need(s.pairs==55&&s.captures>=220&&s.draws&&s.uploads&&s.vertices&&s.words==0&&s.max_motion>100,
        "E3 GPU camera lacks all55 authored shots, motion/replay controls, native uploads or real BSP draws");
    need(s.records.size()==110&&s.nonempty_samples&&s.authored_empty_samples&&s.empty_controls==3&&
         s.nonempty_samples+s.authored_empty_samples==110,
        "E3 GPU camera lacks all110 source-classified actual sample results");
    need(s.resident_draws&&s.dynamic_draws&&s.draws==s.resident_draws+s.dynamic_draws&&s.verified_vertex_bytes,
        "E3 GPU camera lacks complete resident/dynamic BSP payload ownership checks");
    const auto device=s.renderer->device_name();s.renderer.reset();s.restore();
    need(s.diagnostics.str().empty(),"E3 renderer/Vulkan diagnostics: "+s.diagnostics.str().substr(0,3000));
    s.finished=true;return {{"scope","Actual E3 native observer -> source visibility/LOD/resident BSP -> Vulkan camera uniforms/draws/pixels; actors, full mission and material gaps remain separate"},
        {"passed",true},{"device",device},{"captures",s.captures},{"camera_control_pairs",s.pairs},
        {"actual_BSP_draws",s.draws},{"actual_vertices",s.vertices},{"material_uploads",s.uploads},
        {"actual_native_target_frames",s.native_target_frames},
        {"resident_BSP_draws",s.resident_draws},{"dynamic_BSP_draws",s.dynamic_draws},
        {"verified_BSP_vertex_bytes",s.verified_vertex_bytes},
        {"authored_empty_BSP_samples",s.authored_empty_samples},{"nonempty_BSP_samples",s.nonempty_samples},
        {"authored_empty_BSP_controls",s.empty_controls},
        {"uploaded_vertex_instruction_words",s.words},{"maximum_frozen_camera_changed_pixels",s.max_motion},
        {"portal_arena_limits",s.limits},{"material_warnings",s.mesh.material_warnings},
        {"full_mod_render_qualified",false},{"frames",s.records},{"raw_captures_removed",true}};
}
