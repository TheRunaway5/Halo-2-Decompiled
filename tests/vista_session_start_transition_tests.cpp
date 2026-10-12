#include "halo2/vista_session_start_transition.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
using J=nlohmann::json;
namespace {
constexpr std::uint32_t object=0x1a00000,handler_address=object+0x9000;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void put(std::span<std::byte> b,unsigned at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));}
std::string hex(std::span<const std::byte> b){constexpr char chars[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=chars[n>>4];out+=chars[n&15];}return out;}
std::string digest(std::span<const std::byte> b){std::array<std::byte,SHA256_DIGEST_LENGTH> hash;SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),reinterpret_cast<unsigned char*>(hash.data()));return hex(hash);}
std::array<std::byte,vista_session_source_size+32> initial(const J& q){
 std::array<std::byte,vista_session_source_size+32> b;for(unsigned i=0;i<vista_session_source_size;++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13+(i>>8)*17)&255);
 std::fill(b.begin()+vista_session_source_size,b.end(),std::byte{0xa5});
 for(auto [at,value]:{std::pair<unsigned,std::uint32_t>{8,0x13371337},{0x14,0},{0x64,0},{0x73a0,q.at("state")},{0x4c68,q.at("phase")},{0x84,q.at("count")},{0x4c64,0xffffffff},{0x4c60,0xfffffffe},{0x71c0,q.at("host")},{0x74,0},{0x4c98,std::uint32_t(q.at("stage").get<std::int32_t>())},{0x4ca0,q.at("countdown_reason")},{0x4ca4,q.at("reason_stage")},{0x4c90,q.at("preflight")},{0x1258,q.at("players")}})put(b,at,value);
 put(b,0x73a4,q.at("transition"),1);put(b,0x4c94,q.at("running"),1);put(b,0x4c9c,q.at("flag"),1);put(b,0x4c74,q.at("local_flag"),1);put(b,0x5101,q.at("prepared"),1);put(b,0x5eb8,0xffff,2);
 for(unsigned i=0;i<18;++i){put(b,0x71c5+i*0x1c,i%3==0?0x80:0,1);put(b,0x110+i*0x10c,q.at("status"));}
 for(unsigned i=0;i<16;++i){const auto at=0x125c+i*0x128;put(b,at+8,i%std::max(1u,q.at("count").get<unsigned>()));put(b,at+0x14,i);put(b,at+0x118,i%8,1);}
 return b;
}
std::array<std::byte,0x100> manager_bank(const VistaLifecycleManager& m){
 std::array<std::byte,0x100> b{};put(b,0,m.current_kind);for(unsigned k=0;k<7;++k)put(b,4+4*k,handler_address+k*0x100);put(b,0x24,object);
 put(b,0x34,m.transitioning,1);put(b,0x35,m.updating,1);put(b,0x36,m.pending,1);put(b,0x38,m.requested_kind);put(b,0x3c,std::bit_cast<std::uint32_t>(m.payload_length));std::copy(m.payload.begin(),m.payload.end(),b.begin()+0x40);return b;
}
}
int main(int argc,char** argv){try{
 require(argc==2,"fixture argument");std::ifstream file(argv[1]);J records;file>>records;unsigned checked{},completed{};
 for(const auto& r:records){const auto& q=r.at("input");auto storage=initial(q);auto b=std::span(storage).first(vista_session_source_size);J calls=J::array();auto clock=q.at("clock").get<std::uint32_t>();
  std::array<std::byte,32> payload;for(unsigned i=0;i<payload.size();++i)payload[i]=std::byte(i*11+7);
  VistaLifecycleManager m;m.current_kind=q.at("kind");m.payload=payload;
  VistaSessionStartTransitionState task;task.countdown.last_clock=q.at("last_clock");task.countdown.last_stage=std::bit_cast<std::int32_t>(q.at("last_stage").get<std::uint32_t>());task.pregame_word18=0xfedcba98;task.loading={q.at("flag10"),q.at("flag11")};task.pregame_bank.fill(std::byte{0xa5});
  VistaSessionTransitionState world{};VistaSessionStartTransitionServices s;
  s.transition.phase.network_clock=[&](){clock+=q.at("clock_step").get<std::uint32_t>();calls.push_back({"clock",digest(b)});return clock;};
  s.transition.phase.send=[&](const VistaSessionPhaseMessage& v){calls.push_back({"send",v.transport_owner,v.session_slot,v.channel,0,0x22,0x4b4,digest(v.payload)});};
  s.countdown_send=[&](const VistaSessionCountdownMessage& v){calls.push_back({"send",v.transport_owner,v.session_slot,v.channel,0,0x23,0x1c,digest(v.payload)});};
  s.null_network_close=[&](unsigned a,unsigned c){calls.push_back({"null_close",a,c});};
  s.pregame_external=[&](unsigned kind){calls.push_back({"external",kind});};s.transition.postgame_external=s.pregame_external;
  s.seed=[&](){calls.push_back({"seed"});return 0x12345678u;};
  s.random_identity=[&](){calls.push_back({"random"});if(q.at("random_state")!=0xffffffffu)put(b,0x73a0,q.at("random_state"));return std::array<std::uint32_t,2>{0xabcdef01,0x76543210};};
  s.network_busy=[&](){calls.push_back({"busy"});if(q.at("busy_state")!=0xffffffffu){put(b,0x73a0,q.at("busy_state"));put(b,0x73a4,q.at("busy_transition"),1);}if(q.at("busy_local")!=0xffffffffu)put(b,0x4c74,q.at("busy_local"),1);return q.at("busy").get<std::uint8_t>();};
  s.transition.termination_reason=[&](){calls.push_back({"termination_reason"});return q.at("termination_reason").get<std::uint32_t>();};s.transition.notify_termination=[&](unsigned reason){calls.push_back({"notify",reason});};
  s.transition.remote_close=[&](unsigned reason){calls.push_back({"remote_close",reason});put(b,0x73a0,0);};s.transition.closing_flags=[](){return std::array<std::uint8_t,2>{};};
  s.transition.game_over_complete=[&](){calls.push_back({"complete"});return std::uint8_t{};};s.transition.world_ready=[&](){calls.push_back({"ready"});return std::uint8_t{};};
  s.transition.map_change=[&](VistaSessionTransitionState& v){calls.push_back({"map_change"});v.world_identity={0xabcdef01,0x76543210};};
  std::array<VistaLifecycleHandler,7> handlers;
  for(unsigned k=0;k<7;++k){handlers[k].update=[&,k](VistaLifecycleManager&){calls.push_back({"other_update",k});return std::uint8_t{};};handlers[k].enter=[&,k](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){calls.push_back({"other_enter",k});};handlers[k].exit=[&,k](VistaLifecycleManager&,std::uint32_t){calls.push_back({"other_exit",k});};}
  vista_session_bind_transition_handlers(handlers,b,world,s.transition);vista_session_bind_start_handlers(handlers,b,task,s);
  auto pump=[&](){vista_lifecycle_pump(m,handlers,[&](VistaLifecycleManager&){calls.push_back({"network"});});};
  const auto op=q.at("op").get<std::string>();std::optional<unsigned> result;
  if(op=="cycle"){pump();vista_session_countdown_write(b,{1,10,0,0,{}},s);for(unsigned i=0;i<15;++i)pump();if(m.current_kind==3)++completed;}
  else if(op=="pump")pump();
  else if(op=="update0")result=vista_session_null_update(m,b,s);
  else if(op=="update1")result=vista_session_pregame_update(m,b,task,s);
  else if(op=="update2")result=vista_session_start_game_update(m,b,task,s);
  else if(op=="enter1")vista_session_pregame_enter(task,s);
  else if(op=="enter2")vista_session_start_game_enter(b,task,s);
  else if(op=="clear")result=vista_session_countdown_clear(b);
  else result=vista_session_countdown_write(b,{q.at("write_running"),q.at("write_stage"),q.at("write_flag"),q.at("write_reason"),std::array<std::uint32_t,2>{0,0}},s);
  const J actual_manager=J::array({m.current_kind,m.transitioning,m.updating,m.pending,m.requested_kind,m.payload_length,hex(payload)});
  const J actual_task=J::array({task.countdown.last_clock,std::bit_cast<std::uint32_t>(task.countdown.last_stage),task.pregame_word18,task.loading.flag10,task.loading.flag11});
  const bool return_match=r.at("returned").is_null()?!result.has_value():result && *result==r.at("returned").get<unsigned>();
  if(digest(b)!=r.at("after").get<std::string>()||calls!=r.at("events")||!return_match||actual_manager!=r.at("manager")||actual_task!=r.at("task")||digest(task.pregame_bank)!=r.at("pregame").get<std::string>()||hex(manager_bank(m))!=r.at("manager_hex").get<std::string>()){
   std::cerr<<"case="<<checked<<" input="<<q.dump()<<"\nexpected="<<r.dump()<<"\nactual="<<digest(b)<<" calls="<<calls.dump()<<" manager="<<actual_manager.dump()<<" task="<<actual_task.dump()<<" result="<<(result?std::to_string(*result):"none")<<'\n';throw std::runtime_error("original joined start transition mismatch");
  }
  require(std::all_of(storage.begin()+vista_session_source_size,storage.end(),[](auto v){return v==std::byte{0xa5};}),"Start transition overwrote full source bank suffix");++checked;
 }
 require(checked==1405,"complete original joined start transition corpus");require(completed>0,"joined source0-to3 countdown cycle");std::cout<<"Vista joined original start transition cases="<<checked<<" completed cycles="<<completed<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
