#include "halo2/vista_player_birth.hpp"
#include "halo2/vista_slayer_publication.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}
  check(rejected,"Unsupported birth/activation input was silently accepted");}
std::vector<std::byte> bytes(const std::string& hex){check(hex.size()%2==0,"Invalid original hex");
  std::vector<std::byte> result;for(std::size_t i=0;i<hex.size();i+=2)
    result.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
  return result;}
std::uint16_t read16(const std::vector<unsigned char>& b,unsigned at){return std::uint16_t(b[at]|(b[at+1]<<8));}
}
int main(int argc,char** argv){try{
  check(argc==3,"Supply original birth fixture and original spawn-flags directory");
  std::ifstream file(argv[1]);if(!file)return 77;nlohmann::json fixture;file>>fixture;
  check(fixture.at("total_cases")==263235,"Unexpected original executable corpus");
  unsigned birth{},rows{},spawn{},rising{};
  for(const auto& item:fixture.at("birth_cases")) {
    const auto& input=item.at("input");
    const auto actual=item.at("source")=="pc"?vista_player_birth_flags(input.at("new_byte").get<std::uint8_t>(),
        input.at("left").get<std::uint8_t>()):native_player_birth_flags(input.at("new_byte").get<std::uint8_t>(),
        input.at("left").get<std::uint8_t>());
    check(actual==item.at("born_flags"),"Original birth flag word differs");++birth;
  }
  for(const auto& item:fixture.at("world_loader_cases")) {
    auto options=bytes(item.at("options_hex"));const auto machine=bytes(item.at("published_machine_hex"));
    const auto datum=std::uint32_t(std::stoul(item.at("allocated_datum").get<std::string>(),nullptr,16));
    const auto slot=item.at("input").at("slot").get<unsigned>();
    auto record=vista_player_birth_from_initialized_options(options,slot,datum);
    const auto owner=vista_player_local_identity_from_initialized_options(options,
        std::span<const std::byte,6>(machine.data(),6));
    check(record.flags==item.at("born_flags") && record.slot==slot && record.native_datum==datum &&
        !record.complete_player_record && !owner.complete_simulation_metadata &&
        owner.world_kind==item.at("simulation_kind") && owner.local_index==0,"Original world-loader birth projection differs");
    const auto at=0x448+slot*0xd4;
    check(std::equal(record.identity.begin(),record.identity.end(),options.begin()+at+0xe) &&
        std::equal(record.machine.begin(),record.machine.end(),options.begin()+at+8),"PC birth identity bytes differ");
    bool config=false;
    for(const auto& call:item.at("calls"))if(call.at("call")=="configuration_application_external") {
      const auto expected=bytes(call.at("profile_hex"));
      check(expected.size()==record.configuration.size() &&
          std::equal(expected.begin(),expected.end(),record.configuration.begin()),"Actual PC configuration argument differs");config=true;
    }
    check(config,"Original profile application service was not reached");
    check(record.creation_user==std::uint16_t(item.at("input").at("user").get<int>()) &&
        record.creation_controller==std::uint32_t(item.at("input").at("controller").get<int>()),"Actual user/controller differs");
    if(vista_player_activation_query_admitted(std::to_integer<std::uint8_t>(options[4]),record.flags)) {
      const auto update=vista_player_activation_after_query(record.flags,vista_player_local_activation_query(owner,record));
      check(update.flags==item.at("first_flags") && update.call_count==2,"Original first activation differs");++rising;
    }
    auto remote=record;remote.machine[0]^=std::byte{1};rejects([&]{(void)vista_player_local_activation_query(owner,remote);});
    rejects([&]{(void)vista_player_birth_from_initialized_options(options,slot,datum^0x10000u^1u);});
    auto invalid=options;invalid[0x43e]=std::byte{};
    rejects([&]{(void)vista_player_local_identity_from_initialized_options(invalid,std::span<const std::byte,6>(machine.data(),6));});
    invalid=options;invalid[4]=std::byte{4};rejects([&]{(void)vista_player_birth_from_initialized_options(invalid,slot,datum);});
    ++rows;
  }
  for(const auto& name:{"pc","retail","tu11","u5"}) {
    const auto path=std::string(argv[2])+"/"+fixture.at("spawn_completion").at(name).at("path").get<std::string>();
    std::ifstream input(path,std::ios::binary);check(bool(input),"Missing original spawn result bank");
    std::vector<unsigned char> original((std::istreambuf_iterator<char>(input)),{});
    check(original.size()==65536*2,"Truncated original spawn flags");
    for(unsigned flags=0;flags<65536;++flags) {
      const auto actual=std::string(name)=="pc"?vista_player_spawn_completed_flags(std::uint16_t(flags)):
          native_player_spawn_completed_flags(std::uint16_t(flags));
      check(actual==read16(original,flags*2),"Original successful-spawn flag continuation differs");++spawn;
    }
  }
  check(birth==1008 && rows==72 && rising==36 && spawn==262144,"Incomplete original birth/spawn coverage");
  std::cout<<birth<<" original birth flags, "<<rows<<" original world-loader projections and "<<spawn<<" original successful-spawn flags matched\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
