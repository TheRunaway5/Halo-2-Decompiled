#include "halo2/source_lift_power_tu15.hpp"
#include "halo2/source_power_special_tu15.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <xmmintrin.h>

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
std::uint16_t control(){std::uint16_t result;asm volatile("fnstcw %0":"=m"(result));return result;}
void load_control(std::uint16_t value){asm volatile("fldcw %0"::"m"(value):"memory");}
struct RestoreControl {
  std::uint16_t original{control()};
  ~RestoreControl(){load_control(original);}
};
struct Environment {
  alignas(16) std::array<unsigned char,108> saved{};
  unsigned mxcsr{_mm_getcsr()};
  explicit Environment(SourceLiftScalarEnvironment fp){
    if((fp.x87_control_word&0x3fu)!=0x3fu||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::invalid_argument("TU1.5 lift power requires masked native floating-point controls");
    asm volatile("fnsave %0":"=m"(saved)::"memory");auto active=saved;
    std::memcpy(active.data(),&fp.x87_control_word,2);
    const auto status=std::uint16_t((fp.x87_status&~0x3fu)|(fp.x87_exception_flags&0x3fu));
    std::memcpy(active.data()+4,&status,2);constexpr std::uint16_t empty=0xffffu;std::memcpy(active.data()+8,&empty,2);
    asm volatile("frstor %0"::"m"(active):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("frstor %0"::"m"(saved):"memory");_mm_setcsr(mxcsr);}
};
// Native x87 integer/parity test from 37CFD1. Fractional exponent rounding and
// its precision exception are part of zero-base behavior, even for +0.
unsigned integer_parity(long double exponent){
  std::uint16_t status;
  asm volatile("fldt %1; fld %%st(0); frndint; fcomp %%st(1); fnstsw %0"
      :"=am"(status):"m"(exponent):"st");
  if(!(status&0x4000u)){asm volatile("fstp %%st(0)":::"st");return 0;}
  constexpr double half=0.5;
  asm volatile("fmull %1; fld %%st(0); frndint; fcompp; fnstsw %0"
      :"=am"(status):"m"(half):"st");
  return status&0x4000u?2:1;
}
// The semantic algorithm is exp2(exponent*log2(base)): split the scaled
// logarithm into an integer and a remainder, exponentiate the remainder, then
// scale by the integer. These are actual native x87 arithmetic operations.
long double finite_power(double base,long double exponent){
  long double result;
  asm volatile("fldt %1; fldl %2; fyl2x; fld %%st(0); frndint; "
      "fsub %%st(0),%%st(1); fxch %%st(1); fchs; f2xm1; fld1; "
      "faddp %%st,%%st(1); fscale; fstp %%st(1); fstpt %0"
      :"=m"(result):"m"(exponent),"m"(base):"st");
  return result;
}
#endif
}
float source_lift_power_tu15_current_lazy(float base,float exponent,const std::function<float()>& load_acceleration,const SourceMathErrorServicesTu15& services){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  RestoreControl controls;
  if((controls.original&0x3fu)!=0x3fu)
    throw std::invalid_argument("TU1.5 lift power requires masked native floating-point controls");
  const auto exponent_word=std::bit_cast<std::uint32_t>(exponent);
  if((exponent_word&0x80000000u)||(exponent_word&0x7fffffffu)==0||
      ((exponent_word&0x7f800000u)==0x7f800000u&&(exponent_word&0x007fffffu)))
    throw std::invalid_argument("TU1.5 lift power requires the scalar's absolute base and ordered positive exponent");
  double converted_base,converted_exponent;long double extended_exponent;
  // The original thunk stores the base to binary64, stores the exponent to
  // binary64 without popping it, and retains its extended exponent for FYL2X.
  asm volatile("flds %3; flds %4; fxch %%st(1); fstpl %0; fstl %1; fstpt %2"
      :"=m"(converted_base),"=m"(converted_exponent),"=m"(extended_exponent):"m"(base),"m"(exponent):"st");
  load_control(std::uint16_t((controls.original&0x0300u)|0x007fu));
  const auto base_bits=std::bit_cast<std::uint64_t>(converted_base);
  auto report_error=[&](unsigned kind,double value){
    const double handled=source_pow_math_error_tu15_current(kind,value,controls.original,services);
    long double extended;asm volatile("fldl %1; fstpt %0":"=m"(extended):"m"(handled):"st");return extended;
  };
  long double result;
  if((base_bits&0x7ff0000000000000ull)==0x7ff0000000000000ull&&(base_bits&0x000fffffffffffffull)){
    result=report_error(1,converted_base);
  }else if((std::bit_cast<std::uint64_t>(converted_exponent)&0x7ff0000000000000ull)==0x7ff0000000000000ull||
      (base_bits&0x7ff0000000000000ull)==0x7ff0000000000000ull){
    const auto special=source_power_special_positive_tu15(base_bits,std::bit_cast<std::uint64_t>(converted_exponent));
    const auto value=std::bit_cast<double>(special.result_bits);
    if(special.report_domain_error)result=report_error(1,value);
    else asm volatile("fldl %1; fstpt %0":"=m"(result):"m"(value):"st");
  }else if((base_bits&0x7fffffffffffffffull)==0){
    // 3320C9 first loads the base, and 37CF55 discards it before parity.
    asm volatile("fldl %0; fstp %%st(0)"::"m"(converted_base):"st");
    const bool negative=(integer_parity(extended_exponent)==1)&&(base_bits>>63);
    asm volatile("fldz; fstpt %0":"=m"(result)::"st");
    if(negative)asm volatile("fldt %1; fchs; fstpt %0":"=m"(result):"m"(result):"st");
  }else{
    // DAZ can make COMISS treat a negative subnormal target as zero, so the
    // scalar's source "absolute" operation can pass its negative raw bits.
    // 37CFAC admits integer powers by parity and requires CRT domain reporting
    // for a fractional exponent; preserve that original path too.
    bool negate{},domain_error{};
    if(base_bits>>63){
      const auto parity=integer_parity(extended_exponent);
      domain_error=parity==0;
      negate=parity==1;converted_base=std::bit_cast<double>(base_bits&0x7fffffffffffffffull);
    }
    if(domain_error)result=report_error(1,std::bit_cast<double>(0xfff8000000000000ull));
    else {
    result=finite_power(converted_base,extended_exponent);
    if(negate)asm volatile("fldt %1; fchs; fstpt %0":"=m"(result):"m"(result):"st");
    double rounded;
    // Original 33216D checks the binary64 range without discarding the extended
    // result. Range reporting belongs to the actual unported CRT owner.
    asm volatile("fldt %1; fstl %0; fstp %%st(0)":"=m"(rounded):"m"(result):"st");
    const auto classification=std::bit_cast<std::uint64_t>(rounded)&0x7ff0000000000000ull;
    if(classification==0||classification==0x7ff0000000000000ull){
      const double scale=classification==0?1536.:-1536.;long double scaled;
      asm volatile("fldl %1; fldt %2; fscale; fstp %%st(1); fstpt %0":"=m"(scaled):"m"(scale),"m"(result):"st");
      const double boundary=std::bit_cast<double>(classification==0?0x0010000000000000ull:0x7fefffffffffffffull);
      std::uint16_t comparison;
      asm volatile("fldt %1; fabs; fcompl %2; fnstsw %0":"=am"(comparison):"m"(scaled),"m"(boundary):"st");
      if(classification==0?(comparison&0x0100u)!=0:(comparison&0x4100u)==0){
        const double factor=std::bit_cast<double>(classification==0?0ull:0x7ff0000000000000ull);
        asm volatile("fldt %1; fmull %2; fstpt %0":"=m"(scaled):"m"(scaled),"m"(factor):"st");
      }
      double error_value;asm volatile("fldt %1; fstpl %0":"=m"(error_value):"m"(scaled):"st");
      result=report_error(classification==0?4:3,error_value);
    }
    }
  }
  load_control(controls.original);
  const float acceleration=load_acceleration();
  float scaled;
  asm volatile("fldt %1; fmuls %2; fstps %0":"=m"(scaled):"m"(result),"m"(acceleration):"st");
  return scaled;
#else
  (void)base;(void)exponent;(void)load_acceleration;(void)services;
  throw std::runtime_error("TU1.5 lift power requires the native x87 numeric backend");
#endif
}
float source_lift_power_tu15_current(float base,float exponent,float acceleration,const SourceMathErrorServicesTu15& services){
  return source_lift_power_tu15_current_lazy(base,exponent,[&]{return acceleration;},services);
}
SourceLiftPowerOutcome source_lift_power_tu15(float base,float exponent,float acceleration,SourceLiftScalarEnvironment fp,const SourceMathErrorServicesTu15& services){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp);const float value=source_lift_power_tu15_current(base,exponent,acceleration,services);
  std::uint16_t status;asm volatile("fnstsw %0":"=am"(status));return {value,_mm_getcsr(),status};
#else
  (void)fp;return {source_lift_power_tu15_current(base,exponent,acceleration,services)};
#endif
}
}
