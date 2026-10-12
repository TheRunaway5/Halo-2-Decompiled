#include "halo2/fp_registration.hpp"
#include "halo2/multiplayer.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;if(!(row>>value))throw FormatError("Incomplete original registration fixture");
  return std::stoul(value,nullptr,16);
}
void check(bool condition,const std::string &label) {if(!condition)throw FormatError("Original FP binding differs: "+label);}
void compare(std::istringstream &row,const std::array<FirstPersonViewBinding,4> &views,const std::string &label) {
  for(const auto &view:views) {
    check(view.unit==word(row),label+" full unit");check(std::uint32_t(view.representation)==word(row),label+" selector");
    for(const auto &hand:view.hands) {
      check(hand.flags==word(row),label+" hand flags");check(hand.weapon==word(row),label+" full weapon");
      check(hand.weapon_node_count==word(row),label+" weapon count");check(hand.arms_node_count==word(row),label+" arms count");
      for(auto i:hand.weapon_nodes)check(std::uint32_t(i)==word(row),label+" weapon map including retained entries");
      for(auto i:hand.arms_nodes)check(std::uint32_t(i)==word(row),label+" arms map including retained entries");
      check(hand.graph==word(row),label+" graph");check(hand.animation_identifier==word(row),label+" initial channel");
      check(hand.channel_flags==word(row),label+" channel flags");check(hand.channel_options==word(row),label+" channel options");
      check(std::bit_cast<std::uint32_t>(hand.rate)==word(row),label+" rate");
      check(std::bit_cast<std::uint32_t>(hand.frame)==word(row),label+" initial frame");
    }
  }
}
void actual_match(const Cache &cache,const LevelContent &content,const FirstPersonBindingDefinition &definition) {
  SlayerRules rules;rules.weapon=definition.weapon_definition;
  MultiplayerMatch match(content,0,cache.spawns(),rules);std::array<unsigned,4> ids;
  for(unsigned i=0;i<ids.size();++i)ids[i]=match.join("Binding "+std::to_string(i));
  match.configure_local_views(ids,definition);
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned i=0;i<ids.size();++i) {
    const auto &view=match.first_person_view(i);const auto &unit=match.unit(ids[i]);
    check(view.unit==unit.source_datum && view.hands[0].weapon==unit.weapon.source_datum &&
        view.hands[0].flags==6 && unit.weapon.smg.hand_timer_registered,"actual per-view full binding");
    controls[ids[i]].fire=true;controls[ids[i]].yaw=unit.yaw;
  }
  match.step(controls);
  for(auto id:ids)check(match.unit(id).weapon.smg.hand_timers[0]==9,"actual registered fire timer9");
  controls={};match.step(controls);
  for(auto id:ids)check(match.unit(id).weapon.smg.hand_timers[0]==8,"actual registered timer decay8");
  const auto old_weapon=match.unit(ids[0]).weapon.source_datum;
  match.leave(ids[0]);match.step(controls);
  check(match.first_person_view(0).unit==null_datum && match.first_person_view(0).hands[0].weapon==null_datum,
      "actual left view release");
  for(unsigned i=1;i<ids.size();++i)
    check(match.first_person_view(i).hands[0].weapon==match.unit(ids[i]).weapon.source_datum &&
        match.first_person_view(i).hands[0].weapon!=old_weapon,"other actual views retained");
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)throw FormatError("Supply original map and FP binding fixture");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    const Cache cache(argv[1]);const auto content=cache.load_content(0,true);
    std::map<int,FirstPersonBindingDefinition> definitions;
    std::array<FirstPersonViewBinding,4> views;SmgState smg;
    const auto &weapon=content.weapons.at(cache.slayer_starting_weapon(0));
    check(bool(weapon.smg),"authored single SMG");smg.loaded=60;smg.reserve=120;smg_initial_equip(smg,*weapon.smg);
    const auto lookup=[&](auto,int selector)->const FirstPersonBindingDefinition * {
      const auto found=definitions.find(selector);return found==definitions.end()?nullptr:&found->second;
    };
    if(!content.original_data)throw FormatError("Original SMG direction table is required");
    std::uint32_t shared_seed=SlayerRules{}.random_seed;
    std::ifstream file(argv[2]);unsigned steps=0,timer_phases=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);std::string command,label;row>>command;
      if(command=="D") {
        const int selector=int(word(row));const auto datum=word(row);
        auto definition=cache.load_first_person_binding(datum,selector);
        check(definition.record_present,"authored record");check(definition.arms_model==word(row),"authored arms model");
        check(definition.weapon_model==word(row),"authored weapon model");check(definition.graph==word(row),"authored graph");
        check(definition.idle_index==word(row),"authored idle index");
        for(const auto *nodes:{&definition.arms_nodes,&definition.weapon_nodes,&definition.graph_nodes}) {
          check(nodes->size()==word(row),"authored node count");for(auto n:*nodes)check(n==word(row),"full authored StringID");
        }
        definitions.emplace(selector,std::move(definition));continue;
      }
      row>>label;
      if(command=="I")views={};
      else if(command=="O") {
        FirstPersonBindingInput input;input.player=word(row);input.unit=word(row);
        input.representation=std::bit_cast<std::int32_t>(word(row));for(auto &w:input.weapons)w=word(row);
        observe_first_person_binding(views[0],input,lookup);
      } else if(command=="R")release_first_person_binding(views.at(word(row)));
      else if(command=="L") {const auto datum=word(row);check(std::uint32_t(first_person_weapon_view(views,datum))==word(row),label+" exact lookup");}
      else if(command=="F") {
        const auto ticks=word(row);
        for(unsigned t=0;t<ticks;++t) {
          for(auto timer:smg.hand_timers)check(timer==word(row),label+" before timer");++timer_phases;
          smg_input(smg,true);
          for(auto timer:smg.hand_timers)check(timer==word(row),label+" unit timer");++timer_phases;
          smg.hand_timer_registered=first_person_weapon_view(views,views[0].hands[0].weapon)==0;
          smg_update(smg,*weapon.smg,tick_seconds);
          for(auto timer:smg.hand_timers)check(timer==word(row),label+" weapon timer");++timer_phases;
          const bool emitted=bool(smg_dispatch(smg,*weapon.smg,{1,0,0},shared_seed,
              content.original_data->weapon_directions));
          for(auto timer:smg.hand_timers)check(timer==word(row),label+" postpass timer");++timer_phases;
          check(unsigned(emitted)==word(row),label+" original postpass request count");
        }
      } else if(command!="C")throw FormatError("Unknown source binding command");
      compare(row,views,label);std::string extra;check(!(row>>extra),label+" trailing word");++steps;
    }
    check(steps==14 && timer_phases==8,"complete source steps");
    actual_match(cache,content,definitions.at(2));
    std::cout<<steps<<" original FP binding steps,41 node pairs,"<<timer_phases<<" hand-timer phases and4 actual local views matched\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
