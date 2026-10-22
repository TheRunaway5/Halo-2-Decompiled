#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char* message) {
  if(!value)throw FormatError(message);
}
std::array<std::uint32_t,3> words(Vec3 value) {
  return {std::bit_cast<std::uint32_t>(value.x),
      std::bit_cast<std::uint32_t>(value.y),std::bit_cast<std::uint32_t>(value.z)};
}
Vec3 center(const Body& body) {
  return body.position+Vec3{0,0,body.height*.5f};
}
void place_shooter(MultiplayerMatch& match,unsigned shooter,Vec3 target) {
  // Declared fixture setup is confined to the shooter. The victim's
  // authored root, vitality and velocity are never assigned by this test.
  Spawn setup;setup.bsp=0;setup.position=target+Vec3{-1,0,0};
  setup.position.z-=match.unit(shooter).definition.camera_height;
  setup.facing=0;
  test::place_player(match,shooter,setup);
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw FormatError("Supply the original Lockout map");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    auto rules=slayer_rules({"Ordinary fatal momentum",0,{{0x59,5}}});
    rules.weapon=cache.slayer_starting_weapon(5);
    rules.time_ticks=rules.score_limit=0;
    rules.respawn_ticks=30;
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    MultiplayerMatch unhit(content,0,cache.spawns(),rules);
    for(auto* game:{&match,&unhit}) {
      check(game->join("Moving victim")==0 && game->join("SMG shooter")==1,
          "Actual Match participant allocation differs");

    }
    const auto victim_datum=match.unit(0).source_datum;
    const auto owner=match.player(0).source_datum;
    const auto weapon=match.unit(1).weapon.source_datum;
    check(victim_datum!=null_datum && weapon!=null_datum &&
        match.unit(1).weapon.definition==rules.weapon &&
        match.source_inventory(1).slots[0]==weapon &&
        match.source_inventory(1).current[0]==0 &&
        match.unit(1).weapon.loaded==content.weapons.at(rules.weapon).capacity,
        "Actual authored spawn/start-SMG inventory differs");
    std::array<Controls,multiplayer_capacity> input{};
    input[0].yaw=match.unit(0).yaw;
    input[0].forward=.25f;
    Vec3 fatal_velocity{};
    std::uint64_t death_tick{};
    bool fatal=false;
    for(unsigned frame=0;frame<240 && !fatal;++frame) {
      // The unhit production Match executes the identical native movement
      // step. Its current velocity is the victim's value before the shot
      // postpass, without a health/velocity fixture or a movement formula copy.
      place_shooter(unhit,1,center(unhit.unit(0).body));
      auto no_shot=input;no_shot[1].fire=false;
      unhit.step(no_shot);
      check(!unhit.unit(0).vitality.dead,"Authored moving control entered a kill volume");
      const auto expected_velocity=unhit.unit(0).body.velocity;
      place_shooter(match,1,center(unhit.unit(0).body));
      input[1].fire=true;
      const auto before=match.simulation().ticks();
      match.step(input);
      fatal=match.unit(0).vitality.dead;
      if(fatal) {
        check(match.player(1).kills==1 && match.player(0).deaths==1 &&
            match.simulation().hits()>0,"Death was not the actual ordinary SMG hit path");
        check(length(expected_velocity)>0 && words(match.unit(0).body.velocity)==words(expected_velocity),
            "Ordinary fatal damage discarded the victim's actual current movement");
        fatal_velocity=expected_velocity;death_tick=before;
      } else {
        check(words(match.unit(0).body.velocity)==words(expected_velocity),
            "Control and firing Match movement diverged before the fatal postpass");
      }
    }
    check(fatal,"Actual stock SMG did not fatally hit the moving authored victim");
    check(match.player(0).source_previous_unit==victim_datum &&
        match.unit(0).source_datum==victim_datum &&
        match.unit(0).source_previous_player==owner &&
        match.unit(0).source_player==null_datum &&
        match.unit(0).source_death_tick==death_tick,
        "Fatal cleanup lost full corpse identity/previous owner/death clock");
    input={};
    for(unsigned frame=0;frame<180 && match.unit(0).vitality.dead;++frame)match.step(input);
    check(!match.unit(0).vitality.dead && match.unit(0).source_datum!=victim_datum,
        "Source respawn did not allocate a distinct current unit");
    const auto* corpse=match.unit_from_datum(victim_datum);
    check(corpse && corpse!=&match.unit(0) && corpse->vitality.dead &&
        corpse->source_datum==victim_datum && corpse->source_previous_player==owner &&
        corpse->source_player==null_datum && corpse->source_death_tick==death_tick &&
        words(corpse->body.velocity)==words(fatal_velocity),
        "Respawn reset the retained old body's actual fatal momentum or ownership");
    std::cout<<"Actual moving Match SMG death retained velocity, full corpse datum, previous owner and death tick through respawn\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
