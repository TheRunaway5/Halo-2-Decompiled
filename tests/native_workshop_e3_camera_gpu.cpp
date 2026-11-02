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
    std::size_t vertices{};bool warmed{},finished{};
    nlohmann::json records=nlohmann::json::array();
    static SourceBspResidentLighting lighting(const Cache& cache){const auto light=cache.load_bsp_lighting(0);
        return light.structure_matches&&light.bitmap_datum!=0xffffffffu?SourceBspResidentLighting::static_lightmap:SourceBspResidentLighting::classification3;}
    static SourceBspResidentPlan resident_input(const Cache& cache,Mesh& mesh){
        try{return SourceBspResidentPlan(cache,mesh,0,lighting(cache),nullptr,true);}
        catch(const FormatError& error){
            for(const auto& root:mesh.materials)if(!root.name.empty() && std::string(error.what()).find(root.name)!=std::string::npos){
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
    Image capture(Frame& frame){
        auto& view=frame.view;view.source_bsp_admission=&frame.admission;view.source_bsp_bindings=frame.selected.bindings;
        view.completed_material_upload_observer=[&](const RendererCompletedMaterialUpload& upload){
            ++uploads;words+=upload.uploaded_vertex_instruction_words;
            need(upload.material_uniform.size()==sizeof(WorldMaterialUniform),"E3 GPU material upload lost its actual camera-constant bank");
            const auto camera=upload.material_uniform.subspan(offsetof(WorldMaterialUniform,source_camera_world),sizeof(frame.constants));
            need(std::ranges::equal(camera,std::as_bytes(std::span(frame.constants))),"E3 GPU material camera constants differ from its native observer projection");
        };
        view.completed_bsp_draw_observer=[&](const RendererObservedBspDraw& draw){
            need(draw.source_resident&&draw.source_token&&draw.resident_part&&draw.source_binding&&draw.pipeline&&draw.descriptor&&draw.count,
                "E3 camera GPU draw lost its authored resident BSP/pipeline/descriptor owner");
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
    const auto before_draws=s.draws;const auto rendered=s.capture(actual);
    need(s.draws>before_draws,"E3 camera GPU shot emitted no actual resident BSP draws");
    nlohmann::json record={{"root",root},{"frame",index},{"clusters",actual.admission.clusters.size()},
        {"instances",actual.admission.instances.size()},{"selected_parts",actual.selected.selected_parts.size()},
        {"actual_draws",s.draws-before_draws}};
    if(index==3){
        if(checkpoint)s.renderer->restore_bsp_postprocess_state(*checkpoint);
        auto held=s.frame(*s.held);const auto control=s.capture(held);
        const auto changed=difference(rendered,control);++s.pairs;s.max_motion=std::max(s.max_motion,changed);
        if(checkpoint)s.renderer->restore_bsp_postprocess_state(*checkpoint);
        const auto replay=s.capture(actual);need(difference(rendered,replay)==0,"E3 camera GPU repeat changed with the same native observer/material frame");
        record["frozen_camera_changed_pixels"]=changed;record["repeat_exact"]=true;
    }
    s.records.push_back(std::move(record));
}
nlohmann::json E3CameraGpuConsumer::finish(){
    auto& s=*impl_;need(s.pairs==55&&s.captures>=220&&s.draws&&s.uploads&&s.vertices&&s.words==0&&s.max_motion>100,
        "E3 GPU camera lacks all55 authored shots, motion/replay controls, native uploads or real BSP draws");
    const auto device=s.renderer->device_name();s.renderer.reset();s.restore();
    need(s.diagnostics.str().empty(),"E3 renderer/Vulkan diagnostics: "+s.diagnostics.str().substr(0,3000));
    s.finished=true;return {{"scope","Actual E3 native observer -> source visibility/LOD/resident BSP -> Vulkan camera uniforms/draws/pixels; actors, full mission and material gaps remain separate"},
        {"passed",true},{"device",device},{"captures",s.captures},{"camera_control_pairs",s.pairs},
        {"actual_BSP_draws",s.draws},{"actual_vertices",s.vertices},{"material_uploads",s.uploads},
        {"actual_native_target_frames",s.native_target_frames},
        {"uploaded_vertex_instruction_words",s.words},{"maximum_frozen_camera_changed_pixels",s.max_motion},
        {"portal_arena_limits",s.limits},{"material_warnings",s.mesh.material_warnings},
        {"full_mod_render_qualified",false},{"frames",s.records},{"raw_captures_removed",true}};
}
