#include "halo2/multiplayer_world.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/math.hpp"
#include "halo2/navigation.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool condition,const std::string &message){if(!condition)throw FormatError(message);}
std::filesystem::path maps(const std::filesystem::path &root) {
  std::ifstream registry(root/"assets/mcc-source.tsv");std::string schema,line;
  std::getline(registry,schema);std::getline(registry,line);
  if(schema!="H2_MCC_ASSET_SOURCE_V1" || !line.starts_with("cache\t"))return {};
  return root/std::filesystem::path(line.substr(6))/"halo2/h2_maps_win64_dx11";
}
}
int main(int argc,char **argv)try {
  check(argc==2 || (argc==4 && std::string_view(argv[2])=="--inspect"),"Supply root and optional --inspect MAP");const std::filesystem::path root=argv[1],directory=maps(root);
  if(directory.empty() || !std::filesystem::is_regular_file(directory/"turf.map"))return 77;
  const auto variants=load_custom_slayer_variants(root/"data/tags/variants");
  const auto variant=std::ranges::find_if(variants,bomb_defusal_variant);check(variant!=variants.end(),"Missing variant");
  nlohmann::json catalog;std::ifstream(root/"data/tags/bomb-defusal-sites-v1.json")>>catalog;
  unsigned checked=0,planted=0;
  for(const auto &row:catalog.at("maps")) {
    const auto scenario=row.at("scenario").get<std::string>();
    const auto leaf=scenario.substr(scenario.find_last_of('\\')+1);
    if(argc==4 && leaf!=argv[3])continue;
    MultiplayerOptions options;options.map=directory/(leaf+".map");options.shared_maps=directory;
    options.variant=*variant;options.local_players=2;options.selected_teams={0,1};
    if(argc==4)for(unsigned site=0;site<2;++site)options.variant.values.set(native_defusal_site_parameter+site,row.at("sites")[site].at("marker").at("index").get<int>());
    MultiplayerPreparedWorld world(options);auto &match=world.match;
    if(argc==4) {
      std::cout<<"Capsule "<<world.content.player.height<<' '<<world.content.player.radius<<'\n';
      for(const auto &marker:world.content.netgame_flags)if(marker.type==10 || marker.type==3) {
        const auto floor=match.simulation().collision().raycast(marker.position+Vec3{0,0,.25f},{0,0,-1},5.f);
        std::cout<<marker.source_index<<" position "<<marker.position.x<<' '<<marker.position.y<<' '<<marker.position.z;
        if(floor)std::cout<<" floor "<<floor->position.z<<" normal "<<floor->normal.z<<" clear "<<
            !match.simulation().collision().overlaps(floor->position+Vec3{0,0,.03f},world.content.player.height,world.content.player.radius);
        std::cout<<'\n';
      }
      return 0;
    }
    check(world.cache->is_mcc() && world.cache->scenario()==scenario,"Wrong imported scenario owner");
    check(world.rules.defusal_layout && match.state().defusal->sites.size()==2,"World did not select its A/B layout");
    for(unsigned n=0;n<7;++n)for(unsigned team=0;team<2;++team)match.join("Remote "+std::to_string(n),std::int8_t(team));
    std::array<Controls,multiplayer_capacity> controls{};
    for(unsigned n=0;n<200 && match.state().defusal->phase!=BombDefusalPhase::live;++n)match.step(controls);
    auto state=match.state();check(state.defusal->phase==BombDefusalPhase::live,"Map did not start a live round");
    const NavigationWorld navigation(world.content.navigation);
    const auto route=navigation.route(state.defusal->sites[0].position,state.defusal->sites[1].position);
    float route_length=0;auto prior=state.defusal->sites[0].position;
    for(const auto point:route){route_length+=length(point-prior);prior=point;}
    std::cout<<leaf<<": authored navigation route "<<(route.empty()?"unavailable; requires played-route check":std::to_string(route_length)+" game units; not a measured player time")<<std::endl;
    for(unsigned i=0;i<16;++i) {
      check(state.players[i].connected && !state.players[i].dead,"Opening map spawn is not alive");
      for(unsigned j=0;j<i;++j)check(length(state.players[i].position-state.players[j].position)>.45f,"Opening map spawns overlap");
    }
    for(unsigned site=0;site<2;++site) {
      state=match.state();const auto carrier=state.defusal->carrier&65535u;
      auto &unit=const_cast<Unit&>(match.unit(carrier));unit.body.position=state.defusal->sites[site].position;
      unit.body.velocity={};unit.body.grounded=true;controls[carrier].pickup=true;
      for(unsigned n=0;n<world.rules.defusal->plant_ticks;++n)match.step(controls);
      controls={};state=match.state();
      check(state.defusal->phase==BombDefusalPhase::planted && state.defusal->site==int(site),leaf+": native held Use failed to plant");
      ++planted;
      if(!site) {
        auto &defender=const_cast<Unit&>(match.unit(1));defender.body.position=state.defusal->bomb_position;
        defender.body.velocity={};defender.body.grounded=true;controls[1].pickup=true;
        for(unsigned n=0;n<world.rules.defusal->defuse_ticks;++n)match.step(controls);
        controls={};check(match.state().defusal->outcome==BombDefusalOutcome::defused,"Native map defuse failed");
        for(unsigned n=0;n<400 && match.state().defusal->phase!=BombDefusalPhase::live;++n)match.step(controls);
        check(match.state().defusal->phase==BombDefusalPhase::live,"Map round reset failed");
      }
    }
    ++checked;std::cout<<row.at("name").get<std::string>()<<": 16 native opening spawns and both A/B plants passed"<<std::endl;
  }
  check(checked==11 && planted==22,"Incomplete recommended map coverage");
  std::cout<<"PASS imported MCC geometry/native interaction; teleport fixtures do not establish played-route balance\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
