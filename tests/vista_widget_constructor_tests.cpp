#include "halo2/vista_widget_constructor.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& text) {
  if(text.size()%2)throw std::runtime_error("Odd fixture byte extent");
  std::vector<std::byte> result;
  for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return result;
}
std::string hex(std::span<const std::byte> bank) {
  constexpr char digits[]="0123456789abcdef";std::string result;
  for(auto b:bank){const auto n=std::to_integer<unsigned>(b);result+=digits[n>>4];result+=digits[n&15];}
  return result;
}
void run(const Json& row) {
  const auto& input=row.at("input");auto bank=bytes(input.at("initial"));
  const VistaWidgetIdentity id{input.at("identity").get<std::uint32_t>()};
  const auto controller=input.at("controller").get<std::uint32_t>();
  auto snapshots=Json::array();unsigned clocks{};
  auto clock=[&] {
    ++clocks;snapshots.push_back(hex(bank));
    for(const auto& mutation:input.at("clock_mutations")) {
      const auto at=mutation[0].get<std::size_t>();const auto data=bytes(mutation[1]);
      if(at+data.size()>bank.size())throw std::runtime_error("Fixture mutation bounds");
      std::copy(data.begin(),data.end(),bank.begin()+at);
    }
    return input.at("clock").get<std::uint32_t>();
  };
  const std::string owner=input.at("owner");std::uint32_t result{};
  if(owner=="widget")result=vista_widget_construct(bank,id,input.at("kind"),controller,clock).value;
  else if(owner=="text_base")result=vista_widget_text_base_construct(bank,id,controller,clock).value;
  else if(owner=="string")result=vista_widget_string_construct(bank,id.value);
  else if(owner=="text")result=vista_widget_text_construct(bank,id,controller,clock).value;
  else if(owner=="button")result=vista_widget_button_construct(bank,id,input.at("ordinal"),controller,clock).value;
  else throw std::runtime_error("Unknown source constructor");
  check(result==row.at("result").get<std::uint32_t>(),"Source identity return differs");
  check(snapshots==row.at("clock_checkpoints"),"Source cached-clock bank checkpoint differs");
  check(clocks==(owner=="string"?0u:1u),"Independent cached-clock sample count differs");
  check(hex(bank)==row.at("final").get<std::string>(),"Owned or preserved constructor bytes differ");
}
template<class F>void rejected(F&& f,const char* why) {
  bool caught{};try{f();}catch(const FormatError&){caught=true;}check(caught,why);
}
} //namespace
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Usage: vista_widget_constructor_tests fixture.json");
    std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("source_sha256")=="33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109","Source pin differs");
    unsigned cases{};for(const auto& row:fixture.at("cases")){run(row);++cases;}
    std::array<std::byte,0x100> bank;bank.fill(std::byte{0x5a});const auto before=bank;
    unsigned clocks{};const auto clock=[&]{++clocks;return 1u;};
    rejected([&]{vista_widget_construct(std::span(bank).first(0x6f),{1},0,0,clock);},"Truncated base widget admitted");
    rejected([&]{vista_widget_text_base_construct(std::span(bank).first(0x74),{1},0,clock);},"Truncated text base admitted");
    rejected([&]{vista_widget_string_construct(std::span(bank).first(0x43),1);},"Truncated string owner admitted");
    rejected([&]{vista_widget_text_construct(std::span(bank).first(0xbd),{1},0,clock);},"Truncated text owner admitted");
    rejected([&]{vista_widget_button_construct(std::span(bank).first(0xff),{1},0,0,clock);},"Truncated button owner admitted");
    rejected([&]{vista_widget_construct(bank,{0},0,0,clock);},"NULL source identity admitted");
    rejected([&]{vista_widget_construct(bank,{0xffffff91},0,0,clock);},"Wrapping consumed source identity admitted");
    rejected([&]{vista_widget_construct(bank,{1},0,0,{});},"Missing cached-clock owner admitted");
    check(clocks==0 && bank==before,"Rejected constructor changed retained bytes or sampled clock");
    std::cout<<"Vista widget constructors: "<<cases<<" original cases, "<<checks<<" checks PASS\n";return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
