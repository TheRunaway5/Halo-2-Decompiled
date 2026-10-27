#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/kernels.hpp"
#include "halo2/model.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
nlohmann::json report{{"passed",false},{"stage","bootstrap"}};
void need(bool value,const char *message){if(!value)throw FormatError(message);}
Vec3 vector(const SourceObserverState &state,unsigned offset) {
  const auto scalar=[&](unsigned at){return std::bit_cast<float>(state.at(at/4));};
  return {scalar(offset),scalar(offset+4),scalar(offset+8)};
}
std::array<float,3> xyz(Vec3 v){return {v.x,v.y,v.z};}
bool exact(Vec3 a,Vec3 b) {
  return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
      std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
      std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
}
void save(const char *path) {
  const auto bytes=report.dump(2)+"\n";
  need(bytes.size()<=1024*1024,"First-person body receipt exceeds its bounded report");
  std::ofstream out(path);out<<bytes;need(bool(out),"Could not write first-person body report");
}
}
int main(int argc,char **argv)try {
  need(argc==6,"Supply prepared MCC map, shared maps, representation2|3, standing|crouching, report");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
  need(scratch && std::filesystem::path(scratch).is_absolute(),"Run first-person body checks through tools/run_test.py");
  const unsigned representation=unsigned(std::stoul(argv[3]));
  const std::string stance=argv[4];
  need(representation==2 || representation==3,"First-person body check needs human or Elite");
  need(stance=="standing" || stance=="crouching","Unknown first-person body stance");
  report["representation"]=representation;report["stance"]=stance;
  report["scope"]="Grounded native MCC player, original TU1.5 scheduler/observer and original168DED WORLD body; no replacement body pose or camera offset";
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.starting_weapon=5;options.mute=true;options.source_object_world=true;
  options.variant={"Slayer",0,{{2,0},{10,0},{89,5}}};
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"first-person-body";
  auto profile=source_default_player_configuration();profile[0x44]=representation==3?1:0;
  options.selected_profiles={profile};
  report["stage"]="construct-native-world";
  MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
  need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics() && world.match.source_engine_owned(),
      "First-person body check lost its canonical native TU1.5 world");
  auto &objects=*world.source_objects;const auto slot=world.local_slots.at(0);
  const auto &data=*world.cache->original_data();
  need(data.image==OriginalData::Image::TitleUpdate15 && data.observer_constants &&
      data.observer_world_constants && data.death_camera_constants,
      "First-person body check lacks authenticated TU1.5 camera owners");
  NativeVehicleCamera camera(objects,data);
  std::array<Controls,multiplayer_capacity> controls{};
  controls[slot].crouch=stance=="crouching";
  SourceObserverState source_observer{};DeathCameraPacket source_packet{};bool captured{};
  unsigned source_calls{},ordinary_results{},grounded_samples{},body_samples{},nodes_checked{},vertices_checked{};
  float maximum_eye_correction{};
  std::optional<NativeDeathCameraView> final_view;Vec3 expected_position{},expected_forward{},expected_up{};
  nlohmann::json samples=nlohmann::json::array();
  SourcePhysicsEntryObserverBinding observe(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &) {
    if(entering || entry!=0x157fe0u)return;
    // Observe the unchanged original scheduler's complete output before the
    // native adapter decides whether its ordinary on-foot result is visible.
    objects.memory().read(0x4f64a4u,std::as_writable_bytes(std::span(source_observer)));
    objects.memory().read(0x4f5514u+0x5cu,std::as_writable_bytes(std::span(source_packet)));
    captured=true;++source_calls;
  });
  report["stage"]="settle-native-grounded-view";
  for(unsigned tick=0;tick<105;++tick) {
    // Establish the actual retained raw-look baseline, then deliver a steep
    // down-look delta through the ordinary source player command owner.
    controls[slot].pitch=-std::min(1.25f,.05f*float(tick));
    world.match.step(controls);const auto state=world.match.state();const auto &p=state.players[slot];
    need(!p.dead && p.source_unit!=null_datum,"Grounded body scenario lost its living unit");
    const auto frame=objects.observer_world_frame(0,p.source_unit,p.source_flags,DeathCameraSavedClusters{});
    captured=false;
    const auto vehicle=camera.update_player(0,p.source_player,p.source_flags,tick_seconds,false,&final_view);
    need(!vehicle,"Ordinary source eye was incorrectly classified as a vehicle camera");
    need(captured,"First-person body check did not execute actual original157FE0");
    // The packet is original TU1.5 output, with the original six-channel
    // adoption/motion/world owners already separately checked against Xbox.
    // This reference stays available even when the native adapter drops it.
    const auto &c=*data.observer_constants;
    source_observer_register_adopt(source_observer,source_packet,c);
    source_observer_motion(source_observer,source_scope_camera_dt(tick_seconds,c.scalar),c);
    source_observer_world(source_observer,frame,objects.observer_world_services(),c,
        *data.death_camera_constants,*data.observer_world_constants);
    expected_position=vector(source_observer,0xb8);expected_forward=vector(source_observer,0xd8);
    expected_up=vector(source_observer,0xe4);
    report["latest_native_sample"]={{"tick",state.tick},{"raw_pitch",controls[slot].pitch},
        {"published_pitch",p.pitch},{"original_observer_forward",xyz(expected_forward)},
        {"original_observer_position",xyz(expected_position)},{"grounded",p.native_grounded},
        {"crouching",p.native_crouching},{"native_view_present",bool(final_view)}};
    report["input_samples"].push_back(report["latest_native_sample"]);
    if(final_view) {
      ++ordinary_results;
      need(final_view->camera_mode==0,"Grounded on-foot player selected a non-first-person camera");
      need(exact(final_view->position,expected_position) && exact(final_view->forward,expected_forward) &&
          exact(final_view->up,expected_up),"Native on-foot view differs from actual original observer output");
    }
    if(tick<90)continue;
    need(p.native_grounded && p.source_parent==null_datum,"Detached-leg check is not grounded on foot");
    // Input pitch is the original retained command angle; verify down from
    // its completed observer basis, rather than assuming a host Euler sign.
    need(expected_forward.z<-.7f,"Detached-leg check did not reach its steep native down-look");
    need(p.native_crouching==(stance=="crouching"),"Detached-leg check did not reach its requested native stance");
    ++grounded_samples;
    objects.begin_model_frame();std::vector<SourceFirstPersonModelTu15> models;
    objects.render_model_view([&](auto &source){
      models=source.prepare_first_person_models(0,expected_position,expected_forward,expected_up);
    });
    const auto body=std::find_if(models.begin(),models.end(),[&](const auto &model){
      return model.mode==0 && model.owner==p.source_unit;
    });
    need(body!=models.end(),"Steep down-look did not emit original168DED looking-down body");
    const auto unit_nodes=objects.world_nodes(p.source_unit);
    need(body->nodes.size()==unit_nodes.size() && !unit_nodes.empty(),
        "Looking-down body lost its authored unit skeleton identity");
    need(!std::memcmp(body->nodes.data(),unit_nodes.data(),unit_nodes.size()*sizeof(Transform)),
        "Looking-down body detached from the actual unit WORLD node bank");
    const auto model=world.cache->load_model(body->render_model,false);
    const auto vertices=skin_model_world_nodes(model,body->nodes);
    need(!vertices.empty(),"Looking-down body has no actual skinned authored geometry");
    for(const auto &v:vertices)for(float x:{v.position.x,v.position.y,v.position.z})
      need(std::isfinite(x),"Looking-down body has nonfinite WORLD geometry");
    ++body_samples;nodes_checked+=unsigned(unit_nodes.size());vertices_checked+=unsigned(vertices.size());
    const auto &biped=world.content.player_representations.at(representation).biped;
    const auto fallback=ordinary_unit_eye(p.position,biped.camera_height,biped.crouch_camera_height,p.crouch);
    const auto correction=expected_position-fallback;
    maximum_eye_correction=std::max(maximum_eye_correction,length(correction));
    samples.push_back({{"tick",state.tick},{"grounded",p.native_grounded},{"crouching",p.native_crouching},
        {"native_pitch",p.pitch},{"native_view_present",bool(final_view)},
        {"original_observer_position",xyz(expected_position)},
        {"original_observer_forward",xyz(expected_forward)},
        {"fallback_ordinary_eye",xyz(fallback)},{"original_eye_correction",xyz(correction)},
        {"eye_correction_distance",length(correction)},
        {"source_unit_position",xyz(p.position)},
        {"body_render_model",body->render_model},{"unit_nodes",unit_nodes.size()},
        {"body_node0",xyz(body->nodes.front().position)},{"body_vertices",vertices.size()}});
  }
  report["stage"]="compare-native-on-foot-adoption";
  report["source_scheduler_calls"]=source_calls;report["native_ordinary_views"]=ordinary_results;
  report["grounded_samples"]=grounded_samples;report["body_samples"]=body_samples;
  report["exact_unit_world_nodes_checked"]=nodes_checked;report["skinned_vertices_checked"]=vertices_checked;
  report["maximum_original_eye_correction"]=maximum_eye_correction;
  report["nonzero_original_eye_correction"]=maximum_eye_correction>1.e-5f;
  report["samples"]=std::move(samples);
  need(grounded_samples>=10 && body_samples==grounded_samples,"Grounded detached-leg cohort is incomplete");
  // This tolerance only rejects a no-op fixture; it is not a replacement
  // correction. The returned camera itself is compared bit-for-bit above.
  need(maximum_eye_correction>1.e-5f,"Grounded down-look fixture did not exercise a nonzero original eye correction");
  need(ordinary_results==source_calls,
      "Native camera discarded the original on-foot observer, leaving the looking-down body under a substitute eye");
  report["passed"]=true;report["stage"]="complete";save(argv[5]);
  std::cout<<"PASS grounded original on-foot camera/body representation="<<representation<<" stance="<<stance
      <<" original_views="<<ordinary_results<<" body_samples="<<body_samples<<" nodes="<<nodes_checked<<'\n';
  return 0;
}catch(const std::exception &error) {
  report["error"]=error.what();try{if(argc==6)save(argv[5]);}catch(const std::exception &e){std::cerr<<e.what()<<'\n';}
  std::cerr<<"First-person body stage "<<report["stage"]<<": "<<error.what()<<'\n';return 1;
}
