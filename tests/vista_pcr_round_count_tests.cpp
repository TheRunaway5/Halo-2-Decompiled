#include "halo2/vista_pcr_publication.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
auto hex(const SourceGameVariant &v){constexpr char digits[]="0123456789abcdef";std::string result;
 for(auto value:v.bytes){result+=digits[value>>4];result+=digits[value&15];}return result;}
}
int main(int argc,char **argv)try{
 if(argc!=2)return 77;std::ifstream input(argv[1]);if(!input)return 77;
 const auto fixture=nlohmann::json::parse(input);
 check(fixture.at("schema")=="H2_PC_PCR_ROUND_COUNT_ORIGINAL_V1" &&
       fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109",
       "Wrong original PC round-count authority");
 const auto &descriptor=source_game_variant_descriptors()[0];
 check(fixture.at("descriptor")==nlohmann::json::array({descriptor.kind,descriptor.offset,descriptor.operand}),
       "Original round-count descriptor differs");
 unsigned count{};
 for(const auto &row:fixture.at("cases")){
  auto variant=source_game_variant_default(0);
  if(row.at("teams").get<bool>()){source_game_variant_set(variant,91,1);source_game_variant_set(variant,10,8);}
  check(hex(variant)==row.at("before").get<std::string>(),"Actual original team/time baseline differs");
  source_game_variant_set(variant,0,row.at("value").get<std::int32_t>());
  check(hex(variant)==row.at("after").get<std::string>() && source_game_variant_get(variant,0)==row.at("read").get<std::int32_t>(),
        "Complete native round-count write/read differs from original PC");
  const auto bridge=vista_pcr_slayer_variant(variant);
  for(unsigned i=0;i<variant.bytes.size();++i)check(bridge.bytes[i]==std::byte(variant.bytes[i]),
       "PCR changed an original round-count or another source field");
  ++count;
 }
 auto forged=source_game_variant_default(0);source_game_variant_set(forged,0,1);forged.bytes[0x100]^=1;
 bool refused{};try{(void)vista_pcr_slayer_variant(forged);}catch(const std::invalid_argument &){refused=true;}
 check(refused && count==24,"Round-count admission weakened unrelated field checks");
 std::cout<<"PASS24 complete original PC/native round-count writes and PCR buffers; unverified field still refused\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
