#include "halo2/native_tu15_kinematic_motion.hpp"
#include <algorithm>
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
std::vector<std::byte>bytes(const std::string&s){check(s.size()%2==0,"Invalid pose fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Fault{};
struct Memory:SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::vector<std::byte>>banks;
  bool log_reads{};mutable J reads=J::array();J fault;mutable unsigned fault_count{};
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing pose bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing pose extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{
    if(!fault.is_null()&&p==fault.at("address")&&b.size()==fault.at("bytes")&&++fault_count==fault.at("occurrence"))throw Fault{};
    if(log_reads)reads.push_back(J::array({p,b.size()}));auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());
  }
  void read_vector(std::uint32_t p,std::span<std::byte,16>b)const override{read(p,b);}
  void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
std::uint16_t status(){std::uint16_t n;asm volatile("fnstsw %0":"=a"(n));return n;}
std::uint16_t control(){std::uint16_t n;asm volatile("fnstcw %0":"=m"(n));return n;}
}
int main(int argc,char**argv){try{
  check(argc==2&&std::filesystem::exists(argv[1]),"Required original pose fixture missing");std::ifstream input(argv[1]);const auto f=J::parse(input);
  check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Pose TU1.5 source authority changed");
  check(f.at("required_backend")=="unchanged-tu15-physical-i386-operands-stores-rsqrt-full-fp-v1","Pose physical original backend absent");
  check(f.at("body_sites")==J::array({86,58,31,69,62,1,1}),"Original pose helper closure changed");std::size_t count{},writes{},read_count{},faults{},estimates{};
  for(const auto&c:f.at("cases")){
    const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),name+" physical original not verified");Memory original;
    for(const auto&b:c.at("banks"))check(original.banks.emplace(b.at("address"),bytes(b.at("before"))).second,"Duplicate pose bank");original.fault=c.value("fault",J{});
    for(bool read_pass:{false,true}){
      auto m=original;m.log_reads=read_pass;std::vector<SourcePhysicsContactRecordStore>stores;
      SourcePhysicsContactRecordObserver observe;if(!read_pass)observe=[&](const auto&s){stores.push_back(s);};
      NativeTu15KinematicPoseMemory storage(m,c.at("motion"),c.at("step"),observe);NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
      NativeTu15MotionVariantActivity activity;J actual_estimates=J::array();bool failed=false;const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();
      const auto entry=c.at("entry").get<std::uint32_t>();
      try{
        if(entry==0x323730u)native_tu15_integrate_kinematic_motion_pose(storage,fp,0,[&](auto in,auto out){actual_estimates.push_back(J::array({in,out}));},&activity);
        else if(entry==0x8fdb0u)native_tu15_apply_prescribed_motion_forces(fp,&activity);
        else if(entry==0x2e4d60u)native_tu15_keep_fixed_motion_pose(fp,&activity);
        else throw std::runtime_error("Unauthenticated motion variant");
      }catch(const Fault&){failed=true;}
      check(activity.kinematic_pose_calls==unsigned(!failed&&entry==0x323730u)&&activity.prescribed_force_calls==unsigned(entry==0x8fdb0u)&&activity.fixed_pose_calls==unsigned(entry==0x2e4d60u),name+" native variant participation differs");
      check(failed==!c.value("fault",J{}).is_null(),name+" read-fault prefix completion differs");
      check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+" leaked host FP environment");
      check(actual_estimates==c.at("rsqrt_estimates"),name+" native RSQRTSS estimate differs from physical original");
      if(read_pass){J expected_reads=J::array();for(const auto&r:c.at("reads"))expected_reads.push_back(J::array({r.at("address"),r.at("bytes")}));
        if(m.reads!=expected_reads){std::size_t n=0;while(n<m.reads.size()&&n<expected_reads.size()&&m.reads[n]==expected_reads[n])++n;throw std::runtime_error(name+" source read order differs at "+std::to_string(n)+" native "+(n<m.reads.size()?m.reads[n].dump():"end")+" original "+(n<expected_reads.size()?expected_reads[n].dump():"end"));}
        read_count+=m.reads.size();
      }else{
        const auto&w=c.at("writes");check(stores.size()==w.size(),name+" ordered pose store count differs "+std::to_string(stores.size())+"/"+std::to_string(w.size()));
        for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=w[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered pose store differs at "+std::to_string(n));}writes+=stores.size();
      }
      for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" complete retained pose bytes differ");
      const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" complete pose FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+end.dump());
    }
    ++count;faults+=!c.value("fault",J{}).is_null();estimates+=c.at("rsqrt_estimates").size();
  }

  NativeTu15PhysicsFrameFp untouched{0x27f,0x4501,0x1fa0};const auto before=untouched;NativeTu15MotionVariantActivity inactive;
  const auto host_cw=control(),host_sw=status();const auto host_mx=_mm_getcsr();
  native_tu15_apply_prescribed_motion_forces(untouched,&inactive);native_tu15_keep_fixed_motion_pose(untouched,&inactive);
  check(untouched.control_word==before.control_word&&untouched.status==before.status&&untouched.mxcsr==before.mxcsr&&control()==host_cw&&status()==host_sw&&_mm_getcsr()==host_mx,"Authenticated return-only variant changed state/FP");
  check(inactive.prescribed_force_calls==1&&inactive.fixed_pose_calls==1,"Native return-only participation absent");
  check(count>0,"Empty original pose fixture");std::cout<<count<<" original physical TU1.5 pose cases, "<<writes<<" ordered stores, "<<read_count<<" ordered reads, "<<faults<<" read-fault prefixes, "<<estimates<<" native RSQRTSS observations passed; Xbox/PentiumIII estimate bits and full live closure remain separate\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
