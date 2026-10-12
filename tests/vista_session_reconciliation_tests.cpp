#include "halo2/vista_session_reconciliation.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const std::string& why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> pattern(std::size_t size,unsigned seed) {
  std::vector<std::byte> out(size+32,std::byte{0xa5});
  for(std::size_t i=0;i<size;++i)out[i]=std::byte((i*37+(i>>8)*17+seed)&255);
  return out;
}
std::vector<std::byte> hex(const std::string& text) {
  check(text.size()%2==0,"invalid original hex");std::vector<std::byte> out(text.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(std::stoul(text.substr(i*2,2),nullptr,16));
  return out;
}
std::string hexadecimal(std::span<const std::byte> b) {
  constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(b.size()*2);
  for(const auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
void patches(std::vector<std::byte>& b,const Json& rows,std::size_t limit) {
  for(const auto& p:rows){const auto o=p.at(0).get<std::size_t>();const auto v=hex(p.at(1).get<std::string>());
    check(o+v.size()<=limit,"unowned fixture patch");std::copy(v.begin(),v.end(),b.begin()+o);}
}
void same(const std::vector<std::byte>& actual,const std::vector<std::byte>& expected,const std::string& why) {
  if(actual==expected)return;
  const auto offset=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
  throw std::runtime_error(why+" at offset "+std::to_string(offset)+" actual="+std::to_string(std::to_integer<unsigned>(actual[offset]))+" expected="+std::to_string(std::to_integer<unsigned>(expected[offset])));
}
class Services final:public VistaSessionReconciliationServices {
 public:
  explicit Services(const Json& input):config(input){}
  Json events=Json::array();
  std::uint32_t network_clock() override {
    const auto value=config.value("clock_base",1000u)+config.value("clock_step",113u)*clock_count++;
    events.push_back({{"kind","clock"},{"value",value}});return value;
  }
  std::uint32_t language() override {
    const auto value=config.value("language",3u);events.push_back({{"kind","language"},{"value",value}});return value;
  }
  std::uint16_t allowed_team_mask(std::span<const std::byte>) override{return mask("allowed");}
  std::uint16_t primary_allowed_team_mask() override{return mask("primary_allowed");}
  std::uint8_t join_context_byte() override {
    const auto value=config.value("context",0u);events.push_back({{"kind","context"},{"value",value}});return value;
  }
  bool authored_random_name(std::span<std::byte> out) override {
    const auto names=config.value("random_names",Json::array({"Authored Name"}));const auto value=names.at(std::min<std::size_t>(random_count++,names.size()-1));
    events.push_back({{"kind","random_name"},{"value",value}});if(value.is_null())return false;
    const auto name=value.get<std::string>();check(name.size()<32,"fixture authored name too long");
    for(std::size_t i=0;i<name.size();++i){out[i*2]=std::byte(name[i]);out[i*2+1]=std::byte{};}
    out[name.size()*2]=out[name.size()*2+1]=std::byte{};return true;
  }
  bool identity_admitted(std::uint32_t owner,VistaLocalUserIdentity id) override {
    const auto value=config.value("admission",true);events.push_back({{"kind","admission"},{"owner",owner},{"identity",hexadecimal(id)},{"value",value}});return value;
  }
  void send_session_message(std::uint32_t owner,std::uint32_t slot,std::uint32_t channel,std::uint32_t id,std::span<const std::byte> payload) override {
    events.push_back({{"kind","message"},{"owner",owner},{"slot",slot},{"channel",channel},{"id",id},{"payload",hexadecimal(payload)}});
  }
 private:
  std::uint16_t mask(const char* kind){const auto value=config.value("mask",255u);events.push_back({{"kind",kind},{"value",value}});return value;}
  Json config;std::uint32_t clock_count{},random_count{};
};
void boundaries() {
  auto session=pattern(vista_session_source_size,43),users=pattern(vista_local_user_bank_size,43),retained=pattern(vista_session_reconciliation_bank_size,43),controllers=pattern(vista_controller_user_bank_size,43);
  std::fill_n(session.begin()+0x14,4,std::byte{});
  const auto session_before=session,users_before=users,retained_before=retained,controllers_before=controllers;
  Services service(Json::object());const VistaSessionLocalPeerValues peer{};
  for(unsigned bank=0;bank<4;++bank) {
    VistaSessionReconciliationBanks b{std::span(session).first(vista_session_source_size),std::span(users).first(vista_local_user_bank_size),std::span(controllers).first(vista_controller_user_bank_size),std::span(retained).first(vista_session_reconciliation_bank_size)};
    auto* selected=bank==0?&b.session:bank==1?&b.local_users:bank==2?&b.controllers:&b.retained;*selected=selected->first(selected->size()-1);
    bool rejected{};try{vista_session_reconcile_local_users(b,{100,100,100,100},peer,service);}catch(const FormatError&){rejected=true;}
    check(rejected,"truncated owned reconciliation bank admitted");
    check(session==session_before && users==users_before && retained==retained_before && controllers==controllers_before && service.events.empty(),"rejected owner binding consumed a service or changed an owner");
  }
  session[0x14]=std::byte{2};const auto bad_slot=session;bool rejected{};
  try{vista_session_publish_final_players({session,users,controllers,retained},service);}catch(const FormatError&){rejected=true;}
  check(rejected && session==bad_slot && users==users_before && retained==retained_before && controllers==controllers_before && service.events.empty(),"unowned retained cache slot was consumed");
}
}
int main(int argc,char** argv) {
  try {
    check(argc==2,"provide original fixture path");std::ifstream file(argv[1]);check(bool(file),"fixture open failed");
    std::string line;unsigned count{};
    while(std::getline(file,line)) {
      if(line.empty())continue;
      const auto row=Json::parse(line);const auto label=row.at("label").get<std::string>();const auto seed=row.at("seed").get<unsigned>();
      auto session=pattern(vista_session_source_size,seed),users=pattern(vista_local_user_bank_size,seed),retained=pattern(vista_session_reconciliation_bank_size,seed),controllers=pattern(vista_controller_user_bank_size,seed);
      patches(session,row.at("before_patches").at("session"),vista_session_source_size);patches(users,row.at("before_patches").at("users"),vista_local_user_bank_size);
      patches(retained,row.at("before_patches").at("retained"),vista_session_reconciliation_bank_size);patches(controllers,row.at("before_patches").at("controllers"),vista_controller_user_bank_size);
      auto expected_session=session,expected_users=users,expected_retained=retained,expected_controllers=controllers;
      patches(expected_session,row.at("after_patches").at("session"),vista_session_source_size);patches(expected_users,row.at("after_patches").at("users"),vista_local_user_bank_size);
      patches(expected_retained,row.at("after_patches").at("retained"),vista_session_reconciliation_bank_size);patches(expected_controllers,row.at("after_patches").at("controllers"),vista_controller_user_bank_size);
      VistaSessionReconciliationBanks banks{session,users,controllers,retained};Services services(row.at("services"));
      const VistaSessionLocalPeerValues peer{u"Actual Machine",u"Actual Party",1,0x11223344,0x99aabbcc,2,100,0xdeadbeef,{{},0u}};
      if(row.at("kind")=="R")vista_session_reconcile_local_users(banks,{100,100,100,100},peer,services);
      else {check(row.at("kind")=="F","invalid fixture kind");vista_session_publish_final_players(banks,services);}
      same(session,expected_session,label+": session bank differs");same(users,expected_users,label+": user bank differs");
      same(retained,expected_retained,label+": retained cache differs");same(controllers,expected_controllers,label+": controller bank differs");
      check(services.events==row.at("events"),label+": service order/operands differ actual="+services.events.dump()+" expected="+row.at("events").dump());++count;
    }
    check(count==830,"missing original reconciliation cases");boundaries();
    std::cout<<"PASS "<<count<<" original complete Vista session/user/controller/cache banks and ordered genuine-service calls\n";return 0;
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
