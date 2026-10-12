#include "halo2/multiplayer.hpp"
#include <iostream>
#include <iomanip>
#include <set>
#include <algorithm>
using namespace halo2;
int main(int argc,char **argv) {
 try {
  if(argc!=4)return 2;
  Cache cache(argv[1],{},OriginalData::load_xbe(argv[2]));
  auto content=cache.load_content(0,true);SlayerRules rules;
  rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Script host probe");
  const auto &sim=match.simulation();
  const auto expected=std::count_if(content.scripts.scripts.begin(),content.scripts.scripts.end(),[](const auto &root){return root.type==0 || root.type==2;});
  if(sim.scripts().running()!=expected)throw FormatError("Ordinary match did not activate authored roots");
  std::cout<<"AUTOMATIC\t"<<sim.scripts().running()<<'\t'<<sim.status()<<'\n';
  for(const auto &v:content.trigger_volumes)
   std::cout<<"VOLUME\t"<<std::quoted(v.name)<<'\t'<<v.object_name<<'\n';
  std::set<std::uint32_t> previous;
  for(const auto &o:sim.scene().objects())if(o.handle!=null_datum)previous.insert(o.handle);
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned i=0;i<unsigned(std::stoul(argv[3]));++i) {
   match.step(controls);std::set<std::uint32_t> current;
   for(const auto &o:sim.scene().objects())if(o.handle!=null_datum)current.insert(o.handle);
   if(current!=previous) {
    std::cout<<"LIFETIME\t"<<sim.ticks();
    for(auto h:previous)if(!current.contains(h))std::cout<<"\tremove="<<std::hex<<h<<std::dec;
    for(auto h:current)if(!previous.contains(h))std::cout<<"\tcreate="<<std::hex<<h<<std::dec;
    std::cout<<'\n';previous=std::move(current);
   }
   if(!sim.scripts().diagnostics().empty())break;
  }
  for(const auto &error:sim.scripts().diagnostics())std::cout<<"FAULT\t"<<std::quoted(error)<<'\n';
  std::cout<<"FINAL\t"<<sim.status()<<'\n';return sim.scripts().diagnostics().empty()?0:1;
 }catch(const std::exception &e){std::cout<<"EXCEPTION\t"<<std::quoted(e.what())<<'\n';return 1;}
}
