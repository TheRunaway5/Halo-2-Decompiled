#include "halo2/source_math_error_tu15.hpp"
#include <array>
#include <bit>

namespace halo2 {
SourceMathErrorServiceRequiredTu15::SourceMathErrorServiceRequiredTu15(std::int32_t value)
  :std::runtime_error("TU1.5 CRT math error requires native thread errno storage for original00330179"),error_number(value){}
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// Authenticated original46E640 and46E64C, native x87 extended operands used
// by32ED0D to raise actual arithmetic flags under the current controls.
constexpr std::array<unsigned char,10> huge{0,0,0,0,0,0,0,0x80,0x10,0x44};
constexpr std::array<unsigned char,10> tiny{1,0,0,0,0,0,0,0x80,0,0x30};
void raise_invalid(){std::int32_t ignored;asm volatile("fldt %1; fistpl %0":"=m"(ignored):"m"(huge):"st");}
void raise_overflow(){double ignored;asm volatile("fldt %1; fstpl %0":"=m"(ignored):"m"(huge):"st");}
void raise_underflow(){double ignored;asm volatile("fldt %1; fstpl %0":"=m"(ignored):"m"(tiny):"st");}
void raise_inexact(){double ignored;asm volatile("fldpi; fstpl %0":"=m"(ignored)::"st");}
void restore_control(std::uint16_t value){asm volatile("fldcw %0"::"m"(value):"memory");}
double underflow_value(double value){
  constexpr double zero=0.;std::uint16_t comparison;
  asm volatile("fldz; fldl %1; fucompp; fnstsw %0":"=am"(comparison):"m"(value):"st");
  if((comparison&0x4500u)==0x4000u)return value;
  const auto bits=std::bit_cast<std::uint64_t>(value);const bool negative=bits>>63;
  auto significand=bits&0x000fffffffffffffull;
  int exponent=int((bits>>52)&0x7ffu)-1022;
  if(((bits>>52)&0x7ffu)==0){
    exponent=-1021;
    while(!(significand&(1ull<<52))){significand<<=1;--exponent;}
  }else significand|=1ull<<52;
  exponent-=1536;
  double result;
  if(exponent<-1074){
    const double fraction=std::bit_cast<double>((std::uint64_t(negative)<<63)|0x3fe0000000000000ull|(significand&0x000fffffffffffffull));
    asm volatile("fldl %1; fmull %2; fstpl %0":"=m"(result):"m"(fraction),"m"(zero):"st");
  }else{
    if(exponent<-1021)significand>>=unsigned(-1021-exponent);
    const auto positive=std::bit_cast<double>(significand);
    if(negative)asm volatile("fldl %1; fchs; fstpl %0":"=m"(result):"m"(positive):"st");
    else asm volatile("fldl %1; fstpl %0":"=m"(result):"m"(positive):"st");
  }
  return result;
}
#endif
}
double source_pow_math_error_tu15_current(unsigned kind,double value,std::uint16_t caller_control,
    const SourceMathErrorServicesTu15& services){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if((caller_control&0x3fu)!=0x3fu)throw std::invalid_argument("TU1.5 native pow math errors require masked caller exceptions");
  if(kind==1)raise_invalid();
  else if(kind==3){
    raise_overflow();
    constexpr double zero=0.;asm volatile("fldl %0; fcompl %1"::"m"(value),"m"(zero):"st");
    const bool negative=std::bit_cast<std::uint64_t>(value)>>63;
    const unsigned rounding=(caller_control>>10)&3u;
    const bool finite=rounding==3||(rounding==1&&!negative)||(rounding==2&&negative);
    value=std::bit_cast<double>((std::uint64_t(negative)<<63)|(finite?0x7fefffffffffffffull:0x7ff0000000000000ull));
    raise_inexact();
  }else if(kind==4){
    value=underflow_value(value);raise_underflow();raise_inexact();
  }else throw std::invalid_argument("Unported TU1.5 native pow math-error record kind");
  //333595 restores the original caller control before its matherr/errno path.
  restore_control(caller_control);
  if(kind==1||kind==3){
    const std::int32_t error=kind==1?33:34;
    if(services.store_error_number)services.store_error_number(error);
    else {
      if(!services.error_number)throw SourceMathErrorServiceRequiredTu15(error);
      *services.error_number=error;
    }
  }
  return value;
#else
  (void)kind;(void)value;(void)caller_control;(void)services;
  throw std::runtime_error("TU1.5 native pow math errors require native x87");
#endif
}
}
