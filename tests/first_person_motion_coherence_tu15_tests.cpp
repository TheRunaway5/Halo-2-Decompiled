#define SDL_MAIN_HANDLED
// Moving high-refresh first-person coherence on the native TU1.5 host path.
// A real SDL movement hold drives the local biped while the renderer runs at
// its natural wall-clock rate, so frames land at arbitrary fractions between
// 30 Hz ticks (no fixed tick cadence or fraction). Every submitted frame compares
// the actual sampled arms, held weapon, lower body and camera against the
// completed 30 Hz banks of the same frame:
//  - all first-person channels sample one fraction;
//  - completed camera-relative banks are produced in the frame's own camera;
//  - the held weapon keeps its completed grip on the sampled hand;
//  - the lower body and the camera eye share one completed-endpoint blend.
// This is a CPU presentation-continuity check; no Xbox framebuffer claim.
#include "halo2/animation_presentation.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <tuple>

using namespace halo2;
namespace {
using Json=nlohmann::json;
Json report{{"passed",false},{"stage","bootstrap"}};
void need(bool value,const std::string &message){if(!value)throw FormatError(message);}
std::array<float,3> xyz(Vec3 v){return {v.x,v.y,v.z};}
Vec3 lerp3(Vec3 a,Vec3 b,float f){return a+(b-a)*f;}
void save(const char *path) {
  const auto bytes=report.dump(2)+"\n";need(bytes.size()<=1024*1024,"Motion coherence report exceeds its bounded receipt");
  std::ofstream out(path);out<<bytes;need(bool(out),"Could not write the motion coherence report");
}
void push(SDL_Event event){need(SDL_PushEvent(&event)==1,"Could not queue actual first-person motion input");}
using Key=std::tuple<std::uint32_t,std::uint32_t,unsigned>; // owner, render model, mode
struct Bank {std::uint32_t tick{};float fraction{};std::vector<Transform> nodes;};
struct Completed {Vec3 eye,root,position;bool eye_known{};std::vector<Transform> nodes;};
struct Frame {
  std::map<Key,Bank> completed,interpolated;
  std::optional<Camera> camera;std::optional<Bank> body;
};
}
int main(int argc,char **argv)try {
  need(argc==5,"Supply prepared MCC map, shared maps, representation2|3 and report");
  if(!std::getenv("DISPLAY") || !std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
  need(scratch && std::filesystem::path(scratch).is_absolute(),"Run the motion coherence check through tools/run_test.py");
  const unsigned representation=unsigned(std::stoul(argv[3]));
  need(representation==2 || representation==3,"Motion coherence check needs human or Elite");
  report["representation"]=representation;
  SDL_SetMainReady();
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_world=true;
  options.width=640;options.height=480;options.frames=900;
  options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"first-person-motion";
  auto profile=source_default_player_configuration();profile[0x44]=representation==3?1:0;options.selected_profiles={profile};
  report["stage"]="construct-native-world";
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
  need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics() && world.match.source_engine_owned(),
      "Motion coherence check lost its canonical native world");
  const auto slot=world.local_slots.at(0);
  const auto weapon=world.cache->slayer_starting_weapon(5);
  const auto fp=world.cache->load_first_person_binding(weapon,representation);
  // Look down far enough for the original168DED lower-body admission while
  // the arms and held weapon remain in view.
  constexpr float look_pitch=-1.15f;
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned tick=0;tick<60;++tick) {
    controls[slot].pitch=std::max(look_pitch,-.05f*float(tick));world.match.step(controls);
  }
  const auto prepared=world.match.state().players[slot];
  need(!prepared.dead && prepared.source_unit!=null_datum && prepared.source_weapon!=null_datum,
      "Motion coherence check did not spawn its armed on-foot unit");
  const auto unit=prepared.source_unit;
  const auto arms_hierarchy=world.cache->load_model_hierarchy(fp.arms_model);
  options.desktop=std::make_shared<DesktopSession>();
  options.desktop->display_settings.graphics_mode=pstencil::GraphicsMode::Original;
  options.desktop->keyboard_settings.sensitivity=3;options.desktop->keyboard_settings.invert=false;
  options.desktop->display_settings.field_of_view=default_field_of_view;
  auto *window=options.desktop->window(options.width,options.height,false);
  const auto window_id=SDL_GetWindowID(window);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  (void)SDL_SetRelativeMouseMode(SDL_TRUE);
  SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;push(focus);
  constexpr float mouse_gain=.0025f;
  SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
  look.motion.xrel=int(std::lround((prepared.yaw-controls[slot].yaw)/mouse_gain));
  look.motion.yrel=-int(std::lround((controls[slot].pitch-prepared.pitch)/mouse_gain));push(look);
  const auto key=[&](PcAction action,bool down) {
    const auto binding=options.desktop->keyboard_settings.bindings[unsigned(action)];
    need(binding>SDL_SCANCODE_UNKNOWN && binding<SDL_NUM_SCANCODES,"Motion fixture requires a keyboard movement binding");
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;event.key.windowID=window_id;
    event.key.state=down?SDL_PRESSED:SDL_RELEASED;event.key.keysym.scancode=SDL_Scancode(binding);
    event.key.keysym.sym=SDL_GetKeyFromScancode(SDL_Scancode(binding));push(event);
  };
  // Alternate forward/back holds so a long run stays inside the map.
  constexpr unsigned hold_ticks=36;
  std::optional<PcAction> held;unsigned steps{},lost_unit_ticks{};
  std::uint32_t weapon_owner=prepared.source_weapon;unsigned own_world_collections{};
  options.screenshot=std::filesystem::path(scratch)/"first-person-motion.ppm";
  std::map<std::uint32_t,Completed> completed_ticks;
  Frame frame;
  struct Summary {
    unsigned frames{},measured{},fractional{},moving{};
    float fraction_mismatch{},completed_camera_local{},grip{},grip_endpoint_change{};
    float body_camera{},body_world{},eye{},speed{},body_completed_root{};
    float body_rigid_residual{},body_shift_vs_shown{};
    unsigned fraction_mismatch_frames{},body_frames{};
  } s;
  Json rows=Json::array();
  std::optional<unsigned> hand_node;
  std::map<std::uint32_t,Transform> first_camera_local; // tick -> weapon root in its first frame's camera
  std::map<std::uint32_t,Vec3> grip_by_tick;
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState &state) {
    const auto &p=state.players[slot];
    if(steps%hold_ticks==0) {
      if(held)key(*held,false);
      held=(steps/hold_ticks)%2?PcAction::Back:PcAction::Forward;key(*held,true);
    }
    ++steps;
    if(p.dead || p.source_unit!=unit){++lost_unit_ticks;return;}
    weapon_owner=p.source_weapon;
    Completed c;c.position=p.position;
    if(p.source_player_camera){c.eye=p.source_player_camera->position;c.eye_known=true;}
    const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
        [&](const auto &entry){return entry.object==unit;});
    need(state.source_body_poses_authoritative && pose!=state.source_body_poses.end() && !pose->nodes.empty(),
        "Moving unit has no completed native WORLD publication");
    c.root=pose->nodes.front().position;c.nodes=pose->nodes;
    completed_ticks.insert_or_assign(state.tick,c);
    while(completed_ticks.size()>8)completed_ticks.erase(completed_ticks.begin());
  };
  options.node_observer=[&](unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &sample) {
    if(view)return;
    if((sample.stage==MultiplayerNodeStage::world_completed || sample.stage==MultiplayerNodeStage::world_interpolated) &&
       (sample.owner==unit || sample.owner==weapon_owner))++own_world_collections;
    const Key k{sample.owner,sample.render_model,sample.mode};
    Bank bank{tick,sample.fraction,{sample.nodes.begin(),sample.nodes.end()}};
    if(sample.stage==MultiplayerNodeStage::first_person_completed)frame.completed.insert_or_assign(k,std::move(bank));
    else if(sample.stage==MultiplayerNodeStage::first_person_interpolated)frame.interpolated.insert_or_assign(k,std::move(bank));
  };
  options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &sample) {
    if(view || player!=slot || sample.first_person || sample.unit!=unit || sample.body_nodes.empty())return;
    frame.body=Bank{sample.tick,sample.fraction,{sample.body_nodes.begin(),sample.body_nodes.end()}};
  };
  options.view_state_observer=[&](unsigned view,const MultiplayerState &,const Camera &camera) {
    if(!view)frame.camera=camera;
  };
  options.frame_observer=[&](const MultiplayerFrameTiming &timing) {
    auto observed=std::move(frame);frame={};
    if(!timing.submitted)return;
    ++s.frames;
    const Key arms_key{unit,fp.arms_model,1},weapon_key{weapon_owner,fp.weapon_model,1};
    const auto arms=observed.interpolated.find(arms_key),gun=observed.interpolated.find(weapon_key);
    const auto arms_done=observed.completed.find(arms_key),gun_done=observed.completed.find(weapon_key);
    if(!observed.camera || arms==observed.interpolated.end() || gun==observed.interpolated.end() ||
       arms_done==observed.completed.end() || gun_done==observed.completed.end())return;
    const auto tick=arms->second.tick;
    const auto now=completed_ticks.find(tick),before=completed_ticks.find(tick-1);
    if(now==completed_ticks.end() || before==completed_ticks.end() || !now->second.eye_known || !before->second.eye_known)return;
    ++s.measured;
    const auto &camera=*observed.camera;
    const auto forward=camera.oriented?camera.forward:direction(camera.yaw,camera.pitch);
    const auto up=camera.oriented?camera.up:cross(forward,{-std::sin(camera.yaw),std::cos(camera.yaw),0});
    const auto inverse=inverse_transform(camera_transform(camera.position,forward,up));
    // The camera fraction: the shown eye between its completed endpoints.
    const auto eye_delta=now->second.eye-before->second.eye;
    const float eye_step=length(eye_delta);
    const float camera_fraction=eye_step>1e-5f?
        std::clamp(dot(camera.position-before->second.eye,eye_delta)/(eye_step*eye_step),0.f,1.f):1.f;
    const float animation_fraction=arms_done->second.fraction;
    s.speed=std::max(s.speed,eye_step);
    const bool fractional=animation_fraction>.05f && animation_fraction<.95f;
    s.fractional+=fractional;
    const bool moving=eye_step>.01f;s.moving+=moving&&fractional;
    // One presentation time for every first-person channel.
    float mismatch=std::abs(arms->second.fraction-gun->second.fraction);
    std::optional<float> body_fp_fraction;
    for(const auto &[k,bank]:observed.interpolated)
      if(std::get<2>(k)==0 && std::get<0>(k)==unit) {
        body_fp_fraction=bank.fraction;mismatch=std::max(mismatch,std::abs(bank.fraction-arms->second.fraction));
      }
    s.fraction_mismatch=std::max(s.fraction_mismatch,mismatch);s.fraction_mismatch_frames+=mismatch>0;
    const float eye_error=length(camera.position-lerp3(before->second.eye,now->second.eye,camera_fraction));
    s.eye=std::max(s.eye,eye_error);
    // Completed camera-relative banks must be produced in this frame's camera.
    const auto local_root=compose_transform(inverse,gun_done->second.nodes.front());
    const auto first=first_camera_local.try_emplace(tick,local_root).first;
    const float camera_local=length(local_root.position-first->second.position);
    s.completed_camera_local=std::max(s.completed_camera_local,camera_local);
    while(first_camera_local.size()>8)first_camera_local.erase(first_camera_local.begin());
    // Grip: the held weapon root expressed in the gripping hand node.
    if(!hand_node) {
      float best=1e9f;
      for(unsigned n=0;n<arms_done->second.nodes.size();++n) {
        const float d=length(arms_done->second.nodes[n].position-gun_done->second.nodes.front().position);
        if(d<best){best=d;hand_node=n;}
      }
      report["grip_hand_node"]=*hand_node;
      report["grip_hand_name"]=world.cache->string_id(arms_hierarchy.bones.at(*hand_node).name);
      report["grip_hand_distance"]=best;
    }
    const auto grip=[&](const std::vector<Transform> &a,const std::vector<Transform> &w) {
      return compose_transform(inverse_transform(a.at(*hand_node)),w.front()).position;
    };
    const auto completed_grip=grip(arms_done->second.nodes,gun_done->second.nodes);
    const auto shown_grip=grip(arms->second.nodes,gun->second.nodes);
    grip_by_tick.insert_or_assign(tick,completed_grip);
    while(grip_by_tick.size()>8)grip_by_tick.erase(grip_by_tick.begin());
    const auto previous_grip=grip_by_tick.find(tick-1);
    const float endpoint_change=previous_grip==grip_by_tick.end()?0.f:length(completed_grip-previous_grip->second);
    const float grip_error=length(shown_grip-completed_grip);
    s.grip=std::max(s.grip,grip_error);s.grip_endpoint_change=std::max(s.grip_endpoint_change,endpoint_change);
    // Lower body root against the camera, both between the same completed endpoints.
    float body_camera{},body_world{},body_completed{};
    if(observed.body && !observed.body->nodes.empty()) {
      ++s.body_frames;
      const auto root=observed.body->nodes.front().position;
      const auto expected_offset=lerp3(before->second.root-before->second.eye,now->second.root-now->second.eye,camera_fraction);
      body_camera=length((root-camera.position)-expected_offset);
      body_world=length(root-lerp3(before->second.root,now->second.root,camera_fraction));
      s.body_camera=std::max(s.body_camera,body_camera);s.body_world=std::max(s.body_world,body_world);
      for(const auto &[k,bank]:observed.completed)if(std::get<2>(k)==0 && std::get<0>(k)==unit) {
        if(!report.contains("first_person_body_skinned_nodes")) {
          const auto model=world.cache->load_model(std::get<1>(k),false);
          std::set<std::string> names;
          for(const auto &skin:model.skin)for(unsigned w=0;w<4;++w)
            if(skin.weights[w]>0 && skin.bones[w]<model.bones.size())names.insert(world.cache->string_id(model.bones[skin.bones[w]].name));
          report["first_person_body_skinned_nodes"]=names;
        }
        body_completed=length(bank.nodes.front().position-now->second.root);
        s.body_completed_root=std::max(s.body_completed_root,body_completed);
        // A rigid translation of the completed bank identifies a presented
        // root relocation before the original FP collector copied it.
        if(bank.nodes.size()==now->second.nodes.size()) {
          const auto shift=bank.nodes.front().position-now->second.nodes.front().position;
          for(unsigned n=0;n<bank.nodes.size();++n)
            s.body_rigid_residual=std::max(s.body_rigid_residual,
                length(bank.nodes[n].position-now->second.nodes[n].position-shift));
          const auto shown_shift=lerp3(before->second.root,now->second.root,camera_fraction)-now->second.root;
          s.body_shift_vs_shown=std::max(s.body_shift_vs_shown,length(shift-shown_shift));
        }
      }
    }
    if(rows.size()<400 && (moving || rows.size()<40))rows.push_back({{"frame",timing.frame},{"tick",tick},
        {"animation_fraction",animation_fraction},{"camera_fraction",camera_fraction},
        {"arms_fraction",arms->second.fraction},{"weapon_fraction",gun->second.fraction},
        {"body_fp_fraction",body_fp_fraction?Json(*body_fp_fraction):Json(nullptr)},
        {"body_draw_fraction",observed.body?Json(observed.body->fraction):Json(nullptr)},
        {"eye_step",eye_step},{"eye_error",eye_error},{"completed_camera_local",camera_local},
        {"grip_error",grip_error},{"grip_endpoint_change",endpoint_change},
        {"body_camera_error",body_camera},{"body_world_error",body_world},{"body_completed_root_offset",body_completed},
        {"camera",xyz(camera.position)}});
  };
  report["stage"]="render-moving-first-person";
  const auto result=run_multiplayer(options);
  if(held)key(*held,false);
  report["frames"]=s.frames;report["measured_frames"]=s.measured;report["fractional_frames"]=s.fractional;
  report["moving_fractional_frames"]=s.moving;report["body_frames"]=s.body_frames;
  report["max_eye_step"]=s.speed;
  report["max_fraction_mismatch"]=s.fraction_mismatch;report["fraction_mismatch_frames"]=s.fraction_mismatch_frames;
  report["max_eye_error"]=s.eye;report["max_completed_camera_local_drift"]=s.completed_camera_local;
  report["max_grip_error"]=s.grip;report["max_grip_endpoint_change"]=s.grip_endpoint_change;
  report["max_body_camera_error"]=s.body_camera;report["max_body_world_error"]=s.body_world;
  report["max_body_completed_root_offset"]=s.body_completed_root;
  report["max_body_completed_rigid_residual"]=s.body_rigid_residual;
  report["max_body_completed_shift_minus_shown_shift"]=s.body_shift_vs_shown;
  report["rows"]=rows;report["run_result"]=result;report["lost_unit_ticks"]=lost_unit_ticks;
  report["own_unit_or_held_weapon_world_collections_in_first_person_view"]=own_world_collections;
  need(result==0,"Native motion render returned a failure");
  need(s.moving>=20 && s.body_frames>=20,"Motion run did not exercise moving fractional first-person frames");
  report["stage"]="compare";
  need(s.fraction_mismatch==0,"First-person channels sampled different presentation fractions");
  need(s.completed_camera_local<=1e-4f,"Completed first-person bank was produced in a different camera than its frame");
  need(s.grip<=s.grip_endpoint_change+1e-3f,"Held weapon left its sampled hand grip during motion");
  need(s.eye<=1e-3f,"Camera eye left its completed endpoint blend");
  need(s.body_camera<=1e-3f,"First-person body jittered against the camera during motion");
  report["passed"]=true;save(argv[4]);
  std::cout<<"PASS moving first-person arms, weapon, body and camera share one presentation time (representation "
      <<representation<<")\n";
  return 0;
}catch(const std::exception &error) {
  report["error"]=error.what();
  if(argc==5)try{save(argv[4]);}catch(...){}
  std::cerr<<error.what()<<'\n';return 1;
}
