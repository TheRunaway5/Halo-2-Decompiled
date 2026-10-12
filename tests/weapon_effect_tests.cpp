#include "halo2/weapon_effect.hpp"
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
using Bytes=std::vector<std::byte>;
void check(bool v,const std::string &s) {if(!v)throw std::runtime_error(s);}
std::uint32_t num(const std::string &s) {return std::stoul(s,nullptr,16);}
Bytes bytes(const std::string &s) {
  if(s=="-")return {};
  check(s.size()%2==0,"hex extent");Bytes b;
  for(std::size_t i=0;i<s.size();i+=2)b.push_back(std::byte(num(s.substr(i,2))));
  return b;
}
template<std::size_t N> std::array<std::byte,N> fixed(const std::string &s) {
  auto b=bytes(s);check(b.size()==N,"fixed source input extent");
  std::array<std::byte,N> a;std::copy(b.begin(),b.end(),a.begin());return a;
}
std::vector<std::string> fields(const std::string &s) {
  std::istringstream in(s);std::vector<std::string> r;std::string word;
  while(in>>word)r.push_back(word);
  return r;
}
struct Object {std::uint8_t flags;std::uint32_t model;Bytes body;};
void request(const std::vector<std::vector<std::string>> &rows) {
  const auto &r=rows[0];check(r.size()==15,"request schema");
  halo2::WeaponEffectInput in{num(r[2]),num(r[3]),num(r[4]),num(r[5]),num(r[6])};
  const auto expected=bytes(r[8]),directions=bytes(r[9]);
  std::map<std::uint32_t,Object> objects;
  for(std::size_t n=1;n<rows.size();++n) {
    const auto &v=rows[n];check(v[0]=="O"&&v.size()==5,"object schema");
    objects.emplace(num(v[1]),Object{std::uint8_t(num(v[2])),num(v[3]),bytes(v[4])});
  }
  unsigned created=0;
  halo2::WeaponEffectServices services;
  services.object=[&](std::uint32_t id) {
    auto it=objects.find(id);check(it!=objects.end(),"original object identity");
    auto &o=it->second;return halo2::WeaponEffectObject{o.flags,o.body,o.model};
  };
  services.tag_group=[&](std::uint32_t id) {check(id==in.effect,"source effect identity");return num(r[14]);};
  services.center=[&](std::uint32_t id) {
    check(id==num(r[3]),"selected original object");return fixed<12>(r[10]);
  };
  services.gravity=fixed<12>(r[11]);services.up=fixed<12>(r[12]);
  services.directions_address=num(r[13]);
  services.create=[&](const halo2::AttachedEffectRequest &q) {
    check(std::equal(q.packet.begin(),q.packet.end(),expected.begin(),expected.end()),"full original60 creation packet");
    check(std::equal(q.directions.begin(),q.directions.end(),directions.begin(),directions.end()),"full original38 direction entries");
    ++created;return num(r[7]);
  };
  check(halo2::dispatch_source_weapon_effect(in,services)==num(r[7]),"source dispatch return");
  check(created==1,"owning source create callback count");
  // The literal source boundary is mandatory; it cannot turn a request into
  // successful creation if the live allocation/event owner is unavailable.
  services.create={};bool missing=false;
  try { (void)halo2::dispatch_source_weapon_effect(in,services); }
  catch(const std::runtime_error &) {missing=true;}
  check(missing,"missing actual effect owner must remain unavailable");
}
void visibility(const std::vector<std::vector<std::string>> &rows) {
  const auto &r=rows[0],&k=rows[1];check(r.size()==10&&k.size()==4,"visibility schema");
  auto state=bytes(r[3]);const auto expected=bytes(r[4]);
  const auto local=bytes(r[8]),all=bytes(r[9]),callback_before=bytes(k[1]),callback_after=bytes(k[2]);
  unsigned called=0;halo2::EffectVisibilityServices s;
  s.special_view=std::stoul(r[6])!=0;s.camera_override=std::stoul(r[7])!=0;
  s.local_pvs=local;s.all_player_pvs=all;
  if(!callback_before.empty())s.retire=[&] {
    check(state==callback_before,"source retirement entry and visibility store order");
    state=callback_after;++called;return std::stoul(k[3])!=0;
  };
  check(halo2::update_source_effect_visibility(state,num(r[2]),s)==(std::stoul(r[5])!=0),"source visibility AL");
  check(state==expected,"full original190 effect exit buffer");
  check(called==unsigned(!callback_before.empty()),"source retirement callback count");
}
}
int main(int argc,char **argv) {
  try {
    check(argc==2,"usage: weapon_effect_tests original-fixtures.txt");
    std::ifstream in(argv[1]);check(bool(in),"original fixture input");
    std::vector<std::vector<std::string>> rows;std::string line;unsigned count=0;
    while(std::getline(in,line)) {
      auto r=fields(line);if(r.empty())continue;
      if(r[0]!="E") {rows.push_back(std::move(r));continue;}
      check(!rows.empty(),"fixture owning root");
      if(rows[0][0]=="D")request(rows);else if(rows[0][0]=="V")visibility(rows);else check(false,"source fixture root");
      rows.clear();++count;
    }
    check(rows.empty()&&count>0,"complete source cases");
    std::cout<<"PASS "<<count<<" original effect request/visibility roots\n";return 0;
  }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
