#include "halo2/source_lift_power_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
using namespace halo2;
namespace {
using J=nlohmann::json;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
float number(const J& row,const char* name){return std::bit_cast<float>(row.at(name).get<std::uint32_t>());}
std::uint32_t word(const J& banks,std::uint32_t address){
  std::uint32_t result{};
  for(unsigned n=0;n<4;++n){
    bool found{};
    for(const auto& bank:banks){
      const auto start=bank.at("address").get<std::uint32_t>();const auto bytes=bank.at("hex").get<std::string>();
      if(address+n>=start&&std::uint64_t(address+n-start)*2+2<=bytes.size()){
        result|=std::uint32_t(std::stoul(bytes.substr((address+n-start)*2,2),nullptr,16))<<(8*n);found=true;break;
      }
    }
    check(found,"Missing original power-result bytes");
  }
  return result;
}
struct HostEnvironment {
  std::uint16_t cw,status;std::uint32_t mxcsr;
  HostEnvironment():mxcsr(_mm_getcsr()){asm volatile("fnstcw %0; fnstsw %1":"=m"(cw),"=am"(status));}
  void verify()const{const HostEnvironment now;check(now.cw==cw&&now.status==status&&now.mxcsr==mxcsr,"Native power leaked its caller's floating-point environment");}
};
struct MathEnvironment {
  std::array<unsigned char,28> saved{};
  explicit MathEnvironment(std::uint16_t caller_control){
    asm volatile("fnstenv %0":"=m"(saved)::"memory");auto active=saved;
    const auto control=std::uint16_t((caller_control&0x300u)|0x7fu);std::memcpy(active.data(),&control,2);
    std::uint16_t status;std::memcpy(&status,active.data()+4,2);status&=~0x3fu;std::memcpy(active.data()+4,&status,2);
    asm volatile("fldenv %0"::"m"(active):"memory");
  }
  ~MathEnvironment(){asm volatile("fldenv %0"::"m"(saved):"memory");}
};
std::uint64_t raw64(const std::string& bytes){
  check(bytes.size()==16,"Expected original binary64 write");std::uint64_t result{};
  for(unsigned n=0;n<8;++n)result|=std::stoull(bytes.substr(n*2,2),nullptr,16)<<(8*n);
  return result;
}
}
int main(int argc,char** argv)try{
  check(argc==2,"Supply independently executed TU1.5 power fixture");std::ifstream stream(argv[1]);const auto fixture=J::parse(stream);
  check(fixture.at("image")=="1.5"&&fixture.at("oracle_backend")=="original_i386_hardware_single_step","Original TU1.5 physical CPU oracle required");
  const auto words=fixture.at("scalar_constants").get<std::array<std::uint32_t,5>>();
  const SourceLiftScalarConstants constants{std::bit_cast<float>(words[0]),std::bit_cast<float>(words[1]),std::bit_cast<float>(words[2]),std::bit_cast<float>(words[3]),std::bit_cast<float>(words[4])};
  std::size_t completed{},special{},range{},zero{},crt_double_results{};
  for(const auto& original:fixture.at("cases")){
    bool original_errno_owner{};
    for(const auto& site:original.at("instructions")){
      special+=site.get<std::uint32_t>()==0x37d948u;range+=site.get<std::uint32_t>()==0x3334f0u;
      original_errno_owner|=site.get<std::uint32_t>()==0x330179u;
    }
    const auto& row=original.at("native_input");const auto& exit=original.at("exit");
    const SourceLiftScalarInput input{row.at("allow_reverse"),row.at("scale_by_mass"),row.at("force_acceleration"),
      number(row,"mass"),number(row,"target"),number(row,"acceleration"),number(row,"velocity"),number(row,"velocity_limit"),number(row,"position"),number(row,"exponent"),number(row,"time_step")};
    const SourceLiftScalarEnvironment fp{row.at("x87_control_word"),row.at("mxcsr"),row.at("x87_exception_flags"),original.at("entry_abi").at("x87_status")};
    const HostEnvironment host;float power_value{};bool invoked{};
    std::int32_t native_errno=row.at("initial_errno"),explicit_errno=native_errno;
    try{
      const auto native=source_lift_scalar_tu15(input,constants,fp,[&](float base,float exponent,float acceleration){
        invoked=true;const HostEnvironment prefix;
        const auto explicit_result=source_lift_power_tu15(base,exponent,acceleration,
            {prefix.cw,prefix.mxcsr,std::uint8_t(prefix.status&0x3fu),prefix.status},{&explicit_errno});
        prefix.verify();power_value=source_lift_power_tu15_current(base,exponent,acceleration,{&native_errno});
        const HostEnvironment suffix;
        check(std::bit_cast<std::uint32_t>(explicit_result.acceleration)==std::bit_cast<std::uint32_t>(power_value)&&
          explicit_result.mxcsr==suffix.mxcsr&&explicit_result.x87_status==suffix.status&&suffix.cw==prefix.cw&&explicit_errno==native_errno,
          "Explicit native power environment differs from the current-environment seam");
        return power_value;
      });
      check(original.at("complete"),"Native power supplied an unproved original dependency");check(invoked,"Positive exponent omitted the native power owner");
      const auto stack=original.at("entry_abi").at("gp")[4].get<std::uint32_t>();
      const auto expected_errno=word(original.at("final_banks"),0x1006008u);
      check(std::bit_cast<std::uint32_t>(native_errno)==expected_errno,"TU1.5 native pow errno side effect mismatch");
      unsigned error_kind{};std::uint64_t error_input{};std::uint32_t error_value_address{};
      for(const auto& event:original.at("writes")){
        const auto site=event.at("instruction").get<std::uint32_t>();
        if(site==0x332036u)error_kind=std::stoul(event.at("written").get<std::string>().substr(0,2),nullptr,16);
        if(site==0x332050u){error_input=raw64(event.at("written"));error_value_address=event.at("address");}
      }
      if(error_kind){
        check(error_value_address,"Original CRT record has no binary64 value");std::int32_t crt_errno=row.at("initial_errno");double adjusted;
        {MathEnvironment math(fp.x87_control_word);adjusted=source_pow_math_error_tu15_current(error_kind,std::bit_cast<double>(error_input),fp.x87_control_word,{&crt_errno});}
        const auto expected=std::uint64_t(word(original.at("final_banks"),error_value_address))|(std::uint64_t(word(original.at("final_banks"),error_value_address+4))<<32);
        if(std::bit_cast<std::uint64_t>(adjusted)!=expected){
          std::cerr<<"case="<<original.at("id")<<" CRT value="<<std::hex<<std::bit_cast<std::uint64_t>(adjusted)<<" original="<<expected<<std::dec<<'\n';
          throw std::runtime_error("TU1.5 native CRT binary64 result mismatch");
        }
        check(std::bit_cast<std::uint32_t>(crt_errno)==expected_errno,"TU1.5 native CRT errno side effect mismatch");++crt_double_results;
      }
      if(std::bit_cast<std::uint32_t>(power_value)!=word(original.at("final_banks"),stack+32)){
        std::cerr<<"case="<<original.at("id")<<" native_power="<<std::hex<<std::bit_cast<std::uint32_t>(power_value)<<" original="<<word(original.at("final_banks"),stack+32)<<std::dec<<'\n';
        throw std::runtime_error("TU1.5 native power result mismatch");
      }
      check(std::bit_cast<std::uint32_t>(native.result.velocity_delta)==std::stoul(exit.at("xmm")[0].get<std::string>().substr(24,8),nullptr,16)&&
        std::bit_cast<std::uint32_t>(native.result.velocity_ratio)==word(original.at("final_banks"),0x1001000u),"TU1.5 native power/scalar composition result mismatch");
      if(native.mxcsr!=exit.at("mxcsr").get<std::uint32_t>()||native.x87_status!=exit.at("x87_status").get<std::uint16_t>()||native.control_word!=exit.at("x87_control_word").get<std::uint16_t>()){
        std::cerr<<"case="<<original.at("id")<<" native status="<<std::hex<<native.mxcsr<<","<<native.x87_status<<" original="<<exit.at("mxcsr").get<std::uint32_t>()<<","<<exit.at("x87_status").get<std::uint16_t>()<<std::dec<<'\n';
        throw std::runtime_error("TU1.5 native power/scalar numerical status mismatch");
      }
      ++completed;zero+=(std::bit_cast<std::uint32_t>(input.target)&0x7fffffffu)==0;
      if(original_errno_owner){
        bool required{};
        try{(void)source_lift_power_tu15(std::bit_cast<float>(word(original.at("final_banks"),stack-4)),input.exponent,input.acceleration,fp);}
        catch(const SourceMathErrorServiceRequiredTu15& error){
          check(std::bit_cast<std::uint32_t>(error.error_number)==expected_errno&&error.owner==0x330179u,"Native pow requested the wrong thread errno service");required=true;
        }
        check(required,"Native pow supplied undeclared thread errno storage");
      }
    }catch(const std::invalid_argument& error){
      std::cerr<<"case="<<original.at("id")<<" "<<error.what()<<'\n';throw;
    }
    host.verify();
  }
  check(completed>100&&zero&&special&&range&&crt_double_results,"Incomplete power numerical/error-boundary coverage");
  std::cout<<J{{"passed",true},{"complete_original_calls",completed},{"zero_base_calls",zero},{"special_argument_calls",special},{"crt_binary64_results",crt_double_results},{"full_power_parity_proven",false}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
