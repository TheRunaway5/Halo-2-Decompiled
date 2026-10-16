#include "halo2/mcc_effect_parameter_function.hpp"
#include <bit>
#include <cfenv>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2::tu15_effect_function {
namespace {
static_assert(sizeof(void*)==8&&sizeof(long double)==16,"The proved value lift requires native64 x86 x87 precision");
bool finite(std::uint32_t bits){return (bits&0x7fffffff)<0x7f800000;}
float value(std::uint32_t bits){return std::bit_cast<float>(bits);}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}
void require(bool accepted,const char*why){if(!accepted)throw FormatError(why);}
void control(std::uint16_t word){asm volatile("fldcw %0"::"m"(word):"memory");}
std::uint16_t control(){std::uint16_t word;asm volatile("fnstcw %0":"=m"(word));return word;}
struct Environment {
    // REX.W retains native64 x87 instruction/data pointers as well as the
    // condition/status/control fields; FTW==0 means all x87 registers empty.
    alignas(16) std::array<unsigned char,512> retained{};
    explicit Environment(ExplicitFp fp){
        require((fp.loaded_x87_control==0x023f||fp.loaded_x87_control==0x027f||fp.loaded_x87_control==0x037f)&&fp.mxcsr==0x1f80,"Unproved effect-function FP context");
        asm volatile("fxsave64 %0":"=m"(retained)::"memory");
        require(retained[4]==0,"Effect-function conversion requires an empty x87 stack");
        asm volatile("fnclex");control(fp.loaded_x87_control);_mm_setcsr(fp.mxcsr);
    }
    ~Environment(){
        asm volatile("fxrstor64 %0"::"m"(retained):"memory",
            "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)",
            "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7",
            "xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15");
    }
};
long double add(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fadd %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double mul(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fmul %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double div(long double x,long double y){long double z;asm volatile("fldt %2; fldt %1; fdiv %%st(1),%%st; fstpt %0; fstp %%st(0)":"=m"(z):"m"(x),"m"(y):"st","st(1)");return z;}
long double square_root(long double x){long double z;asm volatile("fldt %1; fsqrt; fstpt %0":"=m"(z):"m"(x):"st");return z;}
float store_float(long double x){float z;asm volatile("fldt %1; fstps %0":"=m"(z):"m"(x):"st");return z;}
float plus(float x,float y){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(x),_mm_set_ss(y)));}
float minus(float x,float y){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(x),_mm_set_ss(y)));}
float times(float x,float y){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(x),_mm_set_ss(y)));}
float divided(float x,float y){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(x),_mm_set_ss(y)));}
float clamp(float x){if(x<0)return 0;if(x>1)return 1;return x;}
void validate(const CubicParameterValues& f){
    require(f.input>=0&&f.input<7,"Selected effect input leaves the proved seven-word owner");
    if(f.flags&1)require(f.range_input>=0&&f.range_input<7,"Selected effect range leaves the proved seven-word owner");
    if(f.input==1||((f.flags&1)&&f.range_input==1))require(finite(f.period_bits)&&(f.period_bits&0x7fffffff),"Unproved effect-function time period");
    if(!(f.flags&0xf0))require(finite(f.lower_bits)&&finite(f.upper_bits),"Nonfinite effect-function scalar bounds are unresolved");
    for(unsigned i=0;i<((f.flags&1)?2u:1u);i++)for(auto word:{f.polynomials[i].cubic,f.polynomials[i].quadratic,f.polynomials[i].linear,f.polynomials[i].constant})require(finite(word),"Nonfinite active cubic coefficient is unresolved");
}
float time_fraction(float time,float period){
    const long double ratio=div(time,period);const auto before=control();control(0x133f);
    const long double one=1;long double remainder;std::uint16_t status;
    // Source37DC14 loops FPREM until C2 clears, then removes the divisor.
    asm volatile("fldt %3; fldt %2; 1: fprem; fnstsw %%ax; test $0x400,%%ax; jnz 1b; fstpt %0; fstp %%st(0)":"=m"(remainder),"=a"(status):"m"(ratio),"m"(one):"st","st(1)");
    control(before);return store_float(remainder);
}
float speaker_gain(const Inputs& input,std::int32_t speaker,const SpeakerGeometry& geometry){
    std::array<float,3> direction{value(input.words[4]),value(input.words[5]),value(input.words[6])};
    const auto xx=mul(direction[0],direction[0]),yy=mul(direction[1],direction[1]),zz=mul(direction[2],direction[2]);
    const long double length=square_root(add(add(xx,yy),zz));const float stored_length=store_float(length);
    //30BD0 compares the retained extended root, then uses its float spill.
    if(length>=value(0x38d1b717)){const auto reciprocal=divided(1,stored_length);for(auto&v:direction)v=times(v,reciprocal);}
    const auto& position=geometry.directions[std::size_t(speaker)];
    const float dy=times(minus(direction[1],value(position[1])),.5f),dz=times(minus(direction[2],value(position[2])),.5f),dx=times(minus(direction[0],value(position[0])),.5f);
    return clamp(minus(1,clamp(plus(plus(times(dz,dz),times(dy,dy)),times(dx,dx)))));
}
float input_value(const CubicParameterValues& f,const Inputs& input,std::int16_t index,std::int32_t speaker,const SpeakerGeometry* geometry){
    float x=value(input.words[std::size_t(index)]);
    if(index==1)x=time_fraction(x,value(f.period_bits));
    if(index==3&&speaker!=-1&&speaker>=0&&speaker<=4)x=times(x,speaker_gain(input,speaker,*geometry));
    return x;
}
float polynomial(const CubicPolynomial&p,float x){
    const float squared=times(x,x),cubed=times(squared,x);
    return plus(plus(plus(times(value(p.quadratic),squared),times(value(p.linear),x)),times(value(p.cubic),cubed)),value(p.constant));
}
}
Values evaluate_cubic_unbounded_values(const CubicParameterValues& function,const Inputs& input,std::int32_t speaker,const SpeakerGeometry* geometry,ExplicitFp fp){
    validate(function);require(finite(input.words[std::size_t(function.input)]),"Nonfinite selected effect input is unresolved");
    if(function.flags&1)require(finite(input.words[std::size_t(function.range_input)]),"Nonfinite selected effect range is unresolved");
    const bool panned=(function.input==3||((function.flags&1)&&function.range_input==3))&&speaker>=0&&speaker<=4;
    if(panned){require(geometry,"Effect speaker geometry has no actual typed owner");for(unsigned i=4;i<7;i++)require(finite(input.words[i]),"Nonfinite effect direction is unresolved");for(auto word:geometry->directions[std::size_t(speaker)])require(finite(word),"Nonfinite source speaker direction is unresolved");}
    Environment environment(fp);
    const auto x=input_value(function,input,function.input,speaker,geometry);float fraction=polynomial(function.polynomials[0],x);
    if(function.flags&1){const auto range=input_value(function,input,function.range_input,speaker,geometry);const auto high=polynomial(function.polynomials[1],x);fraction=plus(times(high,range),times(minus(1,range),fraction));}
    fraction=clamp(fraction);float output=fraction;
    if(!(function.flags&0xf0))output=plus(times(minus(value(function.upper_bits),value(function.lower_bits)),clamp(fraction)),value(function.lower_bits));
    return {bits(fraction),bits(output)};
}
Values evaluate_cubic_values(const CubicParameterValues& function,const Inputs& input,std::int32_t speaker,const SpeakerGeometry* geometry,ExplicitFp fp){
    const auto result=evaluate_cubic_unbounded_values(function,input,speaker,geometry,fp);
    require(finite(result.fraction_bits)&&finite(result.value_bits),"Nonfinite effect-function arithmetic is unresolved");return result;
}
CompiledParameterFunction::CompiledParameterFunction(std::shared_ptr<const MccSoundEffectDefinitions> bank,const MccSoundEffectDefinitions::Function* source,CubicParameterValues values):definitions_(std::move(bank)),source_(source),values_(values){}
std::shared_ptr<const CompiledParameterFunction> CompiledParameterFunction::compile(std::shared_ptr<const MccSoundEffectDefinitions> bank,const MccSoundEffectDefinitions::Function& source){
    require(bool(bank),"Effect-function compilation needs retained MCC definitions");bool owned=false;
    for(const auto&collection:bank->collections())for(const auto&effect:collection.effects)for(const auto&instance:effect.instances)for(const auto&root:instance.override_records)for(const auto&component:root.components)for(const auto&candidate:component.functions)if(&candidate==&source)owned=true;
    require(owned,"Selected function is not owned by this exact MCC definition bank");
    const Bytes raw(source.curve.payload.bytes);require(source.curve.size==raw.size()&&source.curve.payload.cache_owner&&source.curve.size>=68,"Selected cubic curve is missing/truncated");
    require(std::to_integer<unsigned>(source.curve.payload.bytes[0])==7,"Selected effect-function algorithm is unresolved");const auto flags=std::to_integer<std::uint8_t>(source.curve.payload.bytes[1]);
    require(!(flags&1)||source.curve.size>=116,"Ranged cubic curve is truncated");
    CubicParameterValues values{source.input,source.range_input,source.time_period_bits,raw.u32(4),raw.u32(8),flags,{}};
    for(unsigned i=0;i<((flags&1)?2u:1u);i++){const auto at=0x34+i*0x30;values.polynomials[i]={raw.u32(at),raw.u32(at+4),raw.u32(at+8),raw.u32(at+12)};}
    validate(values);return std::shared_ptr<const CompiledParameterFunction>(new CompiledParameterFunction(std::move(bank),&source,values));
}
EvaluatedParameter CompiledParameterFunction::evaluate(const Inputs& inputs,std::int32_t speaker,const SpeakerGeometry* geometry,ExplicitFp fp)const{return {evaluate_cubic_values(values_,inputs,speaker,geometry,fp),shared_from_this()};}
EvaluatedParameter CompiledParameterFunction::evaluate_unbounded(const Inputs& inputs,std::int32_t speaker,const SpeakerGeometry* geometry,ExplicitFp fp)const{return {evaluate_cubic_unbounded_values(values_,inputs,speaker,geometry,fp),shared_from_this()};}
}
