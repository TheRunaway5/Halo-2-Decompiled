#include "halo2/native_tu15_function_power_error.hpp"
#include <stdexcept>
namespace halo2 {
double native_tu15_authored_function_power_error_current(unsigned kind,double value,
    std::uint16_t caller_control,const SourceMathErrorServicesTu15&services,
    NativeTu15FunctionPowerErrorStats*stats){
  if(kind==1u||kind==3u||kind==4u){if(stats)++stats->delegated_errors;
    return source_pow_math_error_tu15_current(kind,value,caller_control,services);}
  if((caller_control&0x3fu)!=0x3fu||(kind!=2u&&kind!=8u)){
    if(stats)++stats->refused_profiles;throw std::invalid_argument("Native TU1.5 authored power error requires its remaining CRT profile");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(kind==2u){
    if(stats)++stats->pole_errors;
    // GAS AT&T FDIVP emits DE F1, the source Intel FDIVRP ST1,ST0: 1/0.
    asm volatile("fldz;fld1;fdivp %%st,%%st(1);fstp %%st(0)":::"memory","st","st(1)");
  }else{
    if(stats)++stats->precision_errors;double ignored;
    asm volatile("fldpi;fstpl %0":"=m"(ignored)::"memory","st");
  }
  asm volatile("fldcw %0"::"m"(caller_control):"memory");
  if(kind==2u){
    if(services.store_error_number)services.store_error_number(34);
    else if(services.error_number)*services.error_number=34;
    else throw SourceMathErrorServiceRequiredTu15(34);
  }
  return value;
#else
  (void)value;(void)services;throw std::runtime_error("Native TU1.5 authored power errors require native x87");
#endif
}
} // namespace halo2
