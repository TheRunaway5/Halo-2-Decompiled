#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>bytes(const std::string&s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
void disturb_provider_fp(){
 const std::uint16_t control=0x0b7f;
 asm volatile("fninit;fld1;fldpi;fldz;fldcw %0"::"m"(control):"memory","st","st(1)","st(2)");
 _mm_setcsr(0x9fc0u);
}
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;bool log{},hostile_fp{};mutable J reads=J::array();J fault;mutable unsigned occurrence{};
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing MOPP bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing MOPP extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(log&&hostile_fp)disturb_provider_fp();if(log&&!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++occurrence==fault.at("occurrence"))throw Fault{};if(log)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{if(log&&hostile_fp)disturb_provider_fp();auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv){try{
 check(argc==2,"MOPP fixture missing");std::ifstream file(argv[1]);const auto fixture=J::parse(file);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("required_backend")=="unchanged-tu15-native-mopp-ray-traversal-v1"&&fixture.at("body_sites")==J::array({896}),"Wrong MOPP authority");std::size_t count{},reads{};
 for(const auto&c:fixture.at("cases"))for(const bool hostile_fp:{false,true}){
  const auto name=c.at("name").get<std::string>()+(hostile_fp?" provider-FP-clobber":"");Memory m;m.hostile_fp=hostile_fp;for(const auto&b:c.at("banks"))m.banks.emplace(b.at("address"),bytes(b.at("before")));m.fault=c.value("fault",J{});
  NativeTu15MoppTraversalVariables v;NativeTu15MoppTraversalBounds bounds;
  std::array<std::byte,32>vb{},bb{};m.read(c.at("variables"),vb);m.read(c.at("bounds"),bb);std::memcpy(&v,vb.data(),32);std::memcpy(&bounds,bb.data(),32);m.log=true;
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15MoppRayTraversalServices services;bool boundary_hit{};
  const auto boundary=[&](std::uint32_t owner,std::uint32_t receiver,std::uint32_t key){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("owner")==owner&&b.at("receiver")==receiver&&b.at("key")==key,name+" actual MOPP service formals differ "+J::array({owner,receiver,key}).dump()+"/"+b.dump());boundary_hit=true;throw std::runtime_error("actual-mopp-boundary");};
  services.child_shape=[&](auto collection,auto owner,auto key,auto&)->NativeTu15MoppChildShape{boundary(owner,collection,key);return{};};
  services.filter=[&](auto receiver,auto owner,auto ray,auto collection,auto key,auto&){const auto&b=c.at("boundary");check(b.at("ray")==ray&&b.at("collection")==collection,name+" MOPP filter formals differ");boundary(owner,receiver,key);return false;};
  const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();NativeTu15MoppRayTraversalStats stats;bool failed{};
  try{native_tu15_traverse_mopp_ray(m,c.at("receiver"),c.at("code"),v,bounds,fp,0,services,{},&stats);}catch(const Fault&){failed=true;}catch(const std::runtime_error&e){if(std::string(e.what())!="actual-mopp-boundary")throw;}
  check(failed==c.contains("fault")&&boundary_hit==!c.at("boundary").is_null(),name+" MOPP completion differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" native MOPP leaked host FP");
  const auto variables=c.at("variables").get<std::uint32_t>(),bounds_address=c.at("bounds").get<std::uint32_t>();
  J expected=J::array();for(const auto&r:c.at("reads")){const auto p=r.at("address").get<std::uint32_t>();if(p>=variables&&p<variables+32u||p>=bounds_address&&p<bounds_address+32u)continue;expected.push_back(J::array({r.at("address"),r.at("bytes")}));}
  if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error(name+" ordered MOPP read differs at "+std::to_string(n)+" native="+(n<m.reads.size()?m.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}reads+=m.reads.size();
  // Materialize the independently computed full typed arithmetic output for
  // byte comparison. It is not read from the original expected state.
  m.log=false;m.write(bounds_address,std::as_bytes(std::span(&bounds,1)));
  for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained MOPP bytes differ");const auto&after=c.at("after_fp");check(fp.control_word==after[0]&&fp.status==after[1]&&fp.mxcsr==after[2],name+" complete MOPP FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+after.dump());++count;
 }std::cout<<count<<" original/native MOPP cases and "<<reads<<" strict nonprivate reads passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
