#include "halo2/physics_shape_wrapper.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const std::string&s) {
  if(!value)throw std::runtime_error("Original retained shape wrapper differs: "+s);
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
  std::string name;std::uint32_t entry,eax,ecx,edi,result,xmm0,cw,mxcsr;
  halo2::SourcePhysicsShapeWrapperScratch scratch;
  std::array<std::uint32_t,16>args;Memory before;
  std::map<std::uint32_t,std::vector<std::byte>>after;
  std::vector<halo2::SourcePhysicsContactRecordStore>stores;
};
}
int main(int argc,char**argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply original independently measured wrapper fixture");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream input(argv[1]);std::vector<Case>cases;
  for(std::string line;std::getline(input,line);) {
    std::istringstream in(line);std::vector<std::string>v;
    for(std::string x;in>>x;)v.push_back(x);
    if(v.empty())continue;
    if(v[0]=="C") {
      check(v.size()==30,"complete source wrapper call ABI");Case c{};
      c.name=v[1];c.entry=word(v[2]);c.eax=word(v[3]);c.ecx=word(v[4]);
      c.edi=word(v[5]);c.result=word(v[6]);c.xmm0=word(v[7]);c.cw=word(v[8]);c.mxcsr=word(v[9]);
      c.scratch={word(v[10]),word(v[11]),word(v[12]),word(v[13])};
      for(unsigned n=0;n<16;++n)c.args[n]=word(v[n+14]);
      cases.push_back(std::move(c));
    } else {
      check(!cases.empty(),"owning call");auto&c=cases.back();
      if(v[0]=="M"||v[0]=="A") {
        check(v.size()==3,"literal retained buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
        check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique source span");
      }else if(v[0]=="W") {
        check(v.size()==5,"owning store");
        c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});
      }else throw std::runtime_error("Unknown original wrapper fixture record");
    }
  }
  std::size_t total_bytes=0,total_stores=0;
  for(const auto&c:cases) {
    auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
    const auto observe=[&](const auto&s){stores.push_back(s);};
    const halo2::SourcePhysicsFactoryFloatingPoint fp{static_cast<std::uint16_t>(c.cw),c.mxcsr};
    switch(c.entry) {
    case 0x1dacf0:
      check(halo2::source_commit_physics_shape_wrapper_1_1(m,c.args[0],c.edi,c.args[1],c.scratch,fp,observe)==c.result,c.name+" body identity");break;
    case 0x1dadf0:
      check(halo2::source_prepare_physics_shape_wrapper_1_1(m,c.args[0],c.args[1],c.args[2],c.scratch,fp,observe)==c.result,c.name+" wrapper identity");break;
    case 0x1dad80:
      check(halo2::source_construct_physics_shape_wrapper_1_1(m,c.edi,c.args[0],c.args[1],c.scratch,fp,observe)==c.result,c.name+" allocated wrapper identity");break;
    case 0x184510:
      check(halo2::source_physics_convex_extents_1_1(m,c.args[0],c.scratch,fp,observe)==c.xmm0,c.name+" actual XMM0 return");break;
    default:throw std::runtime_error("Unmeasured original retained wrapper owner");
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
  check(cases.size()==5,"actual mode7 skip/mode4 wrapper and child owners");
  std::cout<<cases.size()<<" actual original calls; "<<total_bytes<<" full retained bytes; "
           <<total_stores<<" ordered owning stores match\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
