#include "halo2/vista_lobby_countdown.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const std::string& message) {
  if(!ok)throw std::runtime_error(message);
}
std::uint32_t word(const std::string& text) {
  std::size_t used{};
  const auto value=std::stoull(text,&used,16);
  check(used==text.size() && value<=0xffffffffu,"Invalid original32-bit word");
  return std::uint32_t(value);
}
std::vector<VistaLobbyCountdownCallIntent> calls(const std::string& text) {
  std::vector<VistaLobbyCountdownCallIntent> out;
  std::istringstream stream(text);
  std::string token;
  while(std::getline(stream,token,',')) {
    if(token=="active")out.push_back({VistaLobbyCountdownService::active_query,{}});
    else if(token=="stage")out.push_back({VistaLobbyCountdownService::stage_query,{}});
    else if(token=="sound:10")out.push_back({VistaLobbyCountdownService::ui_sound,10});
    else if(token=="clock")out.push_back({VistaLobbyCountdownService::clock_query,{}});
    else throw std::runtime_error("Unknown original service-call token: "+token);
  }
  return out;
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista countdown fixture");
    std::ifstream file(argv[1]);
    check(bool(file),"Missing original Vista countdown fixture");
    unsigned cases{},cartesian{},sequential{},inactive{},sound{},clock{},unchanged3{},invalid_stage{};
    bool version{};
    std::optional<VistaLobbyCountdownState> retained;
    std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_LOBBY_COUNTDOWN_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream input(line);
      char kind{};std::string label,queried,now,before_stage,before_clock,after_stage,after_clock,call_bank;
      unsigned active{},count{};
      check(bool(input>>kind>>label>>active>>queried>>now>>before_stage>>before_clock
                 >>after_stage>>after_clock>>count>>call_bank),"Truncated original countdown row");
      std::string extra;
      check(kind=='C' && active<=1 && !(input>>extra),"Invalid original countdown row");
      VistaLobbyCountdownState state{std::bit_cast<std::int32_t>(word(before_stage)),word(before_clock)};
      const bool sequence=label.starts_with("sequential_");
      if(sequence) {
        check(label=="sequential_"+std::to_string(sequential),"Original countdown sequence is out of order");
        if(retained) {
          check(state.stage==retained->stage && state.stage3_clock==retained->stage3_clock,
              label+": original sequence input differs from prior native state");
          state=*retained;
        }
      } else {
        check(!retained,"Cartesian fixture resumed after countdown sequence");
        ++cartesian;
      }
      const VistaLobbyCountdownQueryResults query{bool(active),
          std::bit_cast<std::int32_t>(word(queried)),word(now)};
      const auto result=vista_lobby_countdown_consume(state,query);
      check(std::bit_cast<std::uint32_t>(state.stage)==word(after_stage),label+": stored stage differs");
      check(state.stage3_clock==word(after_clock),label+": stage3 clock differs");
      const auto expected=calls(call_bank);
      check(count==expected.size() && result.call_count==count,label+": service-call count differs");
      check(result.call_count<=result.calls.size(),label+": service-call bank overflow");
      for(unsigned i=0;i<count;++i)
        check(result.calls[i]==expected[i],label+": ordered service-call intent differs");
      if(sequence){retained=state;++sequential;}
      ++cases;
      inactive+=!active;
      sound+=count>=3;
      clock+=count==4;
      unchanged3+=active && query.stage==3 && word(before_stage)==3;
      invalid_stage+=active && (query.stage<0 || query.stage>3);
    }
    check(file.eof(),"Original countdown fixture read failed");
    check(version && cases==518 && cartesian==512 && sequential==6 &&
        inactive && sound && clock && unchanged3 && invalid_stage,
        "Original countdown fixture lacks required branch coverage");
    std::cout<<cases<<" original Vista countdown updates matched; ordered service intents and both fields preserved\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
