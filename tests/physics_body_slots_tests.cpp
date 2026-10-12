#include "halo2/physics_body_slots.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const std::string&s) {
  if(!value)throw std::runtime_error("Original retained body slot differs: "+s);
}
std::uint32_t word(const std::string&s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> bytes(const std::string&s) {
  check(s.size()%2==0,"byte extent");std::vector<std::byte>r;r.reserve(s.size()/2);
  for(std::size_t n=0;n<s.size();n+=2)r.push_back(std::byte(word(s.substr(n,2))));
  return r;
}
struct Memory:halo2::SourcePhysicsContactMemory {
  std::map<std::uint32_t,std::vector<std::byte>>buffers;
  auto at(std::uint32_t p,std::size_t n)const {
    auto q=buffers.upper_bound(p);
    if(q==buffers.begin())throw std::runtime_error("Required retained source backing unavailable");
    --q;
    if(std::uint64_t(p-q->first)+n>q->second.size()) {
      std::ostringstream s;s<<"Required source backing unavailable at "<<std::hex<<p<<" size "<<n;
      throw std::runtime_error(s.str());
    }
    return q;
  }
  void read(std::uint32_t p,std::span<std::byte>out)const override {
    auto q=at(p,out.size());std::copy_n(q->second.begin()+(p-q->first),out.size(),out.begin());
  }
  void write(std::uint32_t p,std::span<const std::byte>in)override {
    auto q=at(p,in.size());std::copy(in.begin(),in.end(),buffers.at(q->first).begin()+(p-q->first));
  }
};
struct Case {
  std::string name;std::uint32_t entry,eax,ecx,edi,result;
  std::array<std::uint32_t,16>args;Memory before;
  std::map<std::uint32_t,std::vector<std::byte>>after;
  std::vector<halo2::SourcePhysicsContactRecordStore>stores;
};
}
int main(int argc,char**argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply original independently measured slot fixture");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream input(argv[1]);std::vector<Case>cases;
  for(std::string line;std::getline(input,line);) {
    std::istringstream in(line);std::vector<std::string>v;
    for(std::string x;in>>x;)v.push_back(x);
    if(v.empty())continue;
    if(v[0]=="C") {
      check(v.size()==23,"complete source call ABI");Case c{};
      c.name=v[1];c.entry=word(v[2]);c.eax=word(v[3]);c.ecx=word(v[4]);
      c.edi=word(v[5]);c.result=word(v[6]);
      for(unsigned n=0;n<16;++n)c.args[n]=word(v[n+7]);
      cases.push_back(std::move(c));
    } else {
      check(!cases.empty(),"owning call");auto&c=cases.back();
      if(v[0]=="M"||v[0]=="A") {
        check(v.size()==3,"literal retained buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
        check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique source span");
      }else if(v[0]=="W") {
        check(v.size()==5,"owning store");
        c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});
      }else throw std::runtime_error("Unknown original slot fixture record");
    }
  }
  std::size_t total_bytes=0,total_stores=0;
  for(const auto&c:cases) {
    auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
    const auto observe=[&](const auto&s){stores.push_back(s);};
    switch(c.entry) {
    case 0x1d3a70:
      check(halo2::source_commit_physics_body_slot_1_1(m,
        {c.args[0],c.eax,c.args[1],c.args[2],c.args[3]},observe)==c.result,c.name+" retained return");break;
    case 0x1dc0c0:
      check(halo2::source_construct_physics_body_slot_1_1(m,
        {c.edi,c.args[0],c.args[1],c.args[2],c.args[3]},observe)==c.result,c.name+" slot identity");break;
    case 0x1d29a0:
      halo2::source_refresh_physics_body_slot_1_1(m,c.args[0],c.eax,observe);break;
    default:throw std::runtime_error("Unmeasured original retained slot owner");
    }
    if(stores!=c.stores) {
      std::size_t n=0;while(n<stores.size()&&n<c.stores.size()&&stores[n]==c.stores[n])++n;
      std::ostringstream s;s<<c.name<<" store "<<n<<" actual count "<<stores.size()<<" original "<<c.stores.size();
      if(n<stores.size())s<<" actual "<<std::hex<<stores[n].instruction<<" at "<<stores[n].address;
      if(n<c.stores.size())s<<" source "<<std::hex<<c.stores[n].instruction<<" at "<<c.stores[n].address;
      throw std::runtime_error(s.str());
    }
    for(const auto&[p,expected]:c.after) {
      std::vector<std::byte>actual(expected.size());m.read(p,actual);
      check(actual==expected,c.name+" complete retained buffer");total_bytes+=actual.size();
    }
    total_stores+=stores.size();
  }
  check(cases.size()==6,"two complete original mode7/mode4 chains");
  std::cout<<cases.size()<<" actual original calls; "<<total_bytes<<" full retained bytes; "
           <<total_stores<<" ordered owning stores match\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
