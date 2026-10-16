#include "halo2/native_tu15_sound_mono_parameters.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <xmmintrin.h>
namespace halo2::tu15_mono_parameter_conversion {
namespace {
static_assert(sizeof(void*)==8&&sizeof(long double)==16,"This private x87 lift requires native64 x86 extended precision");
constexpr double sample_period=std::bit_cast<double>(UINT64_C(0x3ef5d867c3ece2a5));
constexpr double two_pi=std::bit_cast<double>(UINT64_C(0x401921fb54442d18));
constexpr double db_exponent=std::bit_cast<double>(UINT64_C(0x3f9999999999999a));
void control(std::uint16_t value){asm volatile("fldcw %0"::"m"(value):"memory");}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
struct Environment {
    std::fenv_t saved;
    explicit Environment(FpMode mode){
        if(mode!=FpMode::source_pretable_53&&mode!=FpMode::double_53&&mode!=FpMode::extended_64)throw std::invalid_argument("Unproved parameter-conversion FP mode");
        alignas(16) std::array<unsigned char,32> x87_environment{};
        asm volatile("fnstenv %0; fldenv %0":"=m"(x87_environment)::"memory");
        std::uint16_t tags;std::memcpy(&tags,x87_environment.data()+8,sizeof(tags));
        if(tags!=0xffff)throw std::invalid_argument("Parameter conversion requires an empty incoming x87 stack");
        if(std::fegetenv(&saved)!=0)throw std::runtime_error("Cannot retain native FP environment");
        asm volatile("fnclex");control(static_cast<std::uint16_t>(mode));_mm_setcsr(0x1f80);
    }
    ~Environment(){std::fesetenv(&saved);}
};
long double round_integer(long double x){long double y;asm volatile("fldt %1; frndint; fstpt %0":"=m"(y):"m"(x):"st");return y;}
long double add(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fadd %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double sub(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fsub %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double mul(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fmul %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double div(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fdiv %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
double store_double(long double x){double y;asm volatile("fldt %1; fstpl %0":"=m"(y):"m"(x):"st");return y;}
long double fraction_exp2(long double x){long double y;asm volatile("fldt %1; f2xm1; fstpt %0":"=m"(y):"m"(x):"st");return y;}
long double scale2(long double x,long double n){long double y;asm volatile("fldt %2; fldt %1; fscale; fstpt %0; fstp %%st(0)":"=m"(y):"m"(x),"m"(n):"st","st(1)");return y;}
//37CE59/332084 on the source-proven positive finite base10 branch.
double power_ten(long double exponent){
    const auto before=control();control(static_cast<std::uint16_t>((before&0x300)|0x7f));
    const long double base=10;long double product;
    asm volatile("fldt %2; fldt %1; fyl2x; fstpt %0":"=m"(product):"m"(base),"m"(exponent):"st","st(1)");
    const long double n=round_integer(product);const long double fraction=sub(product,n);
    const long double u=fraction_exp2(fraction);const long double significand=add(u,1);
    const long double scaled=scale2(significand,n);const double result=store_double(scaled);
    control(before);return result;
}
//Positive finite sinh source37D370/418/45B, after331E64 installs133F.
//Its small-integer branch preserves expm1 rather than subtracting nearby1s.
long double positive_sinh(long double x){
    const auto before=control();control(0x133f);long double product;
    asm volatile("fldt %1; fldl2e; fmulp; fstpt %0":"=m"(product):"m"(x):"st","st(1)");
    const long double n=round_integer(product);const long double fraction=sub(product,n);
    long double u=fraction_exp2(std::fabs(fraction));
    if(fraction<0){const long double denominator=-add(u,1);u=div(u,denominator);}
    long double delta;
    if(n!=0){const long double significand=add(u,1);const long double exponential=scale2(significand,n);const long double reciprocal=div(1,exponential);delta=sub(exponential,reciprocal);}
    else{const long double denominator=-add(u,1);const long double negative_reciprocal_delta=div(u,denominator);delta=sub(u,negative_reciprocal_delta);}
    const long double result=scale2(delta,-1);control(before);return result;
}
long double source_sin(double x){long double result;asm volatile("fldl %1; fsin; fstpt %0":"=m"(result):"m"(x):"st");return result;}
long double source_cos(double x){long double result;asm volatile("fldl %1; fcos; fstpt %0":"=m"(result):"m"(x):"st");return result;}
Word24 quantize(double x){
    if(x>=1)return Word24(0x7fffff);
    if(x<=-1)return Word24(0x800000);
    if(x>=0){const long double scaled=mul(x,8388608);return Word24(static_cast<std::uint32_t>(scaled)&0xffffff);}
    const long double scaled=mul(x,-8388608);
    return Word24(0xffffff-static_cast<std::uint32_t>(scaled));
}
tu15_sound_mono_distortion::FilterCoefficients filter(float frequency,float decibels,float q){
    const long double exponent=mul(decibels,db_exponent);
    const double amplitude=power_ten(exponent);
    const long double phase1=mul(frequency,sample_period);
    const double omega=store_double(mul(phase1,two_pi));
    const long double twice_q=add(q,q);
    const long double inverse_q=div(1,twice_q);
    const long double hyperbolic=positive_sinh(inverse_q);
    const long double alpha=mul(hyperbolic,source_sin(omega));
    const double beta=store_double(mul(source_cos(omega),-2));
    const long double alpha_div_amplitude=div(alpha,amplitude);
    const double saved_div=store_double(alpha_div_amplitude);
    const long double denominator=add(alpha_div_amplitude,1);
    const double norm=store_double(div(0.5L,denominator));
    const long double alpha_times_amplitude=mul(alpha,amplitude);
    const double saved_product=store_double(alpha_times_amplitude);
    const long double first_sum=add(alpha_times_amplitude,1);
    const double b0=store_double(mul(first_sum,norm));
    const double b1=store_double(mul(norm,beta));
    const double b2=store_double(mul(sub(1,saved_product),norm));
    const double a1=-b1;
    const double a2=store_double(-mul(sub(1,saved_div),norm));
    return {quantize(b0),quantize(b1),quantize(b2),quantize(a1),quantize(a2)};
}
}
Word24 quantize_finite_fraction(double value,FpMode mode){
    Environment environment(mode);
    if(!std::isfinite(value))throw std::invalid_argument("Nonfinite source quantization is unresolved");
    return quantize(value);
}
Parameters convert_bounded_values(const std::array<std::uint32_t,7>& bits,FpMode mode){
    Environment environment(mode);
    std::array<float,7> v;for(unsigned i=0;i<7;i++){v[i]=std::bit_cast<float>(bits[i]);if(!std::isfinite(v[i]))throw std::invalid_argument("Nonfinite source conversion is unresolved");}
    if(v[0]<0||v[0]>1||v[1]<0||v[1]>20000||v[4]<0||v[4]>20000||v[2]<-30||v[2]>30||v[5]<-30||v[5]>30||v[3]<0.001f||v[3]>30||v[6]<0.001f||v[6]>30)throw std::invalid_argument("Parameter outside the proved original/MCC mono template bounds");
    return {quantize(v[0]),filter(v[1],v[2],v[3]),filter(v[4],v[5],v[6])};
}
}
