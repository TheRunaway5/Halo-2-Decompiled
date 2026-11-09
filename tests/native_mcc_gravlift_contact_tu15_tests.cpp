#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_tu15_physics_frame_residuals.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <utility>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why) {if(!ok)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files() {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    check(scratch,"Gravlift contact regression requires tools/run_test.py scratch ownership");
    path=std::filesystem::path(scratch)/"gravlift-world";
    check(std::filesystem::create_directory(path),"Cannot create owned gravlift world directory");
  }
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
Spawn field_entrance(const Cache &cache,const LevelContent &content,std::string_view name,
    int selected_phantom=-1) {
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag &t) {
    return (t.group=="bloc" || t.group=="mach") && t.name==name;
  });
  check(tag!=cache.tags().end(),"Authored gravlift definition absent");
  const auto placement=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &o) {
    return o.definition==tag->datum;
  });
  check(placement!=content.objects.end(),"Authored gravlift placement absent");
  const auto object=cache.load_source_tag_metadata(placement->definition);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
  const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
  const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
  check(p.u32(0x108)>0,"Authored gravlift has no phantom geometry");
  const auto basis=authored_basis(placement->rotation);
  const auto root=source_object_transform(placement->position,basis.forward,basis.up,placement->scale,false);
  Spawn spawn{};spawn.facing=placement->rotation.z;
  bool selected=false;
  for(unsigned n=0;n<p.u32(0x108);++n) {
    if(selected_phantom>=0 && n!=unsigned(selected_phantom))continue;
    const Bytes phantom(physics.bytes_at(p.u32(0x10c)+n*0x20u,0x20));
    const auto type=phantom.u16(0xc),index=phantom.u16(0xe);
    Vec3 center;
    if(type==4) {
      check(index<p.u32(0x70),"Gravlift phantom selects an absent authored hull");
      const Bytes hull(physics.bytes_at(p.u32(0x74)+index*0x100u,0x100));
      Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
      center=compose_transform(root,local).position;
    } else if(type==1) {
      check(index<p.u32(0x58),"Gravlift phantom selects an absent authored capsule");
      const Bytes capsule(physics.bytes_at(p.u32(0x5c)+index*0x50u,0x50));
      Transform first,second;
      first.position={capsule.f32(0x30),capsule.f32(0x34),capsule.f32(0x38)};
      second.position={capsule.f32(0x40),capsule.f32(0x44),capsule.f32(0x48)};
      const auto a=compose_transform(root,first).position,b=compose_transform(root,second).position;
      center=a.z<=b.z?a:b;
    } else if(type==2) {
      check(index<p.u32(0x60),"Gravlift phantom selects an absent authored box");
      const Bytes box(physics.bytes_at(p.u32(0x64)+index*0x90u,0x90));
      Transform local;local.position={box.f32(0x80),box.f32(0x84),box.f32(0x88)};
      for(unsigned axis=0;axis<3;++axis)local.basis[axis]={
          box.f32(0x50+axis*0x10u),box.f32(0x54+axis*0x10u),box.f32(0x58+axis*0x10u)};
      center=compose_transform(root,local).position;
    } else throw FormatError("Gravlift contact regression needs the authored phantom geometry owner");
    if(!selected || center.z<spawn.position.z) {spawn.position=center;selected=true;}
  }
  check(selected,"Gravlift contact regression found no authored intake");
  return spawn;
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=3 && argc!=4)return 2;
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const std::string_view which=argv[2];
  check(which=="lockout" || which=="midship" || which=="coagulation",
      "Select lockout, midship or coagulation authored gravlift");
  const std::string_view name=which=="lockout"?
      "scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube":
      which=="midship"?
      "scenarios\\objects\\multi\\midship\\cov_power_lift\\cov_power_lift":
      "scenarios\\objects\\multi\\halo\\coagulation\\powerhaus\\powerhaus";
  Files files;MultiplayerOptions options;
  options.map=argv[1];options.shared_maps=options.map.parent_path();
  options.local_players=argc==4?unsigned(std::stoul(argv[3])):1u;
  check(options.local_players==1 || options.local_players==2,"Select one or two actual local players");
  options.starting_weapon=5;options.mute=true;
  options.source_object_savegame_directory=files.path;
  MultiplayerPreparedWorld prepared(options);
  check(prepared.cache->is_mcc() && prepared.source_objects && prepared.source_objects->owns_physics(),
      "Gravlift contact regression requires the actual MCC native World");
  check(prepared.rules.requires_source_engine,"Gravlift contact regression requires canonical source engine admission");
  const auto &cache=*prepared.cache;const auto &content=prepared.content;
  check(content.player.height>0 && std::isfinite(content.player.height),"Gravlift regression needs the authored player height");
  // Coagulation's horizontal exit capsule reaches actual2F1806->2FAB70.
  // Its vertical intake is a different authored capsule, and selecting the
  // lowest endpoint does not reproduce this missing-query failure.
  const auto start=field_entrance(cache,content,name,which=="coagulation"?0:-1);
  std::cout<<"Declared authored gravlift intake="<<start.position.x<<','<<start.position.y<<','<<start.position.z<<'\n';
  auto &match=prepared.match;auto &world=*prepared.source_objects;
  for(unsigned tick=0;tick<2;++tick)match.step({});
  const auto slot=prepared.local_slots.front();
  const auto object=match.state().players[slot].source_unit;
  check(object!=0xffffffffu,"Gravlift entrant has no actual native source biped");
  const auto activity_before=world.tu15_world().native_physics_frame_activity();
  // Preserve canonical map spawns and source-engine admission. This one
  // declared location input uses the real B95D0 setter before contact; native
  // contact, membership, force and integration produce every later pose.
  world.tu15_world().set_object_position(object,start.position);
  const auto before=world.biped_motion(object);
  check(length(before.position-start.position)<.1f,"Declared gravlift location input was not admitted");
  std::array<Controls,multiplayer_capacity> controls{};
  controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;
  float peak=before.position.z,fastest{};
  for(unsigned tick=0;tick<90;++tick) {
    std::cout<<"Native gravlift contact tick "<<tick<<'\n';
    match.step(controls);
    const auto body=world.biped_motion(object);
    check(std::isfinite(length(body.velocity)),"Gravlift produced nonfinite native velocity");
    peak=std::max(peak,body.position.z);fastest=std::max(fastest,body.velocity.z);
    check(length(match.unit(slot).body.position-body.position)<.001f,"Gravlift match readback lost native body pose");
  }
  const auto activity=world.tu15_world().native_physics_frame_activity();
  const auto scalar_count=[](const auto &a) {
    return std::accumulate(a.native_lift_scalar_calls.begin(),a.native_lift_scalar_calls.end(),std::uint64_t{});
  };
  const auto calls=scalar_count(activity)-scalar_count(activity_before);
  const auto velocity_calls=activity.native_lift_velocity_completed[0]-
      activity_before.native_lift_velocity_completed[0];
  const auto factories=activity.native_wrapped_factory_calls-activity_before.native_wrapped_factory_calls;
  std::cout<<nlohmann::json{{"map",cache.name()},{"source_unit",object},
      {"local_players",options.local_players},{"neutral_startup_ticks",2},
      {"initial_z",before.position.z},{"peak_z",peak},{"maximum_upward_velocity",fastest},
      {"native_lift_scalar_calls",calls},{"native_wrapped_factory_calls",factories},
      {"native_lift_linear_velocity_calls",velocity_calls},
      {"native_lift_velocity_attempts",activity.native_lift_velocity_attempts},
      {"native_lift_velocity_completed",activity.native_lift_velocity_completed},
      {"native_lift_velocity_refusals",activity.native_lift_velocity_refusals},
      {"native_wrapped_factory_refusals",activity.native_wrapped_factory_refusals},
      {"native_lift_observation_refusals",activity.native_lift_observation_refusals},
      {"ticks",match.state().tick}}.dump()<<'\n';
  check(calls>0 && activity.native_lift_observation_refusals==0,"Native gravlift scalar force was not exercised");
  check(velocity_calls>0 && activity.native_lift_velocity_refusals==0 &&
      activity.native_lift_velocity_attempts==activity.native_lift_velocity_completed,
      "Gravlift force did not complete its native body velocity handoff");
  if(which=="lockout")check(factories>0 && activity.native_wrapped_factory_refusals==0,
      "Native Lockout gravlift contact did not exercise its wrapped contact-agent factory");
  if(which!="coagulation")check(peak-before.position.z>=content.player.height && fastest>0,
      "Native gravlift failed to carry its entrant one authored player height");
  if(which=="coagulation") {
    // A normal jump enters the vertical intake. Compare against the same
    // ordinary jump at the canonical spawn; the engine owns both trajectories.
    // No velocity, force, gravity, membership or activation result is supplied.
    const auto flight=[&](bool enter_lift) {
      // Each trajectory begins with a fresh canonical source player. The
      // earlier exit-contact probe may legitimately kill/retire its entrant.
      auto comparison_options=options;
      comparison_options.source_object_savegame_directory=files.path/
          (enter_lift?"intake-jump":"canonical-jump");
      MultiplayerPreparedWorld comparison(comparison_options);
      auto &comparison_match=comparison.match;auto &comparison_world=*comparison.source_objects;
      for(unsigned tick=0;tick<2;++tick)comparison_match.step({});
      const auto comparison_slot=comparison.local_slots.front();
      const auto entrant=comparison_match.player(comparison_slot).source_current_object;
      check(entrant!=0xffffffffu,"Gravlift jump comparison has no canonical source entrant");
      if(enter_lift)comparison_world.tu15_world().set_object_position(entrant,
          field_entrance(*comparison.cache,comparison.content,name).position);
      const auto initial=comparison_world.biped_motion(entrant).position.z;
      const auto count_before=scalar_count(comparison_world.tu15_world().native_physics_frame_activity());
      std::array<Controls,multiplayer_capacity> jump_controls{};
      jump_controls[comparison_slot].yaw=comparison_match.unit(comparison_slot).yaw;
      jump_controls[comparison_slot].pitch=comparison_match.unit(comparison_slot).pitch;
      float highest=initial;
      for(unsigned tick=0;tick<90;++tick) {
        jump_controls[comparison_slot].jump=tick==20 || tick==21;
        comparison_match.step(jump_controls);
        check(comparison_match.player(comparison_slot).source_current_object==entrant,
            "Gravlift jump control lost its actual native entrant generation");
        const auto body=comparison_world.biped_motion(entrant);
        check(std::isfinite(length(body.velocity)) &&
            length(comparison_match.unit(comparison_slot).body.position-body.position)<.001f,
            "Gravlift jump control lost finite native motion/readback");
        highest=std::max(highest,body.position.z);
      }
      const auto count_after=scalar_count(comparison_world.tu15_world().native_physics_frame_activity());
      if(enter_lift) {
        const auto a=comparison_world.tu15_world().native_physics_frame_activity();
        check(a.native_lift_velocity_completed[0]>0 && !a.native_lift_velocity_refusals &&
            a.native_lift_velocity_attempts==a.native_lift_velocity_completed,
            "Coagulation jump did not complete its native body velocity handoff");
      }
      return std::pair{highest-initial,count_after-count_before};
    };
    const auto control=flight(false);
    const auto lifted=flight(true);
    std::cout<<nlohmann::json{{"canonical_jump_rise",control.first},
        {"authored_intake_jump_rise",lifted.first},{"intake_scalar_calls",lifted.second}}.dump()<<'\n';
    check(lifted.second>0 && lifted.first>=control.first+content.player.height,
        "Coagulation native intake did not carry a normal jump beyond its canonical control");
  }
  std::cout<<"PASS actual MCC gravlift contact, native force, completed integration and match readback\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
