#include "halo2/native_tu15_instanced_scene_ray.hpp"
#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte>file(const std::filesystem::path&p){std::ifstream in(p,std::ios::binary);check(bool(in),"Missing actual instanced bank");std::vector<char>r((std::istreambuf_iterator<char>(in)),{});std::vector<std::byte>b(r.size());std::transform(r.begin(),r.end(),b.begin(),[](char c){return std::byte(std::uint8_t(c));});return b;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
constexpr std::uint32_t private_data=0x4100000u;
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>>banks;mutable J reads=J::array();bool log{};
 auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Unmapped actual instanced operand");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Unmapped actual instanced extent");return i;}
 void read(std::uint32_t p,std::span<std::byte>b)const override{if(log&&!(p>=private_data&&p<private_data+0x1000u))reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
 void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
 void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t cw(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
std::uint16_t sw(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
}
int main(int argc,char**argv)try{
 check(argc==2,"Missing actual instanced fixture");const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);std::size_t profiles{},read_count{},store_count{},byte_count{};
 for(const auto&c:fixture.at("cases"))for(bool read_pass:{false,true}){
  Memory m;for(const auto&b:c.at("banks"))m.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));check(!m.banks.contains(private_data),"Private native DATA overlaps reference");m.banks.emplace(private_data,std::vector<std::byte>(0x1000,std::byte{0xa5}));m.log=read_pass;J stores=J::array();SourcePhysicsContactRecordObserver observer;
  if(!read_pass)observer=[&](const auto&e){if(e.address>=private_data&&e.address<private_data+0x1000u)return;stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});};
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};const auto host_cw=cw(),host_sw=sw();const auto host_mx=_mm_getcsr();NativeTu15InstancedSceneRayStats activity;NativeTu15MoppRayTraversalStats core;NativeTu15InstancedSceneRayServices services;
  services.traversal.traverse=[&](SourcePhysicsContactMemory&guarded,auto cache,auto program,const auto&v,const auto&bounds,auto&current,auto depth){auto local_bounds=bounds;native_tu15_traverse_mopp_ray(guarded,cache,program,v,local_bounds,current,depth,{},observer,&core);};
  const auto q=c.at("request");const bool result=native_tu15_query_instanced_scene_ray(m,{q["cluster"],q["mask"],q["bsp_filter"],q["origin"],q["displacement"],q["result"]},{private_data},fp,0,services,observer,&activity);
  check(activity.calls==1&&activity.completed==1&&activity.refusals==0&&activity.positive_clusters==1&&activity.shape_initializations==1&&activity.shape_casts==1&&activity.shape_destructions==1&&activity.setup.completed==1&&activity.setup.unprovided_reads==0&&activity.setup.unprovided_writes==0&&core.calls==1&&core.completed==1&&core.leaves==0&&core.unbound_services==0,"Actual native instanced activity differs");
  check(result==bool(c.at("return_word").get<unsigned>()&255u),"Actual native instanced return differs");check(cw()==host_cw&&sw()==host_sw&&_mm_getcsr()==host_mx,"Actual native instanced leaked host FP");check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),"Actual native instanced FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+c.at("after_fp").dump());
  if(read_pass){J expected=J::array();std::size_t dispatches{},targets{};for(const auto&r:c.at("reads")){auto pc=r.at("instruction").get<unsigned>();if(pc==0x30aaa0u){++dispatches;continue;}if(pc==0x30aaa7u){++targets;continue;}expected.push_back(J::array({r.at("address"),r.at("bytes")}));}check(dispatches==core.opcodes&&targets==dispatches,"Instanced immutable dispatch count differs");if(m.reads!=expected){std::size_t n{};while(n<m.reads.size()&&n<expected.size()&&m.reads[n]==expected[n])++n;throw std::runtime_error("Instanced ordered read differs at "+std::to_string(n)+" native="+(n<m.reads.size()?m.reads[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}read_count+=m.reads.size();}
  else{check(stores==c.at("writes"),"Actual persistent instanced stores differ "+stores.dump()+"/"+c.at("writes").dump());store_count+=stores.size();}
  for(const auto&b:c.at("banks")){const auto address=b.at("address").get<unsigned>();const auto expected=file(path.parent_path()/b.at("after_file").get<std::string>());check(m.banks.at(address)==expected,"Actual complete instanced bank differs at "+std::to_string(address));if(profiles==0&&!read_pass)byte_count+=expected.size();}if(read_pass)++profiles;
 }
 check(profiles==4,"Actual instanced FP profiles missing");std::cout<<J{{"passed",true},{"profiles",profiles},{"complete_bytes",byte_count},{"ordered_reads",read_count},{"persistent_stores",store_count},{"source_private_bytes_compared",false},{"captured_bank_exclusions",J::array()}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
