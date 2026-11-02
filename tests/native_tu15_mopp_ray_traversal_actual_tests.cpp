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
std::vector<std::byte>file(const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);check(bool(in),"Missing owned MOPP bank");std::vector<char>raw((std::istreambuf_iterator<char>(in)),{});std::vector<std::byte>b(raw.size());std::transform(raw.begin(),raw.end(),b.begin(),[](char c){return std::byte(std::uint8_t(c));});return b;}
struct Memory:SourcePhysicsContactMemory{
 std::map<std::uint32_t,std::vector<std::byte>>banks;bool log{};mutable J reads=J::array();
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing MOPP bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing MOPP extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(log)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
}
int main(int argc,char**argv)try{
 check(argc==2,"Actual MOPP fixture missing");const auto path=std::filesystem::path(argv[1]);std::ifstream input(path);const auto fixture=J::parse(input);check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original MOPP authority");std::size_t count{},total{},read_count{};
 for(const auto&c:fixture.at("cases")){
  Memory m;for(const auto&b:c.at("banks"))m.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));NativeTu15MoppTraversalVariables v;NativeTu15MoppTraversalBounds bounds;std::array<std::byte,32>vb{},bb{};m.read(c.at("variables"),vb);m.read(c.at("bounds"),bb);std::memcpy(&v,vb.data(),32);std::memcpy(&bounds,bb.data(),32);m.log=true;
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();NativeTu15MoppRayTraversalStats activity;
  native_tu15_traverse_mopp_ray(m,c.at("receiver"),c.at("code"),v,bounds,fp,0,{}, {},&activity);
  check(activity.calls==1&&activity.completed==1&&activity.refusals==0&&activity.opcodes>0&&activity.leaves==0&&activity.filters==0&&activity.child_shapes==0,"Actual no-leaf MOPP native activity differs");check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,"Actual native MOPP leaked host FP");
  const auto variables=c.at("variables").get<std::uint32_t>(),bounds_address=c.at("bounds").get<std::uint32_t>();std::size_t dispatches{},targets{};J expected=J::array();for(const auto&r:c.at("reads")){const auto p=r.at("address").get<std::uint32_t>();const auto pc=r.at("instruction").get<std::uint32_t>();if(pc==0x30aaa0u){check(p>=0x30b738u&&p<0x30b738u+108u&&r.at("bytes")==1,"Unauthenticated opcode CODE table read");++dispatches;continue;}if(pc==0x30aaa7u){check(p>=0x30b6b8u&&p<0x30b6b8u+128u&&(p-0x30b6b8u)%4u==0&&r.at("bytes")==4,"Unauthenticated handler CODE table read");++targets;continue;}if(p>=variables&&p<variables+32u||p>=bounds_address&&p<bounds_address+32u)continue;expected.push_back(J::array({r.at("address"),r.at("bytes")}));}check(dispatches==activity.opcodes&&targets==dispatches,"Actual native MOPP dispatch count differs");
  if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error("Actual MOPP ordered read differs at "+std::to_string(n)+" native="+(n<m.reads.size()?m.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();m.log=false;m.write(bounds_address,std::as_bytes(std::span(&bounds,1)));
  for(const auto&b:c.at("banks")){const auto address=b.at("address").get<std::uint32_t>();const auto expected=file(path.parent_path()/b.at("after_file").get<std::string>());check(m.banks.at(address)==expected,"Actual complete MOPP bank differs at "+std::to_string(address));if(!count)total+=expected.size();}
  check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),"Actual complete MOPP FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+c.at("after_fp").dump());++count;
 }check(count==4,"Incomplete actual MOPP FP coverage");std::cout<<J{{"passed",true},{"profiles",count},{"complete_bytes",total},{"ordered_reads",read_count},{"supplied_child_results",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
