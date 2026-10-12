#include "halo2/physics_body_grouping.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {
void check(bool value,const std::string&s) {
  if(!value)throw std::runtime_error("Original retained body grouping differs: "+s);
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
  std::string name;std::uint32_t entry,eax,ecx,esi,edi,result,cw,mxcsr;
  halo2::SourcePhysicsBodyGroupingScratch scratch;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> estimates;
  std::vector<std::array<std::uint32_t,14>>factory_inputs;
  std::array<std::uint32_t,16>args;Memory before;
  std::map<std::uint32_t,std::vector<std::byte>>after;
  std::vector<halo2::SourcePhysicsContactRecordStore>stores;
  std::vector<bool>temporary;
};
}
int main(int argc,char**argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply original independently measured grouping fixture");
  if(!std::filesystem::exists(argv[1]))return 77;
  std::ifstream input(argv[1]);std::vector<Case>cases;
  for(std::string line;std::getline(input,line);) {
    std::istringstream in(line);std::vector<std::string>v;
    for(std::string x;in>>x;)v.push_back(x);
    if(v.empty())continue;
    if(v[0]=="C") {
      check(v.size()==41,"complete source grouping call ABI");Case c{};
      c.name=v[1];c.entry=word(v[2]);c.eax=word(v[3]);c.ecx=word(v[4]);
      c.esi=word(v[5]);c.edi=word(v[6]);c.result=word(v[7]);c.cw=word(v[8]);c.mxcsr=word(v[9]);
      c.scratch={word(v[10]),word(v[11]),word(v[12]),word(v[13]),word(v[14]),word(v[15]),word(v[16]),
        {word(v[17]),word(v[18]),word(v[19]),word(v[20])},
        {word(v[21]),word(v[22]),word(v[23]),word(v[24])}};
      for(unsigned n=0;n<16;++n)c.args[n]=word(v[n+25]);
      cases.push_back(std::move(c));
    } else {
      check(!cases.empty(),"owning call");auto&c=cases.back();
      if(v[0]=="M"||v[0]=="A") {
        check(v.size()==3,"literal retained buffer");auto&bank=v[0]=="M"?c.before.buffers:c.after;
        check(bank.emplace(word(v[1]),bytes(v[2])).second,"unique source span");
      }else if(v[0]=="W"||v[0]=="WS") {
        check(v.size()==5,"owning store");
        c.stores.push_back({word(v[1]),word(v[2]),bytes(v[3]),bytes(v[4])});
        c.temporary.push_back(v[0]=="WS");
      }else if(v[0]=="Q") {
        check(v.size()==3,"original reciprocal estimate boundary");
        c.estimates.emplace_back(word(v[1]),word(v[2]));
      }else if(v[0]=="B") {
        check(v.size()==15,"complete original factory call words");
        std::array<std::uint32_t,14>args{};
        for(unsigned i=0;i<14;++i)args[i]=word(v[i+1]);
        c.factory_inputs.push_back(args);
      }else throw std::runtime_error("Unknown original grouping fixture record");
    }
  }
  std::size_t total_bytes=0,total_stores=0;
  for(const auto&c:cases) {
    auto m=c.before;std::vector<halo2::SourcePhysicsContactRecordStore>stores;
    const auto observe=[&](const auto&s){stores.push_back(s);};
    const halo2::SourcePhysicsFactoryFloatingPoint fp{static_cast<std::uint16_t>(c.cw),c.mxcsr};
    std::size_t estimate_index=0;std::vector<std::array<std::uint32_t,14>>factory_inputs;
    const auto observe_call=[&](const auto&a){factory_inputs.push_back(a);};
    const auto estimate=[&](std::uint32_t operand) {
      check(estimate_index<c.estimates.size(),c.name+" actual reciprocal owner required");
      const auto pair=c.estimates[estimate_index++];
      check(pair.first==operand,c.name+" original reciprocal operand");return pair.second;
    };
    switch(c.entry) {
    case 0x1d9330:
      check(halo2::source_group_physics_bodies_1_1(m,
        {c.args[0],c.args[1],c.args[2],c.args[3]},c.scratch,fp,estimate,observe,observe_call)==c.result,c.name+" selected request");break;
    case 0x20e370:
      check(halo2::source_physics_body_runtime_context_1_1(m,c.eax,c.esi,fp,observe)==((c.result&255u)!=0),c.name+" actual low AL");break;
    case 0x183500:
      halo2::source_physics_body_groups_1_1(m,c.args[0],c.esi,c.args[1],c.scratch.region_result,c.scratch.permutation_result,observe);
      check(c.esi==c.result,c.name+" group bank pointer");break;
    case 0x183770:
      halo2::source_physics_body_region_permutation_1_1(m,c.eax,c.ecx,c.args[0],c.args[1],c.scratch.region_result,c.scratch.permutation_result,observe);break;
    default:throw std::runtime_error("Unmeasured original retained grouping owner");
    }
    check(estimate_index==c.estimates.size(),c.name+" original reciprocal order");
    check(factory_inputs==c.factory_inputs,c.name+" complete original factory argument words/order");
    auto equal=[&](std::size_t n) {
      const auto&a=stores[n];const auto&b=c.stores[n];
      return a.instruction==b.instruction&&a.address==b.address&&a.written==b.written&&
        (c.temporary[n]||a.before==b.before);
    };
    std::size_t n=0;while(n<stores.size()&&n<c.stores.size()&&equal(n))++n;
    if(n!=stores.size()||n!=c.stores.size()) {
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
  check(cases.size()==8,"actual mode7/mode4 grouping and context/region owners");
  // The real positive fixture must stop at its required platform instruction
  // provider. An absent estimate cannot admit a body with an invented value.
  {
    const auto&c=cases.front();auto m=c.before;bool stopped=false;
    try {
      halo2::source_group_physics_bodies_1_1(m,
        {c.args[0],c.args[1],c.args[2],c.args[3]},c.scratch,
        {static_cast<std::uint16_t>(c.cw),c.mxcsr},{});
    }catch(const std::runtime_error&e) {
      stopped=std::string(e.what())=="Required Xbox RSQRTSS estimate provider unavailable";
    }
    check(stopped,"genuine positive path requires reciprocal estimate owner");
  }
  std::cout<<cases.size()<<" actual original calls; "<<total_bytes<<" full retained bytes; "
           <<total_stores<<" ordered owning write packets match (retained before bytes exact; temporary ABI before bytes excluded)\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
