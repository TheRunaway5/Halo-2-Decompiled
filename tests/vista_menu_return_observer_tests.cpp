#include "halo2/vista_menu_return_observer.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace halo2;
namespace {
using Q=VistaMenuReturnQuery;
using A=VistaMenuReturnAction;
void require(bool ok,const std::string& msg){if(!ok)throw std::runtime_error(msg);}
bool recorded(Q q) {
  return q==Q::screen||q==Q::protocol||q==Q::ui_active||q==Q::task_gate_a||q==Q::task_gate_b||q==Q::task_state||
    q==Q::task_blocks||q==Q::local_controller_count||q==Q::closing||q==Q::world_gate||q==Q::world_gate_b||
    q==Q::controller||q==Q::loading||q==Q::loading_secondary;
}
std::uint32_t value(Q q,const std::array<std::uint32_t,19>& v) {
  switch(q) {
    case Q::screen:return v[0];case Q::world_ready:return v[1];case Q::world_is_mainmenu:return v[2]==3;
    case Q::network_enabled:return v[3];case Q::lifecycle_enabled:return v[4];case Q::lifecycle_kind:return v[5];
    case Q::protocol:return v[6];case Q::ui_disabled:return v[7];case Q::ui_active:return v[8];
    case Q::local_controller_count:return v[9];case Q::task_gate_a:return v[10];case Q::task_gate_b:return v[11];
    case Q::task_state:return v[12];case Q::task_blocks:return v[14];case Q::closing:return v[15];
    case Q::world_gate:return v[16];case Q::world_gate_b:return v[17];case Q::controller:return v[18];
    case Q::loading:case Q::loading_secondary:return 0;
  }
  throw std::runtime_error("unowned query enum");
}
}
int main(int argc,char** argv)try {
  if(argc!=2){std::cerr<<"usage: vista_menu_return_observer_tests <original.tsv>\n";return 2;}
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream in(argv[1]);std::string line;unsigned cases=0,blue=0,alternate=0,reason7=0,task=0,blocked=0;
  while(std::getline(in,line)) {
    if(line.empty()||line[0]=='#')continue;
    std::istringstream row(line);std::string kind,label;std::size_t n;row>>kind>>label>>n;
    require(kind=="U", "unknown original fixture row");std::vector<std::uint32_t> words(n);
    for(auto& w:words){std::string token;require(bool(row>>token),label+" truncated fixture");w=std::stoul(token,nullptr,16);}
    std::string extra;require(!(row>>extra),label+" trailing fixture words");std::size_t at=0;
    auto next=[&](){require(at<words.size(),label+" invalid fixture extent");return words[at++];};
    std::array<std::uint32_t,19> input{};for(auto& w:input)w=next();
    const auto retained=next(),count=next();std::vector<VistaMenuReturnIntent> expected;
    for(unsigned j=0;j<count;++j) {
      VistaMenuReturnIntent call;call.action=A(next());call.argument_count=next();
      require(call.argument_count<=call.arguments.size(),label+" fixture intent extent");
      for(unsigned k=0;k<call.argument_count;++k)call.arguments[k]=next();expected.push_back(call);
    }
    const auto query_count=next();std::vector<std::pair<unsigned,std::uint32_t>> expected_queries;
    for(unsigned j=0;j<query_count;++j){const auto q=next(),v=next();expected_queries.emplace_back(q,v);}
    require(at==words.size(),label+" unconsumed fixture");
    VistaMenuReturnState state{input[13]};std::vector<VistaMenuReturnIntent> applied;
    std::vector<std::pair<unsigned,std::uint32_t>> queries;
    VistaMenuReturnServices services;
    services.query=[&](Q q){const auto v=value(q,input);if(recorded(q))queries.emplace_back(unsigned(q),v);return v;};
    services.action=[&](const VistaMenuReturnIntent& call){applied.push_back(call);};
    const auto actual=vista_menu_return_observe(state,services);
    require(state.retained_task_state==retained,label+" retained task cache mismatch");
    require(actual.call_count==expected.size(),label+" action count mismatch");
    for(unsigned j=0;j<actual.call_count;++j)require(actual.calls[j]==expected[j],label+" action "+std::to_string(j)+" mismatch");
    require(applied==expected,label+" synchronous action dispatch/order mismatch");
    require(queries==expected_queries,label+" original external query order/result mismatch");
    for(const auto& call:expected) {
      if(call.action==A::screen_factory&&call.arguments[7]==0x646752)++blue;
      if(call.action==A::screen_factory&&call.arguments[7]==0x6467E9)++alternate;
      if(call.action==A::mainmenu_request&&call.arguments[0]==7)++reason7;
      if(call.action==A::cache_task_state)++task;
    }
    if(input[7]){++blocked;require(expected.size()==1&&expected[0].action==A::observer_suffix,label+" UI disable must bypass observer change");}
    ++cases;
  }
  require(cases>=219&&blue&&alternate&&reason7&&task&&blocked,"missing source observer discriminating controls");
  // The service boundary is compulsory; absence is not a false/default query.
  bool rejected=false;try {VistaMenuReturnState s;vista_menu_return_observe(s,{});}catch(const FormatError&){rejected=true;}
  require(rejected,"missing query owner was silently defaulted");
  std::cout<<cases<<" unchanged PC observer cases; exact ordered actions/queries, both lobby factories, reason7 and task/disable controls\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
