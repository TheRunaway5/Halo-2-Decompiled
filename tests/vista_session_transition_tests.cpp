#include "halo2/vista_session_transition.hpp"
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
constexpr std::uint32_t object=0x1a00000,manager_address=object+0x8000,handler_address=object+0x9000;
void require(bool b,const char* s){if(!b)throw std::runtime_error(s);}
void put(std::span<std::byte> b,unsigned at,std::uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(8*i));}
std::string hex(std::span<const std::byte> b){constexpr char chars[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=chars[n>>4];out+=chars[n&15];}return out;}
std::string digest(std::span<const std::byte> b){std::array<std::byte,SHA256_DIGEST_LENGTH> hash;SHA256(reinterpret_cast<const unsigned char*>(b.data()),b.size(),reinterpret_cast<unsigned char*>(hash.data()));return hex(hash);}
std::array<std::byte,vista_session_source_size+32> initial(const J& q){
 std::array<std::byte,vista_session_source_size+32> b;for(unsigned i=0;i<vista_session_source_size;++i)b[i]=std::byte((i*37+q.at("seed").get<unsigned>()*13+(i>>8)*17)&255);
 std::fill(b.begin()+vista_session_source_size,b.end(),std::byte{0xa5});
 for(auto [at,value]:{std::pair<unsigned,std::uint32_t>{8,0x13371337},{0x14,0},{0x64,0},{0x73a0,q.at("state")},{0x4c68,q.at("phase")},{0x84,q.at("count")},{0x4c64,0xffffffff},{0x4c60,0xfffffffe},{0x71c0,0},{0x74,0},{0x4cb8,2},{0x4dd8,q.at("map_valid").get<unsigned>()?17u:0xffffffffu}})put(b,at,value);
 put(b,0x73a4,q.at("transition"),1);put(b,0x4cb0,q.at("force"),1);put(b,0x4cbc,q.at("path").get<unsigned>()?65:0,2);
 for(unsigned i=0;i<17;++i)put(b,0x71c5+i*0x1c,i%3==0?0x80:0,1);
 return b;
}
std::array<std::byte,0x100> manager_bank(const VistaLifecycleManager& m){
 std::array<std::byte,0x100> b{};put(b,0,m.current_kind);for(unsigned k=0;k<7;++k)put(b,4+4*k,handler_address+k*0x100);put(b,0x24,object);
 put(b,0x34,m.transitioning,1);put(b,0x35,m.updating,1);put(b,0x36,m.pending,1);put(b,0x38,m.requested_kind);put(b,0x3c,std::bit_cast<std::uint32_t>(m.payload_length));std::copy(m.payload.begin(),m.payload.end(),b.begin()+0x40);return b;
}
}
int main(int argc,char** argv){try{
 require(argc==2,"fixture argument");std::ifstream file(argv[1]);J records;file>>records;unsigned checked{};
 for(const auto& r:records){const auto& q=r.at("input");auto storage=initial(q);auto b=std::span(storage).first(vista_session_source_size);J calls=J::array();unsigned clocks{};
  std::array<std::byte,32> payload;for(unsigned i=0;i<payload.size();++i)payload[i]=std::byte(i*11+7);
  VistaLifecycleManager m;m.current_kind=q.at("kind");m.payload=payload;
  VistaSessionTransitionState state{{0x12345678,0x87654321}};
  VistaSessionTransitionServices s;
  s.phase.network_clock=[&](){calls.push_back({"clock",digest(b)});return 0x12340000u+(++clocks);};
  s.phase.send=[&](const VistaSessionPhaseMessage& message){calls.push_back({"send",message.transport_owner,message.session_slot,message.channel,0,0x22,0x4b4,digest(message.payload)});};
  s.termination_reason=[&](){calls.push_back({"termination_reason"});return q.at("reason").get<std::uint32_t>();};
  s.notify_termination=[&](unsigned reason){calls.push_back({"notify",reason});};
  s.remote_close=[&](unsigned reason){calls.push_back({"remote_close",reason});put(b,0x73a0,0);};
  s.closing_flags=[&](){calls.push_back({"closing"});return std::array<std::uint8_t,2>{q.at("closing_a"),q.at("closing_b")};};
  s.game_over_complete=[&](){calls.push_back({"complete"});return std::uint8_t(q.at("complete").get<unsigned>() && q.at("remaining").get<std::uint32_t>()==0);};
  s.world_ready=[&](){calls.push_back({"ready"});return q.at("ready").get<std::uint8_t>();};
  s.world_options=[&](){calls.push_back({"options"});return VistaSessionWorldOptions{{0x12345678u^(q.at("identity_match").get<unsigned>()?0u:1u),0x87654321u},{2,q.at("map_match").get<unsigned>()?17u:18u}};};
  s.force_end=[&](){calls.push_back({"force_end"});};s.network_close=[&](){calls.push_back({"network_close"});};
  s.map_change=[&](VistaSessionTransitionState& target){calls.push_back({"map_change"});target.world_identity={0xabcdef01,0x76543210};};
  s.postgame_external=[&](unsigned kind){calls.push_back({"postgame",kind});};
  std::array<VistaLifecycleHandler,7> handlers;
  for(unsigned k=0;k<7;++k){handlers[k].update=[&,k](VistaLifecycleManager&){calls.push_back({"other_update",k});return std::uint8_t{};};handlers[k].enter=[&,k](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){calls.push_back({"other_enter",k});};handlers[k].exit=[&,k](VistaLifecycleManager&,std::uint32_t){calls.push_back({"other_exit",k});};}
  vista_session_bind_transition_handlers(handlers,b,state,s);
  auto pump=[&](){vista_lifecycle_pump(m,handlers,[&](VistaLifecycleManager&){calls.push_back({"network"});});};
  const auto op=q.at("op").get<std::string>();std::optional<unsigned> result;
  if(op=="done_cycle"){pump();result=unsigned(vista_session_pcr_continue(b,m.current_kind,s.phase));pump();}
  else if(op=="pump")pump();
  else if(op=="preflight")result=vista_session_transition_preflight(m,b,s);
  else if(op=="update3")result=vista_session_in_game_update(m,b,state,s);
  else if(op=="update4")result=vista_session_post_game_update(m,b,s);
  else if(op=="exit")vista_session_in_game_exit(state,s);
  else if(op=="enter3")vista_session_in_game_enter(state,s);
  else vista_session_post_game_enter(s);
  const J actual_manager=J::array({m.current_kind,m.transitioning,m.updating,m.pending,m.requested_kind,m.payload_length,hex(payload)});
  const J actual_identity=state.world_identity;
  const bool return_match=r.at("returned").is_null()?!result.has_value():result && *result==r.at("returned").get<unsigned>();
  if(digest(b)!=r.at("after").get<std::string>()||calls!=r.at("events")||!return_match||actual_manager!=r.at("manager")||actual_identity!=r.at("identity")||hex(manager_bank(m))!=r.at("manager_hex").get<std::string>()){
   std::cerr<<"case="<<checked<<" input="<<q.dump()<<"\nexpected="<<r.dump()<<"\nactual="<<digest(b)<<" calls="<<calls.dump()<<" manager="<<actual_manager.dump()<<" identity="<<actual_identity.dump()<<" result="<<(result?std::to_string(*result):"none")<<'\n';throw std::runtime_error("original joined transition mismatch");
  }
  require(std::all_of(storage.begin()+vista_session_source_size,storage.end(),[](auto v){return v==std::byte{0xa5};}),"Transition overwrote full source bank suffix");++checked;
 }
 require(checked==478,"complete original joined transition corpus");std::cout<<"Vista joined original transition cases="<<checked<<" PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
