// Dedicated active-camera component. The immutable1181 driver and its NONE
// fixtures are separate. This test never fabricates a producer camera/image.
#include "native_texture_camera_authored_setup.hpp"
#include "native_texture_camera_gpu_observation.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/source_campaign_secondary_view_tu15.hpp"
#include "halo2/source_texture_camera_producer_tu15.hpp"
#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/source_object_functions_tu15.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/world_material.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/math.hpp"
#include <SDL.h>
#include <cmath>
#include <memory>
#include <numbers>
#include <iostream>

using namespace halo2;
using namespace halo2::texture_camera_test;
namespace {
constexpr std::string_view camera_template="shaders\\shader_templates\\texture_camera\\texture_camera";
struct Options {
    std::filesystem::path producer,consumer,original,shared,prepared,shaders,output,reference;
    bool offline_reference{};
};
SourceViewProjectionBank controlled_consumer_bank(const Camera &camera) {
    SourceFrustumProjectionInput input;
    input.projection_angle=*camera.projection_vertical_angle;
    input.outer=input.safe={0,0,480,640};
    const auto projection=source_frustum_projection(input);
    return source_view_projection({camera.position,camera.forward,camera.up,.015625f,1024.f,{},
        projection.projection_program});
}
struct Consumer {
    std::vector<Vertex> vertices;
    Camera camera;
    ModelView view;
    std::uint32_t shader{};
};
Consumer consumer_control(const Cache &cache,Mesh &destination,unsigned implementation) {
    const auto actual=cache.load_bsp(3);
    std::vector<unsigned> roots;
    for(unsigned i=0;i<actual.materials.size();++i)
        if(actual.materials[i].template_name==camera_template)roots.push_back(i);
    require(roots.size()==1,"Actual map lost its unique authored viewscreen root");
    auto material=actual.materials.at(roots[0]);
    require(prepare_texture_camera_material(cache,material,implementation),
        "Actual viewscreen implementation lacks its exact source pass");
    Consumer result;result.shader=material.source_shader_datum;
    const auto slot=unsigned(destination.materials.size());destination.materials.push_back(std::move(material));
    for(std::size_t i=0;i<actual.vertices.size();i+=3) {
        require(i+3<=actual.vertices.size(),"Viewsreen BSP is not a triangle bank");
        if(actual.vertices[i].material!=roots[0])continue;
        for(unsigned n=0;n<3;++n) {
            auto vertex=actual.vertices[i+n];require(vertex.material==roots[0],"Viewsreen triangle crosses authored material roots");
            vertex.material=slot;result.vertices.push_back(vertex);
        }
    }
    require(!result.vertices.empty(),"Actual viewscreen has no authored geometry");
    Vec3 low{INFINITY,INFINITY,INFINITY},high{-INFINITY,-INFINITY,-INFINITY};
    for(const auto &v:result.vertices){low=minimum(low,v.position);high=maximum(high,v.position);}
    auto normal=normalized(cross(result.vertices[1].position-result.vertices[0].position,
        result.vertices[2].position-result.vertices[0].position));
    require(std::isfinite(normal.x)&&std::isfinite(normal.y)&&std::isfinite(normal.z)&&length(normal)>.99f,
        "Actual viewscreen first triangle is degenerate");
    const auto center=(low+high)*.5f;const auto distance=std::max(.1f,length(high-low));
    result.camera.position=center+normal*distance;result.camera.forward=normal*-1.f;
    result.camera.up=std::abs(normal.z)>.95f?Vec3{0,1,0}:Vec3{0,0,1};result.camera.oriented=true;
    result.camera.projection_vertical_angle=std::numbers::pi_v<float>/3.f;
    auto &view=result.view;view.camera=result.camera;view.full_surface=true;view.width=640;view.height=480;
    view.vertices=result.vertices;view.projection_clip_planes=Vec2{.015625f,1024.f};
    view.atmosphere=BspFogSample{};view.atmosphere->clear=Vec3{.02f,.03f,.04f};
    view.source_view_shader_constants.emplace();
    view.source_view_shader_constants->world=source_view_shader_constants(controlled_consumer_bank(result.camera),1024.f,false);
    view.source_view_shader_constants->reflection=view.source_view_shader_constants->world;
    return result;
}
struct ModelOwner {
    const Cache &cache;MultiplayerObjectRuntime &objects;Mesh &mesh;
    std::map<std::uint32_t,std::unique_ptr<SourceCampaignModel>> models;
    std::map<std::uint32_t,SourceObjectColorDefinition> colors;
    std::map<std::uint32_t,std::vector<ObjectFunction>> functions;
    std::shared_ptr<const ShaderFunctionTables> tables;
    void resident(std::span<const std::uint32_t> definitions) {
        for(const auto definition:definitions) {
            require(!models.contains(definition),"Secondary residency repeats a real definition");
            auto model=std::make_unique<SourceCampaignModel>(cache,definition);
            model->prepare_materials(cache,mesh);models.emplace(definition,std::move(model));
            colors.emplace(definition,cache.load_source_object_color_definition(definition));
            functions.emplace(definition,cache.load_object_functions(definition));
        }
        tables=cache.shader_function_tables();require(bool(tables),"Actual scene function tables are absent");
    }
    SourceCampaignSecondaryMaterialOwnerTu15 callbacks(const BspLighting &lighting) {
        SourceCampaignSecondaryMaterialOwnerTu15 result;result.baked_lighting=&lighting;
        result.model=[this](auto definition)->const SourceCampaignModel & {
            const auto found=models.find(definition);require(found!=models.end(),"Source collector discovered an unprepared definition");
            return *found->second;
        };
        result.presentation=[this](const SourceLiveModelBaseDrawTu15 &draw) {
            const auto body=objects.object_body(draw.object);const auto raw=Bytes(objects.memory().view(body+0x64u,12u));
            const auto region=objects.object_model_regions(draw.object);
            const auto variant=models.at(draw.definition)->variant_name(region.selection.variant_index);
            auto initial=source_object_initial_colors(colors.at(draw.definition),{raw.f32(0),raw.f32(4),raw.f32(8)},variant);
            // Snapshot the genuine32-slot owner while this exact view remains
            // current. A later renderer callback never calls/mutates World.
            const auto samples=source_object_function_samples_tu15(objects.tu15_world());
            const auto definitions=functions.at(draw.definition);const auto retained_tables=tables;
            ShaderInput input=[samples,definitions,retained_tables,owner=draw.object](auto name) {
                return evaluate_object_function(definitions,*retained_tables,name,0.f,[&](auto variable)->std::optional<float> {
                    const auto found=samples.find({owner,variable});return found==samples.end()?std::nullopt:std::optional(found->second);
                }).value;
            };
            return SourceCampaignSecondaryOwnerInputsTu15{source_live_model_base_colors_tu15(draw,initial),std::move(input)};
        };
        return result;
    }
};
std::array<std::int16_t,4> rectangle(std::span<const std::byte,0x118> view) {
    const Bytes raw(view);std::array<std::int16_t,4> result;
    for(unsigned i=0;i<4;++i)result[i]=std::bit_cast<std::int16_t>(raw.u16(0x3cu+i*2u));
    return result;
}
SourceSceneCopyContext copy_context(PhysicsLiveWorldTu15 &world,std::span<const std::byte,0x118> view) {
    const Bytes depth(world.memory().view(0x4922f4u,8));const auto dimensions=source_texture_camera_target18_extent_tu15();
    return {rectangle(view),dimensions[0],dimensions[1],depth.f32(0),depth.f32(4)};
}
template<class F> void rejects(F &&action,const char *why) {
    bool caught=false;try{action();}catch(const FormatError&){caught=true;}require(caught,why);
}
void run(const Options &options,SDL_Window *window,Json &report) {
    Cache original(options.original),cache(options.producer,options.shared,{},options.prepared),
        consumer_cache(options.consumer,options.shared,{},options.prepared);
    require(cache.is_mcc()&&consumer_cache.is_mcc(),"Runtime camera component requires real prepared MCC owners");
    const auto content=cache.load_content(3),original_content=original.load_content(3);
    const auto original_setup=authored_setup(original,original_content),actual_setup=authored_setup(cache,content);
    Mesh mesh=cache.load_bsp(3);SourceBspResidentPlan resident(cache,mesh,3,SourceBspResidentLighting::classification3,nullptr,true);
    const auto bsp_vertices=mesh.vertices;const auto atmosphere=cache.load_bsp_atmosphere(3);
    BspSkyModels skies(cache,atmosphere,mesh);const auto baked=cache.load_bsp_lighting(3);
    SourceCampaignWaterWeatherTu15 families(cache,3);
    require(families.water_definitions().definitions.empty(),"Directed BSP3 control unexpectedly gained authored water");
    const auto water_materials=families.append_water_materials(mesh);
    CampaignSourceRuntime runtime(cache,3,1,0x12345678u,0x89abcdefu);
    auto &objects=runtime.objects();auto &world=objects.tu15_world();families.initialize(world);
    const auto setup=execute_setup(objects,content,actual_setup,original_setup);
    SourceCampaignViewTu15 source_view(cache,3);pstencil::CampaignRenderer light_owner(cache,mesh,3);
    ModelOwner models{cache,objects,mesh};auto callbacks=models.callbacks(baked);
    SourceCampaignSecondaryViewTu15 secondary(cache,3,bsp_vertices,source_view,atmosphere,skies,families,
        water_materials,callbacks,&light_owner);
    const auto definitions=secondary.resident_definitions(world);models.resident(definitions);
    std::vector<Consumer> consumers;
    for(unsigned implementation=0;implementation<4;++implementation)
        consumers.push_back(consumer_control(consumer_cache,mesh,implementation));
    families.bind_water_pools(mesh.materials);
    Renderer renderer(window,mesh,true,{},&resident,{},nullptr,options.shaders);
    renderer.scene_bsp_atmosphere(atmosphere);
    // Real native two-buffer ownership, independent of the presentation pool.
    // A640x480 logical source is an explicit component control; it is never
    // read from source-cold Xbox allocation rows.
    const auto actual_targets=renderer.target_resources().source_buffers(2);
    require(actual_targets.bank().color.size()==2,"Native camera control lacks its actual independent source images");
    for(const auto &target:actual_targets.bank().color)
        require(target.width==640&&target.height==480&&target.image&&target.view,"Actual native source attachment differs from declared640x480 control");
    SourceWeatherState weather;SourceWeatherClock weather_clock;SourceWeatherRecord weather_record{};
    const SourceWeatherRenderState family_state{weather,weather_clock,weather_record};
    report["setup"]=setup.actions;
    auto &cases=report["cases"];auto &controls=report["controls"];std::uint64_t frame_number{};
    {
        source_texture_camera_off_tu15(world);
        const auto view=source_texture_camera_original_view_tu15(world,0,1,0,-1);
        const auto copy=copy_context(world,view);
        const auto inactive=renderer.prepare_texture_camera_targets(source_texture_camera_inputs_tu15(world.memory()),copy,{640,480});
        require(!inactive.available(18),"Inactive native preparation allocated target18");
        source_texture_camera_set_object_marker_tu15(world,setup.camera,setup.marker,setup.fov_bits);
        rejects([&]{auto forbidden=source_texture_camera_frame_tu15(world,view,inactive);(void)forbidden;},
            "Inactive native capability silently covered later source camera activation");
        controls.push_back({{"inactive_capability_rejects_later_authored_activation",true},
            {"actual_inactive18_absent",true}});
    }
    std::map<std::string,Json> prior_cases;
    if(!options.reference.empty()) {
        std::ifstream file(options.reference);const auto prior=Json::parse(file);
        require(prior.at("passed")==true&&prior.at("offline_reference")==true,"Actual camera reference did not pass");
        for(const auto &row:prior.at("cases"))require(prior_cases.emplace(row.at("id").get<std::string>(),row).second,
            "Actual camera reference repeats an owner case");
    }
    std::size_t recursive_draws{},ordinary_draws{},native_words{},presented_frames{};
    for(unsigned implementation=0;implementation<consumers.size();++implementation) {
        auto &consumer=consumers[implementation];consumer.view.vertices=consumer.vertices;consumer.view.atmosphere_source=&atmosphere;
        std::map<std::string,PixelImage> bsp_pixels;
        for(const bool owner_route:{false,true}) {
        std::map<std::string,PixelImage> state_pixels;
        for(const std::string state:{"none","target18","declared20","retired"}) {
            std::optional<PixelImage> captured;
            for(unsigned phase=0;phase<3;++phase) {
                const bool active=state=="target18" || state=="declared20";
                source_texture_camera_off_tu15(world);
                const auto original_view=source_texture_camera_original_view_tu15(world,0,1,0,-1);
                const auto copy=copy_context(world,original_view);
                if(active)source_texture_camera_set_object_marker_tu15(world,setup.camera,setup.marker,setup.fov_bits);
                const auto before=source_texture_camera_inputs_tu15(world.memory());
                const auto targets=renderer.prepare_texture_camera_targets(before,copy,{640,480},
                    state=="declared20"?std::optional(copy):std::nullopt);
                if(active)require(targets.selected()==(state=="declared20"?20u:18u),"Actual native target priority differs");
                else require(!targets.selected(),"Inactive camera retained a retired native target");
                auto source=source_texture_camera_frame_tu15(world,original_view,targets);
                const auto clock_before=runtime.reference_state();
                require(source.source_rendered_view()==active,"Actual256D0/25800 failed its authored camera marker/view");
                std::unique_ptr<SourceCampaignSecondaryPacketsTu15> scene;
                std::optional<SourceBspResidentViewPlan> selected;
                std::optional<SourceTextureCameraScenePrefixTu15> expired;
                if(active)world.with_texture_camera_scene_prefix_tu15(*source.scene_view(),true,1,[&](const auto &prefix) {
                    expired=prefix;scene=secondary.collect(world,family_state,source,prefix,true,++frame_number,0.f,false);
                    const Bytes raw(prefix.raw_camera);
                    const auto policy=source_geometry_lod_policy(1);
                    selected=resident.select({scene->collection_rendering,scene->source->bsp,
                        std::bit_cast<std::int16_t>(raw.u16(0x30u)),std::bit_cast<std::int16_t>(raw.u16(0x34u)),1,
                        false,policy.coverage_multiplier});
                    scene->view.source_bsp_resident_plan=&resident;scene->view.source_bsp_bindings=selected->bindings;
                    scene->view.vertices=scene->families.water_vertices;
                    require(scene->families.water.empty()&&scene->families.weather_points.empty()&&
                        scene->families.weather_lines.empty()&&scene->families.weather_plates.empty(),
                        "Actual directed BSP3 admitted a water/weather producer outside its authored absence");
                });
                if(expired)rejects([&]{expired->require_current(world.memory());},"A copied source prefix survived native cleanup");
                // The real ordinary CPU-prefix publishes its separate flag0
                // context after the secondary scope. Its GPU boundary is a
                // declared component input, not a completed original2B770.
                std::string ordinary_packet;
                world.with_texture_camera_scene_prefix_tu15(original_view,false,1,[&](const auto &prefix) {
                    prefix.require_current(world.memory());require(!prefix.texture_camera&&
                        Bytes(prefix.packet).slice(7,1)[0]==std::byte{},
                        "Ordinary CPU view retained the secondary recursive flag");
                    ordinary_packet=digest(prefix.packet);
                });
                const auto clock_after=runtime.reference_state();
                require(clock_before.clock==clock_after.clock,"Secondary/main collection advanced the source simulation clock");
                if(active)renderer.renew_texture_camera_frame(source);
                CameraGpuObservation observed;observed.reference=options.offline_reference;
                observed.camera_shaders.insert(consumer.shader);
                for(const auto &material:mesh.materials)if(material.template_name==camera_template)
                    observed.camera_shaders.insert(material.source_shader_datum);
                auto ordinary=consumer.view;
                ModelObjectDraw owner_control;
                if(owner_route) {
                    owner_control.vertices=consumer.vertices;ordinary.vertices={};
                    ordinary.objects=std::span{&owner_control,1};
                }
                observed.bind(ordinary,false,owner_route);if(scene)observed.bind(scene->view,true);
                RendererTextureCameraSubmissionTu15 work{&source,scene?&scene->view:nullptr,
                    [&](auto target,auto image,auto view){observed.copy(target,image,view);}};
                if(active&&phase==0) {
                    const auto native_frames=renderer.target_resources().acquire().bank().source_frames_submitted;
                    auto superseding=renderer.prepare_texture_camera_targets(source.before(),copy,{640,480},
                        state=="declared20"?std::optional(copy):std::nullopt);
                    rejects([&]{renderer.draw_texture_camera(consumer.camera,work,std::span{&ordinary,1});},
                        "Source camera work survived a stale native target epoch");
                    require(renderer.target_resources().acquire().bank().source_frames_submitted==native_frames,
                        "Stale native target recorded a frame before rejection");
                    // Legitimate renewal proves unchanged actualimage owners;
                    // it never reruns25800 or invents another source request.
                    renderer.renew_texture_camera_frame(source);
                    controls.push_back({{"implementation",implementation},{"state",state},
                        {"stale_native_epoch_rejected_before_gpu",true},{"same_image_only_renewal",true}});
                }
                const auto id=options.consumer.filename().string()+":impl"+std::to_string(implementation)+":"+state+(owner_route?":owner-route":"");
                const auto path=options.output/(id+"-"+std::to_string(phase)+".ppm");
                const auto submitted_before=renderer.target_resources().acquire().bank().source_frames_submitted;
                require(renderer.draw_texture_camera(consumer.camera,work,std::span{&ordinary,1},path),
                    "Actual native camera frame was not submitted");
                const auto submitted_after=renderer.target_resources().acquire().bank().source_frames_submitted;
                require(submitted_after==submitted_before+1,"Texture camera introduced an extra presented source frame");
                ++presented_frames;
                require(observed.copy_calls==(active?1u:0u)&&observed.ordinary_camera_draws>0&&observed.uploads>0,
                    "Actual camera frame omitted its selected copy/viewscreen material");
                if(active)require(observed.selected_target==(state=="declared20"?20u:18u),
                    "Actual copy recorded a different target from its source/native selection");
                if(options.offline_reference)require(observed.vertex_words>0,"Offline camera reference did not exercise original VS decoding");
                else require(observed.vertex_words==0,"Native camera path interpreted original Xbox shader instructions");
                auto image=pixels(path);require(colors(image)>1,"Actual viewscreen control produced only a flat image");
                if(phase==1)captured=image;
                if(phase==2)require(captured&&captured->rgb==image.rgb,"Actual source camera GPU repeat is not exact");
                recursive_draws+=observed.scene_camera_draws;ordinary_draws+=observed.ordinary_camera_draws;
                native_words+=observed.vertex_words;
                const auto submitted_once=renderer.target_resources().acquire().bank().source_frames_submitted;
                rejects([&]{renderer.draw_texture_camera(consumer.camera,work,std::span{&ordinary,1});},
                    "Once-only source camera work was submitted twice");
                require(renderer.target_resources().acquire().bank().source_frames_submitted==submitted_once,
                    "Duplicate source work recorded a GPU frame before rejecting");
                if(phase==1) {
                    if(owner_route)require(bsp_pixels.at(state).rgb==image.rgb,
                        "Owner alias lost actual image selection or independent uniforms versus identical BSP control");
                    else bsp_pixels.emplace(state,image);
                    Json row{{"id",id},{"pixel_sha256",image.sha256},{"pixels_distinct",colors(image)},
                        {"source_scene_requested",active},{"observation",observed.report()},
                        {"ordinary_consumer_camera_scope","Controlled view of unchanged actual map viewscreen geometry; producer camera remains authored06a"},
                        {"owner_route_component_control",owner_route},
                        {"owner_route_matches_same_geometry_BSP",owner_route},
                        {"owner_route_scope","Same actual viewscreen vertices through ModelObjectDraw with null optional owner inputs; no authored object/player state claim"},
                        {"source_cleanup",source.after().camera_words},{"copy_complete_after",source.after().copy_complete},
                        {"presented_frames",1},{"once_only_rejected",true},{"source_prefix_expired",active},
                        {"ordinary_cpu_prefix_packet_sha256",ordinary_packet},
                        {"ordinary_original_gpu_boundary_completed",false},
                        {"source_clock_sha256",digest(clock_before.clock)},{"source_clock_unchanged",true},
                        {"random_before",clock_before.random_words},{"random_after",clock_after.random_words}};
                    if(active) {
                        row["producer_view_sha256"]=digest(*source.scene_view());
                        row["completed_view_sha256"]=digest(*source.completed_scene_view());
                        row["submitting_packet_sha256"]=digest(source.completed_scene_prefix()->packet);
                        row["collection_projection_sha256"]=digest(source.completed_scene_prefix()->collection_projection);
                    }
                    if(!options.reference.empty()) {
                        require(prior_cases.contains(id)&&prior_cases.at(id).at("pixel_sha256")==image.sha256,
                            "Actual native camera pixels differ from identical-input offline reference");
                        for(const auto *key:{"source_clock_sha256","ordinary_cpu_prefix_packet_sha256","random_before","random_after"})
                            require(prior_cases.at(id).at(key)==row.at(key),"Native/reference source input owner differs");
                        if(active)for(const auto *key:{"producer_view_sha256","completed_view_sha256",
                            "submitting_packet_sha256","collection_projection_sha256"})
                            require(prior_cases.at(id).at(key)==row.at(key),"Native/reference source camera/prefix input differs");
                        row["reference_exact"]=true;
                    }
                    cases.push_back(std::move(row));state_pixels.emplace(state,image);
                    std::ofstream(options.output/"report.json")<<report.dump(2)<<'\n';
                }
                // Native captures are disposable. The separate owned wrapper
                // may explicitly retain reference frames required by comparison.
                if(!options.offline_reference || phase!=1)std::filesystem::remove(path);
            }
        }
        require(state_pixels.at("none").rgb==state_pixels.at("retired").rgb,"Native target retirement did not restore actual default2D sampling");
        require(state_pixels.at("target18").rgb==state_pixels.at("declared20").rgb,"Real18/20 image priority controls differ at identical producer inputs");
        require(state_pixels.at("none").rgb!=state_pixels.at("target18").rgb,"Actual active viewscreen image has no changed samples against its authored default2D");
        controls.push_back({{"implementation",implementation},{"owner_route",owner_route},{"retirement_default_exact",true},
            {"declared20_priority_exact",true},{"active_changes_visible_pixels",true}});
        }
    }
    require(presented_frames==96,"Finite BSP/owner camera controls did not submit all96 actual frames");
    report["presented_frames"]=presented_frames;
    report["required_presented_frames_source_loop"]=96;
    report["uploaded_vertex_instruction_words"]=native_words;
    report["actual_recursive_viewscreen_draws"]=recursive_draws;
    report["actual_ordinary_viewscreen_draws"]=ordinary_draws;report["device"]=renderer.device_name();
    report["passed"]=true;
}
}
int main(int argc,char **argv)try {
    Options options;
    for(int i=1;i<argc;++i) {
        const std::string key=argv[i];if(key=="--offline-reference"){options.offline_reference=true;continue;}
        require(i+1<argc,"Missing active texture-camera option value");const std::filesystem::path value=argv[++i];
        if(key=="--producer")options.producer=value;else if(key=="--consumer")options.consumer=value;
        else if(key=="--original-producer")options.original=value;else if(key=="--shared-dir")options.shared=value;
        else if(key=="--prepared-root")options.prepared=value;else if(key=="--shaders")options.shaders=value;
        else if(key=="--output")options.output=value;else if(key=="--compare-reference")options.reference=value;
        else throw FormatError("Unknown active texture-camera option");
    }
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch,"Fresh owned active camera scratch is required");
    const auto *observations=std::getenv("HALO2_CAPTURE_BSP_DRAW_BINDINGS");
    require(observations&&std::string_view(observations)=="1","Explicit actual native draw/image observations are required");
    require(!options.output.empty()&&!options.producer.empty()&&!options.consumer.empty()&&!options.original.empty()&&
        !options.shared.empty()&&!options.prepared.empty()&&!options.shaders.empty(),"All actual camera source inputs are required");
    const auto output=std::filesystem::weakly_canonical(options.output);
    const auto relative=output.lexically_relative(std::filesystem::canonical(scratch));
    require(!relative.empty()&&!relative.is_absolute(),"Camera outputs escape owned scratch");
    for(const auto &part:relative)require(part!="..","Camera outputs escape owned scratch");
    require(options.offline_reference==options.reference.empty(),"Choose offline reference or native comparison explicitly");
    require(options.producer.filename()=="06a_sentinelwalls.map"&&
        (options.consumer.filename()=="06a_sentinelwalls.map"||options.consumer.filename()=="07b_forerunnership.map"),
        "Active camera cohort must retain the finite actual producer/consumer owners");
    std::filesystem::create_directories(options.output);
    require(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Native active texture camera",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
        640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);require(window,SDL_GetError());
    Json report{{"schema","HALO2_NATIVE_TEXTURE_CAMERA_ACTIVE_GPU_V2"},{"passed",false},
        {"offline_reference",options.offline_reference},{"producer",options.producer.string()},{"consumer",options.consumer.string()},
        {"producer_scope","Actual prepared06a script64 fresh-name setup/marker/native original view+prefix/shared collection; no cinematic timeline claim"},
        {"consumer_scope","Actual06a or07b BSP3 material/geometry control;07b has no authored producer activation"},
        {"declared20_scope","Real native640x480 image/selection control, no authored20 allocation or timeline claim"},
        {"seed",0x12345678u},{"auxiliary_seed",0x89abcdefu},{"effect_seconds",0},
        {"cases",Json::array()},{"controls",Json::array()},
        {"boundary","CPU original source differential, runtime activation/all-map cohort and physical Xbox pixels remain separate evidence"}};
    try {run(options,window,report);}catch(const std::exception &error){report["error"]=error.what();report["passed"]=false;}
    SDL_DestroyWindow(window);SDL_Quit();std::ofstream(options.output/"report.json")<<report.dump(2)<<'\n';
    std::cout<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
