#include "physics/vehicles/physics_vehicle_motion_internal.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool ok,const std::string&why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte>bytes(const std::string&text){std::vector<std::byte>value;for(unsigned n=0;n<text.size();n+=2)value.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return value;}
std::string hex(const void*data,unsigned size){const auto*p=static_cast<const unsigned char*>(data);const char*d="0123456789abcdef";std::string out;for(unsigned n=0;n<size;++n){out+=d[p[n]>>4];out+=d[p[n]&15];}return out;}
struct Memory:SourcePhysicsContactMemory {
 void read(std::uint32_t,std::span<std::byte>)const override{throw std::runtime_error("Register FSTP unexpectedly read memory");}
 void write(std::uint32_t,std::span<const std::byte>)override{throw std::runtime_error("Register FSTP unexpectedly wrote memory");}
};
struct Host {
 native_raw_x87_detail::Image saved=native_raw_x87_detail::capture();
 Host(){auto empty=saved;empty.word(8,0xffffu);empty.word(4,0);native_raw_x87_detail::restore(empty);}
 ~Host(){native_raw_x87_detail::restore(saved);}
};
}
int main(int argc,char**argv)try{
 check(argc==3,"Supply unchanged FSTP fixture and result");std::ifstream input(argv[1]);const auto fixture=J::parse(input);unsigned cases{},c1_set{};
 for(const auto&row:fixture.at("cases")){
  Host host;Memory memory;const SourcePhysicsContactRecordObserver observe;SourcePhysicsWorldRuntimeServices services;
  physics_vehicle_motion_detail::MotionState state{memory,observe,services,row.at("image")=="MapPack11"?SourcePhysicsContactImage::MapPack11:SourcePhysicsContactImage::TitleUpdate15};
  SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp=row.at("gp");incoming.eflags=row.at("eflags");incoming.xmm=row.at("xmm");incoming.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};incoming.x87_status=row.at("before_fp")[1];incoming.x87_depth=row.at("live_x87").size();
  for(unsigned n=0;n<incoming.x87_depth;++n){const auto raw=bytes(row.at("live_x87")[n]);std::memcpy(&incoming.x87[n],raw.data(),10);}
  state.load(incoming);const auto target=row.at("target").get<unsigned>();
  // Exact current generator expression. The original-byte/opcode proof owns
  // target selection; the candidate receives only the declared incoming ABI.
  state.fp.st(target)=state.fp.st(0);
#ifndef HALO2_MOTION_FST_KEEP_C1
  state.fp.status&=~0x200u;
#endif
  state.fp.pop();const auto after=state.save();
  const auto name=row.at("name").get<std::string>();
  check(J(after.gp)==row.at("expected_gp")&&after.eflags==row.at("expected_flags")&&J(after.xmm)==row.at("expected_xmm"),name+": register FSTP changed GP/flags/SIMD");
  check(J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})==row.at("expected_fp"),name+": register FSTP FP status differs");
  J values=J::array();for(unsigned n=0;n<after.x87_depth;++n)values.push_back(hex(&after.x87[n],10));
  check(values==row.at("expected_live80"),name+": register FSTP full80/topology differs");
  ++cases;if(incoming.x87_status&0x200u)++c1_set;
 }
 const J report{{"passed",true},{"cases",cases},{"incoming_c1_set",c1_set},{"full_gp_flags_xmm_fp_live80",true}};std::ofstream output(argv[2]);output<<report.dump()<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
