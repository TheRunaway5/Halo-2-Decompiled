#include "halo2/source_lens_draw_tu15.hpp"
#include "source_lens_cpu_cache_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_vehicle_motion_math.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <cstdlib>
using namespace halo2;
namespace {
bool sdk(unsigned p) {return (p>=0x40eeb0u&&p<0x411800u)||(p>=0x4c4b00u&&p<0x4c4d00u)||(p>=0x45c488u&&p<0x45c548u)||(p>=0x45f290u&&p<0x45f2c0u);}
std::map<unsigned,std::set<unsigned>> required_values;
std::set<unsigned> all_reads,all_writes;
unsigned instruction;
std::map<std::pair<unsigned,unsigned>,unsigned> read_sites;
struct TrackingMemory final:SourcePhysicsContactMemory {
 SourceRuntimeMemory storage;mutable std::set<unsigned>written;
 std::vector<private_lens_cpu::Region> regions=private_lens_cpu::mcc_cache_plan();
 std::map<unsigned,std::byte> unbacked_initial;
 bool covered(unsigned p)const {for(const auto&r:regions)if(p>=r.address&&std::uint64_t(p)<std::uint64_t(r.address)+r.bytes.size())return true;return false;}
 TrackingMemory(){for(const auto&r:regions)storage.mount(r.address,r.bytes,r.access,r.role);}
 void mount(unsigned p,std::vector<std::byte> data,SourceRuntimeMemory::Access access,const char*name){
  std::size_t begin{};
  while(begin<data.size()){
   if(sdk(p+begin)){const auto at=p+unsigned(begin);if(covered(at)){
     if(storage.is_read_only(at,1)){std::array<std::byte,1> got;storage.read(at,got);if(got[0]!=data[begin])throw std::runtime_error("Fixture differs from authenticated readonly cache/table input");}
     else storage.write(at,std::span<const std::byte>(data).subspan(begin,1));
    }else unbacked_initial.emplace(at,data[begin]);++begin;continue;}
   std::size_t end=begin+1;while(end<data.size()&&!sdk(p+end))++end;
   storage.mount(p+unsigned(begin),std::vector<std::byte>(data.begin()+begin,data.begin()+end),access,name);begin=end;
  }
 }
 // Only the diagnostic final-bank serializer may reconstruct original fixture
 // padding. It is NOT supplied to original kernels: read/write below always
 // use strict actual sparse storage and fail on any omitted SDK byte.
 std::vector<std::byte> view(unsigned p,std::size_t n)const {std::vector<std::byte> out(n);for(std::size_t i=0;i<n;++i){if(const auto at=unbacked_initial.find(p+i);at!=unbacked_initial.end())out[i]=at->second;else storage.read(p+i,std::span(out).subspan(i,1));}return out;}
 void read(unsigned p,std::span<std::byte> out)const override {storage.read(p,out);for(std::size_t i=0;i<out.size();++i)if(sdk(p+i)){all_reads.insert(p+i);++read_sites[{instruction,p+i}];if(!written.contains(p+i))required_values[p+i].insert(std::to_integer<unsigned>(out[i]));}}
 void write(unsigned p,std::span<const std::byte> in)override {storage.write(p,in);for(std::size_t i=0;i<in.size();++i)if(sdk(p+i)){written.insert(p+i);all_writes.insert(p+i);}}
};
struct Boundary {};
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex){std::vector<std::byte>b;for(unsigned n=0;n<hex.size();n+=2)b.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
  check(argc==2 || argc==3,"Supply original lens draw fixture and optional authenticated math correspondence");std::ifstream file(argv[1]);if(!file)return 77;
  std::map<unsigned,unsigned> math_instructions;
  if(argc==3) {
    std::ifstream math_file(argv[2]);check(bool(math_file),"Original CRT math correspondence absent");
    const auto math=nlohmann::json::parse(math_file);
    check(math.at("later_owned_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original CRT correspondence belongs to another image");
    for(const auto &[key,value]:math.at("instruction_map").items())math_instructions.emplace(std::stoul(key,nullptr,16),value.get<unsigned>());
  }
  const auto fixture=nlohmann::json::parse(file);unsigned cases{};
  const bool bad_coordinate=std::getenv("HALO2_LENS_REJECT_COORDINATE_ONE")!=nullptr;
  for(const auto &c:fixture.at("cases")) {
    const auto expected_instructions=c.at("instructions").get<std::vector<unsigned>>();
    if(bad_coordinate && std::ranges::find(expected_instructions,0x409f30u)==expected_instructions.end())continue;
    TrackingMemory m;
    for(const auto &reference:c.at("initial_banks")) {
      const auto &b=fixture.at("banks").at(reference.get<unsigned>());
      auto data=b.contains("hex")?bytes(b.at("hex")):std::vector<std::byte>(b.at("zero_bytes").get<unsigned>());
      if(b.contains("patches"))for(const auto &patch:b.at("patches")) {
        const auto value=bytes(patch.at("hex"));const auto offset=patch.at("offset").get<unsigned>();
        check(offset<=data.size() && value.size()<=data.size()-offset,"Scratch fixture patch exceeds its declared bank");
        std::copy(value.begin(),value.end(),data.begin()+offset);
      }
      m.mount(b.at("address"),std::move(data),SourceRuntimeMemory::Access::ReadWrite,"declared original lens draw ABI bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};abi.gp=c.at("entry_gp").get<decltype(abi.gp)>();abi.eflags=0x202;
    std::vector<std::uint32_t> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){instruction=p;trace.push_back(p);};
    services.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &observe){
      if(c.contains("boundary_target") && call.target==c.at("boundary_target").get<unsigned>())throw Boundary{};
      if(source_lens_draw_tu15_owns_entry(call.target))
        source_run_lens_draw_tu15(memory,call.target,state,{0x27f,0x1f80},services,observe);
      else if(call.target==0x37dc0a) {
        auto fp=state.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80});
        auto math_services=services;math_services.instruction=[&](auto pc){trace.push_back(math_instructions.at(pc));};
        source_run_physics_vehicle_motion_math_runtime(memory,0x37a15a,state,
            SourcePhysicsContactImage::TitleUpdate15,fp,math_services,observe);
      }
      else {std::cerr<<"case="<<cases<<" target="<<std::hex<<call.target<<std::dec<<'\n';throw std::runtime_error("Lens draw reached an unowned original callee");}
    };
    if(bad_coordinate) {const std::array<std::byte,4> one{std::byte{1},std::byte{},std::byte{},std::byte{}};m.storage.write(0x40ef20u,one);}
    bool boundary{};
    try{source_run_lens_draw_tu15(m,c.value("entry",fixture.at("entry").get<unsigned>()),abi,{0x27f,0x1f80},services);}
    catch(const Boundary&){boundary=true;}
    catch(const std::exception&e){if(!bad_coordinate)throw;check(instruction==0x409f30u && std::string(e.what()).find("0x411704")!=std::string::npos,"Unsupported coordinate failed outside exact original lookup");std::cout<<nlohmann::json{{"status","GREEN unsupported coordinate1 remains unbacked"},{"original_read_pc",instruction},{"missing_address",0x411704u},{"exception",e.what()}}.dump(2)<<'\n';return 0;}
    check(!bad_coordinate,"Unsupported coordinate1 incorrectly received SDK table backing");
    check(boundary==c.contains("boundary_target"),"Original lens parent boundary differs");
    check(abi.gp==c.at("exit_gp").get<decltype(abi.gp)>(),"Original lens draw GP output differs");
    check((abi.eflags&c.value("eflags_mask",0xcd5u))==(c.at("eflags").get<unsigned>()&c.value("eflags_mask",0xcd5u)),"Original lens draw flags differ");
    if(trace!=c.at("instructions").get<decltype(trace)>())std::cerr<<"case="<<cases<<" variant="<<c.at("variant")<<'\n';
    check(trace==c.at("instructions").get<decltype(trace)>(),"Original lens draw instruction path differs");
    for(unsigned n=0;n<8;++n){const auto hex=c.at("exit_xmm")[n].get<std::string>();for(unsigned k=0;k<4;++k)check(abi.xmm[n][k]==std::stoul(hex.substr(24-k*8,8),nullptr,16),"Original lens draw SSE output differs");}
    for(const auto &reference:c.at("final_banks")){const auto &b=fixture.at("banks").at(reference.get<unsigned>());const auto expected=bytes(b.at("hex"));const auto actual=m.view(b.at("address"),expected.size());if(!std::ranges::equal(actual,expected)){const auto mismatch=std::ranges::mismatch(actual,expected);std::cerr<<"case="<<cases<<" address="<<std::hex<<b.at("address").get<unsigned>()+std::distance(actual.begin(),mismatch.in1)<<" got="<<unsigned(*mismatch.in1)<<" expected="<<unsigned(*mismatch.in2)<<std::dec<<"\n";throw std::runtime_error("Original lens draw retained memory differs");}}
    check(abi.x87_depth==0,"Lens draw retained an unconsumed x87 value");
    check(abi.x87_status==c.at("x87_status").get<unsigned>(),"Lens draw changed original x87 status");
    check(abi.floating_point && abi.floating_point->mxcsr==c.at("mxcsr").get<unsigned>(),"Lens draw changed original MXCSR");
    ++cases;
  }
  nlohmann::json report;report["status"]="PASS original fixture ABI/instruction/final-memory with only sparse SDK backing";report["cases"]=cases;report["fixture"]=argv[1];report["required_before_write"]=nlohmann::json::array();
  for(const auto&[at,values]:required_values)report["required_before_write"].push_back({{"address",at},{"values",values}});
  report["read_sites"]=nlohmann::json::array();for(const auto&[key,n]:read_sites)report["read_sites"].push_back({{"pc",key.first},{"address",key.second},{"count",n}});
  report["all_sdk_reads"]=all_reads;report["all_sdk_writes"]=all_writes;std::cout<<report.dump(2)<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
