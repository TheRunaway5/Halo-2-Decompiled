#include "halo2/multiplayer_world.hpp"
#include "halo2/source_game_variant.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
int main(int argc,char **argv)try{
  if(argc!=4)throw FormatError("Supply MCC_MAP MCC_MAPS REPORT");
  if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_directory(argv[2]))return 77;
  nlohmann::json report{{"passed",false},{"cases",nlohmann::json::array()},
      {"scope","Actual prepared-world admission operands before any peer snapshot; no gameplay or compatibility result supplied"}};
  bool equal=true;
  for(bool declared_spawn:{true,false}){
    MultiplayerOptions host_options;host_options.map=argv[1];host_options.shared_maps=argv[2];
    host_options.local_players=1;host_options.source_object_world=true;host_options.mute=true;
    host_options.starting_weapon=8;host_options.session_random_seed=0x22de570u;
    host_options.variant.values.set(2,0);host_options.variant.values.set(10,0);
    Cache cache(host_options.map,host_options.shared_maps);
    if(!cache.is_mcc())throw FormatError("Admission regression requires actual MCC content");
    if(declared_spawn){
      if(cache.spawns().empty())throw FormatError("Admission scene requires an actual authored start");
      host_options.native_spawn_inputs={cache.spawns().front()};
    }
    MultiplayerPreparedWorld host(host_options);
    auto client_options=host_options;client_options.source_object_world=false;
    client_options.native_spawn_inputs.clear();client_options.join="127.0.0.1:9";
    client_options.starting_weapon=0;client_options.variant.values.set(89,8);
    MultiplayerPreparedWorld client(client_options);
    const bool same=host.network_starting_weapon_identity()==client.network_starting_weapon_identity();equal&=same;
    const auto identity=client.network_starting_weapon_identity();
    const auto presentation=client.rules.weapon;
    client.rules.weapon=0x12345678u;
    const bool presentation_independent=client.network_starting_weapon_identity()==identity;
    client.rules.weapon=presentation;
    auto other_options=client_options;other_options.variant.values.set(89,3);
    MultiplayerPreparedWorld other(other_options);
    const bool other_choice_rejected=other.network_starting_weapon_identity()!=identity;
    equal&=presentation_independent && other_choice_rejected;
    report["cases"].push_back({{"declared_host_spawn",declared_spawn},
        {"host_identity_weapon",host.network_starting_weapon_identity()},
        {"client_identity_weapon",client.network_starting_weapon_identity()},
        {"host_presentation_weapon",host.rules.weapon},{"client_presentation_weapon",client.rules.weapon},
        {"host_requires_source_engine",host.rules.requires_source_engine},
        {"client_requires_source_engine",client.rules.requires_source_engine},
        {"host_source_engine_owned",host.match.source_engine_owned()},
        {"client_source_engine_owned",client.match.source_engine_owned()},
        {"host_choice",host.rules.starting_weapon},{"client_choice",client.rules.starting_weapon},
        {"presentation_independent",presentation_independent},{"other_choice_distinct",other_choice_rejected},
        {"equal",same}});
  }
  report["passed"]=equal;
  std::ofstream(argv[3])<<report.dump(2)<<'\n';
  std::cout<<report.dump()<<'\n';return equal?0:1;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
