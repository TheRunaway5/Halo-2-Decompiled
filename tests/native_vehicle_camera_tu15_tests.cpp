#include "halo2/multiplayer_world.hpp"
#include "halo2/native_vehicle_camera.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/kernels.hpp"
#include "halo2/gameplay_fov.hpp"
#include "halo2/math.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Entrance {Spawn spawn;unsigned seat{};std::size_t placement{};};
Entrance entrance(const Cache &cache,const LevelContent &content,std::string_view name,bool child,unsigned requested_seat,unsigned classification) {
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,placement]:assets.netgame_placements) {
    if(classification && content.netgame_equipment.at(row).classification!=classification)continue;
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name.find("\\"+std::string(name)+"\\")==std::string::npos)continue;
    const auto basis=authored_basis(object.rotation);
    auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    auto target=placement;
    if(child) {
      const auto found=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p){return p.model_attachment && p.model_attachment->parent==placement;});
      if(found==content.objects.end())continue;
      target=std::size_t(found-content.objects.begin());const auto &attachment=*found->model_attachment;
      const auto &parent=assets.units.at(object.definition),&definition=assets.units.at(found->definition);
      const auto nodes=pose_model_nodes_world(parent.hierarchy,root);
      const auto markers=pose_model_markers(parent.markers.at(attachment.parent_marker),nodes,parent.permutations,{},1);
      check(!markers.empty(),"Missing authored turret attachment marker");Transform local;
      if(attachment.child_marker) {
        const auto child_nodes=pose_model_nodes_world(definition.hierarchy,Transform{});
        const auto child_markers=pose_model_markers(definition.markers.at(attachment.child_marker),child_nodes,definition.permutations,{},1);
        if(!child_markers.empty())local=child_markers.front().world;
      }
      root=compose_transform(markers.front().world,inverse_transform(local));
    }
    const auto &definition=assets.units.at(content.objects.at(target).definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];if(index!=requested_seat)continue;
      auto marker=definition.markers.find(seat.entry_marker);
      if(marker==definition.markers.end() && (seat.flags&0x800u))marker=definition.markers.find(seat.marker);
      if(marker==definition.markers.end())continue;
      const auto entrances=pose_model_markers(marker->second,nodes,definition.permutations,{},1);if(entrances.empty())continue;
      Spawn spawn;spawn.position=entrances.front().world.position+entrances.front().world.basis[0]*(seat.radius*.5f);
      CollisionWorld terrain(content.collision);const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;spawn.position.z=floor->position.z+content.player.radius;
      const auto look=entrances.front().world.position-(spawn.position+Vec3{0,0,content.player.camera_height});
      spawn.facing=std::atan2(look.y,look.x);return {spawn,index,target};
    }
  }
  throw FormatError("Missing authored vehicle HUD approach: "+std::string(name));
}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==4 || argc==5,"Supply Coagulation MAP SHARED TU1.5_XBE [CAPTURE_DIRECTORY]");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  struct Case {const char *label,*name;bool child;unsigned seat,parameter,choice,classification;const char *map{};bool boarding{};};
  const std::array cases{
    Case{"warthog-driver","warthog",false,0,76,1,1},
    Case{"ghost-driver","ghost",false,0,76,3,1},
    Case{"banshee-driver","banshee",false,0,76,1,0},
    Case{"scorpion-driver","scorpion",false,0,78,1,3},
    Case{"wraith-driver","wraith",false,0,78,2,3},
    Case{"spectre-driver","spectre",false,0,76,5,1},
    Case{"spectre-secondary-driver","spectre",false,0,77,5,2},
    Case{"gauss-driver","warthog",false,0,76,2,1},
    Case{"warthog-passenger","warthog",false,1,76,1,1},
    Case{"spectre-passenger1","spectre",false,1,76,5,1},
    Case{"spectre-passenger2","spectre",false,2,76,5,1},
    Case{"warthog-turret","warthog",true,0,76,1,1},
    Case{"spectre-turret","spectre",true,0,76,5,1},
    Case{"gauss-turret","warthog",true,0,76,2,1},
    Case{"human-fixed-primary","h_turret_ap",false,0,81,1,6},
    Case{"human-fixed-secondary","h_turret_ap",false,0,82,1,7,"headlong.map"},
    Case{"plasma-fixed-primary","c_turret_ap",false,0,81,4,6},
    Case{"plasma-fixed-secondary","c_turret_ap",false,0,82,4,7,"headlong.map"},
    Case{"ghost-boarding1","ghost",false,1,76,3,1,nullptr,true},
    Case{"ghost-boarding2","ghost",false,2,76,3,1,nullptr,true},
    Case{"ghost-boarding3","ghost",false,3,76,3,1,nullptr,true},
    Case{"ghost-boarding4","ghost",false,4,76,3,1,nullptr,true},
    Case{"warthog-boarding2","warthog",false,2,76,1,1,nullptr,true},
    Case{"warthog-boarding3","warthog",false,3,76,1,1,nullptr,true},
    Case{"warthog-boarding4","warthog",false,4,76,1,1,nullptr,true},
    Case{"warthog-boarding5","warthog",false,5,76,1,1,nullptr,true},
    Case{"scorpion-boarding5","scorpion",false,5,78,1,3,nullptr,true},
    Case{"scorpion-boarding6","scorpion",false,6,78,1,3,nullptr,true},
    Case{"scorpion-boarding7","scorpion",false,7,78,1,3,nullptr,true},
    Case{"wraith-boarding1","wraith",false,1,78,2,3,nullptr,true},
    Case{"wraith-boarding2","wraith",false,2,78,2,3,nullptr,true},
    Case{"spectre-boarding3","spectre",false,3,76,5,1,nullptr,true},
    Case{"spectre-boarding4","spectre",false,4,76,5,1,nullptr,true},
    Case{"spectre-boarding5","spectre",false,5,76,5,1,nullptr,true},
    Case{"banshee-boarding1","banshee",false,1,76,1,0,nullptr,true},
    Case{"banshee-boarding2","banshee",false,2,76,1,0,nullptr,true},
    Case{"banshee-boarding3","banshee",false,3,76,1,0,nullptr,true},
    Case{"banshee-boarding4","banshee",false,4,76,1,0,nullptr,true},
    Case{"warthog-turret-boarding1","warthog",true,1,76,1,1,nullptr,true},
    Case{"gauss-turret-boarding1","warthog",true,1,76,2,1,nullptr,true},
    Case{"spectre-turret-boarding1","spectre",true,1,76,5,1,nullptr,true},
    Case{"wraith-mortar-boarding0","wraith",true,0,78,2,3,nullptr,true},
    Case{"wraith-mortar-boarding1","wraith",true,1,78,2,3,nullptr,true}};
  nlohmann::json report={{"reference","OG Xbox TU1.5"},{"cases",nlohmann::json::array()},
    {"full_camera_parity",false},{"physical_xbox_framebuffer_comparison",false}};
  for(const auto &selected:cases) {
    if(selected.boarding!=bool(std::getenv("HALO2_TU15_TEST_BOARDING")))continue;
    if(const auto filter=std::getenv("HALO2_VEHICLE_CAMERA_CASE");filter && std::string_view(filter)!=selected.label)continue;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    if(selected.map)options.map=options.map.parent_path()/selected.map;
    options.local_players=2;options.starting_weapon=5;options.source_object_world=true;
    if(std::getenv("HALO2_TU15_TEST_ELITE")) {
      auto profile=source_default_player_configuration();profile[0x44]=3;
      options.selected_profiles={profile,profile};
    }
    options.variant.values.set(selected.parameter,selected.choice);options.session_random_seed=0x13c40au;
    Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto content=cache.load_content(0,true,source_game_variant(options.variant));
    const auto approach=entrance(cache,content,selected.name,selected.child,selected.seat,selected.classification);
    const auto &requested=content.unit_interactions->units.at(content.objects.at(approach.placement).definition).seats.at(selected.seat);
    const auto occupied_seat=selected.boarding && requested.boarding_seat>=0?unsigned(requested.boarding_seat):0u;
    const auto occupied_child=selected.boarding && selected.child && requested.boarding_seat>=0;
    const auto driver_approach=selected.boarding?entrance(cache,content,selected.name,occupied_child,occupied_seat,selected.classification):approach;
    options.native_spawn_inputs={approach.spawn,selected.boarding?driver_approach.spawn:cache.spawns().back()};
    MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
    auto &objects=*world.source_objects;const auto &original=*world.cache->original_data();
    NativeVehicleCamera camera(objects,original);
    std::array<Controls,multiplayer_capacity> controls{};const auto slot=world.local_slots[0];
    controls[slot].yaw=approach.spawn.facing;
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
    if(selected.boarding) {
      const auto driver=world.local_slots[1];controls[driver].yaw=driver_approach.spawn.facing;
      const auto placed=world.match.state();
      const auto actual_target=std::find_if(placed.world_vehicles.begin(),placed.world_vehicles.end(),[&](const auto &v){return v.placement==driver_approach.placement;});
      check(actual_target!=placed.world_vehicles.end(),"Boarding occupant lost the actual authored vehicle");
      controls[driver].vehicle_interaction_target=SourcePlayerActionTarget{5,std::int16_t(occupied_seat),actual_target->handle};
      bool occupied=false;
      for(unsigned tick=0;tick<150;++tick) {
        controls[driver].pickup=true;controls[driver].pickup_milliseconds=900;world.match.step(controls);
        const auto p=world.match.state().players[driver];
        if(p.source_parent!=null_datum){occupied=true;break;}
      }
      check(occupied,"Boarding fixture failed actual driver admission");
      controls[driver].pickup=false;controls[driver].pickup_milliseconds=0;
      const auto aligned=objects.object_transform(actual_target->handle).basis[0];
      controls[driver].yaw=std::atan2(aligned.y,aligned.x);
      controls[driver].pitch=std::atan2(aligned.z,std::hypot(aligned.x,aligned.y));
      for(unsigned tick=0;tick<40;++tick)world.match.step(controls);
      const auto occupied_driver=world.match.state().players[driver];
      check(occupied_driver.source_seat==int(occupied_seat),"Boarding occupant selected a different authored target seat");
      // Actual native UI/network vehicle intent, delivered through the normal
      // command packet and original admission/action owners. This selects the
      // authored boarding row without depending on nearest-marker tie order.
      const auto boarding_target=selected.child && driver_approach.placement!=approach.placement?objects.child_object(occupied_driver.source_parent,
          content.objects.at(approach.placement).definition):occupied_driver.source_parent;
      check(boarding_target!=null_datum,"Boarding target lost its actual source object identity");
      controls[slot].vehicle_interaction_target=SourcePlayerActionTarget{6,std::int16_t(selected.seat),boarding_target};
    }
    const auto before_entry=world.match.state().players[slot];
    check(!camera.update(0,before_entry.source_unit,before_entry.source_flags,tick_seconds),
      "Ordinary source camera unexpectedly selected a vehicle view");
    if(selected.label==std::string_view("ghost-driver")) {
      controls[slot].zoom=0;world.match.step(controls);
      check(!camera.update(0,before_entry.source_unit,before_entry.source_flags,tick_seconds),
        "Ordinary scope transition acquired the vehicle camera");
      controls[slot].zoom=-1;world.match.step(controls);
      check(!camera.update(0,before_entry.source_unit,before_entry.source_flags,tick_seconds),
        "Ordinary scope release acquired the vehicle camera");
    }
    bool entered=false;
    std::optional<std::array<std::uint32_t,2>> reset_result;
    std::optional<test::LivePhysicsEntryCaptureTu15> reset_capture;
    std::optional<SourcePhysicsEntryObserverBinding> reset_observer;
    if(std::getenv("HALO2_TU15_TEST_LOOK_RESET")) {
      if(std::getenv("HALO2_TU15_TEST_REMOTE_LOOK_RESET")) {
        const std::array remaining_local{world.local_slots[1]};world.bind_source_local_views(remaining_local);
      }
      if(argc==5)reset_capture.emplace(std::filesystem::path(argv[4])/selected.label/"driver-look-reset");
      reset_observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
        if(entry!=0x18a2a0u)return;
        if(reset_capture)reset_capture->observe(entering,abi,objects.memory(),entry);
        if(!entering) {
          const auto input=Bytes(objects.memory().view(0x4f9b54u,4)).u32(0)+0x14u;
          const auto words=Bytes(objects.memory().view(input,0x94));
          reset_result=std::array{words.u32(0x10),words.u32(0x14)};
        }
      });
    }
    for(unsigned tick=0;tick<150;++tick) {
      if(selected.boarding) {
        const auto p=world.match.state().players[slot];
        const auto target=controls[slot].vehicle_interaction_target->object;
        const auto &definition=content.unit_interactions->units.at(content.objects.at(approach.placement).definition);
        auto markers=definition.markers.find(requested.entry_marker);
        if(markers==definition.markers.end())markers=definition.markers.find(requested.marker);
        check(markers!=definition.markers.end(),"Boarding approach has no authored marker");
        const auto posed=pose_model_markers(markers->second,objects.world_nodes(target),definition.permutations,{},1);
        check(!posed.empty(),"Boarding approach has no current source WORLD marker");
        const auto &entry=posed.front().world;
        const auto eye=ordinary_unit_eye(p.position,content.player.camera_height,content.player.crouch_camera_height,p.crouch);
        const auto look=entry.position-eye;
        controls[slot].yaw=std::atan2(look.y,look.x);
        controls[slot].pitch=std::atan2(look.z,std::hypot(look.x,look.y));
        const auto desired=entry.position+entry.basis[0]*(requested.radius*.5f);
        const auto travel=desired-p.position;const auto distance=std::hypot(travel.x,travel.y);
        controls[slot].forward=controls[slot].right=0;
        if(distance>.15f) {
          const Vec3 forward{std::cos(controls[slot].yaw),std::sin(controls[slot].yaw),0};
          const Vec3 right{forward.y,-forward.x,0};
          controls[slot].forward=(travel.x*forward.x+travel.y*forward.y)/distance;
          controls[slot].right=(travel.x*right.x+travel.y*right.y)/distance;
        }
        controls[slot].jump=look.z>0 && tick%30==0;
      }
      controls[slot].pickup=true;controls[slot].pickup_milliseconds=900;world.match.step(controls);
      const auto current=world.match.state();const auto &p=current.players[slot];
      std::optional<test::LivePhysicsEntryCaptureTu15> capture;
      std::optional<SourcePhysicsEntryObserverBinding> observer;
      if(argc==5 && p.source_parent!=null_datum && !reset_observer) {
        capture.emplace(std::filesystem::path(argv[4])/selected.label/"entry-director");
        observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry==0x157fe0u)capture->observe(entering,abi,objects.memory(),entry);
        });
      }
      camera.update(0,p.source_unit,p.source_flags,tick_seconds);
      if(p.source_parent!=null_datum) {
        if(std::getenv("HALO2_TU15_TEST_LOOK_RESET") && (requested.flags&4u)) {
          const auto retained=0x60070000u+(p.source_player&0xffffu)*0xa0u;
          const auto actual=Bytes(objects.memory().view(retained,0x94));
          check(reset_result.has_value(),"Driver entry failed to invoke the original retained input reset");
          if(actual.u32(0x10)!=(*reset_result)[0] || actual.u32(0x14)!=(*reset_result)[1]) {
            std::cout<<nlohmann::json{{"source_reset",{std::bit_cast<float>((*reset_result)[0]),std::bit_cast<float>((*reset_result)[1])}},
              {"native_retained",{actual.f32(0x10),actual.f32(0x14)}}}.dump()<<'\n';
            throw FormatError("Driver seat entry discarded original18A2A0 look reset");
          }
          reset_observer.reset();
          std::optional<std::array<std::uint32_t,2>> next_result;
          SourcePhysicsEntryObserverBinding next_observer(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &) {
            if(entering || entry!=0x18a300u)return;
            const auto input=Bytes(objects.memory().view(0x4f9b54u,4)).u32(0)+0x14u;
            const auto completed=Bytes(objects.memory().view(input,0x94));
            if(completed.u32(0)==p.source_unit)next_result=std::array{completed.u32(0x10),completed.u32(0x14)};
          });
          controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;world.match.step(controls);
          const auto next=Bytes(objects.memory().view(retained,0x94));
          check(next_result && next.u32(0x10)==(*next_result)[0] && next.u32(0x14)==(*next_result)[1],
            "Next native command discarded the completed original seated look");
          std::cout<<"PASS retained original driver look reset\n";return 0;
        }
        entered=true;break;
      }
    }
    if(!entered && selected.boarding) {
      const auto p=world.match.state().players[slot];nlohmann::json failure={{"case",selected.label},
        {"unit",p.source_unit},{"dead",p.dead},{"position",{p.position.x,p.position.y,p.position.z}},
        {"mode",cache.string_id(requested.mode)},{"entry_marker",cache.string_id(requested.entry_marker)}};
      if(p.source_unit!=null_datum) {
        const auto target=controls[slot].vehicle_interaction_target->object;
        failure["original_cb7a0_admission"]=objects.tu15_world().call_actual(0xcb7a0u,
            std::array{p.source_unit,target,std::uint32_t(selected.seat),0x6005f000u,0x6005f004u})&255u;
        const auto &body=content.unit_interactions->units.at(content.player.datum);
        const auto animation=body.seat_animations.find(requested.mode);
        failure["entry_animation_count"]=animation==body.seat_animations.end()?0:animation->second.entries.size();
      }
      std::cout<<failure.dump()<<'\n';
    }
    check(entered,"Camera case failed original seat entry");
    controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;controls[slot].jump=false;controls[slot].forward=controls[slot].right=0;
    if(selected.boarding) {
      std::optional<test::LivePhysicsEntryCaptureTu15> sphere_capture;
      std::optional<SourcePhysicsEntryObserverBinding> sphere_observer;
      if(argc==5) {
        sphere_capture.emplace(std::filesystem::path(argv[4])/selected.label/"sphere-query");
        sphere_observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry==0x2f9d70u)sphere_capture->observe(entering,abi,objects.memory(),entry);
        });
      }
      const auto boarding=world.match.state().players[slot];
      if(boarding.source_seat!=int(selected.seat))throw FormatError("Source boarding intent selected seat "+std::to_string(boarding.source_seat)+" instead of "+std::to_string(selected.seat));
      check((content.unit_interactions->units.at(content.objects.at(approach.placement).definition).seats.at(selected.seat).flags&0x800u)!=0,
        "Requested seat is not authored for boarding");
      const auto view=camera.update_player(0,boarding.source_player,boarding.source_flags,tick_seconds);
      check(view && view->camera_mode==1,"Actual boarding seat did not select the original third-person camera");
      unsigned boarding_frames=1,transition_frames=0;
      for(unsigned tick=0;tick<180;++tick) {
        world.match.step(controls);const auto p=world.match.state().players[slot];
        const auto v=camera.update_player(0,p.source_player,p.source_flags,tick_seconds);
        if(p.source_seat==int(selected.seat) && p.source_parent==boarding.source_parent)++boarding_frames;
        else ++transition_frames;
        if(v)check(std::isfinite(v->position.x) && std::abs(length(v->forward)-1)<.0001f,
          "Board/eject transition produced an invalid original camera");
      }
      bool released=false;unsigned exit_frames=0;
      for(unsigned tick=0;tick<150;++tick) {
        controls[slot].pickup=true;controls[slot].pickup_milliseconds=900;world.match.step(controls);
        const auto p=world.match.state().players[slot];
        auto v=camera.update_player(0,p.source_player,p.source_flags,tick_seconds);
        if(p.source_parent!=null_datum)continue;
        released=true;controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
        for(unsigned settle=0;settle<180 && v;++settle) {
          ++exit_frames;world.match.step(controls);const auto p=world.match.state().players[slot];
          v=camera.update_player(0,p.source_player,p.source_flags,tick_seconds);
        }
        check(!v,"Original boarding exit handoff failed to settle");break;
      }
      check(released,"Original boarding camera case failed seat release");
      std::cout<<"PASS "<<selected.label<<" actual boarding camera frames="<<boarding_frames<<" transition frames="<<transition_frames<<" exit frames="<<exit_frames<<'\n';
      report["cases"].push_back({{"case",selected.label},{"boarding_frames",boarding_frames},{"transition_frames",transition_frames},{"released",released},{"exit_frames",exit_frames}});
      if(argc==5){std::ofstream out(std::filesystem::path(argv[4])/"matrix.json");out<<report.dump(2)<<'\n';}
      continue;
    }
    for(unsigned tick=0;tick<40;++tick)world.match.step(controls);
    const auto state=world.match.state();const auto &player=state.players[slot];
    check(player.source_seat==int(selected.seat),"Camera case selected a different original seat");
    check(objects.object_definition(player.source_parent)==content.objects.at(approach.placement).definition,
      "Camera case entered a different authored vehicle definition");
    const auto &seat=content.unit_interactions->units.at(content.objects.at(approach.placement).definition).seats.at(selected.seat);
    const bool third_person=(seat.flags&0x10u)!=0;
    const auto &other=state.players[world.local_slots[1]];
    check(!camera.update(1,other.source_unit,other.source_flags,tick_seconds),
      "Vehicle camera leaked into an unseated local view");
    const float baseline=original.death_camera_constants->default_fov;
    check(std::bit_cast<std::uint32_t>(baseline)==0x3f9c61aau,"TU1.5 static camera default changed");
    nlohmann::json samples=nlohmann::json::array();
    const auto initial_yaw=controls[slot].yaw;
    for(unsigned sample=0;sample<9;++sample) {
      const auto yaw=initial_yaw+float(sample)*.35f;
      const auto pitch=-1.49f+float(sample)*.3725f;
      std::optional<test::LivePhysicsEntryCaptureTu15> director_capture,look_capture,mode_capture;
      std::optional<SourcePhysicsEntryObserverBinding> observer;
      if(argc==5 && sample==4) {
        const auto directory=std::filesystem::path(argv[4])/selected.label;
        director_capture.emplace(directory/"director");
        look_capture.emplace(directory/"look");
        if(selected.label==std::string_view("ghost-driver"))mode_capture.emplace(directory/"perspective");
        observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry==0x158290u && mode_capture)mode_capture->observe(entering,abi,objects.memory(),entry);
          if(entry==0x157fe0u)director_capture->observe(entering,abi,objects.memory(),entry);
          if(entry==0x18a300u)look_capture->observe(entering,abi,objects.memory(),entry);
        });
      }
      controls[slot].yaw=yaw;controls[slot].pitch=pitch;world.match.step(controls);
      const auto retained=0x60070000u+(player.source_player&0xffffu)*0xa0u;
      const Bytes look(objects.memory().view(retained,0x94));
      const auto delivered=world.match.state().players[slot];
      check(std::bit_cast<std::uint32_t>(delivered.yaw)==std::bit_cast<std::uint32_t>(look.f32(0x10)) &&
          std::bit_cast<std::uint32_t>(delivered.pitch)==std::bit_cast<std::uint32_t>(look.f32(0x14)),
          "Snapshot look differs from completed source input");
      std::array<std::byte,0x94> look_before{};objects.memory().read(retained,look_before);
      const auto view=camera.update(0,player.source_unit,player.source_flags,tick_seconds);
      check(std::equal(look_before.begin(),look_before.end(),objects.memory().view(retained,0x94).begin()),
        "Rendering changed command-tick look state");
      check(bool(view)==third_person,"Stable vehicle camera admission differs from authored seat flags");
      if(!view)continue;
      const auto &v=*view;
      check(v.camera_mode==1,"Stable seated perspective differs from original158290");
      check(std::isfinite(v.position.x) && std::isfinite(v.position.y) && std::isfinite(v.position.z) &&
          std::isfinite(v.horizontal_fov) && std::isfinite(v.aim_displacement) && v.horizontal_fov>0 &&
          std::abs(length(v.forward)-1)<.0001f && std::abs(length(v.up)-1)<.0001f &&
          std::abs(dot(v.forward,v.up))<.0001f,"Original camera output has an invalid basis or lens");
      const auto lens=vehicle_camera_angles(v.horizontal_fov,baseline,70);
      const auto expected=source_camera_angles(v.horizontal_fov,1.3333333730697632f,{1,1},{1,1},baseline);
      check(lens.projection_angle==expected.projection_angle && lens.lighting_distance_scale==expected.lighting_distance_scale,
          "Vehicle render lens replaced the actual original observer output");
      samples.push_back({{"yaw",yaw},{"pitch",pitch},{"position",{v.position.x,v.position.y,v.position.z}},
        {"source_look",{look.f32(0x10),look.f32(0x14)}},{"source_pitch_limits",{look.f32(0x80),look.f32(0x84)}},
        {"forward",{v.forward.x,v.forward.y,v.forward.z}},{"horizontal_fov",v.horizontal_fov},
        {"projection_angle",lens.projection_angle},{"hud_aim_displacement",v.aim_displacement}});
    }
    nlohmann::json moving;
    controls[slot].pitch=2.f;world.match.step(controls);
    check(std::abs(world.match.state().players[slot].pitch)<1.55f,
      "Unbounded native pitch escaped the original source look limits");
    if(selected.label==std::string_view("ghost-driver")) {
      bool observed=false;
      SourcePhysicsEntryObserverBinding observer(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
        if(!entering || entry!=0x18a300u || observed)return;
        const auto delta=Bytes(objects.memory().view(abi.gp[4]+4u,4)).f32(0);
        check(std::abs(delta)<.000001f,"Native yaw wrap delivered a full turn to the source seat constraint");
        observed=true;
      });
      controls[slot].yaw+=6.2831854820251465f;world.match.step(controls);
      check(observed,"Wrapped input did not reach the actual source look owner");
    }
    if(seat.flags&4u) {
      controls[slot].forward=.8f;controls[slot].yaw=initial_yaw;controls[slot].pitch=.75f;
      for(unsigned tick=0;tick<20;++tick) {
        world.match.step(controls);const auto p=world.match.state().players[slot];
        camera.update(0,p.source_unit,p.source_flags,tick_seconds);
      }
      const auto parent=objects.object_body(world.match.state().players[slot].source_parent);
      const Bytes velocity(objects.memory().view(parent+0x88u,12));
      const auto speed=length(Vec3{velocity.f32(0),velocity.f32(4),velocity.f32(8)});
      const auto retained=0x60070000u+(player.source_player&0xffffu)*0xa0u;
      const auto previous=Bytes(objects.memory().view(retained,0x94)).f32(0x14);
      std::optional<test::LivePhysicsEntryCaptureTu15> capture;
      std::optional<SourcePhysicsEntryObserverBinding> observer;
      if(argc==5) {
        capture.emplace(std::filesystem::path(argv[4])/selected.label/"moving-look");
        observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry==0x18a300u)capture->observe(entering,abi,objects.memory(),entry);
        });
      }
      world.match.step(controls);
      const auto p=world.match.state().players[slot];camera.update(0,p.source_unit,p.source_flags,tick_seconds);
      moving={{"source_speed",speed},{"pitch_before",previous},
        {"pitch_after",Bytes(objects.memory().view(retained,0x94)).f32(0x14)}};
      controls[slot].forward=0;
    }
    if(third_person) {
      check(samples.size()==9,"Third-person camera sweep incomplete");
      check(samples.front().at("forward")!=samples.back().at("forward"),"Camera look remained frozen");
    }
    if(std::getenv("HALO2_TU15_TEST_SEATED_DEATH")) {
      check(third_person,"Death handoff fixture requires a third-person seat");
      const auto prior=camera.update_player(0,player.source_player,player.source_flags,tick_seconds);
      check(prior.has_value(),"Missing actual seated observer before fatal cleanup");
      const auto auxiliary_address=Bytes(objects.memory().view(0x4f3cd8u,4)).u32(0)+4u;
      objects.kill_biped(player.source_unit);
      const auto auxiliary=Bytes(objects.memory().view(auxiliary_address,4)).u32(0);
      const auto player_pool=Bytes(objects.memory().view(objects.source_player_pool(),0x4c));
      const auto owner=player_pool.u32(0x44)+(player.source_player&0xffffu)*0x21cu;
      check(Bytes(objects.memory().view(owner,0x34)).u32(0x2c)==null_datum,
        "Original fatal cleanup did not detach the actual player");
      const auto present=[&](const char *phase,float seconds,bool cycle=false) {
        std::optional<test::LivePhysicsEntryCaptureTu15> capture;
        std::optional<SourcePhysicsEntryObserverBinding> observer;
        if(argc==5)capture.emplace(std::filesystem::path(argv[4])/selected.label/phase);
        observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry!=0x157fe0u)return;
          if(capture)capture->observe(entering,abi,objects.memory(),entry);
          if(entering && std::string_view(phase)=="death-entry") {
            const auto retained=Bytes(objects.memory().view(0x4f64a4u,0x358));
            check(retained.f32(0xb8)==prior->position.x && retained.f32(0xbc)==prior->position.y &&
                retained.f32(0xc0)==prior->position.z,"Fatal camera reset the previous seated observer");
            check(Bytes(objects.memory().view(0x4f5514u,0x140)).u32(0xc)==0x245ea0u,
                "Fatal camera discarded the previous vehicle controller");
          }
        });
        return camera.update_player(0,player.source_player,player.source_flags,seconds,cycle);
      };
      auto dead_view=present("death-entry",tick_seconds);
      check(dead_view.has_value() && camera.death_view(0),
        "Vehicle death discarded the retained original director and observer");
      for(unsigned n=0;n<60;++n) {
        dead_view=n==30?present("death-stable",tick_seconds):
          camera.update_player(0,player.source_player,player.source_flags,tick_seconds);
        check(dead_view.has_value() && std::isfinite(dead_view->position.x) &&
            std::isfinite(dead_view->horizontal_fov) && std::abs(length(dead_view->forward)-1)<.0001f,
            "Original seated death observer produced invalid output");
      }
      present("death-watch",original.death_camera_constants->watch_seconds+tick_seconds);
      present("death-cycle",tick_seconds,true);
      check(Bytes(objects.memory().view(auxiliary_address,4)).u32(0)==auxiliary,
        "Native camera presentation advanced the shared source auxiliary RNG");
      const auto replacement=objects.create_biped(content.player.datum,cache.spawns().back().position,{1,0,0});
      check(replacement!=null_datum && replacement!=player.source_unit,"Respawn reused the full corpse identity");
      objects.bind_player(player.source_player,replacement);
      objects.initialize_player_equipment(player.source_player,replacement);
      auto restored=present("death-respawn",tick_seconds);
      check(!camera.death_view(0),"Actual replacement unit did not restore the original normal director");
      check(restored.has_value(),"Respawn discarded pending original observer transitions");
      unsigned restoring_frames=0;
      while(restored && restoring_frames<180) {
        ++restoring_frames;
        restored=camera.update_player(0,player.source_player,player.source_flags,tick_seconds);
      }
      check(restoring_frames>0 && !restored,"Original respawn handoff failed to settle into ordinary ownership");
      std::cout<<"PASS "<<selected.label<<" seated fatal cleanup, retained director/observer, watch/cycle and distinct respawn\n";
      return 0;
    }
    bool exited=false;
    unsigned exit_transition_frames=0;bool first_person_during_handoff=false;
    for(unsigned tick=0;tick<150;++tick) {
      controls[slot].pickup=true;controls[slot].pickup_milliseconds=900;world.match.step(controls);
      const auto current=world.match.state();const auto &p=current.players[slot];
      std::optional<test::LivePhysicsEntryCaptureTu15> capture;
      std::optional<SourcePhysicsEntryObserverBinding> observer;
      if(argc==5 && p.source_parent==null_datum) {
        capture.emplace(std::filesystem::path(argv[4])/selected.label/"exit-director");
        observer.emplace(objects.memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
          if(entry==0x157fe0u)capture->observe(entering,abi,objects.memory(),entry);
        });
      }
      const auto exiting_view=camera.update(0,p.source_unit,p.source_flags,tick_seconds);
      if(p.source_parent==null_datum) {
        exited=true;
        controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
        auto settling_view=exiting_view;
        for(unsigned settle=0;settle<150 && settling_view;++settle) {
          first_person_during_handoff|=settling_view->camera_mode==0;
          ++exit_transition_frames;world.match.step(controls);
          const auto settling=world.match.state();const auto &s=settling.players[slot];
          settling_view=camera.update(0,s.source_unit,s.source_flags,tick_seconds);
        }
        check(!settling_view,"Source vehicle exit transition failed to complete");break;
      }
    }
    check(exited,"Camera case failed original seat exit");
    report["cases"].push_back({{"case",selected.label},{"map",options.map.filename().string()},{"seat_flags",seat.flags},{"third_person",third_person},
      {"samples",samples},{"entered",entered},{"exited",exited},{"exit_transition_frames",exit_transition_frames},{"first_person_during_handoff",first_person_during_handoff},
      {"moving_look",moving}});
    if(argc==5){std::ofstream out(std::filesystem::path(argv[4])/"matrix.json");out<<report.dump(2)<<'\n';}
    std::cout<<"PASS "<<selected.label<<" third-person="<<third_person<<" samples="<<samples.size()<<'\n';
  }
  check(!report["cases"].empty(),"No vehicle camera cases selected");
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
