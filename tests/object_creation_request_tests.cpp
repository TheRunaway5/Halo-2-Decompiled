#include "halo2/object_creation_request.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool value,const std::string &message) {
  if(!value)throw std::runtime_error(message);
}
std::uint32_t number(const std::string &s) {
  return std::uint32_t(std::stoul(s,nullptr,16));
}
std::vector<std::byte> raw(const std::string &s) {
  require(s.size()%2==0,"Odd raw hex extent");std::vector<std::byte> result;
  for(std::size_t i=0;i<s.size();i+=2)result.push_back(std::byte(number(s.substr(i,2))));
  return result;
}
std::uint32_t word(const std::string &s) {
  const auto bytes=raw(s);require(bytes.size()<=4,"Raw DWORD extent");
  std::uint32_t value=0;
  for(unsigned i=0;i<bytes.size();++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return value;
}
struct Region {std::uint32_t address;std::vector<std::byte> bytes;};
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    std::string line;std::getline(input,line);
    require(line=="OBJECT_CREATION_REQUEST_V1","Fixture schema");
    halo2::ObjectCreationRequestOperands constants{};
    std::uint32_t pool=0,target=0,definition=0,owner=0,location=0,mask=0,expected_return=0;
    std::string name,operation;std::vector<Region> before,after;
    std::vector<halo2::ObjectCreationRequestStore> stores;
    unsigned cases=0,writes=0;std::size_t compared=0;
    while(std::getline(input,line)) {
      std::istringstream row(line);std::string kind;row>>kind;
      if(kind=="IMAGE") {
        std::string digest,p,t,one,forward,up;row>>digest>>p>>t>>one>>forward>>up;
        require(digest.size()==64,"Image pin extent");pool=number(p);target=number(t);
        constants.one_bits=word(one);const auto f=raw(forward),u=raw(up);
        require(f.size()==12&&u.size()==12,"Basis extent");
        std::copy(f.begin(),f.end(),constants.basis_forward.begin());
        std::copy(u.begin(),u.end(),constants.basis_up.begin());
      } else if(kind=="CASE") {
        std::string d,o,l,m,r;row>>name>>operation>>d>>o>>l>>m>>r;
        definition=number(d);owner=number(o);location=number(l);mask=number(m);expected_return=number(r);
        before.clear();after.clear();stores.clear();
      } else if(kind=="BEFORE"||kind=="AFTER") {
        std::string p,h;row>>p>>h;
        (kind=="BEFORE"?before:after).push_back({number(p),raw(h)});
      } else if(kind=="STORE") {
        std::string pc,p,b,a;unsigned n=0;row>>pc>>p>>n>>b>>a;
        stores.push_back({number(pc),number(p),n,word(b),word(a)});
      } else if(kind=="ENDCASE") {
        auto memory=before;
        halo2::ObjectCreationRequestServices services;
        services.memory=[&](std::uint32_t address,std::size_t n)->std::span<std::byte> {
          for(auto &region:memory)if(address>=region.address&&
              std::uint64_t(address)+n<=std::uint64_t(region.address)+region.bytes.size())
            return std::span(region.bytes).subspan(address-region.address,n);
          throw std::runtime_error(name+": Actual original backing unavailable");
        };
        std::vector<halo2::ObjectCreationRequestStore> actual;
        if(operation=="initialize")
          halo2::source_object_creation_request_initialize(target,definition,owner,location,pool,
              constants,services,[&](const auto &event){actual.push_back(event);});
        else {
          require(operation=="lookup","Operation identity");
          require(halo2::source_object_body_from_datum(pool,owner,mask,services)==expected_return,
              name+": Original salted typed body lookup differs");
        }
        require(actual==stores,name+": Complete original write sequence differs");
        require(memory.size()==after.size(),name+": Whole region count differs");
        for(std::size_t i=0;i<memory.size();++i) {
          require(memory[i].address==after[i].address&&memory[i].bytes==after[i].bytes,
              name+": Whole original region differs");compared+=memory[i].bytes.size();
        }
        ++cases;writes+=unsigned(actual.size());
      } else throw std::runtime_error("Unknown fixture row");
    }
    require(cases==62&&writes==902,"Original coverage changed");
    bool failed=false;
    try { (void)halo2::source_object_body_from_datum(pool,owner,mask,{}); }
    catch(const std::runtime_error &) {failed=true;}
    // The final high-water control is non-NONE and must read actual pool bytes.
    require(failed,"Absent actual storage silently became a lookup result");
    std::cout<<"PASS object request: "<<cases<<" original cases, "<<writes
             <<" stores, "<<compared<<" full compared bytes\n";
    return 0;
  } catch(const std::exception &e) {
    std::cerr<<e.what()<<'\n';return 1;
  }
}
