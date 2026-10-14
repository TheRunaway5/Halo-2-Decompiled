#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/simulation.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/damage_amount.hpp"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-death-cycle-XXXXXX").string();
    const auto p=::mkdtemp(name.data());check(p,"Create native source savegame directory");path=p;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply owned English TU1.5 XBE and Xbox map");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  const auto original=halo2::OriginalData::load_xbe(argv[1]);
  halo2::Cache cache(argv[2],{},original);const auto content=cache.load_content(0,true);
  Files files;halo2::MultiplayerObjectRuntime world(argv[1],cache,0,files.path);
  auto variant=halo2::source_game_variant_default(0);halo2::source_game_variant_set(variant,0x59,5);
  world.configure_session(variant,1,30,0x12345678u);
  const auto &spawn=cache.spawns().front();
  const auto object=world.create_biped(content.player.datum,spawn.position,{1,0,0});
  check(object!=halo2::null_datum,"Actual biped birth rejected");
  halo2::DatumPool players(world.memory(),world.player_pool());const auto player=players.allocate_at(0);
  world.construct_player(player,0,halo2::source_default_player_configuration());
  world.bind_player(player,object);world.initialize_player_equipment(player,object);
  std::uint32_t random=0x12345678u;halo2::NativeDeathCamera camera(world,*original,random);
  const halo2::NativeDeathCameraView ordinary{
    halo2::ordinary_unit_eye(spawn.position,content.player.camera_height,content.player.crouch_camera_height,0),
    {1,0,0},{0,0,1},content.player.camera_field_of_view};
  check(!camera.update(0,player,1,false,halo2::tick_seconds,false,ordinary),
      "Living owner selected a death camera");
  // Defined-input damage owner: actual cache weapon/vitality definitions,
  // followed by delivery into the same body consumed by fatal cleanup.
  const auto initial=world.object_vitality(object);
  halo2::RechargeState vitality;vitality.body=initial.body;vitality.shield=initial.shield;
  auto feedback=initial.feedback;
  const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &entry) {
    return entry.second.smg.has_value();
  });
  check(weapon!=content.weapons.end(),"Actual SMG damage definition unavailable");
  check(weapon->second.damage_amount.has_value() &&
      weapon->second.damage_amount->upper_minimum==weapon->second.damage_amount->upper_maximum,
      "Component damage probe requires its actual deterministic SMG amount owner");
  const auto amount=halo2::damage_amount(weapon->second.damage_amount->lower,
      weapon->second.damage_amount->upper_minimum,1.f,1.f,1.f);
  for(unsigned n=0;n<240 && !vitality.dead;++n) {
    const auto result=halo2::apply_unit_damage(vitality,content.player.vitality,weapon->second,
        content.damage_groups,content.damage_materials,amount);
    halo2::report_damage_feedback(feedback,result.body,result.shield,result.report_flags);
  }
  check(vitality.dead,"Actual cache damage owner did not deplete source vitality");
  const auto persistent=world.object_body(object);
  check(persistent.value>0xffffffffu && world.object_pool().value>0xffffffffu &&
        world.player_pool().value>0xffffffffu && world.source_clock().value>0xffffffffu &&
        world.memory().source_address(persistent,4)==world.source_object_body(object),
        "Live gameplay interfaces lost their native 64-bit backing or original ABI identity");
  const auto maxima=world.memory().view(persistent+0xe4u,8u);
  const std::vector<std::byte> saved_maxima(maxima.begin(),maxima.end());
  world.deliver_vitality(object,{vitality.body,vitality.shield,feedback});
  const auto delivered=world.object_vitality(object);
  check(delivered.body==vitality.body && delivered.shield==vitality.shield &&
      delivered.feedback.body.visible==feedback.body.visible &&
      delivered.feedback.shield.accumulated==feedback.shield.accumulated &&
      delivered.feedback.body.ticks==feedback.body.ticks &&
      std::ranges::equal(saved_maxima,world.memory().view(persistent+0xe4u,8u)),
      "Completed damage delivery changed maxima or lost fractions and feedback");
  world.kill_biped(object);
  const auto motion=world.biped_motion(object);
  check(motion.physical_state==3,"Original cleanup did not enter physical death state");
  const auto other_start=std::find_if(cache.spawns().begin(),cache.spawns().end(),[&](const auto &s) {
    return halo2::length(s.position-spawn.position)>5.f;
  });
  check(other_start!=cache.spawns().end(),"No distinct authored start for the living spectator target");
  const auto other=world.create_biped(content.player.datum,other_start->position,{1,0,0});
  const auto other_player=players.allocate_at(1);
  check(other!=halo2::null_datum && other_player!=halo2::null_datum,"Spectator target birth rejected");
  world.construct_player(other_player,1,halo2::source_default_player_configuration());
  world.bind_player(other_player,other);world.initialize_player_equipment(other_player,other);
  const bool campaign_camera_probe=std::getenv("HALO2_TU15_TEST_CAMPAIGN_CAMERA");
  std::uint32_t game{};
  const auto set_game_type=[&](std::uint32_t type) {
    std::array<std::byte,4> bytes{};
    for(unsigned i=0;i<4;++i)bytes[i]=std::byte(type>>(8*i));
    world.memory().write(game+8,bytes);
  };
  if(campaign_camera_probe) {
    // Declared camera-only input after actual MP object/player construction.
    // This is not a campaign session or a campaign birth/placement fixture.
    const auto pointer=world.memory().view(0x4f3218u,4);
    for(unsigned i=0;i<4;++i)game|=std::to_integer<std::uint32_t>(pointer[i])<<(8*i);
    set_game_type(1);
    auto record=players.lookup(player);record[2]|=8u;
    check(!camera.update(0,player,9,false,halo2::tick_seconds,false,ordinary),
        "Campaign flag8 selected death view instead of ordinary ownership");
    record[2]&=std::uint8_t(~8u);
  }
  for(unsigned n=0;n<60;++n) {
    const auto view=camera.update(0,player,1,false,halo2::tick_seconds,false,ordinary);
    check(view.has_value(),"Detached player did not select original death observer");
    for(float value:{view->position.x,view->position.y,view->position.z,
        view->forward.x,view->forward.y,view->forward.z,view->horizontal_fov})
      check(std::isfinite(value),"Death observer produced non-finite output");
    check(view->horizontal_fov>0,"Death observer lost its original FOV");
  }
  // The real source clock is initialized and unpaused. Expire the original
  // watch timer using a declared presentation-time input; a match
  // state boolean must not suppress its automatic live-player replacement.
  const auto clock=world.memory().view(world.source_clock(),2);
  check(clock[0]!=std::byte{} && clock[1]==std::byte{},"Source camera clock is not initialized and unpaused");
  const auto watch=campaign_camera_probe?original->death_camera_constants->transition_seconds:
      original->death_camera_constants->watch_seconds;
  camera.update(0,player,1,false,watch+halo2::tick_seconds,
      false,ordinary);
  std::optional<halo2::NativeDeathCameraView> watched;
  for(unsigned n=0;n<150;++n)watched=camera.update(0,player,1,false,halo2::tick_seconds,false,ordinary);
  const auto target=world.camera_target(other);
  check(watched && target && halo2::dot(target->node0-watched->position,watched->forward)>0 &&
      halo2::length(halo2::cross(target->node0-watched->position,watched->forward))<.05f,
      "Expired source watch timer did not follow the living player on an unpaused clock");
  const auto old_nodes=world.world_nodes(object);
  if(campaign_camera_probe)set_game_type(2); // Actual existing MP birth owner.
  const auto replacement=world.create_biped(content.player.datum,spawn.position,{1,0,0});
  check(replacement!=object && replacement!=halo2::null_datum,"Respawn reused the full corpse identity");
  world.bind_player(player,replacement);world.initialize_player_equipment(player,replacement);
  if(campaign_camera_probe)set_game_type(1);
  check(!camera.update(0,player,1,false,halo2::tick_seconds,false,ordinary),
      "Respawn did not restore ordinary camera ownership");
  check(world.camera_target(object).has_value() && world.world_nodes(object).size()==old_nodes.size(),
      "New player birth retired the previous source corpse");
  std::cout<<"PASS fresh TU1.5 fatal cleanup, obstructed observer, automatic live-player target, distinct replacement and retained corpse\n";
  std::cout<<"LIMIT no full world tick, continuous ragdoll or live input death run in this component probe\n";
  if(campaign_camera_probe)std::cout<<"PASS source-options campaign camera gate, three-second watch and restoration over actual retained source banks\n"
      <<"LIMIT campaign type is a declared camera stimulus; campaign session, birth, safe placement and network co-op are not supplied\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
