#include "halo2/damage_feedback.hpp"
#include "halo2/damage_feedback_names.hpp"
#include "halo2/cache.hpp"
#include "halo2/shader_animation.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if(!(row>>value))throw std::runtime_error("Missing original feedback field");
  return std::stoul(value,nullptr,16);
}
float real(std::istringstream &row) {return std::bit_cast<float>(word(row));}
std::int8_t timer(std::istringstream &row) {
  return std::bit_cast<std::int8_t>(std::uint8_t(word(row)));
}
RecentDamageState recent(std::istringstream &row) {
  const float visible=real(row),accumulated=real(row);
  return {visible,accumulated,timer(row)};
}
DamageFeedbackState state(std::istringstream &row) {
  const auto shield=recent(row),body=recent(row);
  return {shield,body};
}
bool same(const RecentDamageState &a,const RecentDamageState &b) {
  return std::bit_cast<std::uint32_t>(a.visible)==std::bit_cast<std::uint32_t>(b.visible) &&
    std::bit_cast<std::uint32_t>(a.accumulated)==std::bit_cast<std::uint32_t>(b.accumulated) &&
    a.ticks==b.ticks;
}
DamageFadeDefinition definition(std::istringstream &row) {
  return {real(row),real(row),real(row),real(row)};
}
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
}
int main(int argc,char **argv) {
  try {
    if(argc!=4)throw std::runtime_error("Supply original feedback fixtures, map and authored input fixtures");
    for(int n=1;n<4;++n)if(!std::filesystem::exists(argv[n]))return 77;
    std::ifstream file(argv[1]);
    unsigned reports=0,fades=0,updates=0,inputs=0,line_number=0;
    for(std::string line;std::getline(file,line);) {
      ++line_number;
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);char kind;row>>kind;
      const auto context="Original feedback row "+std::to_string(line_number);
      if(kind=='R') {
        auto actual=state(row);
        const auto body=real(row),shield=real(row);
        const auto flags=word(row),special=word(row);
        const auto expected=state(row);
        report_damage_feedback(actual,body,shield,flags,special);
        check(same(actual.shield,expected.shield)&&same(actual.body,expected.body),context+" report");
        ++reports;
      } else if(kind=='F' || kind=='U') {
        DamageFeedbackState actual;
        if(kind=='F')actual.shield=recent(row);else actual=state(row);
        const auto rate=std::int16_t(word(row)),custom=std::int16_t(word(row));
        auto settings=definition(row);if(!custom)settings={};
        if(kind=='F') {
          const auto reset=word(row);const auto expected=recent(row);const bool returned=word(row);
          const bool result=fade_recent_damage(actual.shield,rate,reset,settings);
          check(same(actual.shield,expected)&&result==returned,context+" fade");
          ++fades;
        } else {
          const auto dead=word(row),depleted=word(row),special=word(row);
          const auto expected=state(row);const bool returned=word(row);
          const bool result=update_damage_feedback(actual,rate,dead,depleted,special,settings);
          check(same(actual.shield,expected.shield)&&same(actual.body,expected.body)&&result==returned,
              context+" update");
          ++updates;
        }
      } else if(kind=='I') {
        const auto before=state(row);const auto body=real(row),shield=real(row);
        const auto dead=word(row),name=word(row),found=word(row),value=word(row),active=word(row);
        const auto actual=damage_feedback_input(before,body,shield,dead,name);
        check(bool(actual)==bool(found),context+" input admission");
        if(actual)check(std::bit_cast<std::uint32_t>(actual->value)==value&&actual->active==bool(active),
            context+" input value");
        ++inputs;
      } else throw std::runtime_error(context+" unknown kind");
      std::string extra;check(!(row>>extra),context+" trailing fields");
    }
    check(reports==1600&&fades==2400&&updates==1600&&inputs==1400,"Incomplete original feedback suite");
    Cache cache(argv[2]);
    const auto names=DamageFeedbackNames::from_cache(cache);
    std::map<std::uint32_t,std::vector<ObjectFunction>> functions;
    const ShaderFunctionTables tables{}; // Actual type-zero identities read no lookup table.
    unsigned authored=0;
    std::ifstream aliases(argv[3]);
    for(std::string line;std::getline(aliases,line);) {
      if(line.empty()||line[0]=='#')continue;
      std::istringstream row(line);
      const auto datum=word(row);const auto visible=real(row);
      const auto name=word(row),active=word(row),value=word(row);
      auto [at,created]=functions.try_emplace(datum);
      if(created)at->second=cache.load_object_functions(datum);
      DamageFeedbackState state;state.shield.visible=visible;
      const auto actual=evaluate_object_function(at->second,tables,name,0,{},
          [&](std::uint32_t input)->std::optional<FunctionValue> {
            if(const auto resolved=damage_feedback_input(state,1,1,false,input))
              return FunctionValue{resolved->value,resolved->active};
            return {};
          });
      check(std::bit_cast<std::uint32_t>(actual.value)==value&&actual.enabled==bool(active),
          "Original authored shield function differs at case "+std::to_string(authored));
      const auto shader=damage_feedback_shader_input(state,1,1,false,at->second,tables,name,0);
      check(std::bit_cast<std::uint32_t>(shader)==(active?value:0u),
          "Live damage shader input differs from original authored lookup at case "+std::to_string(authored));
      const auto bound=damage_feedback_shader_input(state,1,1,false,at->second,tables,name,0,names);
      check(std::bit_cast<std::uint32_t>(bound)==(active?value:0u),
          "Cache-bound damage shader input differs from original authored lookup at case "+std::to_string(authored));
      std::string extra;check(!(row>>extra),"Trailing authored shield function field");++authored;
    }
    check(authored==132&&functions.size()==3,"Incomplete authored shield alias suite");
    std::cout<<reports<<" reports, "<<fades<<" fades, "<<updates<<" update tails and "
             <<inputs<<" shader inputs plus "<<authored
             <<" actual authored shield aliases matched original instructions bit for bit\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
