#include "halo2/source_lift_scalar.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <xmmintrin.h>
using namespace halo2;
namespace {
using J=nlohmann::json;
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
float number(const J&j,const char*name){return std::bit_cast<float>(j.at(name).get<std::uint32_t>());}
std::uint32_t word(const J&banks,std::uint32_t address){
  std::uint32_t value{};
  for(unsigned n=0;n<4;++n){
    bool found{};
    for(const auto&b:banks){const auto base=b.at("address").get<std::uint32_t>();const auto raw=b.at("hex").get<std::string>();
      if(address+n>=base&&std::uint64_t(address+n-base)*2+2<=raw.size()){
        value|=std::uint32_t(std::stoul(raw.substr(std::size_t(address+n-base)*2,2),nullptr,16))<<(8*n);found=true;break;
      }
    }
    check(found,"Original scalar result bank is incomplete");
  }
  return value;
}
struct PowerRequired{float base,exponent,acceleration;};
}
int main(int argc,char**argv)try{
  check(argc==2,"Supply independently executed TU1.5 scalar fixture");
  std::ifstream input(argv[1]);const auto fixture=J::parse(input);check(fixture.at("image")=="1.5","TU1.5 only");
  check(fixture.at("oracle_backend")=="original_i386_hardware_single_step","Independent original CPU result required");
  const auto words=fixture.at("scalar_constants").get<std::array<std::uint32_t,5>>();
  const SourceLiftScalarConstants constants{std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2]),std::bit_cast<float>(words[3]),std::bit_cast<float>(words[4])};
  std::size_t complete{},power_prefixes{},sqrt_cases{},status_cases{};
  for(const auto&c:fixture.at("cases")){
    const auto&row=c.at("native_input");const auto&exit=c.at("exit");
    const SourceLiftScalarInput values{row.at("allow_reverse"),row.at("scale_by_mass"),row.at("force_acceleration"),
      number(row,"mass"),number(row,"target"),number(row,"acceleration"),number(row,"velocity"),number(row,"velocity_limit"),number(row,"position"),number(row,"exponent"),number(row,"time_step")};
    const SourceLiftScalarEnvironment fp{row.at("x87_control_word"),row.at("mxcsr"),row.at("x87_exception_flags"),c.at("entry_abi").at("x87_status")};
    std::uint16_t old_cw,old_status;asm volatile("fnstcw %0; fnstsw %1":"=m"(old_cw),"=am"(old_status));const auto old_mxcsr=_mm_getcsr();
    if(!c.at("complete").get<bool>()){
      check(c.at("exit_entry")==0x37ce0cu,"Unexpected original scalar owner boundary");bool missing{};
      try{(void)source_lift_scalar_tu15(values,constants,fp);}catch(const std::runtime_error&e){missing=std::string(e.what())=="Lift scalar requires original positive-exponent power owner";}
      check(missing,"Native scalar supplied an unproved power result");bool requested{};
      try{(void)source_lift_scalar_tu15(values,constants,fp,[](float a,float b,float d)->float{throw PowerRequired{a,b,d};});}
      catch(const PowerRequired&request){
        const auto stack=c.at("entry_abi").at("gp")[4].get<std::uint32_t>();
        check(std::bit_cast<std::uint32_t>(request.base)==word(c.at("final_banks"),stack-4),"Native scalar power base differs from original prefix");
        check(std::bit_cast<std::uint32_t>(request.exponent)==row.at("exponent").get<std::uint32_t>()&&std::bit_cast<std::uint32_t>(request.acceleration)==row.at("acceleration").get<std::uint32_t>(),"Native scalar power parameters differ");requested=true;
      }
      check(requested,"Native scalar omitted its actual power dependency");++power_prefixes;
    }else{
      const auto result=source_lift_scalar_tu15(values,constants,fp);
      const auto expected_delta=std::stoul(exit.at("xmm")[0].get<std::string>().substr(24,8),nullptr,16);
      if(std::bit_cast<std::uint32_t>(result.result.velocity_delta)!=expected_delta||std::bit_cast<std::uint32_t>(result.result.velocity_ratio)!=word(c.at("final_banks"),0x1001000u)){
        std::cerr<<"case="<<c.at("id")<<" actual="<<std::hex<<std::bit_cast<std::uint32_t>(result.result.velocity_delta)<<","<<std::bit_cast<std::uint32_t>(result.result.velocity_ratio)<<" expected="<<expected_delta<<","<<word(c.at("final_banks"),0x1001000u)<<std::dec<<'\n';
        throw std::runtime_error("TU1.5 lift scalar result mismatch");
      }
      if(result.mxcsr!=exit.at("mxcsr").get<std::uint32_t>()||result.x87_status!=exit.at("x87_status").get<std::uint16_t>()||result.control_word!=exit.at("x87_control_word").get<std::uint16_t>()){
        std::cerr<<"case="<<c.at("id")<<" mxcsr="<<std::hex<<result.mxcsr<<"/"<<exit.at("mxcsr").get<std::uint32_t>()<<" x87="<<result.x87_status<<"/"<<exit.at("x87_status").get<std::uint16_t>()<<std::dec<<'\n';
        throw std::runtime_error("TU1.5 lift scalar numeric status mismatch");
      }
      sqrt_cases+=result.used_x87;++complete;++status_cases;
    }
    std::uint16_t current_cw,current_status;asm volatile("fnstcw %0; fnstsw %1":"=m"(current_cw),"=am"(current_status));
    check(old_cw==current_cw&&old_status==current_status&&old_mxcsr==_mm_getcsr(),"Native scalar leaked its numeric environment to its host caller");
  }
  check(complete>100&&power_prefixes>0&&sqrt_cases>0,"Incomplete native scalar coverage");
  std::cout<<J{{"passed",true},{"complete_original_calls",complete},{"required_power_prefixes",power_prefixes},{"sqrt_calls",sqrt_cases},{"numeric_status_cases",status_cases},{"native_semantic_kernel",true},{"live_force_integration_proven",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
