#include "halo2/vista_session_countdown.hpp"
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
void check(bool value,const std::string& message) {
  if(!value)throw std::runtime_error(message);
}
std::uint32_t word(const std::string& text) {
  std::size_t used{};
  const auto value=std::stoull(text,&used,16);
  check(used==text.size() && value<=0xffffffffu,"Invalid original32 word");
  return std::uint32_t(value);
}
std::int32_t signed_word(const std::string& text) {
  return std::bit_cast<std::int32_t>(word(text));
}
VistaSessionCountdownService service(const std::string& name) {
  if(name=="ui")return VistaSessionCountdownService::ui_state;
  if(name=="user")return VistaSessionCountdownService::controller_user;
  if(name=="host")return VistaSessionCountdownService::host_permission;
  if(name=="active")return VistaSessionCountdownService::active_stage;
  if(name=="session")return VistaSessionCountdownService::session_stage;
  if(name=="flag")return VistaSessionCountdownService::session_flag;
  if(name=="clock")return VistaSessionCountdownService::clock;
  if(name=="elapsed")return VistaSessionCountdownService::elapsed;
  if(name=="write")return VistaSessionCountdownService::write;
  throw std::runtime_error("Unknown original countdown service: "+name);
}
std::vector<VistaSessionCountdownCall> calls(const std::string& text) {
  std::vector<VistaSessionCountdownCall> result;
  std::istringstream stream(text);
  std::string token;
  while(std::getline(stream,token,',')) {
    std::istringstream fields(token);
    std::string field;
    check(bool(std::getline(fields,field,':')),"Missing original service");
    VistaSessionCountdownCall call;
    call.service=service(field);
    while(std::getline(fields,field,':')) {
      check(call.argument_count<call.arguments.size(),"Original service argument overflow");
      call.arguments[call.argument_count++]=word(field);
    }
    result.push_back(call);
  }
  return result;
}
void compare(const VistaSessionCountdownResult& result,unsigned count,
             const std::string& encoded,const std::string& label) {
  const auto expected=calls(encoded);
  check(count==expected.size() && result.call_count==count && count<=result.calls.size(),
      label+": original service count differs");
  for(unsigned index=0;index<count;++index)
    check(result.calls[index]==expected[index],label+": original service/arguments/order differ");
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original Vista session countdown fixture");
    std::ifstream file(argv[1]);
    check(bool(file),"Missing original Vista session countdown fixture");
    unsigned starts{},timers{},retained_count{},writes{},nonboolean_flags{},invalid_ui{},negative_stage{};
    bool version{};
    std::optional<VistaSessionCountdownTaskState> retained;
    std::string line;
    while(std::getline(file,line)) {
      if(line=="# H2V_SESSION_COUNTDOWN_ORIGINAL_V1")version=true;
      if(line.empty() || line.front()=='#')continue;
      std::istringstream input(line);
      char kind{};std::string label,encoded,extra;
      unsigned count{};
      input>>kind>>label;
      if(kind=='S') {
        std::array<std::string,9> words;
        for(auto& text:words)check(bool(input>>text),label+": truncated original start row");
        check(bool(input>>count>>encoded) && !(input>>extra),label+": malformed original start row");
        const VistaSessionCountdownStartRequest request{
            signed_word(words[0]),signed_word(words[1]),signed_word(words[2])};
        const VistaSessionCountdownStartQueries queries{
            signed_word(words[3]),signed_word(words[4]),bool(word(words[5])),
            signed_word(words[6]),bool(word(words[7]))};
        const auto result=vista_session_countdown_start(request,queries);
        check(result.succeeded==bool(word(words[8])),label+": original start returnAL differs");
        compare(result,count,encoded,label);
        ++starts;
        invalid_ui+=queries.ui_state!=10 && queries.ui_state!=11;
        writes+=result.call_count && result.calls[result.call_count-1].service==VistaSessionCountdownService::write;
      } else if(kind=='T') {
        std::array<std::string,10> words;
        for(auto& text:words)check(bool(input>>text),label+": truncated original timer row");
        check(bool(input>>count>>encoded) && !(input>>extra),label+": malformed original timer row");
        check(word(words[3])<=0xff,"Original flag is not a byte");
        VistaSessionCountdownTaskState state{signed_word(words[0]),word(words[1])};
        const bool sequence=label.starts_with("retained_");
        if(sequence) {
          check(label=="retained_"+std::to_string(retained_count),"Original retained task order differs");
          if(retained) {
            check(state.last_stage==retained->last_stage && state.last_clock==retained->last_clock,
                label+": original retained input differs from prior native task output");
            state=*retained;
          }
        } else check(!retained,"Independent timers resumed after retained task sequence");
        const VistaSessionCountdownTimerQueries queries{
            signed_word(words[2]),std::uint8_t(word(words[3])),
            word(words[4]),word(words[5]),word(words[6])};
        const auto result=vista_session_countdown_timer(state,queries);
        check(state.last_stage==signed_word(words[7]) && state.last_clock==word(words[8]),
            label+": original task stage/clock differs");
        check(result.succeeded==bool(word(words[9])),label+": original timer returnAL differs");
        compare(result,count,encoded,label);
        ++timers;
        negative_stage+=queries.session_stage<0;
        nonboolean_flags+=queries.session_flag>1;
        if(sequence){retained=state;++retained_count;}
      } else throw std::runtime_error("Unknown original row kind");
    }
    check(file.eof(),"Original session countdown fixture read failed");
    check(version && starts==1029 && timers==1350 && retained_count==6 && writes &&
        nonboolean_flags && invalid_ui && negative_stage,"Original countdown coverage incomplete");
    std::cout<<starts<<" original Vista start admissions and "<<timers
        <<" original countdown task updates matched; no live session activation\n";
    return 0;
  } catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
