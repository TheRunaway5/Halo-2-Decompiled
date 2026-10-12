#include "halo2/source_player_profile.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;using nlohmann::json;
namespace {
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
template<std::size_t N>std::array<std::uint8_t,N> record(const std::string& text) {
  check(text.size()==2*N,"Source profile record width differs");
  std::array<std::uint8_t,N> result;
  for(unsigned i=0;i<N;++i)result[i]=std::uint8_t(std::stoul(text.substr(i*2,2),nullptr,16));
  return result;
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply the original player-profile fixture");
    std::ifstream stream(argv[1]);json fixture;stream>>fixture;
    check(fixture.at("instruction_patches").empty()&&fixture.at("supplied_returns").empty(),
        "Profile oracle supplied a dependency");
    unsigned count=0;
    for(const auto& [name,data]:fixture.at("versions").items()) {
      check(source_default_full_player_profile()==record<480>(data.at("full").at("output_hex")),name+" full profile constructor");++count;
      check(source_default_player_configuration()==record<144>(data.at("configuration").at("output_hex")),name+" configuration constructor");++count;
      unsigned index=0;
      for(const auto& input:fixture.at("inputs").at("identity")) {
        const auto expected=record<12>(data.at("identity").at(index).at("output_hex"));
        check(source_offline_player_identity(record<6>(input.at("machine_hex")),input.at("user"))==expected,
            name+" identity "+std::to_string(index));++index;++count;
      }
      index=0;
      for(const auto& input:fixture.at("inputs").at("conversion")) {
        SourceOfflineLocalProfile local;local.identity=record<12>(input.at("identity_hex"));
        local.display_name=input.at("name").get<std::array<std::uint16_t,32>>();
        local.appearance=record<16>(input.at("appearance_hex"));local.account=record<12>(input.at("account_hex"));
        local.account_data=record<32>(input.at("account_data_hex"));local.committed_team=input.at("committed_team");
        local.field_200=input.at("field_200");local.field_46c=input.at("field_46c");local.field_204=input.at("field_204");
        local.input_member=input.at("input_member");local.input_member_present=input.at("input_member_present");
        local.input_configuration=record<144>(input.at("input_configuration_hex"));
        SourcePlayerIdentity identity;identity.fill(0xa5);SourcePlayerConfiguration configuration;configuration.fill(0xa5);
        source_convert_offline_local_profile(local,input.at("identity_output").get<bool>() ? &identity : nullptr,
            input.at("configuration_output").get<bool>() ? &configuration : nullptr,
            name=="u5" && input.at("override_four").get<bool>());
        const auto& expected=data.at("conversion").at(index);
        check(identity==record<12>(expected.at("identity_hex")) && configuration==record<144>(expected.at("output_hex")),
            name+" local conversion "+std::to_string(index));++index;++count;
      }
      index=0;
      for(const auto& input:fixture.at("inputs").at("sanitize")) {
        auto profile=record<144>(input.at("raw_hex"));
        const auto scenario=input.at("scenario").get<std::vector<std::int16_t>>();
        SourcePlayerConfigurationContext context;
        context.game_type=input.at("game_type");context.representation_count=input.at("representation_count");
        context.scenario_representations=scenario;context.selected_engine=input.at("selected_engine");
        context.team_mode=input.at("team_mode");context.team_mask=input.at("team_mask");
        context.representation_policy=name=="retail" ? SourceProfileRepresentationPolicy::Retain
            : SourceProfileRepresentationPolicy::MultiplayerMapping;
        source_sanitize_player_configuration(profile,context);
        check(profile==record<144>(data.at("sanitize").at(index).at("output_hex")),
            name+" sanitize "+std::to_string(index));++index;++count;
      }
    }
    check(count==9606,"Original complete profile call coverage changed");
    std::cout<<count<<" complete original identity/profile/sanitizer records match byte for byte\n";
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
