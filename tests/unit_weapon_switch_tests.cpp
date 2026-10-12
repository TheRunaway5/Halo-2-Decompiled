#include "halo2/unit_weapon_switch.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value,const std::string &message) {
  if (!value) throw std::runtime_error("Source weapon switch differs: "+message);
}
std::uint32_t number(const std::string &s) { return std::stoul(s,nullptr,16); }
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size()%2==0,"hex extent");std::vector<std::byte> b;
  for (std::size_t i=0;i<s.size();i+=2) b.push_back(std::byte(number(s.substr(i,2))));
  return b;
}
using State=std::map<std::string,std::vector<std::byte>>;
std::string weapon_key(std::uint32_t datum) {
  std::ostringstream s;s<<'W'<<std::hex;s.width(8);s.fill('0');s<<datum;return s.str();
}
struct Callback { halo2::WeaponSwitchLeafCall call;std::uint32_t result;State before,after; };
struct Case {
  std::string label;std::uint32_t entry,unit,mode,tick;
  bool primary,flag0,flag1;unsigned result;
  std::vector<std::byte> request;State before,after;
  std::vector<Callback> callbacks;std::vector<halo2::WeaponSwitchStore> stores;
};
void apply(State &state,const State &after) {
  check(state.size()==after.size(),"literal service state keys");
  for (const auto &[key,b]:after) {
    auto &dest=state.at(key);check(dest.size()==b.size(),"literal service extent");
    std::copy(b.begin(),b.end(),dest.begin());
  }
}
void run(const Case &c,const std::map<std::uint32_t,std::vector<std::byte>> &definitions,
         unsigned &leaf_count,unsigned &store_count) {
  auto state=c.before;std::size_t next=0;unsigned ticks=0;
  std::vector<halo2::WeaponSwitchStore> stores;
  halo2::UnitWeaponSwitchServices s;
  s.unit_body=[&](std::uint32_t id)->std::span<std::byte>{check(id==c.unit,c.label+" full unit identity");return state.at("U");};
  s.action_component=[&](std::uint32_t id)->std::span<std::byte>{check(id==c.unit,c.label+" source component identity");return state.at("C");};
  s.weapon_body=[&](std::uint32_t id)->std::span<std::byte>{return state.at(weapon_key(id));};
  s.weapon_definition=[&](std::uint32_t id)->std::span<const std::byte>{
    const auto &w=state.at(weapon_key(id));std::uint32_t tag=0;
    for (unsigned i=0;i<4;++i)tag|=std::uint32_t(std::to_integer<unsigned>(w.at(i)))<<(i*8);
    return definitions.at(tag);
  };
  s.source_tick=[&]{++ticks;return c.tick;};
  s.invoke=[&](const halo2::WeaponSwitchLeafCall &call){
    check(next<c.callbacks.size(),c.label+" extra leaf "+std::to_string(call.entry));
    const auto &expected=c.callbacks[next++];
    check(call==expected.call,c.label+" leaf order/ABI "+std::to_string(next));
    check(state==expected.before,c.label+" full buffers at leaf entry "+std::to_string(next));
    // This is ONLY the frozen differential's explicit literal service result.
    // The production API supplies no successful world/pose/clock/admission leaf.
    apply(state,expected.after);++leaf_count;return expected.result;
  };
  auto observe=[&](const halo2::WeaponSwitchStore &w){stores.push_back(w);};
  unsigned result=0;
  switch(c.entry) {
    case 0xe9fe0:result=halo2::begin_source_weapon_switch(c.unit,c.request,s,observe);break;
    case 0xe9b40:result=halo2::put_away_source_weapon(c.unit,c.primary,c.flag0,c.flag1,s,observe);break;
    case 0xea180:result=halo2::update_source_weapon_switch(c.unit,c.mode,s,observe);break;
    case 0xea270:halo2::interrupt_source_weapon_switch(c.unit,c.mode,s,observe);break;
    case 0xea370:halo2::finish_source_weapon_switch(c.unit,c.mode,s,observe);break;
    case 0xe9a90:halo2::clear_current_source_weapon(c.unit,c.primary,s,observe);break;
    case 0xe9d50:halo2::equip_requested_source_weapon(c.unit,c.primary,c.mode,c.flag0,c.flag1,s,observe);break;
    default:throw std::runtime_error("Unknown original owner");
  }
  if(c.entry==0xe9fe0||c.entry==0xe9b40||c.entry==0xea180)
    check(result==c.result,c.label+" original AL");
  check(next==c.callbacks.size(),c.label+" all owning leaves consumed");
  check(state==c.after,c.label+" complete final literal buffers");
  check(stores==c.stores,c.label+" exact ordered owning stores");
  const auto timestamps=std::count_if(c.stores.begin(),c.stores.end(),[](const auto &w){return w.instruction==0xe9ecb;});
  check(ticks==unsigned(timestamps),c.label+" game tick read only at source timestamp store");
  store_count+=unsigned(stores.size());
  // Preserve absence as a hard boundary, never an admission success.
  if (!c.callbacks.empty()) {
    auto unavailable=s;unavailable.invoke={};apply(state,c.before);
    bool stopped=false;
    try {
      if(c.entry==0xe9b40)halo2::put_away_source_weapon(c.unit,c.primary,c.flag0,c.flag1,unavailable);
      else return;
    }catch(const std::runtime_error&){stopped=true;}
    check(stopped && state==c.before,c.label+" missing manager producer fails before changing state");
  }
}
}
int main(int argc,char **argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply original measured weapon-switch fixture");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream file(argv[1]);std::vector<Case> cases;
  std::map<std::uint32_t,std::vector<std::byte>> definitions;
  for(std::string line;std::getline(file,line);) {
   std::istringstream stream(line);std::vector<std::string> r;
   for(std::string x;stream>>x;)r.push_back(x);
   if(r.empty())continue;
   if(r[0]=="D") { check(r.size()==3,"definition row");definitions.emplace(number(r[1]),bytes(r[2])); }
   else if(r[0]=="C") {
    check(r.size()==12,"owner row");Case c;
    c.label="case "+r[1]+" call "+r[2]+" owner "+r[3];c.entry=number(r[3]);c.unit=number(r[4]);
    c.primary=r[5]!="0";c.flag0=r[6]!="0";c.flag1=r[7]!="0";c.mode=number(r[8]);c.tick=number(r[9]);
    c.result=std::stoul(r[10]);if(r[11]!="-")c.request=bytes(r[11]);cases.push_back(std::move(c));
   }else if(r[0]=="K") {
    check(!cases.empty()&&r.size()>=4,"leaf row");Callback k;k.call.entry=number(r[1]);k.call.count=std::stoul(r[2]);
    check(k.call.count<=6&&r.size()==4+k.call.count,"leaf argument extent");
    for(unsigned i=0;i<k.call.count;++i)k.call.arguments[i]=number(r[3+i]);
    k.result=number(r.back());cases.back().callbacks.push_back(std::move(k));
   }else if(r[0]=="W") {
    check(!cases.empty()&&r.size()==7,"store row");const auto &c=cases.back();
    const auto kind=r[2]=="U"?halo2::WeaponSwitchBuffer::Unit:r[2]=="C"?halo2::WeaponSwitchBuffer::ActionComponent:halo2::WeaponSwitchBuffer::Weapon;
    const auto id=kind==halo2::WeaponSwitchBuffer::Weapon?number(r[2].substr(1)):c.unit;
    auto value=[](const std::string &x){auto b=bytes(x);std::uint32_t v=0;for(unsigned i=0;i<b.size();++i)v|=std::uint32_t(std::to_integer<unsigned>(b[i]))<<(i*8);return v;};
    cases.back().stores.push_back({number(r[1]),id,kind,number(r[3]),unsigned(std::stoul(r[4])),value(r[5]),value(r[6])});
   }else if(r[0]=="B"||r[0]=="F"||r[0]=="I"||r[0]=="O") {
    check(!cases.empty()&&r.size()==3,"buffer row");auto &c=cases.back();
    auto *dest=r[0]=="B"?&c.before:r[0]=="F"?&c.after:r[0]=="I"?&c.callbacks.back().before:&c.callbacks.back().after;
    check(dest->emplace(r[1],bytes(r[2])).second,"unique buffer");
   }else throw std::runtime_error("Unknown switch fixture row");
  }
  unsigned leaves=0,stores=0;for(const auto &c:cases)run(c,definitions,leaves,stores);
  check(cases.size()==79 && leaves==443 && stores==273,"complete measured owner suite");
  std::cout<<cases.size()<<" original switch/carry owners, "<<leaves<<" literal owning boundaries and "<<stores<<" ordered stores matched\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
