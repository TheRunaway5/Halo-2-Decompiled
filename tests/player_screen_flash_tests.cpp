#include "halo2/player_screen_flash.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
float f(std::uint32_t word){return std::bit_cast<float>(word);}
bool same(float a,float b){return std::abs(a-b)<.0000002f;}
}
int main(int argc,char **argv)try {
  if(argc!=4)return 2;
  std::ifstream report(argv[3]);nlohmann::json original;report>>original;
#ifdef HALO2_FLASH_KERNEL_ONLY
  auto tables=std::make_shared<ShaderFunctionTables>();
  for(unsigned curve:{1,2})for(unsigned i=0;i<1024;++i)
    tables->transition[curve][i]=original.at("transition_tables").at(std::to_string(curve)).at(i).get<std::uint8_t>();
#else
  const Cache cache(argv[1]);const auto tables=cache.shader_function_tables();
#endif
  std::ifstream fixture(argv[2]);check(bool(fixture),"Missing original flash samples");
  std::string line;unsigned samples{},tags{},flashes{};
  while(std::getline(fixture,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,14> words{};
    for(auto &word:words)check(bool(row>>std::hex>>word),"Malformed original flash sample");
    const PlayerScreenResponse response{std::int16_t(words[3]),{6,2,f(words[4]),5,f(words[5]),{.7f,.2f,.3f,.4f}}};
    PlayerScreenFlashPlayer player;
    player.play(std::span(&response,1),f(words[0])>0,words[1]!=0,f(words[2]),*tables);
    player.advance(words[6]);const auto actual=player.evaluate(*tables);
    check(actual.type==std::int32_t(words[7]),"TU1.5 damage flash is missing or has the wrong type/lifetime");
    check(same(actual.intensity,f(words[8])),"TU1.5 damage flash intensity/fade differs");
    for(unsigned c=0;c<4;++c)check(same(actual.color[c],f(words[9+c])),"TU1.5 shield/body flash color differs");
    if(player_screen_flash_argb(actual)!=words[13]) {
      std::cerr<<"sample "<<samples<<" packed "<<std::hex<<player_screen_flash_argb(actual)<<" expected "<<words[13]<<std::dec<<'\n';
      throw FormatError("TU1.5 flash byte quantization differs");
    }
    ++samples;
  }
  for(const auto &row:original.at("admission")) {
    PlayerScreenFlashPlayer player;
    player.submit({1,row.at("old_priority").get<std::int16_t>(),.8f,0,.2f,{1,1,0,0}},1);
    player.advance(24-row.at("timer").get<unsigned>());
    player.submit({1,row.at("priority").get<std::int16_t>(),row.at("duration").get<float>(),0,.6f,{1,0,.6f,.8f}},.5f);
    const auto &expected=row.at("result");const auto actual=player.evaluate(*tables);
    const float duration=f(expected.at(1).get<std::uint32_t>());
    const float intensity=f(expected.at(3).get<std::uint32_t>());
    const float expected_amount=duration>0?float(row.at("remaining").get<int>())/duration*intensity:intensity;
    check(same(actual.intensity,expected_amount),"TU1.5 repeated-flash priority or truncation differs");
  }
  for(const auto &item:original.at("selection")) {
    std::vector<PlayerScreenResponse> rows;
    for(const auto &row:item.at("rows"))rows.push_back({row[0].get<std::int16_t>(),
        {6,2,row[1].get<float>(),5,row[2].get<float>(),{.7f,.2f,.3f,.4f}}});
    PlayerScreenFlashPlayer player;player.play(rows,item.at("shield").get<float>()>0,false,1,*tables);
    const auto actual=player.evaluate(*tables);const auto &expected=item.at("result");
    check(actual.type==expected[0].get<int>() && same(actual.intensity,f(expected[1].get<unsigned>())) &&
        player_screen_flash_argb(actual)==expected[6].get<unsigned>(),"First admitted response/threshold fallback differs from TU1.5");
  }
  // Real shared response blocks, including the authored NONE flash fields
  // which TU1.5 can override after its duration/intensity admission gates.
#ifndef HALO2_FLASH_KERNEL_ONLY
  for(const auto &tag:cache.select_tags({.group=tag_groups::damage_effect,.require_address=true})) {
    const auto rows=load_player_screen_responses(cache,tag.datum);++tags;
    for(bool shielded:{false,true}) {
      PlayerScreenFlashPlayer player;player.play(rows,shielded,false,1,*tables);
      flashes+=player.evaluate(*tables).intensity>0;
    }
  }
  check(samples==1320 && tags>0 && flashes>0,"Missing original damage-flash controls");
#else
  check(samples==1320,"Missing original damage-flash controls");
#endif
  std::cout<<"PASS "<<samples<<" TU1.5 samples, "<<original.at("admission").size()<<" priority cases, "<<tags<<" authored tags / "<<flashes<<" active flash selections\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
