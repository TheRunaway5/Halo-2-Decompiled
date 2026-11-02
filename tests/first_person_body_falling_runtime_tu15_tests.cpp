#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/first_person_body_geometry.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/math.hpp"
#include "halo2/renderer.hpp"
#include "halo2/render_interpolation.hpp"
#include "halo2/model_node_interpolation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_model_depth_selection.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/world_material_uniform.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool value,const char* text){if(!value)throw FormatError(text);}
std::array<float,3> xyz(Vec3 value){return {value.x,value.y,value.z};}
struct Endpoint {Vec3 root,velocity,eye;std::vector<Transform> nodes;bool grounded{};};
void verify_capture(const std::filesystem::path& path,Json& report){
  std::ifstream input(path,std::ios::binary);std::string magic;unsigned width{},height{},maximum{};
  input>>magic>>width>>height>>maximum;
  need(input&&magic=="P6"&&width==640&&height==480&&maximum==255,"Falling GPU capture lacks the actual RGB extent");
  const auto separator=input.get();need(separator!=EOF&&std::isspace(static_cast<unsigned char>(separator)),"Falling GPU capture has an invalid RGB header");
  std::vector<unsigned char> pixels(width*height*3u);input.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
  need(input.gcount()==std::streamsize(pixels.size())&&input.peek()==EOF,"Falling GPU capture has incomplete RGB output");
  unsigned nonblack{};bool varied{};
  for(unsigned i=0;i<pixels.size();i+=3){nonblack+=std::max({pixels[i],pixels[i+1],pixels[i+2]})>8;
    varied|=pixels[i]!=pixels[0]||pixels[i+1]!=pixels[1]||pixels[i+2]!=pixels[2];}
  report["visible_capture_pixels"]=nonblack;
  need(nonblack>width*height/100u&&varied,"Falling GPU capture is black or uniform and cannot establish visible body motion");
}
int sweep_guard_controls(){
  need(std::getenv("HALO2_TEST_SCRATCH"),"Run sweep guard controls through tools/run_test.py");
  need(SDL_setenv("HALO2_REFERENCE_BODY_FRACTION_SWEEP","1",1)==0,"Admit guard-control environment");
  MultiplayerOptions valid;valid.capture_tick=1;valid.width=640;valid.height=480;
  valid.source_object_world=true;valid.reference_tick_cadence=true;
  valid.screenshot=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"unused.ppm";
  valid.input_observer=[](auto,auto,const auto&){};
  const std::array<std::function<void(MultiplayerOptions&)>,14> corrupt{
    [](auto& p){p.frames=1;},[](auto& p){p.ticks=1;},[](auto& p){p.input_observer={};},
    [](auto& p){p.screenshot.clear();},[](auto& p){p.width=800;},[](auto& p){p.local_players=2;},
    [](auto& p){p.source_object_world=false;},[](auto& p){p.reference_tick_cadence=false;},
    [](auto& p){p.reference_render_fraction=.5f;},[](auto& p){p.capture_root=std::array<Vec3,2>{};},
    [](auto& p){p.reference_player="unused";},[](auto& p){p.host_port=1;},
    [](auto& p){p.join="unreachable";},[](auto& p){p.campaign_difficulty=1;}};
  for(const auto& change:corrupt){auto invalid=valid;change(invalid);bool rejected{};
    try{(void)run_multiplayer(invalid);}catch(const FormatError& error){
      const std::string_view message=error.what();rejected=message=="Body fraction sweep requires an isolated native single-view finite reference tick capture"||
          message=="Input-script capture requires a local finite tick capture";}
    need(rejected,"Controlled body sweep accepted an interactive/network/unscoped owner or reached unrelated world setup");
  }
  std::cout<<"PASS14 explicit body fraction-sweep admission negatives before world startup\n";return 0;
}
}
int main(int argc,char**argv){
  Json report{{"passed",false},{"scope","Actual offline native falling down-look render frames; camera/body root coherence within repeated fractional game-tick samples"}};
  const auto save=[&]{if(argc==4||argc==5)std::ofstream(argv[3])<<report.dump(2)<<'\n';};
  try {
    if(argc==2&&std::string_view(argv[1])=="--sweep-guards")return sweep_guard_controls();
    need(argc==4||(argc==5&&(std::string_view(argv[4])=="--controlled-sweep"||std::string_view(argv[4])=="--without-validation")),
        "Supply prepared MCC MAP SHARED REPORT and optional --controlled-sweep or --without-validation");
    const bool controlled=argc==5&&std::string_view(argv[4])=="--controlled-sweep";
    const bool without_validation=argc==5&&std::string_view(argv[4])=="--without-validation";
    report["vulkan_validation_requested"]=!without_validation;
    report["render_resolution"]={640,480};
    const auto* presentation=std::getenv("HALO2_PSTENCIL_CAPTURE_PRESENT_MODE");
    report["capture_presentation_requested"]=presentation?presentation:"fifo";
    if(!controlled)report["scope"]="Actual warmed Original/Off Lockout native walk from a declared accepted ledge into the void through native death; no supplied contacts or velocity";
    if(controlled){
      need(SDL_setenv("HALO2_REFERENCE_BODY_FRACTION_SWEEP","1",1)==0,"Could not admit controlled native body sweep");
      need(SDL_setenv("HALO2_VERIFY_BORROWED_FRAME_VERTICES","1",1)==0,"Could not enable actual frame upload verification");
      report["actual_borrowed_vertex_upload_verification_requested"]=true;
    }
    if(!std::getenv("DISPLAY")||!std::filesystem::is_regular_file(argv[1]))return 77;
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Run falling body check through tools/run_test.py");
    if(controlled){
      const auto stem=(std::filesystem::path(scratch)/"body-draw").string();
      need(SDL_setenv("HALO2_VIEW_DIAGNOSTICS",stem.c_str(),1)==0,"Could not admit private body draw upload diagnostics");
      need(SDL_setenv("HALO2_CAPTURE_BSP_DRAW_BINDINGS","1",1)==0,"Could not observe actual body draw bindings");
    }
    SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_world=true;
    constexpr unsigned warmup_render_frames=48;
    options.width=640;options.height=480;options.frames=2400;options.validation=!without_validation;options.hidden=true;
    options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};
    options.source_object_savegame_directory=std::filesystem::path(scratch)/"falling-native-world";
    auto profile=source_default_player_configuration();profile[0x44]=0;options.selected_profiles={profile};
    report["stage"]="prepare-native-fall";
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    auto& world=*options.prepared_world;world.bind_source_local_views(world.local_slots);const auto slot=world.local_slots.at(0);
    need(world.cache->is_mcc()&&world.source_objects&&world.source_objects->owns_physics()&&world.match.source_engine_owned(),
        "Falling body fixture lacks its actual local native authority");
    std::array<Controls,multiplayer_capacity> controls{};
    if(!controlled)controls[slot].yaw=3.14159265358979323846f;
    for(unsigned tick=0;tick<105;++tick){controls[slot].pitch=-std::min(1.5f,.05f*float(tick));world.match.step(controls);}
    auto prepared=world.match.state().players[slot];
    need(prepared.native_grounded&&!prepared.dead&&prepared.source_parent==null_datum,"Falling fixture did not establish an ordinary living unit");
    if(!controlled){
      const Vec3 ledge{-6.188333988189697f,-2.5999999046325684f,-5.989999771118164f};
      world.source_objects->tu15_world().set_object_position(prepared.source_unit,ledge);
      for(unsigned tick=0;tick<30;++tick)world.match.step(controls);
      prepared=world.match.state().players[slot];
      need(prepared.native_grounded&&!prepared.dead&&prepared.source_parent==null_datum,
          "Declared accepted Lockout ledge did not settle a genuine standing native unit");
      report["declared_ledge_setup"]=xyz(ledge);
      report["ledge_evidence"]="build/test-results/superbounce-tu15-20261026/lockout-natural-ledge.json";
      report["void_route_evidence"]="analysis/first-person-body-tu15-20261029/lockout-void-route-probe.json";
      report["settled_ledge_root"]=xyz(prepared.position);
    }
    std::optional<std::uint32_t> drop_tick;
    if(controlled){
      // The controlled reference keeps its existing real native endpoint pair.
      world.source_objects->tu15_world().set_object_position(prepared.source_unit,prepared.position+Vec3{0,0,3});
      drop_tick=world.match.state().tick;
      options.frames=0;options.capture_tick=world.match.state().tick+14;options.reference_tick_cadence=true;
      options.screenshot=std::filesystem::path(scratch)/"falling-sweep-final.ppm";
      report["scope"]="Five actual native GPU reference frames at0/.25/.5/.75/1 of one frozen descending native endpoint pair; not a realtime FPS qualification";
      report["controlled_capture_tick"]=options.capture_tick;
    }
    options.desktop=std::make_shared<DesktopSession>();options.desktop->display_settings.graphics_mode=pstencil::GraphicsMode::Original;
    options.desktop->display_settings.upscaler=Upscaler::Off;options.desktop->upscaling_preferences_initialized=true;
    options.desktop->keyboard_settings.sensitivity=3;options.desktop->keyboard_settings.invert=false;
    auto* window=options.desktop->window(640,480,false);const auto window_id=SDL_GetWindowID(window);
    SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    need(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Falling fixture could not capture its actual mouse input");
    SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;
    need(SDL_PushEvent(&focus)==1,"Deliver falling fixture focus");
    SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
    look.motion.xrel=int(std::lround((prepared.yaw-controls[slot].yaw)/.0025f));
    look.motion.yrel=-int(std::lround((controls[slot].pitch-prepared.pitch)/.0025f));
    need(SDL_PushEvent(&look)==1,"Deliver falling fixture down-look");
    std::map<std::uint32_t,Endpoint> endpoints;
    std::optional<RenderModel> body;std::vector<unsigned> ordinals;std::uint32_t body_model=null_datum;
    struct Joint {std::string name;unsigned full,first_person;};std::vector<Joint> joints;
    std::vector<Transform> completed_fp_nodes,interpolated_fp_nodes;
    Json joint_positions=Json::array();float maximum_lower_joint_pose_difference{};
    struct SweepFrame {float fraction{};Camera camera;std::vector<Vec3> vertices;std::array<ShaderVector,4> source_camera;};std::map<unsigned,SweepFrame> sweep_frames;
    std::vector<Vec3> sampled_positions;float maximum_sweep_pose_error{};
    std::optional<SourceViewProjectionBank> native_main_camera;
    std::array<ShaderVector,7> native_main_constants{};
    unsigned camera_uniform_samples{},camera_uniform_mismatches{};std::map<unsigned,unsigned> sweep_uniform_samples;
    unsigned submitted{},falling_samples{},fractional_samples{},same_tick_pairs{},moving_same_tick_pairs{},vertices_checked{};
    unsigned void_descending_ticks{},void_fast_ticks{},void_fast_body_samples{},void_death_views{};
    float void_minimum_z=prepared.position.z,void_minimum_velocity{};
    std::optional<std::uint32_t> void_death_tick;bool quit_delivered{},completed_body_this_frame{},death_view_this_frame{};
    float maximum_root_error{},maximum_eye_error{},maximum_relative_error{};
    float maximum_submitted_depth_error{};unsigned depth_body_samples{},depth_vertices_checked{};
    bool captured{},sample_ready{};float fraction{};std::uint32_t sample_tick{};
    Vec3 sampled_pelvis,completed_fp_pelvis,interpolated_fp_pelvis;
    struct Last {std::uint32_t tick{};float fraction{};Vec3 pelvis,eye;};std::optional<Last> last;
    std::optional<std::chrono::steady_clock::time_point> first_submit,last_submit,first_fall,last_fall;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    options.input_observer=[&](unsigned,std::span<const Controls> input,const MultiplayerState& state){
      need(std::chrono::steady_clock::now()<deadline,"Falling body render exceeded finite deadline");
      need(input.size()==1,"Falling body fixture lost its actual local input owner");
      const auto&p=state.players[slot];
      if(!p.dead)need(std::abs(input[0].pitch-controls[slot].pitch)<=.003f,"Falling body fixture lost actual down-look input");
      if(!controlled&&drop_tick){
        void_minimum_z=std::min(void_minimum_z,p.position.z);void_minimum_velocity=std::min(void_minimum_velocity,p.velocity.z);
        if(!p.native_grounded&&p.velocity.z<-.1f)++void_descending_ticks;
        if(p.velocity.z<-6.f)++void_fast_ticks;
        if(report["void_native_ticks"].size()<256)report["void_native_ticks"].push_back({{"tick",state.tick},
            {"root",xyz(p.position)},{"velocity",xyz(p.velocity)},{"grounded",p.native_grounded},
            {"source_unit",p.source_unit},{"previous_unit",p.source_previous_unit},{"dead",p.dead},
            {"actual_forward_input",input[0].forward},{"ordinary_camera",bool(p.source_player_camera)},
            {"vehicle_camera",bool(p.source_vehicle_camera)},{"death_camera",bool(p.source_death_camera)}});
        if(p.dead){
          need(p.source_unit==null_datum&&p.source_previous_unit==prepared.source_unit,
              "Void death did not retire the actual native living unit");
          if(!void_death_tick)void_death_tick=state.tick;
          return;
        }
      }
      if(!p.source_player_camera)return;
      need(!p.dead&&p.source_unit==prepared.source_unit,"Falling body fixture lost its native living owner");
      if(!controlled&&!drop_tick&&submitted>=warmup_render_frames){
        need(p.native_grounded&&p.source_parent==null_datum,"Warmed native fall setup lost its ordinary standing unit");
        SDL_Event walk{};walk.type=SDL_KEYDOWN;walk.key.windowID=window_id;
        walk.key.state=SDL_PRESSED;walk.key.keysym.scancode=SDL_SCANCODE_W;walk.key.keysym.sym=SDLK_w;
        need(SDL_PushEvent(&walk)==1,"Could not deliver the actual void walk input");
        drop_tick=state.tick;
        report["warmup_submitted_frames_before_walk"]=submitted;
        report["walk_native_tick"]=*drop_tick;report["walk_native_root"]=xyz(p.position);
      }
      const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto& v){return v.object==p.source_unit;});
      need(pose!=state.source_body_poses.end()&&!pose->nodes.empty(),"Falling native tick omitted its completed full-unit WORLD pose");
      endpoints.insert_or_assign(state.tick,Endpoint{p.position,p.velocity,p.source_player_camera->position,pose->nodes,p.native_grounded});
    };
    options.node_observer=[&](unsigned view,std::uint32_t,const MultiplayerNodeObservation& sample){
      if(view||sample.mode||sample.owner!=prepared.source_unit||sample.nodes.empty())return;
      if(sample.stage!=MultiplayerNodeStage::first_person_completed&&sample.stage!=MultiplayerNodeStage::first_person_interpolated)return;
      need(!void_death_tick,"Native first-person body remained admitted after its unit died in the void");
      if(sample.stage==MultiplayerNodeStage::first_person_interpolated){
        interpolated_fp_pelvis=sample.nodes.front().position;interpolated_fp_nodes.assign(sample.nodes.begin(),sample.nodes.end());return;}
      if(sample.stage!=MultiplayerNodeStage::first_person_completed)return;
      completed_body_this_frame=true;
      completed_fp_pelvis=sample.nodes.front().position;
      completed_fp_nodes.assign(sample.nodes.begin(),sample.nodes.end());
      if(body)return;
      const auto definition=world.content.player_representations.at(2).biped.datum;
      const auto geometry=first_person_body_geometry(*world.cache,definition,sample.render_model);
      need(geometry.connected_torso,"Falling body fixture lost its uncapped Chief representation");
      body_model=geometry.render_model;
      body=world.cache->load_model_sections(geometry.render_model,false,0,4,geometry.sections);
      ordinals=first_person_body_vertex_selection(*world.cache,*body,"spine1");
      const auto original=world.cache->load_model_hierarchy(sample.render_model);
      constexpr std::array<std::string_view,10> names{"pelvis","spine","l_thigh","r_thigh","l_calf","r_calf","l_foot","r_foot","l_toe","r_toe"};
      for(auto name:names){
        const auto find=[&](const RenderModel& model){
          const auto at=std::find_if(model.bones.begin(),model.bones.end(),[&](const auto& bone){return world.cache->string_id(bone.name)==name;});
          need(at!=model.bones.end(),"Native full/FP body lost an authored lower-joint name");return unsigned(at-model.bones.begin());};
        joints.push_back({std::string(name),find(*body),find(original)});
      }
    };
    options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation& sample){
      if(view||player!=slot||sample.first_person||sample.unit!=prepared.source_unit)return;
      if(void_death_tick)return;
      need(body&&!sample.body_nodes.empty(),"Falling body draw lacks its actual native model admission");
      need(sample.vertices.size()==ordinals.size(),"Falling body changed the uncapped authored anatomy subset");
      sampled_pelvis=sample.body_nodes.front().position;fraction=sample.fraction;sample_tick=sample.tick;sample_ready=true;
      sampled_positions.clear();for(const auto& vertex:sample.vertices)sampled_positions.push_back(vertex.position);
      const auto expected=skin_model_world_nodes(*body,sample.body_nodes);
      for(unsigned i=0;i<ordinals.size();++i){need(length(sample.vertices[i].position-expected[ordinals[i]].position)==0,
          "Falling submitted vertex detached from its actual sampled native WORLD palette");++vertices_checked;}
      joint_positions=Json::array();
      for(const auto& joint:joints){
        need(joint.full<sample.body_nodes.size()&&joint.first_person<completed_fp_nodes.size()&&joint.first_person<interpolated_fp_nodes.size(),
            "Falling native joint mapping escaped its actual model hierarchy");
        const auto full=sample.body_nodes[joint.full].position,fp=interpolated_fp_nodes[joint.first_person].position;
        const auto difference=length(full-fp);
        joint_positions.push_back({{"name",joint.name},{"full_model_node",joint.full},{"first_person_model_node",joint.first_person},
            {"sampled_full_WORLD",xyz(full)},{"completed_168DED",xyz(completed_fp_nodes[joint.first_person].position)},
            {"sampled_168DED",xyz(fp)},{"sampled_pose_difference",difference}});
      }
    };
    options.view_state_observer=[&](unsigned view,const MultiplayerState& shown,const Camera& camera){
      if(view)return;
      death_view_this_frame=false;
      if(!controlled){
        const auto& player=shown.players[slot];
        if(drop_tick&&report["void_render_membership"].size()<256)report["void_render_membership"].push_back({
            {"tick",shown.tick},{"source_unit",player.source_unit},{"dead",player.dead},
            {"native_mode0_admitted",completed_body_this_frame},{"primary_body_present",sample_ready},
            {"camera",xyz(camera.position)},{"ordinary_camera",bool(player.source_player_camera)},
            {"death_camera",bool(player.source_death_camera)}});
        if(completed_body_this_frame&&!player.dead)need(sample_ready,"Native admitted void body lost its primary geometry");
        completed_body_this_frame=false;
        if(player.dead){
          need(!player.source_player_camera,"Void death retained an ordinary living camera");
          death_view_this_frame=bool(player.source_death_camera);
          sample_ready=false;return;
        }
      }
      if(view||!sample_ready)return;sample_ready=false;
      if(!drop_tick||sample_tick<=*drop_tick+2)return;
      const auto after=endpoints.find(sample_tick),before=endpoints.find(sample_tick-1);
      if(after==endpoints.end()||before==endpoints.end()||after->second.grounded||after->second.velocity.z>=-.1f)return;
      need(camera.forward.z<-.8f,"Falling body capture did not look down");++falling_samples;
      if(after->second.velocity.z<-6.f)++void_fast_body_samples;
      const auto stamp=std::chrono::steady_clock::now();if(!first_fall)first_fall=stamp;last_fall=stamp;
      if(fraction>0&&fraction<1)++fractional_samples;
      const auto& a=before->second;const auto& b=after->second;
      const auto pelvis=a.nodes.front().position+(b.nodes.front().position-a.nodes.front().position)*fraction;
      const auto eye=a.eye+(b.eye-a.eye)*fraction;
      const auto root_error=length(sampled_pelvis-pelvis),eye_error=length(camera.position-eye);
      const auto relative_error=length((sampled_pelvis-camera.position)-(pelvis-eye));
      maximum_root_error=std::max(maximum_root_error,root_error);maximum_eye_error=std::max(maximum_eye_error,eye_error);
      maximum_relative_error=std::max(maximum_relative_error,relative_error);
      for(const auto& joint:joint_positions)
        maximum_lower_joint_pose_difference=std::max(maximum_lower_joint_pose_difference,joint["sampled_pose_difference"].get<float>());
      if(last&&last->tick==sample_tick&&fraction>last->fraction+.001f){++same_tick_pairs;
        if(length(sampled_pelvis-last->pelvis)>.00001f&&length(camera.position-last->eye)>.00001f)++moving_same_tick_pairs;}
      last=Last{sample_tick,fraction,sampled_pelvis,camera.position};
      if(controlled&&sample_tick==options.capture_tick){
        const auto point=unsigned(std::lround(fraction*4));
        need(point<5&&fraction==float(point)*.25f,"Controlled native sweep lost its admitted fraction");
        need(world.match.simulation().ticks()==options.capture_tick,"Controlled fractional draws advanced the actual native gameplay tick");
        const auto expected_nodes=interpolate_model_nodes(*body,a.nodes,b.nodes,fraction);
        const auto expected_vertices=skin_model_world_nodes(*body,expected_nodes);
        for(unsigned i=0;i<ordinals.size();++i)
          maximum_sweep_pose_error=std::max(maximum_sweep_pose_error,length(sampled_positions[i]-expected_vertices[ordinals[i]].position));
        std::array<ShaderVector,4> rows;std::copy_n(native_main_constants.begin(),4,rows.begin());
        sweep_frames.insert_or_assign(point,SweepFrame{fraction,camera,sampled_positions,rows});
        if(point==0)report["controlled_native_pair"]={{"previous_tick",sample_tick-1},{"current_tick",sample_tick},
            {"previous_root",xyz(a.root)},{"current_root",xyz(b.root)},{"previous_eye",xyz(a.eye)},{"current_eye",xyz(b.eye)},
            {"previous_velocity",xyz(a.velocity)},{"current_velocity",xyz(b.velocity)},
            {"previous_grounded",a.grounded},{"current_grounded",b.grounded}};
        if(native_main_camera)need(length(native_main_camera->camera_to_world.position-camera.position)==0,
            "Controlled native completed main camera differs from the submitted fractional eye");
      }
      if(report["samples"].size()<48)report["samples"].push_back({{"tick",sample_tick},{"fraction",fraction},{"velocity",xyz(b.velocity)},
          {"native_root",xyz(b.root)},{"submitted_pelvis",xyz(sampled_pelvis)},{"submitted_eye",xyz(camera.position)},
          {"expected_pelvis",xyz(pelvis)},{"expected_eye",xyz(eye)},{"root_error",root_error},{"eye_error",eye_error},
          {"camera_relative_error",relative_error},{"shown_root",xyz(shown.players[slot].position)},
          {"completed_native_full_body_pelvis",xyz(b.nodes.front().position)},
          {"completed_native_fp_body_pelvis",xyz(completed_fp_pelvis)},
          {"interpolated_native_fp_body_pelvis",xyz(interpolated_fp_pelvis)},{"lower_joint_positions",joint_positions}});
    };
    options.frame_observer=[&](const MultiplayerFrameTiming& frame){submitted+=frame.submitted;
      if(frame.submitted){const auto stamp=std::chrono::steady_clock::now();if(!first_submit)first_submit=stamp;last_submit=stamp;
        if(death_view_this_frame)++void_death_views;
        if(!controlled&&void_death_views>=2&&!quit_delivered){
          SDL_Event quit{};quit.type=SDL_QUIT;need(SDL_PushEvent(&quit)==1,"Could not finish bounded void death capture");quit_delivered=true;
        }
      }
      need(std::chrono::steady_clock::now()<deadline,"Falling body GPU frames exceeded finite deadline");};
    if(controlled){
      options.shadow_admission_observer=[&](bool before,unsigned view,std::uint32_t tick,std::uint32_t,std::uint32_t entry,
          const SourcePhysicsWorldRuntimeAbi&,const SourceRuntimeMemory& memory){
        if(before||view||tick!=options.capture_tick||entry!=0x44710u)return;
        SourceViewProjectionBank bank;memory.read(0x4c6634u,std::as_writable_bytes(std::span(&bank,1)));
        native_main_camera=bank;native_main_constants=source_view_shader_constants(bank,1024.f,false);
      };
      options.completed_material_upload_observer=[&](unsigned view,std::uint32_t tick,const RendererCompletedMaterialUpload& upload){
        if(view||tick!=options.capture_tick||!native_main_camera)return;
        need(upload.material_uniform.size()==sizeof(WorldMaterialUniform),"Controlled body sweep lost its actual material uniform ABI");
        WorldMaterialUniform uniform;std::memcpy(&uniform,upload.material_uniform.data(),sizeof(uniform));
        if(!(uniform.source_camera_control[0]&1u))return;
        ++camera_uniform_samples;++sweep_uniform_samples[unsigned(std::lround(fraction*4))];
        camera_uniform_mismatches+=std::memcmp(uniform.source_camera_world.data(),native_main_constants.data(),sizeof(native_main_constants))!=0;
      };
    }
    options.model_depth_vertices_observer=[&](unsigned view,std::uint32_t tick,
        std::span<const SourceModelDepthSelection> selections,std::span<const Vertex> before,std::span<const Vertex> after){
      if(view||!body||tick!=sample_tick)return;
      if(!drop_tick||tick<=*drop_tick+2)return;
      const auto endpoint=endpoints.find(tick);
      if(endpoint==endpoints.end()||endpoint->second.grounded||endpoint->second.velocity.z>=-.1f)return;
      for(const auto& selection:selections){
        if(selection.object!=prepared.source_unit||selection.render_model!=body_model)continue;
        need(selection.source_vertices.size()==ordinals.size(),"Final native receiver did not retain this exact primary lower-body subset");
        need(selection.first+selection.source_vertices.size()<=before.size()&&before.size()==after.size(),"Final body receiver range escaped its submitting bank");
        // Native strips and Cache expansion can cyclically rotate a triangle.
        // Admit that exact oriented ordering, but never a reverse winding or
        // another triangle/object merely because a float position is close.
        float error{};
        for(unsigned first=0;first<selection.source_vertices.size();first+=3){
          float triangle_error=std::numeric_limits<float>::infinity();
          for(unsigned cyclic=0;cyclic<3;++cyclic){float candidate{};
            for(unsigned v=0;v<3;++v)
              candidate=std::max(candidate,length(after[selection.first+first+v].position-before[selection.first+first+(v+cyclic)%3].position));
            triangle_error=std::min(triangle_error,candidate);
          }
          error=std::max(error,triangle_error);
        }
        maximum_submitted_depth_error=std::max(maximum_submitted_depth_error,error);
        ++depth_body_samples;depth_vertices_checked+=selection.source_vertices.size();
      }
    };
    const auto capture=std::filesystem::path(scratch)/"falling-body.ppm";
    options.frame_capture=[&](const MultiplayerState&)->std::filesystem::path {
      if(controlled){
        if(sample_tick!=options.capture_tick)return {};
        captured=true;return std::filesystem::path(scratch)/("falling-sweep-"+std::to_string(unsigned(std::lround(fraction*4)))+".ppm");
      }
      if(captured||falling_samples<8||fractional_samples<4||void_fast_body_samples<5)return {};captured=true;return capture;};
    report["stage"]="actual-native-fractional-fall";const auto result=run_multiplayer(options);
    report.update({{"submitted_frames",submitted},{"falling_samples",falling_samples},{"fractional_falling_samples",fractional_samples},
      {"same_tick_fraction_pairs",same_tick_pairs},{"moving_same_tick_pairs",moving_same_tick_pairs},{"posed_vertices_checked",vertices_checked},
      {"maximum_pelvis_error",maximum_root_error},{"maximum_eye_error",maximum_eye_error},{"maximum_camera_relative_error",maximum_relative_error}});
    report["native_depth_body_samples"]=depth_body_samples;report["native_depth_vertices_checked"]=depth_vertices_checked;
    report["maximum_submitted_depth_position_error"]=maximum_submitted_depth_error;
    report["primary_body_vertices_per_frame"]=ordinals.size();
    report["maximum_lower_joint_full_vs_FP_pose_difference"]=maximum_lower_joint_pose_difference;
    if(!controlled){
      report["void_descending_native_ticks"]=void_descending_ticks;report["void_fast_native_ticks"]=void_fast_ticks;
      report["void_fast_body_render_samples"]=void_fast_body_samples;report["void_minimum_root_z"]=void_minimum_z;
      report["void_minimum_vertical_velocity"]=void_minimum_velocity;report["void_death_rendered_frames"]=void_death_views;
      if(void_death_tick)report["void_native_death_tick"]=*void_death_tick;
      report["bounded_death_quit_delivered"]=quit_delivered;
    }
    report["falling_sample_rows_omitted"]=falling_samples-report["samples"].size();
    if(first_submit&&last_submit){const auto seconds=std::chrono::duration<double>(*last_submit-*first_submit).count();
      report["submitted_span_seconds"]=seconds;report["measured_submitted_fps"]=seconds>0?double(submitted-1)/seconds:0.;}
    if(first_fall&&last_fall){const auto seconds=std::chrono::duration<double>(*last_fall-*first_fall).count();
      report["falling_span_seconds"]=seconds;report["measured_falling_sample_fps"]=seconds>0?double(falling_samples-1)/seconds:0.;}
    report["native_endpoint_ticks_observed"]=endpoints.size();
    report["native_endpoints"]=Json::array();
    for(const auto& [tick,endpoint]:endpoints){
      if(drop_tick&&tick+1<*drop_tick)continue;
      if(report["native_endpoints"].size()==60)break;
      report["native_endpoints"].push_back({{"tick",tick},{"source_unit",prepared.source_unit},
          {"root",xyz(endpoint.root)},{"velocity",xyz(endpoint.velocity)},{"eye",xyz(endpoint.eye)},
          {"full_model_pelvis",xyz(endpoint.nodes.front().position)},{"grounded",endpoint.grounded}});
    }
    report["native_endpoint_rows_omitted"]=endpoints.size()-report["native_endpoints"].size();
    if(controlled){
      report["maximum_controlled_vertex_pose_error"]=maximum_sweep_pose_error;
      report["native_camera_uniform_samples"]=camera_uniform_samples;report["native_camera_uniform_mismatches"]=camera_uniform_mismatches;
      need(result==0&&sweep_frames.size()==5&&maximum_sweep_pose_error<.00002f,
          "Controlled falling sweep did not preserve all five actual native interpolated poses");
      need(maximum_root_error<.00002f&&maximum_eye_error<.00002f&&maximum_relative_error<.00003f,
          "Controlled falling body and eye use different actual native endpoints or render fractions");
      need(maximum_submitted_depth_error<.00003f,
          "Controlled GPU-submitted native receiver body disagrees with its fractional first-person palette");
      need(camera_uniform_samples&&camera_uniform_mismatches==0&&sweep_uniform_samples.size()==5,
          "Controlled falling GPU material cameras differ from the actual native main projection at a fractional point");
      unsigned moving_intervals{},minimum_moving_vertices=unsigned(ordinals.size());float maximum_reprojection_motion{};
      const auto project=[](const SweepFrame& frame,Vec3 point)->std::optional<Vec2>{
        const auto dot4=[&](const ShaderVector& row){return ((point.x*row[0]+point.y*row[1])+point.z*row[2])+row[3];};
        const auto w=dot4(frame.source_camera[3]);if(w<=0)return {};
        const auto x=dot4(frame.source_camera[0])/w,y=dot4(frame.source_camera[1])/w;
        if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>4||std::abs(y)>4)return {};
        return Vec2{(x+1)*320,(1-y)*240};
      };
      for(unsigned point=0;point<5;++point){
        const auto path=std::filesystem::path(scratch)/("falling-sweep-"+std::to_string(point)+".ppm");verify_capture(path,report);
        std::uint64_t fingerprint=14695981039346656037ull;
        for(const auto position:sweep_frames.at(point).vertices)for(const auto component:xyz(position)){
          const auto word=std::bit_cast<std::uint32_t>(component);
          for(unsigned byte=0;byte<4;++byte)fingerprint=(fingerprint^((word>>(byte*8))&255u))*1099511628211ull;
        }
        report["controlled_points"].push_back({{"fraction",sweep_frames.at(point).fraction},{"tick",options.capture_tick},
            {"actual_gameplay_tick",world.match.simulation().ticks()},
            {"vertices",sweep_frames.at(point).vertices.size()},{"camera",xyz(sweep_frames.at(point).camera.position)},
            {"submitted_body_position_fnv1a",fingerprint},
            {"native_source_camera_rows",sweep_frames.at(point).source_camera},
            {"native_camera_uniform_samples",sweep_uniform_samples.at(point)},{"capture_path",path.string()}});
        if(point&&length(sweep_frames.at(point).vertices.front()-sweep_frames.at(point-1).vertices.front())>.00001f&&
            length(sweep_frames.at(point).camera.position-sweep_frames.at(point-1).camera.position)>.00001f)++moving_intervals;
        if(point){unsigned moving{};
          for(unsigned vertex=0;vertex<ordinals.size();++vertex){
            moving+=length(sweep_frames.at(point).vertices[vertex]-sweep_frames.at(point-1).vertices[vertex])>.00001f;
            const auto a=project(sweep_frames.at(point-1),sweep_frames.at(point-1).vertices[vertex]);
            const auto b=project(sweep_frames.at(point),sweep_frames.at(point).vertices[vertex]);
            if(a&&b){const Vec2 delta{a->x-b->x,a->y-b->y};maximum_reprojection_motion=std::max(maximum_reprojection_motion,std::sqrt(delta.x*delta.x+delta.y*delta.y));}
          }
          minimum_moving_vertices=std::min(minimum_moving_vertices,moving);
        }
      }
      need(moving_intervals==4,"Controlled falling body or camera retained a game-tick step inside the actual native pair");
      need(minimum_moving_vertices==ordinals.size(),"Controlled falling geometry contains vertices held at the game tick instead of moving at every fraction");
      report["controlled_moving_intervals"]=moving_intervals;report["controlled_minimum_moving_vertices"]=minimum_moving_vertices;
      report["controlled_maximum_adjacent_projected_motion_pixels"]=maximum_reprojection_motion;
      report["passed"]=true;report["stage"]="complete";save();return 0;
    }
    need(result==0&&falling_samples>=8&&fractional_samples>=4&&same_tick_pairs>=2,
        "Falling body fixture did not submit multiple fractional frames within real descending native ticks");
    need(drop_tick&&report.value("warmup_submitted_frames_before_walk",0u)>=warmup_render_frames,
        "Falling body fixture did not warm actual standing GPU frames before its ordinary void walk");
    need(void_descending_ticks>=60&&void_fast_ticks>=5&&void_minimum_z<-20.f&&void_death_tick&&void_death_views>=2,
        "Warmed Lockout walk did not exercise a long native void fall and its rendered native death camera");
    need(void_fast_body_samples>=5,"Void fall did not retain first-person body samples at high native falling velocity");
    need(maximum_root_error<.00002f&&maximum_eye_error<.00002f&&maximum_relative_error<.00003f,
        "Offline falling body and eye use different native tick endpoints or render fractions");
    need(moving_same_tick_pairs==same_tick_pairs,"Falling body or eye snapped at the game tick instead of tracking every submitted fractional frame");
    need(maximum_submitted_depth_error<.00003f,"Actual GPU-submitted native receiver body disagrees with its fractional first-person palette");
    need(captured&&std::filesystem::is_regular_file(capture),"Falling body fixture did not capture actual fractional GPU output");
    verify_capture(capture,report);
    report["passed"]=true;report["stage"]="complete";save();return 0;
  }catch(const std::exception& error){report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;}
}
