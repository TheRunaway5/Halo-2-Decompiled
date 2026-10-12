#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include <array>
#include <bit>
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
}
int main(int argc,char** argv) {
  try {
    if(argc!=4)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
    Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    const auto content=cache.load_content(0,true);
    check(!content.kill_triggers.empty(),"Actual Lockout has no imported kill trigger");
    const auto& volume=content.trigger_volumes.at(content.kill_triggers.front());
    check(volume.object_name==-1 && words(volume.up)==words({0,0,1}),
        "This diagnostic requires the actual unparented horizontal Lockout kill volume");
    auto control_content=content;
    // Explicit native control: remove only kill admission to observe the same
    // production movement's velocity immediately before the fatal postpass.
    // This twin is not another original oracle or an alternate collision world.
    control_content.kill_triggers.clear();
    auto rules=slayer_rules({"Kill-trigger momentum",0,{{0x59,5}}});
    rules.weapon=cache.slayer_starting_weapon(5);
    rules.time_ticks=rules.score_limit=0;rules.respawn_ticks=30;
    MultiplayerMatch match(content,0,cache.spawns(),rules);
    MultiplayerMatch control(control_content,0,cache.spawns(),rules);
    for(auto* game:{&match,&control})check(game->join("Falling victim")==0,
        "Actual Match participant allocation differs");
    const auto victim=match.unit(0).source_datum;
    const auto owner=match.player(0).source_datum;
    check(victim!=null_datum && owner!=null_datum &&
        match.unit(0).weapon.definition==rules.weapon &&
        match.source_inventory(0).slots[0]==match.unit(0).weapon.source_datum &&
        match.unit(0).weapon.loaded==content.weapons.at(rules.weapon).capacity,
        "Actual authored spawn/start-SMG inventory differs");
    // Explicit public debug start, one unit above the authored volume's upper
    // face. Native normal movement/gravity must produce every later position
    // and velocity. No velocity, vitality, animation pose or contact is assigned.
    const auto right=cross(volume.up,volume.forward);
    Spawn above;above.bsp=0;above.facing=match.unit(0).yaw;
    above.position=volume.position+volume.forward*(volume.extents.x*.5f)+
        right*(volume.extents.y*.5f)+volume.up*(volume.extents.z+1.f);
    DebugGameRequest request;request.kind=DebugGameRequest::Teleport;request.player=0;
    match.debug_request(request,&above);control.debug_request(request,&above);
    std::array<Controls,multiplayer_capacity> input{};
    input[0].yaw=match.unit(0).yaw;
    bool fatal=false;Vec3 velocity{};std::uint64_t death_tick{};
    for(unsigned frame=0;frame<240 && !fatal;++frame) {
      control.step(input);
      check(!control.unit(0).vitality.dead,"Native control died outside disabled kill postpass");
      const auto expected=control.unit(0).body.velocity;
      const auto clock=match.simulation().ticks();
      match.step(input);fatal=match.unit(0).vitality.dead;
      check(words(match.unit(0).body.position)==words(control.unit(0).body.position),
          "Identical native falling movement diverged before kill admission");
      if(fatal) {
        check(expected.z<0 && words(match.unit(0).body.velocity)==words(expected),
            "Kill-trigger fatal postpass discarded actual current falling momentum");
        check(match.unit(0).last_damager==-2 && match.simulation().hits()==0 &&
            match.player(0).deaths==1,"Death was not the actual Match kill-volume path");
        velocity=expected;death_tick=clock;
      } else check(words(match.unit(0).body.velocity)==words(expected),
          "Control and admitted Match movement differ before fatal postpass");
    }
    check(fatal,"Native normal falling did not enter the authored kill volume");
    check(match.player(0).source_previous_unit==victim &&
        match.unit(0).source_datum==victim && match.unit(0).source_player==null_datum &&
        match.unit(0).source_previous_player==owner && match.unit(0).source_death_tick==death_tick,
        "Kill cleanup lost full corpse identity, previous owner or death clock");
    input={};
    for(unsigned frame=0;frame<180 && match.unit(0).vitality.dead;++frame)match.step(input);
    check(!match.unit(0).vitality.dead && match.unit(0).source_datum!=victim,
        "Source respawn did not allocate a distinct salted current unit");
    const auto* corpse=match.unit_from_datum(victim);
    check(corpse && corpse!=&match.unit(0) && corpse->vitality.dead &&
        corpse->source_datum==victim && corpse->source_player==null_datum &&
        corpse->source_previous_player==owner && corpse->source_death_tick==death_tick &&
        words(corpse->body.velocity)==words(velocity),
        "Respawn discarded the retained old body's current momentum or identity");
    std::cout<<"Actual Match falling kill retained momentum, full corpse datum, previous owner and death tick through respawn\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
