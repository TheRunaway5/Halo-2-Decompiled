#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_geometry_lod.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace halo2;using Json=nlohmann::json;
namespace {void check(bool v,const char *why){if(!v)throw FormatError(why);}}
int main(int argc,char **argv)try {
    if(argc!=3 && argc!=4)throw FormatError("Supply TU15_XBE OPENING_MAP [FRESH_OUTPUT]");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto content=cache.load_content(0,false);const auto &name=content.object_names.at(26);
    const auto placed=std::ranges::find_if(content.objects,[&](const auto &o){return o.type==name.type && o.placement==name.placement;});
    check(placed!=content.objects.end() && placed->definition==0xe201008cu,"Authored opening explosion scenery differs");
    auto path=(std::filesystem::temp_directory_path()/"halo2-particle-main-XXXXXX").string();check(::mkdtemp(path.data()),"Particle composition storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    const auto output=argc==4?std::filesystem::path(argv[3]):std::filesystem::path(path)/"evidence";
    check(!std::filesystem::exists(output),"Preserve campaign particle composition evidence");std::filesystem::create_directories(output);
    std::cout<<"PHASE ordinary-native-world\n"<<std::flush;
    MultiplayerObjectRuntime objects(cache,0,path);auto &world=objects.tu15_world();world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    source_campaign_cinematic_control_tu15(world,true);
    const auto owner=objects.create_scenario_object(*placed,SourceScenarioBirth::runtime);check(owner!=null_datum,"Authored native explosion scenery birth failed");
    source_campaign_cinematic_lod_tu15(world,owner,true);
    for(const auto effect:{0xea070892u,0xea03088eu})source_script_marker_effect_tu15(world,effect,owner,0x130008fdu);
    const auto atmosphere=cache.load_bsp_atmosphere(0);SourceCampaignViewTu15 consumer(cache,0);
    const Vec3 eye{353.634918212890625f,294.381256103515625f,24.961688995361328125f};
    const Vec3 forward{-.22934174537658691f,-.9558929204940796f,-.18349668383598328f},up{0,0,1};
    const auto angles=source_camera_angles(1.2217304706573486f);
    unsigned submitted{};Json rows=Json::array();
    for(unsigned tick=0;tick<8;++tick) {
        world.step_objects();objects.advance_effect_frame(1.f/30.f);
        auto &m=world.memory();SourceExecutionInputs capture;capture.track_calls();
        SourceCampaignViewFrameTu15 frame;SourceCampaignEnvironmentViewTu15 environment;
        {SourceExecutionInputBinding binding(m,capture);
            const auto configured=source_campaign_clip_planes_tu15(world);
            environment=source_campaign_environment_view_tu15(world,atmosphere,eye,forward,configured.x,configured.y);
            SourceFrustumProjectionInput p;p.projection_angle=angles.projection_angle;p.outer=p.safe={0,0,480,640};p.scenario_present=true;
            const auto projected=source_frustum_projection(p);
            const auto bank=source_view_projection({eye,forward,up,configured.x,environment.source.far_distance,{},projected.projection_program});
            const auto frustum=source_frustum_camera({eye,forward,up,configured.x,environment.source.far_distance,projected.clipping_rectangle()});
            MuzzleEffectView particles;particles.camera=source_view_shader_bank(bank,environment.source.far_distance,eye,{0,0,640,480},false);
            particles.fog=bsp_fog_constants(environment.fog,eye,forward);
            const auto &b=bank.camera_to_world.basis;particles.orientation={b[0].x,b[0].y,b[0].z,b[1].x,b[1].y,b[1].z,b[2].x,b[2].y,b[2].z};
            particles.position=eye;particles.forward=forward;particles.up=up;particles.width=640;particles.height=480;
            const auto depth=menu_world_depth_clip_planes({configured.x,configured.y,MenuWorldDepthBoundary::none,0,false});
            particles.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};
            SourceCampaignViewInputTu15 input{bank,frustum,eye,forward,p.outer,angles.lighting_distance_scale,source_geometry_lod_policy(1).coverage_multiplier,projected.view_modifier};
            input.particles=SourceCampaignParticleViewTu15{particles,angles.projection_angle,1.2217304706573486f};input.environment=&environment.source;
            if(tick&1u) {
                world.call_actual(0x20680u);
                input.prepare_lens_view=[](auto &owner,unsigned view) {
                    auto gate=owner.prepare_source_call();gate.gp[0]=view;owner.run_actual(0x20700u,gate);
                };
            }
            frame=consumer.collect(world,input,atmosphere,environment.fog);
        }
        (void)capture.finish();const auto calls=capture.calls();std::map<std::uint32_t,unsigned> entries;unsigned publications{};
        for(const auto &call:calls){const Bytes a(call.arguments);
            if(call.kind==SourceExecutionCallKind::RunActualSourceCall || call.kind==SourceExecutionCallKind::CallActualSource)++entries[a.u32(0)];
            if(call.kind==SourceExecutionCallKind::WriteSourceCallInput && a.u32(0)==0x4c6710u){++publications;check(a.u32(4)==0x120u,"Particle main partially replaced full environment");}}
        check(frame.main_commands_completed && entries[0x1373f0u]==1 && entries[0x136ea0u]==1 && entries[0x44710u]==1 && entries[0x40e80u]==1,"Campaign main reset/cull/command phases omitted or repeated");
        check(entries[0x20680u]==1 && entries[0x20700u]==1,"Campaign lens request gates omitted or repeated by the borrowed owner branch");
        check(entries[0x1770c0u]==0 && publications==1 && std::ranges::equal(environment.source.bytes,m.view(0x4c6710u,0x120u)),"Standalone particle producer repeated or complete environment changed");
        submitted+=frame.world_particles.size();
        rows.push_back({{"tick",tick},{"cluster",environment.fog.cluster},{"source_commands",frame.source_particle_commands},{"particle_draws",frame.world_particles.size()},{"units",frame.units.draw_count},{"main_complete",frame.main_commands_completed}});
        std::ofstream(output/"progress.json")<<rows.dump(2)<<'\n';
        std::cout<<"FRAME "<<tick<<" commands="<<frame.source_particle_commands<<" particles="<<frame.world_particles.size()<<" units="<<frame.units.draw_count<<'\n'<<std::flush;
    }
    check(submitted>0,"Actual authored opening effect produced no WORLD particle submissions in the bounded main consumer");
    std::ofstream(output/"result.json")<<Json{{"status","PASS_AUTHORED_EFFECT_FULL_ENVIRONMENT_MAIN_QUEUES_WORLD_PARTICLES"},{"world_constructor_xbe_path_supplied",false},{"frames",rows},{"particle_draws",submitted},{"gpu_or_matched_frame_claim",false}}.dump(2)<<'\n';
    std::cout<<"PASS authored marker allocations through full source environment, real BSP/model main queues, one cull/command phase and WORLD particle packets; draws="<<submitted<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
