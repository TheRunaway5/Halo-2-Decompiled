#include "halo2/source_model_transparent_order.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
using namespace halo2;
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
int main(int argc,char**argv)try {
  check(argc==3,"Supply captured complete MCC queue and main-unit fixtures");
  const std::string owner="MCC 00a_introduction f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8";
  std::ifstream commands(argv[1]),bank(argv[2]);std::string line;
  check(bool(std::getline(commands,line))&&line==owner,"Wrong command fixture owner");
  check(bool(std::getline(bank,line))&&line==owner,"Wrong main-unit fixture owner");
  std::vector<SourceTransparentCommand> queue;
  while(commands>>line) {
    check(line.size()==224,"Incomplete command record");std::array<std::byte,112> raw;
    for(unsigned i=0;i<112;++i)raw[i]=std::byte(std::stoul(line.substr(i*2,2),nullptr,16));
    std::memcpy(&queue.emplace_back(),raw.data(),112);
  }
  std::vector<SourceLiveModelBaseDrawTu15> units;
  while(std::getline(bank,line)) {
    std::istringstream row(line);auto &u=units.emplace_back();
    check(bool(row>>u.ordinal>>u.object>>u.definition),"Incomplete main-unit identity");
    for(auto &word:u.unit.words)check(bool(row>>word),"Incomplete source main-unit words");
  }
  check(queue.size()==12 && units.size()>6,"Incomplete captured main frame");
  const auto &city=*std::ranges::find(units,6u,&SourceLiveModelBaseDrawTu15::ordinal);
  const auto &planet=*std::ranges::find(units,5u,&SourceLiveModelBaseDrawTu15::ordinal);
  // Actual v785 GPU submission slots: city14, then planet15. Original
  // TU1.5 must submit the planet before the city for these non-writing mattes.
  const std::array draws{
    SourceModelTransparentDraw{city.object,city.definition,(city.unit.words[3]>>9)&511u,0,0xe28a0114u,14},
    SourceModelTransparentDraw{planet.object,planet.definition,(planet.unit.words[3]>>9)&511u,0,0xe24e00d8u,15}};
  const auto actual=source_model_transparent_order_tu15(queue,units,draws,{});
  if(actual){std::cout<<"Matte draw slots:";for(auto i:*actual)std::cout<<' '<<i;std::cout<<'\n';}
  check(actual && *actual==std::vector<std::size_t>{15,14},"Matte consumer draws High Charity before the planet instead of following the complete TU1.5 queue");
  const auto rejected=[&](auto commands,auto bases,auto bindings) {
    check(!source_model_transparent_order_tu15(commands,bases,bindings,{}),"Incomplete or changed source join applied a partial matte reorder");
  };
  auto changed=queue;
  const auto city_command=std::ranges::find_if(changed,[&](const auto& c){std::uint32_t shader;std::memcpy(&shader,c.payload.data(),4);return shader==draws[0].shader;});
  check(city_command!=changed.end(),"Captured city command missing");
  const auto city_index=std::size_t(city_command-changed.begin());
  changed[city_index].payload[0]^=1;rejected(changed,units,draws);
  changed=queue;changed[city_index].payload[4]^=1;rejected(changed,units,draws);
  changed=queue;changed[city_index].payload[7]=0;rejected(changed,units,draws);
  changed=queue;changed[city_index].callback=0x41011;rejected(changed,units,draws);
  changed=queue;changed.push_back(changed[city_index]);rejected(changed,units,draws);
  auto bases=units;bases.push_back(units.front());rejected(queue,bases,draws);
  bases=units;std::erase_if(bases,[](const auto& u){return u.ordinal==6;});rejected(queue,bases,draws);
  auto bindings=draws;bindings[0].definition^=0x10000;rejected(queue,units,bindings);
  bindings=draws;bindings[0].section^=1;rejected(queue,units,bindings);
  bindings=draws;bindings[1].draw=bindings[0].draw;rejected(queue,units,bindings);
  bindings=draws;bindings[1].object=bindings[0].object;rejected(queue,units,bindings);
  check(source_model_transparent_order_tu15(queue,units,std::span<const SourceModelTransparentDraw>{},{})==std::vector<std::size_t>{},"Empty model selection acquired another owner");
  std::cout<<"PASS actual MCC complete queue orders planet before High Charity\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
