#include "halo2/vista_mainmenu_network.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using nlohmann::json;
namespace {
void require(bool value,const char* message) {
  if(!value)throw std::runtime_error(message);
}
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto byte:bytes) {
    const auto v=std::to_integer<unsigned>(byte);out+=digits[v>>4];out+=digits[v&15];
  }
  return out;
}
void assign(std::span<std::byte> dst,const std::string& value) {
  require(value.size()==dst.size()*2,"Fixture storage length differs");
  for(std::size_t i=0;i<dst.size();++i)
    dst[i]=std::byte(std::stoul(value.substr(i*2,2),nullptr,16));
}
}
int main(int argc,char** argv)try {
  require(argc==2,"Expected original game-state fixture path");
  std::ifstream input(argv[1]);json fixture;input>>fixture;
  halo2::VistaMainmenuGameState state;
  for(std::size_t i=0;i<41;++i) {
    const auto& child=fixture.at("constructor").at(i);
    require(state.child(i).size()==child.at("size").get<std::size_t>(),"Original child size differs");
    require(hex(state.child(i))==child.at("bytes").get<std::string>(),"Original child constructor bytes differ");
  }
  require(hex(state.profile_service())==fixture.at("profile").get<std::string>(),"Original profile-service constructor differs");
  std::size_t count{};
  for(const auto& row:fixture.at("cases")) {
    assign(state.root().first(8),row.at("root_prefix_before").get<std::string>());
    assign(state.root().subspan(0xb0),row.at("root_tail").get<std::string>());
    assign(state.counter(),row.at("counter_before").get<std::string>());
    for(std::size_t i=0;i<41;++i)assign(state.child(i),row.at("children_before").at(i).get<std::string>());
    state.reset();
    require(hex(state.root().first(8))==row.at("root_prefix_after").get<std::string>(),"Original reset root prefix differs");
    require(hex(state.root().subspan(0xb0))==row.at("root_tail").get<std::string>(),"Original reset root suffix changed");
    require(hex(state.counter())==row.at("counter_after").get<std::string>(),"Original slot17 counter reset differs");
    for(std::size_t i=0;i<41;++i)
      require(hex(state.child(i))==row.at("children_after").at(i).get<std::string>(),"Original virtual child reset differs");
    ++count;
  }
  require(count==256,"Original reset corpus count differs");
  std::cout<<"PASS original game-state constructor children=41 virtual resets="<<count<<" full child bytes and slot17 counter\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
